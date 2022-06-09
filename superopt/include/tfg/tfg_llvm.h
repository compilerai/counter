#pragma once

#include "gsupport/scev_toplevel.h"
#include "gsupport/scev_expr.h"
#include "gsupport/reaching_defs.h"

#include "tfg/tfg.h"
//#include "tfg/tfg_ssa.h"
#include "tfg/tfg_alias_result.h"

namespace eqspace {

class rdefs_locs_t : public reaching_defs_t<expr_ref, edge_id_t<pc>>
{
public:
  rdefs_locs_t() : reaching_defs_t()
  { }

  rdefs_locs_t(reaching_defs_t<expr_ref, edge_id_t<pc>> const& rdefs) : reaching_defs_t(rdefs)
  { }

  rdefs_locs_t(istream& is, context* ctx) : rdefs_locs_t(rdefs_locs_t::read_rdefs_locs_map(is, ctx))
  { }

  void rdefs_locs_to_stream(ostream& os) const;

  static std::function<rdefs_locs_t (pc const &)>
  get_boundary_value(dshared_ptr<graph<pc, tfg_node, tfg_edge> const> g)
  {
    auto f = [g](pc const &p)
    {
      ASSERT(p.is_start());
      dshared_ptr<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const> gp = dynamic_pointer_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const>(g);

      context* ctx = gp->get_context();
      // add input args
      graph_arg_regs_t const &arg_regs = gp->get_argument_regs();
      rdefs_locs_t retval;
      //tfg_edge_ref start_edge = mk_tfg_edge(pc::start(), pc::start(), expr_true(ctx), state(), unordered_set<expr_ref>(), te_comment_t(false, 0,"start-edge"));
      edge_id_t<pc> start_edge(pc::start(), pc::start());
      for (auto const& arg : arg_regs.get_map()) {
        if (arg.second.get_val()->is_var() && (arg.second.get_val()->is_bv_sort() || arg.second.get_val()->is_bool_sort())) {
          //retval.insert(arg.second->get_name());
          retval.add_reaching_definition(arg.second.get_val(), start_edge);
        }
      }

      expr_ref mem = gp->get_memvar_version_at_pc(pc::start());
      expr_ref mem_alloc = gp->get_memallocvar_version_at_pc(pc::start());
      //state const& start_state = gp->get_start_state();
      //bool ret = start_state.get_memory_expr(start_state, mem);
      //ASSERT(ret);
      ASSERT(mem->is_var());
      //expr_ref mem_alloc = get_corresponding_mem_alloc_from_mem_expr(mem);

      //add memmasks
      set<memlabel_ref> const& mls = gp->graph_get_relevant_memlabels().relevant_memlabels_get_ml_set();
      for (auto const& ml : mls) {
        retval.add_reaching_definition(ctx->mk_memmask(mem, mem_alloc, ml->get_ml()), start_edge);
      }

      return retval;
    };
    return f;
  }
private:

  static map<expr_ref, set<edge_id_t<pc>>> read_rdefs_locs_map(istream& is, context* ctx);
};



class tfg_llvm_t : public tfg
{
private:
//  map<pc, set<expr_ref>> m_defined_ssa_varnames_map;
  map<pc, rdefs_locs_t> m_locs_reaching_definitions_map;
//  map<string_ref, graph_loc_id_t> m_varname_loc_id_map;
//  map<string_ref, list<dshared_ptr<tfg_edge const>>> m_varname_modification_edges_map;
  map<string_ref, lr_status_ref> m_varname_lr_status_map;
  map<string_ref, string_ref> m_source_declarations_to_llvm_varnames; //map from source code variable names to LLVM variable names (as obtained from llvm.debug.declare annotations)
  map<string_ref, pair<pc, string_ref>> m_input_llvm_to_source_varnames; //from llvm varname to <PC, source varname> (because the same source varname can have different LLVM varnames at different PCs, we also maintain the PC value)
  map<pc, map<string_ref, string_ref>> m_pc_to_llvm_to_source_varname_map; //map derived using DFA on m_input_llvm_to_source_varnames

  map<pc, string> m_pc_linename_map; //map from pc to C source linename (linenumber)
  map<pc, string> m_pc_column_name_map; //map from pc to C source column name (column number)
  map<pc, string> m_pc_line_and_column_map; //map from pc to C source line and column name (combined)

