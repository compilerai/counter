#pragma once
#include "expr/memlabel.h"
#include "expr/expr_utils.h"
#include "support/string_ref.h"

namespace eqspace {

enum graph_cp_location_type_t {
  GRAPH_CP_LOCATION_TYPE_MEMSLOT = 0, //the order of these values is important; they are in increasing order of location-size for the same reg_type, e.g., slot < masked < full_array
  GRAPH_CP_LOCATION_TYPE_MEMMASKED,
  GRAPH_CP_LOCATION_TYPE_REGMEM,
};

class graph_cp_location
{
  graph_cp_location_type_t m_type;
  string_ref m_varname;
  expr_ref m_var;
  memlabel_ref m_memlabel;
  expr_ref m_addr;
  mlvarname_t m_mlvar;
  int m_nbytes = -1;
  bool m_bigendian = 0;
public:
  expr_ref m_memvar;
  expr_ref m_mem_allocvar;

  static graph_cp_location mk_var_loc(string const& varname, sort_ref const& sort, context* ctx)
  {
    graph_cp_location newloc;
    newloc.m_type = GRAPH_CP_LOCATION_TYPE_REGMEM;
    newloc.m_varname = mk_string_ref(varname);
    newloc.m_var = ctx->get_input_expr_for_key(newloc.m_varname, sort);
    return newloc;
  }
  static graph_cp_location mk_memslot_loc(expr_ref const& memvar, expr_ref const& mem_allocvar, mlvarname_t const& mlv, expr_ref const& addr, size_t nbytes, bool bigendian)
  {
    graph_cp_location newloc;
    newloc.m_type = GRAPH_CP_LOCATION_TYPE_MEMSLOT;
    newloc.m_memvar = memvar;
    newloc.m_mem_allocvar = mem_allocvar;
    newloc.m_mlvar = mlv;
    newloc.m_addr = addr;
    newloc.m_nbytes = nbytes;
    newloc.m_bigendian = bigendian;
    return newloc;
  }
  static graph_cp_location mk_memmask_loc(expr_ref const& memvar, expr_ref const& mem_allocvar, memlabel_ref const& ml)
  {
    graph_cp_location newloc;
    newloc.m_type = GRAPH_CP_LOCATION_TYPE_MEMMASKED;
    newloc.m_memvar = memvar;
    newloc.m_mem_allocvar = mem_allocvar;
    newloc.m_memlabel = ml;
    return newloc;
  }

  void set_memlabel(memlabel_ref const& mlr) { m_memlabel = mlr; }

  string to_string() const;
  bool equals_mod_mlvar(graph_cp_location const &other) const;
  bool equals(graph_cp_location const &other) const { return this->equals_mod_mlvar(other); }
  bool operator==(graph_cp_location const& other) const { return this->equals(other); }

  string_ref const& get_varname() const { ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_REGMEM); return m_varname; }
  expr_ref get_var() const { ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_REGMEM); return m_var; }

  expr_ref const& get_memslot_addr() const     { ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT); return m_addr; }
  mlvarname_t const& get_memslot_mlvar() const { ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT); return m_mlvar; }
  int get_memslot_nbytes() const               { ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT); return m_nbytes; }
  bool get_memslot_bigendian() const           { ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT); return m_bigendian; }

  expr_ref get_memslot_expr(memlabel_ref const& mlr = nullptr) const
  {
    if (mlr) return this->m_addr->get_context()->mk_select(this->m_memvar, this->m_mem_allocvar, mlr->get_ml(), this->m_addr, this->m_nbytes, this->m_bigendian);
    else     return this->m_addr->get_context()->mk_select(this->m_memvar, this->m_mem_allocvar, this->m_mlvar, this->m_addr, this->m_nbytes, this->m_bigendian);
  }

  expr_ref get_memmask_expr() const
  {
    ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED);
    ASSERT(this->m_memlabel->get_ml().memlabel_is_atomic());
    return this->m_memvar->get_context()->mk_memmask(this->m_memvar, this->m_mem_allocvar, this->m_memlabel->get_ml());
  }

  memlabel_t const& get_memmask_ml() const    { ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED); return m_memlabel->get_ml(); }
  memlabel_ref const& get_memmask_mlr() const { ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED); return m_memlabel; }

  bool is_reg() const { return m_type == GRAPH_CP_LOCATION_TYPE_REGMEM; }
  bool is_var() const { return m_type == GRAPH_CP_LOCATION_TYPE_REGMEM; }
  bool is_memmask() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED; }
  bool is_memslot() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT; }

  bool graph_cp_location_represents_hidden_var(context* ctx) const;
  bool graph_cp_location_represents_ghost_var(context* ctx) const;

  ostream& graph_loc_to_stream(ostream& os) const;
  static string graph_loc_from_stream(istream& in, context* ctx, graph_cp_location& loc);
};
}
