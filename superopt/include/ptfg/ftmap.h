#pragma once

#include <ostream>
#include <string>
#include <list>
#include <map>

#include "expr/context.h"
#include "expr/call_context.h"

#include "graph/context_sensitive_ftmap_dfa_val.h"

#include "tfg/tfg.h"
#include "tfg/tfg_llvm.h"
#include "tfg/tfg_ssa.h"

#include "ptfg/ftmap_call_graph.h"
//#include "ptfg/ftmap_call_loop.h"

namespace eqspace {

using namespace std;

using pointsto_val_t = available_exprs_alias_analysis_combo_val_t<pc,tfg_node,tfg_edge,predicate>;

class ftmap_t : public enable_shared_from_this<ftmap_t>
{
private:
  map<call_context_ref, dshared_ptr<tfg_ssa_t>> m_fname_tfg_map;

  dshared_ptr<ftmap_call_graph_t> m_call_graph;
  //map<string, callee_summary_t> m_fname_csum_map;
public:
  //ftmap_t(map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> const& function_tfg_map);
  ftmap_t(map<call_context_ref, dshared_ptr<tfg_ssa_t>> const& function_tfg_map);
  ftmap_t(istream &is, context *ctx);
  ftmap_t(dshared_ptr<tfg_ssa_t> const& t, string const& fname);
  ftmap_t(dshared_ptr<tfg_ssa_t> const& t);
  void print(ostream &os) const;

  dshared_ptr<ftmap_call_graph_t> get_call_graph() const { return m_call_graph; }

  //list<ftmap_call_loop_t> ftmap_identify_natural_call_loops() const;

  tfg_alias_result_t ftmap_get_aliasing_relationship_between_memaccesses(string const& fname, string const& varA, uint64_t sizeA, string const& varB, uint64_t sizeB) const;

  void ftmap_run_pointsto_analysis(/*callee_rw_memlabels_t::get_callee_rw_memlabels_fn_t get_callee_rw_memlabels, */bool is_asm_tfg, dshared_ptr<tfg_llvm_t const> const& src_tfg, map<graph_loc_id_t, graph_cp_location> const &llvm_locs, int call_context_depth, bool update_callee_memabels, context::xml_output_format_t xml_output_format = context::XML_OUTPUT_FORMAT_TEXT_NOCOLOR);
  void ftmap_add_start_pc_preconditions_for_each_tfg();

  map<call_context_ref, dshared_ptr<tfg_ssa_t>> const& get_function_tfg_map() const { return m_fname_tfg_map; }
  //map<string, callee_summary_t> const& get_function_csum_map() const { return m_fname_csum_map; }

  //bool ftmap_has_entry_for_function_name(string const& function_name) const;

  dshared_ptr<ftmap_t> dshared_from_this()
  {
    return dshared_ptr<ftmap_t>(this->shared_from_this());
  }

  dshared_ptr<ftmap_t const> dshared_from_this() const
  {
    return dshared_ptr<ftmap_t const>(this->shared_from_this());
  }

  //bool caller_callee_represent_back_edge_in_call_graph(call_context_ref const& caller_context, call_context_ref const& callee_context) const;

  static void tfg_run_pointsto_analysis(dshared_ptr<tfg_ssa_t> t/*, callee_rw_memlabels_t::get_callee_rw_memlabels_fn_t get_callee_rw_memlabels*/, bool is_asm_tfg, dshared_ptr<tfg_llvm_t const> const& src_tfg, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = {}/*, context::xml_output_format_t xml_output_format*/);


  static bool function_name_represents_mallocfree(string const& caller_name, string const& fun_name, pc const& p, state const& fcall_state, string const& mem_alloc_reg, string const& mem_reg, size_t addrlen, state& heap_allocfree_state, unordered_set<expr_ref>& heap_allocfree_assumes);

  //bool ftmap_find_or_create_callee_context(call_context_ref const& caller_name, pc const& caller_pc, string const& callee_name);

  dshared_ptr<tfg_ssa_t> ftmap_get_merged_tfg_for_fname(string const& fname) const;

  //static string const& get_fname_from_call_context(call_context_ref call_context)
  //{
  //  list<call_context_entry_ref> const& stack = call_context->get_stack();
  //  ASSERT(stack.size() == 1);
  //  call_context_entry_ref const& cce = stack.front();
  //  return cce->get_fname()->get_str();
  //}

  set<string> ftmap_identify_potential_entry_functions() const;
  context_sensitive_ftmap_dfa_val_t<pointsto_val_t> ftmap_identify_start_pc_value_for_entry_function(call_context_ref const& fname, graph_locs_map_t& flocs) const;

  dshared_ptr<tfg> ftmap_identify_tfg_for_call_context(call_context_ref const& cc) const;
  call_context_ref ftmap_identify_most_relevant_call_context(call_context_ref const& cc) const;
  call_context_ref ftmap_identify_most_relevant_call_context_or_null(call_context_ref const& cc) const;
  //bool ftmap_contains_tfg_for_call_context(call_context_ref const& cc) const
  //{
  //  return m_fname_tfg_map.count(cc) != 0;
  //}

private:
  void ftmap_construct_call_graph();
  template<bool POSTDOM> map<ftmap_call_graph_pc_t, list<ftmap_call_graph_pc_t>> ftmap_find_call_dominators() const;
  list<ftmap_call_graph_edge_t> ftmap_identify_call_edges_for_tfg(call_context_ref const& cc, dshared_ptr<tfg> const& t) const;
  list<ftmap_call_graph_edge_t> ftmap_identify_all_call_edges() const;
  //list<ftmap_call_loop_edge_t> ftmap_identify_back_call_edges() const;
  void ftmap_set_relevant_memlabels_for_each_tfg(relevant_memlabels_t const& relevant_mls);
  void ftmap_populate_relevant_memlabels_for_each_tfg(int call_context_depth);
  void ftmap_populate_non_memslot_locs_for_each_tfg(void);
  set<memlabel_ref> ftmap_identify_heapalloc_and_local_memlabels(int call_context_depth) const;
  void ftmap_postprocess_after_pointsto_analysis_on_function(call_context_ref const& fname, set<string> const& func_call_chain, context::xml_output_format_t xml_outptut_format);

  //list<ftmap_call_graph_pc_t> ftmap_get_entry_pcs() const;
  //list<ftmap_call_graph_pc_t> ftmap_get_exit_pcs() const;

  //void ftmap_run_pointsto_analysis_on_function(string const& fun_name, set<string> const& func_call_chain, context::xml_output_format_t xml_outptut_format);
  static bool function_name_represents_malloc(string const& caller_name, string const& fun_name, pc const& p, state const& fcall_state, string const& mem_alloc_reg, string const& mem_reg, size_t addrlen, state& heap_allocfree_state, unordered_set<expr_ref>& heap_allocfree_assumes);
  static bool function_name_represents_free(string const& caller_name, string const& fun_name, pc const& p, state const& fcall_state, string const& mem_alloc_reg, string const& mem_reg, size_t addrlen, state& heap_allocfree_state, unordered_set<expr_ref>& heap_allocfree_assumes);
  static expr_ref fcall_malloc_state_identify_retaddr(state const& fcall_state);
};

}
