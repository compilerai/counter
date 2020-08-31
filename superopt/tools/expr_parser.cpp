#include <fstream>
#include <iostream>
#include <string>

#include "expr/context.h"
#include "expr/expr.h"
#include "support/cl.h"

using namespace std;

int main(int argc, char **argv)
{
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to .expr file");
  cl::cl cmd("Expression Parser : Tries to parse a given expr file");
  cmd.add_arg(&expr_file);
  cmd.parse(argc, argv);

  context::config cfg(600, 600);
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);

  ifstream in(expr_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << '\n';
    return 1;
  }

  map<expr_id_t, expr_ref> emap;
  expr_ref e;
  string line = read_expr_and_expr_map(in, e, emap, &ctx);

  cout << expr_string(e) << endl;

  return 0;
}
