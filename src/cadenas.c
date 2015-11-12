#include <stdlib.h>

#include "cadenas.h"
#include "parametros.h"


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