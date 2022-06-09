#pragma once

#include <map>
#include <list>
#include <string>
#include <cassert>
#include <set>
#include <memory>

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"


#include "graph/graph.h"
#include "graph/dfa.h"
#include "graph/graph_with_regions.h"
#include "graph/var_version.h"


namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E>
class graph_with_var_versions : public graph_with_regions_t<T_PC, T_N, T_E>
{
public:

  graph_with_var_versions(context* ctx) : graph_with_regions_t<T_PC, T_N, T_E>(), m_ctx(ctx), m_is_ssa_graph(false), m_start_state()
  { 
  }

  graph_with_var_versions(graph_with_var_versions const &other) : graph_with_regions_t<T_PC, T_N, T_E>(other),
                                                              m_ctx(other.m_ctx),
                                                              m_is_ssa_graph(other.m_is_ssa_graph),
                                                              m_pc_to_var_version_map(other.m_pc_to_var_version_map),
//                                                              m_pc_reaching_mem_expr(other.m_pc_reaching_mem_expr),
//                                                              m_pc_reaching_memalloc_expr(other.m_pc_reaching_memalloc_expr),
                                                              m_mem_name(other.m_mem_name),
                                                              m_memalloc_name(other.m_memalloc_name),
                                                              m_start_state(other.m_start_state)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  void graph_ssa_copy(graph_with_var_versions const& other) 
  {
    graph_with_regions_t<T_PC, T_N, T_E>::graph_ssa_copy(other);
    ASSERT(m_ctx);
    if(other.m_is_ssa_graph)
      this->set_graph_as_ssa();
    this->set_start_state(other.m_start_state);
  }

  void set_graph_as_ssa() {
    m_is_ssa_graph = true;
  }

  virtual bool graph_is_ssa() const {
    return m_is_ssa_graph;
  }


  context* get_context() const { return m_ctx; }

  set<expr_ref> get_all_memvar_versions() const 
  {
    set<expr_ref> ret;
    for(auto const& p : this->get_all_pcs()) {
      ret.insert(this->get_memvar_version_at_pc(p));
    }
    return ret;
  }

  set<expr_ref> get_all_memallocvar_versions() const 
  {
    set<expr_ref> ret;
    for(auto const& p : this->get_all_pcs()) {
      ret.insert(this->get_memallocvar_version_at_pc(p));
    }
    return ret;
  }

  expr_ref get_var_version_at_pc(T_PC p, string_ref const& varname) const;
  expr_ref get_memvar_version_at_pc(T_PC p) const; 
  expr_ref get_memallocvar_version_at_pc(T_PC p) const; 
  
  /*virtual */void start_state_to_stream(ostream& ss, string const& prefix) const
  {
    ss << this->get_start_state().get_state().state_to_string_for_eq();

  }

  set<expr_ref> get_all_versions_of_var(string_ref const& varname) const;
  
  void graph_to_stream(ostream& ss, string const& prefix) const
  {
    this->graph<T_PC, T_N, T_E>::graph_to_stream(ss, prefix);
    ss << "=is_ssa_graph:\n";
    ss << m_is_ssa_graph << endl;
    ss << "=StartState:\n";
    this->start_state_to_stream(ss, prefix);
    for (auto const& [p,vv] : m_pc_to_var_version_map) {
      ss << "=PC to var-version map at " << p.to_string() << endl;
      vv.var_version_to_stream(ss);
    }
    ss << "=graph_with_var_versions done\n";
  }

  graph_with_var_versions(istream& in, context* ctx);

  /*virtual */start_state_t const &get_start_state() const { return m_start_state; }
  //virtual start_state_t       &get_start_state()       { return m_start_state; }

  void graph_add_keys_and_exprs_to_start_state(map<string_ref, expr_ref> const& keys_and_exprs)
  {
    for (auto const& [key,expr] : keys_and_exprs) {
      ASSERT(!m_start_state.start_state_has_expr(key));
      m_start_state.start_state_set_mapping(key, expr);
    }
  }

  void set_start_state(start_state_t const& st) 
  { 
    m_start_state = st; 
    expr_ref mem, mem_alloc;
    if(m_start_state.get_state().get_memory_expr(mem))
     m_mem_name = m_ctx->get_key_from_input_expr(mem);
    if(m_start_state.get_state().get_memalloc_expr(mem_alloc))
      m_memalloc_name = m_ctx->get_key_from_input_expr(mem_alloc);
//    populate_pc_mem_structures();
  }

  void set_start_state(state const& st) { 
    start_state_t start_st(st);
    this->set_start_state(start_st);
  }
  
  virtual set<string_ref> graph_get_externally_visible_variables() const;

  /*virtual */void populate_pc_var_versions() const;


protected:
  map<T_PC,var_version_t> get_pc_to_var_version_map() const { return m_pc_to_var_version_map; }
private:
  context *m_ctx;
  bool m_is_ssa_graph;
  mutable map<T_PC, var_version_t> m_pc_to_var_version_map;
//  mutable map<T_PC, expr_ref> m_pc_reaching_mem_expr;
//  mutable map<T_PC, expr_ref> m_pc_reaching_memalloc_expr;
  string_ref m_mem_name;
  string_ref m_memalloc_name;
  start_state_t m_start_state;
};

}
