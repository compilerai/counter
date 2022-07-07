#pragma once

#include "support/manager.h"
#include "support/mytimer.h"
#include "support/utils.h"
#include "support/debug.h"
#include "support/types.h"
#include "support/serpar_composition.h"

#include "gsupport/edge_id.h"
#include "gsupport/graph_ec.h"
#include "gsupport/edge_with_graph_ec.h"

namespace eqspace {

template <typename T_PC, typename T_N, typename T_E> class graph;
template <typename T_PC, typename T_E> class graph_ec_unroll_t;

using namespace std;

template <typename T_PC>
class node
{
public:
  node(const T_PC& p) : m_pc(p)
  {
/*#ifndef NDEBUG
    m_num_allocated_node++;
#endif*/
  }
  node(node<T_PC> const &other) : m_pc(other.m_pc)
  {
/*#ifndef NDEBUG
    m_num_allocated_node++;
#endif*/
  }

  virtual ~node()
  {
/*#ifndef NDEBUG
    m_num_allocated_node--;
#endif*/
  }

  const T_PC& get_pc() const
  {
    return m_pc;
  }

  void set_pc(T_PC const &p)
  {
    m_pc = p;
  }

  virtual string to_string() const
  {
    return m_pc.to_string();
  }

  friend ostream& operator<<(ostream& os, const node& n)
  {
    os << n.to_string();
    return os;
  }

/*#ifndef NDEBUG
  static long long get_num_allocated()
  {
    return m_num_allocated_node;
  }
#endif*/

private:
  T_PC m_pc;

/*#ifndef NDEBUG
  static long long m_num_allocated_node;
#endif*/
};

}
