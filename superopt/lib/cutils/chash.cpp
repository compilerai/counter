/* Hash table.

   This data structure is thoroughly documented in the Tour of
   Pintos for Project 3.

   See hash.h for basic information. */

#include <support/debug.h>
#include <stdlib.h>
#include "support/timers.h"
#include "cutils/chash.h"

#define clist_elem_to_hash_elem(LIST_ELEM)                       \
        clist_entry(LIST_ELEM, struct myhash_elem, list_elem)

static struct clist *find_bucket (struct chash *, struct myhash_elem *);
static struct myhash_elem *find_elem (struct chash *, struct clist *,
                                    struct myhash_elem *);
static void
find_iterator (struct chash *h, struct clist *bucket, struct myhash_elem *e,
    struct chash_iterator *i);
static void insert_elem (struct chash *, struct clist *, struct myhash_elem *);
static void remove_elem (struct chash *, struct myhash_elem *);
static void rehash (struct chash *);

/* Initializes a chash table of size SIZE. */
bool
myhash_init_size (struct chash *h, size_t size, chash_hash_func *hash,
    chash_equal_func *equal, void *aux)
{
  h->min_bucket_count = size;
  h->elem_cnt = 0;
  h->bucket_cnt = h->min_bucket_count;
  h->buckets = (struct clist *)malloc (sizeof *h->buckets * h->bucket_cnt);
  h->hash = hash;
  h->equal = equal;
  h->aux = aux;

  if (h->buckets != NULL) 
    {
      myhash_clear (h, NULL);
      return true;
    }
  else
    return false;
}

/* Initializes hash table H to compute hash values using HASH and
   compare hash elements using LESS, given auxiliary data AUX. */
bool
myhash_init (struct chash *h, chash_hash_func *chash, chash_equal_func *equal,
    void *aux)
{
  return myhash_init_size(h, 4, chash, equal, aux);
}

/* Removes all the elements from H.
   
   If DESTRUCTOR is non-null, then it is called for each element
   in the chash.  DESTRUCTOR may, if appropriate, deallocate the
   memory used by the hash element.  However, modifying hash
   table H while hash_clear() is running, using any of the
   functions hash_clear(), hash_destroy(), hash_insert(),
   hash_replace(), or hash_delete(), yields undefined behavior,
   whether done in DESTRUCTOR or elsewhere. */
void
myhash_clear (struct chash *h, chash_action_func *destructor) 
{
  size_t i;

  for (i = 0; i < h->bucket_cnt; i++) 
    {
      struct clist *bucket = &h->buckets[i];

      if (destructor != NULL) 
        while (!clist_empty (bucket)) 
          {
            struct clist_elem *clist_elem = clist_pop_front (bucket);
            struct myhash_elem *myhash_elem = clist_elem_to_hash_elem (clist_elem);
            destructor (myhash_elem, h->aux);
          }

      clist_init (bucket); 
    }    

  h->elem_cnt = 0;
}

/* Destroys hash table H.

   If DESTRUCTOR is non-null, then it is first called for each
   element in the hash.  DESTRUCTOR may, if appropriate,
   deallocate the memory used by the hash element.  However,
   modifying hash table H while hash_clear() is running, using
   any of the functions hash_clear(), hash_destroy(),
   hash_insert(), hash_replace(), or hash_delete(), yields
   undefined behavior, whether done in DESTRUCTOR or
   elsewhere. */
void
myhash_destroy (struct chash *h, chash_action_func *destructor) 
{
  if (destructor != NULL)
    myhash_clear (h, destructor);
  free (h->buckets);
}

/* Inserts NEW into hash table H and returns a null pointer, if
   no equal element is already in the table.
   If an equal element is already in the table, returns it
   without inserting NEW. */   
struct myhash_elem *
myhash_insert (struct chash *h, struct myhash_elem *_new)
{
  stopwatch_run(&chash_insert_timer);
  struct clist *bucket = find_bucket (h, _new);
  struct myhash_elem *old = find_elem (h, bucket, _new);

  if (old == NULL) 
    insert_elem (h, bucket, _new);

  rehash (h);

  stopwatch_stop(&chash_insert_timer);
  return old; 
}

/* Inserts NEW into hash table H, replacing any equal element
   already in the table, which is returned. */
struct myhash_elem *
myhash_replace (struct chash *h, struct myhash_elem *_new) 
{
  struct clist *bucket = find_bucket (h, _new);
  struct myhash_elem *old = find_elem (h, bucket, _new);

  if (old != NULL)
    remove_elem (h, old);
  insert_elem (h, bucket, _new);

  rehash (h);

  return old;
}

/* Finds and returns an element equal to E in hash table H, or a
   null pointer if no equal element exists in the table. */
struct myhash_elem *
myhash_find (struct chash const *h, struct myhash_elem *e) 
{
  struct myhash_elem *ret;

  stopwatch_run(&chash_find_timer);
  ret = find_elem((struct chash *)h, find_bucket ((struct chash *)h, e), e);
  stopwatch_stop(&chash_find_timer);

  return ret;
}

/* Returns a clist of all items whose chash value is equal to the
   hash value of E. */
