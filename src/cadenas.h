#ifndef TAREA2_ALGORITMOS_CADENAS_H
#define TAREA2_ALGORITMOS_CADENAS_H

#define BORRAR_CADENAS 10000
#define BUSCAR_CADENAS 10000
#define TOTAL_CADENAS (1<<20)

char base_rand();
void cadena_rand(char *buffer);
char *get_random_from_array(char **arr, long size);

/* funcion de hash para las cadenas de ADN */
unsigned int DNAhash(char* s);

struct cadena_struct{
  int total_alloc;
  int num_elems;
  char **cadenas;
};

void crear_cadenas(struct cadena_struct *cs, int size);
char *get_cadena(struct cadena_struct *cs, int k);
void eliminar_cadena(struct cadena_struct *cs, int k);
void dispose_cadenas(struct cadena_struct *cs);

#endif //TAREA2_ALGORITMOS_CADENAS_H
