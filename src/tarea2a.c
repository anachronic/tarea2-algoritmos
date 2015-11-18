#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#define HASHLIN_BIT_MAX 32
#define HASHLIN_BIT 6
#define HASHLIN_NODE_SIZE 32

typedef struct hash_node_struct {

	/**
	 * Previous node.
	 * The head node points to the tail node, like a circular list.
	 */
	struct hash_node_struct* prev;

	/**
	 * Next node.
	 * The tail node has it at 0, like a 0 terminated list.
	 */
	struct hash_node_struct* next;

	/**
	 * Pointer at the object containing the node.
	 * This field is initialized when inserting nodes into a data structure.
	 */
	void* data;

	/**
	 * Key used to store the node.
	 * With hashtables this field is used to store the hash value.
	 * With lists this field is not used.
	 */
	unsigned int key;
} hash_node;

typedef struct hashlin_struct {
	char* bucket[HASHLIN_BIT_MAX]; /**< Dynamic array of hash buckets. One list for each hash modulus. */
	unsigned int bucket_max; /**< Number of buckets. */
	unsigned int bucket_bit; /**< Bits used in the bit mask. */
	unsigned int bucket_mask; /**< Bit mask to access the buckets. */
	unsigned int low_max; /**< Low order max value. */
	unsigned int low_mask; /**< Low order mask value. */
	unsigned int split; /**< Split position. */
	unsigned int count; /**< Number of elements. */
	unsigned int state; /**< Reallocation state. */
} hashlin;

void hashlin_stable(hashlin* hashlin){
	hashlin->state = 0;

	/* setup low_mask/max/split to allow tommy_hashlin_bucket_ref() */
	/* and tommy_hashlin_foreach() to work regardless we are in stable state */
	hashlin->low_max = hashlin->bucket_max;
	hashlin->low_mask = hashlin->bucket_mask;
	hashlin->split = 0;
}

void hashlin_init(hashlin* hashlin){
	int i;

	/* fixed initial size */
	hashlin->bucket_bit = HASHLIN_BIT;
	hashlin->bucket_max = 1 << hashlin->bucket_bit;
	hashlin->bucket_mask = hashlin->bucket_max - 1;
	for (i = 0; i < HASHLIN_BIT; ++i){
    hashlin->bucket[i] = (char*)malloc(sizeof(char)*32);
		sprintf(hashlin->bucket[i], "b%d.data", i);
  }

	/* stable state */
	hashlin_stable(hashlin);

	hashlin->count = 0;
}

int main(){
/*
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
    hashlin_destroy(table);

    */
		/*
		int size=5, i;
		char** arr=(char**)malloc(sizeof(char*)*size);
    arr[0]=(char*)malloc(16);
		arr[1]=(char*)malloc(16);
		arr[2]=(char*)malloc(16);
		arr[3]=(char*)malloc(16);
		arr[4]=(char*)malloc(16);
		arr[0]="A234567812345678";
		arr[1]="B234567812345678";
		arr[2]="C234567812345678";
		arr[3]="D234567812345678";
		arr[4]="E234567812345678";
		int k=2;
		char** arr2=(char**)malloc(sizeof(char*)*size);
		memmove(arr2, arr, 16*k);
		memmove(arr2+k, arr+k+1, 16*(size-k-1));
		free(arr);
		printf("s0=%s, s1=%s, s2=%s, s3=%s\n", arr2[0], arr2[1], arr2[2], arr2[3]);
		*/

		char* holi1=(char*)malloc(sizeof(char)*5);
		char* holi2=(char*)malloc(sizeof(char)*5);
		holi1="holasas";
		free(holi1);
		//printf("%s\n", holi1);
}
