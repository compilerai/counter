#pragma once

#include "support/debug.h"
#include "support/timers.h"
#include "support/types.h"

#include "gsupport/tfg_edge.h"

namespace eqspace {

class tfg_edge_set_t;
using tfg_edge_set_ref = shared_ptr<tfg_edge_set_t const>;

class tfg_edge_set_t
{
private:
  set<tfg_edge_ref> m_set;
  bool m_is_managed = false;
public:
  tfg_edge_set_t() = delete;
  ~tfg_edge_set_t();

  set<tfg_edge_ref> const& get_set() const
  {
    return m_set;
  }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }

  bool operator==(tfg_edge_set_t const &other) const
  {
    return m_set == other.m_set;
  }

  static manager<tfg_edge_set_t> *get_tfg_edge_set_manager();
  string tfg_edge_set_to_string() const;

  static shared_ptr<tfg_edge_set_t const> mk_tfg_edge_set(set<tfg_edge_ref> const& s);
private:
  tfg_edge_set_t(set<tfg_edge_ref> const& s) : m_set(s)
  { }
};

}

namespace std
{
using namespace eqspace;
template<>
struct hash<tfg_edge_set_t>
{
  std::size_t operator()(tfg_edge_set_t const &s) const
  {
    std::size_t seed = s.get_set().size();
    for (auto const& te : s.get_set()) {
      myhash_combine<size_t>(seed, (size_t)te.get());
    }
    return seed;
  }
};
}
