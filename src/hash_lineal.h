#ifndef TAREA2_ALGORITMOS_HASH_LINEAL_H
#define TAREA2_ALGORITMOS_HASH_LINEAL_H

#include "parametros.h"


#define NUM_ELEMS_PAGINA_LIN 127

struct hash_lineal {
  int num_elems;
  int num_buckets;
  int s; // la "mitad" de max_buckets.
  // esta funcion debe retornar 0 si no se debe hacer nada
  //  1 si hay que expandir
  // -1 si hay que contraer
  int (*politica)(int,int);
};


/*
Estructura de una pagina en MEMORIA SECUNDARIA:
4    bytes: numero de elementos
4    bytes: indice en la cola de paginas.
4064 bytes: 127 pares (key,value)

TOTAL: 4072 bytes por pagina (de un total de 4096=B)
 */
struct hashlin_pagina {
  int num_elems;
  int list_index;
  char **hashes;
  char **values;
};

void hashlin_new(struct hash_lineal *h, int (*politica)(int,int));
void hashlin_insertar(struct hash_lineal *h, char *key, char *value);
int hashlin_buscar(struct hash_lineal *h, char *key);
void hashlin_eliminar(struct hash_lineal *h, char *key);
void hashlin_dispose(struct hash_lineal *h);

struct hashlin_pagina *deserializar_pagina_lin(char *buf);
char *serializar_pagina_lin(struct hashlin_pagina *p);

float get_ocupacion_hlin(struct hash_lineal *h);


#endif //TAREA2_ALGORITMOS_HASH_LINEAL_H
