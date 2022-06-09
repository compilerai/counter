#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/range.hpp>
#include <magic.h>

#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/globals_cpp.h"
#include "support/timers.h"
#include "support/read_source_files.h"
#include "support/src-defs.h"
#include "support/dyn_debug.h"

#include "expr/consts_struct.h"
#include "expr/z3_solver.h"

#include "rewrite/assembly_handling.h"

#include "tfg/tfg_llvm.h"

#include "ptfg/llptfg.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"

#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_asm_annotation.h"
#include "eq/function_cg_map.h"


#define DEBUG_TYPE "main"

using namespace std;

#define MAX_GUESS_LEVEL 1

int main(int argc, char **argv)
{
  autostop_timer func_timer(__func__);

  // command line args processing
  cl::cl cmd("tfg_unroll_till_no_repeated_pcs: required during SSA conversion");
  cl::arg<string> tfg_file(cl::implicit_prefix, "", "path to tfg file");
  cmd.add_arg(&tfg_file);
  cl::arg<string> dyn_debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1,dumptfg,decide_hoare_triple,update_invariant_state_over_edge");
  cmd.add_arg(&dyn_debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(dyn_debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());
  g_query_dir_init();
  src_init();
  dst_init();
  init_timers();

  DYN_DBG_SET(tfg_unroll_till_no_repeat_debug, 2);

  g_ctx_init();
  context *ctx = g_ctx;
  ctx->parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx->get_consts_struct();

  string tfg_filename = tfg_file.get_value();
  ifstream in(tfg_filename);
  if (!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open etfg_filename '" << tfg_filename << "'\n";
  }
  dshared_ptr<tfg> t = tfg::tfg_from_stream(in, ctx);

  string line;
  bool end;

  end = !getline(in, line);
  ASSERT(!end);
  string prefix = "=src_pathset_to_be_added";
  ASSERT(line == prefix);

  shared_ptr<tfg_full_pathset_t const> src_pathset_to_be_added = t->read_tfg_full_pathset(in, prefix);

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=existing_pcs");

  map<pc, clone_count_t> existing_pcs;
  end = !getline(in, line);
  ASSERT(!end);
  while (line != "=done") {
    size_t colon = line.find(':');
    ASSERT(colon != string::npos);
    string first_field = line.substr(0, colon);
    string second_field = line.substr(colon + 1);
    trim(first_field);
    trim(second_field);
    pc p = pc::create_from_string(first_field);
    clone_count_t c = string_to_int(second_field);
    existing_pcs.insert(make_pair(p, c));
    end = !getline(in, line);
    ASSERT(!end);
    //istringstream iss(line);
    //edge_id_t<pc> edge_id(iss);
    //dshared_ptr<tfg_edge const> te = t->find_edge(edge_id.get_from_pc(), edge_id.get_to_pc());
    //ASSERT(te);
    //existing_edges.insert(te.get_shared_ptr());
  }

  cout << _FNLN_ << ": src_pathset_to_be_added =\n" << src_pathset_to_be_added->graph_full_pathset_to_string("orig_pathset") << endl;
  //pc new_to_pc;
  tfg_edge_composition_ref new_ec;
  dshared_ptr<tfg> new_tfg = corr_graph::cg_src_dst_tfg_unroll_till_no_repeated_pcs(*t, src_pathset_to_be_added, existing_pcs, new_ec/*, new_to_pc*/);
  cout << _FNLN_ << ": new_tfg =\n";
  if (new_tfg) {
    new_tfg->graph_to_stream(cout);
  } else {
    cout << "(nullptr)\n";
  }
  //cout << "new_to_pc = " << new_to_pc.to_string() << endl;
  return 0;
}
