#pragma once

#include <list>
#include <vector>
#include <map>
#include <deque>
#include <set>

#include "support/log.h"
#include "support/mymemory.h"
#include "support/bv_rank_val.h"
#include "tfg/tfg.h"
#include "tfg/tfg_asm.h"
#include "tfg/edge_guard.h"
#include "expr/counter_example.h"
#include "graph/expr_group.h"
#include "graph/graph_with_guessing.h"
#include "gsupport/corr_graph_edge.h"

namespace eqspace {

class suffixpath;
class eqcheck;

using namespace std;


class corr_graph : public graph_with_guessing<pcpair, corr_graph_node, corr_graph_edge, predicate>
{
public:
  corr_graph(string_ref const &name, shared_ptr<eqcheck> const& eq) :
    graph_with_guessing<pcpair, corr_graph_node, corr_graph_edge, predicate>(name->get_str(), eq->get_context(), eq->get_memlabel_assertions()),
    m_eq(eq), m_dst_tfg(make_unique<tfg_asm_t>(eq->get_dst_tfg()))
  {
    init_cg_elems();
    set<cs_addr_ref_id_t> const &src_relevant_addr_refs = eq->get_src_tfg().graph_get_relevant_addr_refs();
    set<cs_addr_ref_id_t> const &dst_relevant_addr_refs = m_dst_tfg->graph_get_relevant_addr_refs();
    set<cs_addr_ref_id_t> relevant_addr_refs;
    set_union(src_relevant_addr_refs, dst_relevant_addr_refs, relevant_addr_refs);
    this->set_relevant_addr_refs(relevant_addr_refs);
    tfg const& src_tfg = eq->get_src_tfg();
    tfg const& dst_tfg = eq->get_dst_tfg();
    this->set_symbol_map(eq->get_symbol_map());
    this->set_locals_map(src_tfg.get_locals_map());
    this->set_argument_regs(src_tfg.get_argument_regs());
    predicate_set_t global_assumes = src_tfg.get_global_assumes();
    unordered_set_union(global_assumes, dst_tfg.get_global_assumes());
    this->set_global_assumes(global_assumes);
    stats_num_cg_constructions++;
  }

  corr_graph(corr_graph const &other) : graph_with_guessing<pcpair, corr_graph_node, corr_graph_edge, predicate>(other),
                                        m_eq(other.m_eq),
                                        m_dst_start_loc_id(other.m_dst_start_loc_id),
                                        m_cg_locs_for_dst_rodata_symbol_masks_on_src_mem(other.m_cg_locs_for_dst_rodata_symbol_masks_on_src_mem),
                                        m_dst_tfg(make_unique<tfg_asm_t>(*other.m_dst_tfg)),
                                        m_src_suffixpath(other.m_src_suffixpath),
                                        m_dst_fcall_edges_already_updated_from_pcs(other.m_dst_fcall_edges_already_updated_from_pcs),
                                        m_local_sprel_expr_assumes(other.m_local_sprel_expr_assumes),
                                        //stats
                                        m_num_src_to_pcs_dst_ec(other.m_num_src_to_pcs_dst_ec),
                                        m_num_src_ecs_dst_ec(other.m_num_src_ecs_dst_ec),
                                        m_non_mem_assumes_cache(other.m_non_mem_assumes_cache)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    stats_num_cg_constructions++;
  }

  corr_graph(istream& is, string const& name, shared_ptr<eqcheck> const& eq) : graph_with_guessing<pcpair, corr_graph_node, corr_graph_edge, predicate>(is, name, eq->get_context()), m_eq(eq), m_dst_tfg(make_unique<tfg_asm_t>(eq->get_dst_tfg()))
  {
    init_cg_elems();
    stats_num_cg_constructions++;
  }

  ~corr_graph()
  {
    stats_num_cg_destructions++;
  }

  void cg_add_node(shared_ptr<corr_graph_node> n);

  tfg const& get_dst_tfg() const { return *m_dst_tfg; }
  tfg &get_dst_tfg()             { return *m_dst_tfg; }
  void set_dst_tfg(tfg_asm_t const &t) { m_dst_tfg = make_unique<tfg_asm_t>(t); }

