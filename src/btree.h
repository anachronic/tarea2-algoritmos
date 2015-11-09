#ifndef TAREA2_ALGORITMOS_BTREE_H
#define TAREA2_ALGORITMOS_BTREE_H

#include "parametros.h"

#define BTREE_FILE "btree.data"

typedef struct btree_nodo {
  int num_elems;
  int max_elems;
  int *elementos;
  struct btree_nodo **hijos;
} btree;

struct overflow_result{
  int overflow;
  
};

void btree_nodo_new(struct btree_nodo *btree);

//retorna bool
int btree_search(struct btree_nodo *btree, int clave);
void btree_eliminar(struct btree_nodo *btree, int clave);
struct btree_nodo *btree_insertar(struct btree_nodo *btree, int clave);

void btree_dispose(struct btree_nodo *btree);


#endif //TAREA2_ALGORITMOS_BTREE_H
