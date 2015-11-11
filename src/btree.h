#ifndef TAREA2_ALGORITMOS_BTREE_H
#define TAREA2_ALGORITMOS_BTREE_H

#include "parametros.h"

#define BTREE_FILE "btree.data"
#define BTREE_ELEMS_NODO 204

typedef struct btree_nodo {
  int num_elems;
  int num_hijos;
  char **elementos;
  int *hijos;
} btree;


void btree_new(char *archivo);

//retorna bool
int btree_search(const char *btree, const char *cadena);
void btree_eliminar(struct btree_nodo *btree, int clave);
struct btree_nodo *btree_insertar(struct btree_nodo *btree, int clave);

void btree_dispose(char *archivo);

void btree_nodo_dispose(struct btree_nodo *b);

/*
 * SERIALIZACION Y DESERIALIZACION DE NODOS
 */
char *serializar_nodo(struct btree_nodo *b);
struct btree_nodo *deserializar_nodo(char *buffer);

#endif //TAREA2_ALGORITMOS_BTREE_H
