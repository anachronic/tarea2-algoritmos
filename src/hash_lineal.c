#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hash_lineal.h"
#include "parametros.h"
#include "extmem.h"

#define HASHLIN_STATE_STABLE 0
#define HASHLIN_STATE_GROW 1
#define HASHLIN_STATE_SHRINK 2

void hashlin_stable(hashlin* hashlin){
	hashlin->state = HASHLIN_STATE_STABLE;

	/* setup low_mask/max/split to allow tommy_hashlin_bucket_ref() */
	/* and tommy_hashlin_foreach() to work regardless we are in stable state */
	hashlin->low_max = hashlin->bucket_max;
	hashlin->low_mask = hashlin->bucket_mask;
	hashlin->split = 0;
}

void hashlin_init(hashlin* hashlin, char* file){
	int i;

	/* fixed initial size */
	hashlin->bucket_bit = HASHLIN_BIT;
	hashlin->bucket_max = 1 << hashlin->bucket_bit;
	hashlin->bucket_mask = hashlin->bucket_max - 1;
	hashlin->bucket[0] = (hash_node**)calloc(hashlin->bucket_max, sizeof(hash_node*));
	for (i = 1; i < HASHLIN_BIT; ++i)
		hashlin->bucket[i] = hashlin->bucket[0];

	/* stable state */
	hashlin_stable(hashlin);

	hashlin->count = 0;

    hashlin->next_b=0;
    hashlin->file=file;
}

void hashlin_destroy(hashlin* hashlin){
	unsigned int i;

	free(hashlin->bucket[0]);
	for (i = HASHLIN_BIT; i < hashlin->bucket_bit; ++i) {
		hash_node** segment = hashlin->bucket[i];
		free(&segment[((ptrdiff_t)1) << i]);
	}
}

/**
 * Grow one step.
 */
void hashlin_grow_step(hashlin* hashlin){
	/* grow if more than 50% full */
	if (hashlin->state != HASHLIN_STATE_GROW
		&& hashlin->count > hashlin->bucket_max / 2) {
		/* if we are stable, setup a new grow state */
		/* otherwise continue with the already setup shrink one */
		/* but in backward direction */
		if (hashlin->state == HASHLIN_STATE_STABLE) {
			hash_node** segment;

			/* set the lower size */
			hashlin->low_max = hashlin->bucket_max;
			hashlin->low_mask = hashlin->bucket_mask;

			/* allocate the new vector using malloc() and not calloc() */
			/* because data is fully initialized in the split process */
			segment = (hash_node**)malloc(hashlin->low_max * sizeof(hash_node*));

			/* store it adjusting the offset */
			/* cast to ptrdiff_t to ensure to get a negative value */
			hashlin->bucket[hashlin->bucket_bit] = &segment[-(ptrdiff_t)hashlin->low_max];

			/* grow the hash size */
			++hashlin->bucket_bit;
			hashlin->bucket_max = 1 << hashlin->bucket_bit;
			hashlin->bucket_mask = hashlin->bucket_max - 1;

			/* start from the beginning going forward */
			hashlin->split = 0;
		}

		/* grow state */
		hashlin->state = HASHLIN_STATE_GROW;
	}

	/* if we are growing */
	if (hashlin->state == HASHLIN_STATE_GROW) {
		/* compute the split target required to finish the reallocation before the next resize */
		unsigned int split_target = 2 * hashlin->count;

		/* reallocate buckets until the split target */
		while (hashlin->split + hashlin->low_max < split_target) {
			hash_node** split[2];
			hash_node* j;
			unsigned int mask;

			/* get the low bucket */
			split[0] = hashlin_pos(hashlin, hashlin->split);

			/* get the high bucket */
			split[1] = hashlin_pos(hashlin, hashlin->split + hashlin->low_max);

			/* save the low bucket */
			j = *split[0];

			/* reinitialize the buckets */
			*split[0] = 0;
			*split[1] = 0;

			/* the bit used to identify the bucket */
			mask = hashlin->low_max;

			/* flush the bucket */
			while (j) {
				hash_node* j_next = j->next;
				unsigned int pos = (j->key & mask) != 0;
				if (*split[pos])
					list_insert_tail_not_empty(*split[pos], j);
				else
					list_insert_first(split[pos], j);
				j = j_next;
			}

			/* go forward */
			++hashlin->split;

			/* if we have finished, change the state */
			if (hashlin->split == hashlin->low_max) {
				/* go in stable mode */
				hashlin_stable(hashlin);
				break;
			}
		}
	}
}

