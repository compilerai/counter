#include <stdlib.h>
#include "cutils/string_elem.h"
#include "cutils/chash.h"
#include <string.h>

unsigned
string_hash_func(struct myhash_elem const *a, void *aux)
{
  struct string_elem_t *e1;

  e1 = chash_entry(a, struct string_elem_t, h_elem);
  return myhash_string(e1->str);
}

bool
string_equal_func(struct myhash_elem const *a, struct myhash_elem const *b,
    void *aux)
{
  struct string_elem_t *e1, *e2;
  int ret;

  e1 = chash_entry(a, struct string_elem_t, h_elem);
  e2 = chash_entry(b, struct string_elem_t, h_elem);

  if (!strcmp(e1->str, e2->str)) {
    return true;
  }
  return false;
}

void
string_elem_free(struct myhash_elem *e, void *aux)
{
  struct string_elem_t *te;

  te = chash_entry(e, struct string_elem_t, h_elem);
  free(te->str);
  free(te);
}
