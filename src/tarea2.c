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
  srand48(getpid());

  system("rm *.data");

  btree_new(BTREE_FILE);

  struct hash_extendible hext;
  hashext_new(&hext);

  struct hash_lineal hlin;
  hashlin_new(&hlin, politica1);

  int strings = TOTAL_CADENAS;
  int buscar = BUSCAR_CADENAS;
  int borrar = BORRAR_CADENAS;

  char **aleatorias = (char**)malloc(sizeof(char*)*strings);
  int k = 0;

  printf("Insertando %i cadenas en cada estructura\n", strings);
  for(k=0; k<strings; k++){
    aleatorias[k] = (char*)malloc(sizeof(char)*16);
    cadena_rand(aleatorias[k]);

    btree_insertar(BTREE_FILE, aleatorias[k]);
    hashext_insertar(&hext, aleatorias[k], aleatorias[k]);
    hashlin_insertar(&hlin, aleatorias[k], aleatorias[k]);
  }

  int encontrados_h1=0;
  int encontrados_h2=0;
  int encontrados_bt=0;

  printf("Eliminando %i cadenas\n", borrar);

  for(k=0; k<borrar; k++){
    hashext_eliminar(&hext, aleatorias[k]);
    hashlin_eliminar(&hlin, aleatorias[k]);
    btree_borrar(BTREE_FILE, aleatorias[k]);
  }

  printf("Buscando %i cadenas.\n", buscar);

  for(k=0; k<buscar; k++){
    if(btree_search(BTREE_FILE, aleatorias[k])) encontrados_bt++;
    if(hashext_buscar(&hext, aleatorias[k])) encontrados_h1++;
    if(hashlin_buscar(&hlin, aleatorias[k])) encontrados_h2++;
  }

  printf("Ocupación BTree\t\t%f\n", get_ocupacion_btree(BTREE_FILE));
  printf("Ocupación Hash Lineal\t%f\n", get_ocupacion_hlin(&hlin));
  printf("Ocupación Hash Extendible\t%f\n", get_ocupacion_hext(&hext));

  printf("Btree encontró %i\n", encontrados_bt);
  printf("Hashing extendible encontró %i\n", encontrados_h1);
  printf("Hashing lineal encontró %i\n", encontrados_h2);

  printf("Accesos B-Tree\t%i\n", tree_accesos);
  printf("Accesos Hash Lineal\t%i\n", hlin_accesos);
  printf("Accesos Hash Extendible\t%i\n", hext_accesos);

  for(k=0; k<strings; k++) free(aleatorias[k]);

  free(aleatorias);
  hashext_dispose(&hext);


  return 0;
}
