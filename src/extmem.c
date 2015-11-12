#include <stdio.h>
#include <stdlib.h>

#include "extmem.h"
#include "parametros.h"


/*
 * Recupera el bloque-ésimo bloque del archivo. Los bloques
 * se cuentan desde 0, y cada bloque tiene tamaño B, indicado
 * en el archivo parametros.h. La búsqueda de bloques se hace
 * con offset bytes de offset con respecto al principio del
 * programa. Por ejemplo, si se busca el bloque 0 con offset 4
 * obtendremos B elementos que empiezan desde el byte 4 (inclusive)
 *
 * De no existir el bloque, se termina el programa.
 *
 * El usuario de esta función es reponsable de liberar (free)
 * el buffer retornado!!!!!!
 */
char *recuperar_bloque(const char *archivo, int bloque, size_t offset) {
  FILE *f;
  char *buffer;

  f = fopen(archivo, "rb");
  buffer = (char *) malloc(B * sizeof(char));

  if (fseek(f, B * bloque + offset, SEEK_SET) != 0) {
    fprintf(stderr, "El bloque especificado no existe. Abortando.\n");
    exit(-1);
  }

  if (fread(buffer, sizeof(char), B, f) != B) {
    fprintf(stderr, "El archivo en memoria secundaria está corrupto\n");
    exit(-1);
  }

  fclose(f);

  return buffer;
}

void set_bloque(const char *archivo, char *content, int bloque, size_t offset){
  FILE *f;

  f = fopen(archivo, "rb+");
  fseek(f, offset + B * bloque, SEEK_SET);

  if(fwrite(content, sizeof(char), B, f) != B)
    fallar("No se puede setear el bloque!!");
  fflush(f);

  fclose(f);
}

/*
 * Inserta el contenido de content en el archivo en memoria
 * secundaria. Ojo, SIEMPRE se copiarán B elementos desde content,
 * sin importar su verdadero largo
 */
void append_bloque(const char *archivo, char *content) {
  FILE *f;

  f = fopen(archivo, "ab+");

  if (fwrite(content, sizeof(char), B, f) != B) {
    fprintf(stderr, "Error al escribir los datos al archivo, el source está corrupto\n");
    exit(-1);
  }

  fflush(f);
  fclose(f);
}


int last_indice(const char *archivo){
  FILE *f;
  long int size;

  f = fopen(archivo, "rb");
  fseek(f, 0, SEEK_END);
  size = ftell(f);

  return (int)(size - 1)/B;
}


















