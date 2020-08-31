#pragma once
//#include "gsupport/pc.h"
#include "expr/expr.h"
#include "graph/graph_cp_location.h"
#include "support/types.h"

namespace eqspace {

//class tfg;
struct consts_struct_t;

#define MAX_SPREL_VALUES 4

class sprel_status_t {
public:
  typedef enum { SPREL_STATUS_TOP, SPREL_STATUS_CONSTANT, SPREL_STATUS_BOTTOM } sprel_status_type_t;
  //sprel_status_t meet(sprel_status_t const &other) const;
  string to_string() const;
  sprel_status_type_t get_type() const { return m_type; }
  //expr_ref get_unique_sprel_value() const { assert(m_type == SPREL_STATUS_CONSTANT); assert(m_sprel_vals.size() == 1); return m_sprel_vals.at(0); }
  //vector<expr_ref> const &get_multiple_sprel_values() const { assert(m_type == SPREL_STATUS_CONSTANT); return m_sprel_vals; }
  expr_ref get_constant_value() const { assert(m_type == SPREL_STATUS_CONSTANT); return m_sprel_val; }
  //void set_type(sprel_status_type_t type) { m_type = type; }
  //bool constant_is_unique() const { assert(m_type == SPREL_STATUS_CONSTANT); return m_sprel_vals.size() == 1; }
  static sprel_status_t constant(expr_ref val) {  sprel_status_t ret; ret.m_type = SPREL_STATUS_CONSTANT; ret.m_sprel_val = val; return ret; }
  static sprel_status_t sprel_status_bottom() {  sprel_status_t ret; ret.m_type = SPREL_STATUS_BOTTOM; return ret; }
  //void set_sprel_value(expr_ref e) { assert(m_type == SPREL_STATUS_CONSTANT); m_sprel_val = e; }
  //void add_sprel_value(expr_ref e) { assert(m_type == SPREL_STATUS_CONSTANT); m_sprel_vals.push_back(e); }
  /*bool equal(sprel_status_t const &other) {
    if (m_type != other.m_type) {
      return false;
    }
    if (m_type != SPREL_STATUS_CONSTANT) {
      return true;
    }
    ASSERT(m_type == SPREL_STATUS_CONSTANT);
    if (m_sprel_vals.size() != other.m_sprel_vals.size()) {
      return false;
    }
    for (size_t i = 0; i < m_sprel_vals.size(); i++) {
      if (m_sprel_vals.at(i) != other.m_sprel_vals.at(i)) {
        return false;
      }
    }
    return true;
  }
  void set_to_top()
  {
    m_type = SPREL_STATUS_TOP;
  }
  bool is_top() const
  {
    return m_type == SPREL_STATUS_TOP;
  }
  void set_to_bottom()
  {
    m_type = SPREL_STATUS_BOTTOM;
  }
  bool is_bottom() const
  {
    return m_type == SPREL_STATUS_BOTTOM;
  }
  bool is_constant() const
  {
    return m_type == SPREL_STATUS_CONSTANT;
  }*/
  bool operator==(sprel_status_t const &other) const
  {
    return    m_type == other.m_type
           && m_sprel_val == other.m_sprel_val;
  }

private:
  sprel_status_type_t m_type;
  expr_ref m_sprel_val;
  //vector<expr_ref> m_sprel_vals;
};

class sprel_map_t {
private:
  map<graph_loc_id_t, sprel_status_t> m_map;  //(loc_id --> status)
  map<graph_loc_id_t, expr_ref> const &m_locid2expr_map;
  map<expr_id_t, pair<expr_ref, expr_ref>> m_submap;
  consts_struct_t const &m_cs;

  void sprel_map_update_submap();
public:
  sprel_map_t(map<graph_loc_id_t, expr_ref> const &locid2expr_map, consts_struct_t const &cs) : m_locid2expr_map(locid2expr_map), m_cs(cs) {}
  sprel_map_t(sprel_map_t const &other) : m_map(other.m_map), m_locid2expr_map(other.m_locid2expr_map), m_submap(other.m_submap), m_cs(other.m_cs) {}
  void operator=(sprel_map_t const &other)
  {
    ASSERT(&m_cs == &other.m_cs);
    ASSERT(m_locid2expr_map == other.m_locid2expr_map);
    m_map = other.m_map;
    m_submap = other.m_submap;
  }
  void sprel_map_add_constant_mapping(graph_loc_id_t loc_id, expr_ref const &e)
  {
    m_map[loc_id] = sprel_status_t::constant(e);
    sprel_map_update_submap();
  }
  void sprel_map_set_remaining_mappings_to_bottom(map<graph_loc_id_t, graph_cp_location> const &locs)
  {
    for (const auto &l : locs) {
      if (m_map.count(l.first) == 0) {
        m_map.insert(make_pair(l.first, sprel_status_t::sprel_status_bottom()));
      }
    }
  }
  void set_sprel_mappings(map<graph_loc_id_t, sprel_status_t> const &loc_ids);
  /*void add_mapping(graph_loc_id_t loc_id, sprel_status_t const &s)
  {
    m_map[loc_id] = s;
  }*/
  sprel_status_t const &get_mapping(graph_loc_id_t loc_id) const
  {
    ASSERT(m_map.count(loc_id));
    return m_map.at(loc_id);
  }
  bool has_mapping_for_loc(graph_loc_id_t loc_id) const;
  //map<int, sprel_status_t> &get_linearly_related_for_addr_ref_id(int cs_addr_ref_id);
  //void set_linearly_related_for_addr_ref_id(int cs_addr_ref_id, map<int, lr_status_t> const &lr_status);
  //long get_linearly_related_addr_ref_id(long loc_id) const;
  //int get_loc_id() const;
  //int get_cs_addr_ref_id() const;
  //sprel_status_t get_status(int loc_id) const;
  //void set_status(int loc_id, sprel_status_t status);
  //bool status_contains_top() const;
  //bool is_linearly_related_to_input_sp(int loc_id, expr_ref &out) const;
  map<expr_id_t, pair<expr_ref, expr_ref>> const &sprel_map_get_submap(/*map<graph_loc_id_t, expr_ref> const &locid2expr_map*/) const;
  //unordered_set<predicate> sprel_map_get_assumes(/*map<graph_loc_id_t, expr_ref> const &locid2expr_map*/) const;
  string to_string(map<graph_loc_id_t, graph_cp_location> const &locs, eqspace::consts_struct_t const &cs, string prefix) const;
  bool equals(sprel_map_t const &other) const;
  void sprel_map_union(sprel_map_t const &other);
  string to_string() const;
  string to_string_for_eq() const;
};

//sprel_status_t expr_sp_relation_holds(expr_ref e, sprel_map_t const &pred_sprel_map, consts_struct_t const &cs);
//void sprel_map_get_submap(sprel_map_t const &sprel_map, map<graph_loc_id_t, expr_ref> const &loc_id_expr_map, map<expr_id_t, pair<expr_ref, expr_ref>> &submap);

}
//sprel_status_t sprel_status_meet(sprel_status_t a, sprel_status_t b);
