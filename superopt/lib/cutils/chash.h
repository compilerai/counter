#pragma once

/* Hash table.

   This data structure is thoroughly documented in the Tour of
   Pintos for Project 3.

   This is a standard hash table with chaining.  To locate an
   element in the table, we compute a hash function over the
   element's data and use that as an index into an array of
   doubly linked lists, then linearly search the list.

   The chain lists do not use dynamic allocation.  Instead, each
   structure that can potentially be in a hash must embed a
   struct hash_elem member.  All of the hash functions operate on
   these `struct hash_elem's.  The hash_entry macro allows
   conversion from a struct hash_elem back to a structure object
   that contains it.  This is the same technique used in the
   linked list implementation.  Refer to lib/kernel/list.h for a
   detailed explanation. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "clist.h"

/* Hash element. */
struct myhash_elem 
  {
    struct clist_elem list_elem;
  };

/* Converts pointer to hash element HASH_ELEM into a pointer to
   the structure that HASH_ELEM is embedded inside.  Supply the
   name of the outer structure STRUCT and the member name MEMBER
   of the hash element.  See the big comment at the top of the
   file for an example. */
#define chash_entry(HASH_ELEM, STRUCT, MEMBER)                   \
        ((STRUCT *) ((uint8_t *) &(HASH_ELEM)->list_elem        \
                     - offsetof (STRUCT, MEMBER.list_elem)))

/* Computes and returns the hash value for hash element E, given
   auxiliary data AUX. */
typedef unsigned chash_hash_func (const struct myhash_elem *e, void *aux);

/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is equal to B, or
   false if A is greater than or less than B. */
typedef bool chash_equal_func (const struct myhash_elem *a,
                              const struct myhash_elem *b,
                              void *aux);

/* Performs some operation on hash element E, given auxiliary
   data AUX. */
typedef void chash_action_func (struct myhash_elem *e, void *aux);

/* Hash table. */
struct chash 
  {
    size_t elem_cnt;            /* Number of elements in table. */
    size_t bucket_cnt;          /* Number of buckets, a power of 2. */
    struct clist *buckets;       /* Array of `bucket_cnt' lists. */
    chash_hash_func *hash;       /* Hash function. */
    chash_equal_func *equal;     /* Comparison function. */
    void *aux;                  /* Auxiliary data for `hash' and `less'. */
    size_t min_bucket_count;    /* The minimum size of the table. default:4. */
  };

/* A chash table iterator. */
struct chash_iterator 
  {
    struct chash *chash;          /* The hash table. */
    struct clist *bucket;        /* Current bucket. */
    struct myhash_elem *elem;     /* Current hash element in current bucket. */
  };

/* Basic life cycle. */
bool myhash_init (struct chash *, chash_hash_func *, chash_equal_func *, void *aux);
bool myhash_init_size (struct chash *h, size_t size, chash_hash_func *hash,
    chash_equal_func *equal, void *aux);
void myhash_clear (struct chash *, chash_action_func *);
void myhash_destroy (struct chash *, chash_action_func *);

/* Search, insertion, deletion. */
struct myhash_elem *myhash_insert (struct chash *, struct myhash_elem *);
struct myhash_elem *myhash_replace (struct chash *, struct myhash_elem *);
struct myhash_elem *myhash_find (struct chash const *, struct myhash_elem *);
struct myhash_elem *myhash_delete (struct chash *, struct myhash_elem *);

/* The following functions should be used if duplicate elements are
   allowed. */
struct clist *myhash_find_bucket (struct chash *, struct myhash_elem *);
struct clist *myhash_find_bucket_with_hash (struct chash *, unsigned hval);

/* Iteration. */
void myhash_apply (struct chash *, chash_action_func *);
void myhash_first (struct chash_iterator *, struct chash *);
struct myhash_elem *myhash_next (struct chash_iterator *);
struct myhash_elem *myhash_cur (struct chash_iterator *);

/* Information. */
size_t myhash_size (struct chash *);
bool myhash_empty (struct chash *);

/* Sample chash functions. */
unsigned myhash_bytes (const void *, size_t);
unsigned myhash_string (const char *);
unsigned myhash_int (int);
