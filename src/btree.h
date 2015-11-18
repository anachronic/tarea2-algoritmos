#ifndef TAREA2_ALGORITMOS_BTREE_H
#define TAREA2_ALGORITMOS_BTREE_H

#include "parametros.h"

#define BTREE_FILE "btree.data"
#define BTREE_ELEMS_NODO 204


/*
 * ESTRUCTURA DE UN NODO EN MEMORIA SECUNDARIA
 * Un pequeño análisis (se pone en el informe) nos lleva a que la cantidad de palabras (n)
 * almacenadas en el nodo debe cumplir que n <= (B-8)/20. En particular en el sistema donde
 * se corrió el programa, B=4096, de manera que n=204 y esto lleva a que el tamaño real de
 * un nodo en memoria secundaria es 4096 (por lo tanto, el bloque completo es ocupado con datos).
 *
 * La serialización ha sido hecha de la sgte. manera (tb se explica en el informe)
 *
 * __________________________________________________________________________________________________________
 * | CANT ELEMENTOS (4 bytes) | n palabras (3264=204*16 bytes) | n+1 índices de hijos (4*(204+1)=820 bytes) |
 * | CANT HIJOS (4 bytes)     |                                |                                            |
 * | INDICE PROPIO (4 bytes)  |                                |                                            |
 * |__________________________|________________________________|____________________________________________|
 * <------------------------------------ TOTAL = 4096 bytes ------------------------------------------------>
 *
 * Por último, cabe destacar que los strings que almacena el árbol son de ___16___ bytes, no de 15.
 * Hemos decidido usar strings normales de C para esto. (facilita el uso de strcmp, etc...)
 */

typedef struct btree_nodo {
  int num_elems;
  int num_hijos;
  int indice;
  char **elementos;
  int *hijos;
} btree;

void btree_new(char *archivo);

// diremos que *btree SOLO TIENE LA RAIZ.
int btree_search(const char *btree, const char *cadena);
void btree_eliminar(const char *btree, const char *cadena);
void btree_insertar(const char *btree, const char *cadena);
void btree_dispose(const char *archivo);

void btree_nodo_dispose(struct btree_nodo *b);

/*
 * SERIALIZACION Y DESERIALIZACION DE NODOS
 */
char *serializar_nodo(struct btree_nodo *b);
struct btree_nodo *deserializar_nodo(char *buffer);

#endif //TAREA2_ALGORITMOS_BTREE_H
