#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "support/src-defs.h"
#include "i386/insn.h"
#include "ppc/insn.h"
#include "codegen/etfg_insn.h"
#include "rewrite/src-insn.h"
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
  // command line args processing
  cl::arg<int> mode_arg(cl::explicit_prefix, "mode", 32, "32/64 bit mode?");
  cl::arg<string> dst_binary(cl::implicit_prefix, "", "dst binary in comma-separated hexadecimal format");
  cl::cl cmd("dst_insn_disassemble");
  cmd.add_arg(&dst_binary);
  cmd.add_arg(&mode_arg);
  cmd.parse(argc, argv);

  init_timers();

  i386_as_mode_t mode;
  if (mode_arg.get_value() == 32) {
    mode = I386_AS_MODE_32;
  } else {
    mode = I386_AS_MODE_64;
  }

  src_init();
  dst_init();
  g_ctx_init();
  g_se_init();

  //usedef_init();
  //sym_exec *se = new sym_exec(true);

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);
  //se->src_parse_sym_exec_db(g_ctx);
  //se->set_consts_struct(g_ctx->get_consts_struct());
  cpu_set_log_filename(DEFAULT_LOGFILE);

  string bin = dst_binary.get_value();
  istringstream iss(bin);
  uint8_t bincode[bin.size()];
  int n = 0, r;
  cout << "bin = " << bin << endl;
  
  while (iss >> hex >> r) {
    //cout << "bincode[" << n << "] = " << hex << bincode[n] << dec << endl;
    bincode[n] = r;
    n++;
    char ch;
    if (iss >> ch) {
      //cout << "ch = " << ch << endl;
      ASSERT(ch == ',');
    } else {
      break;
    }
  }
  cout << "bincode =";
  for (int i = 0; i < n; i++) {
    cout << " 0x" << hex << int(bincode[i]) << dec;
  }
  cout << endl;
  dst_insn_t insn;
  dst_insn_disassemble(&insn, bincode, mode);
  cout << "insn: " << dst_insn_to_string(&insn, as1, sizeof as1) << endl;;
  exit(0);
}
