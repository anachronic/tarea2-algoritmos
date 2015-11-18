#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "btree.h"
#include "cadenas.h"
#include "hash_lineal.h"
#include "hash_extendible.h"

int main() {
  srand48(1);
//  btree_new(BTREE_FILE);
//  struct hash_extendible h;
//  hashext_new(&h);
  struct hash_lineal h;
  hashlin_new(&h, NULL);

  int cadenillas = 127;
  int eliminar = 19900;

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

//  printf("Eliminando %i cadenas\n", eliminar);
//
//  for(k=0; k<eliminar; k++){
//    hashext_eliminar(&h, aleatorias[k]);
//  }

  printf("Buscando %i cadenas. Se deben encontrar %i\n", cadenillas, cadenillas /*- eliminar*/);

  for(k=0; k<cadenillas; k++){
//    if(btree_search(BTREE_FILE, aleatorias[k]) == 1)
//    if(hashext_buscar(&h, aleatorias[k]) == 1)
    if(hashlin_buscar(&h, aleatorias[k])==1)
      encontrados++;
//    else printf("WARNING: No se encontró %s\n", aleatorias[k]);
  }

  printf("Encontrados %i elementos\n", encontrados);

  for(k=0; k<cadenillas; k++) free(aleatorias[k]);

  free(aleatorias);
//  hashext_dispose(&h);


  return 0;
}
