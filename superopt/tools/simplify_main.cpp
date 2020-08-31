#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
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
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to .expr file ('-' for stdin)");
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::cl cmd("Expression Simplifier");
  cmd.add_arg(&expr_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&debug);
  cl::arg<string> output_file(cl::explicit_prefix, "o", "simplify.out", "path to output expr file");
  cmd.add_arg(&output_file);
  cmd.parse(argc, argv);

  eqspace::init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, eqspace::print_debug_class_levels());

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  //DYN_DBG_SET(expr_simplify_solver, 2);
  //DYN_DBG_SET(expr_simplify, 2);

  ifstream fin;
  istream* in = &cin;
  if (expr_file.get_value() != "-") {
    fin.open(expr_file.get_value());
    if(!fin.is_open()) {
      cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
      exit(1);
    }
    in = &fin;
  }

  expr_ref e;
  string line = read_expr(*in, e, &ctx);
  ASSERT(e);
  expr_ref e_simpl = ctx.expr_do_simplify(e);
  ofstream ofs(output_file.get_value());
  ofs << ctx.expr_to_string_table(e_simpl) << endl;
  ofs.close();

  CPP_DBG_EXEC(STATS,
    cout << __func__ << " " << __LINE__ << ":Printing stats:\n";
    print_all_timers();
    cout << stats::get();
    cout << ctx.stat() << endl;
  );

  return 0;
}
