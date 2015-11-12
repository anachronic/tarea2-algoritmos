#ifndef TAREA2_ALGORITMOS_EXTMEM_H
#define TAREA2_ALGORITMOS_EXTMEM_H

char* recuperar_bloque(const char* archivo, int bloque, size_t offset);
void append_bloque(const char *archivo, char *content);
void set_bloque(const char *archivo, char *content, int bloque, size_t offset);
int last_indice(const char *archivo);

#endif //TAREA2_ALGORITMOS_EXTMEM_H
