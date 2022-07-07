#ifndef EQCHECK_CORREL_ENTRY_H
#define EQCHECK_CORREL_ENTRY_H

#include "eq/eqcheck.h"
#include "eq/cg_with_asm_annotation.h"
#include "graph/graph_with_guessing.h"
#include "support/backtracker.h"

namespace eqspace {

class correlate;

class correl_entry_ladder_t
{
public:
  typedef enum { LADDER_STARTPC_NODE = 0, LADDER_JUST_STARTED/*, LADDER_ENUM_IMPLYING_CHILDREN*/, LADDER_ENUM_IMPLIED_CHILDREN, LADDER_DONE } ladder_status_t;
  correl_entry_ladder_t(/*tfg const &dst_tfg, */ladder_status_t lstatus) : /*m_dst_tfg(dst_tfg), */m_ladder_status(lstatus)/*, m_ladder_impl(LADDER_IMPLYING)*/
  {
    //m_just_started = true;
    //m_dst_anchor_pcs = m_dst_tfg.tfg_identify_anchor_pcs();
    //m_dst_anchor_pcs = m_dst_tfg.get_all_pcs();
  }

  string correl_entry_ladder_to_string() const;
  static correl_entry_ladder_t correl_entry_ladder_from_string(string const& s);

  ladder_status_t ladder_get_status() const { return m_ladder_status; }
  //ladder_impl_t ladder_get_impl() const { return m_ladder_impl; }
  void ladder_update_status()
  {
    switch (m_ladder_status) {
      //case LADDER_JUST_STARTED:           m_ladder_status = LADDER_ENUM_IMPLYING_CHILDREN; return;
      case LADDER_STARTPC_NODE:           m_ladder_status = LADDER_ENUM_IMPLIED_CHILDREN; return;
      case LADDER_JUST_STARTED:           m_ladder_status = LADDER_ENUM_IMPLIED_CHILDREN; return;
      //case LADDER_ENUM_IMPLYING_CHILDREN: m_ladder_status = LADDER_ENUM_IMPLIED_CHILDREN; return;
      case LADDER_ENUM_IMPLIED_CHILDREN:  m_ladder_status = LADDER_DONE; return;
      case LADDER_DONE:                   NOT_REACHED();
    }
    NOT_REACHED();
  }

  //bool just_started_test_and_set() { if (!m_just_started) { return false; } m_just_started = false; return true; }
  //bool get_implying() const { return m_ladder_impl == LADDER_IMPLYING; }

  //bool next(pc const &p)
  //{
  //  if (m_just_started) {
  //    m_just_started = false;
  //    return false;
  //  }
  //  if (m_implying && !p.is_exit()) {
  //    m_implying = false;
  //    return true;
  //  }
  //  return false;
  //  /*if (!set_belongs(m_dst_anchor_pcs, p)) {
  //    return false;
  //  }
  //  m_dst_anchor_pcs.erase(p);
  //  if (m_dst_tfg.tfg_has_anchor_free_cycle(m_dst_anchor_pcs)) {
  //    return false;
  //  }
  //  m_implying = true;
  //  return true;*/
  //}

bool get_next_dst_edge_composition_to_correlate(dshared_ptr<cg_with_asm_annotation const> const &cg, map<pc, int> const &ordered_dst_pcs, list<shared_ptr<tfg_full_pathset_t const>> &unrolled_dst_pths, pcpair &chosen_pcpair) const;
  static void get_dst_unrolled_paths_from_pc(dshared_ptr<cg_with_asm_annotation const> const &cg, pcpair &chosen_pcpair, map<pc, list<shared_ptr<tfg_full_pathset_t const>>> &unrolled_dst_pths);
  static bool dst_edge_appears_earlier(std::map<pc, int> const &ordered_dst_pcs, shared_ptr<tfg_full_pathset_t const> const &a, shared_ptr<tfg_full_pathset_t const> const &b);

private:
  //tfg const &m_dst_tfg;
  //bool m_implying;
  //bool m_just_started;
  ladder_status_t m_ladder_status;
  //ladder_impl_t const m_ladder_impl;
  //set<pc> m_dst_anchor_pcs;
};

class correl_entry_t : public backtracker_node_t
{
  using correl_entry_apply_retval_t = enum { CORREL_ENTRY_APPLY_RETVAL_OK, CORREL_ENTRY_APPLY_RETVAL_CG_EDGE_NOT_WELL_FORMED, CORREL_ENTRY_APPLY_RETVAL_DELAYED_EXPLORATION };
public: 
  using ce_propagation_status_t = enum { CE_PROPAGATED, CE_NOT_PROPAGATED };
  //correl_entry_t(correlate const &c);
  correl_entry_t(correlate const &c, dshared_ptr<cg_with_asm_annotation> const& cg_new, pcpair const& pp, int path_delta, int path_mu, correl_entry_ladder_t ladder_status, string const& correl_entry_name);
  virtual ~correl_entry_t() { };

