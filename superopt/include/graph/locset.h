#pragma once

#include "support/types.h"
#include "support/manager.h"

namespace eqspace {

using namespace std;

class locset_t
{
private:
  set<graph_loc_id_t> m_locs;

  bool m_is_managed;
public:
  ~locset_t();
  set<graph_loc_id_t> const &get_locs() const
  { return m_locs; }
  bool operator==(locset_t const &other) const { return m_locs == other.m_locs; }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    assert(!m_is_managed);
    m_is_managed = true;
  }
  static manager<locset_t> *get_locset_manager();
  friend shared_ptr<locset_t const> mk_locset(set<graph_loc_id_t> const &locs);
private:
  locset_t() : m_is_managed(false)
  { }
  locset_t(set<graph_loc_id_t> const &locs) : m_locs(locs), m_is_managed(false)
  { }
};

using locset_ref = shared_ptr<locset_t const>;
locset_ref mk_locset(set<graph_loc_id_t> const &locs);

}

namespace std
{
using namespace eqspace;
template<>
struct hash<locset_t>
{
  std::size_t operator()(locset_t const &lset) const
  {
    std::size_t seed = 0;
    for (auto const& loc_id: lset.get_locs()) {
      seed += 13 * loc_id;
    }
    return seed;
  }
};
}
