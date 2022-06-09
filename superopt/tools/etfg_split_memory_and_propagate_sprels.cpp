#include "support/debug.h"
#include "support/cl.h"
#include "support/globals.h"
#include "tfg/tfg_llvm.h"
#include "expr/consts_struct.h"
#include "ptfg/ftmap.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> function_name(cl::implicit_prefix, "", "function name");
  cl::arg<string> etfg_file(cl::implicit_prefix, "", "path to .etfg file");
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1,dumptfg,decide_hoare_triple,update_invariant_state_over_edge");
  cl::cl cmd("etfg_split_memory_and_propagate_sprels");

  cmd.add_arg(&function_name);
  cmd.add_arg(&etfg_file);
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());

  context::config cfg(3600, 3600);
  context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  string filename_llvm = etfg_file.get_value();
  ifstream in_llvm(filename_llvm);
  if (!in_llvm.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open tfg_filename '" << filename_llvm << "'\n";
  }
  ASSERT(in_llvm.is_open());

  string line;
  getline(in_llvm, line);
  string match_line(string(FUNCTION_NAME_FIELD_IDENTIFIER) + " " + function_name.get_value());
  while (!is_line(line, match_line)) {
    bool end = !getline(in_llvm, line);
    if (end) {
      cout << __func__ << " " << __LINE__ << ": function " << function_name.get_value() << " not found in llvm file " << filename_llvm << endl;
      return 2;
    }
  }
  if(!is_next_line(in_llvm, TFG_IDENTIFIER)) {
    cout << __func__ << " " << __LINE__ << ": parsing failed\n";
    return 3;
  }
  auto tfg_llvm = tfg_llvm_t::tfg_llvm_from_stream(in_llvm, function_name.get_value(), &ctx);
  ASSERT(tfg_llvm);
  tfg_llvm->rename_llvm_symbols_to_keywords(&ctx);

  DBG_SET(SPLIT_MEM, 2);
  DYN_DBG_SET(tfg_locs, 2);
  DYN_DBG_SET(linear_relations, 2);
  set_debug_class_level("alias_analysis", 2);

  tfg_llvm->collapse_tfg();
  NOT_IMPLEMENTED();
  //ftmap_t::tfg_run_pointsto_analysis(tfg_llvm/*, callee_rw_memlabels_t::callee_rw_memlabels_nop()*/, false, dshared_ptr<tfg_llvm_t const>::dshared_nullptr());
  cout << "t =\n" << tfg_llvm->graph_to_string() << endl;
  return 0;
}