  mutable map<pc, tfg_suffixpath_t> m_suffixpaths;

//  mutable map<pair<pc,pc>, map<expr_ref, int>> m_invariant_inference_exprs_ranking_map;  
  // Int for multiple ranking levels
  // Right now considering the exprs with unroll factor 1 as rank 0 (most relevant) than exprs at higher unroll factor

//  map<pc, pc> m_idominator_relation;
//  map<pc, pc> m_ipostdominator_relation;
  map<string_ref, scev_toplevel_t<pc>> m_potential_scev_relations;

  //set<memlabel_ref> m_relevant_memlabels;

  list<string> m_sorted_bbl_indices;

  //acting as cache
  //mutable map<pair<shared_ptr<tfg_edge_composition_t>,bool>, expr_ref> m_ec_econd_map;
  mutable map<tfg_suffixpath_t, expr_ref> m_suffixpath2expr;
  mutable set<allocsite_t> m_addr_taken_locals;
private:
  //void tfg_llvm_get_relevant_memlabels_except_args(set<memlabel_ref> &relevant_memlabels) const;
  expr_ref tfg_llvm_expr_substitute_using_avail_exprs_till_have_source_mappings_for_all(expr_ref const& e, avail_exprs_t const& avail_exprs);
  void tfg_llvm_fill_remaining_llvm_to_source_mappings_using_avail_exprs_at_pc(pc const& p, avail_exprs_t const& avail_exprs, context::xml_output_format_t xml_output_format);
  expr_ref tfg_suffixpath_get_expr_helper(tfg_suffixpath_t const &src_ec) const;
  //virtual void get_path_enumeration_stop_pcs(set<pc> &stop_pcs) const override;
  lr_status_ref get_lr_status_for_varname(string const& varname) const;
  expr_ref get_expr_for_varname(string const& varname) const;
  void tfg_llvm_populate_varname_loc_id();
  void tfg_llvm_populate_varname_lr_status();
//  void tfg_llvm_populate_dominator_and_postdominator_relations();
  symbol_id_t varname_is_symbol(string const& varname) const;
  bool tfg_llvm_memlabel_max_object_size_is_less_than_limit(memlabel_t const& ml, size_t limit) const;
  //bool loc_represents_entire_symbol(string const& var, uint64_t size) const;
  void resolve_unknown_size_if_possible(memlabel_t const& ml, string const& var, uint64_t& size) const;
  dshared_ptr<pc> get_pc_following_definition_of_varname(string const& var) const;
  static bool varname_is_constexpr(string const& varname);
  bool varname_is_function_argument(string const& varname) const;

public:
  tfg_llvm_t(string const &name, string const& fname, context *ctx/*, state const &start_state*/) : tfg(name, fname, ctx/*, start_state*/)
  { }

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override;
  virtual dshared_ptr<tfg> tfg_ssa_copy() const override;
  virtual dshared_ptr<tfg> tfg_copy() const override;
//  virtual dshared_ptr<graph_with_ssa_funcs<pc, tfg_node, tfg_edge, predicate>> construct_ssa_graph(map<graph_loc_id_t, graph_cp_location> const &llvm_locs = {}) const override;

  void add_scev_mapping(string_ref const& s, scev_toplevel_t<pc> const& scevt);

  void tfg_llvm_print_llvm2source_mapping(ostream& os) const;

  tfg_alias_result_t tfg_llvm_get_aliasing_relationship(string const& varA, uint64_t sizeA, string const& varB, uint64_t sizeB) const;

  //virtual void get_path_enumeration_reachable_pcs(pc const &from_pc, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, set<pc> &reachable_pcs) const override;
  //virtual void tfg_preprocess(bool is_asm_tfg, dshared_ptr<tfg_llvm_t const> src_llvm_tfg/*, list<string> const& sorted_bbl_indices = {}, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>()*/, context::xml_output_format_t xml_output_format = context::XML_OUTPUT_FORMAT_TEXT_NOCOLOR) override;

