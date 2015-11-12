#ifndef TAREA2_ALGORITMOS_HASH_LINEAL_H
#define TAREA2_ALGORITMOS_HASH_LINEAL_H

#endif //TAREA2_ALGORITMOS_HASH_LINEAL_H

#define HASHLIN_BIT_MAX 32
#define HASHLIN_BIT 6

typedef struct hash_node_struct {
	/**
	 * Next node.
	 * The tail node has it at 0, like a 0 terminated list.
	 */
	struct hash_node_struct* next;

	/**
	 * Previous node.
	 * The head node points to the tail node, like a circular list.
	 */
	struct hash_node_struct* prev;

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
	hash_node** bucket[HASHLIN_BIT_MAX]; /**< Dynamic array of hash buckets. One list for each hash modulus. */
	unsigned int bucket_max; /**< Number of buckets. */
	unsigned int bucket_bit; /**< Bits used in the bit mask. */
	unsigned int bucket_mask; /**< Bit mask to access the buckets. */
	unsigned int low_max; /**< Low order max value. */
	unsigned int low_mask; /**< Low order mask value. */
	unsigned int split; /**< Split position. */
	unsigned int count; /**< Number of elements. */
	unsigned int state; /**< Reallocation state. */
} hashlin;

void hashlin_init(hashlin* hashlin);

void hashlin_destroy(hashlin* hashlin);

void hashlin_insert(hashlin* hashlin, hash_node* node, void* data, unsigned int hash);

void* hashlin_remove(hashlin* hashlin, const void* cmp_arg, unsigned int hash);

hash_node** hashlin_bucket_ref(hashlin* hashlin, unsigned int hash){
	unsigned int pos;
	unsigned int high_pos;

	pos = hash & hashlin->low_mask;
	high_pos = hash & hashlin->bucket_mask;

	/* if this position is already allocated in the high half */
	if (pos < hashlin->split) {
		pos = high_pos;
	}

	unsigned int bsr;

	posaux=pos;
	bsr = 0;
	while(posaux>0){
        posaux>>1;
        bsr++;
	}

	return &hashlin->bucket[bsr][pos];
}

/* Se quita a la función de separación de los argumentos, y se usa solo la igualdad */
void* hashlin_search(hashlin* hashlin, const void* cmp_arg, unsigned int hash){
	hash_node* i = *hashlin_bucket_ref(hashlin, hash);

	while (i) {
		/* we first check if the hash matches, as in the same bucket we may have multiples hash values */
		if (i->key == hash && cmp_arg == i->data)
			return i->data;
		i = i->next;
	}
	return 0;
}

void list_insert_tail(hash_node** list, hash_node* node, void* data);

void list_insert_tail_not_empty(hash_node* head, hash_node* node);

void list_insert_first(hash_node** list, hash_node* node);

int list_empty(hash_node* list);

void list_concat(hash_node** first, hash_node** second);

hash_node* list_head(hash_node** list);

hash_node* list_tail(hash_node** list);

void* list_remove_existing(hash_node** list, hash_node* node);
