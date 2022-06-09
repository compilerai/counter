#pragma once

#include "gsupport/pc_local_sprel_expr_guesses.h"
#include "tfg/sp_version_relations_dfa.h"
#include "tfg/tfg.h"
//#include "tfg/tfg_ssa.h"

namespace eqspace {

class tfg_asm_t : public tfg
{
private:
  static map<string_ref, string_ref> m_asm_regnames;
  map<expr_ref,set<expr_ref>> m_sp_version_to_parent_sp_versions_map;
  map<pc,expr_ref> m_pc_to_sp_version_map;
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> m_pc_lsprel_assumes;
  map<pc,sp_version_relations_val_t> m_pc_to_sp_version_relations;

public:
  tfg_asm_t(string const &name, string const& fname, context *ctx) : tfg(name, fname, ctx)
  { }
  void graph_to_stream(ostream& ss, string const& prefix) const override;

  list<string> tfg_get_sorted_bbl_indices() const override;

  virtual tfg_llvm_t const &get_src_tfg() const override { NOT_REACHED(); }
  virtual dshared_ptr<tfg> tfg_ssa_copy() const override;
  virtual dshared_ptr<tfg> tfg_copy() const override;

  //virtual void tfg_preprocess(bool is_asm_tfg, dshared_ptr<tfg_llvm_t const> src_llvm_tfg/*, list<string> const& sorted_bbl_indices = {}, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>()*/, context::xml_output_format_t xml_output_format = context::XML_OUTPUT_FORMAT_TEXT_NOCOLOR) override;
  //virtual void get_path_enumeration_stop_pcs(set<pc> &stop_pcs) const override;
  //virtual void get_path_enumeration_reachable_pcs(pc const &from_pc, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, set<pc> &reachable_pcs) const override;

  //virtual map<pair<pc, int>, shared_ptr<tfg_full_pathset_t const>> get_composite_path_between_pcs_till_unroll_factor(pc const& from_pc, set<pc> const& to_pcs/*, map<pc,int> const& visited_count*/, int unroll_factor, set<pc> const& stop_pcs) const override;
  void tfg_asm_rename_vars_in_expr_for_xml_printing(expr_ref& e, pc const& p) const;

  //map<pc, string> tfg_asm_read_asm_pc_linename_map(string const& asm_filename, string const& function_name) const;

  static map<string_ref, string_ref> init_asm_regnames();

  dshared_ptr<tfg_asm_t const> tfg_asm_shared_from_this() const
  {
    return dynamic_pointer_cast<tfg_asm_t const>(this->shared_from_this());
  }

  void tfg_asm_add_static_single_assignments_for_stack_pointer(relevant_memlabels_t const& relevant_memlabels);
  void tfg_asm_add_alloc_edge_at_pc(pc const& p, allocsite_t const& lid, expr_ref const& lsp_addr, expr_ref const& local_alloc_count_var);
  void tfg_asm_add_dealloc_edge_at_pc(pc const& p, allocsite_t const& lid, expr_ref const& addr);

  eqclasses_ref compute_expr_eqclasses_at_pc(pc const& p) const override;
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> tfg_asm_prune_implausible_pc_local_sprel_expr_assumes(map<local_sprel_expr_guesses_t,list<pc>> const& lsprels_to_potential_pcs) const;
  void tfg_asm_populate_sp_version_to_parent_sp_versions_map();
  set<expr_ref> get_parent_sp_versions_of(expr_ref const& e) const;

  void tfg_asm_populate_pc_to_sp_version_map();
  map<pc,expr_ref> const& get_pc_to_sp_version_map() const;
  pc get_pc_for_sp_version(expr_ref const& e) const;

  void set_pc_local_sprel_expr_assumes(pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> const& pc_lsprels) { m_pc_lsprel_assumes = pc_lsprels; }
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> tfg_asm_get_pc_local_sprel_expr_assumes() const { return m_pc_lsprel_assumes; }
  expr_ref find_closest_sp_version_for_sp_expression(expr_ref const& sp_e) const;
  virtual unsigned tfg_get_num_sp_updates() const override;

  bool is_llvm_graph() const override { return false; }

  virtual set<graph_loc_id_t> graph_get_defined_locids_at_entry(pc const& entry_pc) const override;
  //virtual set<graph_loc_id_t> graph_get_defined_locids_at_entry() const override;

  vector<exreg_id_t> tfg_asm_get_callee_save_gprs() const;
  set<string_ref> tfg_asm_get_callee_preserved_regs() const;

  virtual set<graph_loc_id_t> graph_get_live_locids_at_boundary_pc(pc const& p) const override;

  bool tfg_asm_expr_represents_sp_derived_value(expr_ref const& e) const;

  void tfg_asm_compute_sp_version_relations();
  list<shared_ptr<predicate const>> get_sp_version_relations_preds_at_pc(pc const& p) const override;

