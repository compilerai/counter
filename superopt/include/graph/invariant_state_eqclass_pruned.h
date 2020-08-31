#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "graph/point_set.h"
#include "graph/invariant_state_eqclass.h"

//XXX: this file is not being used; it may become relevant if the linear solver ever becomes the performance bottleneck.
namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_pruned_t : public invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>
{
private:
  T_PC m_pc;
  map<edge_id_t<T_PC>, set<expr_idx_t>> m_edge_to_touched_exprs;
  map<edge_id_t<T_PC>, set<expr_idx_t>> m_edge_to_untouched_exprs;
public:
  invariant_state_eqclass_pruned_t(T_PC const &p, vector<expr_ref> const &exprs_to_be_correlated, graph_with_ce<T_PC, T_N, T_E, T_PRED> const &g) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(exprs_to_be_correlated, g), m_pc(p)
  {
    //this->populate_edge_to_touched_untouched_exprs_map();
  }

  bool eqclass_xfer_on_incoming_edge(shared_ptr<T_E const> const &e, unordered_set<T_PRED> const &from_pc_preds) override
  {
    NOT_IMPLEMENTED();
    //ASSERT(m_edge_to_untouched_exprs.count(e->get_edge_id()));
    //set<expr_idx_t> ignore_exprs = m_edge_to_untouched_exprs.at(e->get_edge_id());
    //set<expr_idx_t> old_ignore_exprs;
    //do {
    //  old_ignore_exprs = ignore_exprs;
    //  for (const auto &pred : from_pc_preds) {
    //    if (this->predicate_involves_unignored_expr(pred, ignore_exprs)) {
    //      set<expr_idx_t> ptouched = this->predicate_get_touched_exprs(pred);
    //      set_difference(ignore_exprs, ptouched);
    //    }
    //  }
    //  //the size of ignore_exprs can only decrease in one iteration of the outer loop
    //  //thus the outer loop is guaranteed to converge
    //  ASSERT(ignore_exprs.size() <= old_ignore_exprs);
    //} while (ignore_exprs.size() != old_ignore_exprs.size());

    //unordered_set<T_PRED> preds_that_need_no_proof;
    //for (auto const& pred : from_pc_preds) {
    //  if (!this->predicate_involves_unignored_expr(pred, ignore_exprs)) {
    //    preds_that_need_no_proof.insert(pred);
    //  }
    //}
    //return this->eqclass_xfer_on_incoming_edge_helper(e, from_pc_preds, ignore_exprs, preds_that_need_no_proof);
  }
private:
  //void populate_edge_to_touched_untouched_exprs_map()
  //{
  //  list<shared_ptr<T_E>> incoming;
  //  m_graph->get_edges_incoming(m_pc, incoming);
  //  for (const auto &ed : incoming) {
  //    for (expr_idx_t i = 0; i < m_exprs_to_be_correlated.size(); i++) {
  //      if (m_graph.expr_var_is_touched_on_edge(m_exprs_to_be_correlated.at(i), ed)) {
  //        m_edge_to_touched_exprs[ed->get_edge_id()].insert(i);
  //      } else {
  //        m_edge_to_untouched_exprs[ed->get_edge_id()].insert(i);
  //      }
  //    }
  //  }
  //}
};

}
