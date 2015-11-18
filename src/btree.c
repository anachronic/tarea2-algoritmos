#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "btree.h"
#include "parametros.h"
#include "extmem.h"
#include "fallos.h"

void btree_new(char *archivo) {
  FILE *f;
  int raiz;

  f = fopen(archivo, "wb");

  // marcar que es un arbol nuevo (raiz es -1)
  raiz = -1;

  fwrite(&raiz, sizeof(int), 1, f);

  // proximo indice de nodo disponible
  int proximo = 0;
  fwrite(&proximo, sizeof(int), 1, f);

  fclose(f);
}

static int _get_raiz(const char *archivo) {
  FILE *f;
  int raiz;

  if ((f = fopen(archivo, "rb")) == NULL)
    fallar("Error al abrir en B-Tree");

  fread(&raiz, sizeof(int), 1, f);
  fclose(f);
  return raiz;
}

static int _last_indice(const char *archivo) {
  FILE *f;
  int raiz;

  if ((f = fopen(archivo, "rb+")) == NULL)
    fallar("Error al abrir en B-Tree");

  fseek(f, sizeof(int), SEEK_SET);
  fread(&raiz, sizeof(int), 1, f);

  int nraiz = raiz + 1;
  fseek(f, sizeof(int), SEEK_SET);
  fwrite(&nraiz, sizeof(int), 1, f);
  fflush(f);

  fclose(f);
  return nraiz;
}

static void _set_raiz(const char *archivo, int indice) {
  FILE *f;

  if ((f = fopen(archivo, "rb+")) == NULL)
    fallar("Error al abrir en B-Tree");

  fseek(f, 0, SEEK_SET);
  fwrite(&indice, sizeof(int), 1, f);
  fflush(f);
  fclose(f);
}

static void _volcar_memext(struct btree_nodo *nodo, int indice_nodo) {
  char *buf;
  FILE *f;
  char archivo[64];

  sprintf(archivo, "btree_nodo%i.data", indice_nodo);
  f = fopen(archivo, "wb");

  buf = serializar_nodo(nodo);
  fwrite(buf, sizeof(char), B, f);
  fflush(f);
  free(buf);
  fclose(f);
  btree_nodo_dispose(nodo);
}

static struct btree_nodo *_get_nodo(int indice){
  char *buf;
  struct btree_nodo *n;
  char archivo[64];

  sprintf(archivo, "btree_nodo%i.data", indice);
  buf = recuperar_bloque(archivo, 0, 0);
  n = deserializar_nodo(buf);
  free(buf);

  return n;
}


static int _btree_search(const char *cadena, int indice) {
  struct btree_nodo *nodo;
  int k;
  int indice_hijo;

  if (indice == -1) {
    return 0; // NOT FOUND
  }

  nodo = _get_nodo(indice);

  k = 0;
  // se puede usar bsearch para esto. queda propuesto xD
  // http://www.cplusplus.com/reference/cstdlib/bsearch/
  while (k < nodo->num_elems && strcmp(cadena, nodo->elementos[k]) > 0) k++;

  if (k < nodo->num_elems && strcmp(cadena, nodo->elementos[k]) == 0) {
    btree_nodo_dispose(nodo);
    return 1;
  }
  if (nodo->num_hijos <= 0) {
    btree_nodo_dispose(nodo);
    return 0;
  }

  indice_hijo = nodo->hijos[k];
  btree_nodo_dispose(nodo);
  return _btree_search(cadena, indice_hijo);
}


int btree_search(const char *btree, const char *cadena) {
  int raiz = _get_raiz(btree);

  if (raiz < 0) {
    return -1;
  }

  return _btree_search(cadena, raiz);
}

static void _btree_insertar_elemento(struct btree_nodo *btree, const char *clave, int posicion) {
  if (posicion > btree->num_elems) {
    fprintf(stderr, "error de consistencia en nodo del b-tree\n");
    exit(-1);
  }

  if (posicion < btree->num_elems) {
    memmove(btree->elementos + (posicion + 1), btree->elementos + posicion,
            sizeof(char *) * (btree->num_elems - posicion));
  }

  btree->elementos[posicion] = (char *) malloc(TAMANO_CADENA);
  strcpy(btree->elementos[posicion], clave);
  btree->num_elems++;
}

