#ifndef __TRANSMAP_SET_H
#define __TRANSMAP_SET_H

//#include <cpu-defs.h>
#include <stdint.h>
#include "peephole.h"

#define MAX_TRANSMAP_SET_SIZE 65536

struct transmap_set_row_t;
struct ppc_regmap_t;
struct ppc_regset_t;
struct arm_regmap_t;
struct arm_regset_t;
struct transmap_t;
struct peep_entry_t;

typedef struct transmap_set_elem_t {
private:
  struct peep_entry_t const *peeptab_entry_id;
public:
  struct transmap_t *in_tmap;
  struct transmap_t *out_tmaps;
  struct transmap_set_elem_t *next;
  struct transmap_set_row_t const *row;

  /* the following fields are used to pass information
     from get_all_possible_transmaps() to peepcost computation */
  long num_temporaries;

  /* following fields are used for DP-solving */
  long num_decisions;
  struct transmap_set_elem_t **decisions;
  long cost;
#ifdef DEBUG_TMAP_CHOICE_LOGIC
#define MAX_NUM_PREDS 4
  long tmap_elem_id;
  long peepcost;
  long transcost[MAX_NUM_PREDS];
  long pred_cost[MAX_NUM_PREDS];
  src_ulong pred_decision_pc[MAX_NUM_PREDS];
  long pred_decision_id[MAX_NUM_PREDS];
  long total_cost_on_pred_path[MAX_NUM_PREDS];
#endif
  struct peep_entry_t const *get_peeptab_entry_id() const { return peeptab_entry_id; }
  void set_peeptab_entry_id(struct peep_entry_t const *id) { peeptab_entry_id = id; }
  long transmap_set_elem_get_peepcost() const
  {
    if (peeptab_entry_id) {
      return peeptab_entry_id->cost;
    } else {
      return 0;
    }
  }
} transmap_set_elem_t;

typedef struct transmap_set_row_t {
  struct si_entry_t const *si;
  long fetchlen;

  nextpc_t *nextpcs;
  long num_nextpcs;

  src_ulong *lastpcs;
  double *lastpc_weights;
  long num_lastpcs;

  transmap_set_elem_t *list;
} transmap_set_row_t;

typedef struct transmap_set_t {
  transmap_set_row_t **head;
  size_t head_size;
} transmap_set_t;

typedef
struct pred_transmap_set_t
{
  transmap_set_elem_t *list;

  struct pred_transmap_set_t *next;
} pred_transmap_set_t;

void free_transmap_set_row(transmap_set_elem_t *head);
void transmap_set_elem_free(transmap_set_elem_t *cur);

bool transmap_set_clean_bit_appears(pred_transmap_set_t const *pred_set, int regno);
bool transmap_set_dirty_bit_appears(pred_transmap_set_t const *pred_set, int regno);

int transmap_set_get_crf_sign_ov(pred_transmap_set_t const *prev_set, int regno,
    int sign_ov);

transmap_set_elem_t *transmap_set_union(transmap_set_elem_t *set,
    transmap_set_elem_t *new_set);
long transmap_set_count(transmap_set_elem_t const *set);

#endif
