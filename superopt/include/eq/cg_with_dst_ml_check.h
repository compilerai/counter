#pragma once

#include "support/log.h"
#include "support/mymemory.h"

#include "tfg/tfg.h"
//#include "gsupport/edge_guard.h"
#include "expr/counter_example.h"
#include "graph/eqclasses.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_safety.h"

#include "graph/graph_with_guessing.h"
//#include "graph/predicate_acceptance.h"
#include "gsupport/corr_graph_edge.h"

namespace eqspace {

using namespace std;


class cg_with_dst_ml_check : public cg_with_safety
{
public:
  cg_with_dst_ml_check(cg_with_safety const &cgs) : cg_with_safety(cgs),
                                                    m_dst_memlabel_map(cgs.get_dst_tfg().get_memlabel_map(call_context_t::call_context_null()))
  { }

  bool check_dst_mls() const;

  graph_memlabel_map_t get_memlabel_map(call_context_ref const& cc) const override;

private:
  graph_memlabel_map_t cg_eliminate_dst_memlabels_in_memlabel_map();
  virtual aliasing_constraints_t get_aliasing_constraints_for_edge(dshared_ptr<corr_graph_edge const> const& e) const override;

  static list<guarded_pred_t> tfg_ec_memlabel_assumptions_negated(shared_ptr<tfg_edge_composition_t> const& ec, tfg const& t, corr_graph const& cg/*, pcpair const& pp*/, graph_memlabel_map_t const& dst_memlabel_map);

  graph_memlabel_map_t m_dst_memlabel_map;
};

}
