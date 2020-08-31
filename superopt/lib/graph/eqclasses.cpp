#include "graph/eqclasses.h"

namespace eqspace {

eqclasses_ref
mk_eqclasses(list<expr_group_ref> const &expr_group_ls)
{
  eqclasses_t eqc(expr_group_ls);
  return eqclasses_t::get_eqclasses_manager()->register_object(eqc);

}

eqclasses_t::~eqclasses_t()
{
  if (m_is_managed) {
    this->get_eqclasses_manager()->deregister_object(*this);
  }
}

manager<eqclasses_t> *
eqclasses_t::get_eqclasses_manager()
{
  static manager<eqclasses_t> *ret = NULL;
  if (!ret) {
    ret = new manager<eqclasses_t>;
  }
  return ret;
}

//void
//eqclasses_t::eqclasses_to_stream(ostream& os) const
//{
//  size_t i = 0;
//  for(auto const& eg : m_classes) {
//    os << "=expr_group" << i << "\n";
//    i++;
//    eg->expr_group_to_stream(os);
//  }
//}

}
