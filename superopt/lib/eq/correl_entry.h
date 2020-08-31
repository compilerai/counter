#ifndef EQCHECK_CORREL_ENTRY_H
#define EQCHECK_CORREL_ENTRY_H

#include "eq/eqcheck.h"
#include "eq/corr_graph.h"
#include "graph/graph_with_guessing.h"
#include "support/backtracker.h"

namespace eqspace {

class correlate;

class correl_entry_ladder_t
{
public:
  typedef enum { LADDER_STARTPC_NODE = 0, LADDER_JUST_STARTED/*, LADDER_ENUM_IMPLYING_CHILDREN*/, LADDER_ENUM_IMPLIED_CHILDREN, LADDER_DONE } ladder_status_t;
  correl_entry_ladder_t(tfg const &dst_tfg, ladder_status_t lstatus) : /*m_dst_tfg(dst_tfg), */m_ladder_status(lstatus)/*, m_ladder_impl(LADDER_IMPLYING)*/
  {
    //m_just_started = true;
    //m_dst_anchor_pcs = m_dst_tfg.tfg_identify_anchor_pcs();
    //m_dst_anchor_pcs = m_dst_tfg.get_all_pcs();
  }

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

bool get_next_dst_edge_composition_to_correlate(shared_ptr<corr_graph const> const &cg, map<pc, int> const &ordered_dst_pcs, list<shared_ptr<tfg_edge_composition_t>> &unrolled_dst_pths, pcpair &chosen_pcpair) const;
  static void get_dst_unrolled_paths_from_pc(shared_ptr<corr_graph const> const &cg, pcpair &chosen_pcpair, map<pc, list<shared_ptr<tfg_edge_composition_t>>> &unrolled_dst_pths);
  static bool dst_edge_appears_earlier(std::map<pc, int> const &ordered_dst_pcs, shared_ptr<tfg_edge_composition_t> const &a, shared_ptr<tfg_edge_composition_t> const &b);

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
public:
  //correl_entry_t(correlate const &c);
  correl_entry_t(correlate const &c, shared_ptr<corr_graph> const &cg);
  //correl_entry_t(correlate const &c, shared_ptr<corr_graph const> const &cg, pcpair const &pp, shared_ptr<tfg_edge_composition_t> const &dst_ec, pc const &src_pc, int path_delta, int path_mu, shared_ptr<tfg_edge_composition_t> const &candidate_src_path/*, bool implying*/);
  correl_entry_t(correlate const &c, shared_ptr<corr_graph> const& cg_new, shared_ptr<corr_graph_edge const> const& cg_new_edge, int path_delta, int path_mu);

  correlate const &get_correlate() const { return m_correlate; }
  shared_ptr<corr_graph> get_cg() const { return m_cg; }
  pcpair const &get_pp() const { return m_pp; }
  //edge_id_t<pc> const &get_dst_edge_id() const { return m_dst_edge_id; }
  shared_ptr<tfg_edge_composition_t> get_dst_ec() const { return m_cg_edge ? m_cg_edge->get_dst_edge() : nullptr; }
  //shared_ptr<tfg_edge_composition_t> const &get_dst_ec() const { return m_dst_ec; }
  //pc const &get_src_pc() const { return m_src_pc; }
  //shared_ptr<tfg_edge_composition_t> const &get_candidate_src_path() const { return m_candidate_src_path; }
  shared_ptr<corr_graph_edge const> get_cg_edge() const { return m_cg_edge; }
  //bool get_implying() const { return m_implying/*return m_ladder.get_implying()*/; }
  bool dst_edge_id_is_empty() const { return !m_cg_edge; }
  //void set_dst_edge_id_to_empty() { m_dst_ec = nullptr; /*m_dst_edge_id = edge_id_t<pc>::empty(); *//*m_dst_edge_id.first = pc::start(); m_dst_edge_id.second = pc::start();*/ }
  void set_dst_edge_id_to_empty() { m_cg_edge = nullptr; }

  bv_rank_val_t get_bv_rank() const { return m_bv_rank; }
  void set_bv_rank(bv_rank_val_t bv_rank);
  backtracker_node_t::backtracker_explore_retval_t correl_entry_apply(backtracker &bt);
  //void set_cg(shared_ptr<corr_graph const> const &cg) { m_cg = cg; }

  string correl_entry_to_string(string prefix) const;

  int get_path_delta() const { return m_path_delta; }
  int get_path_mu() const { return m_path_mu; }

private:

  //backtracker_node_t functions
  virtual backtracker_node_t::backtracker_explore_retval_t get_next_level_possibilities(list<backtracker_node_t *> &children, backtracker &bt) override;
  list<backtracker_node_t *> find_siblings_with_same_dst_ec_src_to_pc_higher_delta() const;

private:
  correlate const &m_correlate;
  shared_ptr<corr_graph> m_cg;
  pcpair m_pp;
  shared_ptr<corr_graph_edge const> m_cg_edge;
  //shared_ptr<tfg_edge_composition_t> m_dst_ec;
  //pc m_src_pc;
  int m_path_delta;
  int m_path_mu;
  //shared_ptr<tfg_edge_composition_t> m_candidate_src_path;
  //bool m_implying;
  bv_rank_val_t m_bv_rank;
  //int m_num_times_get_next_level_possibilities_called;
  correl_entry_ladder_t m_ladder;
};

}


#endif
