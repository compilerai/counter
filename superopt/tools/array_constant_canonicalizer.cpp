#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/dshared_ptr.h"
#include "support/dyn_debug.h"
#include "expr/z3_solver.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "eq/corr_graph.h"

#include "support/timers.h"

#include <fstream>

#include <iostream>
#include <string>
#include "support/utils.h"
#include "support/debug.h"
#include "expr/expr.h"
#include "expr/context.h"

/*   INPUT FILE FORMAT - Write   array constant exprs (upto 2) in below format
     -----------------
   =Array constant begin
   1 :([ 0; 2147483655 ] -> 0, [ 2147483656; 2147483656 ] -> 109, [ 2147483657; 4294967295 ] -> 0, ) : ARRAY[BV:32 -> BV:8]
   =end
*/

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> file_ac(cl::implicit_prefix, "", "path to input Array Constant file");
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");

  cl::cl cmd("Print difference between predicate sets of two .prove files");
  cmd.add_arg(&file_ac);
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());
  context::config cfg(100, 100);
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  ifstream in_ac(file_ac.get_value());
  if(!in_ac.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << file_ac.get_value() << "." << endl;
    exit(1);
  }

  string line;
  bool end;
  end = !getline(in_ac, line);
  ASSERT(!end);
  ASSERT(is_line(line, "=Array constant begin"));
  expr_ref ac_expr1, ac_expr2;
  line = read_expr(in_ac, ac_expr1, &ctx);
  ASSERT(ac_expr1);
  ASSERT(is_line(line, "=end"));
  cout << "Array constant 1 after canonicalization\n" << ctx.expr_to_string_table(ac_expr1) << endl; 
  end = !getline(in_ac, line);
  if(!end) {
    ASSERT(is_line(line, "=Array constant begin"));
    line = read_expr(in_ac, ac_expr2, &ctx);
    ASSERT(ac_expr2);
    cout << "Array constant 2 after canonicalization\n" << ctx.expr_to_string_table(ac_expr2) << endl; 
    if(ac_expr1 == ac_expr2)
      cout << "AC1 EQUALS AC2" << endl;
    else
      cout << "AC1 NOT EQUALS AC2" << endl;
  }

  call_on_exit_function();


  exit(0);
}
