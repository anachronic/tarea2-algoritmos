#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hash_lineal.h"
#include "parametros.h"
#include "cadenas.h"



static void _dispose_pagina(struct hashlin_pagina *p){
  int k;

  for(k=0; k<p->num_elems; k++){
    free(p->hashes[k]);
    free(p->values[k]);
  }

  if(p->hashes != NULL) free(p->hashes);
  if(p->values != NULL) free(p->values);
}

static void _volcar_pagina(struct hashlin_pagina *p, int s){
  char archivo[64];
  char *buf;
  FILE *f;

  
  buf = serializar_pagina(p);
  
  sprintf(archivo, "hashlin_nodo%i-%i.data", s, p->list_index);
  f = fopen(archivo, "wb");
  
  fwrite(buf, sizeof(char), B, f);
  fflush(f);
  fclose(f);
  free(buf);

  _dispose_pagina(p);
}

void hashlin_new(struct hash_lineal *h){
  h->s = 1;
  h->buckets = 1;
  h->paginas = (int*)malloc(sizeof(int));

  // al principio el hash esta lleno:
  // h->paginas SIEMPRE tiene tamano s
}

static void _insertar_indice(struct hashlin_pagina *p, char *key, char *value, int indice){
  int k;
}












/*** SERIALIZACION ***/
struct hashlin_pagina *deserializar_pagina_lin(char *buf){
  int k;
  struct hashlin_pagina *p;

  p = (struct hashlin_pagina *)calloc(1, sizeof(struct hashlin_pagina));
  memcpy(&p->num_elems, buf, sizeof(int));
  memcpy(&p->list_index, buf + sizeof(int), sizeof(int));
  
  p->hashes = (char**)malloc(NUM_ELEMS_PAGINA_LIN * sizeof(char *));
  p->values = (char**)malloc(NUM_ELEMS_PAGINA_LIN * sizeof(char *));

  for(k=0; k<p->num_elems; k++){
    p->hashes[k] = (char*)malloc(TAMANO_CADENA);
    memcpy(p->hashes[k], buf+2*sizeof(int)+32*k, TAMANO_CADENA);

    p->values[k] = (char*)malloc(TAMANO_CADENA);
    memcpy(p->values[k], buf+2*sizeof(int)+32*k + TAMANO_CADENA, TAMANO_CADENA);
  }

  return p;
}

char *serializar_pagina_lin(struct hashlin_pagina *p){
  int k;
  char *buffer = (char *)calloc(B, sizeof(char));

  memcpy(buffer, &p->num_elems, sizeof(int));
  memcpy(buffer + sizeof(int), &p->list_index, sizeof(int));

  for(k=0; k<p->num_elems; k++){
    memcpy(buffer + 2*sizeof(int) + 32*k, p->hashes[k], TAMANO_CADENA);
    memcpy(buffer + 2*sizeof(int) + 32*k + TAMANO_CADENA,
	   p->values[k], TAMANO_CADENA);
  }

  return buffer;
}


