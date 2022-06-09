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
  cl::cl cmd("update_dst_edge_for_local_alloc_and_dealloc query");
  cl::arg<string> query_file(cl::implicit_prefix, "", "path to input file");
  cmd.add_arg(&query_file);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  DYN_DBG_SET(update_dst_edge_for_local_allocation, 2);

  context::config cfg(600, 600);
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
  shared_ptr<tfg_full_pathset_t const> src_path, dst_path;

  if (!is_line(line, "=src_path")) {
    cout << _FNLN_ << ": line =\n" << line << endl;
  }
  ASSERT(is_line(line, "=src_path"));
  src_path = src_tfg.read_tfg_full_pathset(in, "=update_dst_edge_for_local_alloc_and_dealloc.src_path");

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(is_line(line, "=dst_path"));
  dst_path = dst_tfg.read_tfg_full_pathset(in, "=update_dst_edge_for_local_alloc_and_dealloc.dst_path");

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(is_line(line, "=pc_lsprels_allocs"));
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> pc_lsprels_allocs;
  line = pc_lsprels_allocs.pc_local_sprel_expr_guesses_from_stream(in, &ctx);
  ASSERT(line == "=pc_lsprels_deallocs");
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> pc_lsprels_deallocs;
  line = pc_lsprels_deallocs.pc_local_sprel_expr_guesses_from_stream(in, &ctx);
  ASSERT(line == "=end");

  auto new_dst_path = cg->update_dst_edge_for_local_allocations_and_deallocations(src_path, dst_path, pc_lsprels_allocs, pc_lsprels_deallocs);

  string outfilename = "cg.out";
  ofstream ofs(outfilename);
  cg->graph_to_stream(ofs);
  ofs.close();

  cout << "new_dst_path:\n" << new_dst_path->graph_full_pathset_to_string("new_dst_path") << endl;
  cout << "New CG written to " << outfilename << endl;

  solver_kill();
  call_on_exit_function();

  exit(0);
}
