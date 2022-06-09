#include <fstream>
#include <iostream>
#include <string>

#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/src-defs.h"
#include "support/timers.h"

#include "expr/consts_struct.h"
#include "expr/z3_solver.h"
#include "expr/expr.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"

#include "gsupport/parse_edge_composition.h"

#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_inductive_preds.h"
#include "eq/cg_with_relocatable_memlabels.h"

#define DEBUG_TYPE "main"

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::cl cmd("graph_propagate_CEs_across_new_edge");
  cl::arg<string> query_file(cl::implicit_prefix, "", "path to input file");
  cmd.add_arg(&query_file);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());

  //DYN_DBG_SET(smt_query, 2);
  DYN_DBG_ELEVATE(ce_add, 2);
  DYN_DBG_ELEVATE(ce_clone, 2);
  DYN_DBG_ELEVATE(ce_translate, 2);
  DYN_DBG_ELEVATE(check_wfconds_on_edge_debug, 2);

  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  int smt_query_timeout = 600;
  int sage_query_timeout = 600;
  context::config cfg(smt_query_timeout, sage_query_timeout);
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();

  g_query_dir_init();
  src_init();
  dst_init();

  ifstream in(query_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << query_file.get_value() << "." << endl;
    exit(1);
  }

  string line;
  bool end;
  
  dshared_ptr<cg_with_asm_annotation> cg = cg_with_asm_annotation::corr_graph_from_stream(in, &ctx);

  tfg const& src_tfg = cg->get_src_tfg();
  tfg const& dst_tfg = cg->get_dst_tfg();

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=edge");
  end = !getline(in, line);
  ASSERT(!end);

  size_t arrow = line.find("=>");
  ASSERT(arrow != string::npos);
  string from_pc_str = line.substr(0, arrow);
  string to_pc_str = line.substr(arrow + 2);
  pcpair from_pc = pcpair::create_from_string(from_pc_str);
  pcpair to_pc = pcpair::create_from_string(to_pc_str);

  dshared_ptr<corr_graph_edge const> e = cg->find_edge(from_pc, to_pc);
  ASSERT(e);

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(is_line(line, "=end"));
  cout << "=edge\n" << e->to_string() << endl;

  cg->graph_propagate_CEs_across_new_edge(e);

  cout << "Propagated CEs across " << e->to_string_concise() << ":\n";
  string outfilename = query_file.get_value() + ".new_graph";
  ofstream fo(outfilename.data());
  ASSERT(fo);
  cg->graph_to_stream(fo);
  fo.close();
  cout << "New graph written to " << outfilename << endl;
  //cout << cg->get_invariant_state_at_pc(to_pc).invariant_state_to_string("  ", with_points.get_value());

  solver_kill();
  call_on_exit_function();

  exit(0);
}
