#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "btree.h"
#include "cadenas.h"
#include "hash_lineal.h"

int main() {
/*
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
  */

    hashlin* table=(hashlin*)malloc(sizeof(hashlin));
    hashlin_init(table);

    hashlin_insert(table, "AGT");
    hashlin_insert(table, "CAG");
    hashlin_insert(table, "TCG");

    char* s1=(char*)hashlin_search(table, "CAG");
    char* s2=(char*)hashlin_search(table, "AGT");
    char* s3=(char*)hashlin_search(table, "TCG");
    char* s4=(char*)hashlin_search(table, "AAAAAAA");
    printf("s1=%s, s2=%s, s3=%s, s4=%s\n", s1, s2, s3, s4);

    hashlin_remove(table, "TCG");

    s1=(char*)hashlin_search(table, "CAG");
    s2=(char*)hashlin_search(table, "AGT");
    s3=(char*)hashlin_search(table, "TCG");
    s4=(char*)hashlin_search(table, "AAAAAAA");
    printf("s1=%s, s2=%s, s3=%s, s4=%s\n", s1, s2, s3, s4);

    hashlin_destroy(table);

    return 0;
}
