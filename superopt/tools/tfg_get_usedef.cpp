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
  cl::arg<string> tfg_file(cl::implicit_prefix, "", "path to .tfg file");
  cl::cl cmd("tfg_get_usedef");
  cmd.add_arg(&tfg_file);
  cmd.parse(argc, argv);

  init_timers();

  src_init();
  dst_init();
  g_ctx_init();

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);
  cpu_set_log_filename(DEFAULT_LOGFILE);

  usedef_init();
  context *ctx = g_ctx;
  consts_struct_t const &cs = ctx->get_consts_struct();

  DBG_SET(TYPESTATE, 2);
  ifstream in(tfg_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << tfg_file.get_value() << "." << endl;
    exit(1);
  }

  string line;
  getline(in, line);
  ASSERT(is_line(line, "=Base state"));
  map<string_ref, expr_ref> value_expr_map;
  line = state::read_state(in, value_expr_map, ctx/*, NULL*/);
  state base_state;
  base_state.set_value_expr_map(value_expr_map);
  base_state.populate_reg_counts(/*NULL*/);
  ASSERT(is_line(line, "=tfg"));
  tfg *t = new tfg(mk_string_ref("tfg_get_usedef"), ctx, base_state);
  dshared_ptr<tfg_node> n = make_dshared<tfg_node>(pc::start());
  t->add_node(n);
  t->read_from_ifstream(in, ctx, base_state);

  //pair<bool, string> p = read_tfg(in, &t, "src_tfg", ctx, cs);
  regset_t use, def;
  memset_t memuse, memdef;
  tfg_get_usedef(t, &use, &def, &memuse, &memdef);
  cout << "use: " << regset_to_string(&use, as1, sizeof as1) << endl;
  cout << "def: " << regset_to_string(&def, as1, sizeof as1) << endl;
  cout << "memuse: " << memset_to_string(&memuse, as1, sizeof as1);
  cout << "memdef: " << memset_to_string(&memdef, as1, sizeof as1);
  exit(0);
}
