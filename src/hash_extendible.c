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

static void _dispose_pagina(struct hashext_pagina *p){
  int k;

  for(k=0; k<p->num_elems; k++){
    free(p->hashes[k]);
    free(p->valores[k]);
  }

  if(p->hashes != NULL) free(p->hashes);
  if(p->valores != NULL) free(p->valores);
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
  _dispose_pagina(p);
}

static void _insertar(struct hashext_pagina *p, char *key, char *val){
  int k;

  // adios duplicados!!
  for(k=0; k<p->num_elems; k++){
    if(strcmp(key, p->hashes[k])==0) return;
  }
  p->hashes[p->num_elems] = strdup(key);
  p->valores[p->num_elems] = strdup(val);

  p->num_elems++;
}

void hashext_new(struct hash_extendible *h){
  h->h = (struct hash_extendible_p *)malloc(sizeof(struct hash_extendible_p));

  h->h->indice_pagina = 0; //
  h->h->hizq = NULL;
  h->h->hder = NULL;

  struct hashext_pagina p;
  p.profundidad = 0;
  p.num_elems = 0;
  p.hashes = NULL;
  p.valores = NULL;

  _volcar_pagina(&p, 0);
  h->max_indice = 0;
}



static void _hashext_insertar(struct hash_extendible_p *h, char *key, char *valor, int profundidad,
                              struct hash_extendible *master){
  unsigned int hashval;
  unsigned int bit_hash;
  struct hash_extendible_p *hijo;
  int k;
  struct hashext_pagina *p;
  char archivo[50];

  // caso base: h es una hoja
  if(h->hder == NULL && h->hizq == NULL){
    p = _get_pagina(h->indice_pagina);
    if(p->num_elems == NUM_ELEMS_PAGINA){
      // overflow en la pagina!!
      struct hashext_pagina pizq;
      struct hashext_pagina pder;

      pizq.num_elems = 0;
      pizq.profundidad = profundidad+1;
      pizq.hashes = (char**)malloc(sizeof(char*)*NUM_ELEMS_PAGINA);
      pizq.valores = (char**)malloc(sizeof(char*)*NUM_ELEMS_PAGINA);

      pder.num_elems = 0;
      pder.profundidad = profundidad+1;
      pder.hashes = (char**)malloc(sizeof(char*)*NUM_ELEMS_PAGINA);
      pder.valores = (char**)malloc(sizeof(char*)*NUM_ELEMS_PAGINA);

      for(k=0; k<p->num_elems; k++){
        hashval = DNAhash(p->hashes[k]);
        bit_hash = hashval >> (profundidad + 1) & 1;
        if(bit_hash==0) _insertar(&pizq, p->hashes[k], p->valores[k]);
        else _insertar(&pder, p->hashes[k], p->valores[k]);
      }

      struct hash_extendible_p *nuevoizq = (struct hash_extendible_p *)malloc(sizeof(struct hash_extendible_p));
      struct hash_extendible_p *nuevoder = (struct hash_extendible_p *)malloc(sizeof(struct hash_extendible_p));

      nuevoizq->indice_pagina = ++master->max_indice;
      nuevoizq->num_elems = pizq.num_elems;
      nuevoizq->hizq = NULL;
      nuevoizq->hder = NULL;

      nuevoder->indice_pagina = ++master->max_indice;
      nuevoder->num_elems = pder.num_elems;
      nuevoder->hizq = NULL;
      nuevoder->hder = NULL;

      h->hizq = nuevoizq;
      h->hder = nuevoder;
      h->num_elems = 0;
      sprintf(archivo, "hashext_nodo%i.data", h->indice_pagina);

      _volcar_pagina(&pizq, nuevoizq->indice_pagina);
      _volcar_pagina(&pder, nuevoder->indice_pagina);

      _dispose_pagina(p);
      free(p);

      remove(archivo);

      // volver a llamar a esta misma funcion con los mismos parametros, pero esta vez
      // el nodo ya no es hoja, sino interno.
      _hashext_insertar(h, key, valor, profundidad, master);
      return;
    }

    _insertar(p, key, valor);
    h->num_elems++;
    _volcar_pagina(p, h->indice_pagina);
    free(p);
    return;
  }

  // estoy en un nodo interno
  hashval = DNAhash(key);

  hijo = (hashval >> (profundidad + 1) & 1) ? h->hder : h->hizq;
  _hashext_insertar(hijo, key, valor, profundidad+1, master);
}


void hashext_insertar(struct hash_extendible *h, char *key, void *val){
  _hashext_insertar(h->h, key, val, 0, h);
}

int _hashext_buscar(struct hash_extendible_p *h, char *key, int profundidad){
  struct hashext_pagina *p;
  struct hash_extendible_p *hijo;
  unsigned int hashval;
  int k;

  if(h->hizq == NULL && h->hder == NULL){
    // estoy en una hoja :D
    p = _get_pagina(h->indice_pagina);

    for(k=0; k<p->num_elems; k++){
      if(strcmp(key, p->hashes[k]) == 0){
        _dispose_pagina(p);
        free(p);
        return 1;
      }
    }

    _dispose_pagina(p);
    free(p);
    return 0;
  }

  hashval = DNAhash(key);
  hijo = (hashval >> (profundidad + 1) & 1) ? h->hder : h->hizq;

  return _hashext_buscar(hijo, key, profundidad+1);
}

int hashext_buscar(struct hash_extendible *h, char *key){
  return _hashext_buscar(h->h, key, 0);
}

char *serializar_pagina(struct hashext_pagina *p){
  int k;
  char *buffer = (char *)calloc(B, sizeof(char));

  memcpy(buffer, &p->num_elems, sizeof(int));
  memcpy(buffer + sizeof(int), &p->profundidad, sizeof(int));

  for(k=0; k<p->num_elems; k++){
    // 20*k viene de 16 (tamano de la key) + 4 (sizeof(unsigned int))
    // primero el hash
    memcpy(buffer + 2*sizeof(int) + 32*k, p->hashes[k], TAMANO_CADENA);
    // despues el string
    memcpy(buffer + 2*sizeof(int) + 32*k + TAMANO_CADENA,
	   p->valores[k], TAMANO_CADENA);
  }

  return buffer;
}

static void _hashext_dispose(struct hash_extendible_p *h){
  if(h==NULL) return;

  _hashext_dispose(h->hizq);
  _hashext_dispose(h->hder);

  free(h->hizq);
  free(h->hder);
}

void hashext_dispose(struct hash_extendible *h){
  _hashext_dispose(h->h);
  free(h->h);
}

struct hashext_pagina *deserializar_pagina(char *buf){
  int k;
  struct hashext_pagina *p;

  p = (struct hashext_pagina *)calloc(1, sizeof(struct hashext_pagina));
  memcpy(&p->num_elems, buf, sizeof(int));
  memcpy(&p->profundidad, buf + sizeof(int), sizeof(int));
  
  p->hashes = (char**)malloc(NUM_ELEMS_PAGINA * sizeof(char *));
  p->valores = (char**)malloc(NUM_ELEMS_PAGINA * sizeof(char *));

  for(k=0; k<p->num_elems; k++){
    // copio el hash
    p->hashes[k] = (char*)malloc(TAMANO_CADENA);
    memcpy(p->hashes[k], buf+2*sizeof(int)+32*k, TAMANO_CADENA);

    p->valores[k] = (char*)malloc(TAMANO_CADENA);
    memcpy(p->valores[k], buf+2*sizeof(int)+32*k + TAMANO_CADENA, TAMANO_CADENA);
  }

  return p;
}
