#pragma once

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>

#include "graph/graph.h"
#include "graph/graph_bbl.h"
#include "graph/graph_loop.h"
#include "graph/graph_region_type.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E>
class graph_region_t;

template<typename T_PC, typename T_E>
class graph_region_pc_t {
private:
  T_PC m_entry;
  list<dshared_ptr<T_E const>> m_incoming_edges;
  list<dshared_ptr<T_E const>> m_outgoing_edges;
public:
  graph_region_pc_t(T_PC const& entry, list<dshared_ptr<T_E const>> const& incoming_edges, list<dshared_ptr<T_E const>> const& outgoing_edges) : m_entry(entry), m_incoming_edges(incoming_edges), m_outgoing_edges(outgoing_edges)
  { }
  list<dshared_ptr<T_E const>> const& region_pc_get_incoming_edges() const { return m_incoming_edges; }
  list<dshared_ptr<T_E const>> const& region_pc_get_outgoing_edges() const { return m_outgoing_edges; }
  T_PC const& region_pc_get_entry_pc() const { return m_entry; }
  bool is_start() const { return m_entry.is_start(); }
  bool operator<(graph_region_pc_t const& other) const
  {
    return this->m_entry < other.m_entry;
  }
  bool operator==(graph_region_pc_t const& other) const
  {
    return this->m_entry == other.m_entry;
  }
  string to_string() const
  {
    string ret = m_entry.to_string();
    ret += "[ in:";
    for (auto const& i : m_incoming_edges) {
      ret += " " + i->get_from_pc().to_string() + "=>" + i->get_to_pc().to_string() + ";";
    }
    ret += "out:";
    for (auto const& o : m_outgoing_edges) {
      ret += " " + o->get_from_pc().to_string() + "=>" + o->get_to_pc().to_string() + ";";
    }
    ret += " ]";
    return ret;
  }
};

template<typename T_PC, typename T_N, typename T_E>
class graph_region_node_t : public node<graph_region_pc_t<T_PC, T_E>> {
private:
  graph_region_t<T_PC, T_N, T_E> m_region;
public:
  graph_region_node_t(graph_region_pc_t<T_PC, T_E> const& p, graph_region_t<T_PC, T_N, T_E> const& r) : node<graph_region_pc_t<T_PC, T_E>>(p), m_region(r)
  { }
  void graph_region_node_print(ostream& os, string const& prefix) const;
  graph_region_t<T_PC, T_N, T_E> const& get_region() const { return m_region; }

  list<T_PC> graph_region_node_get_constituent_pcs_among_these_pcs(list<T_PC> const& pcs) const
  {
    set<T_PC> constituent_pcs = this->graph_region_node_get_all_constituent_pcs();
    list<T_PC> n_pcs;
    for (auto const& p : pcs) {
      if (set_belongs(constituent_pcs, p)) {
        n_pcs.push_back(p);
      }
    }
    return n_pcs;
  }

  set<T_PC> graph_region_node_get_all_constituent_pcs() const
  {
    if (m_region.get_type() == graph_region_type_instruction) {
      return { m_region.get_leaf()->get_pc() };
    } else {
      set<T_PC> ret;
      for (auto const& gn : m_region.get_region_graph()->get_nodes()) {
        set_union(ret, gn->graph_region_node_get_all_constituent_pcs());
      }
      return ret;
    }
  }
};

template<typename T_PC, typename T_N, typename T_E>
class graph_region_edge_t : public edge<graph_region_pc_t<T_PC, T_E>> {
private:
  list<dshared_ptr<T_E const>> m_constituent_edges;
public:
  graph_region_edge_t(graph_region_pc_t<T_PC, T_E> const& from, graph_region_pc_t<T_PC, T_E> const& to, list<dshared_ptr<T_E const>> const& edges) : edge<graph_region_pc_t<T_PC, T_E>>(from, to), m_constituent_edges(edges)
  { }
  list<dshared_ptr<T_E const>> const& get_constituent_edges() const
  {
    return m_constituent_edges;
  }
  dshared_ptr<graph_region_edge_t<T_PC, T_N, T_E> const> add_constituent_edge(dshared_ptr<T_E const> const& e) const
  {
    dshared_ptr<graph_region_edge_t<T_PC, T_N, T_E>> new_e = make_dshared<graph_region_edge_t<T_PC, T_N, T_E>>(*this);
    if (!list_contains(new_e->m_constituent_edges, e)) {
      new_e->m_constituent_edges.push_back(e);
    }
    return new_e;
  }
};

template<typename T_PC, typename T_N, typename T_E>
class graph_region_t {
private:
  graph_region_type_t m_type;
  dshared_ptr<graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>>> m_region_graph;
  dshared_ptr<T_N> m_leaf_node;
public:
  graph_region_t() {}
  graph_region_t(graph_region_type_t typ, dshared_ptr<T_N> const& node) : m_type(typ), m_leaf_node(node)
  { }
  graph_region_t(graph_region_type_t typ, dshared_ptr<graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N, T_E>, graph_region_edge_t<T_PC, T_N, T_E>>> const& g) : m_type(typ), m_region_graph(g)
  { }
  dshared_ptr<graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>>> const&
  get_region_graph() const
  {
    return m_region_graph;
  }
  bool region_graph_has_cycle() const;
  dshared_ptr<T_N> get_leaf() const { return m_leaf_node; }
  graph_region_type_t get_type() const { return m_type; }
  set<shared_ptr<T_E const>> graph_region_get_all_atomic_constituent_edges() const;
  static string region_type_to_string(graph_region_type_t typ);
};

}
