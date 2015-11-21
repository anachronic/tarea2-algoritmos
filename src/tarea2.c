#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "btree.h"
#include "cadenas.h"
#include "hash_lineal.h"
#include "hash_extendible.h"
#include "parametros.h"
#include "extmem.h"

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
//  btree_new(BTREE_FILE);
//  struct hash_extendible h;
//  hashext_new(&h);
  struct hash_lineal h;
  hashlin_new(&h, politica1);

  int cadenillas = atoi(argv[1]);
  int eliminar = atoi(argv[2]);

  char **aleatorias = (char**)malloc(sizeof(char*)*cadenillas);
  int k = 0;

  printf("Insertando %i cadenas\n", cadenillas);
  for(k=0; k<cadenillas; k++){
    aleatorias[k] = (char*)malloc(sizeof(char)*16);
    cadena_rand(aleatorias[k]);

//    btree_insertar(BTREE_FILE, aleatorias[k]);
//    hashext_insertar(&h, aleatorias[k], aleatorias[k]);
    hashlin_insertar(&h, aleatorias[k], aleatorias[k]);
  }

  int encontrados=0;

  printf("Eliminando %i cadenas\n", eliminar);

  for(k=0; k<eliminar; k++){
//    hashext_eliminar(&h, aleatorias[k]);
    hashlin_eliminar(&h, aleatorias[k]);
  }
//  btree_borrar(BTREE_FILE, "CTCGTCTGAGAACTC");
//  btree_borrar(BTREE_FILE, aleatorias[0]);

  printf("Buscando %i cadenas. Se deben encontrar %i\n", cadenillas, cadenillas-eliminar);

  for(k=0; k<cadenillas; k++){
//    if(btree_search(BTREE_FILE, aleatorias[k]) == 1)
//    if(hashext_buscar(&h, aleatorias[k]) == 1)
    if(hashlin_buscar(&h, aleatorias[k])==1)
      encontrados++;
    else{
      if(k >= eliminar) printf("WARNING: No se encontró %s\n", aleatorias[k]);
    }
  }

  printf("Encontrados %i elementos\n", encontrados);

  for(k=0; k<cadenillas; k++) free(aleatorias[k]);

  free(aleatorias);
//  hashext_dispose(&h);


  return 0;
}
