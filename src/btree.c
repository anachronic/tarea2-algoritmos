#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "btree.h"
#include "parametros.h"
#include "extmem.h"
#include "fallos.h"

void btree_new(char *archivo) {
  FILE *f;
  int raiz;

  f = fopen(archivo, "wb");

  // marcar que es un arbol nuevo (raiz es -1)
  raiz = -1;

  fwrite(&raiz, sizeof(int), 1, f);

  fclose(f);
}

static int _get_raiz(const char *archivo) {
  FILE *f;
  int raiz;

  if ((f = fopen(archivo, "rb")) == NULL)
    fallar("Error al abrir en B-Tree");

  fread(&raiz, sizeof(int), 1, f);
  fclose(f);
  return raiz;
}

static void _set_raiz(const char *archivo, int indice) {
  FILE *f;

  if ((f = fopen(archivo, "rb+")) == NULL)
    fallar("Error al abrir en B-Tree");

  fseek(f, 0, SEEK_SET);
  fwrite(&indice, sizeof(int), 1, f);
  fflush(f);
  fclose(f);
}

static void _volcar_memext(const char *btree, struct btree_nodo *nodo, int indice_nodo) {
  char *buf;

  buf = serializar_nodo(nodo);
  set_bloque(btree, buf, indice_nodo, sizeof(int));
  free(buf);
  btree_nodo_dispose(nodo);
}


static int _btree_search(const char *btree, const char *cadena, int indice) {
  char *serializado;
  struct btree_nodo *nodo;
  int k;
  int indice_hijo;

  if (indice == -1) {
    return 0; // NOT FOUND
  }

  serializado = recuperar_bloque(btree, indice, sizeof(int));
  nodo = deserializar_nodo(serializado);
  free(serializado);

  k = 0;
  // se puede usar bsearch para esto. queda propuesto xD
  // http://www.cplusplus.com/reference/cstdlib/bsearch/
  while (k < nodo->num_elems && strcmp(cadena, nodo->elementos[k]) > 0) k++;

  if (k < nodo->num_elems && strcmp(cadena, nodo->elementos[k]) == 0) {
    btree_nodo_dispose(nodo);
    return 1;
  }
  if (nodo->num_hijos <= 0) {
    btree_nodo_dispose(nodo);
    return 0;
  }

  indice_hijo = nodo->hijos[k];
  btree_nodo_dispose(nodo);
  return _btree_search(btree, cadena, indice_hijo);
}


int btree_search(const char *btree, const char *cadena) {
  int raiz = _get_raiz(btree);

  if (raiz < 0) {
    return -1;
  }

  return _btree_search(btree, cadena, raiz);
}

static void _btree_insertar_elemento(struct btree_nodo *btree, const char *clave, int posicion) {
  if (posicion > btree->num_elems) {
    fprintf(stderr, "error de consistencia en nodo del b-tree\n");
    exit(-1);
  }

  if (posicion < btree->num_elems) {
    memmove(btree->elementos + (posicion + 1), btree->elementos + posicion,
            sizeof(char *) * (btree->num_elems - posicion));
  }

  btree->elementos[posicion] = (char *) malloc(TAMANO_CADENA);
  strcpy(btree->elementos[posicion], clave);
  btree->num_elems++;
}

static int _encontrar_candidato(struct btree_nodo *b, const char *clave) {
  int k = 0;
  if (b->num_elems == 0) return 0;

  while (k < b->num_elems && strcmp(clave, b->elementos[k]) > 0) k++;
  if (k < b->num_elems && strcmp(clave, b->elementos[k]) == 0) return -1;
  return k;
}

static void _btree_handle_overflow(const char *btree, struct btree_nodo *b, struct btree_nodo *hijo) {
  int medio;
  int old_elems;
  int indice_elemento_candidato;
  int k;
  struct btree_nodo *nuevo_derecho;
  char *buf;

  medio = (int) BTREE_ELEMS_NODO / 2;
  indice_elemento_candidato = _encontrar_candidato(b, hijo->elementos[medio]);

  // creamos un nuevo nodo hijo.
  nuevo_derecho = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));

  nuevo_derecho->elementos = (char **) malloc(sizeof(char *) * BTREE_ELEMS_NODO);
  nuevo_derecho->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 1));
  nuevo_derecho->num_elems = medio;
  nuevo_derecho->num_hijos = 0; // puede ser que cambie
  nuevo_derecho->indice = last_indice(btree);

  // copiamos los elementos "del lado derecho" al nuevo nodo.
  for (k = 0; k < nuevo_derecho->num_elems; k++) {
    nuevo_derecho->elementos[k] = (char *) malloc(sizeof(char) * TAMANO_CADENA);
    strcpy(nuevo_derecho->elementos[k], hijo->elementos[nuevo_derecho->num_elems + k + 1]);
    free(hijo->elementos[nuevo_derecho->num_elems + k + 1]);
  }
