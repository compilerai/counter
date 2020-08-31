#ifndef EDGE_ID_H
#define EDGE_ID_H

#include <string>
#include "gsupport/pc.h"

namespace eqspace {
using namespace std;

template<typename T_PC>
class edge_id_t {
public:
  edge_id_t() {}
  edge_id_t(T_PC const &from, T_PC const &to) : m_from(from), m_to(to), m_is_empty(false) {}
  edge_id_t(T_PC const &from, T_PC const &to, bool is_empty) : m_from(from), m_to(to), m_is_empty(is_empty) {}
  static edge_id_t empty() { return edge_id_t(T_PC::start(), T_PC::start(), true); }
  bool is_empty() const { return (m_is_empty == true); }
  bool operator<(edge_id_t const &other) const
  {
    return    m_from < other.m_from
           || (m_from == other.m_from && m_to < other.m_to);
  }
  bool operator==(edge_id_t const &other) const
  {
    return m_from == other.m_from && m_to == other.m_to && m_is_empty == m_is_empty;
  }
  bool operator!=(edge_id_t const &other) const { return !(*this == other); }
  T_PC const &get_from_pc() const { return m_from; }
  T_PC const &get_to_pc() const { return m_to; }
  string edge_id_to_string() const { return m_from.to_string() + "=>" + m_to.to_string(); }
  string to_string_concise() const { return this->edge_id_to_string(); }
private:
  T_PC m_from, m_to;
  bool m_is_empty;
};

}

namespace std
{
using namespace eqspace;
template<typename T_PC>
struct hash<edge_id_t<T_PC>>
{
  std::size_t operator()(edge_id_t<T_PC> const &e) const
  {
    std::hash<T_PC> pc_hasher;
    std::size_t seed = 0;
    myhash_combine<size_t>(seed, pc_hasher(e.get_from_pc()));
    myhash_combine<size_t>(seed, pc_hasher(e.get_to_pc()));
    return seed;
  }
};

}

#endif
