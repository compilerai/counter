#include <sys/stat.h>
#include <sys/types.h>
#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "expr/memlabel.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "eq/corr_graph.h"
#include "support/globals_cpp.h"
#include "codegen/etfg_insn.h"
#include "i386/insn.h"
#include "rewrite/src-insn.h"
#include "rewrite/dst-insn.h"

#include "support/timers.h"
#include "rewrite/peephole.h"
#include "sym_exec/sym_exec.h"

#include <iostream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;

static char as1[409600];

int
main(int argc, char **argv)
{
  // command line args processing

  init_timers();

  src_init();
  dst_init();

  g_ctx_init();
  context *ctx = g_ctx;

  usedef_init();

  return 0;
}
