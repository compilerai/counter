#pragma once

#include "support/types.h"
#include "expr/expr.h"

namespace eqspace {

using namespace std;

class graph_local_t
{
private:
  string_ref m_name;
  size_t     m_size;
  align_t    m_align;
  bool       m_varsize;
public:
  graph_local_t(string const& name, size_t size, align_t align, bool varsize = false) : graph_local_t(mk_string_ref(name), size, align, varsize)
  { }
  graph_local_t(string_ref const& name, size_t size, align_t align, bool varsize = false) : m_name(name), m_size(size), m_align(align), m_varsize(varsize)
  { }
  string_ref const& get_name() const { return m_name; }
  align_t get_alignment() const { return m_align; }
  size_t get_size() const { ASSERT(!m_varsize); return m_size; }
  bool is_varsize() const { return m_varsize; }

  static graph_local_t vararg_local() { return graph_local_t(G_SRC_KEYWORD "." VARARG_LOCAL_VARNAME, -1, ALIGNMENT_FOR_VARARG_LOCAL, true); }
};

}
