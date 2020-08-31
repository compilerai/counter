#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/fail.h>
#include <caml/callback.h>
#include <caml/custom.h>
#include <caml/intext.h>
#include <inttypes.h>
#include <stdio.h>
#include <map>

#include "i386/insn.h"
#include "ppc/insn.h"
#include "codegen/etfg_insn.h"
#include "support/timers.h"
#include "support/stopwatch.h"
#include "i386/regs.h"
#include "ppc/regs.h"
#include "sym_exec/sym_exec.h"
#include "valtag/nextpc.h"
#include "rewrite/insn_list.h"

static char as1[40960];

#include <vector>
#include <set>
#include <stack>
//#include "eqcheck/eqcheck.h"
#include "valtag/regset.h"
#include "rewrite/transmap.h"
#include "expr/expr.h"
#include "support/debug.h"


#define MAX_ARGSLEN 655360

using namespace std;

//context* arg_to_context(value v)
//{
//  context* ctx = *((context**) Bp_val(v));
//  set_context(ctx);
//  return ctx;
//}

//int arg_to_int(value v)
//{
//  return Int_val(v);
//}

//int arg_to_bool(value v)
//{
//  return Bool_val(v);
//}

//pc arg_to_pc(value v)
//{
//  pc::pc_type type;
//  switch(Int_val(Field(v, 0)))
//  {
//  case 1: type = pc::insn_label; break;
//  case 2: type = pc::exit; break;
//  default: assert(0);
//  }
//  int index = Int_val(Field(v, 1));
//  return pc(type, index);
//}

//expr_ref arg_to_expr(value v)
//{
//  expr* e  = *((expr**) Bp_val(v));
//  return expr_ref(e)->simplify();
//}

//vector<expr_ref> array_value_to_expr_vector(value arr)
//{
//  int length = Wosize_val(arr);
//  vector<expr_ref> res;
//  for(int i = 0; i < length; ++i)
//  {
//    expr_ref e = arg_to_expr(Field(arr, i));
//    res.push_back(e);
//  }
//  return res;
//}

//list<expr_ref> array_value_to_expr_list(value arr)
//{
//  int length = Wosize_val(arr);
//  list<expr_ref> res;
//  for(int i = 0; i < length; ++i)
//  {
//    expr_ref e = arg_to_expr(Field(arr, i));
//    res.push_back(e);
//  }
//  return res;
//}

//vector<vector<expr_ref> > array_array_value_to_expr_vector_vector(value arr_arr)
//{
//  int length = Wosize_val(arr_arr);
//  vector<vector<expr_ref> > vec_vec;
//  for(int i = 0; i < length; ++i)
//  {
//    vec_vec.push_back(array_value_to_expr_vector(Field(arr_arr, i)));
//  }
//  return vec_vec;
//}

//state* arg_to_state(value v, bool src)
//{
//  state* s = new state(src);
//  s->m_mexvregs = array_array_value_to_expr_vector_vector(Field(v, 1));
//  s->m_memory = arg_to_expr(Field(v, 6));
//  s->m_stack = arg_to_expr(Field(v, 9));
//  return s;
//}

//vector<pair<pc, condition_list> > arg_to_nextpc_constraints(value v)
//{
//  vector<pair<pc, condition_list > > res;
//  int count = Wosize_val(v);
//  for(int i = 0; i < count; ++i)
//  {
//    value nextpc_value = Field(v, i);
//    pc p = arg_to_pc(Field(nextpc_value, 0));
//    expr_list exp_constraints = array_value_to_expr_list(Field(nextpc_value, 1));
//    res.push_back(make_pair(p, expr_list_to_conditions(exp_constraints, pdt_graph::path(), condition::exit_live_reg)));
//  }
//  return res;
//}

//trans_fun_graph* arg_to_trans_func_graph(value v, eqcheck* eq, bool src)
//{
//  trans_fun_graph* tfg = new trans_fun_graph(eq);
//  int nodes = Wosize_val(v);
//  for(int i = 0; i < nodes; ++i)
//  {
//    value node_value = Field(v, i);
//    pc pc_from = arg_to_pc(Field(node_value, 0));
//    state* state_from = arg_to_state(Field(node_value, 1), src);
//    trans_fun_graph::node* n = new trans_fun_graph::node(pc_from, *state_from);
//    delete state_from;
//    tfg->add_node(n);

//    value val_edges = Field(node_value, 2);
//    int edge_count = Wosize_val(val_edges);
//    for(int i = 0; i < edge_count; ++i)
//    {
//      value val_edge = Field(val_edges, i);
//      pc pc_to = arg_to_pc(Field(val_edge, 0));
//      state* state_to = arg_to_state(Field(val_edge, 1), src);
//      expr_ref predicate = arg_to_expr(Field(val_edge, 2));
//      trans_fun_graph::edge* e = new trans_fun_graph::edge(pc_from, pc_to, *state_to, predicate);
//      delete state_to;
//      e->add_edge_for_compounding(e);
//      tfg->add_edge(e);
//    }
//  }
//  return tfg;
//}

static void
caml_store_int(value args, int off, int val)
{
  CAMLparam1(args);
  ASSERT(val < (1<<16));
  ASSERT(off < MAX_ARGSLEN);
  Store_field(args, off, Val_int(val));
  CAMLreturn0;
}

