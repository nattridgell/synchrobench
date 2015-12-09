
#include "versioned.h"

// Basic sequential testing
void print_sl(sl_intset_t *set) {
  printf("\nPrinting skiplist with size %d\n", sl_set_size(set));
  sl_node_t *curr = set->head;
  while (curr != NULL) {
    printf("%d:", curr->val);
    for (int i = 0; i < curr->current_height; ++i) {
      printf("-*");
    }
    printf("(curr %d, target %d)\n",curr->current_height, curr->target_height);
    curr = curr->next[0];
  }
}


int main() {
  printf("Successfully compiled and started.\n");
  sl_intset_t *set = sl_set_new();
  printf("Intset Successfully made.\n");
  printf("Empty does not contain val: %d\n", set_contains(set, 3));
  printf("Empty cannot delete val: %d\n", set_delete(set, 3));
  set_insert(set, 5);
  set_insert(set, 3);
  set_insert(set, 7);
  set_insert(set, 3);
  print_sl(set);
  set_delete(set, 1);
  set_delete(set, 200);
  set_delete(set, 3);
  print_sl(set);
  /* code */
  return 0;
}