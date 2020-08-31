#include "graph/point.h"

namespace eqspace {
manager<point_t> *
point_t::get_point_manager()
{
  static manager<point_t> *ret = NULL;
  if (!ret) {
    ret = new manager<point_t>;
  }
  return ret;
}

point_ref
mk_point(vector<point_val_t> const &vals)
{
  point_t p(vals);
  return point_t::get_point_manager()->register_object(p);
}

point_t::~point_t()
{
  if (m_is_managed) {
    this->get_point_manager()->deregister_object(*this);
  }
}

}
