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

#include "support/timers.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "expr/z3_solver.h"
#include "graph/tfg_rewrite.h"

#include <fstream>

#include <iostream>
#include <string>

using namespace std;

static char as1[4096];

int
main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> etfg_file(cl::implicit_prefix, "", "path to .etfg file");
  cl::arg<string> output_file(cl::explicit_prefix, "o", "out.etfg", "path to output object file");
  cl::cl cmd("Translate an etfg file to dst-arch (codegen)");
  cmd.add_arg(&etfg_file);
  cmd.add_arg(&output_file);
  cmd.parse(argc, argv);

  init_timers();
  cmd_helper_init();

  etfg_init();
  dst_init();

  g_ctx_init();

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);

  usedef_init();

  map<string, dshared_ptr<tfg>> function_tfg_map = get_function_tfg_map_from_etfg_file(etfg_file.get_value(), g_ctx);
  ofstream outputStream;
  outputStream.open(output_file.get_value(), ios_base::out | ios_base::trunc);

  for (const auto &ft : function_tfg_map) {
    string const &fname = ft.first;
    outputStream << "=FunctionName: " << fname << "\n";
    dshared_ptr<tfg> new_tfg = tfg::tfg_rewrite(*ft.second);
    outputStream << new_tfg->graph_to_string() << "\n";
    //delete new_tfg;
  }
  outputStream.close();
  outputStream.flush();
  
  dst_free();
  CPP_DBG_EXEC(STATS,
    print_all_timers();
    cout << stats::get() << endl;
  );
  solver_kill();
  call_on_exit_function();
  return 0;
}
