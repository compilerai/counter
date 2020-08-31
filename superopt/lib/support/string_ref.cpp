#include "support/string_ref.h"
#include "support/manager.h"
#include "support/utils.h"

string_ref
mk_string_ref(string const &s)
{
  string_obj_t string_obj(s);
  return string_obj_t::get_string_obj_manager()->register_object(string_obj);

}

string_obj_t::~string_obj_t()
{
  if (m_is_managed) {
    this->get_string_obj_manager()->deregister_object(*this);
  }
}

manager<string_obj_t> *
string_obj_t::get_string_obj_manager()
{
  static manager<string_obj_t> *ret = NULL;
  if (!ret) {
    ret = new manager<string_obj_t>;
  }
  return ret;
}

size_t
hash<string_obj_t>::operator()(string_obj_t const& s) const
{
  std::size_t seed = 0;
  string const &str = s.get_str();
  myhash_combine<string>(seed, str);
  return seed;
}
