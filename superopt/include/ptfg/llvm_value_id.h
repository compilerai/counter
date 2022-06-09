#pragma once
#include "support/string_ref.h"
using namespace std;

namespace eqspace {

class llvm_value_id_t {
private:
  string_ref m_function;
  string_ref m_value_desc;
public:
  llvm_value_id_t(string const& f, string const& value_desc) : m_function(mk_string_ref(f)),
                                                               m_value_desc(mk_string_ref(value_desc))
  { }
  string const& get_function() const { return m_function->get_str(); }
  string const& get_value_desc() const { return m_value_desc->get_str(); }
  bool operator<(llvm_value_id_t const& other) const
  {
    if (m_function != other.m_function) {
      return m_function < other.m_function;
    }
    if (m_value_desc != other.m_value_desc) {
      return m_value_desc < other.m_value_desc;
    }
    return false;
  }
  string llvm_value_id_to_string() const
  {
    return m_function->get_str() + ":" + m_value_desc->get_str();
  }
};

}
