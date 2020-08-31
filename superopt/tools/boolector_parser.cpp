#include "parser/parse_tree_boolector.h"

int main(int argc,char** argv)
{
  g_ctx_init();
  assert(g_ctx);
  map<string_ref,expr_ref> r = parse_boolector_model(g_ctx,argv[1]);
  for (auto const& p : r) {
    cout << p.first->get_str() << " = " << expr_string(p.second) << endl;
  }
  return 0;
}