  //void tfg_run_pointsto_analysis(bool is_asm_tfg/*, list<string> const& sorted_bbl_indices = {}*/, callee_rw_memlabels_t::get_callee_rw_memlabels_fn_t get_callee_rw_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>(), context::xml_output_format_t xml_output_format = context::XML_OUTPUT_FORMAT_TEXT_NOCOLOR) override;
  void tfg_postprocess_after_pointsto_analysis(bool is_asm_tfg/*, map<call_context_ref, map<pc, pointsto_val_t>> const& vals*/, context::xml_output_format_t xml_output_format = context::XML_OUTPUT_FORMAT_TEXT_NOCOLOR) override;

  callee_summary_t tfg_llvm_compute_callee_summary() const;

  void populate_exit_return_values_for_llvm_method();
//  map<expr_ref,int> const& get_invariant_inference_exprs_ranking_map_for_path_id(tfg_path_id_t const& path_id) const { 
//    context *ctx = this->get_context();
//    if(!m_invariant_inference_exprs_ranking_map.count(path_id))
//      this->populate_interesting_exprs_for_cg_invariant_inference(path_id, min(ctx->get_config().path_exprs_lookahead, ctx->get_config().unroll_factor));
//    ASSERT(m_invariant_inference_exprs_ranking_map.count(path_id));
//    return m_invariant_inference_exprs_ranking_map.at(path_id); 
//  }
  void tfg_llvm_add_source_declaration_to_llvm_varname_mapping_at_pc(string const& source_varname, string const& llvm_varname, pc const& p);
  void tfg_llvm_add_source_to_llvm_varname_mapping_at_pc(string const& source_varname, string const& llvm_varname, pc const& p);
  void tfg_llvm_add_pc_line_number_mapping(pc const& p, unsigned linenum, unsigned column_num);
  void tfg_llvm_fill_remaining_llvm_to_source_mappings_using_avail_exprs(context::xml_output_format_t xml_output_format);
//  void tfg_llvm_populate_varname_modification_edges();
  void tfg_llvm_populate_reaching_definitions();

  set<memlabel_ref> tfg_llvm_get_relevant_memlabels() const;
  //void tfg_llvm_get_relevant_memlabels(set<memlabel_ref> &relevant_memlabels) const;

  dshared_ptr<tfg_llvm_t const> tfg_llvm_dshared_from_this() const
  {
    dshared_ptr<tfg_llvm_t const> ret = dynamic_pointer_cast<tfg_llvm_t const>(this->shared_from_this());
    ASSERT(ret);
    return ret;
  }

  void tfg_llvm_recompute_line_and_column_map(set<pc> const& pcs);
  void canonicalize_llvm_nextpcs(dshared_ptr<tfg_llvm_t const> src_llvm_tfg);

  void populate_suffixpaths() const;
  tfg_suffixpath_t const& get_suffixpath_ending_at_pc(pc const &p) const
  {
    if (!m_suffixpaths.count(p)) {
      cout << __func__ << " " << __LINE__ << ": !m_suffixpaths.count(p). p = " << p.to_string() << endl;
    }
    ASSERT(m_suffixpaths.count(p));
    return m_suffixpaths.at(p);
  }
  void populate_suffixpath2expr_map() const;
  expr_ref tfg_suffixpath_get_expr(tfg_suffixpath_t const &sfpath) const;

  void populate_address_taken_locals() const;
  set<allocsite_t> tfg_get_addr_taken_locals() const;

  string_ref get_llvm_varname_for_source_varname_declaration(string_ref const& source_varname) const { if (m_source_declarations_to_llvm_varnames.count(source_varname)) return m_source_declarations_to_llvm_varnames.at(source_varname); else return nullptr; }
  void tfg_llvm_populate_varname_definedness();

  virtual tfg_llvm_t const &get_src_tfg() const override //llvm tfg is the tfg against which the edge-guards are computed
  {
    return *this;
  }
  eqclasses_ref compute_expr_eqclasses_at_pc(pc const& p) const override;
  //local_sprel_expr_guesses_t get_local_sprel_expr_assumes() const override { static local_sprel_expr_guesses_t empty; return empty; }
  //virtual expr_ref tfg_edge_composition_get_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified) const override;
  //virtual list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> collect_assumes_around_edge(dshared_ptr<tfg_edge const> const& te) const override;
  //virtual map<pair<pc, int>, shared_ptr<tfg_full_pathset_t const>> get_composite_path_between_pcs_till_unroll_factor(pc const& from_pc, set<pc> const& to_pcs/*, map<pc,int> const& visited_count*/, int unroll_factor, set<pc> const& stop_pcs) const override;

