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

  if (fwrite(&raiz, sizeof(int), 1, f) != sizeof(int))
    fallar("No se ha podido inicializar el diccionario. Abortando");

  fclose(f);
}

static int _get_raiz(char *archivo) {
  FILE *f;
  int raiz;

  if ((f = fopen(archivo, "rb")) == NULL)
    fallar("Error al abrir en B-Tree");

  fread(&raiz, sizeof(int), 1, f);
  fclose(f);
  return raiz;
}

static void _volcar_memext(char *btree, struct btree_nodo *nodo, int indice_nodo){
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

  if (strcmp(cadena, nodo->elementos[k]) == 0){
    btree_nodo_dispose(nodo);
    return 1;
  }
  if (nodo->num_hijos <= 0){
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

static void _btree_insertar_elemento(struct btree_nodo *btree, char *clave, int posicion) {
  char *cadena;
  if (posicion > btree->num_elems) {
    fprintf(stderr, "error de consistencia en nodo del b-tree\n");
    exit(-1);
  }

  cadena = strdup(clave);

  if (posicion < btree->num_elems) {
    memmove(btree->elementos + posicion, btree->elementos + (posicion + 1),
            sizeof(char *) * (btree->num_elems - posicion));
  }

  btree->elementos[posicion] = clave;
  btree->num_elems++;
}

static int _encontrar_candidato(struct btree_nodo *b, char *clave) {
  int k = 0;
  if (b->num_elems == 0) return 0;

  while (k < b->num_elems && strcmp(clave, b->elementos[k]) > 0) k++;
  return k;
}

static void _btree_handle_overflow(char *btree, struct btree_nodo *b, struct btree_nodo *hijo) {
  int medio;
  int old_elems;
  int indice_elemento_candidato;
  int k;
  char *temp;
  struct btree_nodo *nuevo_derecho;

  medio = (int) BTREE_ELEMS_NODO / 2;

  // creamos un nuevo nodo hijo.
  nuevo_derecho = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));

  nuevo_derecho->elementos = (char**)malloc(sizeof(char*) * BTREE_ELEMS_NODO);
  nuevo_derecho->hijos = (int*)malloc(sizeof(int) * (BTREE_ELEMS_NODO + 1));
  nuevo_derecho->num_elems = hijo->max_elems - medio;
  nuevo_derecho->num_hijos = 0; // puede ser que cambie
  nuevo_derecho->indice = last_indice(btree);

  // copiamos los elementos "del lado derecho" al nuevo nodo.
  // en memoria principal ambas estructuras quedan linkeadas
  // pero en memoria secundaria cada una queda con sus propias
  // copias de las cadenas
  memcpy(nuevo_derecho->elementos, hijo->elementos + nuevo_derecho->num_elems,
         sizeof(char*) * nuevo_derecho->num_elems);

  old_elems = hijo->num_elems;
  hijo->num_elems = medio;

  // insertar el elemento del medio del hijo en el padre
  indice_elemento_candidato = _encontrar_candidato(b, hijo->elementos[medio]);
  _btree_insertar_elemento(b, hijo->elementos[medio], indice_elemento_candidato);

  // antes de colgar, si el hijo tiene hijos hay que arreglarlos
  if (hijo->num_hijos > 0) {
    // los hijos izquierdos quedaran bien puestos al haber reducido hijo->num_elems
    // pero los hijos derechos debemos recolgarlos al nuevo nodo y re-setearle
    // los indices de sus hijos
    nuevo_derecho->hijos = nuevo_derecho->num_elems + 1;
    memcpy(nuevo_derecho->hijos, hijo->hijos + medio + 1, sizeof(int) * (old_elems - medio));
  }

  b->hijos[indice_elemento_candidato] = hijo->indice;
  b->hijos[indice_elemento_candidato + 1] = nuevo_derecho->indice;

  // guardar SI O SI el elemento nuevo en memoria
  _volcar_memext(btree, nuevo_derecho, nuevo_derecho->indice);
}

// retorna el nodo (en MEMORIA PRINCIPAL) que tiene overflow. Dicho nodo está en
// el HEAP. De manera que al mandarlo a memoria secundaria debe hacérsele FREE.
// De no haber overflow en un nodo esta func. retorna NULL
static struct btree_nodo *_btree_insertar(char *btree, char *cadena, int nodo) {
  int indice_hijo_candidato;
  int indice_elemento;
  int retorno;
  struct btree_nodo *hijo;
  struct btree_nodo *padre; // el nodo-esimo nodo en btree.
  char *buf;

  buf = recuperar_bloque(btree, nodo, 2 * sizeof(int));
  padre = deserializar_nodo(buf);
  free(buf);

  // este es el procedimiento para insertar el elemento en una HOJA
  if (padre->num_hijos == 0) {
    indice_elemento = _encontrar_candidato(padre, cadena);
    _btree_insertar_elemento(padre, cadena, indice_elemento);

    // si hay overflow mi padre "me saca" mi overflow y él es responsable de mandarme a disco
    if (padre->num_elems > BTREE_ELEMS_NODO) return padre;
    else {
      // pero si no lo hay, yo mismo me mando a disco.
      _volcar_memext(btree, padre, nodo);
      return NULL;
    }
  }

