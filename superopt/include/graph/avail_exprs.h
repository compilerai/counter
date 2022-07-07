#pragma once

#include "support/types.h"

#include "expr/expr.h"
#include "expr/ssa_utils.h"

#include "gsupport/tfg_edge.h"
#include "gsupport/corr_graph_edge.h"

#include "graph/avail_exprs_val.h"
#include "graph/graph_cp_location.h"
#include "graph/graph_per_loc_dfa_val.h"

using namespace std;
namespace eqspace {

class pcpair;
class corr_graph_node;
class corr_graph_edge;
class predicate;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_aliasing;

//class pred_avail_exprs_analysis;
template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class avail_exprs_analysis;
//class pred_avail_exprs_val_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class available_exprs_alias_analysis_combo_val_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class available_exprs_alias_analysis_combo_dfa_t;

using avail_exprs_t = graph_per_loc_dfa_val_t<avail_exprs_val_t>;

expr_ref avail_exprs_and(expr_ref const &in, avail_exprs_t const& av, map<graph_loc_id_t, expr_ref> const &locid2expr_map);

}