  list<string> tfg_get_sorted_bbl_indices() const override;

  void tfg_llvm_set_sorted_bbl_indices(list<string> const& sorted_bbl_indices);
  void tfg_llvm_rename_vars_in_expr_at_pc_for_xml_printing(expr_ref& e, pc const& p) const;
  void tfg_llvm_rename_vars_in_counter_example_at_pc_for_xml_printing(counter_example_t& e, pc const& p) const;
  //virtual expr_ref tfg_edge_composition_get_unsubstituted_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified) const override;
  map<pc, string> const& tfg_llvm_get_pc_line_and_column_map() const;

  bool tfg_llvm_edge_represents_stacksave(dshared_ptr<tfg_edge const> const& e) const;
  bool tfg_llvm_edge_represents_stackrestore(dshared_ptr<tfg_edge const> const& e) const;
  bool tfg_llvm_edge_represents_alloca(dshared_ptr<tfg_edge const> const& e) const;

  set<allocsite_t> const& tfg_llvm_get_locals_active_at_pc(pc const& p) const;

  void tfg_llvm_add_explicit_deallocs_for_stackrestores();

  void tfg_llvm_eliminate_memalloc_ssa_assignments();
  //virtual bool graph_has_stack() const override { return false; }

  static dshared_ptr<tfg_llvm_t> tfg_llvm_from_stream(istream& is, string const& name, context* ctx);
  static dshared_ptr<tfg_llvm_t> tfg_llvm_copy(tfg_llvm_t const& other);
  //static unique_ptr<tfg_llvm_t> tfg_llvm_copy_to_unique_ptr(tfg_llvm_t const& other);

  //void tfg_llvm_initialize_uninit_nonce_on_start_edge(set<local_id_t> const& lids, string const& srcdst_keyword);
  unsigned tfg_llvm_get_num_alloca() const;

  void tfg_llvm_populate_structs_after_construction(bool compute_suffixpaths = false);
  void tfg_llvm_add_start_pc_preconditions(/*string const& srcdst_keyword*/);

  //bool tfg_update_to_state_for_local_allocation_and_deallocation_at_pcs(pc_local_sprel_expr_guesses_t const& pc_lsprels_allocs, pc_local_sprel_expr_guesses_t const& pc_lsprels_deallocs) override;

// Irrespective of our internal SSA, the llvm src is always SSA
//  bool graph_is_ssa() const override { return true; }
  bool is_llvm_graph() const override { return true; }

  virtual void tfg_add_ghost_vars() override;

  set<string_ref> graph_get_externally_visible_variables() const override;

protected:
  tfg_llvm_t(istream& is, string const &name, context *ctx);
  tfg_llvm_t(tfg_llvm_t const &other) : tfg(other),
                                        //m_defined_ssa_varnames_map(other.m_defined_ssa_varnames_map),
                                        m_locs_reaching_definitions_map(other.m_locs_reaching_definitions_map),
                                        //m_varname_loc_id_map(other.m_varname_loc_id_map),
                                        //m_varname_modification_edges_map(other.m_varname_modification_edges_map),
                                        m_varname_lr_status_map(other.m_varname_lr_status_map),
                                        m_source_declarations_to_llvm_varnames(other.m_source_declarations_to_llvm_varnames),
                                        m_input_llvm_to_source_varnames(other.m_input_llvm_to_source_varnames),
                                        m_pc_to_llvm_to_source_varname_map(other.m_pc_to_llvm_to_source_varname_map),
                                        m_pc_linename_map(other.m_pc_linename_map),
                                        m_pc_column_name_map(other.m_pc_column_name_map),
                                        m_pc_line_and_column_map(other.m_pc_line_and_column_map),
                                        m_suffixpaths(other.m_suffixpaths),
                                        //m_idominator_relation(other.m_idominator_relation),
                                        //m_ipostdominator_relation(other.m_ipostdominator_relation),
                                        m_potential_scev_relations(other.m_potential_scev_relations),
                                        //m_ec_econd_map(other.m_ec_econd_map),
                                        m_suffixpath2expr(other.m_suffixpath2expr),
                                        m_addr_taken_locals(other.m_addr_taken_locals)
  { }