  correlate const &get_correlate() const { return m_correlate; }
  virtual bool corr_graph_add_correlation_and_create_new_correl_entry() = 0;
  dshared_ptr<cg_with_asm_annotation> get_cg() const { return m_cg; }
  pcpair const &get_pp() const { return m_pp; }
  //edge_id_t<pc> const &get_dst_edge_id() const { return m_dst_edge_id; }
  shared_ptr<tfg_full_pathset_t const> virtual get_dst_ec() const = 0;
  shared_ptr<tfg_full_pathset_t const> virtual get_src_ec() const = 0;
  //shared_ptr<tfg_edge_composition_t> const &get_dst_ec() const { return m_dst_ec; }
  //pc const &get_src_pc() const { return m_src_pc; }
  //shared_ptr<tfg_edge_composition_t> const &get_candidate_src_path() const { return m_candidate_src_path; }
  virtual graph_edge_composition_ref<pcpair,corr_graph_edge> get_cg_ec() const = 0;
  //bool get_implying() const { return m_implying/*return m_ladder.get_implying()*/; }
  bool virtual dst_edge_id_is_empty() const = 0;
  //void set_dst_edge_id_to_empty() { m_dst_ec = nullptr; /*m_dst_edge_id = edge_id_t<pc>::empty(); *//*m_dst_edge_id.first = pc::start(); m_dst_edge_id.second = pc::start();*/ }
  void virtual set_dst_edge_id_to_empty() = 0;
  void correl_entry_xml_print(ostream& os, string const& prefix) const;

  //bool virtual backtracker_node_is_useless() const override { return !m_cg->graph_is_stable(); }

  bv_rank_val_t get_bv_rank() const { return m_cg->get_rank(); }
  //void set_bv_rank(bv_rank_val_t bv_rank);
  correl_entry_apply_retval_t correl_entry_apply(/*backtracker &bt*/);
  //void set_cg(shared_ptr<corr_graph const> const &cg) { m_cg = cg; }

  string correl_entry_to_string(string prefix) const;

  int get_path_delta() const { return m_path_delta; }
  int get_path_mu() const { return m_path_mu; }
  static string get_next_groupname(string const& correl_entry_name);
  string const& get_correl_entry_name() const { return m_correl_entry_name; }
  ce_propagation_status_t virtual get_ce_propagation_status() const  = 0;

  void correl_entry_to_stream(ostream& os, string const& prefix = "") const;
  static dshared_ptr<correl_entry_t> correl_entry_from_stream(istream& os, string const& prefix = "");

protected:
  list<dshared_ptr<backtracker_node_t>> find_siblings_with_same_dst_ec_src_to_pc_higher_delta() const;

private:

  //backtracker_node_t functions
  virtual backtracker_node_t::backtracker_explore_retval_t get_next_level_possibilities(list<dshared_ptr<backtracker_node_t>> &children/*, backtracker &bt*/) override;

private:
  correlate const &m_correlate;
  dshared_ptr<cg_with_asm_annotation> m_cg;
  pcpair m_pp;
  //shared_ptr<tfg_edge_composition_t> m_dst_ec;
  //pc m_src_pc;
  int m_path_delta;
  int m_path_mu;
  //shared_ptr<tfg_edge_composition_t> m_candidate_src_path;
  //bool m_implying;
  //bv_rank_val_t m_bv_rank;
  //int m_num_times_get_next_level_possibilities_called;
  correl_entry_ladder_t m_ladder;
  