  // si este nodo es interno, busco el camino por donde bajar e inserto recursivamente ahí
  indice_hijo_candidato = _encontrar_candidato(candidato, clave);
  retorno = _btree_insertar(btree, clave, indice_hijo_candidato);

  // si no hay overflow, serializo este nodo (correspondiente al "padre")
  // lo mando a disco
  // y retorno que yo "no tengo overflow"
  if (retorno == NULL) {
    _volcar_memext(btree, padre, nodo);
    return NULL;
  }

  // ahora viene lo complicado.
  // si hay overflow en mi hijo, obtengo un nodo en memoria principal para él
  // le saco su overflow y traspaso su potencial overflow a mí (el padre)
  // la responsabilidad de sacarme mi potencial overflow se la delego a mi padre.
  buf = recuperar_bloque(btree, indice_hijo_candidato, 2* sizeof(int));
  hijo = deserializar_nodo(buf);
  free(buf);

  _btree_handle_overflow(candidato, hijo);

  // hasta ahora mi "hijo" no tiene overflow, así que lo guardo en memoria secundaria
  _volcar_memext(btree, hijo, indice_hijo_candidato);

  // si después de esto yo (el padre) no tengo overflow, también me mando a mí mismo a
  // memoria secundaria y retorno null
  if(padre->num_elems <= BTREE_ELEMS_NODO){
    _volcar_memext(btree, padre, nodo);
    return NULL;
  }

  // pero si tengo overflow, mi padre (ie: el "abuelo") será responsable de mandarme
  // a memoria secundaria, porque en principal tengo un elemento más.
  return padre;
}

void btree_insertar(const char *btree, const char *cadena) {
  int raiz;
  int retorno;
  struct btree_nodo *nodo;
  char *clave;
  char *buffer;

  raiz = _get_raiz(btree);
  // Caso base: B-Tree vacío.
  if (raiz < 0) {
    clave = strdup(cadena);
    nodo = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));
    nodo->num_hijos = 0;
    nodo->num_elems = 1;
    nodo->elementos = (char **) malloc(sizeof(char *) * BTREE_ELEMS_NODO);
    nodo->elementos[0] = clave;
    nodo->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 1));

    buffer = serializar_nodo(nodo);
    append_bloque(btree, buffer);

    free(clave);
    free(buffer);
    btree_nodo_dispose(nodo);
    return;
  }

  retorno = _btree_insertar(btree, cadena, raiz);
  if (retorno == 0) {
    // no hay overflow en la raiz.
    /*
     * ALGORITMO:
     *
     * Ahora _btree_insertar retorna un struct que tiene
     * un puntero al nodo (padre) y si tiene overflow. En este caso
     * retorno tiene valor 0, de manera que serializamos el nodo que
     * entrega el resultado y lo ponemos en memoria secundaria.
     */
  }


}

struct btree_nodo *btree_insertar(struct btree_nodo *btree, int clave) {
  int retorno, k;
  struct btree_nodo *nueva_raiz;

  // caso base: B-Tree vacío
  if (btree->num_elems == 0) {
    _btree_insertar_elemento(btree, clave, 0);
    return btree;
  }

  retorno = _btree_insertar(btree, clave);
  if (retorno == 0) return btree;

  // a partir de este momento hay overflow en la raíz.
  nueva_raiz = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));
  btree_nodo_new(nueva_raiz);

  // esto siempre retorna 0, asi que chao con el valor de retorno.
  _btree_handle_overflow(nueva_raiz, btree);

  return nueva_raiz;
}


void btree_dispose(char *archivo) {
  if (remove(archivo) != 0)
    perror("Error al eliminar el B-Tree");
}



char *serializar_nodo(struct btree_nodo *b) {
  char *buffer;
  int num_elems;
  int elems_zero;
  int k;

  buffer = (char *) calloc(B, sizeof(char)); // inicializamos todos los bits a 0 (en partic. los elementos).
  num_elems = b->num_elems;

  memcpy(buffer, &num_elems, sizeof(int));
  memcpy(buffer + sizeof(int), &b->num_hijos, sizeof(int));
  memcpy(buffer + 2* sizeof(int), &b->indice, sizeof(int));
  for (k = 0; k < num_elems; k++) {
    memcpy(buffer + 3 * sizeof(int) + k * TAMANO_CADENA * sizeof(char), b->elementos[k], TAMANO_CADENA);
  }

  // hasta aquí tenemos la cant. de elementos, todas las palabras y sólo faltan los hijos
  // ie: tenemos 4 + BTREE_ELEMS_NODO * 16 bytes.
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
  memcpy(&nodo->indice, buffer + 2* sizeof(int), sizeof(int));

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




