static int _encontrar_candidato(struct btree_nodo *b, const char *clave) {
  int k = 0;
  if (b->num_elems == 0) return 0;

  while (k < b->num_elems && strcmp(clave, b->elementos[k]) > 0) k++;
  if (k < b->num_elems && strcmp(clave, b->elementos[k]) == 0) return -1;
  return k;
}

static void _btree_handle_overflow(const char *btree, struct btree_nodo *b, struct btree_nodo *hijo) {
  int medio;
  int old_elems;
  int indice_elemento_candidato;
  int k;
  struct btree_nodo *nuevo_derecho;

  medio = (int) BTREE_ELEMS_NODO / 2;
  indice_elemento_candidato = _encontrar_candidato(b, hijo->elementos[medio]);

  // creamos un nuevo nodo hijo.
  nuevo_derecho = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));

  nuevo_derecho->elementos = (char **) malloc(sizeof(char *) * BTREE_ELEMS_NODO);
  nuevo_derecho->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 1));
  nuevo_derecho->num_elems = medio;
  nuevo_derecho->num_hijos = 0; // puede ser que cambie
  nuevo_derecho->indice = _last_indice(btree);

  // copiamos los elementos "del lado derecho" al nuevo nodo.
  for (k = 0; k < nuevo_derecho->num_elems; k++) {
    nuevo_derecho->elementos[k] = (char *) malloc(sizeof(char) * TAMANO_CADENA);
    strcpy(nuevo_derecho->elementos[k], hijo->elementos[nuevo_derecho->num_elems + k + 1]);
    free(hijo->elementos[nuevo_derecho->num_elems + k + 1]);
  }
//  memcpy(nuevo_derecho->elementos, hijo->elementos + nuevo_derecho->num_elems + 1,
//         sizeof(char*) * nuevo_derecho->num_elems);

  old_elems = hijo->num_elems;
  hijo->num_elems = medio;

  // insertar el elemento del medio del hijo en el padre
  _btree_insertar_elemento(b, hijo->elementos[medio], indice_elemento_candidato);
  free(hijo->elementos[medio]);

  // antes de colgar, si el hijo tiene hijos hay que arreglarlos
  if (hijo->num_hijos > 0) {
    // los hijos izquierdos quedaran bien puestos al haber reducido hijo->num_elems
    // pero los hijos derechos debemos recolgarlos al nuevo nodo y re-setearle
    // los indices de sus hijos
    nuevo_derecho->num_hijos = nuevo_derecho->num_elems + 1;
    hijo->num_hijos = hijo->num_elems + 1;
    if (old_elems - medio + 1 > 0) {
      int offset = 0;
      if(nuevo_derecho->num_hijos % 2 == 0) offset = 1;
      memcpy(nuevo_derecho->hijos, hijo->hijos + medio + 1, sizeof(int) * (old_elems - medio + offset));
    }
  }

  // mover los hijos que no conciernen con el nuevo elemento insertado.
  if (b->num_hijos - indice_elemento_candidato - 1 > 0 && indice_elemento_candidato < BTREE_ELEMS_NODO - 1) {
    memmove(b->hijos + (indice_elemento_candidato + 2), b->hijos + (indice_elemento_candidato + 1),
            sizeof(int) * (b->num_hijos - indice_elemento_candidato - 1));
  }
  b->num_hijos = b->num_elems + 1;
  b->hijos[indice_elemento_candidato] = hijo->indice;
  b->hijos[indice_elemento_candidato + 1] = nuevo_derecho->indice;

  // guardar SI O SI el elemento nuevo en memoria
  _volcar_memext(nuevo_derecho, nuevo_derecho->indice);
//  btree_nodo_dispose(nuevo_derecho);
}

// retorna el nodo (en MEMORIA PRINCIPAL) que tiene overflow. Dicho nodo está en
// el HEAP. De manera que al mandarlo a memoria secundaria debe hacérsele FREE.
// De no haber overflow en un nodo esta func. retorna NULL
static struct btree_nodo *_btree_insertar(const char *btree, const char *cadena, int nodo) {
  int indice_hijo_candidato;
  int indice_elemento;
  struct btree_nodo *hijo;
  struct btree_nodo *padre; // el nodo-esimo nodo en btree.

  padre = _get_nodo(nodo);

