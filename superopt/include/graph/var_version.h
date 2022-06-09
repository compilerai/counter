#pragma once

#include "expr/expr.h"

namespace eqspace {
using namespace std;

class var_version_t {
private:
  map<string_ref, expr_ref> m_ver_map;

public:
  var_version_t()
  { }
  
  var_version_t(map<string_ref, expr_ref> ver_map) : m_ver_map(ver_map)
  { }

  var_version_t(var_version_t  const& other) : m_ver_map(other.m_ver_map)
  { }

  var_version_t(istream& is, context* ctx);
  void var_version_to_stream(ostream& os) const;

  bool is_top() const { 
    return (m_ver_map.size() == 0); 
  }

  static var_version_t top()
  {
    var_version_t ret;
    return ret;
  }

  bool operator==(var_version_t const& other) const {
    return (this->m_ver_map == other.m_ver_map);
  }

  map<string_ref, expr_ref> const& get_ver_map() const {
    return m_ver_map;
  }

  expr_ref get_ver_for_input_varname(string_ref const& s) const { 
    if(!m_ver_map.count(s)) {
      cout << "Input string: " << s->get_str() << endl;
      cout << "Var version map " << endl;
      for(auto const& m : m_ver_map) 
        cout << m.first->get_str() << ": " << expr_string(m.second) << endl;
    }
    ASSERT(m_ver_map.count(s)); 
    return m_ver_map.at(s); 
  }
  void set_ver_for_varname(string_ref const& s, expr_ref const& e) { ASSERT(!m_ver_map.count(s) || (m_ver_map.at(s) == e)); m_ver_map[s] = e; }
  
};

}
