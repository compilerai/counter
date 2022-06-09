#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//#include "cpu-defs.h"
#include "insn/edge_table.h"

typedef struct node
{
  int nip1;
  int nip2;
  long long num_occurences;

  struct node *next;
} node_t;

struct edge_table_t
{
  int size;
  node_t **htable;
};

static int hash_fn (unsigned long nip)
{
  return (nip>>2);
}

void edge_table_init (edge_table_t **table_ptr, int size)
{
  edge_table_t *table;
  table = (edge_table_t *)malloc(sizeof(edge_table_t));
  assert(table);
  *table_ptr = table;

  table->size = size;
  table->htable = (node_t **)malloc(size*sizeof(node_t *));
  assert(table);
  memset(table->htable, 0x0, size*sizeof(node_t *));
}

void
edge_register(edge_table_t *table, src_ulong nip1, src_ulong nip2, long long num_occur)
{
  int i = hash_fn(nip1) % table->size;

  node_t *cur, *prev;

  for (prev=NULL, cur = table->htable[i];
      cur && (cur->nip1 < nip1 || (cur->nip1==nip1 && cur->nip2<nip2));
      prev=cur, cur=cur->next);

  if (cur && cur->nip1 == nip1 && cur->nip2 == nip2) {
    cur->num_occurences+=num_occur;
    return;
  }

  if (prev) {
    prev->next = (node_t *)malloc(sizeof(node_t));
    assert(prev->next);
    prev->next->next = cur;
    cur = prev->next;
  } else {
    table->htable[i] = (node_t *)malloc(sizeof(node_t));
    assert(table->htable[i]);
    table->htable[i]->next = cur;
    cur = table->htable[i];
  }

  cur->nip1 = nip1;
  cur->nip2 = nip2;
  cur->num_occurences = num_occur;
}

void edge_table_dump (FILE *fp, edge_table_t const *table, char const *dir)
{
  int i;
  for (i = 0; i < table->size; i++) {
    node_t *cur = table->htable[i];
    while (cur) {
      node_t *next = cur->next;
      if (((cur->nip1 == cur->nip2+4) || (cur->nip2 == cur->nip1 + 4))
          && !(next && next->nip1 == cur->nip1)) {
        cur = next;
        continue;
      }

      unsigned long nip1 = cur->nip1;
      while (cur && cur->nip1 == nip1) {
        fprintf (fp, "%x%s%x %lld\n", cur->nip1, dir, cur->nip2,
            cur->num_occurences);
        cur = cur->next;
      }
    }
  }
}

void edge_table_read (FILE *fp, edge_table_t *table)
{
  unsigned long nip1, nip2;
  long long num_occur;

  while (fscanf (fp, "%lx%*c%*c%lx %lld\n", &nip1, &nip2, &num_occur) == 3) {
    if (table) {
      edge_register (table, nip1, nip2, num_occur);
    }
  }
}

void *edge_table_find_next (edge_table_t const *table, void *iter,
    src_ulong nip1, src_ulong *nip2, long long *num_occur)
{
  if (iter) {
    node_t *cur = (node_t *)iter;
    assert (cur->nip1 == nip1);
    cur = cur->next;
    if (!cur || cur->nip1 != nip1) {
      return NULL;
    }
    *nip2 = cur->nip2;
    *num_occur = cur->num_occurences;
    return cur;
  }

  int i = hash_fn(nip1) % table->size;
  
  node_t *cur;
  for (cur = table->htable[i]; cur && cur->nip1 < nip1; cur = cur->next);
  if (!cur || cur->nip1 != nip1) {
    return NULL;
  }
  *nip2 = cur->nip2;
  *num_occur = cur->num_occurences;
  return cur;
}

void edge_table_free (edge_table_t *etable)
{
  free(etable->htable);
  free(etable);
}

long long
edge_table_query(edge_table_t const *table, src_ulong nip1, src_ulong nip2)
{
  int i = hash_fn(nip1) % table->size;

  node_t *cur, *prev;

  for (prev=NULL, cur = table->htable[i];
      cur && (cur->nip1 < nip1 || (cur->nip1==nip1 && cur->nip2<nip2));
      prev=cur, cur=cur->next);

  if (cur && cur->nip1 == nip1 && cur->nip2 == nip2) {
    return cur->num_occurences;
  }
  return 0;
}

