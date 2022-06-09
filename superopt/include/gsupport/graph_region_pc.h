#pragma once

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>

namespace eqspace {

using namespace std;

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

}
