#pragma once

#include "expr/expr.h"
//#include "gsupport/predicate.h"
#include "support/types.h"

namespace eqspace {

class sprel_map_pair_t {
private:
  map<expr_id_t, pair<expr_ref, expr_ref>> m_submap;
public:
  sprel_map_pair_t(sprel_map_t const &src_sprel/*, map<graph_loc_id_t, expr_ref> const &src_locid2expr_map*/, sprel_map_t const &dst_sprel/*, map<graph_loc_id_t, expr_ref> const &dst_locid2expr_map*/);
  sprel_map_pair_t(sprel_map_t const &src_sprel/*, map<graph_loc_id_t, expr_ref> const &src_locid2expr_map*/);
  sprel_map_pair_t(map<expr_id_t, pair<expr_ref, expr_ref>> const& submap) : m_submap(submap)
  { }
  sprel_map_pair_t() {}
  sprel_map_pair_t(consts_struct_t const &cs) {}
  map<expr_id_t, pair<expr_ref, expr_ref>> const &sprel_map_pair_get_submap() const;
  bool operator==(sprel_map_pair_t const &other) const
  {
    return m_submap == other.m_submap;
  }
  //expr_ref sprel_map_pair_get_expr(context *ctx) const;
  string to_string_for_eq() const;
  bool sprel_map_pair_implies(sprel_map_pair_t const &other) const;
  void init_from_submap(map<expr_id_t, pair<expr_ref, expr_ref>> const &submap)
  {
    m_submap = submap;
  }
};

expr_ref sprel_map_pair_and_var_locs(sprel_map_pair_t const& sprel_map_pair, expr_ref const &in);
}

namespace std {
template<>
struct hash<sprel_map_pair_t>
{
  std::size_t operator()(sprel_map_pair_t const &t) const
  {
    size_t seed = 0;
    for (auto const& p : t.sprel_map_pair_get_submap()) {
      myhash_combine<size_t>(seed, hash<size_t>()(p.first));
      myhash_combine<size_t>(seed, hash<expr_ref>()(p.second.second));
    }
    return seed;
  }
};
}
