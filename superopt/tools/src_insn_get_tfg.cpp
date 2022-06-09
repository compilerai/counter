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

#include <fstream>

#include "support/timers.h"

#include <iostream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;
static char as1[40960];

int main(int argc, char **argv)
{
  DBG_SET(INSN_MATCH, 2);
  DBG_SET(PEEP_TAB, 2);
  // command line args processing
  cl::arg<string> src_iseq_file(cl::implicit_prefix, "", "path to .src_iseq file");
  cl::cl cmd("src_iseq_get_tfg");
  cmd.add_arg(&src_iseq_file);
  cmd.parse(argc, argv);

  init_timers();

  src_init();
  dst_init();
  g_ctx_init();
  g_se_init();

  usedef_init();
  //sym_exec *se = new sym_exec(true);

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);
  //se->src_parse_sym_exec_db(g_ctx);
  //se->set_consts_struct(g_ctx->get_consts_struct());
  cpu_set_log_filename(DEFAULT_LOGFILE);

  ifstream in(src_iseq_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << src_iseq_file.get_value() << "." << endl;
    exit(1);
  }

  string src_iseq_str((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
#define MAX_SRC_ISEQ_LEN 1024
  cout << "src_iseq_str:\n" << src_iseq_str << endl;
  src_insn_t src_iseq[MAX_SRC_ISEQ_LEN];
  long src_iseq_len = src_iseq_from_string(NULL, src_iseq, MAX_SRC_ISEQ_LEN, src_iseq_str.c_str(), I386_AS_MODE_32);
  cout << "src_iseq:\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
  ASSERT(src_iseq_len == 1);
  tfg *t = g_se->src_insn_get_tfg(&src_iseq[0], 0, g_ctx);
  cout << "TFG:\n" << t->graph_to_string() << endl;
  //delete se;
  exit(0);
}
