#ifndef TAREA2_ALGORITMOS_BTREE_H
#define TAREA2_ALGORITMOS_BTREE_H

#include "parametros.h"

#define BTREE_FILE "btree.data"

typedef struct btree_nodo {
  int num_elems;
  char **elementos;
  int *hijos;
} btree;


void btree_nodo_new(char *archivo);

//retorna bool
int btree_search(struct btree_nodo *btree, int clave);
void btree_eliminar(struct btree_nodo *btree, int clave);
struct btree_nodo *btree_insertar(struct btree_nodo *btree, int clave);

void btree_dispose(struct btree_nodo *btree);

/*
 * SERIALIZACION Y DESERIALIZACION DE NODOS
 */
char *serializar_nodo(struct btree_nodo *b);


#endif //TAREA2_ALGORITMOS_BTREE_H
