#include "graph/locset.h"
#include "support/manager.h"

namespace eqspace {

locset_ref
mk_locset(set<graph_loc_id_t> const &locs)
{
  locset_t locset(locs);
  return locset_t::get_locset_manager()->register_object(locset);

}

locset_t::~locset_t()
{
  if (m_is_managed) {
    this->get_locset_manager()->deregister_object(*this);
  }
}

manager<locset_t> *
locset_t::get_locset_manager()
{
  static manager<locset_t> *ret = NULL;
  if (!ret) {
    ret = new manager<locset_t>;
  }
  return ret;
}

}
