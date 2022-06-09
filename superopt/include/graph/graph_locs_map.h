#pragma once

#include "graph/graph_cp_location.h"

namespace eqspace {

class graph_locs_map_t {
private:
  map<graph_loc_id_t, graph_cp_location> m_locs;
  mutable map<graph_loc_id_t, expr_ref>  m_locid2expr_map;
  mutable map<expr_id_t, graph_loc_id_t> m_exprid2locid_map;
  mutable map<string_ref, graph_loc_id_t> m_varname_loc_id_map;
public:
  graph_locs_map_t(map<graph_loc_id_t, graph_cp_location> const& locs);
  map<graph_loc_id_t, graph_cp_location> const& graph_locs_map_get_locs() const
  {
    return m_locs;
  }
  expr_ref const& graph_locs_map_get_expr_at_loc_id(graph_loc_id_t loc_id) const;
  map<graph_loc_id_t, expr_ref> const& graph_locs_map_get_locid2expr_map() const;
  //map<graph_loc_id_t, expr_ref>& graph_locs_map_get_locid2expr_map_ref() const;
  map<expr_id_t, graph_loc_id_t> const& graph_locs_map_get_exprid2locid_map() const;
  //map<expr_id_t, graph_loc_id_t>& graph_locs_map_get_exprid2locid_map_ref() const;
  void graph_loc_exprs_to_stream(ostream& os) const;
  graph_loc_id_t graph_locs_map_get_locid_for_varname(string_ref const& varname) const; 
  void graph_locs_map_update_memlabels_for_memslot_locs(std::function<memlabel_ref (expr_ref)> addr2memlabel_fn);
  graph_loc_id_t graph_locs_map_add_location_memslot(expr_ref const &memvar, expr_ref const& mem_allocvar, expr_ref addr, int nbytes, bool bigendian, memlabel_t const& ml, graph_loc_id_t const& start_loc_id);
  static graph_loc_id_t graph_locs_add_location_memslot(map<graph_loc_id_t, graph_cp_location> &new_locs, expr_ref const &memvar, expr_ref const& mem_allocvar, expr_ref addr, int nbytes, bool bigendian, memlabel_t const& ml, map<graph_loc_id_t, graph_cp_location> const &old_locs, graph_loc_id_t const& start_loc_id);
  static graph_loc_id_t graph_locs_map_add_new_loc(map<graph_loc_id_t, graph_cp_location>& new_locs, graph_cp_location const &newloc, map<graph_loc_id_t, graph_cp_location> const &old_locs, graph_loc_id_t const& start_loc_id, graph_loc_id_t suggested_loc_id = -1);
  static graph_cp_location graph_locs_map_mk_memslot_loc(expr_ref const& memvar/*string const &memname*/,expr_ref const& mem_allocvar, memlabel_ref const& ml, expr_ref addr, size_t nbytes, bool bigendian);
private:
  void populate_locid2expr_map();
  void populate_varname_loc_id();
  static graph_loc_id_t get_max_loc_id(map<graph_loc_id_t, graph_cp_location> const &old_locs, map<graph_loc_id_t, graph_cp_location> const &new_locs, graph_loc_id_t const& start_loc_id);
  static graph_loc_id_t get_loc_id_for_newloc(graph_cp_location const &newloc, map<graph_loc_id_t, graph_cp_location> const &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs, graph_loc_id_t const& start_loc_id);

};

}
