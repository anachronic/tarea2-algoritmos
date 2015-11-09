#include <stdlib.h>
#include <string.h>

#include "btree.h"
#include "parametros.h"

void btree_nodo_new(struct btree_nodo *btree) {
  int k;

  btree->num_elems = 0;
  btree->max_elems = B;
  // Almacena 1 elemento más (en caso de overflow)
  btree->elementos = (int *) malloc((B + 1) * sizeof(int));
  btree->hijos = NULL;
}


int btree_search(struct btree_nodo *btree, int clave) {
  int k = 0;

  if (btree == NULL || btree->num_elems == 0) return -1;

  while (k < btree->num_elems && clave < btree->elementos[k]) k++;

  if (clave == btree->elementos[k] && k < btree->max_elems) return 1; // true

  return btree_search(btree->hijos[k], clave);
}

static void _btree_insertar_elemento(struct btree_nodo *btree, int clave, int posicion) {
  int *elementos;
  int bloques_a_mover;

  elementos = btree->elementos;

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
  int indice_elemento_candidato;
  struct btree_nodo *nuevo_derecho;
  struct btree_nodo *hijo;

  medio = (int) hijo->max_elems / 2;

  // manipulacion cochina.
  nuevo_derecho = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));
  btree_nodo_new(nuevo_derecho);
  nuevo_derecho->num_elems = hijo->max_elems - medio;
  // copiamos los elementos "del lado derecho" al nuevo nodo.
  memcpy(nuevo_derecho->elementos, hijo->elementos + nuevo_derecho->num_elems + 1,
         sizeof(int) * nuevo_derecho->num_elems);

  // otra manipulación cochina al hijo.
  hijo->num_elems = medio;

  indice_elemento_candidato = _encontrar_candidato(b, hijo->elementos[medio]);
  _btree_insertar_elemento(b, hijo->elementos[medio], indice_elemento_candidato);

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
  int medio;
  struct btree_nodo *hijo;
  struct btree_nodo *nuevo_derecho;

  if (candidato->hijos == NULL) {
    indice_elemento_candidato = _encontrar_candidato(candidato, clave);
    retorno = _btree_insertar_elemento(candidato, clave, indice_elemento_candidato);

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

void btree_insertar(struct btree_nodo *btree, int clave) {
  int retorno;
  struct btree_nodo *nueva_raiz;

  // caso base: B-Tree vacío
  if (btree->num_elems == 0) {
    _btree_insertar_elemento(btree, clave, 0);
    return;
  }

  retorno = _btree_insertar(btree, clave);
  if (retorno == 0) return;

  // a partir de este momento hay overflow en la raíz.
  nueva_raiz = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));
  btree_nodo_new(nueva_raiz);

  // esto siempre retorna 0, asi que chao con el valor de retorno.
  _btree_handle_overflow(nueva_raiz, btree);
}


void btree_dispose(struct btree_nodo *btree) {
  if (btree->hijos != NULL) {
    // puede ser que haya que borrar los elems del array de hijos
    free(btree->hijos);
  }

  if (btree->elementos != NULL) {
    free(btree->elementos);
  }
}
