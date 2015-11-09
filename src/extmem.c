#include <stdio.h>
#include <stdlib.h>

#include "extmem.h"
#include "parametros.h"


/*
 * Recupera el bloque-ésimo bloque del archivo. Los bloques
 * se cuentan desde 0, y cada bloque tiene tamaño B, indicado
 * en el archivo parametros.h
 *
 * De no existir el bloque, se termina el programa.
 */
char *recuperar_bloque(const char *archivo, int bloque) {
  FILE *f;
  char *buffer;

  f = fopen(archivo, "rb");
  buffer = (char *) malloc(B * sizeof(char));

  if (fseek(f, TAMANO_BLOQUE * bloque, SEEK_SET) != 0) {
    fprintf(stderr, "El bloque especificado no existe. Abortando.\n");
    exit(-1);
  }

  if (fread(buffer, sizeof(char), TAMANO_BLOQUE, f) != TAMANO_BLOQUE) {
    fprintf(stderr, "El archivo en memoria secundaria está corrupto\n");
    exit(-1);
  }

  fclose(f);

  return buffer;
}

/*
 * Inserta el contenido de content en el archivo en memoria
 * secundaria. Ojo, SIEMPRE se copiarán B elementos desde content,
 * sin importar su verdadero largo
 */
void append_bloque(const char *archivo, char *content) {
  FILE *f;

  f = fopen(archivo, "ab+");

  if (fwrite(content, sizeof(char), TAMANO_BLOQUE, f) != TAMANO_BLOQUE) {
    fprintf(stderr, "Error al escribir los datos al archivo, el source está corrupto\n");
    exit(-1);
  }

  fclose(f);
}