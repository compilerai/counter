#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "rewrite/translation_instance.h"
#include "support/debug.h"

#include <fstream>

#include "support/timers.h"

#include <iostream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to .expr file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::cl cmd("Expression Simplifier");
  cmd.add_arg(&expr_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.parse(argc, argv);

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  ifstream in(expr_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
    exit(1);
  }

  sprel_map_pair_t sprel_map_pair;
  map<mlvarname_t, memlabel_ref> memlabel_map;
  expr_ref e;

  string line;
  getline(in, line);
  ASSERT(is_line(line, "=sprel_map_pair"));
  line = read_sprel_map_pair(in, &ctx, sprel_map_pair);

  ASSERT(is_line(line, "=memlabel_map"));
  getline(in, line);
  NOT_IMPLEMENTED(); //line = tfg::read_memlabel_map(in, &ctx, line, /*dst_*/memlabel_map);

  ASSERT(is_line(line, "=expr"));
  line = read_expr(in, e, &ctx);

  //vector<memlabel_ref> relevant_memlabels;
  //memlabel_map_get_relevant_memlabels(memlabel_map, relevant_memlabels);
  //cs.solver_set_relevant_memlabels(relevant_memlabels);

  expr_ref e_simpl = ctx.expr_simplify_using_sprel_pair_and_memlabel_maps(e, sprel_map_pair, memlabel_map);

  //cspace::call_on_exit_function();

  cout << ctx.expr_to_string_table(e_simpl) << endl;
  //cout << __func__ << ' ' <<  __LINE__ << ":\n"; print_all_timers();
  //cout << __func__ << ' ' <<  __LINE__ << ":\n" << stats::get();
  //cout << __func__ << ' ' <<  __LINE__ << ":\n" << ctx.stat() << endl;

  exit(0);
}
