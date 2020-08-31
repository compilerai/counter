#include "parser/parse_tree_cvc4.h"

int main(int argc,char** argv)
{
	g_ctx_init();
	assert(g_ctx);
	map<string_ref,expr_ref> r = parse_cvc4_model(g_ctx,argv[1]);
	map<string_ref,expr_ref>::iterator it;
	for(auto const& p : r) {
		expr_ref const& a = (p.second);
		cout << p.first->get_str() << " : " << expr_string(a) << endl;
	}
	return 0;
}