struct clist *
myhash_find_bucket (struct chash *h, struct myhash_elem *e)
{
  return find_bucket (h, e);
}

struct clist *
myhash_find_bucket_with_hash (struct chash *h, unsigned hashval)
{
  return &h->buckets[hashval & (h->bucket_cnt - 1)];
}

/* Finds, removes, and returns an element equal to E in hash
   table H.  Returns a null pointer if no equal element existed
   in the table.

   If the elements of the hash table are dynamically allocated,
   or own resources that are, then it is the caller's
   responsibility to deallocate them. */
struct myhash_elem *
myhash_delete (struct chash *h, struct myhash_elem *e)
{
  struct myhash_elem *found = find_elem (h, find_bucket (h, e), e);
  if (found != NULL) 
    {
      remove_elem (h, found);
      rehash (h); 
    }
  return found;
}

/* Calls ACTION for each element in hash table H in arbitrary
   order. 
   Modifying hash table H while hash_apply() is running, using
   any of the functions hash_clear(), hash_destroy(),
   hash_insert(), hash_replace(), or hash_delete(), yields
   undefined behavior, whether done from ACTION or elsewhere. */
void
myhash_apply (struct chash *h, chash_action_func *action) 
{
  size_t i;
  
  ASSERT (action != NULL);

  for (i = 0; i < h->bucket_cnt; i++) 
    {
      struct clist *bucket = &h->buckets[i];
      struct clist_elem *elem, *next;

      for (elem = clist_begin (bucket); elem != clist_end (bucket); elem = next) 
        {
          next = clist_next (elem);
          action (clist_elem_to_hash_elem (elem), h->aux);
        }
    }
}

/* Initializes I for iterating hash table H.

   Iteration idiom:

      struct hash_iterator i;

      hash_first (&i, h);
      while (hash_next (&i))
        {
          struct foo *f = hash_entry (hash_cur (&i), struct foo, elem);
          ...do something with f...
        }

   Modifying hash table H during iteration, using any of the
   functions hash_clear(), hash_destroy(), hash_insert(),
   hash_replace(), or hash_delete(), invalidates all
   iterators. */
void
myhash_first (struct chash_iterator *i, struct chash *h) 
{
  ASSERT (i != NULL);
  ASSERT (h != NULL);

  i->chash = h;
  i->bucket = i->chash->buckets;
  i->elem = clist_elem_to_hash_elem (clist_head (i->bucket));
}

/* Advances I to the next element in the hash table and returns
   it.  Returns a null pointer if no elements are left.  Elements
   are returned in arbitrary order.

   Modifying a hash table H during iteration, using any of the
   functions hash_clear(), hash_destroy(), hash_insert(),
   hash_replace(), or hash_delete(), invalidates all
   iterators. */
struct myhash_elem *
myhash_next (struct chash_iterator *i)
{
  ASSERT (i != NULL);

  i->elem = clist_elem_to_hash_elem (clist_next (&i->elem->list_elem));
  while (i->elem == clist_elem_to_hash_elem (clist_end (i->bucket)))
    {
      if (++i->bucket >= i->chash->buckets + i->chash->bucket_cnt)
        {
          i->elem = NULL;
          break;
        }
      i->elem = clist_elem_to_hash_elem (clist_begin (i->bucket));
    }
  
  return i->elem;
}

/* Returns the current element in the hash table iteration, or a
   null pointer at the end of the table.  Undefined behavior
   after calling hash_first() but before hash_next(). */
struct myhash_elem *
myhash_cur (struct chash_iterator *i) 
{
  return i->elem;
}

/* Returns the number of elements in H. */
size_t
myhash_size (struct chash *h) 
{
  return h->elem_cnt;
}

/* Returns true if H contains no elements, false otherwise. */
bool
myhash_empty (struct chash *h) 
{
  return h->elem_cnt == 0;
}

/* Fowler-Noll-Vo chash constants, for 32-bit word sizes. */
#define FNV_32_PRIME 16777619u
#define FNV_32_BASIS 2166136261u

/* Returns a hash of the SIZE bytes in BUF. */
unsigned
myhash_bytes (const void *buf_, size_t size)
{
  /* Fowler-Noll-Vo 32-bit hash, for bytes. */
  const unsigned char *buf = (const unsigned char *)buf_;
  unsigned hash;

  ASSERT (buf != NULL);

  hash = FNV_32_BASIS;
  while (size-- > 0)
    hash = (hash * FNV_32_PRIME) ^ *buf++;

  return hash;
} 

/* Returns a hash of string S. */
unsigned
myhash_string (const char *s_) 
{
  const unsigned char *s = (const unsigned char *) s_;
  unsigned hash;

  ASSERT (s != NULL);

  hash = FNV_32_BASIS;
  while (*s != '\0')
    hash = (hash * FNV_32_PRIME) ^ *s++;

  return hash;
}

/* Returns a hash of integer I. */
unsigned
myhash_int (int i) 
{
  return myhash_bytes (&i, sizeof i);
}

