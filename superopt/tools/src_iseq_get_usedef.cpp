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
#include "expr/z3_solver.h"
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
  cl::arg<string> src_iseq_file(cl::implicit_prefix, "", "path to .src_iseq file");
  cl::cl cmd("src_iseq_get_usedef");
  cmd.add_arg(&src_iseq_file);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1,dumptfg,decide_hoare_triple,update_invariant_state_over_edge");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_timers();

  src_init();
  dst_init();
  g_ctx_init();

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);
  cpu_set_log_filename(DEFAULT_LOGFILE);

  usedef_init();

  //DBG_SET(INSN_MATCH, 2);
  DYN_DBG_SET(insn_match, 2);
  ifstream in(src_iseq_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << src_iseq_file.get_value() << "." << endl;
    exit(1);
  }

  string src_iseq_str((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
#define MAX_SRC_ISEQ_LEN 1024
  src_insn_t src_iseq[MAX_SRC_ISEQ_LEN];
  long src_iseq_len = src_iseq_from_string(NULL, src_iseq, MAX_SRC_ISEQ_LEN, src_iseq_str.c_str(), I386_AS_MODE_32);
  cout << "src_iseq:\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
  regset_t use, def;
  memset_t memuse, memdef;
  src_iseq_get_usedef(src_iseq, src_iseq_len, &use, &def, &memuse, &memdef);
  cout << "use: " << regset_to_string(&use, as1, sizeof as1) << endl;
  cout << "def: " << regset_to_string(&def, as1, sizeof as1) << endl;
  cout << "memuse: " << memset_to_string(&memuse, as1, sizeof as1) << endl;
  cout << "memdef: " << memset_to_string(&memdef, as1, sizeof as1) << endl;

  CPP_DBG_EXEC(STATS,
    print_all_timers();
    cout << __func__ << " " << __LINE__ << ":\n" << stats::get();
    cout << g_ctx->stats() << endl;
  );
  solver_kill();
  call_on_exit_function();
  exit(0);
}
