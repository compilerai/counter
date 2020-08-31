#pragma once
#include <map>
#include <string>

#include "graph/expr_group.h"

namespace eqspace {

using namespace std;

class eqclasses_t
{
private:
  list<expr_group_ref> m_classes;

  bool m_is_managed;
public:
  ~eqclasses_t();
  list<expr_group_ref> const &get_expr_group_ls() const
  { return m_classes; }
  bool operator==(eqclasses_t const &eqc) const { return m_classes == eqc.m_classes; }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  //void eqclasses_to_stream(ostream& os) const;
  static manager<eqclasses_t> *get_eqclasses_manager();
  friend shared_ptr<eqclasses_t const> mk_eqclasses(list<expr_group_ref> const &expr_group_ls);
private:
  eqclasses_t() : m_is_managed(false)
  { }
  eqclasses_t(list<expr_group_ref> const &classes) : m_classes(classes), m_is_managed(false)
  { }
};

using eqclasses_ref = shared_ptr<eqclasses_t const>;
eqclasses_ref mk_eqclasses(list<expr_group_ref> const &expr_group_ls);

}

namespace std
{
using namespace eqspace;
template<>
struct hash<eqclasses_t>
{
  std::size_t operator()(eqclasses_t const &eqc) const
  {
    std::size_t seed = 0;
    std::hash<expr_group_t> h;
    for (auto const& eg : eqc.get_expr_group_ls()) {
      myhash_combine<size_t>(seed, h(*eg));
    }
    return seed;
  }
};
}