  // este es el procedimiento para insertar el elemento en una HOJA
  if (padre->num_hijos == 0) {
    indice_elemento = _encontrar_candidato(padre, cadena);

    if (indice_elemento >= 0) {
      _btree_insertar_elemento(padre, cadena, indice_elemento);
    }

    // si hay overflow mi padre "me saca" mi overflow y él es responsable de mandarme a disco
    if (padre->num_elems > BTREE_ELEMS_NODO) return padre;
    else {
      // pero si no lo hay, yo mismo me mando a disco.
      _volcar_memext(padre, nodo);
      return NULL;
    }
  }

  // si este nodo es interno, busco el camino por donde bajar e inserto recursivamente ahí
  indice_hijo_candidato = _encontrar_candidato(padre, cadena);
  if (indice_hijo_candidato >= 0)
    hijo = _btree_insertar(btree, cadena, padre->hijos[indice_hijo_candidato]);
  else {
    _volcar_memext(padre, nodo);
    return NULL;
  }

  // si no hay overflow, serializo este nodo (correspondiente al "padre")
  // lo mando a disco
  // y retorno que yo "no tengo overflow"
  if (hijo == NULL) {
    _volcar_memext(padre, nodo);
    return NULL;
  }

  _btree_handle_overflow(btree, padre, hijo);

  // hasta ahora mi "hijo" no tiene overflow, así que lo guardo en memoria secundaria
  _volcar_memext(hijo, hijo->indice);

  // si después de esto yo (el padre) no tengo overflow, también me mando a mí mismo a
  // memoria secundaria y retorno null
  if (padre->num_elems <= BTREE_ELEMS_NODO) {
    _volcar_memext(padre, nodo);
    return NULL;
  }

  // pero si tengo overflow, mi padre (ie: el "abuelo") será responsable de mandarme
  // a memoria secundaria, porque en principal tengo un elemento más.
  return padre;
}

void btree_insertar(const char *btree, const char *cadena) {
  int raiz;
  struct btree_nodo *nodo;
  struct btree_nodo *hijo;
  char *clave;

  raiz = _get_raiz(btree);
  // Caso base: B-Tree vacío.
  if (raiz < 0) {
    clave = strdup(cadena);
    nodo = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));
    nodo->num_hijos = 0;
    nodo->num_elems = 1;
    nodo->indice = 0;
    nodo->elementos = (char **) malloc(sizeof(char *) * BTREE_ELEMS_NODO);
    nodo->elementos[0] = clave;
    nodo->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 1));

    _volcar_memext(nodo, nodo->indice);
    _set_raiz(btree, 0);
    return;
  }

  hijo = _btree_insertar(btree, cadena, raiz);

  // si hijo es NULL significa que el elemento quedó correctamente insertado
  // en algun nodo interno u hoja que NO ES LA RAÍZ y por lo tanto no hay nada
  // que hacer
  if (hijo == NULL) return;

  // pero si hijo es != NULL tengo overflow en la raíz y hay que crear
  // una nueva raíz con un elemento :O
  nodo = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));

  // la raiz siempre tendra de hijo al nodo que tiene overflow
  // y al hacer _btree_handle_overflow
  // tendremos el otro hijo creado.
  nodo->num_hijos = 0;
  nodo->num_elems = 0;
  nodo->elementos = (char **) malloc(sizeof(char *) * BTREE_ELEMS_NODO);
  nodo->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 1));

  _btree_handle_overflow(btree, nodo, hijo);

  // ahora sí que podemos volcar los nodos a memoria secundaria y terminar
  nodo->indice = _last_indice(btree);
  _set_raiz(btree, nodo->indice);
  _volcar_memext(hijo, hijo->indice);

  _volcar_memext(nodo, nodo->indice);
//  btree_nodo_dispose(nodo);
}


void btree_dispose(const char *archivo) {
  if (remove(archivo) != 0)
    perror("Error al eliminar el B-Tree");
}


