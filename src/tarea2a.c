#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "hash_lineal.h"

int main(){
    srand48(1);
    hashlin* table=(hashlin*)malloc(sizeof(hashlin));
    hashlin_init(table, HASHLIN_FILE);

    int cadenillas = 800;

    char **aleatorias = (char**)malloc(sizeof(char*)*cadenillas);
    int k = 0;

    for(k=0; k<cadenillas; k++){
        aleatorias[k] = (char*)malloc(sizeof(char)*16);
        cadena_rand(aleatorias[k]);

        hashlin_insert(table, aleatorias[k]);
    }

    hashlin_remove(table, aleatorias[0]);

    printf("No se debería encontrar %s\n", aleatorias[0]);

    int encontrados=0;

    printf("Buscando %i cadenas. Todas deben ser encontradas\n", cadenillas);

    for(k=0; k<cadenillas; k++){
        if(strcmp((char*)hashlin_search(table, aleatorias[k]), aleatorias[k]) == 0)
            encontrados++;
        else printf("WARNING: No se encontró %s\n", aleatorias[k]);
    }

    printf("Encontrados %i elementos\n", encontrados);

    for(k=0; k<cadenillas; k++) free(aleatorias[k]);

    free(aleatorias);

    return 0;
}
