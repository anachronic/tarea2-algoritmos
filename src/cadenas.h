#ifndef TAREA2_ALGORITMOS_CADENAS_H
#define TAREA2_ALGORITMOS_CADENAS_H

#define BORRAR_CADENAS 10000
#define BUSCAR_CADENAS 10000
#define TOTAL_CADENAS (1<<14)

char base_rand();
void cadena_rand(char *buffer);
char *get_random_from_array(char **arr, long size);

/* funcion de hash para las cadenas de ADN */
unsigned int DNAhash(char* s);

#endif //TAREA2_ALGORITMOS_CADENAS_H
