#include <stdio.h>
#include <stdlib.h>

#include "btree.h"

int main() {
  struct btree_nodo *a23 = (struct btree_nodo *)malloc(sizeof(struct btree_nodo));
  btree_nodo_new(a23);

  a23 = btree_insertar(a23, 1);
  a23 = btree_insertar(a23, 2);
  a23 = btree_insertar(a23, 3);
  a23 = btree_insertar(a23, 4);
  a23 = btree_insertar(a23, 5);
  a23 = btree_insertar(a23, 6);
  a23 = btree_insertar(a23, 7);
  a23 = btree_insertar(a23, 8);
  a23 = btree_insertar(a23, 9);
  a23 = btree_insertar(a23, 10);

  printf("%i deberia ser igual a 1\n", btree_search(a23, 1));
  printf("%i deberia ser igual a 1\n", btree_search(a23, 2));
  printf("%i deberia ser igual a 1\n", btree_search(a23, 4));
  printf("%i deberia ser igual a 1\n", btree_search(a23, 8));
  printf("%i deberia ser igual a 1\n", btree_search(a23, 10));
  printf("%i deberia ser igual a 0\n", btree_search(a23, 912));

  return 0;
}