char *serializar_nodo(struct btree_nodo *b) {
  char *buffer;
  int num_elems;
  int k;

  buffer = (char *) calloc(B, sizeof(char)); // inicializamos todos los bits a 0 (en partic. los elementos).
  num_elems = b->num_elems;

  memcpy(buffer, &num_elems, sizeof(int));
  memcpy(buffer + sizeof(int), &b->num_hijos, sizeof(int));
  memcpy(buffer + 2 * sizeof(int), &b->indice, sizeof(int));
  for (k = 0; k < num_elems; k++) {
    memcpy(buffer + 3 * sizeof(int) + k * TAMANO_CADENA * sizeof(char), b->elementos[k], TAMANO_CADENA);
  }

  // hasta aquí tenemos la cant. de elementos, todas las palabras y sólo faltan los hijos
  // ie: tenemos 12 + BTREE_ELEMS_NODO * 16 bytes.
  memcpy(buffer + 3 * sizeof(int) + BTREE_ELEMS_NODO * TAMANO_CADENA * sizeof(char),
         b->hijos, sizeof(int) * (BTREE_ELEMS_NODO + 1));

  // buffer listo
  return buffer;
}

struct btree_nodo *deserializar_nodo(char *buffer) {
  struct btree_nodo *nodo;
  int k;

  nodo = (struct btree_nodo *) malloc(sizeof(struct btree_nodo));

  memcpy(&nodo->num_elems, buffer, sizeof(int));
  memcpy(&nodo->num_hijos, buffer + sizeof(int), sizeof(int));
  memcpy(&nodo->indice, buffer + 2 * sizeof(int), sizeof(int));

  nodo->elementos = (char **) malloc((BTREE_ELEMS_NODO + 1) * sizeof(char *));
  for (k = 0; k < nodo->num_elems; k++) {
    nodo->elementos[k] = (char *) malloc(sizeof(char) * TAMANO_CADENA);
    memcpy(nodo->elementos[k], buffer + 3 * sizeof(int) + k * TAMANO_CADENA * sizeof(char), TAMANO_CADENA);
  }

  // finalmente, copiamos los indices de los hijos.
  nodo->hijos = (int *) malloc(sizeof(int) * (BTREE_ELEMS_NODO + 2));
  memcpy(nodo->hijos, buffer + 3 * sizeof(int) + BTREE_ELEMS_NODO * TAMANO_CADENA * sizeof(char),
         sizeof(int) * (BTREE_ELEMS_NODO + 1));

  return nodo;
}

void btree_nodo_dispose(struct btree_nodo *b) {
  int k;

  for (k = 0; k < b->num_elems; k++) {
    free(b->elementos[k]);
  }

  free(b->elementos);
  free(b->hijos);
  free(b);
}

/*
borrar(btree, cadena, indice){
  nodo=get_bloque(indice);
  buscar en llaves de nodo;
  if(lo encontramos en llaves){
    borrar indice;
    merge;
  }
  else{
    if(es hoja)
      no está :c
    else
      borrar(btree, cadena, indice_hijo);
  }
}

manejar_underflow(btree, indice){
  nodo=get_bloque(indice);
  if(nodo es hoja){
    borrar nodo;
    memmove archivo;
    padre.tirrarLlaveAHijo(nodo->indice); (también chequea si se produce overflow en hijo, y underflow en padre)
  }
  else{
    shiftLR o shiftRL
  }
}

merge(){
  if(es nodo interno){
    poner llave más a la derecha de hijo[k], o llave más a la izquierda de hijo[k+1]
    merge(hijo[k] o hijo[k+1]);
  }
  else{
    manejar_underflow;
  }
}
*/

static void _btree_shiftRL(struct btree_nodo* padre, struct btree_nodo* right, struct btree_nodo* left, int k){
  //Ponemos llave de padre al principio en arreglo de right
  char* llave_padre=padre->elementos[k-1];
  memmove(right->elementos+1, right->elementos, TAMANO_CADENA*right->num_elems);
  right->elementos[0]=llave_padre;
  right->num_elems++;
  //Ponemos última llave de arreglo de left en padre
  char* llave_left=left->elementos[left->num_elems-1];
  padre->elementos[k]=llave_left;
  //Ponemos último hijo de left como primer hijo de right (si es que existe)
  if(left->num_hijos>0){
    memmove(right->hijos+1, right->hijos, sizeof(int)*right->num_hijos);
    right->hijos[0]=left->hijos[left->num_hijos-1];
    right->num_hijos++;
  }
  //Achicamos left->elementos
  left->elementos[left->num_elems-1]=0;
  left->num_elems--;
  //Achicamos left->hijos
  left->hijos[left->num_hijos-1]=0;
  left->num_hijos--;

  _volcar_memext(padre, padre->indice);
  _volcar_memext(left, left->indice);
  _volcar_memext(right, right->indice);

  //Liberar nodos
  free(padre);
  free(left);
  free(right);
}

