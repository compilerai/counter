#pragma once

#include "graph/graph_locs_map.h"

using namespace std;

namespace eqspace {

template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
class graph_with_aliasing;

class avail_exprs_val_t
{
private:
  expr_ref m_expr; // nullptr => bottom
  bool m_is_top;

  avail_exprs_val_t(expr_ref const& e) : m_expr(e), m_is_top(false) { }
public:
  avail_exprs_val_t(istream& is, string line, context* ctx);

  static string serialization_tag;
  friend ostream& operator<<(ostream&, avail_exprs_val_t const&);

  static avail_exprs_val_t top()
  {
    avail_exprs_val_t ret(nullptr);
    ret.m_is_top = true;
    return ret;
  }
  static avail_exprs_val_t bottom()
  {
    avail_exprs_val_t ret(nullptr);
    return ret;
  }

  bool is_top() const { return m_is_top; }
  bool is_bottom() const { return !m_is_top && !m_expr; }
  bool operator==(avail_exprs_val_t const& other) const
  {
    return    m_expr == other.m_expr
           && m_is_top == other.m_is_top
    ;
  }
  bool operator!=(avail_exprs_val_t const& other) const
  {
    return !(*this == other);
  }
  string to_string() const
  {
    if (m_is_top) return "avail-exprs-val-top";
    return expr_string(m_expr);
  }
  expr_ref avail_exprs_val_get_expr() const { ASSERT(!m_is_top); ASSERT(m_expr); return m_expr; }

  bool erase_top_and_bottom_mappings() { return this->is_top() || this->is_bottom(); }

  bool weak_update(avail_exprs_val_t const& other)
  {
    // a weak update is never expected
    NOT_REACHED();
    return false;
  }
  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  bool meet(avail_exprs_val_t const& other, map<graph_loc_id_t,avail_exprs_val_t> const& this_loc_map, map<graph_loc_id_t,avail_exprs_val_t> const& other_loc_map, map<graph_loc_id_t,expr_ref> const& locid2expr_map, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g);

  bool dep_killed_by_killed_locs(set<graph_loc_id_t> const& killed_locids, graph_locs_map_t const& locs_map);

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  static map<graph_loc_id_t,avail_exprs_val_t> generate_vals_from_gen_set(map<graph_loc_id_t,expr_ref> const& gen_set, set<graph_loc_id_t> const& killed_locids, map<graph_loc_id_t,avail_exprs_val_t> const& in_map, graph_locs_map_t const& locs_map, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g);

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  static map<graph_loc_id_t,avail_exprs_val_t> generate_vals_for_edge(dshared_ptr<T_E const> const& e, set<graph_loc_id_t> const& killed_locids, map<graph_loc_id_t,avail_exprs_val_t> const& in_map, map<graph_loc_id_t,expr_ref> const& locid2expr_map, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g) { return {}; }
};

ostream& operator<<(ostream&, avail_exprs_val_t const&);

map<expr_id_t, pair<expr_ref, expr_ref>> avail_exprs_create_submap(map<graph_loc_id_t,avail_exprs_val_t> const& av_map, map<graph_loc_id_t, expr_ref> const& locid2expr);

template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
expr_ref expr_substitute_using_available_exprs_submap(expr_ref const& e, expr_submap_t const& av_submap, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g);

}

namespace std {
template<>
struct hash<avail_exprs_val_t>
{
  std::size_t operator()(avail_exprs_val_t const &av) const
  {
    size_t seed = 0;
    myhash_combine<size_t>(seed, hash<bool>()(av.is_top()));
    myhash_combine<size_t>(seed, hash<bool>()(av.is_bottom()));
    if (!av.is_top() && !av.is_bottom()) {
      expr_ref e = av.avail_exprs_val_get_expr();
      myhash_combine<size_t>(seed, hash<expr_ref>()(e));
    }
    return seed;
  }
};

}
