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

static btree *_check_underflow(btree *nodo){
  if(nodo->num_elems >= (int)BTREE_ELEMS_NODO/2){
    _volcar_memext(nodo, nodo->indice);
    return NULL;
  }

  // si tengo overflow, me tienen que ayudar
  return nodo;
}

// Hacemos una "rotación" en sentido antihorario de los elementos. ie: right pasa su menor a padre, y padre pasa su
// indice-esimo a la derecha de left.
static void _btree_shiftAntihorario(struct btree_nodo* padre, struct btree_nodo* left, struct btree_nodo* right, int indice){
  char *oldkey;

  // ver _btree_shiftHorario para los comentarios, es lo mismo pero hacia el otro lado
  // almacenamos la antigua key del padre
  oldkey = strdup(padre->elementos[indice]);
  free(padre->elementos[indice]);

  // ponemos el + a la izq. de right en el padre y shifteamos los elems de right.
  padre->elementos[indice] = strdup(right->elementos[0]);
  free(right->elementos[0]);
  if(right->num_elems-1 > 0)
    memmove(right->elementos, right->elementos + 1, sizeof(char*)*(right->num_elems-1));
  right->num_elems--;

  // en left va la key antigua ya duplicada
  left->elementos[left->num_elems] = oldkey;
  left->num_elems++;

  // pasamos el primer hijo de right al ultimo hijo de left.
  if(right->num_hijos>0){
    left->hijos[left->num_hijos] = right->hijos[0];
    left->num_hijos++;

    // los hijos de right están desfasados, los arreglamos
    if(right->num_hijos-1 > 0)
      memmove(right->hijos, right->hijos + 1, sizeof(int)*(right->num_hijos-1));
    right->num_hijos--;
  }


  // left y right no tienen overflow
  _volcar_memext(left, left->indice);
  _volcar_memext(right, right->indice);
}


// Hacemos una "rotación" en sentido horario de los elementos. ie: left pasa su mayor a padre, y padre pasa su indice-esimo
// a la izquierda de right.
static void _btree_shiftHorario(struct btree_nodo* padre, struct btree_nodo* left, struct btree_nodo* right, int indice) {
  char *oldkey;

  // nos piteamos la llave del padre SIN HACER SHIFT DE LOS ELEMENTOS
  oldkey = strdup(padre->elementos[indice]);
  free(padre->elementos[indice]);

  // tomamos el derecho de left y lo pasamos al padre. left queda con un elemento menos.
  padre->elementos[indice] = strdup(left->elementos[left->num_elems-1]);
  left->num_elems--;

  // todavía existe el elem en left. como lo duplicamos hay que borrarlo.
  free(left->elementos[left->num_elems]);

  // el antiguo indice-esimo del padre ahora es el más a la izquierda de right
  memmove(right->elementos + 1, right->elementos, sizeof(char*)*right->num_elems);
  right->elementos[0] = oldkey; // ya está duplicada..
  right->num_elems++;

  // ahora veamos los hijos

  if(left->num_hijos>0){
    // el último hijo de left ahora es el primero de right
    if(right->num_hijos > 0)
      memmove(right->hijos + 1, right->hijos, sizeof(int)*right->num_hijos);
    right->hijos[0] = left->hijos[left->num_hijos-1];

    // left tiene un hijo menos y right tiene un hijo más.
    left->num_hijos--;
    right->num_hijos++;
  }

  // finalmente, left y right no pueden tener underflow.
  _volcar_memext(left, left->indice);
  _volcar_memext(right, right->indice);
}

