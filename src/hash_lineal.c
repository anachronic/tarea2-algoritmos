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

void _rehash_bucket(struct hash_lineal *h, int bucket, int bucket_nuevo){
  int k, i, j;
  int total;
  char archivo_viejo[64];
  char archivo_nuevo[64];


  for(k=0; 1; k++){
    sprintf(archivo_viejo, "hashlin_nodo%i-%i.data", bucket, k);

    if(access(archivo_viejo, F_OK)==0){
      sprintf(archivo_nuevo, "hashlin_temp%i.data", k);
      rename(archivo_viejo, archivo_nuevo);
    } else break;
  }

  // ahora me toca rehashear.
  total = 0;
  struct hashlin_pagina *p;
  char *buf;
  unsigned int rehash;

  for(i=0; i<k; i++) {
    sprintf(archivo_viejo, "hashlin_temp%i.data", i);

    buf = recuperar_bloque(archivo_viejo, 0, 0);
    p = deserializar_pagina_lin(buf);
    free(buf);

    total += p->num_elems;

    for(j=0; j<p->num_elems; j++){
      rehash = DNAhash(p->hashes[j]) % (2*h->s);
      if(rehash == bucket){
        // dejarlo en el bucket que ya estaba
        _insertar_bucket(h, p->hashes[j], p->values[j], bucket);
      } else if(rehash == bucket_nuevo){
        // dejarlo en el nuevo bucket
        _insertar_bucket(h, p->hashes[j], p->values[j], bucket_nuevo);
      } else {
        printf("Hashes esperados\t%u\t%u\t\tObtenido:%u\n", bucket, bucket_nuevo, rehash);
      }
    }

    remove(archivo_viejo);
    _dispose_pagina(p);
    free(p);
  }

  // le restamos al total "lo que reinsertamos"
  h->num_elems -= total;
}


void hashlin_insertar(struct hash_lineal *h, char *key, char *value){
  unsigned int hashval;
  int bucket;

  hashval = DNAhash(key);

  bucket = (int)hashval % h->s;
  if(bucket < h->num_buckets % h->s) bucket = (int) hashval % (2*h->s);

  _insertar_bucket(h, key, value, bucket);

  if(h->politica == NULL) return; // la politica nula es nunca expandir.

  if(h->politica(h->num_elems, 2*h->s) <= 0){
    // no toca expandir
    return;
  }

  //hay que expandir!
  int bucket_nuevo = h->num_buckets;

  // se supone que h->step apunta al bucket a dividir.
  _rehash_bucket(h, h->num_buckets - h->s, bucket_nuevo);

  h->num_buckets++;
  if(h->num_buckets == 2*h->s){
    h->s = 2*h->s;
  }
}

static int _eliminar_from_pagina(struct hash_lineal *h, struct hashlin_pagina *p, char *key){
  int k;

  for(k=0; k<p->num_elems; k++){
    if(strcmp(key, p->hashes[k])==0){
      free(p->hashes[k]);
      free(p->values[k]);

      p->num_elems--;
      h->num_elems--;

      if(p->num_elems == 0) return 1;

      //si quedan elemens, shift a la izq a esos elems
      if(p->num_elems - k > 0){
        memmove(p->hashes + k, p->hashes + k + 1, sizeof(char *) * (p->num_elems - k));
        memmove(p->values + k, p->values + k + 1, sizeof(char *) * (p->num_elems - k));
      }
      return 1;
    }
  }

  return 0;
}

// elimina el par (K,V) asociado a key en el bucket si es que existe
static void _eliminar_bucket(struct hash_lineal *h, char *key, int bucket){
  struct hashlin_pagina *p;
  char archivo[64];
  int borrado;
  int k;

  borrado = 0;

  for(k=0; 1; k++){
    sprintf(archivo, "hashlin_nodo%i-%i.data", bucket, k);

    if(access(archivo, F_OK)==0){
      p = _get_pagina(bucket, k);

      borrado = _eliminar_from_pagina(h, p, key);
      if(borrado){
        _volcar_pagina(p, bucket);
        free(p);
        break;
      } else {
        _dispose_pagina(p);
        free(p);
      }
    } else break;
  }
}

static void _contraer(struct hash_lineal *h, int bucket_viejo, int bucket_nuevo){
  int k, j;
  int total;
  char *buf;
  char archivo[64];
  struct hashlin_pagina *p;

  total = 0;
  strcpy(archivo, "");

  for(k=0; 1; k++){
    if(strcmp(archivo, "")!=0) remove(archivo);
    sprintf(archivo, "hashlin_nodo%i-%i.data", bucket_viejo, k);

    if(access(archivo, F_OK) != 0) break;

    buf = recuperar_bloque(archivo, 0, 0);
    p = deserializar_pagina_lin(buf);
    free(buf);

    total += p->num_elems;

    for(j=0; j<p->num_elems; j++){
      _insertar_bucket(h, p->hashes[j], p->values[j], bucket_nuevo);
    }

    _dispose_pagina(p);
    free(p);
  }

  // le restamos al total "lo que reinsertamos"
  h->num_elems -= total;
}

void hashlin_eliminar(struct hash_lineal *h, char *key){
  unsigned int hashval;
  int bucket;

  hashval = DNAhash(key);
  bucket = (int)hashval % h->s;
  if(bucket < h->num_buckets % h->s) bucket = (int)hashval % (2*h->s);

  _eliminar_bucket(h, key, bucket);

  if(h->politica == NULL) return; // politica nula = nunca contraer.
  if(h->politica(h->num_elems, h->num_buckets) >= 0) return; // No nos toca contraer

  if(h->num_buckets == 1) return; //no puedo contraer...

  if(h->num_buckets == h->s){
    h->s = h->s/2;
  }
  h->num_buckets--;
  _contraer(h, h->num_buckets, h->num_buckets - h->s);
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


