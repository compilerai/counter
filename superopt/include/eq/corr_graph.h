#pragma once

#include <list>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include <chrono>

#include "support/log.h"
#include "support/mymemory.h"
#include "support/bv_rank_val.h"

#include "expr/counter_example.h"

#include "gsupport/corr_graph_edge.h"
#include "gsupport/pc_local_sprel_expr_guesses.h"

#include "graph/expr_group.h"
#include "graph/graph_with_correctness_covers.h"

#include "tfg/tfg.h"
#include "tfg/tfg_asm.h"
#include "tfg/tfg_llvm.h"
#include "tfg/tfg_ssa.h"

#include "eq/parse_input_eq_file.h"
#include "eq/eqcheck.h"

namespace eqspace {

class suffixpath;
class eqcheck;

using namespace std;

class cg_stats_t
{
  map<shared_ptr<tfg_edge_composition_t>, int> m_num_src_to_pcs_dst_ec;
  map<shared_ptr<tfg_edge_composition_t>, int> m_num_src_ecs_dst_ec;
  unsigned m_cg_prove_queries{0};
  chrono::duration<double> m_cg_timer{0};

public:
  void set_src_to_pcs_for_dst_ec(shared_ptr<tfg_edge_composition_t> const& dst_ec, int num_src_to_pcs) { m_num_src_to_pcs_dst_ec[dst_ec] = num_src_to_pcs; }
  void set_src_ecs_for_dst_ec(shared_ptr<tfg_edge_composition_t> const& dst_ec, int num_src_ecs)       { m_num_src_ecs_dst_ec[dst_ec] = num_src_ecs; }

  void cg_prove_queries_add(unsigned d)
  { m_cg_prove_queries += d; }

  void cg_timer_add(chrono::duration<double> const& d)
  { m_cg_timer += d; }

  void post_to_global_stats() const;
};

class corr_graph : public graph_with_correctness_covers<pcpair, corr_graph_node, corr_graph_edge, predicate>
{
public:
  corr_graph(string_ref const &name, string_ref const& fname, dshared_ptr<eqcheck> const& eq) :
    graph_with_correctness_covers<pcpair, corr_graph_node, corr_graph_edge, predicate>(name->get_str(), fname->get_str(), eq->get_context()/*, eq->eqcheck_get_memlabel_assertions()*/),
    m_eq(eq), m_src_tfg(tfg_ssa_t::tfg_ssa_construct_from_non_ssa_tfg(eq->get_src_tfg_ptr()->get_ssa_tfg(), dshared_ptr<tfg const>::dshared_nullptr())),
    m_dst_tfg(tfg_ssa_t::tfg_ssa_construct_from_non_ssa_tfg(eq->get_dst_tfg_ptr()->get_ssa_tfg(), eq->get_src_tfg_ptr()->get_ssa_tfg()))
  {
    tfg const& src_tfg = *(m_src_tfg->get_ssa_tfg());
    tfg const& dst_tfg = *(m_dst_tfg->get_ssa_tfg());
    //init_cg_elems();
//    set<memlabel_ref> const &src_relevant_addr_refs = src_tfg.graph_get_relevant_addr_refs();
//    set<memlabel_ref> const &dst_relevant_addr_refs = dst_tfg.graph_get_relevant_addr_refs();
//    set<memlabel_ref> relevant_addr_refs;
//    set_union(src_relevant_addr_refs, dst_relevant_addr_refs, relevant_addr_refs);
//    this->set_relevant_addr_refs(relevant_addr_refs);
    this->graph_set_relevant_memlabels(dst_tfg.graph_get_relevant_memlabels()); //use dst tfg's memlabels because they will include src tfg's memlabels
    this->set_symbol_map(eq->get_symbol_map());
    this->set_locals_map(src_tfg.get_locals_map());
    this->set_argument_regs(src_tfg.get_argument_regs());
    predicate_set_t global_assumes = src_tfg.get_global_assumes();
    unordered_set_union(global_assumes, dst_tfg.get_global_assumes());
    this->set_global_assumes(global_assumes);
    this->populate_graph_bvconsts();
    //this->populate_memlabel_assertions();
    stats_num_cg_constructions++;
  }

  corr_graph(corr_graph const &other) : graph_with_correctness_covers<pcpair, corr_graph_node, corr_graph_edge, predicate>(other),
                                        m_eq(other.m_eq),
                                        m_ranking_exprs(other.m_ranking_exprs),
                                        m_src_tfg(other.m_src_tfg),
                                        m_dst_tfg(other.m_dst_tfg),
                                        // Since the dst_tfg in the CG is const now in SSA framework, so no need to create new copy now
                                        // m_dst_tfg(corr_graph::dst_tfg_copy(*other.m_dst_tfg)),
                                        //m_src_suffixpath(other.m_src_suffixpath),
                                        m_dst_fcall_edges_already_updated_from_pcs(other.m_dst_fcall_edges_already_updated_from_pcs),
                                        m_internal_pcs(other.m_internal_pcs),
                                        m_super_edges(other.m_super_edges),
                                        m_pc_local_sprel_expr_assumes_for_allocation(other.m_pc_local_sprel_expr_assumes_for_allocation),
                                        m_pc_local_sprel_expr_assumes_for_deallocation(other.m_pc_local_sprel_expr_assumes_for_deallocation),
                                        m_src_pcs_reaching_cg_pcpair(other.m_src_pcs_reaching_cg_pcpair),
                                        //m_preallocated_locals(other.m_preallocated_locals),
                                        //m_start_pc_preconditions(other.m_start_pc_preconditions),
                                        m_exit_pc_asserts(other.m_exit_pc_asserts),
                                        m_edge_well_formedness_conditions(other.m_edge_well_formedness_conditions),
                                        m_cg_edge_contains_repeated_src_tfg_edge(other.m_cg_edge_contains_repeated_src_tfg_edge),
                                        //stats
                                        m_cg_stats(other.m_cg_stats),
                                        //caches
                                        m_per_edge_aliasing_constrainsts_map(other.m_per_edge_aliasing_constrainsts_map),
                                        m_non_mem_assumes_cache(other.m_non_mem_assumes_cache)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    stats_num_cg_constructions++;
  }

