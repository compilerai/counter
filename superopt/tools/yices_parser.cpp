#include "parser/parse_tree_yices.h"
#include "support/utils.h"
#include "support/dyn_debug.h"
#include "support/cl.h"

int main(int argc,char** argv)
{
  cl::arg<string> input_filename(cl::implicit_prefix, "", "path to input file");
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");

  cl::cl cmd("Yices CE Parser");
  cmd.add_arg(&input_filename);
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  g_ctx_init();
  assert(g_ctx);
  init_dyn_debug_from_string(debug.get_value());

  //DYN_DBG_SET(ce_parse,1);
  //DYN_DBG_SET(z3_parser,1);
  map<string_ref,expr_ref> r = parse_yices_model(g_ctx,input_filename.get_value().c_str());
  for (auto const& p : r) {
    cout << p.first->get_str() << " = " << expr_string(p.second) << endl;
  }

  print_all_timers();
  cout << "stats:\n" << stats::get();
  return 0;
}