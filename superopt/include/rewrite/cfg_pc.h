#pragma once
#include "support/utils.h"

class cfg_pc
{
public:
  cfg_pc() : m_bbl_index(0), m_orig_bbl_index(0) {}
  cfg_pc(long bbl_index) : m_bbl_index(bbl_index), m_orig_bbl_index(bbl_index) {}
  long get_bbl_index() const { return m_bbl_index; }
  long get_orig_bbl_index() const { return m_orig_bbl_index; }
  bool is_start() const { NOT_REACHED(); }
  bool is_exit() const { NOT_REACHED(); }
  string to_string() const
  {
    stringstream ss;
    ss << m_bbl_index << ":" << m_orig_bbl_index;
    return ss.str();
  }
  bool operator<(cfg_pc const &other) const
  {
    return m_bbl_index < other.m_bbl_index;
  }
  bool operator==(cfg_pc const &other) const
  {
    return m_bbl_index == other.m_bbl_index;
  }
  void set_bbl_index_for_graph_reduction(bbl_index_t bbl_index)
  {
    m_bbl_index = bbl_index;
  }
private:
  long m_bbl_index;
  long m_orig_bbl_index;
};


namespace std{
using namespace eqspace;
template<>
struct hash<cfg_pc>
{
  std::size_t operator()(cfg_pc const &p) const
  {
    size_t seed = 0;
    myhash_combine<std::size_t>(seed, p.get_bbl_index());
    myhash_combine<std::size_t>(seed, p.get_orig_bbl_index());
    return seed;
  }
};
}
