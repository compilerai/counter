#include "support/debug.h"
#include "support/cmd_helper.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "expr/local_sprel_expr_guesses.h"
#include "rewrite/static_translate.h"
#include "etfg/etfg_insn.h"
#include "insn/src-insn.h"
#include "rewrite/alloc_constraints.h"
#include "support/timers.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "expr/z3_solver.h"

#include <fstream>

#include <iostream>
#include <string>

using namespace std;

int
main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> input_file(cl::implicit_prefix, "", "path to .alloc_constraints file");
  cl::cl cmd("Translate an etfg file to dst-arch (codegen)");
  cmd.add_arg(&input_file);
  cmd.parse(argc, argv);

  init_timers();
  cmd_helper_init();

  etfg_init();
  dst_init();

  g_ctx_init();

  //usedef_init();

  ifstream in(input_file.get_value());
  ASSERT(in.is_open());
  string infile_str((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
  in.close();

  alloc_constraints_t<exreg_id_obj_t> ac = alloc_constraints_t<exreg_id_obj_t>::alloc_constraints_from_string(infile_str);
  cout << __func__ << " " << __LINE__ << ": printing alloc constraints:\n" << ac.alloc_constraints_to_string() << endl;
  DBG_SET(STACK_SLOT_CONSTRAINTS, 1);
  map<prog_point_t, alloc_map_t<exreg_id_obj_t>> ssm;
  ac.solve(ssm);
  call_on_exit_function();
  return 0;
}
