#include "parser/parse_tree_z3.h"
#include "support/utils.h"

int main(int argc,char** argv)
{
  g_ctx_init();
  assert(g_ctx);
  assert(argc >= 2);

  map<string_ref,expr_ref> r = parse_z3_model(g_ctx,argv[1]);
  for (auto const& p : r) {
    cout << p.first->get_str() << " = " << expr_string(p.second) << endl;
  }

  print_all_timers();
  cout << "stats:\n" << stats::get();
  return 0;
}
