#pragma once
#include <map>
#include <string>

#include "support/manager.h"
#include "support/debug.h"
#include "support/utils.h"
#include "support/bv_const.h"

using namespace std;

namespace eqspace
{

class bv_const_obj_t
{
private:
  bv_const m_bv;

  bool m_is_managed;
public:
  ~bv_const_obj_t();
  bv_const const &get_bv() const { return m_bv; }
  bool operator==(bv_const_obj_t const &other) const { return m_bv == other.m_bv; }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<bv_const_obj_t> *get_bv_const_obj_manager();
  friend shared_ptr<bv_const_obj_t const> mk_bv_const_ref(bv_const const &bvc);
private:
  bv_const_obj_t() : m_is_managed(false)
  { }
  bv_const_obj_t(bv_const const &bvc) : m_bv(bvc), m_is_managed(false)
  { }
};

using bv_const_ref = shared_ptr<bv_const_obj_t const>;

bv_const_ref mk_bv_const_ref(bv_const const &bvc);
}

namespace std
{

using namespace eqspace;

template<>
struct hash<bv_const_obj_t>
{
  std::size_t operator()(bv_const_obj_t const &s) const
  {
    std::size_t seed = 0;
    bv_const const &bvc= s.get_bv();
    myhash_combine<size_t>(seed, bvc.get_ui());
    return seed;
  }
};
}

