#include "graph/graph_with_ce.h"
#include "eq/corr_graph.h"

namespace eqspace {

//template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
//void
//print_cg_helper(graph_with_ce<T_PC, T_N, T_E, T_PRED> const &g)
//{
//  corr_graph const *cg = dynamic_cast<corr_graph const *>(&g);
//  ASSERT(cg);
//  tfg const &src_tfg = cg->get_eq()->get_src_tfg();
//  tfg const &dst_tfg = cg->get_dst_tfg();
//
//  cout << "SRC TFG:\n" << src_tfg.graph_to_string() << endl;
//  cout << "DST TFG:\n" << dst_tfg.graph_to_string() << endl;
//}
//
//template void print_cg_helper<pcpair, corr_graph_node, corr_graph_edge, predicate>(graph_with_ce<pcpair, corr_graph_node, corr_graph_edge, predicate> const &g);

}
