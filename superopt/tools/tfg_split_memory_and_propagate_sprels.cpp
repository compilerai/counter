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
#include "graph/prove.h"
#include "tfg/tfg_asm.h"

#include <fstream>

#include "support/timers.h"

#include <iostream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;
static char as1[40960];

static void
skip_till_next_function(ifstream &in)
{
  string line;
  do {
    if (!getline(in, line)) {
      cout << __func__ << " " << __LINE__ << ": reached end of file\n";
      return;
    }
  } while (!string_has_prefix(line, FUNCTION_FINISH_IDENTIFIER));
  return;
}


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
  shared_ptr<tfg> tfg_dst = make_shared<tfg_asm_t>(in_dst, "dst", &ctx);

  DBG_SET(SPLIT_MEM, 2);
  DBG_SET(TFG_LOCS, 2);
  DYN_DBG_SET(linear_relations, 2);
  DBG_SET(ALIAS_ANALYSIS, 2);
  tfg_dst->collapse_tfg(true);
  tfg_dst->split_memory_in_graph_and_propagate_sprels_until_fixpoint(false);
  cout << "t =\n" << tfg_dst->graph_to_string() << endl;
  return 0;
}
