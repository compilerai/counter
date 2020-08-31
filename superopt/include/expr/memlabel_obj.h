#pragma once
#include <map>
#include <string>
#include <stdlib.h>
#include "cutils/chash.h"
#include "support/manager.h"
#include "support/debug.h"
#include "support/utils.h"
#include "expr/memlabel.h"

using namespace std;

namespace eqspace {

class memlabel_obj_t
{
private:
  memlabel_t m_ml;

  bool m_is_managed;
public:
  ~memlabel_obj_t();
  memlabel_t const &get_ml() const
  { return m_ml; }
  bool operator==(memlabel_obj_t const &other) const { return memlabel_t::memlabels_equal(&m_ml, &other.m_ml); }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<memlabel_obj_t> *get_memlabel_obj_manager();
  friend shared_ptr<memlabel_obj_t const> mk_memlabel_ref(memlabel_t const &ml);
private:
  memlabel_obj_t() : m_is_managed(false)
  { }
  memlabel_obj_t(memlabel_t const &ml) : m_ml(ml), m_is_managed(false)
  { }
};

memlabel_ref mk_memlabel_ref(memlabel_t const &s);

}

using namespace eqspace;

namespace std
{
template<>
struct hash<memlabel_obj_t>
{
  std::size_t operator()(memlabel_obj_t const &s) const
  {
    std::size_t seed = 0;
    memlabel_t const &ml= s.get_ml();
    myhash_combine<size_t>(seed, ml.get_alias_set_size());
    return seed;
  }
};
}
