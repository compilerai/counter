#include <stdlib.h>
#include "cutils/imap_elem.h"
#include "cutils/chash.h"

unsigned
imap_hash_func(struct myhash_elem const *a, void *aux)
{
  struct imap_elem_t *e1;

  e1 = chash_entry(a, struct imap_elem_t, h_elem);
  return (unsigned)e1->pc;
}

bool
imap_equal_func(struct myhash_elem const *a, struct myhash_elem const *b,
    void *aux)
{
  struct imap_elem_t *e1, *e2;
  //int ret;

  e1 = chash_entry(a, struct imap_elem_t, h_elem);
  e2 = chash_entry(b, struct imap_elem_t, h_elem);

  if (e1->pc == e2->pc) {
    return true;
  }
  return false;

}

void
imap_elem_free(struct myhash_elem *e, void *aux)
{
  struct imap_elem_t *te;
  te = chash_entry(e, struct imap_elem_t, h_elem);
  free(te);
}