  void add_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &local_sprel_expr_assumes) const
  {
    //m_local_sprel_expr_assumes = local_sprel_expr_assumes;
    m_local_sprel_expr_assumes.set_union(local_sprel_expr_assumes);
  }

  virtual local_sprel_expr_guesses_t get_local_sprel_expr_assumes() const override
  {
    return m_local_sprel_expr_assumes;
  }

  virtual aliasing_constraints_t get_aliasing_constraints_for_edge(corr_graph_edge_ref const& e) const override;
  virtual aliasing_constraints_t graph_apply_trans_funs_on_aliasing_constraints(corr_graph_edge_ref const& e, aliasing_constraints_t const& als) const override;

  virtual callee_summary_t get_callee_summary_for_nextpc(nextpc_id_t nextpc_num, size_t num_fargs) const override;

  tfg const &get_src_tfg() const override { return m_eq->get_src_tfg(); }
  pred_avail_exprs_t const &get_src_pred_avail_exprs_at_pc(pcpair const &pp) const override
  {
    return this->get_src_tfg().get_pred_avail_exprs_at_pc(pp.get_first());
  }

  virtual void graph_to_stream(ostream& os) const override;
  static shared_ptr<corr_graph> corr_graph_from_stream(istream& is, context* ctx);

  edge_guard_t
  graph_get_edge_guard_from_edge_composition(graph_edge_composition_ref<pcpair,corr_graph_edge> const &ec) const override
  {
    NOT_IMPLEMENTED();
  }

  //virtual void graph_get_relevant_memlabels(vector<memlabel_ref> &relevant_memlabels) const override
  //{
  //  NOT_REACHED();
  //  //m_eq->get_src_tfg().graph_get_relevant_memlabels(relevant_memlabels);
  //  //this->get_dst_tfg().graph_get_relevant_memlabels(relevant_memlabels);
  //}
  //virtual void graph_get_relevant_memlabels_except_args(vector<memlabel_ref> &relevant_memlabels) const override
  //{
  //  m_eq->get_src_tfg().graph_get_relevant_memlabels_except_args(relevant_memlabels);
  //  this->get_dst_tfg().graph_get_relevant_memlabels_except_args(relevant_memlabels);
  //}

  //virtual void graph_populate_relevant_addr_refs() override
  //{
  //  set<cs_addr_ref_id_t> const &src_relevant_addr_refs = m_eq->get_src_tfg().graph_get_relevant_addr_refs();
  //  set<cs_addr_ref_id_t> const &dst_relevant_addr_refs = m_eq->get_dst_tfg().graph_get_relevant_addr_refs();
  //  set_union(src_relevant_addr_refs, dst_relevant_addr_refs, m_relevant_addr_refs);
  //}

  bool dst_tfg_loc_has_sprel_mapping_at_pc(pcpair const &p, graph_loc_id_t loc_id) const
  {
    return m_dst_tfg->loc_has_sprel_mapping_at_pc(p.get_second(), loc_id);
  }

  //virtual void init_graph_locs(graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>()) override;
  void cg_add_locs_for_dst_rodata_symbols(map<graph_loc_id_t, graph_cp_location>& locs);
  virtual set<expr_ref> get_live_vars(pcpair const& pp) const override
  {
    set<expr_ref> ret = m_eq->get_src_tfg().get_live_vars(pp.get_first());
    set_union(ret, m_dst_tfg->get_live_vars(pp.get_second()));
    return ret;
  }

  void populate_branch_affecting_locs() override { /*NOP*/ }
  //void populate_loc_liveness() override { NOT_REACHED(); }
  void populate_loc_definedness() override { /*NOP*/ }

  shared_ptr<eqcheck> const& get_eq() const;

  list<shared_ptr<corr_graph_node>> find_cg_nodes_with_dst_node(pc dst) const;

  bool is_edge_conditions_and_fcalls_equal();
  bool get_preds_after_guess_and_check(const pcpair& pp, expr_ref& pred);
  bool is_exit_constraint_provable();

  void cg_add_precondition(/*vector<memlabel_t> const &relevant_memlabels*/);
  void cg_add_exit_asserts_at_pc(pcpair const &pp);
  void cg_add_asserts_at_pc(pcpair const &pp);

  bool cg_check_new_cg_edge(corr_graph_edge_ref cg_new_edge);
  pair<bool,bool> corr_graph_propagate_CEs_on_edge(corr_graph_edge_ref const& cg_new_edge, bool propagate_initial_CEs = true);

  state const &get_src_start_state() const;
  state const &get_dst_start_state() const;

  bool cg_covers_dst_edge_at_pcpair(pcpair const &pp, shared_ptr<tfg_edge const> const &dst_e, tfg const &dst_tfg) const;
  bool cg_covers_all_dst_tfg_paths(tfg const &dst_tfg) const;

  friend ostream& operator<<(ostream& os, const corr_graph&);

  static void write_failure_to_proof_file(string const &filename, string const &function_name);
  void write_provable_cg_to_proof_file(string const &filename, string const &function_name) const;
  void to_dot(string filename, bool embed_info) const;
  void cg_import_locs(tfg const &tfg, graph_loc_id_t start_loc_id);

  shared_ptr<corr_graph> cg_copy_corr_graph() const;

  bool cg_src_dst_exprs_are_relatable_at_pc(pcpair const &pp, expr_ref const &src_expr, expr_ref const &dst_expr) const;

  set<local_sprel_expr_guesses_t> generate_local_sprel_expr_guesses(pcpair const &pp, predicate_set_t const &lhs, edge_guard_t const &guard, local_sprel_expr_guesses_t const &local_sprel_expr_assumes_required_to_prove, expr_ref src, expr_ref dst, set<local_id_t> const &src_locals) const override;
  bool cg_correlation_exists(corr_graph_edge_ref const& new_cg_edge) const;
  bool cg_correlation_exists_to_another_src_pc(corr_graph_edge_ref const& new_cg_edge) const;
  bool src_path_contains_already_correlated_node_as_intermediate_node(shared_ptr<tfg_edge_composition_t> const &src_path, pc const &dst_pc) const;
  void cg_print_stats() const;

  void update_dst_fcall_edge_using_src_fcall_edge(pc const &src_pc, pc const &dst_pc);
  virtual bool propagate_sprels() override;
  sprel_map_pair_t get_sprel_map_pair(pcpair const &pp) const override;

  void update_src_suffixpaths_cg(corr_graph_edge_ref e = nullptr);
  tfg_suffixpath_t get_src_suffixpath_ending_at_pc(pcpair const &pp) const override
  {
    ASSERT(m_src_suffixpath.count(pp));
    return m_src_suffixpath.at(pp);
  }

  bool dst_edge_composition_proves_false(pcpair const &cg_from_pc, shared_ptr<tfg_edge_composition_t> const &dst_edge/*, counter_example_t &counter_example*/) const;

  virtual graph_memlabel_map_t get_memlabel_map() const override
  {
    graph_memlabel_map_t memlabel_map = get_eq()->get_src_tfg().get_memlabel_map();
    memlabel_map_union(memlabel_map, m_dst_tfg->get_memlabel_map());
    // we no longer maintain separate to_state for CG edges and therefore there is no need to keep memlabel_map for it
    //memlabel_map_intersect(memlabel_map, this->m_memlabel_map);
    /* 
     * This is sutble.
     * Intersecting with this->m_memlabel_map lends precision and can be
     * especially helpful for local_sprel_expr_guesses (not sure).
     *
     * On the other hand, just using this->m_memlabel_map is sometimes
     * insufficient because the CG's locs are formed by union-ing the locs
     * of the src and the dst tfgs. However, when we actually compose the
     * respective tfg edges and simplify them, new locs may emerge that
     * are not present in the CG's locs causing the alias analysis to
     * become overly conservative. An example is: select(mem, esp, 4)
     * is a loc in dst tfg but CG has a read on select(mem, esp, 2) and
     * the latter is not a loc in dst tfg. In this
     * example, the CG locs would not contain
     * select(mem, esp, 2) [even though there is a read on it];
     * moreover even though CG locs contain select(mem, esp, 4), we won't
     * have an entry for it in this->m_memlabel_map (but we would have an
     * entry for it in dst_tfg's memlabel_map)
     *
     * For this reason taking an
     * intersection of the two is sound and has best precision.
     */
    return memlabel_map;
  }

  bool dst_loc_is_live_at_pc(pcpair const &p, graph_loc_id_t loc_id) const { return m_dst_tfg->loc_is_live_at_pc(p.get_second(), loc_id); }

  list<pair<expr_ref, predicate_ref>> edge_composition_apply_trans_funs_on_pred(predicate_ref const &p, shared_ptr<cg_edge_composition_t> const &ec) const override;

  virtual pair<bool,bool> counter_example_translate_on_edge(counter_example_t const &c_in, corr_graph_edge_ref const &edge, counter_example_t &ret, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels) const override;
  bool dst_edge_exists_in_outgoing_cg_edges(pcpair const &pp, pc const& p) const;
  bool dst_edge_exists_in_cg_edges(shared_ptr<tfg_edge const> const& dst_edge) const;
  //shared_ptr<tfg_edge_composition_t> dst_edge_composition_subtract_those_that_exist_in_cg_edges(pcpair const &pp, shared_ptr<tfg_edge_composition_t> ec) const;
  string cg_preds_to_string_for_eq(pcpair const &pp) const;

  predicate_set_t collect_inductive_preds_around_path(pcpair const& from_pc, shared_ptr<cg_edge_composition_t> const& pth) const override;

  virtual bool check_stability_for_added_edge(corr_graph_edge_ref const& cg_edge) const override;
  bool falsify_fcall_proof_query(expr_ref const& src, expr_ref const& dst, pcpair const& from_pc, counter_example_t& counter_example) const override;

  bool need_stability_at_pc(pcpair const &p) const override;

  virtual expr_ref edge_guard_get_expr(edge_guard_t const& eg, bool simplified) const override
  {
    // XXX HACK assumes edge_guard belongs to src tfg
    return eg.edge_guard_get_expr(this->get_src_tfg(), simplified);
  }

  bool dst_fcall_edge_already_updated(pc const& dst_pc) const
  {
    return set_belongs(m_dst_fcall_edges_already_updated_from_pcs, dst_pc);
  }
  void dst_fcall_edge_mark_updated(pc const& dst_pc)
  {
    m_dst_fcall_edges_already_updated_from_pcs.insert(dst_pc);
  }
  void update_corr_counter(shared_ptr<tfg_edge_composition_t> const &dst_ec, int num_src_to_pcs, int num_corr) const
  {
    m_num_src_to_pcs_dst_ec[dst_ec] = num_src_to_pcs;
    m_num_src_ecs_dst_ec[dst_ec] = num_corr;
  }
  void print_corr_counters_stat() const
  {
    for(auto const &n : m_num_src_to_pcs_dst_ec) {
      auto const &dst_ec = n.first;
      stats::get().add_counter(dst_ec->graph_edge_composition_get_from_pc().to_string()+"=>"+dst_ec->graph_edge_composition_get_to_pc().to_string()+"-src_to_pcs", n.second);
    }
    for(auto const &n : m_num_src_ecs_dst_ec) {
      auto const &dst_ec = n.first;
      stats::get().add_counter(dst_ec->graph_edge_composition_get_from_pc().to_string()+"=>"+dst_ec->graph_edge_composition_get_to_pc().to_string()+"-src_ecs", n.second);
    }
  }

