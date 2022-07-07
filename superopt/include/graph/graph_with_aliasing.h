#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"
#include "support/dyn_debug.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/memlabel_map.h"
#include "expr/callee_rw_memlabels.h"

#include "graph/graph_with_simplified_assets.h"
#include "graph/lr_status.h"
#include "graph/lr_map.h"
#include "graph/alias_dfa.h"
#include "graph/sp_version_relations.h"
//#include "graph/available_exprs_alias_analysis_combo.h"
#include "graph/context_sensitive_ftmap_dfa_val.h"

namespace eqspace {

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class available_exprs_alias_analysis_combo_val_t;

using namespace std;
template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class populate_memlabel_map_visitor ;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_aliasing : public graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED>
{
public:
  graph_with_aliasing(string const &name, string const& fname, context* ctx) : graph_with_simplified_assets<T_PC,T_N,T_E,T_PRED>(name, fname, ctx)
  {
  }

  graph_with_aliasing(graph_with_aliasing const &other) : graph_with_simplified_assets<T_PC,T_N,T_E,T_PRED>(other),
                                                          //m_relevant_addr_refs(other.m_relevant_addr_refs),
                                                          m_lr_status_map(other.m_lr_status_map),
                                                          m_lr_status_for_sprel_locs_map(other.m_lr_status_for_sprel_locs_map)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  void graph_ssa_copy(graph_with_aliasing const& other)
  {
    graph_with_locs<T_PC, T_N, T_E, T_PRED>::graph_ssa_copy(other);
//    m_relevant_addr_refs = other.m_relevant_addr_refs;
  }

  graph_with_aliasing(istream& in, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx);
  virtual ~graph_with_aliasing() = default;

  //set<memlabel_ref> const &graph_get_relevant_addr_refs() const { return m_relevant_addr_refs; }
  //void set_relevant_addr_refs(set<memlabel_ref> const &relevant_addr_refs) { m_relevant_addr_refs = relevant_addr_refs; }

  //string to_string_with_linear_relations(map<T_PC, map<graph_loc_id_t, lr_status_ref>> const &linear_relations, bool details = true) const;

  dshared_ptr<graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const> graph_with_aliasing_shared_from_this() const
  {
    dshared_ptr<graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const> ret = dynamic_pointer_cast<graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const>(this->shared_from_this());
    ASSERT(ret);
    return ret;
  }


  void graph_populate_non_memslot_locs(map<graph_loc_id_t,graph_cp_location> const& old_locs);


  //virtual callee_summary_t get_callee_summary_for_nextpc(nextpc_id_t nextpc_num, size_t num_fargs) const = 0;


  //virtual void graph_init_memlabels_to_top(bool update_callee_memlabels) override
  //{
  //  if (update_callee_memlabels) {
  //    this->get_memlabel_map_ref().clear();
  //  } else {
  //    for (auto &mlvar : this->get_memlabel_map_ref()) {
  //      if (string_has_prefix(mlvar.first->get_str(), G_MEMLABEL_VAR_PREFIX)) {
  //        mlvar.second = mk_memlabel_ref(memlabel_t::memlabel_top());
  //      }
  //    }
  //  }
  //  graph_memlabel_map_t& memlabel_map = this->get_memlabel_map_ref();
  //  function<void (string const&, expr_ref)> f = [&memlabel_map](string const& key, expr_ref e)
  //  {
  //    expr_set_memlabels_to_top(e, memlabel_map);
  //  };
  //  this->graph_visit_exprs_const(f);
  //}

  bool populate_auxilliary_structures_dependent_on_locs(dshared_ptr<T_E const> const& e = dshared_ptr<T_E const>::dshared_nullptr());

  static void update_memlabel_map(graph_memlabel_map_t& memlabel_map, mlvarname_t const &mlvarname, memlabel_t const &ml);

//  void graph_populate_lr_status_at_cloned_pc(dshared_ptr<T_E const> const& cloned_edge, T_PC const& orig_pc/*, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map*/);

  //virtual void graph_locs_compute_and_set_memlabels(map<graph_loc_id_t, graph_cp_location> &locs_map)
  //{
  //  std::function<graph_loc_id_t (expr_ref const &e)> dummy_expr2locid_fn = [](expr_ref const &e)
  //  { return GRAPH_LOC_ID_INVALID; };

  //  for (auto &l : locs_map) {
  //    if (l.second.m_type != GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
  //      continue;
  //    }
  //    lr_status_ref loc_lr = this->compute_lr_status_for_expr(l.second.m_addr, map<expr_id_t, graph_loc_id_t>(), map<graph_loc_id_t, lr_status_ref>());
  //    l.second.m_memlabel = mk_memlabel_ref(loc_lr->to_memlabel(this->get_consts_struct(), this->get_symbol_map(), this->get_locals_map()));
  //  }
  //}

  void populate_locs_potentially_modified_on_edge(dshared_ptr<T_E const> const &e);
  
  //void
  //node_update_memlabel_map_for_mlvars(T_PC const& p, map<graph_loc_id_t, lr_status_ref> const& in, bool update_callee_memlabels, graph_memlabel_map_t& memlabel_map) const
  //{
  //  DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << endl);
  //  sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(p);
  //  std::function<void (T_PC const&, string const &, expr_ref)> f = [this,update_callee_memlabels,&memlabel_map,&in,&sprel_map_pair](T_PC const&, string const &key, expr_ref e)
  //  {
  //    if (e->is_const() || e->is_var()) { //common case
  //      return;
  //    }
  //    this->populate_memlabel_map_for_expr_and_loc_status(e, this->get_locs(), in, sprel_map_pair, this->graph_get_relevant_addr_refs(), this->get_argument_regs(), this->get_symbol_map(), this->get_locals_map(), update_callee_memlabels, memlabel_map/*, this->m_get_callee_summary_fn, this->m_expr2locid_fn*/);
  //  };

  //  this->graph_preds_visit_exprs({p}, f);
  //}

  set<graph_loc_id_t> graph_add_location_slots_using_state_mem_acc_map(graph_locs_map_t& new_locs/*map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, expr_ref>& locid2expr_map, map<expr_id_t, graph_loc_id_t>& exprid2locid_map*/, set<eqspace::state::mem_access> const &mem_acc, map<graph_loc_id_t, graph_cp_location> const &old_locs, sprel_map_pair_t const& sprel_map_pair, sp_version_relations_lattice_t const& spvr_lattice, graph_memlabel_map_t const& memlabel_map, map<graph_loc_id_t, lr_status_ref> const& lr_status_map, bool coarsen_ro_memlabels_to_rodata = false) const;

  void graph_update_callee_memlabels_in_expr(expr_ref const& e, graph_memlabel_map_t& memlabel_map, memlabel_t const& ml_readable, memlabel_t const& ml_writeable) const;

  set<graph_loc_id_t> add_new_locs_based_on_edge(dshared_ptr<T_E const> const& e, graph_locs_map_t& new_locs, sprel_map_pair_t const& sprel_map_pair, sp_version_relations_lattice_t const& spvr_lattice, graph_memlabel_map_t const& memlabel_map, map<graph_loc_id_t, lr_status_ref> const& lr_status_map, bool coarsen_ro_memlabels_to_rodata, map<graph_loc_id_t,graph_cp_location> const& old_locs) const;
  void update_memlabels_for_memslot_locs(call_context_ref const& cc, graph_locs_map_t& locs, map<graph_loc_id_t, lr_status_ref> const& lr_status_map/*, avail_exprs_t const& avail_exprs, graph_memlabel_map_t const& memlabel_map*/, bool coarsen_ro_memlabels_to_rodata = false) const;

  void
  edge_update_memlabel_map_for_mlvars(call_context_ref const& cc, dshared_ptr<T_E const> const& e, map<graph_loc_id_t, lr_status_ref> const& in, bool update_callee_memlabels, bool coarsen_ro_memlabels_to_rodata/*, callee_rw_memlabels_t::get_callee_rw_memlabels_fn_t get_callee_rw_memlabels*/, graph_memlabel_map_t& memlabel_map, graph_locs_map_t const& locs, sprel_map_pair_t const& sprel_map_pair, sp_version_relations_lattice_t const& spvr_lattice) const;


  void populate_gen_and_kill_sets_for_edge(dshared_ptr<T_E const> const &e, graph_locs_map_t const& locs, graph_memlabel_map_t const& memlabel_map, sprel_map_pair_t const& sprel_map_pair, map<graph_loc_id_t, expr_ref> &gen_set, set<graph_loc_id_t> &killed_locs) const;


  //void split_memory_in_graph_initialize(bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>())
  //{
  //  autostop_timer func_timer(__func__);

  //  DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": before set_memlabels_to_top_or_constant_values, TFG:\n" << this->graph_to_string(/*true*/) << endl);

  //  this->graph_init_memlabels_to_top(update_callee_memlabels);
  //  this->graph_init_sprels();

  //  DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": after set_memlabels_to_top_or_constant_values, TFG:\n" << this->graph_to_string(/*true*/) << endl);

  //  this->init_graph_locs(this->get_symbol_map(), this->get_locals_map(), update_callee_memlabels, llvm_locs);

  //  //m_alias_analysis.reset(update_callee_memlabels); //, this->get_memlabel_map_ref()
  //}

  //void split_memory_in_graph(bool update_callee_memlabels);

  //set<T_PC> get_from_pcs_for_edges(set<shared_ptr<T_E const>> const &edges)
  //{
  //  set<T_PC> ret;
  //  for (auto e : edges) {
  //    ret.insert(e->get_from_pc());
  //  }
  //  return ret;
  //}

  //void
  //split_memory_in_graph_and_propagate_sprels_until_fixpoint(callee_rw_memlabels_t::get_callee_rw_memlabels_fn_t get_callee_rw_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = {});

  //virtual void graph_populate_relevant_addr_refs();


  void set_locs_lr_status_and_avail_exprs_using_combo_dfa_vals(context_sensitive_ftmap_dfa_val_t<available_exprs_alias_analysis_combo_val_t<T_PC,T_N,T_E,T_PRED>> const& vals);
  void clear_locs_for_call_context(call_context_ref const& cc)
  {
    this->clear_loc_structures();
    this->m_lr_status_for_sprel_locs_map.clear();
    
    map<call_context_ref, map<graph_loc_id_t, lr_status_ref>> new_lr_status_map;
    /*for(auto const& m: m_lr_status_map) */{
      map<call_context_ref, map<graph_loc_id_t, lr_status_ref>> new_cc_lr_status_map = m_lr_status_map/*m.second*/;
      if(new_cc_lr_status_map.count(cc))
        new_cc_lr_status_map.erase(cc);
      /*if(new_cc_lr_status_map.size()) */{
        //new_lr_status_map.insert(make_pair(m.first, new_cc_lr_status_map));
        new_lr_status_map = new_cc_lr_status_map;
      }
    }
    m_lr_status_map = new_lr_status_map;
  }

  set<memlabel_ref> graph_get_non_arg_local_memlabels() const;

  virtual bool graph_expr_is_ssa_var(expr_ref const& e) const override;
  virtual bool is_llvm_graph() const { return false; }
  virtual set<expr_ref> get_parent_sp_versions_of(expr_ref const& e) const { return {}; }
  virtual expr_ref graph_get_upper_bound_spv_for_ml(memlabel_t const& ml) const { return nullptr; }
  virtual set<expr_ref> graph_get_all_sp_versions() const { return {}; }
  virtual bool graph_expr_refers_to_alloca_addr(expr_ref const& e) const { return false; }

protected:

  reachable_memlabels_map_t compute_reachable_memlabels_map(map<graph_loc_id_t, lr_status_ref> const& in) const;
  memlabel_t get_reachable_memlabels(map<graph_loc_id_t, lr_status_ref> const& in, memlabel_t const &ml) const;
  map<graph_loc_id_t,expr_ref> compute_simplified_loc_exprs_for_edge(dshared_ptr<T_E const> const& e, graph_locs_map_t const& locs, graph_memlabel_map_t const& memlabel_map, sprel_map_pair_t const& sprel_map_pair) const;

  map<graph_loc_id_t,expr_ref> compute_locs_potentially_written_on_edge(dshared_ptr<T_E const> const& e, graph_locs_map_t const& locs, graph_memlabel_map_t const& memlabel_map, sprel_map_pair_t const& sprel_map_pair) const;

  map<graph_loc_id_t, lr_status_ref> compute_new_lr_status_on_locs(call_context_ref const& cc, dshared_ptr<T_E const> const &e, map<graph_loc_id_t, lr_status_ref> const &in, graph_locs_map_t const& locs_map, graph_memlabel_map_t const& memlabel_map, sprel_map_pair_t const& sprel_map_pair) const;
  map<string, expr_ref> graph_get_mem_simplified_at_pc_for_locs(state const& to_state, graph_memlabel_map_t const& memlabel_map, sprel_map_pair_t const& sprel_map_pair) const;

  expr_ref graph_get_value_simplified_using_mem_simplified_at_pc_for_locs(dshared_ptr<T_E const> const& e, state const& to_state, graph_loc_id_t l, map<string, expr_ref> const& mem_simplified, graph_locs_map_t const& locs_map, graph_memlabel_map_t const& memlabel_map, sprel_map_pair_t const& sprel_map_pair) const;
  lr_status_ref compute_lr_status_for_expr(call_context_ref const& cc, expr_ref const& e, map<expr_id_t, graph_loc_id_t> const &exprid2locid_map, map<graph_loc_id_t, lr_status_ref> const &prev_lr) const;

  set<graph_loc_id_t> get_start_relevant_locids_for_pointsto_analysis(bool function_is_program_entry) const;
  virtual callee_rw_memlabels_t graph_get_callee_rw_memlabels(expr_ref const& nextpc, vector<memlabel_t> const& farg_memlabels, reachable_memlabels_map_t const& reachable_mls) const = 0;
  expr_ref expr_convert_memmask_to_select_for_symbols_and_locals(expr_ref const& e) const;
  virtual void graph_to_stream(ostream& ss, string const& prefix="") const override;
  lr_status_ref get_lr_status_for_loc_at_call_context(graph_loc_id_t loc_id/*, T_PC const& p*/, call_context_ref const& call_context) const;
  lr_status_ref get_lr_status_for_loc/*_at_pc*/(graph_loc_id_t loc_id/*, T_PC const& p*/) const;
  //bool has_lr_status_map_at_pc(T_PC const& p) const
  //{
  //  return m_lr_status_map.count(p);
  //}
  map<call_context_ref, map<graph_loc_id_t, lr_status_ref>> const& get_lr_status_map/*_at_pc*/(/*T_PC const& p*/) const;
  void populate_lr_status_for_sprel_locs_map();
  void set_lr_status_map/*_at_pc*/(/*T_PC const& p, */map<call_context_ref, map<graph_loc_id_t, lr_status_ref>> const& lr_status_map)
  {
    //ASSERT(!this->has_lr_status_map_at_pc(p));
    //m_lr_status_map.insert(make_pair(p, lr_status_map));
    m_lr_status_map = lr_status_map;
  }
  //void populate_lr_status_for_sprel_locs_map/*_at_pc*/(/*T_PC const& p*/);

private:

  void check(map<graph_loc_id_t, graph_cp_location> const& locs) const;

  static map<graph_loc_id_t, lr_status_ref> get_intersected_lr_status(map<call_context_ref, map<graph_loc_id_t, lr_status_ref>> const& lr_status_map);

  void read_alias_analysis(istream& in);

  void populate_memlabel_map_for_expr_and_loc_status(call_context_ref const& cc, expr_ref const &e, graph_locs_map_t const &locs, map<graph_loc_id_t, lr_status_ref> const &in, sprel_map_pair_t const &sprel_map_pair, sp_version_relations_lattice_t const& spvr_lattice/*, set<memlabel_ref> const &relevant_addr_refs*/, graph_arg_regs_t const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, bool coarsen_ro_memlabels_to_rodata/*, callee_rw_memlabels_t::get_callee_rw_memlabels_fn_t get_callee_rw_memlabels*/, graph_memlabel_map_t &memlabel_map/*, std::function<callee_summary_t (nextpc_id_t, int)> get_callee_summary_fn, std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn*/) const;

private:

  //set<memlabel_ref> m_relevant_addr_refs;
  //alias_analysis<T_PC, T_N, T_E, T_PRED> m_alias_analysis;
  map<call_context_ref, map<graph_loc_id_t, lr_status_ref>> m_lr_status_map;
  map<graph_loc_id_t, lr_status_ref> m_lr_status_for_sprel_locs_map;
  friend class ftmap_t;
  friend class ftmap_pointsto_analysis_combo_dfa_t ;
  friend class populate_memlabel_map_visitor<T_PC, T_N, T_E, T_PRED>;
};

}