  corr_graph(istream& in, context* ctx, string const& name, dshared_ptr<tfg_ssa_t> const& src_tfg, dshared_ptr<tfg_ssa_t> const& dst_tfg, dshared_ptr<eqcheck> const& eq, std::function<dshared_ptr<corr_graph_edge const> (istream&, string const&, string const&, context*)> read_edge_fn) : graph_with_correctness_covers<pcpair, corr_graph_node, corr_graph_edge, predicate>(in, name, read_edge_fn, ctx), m_eq(eq), m_src_tfg(src_tfg), m_dst_tfg(dst_tfg)
  {
    string line;
    bool end;

    // XXX: read the ssa tfgs
    // init_cg_elems();
    this->graph_set_relevant_memlabels(m_dst_tfg->get_ssa_tfg()->graph_get_relevant_memlabels()); //use dst tfg's memlabels because they will include src tfg's memlabels
    this->populate_assumes_around_edge(dshared_ptr<corr_graph_edge const>::dshared_nullptr());
    do {
      end = !getline(in, line);
      ASSERT(!end);
    } while (line == "");

    ASSERTCHECK((line == "=m_dst_fcall_edges_already_updated_from_pcs:"), cout << "line = " << line << endl);
    end = !getline(in, line);
    ASSERT(!end);
    while (line != "=m_internal_pcs:") {
      this->m_dst_fcall_edges_already_updated_from_pcs.insert(pc::create_from_string(line));
      end = !getline(in, line);
      ASSERT(!end);
    }
    ASSERTCHECK((line == "=m_internal_pcs:"), cout << "line = " << line << endl);
    end = !getline(in, line);
    ASSERT(!end);
    while (line != "=m_super_edges:") {
      this->m_internal_pcs.insert(pcpair::create_from_string(line));
      end = !getline(in, line);
      ASSERT(!end);
    }
    ASSERTCHECK((line == "=m_super_edges:"), cout << "line = " << line << endl);
    end = !getline(in, line);
    ASSERT(!end);
    while (line != "=AllocaPCLocalSprelAssumptionsBegin:") {
      istringstream es(line);
      edge_id_t<pcpair> eid(es);
      this->m_super_edges.push_back(eid);
      end = !getline(in, line);
      ASSERT(!end);
    }

    ASSERTCHECK((line == "=AllocaPCLocalSprelAssumptionsBegin:"), cout << "line = " << line << endl);
    line = this->m_pc_local_sprel_expr_assumes_for_allocation.pc_local_sprel_expr_guesses_from_stream(in, ctx);
    ASSERTCHECK((line == "=AllocaPCLocalSprelAssumptionsEnd"), cout << "line = " << line << endl);
    end = !getline(in, line);
    ASSERT(!end);
    ASSERTCHECK((line == "=DeallocaPCLocalSprelAssumptionsBegin:"), cout << "line = " << line << endl);
    line = this->m_pc_local_sprel_expr_assumes_for_deallocation.pc_local_sprel_expr_guesses_from_stream(in, ctx);
    ASSERTCHECK((line == "=DeallocaPCLocalSprelAssumptionsEnd"), cout << "line = " << line << endl);

    end = !getline(in, line);
    ASSERT(!end);
    string prefix = "=src_tfg pcs reaching pcpair ";
    while (string_has_prefix(line, prefix)) {
      pcpair pp = pcpair::create_from_string(line.substr(prefix.size()));
      end = !getline(in, line);
      ASSERT(!end);
      set<pc> src_tfg_pcs;
      string pc_prefix = "=src_tfg pc: ";
      while (string_has_prefix(line, pc_prefix)) {
        src_tfg_pcs.insert(pc::create_from_string(line.substr(pc_prefix.size())));
        end = !getline(in, line);
        ASSERT(!end);
      }
      m_src_pcs_reaching_cg_pcpair.insert(make_pair(pp, src_tfg_pcs));
    }
    //string const& preallocated_locals_prefix = "=PreallocatedLocals:";
    //ASSERTCHECK(string_has_prefix(line, preallocated_locals_prefix), cout << "line = " << line << endl);
    //istringstream iss(line.substr(preallocated_locals_prefix.length()));
    //allocsite_t lid;
    //while (allocsite_t::allocsite_from_stream(iss, lid)) {
    //  cg->m_preallocated_locals.insert(lid);
    //}

    //end = !getline(in, line);
    //ASSERT(!end);

    prefix = "=exit_pc_asserts at ";
    while (string_has_prefix(line, prefix)) {
      pcpair exit_pc = pcpair::create_from_string(line.substr(prefix.size()));
      end = !getline(in, line);
      ASSERT(!end);
      while (string_has_prefix(line, "=exit_pc_assert.")) {
        predicate_ref epred = predicate::mk_predicate_ref(in, ctx);
        this->m_exit_pc_asserts[exit_pc].insert(epred);
        end = !getline(in, line);
        ASSERT(!end);
      }
    }

    prefix = "=well-formedness-conditions for ";
    while (string_has_prefix(line, prefix)) {
      string edge_id_str = line.substr(prefix.size());
      istringstream es(edge_id_str);
      edge_id_t<pcpair> eid(es);
      edge_wf_cond_t wf_cond = edge_wf_cond_from_stream(in, line, *this, this->get_src_tfg(), this->get_dst_tfg());
      this->m_edge_well_formedness_conditions.insert(make_pair(eid, wf_cond));
      end = !getline(in, line);
      ASSERT(!end);
    }

    prefix = "=Ranking exprs at node ";
    while (string_has_prefix(line, prefix)) {
      pcpair pp = pcpair::create_from_string(line.substr(prefix.size()));
      bv_rank_exprs_t ranking_exprs = bv_rank_exprs_t::bv_rank_exprs_from_stream(in/*, line*/, ctx);
      this->m_ranking_exprs.insert(make_pair(pp, ranking_exprs));

      end = !getline(in, line);
      ASSERT(!end);
    }
    ASSERT(line == "=Ranking exprs done");

    this->set_edge_to_assumes_around_edge_map(this->get_src_tfg().tfg_read_assumes_around_edge<pcpair>(in, "cg"));

    end = !getline(in, line);
    ASSERT(!end);

    if (line != "=corr_graph_done") {
      cout << "line = '" << line << "'\n";
    }
    ASSERT(line == "=corr_graph_done");

    stats_num_cg_constructions++;
  }

  ~corr_graph()
  {
    stats_num_cg_destructions++;
  }

  shared_ptr<tfg_full_pathset_t const> cg_create_src_tfg_ssa_for_new_ec(shared_ptr<tfg_full_pathset_t const> const& new_ec, pcpair const& to_pcpair);
  void populate_graph_bvconsts()
  {
    set<expr_ref> tfg_consts = this->get_src_tfg().compute_graph_bvconsts();
    set<graph_edge_composition_ref<pc, tfg_edge>> dst_corr_paths = this->get_dst_tfg().dst_graph_get_correlation_paths();
    ASSERT(dst_corr_paths.size());
    set<expr_ref> dst_tfg_consts = this->get_dst_tfg().compute_graph_bvconsts(dst_corr_paths);
    set_union(tfg_consts, dst_tfg_consts);
    this->set_graph_bvconsts(tfg_consts);
  }

  static dshared_ptr<tfg> cg_src_dst_tfg_copy(tfg const& t);

//  static dshared_ptr<tfg> cg_src_dst_tfg_unroll_till_no_repeated_pcs(tfg const& t, shared_ptr<tfg_full_pathset_t const> const& src_ec_to_be_added, map<pc, clone_count_t> const& existing_pcs, tfg_edge_composition_ref& new_ec);

