// A simple C program to introduce
// a linked list
#include <stdio.h>
#include <stdlib.h>

void* pmem_malloc(size_t);
 
struct Node {
    int* data;
    struct Node* next;
};

struct Node* head = NULL;

struct Node* head2 = NULL;

void insert(struct Node**h, int d)
{
  struct Node* n = (struct Node*)malloc(sizeof(struct Node));
  if (!n) {
    return;
  }
  n->data = malloc(sizeof(int));
  *n->data = d;
  n->next = *h;
  *h = n;
}


void insert2(struct Node**h, int d)
{
  struct Node* n = (struct Node*)pmem_malloc(sizeof(struct Node));
  if (!n) {
    return;
  }
  n->data = malloc(sizeof(int));
  *n->data = d;
  n->next = *h;
  *h = n;
}

 
// Program to create a simple linked
// list with 3 nodes
int main()
{
  insert(&head, 1);
  insert(&head, 2);
  //insert(&head2, 3);

  insert2(&head2, 4);

  return 0;
}