// XXX: Verify this
  virtual predicate_set_t collect_assumes_around_edge(corr_graph_edge_ref const& te) const override;
  virtual predicate_set_t collect_assumes_for_edge(corr_graph_edge_ref const& te) const override;
  bv_rank_val_t calculate_rank_bveqclass_at_pc(pcpair const &) const;

  pair<expr_ref, expr_ref> compute_mem_stability_exprs(pcpair const& p) const;
  virtual bool graph_edge_is_well_formed(corr_graph_edge_ref const& e) const override;

  list<pair<graph_edge_composition_ref<pcpair,corr_graph_edge>, predicate_ref>> graph_apply_trans_funs(graph_edge_composition_ref<pcpair,corr_graph_edge> const &pth, predicate_ref const &pred, bool simplified = false) const override { NOT_REACHED(); }

  void populate_simplified_edgecond(corr_graph_edge_ref const& e) const override { /*NOP*/ };
  void populate_simplified_assumes(corr_graph_edge_ref const& e) const override { /*NOP*/ };
  void populate_simplified_to_state(corr_graph_edge_ref const& e) const override { /*NOP*/ };
  void populate_locs_potentially_modified_on_edge(corr_graph_edge_ref const &e) const override { /*NOP*/ }
  void generate_new_CE(pcpair const& cg_from_pc, shared_ptr<tfg_edge_composition_t> const &dst_ec) const;
  static corr_graph_edge_ref cg_edge_semantically_simplify_src_tfg_ec(corr_graph_edge const& cg_edge, tfg const& src_tfg);

  expr_ref cg_edge_get_src_cond(corr_graph_edge const &cg_edge) const;
  expr_ref cg_edge_get_dst_cond(corr_graph_edge const &cg_edge) const;

  state cg_edge_get_src_to_state(corr_graph_edge const &cg_edge) const;
  state cg_edge_get_dst_to_state(corr_graph_edge const &cg_edge) const;



