#include <stdlib.h>
#include <string.h>

#include "cadenas.h"
#include "parametros.h"


static long intrand(long max){
  return (long)(drand48() * max);
}


char base_rand(){
  double r = drand48();

  if(r < 0.25) return 'A';
  else if (r < 0.5) return 'T';
  else if (r < 0.75) return 'G';
  else return 'C';
}

// Para usar esta función se debe PROVEER un array de chars
// de tamaño AL MENOS 16
void cadena_rand(char *buffer){
  int k;

  for(k=0; k<TAMANO_CADENA-1; k++){
    buffer[k] = base_rand();
  }
  buffer[k] = 0;
}

char *get_random_from_array(char **arr, long size){
  long index;

  index = intrand(size);
  return arr[index];
}


void crear_cadenas(struct cadena_struct *cs, int size){
  cs->total_alloc = size;
  cs->num_elems = size;
  cs->cadenas = (char**)malloc(sizeof(char*)*size);

  int k;
  for(k=0; k<cs->num_elems; k++){
    cs->cadenas[k] = (char*)malloc(TAMANO_CADENA);
    cadena_rand(cs->cadenas[k]);
  }
}

char *get_cadena(struct cadena_struct *cs, int k){
  return cs->cadenas[k];
}

void eliminar_cadena(struct cadena_struct *cs, int k){
  if(cs == NULL || cs->cadenas == NULL || cs->cadenas[k] == NULL) return;

  free(cs->cadenas[k]);
  if(cs->num_elems - k - 1 > 0)
    memmove(cs->cadenas + k, cs->cadenas + k + 1, sizeof(char*)*(cs->num_elems - k - 1));
  cs->num_elems--;
}

void dispose_cadenas(struct cadena_struct *cs){
  int k;

  if(cs!=NULL){
    if(cs->cadenas != NULL){
      for(k=0; k<cs->num_elems; k++){
        free(cs->cadenas[k]);
      }
      free(cs->cadenas);
    }
  }
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
