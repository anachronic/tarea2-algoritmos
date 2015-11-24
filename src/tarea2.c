#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "btree.h"
#include "cadenas.h"
#include "hash_lineal.h"
#include "hash_extendible.h"
#include "parametros.h"
#include "extmem.h"

unsigned long hlin_accesos = 0;
unsigned long hext_accesos = 0;
unsigned long tree_accesos = 0;

int politica1(int num_elems, int num_buckets){
  if(1.0*num_elems/num_buckets > 1.5*NUM_ELEMS_PAGINA_LIN) return 1;
  if(1.0*num_elems/num_buckets < 0.83*NUM_ELEMS_PAGINA_LIN) return -1;
  return 0;
}

int politicatest(int a, int b){
  if(a==160) return 1;
  if(a > 175) return -1;
  return 0;
}

static long intrand(long max){
  return (long)(drand48() * max);
}


int main(int argc, char **argv) {
  srand48(getpid());

  // borrar los vestigios de algún experimento anterior
  system("rm *.data");

  btree_new(BTREE_FILE);

  FILE *fbtree = fopen("btree_resultados_insercion.dat", "wt");
  fprintf(fbtree, "#milestone: Número de elementos con los que se pide las pruebas\n");
  fprintf(fbtree, "#I/O acumulado: cantidad ACUMULADA de lecturas + escrituras totales en las ops. realizadas\n");
  fprintf(fbtree, "#p ocupacion: el porcentaje de ocupación de la estructura de datos y sus archivos\n");
  fprintf(fbtree, "#milestone\tI/O acumulado\tp ocupacion\n");
  FILE *fhext = fopen("hext_resultados_insercion.dat", "wt");
  fprintf(fhext, "#milestone: Número de elementos con los que se pide las pruebas\n");
  fprintf(fhext, "#I/O acumulado: cantidad ACUMULADA de lecturas + escrituras totales en las ops. realizadas\n");
  fprintf(fhext, "#p ocupacion: el porcentaje de ocupación de la estructura de datos y sus archivos\n");
  fprintf(fhext, "#milestone\tI/O acumulado\tp ocupacion\n");
  FILE *fhlin = fopen("hlin_resultados_insercion.dat", "wt");
  fprintf(fhlin, "#milestone: Número de elementos con los que se pide las pruebas\n");
  fprintf(fhlin, "#I/O acumulado: cantidad ACUMULADA de lecturas + escrituras totales en las ops. realizadas\n");
  fprintf(fhlin, "#p ocupacion: el porcentaje de ocupación de la estructura de datos y sus archivos\n");
  fprintf(fhlin, "#milestone\tI/O acumulado\tp ocupacion\n");

  struct hash_extendible hext;
  hashext_new(&hext);

  struct hash_lineal hlin;
  hashlin_new(&hlin, politica1);

//  int strings = TOTAL_CADENAS;
  int buscar = BUSCAR_CADENAS;
  int borrar = BORRAR_CADENAS;

//  char **aleatorias = (char**)malloc(sizeof(char*)*strings);

  struct cadena_struct cs;
  crear_cadenas(&cs, TOTAL_CADENAS);
  int k = 0;
  int potencia;

  printf("Insertando %i cadenas en cada estructura\n", TOTAL_CADENAS);
  for(k=0; k<cs.num_elems; k++){
    btree_insertar(BTREE_FILE, get_cadena(&cs, k));
    hashext_insertar(&hext, get_cadena(&cs, k), get_cadena(&cs, k));
    hashlin_insertar(&hlin, get_cadena(&cs, k), get_cadena(&cs, k));

    for (potencia=15; potencia<=20; potencia++){
      if(k+1==(1<<potencia)){
        fprintf(fbtree, "%i\t%lu\t%f\n", k+1, tree_accesos, get_ocupacion_btree(BTREE_FILE));
        fprintf(fhext, "%i\t%lu\t%f\n", k+1, hext_accesos, get_ocupacion_hext(&hext));
        fprintf(fhlin, "%i\t%lu\t%f\n", k+1, hlin_accesos, get_ocupacion_hlin(&hlin));
      }
    }
  }

  fclose(fbtree);
  fclose(fhext);
  fclose(fhlin);

  tree_accesos = 0;
  hlin_accesos = 0;
  hext_accesos = 0;

  fbtree = fopen("btree_resultados_busqueda.dat", "wt");
  fprintf(fbtree, "#milestone: Número de elementos con los que se pide las pruebas\n");
  fprintf(fbtree, "#I/O acumulado: cantidad ACUMULADA de lecturas + escrituras totales en las ops. realizadas\n");
  fprintf(fbtree, "#p ocupacion: el porcentaje de ocupación de la estructura de datos y sus archivos\n");
  fprintf(fbtree, "#milestone\tI/O acumulado\tp ocupacion\n");
  fhext = fopen("hext_resultados_busqueda.dat", "wt");
  fprintf(fhext, "#milestone: Número de elementos con los que se pide las pruebas\n");
  fprintf(fhext, "#I/O acumulado: cantidad ACUMULADA de lecturas + escrituras totales en las ops. realizadas\n");
  fprintf(fhext, "#p ocupacion: el porcentaje de ocupación de la estructura de datos y sus archivos\n");
  fprintf(fhext, "#milestone\tI/O acumulado\tp ocupacion\n");
  fhlin = fopen("hlin_resultados_busqueda.dat", "wt");
  fprintf(fhlin, "#milestone: Número de elementos con los que se pide las pruebas\n");
  fprintf(fhlin, "#I/O acumulado: cantidad ACUMULADA de lecturas + escrituras totales en las ops. realizadas\n");
  fprintf(fhlin, "#p ocupacion: el porcentaje de ocupación de la estructura de datos y sus archivos\n");
  fprintf(fhlin, "#milestone\tI/O acumulado\tp ocupacion\n");

  buscar = cs.num_elems;
  printf("Buscando %i cadenas.\n", buscar);

  for(k=0; k<buscar; k++){
    btree_search(BTREE_FILE, get_cadena(&cs, k));
    hashext_buscar(&hext, get_cadena(&cs, k));
    hashlin_buscar(&hlin, get_cadena(&cs, k));

    for (potencia=15; potencia<=20; potencia++){
      if(k+1==(1<<potencia)){
        fprintf(fbtree, "%i\t%lu\t%f\n", k+1, tree_accesos, get_ocupacion_btree(BTREE_FILE));
        fprintf(fhext, "%i\t%lu\t%f\n", k+1, hext_accesos, get_ocupacion_hext(&hext));
        fprintf(fhlin, "%i\t%lu\t%f\n", k+1, hlin_accesos, get_ocupacion_hlin(&hlin));
      }
    }
  }

  fclose(fbtree);
  fclose(fhext);
  fclose(fhlin);

  tree_accesos = 0;
  hlin_accesos = 0;
  hext_accesos = 0;

  fbtree = fopen("btree_resultados_eliminacion.dat", "wt");
  fprintf(fbtree, "#milestone: Número de elementos con los que se pide las pruebas\n");
  fprintf(fbtree, "#I/O acumulado: cantidad ACUMULADA de lecturas + escrituras totales en las ops. realizadas\n");
  fprintf(fbtree, "#p ocupacion: el porcentaje de ocupación de la estructura de datos y sus archivos\n");
  fprintf(fbtree, "#milestone\tI/O acumulado\tp ocupacion\n");
  fhext = fopen("hext_resultados_eliminacion.dat", "wt");
  fprintf(fhext, "#milestone: Número de elementos con los que se pide las pruebas\n");
  fprintf(fhext, "#I/O acumulado: cantidad ACUMULADA de lecturas + escrituras totales en las ops. realizadas\n");
  fprintf(fhext, "#p ocupacion: el porcentaje de ocupación de la estructura de datos y sus archivos\n");
  fprintf(fhext, "#milestone\tI/O acumulado\tp ocupacion\n");
  fhlin = fopen("hlin_resultados_eliminacion.dat", "wt");
  fprintf(fhlin, "#milestone: Número de elementos con los que se pide las pruebas\n");
  fprintf(fhlin, "#I/O acumulado: cantidad ACUMULADA de lecturas + escrituras totales en las ops. realizadas\n");
  fprintf(fhlin, "#p ocupacion: el porcentaje de ocupación de la estructura de datos y sus archivos\n");
  fprintf(fhlin, "#milestone\tI/O acumulado\tp ocupacion\n");

  borrar = cs.num_elems;
  printf("Eliminando %i cadenas\n", borrar);

  while(cs.num_elems>0){
    k=intrand(cs.num_elems);
    hashext_eliminar(&hext, get_cadena(&cs, k));
    hashlin_eliminar(&hlin, get_cadena(&cs, k));
    btree_borrar(BTREE_FILE, get_cadena(&cs, k));

    for (potencia=15; potencia<=20; potencia++){
      if(cs.num_elems==(1<<potencia)){
        fprintf(fbtree, "%i\t%lu\t%f\n", 1<<potencia, tree_accesos, get_ocupacion_btree(BTREE_FILE));
        fprintf(fhext, "%i\t%lu\t%f\n", 1<<potencia, hext_accesos, get_ocupacion_hext(&hext));
        fprintf(fhlin, "%i\t%lu\t%f\n", 1<<potencia, hlin_accesos, get_ocupacion_hlin(&hlin));
      }
    }
    eliminar_cadena(&cs, k);
  }

  fclose(fbtree);
  fclose(fhext);
  fclose(fhlin);


  dispose_cadenas(&cs);

//  for(k=0; k<strings; k++) free(aleatorias[k]);

//  free(aleatorias);
  hashext_dispose(&hext);



  return 0;
}
