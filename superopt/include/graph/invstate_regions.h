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
class invstate_regions_t
{
private:
  map<T_PC, invstate_region_t<T_PC>> m_regions;
public:
  invstate_regions_t(map<T_PC, invstate_region_t<T_PC>> const& regions) : m_regions(regions)
  { }
  size_t size() const { return m_regions.size(); }
  map<T_PC, invstate_region_t<T_PC>> const& invstate_regions_get_map() const
  { return m_regions; }
};

}
