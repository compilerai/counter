/* Cache. */

#include <cutils/cache.h>
#include <support/debug.h>
#include <stdlib.h>
#include <stdio.h>

#define hash_elem_to_cache_elem(HASH_ELEM)                       \
        hash_entry(HASH_ELEM, struct cache_elem, chash_elem)

/* Deletes a random element from the cache. */
static void cache_replace (struct cache *c);

static unsigned
cache_hash_func(struct chash_elem const *e, void *aux)
{
  struct cache *c = (struct cache *)aux;
  ASSERT(e);
  ASSERT(aux);
  return (*c->key)(chash_entry(e, struct cache_elem, chash_elem), c->aux);
}

static bool
cache_equal_func(struct chash_elem const *a, struct chash_elem const *b, void *aux)
{
  struct cache *c = (struct cache *)aux;
  unsigned ka, kb;
  ASSERT(aux); ASSERT(a); ASSERT(b);
  ka = (*c->key)(chash_entry(a, struct cache_elem, chash_elem), c->aux);
  kb = (*c->key)(chash_entry(b, struct cache_elem, chash_elem), c->aux);
  return ka == kb;
}

static void
chash_elem_delete_fn (struct chash_elem *e, void *aux)
{
  struct cache *c = (struct cache *)aux;
  ASSERT(aux); ASSERT(e);
  if (c->delete_fn) {
    (*c->delete_fn)(chash_entry(e, struct cache_elem, chash_elem), c->aux);
  }
}

/* Initializes cache C of size SIZE to compute key values using KEY,
 * on deletion/replacement, call DELETE_FN given auxiliary data AUX. */
bool
cache_init(struct cache *c, size_t size, cache_key_func *key, 
    cache_delete_func *delete_fn, void *aux)
{
  ASSERT(size > 0);
  c->size = size;
  c->key = key;
  c->aux = aux;
  c->delete_fn = delete_fn;

  return chash_init(&c->table, cache_hash_func, cache_equal_func, c);
}

/* Removes all the elements from C.
   
   If DESTRUCTOR is non-null, then it is called for each element in the chash. 
   DESTRUCTOR may, if appropriate, deallocate the memory used by the hash element. */
void
cache_clear(struct cache *c) 
{
  chash_clear(&c->table, hash_elem_delete_fn);
}

/* Destroys cache C. */
void
cache_destroy(struct cache *c) 
{
  chash_destroy(&c->table, hash_elem_delete_fn);
}

/* Inserts NEW into cache C and returns a null pointer, if no equal element is already
 * in the cache. If an equal element is already in the table, returns it without
 * inserting NEW. */
struct cache_elem *
cache_insert(struct cache *c, struct cache_elem *new)
{
  struct chash_elem *h;
  ASSERT(chash_size(&c->table) <= c->size);
  if (chash_size(&c->table) == c->size) {
    cache_replace(c);
  }
  h = chash_insert(&c->table, &new->chash_elem);
  if (h) {
    return chash_entry(h, struct cache_elem, chash_elem);
  }
  return NULL;
}

/* Finds and returns an element equal to E in cache C, or a null pointer if no equal
 * element exists in the table. */
struct cache_elem *
cache_find(struct cache *c, struct cache_elem *e) 
{
  struct chash_elem *h;
  h = chash_find(&c->table, &e->chash_elem);
  if (h) {
    return chash_entry(h, struct cache_elem, chash_elem);
  }
  return NULL;
}

/* Finds, removes, and returns an element equal to E in cache C.
   Returns a null pointer if no equal element existed in the cache.

   If the elements of the cache are dynamically allocated, or own resources
   that are, then it is the caller's responsibility to deallocate them. */
bool
cache_delete(struct cache *c, struct cache_elem *e)
{
  struct chash_elem *deleted;
  deleted = chash_delete(&c->table, &e->chash_elem);
  if (deleted) {
    struct cache_elem *c_elem;
    c_elem = chash_entry(deleted, struct cache_elem, chash_elem);
    if (c->delete_fn) {
      (*c->delete_fn)(c_elem, c->aux);
    }
    return true;
  }
  return false;
}

static void
cache_replace(struct cache *c)
{
  struct chash_iterator iter;
  struct chash_elem *e;
  bool deleted;
  size_t r;

  ASSERT(chash_size(&c->table) == c->size);
  ASSERT(c->size > 0);
  //r = rand() % c->size;
  r = rand() % 4; /* XXX */

  chash_first(&iter, &c->table);
  while ((e = chash_next(&iter)) && r > 0) {
    r--;
    ASSERT(e);
  }
  ASSERT(e);
  deleted = cache_delete(c, chash_entry(e, struct cache_elem, chash_elem));
  ASSERT(deleted);
}
