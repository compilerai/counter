#pragma once

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include "expr/expr.h"
#include "expr/context.h"
#include "support/string_ref.h"

namespace eqspace { class context; } // fwd decl

using namespace std;
using namespace eqspace;

struct funcNode{
	int arity;
  sort_ref ret_type;
	vector<sort_ref> func_args;
	sort_ref sort;
	funcNode(){}
};


struct val_case{
	expr_ref b;
	expr_ref v;
	val_case(expr_ref b_,expr_ref v_)
	{
		b = b_;
		v = v_;
	}
};

struct fn_value{
	expr_ref f;
	expr_ref a;
	fn_value(expr_ref f_,expr_ref a_){
		f = f_;
		a = a_;
	}
	fn_value(){}
};

struct env{
	sort_ref z_sort_ref;
	sort_ref z_sort_domain;
	sort_ref z_sort_range;
	vector<sort_ref> z_sort_vector;
	expr_ref z_expr;
	expr_ref z_ite_def;
	expr_ref z_ite;
	expr_ref z_bool_ite;
	expr_ref z_ite_arg;
	expr_ref z_expr_arg1;
	expr_ref z_expr_arg2;
	expr_ref z_expr_bool;
	expr_ref z_expr_bool_list;
	vector<expr_ref> z_val_list;
	vector<int> z_id_list;
	map<string_ref, funcNode> funcMeta;
	map<string_ref, expr_ref> funcMap;
	map<string_ref, expr_ref> localfuncMap;
	map<string_ref,fn_value> function_vals;
	vector<val_case> values;
	int arg_index;
	int sort_index;
};


/*environment operations*/
void init_env();
void destroy_env();

/*utils*/
string get_arg_name(int index);
string convert_binary_to_hex(string cons);

/*constructors*/
expr_ref value(string name);
expr_ref make_eq(expr_ref v1,expr_ref v2);	//to resolve sort mismatch due to incomplete type
expr_ref make_select(expr_ref v1,expr_ref v2);	//to resolve sort mismatch due to incomplete type
expr_ref get_const_array(sort_ref s,string val);
expr_ref create_ite_expr_from_list(expr_ref def);
expr_ref create_concat_expr_from_list();

//free up memory
void cleanup_state();


//solver functions
void solve_list();
void fix_function_parameters();
void add_func(string name, expr_ref v);
expr_ref get_bool_bv_cons(context *ctx, string const &cons);
expr_ref solve_func(string name, bool as_array);
expr_ref solve_val(expr_ref v, int arity);
expr_ref solve_ite(expr_ref v, int arity);
expr_ref solve_concat(expr_ref v);
expr_ref solve_select(expr_ref v);
expr_ref solve_and(expr_ref v);
expr_ref solve_or(expr_ref v);
expr_ref solve_xor(expr_ref v);
expr_ref solve_not(expr_ref v);
expr_ref solve_bvnot(expr_ref v);
expr_ref solve_bvadd(expr_ref v);
expr_ref solve_bvmul(expr_ref v);
expr_ref solve_bvsle(expr_ref v);
vector< pair<string,expr_ref> > solve_bool(expr_ref b);
vector<expr_ref> resolve_bool_expr(expr_ref b);

map<string_ref,expr_ref> parse_yices_model(context* ctx, const char* str);

extern context* z_ctx;
extern list<env> env_list;
extern bool is_solved;
