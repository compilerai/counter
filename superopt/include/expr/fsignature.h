#ifndef FSIGNATURE_H
#define FSIGNATURE_H

#include <vector>
#include <string>
#include "graph/farg_type.h"

class fsignature_t {
private:
  vector<farg_t> m_args;
  vector<farg_t> m_retvals;
public:
  fsignature_t()
  { }
  fsignature_t(std::vector<farg_t> const& args, std::vector<farg_t> const& retvals) : m_args(args), m_retvals(retvals)
  { }
  vector<farg_t> const& get_args() const { return m_args; }
  vector<farg_t> const& get_retvals() const { return m_retvals; }
  static std::vector<expr_ref> get_dst_fcall_args_for_fsignature(fsignature_t const& fsignature, expr_ref const& mem, expr_ref const& mem_alloc, expr_ref const& stackpointer, std::string const& graph_name, long& max_memlabel_varnum, expr_ref& dst_fcall_retval_buffer);
};

#endif
