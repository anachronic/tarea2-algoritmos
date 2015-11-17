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