  string m_correl_entry_name;
};

class enum_correl_entry_t : public correl_entry_t
{
public: 
  enum_correl_entry_t(correlate const &c, dshared_ptr<cg_with_asm_annotation> const& cg_new, pcpair const& pp, shared_ptr<tfg_full_pathset_t const> const& src_ec, shared_ptr<tfg_full_pathset_t const> const& dst_ec, int path_delta, int path_mu, string const& correl_entry_name, pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> const& pc_lsprels_for_allocs, pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> const& pc_lsprels_for_deallocs);

  virtual bool corr_graph_add_correlation_and_create_new_correl_entry() override;
  ce_propagation_status_t virtual get_ce_propagation_status() const override { return correl_entry_t::CE_NOT_PROPAGATED; }
  shared_ptr<tfg_full_pathset_t const> virtual get_dst_ec() const override { return m_dst_ec; }
  shared_ptr<tfg_full_pathset_t const> virtual get_src_ec() const override { return m_src_ec; }
  virtual graph_edge_composition_ref<pcpair,corr_graph_edge> get_cg_ec() const override { NOT_REACHED(); }
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> correl_entry_get_pc_local_sprel_expr_assumes_for_allocation() const { return m_pc_lsprels_allocs; }
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> correl_entry_get_pc_local_sprel_expr_assumes_for_deallocation() const { return m_pc_lsprels_deallocs; }
  bool virtual dst_edge_id_is_empty() const override { NOT_REACHED(); }
  void virtual set_dst_edge_id_to_empty() override { NOT_REACHED(); }
  bool virtual backtracker_node_is_useless() const override { return !this->get_cg()->graph_is_stable(); }

private:
  shared_ptr<tfg_full_pathset_t const> m_src_ec;
  shared_ptr<tfg_full_pathset_t const> m_dst_ec;
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> m_pc_lsprels_allocs;
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> m_pc_lsprels_deallocs;
};

class CE_propagated_correl_entry_t : public correl_entry_t
{
public: 
  CE_propagated_correl_entry_t(correlate const &c, dshared_ptr<cg_with_asm_annotation> const& cg_new, graph_edge_composition_ref<pcpair,corr_graph_edge> const& cg_new_ec, shared_ptr<tfg_full_pathset_t const> const& src_ec, shared_ptr<tfg_full_pathset_t const> const& dst_ec, int path_delta, int path_mu, string const& correl_entry_name);
  CE_propagated_correl_entry_t(correlate const &c, dshared_ptr<cg_with_asm_annotation> const &cg, string const& correl_entry_name);
  
  virtual bool corr_graph_add_correlation_and_create_new_correl_entry() override { NOT_REACHED(); }

  ce_propagation_status_t virtual get_ce_propagation_status() const override { return correl_entry_t::CE_PROPAGATED; }
  virtual shared_ptr<tfg_full_pathset_t const> get_dst_ec() const override { return m_dst_ec; }
  virtual shared_ptr<tfg_full_pathset_t const> get_src_ec() const override { return m_src_ec; }
  virtual graph_edge_composition_ref<pcpair,corr_graph_edge> get_cg_ec() const override { return m_cg_ec; }
  bool virtual dst_edge_id_is_empty() const override { return !m_dst_ec; }
  void virtual set_dst_edge_id_to_empty() override
  {
    m_dst_ec = nullptr;
    //m_cg_edge = dshared_ptr<corr_graph_edge const>::dshared_nullptr();
    m_cg_ec = nullptr;
  }
  bool virtual backtracker_node_is_useless() const override { return !this->get_cg()->graph_is_stable(); }

private:
  graph_edge_composition_ref<pcpair,corr_graph_edge> m_cg_ec;
  shared_ptr<tfg_full_pathset_t const> m_src_ec;
  shared_ptr<tfg_full_pathset_t const> m_dst_ec;
};

}


#endif