  static dshared_ptr<tfg_asm_t> tfg_asm_from_stream(istream& is, string const& name, context* ctx);
  static dshared_ptr<tfg_asm_t> tfg_asm_copy(tfg_asm_t const& other);
  static dshared_ptr<tfg_asm_t> tfg_asm_copy(tfg const& other);
  //static unique_ptr<tfg_asm_t> tfg_asm_copy_to_unique_ptr(tfg_asm_t const& other);
  //static unique_ptr<tfg_asm_t> tfg_asm_copy_to_unique_ptr(tfg const& other);
  void tfg_asm_populate_structs_after_construction();
  set<string_ref> graph_get_externally_visible_variables() const override
  {
    set<string_ref> ret = graph_with_var_versions::graph_get_externally_visible_variables();
    // add callee-preserved registers
    //ret.insert(state::get_stack_pointer_regname());
    for (auto const& regname : this->tfg_asm_get_callee_preserved_regs()) {
      ret.insert(regname);
    }
    context* ctx = this->get_context();
    for (auto const& allocsite : this->get_locals_map().graph_locals_map_get_allocsites()) {
      string_ref local_size_key = ctx->get_local_size_key_for_id(allocsite, G_DST_KEYWORD);
      if (this->get_start_state().start_state_has_expr(local_size_key)) {
        ret.insert(local_size_key);
      }
    }
    return ret;
  }
  virtual void tfg_add_ghost_vars() override
  {
    tfg_asm_add_dst_local_size_vars_to_start_state();
  }

protected:
  tfg_asm_t(istream& is, string const &name, context *ctx);
  tfg_asm_t(tfg_asm_t const &other)
  : tfg(other),
    m_sp_version_to_parent_sp_versions_map(other.m_sp_version_to_parent_sp_versions_map),
    m_pc_to_sp_version_map(other.m_pc_to_sp_version_map),
    m_pc_lsprel_assumes(other.m_pc_lsprel_assumes),
    m_pc_to_sp_version_relations(other.m_pc_to_sp_version_relations)
  { }
  tfg_asm_t(tfg const &other) : tfg(other)
  { }

  void graph_ssa_copy(tfg_asm_t const& other)
  {
    tfg::graph_ssa_copy(other);
    m_sp_version_to_parent_sp_versions_map = other.m_sp_version_to_parent_sp_versions_map;
    m_pc_to_sp_version_map = other.m_pc_to_sp_version_map;
    m_pc_lsprel_assumes = other.m_pc_lsprel_assumes;
    m_pc_to_sp_version_relations = other.m_pc_to_sp_version_relations;
  }
  
private:
  //void tfg_asm_add_dealloc_edges_in_path(shared_ptr<tfg_full_pathset_t const> const& dst_path, list<alloc_dealloc_t> const& alloc_dealloc_ls);
  //void tfg_asm_add_dealloc_edge_in_path(shared_ptr<tfg_full_pathset_t const> const& dst_path, alloc_dealloc_t const& dealloc, list<alloc_dealloc_t> const& alloc_dealloc_ls);

  //bool tfg_update_to_state_for_local_deallocation(shared_ptr<tfg_full_pathset_t const> const& src_path, shared_ptr<tfg_full_pathset_t const> const& dst_path);
  bool sp_version_represents_sp_update(expr_ref const& sp_version) const;
  bool tfg_asm_sp_version_matches_stack_pointer_on_edge(dshared_ptr<tfg_edge const> const& ed, expr_ref const& spv) const;

  bool has_sp_version_at_pc(pc const& p) const;
  expr_ref get_sp_version_at_pc(pc const& p) const;

  virtual bool graph_edge_contains_unknown_function_call(dshared_ptr<tfg_edge const> const& e) const override;
  //void get_anchor_pcs(set<pc>& anchor_pcs) const override;
  expr_ref find_closest_sp_version_equal_to_expr(pc const& pp, expr_ref const& target_expr, shared_ptr<tfg_edge_composition_t const> pth = tfg_edge_composition_t::mk_epsilon_ec()) const;
  set<expr_ref> get_sp_update_parent_sp_versions_of(map<expr_ref,set<expr_ref>> const& sp_version_to_parent_sp_versions, expr_ref const& e) const;

  void tfg_asm_add_dst_local_size_vars_to_start_state()
  {
    context* ctx = this->get_context();
    map<string_ref, expr_ref> new_start_state_keys; //the keys that would potentially need to be externally visible (e.g., in the CG)
    for (auto const& allocsite : this->get_locals_map().graph_locals_map_get_allocsites()) {
      if (local_id_refers_to_arg(allocsite, this->get_argument_regs(), this->get_locals_map()))
        continue;
      expr_ref lsp_size = ctx->get_local_size_expr_for_id(allocsite, ctx->get_addr_sort(), G_DST_KEYWORD);
      string lsp_size_key = ctx->get_key_from_input_expr(lsp_size)->get_str();
      new_start_state_keys.insert(make_pair(mk_string_ref(lsp_size_key), lsp_size));
    }
    this->graph_add_keys_and_exprs_to_start_state(new_start_state_keys);
  }
};

struct make_dshared_enabler_for_tfg_asm_t : public tfg_asm_t { template <typename... Args> make_dshared_enabler_for_tfg_asm_t(Args &&... args):tfg_asm_t(std::forward<Args>(args)...) {} };

}
