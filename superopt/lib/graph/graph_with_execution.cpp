#include "graph/graph_with_execution.h"
#include "eq/corr_graph.h"
//#include "tfg/tfg.h"

namespace eqspace {

template class graph_with_execution<pcpair, corr_graph_node, corr_graph_edge, predicate>;
template class graph_with_execution<pc, tfg_node, tfg_edge, predicate>;


}


