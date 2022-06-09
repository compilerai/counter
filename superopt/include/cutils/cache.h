#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "cutils/chash.h"

/* Cache element. */
struct cache_elem {
  struct myhash_elem hash_elem;
};

/* Converts pointer to cache element CACHE_ELEM into a pointer to
   the structure that CACHE_ELEM is embedded inside.  Supply the
   name of the outer structure STRUCT and the member name MEMBER
   of the cache element. */
#define cache_entry(CACHE_ELEM, STRUCT, MEMBER)                   \
        ((STRUCT *) ((uint8_t *) &(CACHE_ELEM)->hash_elem        \
                     - offsetof (STRUCT, MEMBER.hash_elem)))

/* Computes and returns the KEY for cache element E, given auxiliary data AUX. */
typedef unsigned cache_key_func(const struct cache_elem *e, void *aux);

/* Calls on the deleted element during cache deletion/replacement. */
typedef void cache_delete_func(struct cache_elem *e, void *aux);

/* Performs some operation on hash element E, given auxiliary
   data AUX. */
typedef void cache_action_func(struct cache_elem *e, void *aux);

/* Cache. */
struct cache {
  size_t size;                  /* Size of the cache. */
  struct chash table;            /* Hash table storing the elements. */
  cache_key_func *key;          /* Key function. */
  cache_delete_func *delete_fn; /* Function to call on delete. */
  void *aux;                    /* Auxiliary data for 'key'. */
};

/* Basic life cycle. */
bool cache_init(struct cache *, size_t size, cache_key_func *, cache_delete_func *,
    void *aux);
void cache_clear (struct cache *);
void cache_destroy (struct cache *);

/* Search, insertion, deletion. */
struct cache_elem *cache_insert (struct cache *, struct cache_elem *);
struct cache_elem *cache_find (struct cache *, struct cache_elem *);
bool cache_delete (struct cache *, struct cache_elem *);