protected:

  map<graph_loc_id_t,expr_ref> compute_simplified_loc_exprs_for_edge(corr_graph_edge_ref const& e, set<graph_loc_id_t> const& locids, bool force_update = false) const override { NOT_REACHED(); }
  map<string_ref,expr_ref> graph_get_vars_written_on_edge(corr_graph_edge_ref const& e) const override { NOT_REACHED(); }

private:

  predicate_set_t get_simplified_non_mem_assumes(corr_graph_edge_ref const& e) const override;
  predicate_set_t get_simplified_non_mem_assumes_helper(corr_graph_edge_ref const& e) const;

  void init_cg_elems()
  {
    shared_ptr<eqcheck>const& eq = m_eq;
    tfg const& src_tfg = eq->get_src_tfg();
    tfg const& dst_tfg = eq->get_dst_tfg();
    state const &src_start_state = src_tfg.get_start_state();
    state const &dst_start_state = dst_tfg.get_start_state();

    this->set_start_state(state::state_union(src_start_state, dst_start_state));
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
  }
  void init_cg_node(shared_ptr<corr_graph_node> n);

  bool expr_pred_def_is_likely_nearer(pcpair const &pp, expr_ref const &a, expr_ref const &b) const;

  void check_asserts_over_edge(corr_graph_edge_ref const &cg_edge) const;

  void select_llvmvars_designated_by_src_loc_id(graph_loc_id_t loc_id, set<expr_ref>& dst, set<expr_ref> const& src) const;
  static void select_non_llvmvars_from_src_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src);
  void select_llvmvars_live_at_pc_and_add_to_dst(set<expr_ref> &dst, set<expr_ref> const& src, pcpair const &p) const;
  void select_llvmvars_modified_on_incoming_edge_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const &p) const;
  void select_llvmvars_correlated_at_pred_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const& p) const;
  void select_llvmvars_having_sprel_mappings(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const& p) const;
  void select_llvmvars_appearing_in_pred_avail_exprs_at_pc(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const& p) const;
  static void select_llvmvars_appearing_in_expr(set<expr_ref>& dst, set<expr_ref> const& src, expr_ref const& e);
  bool expr_llvmvar_is_correlated_with_dst_loc_at_pc(expr_ref const& e, pcpair const& pp) const;
  bool is_live_llvmvar_at_pc(expr_ref const& e, pcpair const& pp) const;

  bool src_expr_contains_fcall_mem_on_incoming_edge(pcpair const &p, expr_ref const &e) const;
  bool src_expr_is_relatable_at_pc(pcpair const &p, expr_ref const &e) const;
  
  set<expr_ref> compute_bv_rank_dst_exprs(pcpair const& pp) const;
  eqclasses_ref compute_expr_eqclasses_at_pc(pcpair const& p) const override;
  expr_group_ref compute_mem_eqclass(pcpair const& p, set<expr_ref> const& src_interesting_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  pair<expr_ref, expr_ref> compute_mem_eq_exprs(pcpair const& p, set<expr_ref> const& src_interesting_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_bv_bool_eqclass(pcpair const& p, set<expr_ref> const& src_interesting_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_dst_ineq_eqclass(pcpair const& pp) const;
  expr_group_ref compute_ismemlabel_eqclass(pcpair const& pp, set<expr_ref,expr_ref_cmp_t> const& all_eqclass_exprs) const;
  vector<expr_group_ref> compute_ineq_eqclasses(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  void break_sse_reg_exprs(set<expr_ref> &dst_interesting_exprs) const;

protected:

  shared_ptr<eqcheck> m_eq;

private:

  graph_loc_id_t m_dst_start_loc_id;
  set<graph_loc_id_t> m_cg_locs_for_dst_rodata_symbol_masks_on_src_mem;

  unique_ptr<tfg_asm_t> m_dst_tfg;
  map<pcpair, tfg_suffixpath_t> m_src_suffixpath;
  set<pc> m_dst_fcall_edges_already_updated_from_pcs;
  mutable local_sprel_expr_guesses_t m_local_sprel_expr_assumes;

  //stats
  mutable map<shared_ptr<tfg_edge_composition_t>, int> m_num_src_to_pcs_dst_ec;
  mutable map<shared_ptr<tfg_edge_composition_t>, int> m_num_src_ecs_dst_ec;

  // cache
  mutable map<corr_graph_edge_ref,predicate_set_t> m_non_mem_assumes_cache;
};

shared_ptr<tfg_edge_composition_t> cg_edge_composition_to_src_edge_composite(corr_graph const &cg, shared_ptr<cg_edge_composition_t> const &ec, tfg const &src_tfg/*, state const &src_start_state*/);
  
shared_ptr<tfg_edge_composition_t> cg_edge_composition_to_dst_edge_composite(corr_graph const &cg, shared_ptr<cg_edge_composition_t> const &ec, tfg const &dst_tfg/*, state const &dst_start_state*/);


}
