#pragma once

#include "support/consts.h"
#include "support/str_utils.h"
#include "support/string_ref.h"

namespace eqspace {

class mlvarname_t {
private:
  string_ref m_str;
public:
  mlvarname_t() : m_str()
  { }
  mlvarname_t(string_ref const& str) : m_str(str)
  { }
  mlvarname_t(string const& str) : m_str(mk_string_ref(str))
  { }
  string_ref const& mlvar_get_str_ref() const { return m_str; }
  string const& mlvar_get_str() const { return m_str->get_str(); }
  bool operator<(mlvarname_t const& other) const
  { return m_str < other.m_str; }
};

mlvarname_t memlabel_varname(string const &graph_name, int varnum);
mlvarname_t memlabel_varname_for_fcall(string const &graph_name, int varnum);
mlvarname_t memlabel_varname_for_loc(int varnum);

bool mlvarname_refers_to_fcall(mlvarname_t const& vname, string const& graph_name);
bool mlvarname_refers_to_loc(mlvarname_t const& vname, string const& graph_name);

}
