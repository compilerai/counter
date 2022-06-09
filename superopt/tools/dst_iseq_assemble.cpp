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
  // command line args processing
  cl::arg<int> mode_arg(cl::explicit_prefix, "mode", 32, "32/64 bit mode?");
  cl::arg<string> dst_assembly(cl::implicit_prefix, "", "filename with dst assembly to be assembled");
  cl::cl cmd("dst_insn_disassemble");
  cmd.add_arg(&dst_assembly);
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
  DBG_SET(CMN_ISEQ_FROM_STRING, 2);
  DBG_SET(INFER_CONS, 3);
  DBG_SET(TCDBG_I386, 2);

  ifstream in(dst_assembly.get_value());
  string assembly((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
  size_t dst_iseq_size = assembly.size();
  dst_insn_t *dst_iseq = new dst_insn_t[dst_iseq_size];
  size_t dst_iseq_len = dst_iseq_from_string(NULL, dst_iseq, dst_iseq_size, assembly.c_str(), mode);
  ASSERT(dst_iseq_len != -1);

  cout << __func__ << " " << __LINE__ << ": disassembled dst_iseq:\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
  size_t binbuf_size = dst_iseq_len * 16;
  uint8_t *binbuf = new uint8_t[binbuf_size];
  size_t binlen = dst_iseq_assemble(NULL, dst_iseq, dst_iseq_len, binbuf, binbuf_size, mode);

  cout << "bincode =";
  for (int i = 0; i < binlen; i++) {
    cout << " 0x" << hex << int(binbuf[i]) << dec;
  }
  exit(0);
}
