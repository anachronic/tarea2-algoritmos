#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "btree.h"
#include "cadenas.h"

int main() {
  srand48(getpid());
  btree_new(BTREE_FILE);

  int cadenillas = 800;

  char **aleatorias = (char**)malloc(sizeof(char*)*cadenillas);
  int k = 0;

  for(k=0; k<cadenillas; k++){
    aleatorias[k] = (char*)malloc(sizeof(char)*16);
    cadena_rand(aleatorias[k]);

    btree_insertar(BTREE_FILE, aleatorias[k]);
  }

  int encontrados=0;

  printf("Buscando %i cadenas. Todas deben ser encontradas\n", cadenillas);

  for(k=0; k<cadenillas; k++){
    if(btree_search(BTREE_FILE, aleatorias[k]) == 1)
      encontrados++;
    else printf("WARNING: No se encontró %s\n", aleatorias[k]);
  }

  printf("Encontrados %i elementos\n", encontrados);

  for(k=0; k<cadenillas; k++) free(aleatorias[k]);

  free(aleatorias);

  return 0;
}