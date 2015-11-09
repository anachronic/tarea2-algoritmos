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

// retorna true si debe subir el elemento por overflow
static int _btree_insertar(struct btree_nodo *candidato, int clave) {
  int indice_hijo_candidato;
  int indice_elemento_candidato;
  int retorno;
  int medio;
  struct btree_nodo *hijo;
  struct btree_nodo *nuevo_derecho;

  if(candidato->hijos == NULL){
    indice_elemento_candidato = _encontrar_candidato(candidato, clave);
    retorno = _btree_insertar_elemento(candidato, clave, indice_elemento_candidato);

    if(candidato->num_elems > candidato->max_elems) return 1;
    else return 0;
  }

  indice_hijo_candidato = _encontrar_candidato(candidato, clave);
  hijo = candidato->hijos[indice_hijo_candidato];
  retorno = _btree_insertar(hijo, clave);

  // si no hay overflow, chao
  if(retorno == 0) return 0;

  // de aquí en adelante HAY OVERFLOW
  medio = (int)hijo->max_elems/2;

  // manipulacion cochina.
  nuevo_derecho = (struct btree_nodo *)malloc(sizeof(struct btree_nodo));
  btree_nodo_new(nuevo_derecho);
  nuevo_derecho->num_elems = hijo->max_elems - medio;
  // copiamos los elementos "del lado derecho" al nuevo nodo.
  memcpy(nuevo_derecho->elementos, hijo->elementos + nuevo_derecho->num_elems + 1,
         sizeof(int) * nuevo_derecho->num_elems);

  // otra manipulación cochina al hijo.
  hijo->num_elems = medio;

  indice_elemento_candidato = _encontrar_candidato(candidato, hijo->elementos[medio]);
  _btree_insertar_elemento(candidato, hijo->elementos[medio], indice_elemento_candidato);

  candidato->hijos[indice_elemento_candidato] = hijo;
  candidato->hijos[indice_elemento_candidato + 1] = nuevo_derecho;

  if(candidato->num_elems > candidato->max_elems) return 1;
  return 0;
}

/*void btree_insertar(struct btree_nodo *btree, int clave){
 * // caso base: B-Tree vacío
  if(btree->num_elems == 0){
    _btree_insertar_elemento(btree, clave, 0);
    return;
  }
  if(btree->num_elems < btree->max_elems){
    int k, desiredPos;
    k=0;
    while(clave < btree->elems
  }
}*/





void btree_dispose(struct btree_nodo *btree) {
  if (btree->hijos != NULL) {
    // puede ser que haya que borrar los elems del array de hijos
    free(btree->hijos);
  }

  if (btree->elementos != NULL) {
    free(btree->elementos);
  }
}
