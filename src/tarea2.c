#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "btree.h"
#include "cadenas.h"
#include "hash_lineal.h"
#include "hash_extendible.h"
#include "parametros.h"
#include "extmem.h"

int hlin_accesos = 0;
int hext_accesos = 0;
int tree_accesos = 0;

int politica1(int num_elems, int num_buckets){
  if(1.0*num_elems/num_buckets > 1.5*NUM_ELEMS_PAGINA_LIN) return 1;
  if(1.0*num_elems/num_buckets < 0.83*NUM_ELEMS_PAGINA_LIN) return -1;
  return 0;
}

int politicatest(int a, int b){
  if(a==160) return 1;
  if(a > 175) return -1;
  return 0;
}

int main(int argc, char **argv) {
  srand48(1);

  system("rm *.data");

  btree_new(BTREE_FILE);

  struct hash_extendible hext;
  hashext_new(&hext);

  struct hash_lineal hlin;
  hashlin_new(&hlin, politica1);

  int cadenas = CADENAS_TOTAL;
  int buscar = CADENAS_BUSCAR;
  int borrar = CADENAS_BORRAR;

  char **aleatorias = (char**)malloc(sizeof(char*)*cadenas);
  int k = 0;

  printf("Insertando %i cadenas en cada estructura\n", cadenas);
  for(k=0; k<cadenas; k++){
    aleatorias[k] = (char*)malloc(sizeof(char)*16);
    cadena_rand(aleatorias[k]);

    btree_insertar(BTREE_FILE, aleatorias[k]);
    hashext_insertar(&hext, aleatorias[k], aleatorias[k]);
    hashlin_insertar(&hlin, aleatorias[k], aleatorias[k]);
  }

  int encontrados=0;

  printf("Eliminando %i cadenas\n", borrar);

  for(k=0; k<borrar; k++){
    hashext_eliminar(&hext, aleatorias[k]);
    hashlin_eliminar(&hlin, aleatorias[k]);
    btree_borrar(BTREE_FILE, aleatorias[k]);
  }

  printf("Buscando %i cadenas.\n", buscar);

  for(k=0; k<buscar; k++){
    btree_search(BTREE_FILE, aleatorias[k]);
    hashext_buscar(&hext, aleatorias[k]);
    hashlin_buscar(&hlin, aleatorias[k]);
  }

  printf("Encontrados %i elementos\n", encontrados);

  for(k=0; k<cadenas; k++) free(aleatorias[k]);

  free(aleatorias);
  hashext_dispose(&hext);


  return 0;
}
