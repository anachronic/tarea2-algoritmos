

#include "btree.h"
#include "parametros.h"

void btree_nodo_new(struct btree_nodo *btree){
  int k;

  btree->num_elems = 0;
  btree->max_elems = B;
  btree->hijos_activos = 0;
  btree->elementos = (int*)malloc((B-1) * sizeof(int));
  btree->hijos = NULL;
}


int btree_search(struct btree_nodo *btree, int clave){
  int k=0;

  if(btree->hijos == NULL) return -1;

  while(clave < btree->elementos[k] && k < btree->num_elems) k++;

  if(clave == btree->elementos[k] && k < btree->max_elems) return 1; // true

  return btree_search(btree->hijos[k], clave);
}

/*void btree_insertar(struct btree_nodo *btree, int clave){
  if(btree->num_elems < btree->max_elems){
    int k, desiredPos;
    k=0;
    while(clave < btree->elems
  }
}*/





void btree_dispose(struct btree_nodo *btree){
  if(btree->hijos != NULL){
    // puede ser que haya que borrar los elems del array de hijos
    free(btree->hijos);
  }

  if(btree->elementos != NULL){
    free(btree->elementos);
  }
}
