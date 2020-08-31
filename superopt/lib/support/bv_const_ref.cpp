#include "support/bv_const_ref.h"

namespace eqspace
{
bv_const_ref
mk_bv_const_ref(bv_const const &bvc)
{
  bv_const_obj_t bvc_obj(bvc);
  return bv_const_obj_t::get_bv_const_obj_manager()->register_object(bvc_obj);
}

bv_const_obj_t::~bv_const_obj_t()
{
  if (m_is_managed) {
    this->get_bv_const_obj_manager()->deregister_object(*this);
  }
}

manager<bv_const_obj_t> *
bv_const_obj_t::get_bv_const_obj_manager()
{
  static manager<bv_const_obj_t> *ret = NULL;
  if (!ret) {
    ret = new manager<bv_const_obj_t>;
  }
  return ret;
}
}
