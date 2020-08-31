#include "support/debug.h"
#include "graph/farg_type.h"
#include "expr/expr.h"
#include "expr/context.h"
#include "support/consts.h"
#include "support/log.h"
#include "support/types.h"
#include "expr/consts_struct.h"
#include "expr/expr_utils.h"
#include <vector>

using namespace std;

vector<expr_ref>
fsignature_t::get_dst_fcall_args_for_fsignature(fsignature_t const& fsignature, expr_ref const& mem, expr_ref const& stackpointer, string const& graph_name, long& max_memlabel_varnum, expr_ref& dst_fcall_retval_buffer)
{
  context *ctx = mem->get_context();
  ASSERT(stackpointer->is_bv_sort());
  ASSERT(stackpointer->get_sort()->get_size() == DWORD_LEN);

  vector<expr_ref> ret;
#if CALLING_CONVENTIONS == CDECL
  mlvarname_t mlvarname = ctx->memlabel_varname(graph_name, max_memlabel_varnum);
  max_memlabel_varnum++;
  expr_ref cur_stackpos = stackpointer;
  vector<farg_t> const& fargs = fsignature.get_args();
  vector<farg_t> const& fretvals = fsignature.get_retvals();

  if (   fretvals.size() > 1
      || (fretvals.size() == 1 && fretvals.at(0).get_size() > QWORD_LEN)) {
    size_t nbytes = DIV_ROUND_UP(stackpointer->get_sort()->get_size(), DWORD_LEN) * DWORD_LEN/BYTE_LEN;
    expr_ref stackslot = ctx->mk_select(mem, mlvarname, cur_stackpos, nbytes, false);
    cur_stackpos = ctx->mk_bvadd(cur_stackpos, ctx->mk_bv_const(stackpointer->get_sort()->get_size(), nbytes));
    cur_stackpos = ctx->expr_do_simplify(cur_stackpos);
    dst_fcall_retval_buffer = stackslot;
  }

  for (auto const& farg : fargs) {
    size_t nbytes_alloc = DIV_ROUND_UP(farg.get_size(), DWORD_LEN) * DWORD_LEN/BYTE_LEN;
    size_t nbytes = DIV_ROUND_UP(farg.get_size(), BYTE_LEN);
    expr_ref stackslot = ctx->mk_select(mem, mlvarname, cur_stackpos, nbytes, false);
    cur_stackpos = ctx->mk_bvadd(cur_stackpos, ctx->mk_bv_const(stackpointer->get_sort()->get_size(), nbytes_alloc));
    cur_stackpos = ctx->expr_do_simplify(cur_stackpos);
    ret.push_back(stackslot);
  }
#else
  NOT_IMPLEMENTED();
#endif
  return ret;
}