//  memcpy(nuevo_derecho->elementos, hijo->elementos + nuevo_derecho->num_elems + 1,
//         sizeof(char*) * nuevo_derecho->num_elems);

  old_elems = hijo->num_elems;
  hijo->num_elems = medio;

  // insertar el elemento del medio del hijo en el padre
  _btree_insertar_elemento(b, hijo->elementos[medio], indice_elemento_candidato);
  free(hijo->elementos[medio]);

  // antes de colgar, si el hijo tiene hijos hay que arreglarlos
  if (hijo->num_hijos > 0) {
    // los hijos izquierdos quedaran bien puestos al haber reducido hijo->num_elems
    // pero los hijos derechos debemos recolgarlos al nuevo nodo y re-setearle
    // los indices de sus hijos
    nuevo_derecho->num_hijos = nuevo_derecho->num_elems + 1;
    hijo->num_hijos = hijo->num_elems + 1;
    if (old_elems - medio + 1 > 0) {
      int offset = 0;
      if(nuevo_derecho->num_hijos % 2 == 0) offset = 1;
      memcpy(nuevo_derecho->hijos, hijo->hijos + medio + 1, sizeof(int) * (old_elems - medio + offset));
    }
  }

  // mover los hijos que no conciernen con el nuevo elemento insertado.
  if (b->num_hijos - indice_elemento_candidato - 1 > 0 && indice_elemento_candidato < BTREE_ELEMS_NODO - 1) {
    memmove(b->hijos + (indice_elemento_candidato + 2), b->hijos + (indice_elemento_candidato + 1),
            sizeof(int) * (b->num_hijos - indice_elemento_candidato - 1));
  }
  b->num_hijos = b->num_elems + 1;
  b->hijos[indice_elemento_candidato] = hijo->indice;
  b->hijos[indice_elemento_candidato + 1] = nuevo_derecho->indice;

  // guardar SI O SI el elemento nuevo en memoria
  buf = serializar_nodo(nuevo_derecho);
  append_bloque(btree, buf);
  free(buf);
  btree_nodo_dispose(nuevo_derecho);
}

// retorna el nodo (en MEMORIA PRINCIPAL) que tiene overflow. Dicho nodo está en
// el HEAP. De manera que al mandarlo a memoria secundaria debe hacérsele FREE.
// De no haber overflow en un nodo esta func. retorna NULL
static struct btree_nodo *_btree_insertar(const char *btree, const char *cadena, int nodo) {
  int indice_hijo_candidato;
  int indice_elemento;
  struct btree_nodo *hijo;
  struct btree_nodo *padre; // el nodo-esimo nodo en btree.
  char *buf;

  buf = recuperar_bloque(btree, nodo, sizeof(int));
  padre = deserializar_nodo(buf);
  free(buf);

  // este es el procedimiento para insertar el elemento en una HOJA
  if (padre->num_hijos == 0) {
    indice_elemento = _encontrar_candidato(padre, cadena);

    if (indice_elemento >= 0) {
      _btree_insertar_elemento(padre, cadena, indice_elemento);
    }

    // si hay overflow mi padre "me saca" mi overflow y él es responsable de mandarme a disco
    if (padre->num_elems > BTREE_ELEMS_NODO) return padre;
    else {
      // pero si no lo hay, yo mismo me mando a disco.
      _volcar_memext(btree, padre, nodo);
      return NULL;
    }
  }

  // si este nodo es interno, busco el camino por donde bajar e inserto recursivamente ahí
  indice_hijo_candidato = _encontrar_candidato(padre, cadena);
  if (indice_hijo_candidato >= 0)
    hijo = _btree_insertar(btree, cadena, padre->hijos[indice_hijo_candidato]);
  else {
    _volcar_memext(btree, padre, nodo);
    return NULL;
  }

  // si no hay overflow, serializo este nodo (correspondiente al "padre")
  // lo mando a disco
  // y retorno que yo "no tengo overflow"
  if (hijo == NULL) {
    _volcar_memext(btree, padre, nodo);
    return NULL;
  }

  _btree_handle_overflow(btree, padre, hijo);

  // hasta ahora mi "hijo" no tiene overflow, así que lo guardo en memoria secundaria
  _volcar_memext(btree, hijo, hijo->indice);

  // si después de esto yo (el padre) no tengo overflow, también me mando a mí mismo a
  // memoria secundaria y retorno null
  if (padre->num_elems <= BTREE_ELEMS_NODO) {
    _volcar_memext(btree, padre, nodo);
    return NULL;
  }

  // pero si tengo overflow, mi padre (ie: el "abuelo") será responsable de mandarme
  // a memoria secundaria, porque en principal tengo un elemento más.
  return padre;
}

