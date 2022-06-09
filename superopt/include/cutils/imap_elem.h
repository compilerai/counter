#ifndef IMAP_ELEM_H
#define IMAP_ELEM_H
#include <support/types.h>
#include "cutils/chash.h"

/* Integer -> Integer hash map */

typedef struct imap_elem_t {
  src_ulong pc;
  unsigned long val;

  struct myhash_elem h_elem;
} imap_elem_t;

unsigned imap_hash_func(struct myhash_elem const *a, void *aux);
bool imap_equal_func(struct myhash_elem const *a, struct myhash_elem const *b,
    void *aux);
void imap_elem_free(struct myhash_elem *e, void *aux);


#endif