static int
caml_store_int64(value args, int off, int64_t val)
{
  CAMLparam1(args);
  DBG(CAMLARGS, "storing at %d, %" PRId64 " = %d:%d:%d:%d\n", off, val,
      (int)((val >> 48) & 0xffff), (int)((val >> 32) & 0xffff),
      (int)((val >> 16) & 0xffff), (int)(val & 0xffff));
  caml_store_int(args, off, val & 0xffff);
  caml_store_int(args, off + 1, (val >> 16) & 0xffff);
  caml_store_int(args, off + 2, (val >> 32) & 0xffff);
  caml_store_int(args, off + 3, (val >> 48) & 0xffff);
  CAMLreturnT(int, 4);
}

/*static int
i386_insn_arg(i386_insn_t const *insn, value args, int off)
{
  CAMLparam1(args);
  int64_t arr[MAX_ARGSLEN];
  int i, len, start = off;
  len = i386_insn_to_int_array(insn, arr, MAX_ARGSLEN);
  for (i = 0; i < len; i++) {
    off += caml_store_int64(args, off, arr[i]);
  }
  CAMLreturnT(int, off - start);
}

static int
src_insn_arg(src_insn_t const *insn, value args, int off)
{
  CAMLparam1(args);
  int64_t *arr;
  int i, len, start = off;
  arr = (int64_t *)malloc(MAX_ARGSLEN * sizeof(int64_t));
  ASSERT(arr);
  len = src_insn_to_int_array(insn, arr, MAX_ARGSLEN);
  for (i = 0; i < len; i++) {
    off += caml_store_int64(args, off, arr[i]);
  }
  free(arr);
  CAMLreturnT(int, off - start);
}*/

static int
regset_arg(regset_t const *regset, value args, int off)
{
  CAMLparam1(args);
  int64_t *arr;
  int i, len, start = off;

  arr = (int64_t *)malloc(MAX_ARGSLEN * sizeof(int64_t));
  len = regset_to_int_array(regset, arr, MAX_ARGSLEN);
  for (i = 0; i < len; i++) {
    off += caml_store_int64(args, off, arr[i]);
  }
  free(arr);
  CAMLreturnT(int, off - start);
}

/*static int
transmap_arg(transmap_t const *tmap, value args, int off)
{
  CAMLparam1(args);
  int64_t *arr;
  int i, len, start = off;

  arr = (int64_t *)malloc(MAX_ARGSLEN * sizeof(int64_t));
  len = transmap_to_int_array(tmap, arr, MAX_ARGSLEN);
  for (i = 0; i < len; i++) {
    off += caml_store_int64(args, off, arr[i]);
  }
  free(arr);
  CAMLreturnT(int, off - start);
}*/

//static int
//i386_regset_arg(i386_regset_t const *regset, value args, int off)
//{
//  CAMLparam1(args);
//  int64_t arr[MAX_ARGSLEN];
//  int i, len, start = off;
//  len = i386_regset_to_int_array(regset, arr, MAX_ARGSLEN);
//  for (i = 0; i < len; i++) {
//    off += caml_store_int64(args, off, arr[i]);
//  }
//  CAMLreturnT(int, off - start);
//}




bool eqcheck_check_equivalence(value args)
{
//  stopwatch_run(&eqcheck_logging_timer);
//  cout << "eqcheck_start\n";
//  context* ctx = arg_to_context(Field(args, 0));
//  stopwatch_run(&eqcheck_preprocessing_step_timer);
//  eqcheck eq(ctx);

//  trans_fun_graph* src_tfg = arg_to_trans_func_graph(Field(args, 1), &eq, true);
//  trans_fun_graph* dst_tfg = arg_to_trans_func_graph(Field(args, 2), &eq, false);
//  vector<pair<pc, condition_list> > exit_constraints =
//      arg_to_nextpc_constraints(Field(args, 3));

//  srand(time(0));

//  eq.set_src_tfg(src_tfg);
//  eq.set_dst_tfg(dst_tfg);
//  eq.set_exit_constraints(exit_constraints);
//  stopwatch_stop(&eqcheck_preprocessing_step_timer);

//  cout << "calling check_equivalence\n";
//  stopwatch_run(&eqcheck_check_equivalence_timer);
//  bool res = eq.check_equivalence();
//  eq.print_stats();
//  stopwatch_stop(&eqcheck_check_equivalence_timer);
//  stopwatch_stop(&eqcheck_logging_timer);
//  cout << res << endl;
//  return res;
  return false;
}

char *
get_consts_struct_str()
{
//  CAMLparam0();
  value args, str;
  static value *mlfunc = NULL;
  int i, ncur, alen;
  char* ret;
  if (!mlfunc) {
    mlfunc = caml_named_value(__func__);
    ASSERT(mlfunc);
  }
  args = alloc(0, 0);

  str = callback(*mlfunc, args);
  ret = strdup(String_val(str));
  return ret;
}

#define CMN COMMON_SRC
#include "cmn/cmn_get_transfer_state_str.h"
#undef CMN

#define CMN COMMON_DST
#include "cmn/cmn_get_transfer_state_str.h"
#undef CMN

void
gen_consts_db(char const *out_filename)
{
  char *str;
  FILE *fp;

  fp = fopen(out_filename, "w");
  ASSERT(fp);
  str = get_consts_struct_str();
  fputs(str, fp);
  free(str);
  fclose(fp);
}

#define CMN COMMON_SRC
#include "cmn/cmn_sym_exec_db.h"
#undef CMN

#define CMN COMMON_DST
#include "cmn/cmn_sym_exec_db.h"
#undef CMN
