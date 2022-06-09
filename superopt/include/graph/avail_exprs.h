#pragma once
#include <map>
#include <string>

#include "support/types.h"

#include "expr/expr.h"
#include "expr/ssa_utils.h"

#include "gsupport/tfg_edge.h"
#include "gsupport/corr_graph_edge.h"

#include "graph/avail_exprs_val.h"
#include "graph/graph_cp_location.h"

using namespace std;
namespace eqspace {

class pcpair;
class corr_graph_node;
class corr_graph_edge;
class predicate;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_locs;

//class pred_avail_exprs_analysis;
template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class avail_exprs_analysis;
//class pred_avail_exprs_val_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class available_exprs_alias_analysis_combo_val_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class available_exprs_alias_analysis_combo_dfa_t;

//template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
//class available_exprs_alias_analysis_combo_dfa_old_t;

class avail_exprs_t {
private:
  map<graph_loc_id_t, avail_exprs_val_t> m_loc_map;
public:
  avail_exprs_t()
  { }
  avail_exprs_t(map<graph_loc_id_t, avail_exprs_val_t> const& m) : m_loc_map(m)
  { }
  avail_exprs_t(istream& is, context* ctx);

  static avail_exprs_t
  avail_exprs_top(set<graph_loc_id_t> const& locs)
  {
    avail_exprs_t ret;
    for (auto const& lid : locs) {
      ret.m_loc_map.insert(make_pair(lid, avail_exprs_val_t::avail_exprs_val_top()));
    }
    return ret;
  }

  static avail_exprs_t
  avail_exprs_start_pc_value(map<graph_loc_id_t,graph_cp_location> const& locs_map)
  {
    set<graph_loc_id_t> ret_locids;
    for (auto const& [locid,loc] : locs_map)  {
      if (loc.is_memmask() && !expr_represents_ssa_renamed_var<pc>(loc.m_memvar) && !expr_represents_ssa_renamed_var<pc>(loc.m_mem_allocvar)) {
        // set mem-version and memalloc-version at entry to bottom
        continue;
      }
      ret_locids.insert(locid);
    }
    return avail_exprs_top(ret_locids);
  }

  expr_ref avail_exprs_and(expr_ref const &in, map<graph_loc_id_t, expr_ref> const &locid2expr_map) const;

  void avail_exprs_intersect(avail_exprs_t const& other);

  void avail_exprs_add(avail_exprs_t const& other);
  
  bool operator!=(avail_exprs_t const& other) const
  {
    return m_loc_map != other.m_loc_map;
  }

  map<expr_id_t, pair<expr_ref, expr_ref>> avail_exprs_get_submap(map<graph_loc_id_t, expr_ref> const& locid2expr/*graph_with_locs<T_PC, T_N, T_E, T_PRED> const &t*/) const;
  string avail_exprs_to_string(map<graph_loc_id_t, expr_ref> const& locid2expr_map) const;
  void avail_exprs_to_stream(ostream& os, std::map<graph_loc_id_t, expr_ref> const& locid2expr_map) const;
  bool avail_exprs_contains(avail_exprs_t const &needle) const;
  map<graph_loc_id_t, avail_exprs_val_t> const& avail_exprs_get_loc_map() const { return m_loc_map; }
  void avail_exprs_add_top_for_loc_id(graph_loc_id_t loc_id)
  {
    bool inserted = m_loc_map.insert(make_pair(loc_id, avail_exprs_val_t::avail_exprs_val_top())).second;
    ASSERT(inserted);
  }

