#ifndef TAREA2_ALGORITMOS_HASH_EXTENDIBLE_H
#define TAREA2_ALGORITMOS_HASH_EXTENDIBLE_H

#define NUM_ELEMS_PAGINA 204

struct hash_extendible {
  struct hash_extendible_p *h;
};

/* este es el "trie" */
struct hash_extendible_p {
  int indice_pagina;
  int num_elems;
  struct hash_extendible_p *hizq;
  struct hash_extendible_p *hder;
};

struct hashext_pagina {
  int num_elems;
  unsigned int *hashes;
  char **valores;
};

/*
Estructura en memoria secundaria
4 bytes del indice de la pagina
4 bytes de la cantidad de elementos en la pagina
4080 bytes correspondientes a 204 pares de 20 bytes (hash, valor)
total = 4088 bytes por bloques para datos
*/


void hashext_new(struct hash_extendible *h);
void hashext_insertar(struct hash_extendible *h, char *key, void *val);
int hashext_buscar(struct hash_extendible *h, const char *key);
void hashext_dispose(struct hash_extendible *h);

struct hashext_pagina *deserializar_pagina(char *buf);
char *serializar_pagina(struct hashext_pagina *p);


#endif //TAREA2_ALGORITMOS_HASH_EXTENDIBLE_H
