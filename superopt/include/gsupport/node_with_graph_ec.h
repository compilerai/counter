#pragma once

#include "gsupport/pc_with_unroll.h"
#include "gsupport/graph_ec.h"
#include "gsupport/node.h"

namespace eqspace {

template<typename T_PC, typename T_E>
class node_with_graph_ec_t : public node<pc_with_unroll<T_PC>> {
private:
  map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>> m_ecs; //what is the edge composition from the FROM_PC to each of the PCs that have been collected as a "sink"
public:
  node_with_graph_ec_t(pc_with_unroll<T_PC> const& p) : node<pc_with_unroll<T_PC>>(p)
  { }
  map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>> const& get_ec_map() const { return m_ecs; }
  void node_with_graph_ec_union_ec_map(map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>> const& ec_map);
};

}
