#include <sys/stat.h>
#include <sys/types.h>
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "expr/memlabel.h"
#include "expr/z3_solver.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/globals_cpp.h"
#include "codegen/etfg_insn.h"
#include "i386/insn.h"
#include "rewrite/src-insn.h"
#include "rewrite/dst-insn.h"
#include "rewrite/peep_entry_list.h"
#include "superopt/rand_states.h"
#include "support/timers.h"
#include "rewrite/peephole.h"
#include "sym_exec/sym_exec.h"
#include "equiv/bequiv.h"
#include "equiv/equiv.h"
#include "superopt/superopt.h"
#include "support/c_utils.h"
#include "superopt/itable.h"
#include "equiv/jtable.h"
#include "superopt/itable_stopping_cond.h"
#include "rewrite/peephole.h"
#include "support/cmd_helper.h"
#include "eq/eqcheck.h"
#include "eq/corr_graph.h"

#include <iostream>
#include <fstream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;

static char as1[409600];

int
main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> rand_states_filename(cl::implicit_prefix, "", "filename with rand states");
  cl::arg<string> typestate_filename(cl::implicit_prefix, "", "filename with typestates");
  cl::cl cmd("typestates_make_non_poly_copyable: transform typestates to make all non-poly type elements copyable");
  cmd.add_arg(&rand_states_filename);
  cmd.add_arg(&typestate_filename);
  cmd.parse(argc, argv);

  init_timers();
  cmd_helper_init();

  g_query_dir_init();

  /*g_helper_pid = */eqspace::solver_init();


  src_init();
  dst_init();

  g_ctx_init();
  g_se_init();
  context *ctx = g_ctx;

  usedef_init();
  types_init();
  equiv_init();

  cpu_set_log_filename(DEFAULT_LOGFILE);

  ifstream ifs;
  ifs.open(rand_states_filename.get_value());
  rand_states_t *rand_states = rand_states_t::read_rand_states(ifs);
  ifs.close();

  size_t num_states = rand_states->get_num_states();

  typestate_t *typestate_initial, *typestate_final;

  typestate_initial = new typestate_t[num_states];
  ASSERT(typestate_initial);
  for (size_t i = 0; i < num_states; i++) {
    typestate_init(&typestate_initial[i]);
  }
  typestate_final = new typestate_t[num_states];
  ASSERT(typestate_final);
  for (size_t i = 0; i < num_states; i++) {
    typestate_init(&typestate_final[i]);
  }
  typestate_from_file(typestate_initial, typestate_final, num_states, typestate_filename.get_value().c_str());

  typestates_make_non_poly_copyable(typestate_initial, num_states);

  emit_types_to_file(stdout, typestate_initial, typestate_final, num_states);
  solver_kill();
  call_on_exit_function();

  return 0;
}