/**
 * Shrink one step.
 */
void hashlin_shrink_step(hashlin* hashlin)
{
	/* shrink if less than 12.5% full */
	if (hashlin->state != HASHLIN_STATE_SHRINK
		&& hashlin->count < hashlin->bucket_max / 8
	) {
		/* avoid to shrink the first bucket */
		if (hashlin->bucket_bit > HASHLIN_BIT) {
			/* if we are stable, setup a new shrink state */
			/* otherwise continue with the already setup grow one */
			/* but in backward direction */
			if (hashlin->state == HASHLIN_STATE_STABLE) {
				/* set the lower size */
				hashlin->low_max = hashlin->bucket_max / 2;
				hashlin->low_mask = hashlin->bucket_mask / 2;

				/* start from the half going backward */
				hashlin->split = hashlin->low_max;
			}

			/* start reallocation */
			hashlin->state = HASHLIN_STATE_SHRINK;
		}
	}

	/* if we are shrinking */
	if (hashlin->state == HASHLIN_STATE_SHRINK) {
		/* compute the split target required to finish the reallocation before the next resize */
		unsigned int split_target = 8 * hashlin->count;

		/* reallocate buckets until the split target */
		while (hashlin->split + hashlin->low_max > split_target) {
			hash_node** split[2];

			/* go backward position */
			--hashlin->split;

			/* get the low bucket */
			split[0] = hashlin_pos(hashlin, hashlin->split);

			/* get the high bucket */
			split[1] = hashlin_pos(hashlin, hashlin->split + hashlin->low_max);

			/* concat the high bucket into the low one */
			list_concat(split[0], split[1]);

			/* if we have finished, clean up and change the state */
			if (hashlin->split == 0) {
				hash_node** segment;

				/* shrink the hash size */
				--hashlin->bucket_bit;
				hashlin->bucket_max = 1 << hashlin->bucket_bit;
				hashlin->bucket_mask = hashlin->bucket_max - 1;

				/* free the last segment */
				segment = hashlin->bucket[hashlin->bucket_bit];
				free(&segment[((ptrdiff_t)1) << hashlin->bucket_bit]);

				/* go in stable mode */
				hashlin_stable(hashlin);
				break;
			}
		}
	}
}

void hashlin_insert(hashlin* hashlin, void* data){

    hash_node* node=(hash_node*)malloc(sizeof(hash_node));

    unsigned int hash=DNAhash((char*)data);

    /* get block where to insert */
    int block=hashlin->next_b;
    /* write data in file, in that block */
    set_bloque(hashlin->file, (char*)data, block, 0);
	list_insert_tail(hashlin_bucket_ref(hashlin, hash), node);

	node->key = hash;
	/* asign block number to node */
	node->data_block = block;

	++hashlin->count;
	/* increase next_b */
	++hashlin->next_b;

	hashlin_grow_step(hashlin);
}

/* NO USAR, y si se va a usar, ARREGLAR
int hashlin_remove_existing(hashlin* hashlin, hash_node* node)
{
	list_remove_existing(hashlin_bucket_ref(hashlin, node->key), node);

	--hashlin->count;

	hashlin_shrink_step(hashlin);

	return node->data;
}
*/

/* Se quita a la función de comparación de los argumentos, y se usa solo la igualdad */
int hashlin_remove(hashlin* hashlin, const void* cmp_arg){
    unsigned int hash=DNAhash((char*)cmp_arg);
	hash_node** let_ptr = hashlin_bucket_ref(hashlin, hash);
	hash_node* node = *let_ptr;

	while (node) {
		/* we first check if the hash matches, as in the same bucket we may have multiples hash values */
		if (node->key == hash) { /* No se compara com cmp_arg, ya que hash identifica únicamente a un string */
			list_remove_existing(let_ptr, node);

			--hashlin->count;

			hashlin_shrink_step(hashlin);

			return node->data_block;
		}
		node = node->next;
	}

	return -1; /* Por favor chequear esto, pero yo creo que es solo cambiar la API, no es problema */
}