  void cg_add_node(dshared_ptr<corr_graph_node> n);
  void cg_remove_node(dshared_ptr<corr_graph_node> n);

  string cg_get_function_name_from_name() const;

  tfg_llvm_t const& get_src_tfg() const override { return *(dynamic_pointer_cast<tfg_llvm_t const>(m_src_tfg->get_ssa_tfg())); }
  bool CE_copy_required_for_input_edge(edge_id_t<pcpair> const& e) const override
  {
    ASSERT(m_cg_edge_contains_repeated_src_tfg_edge.count(e));
    return m_cg_edge_contains_repeated_src_tfg_edge.at(e);
  }

  tfg const& get_dst_tfg() const { return *(m_dst_tfg->get_ssa_tfg()); }
  //tfg &get_dst_tfg()             { return *m_dst_tfg; }

  //void cg_xml_print(ostream& os) const;
  virtual aliasing_constraints_t get_aliasing_constraints_for_edge(dshared_ptr<corr_graph_edge const> const& e) const override;
  virtual aliasing_constraints_t graph_generate_aliasing_constraints_at_path_to_pc(shared_ptr<cg_edge_composition_t> const& pth) const override;
  aliasing_constraints_t get_aliasing_constraints_for_edge_helper(dshared_ptr<corr_graph_edge const> const& e) const;
  virtual aliasing_constraints_t graph_apply_trans_funs_on_aliasing_constraints(dshared_ptr<corr_graph_edge const> const& e, aliasing_constraints_t const& als) const override;
  callee_rw_memlabels_t graph_get_callee_rw_memlabels(expr_ref const& nextpc, vector<memlabel_t> const& farg_memlabels, reachable_memlabels_map_t const& reachable_mls) const override { NOT_REACHED(); }

  //virtual callee_summary_t get_callee_summary_for_nextpc(nextpc_id_t nextpc_num, size_t num_fargs) const override;

//  tfg_llvm_t const &get_src_tfg() const override { return m_eq->get_src_tfg(); }
  avail_exprs_t const &get_src_avail_exprs/*_at_pc*/(/*pcpair const &pp*/) const override
  {
      pcpair pp = pcpair::pcpair_notimplemented();
      return this->get_src_tfg().get_avail_exprs_for_path_id(get_path_id_from_pcpair(true, pp));
  }

  void compute_reaching_src_tfg_pcs_at_pcpair(pcpair const& from_pp, pcpair const& to_pp, shared_ptr<tfg_full_pathset_t const> const& src_edge);
  void compute_renamed_locs_at_pcpair(pcpair const& p);

  virtual set<string_ref> get_var_definedness_at_pc(pcpair const& pp) const override
  {
    tfg_path_id_t src_path_id = get_path_id_from_pcpair(true, pp);
    tfg_path_id_t dst_path_id = get_path_id_from_pcpair(false, pp);
    set<string_ref> defined_vars = this->get_src_tfg().get_var_definedness_for_path_id(src_path_id);
    set_union(defined_vars, this->get_dst_tfg().get_var_definedness_for_path_id(dst_path_id));
    return defined_vars;

  }
  virtual map<graph_loc_id_t, graph_cp_location> get_locs() const override
  {
    auto all_locs = this->get_src_tfg().get_locs();
    for(auto const& m : this->get_dst_tfg().get_locs()) {
      bool inserted = all_locs.insert(m).second;
      ASSERT(inserted);
    }
    return all_locs;
  }

  virtual map<graph_loc_id_t, graph_cp_location> get_locs_at_pc(pcpair const& pp) const override
  {
    tfg_path_id_t src_path_id = get_path_id_from_pcpair(true, pp);
    tfg_path_id_t dst_path_id = get_path_id_from_pcpair(false, pp);
    auto all_locs = this->get_src_tfg().get_locs_for_path_id(src_path_id);
    for(auto const& m : this->get_dst_tfg().get_locs_for_path_id(dst_path_id)) {
      bool inserted = all_locs.insert(m).second;
      ASSERT(inserted);
    }
    return all_locs;
  }

  virtual bool is_memvar_or_memalloc_var(expr_ref const& e) const override
  {
    ASSERT(e->is_var());
    string name = e->get_name()->get_str();
    if(e->is_array_sort() && (string_has_prefix(name, this->get_src_tfg().get_memvar_version_at_pc(pc::start())->get_name()->get_str()) || string_has_prefix(name, this->get_dst_tfg().get_memvar_version_at_pc(pc::start())->get_name()->get_str())))
      return true;
    if(e->is_memalloc_sort() && (string_has_prefix(name, this->get_src_tfg().get_memallocvar_version_at_pc(pc::start())->get_name()->get_str()) || string_has_prefix(name, this->get_dst_tfg().get_memallocvar_version_at_pc(pc::start())->get_name()->get_str())))
      return true;
    return false;

  }
  virtual list<shared_ptr<predicate const>> get_sp_version_relations_preds_at_pc(pcpair const &pp) const override
  {
    auto ret = this->get_src_tfg().get_sp_version_relations_preds_at_pc(pp.get_first());
    ret.splice(ret.begin(), this->get_dst_tfg().get_sp_version_relations_preds_at_pc(pp.get_second()));
    return ret;
  }

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override;

  //edge_guard_t
  //graph_get_edge_guard_from_edge_composition(graph_edge_composition_ref<pcpair,corr_graph_edge> const &ec) const override
  //{
  //  NOT_IMPLEMENTED();
  //}

  //template<typename T_VAL, xfer_dir_t T_DIR>
  //T_VAL xfer_over_edge(shared_ptr<corr_graph_edge const> e, T_VAL const& in, function<T_VAL (T_VAL const&, expr_ref const&, state const&, unordered_set<expr_ref> const&)> atom_f, function<T_VAL (T_VAL const&, T_VAL const&)> meet_f, bool simplified) const
  //{
  //  shared_ptr<eqcheck>const& eq = m_eq;
  //  tfg const& src_tfg = eq->get_src_tfg();
  //  tfg const& dst_tfg = eq->get_dst_tfg();

  //  shared_ptr<tfg_edge_composition_t> src_ec = e->get_src_edge();
  //  shared_ptr<tfg_edge_composition_t> dst_ec = e->get_dst_edge();

  //  ASSERT(src_ec);
  //  ASSERT(dst_ec);

  //  T_VAL const& src_val = src_tfg.xfer_over_graph_edge_composition<T_VAL,T_DIR>(src_ec, in, atom_f, meet_f, simplified);
  //  T_VAL const& ret_val = dst_tfg.xfer_over_graph_edge_composition<T_VAL,T_DIR>(dst_ec, src_val, atom_f, meet_f, simplified);