// merge: tanto left o right quedarian con menos de B/2 elementos si hacemos un shift, de manera que
// metemos left, right y el indice-esimo del padre dentro de un nuevo y le decimos baibai a lo que había antes.
static void _btree_merge(struct btree_nodo* padre, struct btree_nodo* left, struct btree_nodo* right, int indice){
  char *oldkey;
  char archivo[64];
  int i_derecho;
  int k;

  // primero, chao con el indice-esimo del padre
  oldkey = strdup(padre->elementos[indice]);
  free(padre->elementos[indice]);

  // dejaremos todo en left
  // copiamos el elem del padre en left.
  left->elementos[left->num_elems] = oldkey;
  left->num_elems++;

  // copiamos todos los elementos de right
  for(k=0; k<right->num_elems; k++){
    left->elementos[left->num_elems + k] = strdup(right->elementos[k]);
  }

  // actualizamos el numero de elementos de left.
  left->num_elems += right->num_elems;

  // ahora copiamos todos los hijos de right a la derecha de left
  if(right->num_hijos > 0)
    memcpy(left->hijos + left->num_hijos, right->hijos, sizeof(int)*right->num_hijos);
  left->num_hijos += right->num_hijos;

  // el nodo nuevo está listo. hacemos shift de los elementos del padre.
  if(padre->num_elems - indice - 1 > 0)
    memmove(padre->elementos + indice, padre->elementos + indice + 1, sizeof(char*)*(padre->num_elems - indice - 1));


  // shift de los hijos tb. OJO con la cant. de hijos que movemos xq no está en términos de padre->num_hijos
  if(padre->num_hijos - indice - 2 > 0)
    memmove(padre->hijos + indice + 1, padre->hijos + indice + 2, sizeof(int)*(padre->num_hijos - indice - 2));

  // no nos olvidemos que el padre tiene 1 elem y 1 hijo menos.
  padre->num_elems--;
  padre->num_hijos--;

  // el hijo derecho ya no corre (xD)
  i_derecho = right->indice;
  btree_nodo_dispose(right);

  // y sería el famoso merge. ahora volcamos left en memoria
  _volcar_memext(left, left->indice);

  // no nos olvidemos de borrar el nodo derecho
  sprintf(archivo, "btree_nodo%i.data", i_derecho);
  remove(archivo);
}

