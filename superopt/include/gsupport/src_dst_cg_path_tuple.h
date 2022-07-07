#pragma once

#include "expr/pc.h"

#include "gsupport/graph_ec.h"
#include "gsupport/predicate.h"
#include "gsupport/predicate_set.h"
#include "gsupport/tfg_edge.h"

namespace eqspace {

using namespace std;

class src_dst_cg_path_tuple_t {
private:
  graph_edge_composition_ref<pcpair,corr_graph_edge> m_cg_path;
  graph_edge_composition_ref<pc,tfg_edge> m_src_path;
  graph_edge_composition_ref<pc,tfg_edge> m_dst_path;
public:
  src_dst_cg_path_tuple_t() : m_cg_path(graph_edge_composition_t<pcpair, corr_graph_edge>::mk_epsilon_ec()), m_src_path(graph_edge_composition_t<pc, tfg_edge>::mk_epsilon_ec()), m_dst_path(graph_edge_composition_t<pc,tfg_edge>::mk_epsilon_ec())
  { }
  src_dst_cg_path_tuple_t(graph_edge_composition_ref<pcpair,corr_graph_edge> const& cg_path) : m_cg_path(cg_path), m_src_path(graph_edge_composition_t<pc, tfg_edge>::mk_epsilon_ec()), m_dst_path(graph_edge_composition_t<pc,tfg_edge>::mk_epsilon_ec())
  { }
  src_dst_cg_path_tuple_t(graph_edge_composition_ref<pc,tfg_edge> const& src_path, graph_edge_composition_ref<pc,tfg_edge> const& dst_path) : m_cg_path(graph_edge_composition_t<pcpair, corr_graph_edge>::mk_epsilon_ec()), m_src_path(src_path), m_dst_path(dst_path)
  { }
  src_dst_cg_path_tuple_t(graph_edge_composition_ref<pcpair,corr_graph_edge> const& cg_path, graph_edge_composition_ref<pc,tfg_edge> const& src_path, graph_edge_composition_ref<pc,tfg_edge> const& dst_path) : m_cg_path(cg_path), m_src_path(src_path), m_dst_path(dst_path)
  { }
  graph_edge_composition_ref<pcpair,corr_graph_edge> const& get_cg_path() const { return m_cg_path; }
  graph_edge_composition_ref<pc,tfg_edge> const& get_src_path() const { return m_src_path; }
  graph_edge_composition_ref<pc,tfg_edge> const& get_dst_path() const { return m_dst_path; }
  string src_dst_cg_path_tuple_to_string() const;
  void src_dst_cg_path_tuple_to_stream(ostream& os, string const& prefix) const;

  bool operator<(src_dst_cg_path_tuple_t const& other) const
  {
    if (m_cg_path != other.m_cg_path) {
      return m_cg_path < other.m_cg_path;
    }
    if (m_src_path != other.m_src_path) {
      return m_src_path < other.m_src_path;
    }
    if (m_dst_path != other.m_dst_path) {
      return m_dst_path < other.m_dst_path;
    }
    return false;
  }
};

}
