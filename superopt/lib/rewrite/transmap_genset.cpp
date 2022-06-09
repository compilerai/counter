#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>
#include "support/debug.h"
#include "rewrite/transmap_genset.h"
#include "valtag/transmap.h"
#include "valtag/regcount.h"
#include "cutils/chash.h"

typedef struct tmap_elem_t {
  transmap_t tmap;
  vector<transmap_t> out_tmap;
  regcount_t temps_count;

  struct myhash_elem h_elem;
} tmap_elem_t;

static void
tmap_elem_free(struct myhash_elem *e, void *aux)
{
  struct tmap_elem_t *a;
  a = chash_entry(e, struct tmap_elem_t, h_elem);
  //free(a);
  delete a;
}

static bool
tmap_elem_equal(struct myhash_elem const *a, struct myhash_elem const *b,
    void *aux)
{
  struct tmap_elem_t const *e1, *e2;
  e1 = chash_entry(a, struct tmap_elem_t, h_elem);
  e2 = chash_entry(b, struct tmap_elem_t, h_elem);
  if (e1->out_tmap.size() != e2->out_tmap.size()) {
    return false;
  }
  if (!transmaps_locs_equal(&e1->tmap, &e2->tmap)) {
    return false;
  }
  for (size_t i = 0; i < e1->out_tmap.size(); i++) {
    if (!transmaps_locs_equal(&e1->out_tmap.at(i), &e2->out_tmap.at(i))) {
      return false;
    }
  }
  if (!regcounts_equal(&e1->temps_count, &e2->temps_count)) {
    return false;
  }
  return true;
}

static unsigned
tmap_elem_hash(struct myhash_elem const *a, void *aux)
{
  struct tmap_elem_t const *e;
  e = chash_entry(a, struct tmap_elem_t, h_elem);
  unsigned ret = 0;
  ret += transmap_hash(&e->tmap) * 101;
  unsigned i = 1;
  for (const auto &ot : e->out_tmap) {
    ret += transmap_hash(&ot) * 199 * i;
    i++;
  }
  ret += regcount_hash(&e->temps_count);
  return ret;
}

//static void
//tmap_entry_free(struct myhash_elem *e, void *aux)
//{
//  struct tmap_elem_t *te;
//  te = chash_entry(e, struct tmap_elem_t, h_elem);
//  free(te);
//}

void
transmap_genset_init(transmap_genset_t *genset)
{
  myhash_init(&genset->chash, &tmap_elem_hash, &tmap_elem_equal, NULL);
}

void
transmap_genset_free(transmap_genset_t *genset)
{
  myhash_destroy(&genset->chash, tmap_elem_free);
  //struct rbtree_elem *iter;
  //ASSERT(genset);
  /*
  for (iter = rbtree_begin(&genset->rbtree); iter != NULL; iter = rbtree_next(iter)) {
    struct tmap_elem_t *te;
    te = rbtree_entry(iter, struct tmap_elem_t, rb_elem);
  }
  */
  //rbtree_iterate_unordered(&genset->rbtree, tmap_entry_free);
  //rbtree_destroy(genset->rbtree);
  //free(genset);
}

bool
transmap_genset_belongs(transmap_genset_t const *genset, struct transmap_t const *tmap, struct transmap_t const *out_tmaps, size_t num_out_tmaps,
    regcount_t const *temps_count)
{
  tmap_elem_t elem;

  transmap_copy(&elem.tmap, tmap);
  for (size_t i = 0; i < num_out_tmaps; i++) {
    elem.out_tmap.push_back(out_tmaps[i]);
  }
  regcount_copy(&elem.temps_count, temps_count);

  if (myhash_find(&genset->chash, &elem.h_elem)) {
    return true;
  } else {
    return false;
  }
}

void
transmap_genset_add(transmap_genset_t *genset, transmap_t const *tmap,
    transmap_t const *out_tmaps, size_t num_out_tmaps, regcount_t const *temps_count)
{
  vector<transmap_t> out_transmaps;
  for (size_t i = 0; i < num_out_tmaps; i++) {
    out_transmaps.push_back(out_tmaps[i]);
  }
  transmap_genset_add(genset, tmap, out_transmaps, temps_count);
}

void transmap_genset_add(transmap_genset_t *genset,
    struct transmap_t const *tmap, vector<struct transmap_t> const &out_tmaps, struct regcount_t const *temps_count)
{
  tmap_elem_t *elem, needle;

  transmap_copy(&needle.tmap, tmap);
  needle.out_tmap = out_tmaps;
  regcount_copy(&needle.temps_count, temps_count);

  if (!myhash_find(&genset->chash, &needle.h_elem)) {
    //elem = (tmap_elem_t *)malloc(sizeof(tmap_elem_t));
    elem = new tmap_elem_t;
    ASSERT(elem);
    //printf("Adding elem %p\n", elem);
    transmap_copy(&elem->tmap, tmap);
    elem->out_tmap = out_tmaps;
    regcount_copy(&elem->temps_count, temps_count);
    myhash_insert(&genset->chash, &elem->h_elem);
  }
}