static void _btree_shiftLR(struct btree_nodo* padre, struct btree_nodo* left, struct btree_nodo* right, int k){
  //Ponemos llave de padre al final en arreglo de left
  char* llave_padre=padre->elementos[k];
  left->elementos[left->num_elems]=llave_padre;
  left->num_elems++;
  //Ponemos primera llave de arreglo de right en padre
  char* llave_right=right->elementos[0];
  padre->elementos[k]=llave_right;
  //Ponemos primer hijo de right como último hijo de left (si es que existe)
  if(right->num_hijos>0){
    left->hijos[left->num_hijos]=right->hijos[0];
    left->num_hijos++;
  }
  //Achicamos right->elementos
  char** achicado=(char**)malloc(sizeof(char*)*BTREE_ELEMS_NODO);
  memmove(achicado, right->elementos+1, TAMANO_CADENA*(right->num_elems-1));
  free(right->elementos);
  right->elementos=achicado;
  right->num_elems--;
  //Achicamos right->hijos
  int* achicado2=(int*)malloc(sizeof(int)*BTREE_ELEMS_NODO+1);
  memmove(achicado2, right->hijos+1, TAMANO_CADENA*(right->num_hijos-1));
  free(right->hijos);
  right->hijos=achicado2;
  right->num_hijos--;

  _volcar_memext(padre, padre->indice);
  _volcar_memext(left, left->indice);
  _volcar_memext(right, right->indice);

  //Liberar nodos
  free(padre);
  free(left);
  free(right);
}

static void _btree_merge(struct btree_nodo* padre, struct btree_nodo* left, struct btree_nodo* right, int k){
  //Sacamos llave de arreglo y lo achicamos
  char* llave=padre->elementos[k];
  char** achicado=(char**)malloc(sizeof(char*)*BTREE_ELEMS_NODO);
  memmove(achicado, padre->elementos, TAMANO_CADENA*k);
  memmove(achicado+k, padre->elementos+k+1, TAMANO_CADENA*(padre->num_elems-k-1));
  free(padre->elementos);
  padre->elementos=achicado;
  padre->num_elems--;

  //Mergear
  left->elementos[left->num_elems]=llave;
  memmove(left->elementos + 1 + left->num_elems, right->elementos, TAMANO_CADENA*right->num_elems);
  left->num_elems=left->num_elems+1+right->num_elems;
  //Achicar arreglo de hijos
  int* achicado2=(int*)malloc(sizeof(int)*BTREE_ELEMS_NODO+1);
  memmove(achicado2, padre->hijos, sizeof(int)*(k+1));
  memmove(achicado2+k+1, padre->hijos+k+2, sizeof(int)*(padre->num_hijos-k-2));
  free(padre->hijos);
  padre->hijos=achicado2;
  padre->num_hijos--;

  //Sobreescribir nodos en archivo
  //char* serializado=serializar_nodo(padre);
  //set_bloque(btree, serializado, padre->indice, sizeof(int));
  //free(serializado);
  _volcar_memext(padre, padre->indice);
  _volcar_memext(left, left->indice);

  //Borrar hijo derecho en archivo: borrar archivo, no actualizo índices :D
  char archivo[64];
  sprintf(archivo, "btree_nodo%i.data", right->indice);
  remove(archivo);

  //Liberar nodos
  free(padre);
  free(left);
  free(right);
}

