#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "support/src-defs.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "ppc/insn.h"
#include "etfg/etfg_insn.h"
#include "insn/src-insn.h"
#include "sym_exec/sym_exec.h"
#include "valtag/regcons.h"

#include <fstream>

#include "support/timers.h"

#include <iostream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;
static char as1[40960];

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> dst_assembly(cl::implicit_prefix, "", "dst assembly");
  cl::cl cmd("dst_infer_regcons_from_assembly");
  cmd.add_arg(&dst_assembly);
  cmd.parse(argc, argv);

  init_timers();

  //src_init();
  dst_init();
  g_ctx_init();
  //g_se_init();

  //usedef_init();
  //sym_exec *se = new sym_exec(true);

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);
  //se->src_parse_sym_exec_db(g_ctx);
  //se->set_consts_struct(g_ctx->get_consts_struct());
  //cpu_set_log_filename(DEFAULT_LOGFILE);

  ifstream in(dst_assembly.get_value());
  string dst_assembly_str((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());

  DBG_SET(INFER_CONS, 3);
  regcons_t regcons;
  bool ret = dst_infer_regcons_from_assembly(dst_assembly_str.c_str(), &regcons);
  if (!ret) {
    cout << "dst_infer_regcons_from_assembly() returned false!\n";
  } else {
    cout << "dst_infer_regcons_from_assembly() returned true, regcons:\n";
    cout << regcons_to_string(&regcons, as1, sizeof as1) << endl;
  }
  exit(0);
}
