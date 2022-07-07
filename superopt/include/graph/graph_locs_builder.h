#pragma once

#include "expr/memlabel_map.h"
#include "graph/graph_cp_location.h"

namespace eqspace {

class graph_locs_builder_t
{
public:
  graph_locs_builder_t(graph_loc_id_t const start_locid, map<graph_loc_id_t,graph_cp_location> const& old_locs, context* ctx)
  : m_start_locid(start_locid),
    m_old_locs(old_locs),
    m_ctx(ctx),
    m_max_locid_so_far(get_max_loc_id(old_locs,start_locid))
  {}

  graph_locs_builder_t(graph_loc_id_t const start_locid, map<graph_loc_id_t,graph_cp_location> const& locs, graph_memlabel_map_t const& mlmap, map<graph_loc_id_t,graph_cp_location> const& old_locs, context* ctx)
  : graph_locs_builder_t(start_locid, old_locs, ctx)
  {
    m_max_locid_so_far = get_max_loc_id(locs,m_max_locid_so_far);
    m_locs = locs;
    m_memlabel_map = mlmap;
  }

  map<graph_loc_id_t,graph_cp_location> const& graph_locs_builder_get_locs() const { return m_locs; }
  graph_memlabel_map_t const& graph_locs_builder_get_memlabel_map() const { return m_memlabel_map; }

  graph_loc_id_t graph_locs_add_location_reg(string const& name, sort_ref const& sort);
  graph_loc_id_t graph_locs_add_location_llvmvar(string const& varname, sort_ref const& sort);
  graph_loc_id_t graph_locs_add_location_memmasked(expr_ref const& memvar, expr_ref const& mem_allocvar, memlabel_ref const& ml);
  graph_loc_id_t graph_locs_add_location_memslot(expr_ref const &memvar, expr_ref const& mem_allocvar, expr_ref const& addr, int nbytes, bool bigendian, memlabel_t const& ml);

private:
  graph_loc_id_t get_loc_id_for_newloc(graph_cp_location const &newloc, graph_loc_id_t suggested_loc_id);
  graph_loc_id_t graph_locs_map_add_new_loc(graph_cp_location const &newloc, graph_loc_id_t suggested_loc_id = GRAPH_LOC_ID_INVALID);

  mlvarname_t get_mlvarname_for_memslot(expr_ref const& memvar, expr_ref const& mem_allocvar, expr_ref const& addr, int nbytes, bool bigendian) const;

  static graph_loc_id_t get_max_loc_id(map<graph_loc_id_t,graph_cp_location> const &locs, graph_loc_id_t init_val);

  graph_loc_id_t const m_start_locid;
  map<graph_loc_id_t,graph_cp_location> const& m_old_locs;
  context* m_ctx;

  graph_loc_id_t m_max_locid_so_far;
  map<graph_loc_id_t,graph_cp_location> m_locs;
  graph_memlabel_map_t m_memlabel_map;
};

}