static void _btree_handle_underflow(struct btree_nodo *padre, struct btree_nodo *hijo){
  //Están nodos escritos en disco
  //Solo arreglo un nivel del underflow, el padre del padre eventualmente verá si el padre quedó con underflow
  int k;
  struct btree_nodo* aux;

  for(k=0; k<padre->num_hijos; k++){
    if(padre->hijos[k]==hijo->indice) break;
  }
  if(k==padre->num_hijos-1){ //Último hijo, usar hijo a la izquierda
    //aux_s = recuperar_bloque(btree, padre->hijos[k-1], sizeof(int));
    //aux = deserializar_nodo(aux_s);
    //free(aux_s);
    aux=_get_nodo(padre->hijos[k-1]);
    //Decidir si uso shiftLR o merge
    if(aux->num_elems == BTREE_ELEMS_NODO/2)
      _btree_merge(padre, aux, hijo, k-1);
    else
      _btree_shiftRL(padre, hijo, aux, k);
  }
  else{ //Usar hijo a la derecha
    //aux_s = recuperar_bloque(btree, padre->hijos[k+1], sizeof(int));
    ///aux = deserializar_nodo(aux_s);
    //free(aux_s);
    aux=_get_nodo(padre->hijos[k+1]);
    //Decidir si uso shiftLR o merge
    if(aux->num_elems == BTREE_ELEMS_NODO/2)
      _btree_merge(padre, hijo, aux, k);
    else
      _btree_shiftLR(padre, hijo, aux, k);
  }
  btree_nodo_dispose(aux);
}

static void _btree_borrar_llave(struct btree_nodo* nodo, int k){
  //Borrar llave
  free(nodo->elementos[k]);
  //Si es hoja, achicamos arreglo, si no, subimos elemento más a la derecha de hijo[k]
  if(nodo->num_hijos<=0){
    char** achicado=(char**)malloc(sizeof(char*)*BTREE_ELEMS_NODO);
    memmove(achicado, nodo->elementos, TAMANO_CADENA*k);
		memmove(achicado+k, nodo->elementos+k+1, TAMANO_CADENA*(nodo->num_elems-k-1));
    printf("%s\n", nodo->elementos[0]);
    free(nodo->elementos);
    nodo->elementos=achicado;
    nodo->num_elems--;
  }
  else{
    //char* hijo_s = recuperar_bloque(btree, nodo_hijos[k], sizeof(int));
    //struct btree_nodo* hijo = deserializar_nodo(hijo_s);
    //free(hijo_s);
    struct btree_nodo* hijo=_get_nodo(nodo->hijos[k]);
    char* llave=hijo->elementos[hijo->num_elems-1];
    nodo->elementos[k]=llave;
    _btree_borrar_llave(hijo, hijo->num_elems-1);
    btree_nodo_dispose(hijo);
  }

  //Escribir en btree
  //char* serializado=serializar_nodo(nodo);
  //set_bloque(btree, serializado, nodo->indice, sizeof(int));
  //free(serializado);
  _volcar_memext(nodo, nodo->indice);

  //Chequear underflow
  /*if(nodo->num_elems < BTREE_ELEMS_NODO/2){
    _btree_handle_underflow(const char *btree, struct btree_nodo *b, struct btree_nodo *hijo)
  }*/

  printf("holi\n");
}

static btree* _btree_borrar(const char *cadena, int indice){
  //char* serializado;
  struct btree_nodo* nodo;
  int k;
  int indice_hijo;

  if(indice == -1){
    return NULL;
  }

  //serializado = recuperar_bloque(btree, indice, sizeof(int));
  //nodo = deserializar_nodo(serializado);
  //free(serializado);
  nodo=_get_nodo(indice);

  k = 0;
  while (k < nodo->num_elems && strcmp(cadena, nodo->elementos[k]) > 0) k++;

  if(k < nodo->num_elems && strcmp(cadena, nodo->elementos[k]) == 0){ //Encontramos elemento
    //borrar
    _btree_borrar_llave(nodo, k);
    if(nodo->num_elems < BTREE_ELEMS_NODO/2){
      return nodo;
    }
    else{
      btree_nodo_dispose(nodo);
      return NULL;
    }
  }
  else{
    if(nodo->num_hijos <= 0){ //No está :c
      btree_nodo_dispose(nodo);
      return NULL;
    }
    else{ //Buscar en hijo[k]
      indice_hijo = nodo->hijos[k];
      btree_nodo_dispose(nodo);
      struct btree_nodo* nodo_underflow = _btree_borrar(cadena, indice_hijo);
      if(nodo_underflow!=NULL){ //Hay underflow :c
        _btree_handle_underflow(nodo, nodo_underflow);
        //btree_nodo_dispose(nodo_underflow);
      }
    }
  }
  return NULL;
}

void btree_borrar(const char *btree, const char *cadena){
  int raiz = _get_raiz(btree);
  if(raiz < 0){
    return;
  }

  _btree_borrar(cadena, raiz);
}