  //  return ret_val;
  //}


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
    return this->get_dst_tfg().loc_has_sprel_mapping_for_path_id(make_pair(p.get_second(),p.get_first()), loc_id);
  }

  //virtual void init_graph_locs(graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>()) override;
  void cg_add_locs_for_dst_rodata_symbols(map<graph_loc_id_t, graph_cp_location>& locs);
  virtual set<expr_ref> get_live_vars(pcpair const& pp) const override
  {
    set<expr_ref> ret = this->get_src_tfg().get_live_vars(pp.get_first());
    set_union(ret, this->get_dst_tfg().get_live_vars(pp.get_second()));
    return ret;
  }

  void populate_branch_affecting_locs() override { /*NOP*/ }
  //void populate_loc_liveness() override { NOT_REACHED(); }
  void populate_loc_and_var_definedness(/*dshared_ptr<corr_graph_edge const> const& e*/) override { /*NOP*/ }

  dshared_ptr<eqcheck> const& get_eq() const;

  list<dshared_ptr<corr_graph_node>> find_cg_nodes_with_dst_node(pc dst) const;

  bool is_edge_conditions_and_fcalls_equal();
  bool get_preds_after_guess_and_check(const pcpair& pp, expr_ref& pred);
  bool is_exit_constraint_provable();
  virtual expr_ref get_incident_fcall_nextpc(pcpair const &p) const override;

  void cg_add_precondition();
  void cg_add_exit_asserts_at_pc(pcpair const &pp);
  void cg_add_asserts_at_pc(pcpair const &pp);

  bool cg_check_new_cg_edge(dshared_ptr<corr_graph_edge const> cg_new_edge, string const& correl_entry_name);
  //pair<bool,bool> corr_graph_propagate_CEs_on_edge(corr_graph_edge_ref const& cg_new_edge, bool propagate_initial_CEs = true);

  start_state_t const &get_src_start_state() const;
  start_state_t const &get_dst_start_state() const;

  bool cg_covers_dst_edge_at_pcpair(pcpair const &pp, dshared_ptr<tfg_edge const> const &dst_e, tfg const &dst_tfg) const;
  bool cg_covers_all_dst_tfg_paths(tfg const &dst_tfg) const;

  friend ostream& operator<<(ostream& os, const corr_graph&);

  static void write_failure_to_proof_file(string const &filename, string const &function_name);
  void write_provable_cg_to_proof_file(string const &filename, string const &function_name) const;
  void to_dot(string filename, bool embed_info) const;
