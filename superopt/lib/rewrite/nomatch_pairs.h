#ifndef NOMATCH_PAIRS_H
#define NOMATCH_PAIRS_H

#include "support/src-defs.h"
#include "valtag/reg_identifier.h"

typedef struct nomatch_pairs_t {
  exreg_group_id_t group1;
  reg_identifier_t src_reg1;
  exreg_group_id_t group2;
  reg_identifier_t src_reg2;

  nomatch_pairs_t() : group1(-1), src_reg1(reg_identifier_t::reg_identifier_invalid()), group2(-1), src_reg2(reg_identifier_t::reg_identifier_invalid()) {}
} nomatch_pairs_t;

struct regset_t;
struct transmap_t;
struct regmap_t;
struct dst_insn_t;
struct nomatch_pairs_set_t;

nomatch_pairs_set_t
compute_nomatch_pairs(//struct nomatch_pairs_t *nomatch_pairs, int nomatch_pairs_size,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps, long num_out_tmaps,
    struct dst_insn_t const *iseq, long iseq_len);
char *nomatch_pairs_to_string(struct nomatch_pairs_t const *nomatch_pairs,
    int num_nomatch_pairs, char *buf, size_t buf_size);
bool src_inv_regmap_agrees_with_nomatch_pairs(struct regmap_t const *inv_src_regmap,
    //nomatch_pairs_t const *nomatch_pairs, int num_nomatch_pairs
    nomatch_pairs_set_t const &nomatch_pairs_set
    );
void nomatch_pair_copy(nomatch_pairs_t *dst, nomatch_pairs_t const *src);
bool nomatch_pair_equals(nomatch_pairs_t const *a, nomatch_pairs_t const  *b);
bool nomatch_pair_belongs(nomatch_pairs_t const *nomatch_pair, exreg_group_id_t group1, reg_identifier_t const &reg1, exreg_group_id_t group2, reg_identifier_t const &reg2);

#endif /* nomatch_pairs.h */
