#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(){
  printf("Los siguientes resultados son en BYTES\n\n");
  printf("Tamaño de un int: %i\n", (int) sizeof(int));
  printf("Tamaño de un char: %i\n", (int) sizeof(char));
  printf("Tamaño de un puntero: %i\n", (int) sizeof(void*));

  // Fuente: http://stackoverflow.com/questions/3080836/how-to-find-the-filesystem-block-size
  struct stat fi;
  stat("/", &fi);
  printf("Tamaño del bloque: %li\n", fi.st_blksize);

  return 0;
}
