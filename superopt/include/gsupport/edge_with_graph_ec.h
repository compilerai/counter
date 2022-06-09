#pragma once

#include "gsupport/pc_with_unroll.h"
#include "gsupport/graph_ec.h"
#include "gsupport/edge.h"
#include "gsupport/edge_with_unroll.h"

namespace eqspace {

template<typename T_PC, typename T_E>
class edge_with_graph_ec_t : public edge<pc_with_unroll<T_PC>> {
private:
  map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>> m_ecs; //what is the edge composition from the FROM_PC to each of the reduced PCs that are contained in this edge (including the TO_PC).
public:
  edge_with_graph_ec_t(pc_with_unroll<T_PC> const& from, pc_with_unroll<T_PC> const& to, dshared_ptr<edge_with_unroll_t<T_PC,T_E> const> const& e) : edge<pc_with_unroll<T_PC>>(from, to), m_ecs({{to, graph_edge_composition_t<T_PC,T_E>::mk_edge_composition(e)}})
  { }
  edge_with_graph_ec_t(pc_with_unroll<T_PC> const& from, pc_with_unroll<T_PC> const& to, map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>> const& ecs) : edge<pc_with_unroll<T_PC>>(from, to), m_ecs(ecs)
  { }

  map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>> const& get_ec_map() const { return m_ecs; }
  shared_ptr<graph_edge_composition_t<T_PC, T_E> const> const& get_ec_for_to_pc() const { return m_ecs.at(this->get_to_pc()); }

  bool edge_with_graph_ec_contains_pc(T_PC const& p) const { return m_ecs.count(p) != 0; }

  string edge_with_graph_ec_to_string(string const& prefix = "") const;

  static dshared_ptr<edge_with_graph_ec_t<T_PC, T_E> const> edge_with_graph_ec_mk_series(dshared_ptr<edge_with_graph_ec_t<T_PC, T_E> const> const& a, dshared_ptr<edge_with_graph_ec_t<T_PC, T_E> const> const& b, map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>> const& ec_map_at_common_node);

  static dshared_ptr<edge_with_graph_ec_t<T_PC, T_E> const> edge_with_graph_ec_mk_parallel(dshared_ptr<edge_with_graph_ec_t<T_PC, T_E> const> const& a, dshared_ptr<edge_with_graph_ec_t<T_PC, T_E> const> const& b);

  static void ecs_union_after_prepending_path(
      map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>>& dst,
      map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>> const& src,
      shared_ptr<graph_edge_composition_t<T_PC, T_E> const> pth);

  static void ecs_union_except_for_pc(
      map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>>& dst,
      map<pc_with_unroll<T_PC>, shared_ptr<graph_edge_composition_t<T_PC, T_E> const>> const& src,
      pc_with_unroll<T_PC> const* pc_exception);

private:
};

}
