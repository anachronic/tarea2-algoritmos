#include <stdio.h>
#include <stdlib.h>

#include "btree.h"

int main() {
  btree_new(BTREE_FILE);

  btree_insertar(BTREE_FILE, "AAAAAAAAAAAAAAA");
  btree_insertar(BTREE_FILE, "GGGGGGGGGGGGGGG");

  printf("%i = 1 (solo A)\n", btree_search(BTREE_FILE, "AAAAAAAAAAAAAAA"));
  printf("%i = 1 (solo G)\n", btree_search(BTREE_FILE, "GGGGGGGGGGGGGGG"));
  printf("%i = 0 (solo T)\n", btree_search(BTREE_FILE, "TTTTTTTTTTTTTTT"));

  return 0;
}