static btree* _btree_borrar(const char *cadena, int indice, char *insertar_llave){
  //char* serializado;
  struct btree_nodo* nodo;
  struct btree_nodo *hijo;
  struct btree_nodo *hermano;
  int k;
  int porladerecha;

  if(indice == -1){
    return NULL;
  }

  porladerecha = 0;
  hijo = NULL;
  nodo=_get_nodo(indice);

  // parte de BAJADA
  // decido por donde bajar (ie: si me están pidiendo "borrow" o si simplemente estoy
  // tratando de encontrar el elemento
  // borrow es siempre k=0
  k = 0;

  while (k < nodo->num_elems && strcmp(cadena, nodo->elementos[k]) > 0) k++;

  if(k < nodo->num_elems && strcmp(cadena, nodo->elementos[k]) == 0){
    // este es el nodo donde lo pillé

    // si está en este nodo, borro el elemento siempre
    free(nodo->elementos[k]);

    // si este nodo es hoja, ya borré el elemento, me shifteo y calculo si tengo underflow
    // retorno NULL si no tengo o bien mi mismo nodo si sí tengo overflow.
    if(nodo->num_hijos <= 0){
      if(nodo->num_elems - k - 1 >  0) {
        memmove(nodo->elementos + k, nodo->elementos + k + 1, sizeof(char *) * (nodo->num_elems - k - 1));
      }
      nodo->num_elems--;

      // si no tengo overflow, gg
      return _check_underflow(nodo);
    }

    // si soy nodo interno y mi elem está aquí pido el más chico del árbol derecho
    // para insertarlo donde había borrado.
    nodo->elementos[k] = (char*)malloc(TAMANO_CADENA);

    // cuando llegue a la hoja SIEMPRE me voy a dar cuenta que el elem no está
    // y la hoja que se debe seguir me pasa el elemento.
    k++;
//    porladerecha = 1;
    hijo = _btree_borrar(cadena, nodo->hijos[k], nodo->elementos[k-1]);

//    if(k <= BTREE_ELEMS_NODO){
//      k++;
//      hijo = _btree_borrar(cadena, nodo->hijos[k], nodo->elementos[k-1]);
//    } else {
//      hijo = _btree_borrar(cadena, nodo->hijos[k], nodo->elementos[k]);
//    }

  } else {
    // el elemento NO ESTÁ

    // si soy hoja y me están pidiendo borrow, lo paso, calculo underflow y etc...
    if(nodo->num_hijos <= 0 && insertar_llave != NULL){
      //si me están pidiendo que pase mi ele más chico lo paso
      strcpy(insertar_llave, nodo->elementos[0]);
      free(nodo->elementos[0]);
      if(nodo->num_elems - 1 >  0) {
        memmove(nodo->elementos, nodo->elementos + 1, sizeof(char *) * (nodo->num_elems - 1));
        nodo->num_elems--;
      }

      return _check_underflow(nodo);
    }

    // si soy raiz pero no me están pidiendo borrow, nada que hacer, el elemento no está no más.
    if(nodo->num_hijos<=0 && insertar_llave == NULL) {
      btree_nodo_dispose(nodo);
      return NULL;
    }

    // si me están pidiendo borrow pero soy nodo interno bajo siempre x la izquierda
    if(nodo->num_hijos > 0 && insertar_llave != NULL) k=0;

    // finalmente, si lo anterior falla simplemente sigo bajando por el arbol
    hijo = _btree_borrar(cadena, nodo->hijos[k], insertar_llave);
  }

  // ahora voy de SUBIDA. Es bien penca el código, pero si se llegó aquí es porque estamos
  // en un nodo interno e hijo es NULL si no hay underflow o apunta a un hijo con underflow de haberlo.

  if(hijo == NULL){
    // sin underflow, pero potencialmente me pueden haber modificado
    _volcar_memext(nodo, nodo->indice);
    return NULL;
  }

  // tenemos underflow, ahora viene la parte cabrona.
  // Bueno, en verdad la parte cabrona es implementar los shifts y merges
  // El default es usar antihorario cuando pueda
//  if(porladerecha) k--;
  if(k+1 < nodo->num_hijos){
    // existe un nodo derecho
    hermano = _get_nodo(nodo->hijos[k+1]);

    // si el hermano tiene por lo menos B/2 + 1 elementos puedo hacer shift
    // del derecho a mi
    if(hermano->num_elems >= (int)BTREE_ELEMS_NODO/2 + 1) _btree_shiftAntihorario(nodo, hijo, hermano, k);
    else {
      // si no tiene, cagamos, hay que mergear
      _btree_merge(nodo, hijo, hermano, k);
    }
  } else {
    // no existe un nodo derecho, uso el izquierdo y compruebo lo mismo que en el caso anterior
    hermano = _get_nodo(nodo->hijos[k-1]);
    if(hermano->num_elems >= (int)BTREE_ELEMS_NODO/2 + 1) _btree_shiftHorario(nodo, hermano, hijo, k-1);
    else {
      _btree_merge(nodo, hermano, hijo, k-1);
    }
  }

  // tanto shift como merge vuelcan el nodo en mem secundaria
  // reviso si yo mismo tengo underflow y retorno.
  return _check_underflow(nodo);
}


void btree_borrar(const char *btree, const char *cadena){
  struct btree_nodo *b;
  char arch[64];
  int raiz = _get_raiz(btree);
  if(raiz < 0){
    return;
  }

  // la raiz no importa que tenga underflow
  b = _btree_borrar(cadena, raiz, NULL);
  if(b != NULL){
    // no me importa el underflow en la raiz, pero sí que me importa
    // una raíz con 0 elementos
    if(b->num_elems==0){
      _set_raiz(btree, b->hijos[0]);
      sprintf(arch, "btree_nodo%i.data", b->indice);
      btree_nodo_dispose(b);
      remove(arch);
      return;
    }
    _volcar_memext(b, b->indice);
  }
}