  void graph_ssa_copy(tfg_llvm_t const& other)
  {
    tfg::graph_ssa_copy(other);
    m_source_declarations_to_llvm_varnames = other.m_source_declarations_to_llvm_varnames;
    m_input_llvm_to_source_varnames = other.m_input_llvm_to_source_varnames;
    m_pc_to_llvm_to_source_varname_map = other.m_pc_to_llvm_to_source_varname_map;
    m_pc_linename_map = other.m_pc_linename_map;
    m_pc_column_name_map = other.m_pc_column_name_map;
    m_pc_line_and_column_map = other.m_pc_line_and_column_map;
    //m_pc_to_active_locals = other.m_pc_to_active_locals;  tfg_llvm_populate_locals_scope_information
    m_potential_scev_relations = other.m_potential_scev_relations;
    //m_addr_taken_locals = other.m_addr_taken_locals; populate_address_taken_locals
    //populate_suffixpaths()
//    ASSERT(other.m_scope_id_to_pcs.empty());
  }
private:

  set<allocsite_t> tfg_llvm_identify_locals_with_explicit_alloca() const;

  set<tfg_edge_ref> tfg_llvm_get_edge_set_from_edge_ids(set<edge_id_t<pc>> const& eids) const;
  set<expr_ref> identify_loop_invariant_exprs(dshared_ptr<graph_region_node_t<pc, tfg_node, tfg_edge> const> const& loop_region_node) const;
  expr_ref tfg_llvm_get_expr_from_scev(scev_ref const& scev) const;
  expr_ref tfg_llvm_get_loop_entry_value_for_expr(expr_ref const& e, dshared_ptr<graph_region_node_t<pc, tfg_node, tfg_edge> const> const& loop) const;
  expr_ref tfg_llvm_get_begin_scev_expr_using_var_and_its_scev_toplevel(expr_ref const& e, expr_ref const& var, scev_toplevel_t<pc> const& var_scev_toplevel, pc const& loop_head, set<expr_ref> const& loop_invariant_exprs) const;
  expr_ref tfg_llvm_get_end_scev_expr_using_var_and_its_scev_toplevel(expr_ref const& e, expr_ref const& var, scev_toplevel_t<pc> const& var_scev_toplevel, pc const& loop_head, set<expr_ref> const& loop_invariant_exprs) const;

  dshared_ptr<scev_expr_t const> tfg_llvm_expr_represents_scev_addr_memaccess(expr_ref const& e, dshared_ptr<graph_region_node_t<pc, tfg_node, tfg_edge> const> const& loop, set<expr_ref> const& loop_invariant_exprs) const;
  //bool tfg_llvm_expr_involves_only_loop_invariants(expr_ref const& e, dshared_ptr<graph_region_node_t<pc, tfg_node, tfg_edge> const> const& loop) const;
  //expr_ref tfg_llvm_expr_involves_scev_memaccess_and_loop_invariants_only(expr_ref const& cond, dshared_ptr<graph_region_node_t<pc, tfg_node, tfg_edge> const> const& loop) const;
  set<expr_ref> tfg_llvm_identify_defined_varnames_at_loop_exit(dshared_ptr<graph_region_node_t<pc, tfg_node, tfg_edge> const> const& loop) const;
  bool varname_is_modified_in_loop(string_ref const& varname, pc const& p) const;
  bool varname_has_scev_addrec_mapping_in_loop(string_ref const& varname, pc const& p) const;
  expr_group_ref compute_container_invariants_eqclass(pc const& p) const;
  vector<expr_group_ref> compute_ineq_eqclasses(pc const& p, set<expr_ref> const& interesting_exprs) const;
  virtual bool graph_edge_contains_unknown_function_call(dshared_ptr<tfg_edge const> const& e) const override;
//  set<expr_ref> compute_interesting_exprs_at_pc(pc const& p, int unroll_factor) const;
  map<string_ref, string_ref> get_llvm_to_source_varname_map_at_pc(pc const& p) const;
  void tfg_llvm_compute_pc_to_llvm_to_source_varname_map();

  void tfg_add_ghost_vars_for_local_allocations();
};

struct make_dshared_enabler_for_tfg_llvm_t : public tfg_llvm_t { template <typename... Args> make_dshared_enabler_for_tfg_llvm_t(Args &&... args):tfg_llvm_t(std::forward<Args>(args)...) {} };

}
