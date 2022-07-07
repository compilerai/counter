#pragma once

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E>
class graph_bbl_t {
private:
  T_PC m_head;
  list<T_PC> m_instructions;
public:
  graph_bbl_t(T_PC const& head, list<T_PC> const& instructions = {}) : m_head(head), m_instructions(instructions)
  { }
  T_PC const& get_head() const { return m_head; }
  list<T_PC> const& get_instructions() const { return m_instructions; }
  bool operator<(graph_bbl_t<T_PC, T_N, T_E> const& other) const
  {
    return this->m_head < other.m_head;
  }
};

}
