#include <stdio.h>
#include <stdlib.h>

//void* pmem_malloc(size_t);
 
struct Node {
    int data;
    struct Node* left, *right;
};

struct Node* pmem_root = NULL;

struct Node* root2 = NULL;

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

void insert(struct Node** h, int d)
{
  struct Node* n = (struct Node*)malloc(sizeof(struct Node));
  if (!n) {
    return;
  }
  n->data = d;
  n->left = n->right = NULL;
  if (*h == NULL) {
    *h = n;
    return;
  }
  insert_helper(*h, n);
}

void insert_root2()
{
  insert(&root2, 1);
  insert(&root2, 2);
}

void insert_pmem_root()
{
  insert(&pmem_root, 1);
  insert(&pmem_root, 2);
}

int main()
{
  insert_root2();
  insert_pmem_root();
  return 0;
}
