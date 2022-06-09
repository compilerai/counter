#pragma once

#include "expr/pc.h"

#include "gsupport/pc_with_unroll.h"
#include "gsupport/node_with_graph_ec.h"
#include "gsupport/edge_with_graph_ec.h"
#include "gsupport/tfg_edge.h"
#include "gsupport/corr_graph_edge.h"

#include "graph/graph.h"

namespace eqspace {

template<typename T_PC, typename T_E>
class graph_ec_unroll_t : public graph<pc_with_unroll<T_PC>, node_with_graph_ec_t<T_PC, T_E>, edge_with_graph_ec_t<T_PC, T_E>>
{
public:
  map<pc_with_unroll<T_PC>, graph_edge_composition_ref<T_PC, T_E>> reduce_and_return_serpar_decomposition();
  bool append_graph(dshared_ptr<graph_ec_unroll_t<T_PC,T_E> const> other);
  //shared_ptr<pc_with_unroll<T_PC> const> get_most_promising_sink(T_PC const& from_pc, map<T_PC, int> const& from_pc_distances) const;

  graph_edge_composition_ref<T_PC,T_E> graph_ec_unroll_get_ec_from_source_to_sink() const;

  pc_with_unroll<T_PC> graph_ec_unroll_get_source_pc() const;
  pc_with_unroll<T_PC> graph_ec_unroll_get_sink_pc() const;

  void graph_ec_unroll_to_stream(ostream& os, string const& prefix) const;

  //returns true if an edge was added
  bool graph_ec_unroll_add_edge(dshared_ptr<T_E const> const& e, int from_pc_count, int to_pc_count);

  //returns true if there was a change due to the union
  bool graph_ec_unroll_union(graph_ec_unroll_t<T_PC, T_E> const& other);

private:
  bool graph_ec_unroll_has_a_unique_sink() const;
  //pc_with_unroll<T_PC> determine_source_node() const;
  bool merge_parallel(/*list<shared_ptr<edge_with_graph_ec_t<T_PC, T_E> const>> const& outgoing*/);
  bool merge_serial(/*list<shared_ptr<edge_with_graph_ec_t<T_PC, T_E> const>> const& outgoing, map<pc_with_unroll<T_PC>, graph_edge_composition_ref<T_PC, T_E>>& pc_pathset_map*/);
  bool collect_sink(/*stack<shared_ptr<edge_with_graph_ec_t<T_PC,T_E> const>>* sinks, */bool require_one_incoming_edge);
  int compute_weight_for_sink(pc_with_unroll<T_PC> const& from_pc, map<T_PC, int> const& from_pc_distances) const;
  static map<pc_with_unroll<T_PC>, graph_edge_composition_ref<T_PC, T_E>> get_ec_map_from_sink_edges(stack<dshared_ptr<edge_with_graph_ec_t<T_PC,T_E> const>> const& sinks);
};

}
