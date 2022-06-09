// A simple C++ program to introduce
// a linked list
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

//void* pmem_malloc(size_t);
 
struct Node {
    int* data;
    struct Node* next;
};

struct Node* pmem_head = NULL;

struct Node* head2 = NULL;

void insert(struct Node*& h, int d)
{
  //struct Node* n = (struct Node*)malloc(sizeof(struct Node));
  struct Node* pmem_n = new Node;
  struct Node* n = pmem_n;
  //if (!n) {
  //  return;
  //}
  //n->data = malloc(sizeof(int));
  n->data = new int;
  *n->data = d;
  n->next = h;
  h = n;
}

void insert2(struct Node*& h, int d)
{
  //struct Node* n = (struct Node*)malloc(sizeof(struct Node));
  struct Node* n = new Node;
  //if (!n) {
  //  return;
  //}
  //n->data = malloc(sizeof(int));
  n->data = new int;
  *n->data = d;
  n->next = h;
  h = n;
}
 
// Program to create a simple linked
// list with 3 nodes
int main()
{
  insert(pmem_head, 1);
  insert(pmem_head, 2);

  insert(head2, 3);
  insert2(head2, 4);

  struct Node* next;
  for (auto iter = head2; iter; iter = next) {
    next = iter->next;
    delete iter;
  }

  return 0;
}
