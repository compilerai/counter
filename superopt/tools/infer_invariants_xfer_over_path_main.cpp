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
  cl::cl cmd("infer_invariants_xfer_over_path");
  cl::arg<string> query_file(cl::implicit_prefix, "", "path to input file");
  cl::arg<string> record_filename(cl::explicit_prefix, "record", "", "generate record log of SMT queries to given filename");
  cl::arg<string> replay_filename(cl::explicit_prefix, "replay", "", "replay log of SMT queries from given filename");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::arg<bool> with_points(cl::explicit_flag, "with-points", false, "Also output the final set of points at to_pc\n");
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");

  cmd.add_arg(&query_file);
  cmd.add_arg(&record_filename);
  cmd.add_arg(&replay_filename);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&with_points);
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  DYN_DBG_ELEVATE(stability, 1);
  DYN_DBG_ELEVATE(decide_hoare_triple_debug, 1);
  //DYN_DBG_SET(smt_query, 2);
  //DYN_DBG_ELEVATE(prove_dump, 1);
  //DYN_DBG_SET(decide_hoare_triple_dump, 1);
  //DYN_DBG_SET(invariant_inference, 2);
  //DYN_DBG_SET(dfa, 2);

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  solver_init(record_filename.get_value(), replay_filename.get_value());

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
  
  ctx.restore_context_state_from_stream(in);
  dshared_ptr<cg_with_asm_annotation> cg = cg_with_asm_annotation::corr_graph_from_stream(in, &ctx);

  tfg const& src_tfg = cg->get_src_tfg();
  tfg const& dst_tfg = cg->get_dst_tfg();

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=path");
  shared_ptr<cg_edge_composition_t> pth;
  pth = cg->graph_edge_composition_from_stream(in, "=infer_invariants_xfer_over_path");

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(is_line(line, "=end"));

  cout << "path: " << pth->graph_edge_composition_to_string() << endl;
  pcpair const& from_pc = pth->graph_edge_composition_get_from_pc();
  pcpair const&   to_pc = pth->graph_edge_composition_get_to_pc();

  auto& invstate_in = cg->get_invariant_state_at_pc_ref(from_pc, reason_for_counterexamples_t::inductive_invariants());
  auto& invstate_out = cg->get_invariant_state_at_pc_ref(to_pc, reason_for_counterexamples_t::inductive_invariants());

  auto changed = invstate_out.inductive_invariant_xfer_on_incoming_edge_composition(pth, invstate_in, *cg, "infer_invariants_xfer_over_path tool");
  bool ret = cg->graph_is_stable();

  cout << "After update_invariant_state_over_edge, graph is" << (ret ? "" : " NOT") << " stable" << endl;
  //cout << "Updated invariant state at " << e->get_to_pc().to_string() << ":\n";
  //string outfilename = query_file.get_value() + ".new_graph";
  //ofstream fo(outfilename.data());
  //ASSERT(fo);
  //cg->graph_to_stream(fo);
  //fo.close();
  //cout << "New graph written to " << outfilename << endl;
  //cout << cg->get_invariant_state_at_pc(to_pc, reason_for_counterexamples_t::inductive_invariants()).invariant_state_to_string("  ", with_points.get_value());

  solver_kill();
  call_on_exit_function();

  exit(0);
}
