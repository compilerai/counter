#pragma once

namespace eqspace {

template<typename T_PC>
class pc_with_unroll {
private:
  T_PC m_pc;
  int m_unroll;
public:
  pc_with_unroll() { }
  pc_with_unroll(T_PC const& p, int unroll = EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN) : m_pc(p), m_unroll(unroll)
  { }
  T_PC const& get_pc() const { return m_pc; }
  bool is_start() const { return m_pc.is_start(); }
  bool is_exit() const { return m_pc.is_exit(); }
  int get_unroll() const { return m_unroll; }
  bool operator<(pc_with_unroll<T_PC> const& other) const
  {
    return    (m_pc < other.m_pc)
           || (m_pc == other.m_pc && m_unroll < other.m_unroll);
  }
  bool operator==(pc_with_unroll<T_PC> const& other) const
  {
    return    m_pc == other.m_pc
           && m_unroll == other.m_unroll;
  }
  string to_string() const
  {
    stringstream ss;
    ss << "{" << m_pc.to_string() << "," << m_unroll << "}";
    return ss.str();
  }
};

}
