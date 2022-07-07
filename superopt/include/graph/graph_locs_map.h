#pragma once

#include "graph/graph_cp_location.h"
#include "graph/graph_locs_builder.h"
#include "graph/lr_status.h"

namespace eqspace {

class graph_locs_map_t
{
private:
  map<graph_loc_id_t,graph_cp_location> m_locs;
  graph_memlabel_map_t m_memlabel_map;
  map<graph_loc_id_t,expr_ref> m_locid2expr_map;
  map<expr_id_t,graph_loc_id_t> m_exprid2locid_map;
  map<string_ref,graph_loc_id_t> m_varname_loc_id_map;

  graph_locs_map_t(map<graph_loc_id_t,graph_cp_location> const& locs, graph_memlabel_map_t const& memlabel_map);

public:
  graph_locs_map_t() {}
  graph_locs_map_t(graph_locs_builder_t const& locs);

  map<graph_loc_id_t, graph_cp_location> const& graph_locs_map_get_locs() const     { return m_locs; }
  map<graph_loc_id_t, expr_ref> const& graph_locs_map_get_locid2expr_map() const    { return m_locid2expr_map; }
  map<expr_id_t, graph_loc_id_t> const& graph_locs_map_get_exprid2locid_map() const { return m_exprid2locid_map; }
  expr_ref const& graph_locs_map_get_expr_at_loc_id(graph_loc_id_t loc_id) const
  {
    ASSERT(m_locid2expr_map.count(loc_id));
    return m_locid2expr_map.at(loc_id);
  }
  graph_cp_location graph_locs_map_get_loc_for_id(graph_loc_id_t const& locid) const
  {
    ASSERT(m_locs.count(locid));
    return m_locs.at(locid);
  }
  graph_loc_id_t graph_locs_map_get_locid_for_varname(string_ref const& varname) const
  {
    return (m_varname_loc_id_map.count(varname)) ? m_varname_loc_id_map.at(varname) : GRAPH_LOC_ID_INVALID;
  }
  graph_loc_id_t graph_locs_map_get_locid_for_expr(expr_ref const& e) const
  {
    return (m_exprid2locid_map.count(e->get_id())) ? m_exprid2locid_map.at(e->get_id()) : GRAPH_LOC_ID_INVALID;
  }
  bool graph_locs_map_has_mapping_for_locid(graph_loc_id_t const& locid) const { return m_locs.count(locid); }
  memlabel_ref graph_locs_map_get_memlabel_for_memslot_locid(graph_loc_id_t const& locid) const;
  bool graph_locs_map_locid_refers_to_stack_memslot(graph_loc_id_t const& locid) const
  {
    auto mlr = graph_locs_map_get_memlabel_for_memslot_locid(locid);
    if (!mlr)
      return false;
    return mlr->get_ml().memlabel_is_stack();
  }
  //memlabel_ref graph_locs_map_get_ml_for_memslot_loc(graph_loc_id_t loc_id) const;
  graph_loc_id_t graph_locs_map_get_loc_id_for_memmask_ml(memlabel_ref const& mlr) const;
  set<graph_loc_id_t> get_locs_potentially_read_in_expr(expr_ref const &e, string const& graph_name) const;

  void graph_loc_exprs_to_stream(ostream& os) const;
  ostream& graph_locs_map_to_stream(ostream& os, string const& graph_name) const;
  static graph_locs_map_t graph_locs_map_from_stream(istream& is, context* ctx);

  // mutators
  void graph_locs_map_update_memlabels_for_memslot_locs(std::function<memlabel_ref (expr_ref, size_t, map<expr_id_t,graph_loc_id_t> const&)> addr2memlabel_fn);
  graph_loc_id_t graph_locs_map_add_location_memslot(expr_ref const &memvar, expr_ref const& mem_allocvar, expr_ref addr, int nbytes, bool bigendian, memlabel_t const& ml, graph_loc_id_t const& start_loc_id, map<graph_loc_id_t,graph_cp_location> const& old_locs);
  void graph_locs_map_union(graph_locs_map_t const& other);

  void lr_status_for_locs_to_stream(ostream& os, map<graph_loc_id_t, lr_status_ref> const& lr_status, consts_struct_t const& cs) const;

private:
  void populate_locid2expr_map();
  void populate_varname_loc_id();
  string graph_locs_map_loc_to_string(graph_loc_id_t loc_id) const;
};

}
