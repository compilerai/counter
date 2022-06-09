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
#include "graph/prove.h"
#include "tfg/tfg_asm.h"
#include "ptfg/ftmap.h"

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
  cl::arg<string> function_name(cl::implicit_prefix, "", "function name");
  cl::arg<string> tfg_file(cl::implicit_prefix, "", "path to .tfg file");
  cl::cl cmd("tfg_split_memory_and_propagate_sprels");

  cmd.add_arg(&function_name);
  cmd.add_arg(&tfg_file);
  cmd.parse(argc, argv);

  context::config cfg(3600, 3600);
  context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  string filename_dst = tfg_file.get_value();
  ifstream in_dst(filename_dst);
  if (!in_dst.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open tfg_filename '" << filename_dst << "'\n";
  }
  ASSERT(in_dst.is_open());

  string line;
  string cur_function_name;
  bool found_function = false;
  while (getNextFunctionInTfgFile(in_dst, cur_function_name)) {
    if (function_name.get_value() != cur_function_name) {
      skip_till_next_function(in_dst);
    }
    else {
      found_function = true;
      break;
    }
  }
  if (!found_function)
    return 1;

  if (!is_next_line(in_dst, "=Rodata-map")) {
    cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
    NOT_REACHED();
    return 2;
  }
  auto rodata_map = rodata_map_t(in_dst);

  bool end = !getline(in_dst, line);
  ASSERT(!end);
  ASSERT(line == "=dst_iseq");
  while (!!getline(in_dst, line) && line.at(0) != '=');
  ASSERT(line == "=dst_iseq done");

  end = !getline(in_dst, line);
  ASSERT(!end);
  ASSERT(line == "=dst_insn_pcs");
  while (!!getline(in_dst, line) && line.at(0) != '=');
  ASSERT(line == "=dst_insn_pcs done");

  end = !getline(in_dst, line);
  ASSERT(!end);
  // XXX read tfg_ssa!
  dshared_ptr<tfg> tfg_dst = tfg_asm_t::tfg_asm_from_stream(in_dst, "dst", &ctx);

  DBG_SET(SPLIT_MEM, 2);
  DYN_DBG_SET(tfg_locs, 2);
  DYN_DBG_SET(linear_relations, 2);
  DBG_SET(ALIAS_ANALYSIS, 2);
  tfg_dst->collapse_tfg();
  NOT_IMPLEMENTED();
  //ftmap_t::tfg_run_pointsto_analysis(tfg_dst/*, callee_rw_memlabels_t::callee_rw_memlabels_nop()*/, false, dshared_ptr<tfg_llvm_t const>::dshared_nullptr());
  cout << "t =\n" << tfg_dst->graph_to_string() << endl;
  return 0;
}
