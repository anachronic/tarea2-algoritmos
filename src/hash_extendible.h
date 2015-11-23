#ifndef TAREA2_ALGORITMOS_HASH_EXTENDIBLE_H
#define TAREA2_ALGORITMOS_HASH_EXTENDIBLE_H

#define NUM_ELEMS_PAGINA 127
#define NUM_RECOMBINACION ((int)(5/7*NUM_ELEMS_PAGINA))

struct hash_extendible {
  struct hash_extendible_p *h;
  int max_indice;
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
  int profundidad;
  char **hashes;
  char **valores;
};

/*
Estructura en memoria secundaria de una pagina
4 bytes que indican el numero de elementos
4 bytes que indican la profundidad de la pagina (en el trie)
4064 bytes correspondientes a 127 pares de 32 bytes (key, valor)
total = 4072 bytes por bloques para datos
*/


void hashext_new(struct hash_extendible *h);
void hashext_insertar(struct hash_extendible *h, char *key, void *val);
int hashext_buscar(struct hash_extendible *h, char *key);
void hashext_eliminar(struct hash_extendible *h, char *key);
void hashext_dispose(struct hash_extendible *h);

struct hashext_pagina *deserializar_pagina(char *buf);
char *serializar_pagina(struct hashext_pagina *p);

float get_ocupacion_hext(struct hash_extendible *h);


#endif //TAREA2_ALGORITMOS_HASH_EXTENDIBLE_H