/* Returns the bucket in H that E belongs in. */
static struct clist *
find_bucket (struct chash *h, struct myhash_elem *e) 
{
  size_t bucket_idx = h->hash (e, h->aux) & (h->bucket_cnt - 1);
  return &h->buckets[bucket_idx];
}

/* Searches BUCKET in H for a hash element equal to E.  Returns
   it if found or a null pointer otherwise. */
static struct myhash_elem *
find_elem (struct chash *h, struct clist *bucket, struct myhash_elem *e) 
{
  struct clist_elem *i;

  for (i = clist_begin (bucket); i != clist_end (bucket); i = clist_next (i)) 
    {
      struct myhash_elem *hi = clist_elem_to_hash_elem (i);
      //if (!h->less (hi, e, h->aux) && !h->less (e, hi, h->aux))
      if (h->equal (hi, e, h->aux))
        return hi; 
    }
  return NULL;
}

/* Searches BUCKET in H for a hash element equal to E.  Returns
   an iterator in I, such that hash_next() on the iterator returns all
   elements equal to E. */
static void
find_iterator (struct chash *h, struct clist *bucket, struct myhash_elem *e,
    struct chash_iterator *i)
{
  struct clist_elem *l;

  i->chash = h;
  i->bucket = bucket;
  i->elem = clist_elem_to_hash_elem (clist_head(bucket));
  for (l = clist_begin (bucket); l != clist_end (bucket); l = clist_next (l)) {
    struct myhash_elem *hi = clist_elem_to_hash_elem (l);
    if (h->equal (hi, e, h->aux)) {
      return;
    }
    i->elem = clist_elem_to_hash_elem(l);
  }
}

/* Returns X with its lowest-order bit set to 1 turned off. */
static inline size_t
turn_off_least_1bit (size_t x) 
{
  return x & (x - 1);
}

/* Returns true if X is a power of 2, otherwise false. */
static inline size_t
is_power_of_2 (size_t x) 
{
  return x != 0 && turn_off_least_1bit (x) == 0;
}

/* Element per bucket ratios. */
#define MIN_ELEMS_PER_BUCKET  1 /* Elems/bucket < 1: reduce # of buckets. */
#define BEST_ELEMS_PER_BUCKET 2 /* Ideal elems/bucket. */
#define MAX_ELEMS_PER_BUCKET  4 /* Elems/bucket > 4: increase # of buckets. */

/* Changes the number of buckets in hash table H to match the
   ideal.  This function can fail because of an out-of-memory
   condition, but that'll just make hash accesses less efficient;
   we can still continue. */
static void
rehash (struct chash *h) 
{
  size_t old_bucket_cnt, new_bucket_cnt;
  struct clist *new_buckets, *old_buckets;
  size_t i;

  ASSERT (h != NULL);

  /* Save old bucket info for later use. */
  old_buckets = h->buckets;
  old_bucket_cnt = h->bucket_cnt;

  /* Calculate the number of buckets to use now.
     We want one bucket for about every BEST_ELEMS_PER_BUCKET.
     We must have at least four buckets, and the number of
     buckets must be a power of 2. */
  new_bucket_cnt = h->elem_cnt / BEST_ELEMS_PER_BUCKET;
  if (new_bucket_cnt < h->min_bucket_count)
    new_bucket_cnt = h->min_bucket_count;
  while (!is_power_of_2 (new_bucket_cnt))
    new_bucket_cnt = turn_off_least_1bit (new_bucket_cnt);

  /* Don't do anything if the bucket count wouldn't change. */
  if (new_bucket_cnt == old_bucket_cnt)
    return;

  /* Allocate new buckets and initialize them as empty. */
  new_buckets = (struct clist *)malloc (sizeof *new_buckets * new_bucket_cnt);
  if (new_buckets == NULL) 
    {
      /* Allocation failed.  This means that use of the hash table will
         be less efficient.  However, it is still usable, so
         there's no reason for it to be an error. */
      return;
    }
  for (i = 0; i < new_bucket_cnt; i++) 
    clist_init (&new_buckets[i]);

  /* Install new bucket info. */
  h->buckets = new_buckets;
  h->bucket_cnt = new_bucket_cnt;

  /* Move each old element into the appropriate new bucket. */
  for (i = 0; i < old_bucket_cnt; i++) 
    {
      struct clist *old_bucket;
      struct clist_elem *elem, *next;

      old_bucket = &old_buckets[i];
      for (elem = clist_begin (old_bucket);
           elem != clist_end (old_bucket); elem = next) 
        {
          struct clist *new_bucket
            = find_bucket (h, clist_elem_to_hash_elem (elem));
          next = clist_next (elem);
          clist_remove (elem);
          clist_push_front (new_bucket, elem);
        }
    }

  free (old_buckets);
}

/* Inserts E into BUCKET (in hash table H). */
static void
insert_elem (struct chash *h, struct clist *bucket, struct myhash_elem *e) 
{
  h->elem_cnt++;
  clist_push_front (bucket, &e->list_elem);
}

/* Removes E from hash table H. */
static void
remove_elem (struct chash *h, struct myhash_elem *e) 
{
  h->elem_cnt--;
  clist_remove (&e->list_elem);
}