void btree_insertar(const char *btree, const char *cadena) {
  int raiz;
  struct btree_nodo *nodo;
  struct btree_nodo *hijo;
  char *clave;
  char *buffer;

  raiz = _get_raiz(btree);
  // Caso base: B-Tree vacío.
  if (raiz < 0) {
    clave = strdup(cadena);
    nodo = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));
    nodo->num_hijos = 0;
    nodo->num_elems = 1;
    nodo->indice = 0;
    nodo->elementos = (char **) malloc(sizeof(char *) * BTREE_ELEMS_NODO);
    nodo->elementos[0] = clave;
    nodo->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 1));

    buffer = serializar_nodo(nodo);
    append_bloque(btree, buffer);

    free(buffer);
    btree_nodo_dispose(nodo);
    _set_raiz(btree, 0);
    return;
  }

  hijo = _btree_insertar(btree, cadena, raiz);

  // si hijo es NULL significa que el elemento quedó correctamente insertado
  // en algun nodo interno u hoja que NO ES LA RAÍZ y por lo tanto no hay nada
  // que hacer
  if (hijo == NULL) return;

  // pero si hijo es != NULL tengo overflow en la raíz y hay que crear
  // una nueva raíz con un elemento :O
  nodo = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));

  // la raiz siempre tendra de hijo al nodo que tiene overflow
  // y al hacer _btree_handle_overflow
  // tendremos el otro hijo creado.
  nodo->num_hijos = 0;
  nodo->num_elems = 0;
  nodo->elementos = (char **) malloc(sizeof(char *) * BTREE_ELEMS_NODO);
  nodo->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 1));

  _btree_handle_overflow(btree, nodo, hijo);

  // ahora sí que podemos volcar los nodos a memoria secundaria y terminar
  nodo->indice = last_indice(btree);
  _set_raiz(btree, nodo->indice);
  _volcar_memext(btree, hijo, hijo->indice);

  buffer = serializar_nodo(nodo);
  append_bloque(btree, buffer);
  free(buffer);
  btree_nodo_dispose(nodo);
}


void btree_dispose(const char *archivo) {
  if (remove(archivo) != 0)
    perror("Error al eliminar el B-Tree");
}


char *serializar_nodo(struct btree_nodo *b) {
  char *buffer;
  int num_elems;
  int k;

  buffer = (char *) calloc(B, sizeof(char)); // inicializamos todos los bits a 0 (en partic. los elementos).
  num_elems = b->num_elems;

  memcpy(buffer, &num_elems, sizeof(int));
  memcpy(buffer + sizeof(int), &b->num_hijos, sizeof(int));
  memcpy(buffer + 2 * sizeof(int), &b->indice, sizeof(int));
  for (k = 0; k < num_elems; k++) {
    memcpy(buffer + 3 * sizeof(int) + k * TAMANO_CADENA * sizeof(char), b->elementos[k], TAMANO_CADENA);
  }

  // hasta aquí tenemos la cant. de elementos, todas las palabras y sólo faltan los hijos
  // ie: tenemos 12 + BTREE_ELEMS_NODO * 16 bytes.
  memcpy(buffer + 3 * sizeof(int) + BTREE_ELEMS_NODO * TAMANO_CADENA * sizeof(char),
         b->hijos, sizeof(int) * (BTREE_ELEMS_NODO + 1));

  // buffer listo
  return buffer;
}

struct btree_nodo *deserializar_nodo(char *buffer) {
  struct btree_nodo *nodo;
  int k;

  nodo = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));

  memcpy(&nodo->num_elems, buffer, sizeof(int));
  memcpy(&nodo->num_hijos, buffer + sizeof(int), sizeof(int));
  memcpy(&nodo->indice, buffer + 2 * sizeof(int), sizeof(int));

  nodo->elementos = (char **) malloc((BTREE_ELEMS_NODO + 1) * sizeof(char *));
  for (k = 0; k < nodo->num_elems; k++) {
    nodo->elementos[k] = (char *) malloc(sizeof(char) * TAMANO_CADENA);
    memcpy(nodo->elementos[k], buffer + 3 * sizeof(int) + k * TAMANO_CADENA * sizeof(char), TAMANO_CADENA);
  }

  // finalmente, copiamos los indices de los hijos.
  nodo->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 2));
  memcpy(nodo->hijos, buffer + 3 * sizeof(int) + BTREE_ELEMS_NODO * TAMANO_CADENA * sizeof(char),
         sizeof(int) * (BTREE_ELEMS_NODO + 1));

  return nodo;
}

void btree_nodo_dispose(struct btree_nodo *b) {
  int k;

  for (k = 0; k < b->num_elems; k++) {
    free(b->elementos[k]);
  }

  free(b->elementos);
  free(b->hijos);
  free(b);
}




















