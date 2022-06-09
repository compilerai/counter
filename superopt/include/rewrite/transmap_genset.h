#ifndef __TRANSMAP_GENSET_H
#define __TRANSMAP_GENSET_H
#include <stdbool.h>
#include <vector>
#include "cutils/chash.h"

struct transmap_t;
struct transmap_genset_t;
struct regcount_t;
typedef struct transmap_genset_t transmap_genset_t;

void transmap_genset_init(transmap_genset_t *genset);
bool transmap_genset_belongs(transmap_genset_t const *genset,
    struct transmap_t const *tmap, struct transmap_t const *out_tmaps, size_t num_out_tmaps, struct regcount_t const *touch_not_live_in);
void transmap_genset_add(transmap_genset_t *genset,
    struct transmap_t const *tmap, struct transmap_t const *out_tmaps, size_t num_out_tmaps, struct regcount_t const *touch_not_live_in);

void transmap_genset_add(transmap_genset_t *genset,
    struct transmap_t const *tmap, std::vector<struct transmap_t> const &out_tmaps, struct regcount_t const *touch_not_live_in);
void transmap_genset_free(transmap_genset_t *genset);

struct transmap_genset_t
{
  struct chash chash;
};


#endif
