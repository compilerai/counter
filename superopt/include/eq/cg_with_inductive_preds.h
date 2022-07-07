#pragma once

#include "support/log.h"
#include "support/mymemory.h"

#include "tfg/tfg.h"
//#include "gsupport/edge_guard.h"
#include "expr/counter_example.h"
#include "graph/eqclasses.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_asm_annotation.h"

#include "graph/graph_with_guessing.h"
//#include "graph/predicate_acceptance.h"
#include "gsupport/corr_graph_edge.h"

namespace eqspace {

using namespace std;


class cg_with_inductive_preds : public cg_with_asm_annotation
{
private:
  //map<pcpair, unordered_set<predicate>> m_inductive_preds;
public:
  cg_with_inductive_preds(cg_with_asm_annotation const &cg/*, map<pcpair, unordered_set<predicate>> const& inductive_preds*/) : cg_with_asm_annotation(cg)//, m_inductive_preds(inductive_preds)
  { }

  cg_with_inductive_preds(cg_with_inductive_preds const& other) : cg_with_asm_annotation(other)//, m_inductive_preds(other.m_inductive_preds)
  { }

  //virtual unordered_set<predicate> graph_with_guessing_get_invariants_at_pc(pcpair const& pc) const override
  //{
  //  return m_inductive_preds.at(pc);
  //}
  //virtual map<pcpair, unordered_set<predicate>> graph_with_guessing_get_invariants_map() const override
  //{
  //  //cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": entry\n";
  //  return m_inductive_preds;
  //}
  //map<pcpair, unordered_set<predicate>>& cg_get_invariants_map_ref()
  //{
  //  return m_inductive_preds;
  //}

  bool check_equivalence_proof() const;
  //static shared_ptr<cg_with_inductive_preds> cg_read_from_file(string const &filename, shared_ptr<eqcheck> const& eq);

private:
  expr_ref cg_get_simplified_edgecond(dshared_ptr<corr_graph_edge const> const& e) const;

  bool cg_outgoing_edges_at_pc_cover_all_possibilities(pcpair const& pp) const;
  bool check_inductive_preds_on_edge(dshared_ptr<corr_graph_edge const> const& e_in) const;
  //bool check_asserts_on_edge(shared_ptr<corr_graph_edge const> const& e_in) const;

  //bool check_preds_on_edge(shared_ptr<corr_graph_edge const> const& e_in, predicate_set_t const& preds) const;
};

}
