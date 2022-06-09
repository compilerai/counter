#ifndef EDGE_ID_WITH_UNROLL_H
#define EDGE_ID_WITH_UNROLL_H

#include <string>
#include <memory>

#include "gsupport/edge_id.h"
#include "gsupport/edge_with_unroll.h"

template<typename T> class dshared_ptr;
template<class T, class... Args>
dshared_ptr<T> make_dshared(Args&&... args);

namespace eqspace {
using namespace std;

template<typename T_PC>
class edge_id_with_unroll_t : public edge_id_t<T_PC> {
private:
  int m_unroll;
public:
  //edge_id_with_unroll_t(edge_id_t<T_PC> const& eid) : edge_id_t<T_PC>(eid), m_unroll(EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN)
  //{ }

  edge_id_with_unroll_t(T_PC const& from_pc, T_PC const& to_pc, int unroll) : edge_id_t<T_PC>(from_pc, to_pc), m_unroll(unroll)
  { }

  edge_id_with_unroll_t(T_PC const& from_pc, T_PC const& to_pc, bool is_empty, int unroll) : edge_id_t<T_PC>(from_pc, to_pc, is_empty), m_unroll(unroll)
  { }

  int edge_id_with_unroll_get_unroll() const
  {
    return m_unroll;
  }

  string edge_id_with_unroll_to_string() const
  {
    return this->edge_id_to_string() + "#" + int_to_string(m_unroll);
  }
  static dshared_ptr<edge_id_with_unroll_t const> empty() { return make_dshared<edge_id_with_unroll_t const>(T_PC::start(), T_PC::start(), true, EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN); }
  static shared_ptr<edge_id_with_unroll_t const> empty(T_PC const&) { NOT_REACHED(); }
};

}

namespace std
{
using namespace eqspace;
template<typename T_PC>
struct hash<edge_id_with_unroll_t<T_PC>>
{
  std::size_t operator()(edge_id_with_unroll_t<T_PC> const &e) const
  {
    return hash<edge_id_t<T_PC>>(e);
  }
};

}

#endif
