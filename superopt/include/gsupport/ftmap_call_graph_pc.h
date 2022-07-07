#pragma once

#include "expr/context.h"
#include "expr/call_context.h"
#include "expr/pc.h"

namespace eqspace {

using namespace std;

class ftmap_call_graph_pc_t {
private:
  call_context_ref m_cc;
  pc m_pc;
public:
  ftmap_call_graph_pc_t() { }
  ftmap_call_graph_pc_t(call_context_ref const& cc, pc const& p) : m_cc(cc), m_pc(p)
  { }
  call_context_ref get_call_context() const { return m_cc; }
  pc get_pc() const { return m_pc; }
  bool operator==(ftmap_call_graph_pc_t const& other) const { return m_cc == other.m_cc && m_pc == other.m_pc; }
  bool operator!=(ftmap_call_graph_pc_t const& other) const { return m_cc != other.m_cc || m_pc != other.m_pc; }
  bool operator<(ftmap_call_graph_pc_t const& other) const { return m_cc < other.m_cc || (m_cc == other.m_cc && m_pc < other.m_pc); }
  bool is_start() const { return *this == start(); }
  bool is_exit() const { return false; }
  bool is_cloned_pc() const { return false; }
  bool is_label() const { NOT_IMPLEMENTED(); }
  static ftmap_call_graph_pc_t start() { return ftmap_call_graph_pc_t(call_context_t::empty_call_context(FTMAP_CALL_GRAPH_START_FNAME), pc::start()); }
  static ftmap_call_graph_pc_t create_from_string(string const& s) { NOT_IMPLEMENTED(); }
  string to_string() const
  {
    return m_cc->call_context_to_string() + FTMAP_CALL_GRAPH_PC_SEPARATOR + m_pc.to_string();
  }
  void set_bbl_index_for_graph_reduction(bbl_index_t bbl_index)
  {
    NOT_REACHED();
  }
  string get_string_for_pc_using_pc_string_map(map<ftmap_call_graph_pc_t, string> const& pc_string_map) const { return this->to_string(); }
};

}
