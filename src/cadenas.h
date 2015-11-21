#ifndef TAREA2_ALGORITMOS_CADENAS_H
#define TAREA2_ALGORITMOS_CADENAS_H

#define CADENAS_BORRAR 10000
#define CADENAS_BUSCAR 10000
#define CADENAS_TOTAL (1<<20)

char base_rand();
void cadena_rand(char *buffer);
char *get_random_from_array(char **arr, long size);

/* funcion de hash para las cadenas de ADN */
unsigned int DNAhash(char* s);

#endif //TAREA2_ALGORITMOS_CADENAS_H
