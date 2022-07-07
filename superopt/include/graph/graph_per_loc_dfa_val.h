#pragma once

#include "graph/dfa_types.h"

namespace eqspace {

using namespace std;

using graph_locid_to_expr_map_t = map<graph_loc_id_t,expr_ref>;

template<typename T_VAL>
class graph_per_loc_dfa_val_t
{
  map<graph_loc_id_t,T_VAL> m_loc_map; // values absent in map are automatically considered bottom
  bool m_is_top = false;

public:
  graph_per_loc_dfa_val_t() {} // bottom by default
  graph_per_loc_dfa_val_t(map<graph_loc_id_t,T_VAL> const& loc_map) : m_loc_map(loc_map) {}
  graph_per_loc_dfa_val_t(istream& is, context* ctx);

  static graph_per_loc_dfa_val_t<T_VAL> bottom()
  {
    return graph_per_loc_dfa_val_t<T_VAL>();
  }
  static graph_per_loc_dfa_val_t<T_VAL> top()
  {
    graph_per_loc_dfa_val_t<T_VAL> ret;
    ret.m_is_top = true;
    return ret;
  }
  static graph_per_loc_dfa_val_t<T_VAL> start_pc_value(graph_locs_map_t const& locs_map)
  {
    set<graph_loc_id_t> ret_locids;
    for (auto const& [locid,loc] : locs_map.graph_locs_map_get_locs())  {
      if (loc.is_memmask() && !expr_represents_ssa_renamed_var<pc>(loc.m_memvar) && !expr_represents_ssa_renamed_var<pc>(loc.m_mem_allocvar)) {
        // set mem-version and memalloc-version at entry to bottom
        continue;
      }
      ret_locids.insert(locid);
    }
    graph_per_loc_dfa_val_t<T_VAL> ret;
    for (auto const& locid : ret_locids) {
      ret.m_loc_map.emplace(locid, T_VAL::top());
    }
    return ret;
  }
  static T_VAL conservative_dfa_val_for_newly_defined_loc(graph_loc_id_t const& new_locid, graph_per_loc_dfa_val_t<T_VAL> const& in, graph_locs_map_t const& locs_map);

  ostream& to_stream(ostream& os, graph_locid_to_expr_map_t const& locid2expr) const;
  string to_string(graph_locid_to_expr_map_t const& locid2expr) const;

  bool is_top() const { return m_is_top; }
  map<graph_loc_id_t,T_VAL> const& get_map() const { return m_loc_map; }
  map<graph_loc_id_t,T_VAL>& get_map_ref() { return m_loc_map; }

  bool has_val_for_locid(graph_loc_id_t const& locid) const             { return m_loc_map.count(locid); }
  void erase(graph_loc_id_t const& locid)                               { m_loc_map.erase(locid); }
  void set_val_for_locid(graph_loc_id_t const& locid, T_VAL const& val)
  {
    if (m_loc_map.count(locid)) {
      m_loc_map.erase(locid);
    }
    m_loc_map.emplace(locid, val);
  }
  // returns true if value was added/updated
  bool add_val_for_locid_weak(graph_loc_id_t const& locid, T_VAL const& val)
  {
    auto itr = m_loc_map.find(locid);
    if (itr == m_loc_map.end()) {
      m_loc_map.emplace(locid,val);
      return true;
    }
    return m_loc_map.at(locid).weak_update(val);
  }

  void add_val_for_locid(graph_loc_id_t const& locid, T_VAL const& val)
  {
    bool inserted = add_val_for_locid_weak(locid, val);
    ASSERT(inserted);
  }
  void add_top_for_locid(graph_loc_id_t const& locid)
  {
    this->add_val_for_locid(locid, T_VAL::top());
  }
  T_VAL get_val_for_locid(graph_loc_id_t const& locid) const
  {
    T_VAL bottom = T_VAL::bottom();
    if (m_loc_map.count(locid)) {
      return bottom;
    }
    return m_loc_map.at(locid);
  }

  void erase_top_and_bottom_mappings()
  {
    function<bool(graph_loc_id_t const&,T_VAL&)> mut_or_erase_val = [](graph_loc_id_t const&, T_VAL& m) -> bool { return m.erase_top_and_bottom_mappings(); };
    map_mutate_or_erase(m_loc_map, mut_or_erase_val);
  }

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  bool meet(graph_per_loc_dfa_val_t<T_VAL> const& other, graph_locid_to_expr_map_t const& locid2expr_map, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g);

  void intersect(graph_per_loc_dfa_val_t<T_VAL> const& other);

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  static dfa_xfer_retval_t ftmap_xfer_and_meet_flow_insensitive(dshared_ptr<T_E const> const& e, graph_per_loc_dfa_val_t<T_VAL> const& in, graph_memlabel_map_t const& memlabel_map, sprel_map_pair_t const& sprel_map_pair, bool out_is_top, graph_per_loc_dfa_val_t<T_VAL>& out, graph_locs_map_t const& graph_locs_map, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g, set<graph_loc_id_t> const& new_locids);
};

}

namespace std {
template<typename T_VAL>
struct hash<graph_per_loc_dfa_val_t<T_VAL>>
{
  std::size_t operator()(graph_per_loc_dfa_val_t<T_VAL> const &t) const
  {
    size_t seed = 0;
    for (auto const& [locid,val] : t.get_map()) {
      myhash_combine<size_t>(seed, hash<graph_loc_id_t>()(locid));
      myhash_combine<size_t>(seed, hash<T_VAL>()(val));
    }
    return seed;
  }
};

}
