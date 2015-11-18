#ifndef TAREA2_ALGORITMOS_HASH_LINEAL_H
#define TAREA2_ALGORITMOS_HASH_LINEAL_H

#define HASHLIN_BIT_MAX 32
#define HASHLIN_BIT 6
#define HASHLIN_NODE_SIZE 32

typedef struct hash_node_struct {

	struct hash_node_struct* next;

	struct hash_node_struct* prev;

	void* data;

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

/* Operaciones del diccionario */
void hashlin_init(hashlin* hashlin);
void hashlin_destroy(hashlin* hashlin);
void hashlin_insert(hashlin* hashlin, void* data);
void* hashlin_remove(hashlin* hashlin, const void* cmp_arg);
void* hashlin_search(hashlin* hashlin, const void* cmp_arg);

/* Función de hash para las cadenas de ADN */
unsigned int DNAhash(char* s);

/* Funciones auxiliares */
hash_node** hashlin_pos(hashlin* hashlin, unsigned int pos);
unsigned int ilog2_u32(unsigned int value);
hash_node** hashlin_bucket_ref(hashlin* hashlin, unsigned int hash);

/* Listas enlazadas */
void list_insert_tail(hash_node** list, hash_node* node, void* data);
void list_insert_tail_not_empty(hash_node* head, hash_node* node);
void list_insert_first(hash_node** list, hash_node* node);
int list_empty(hash_node** list);
void list_concat(hash_node** first, hash_node** second);
hash_node* list_head(hash_node** list);
hash_node* list_tail(hash_node** list);
void* list_remove_existing(hash_node** list, hash_node* node);

/* Serialización */
char* serializar_lista_hashlin(hash_node* list);
hash_node* deserializar_lista_hashlin(char* buffer);

#endif //TAREA2_ALGORITMOS_HASH_LINEAL_H
