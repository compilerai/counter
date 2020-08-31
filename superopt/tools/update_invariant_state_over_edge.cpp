#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/z3_solver.h"
#include "expr/expr.h"
#include "eq/corr_graph.h"
#include "support/src-defs.h"
#include "support/timers.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include <fstream>

#include <iostream>
#include <string>
#include "gsupport/parse_edge_composition.h"

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
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&with_points);
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  eqspace::init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, eqspace::print_debug_class_levels());

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
  
  shared_ptr<corr_graph> cg = corr_graph::corr_graph_from_stream(in, &ctx);

  tfg const& src_tfg = cg->get_src_tfg();
  tfg const& dst_tfg = cg->get_dst_tfg();

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=edge");
  end = !getline(in, line);
  ASSERT(!end);
  shared_ptr<cg_edge_composition_t> pth;
  line = graph_edge_composition_t<pcpair,corr_graph_edge>::graph_edge_composition_from_stream(in, line, "=update_invariant_state_over_edge"/*, cg->get_start_state()*/, &ctx, pth);
  ASSERT(is_line(line, "=end"));

  ASSERT(pth->is_atom());
  corr_graph_edge_ref e = pth->get_atom();
  pcpair to_pc = e->get_to_pc();

  cout << __func__ << " " << __LINE__ << ": cg =\n";
  cg->graph_to_stream(cout);
  cout << "=edge\n" << e->to_string() << endl;

  bool ret = cg->update_invariant_state_over_edge(e);

  cout << "update_invariant_state_over_edge returned (indicating if asserts still pass or not): " << ret << endl;
  cout << "Updated invariant state at " << e->get_to_pc().to_string() << ":\n";
  string outfilename = query_file.get_value() + ".new_graph";
  ofstream fo(outfilename.data());
  ASSERT(fo);
  cg->graph_to_stream(fo);
  fo.close();
  cout << "New graph written to " << outfilename << endl;
  cout << cg->get_invariant_state_at_pc(to_pc).invariant_state_to_string("  ", with_points.get_value());

  solver_kill();
  call_on_exit_function();

  exit(0);
}
