#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hash_extendible.h"
#include "hash_lineal.h" // para la funcion de hash
#include "parametros.h"
#include "extmem.h"



static struct hashext_pagina *_get_pagina(int indice){
  char *buf;
  struct hashext_pagina *p;
  char archivo[50];

  sprintf(archivo, "hashext_nodo%i.data", indice);

  buf = recuperar_bloque(archivo, 0, 0);
  p = deserializar_pagina(buf);
  free(buf);

  return p;
}

static void _volcar_pagina(struct hashext_pagina *p, int indice){
  char *buf;
  char archivo[50];
  FILE *f;

  sprintf(archivo, "hashext_nodo%i.data", indice);

  buf = serializar_pagina(p);
  f = fopen(archivo, "wb");

  // mandamos el buffer entero de 1.
  fwrite(buf, sizeof(char), B, f);
  fflush(f);
  fclose(f);

  free(buf);
}

void hashext_new(struct hash_extendible *h){
  

  h->h->indice_pagina = 0;
  h->h->hizq = NULL;
  h->h->hder = NULL;
}


void hashext_insertar(struct hash_extendible *h, char *key, void *val){
  unsigned int hashval;
  char nombre_pagina[50];
  // caso base: h es una hoja
  if(h->h->hder == NULL && h->h->hizq == NULL){
    hashval = DNAhash(key);
    
    // por ahora (y solo por experimentar insertaremos directo
    //struct hashext_pagina *p = _get_pagina(
    
  }
}


char *serializar_pagina(struct hashext_pagina *p){
  int k;
  char *buffer = (char *)calloc(B, sizeof(char));

  memcpy(buffer, &p->num_elems, sizeof(int));

  for(k=0; k<p->num_elems; k++){
    // 20*k viene de 16 (tamano de la key) + 4 (sizeof(unsigned int))
    // primero el hash
    memcpy(buffer + sizeof(int) + 20*k, &p->hashes[k], sizeof(unsigned int));
    // despues el string
    memcpy(buffer + sizeof(int) + 20*k + sizeof(unsigned int),
	   p->valores[k], TAMANO_CADENA);
  }

  return buffer;
}

struct hashext_pagina *deserializar_pagina(char *buf){
  int k;
  struct hashext_pagina *p;

  p = (struct hashext_pagina *)calloc(1, sizeof(struct hashext_pagina));
  memcpy(&p->num_elems, buf, sizeof(int));
  
  p->hashes = (unsigned int *)malloc(NUM_ELEMS_PAGINA * sizeof(unsigned int));
  p->valores = (char**)malloc(NUM_ELEMS_PAGINA * sizeof(char *));

  for(k=0; k<p->num_elems; k++){
    // copio el hash
    memcpy(&p->hashes[k], buf+sizeof(int)+20*k, sizeof(unsigned int));

    p->valores[k] = (char*)malloc(TAMANO_CADENA);
    memcpy(&p->valores[k], buf+sizeof(int)+20*k + sizeof(unsigned int), TAMANO_CADENA);
  }

  return p;
}
