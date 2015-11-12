#include <stdlib.h>
#include <stdio.h>
#include "fallos.h"

void fallar(char *msg){
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(-1);
}