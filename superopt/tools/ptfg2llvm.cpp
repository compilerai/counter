#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "eq/corr_graph.h"
#include "support/globals_cpp.h"
#include "expr/z3_solver.h"
#include "support/timers.h"
#include "tfg/tfg_llvm.h"
#include "ptfg/llptfg.h"

#include <iostream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;

int main(int argc, char **argv)
{
  autostop_timer func_timer(__func__);
  // command line args processing
  cl::arg<string> etfg_file(cl::implicit_prefix, "", "path to .etfg file");
  cl::arg<string> output_file(cl::explicit_prefix, "o", "out.ll", "output file");
  cl::cl cmd("Outputs .ll file for a given .etfg file");
  cmd.add_arg(&etfg_file);
  cmd.add_arg(&output_file);

  for (int i = 0; i < argc; i++) {
    cout << "argv[" << i << "] = " << argv[i] << endl;
  }
  cmd.parse(argc, argv);

  g_ctx_init();
  context *ctx = g_ctx;

  ifstream etfg(etfg_file.get_value());
  ASSERT(etfg.is_open());
  llptfg_t llptfg = llptfg_t::read_from_stream(etfg, ctx);
  etfg.close();

  ofstream ll(output_file.get_value());
  ASSERT(ll.is_open());
  llptfg.output_llvm_code(ll);
  ll.close();

  return 0;
}
