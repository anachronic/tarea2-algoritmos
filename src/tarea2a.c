#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "hash_lineal.h"

int main(){
/*
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
*/
    printf("uint=%d, \n", (int)(sizeof(unsigned int)));
}
