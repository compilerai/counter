#ifndef REGCONST_MAP_POSSIBILITIES_H
#define REGCONST_MAP_POSSIBILITIES_H

#include "support/src-defs.h"
#include "valtag/reg_identifier.h"
#include <set>
#include <string>

struct regmap_t;
struct transmap_t;
struct dst_insn_t;

class regconst_map_possibilities_t
{
private:
  std::set<std::pair<exreg_group_id_t, reg_identifier_t>> m_set;
public:
  std::string to_string() const;
  bool equals(regconst_map_possibilities_t const &other) const;
  void add_reg_id(exreg_group_id_t group, reg_identifier_t const &regid);
  bool belongs(exreg_group_id_t group, reg_identifier_t const &regid) const;
};

bool
src_inv_regmap_agrees_with_regconst_map_possibilities(struct regmap_t const *inv_src_regmap,
          transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_live_out,
          regconst_map_possibilities_t const &regconst_map_possibilities);
regconst_map_possibilities_t
compute_regconst_map_possibilities(struct transmap_t const *tmap, struct transmap_t const *out_tmaps, struct dst_insn_t const *iseq, long iseq_len, size_t num_nextpcs);

#endif
