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
  cl::cl cmd("decide_hoare_triple query");
  cl::arg<string> query_file(cl::implicit_prefix, "", "path to input file");
  cmd.add_arg(&query_file);
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cmd.add_arg(&smt_query_timeout);
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cmd.add_arg(&sage_query_timeout);
  cl::arg<bool> with_points(cl::explicit_flag, "with-points", false, "Also output the final set of points at to_pc\n");
  cmd.add_arg(&with_points);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  DYN_DBG_ELEVATE(decide_hoare_triple_debug, 1);
  DYN_DBG_ELEVATE(decide_hoare_triple_dump, 1);
  DYN_DBG_ELEVATE(prove_dump, 1);
  DYN_DBG_ELEVATE(smt_query_dump, 1);
  DYN_DBG_ELEVATE(stats, 1);

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
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
  shared_ptr<cg_edge_composition_t> pth;
  pth = cg->graph_edge_composition_from_stream(in/*, line*/, "=check_wfconds_on_edge"/*, &ctx, pth*/);
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(is_line(line, "=end") || is_line(line, "=failing_pred"));

  ASSERT(pth->is_atom());
  auto e = pth->get_atom()->edge_with_unroll_get_edge();
  pcpair to_pc = e->get_to_pc();

  //cout << __func__ << " " << __LINE__ << ": cg =\n";
  //cg->graph_to_stream(cout);
  cout << "=edge\n" << e->to_string() << endl;

  bool ret = cg->check_wfconds_on_edge(e);

  cout << "check_wfconds_on_edge returned: " << ret << endl;

  solver_kill();
  call_on_exit_function();

  exit(0);
}
