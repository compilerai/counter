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
#include "insn/dst-insn.h"

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
  cl::arg<string> dst_insn_file(cl::implicit_prefix, "", "path to .dst_insn file");
  cl::cl cmd("dst_insn_get_usedef");
  cmd.add_arg(&dst_insn_file);
  cmd.parse(argc, argv);

  init_timers();

  src_init();
  dst_init();
  g_ctx_init();

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);
  cpu_set_log_filename(DEFAULT_LOGFILE);

  usedef_init();

  DBG_SET(INSN_MATCH, 2);
  ifstream in(dst_insn_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << dst_insn_file.get_value() << "." << endl;
    exit(1);
  }

  string dst_insn_str((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
  dst_insn_t dst_insn;
  long dst_iseq_len = dst_iseq_from_string(NULL, &dst_insn, 1, dst_insn_str.c_str(), I386_AS_MODE_32);
  ASSERT(dst_iseq_len == 1);
  dst_iseq_rename_vregs_to_cregs(&dst_insn, 1);
  cout << "dst_insn:\n" << dst_iseq_to_string(&dst_insn, 1, as1, sizeof as1) << endl;
  regset_t use, def;
  memset_t memuse, memdef;
  dst_insn_get_usedef(&dst_insn, &use, &def, &memuse, &memdef);
  cout << "use: " << regset_to_string(&use, as1, sizeof as1) << endl;
  cout << "def: " << regset_to_string(&def, as1, sizeof as1) << endl;
  cout << "memuse: " << memset_to_string(&memuse, as1, sizeof as1) << endl;
  cout << "memdef: " << memset_to_string(&memdef, as1, sizeof as1) << endl;

  CPP_DBG_EXEC(STATS,
    print_all_timers();
    cout << __func__ << " " << __LINE__ << ":\n" << stats::get();
    cout << g_ctx->stats() << endl;
  );
  //solver_kill();
  call_on_exit_function();
  exit(0);
}
