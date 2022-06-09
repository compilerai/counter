// A simple C++ program to introduce
// a linked list
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

//void* pmem_malloc(size_t);
 
struct Node {
    int data;
    struct Node* left, *right;
};

struct Node* pmem_root = nullptr;

struct Node* root2 = nullptr;

void insert_helper(struct Node* h, struct Node* n)
{
  if (h->data < n->data) {
    if (!h->left) {
      h->left = n;
    } else {
      insert_helper(h->left, n);
    }
  } else {
    if (!h->right) {
      h->right = n;
    } else {
      insert_helper(h->right, n);
    }
  }
}

void insert(struct Node*& h, int d)
{
  //struct Node* n = (struct Node*)malloc(sizeof(struct Node));
  struct Node* n = new Node;
  //if (!n) {
  //  return;
  //}
  //n->data = malloc(sizeof(int));
  n->data = d;
  n->left = n->right = nullptr;
  if (h == nullptr) {
    h = n;
    return;
  }
  insert_helper(h, n);
}

void insert_root2()
{
  insert(root2, 1);
  insert(root2, 2);
}

void insert_pmem_root()
{
  insert(pmem_root, 1);
  insert(pmem_root, 2);
}

int main()
{
  insert_root2();
  insert_pmem_root();
  return 0;
}
