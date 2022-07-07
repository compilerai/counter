#pragma once

#include "support/log.h"
#include "support/mymemory.h"
#include "support/bv_rank_val.h"
#include "support/backtracker.h"

#include "expr/counter_example.h"

#include "rewrite/assembly_handling.h"

#include "gsupport/corr_graph_edge.h"
#include "gsupport/failcond.h"

#include "graph/eqclasses.h"
#include "graph/graph_with_guessing.h"
#include "graph/reason_for_counterexamples.h"

#include "tfg/tfg.h"

#include "eq/cg_with_rank.h"
#include "eq/unsafe_cond.h"

namespace eqspace {

using namespace std;

class cg_with_backtracker : public cg_with_rank
{
private:
  dshared_ptr<backtracker> m_backtracker;
public:
  cg_with_backtracker(cg_with_rank const &cg, dshared_ptr<backtracker> const& backtracker) : cg_with_rank(cg), m_backtracker(backtracker)
  { }

  cg_with_backtracker(string_ref const &name, string_ref const& fname, dshared_ptr<eqcheck> const& eq, dshared_ptr<backtracker> const& backtracker) : cg_with_rank(name, fname, eq), m_backtracker(backtracker)
  { }

  cg_with_backtracker(cg_with_backtracker const& other) : cg_with_rank(other), m_backtracker(other.m_backtracker)
  { }

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override;
  static dshared_ptr<cg_with_backtracker> corr_graph_from_stream(istream& is, context* ctx);

  //will also add counterexamples and propagate in other CGs in the frontier of the backtracker
  virtual bool add_fresh_counterexample_at_pc_and_propagate(pcpair const &p, reason_for_counterexamples_t const& reason_for_counterexamples, counter_example_t const& ce, graph_edge_composition_ref<pcpair,corr_graph_edge> const &query_path_on_which_the_counterexample_was_generated) const override;

protected:
private:
  void add_counterexample_to_other_relevant_cgs_in_backtracker(pcpair const& p, reason_for_counterexamples_t const& reason, counter_example_t const& ce) const;
};

}