void* hashlin_search(hashlin* hashlin, const void* cmp_arg){
    unsigned int hash=DNAhash((char*)cmp_arg);
	hash_node* i = *hashlin_bucket_ref(hashlin, hash);
	int block;

	while (i) {
		/* we first check if the hash matches, as in the same bucket we may have multiples hash values */
		if (i->key == hash){ /* no es necesario comparar con cmp_arg, el hash solo puede corresponder a un string */
            block=i->data_block;
            return recuperar_bloque(hashlin->file, block, 0);
		}
		i = i->next;
	}
	return 0;
}

unsigned int DNAhash(char* s){
    /* Pasamos a binario el primer caracter del string */
    if(strlen(s)>0){
        unsigned int n;
        switch(s[0]){
            case 'G':
                n=0;
                break;
            case 'C':
                n=1;
                break;
            case 'A':
                n=2;
                break;
            case 'T':
                n=3;
                break;
        }
        n=n<<(2*strlen(s+1));
        return n + DNAhash(s+1);
    }
    else
        return 0;
}

hash_node** hashlin_bucket_ref(hashlin* hashlin, unsigned int hash){
	unsigned int pos;
	unsigned int high_pos;

	pos = hash & hashlin->low_mask;
	high_pos = hash & hashlin->bucket_mask;

	/* if this position is already allocated in the high half */
	if (pos < hashlin->split) {
		pos = high_pos;
	}

	return hashlin_pos(hashlin, pos);
}

hash_node** hashlin_pos(hashlin* hashlin, unsigned int pos){
	unsigned int bsr;

	/* get the highest bit set, in case of all 0, return 0 */
	bsr = ilog2_u32(pos | 1);

	return &hashlin->bucket[bsr][pos];
}

unsigned int ilog2_u32(unsigned int value){
    return __builtin_clz(value) ^ 31; /* Khé??? */
}

void list_insert_tail(hash_node** list, hash_node* node){
	hash_node* head = *list;

	if (head)
		list_insert_tail_not_empty(head, node);
	else
        list_insert_first(list, node);
}

void list_insert_tail_not_empty(hash_node* head, hash_node* node)
{
	/* insert in the "circular" prev list */
	node->prev = head->prev;
	head->prev = node;

	/* insert in the "0 terminated" next list */
	node->next = 0;
	node->prev->next = node;
}

void list_insert_first(hash_node** list, hash_node* node){
	/* one element "circular" prev list */
	node->prev = node;

	/* one element "0 terminated" next list */
	node->next = 0;

	*list = node;
}

int list_empty(hash_node** list)
{
	return list_head(list) == 0;
}

void list_concat(hash_node** first, hash_node** second){
	hash_node* first_head;
	hash_node* first_tail;
	hash_node* second_head;

	if (list_empty(second))
		return;

	if (list_empty(first)) {
		*first = *second;
		return;
	}

	first_head = list_head(first);
	second_head = list_head(second);
	first_tail = list_tail(first);

	/* set the "circular" prev list */
	first_head->prev = second_head->prev;
	second_head->prev = first_tail;

	/* set the "0 terminated" next list */
	first_tail->next = second_head;
}

hash_node* list_head(hash_node** list){
	return *list;
}

hash_node* list_tail(hash_node** list){
	hash_node* head = list_head(list);

	if (!head)
		return 0;

	return head->prev;
}

int list_remove_existing(hash_node** list, hash_node* node){
	hash_node* head = list_head(list);

	/* remove from the "circular" prev list */
	if (node->next)
		node->next->prev = node->prev;
	else
		head->prev = node->prev; /* the last */

	/* remove from the "0 terminated" next list */
	if (head == node)
		*list = node->next; /* the new head, in case 0 */
	else
		node->prev->next = node->next;

	return node->data_block;
}
