#pragma once

#include <map>
#include <list>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

#include "support/utils.h"
#include "support/log.h"
#include "support/searchable_queue.h"
#include "support/timers.h"
#include "support/dyn_debug.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "expr/local_sprel_expr_guesses.h"
#include "expr/pc.h"

#include "gsupport/edge_with_cond.h"
#include "gsupport/failcond.h"
#include "gsupport/sprel_map.h"

#include "graph/graph.h"
#include "graph/graph_ec_unroll.h"
#include "graph/graph_with_proofs.h"
#include "graph/graph_with_points.h"
#include "graph/lr_map.h"
#include "graph/dfa.h"
#include "graph/dfa_over_paths.h"
#include "graph/point_set.h"
#include "graph/invariant_state.h"
#include "graph/eqclasses.h"
#include "graph/expr_group.h"
#include "graph/reason_for_counterexamples.h"

namespace eqspace {

using namespace std;

template <typename T_PC>
class invstate_region_t
{
private:
  T_PC m_representative_pc;
  list<T_PC> m_pcs;
public:
  invstate_region_t(T_PC const& name, list<T_PC> const& pcs) : m_representative_pc(name), m_pcs(pcs)
  { }
  string invstate_region_to_string() const { NOT_IMPLEMENTED(); }
  static invstate_region_t<T_PC> invstate_region_create_from_string(string const& pc_str) { NOT_IMPLEMENTED(); }
  T_PC const& get_representative_pc() const { return m_representative_pc; }
  bool operator<(invstate_region_t const& other) const
  {
    return this->m_representative_pc < other.m_representative_pc;
  }
  bool operator==(invstate_region_t const& other) const
  {
    return this->m_representative_pc == other.m_representative_pc;
  }

};

}
