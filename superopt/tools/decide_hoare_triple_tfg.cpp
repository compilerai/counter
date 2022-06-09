#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/z3_solver.h"
#include "expr/expr.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_asm_annotation.h"
#include "etfg/etfg_insn.h"
#include "i386/insn.h"
#include "x64/insn.h"

#include "support/timers.h"

#include <fstream>

#include <iostream>
#include <string>
//#include "parser/parse_edge_composition.h"

#define DEBUG_TYPE "main"

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::cl cmd("decide_hoare_triple query");
  cl::arg<string> query_file(cl::implicit_prefix, "", "path to input file");
  cmd.add_arg(&query_file);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&debug);
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cmd.add_arg(&smt_query_timeout);
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cmd.add_arg(&sage_query_timeout);

  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  DYN_DBG_SET(decide_hoare_triple_debug, 1);
  DYN_DBG_SET(prove_dump, 1);
  DYN_DBG_SET(smt_query_dump, 1);

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();
  src_init();
  dst_init();

  g_query_dir_init();

  ifstream in(query_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << query_file.get_value() << "." << endl;
    exit(1);
  }

  string line;
  bool end;

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(is_line(line, TFG_LLVM_IDENTIFIER " "));
  size_t start = strlen(TFG_LLVM_IDENTIFIER " ");
  size_t colon = line.find(':');
  ASSERT(colon != string::npos);
  ASSERT(colon > start);
  string llvm_tfg_name = line.substr(start, colon - start);
  
  dshared_ptr<tfg_llvm_t> tfg = tfg_llvm_t::tfg_llvm_from_stream(in, llvm_tfg_name, &ctx);

  end = !getline(in, line);
  ASSERT(!end);
  string const prefix_from_pc = "=from_pc ";
  ASSERT(string_has_prefix(line, prefix_from_pc));
  pc from_pc = pc::create_from_string(line.substr(prefix_from_pc.length()));

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(string_has_prefix(line, "=pre size "));

  end = !getline(in, line);
  ASSERT(!end);
  list<pair<graph_edge_composition_ref<pc,tfg_edge>,predicate_ref>> pre;
  while (string_has_prefix(line, "=pre.")) {
    guarded_pred_t pred = tfg->guarded_pred_from_stream(in, line, &ctx);
    //predicate::predicate_from_stream(in/*, cg->get_start_state()*/, &ctx, pred);
    pre.push_back(pred);
    end = !getline(in, line);
    ASSERT(!end);
    while (line == "") {
      end = !getline(in, line);
      ASSERT(!end);
    }
  }
  if (line != "=pth") {
    cout << _FNLN_ << ": line = '" << line << "'\n";
  }
  ASSERT(line == "=pth");
  //shared_ptr<cg_edge_composition_t> pth = parse_edge_composition<pcpair,corr_graph_edge>(line.c_str());
  //end = !getline(in, line);
  //ASSERT(!end);
  graph_edge_composition_ref<pc, tfg_edge> pth;
  pth = tfg->graph_edge_composition_from_stream(in/*, line*/, "=pth"/*, &ctx, pth*/);
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=post");
  predicate_ref post;
  predicate::predicate_from_stream(in/*, cg->get_start_state()*/, &ctx, post);
  end = !getline(in, line);
  ASSERT(end);

  cout << __func__ << " " << __LINE__ << ": tfg =\n";
  tfg->graph_to_stream(cout);
  cout << "from_pc = " << from_pc.to_string() << endl;
  cout << "pre:\n";
  size_t i = 0;
  for (auto const& pre_pred : pre) {
    cout << "pre." << i++ << ":\n";
    pre_pred.second->predicate_to_stream(cout);
  }
  cout << "=pth\n" << pth->graph_edge_composition_to_string() << endl;
  cout << "=post\n";
  post->predicate_to_stream(cout);
  //cout << ctx.stat() << endl;

  bool guess_made_weaker;
  proof_status_t proof_status = tfg->decide_hoare_triple(pre, from_pc, pth, post, guess_made_weaker, reason_for_counterexamples_t::inductive_invariants());
  cout << "proof_status: " << ctx.proof_status_to_string(proof_status) << endl;
  cout << "guess_made_weaker: " << guess_made_weaker << endl;

  solver_kill();
  call_on_exit_function();

  exit(0);
}
