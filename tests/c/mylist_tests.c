#include "mylist.h"

struct node {
  int data;
  struct mylist_elem mylist_elem;
};

struct mylist ls;

int
main(void)
{
  mylist_init(&ls);
  int i;
  for (i = 0; i < 10; i++) {
    struct node* n = (struct node*)malloc(sizeof(struct node));
    mylist_insert(&ls, &n->mylist_elem);
  }
}
