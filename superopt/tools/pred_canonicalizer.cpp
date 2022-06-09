#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"

#include <fstream>

#include "support/timers.h"

#include <iostream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> pred_file(cl::implicit_prefix, "", "path to .pred file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::cl cmd("Predicate Canonicalizer(TM)");
  cmd.add_arg(&pred_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.parse(argc, argv);

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  ifstream in(pred_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << pred_file.get_value() << "." << endl;
    exit(1);
  }

  expr_ref lhs, rhs;
  string line;

  getline(in, line);
  ASSERT(is_line(line, "=lhs"));
  line = read_expr(in, lhs, &ctx);
  ASSERT(is_line(line, "=rhs"));
  line = read_expr(in, rhs, &ctx);

  predicate_ref p = predicate::mk_predicate_ref(precond_t(/*&ctx*/), lhs, rhs, "predicate");

  //vector<memlabel_ref> relevant_memlabels;
  //expr_get_relevant_memlabels(lhs, relevant_memlabels);
  //expr_get_relevant_memlabels(rhs, relevant_memlabels);
  //cs.solver_set_relevant_memlabels(relevant_memlabels);

  p = p->predicate_canonicalized();

  cout << p->to_string_for_eq() << endl;

  exit(1);
}
