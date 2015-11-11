#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "btree.h"
#include "parametros.h"

void btree_new(char *archivo) {
  FILE *f;
  char *buffer;

  f = fopen(archivo, "wb");

  // Todos los bytes son 0.
  buffer = (char *) calloc(1, sizeof(int));

  if (fwrite(buffer, sizeof(char), sizeof(int), f) != sizeof(int) * sizeof(char)) {
    fprintf(stderr, "No se ha podido inicializar el diccionario. Abortando");
    exit(-1);
  }

  free(buffer);
  fclose(f);
}

static int _get_raiz(char *archivo) {
  FILE *f;
  int raiz;

  if ((f = fopen(archivo, "rb")) == NULL) {
    fprintf(stderr, "No existe el archivo especificado que contiene el B-Tree");
    exit(-1);
  }

  fread(&raiz, sizeof(int), 1, f);

  if (raiz < 0) {
    fprintf(stderr, "El archivo no es un archivo válido para diccionarios");
    exit(-1);
  }

  fclose(f);

  return raiz;
}


int btree_search(struct btree_nodo *btree, int clave) {
  int k = 0;

  if (btree == NULL || btree->num_elems == 0) return 0;

  while (k < btree->num_elems && clave > btree->elementos[k]) k++;

  if (clave == btree->elementos[k] && k < btree->num_elems) return 1; // true

  if (btree->hijos == NULL) return 0;
  return btree_search(btree->hijos[k], clave);
}

static void _btree_insertar_elemento(struct btree_nodo *btree, int clave, int posicion) {
  if (posicion > btree->num_elems) {
    fprintf(stderr, "error de consistencia en nodo del b-tree\n");
    exit(-1);
  }

  if (posicion < btree->num_elems) {
    memmove(btree->elementos + posicion, btree->elementos + (posicion + 1),
            sizeof(int) * (btree->num_elems - posicion));
  }

  btree->elementos[posicion] = clave;
  btree->num_elems++;
}

static int _encontrar_candidato(struct btree_nodo *b, int clave) {
  int k = 0;

  if (b->num_elems == 0) return 0;

  while (k < b->num_elems && clave > b->elementos[k]) {
    k++;
  }

  return k;
}

static int _btree_handle_overflow(struct btree_nodo *b, struct btree_nodo *hijo) {
  int medio;
  int old_elems;
  int indice_elemento_candidato;
  int k;
  struct btree_nodo *nuevo_derecho;

  medio = (int) hijo->max_elems / 2;

  if (b->hijos == NULL) {
    // tendremos B+2 hijos (es +1 que elementos, xq daremos espacio adicional para overflow.
    b->hijos = (struct btree_nodo **) malloc((B + 2) * sizeof(struct btree_nodo *));
//    for(k=0; k<B+2; k++)
//      b->hijos[k] = (struct btree_nodo *)malloc(sizeof(struct btree_nodo *));
  }

  // manipulacion cochina.
  nuevo_derecho = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));
  btree_nodo_new(nuevo_derecho);
  nuevo_derecho->num_elems = hijo->max_elems - medio;
  // copiamos los elementos "del lado derecho" al nuevo nodo.
  memcpy(nuevo_derecho->elementos, hijo->elementos + nuevo_derecho->num_elems,
         sizeof(int) * nuevo_derecho->num_elems);

  // otra manipulación cochina al hijo.
  old_elems = hijo->num_elems;
  hijo->num_elems = medio;

  // insertar el elemento del medio del hijo en el padre
  indice_elemento_candidato = _encontrar_candidato(b, hijo->elementos[medio]);
  _btree_insertar_elemento(b, hijo->elementos[medio], indice_elemento_candidato);

  // antes de colgar, si el hijo tiene hijos hay que arreglarlos
  if (hijo->hijos != NULL) {
    // los hijos izquierdos quedaran bien puestos al haber reducido hijo->num_elems
    // pero los hijos derechos debemos recopiarlos
    nuevo_derecho->hijos = (struct btree_nodo **) malloc((B + 2) * sizeof(struct btree_nodo *));
    memcpy(nuevo_derecho->hijos, hijo->hijos + medio + 1, sizeof(struct btree_nodo *) * (old_elems - medio));
  }

  b->hijos[indice_elemento_candidato] = hijo;
  b->hijos[indice_elemento_candidato + 1] = nuevo_derecho;

  if (b->num_elems > b->max_elems) return 1;
  return 0;
}

