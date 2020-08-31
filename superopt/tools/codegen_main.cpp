#include "support/debug.h"
#include "support/cmd_helper.h"
#include "tfg/parse_input_eq_file.h"
#include "graph/prove.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "expr/local_sprel_expr_guesses.h"
#include "codegen/codegen.h"

#include "support/timers.h"
#include "support/utils.h"

#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "expr/z3_solver.h"

#include <fstream>

#include <iostream>
#include <string>

using namespace std;

static char as1[4096];

void dst_free();

int
main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> etfg_file(cl::implicit_prefix, "", "path to .etfg file");
  cl::arg<string> logfile(cl::explicit_prefix, "l", DEFAULT_LOGFILE, "path to log file");
  cl::arg<string> output_file(cl::explicit_prefix, "o", "a.out", "path to output object file");
  cl::arg<bool> dyn_debug_arg(cl::explicit_flag, "d", false, "generate dyn debug code in the executable");
  cl::arg<int> prune_row_size_arg(cl::explicit_prefix, "prune_row_size", prune_row_size, "size of prune row; default: 16");
  cl::cl cmd("Translate an etfg file to dst-arch (codegen)");
  cmd.add_arg(&etfg_file);
  cmd.add_arg(&output_file);
  cmd.add_arg(&logfile);
  cmd.add_arg(&dyn_debug_arg);
  cmd.add_arg(&prune_row_size_arg);
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=insn_match=2,smt_query=1");
  cmd.add_arg(&debug);
  cl::arg<string> dst_insn_disable_folding(cl::explicit_prefix, "no-fold", "", "Disable dst insn folding optimizations for specified functions; default: <empty-string>. Use 'ALL' for disabling folding on all functions.  Use comma-separated list of functions otherwise. e.g., --no-fold=main,fib,lists");
  cmd.add_arg(&dst_insn_disable_folding);
  cmd.parse(argc, argv);

  eqspace::init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, eqspace::print_debug_class_levels());

  vector<string> disable_folding_functions = splitOnChar(dst_insn_disable_folding.get_value(), ',');

  prune_row_size = prune_row_size_arg.get_value();
  init_timers();
  cmd_helper_init();
  g_query_dir_init();

  etfg_init();
  dst_init();

  g_ctx_init();

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);

  usedef_init();

  cpu_set_log_filename(logfile.get_value().c_str());

  if (dyn_debug_arg.get_value()) {
    //cout << __func__ << " " << __LINE__ << ": setting dyn_debug to true\n";
    dyn_debug = true;
    disable_folding_functions = {"ALL"};
  }

  map<string, shared_ptr<tfg>> function_tfg_map = get_function_tfg_map_from_etfg_file(etfg_file.get_value(), g_ctx);
  codegen(function_tfg_map, output_file.get_value(), disable_folding_functions, g_ctx);

  dst_free();
  CPP_DBG_EXEC(STATS,
    print_all_timers();
    cout << stats::get() << endl;
  );
  solver_kill();
  call_on_exit_function();
  return 0;
}
