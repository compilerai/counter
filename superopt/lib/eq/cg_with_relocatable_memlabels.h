#pragma once

#include "support/log.h"
#include "support/mymemory.h"

#include <list>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include "tfg/tfg.h"
#include "tfg/edge_guard.h"
#include "expr/counter_example.h"
#include "graph/eqclasses.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_inductive_preds.h"
#include "eq/unsafe_cond.h"

#include "graph/graph_with_guessing.h"
//#include "graph/predicate_acceptance.h"
#include "gsupport/corr_graph_edge.h"

namespace eqspace {

using namespace std;


class cg_with_relocatable_memlabels : public cg_with_inductive_preds
{
public:
  /*class invariants_val_t
  {
    private:
      predicate_set_t m_preds;
      bool m_is_top;
    public:
      invariants_val_t() : m_is_top(false) { }
      invariants_val_t(predicate_set_t const& preds) : m_preds(preds), m_is_top(false) { }

      static invariants_val_t top()
      {
        invariants_val_t ret = invariants_val_t();
        ret.m_is_top = true;
        return ret;
      }

      predicate_set_t const& get_preds() const { return m_preds; }

      bool xfer_on_incoming_edge(shared_ptr<corr_graph_edge const> const& e, invariants_val_t const& in_unused, corr_graph const& cg);

      bool update_proof_status_for_preds_over_edge(shared_ptr<corr_graph_edge const> const& e, predicate_set_t const& preds, corr_graph const& cg) const;
  };*/

public:
  cg_with_relocatable_memlabels(cg_with_inductive_preds const& cgi)
    : cg_with_inductive_preds(cgi)
  {
    //for (auto const& pp : cgi.graph_with_guessing_get_invariants_map()) {
    //  m_invariants_map.insert(make_pair(pp.first, invariants_val_t(pp.second)));
    //}
  }

  cg_with_relocatable_memlabels(cg_with_relocatable_memlabels const& other)
    : cg_with_inductive_preds(other)//,
      //m_invariants_map(other.m_invariants_map)
  { }

  //virtual predicate_set_t graph_with_guessing_get_invariants_at_pc(pcpair const& pc) const override
  //{
  //  return m_invariants_map.at(pc).get_preds();
  //}

  //virtual map<pcpair, predicate_set_t> graph_with_guessing_get_invariants_map() const override
  //{
  //  map<pcpair, predicate_set_t> ret;
  //  for (auto const& pi : m_invariants_map) {
  //    ret.insert(make_pair(pi.first, pi.second.get_preds()));
  //  }
  //  return ret;
  //}

  bool check_equivalence_proof();

private:
  //void update_invariants_proof_status();

  //map<expr_id_t,pair<expr_ref,expr_ref>> reset_memlabel_range_submap() const;
  //void set_memlabel_range_submap(map<expr_id_t,pair<expr_ref,expr_ref>> const& memlabel_range_submap) const;

  //map<pcpair, invariants_val_t> m_invariants_map;
};

}
