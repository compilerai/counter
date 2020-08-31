#include "support/types.h"
#include "graph/graph_with_guessing.h"
#include "expr/counter_example.h"
#include "graph/graphce.h"
#include "graph/nodece.h"
#include "graph/point_set.h"
#include "graph/invariant_state_eqclass.h"
#include "graph/invariant_state_eqclass_arr.h"
#include "eq/corr_graph.h"

namespace eqspace {

template<>
long invariant_state_eqclass_arr_t<pc, tfg_node, tfg_edge, predicate>::m_eqclass_arr_id = 0;
template<>
long invariant_state_eqclass_arr_t<pcpair, corr_graph_node, corr_graph_edge, predicate>::m_eqclass_arr_id = 0;

}