// retorna true si debe subir el elemento por overflow
static int _btree_insertar(struct btree_nodo *candidato, int clave) {
  int indice_hijo_candidato;
  int indice_elemento_candidato;
  int retorno;
  struct btree_nodo *hijo;

  if (candidato->hijos == NULL) {
    indice_elemento_candidato = _encontrar_candidato(candidato, clave);
    _btree_insertar_elemento(candidato, clave, indice_elemento_candidato);

    if (candidato->num_elems > candidato->max_elems) return 1;
    else return 0;
  }

  indice_hijo_candidato = _encontrar_candidato(candidato, clave);
  hijo = candidato->hijos[indice_hijo_candidato];
  retorno = _btree_insertar(hijo, clave);

  // si no hay overflow, chao
  if (retorno == 0) return 0;

  // de aquí en adelante HAY OVERFLOW
  return _btree_handle_overflow(candidato, hijo);
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
  // TODO: borrar el archivo del btree.
}

/*
 * ESTRUCTURA DE UN NODO EN MEMORIA SECUNDARIA
 * Un pequeño análisis (se pone en el informe) nos lleva a que la cantidad de palabras (n)
 * almacenadas en el nodo debe cumplir que n <= (B-8)/20. En particular en el sistema donde
 * se corrió el programa, B=4096, de manera que n=204 y esto lleva a que el tamaño real de
 * un nodo en memoria secundaria es 4088 (sobran 8 bytes que rellenaremos con 0s).
 *
 * La serialización ha sido hecha de la sgte. manera (tb se explica en el informe)
 *
 * __________________________________________________________________________________________________________
 * | CANT ELEMENTOS (4 bytes) | n palabras (3264=204*16 bytes) | n+1 índices de hijos (4*(204+1)=820 bytes) |
 * |__________________________|________________________________|____________________________________________|
 * <------------------------------------ TOTAL = 4088 bytes ------------------------------------------------>
 *
 * Por último, cabe destacar que los strings que almacena el árbol son de ___16___ bytes, no de 15.
 * Hemos decidido usar strings normales de C para esto. (facilita el uso de strcmp, etc...)
 */

char *serializar_nodo(struct btree_nodo *b) {
  char *buffer;
  int num_elems;
  int elems_zero;
  int k;

  buffer = (char *) calloc(B, sizeof(char)); // inicializamos todos los bits a 0 (en partic. los elementos).
  num_elems = b->num_elems;

  // copiamos los bits desde el int al buffer [# de elementos]
  memcpy(buffer, &num_elems, sizeof(int));
  for (k = 0; k < num_elems; k++) {
    memcpy(buffer + sizeof(int) + k * TAMANO_CADENA * sizeof(char), b->elementos[k], TAMANO_CADENA);
  }

  // hasta aquí tenemos la cant. de elementos, todas las palabras y sólo faltan los hijos
  // ie: tenemos 4 + BTREE_ELEMS_NODO * 16 bytes.
  memcpy(buffer + sizeof(int) + BTREE_ELEMS_NODO * TAMANO_CADENA * sizeof(char),
         b->hijos, sizeof(int) * (BTREE_ELEMS_NODO + 1));

  // buffer listo
  return buffer;
}

struct btree_nodo *deserializar_nodo(char *buffer) {
  struct btree_nodo *nodo;
  int k;

  nodo = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));

  memcpy(&nodo->num_elems, buffer, sizeof(int));

  nodo->elementos = (char **) malloc(BTREE_ELEMS_NODO * sizeof(char *));
  for (k = 0; k < nodo->num_elems; k++) {
    nodo->elementos[k] = (char *) malloc(sizeof(char) * TAMANO_CADENA);
    memcpy(nodo->elementos[k], buffer + sizeof(int) + k * TAMANO_CADENA * sizeof(char), TAMANO_CADENA);
  }

  // finalmente, copiamos los indices de los hijos.
  nodo->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 1));
  memcpy(nodo->hijos, buffer + sizeof(int) + BTREE_ELEMS_NODO * TAMANO_CADENA * sizeof(char),
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





