//  void cg_import_locs(tfg const &tfg, graph_loc_id_t start_loc_id);

  shared_ptr<corr_graph> cg_copy_corr_graph() const;

  bool cg_src_dst_exprs_are_relatable_at_pc(pcpair const &pp, expr_ref const &src_expr, expr_ref const &dst_expr) const;

  pair<pc,expr_ref> corr_graph_get_lsprel_mapping_for_local_allocation(allocsite_t const& lid) const
  {
    auto ret = m_pc_local_sprel_expr_assumes_for_allocation.get_mappings_for_local(lid);
    ASSERT(ret.size() <= 1); // at most one mapping for alloca
    return ret.size() == 0 ? make_pair(pc(), nullptr) : ret.front();
  }

  bool corr_graph_have_correlated_allocation(allocsite_t const& lid) const   { return m_pc_local_sprel_expr_assumes_for_allocation.have_mapping_for_local(lid); }
  //bool corr_graph_have_correlated_deallocation(allocsite_t const& lid) const { return m_pc_local_sprel_expr_assumes_for_deallocation.have_mapping_for_local(lid); }

  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> const& corr_graph_get_pc_local_sprel_expr_assumes_for_allocation() const { return this->m_pc_local_sprel_expr_assumes_for_allocation; }
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> const& corr_graph_get_pc_local_sprel_expr_assumes_for_deallocation() const { return this->m_pc_local_sprel_expr_assumes_for_deallocation; }
  size_t corr_graph_get_num_of_alloca_correlations() const { return this->m_pc_local_sprel_expr_assumes_for_allocation.size(); }
  size_t corr_graph_get_num_of_dealloc_correlations() const { return this->m_pc_local_sprel_expr_assumes_for_deallocation.size(); }

  bool corr_graph_add_pc_local_sprel_expr_assumes_for_allocation(pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> const& pc_lsprels)   { return this->m_pc_local_sprel_expr_assumes_for_allocation.pc_local_sprel_expr_guesses_union(pc_lsprels); }
  bool corr_graph_add_pc_local_sprel_expr_assumes_for_deallocation(pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> const& pc_lsprels) { return this->m_pc_local_sprel_expr_assumes_for_deallocation.pc_local_sprel_expr_guesses_union(pc_lsprels); }

  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> corr_graph_get_pc_lsprels_for_local_allocations(set<allocsite_t> const& lids) const { return m_pc_local_sprel_expr_assumes_for_allocation.get_pc_lsprels_for_locals(lids); }
  list<pair<pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc>,pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc>>> corr_graph_generate_local_sprel_expr_guesses_for_locals(shared_ptr<tfg_full_pathset_t const> const& src_path, shared_ptr<tfg_full_pathset_t const> const& dst_ec) const;

  shared_ptr<tfg_full_pathset_t const> update_dst_edge_for_local_allocations_and_deallocations(shared_ptr<tfg_full_pathset_t const> const& src_fp, shared_ptr<tfg_full_pathset_t const> const& dst_fp, pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> const& pc_lsprels_allocs, pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> const& pc_lsprels_deallocs);

  bool cg_different_correlation_exists_for_src_to_pc(shared_ptr<tfg_full_pathset_t const> const& src_path, shared_ptr<tfg_full_pathset_t const> const& dst_ec) const;
  bool cg_correlation_exists(shared_ptr<tfg_full_pathset_t const> const& src_path, shared_ptr<tfg_full_pathset_t const> const& dst_ec) const;
  bool cg_correlation_exists_to_another_src_pc(shared_ptr<tfg_full_pathset_t const> const& src_path, shared_ptr<tfg_full_pathset_t const> const& dst_ec) const;
  bool src_path_contains_already_correlated_node_as_intermediate_node(shared_ptr<tfg_edge_composition_t> const &src_path, pc const &dst_pc) const;
  void cg_print_stats() const;

  expr_ref cg_get_stack_pointer_expr_at_pc(pc const& dst_from_pc) const;
  void update_dst_fcall_edge_using_src_fcall_edge(pc const &src_pc, pc const &dst_pc);
  //virtual bool propagate_sprels() override;
  //virtual void populate_avail_exprs() override;
  sprel_map_pair_t get_sprel_map_pair(/*pcpair const &pp*/) const override;

  //void update_src_suffixpaths_cg(corr_graph_edge_ref e = nullptr);
  //tfg_suffixpath_t get_src_suffixpath_ending_at_pc(pcpair const &pp) const override
  //{
  //  ASSERT(m_src_suffixpath.count(pp));
  //  return m_src_suffixpath.at(pp);
  //}

  bool dst_edge_composition_proves_false(pcpair const &cg_from_pc, shared_ptr<tfg_edge_composition_t> const &dst_edge) const;

  bool cg_has_assumes_around_edge_populated() const;

  virtual graph_memlabel_map_t get_memlabel_map(call_context_ref const& cc) const override
  {
    ASSERT(cc == call_context_t::call_context_null());
    graph_memlabel_map_t memlabel_map = this->get_src_tfg().get_memlabel_map(cc);
    memlabel_map_union(memlabel_map, this->get_dst_tfg().get_memlabel_map(cc));
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

  bool dst_loc_is_live_at_pc(pcpair const &p, graph_loc_id_t loc_id) const { return this->get_dst_tfg().loc_is_live_at_pc(p.get_second(), loc_id); }

  virtual list<tuple<graph_edge_composition_ref<pc, tfg_edge>, expr_ref, predicate_ref>> edge_composition_apply_trans_funs_on_pred(predicate_ref const &p, shared_ptr<cg_edge_composition_t> const &ec) const override;

  virtual counterexample_translate_retval_t counter_example_translate_on_edge(counter_example_t &c_in, dshared_ptr<corr_graph_edge const> const &edge, counter_example_t &rand_vals, failcond_t& failcond, relevant_memlabels_t const& relevant_memlabels, bool ignore_assumes = false, bool ignore_wfconds = false) const override;
  bool dst_edge_exists_in_outgoing_cg_edges(pcpair const &pp, pc const& p) const;
  bool dst_edge_exists_in_cg_edges(dshared_ptr<tfg_edge const> const& dst_edge) const;
  //shared_ptr<tfg_edge_composition_t> dst_edge_composition_subtract_those_that_exist_in_cg_edges(pcpair const &pp, shared_ptr<tfg_edge_composition_t> ec) const;
  //string cg_preds_to_string_for_eq(pcpair const &pp) const;

  virtual list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<predicate const>>> collect_inductive_preds_around_path(pcpair const& from_pc, shared_ptr<cg_edge_composition_t> const& pth) const override;
  virtual list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<predicate const>>> collect_assumes_around_path(pcpair const& from_pc, graph_edge_composition_ref<pcpair,corr_graph_edge> const &ec) const override;

  //virtual bool check_stability_for_added_edge(corr_graph_edge_ref const& cg_edge) const override;
  //bool falsify_fcall_proof_query(expr_ref const& src, expr_ref const& dst, pcpair const& from_pc, counter_example_t& counter_example) const override;

  //bool need_stability_at_pc(pcpair const &p) const override;

  //virtual expr_ref edge_guard_get_simplified_expr(edge_guard_t const& eg/*, bool simplified*/) const override //TODO: this function does not need to be virtual
  //{
  //  NOT_IMPLEMENTED();
  //  //if (simplified) {
  //  //  NOT_IMPLEMENTED();
  //  //}
  //  //return eg.edge_guard_get_expr(this->get_context()/*, simplified*/);
  //}

  bool dst_fcall_edge_already_updated(pc const& dst_pc) const
  {
    return set_belongs(m_dst_fcall_edges_already_updated_from_pcs, dst_pc);
  }
  void dst_fcall_edge_mark_updated(pc const& dst_pc)
  {
    m_dst_fcall_edges_already_updated_from_pcs.insert(dst_pc);
  }

  cg_stats_t& get_stats() { return m_cg_stats; }
  void update_corr_counter(shared_ptr<tfg_edge_composition_t> const &dst_ec, int num_src_to_pcs, int num_corr) const
  {
    m_cg_stats.set_src_to_pcs_for_dst_ec(dst_ec, num_src_to_pcs);
    m_cg_stats.set_src_ecs_for_dst_ec(dst_ec, num_corr);
  }
  void post_cg_stats_to_global_stats() const;

// XXX: Verify this
  bv_rank_val_t calculate_rank_bveqclass_at_pc(pcpair const &) const;

  pair<expr_ref, expr_ref> compute_mem_stability_exprs(pcpair const& p) const;
  edge_wf_cond_t const& graph_edge_get_well_formedness_conditions(dshared_ptr<corr_graph_edge const> const& e) const override;

  //list<pair<graph_edge_composition_ref<pcpair,corr_graph_edge>, predicate_ref>> graph_apply_trans_funs(graph_edge_composition_ref<pcpair,corr_graph_edge> const &pth, predicate_ref const &pred, bool simplified = false) const override { NOT_REACHED(); }

  virtual list<pair<graph_edge_composition_ref<pc, tfg_edge>, shared_ptr<predicate const>>> pth_collect_simplified_preds_using_atom_func(shared_ptr<graph_edge_composition_t<pcpair, corr_graph_edge> const> const& ec, std::function<list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<predicate const>>> (dshared_ptr<corr_graph_edge const> const&)> f_atom, list<pair<graph_edge_composition_ref<pc, tfg_edge>, shared_ptr<predicate const>>> const& from_pc_preds, context* ctx) const override
  { NOT_REACHED(); }

  void add_edge(dshared_ptr<corr_graph_edge const> e) override;

  void populate_simplified_edgecond(dshared_ptr<corr_graph_edge const> const& e) override { /*NOP*/ };
  void populate_simplified_assumes(dshared_ptr<corr_graph_edge const> const& e) override { /*NOP*/ };
  void populate_simplified_to_state(dshared_ptr<corr_graph_edge const> const& e) override { /*NOP*/ };
  //void populate_locs_potentially_modified_on_edge(corr_graph_edge_ref const &e) override { /*NOP*/ }
  void populate_assumes_around_edge(dshared_ptr<corr_graph_edge const> const& e) override;
  void cg_clear_aliasing_constraints_map() const
  {
    //cout << __func__ << " " << __LINE__ << ": clearing aliasing_constraints cache\n";
    m_per_edge_aliasing_constrainsts_map.clear();
  }

  //void generate_new_CE(pcpair const& cg_from_pc, shared_ptr<tfg_edge_composition_t> const &dst_ec) const;
  //static corr_graph_edge_ref cg_edge_semantically_simplify_src_tfg_ec(corr_graph_edge const& cg_edge, tfg const& src_tfg);

  //expr_ref cg_edge_get_src_cond(corr_graph_edge const &cg_edge) const;
  //expr_ref cg_edge_get_dst_cond(corr_graph_edge const &cg_edge) const;

  //state cg_edge_get_src_to_state(corr_graph_edge const &cg_edge) const;
  //state cg_edge_get_dst_to_state(corr_graph_edge const &cg_edge) const;

  //virtual string read_graph_edge(istream& in, string const& first_line, string const& prefix, context* ctx, shared_ptr<corr_graph_edge const>& e) const override;
  virtual rodata_map_t const& get_rodata_map() const override { return this->m_eq->get_rodata_map(); }

  //virtual bool graph_has_stack() const override { return this->get_src_tfg().graph_has_stack() || this->get_dst_tfg().graph_has_stack(); }

  // The mem alloc exprs that are reaching the pcpair pp
  virtual vector<expr_ref> graph_get_mem_alloc_exprs(pcpair const& pp) const override
  {
    auto ret = this->get_src_tfg().graph_get_mem_alloc_exprs(pp.get_first());
    vector_append(ret, this->get_dst_tfg().graph_get_mem_alloc_exprs(pp.get_second()));
    return ret;
  }
  map<allocsite_t,expr_ref> cg_get_mappings_for_allocated_locals(set<allocsite_t> const& local_ids) const;
  unordered_set<expr_ref> cg_get_alloca_dst_assumes(map<allocsite_t, expr_ref> const& allocas_added_in_dst, dshared_ptr<corr_graph_edge const> const& cg_edge) const;

  void corr_graph_add_internal_pc(pcpair const& pp) { m_internal_pcs.insert(pp); }
  bool corr_graph_is_internal_pc(pcpair const& pp) const { return set_belongs(m_internal_pcs, pp); }

  void add_super_edge(edge_id_t<pcpair> const& eid)
  {
    ASSERT(find(m_super_edges.begin(), m_super_edges.end(), eid) == m_super_edges.end());
    m_super_edges.push_back(eid);
  }
  list<edge_id_t<pcpair>> const& get_super_edges() const { return m_super_edges; }

  bool graph_is_ssa() const override { return this->get_src_tfg().graph_is_ssa() && this->get_dst_tfg().graph_is_ssa(); }
  src_dst_cg_path_tuple_t src_dst_cg_path_tuple_from_stream(istream& is, string const& prefix) const override;

  virtual set<string_ref> graph_get_externally_visible_variables() const override
  {
    set<string_ref> ret = this->get_src_tfg().graph_get_externally_visible_variables();
    set_union(ret, this->get_dst_tfg().graph_get_externally_visible_variables());
    return ret;
  }

  set<pc> cg_get_reaching_src_tfg_pcs_at_pcpair(pcpair const& pp) const
  {
    set<pc> ret;
    if(m_src_pcs_reaching_cg_pcpair.count(pp))
      ret = m_src_pcs_reaching_cg_pcpair.at(pp);
    return ret;
  }

  void cg_add_reaching_src_tfg_pcs_at_pcpair(pcpair const& pp, set<pc> const& src_tfg_pcs)
  {
    m_src_pcs_reaching_cg_pcpair[pp].insert(src_tfg_pcs.begin(), src_tfg_pcs.end());
  }

  virtual void populate_pcs_topological_order() override;
  void compute_loop_hoisting_select_exprs_at_pcpair(pcpair const& to_pp, shared_ptr<tfg_full_pathset_t const> const& dst_edge);
  void compute_loop_hoisting_non_linear_exprs_at_pcpair(pcpair const& to_pp, shared_ptr<tfg_full_pathset_t const> const& dst_edge);

  expr_ref graph_get_memlabel_assertion_on_preallocated_memlabels_for_all_memallocs() const override;

  virtual unordered_set<predicate_ref> get_global_assumes_at_pc(pcpair const& p) const override
  {
    auto ret = this->get_src_tfg().get_global_assumes_at_pc(p.get_first());
    unordered_set_union(ret, this->get_dst_tfg().get_global_assumes_at_pc(p.get_second()));
    return ret;
  }

protected:
  list<tuple<graph_edge_composition_ref<pc, tfg_edge>, expr_ref, predicate_ref>> src_dst_cg_path_tuple_apply_trans_funs_on_pred(predicate_ref const &p, src_dst_cg_path_tuple_t const& src_dst_cg_path_tuple) const;

  virtual bool edge_assumes_satisfied_by_counter_example(dshared_ptr<corr_graph_edge const> const& e, counter_example_t const& ce, counter_example_t& rand_vals, relevant_memlabels_t const& relevant_memlabels) const override;
  virtual failcond_t edge_well_formedness_conditions_falsified_by_counter_example(dshared_ptr<corr_graph_edge const> const& e, counter_example_t const& ce, relevant_memlabels_t const& relevant_memlabels) const override;
  virtual predicate_ref check_preds_on_edge_compositions(pcpair const& from_pp, unordered_set<expr_ref> const& preconditions, map<src_dst_cg_path_tuple_t, unordered_set<shared_ptr<predicate const>>> const& ec_preds) const override;
  //list<pair<graph_edge_composition_ref<pcpair,corr_graph_edge>, predicate_ref>> gen_predicate_set_from_src_ec_preds(list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> const& in) const;
  static dshared_ptr<corr_graph> corr_graph_from_stream(istream& is, context* ctx);
  void cg_xml_print_helper(ostream& os, map<pc, string> const& src_pc_line_map, map<pc, string> const& asm_pc_line_map, context::xml_output_format_t xml_output_format, string const& prefix) const;

  //map<graph_loc_id_t,expr_ref> compute_simplified_loc_exprs_for_edge(corr_graph_edge_ref const& e, set<graph_loc_id_t> const& locids, bool force_update = false) const override { NOT_REACHED(); }
  map<string_ref,expr_ref> graph_get_vars_written_on_edge(dshared_ptr<corr_graph_edge const> const& e) const override { NOT_REACHED(); }

  expr_ref cg_get_preimage_for_expr_across_edge(expr_ref const& e, dshared_ptr<corr_graph_edge const> const& ed) const;


private:
  expr_ref cg_get_local_addr_expr_at_pc(pc const& src_pc, allocstack_t const& allocstack) const;
  expr_ref cg_get_local_size_expr_at_pc(pc const& src_pc, allocsite_t const& id, string const& srcdst_keyword) const;
  expr_ref cg_get_lb_expr_for_memlabel_at_pc(pc const& src_pc, memlabel_t const& lml) const;
  expr_ref cg_get_ub_expr_for_memlabel_at_pc(pc const& src_pc, memlabel_t const& lml) const;

  expr_ref cg_get_memlabel_is_absent_expr_at_pc(pc const& src_pc, memlabel_t const& ml) const;
  expr_ref cg_get_local_alloc_count_ssa_var_at_pc(pc const& src_pc, allocsite_t const& allocsite) const;
  expr_ref cg_get_preimage_for_local_alloc_count_ssa_var_across_edge_composition(tfg const& t, allocsite_t const& lid, shared_ptr<tfg_edge_composition_t> const& ec) const;

//  static tfg_edge_composition_ref append_from_and_to_pcs_to_new_ec_after_unroll(dshared_ptr<tfg> const& t, tfg_edge_composition_ref const& ec);
//  static void add_missing_outgoing_edges_for_cloned_pcs_to_tfg(tfg& t, tfg const& orig_tfg, map<pc, clone_count_t> const& cloned_pcs);
//  static void add_missing_outgoing_edges_for_cloned_pc_to_tfg(tfg& t, tfg const& orig_tfg, pc const& p, clone_count_t clone_count);
//  static void add_missing_outgoing_edge_for_cloned_pc(tfg& t, pc const& p_cloned, dshared_ptr<tfg_edge const> const& o);

  map<pc, map<pcpair, clone_count_t>> cg_identify_existing_src_pc_clones_for_to_pcpairs(string const& clone_prefix) const;
  void cg_update_definedness_after_cloning(pcpair const& pp, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map);
  //map<pc, map<pcpair, clone_count_t>> cg_identify_existing_src_pc_empty_clones_for_to_pcpairs() const;
  //map<pc, clone_count_t> cg_get_existing_src_tfg_pcs/*_reaching_pc*/(/*pc const& from_pc*/) const;
  //dshared_ptr<tfg> cg_copy_the_repeated_ec_in_src_tfg(shared_ptr<tfg_full_pathset_t const> const& src_ec_to_be_added) const;
  void  break_vectorized_exprs_to_unit_size_exprs(pcpair const& pp, set<expr_ref> &interesting_exprs, unsigned unit_size) const;
  virtual void get_anchor_pcs(set<pcpair>& anchor_pcs) const override;
  expr_ref cg_get_expr_for_ec_pred(src_dst_cg_path_tuple_t const& src_dst_cg_path_tuple, shared_ptr<predicate const> const& pred) const;
  edge_wf_cond_t cg_compute_well_formedness_conditions_for_alloca_edge(dshared_ptr<corr_graph_edge const> const& e) const;
  edge_wf_cond_t cg_compute_well_formedness_conditions_for_dealloc_edge(dshared_ptr<corr_graph_edge const> const& e) const;
  edge_wf_cond_t cg_compute_well_formedness_conditions_for_fcall_edge(dshared_ptr<corr_graph_edge const> const& e) const;
  edge_wf_cond_t cg_compute_well_formedness_conditions_for_stack_pointer(dshared_ptr<corr_graph_edge const> const& e) const;
  edge_wf_cond_t cg_compute_well_formedness_conditions_for_edge(dshared_ptr<corr_graph_edge const> const& e) const;
  void cg_populate_well_formedness_conditions_for_edge(dshared_ptr<corr_graph_edge const> const& e_in);
  static shared_ptr<corr_graph_edge const> corr_graph_edge_from_stream(istream &in, string const& first_line, string const &prefix, context* ctx, tfg const& src_tfg, tfg const& dst_tfg);

  void cg_populate_pc_local_sprel_expr_assumes_for_fargs();

  virtual void cg_pcpair_xml_print(ostream& os, pcpair const& pp) const { NOT_REACHED(); }
  list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> collect_assumes_around_edge(dshared_ptr<corr_graph_edge const> const& te) const;
  list<pair<graph_edge_composition_ref<pcpair,corr_graph_edge>, predicate_ref>> get_cg_assumes_for_edge(dshared_ptr<corr_graph_edge const> const& e) const;

  static string cg_pcpair_xml_name(pcpair const& pp, map<pc, string> const& src_pc_xml_linename_map, map<pc, string> const& dst_pc_xml_linename_map);

  list<pair<graph_edge_composition_ref<pc, tfg_edge>, predicate_ref>> get_simplified_non_mem_assumes(dshared_ptr<corr_graph_edge const> const& e) const override;
  list<pair<graph_edge_composition_ref<pc, tfg_edge>, predicate_ref>> get_simplified_non_mem_assumes_helper(dshared_ptr<corr_graph_edge const> const& e) const;
  // CG doesn't require a start state
  // Always query for the underlying src_tg and dst_tfg start state
  // and mem/mem_alloc reaching versions
  //virtual start_state_t const &get_start_state() const override { NOT_REACHED(); }
  //virtual start_state_t       &get_start_state() override       { NOT_REACHED(); }
  //virtual void populate_pc_mem_structures() const override { }

  //virtual void start_state_to_stream(ostream& ss, string const& prefix) const override
  //{
  //  //start_state_t const &src_start_state = this->get_src_tfg().get_start_state();
  //  //start_state_t const &dst_start_state = this->get_dst_tfg().get_start_state();

  //  //start_state_t cg_st = start_state_t::start_state_union(src_start_state, dst_start_state);
  //  //ss << cg_st.get_state().state_to_string_for_eq();
  //}

//  void init_cg_elems()
//  {
////    dshared_ptr<eqcheck>const& eq = m_eq;
//    tfg const& src_tfg = this->get_src_tfg();
//    tfg const& dst_tfg = this->get_dst_tfg();
//
//    //start_state_t const &src_start_state = src_tfg.get_start_state();
//    //start_state_t const &dst_start_state = dst_tfg.get_start_state();
//
//    //this->set_start_state(start_state_t::start_state_union(src_start_state, dst_start_state));
//    //this->graph_set_relevant_memlabels(src_tfg.graph_get_relevant_memlabels());
//    this->graph_set_relevant_memlabels(dst_tfg.graph_get_relevant_memlabels()); //use dst tfg's memlabels because they will include src tfg's memlabels
//    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
//    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
//  }
  //void init_cg_node(dshared_ptr<corr_graph_node> n);

  bool expr_pred_def_is_likely_nearer(pcpair const &pp, expr_ref const &a, expr_ref const &b) const;

  //void check_asserts_over_edge(corr_graph_edge_ref const &cg_edge) const;

  void select_llvmvars_designated_by_src_loc_id(graph_loc_id_t loc_id, set<expr_ref>& dst, set<expr_ref> const& src, tfg const& tfg_exprs_belong_to, bool exprs_belong_to_src_tfg) const;
  static void select_non_llvmvars_from_src_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src);
  void select_ghost_llvmvars_from_src_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src) const;
  void select_llvmvars_live_at_pc_and_add_to_dst(set<expr_ref> &dst, set<expr_ref> const& src, pc const &p, tfg const& tfg_exprs_belong_to, bool exprs_belong_to_src_tfg) const;
  void select_llvmvars_modified_on_incoming_edge_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const &pp, bool exprs_belong_to_src_tfg) const;
  void select_llvmvars_correlated_at_pred_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const& pp, bool exprs_belong_to_src_tfg) const;
  void select_llvmvars_having_sprel_mappings(set<expr_ref>& dst, set<expr_ref> const& src, tfg_path_id_t const& p, tfg const& tfg_exprs_belong_to) const;
  void select_llvmvars_appearing_in_avail_exprs_at_pc(set<expr_ref>& dst, set<expr_ref> const& src, tfg_path_id_t const& p, tfg const& tfg_exprs_belong_to) const;
  static void select_llvmvars_appearing_in_expr(set<expr_ref>& dst, set<expr_ref> const& src, expr_ref const& e);
  bool expr_llvmvar_is_correlated_with_dst_loc_at_pc(expr_ref const& e, pcpair const& pp) const;
  set<expr_ref> remove_complex_bv_exprs(set<expr_ref>& relevant_exprs) const;
  set<expr_ref> get_lsprel_mapping_related_exprs(pcpair const& pp) const;

  bool src_expr_contains_fcall_mem_on_incoming_edge(pcpair const &p, expr_ref const &e) const;

  virtual bool graph_edge_contains_unknown_function_call(dshared_ptr<corr_graph_edge const> const& cg_edge) const override;

  eqclasses_ref compute_expr_eqclasses_at_pc(pcpair const& p) const override;
  vector<expr_group_ref> compute_mem_eqclasses(pcpair const& p, set<expr_ref> const& src_interesting_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  vector<tuple<string,expr_ref,expr_ref>> compute_mem_eq_exprs(pcpair const& p, set<expr_ref> const& src_interesting_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_bv_bool_eqclass(pcpair const& p, set<expr_ref> const& src_interesting_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_dst_ineq_eqclass(pcpair const& pp) const;
  expr_group_ref compute_region_agrees_with_memlabel_eqclass(pcpair const& pp, set<expr_ref,expr_ref_cmp_t> const& all_eqclass_exprs) const;
  expr_group_ref compute_const_ineq_eqclass(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_tfg_pred_eqclass(pcpair const& pp) const;
  vector<expr_group_ref> compute_ineq_eqclasses(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  map<allocsite_t,expr_ref> get_local_id_to_sp_version_map() const;
  expr_group_ref compute_variably_sized_alloca_sp_versions_comp_eqclass(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_locals_lower_upper_bounds_and_sp_versions_ineq_eqclass(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_locals_ub_upper_bound_eqclass(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  void preprocess_bvexprs_and_identify_ranking_exprs(pcpair const& pp, set<expr_ref> &src_relevant_exprs, set<expr_ref> &dst_relevant_exprs) const;
  void prune_non_relatable_exprs(pcpair const& pp, set<expr_ref> &src_relevant_exprs, set<expr_ref> &dst_relevant_exprs) const;
  vector<expr_group_ref> compute_nonarg_locals_iscontiguous_eqclasses(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_mem_allocs_are_equal_eqclass(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_memlabel_assertions_eqclass(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  vector<expr_group_ref> compute_nonarg_locals_local_sizes_are_equal_eqclasses(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  vector<expr_group_ref> compute_nonarg_locals_sp_ml_lb_ub_inequality_eqclasses(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  vector<expr_group_ref> compute_nonces_are_equal_eqclasses(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  expr_group_ref compute_memlabel_is_absent_eqclass(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;
  vector<expr_group_ref> compute_stack_pointer_well_behaved_eqclasses(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const;

  //expr_ref get_feasibility_conditions_for_lsprel_guess(allocsite_t const& lid, expr_ref const& lsprel_expr) const;
  //edge_wf_cond_t const& graph_edge_get_well_formedness_conditions(dshared_ptr<corr_graph_edge const> const& e) const;
  void graph_set_well_formedness_conditions_for_edge(dshared_ptr<corr_graph_edge const> const& e, edge_wf_cond_t const& wfconds);

  pair<list<allocsite_t>,list<allocsite_t>> compute_allocs_and_deallocs_needing_correlation_from_alloc_deallocs_ls(list<pair<pc,pair<alloc_dealloc_pc_t,alloc_dealloc_t>>> const& pc_alloc_deallocs_ls) const;
  list<pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc>> generate_pc_local_sprel_expr_guesses_for_allocation(allocsite_t const& lid, function<bool(pc const&)> is_valid_pc) const;
  list<pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc>> generate_pc_local_sprel_expr_guesses_for_deallocation(allocsite_t const& lid, expr_ref const& addr, function<bool(pc const&)> is_valid_pc) const;
  virtual graph_loc_id_t graph_expr_to_locid(expr_ref const &e) const override;
  virtual graph_cp_location const &get_loc(graph_loc_id_t loc_id) const override;
  static
  tfg_path_id_t get_path_id_from_pcpair(bool is_src_tfg, pcpair const& pp)
  {
    if(is_src_tfg)
      return make_pair(pp.get_first(), pp.get_second());
    else
      return make_pair(pp.get_second(), pp.get_first());
  }

  void populate_cg_edge_contains_repeated_src_tfg_edge(dshared_ptr<corr_graph_edge const> const& new_cg_edge);

  set<memlabel_ref> cg_get_nonarg_locals() const;

protected:

  dshared_ptr<eqcheck> m_eq;
  mutable map<pcpair, bv_rank_exprs_t> m_ranking_exprs;
//  set<expr_ref> prune_exprs_not_defined_at_pcpair(set<expr_ref> const& interesting_exprs, pcpair const& pp) const;
  set<expr_ref> prune_llvm_exprs_for_invariant_inference(set<expr_ref> const& interesting_exprs, pcpair const& pp, bool exprs_belong_to_src_tfg) const;
  bool src_expr_is_relatable_at_pc(pcpair const &p, expr_ref const &e) const;

private:

//  graph_loc_id_t m_dst_start_loc_id;
//  set<graph_loc_id_t> m_cg_locs_for_dst_rodata_symbol_masks_on_src_mem;

  dshared_ptr<tfg_ssa_t> m_src_tfg;
  dshared_ptr<tfg_ssa_t> m_dst_tfg;

  //map<pcpair, tfg_suffixpath_t> m_src_suffixpath;
  set<pc> m_dst_fcall_edges_already_updated_from_pcs;
  set<pcpair> m_internal_pcs;
  list<edge_id_t<pcpair>> m_super_edges;
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> m_pc_local_sprel_expr_assumes_for_allocation;
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> m_pc_local_sprel_expr_assumes_for_deallocation;
  map<pcpair, set<pc>> m_src_pcs_reaching_cg_pcpair;
  map<pcpair, set<expr_ref>> m_loop_hoisting_dst_exprs;

  //set<allocsite_t> m_preallocated_locals;

  //unordered_set<predicate_ref> m_start_pc_preconditions;
  map<pcpair, unordered_set<predicate_ref>> m_exit_pc_asserts;
  map<edge_id_t<pcpair>, edge_wf_cond_t> m_edge_well_formedness_conditions;
  map<edge_id_t<pcpair>, bool> m_cg_edge_contains_repeated_src_tfg_edge;

  //stats
  mutable cg_stats_t m_cg_stats;

  // cache
  mutable map<corr_graph_edge_ref, aliasing_constraints_t> m_per_edge_aliasing_constrainsts_map;
  mutable map<corr_graph_edge_ref, list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>>> m_non_mem_assumes_cache;
};

shared_ptr<tfg_edge_composition_t> cg_edge_composition_to_src_edge_composite(corr_graph const &cg, shared_ptr<cg_edge_composition_t> const &ec, tfg const &src_tfg/*, state const &src_start_state*/);

shared_ptr<tfg_edge_composition_t> cg_edge_composition_to_dst_edge_composite(corr_graph const &cg, shared_ptr<cg_edge_composition_t> const &ec, tfg const &dst_tfg/*, state const &dst_start_state*/);


}