  //friend class pred_avail_exprs_analysis;
  friend class avail_exprs_val_t;
  friend class avail_exprs_analysis<pc, tfg_node, tfg_edge, predicate>;
  friend class avail_exprs_analysis<pcpair, corr_graph_node, corr_graph_edge, predicate>;
  friend class available_exprs_alias_analysis_combo_val_t<pc, tfg_node, tfg_edge, predicate>;
  friend class available_exprs_alias_analysis_combo_val_t<pcpair, corr_graph_node, corr_graph_edge, predicate>;
  friend class ftmap_pointsto_analysis_combo_dfa_t;
  //friend class available_exprs_alias_analysis_combo_dfa_t<pc, tfg_node, tfg_edge, predicate>;
  //friend class available_exprs_alias_analysis_combo_dfa_t<pcpair, corr_graph_node, corr_graph_edge, predicate>;
private:
  static expr_ref expr_substitute_iteratively_using_submap(expr_ref e, map<expr_id_t, pair<expr_ref, expr_ref>> const& submap1);
};

//using avail_exprs_t = map<graph_loc_id_t, expr_ref>;
//class pred_avail_exprs_t {
//private:
//  map<tfg_suffixpath_t, avail_exprs_t> m_map;
//public:
//  pred_avail_exprs_t()
//  { }
//  pred_avail_exprs_t(map<tfg_suffixpath_t, avail_exprs_t> const& m) : m_map(m)
//  { }
//  map<tfg_suffixpath_t, avail_exprs_t> const& get_map() const
//  {
//    return m_map;
//  }
//  bool equals(pred_avail_exprs_t const& other) const
//  {
//    if (m_map.size() != other.m_map.size()) {
//      return false;
//    }
//    for (auto const& m : m_map) {
//      if (!other.m_map.count(m.first)) {
//        return false;
//      }
//      if (m.second.get_loc_map() != other.m_map.at(m.first).get_loc_map()) {
//        return false;
//      }
//    }
//    return true;
//  }
//
//  expr_ref pred_avail_exprs_and(expr_ref const &in, tfg const &t, tfg_suffixpath_t const &cg_src_suffixpath) const;
//  map<expr_id_t, pair<expr_ref, expr_ref>> pred_avail_exprs_get_submap(tfg const &t, tfg_suffixpath_t const &cg_src_suffixpath) const;
//
//  bool pred_avail_exprs_contains(pred_avail_exprs_t const &needle) const;
//  string pred_avail_exprs_to_string() const;
//
//  friend class pred_avail_exprs_analysis;
//  friend class avail_exprs_analysis;
//  friend class pred_avail_exprs_val_t;
//};


}

namespace std {
template<>
struct hash<avail_exprs_t>
{
  std::size_t operator()(avail_exprs_t const &t) const
  {
    size_t seed = 0;
    for (auto const& pp : t.avail_exprs_get_loc_map()) {
      //myhash_combine<size_t>(seed, hash<tfg_suffixpath_t>()(p.first));
      //map<graph_loc_id_t, expr_ref> const& m = p.get_loc_map();
      //for (auto const& pp : m) {
        myhash_combine<size_t>(seed, hash<graph_loc_id_t>()(pp.first));
        myhash_combine<size_t>(seed, hash<bool>()(pp.second.avail_exprs_val_is_top()));
        if (!pp.second.avail_exprs_val_is_top()) {
          expr_ref e = pp.second.avail_exprs_val_get_expr();
          ASSERT(e);
          myhash_combine<size_t>(seed, hash<expr_ref>()(e));
        }
      //}
    }
    return seed;
  }
};
}

//namespace std {
//template<>
//struct hash<pred_avail_exprs_t>
//{
//  std::size_t operator()(pred_avail_exprs_t const &t) const
//  {
//    size_t seed = 0;
//    for (auto const& p : t.get_map()) {
//      myhash_combine<size_t>(seed, hash<tfg_suffixpath_t>()(p.first));
//      map<graph_loc_id_t, expr_ref> const& m = p.second.get_loc_map();
//      for (auto const& pp : m) {
//        myhash_combine<size_t>(seed, hash<graph_loc_id_t>()(pp.first));
//        myhash_combine<size_t>(seed, hash<expr_ref>()(pp.second));
//      }
//    }
//    return seed;
//  }
//};
//}
