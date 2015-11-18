#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "hash_lineal.h"
#include "parametros.h"
#include "cadenas.h"
#include "extmem.h"



static void _dispose_pagina(struct hashlin_pagina *p){
  int k;

  for(k=0; k<p->num_elems; k++){
    free(p->hashes[k]);
    free(p->values[k]);
  }

  if(p->hashes != NULL) free(p->hashes);
  if(p->values != NULL) free(p->values);
}

static void _volcar_pagina(struct hashlin_pagina *p, int bucket){
  char archivo[64];
  char *buf;
  FILE *f;

  buf = serializar_pagina_lin(p);

  sprintf(archivo, "hashlin_nodo%i-%i.data", bucket, p->list_index);
  f = fopen(archivo, "wb");

  fwrite(buf, sizeof(char), B, f);
  fflush(f);
  fclose(f);
  free(buf);

  _dispose_pagina(p);
}

static struct hashlin_pagina *_get_pagina(int bucket, int index){
  struct hashlin_pagina *p;
  char archivo[64];
  char *buf;

  sprintf(archivo, "hashlin_nodo%i-%i.data", bucket, index);

  buf = recuperar_bloque(archivo, 0, 0);
  p = deserializar_pagina_lin(buf);
  free(buf);
  return p;
}

void hashlin_new(struct hash_lineal *h, int (*politica)(int,int)){
  h->num_buckets = 1;
  h->s = 1;
  h->politica = politica;
  h->num_elems = 0;

  struct hashlin_pagina p;
  p.num_elems = 0;
  p.list_index = 0;
  p.hashes = NULL;
  p.values = NULL;

  _volcar_pagina(&p, 0);
}


// retorna 0 si key existe en la pagina.
static int _check_exists(struct hashlin_pagina *p, char *key){
  int k;

  for (k=0; k<p->num_elems; k++){
    if(strcmp(key, p->hashes[k])==0) return 1;
  }
  return 0;
}


static int _insertar_pagina(struct hashlin_pagina *p, char *key, char *value){
  int k;

  for (k=0; k<p->num_elems; k++){
    if(strcmp(key, p->hashes[k])==0) return 0;
  }

  p->hashes[k] = strdup(key);
  p->values[k] = strdup(value);
  p->num_elems++;
  return 1;
}


/*
 * Inserta el par (K,V) en un bucket donde un bucket corresponde a una lista
 * de páginas que se corresponden sólo por nombre. Retorna 1 si se inserta, 0 si no.
 */
static void _insertar_bucket(struct hash_lineal *h, char *key, char *value, int bucket){
  struct hashlin_pagina *p;
  char archivo[64];
  int k;

  p = NULL;

  // asumimos que el bucket existe.
  // encontramos la pagina que pueda albergar el par (K,V)
  for(k=0; 1; k++){
    sprintf(archivo, "hashlin_nodo%i-%i.data", bucket, k);

    if(access(archivo, F_OK)==0){ // EXISTE
      p = _get_pagina(bucket, k);

      if(_check_exists(p, key)){
        // el elemento YA EXISTE
        _dispose_pagina(p);
        free(p);
        return;
      }

      if(p->num_elems >= NUM_ELEMS_PAGINA_LIN){
        // no me sirve esta pagina :(
        _dispose_pagina(p);
        free(p);
        continue;
      } else break;
    } else {
      p = NULL;
      break;
    }
  }

  if(p==NULL){
    p = (struct hashlin_pagina *)malloc(sizeof(struct hashlin_pagina));
    p->num_elems = 0;
    p->list_index = k;
    p->hashes = (char**)malloc(sizeof(char*) * NUM_ELEMS_PAGINA_LIN);
    p->values = (char**)malloc(sizeof(char*) * NUM_ELEMS_PAGINA_LIN);
  }

  if(_insertar_pagina(p, key, value)==1){
    h->num_elems++;
  }

  _volcar_pagina(p, bucket);
  free(p);
}

static void _expandir(struct hash_lineal *h, int bucket_viejo, int bucket_nuevo){
  struct hashlin_pagina *vpag;
  int k;

  struct hashlin_pagina pizq;
  pizq.num_elems = 0;
  pizq.list_index = 0;
  pizq.hashes = (char**)malloc(sizeof(char*)*NUM_ELEMS_PAGINA_LIN);
  pizq.values = (char**)malloc(sizeof(char*)*NUM_ELEMS_PAGINA_LIN);

  struct hashlin_pagina pder;
  pder.num_elems = 0;
  pder.list_index = 0;
  pder.hashes = (char**)malloc(sizeof(char*)*NUM_ELEMS_PAGINA_LIN);
  pder.values = (char**)malloc(sizeof(char*)*NUM_ELEMS_PAGINA_LIN);

  vpag = _get_pagina(bucket_viejo, 0);
  unsigned int rehash;

  for(k=0; k<vpag->num_elems; k++){
    rehash = DNAhash(vpag->hashes[k]) % 2*h->s;
    if(rehash == bucket_viejo){
      _insertar_pagina(&pizq, vpag->hashes[k], vpag->values[k]);
    } else if (rehash == bucket_nuevo) {
      _insertar_pagina(&pder, vpag->hashes[k], vpag->values[k]);
    } else {
      fprintf(stderr, "Error: hashes no calzan.\n");
    }
  }

  _dispose_pagina(vpag);
  free(vpag);

  char pagina_vieja[50];

  k=0;

  while(1){
    sprintf(pagina_vieja, "hashlin_nodo%i-%i.data", bucket_viejo, k++);
    if(access(pagina_vieja, F_OK) != 0) break;

    remove(pagina_vieja);
  }

  _volcar_pagina(&pizq, bucket_viejo);
  _volcar_pagina(&pder, bucket_nuevo);
}

void hashlin_insertar(struct hash_lineal *h, char *key, char *value){
  unsigned int hashval;
  int bucket;

  hashval = DNAhash(key);

  // De Wikipedia: https://en.wikipedia.org/wiki/Linear_hashing
  // h->inicial << h->nivel
  // corresponde a la expresion N * 2^L
  bucket = (int)hashval % h->s;
  if(bucket < h->num_buckets % h->s) bucket = (int) hashval % (2*h->s);

  _insertar_bucket(h, key, value, bucket);

  if(h->politica == NULL) return; // la politica nula es nunca expandir.

  if(h->politica(h->num_elems, 2*h->s) == 0){
    // no toca expandir
    return;
  }

  //hay que expandir!
  int bucket_nuevo = h->num_buckets;

  // se supone que h->step apunta al bucket a dividir.
  _expandir(h, bucket, bucket_nuevo);

  h->num_buckets++;
  if(h->num_buckets == 2*h->s){
    h->s = 2*h->s;
  }
}


static int _buscar_bucket(char *key, int bucket){
  struct hashlin_pagina *p;
  char archivo[64];
  int k;
  int existe = 0;

  for(k=0; 1; k++){
    sprintf(archivo, "hashlin_nodo%i-%i.data", bucket, k);

    if(access(archivo, F_OK) == -1) return 0;
    p = _get_pagina(bucket, k);
    existe = _check_exists(p, key);

    _dispose_pagina(p);
    free(p);

    if(existe) return 1;
  }
}

int hashlin_buscar(struct hash_lineal *h, char *key){
  unsigned int hashval;
  int bucket;

  hashval = DNAhash(key);

  bucket = (int)hashval % h->s;
  if(bucket < h->num_buckets % h->s) bucket = (int) hashval % (2*h->s);
  return _buscar_bucket(key, bucket);
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


