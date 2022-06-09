#pragma once

#include <support/types.h>
#include "cutils/chash.h"

/* String -> Integer hash map */

typedef struct string_elem_t {
  char *str;
  unsigned long val;

  struct myhash_elem h_elem;
} string_elem_t;

unsigned string_hash_func(struct myhash_elem const *a, void *aux);
bool string_equal_func(struct myhash_elem const *a, struct myhash_elem const *b,
    void *aux);
void string_elem_free(struct myhash_elem *e, void *aux);
