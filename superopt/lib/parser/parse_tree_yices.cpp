#include "parser/parse_tree_yices.h"
#include "parser/parse_tree_common.h"
#include "support/debug.h"

context* z_ctx;
list<env> env_list;
bool is_solved;

/*environment operators*/
void init_env()
{
	env t;
	t.arg_index = 1;
	t.sort_index = -1;
	env_list.push_front(t);
}

void destroy_env()
{
	env_list.pop_front();
}

/***************utils****************/

string get_arg_name(int index)
{
	stringstream ss;
	ss << index;
	string str = "x!" + ss.str();
	return str;
}

expr_ref eq_adjust_sort(expr_ref v1,expr_ref v2,int type)
{
	//use type information of v1 for v2
	if(type == 0)
	{
		ASSERT(v2->is_var());
		string_ref const &n = v2->get_name();
		expr_ref tmp = z_ctx->mk_var(n, v1->get_sort());
		return z_ctx->mk_eq(v1,tmp);
	}
	else
	{
		ASSERT(v1->is_var());
		string_ref const &n = v1->get_name();
		expr_ref tmp = z_ctx->mk_var(n, v2->get_sort());
		return z_ctx->mk_eq(tmp,v2);
	}
}

/*****constructors*********/
expr_ref value(string name)
{
	env& e = env_list.front();

	//currently in local env
	if(env_list.size()>1)
	{
		//distinguish between reference to a local definition in the current environment(new or defined earlier)
		// or to a existing function

		//if entry in funcMap of current env- local definition previously defined
		//else either a new local definition or reference to a discovered/undiscovered function

		map<string_ref,expr_ref>::iterator it;
		it = (e.funcMap).find(mk_string_ref(name));
		if (it != (e.funcMap).end()) {
			return it->second;
        }

		//search in outer environments
		list<env>::iterator list_iter;
		map<string_ref,funcNode>::iterator it1;
		for(list_iter = ++env_list.begin();list_iter!= env_list.end();list_iter++)
		{
			it1 = (list_iter->funcMeta).find(mk_string_ref(name));
			if(it1 != (list_iter->funcMeta).end())
			{
				sort_ref s = z_ctx->mk_array_sort((it1->second).func_args,(it1->second).ret_type);
				expr_ref v = z_ctx->mk_var(name,s);
				return v;
			}
		}

		//if its a new local definition then returned value doesn't matter since
		//only name would be used to create the subsequent entry inn funcMap

		return z_ctx->mk_undefined();
	}

	//check if function has been defined earlier
	map<string_ref,funcNode>::iterator it1;
	it1 = (e.funcMeta).find(mk_string_ref(name));
	if(it1 != (e.funcMeta).end())
	{
		sort_ref s = z_ctx->mk_array_sort((it1->second).func_args,(it1->second).ret_type);
		expr_ref v = z_ctx->mk_var(name,s);
		return v;
	}
	else
		return z_ctx->mk_undefined();
}

expr_ref make_eq(expr_ref v1,expr_ref v2)
{
	// NOTE:- if type info is unavailable, then by default
	// var of bool sort is made for a variable

	//sorts satisfy (=> both undefined or both defined)
	if(v1->get_sort() == v2->get_sort())
		return z_ctx->mk_eq(v1,v2);

	else if(v1->is_var() && v2->is_var())
	{
		//v2 has well defined type
		if(v1->is_bool_sort())
			return eq_adjust_sort(v1,v2,1);

		//v1 has well defined type
		else
			return eq_adjust_sort(v1,v2,0);
	}
	else if(v1->is_var())
		return eq_adjust_sort(v1,v2,1);
	else
		return eq_adjust_sort(v1,v2,0);
}

expr_ref make_select(expr_ref v1,expr_ref v2)
{
	//comment_t c;
	memlabel_t m = memlabel_t::memlabel_top();
	//m.m_type = memlabel_t::MEMLABEL_TOP;
	bool is_same_type = v1->is_array_sort() && (v1->get_sort()->get_domain_sort()[0] == v2->get_sort());

	if(!is_same_type){
		cout << __func__ << ':' << __LINE__ << " sorts are mis-matching: [" << expr_string(v1) << "], [" << expr_string(v2) << "]\n";
		sort_ref s = z_ctx->mk_array_sort(v2->get_sort(),v1->get_sort());
		array_constant_ref arr_cons = z_ctx->mk_array_constant(v1);
		expr_ref tmp = z_ctx->mk_array_const(arr_cons,s);
		return z_ctx->mk_select(tmp,m,v2,(unsigned)1,true/*,c*/);
	}
	else
	{
		ASSERT(v2->is_const());
		return z_ctx->mk_select(v1,m,v2,(unsigned)1,false/*,c*/);
	}
}

expr_ref get_const_array(sort_ref s, string val){
	expr_ref v = get_bool_bv_cons(z_ctx,val);
    context *ctx = v->get_context();
    consts_struct_t const &cs = ctx->get_consts_struct();
	array_constant_ref arr_cons = ctx->mk_array_constant(map<vector<expr_ref>, expr_ref>(), v);
	expr_ref f = z_ctx->mk_array_const(arr_cons,s);
	return f;
}

expr_ref create_ite_expr_from_list(expr_ref def)
{
	env& e = env_list.front();
	int sz = e.values.size();

  vector<val_case> new_values = e.values;

	//check if all constituent exprs are defined
	if(def->is_undefined())
		return z_ctx->mk_undefined();
	for(int i=0;i< new_values.size();i++)
	{
		if((new_values[i].b)->is_undefined() || (new_values[i].v)->is_undefined())
			return z_ctx->mk_undefined();
	}

  ASSERT(sz > 0);
	expr_ref curr;
	expr_ref tmp;
	curr = z_ctx->mk_ite(new_values[sz-1].b,new_values[sz-1].v,def);
	// cout << curr->to_string() << endl;
	for(int i = new_values.size()-2; i >= 0; i--)
	{
		tmp = z_ctx->mk_ite(new_values[i].b,new_values[i].v,curr);
		curr = tmp;
	}
	return curr;
}

expr_ref create_concat_expr_from_list(){
	env& e = env_list.front();
	int sz = e.z_val_list.size();

	//check if all constituent exprs are defined
	for(int i=0;i<sz;i++){
		if((e.z_val_list[i])->is_undefined())
			return z_ctx->mk_undefined();
	}

	expr_ref res = e.z_val_list[0];
	for(int i=1;i<sz;i++){
		res = z_ctx->mk_bvconcat(res,e.z_val_list[i]);
	}
	return res;
}

/******solver functions********/

void add_func(string name,expr_ref v)
{
	map<string_ref,expr_ref>::iterator it1;
	map<string_ref,funcNode>::iterator it2;
	env& e = env_list.front();

	it2 = e.funcMeta.find(mk_string_ref(name));
	if(it2 == e.funcMeta.end())
	{
		funcNode f;
		f.arity = e.z_sort_vector.size();
		f.ret_type = e.z_sort_range;
		f.func_args = e.z_sort_vector;
		f.sort = e.z_sort_ref;
		e.funcMeta.insert(make_pair(mk_string_ref(name),f));
	}
	it1 = e.funcMap.find(mk_string_ref(name));

	if(it1 != e.funcMap.end())
		it1->second = v;
	else
		e.funcMap.insert(make_pair(mk_string_ref(name),v));
}

void fix_function_parameters(){

	map<string_ref,expr_ref>::iterator it;
	map<string_ref,funcNode>::iterator it_meta;
	env& e = env_list.front();
	for(it = e.funcMap.begin();it != e.funcMap.end(); ++it)
	{

		it_meta = e.funcMeta.find(it->first);
		if(!((it->first->get_str()).substr(0,4)).compare("@fun"))
			continue;

		if((it->second)->is_const()){
			(it_meta->second).arity = 0;
			// (it_meta->second).sort = (it->second)->get_sort();
		}

		else if((it->second)->is_var())
		{
			string const &fn = (it->second)->get_name()->get_str();
			map<string_ref,funcNode>::iterator it2;
			it2 = e.funcMeta.find(mk_string_ref(fn));
			assert(it2 != e.funcMeta.end());
			(it_meta->second).arity = (it2->second).arity;
			(it_meta->second).ret_type = (it2->second).ret_type;
			(it_meta->second).func_args = (it2->second).func_args;
			(it_meta->second).sort = (it2->second).sort;
		}
	}
}

//fixed point iteration for all the functions
void solve_list()
{
	env& e = env_list.front();
	expr_ref r;
	map<string_ref,fn_value>::iterator it;
	map<string_ref,expr_ref>::iterator it1;
	u_int n_fns = e.funcMap.size();

	while(e.function_vals.size() != n_fns)
	{
		for (it1 = e.funcMap.begin(); it1 != e.funcMap.end(); ++it1)
		{
			// cout << (it1->second)->to_string() << endl;
			//check if function has already been evaluated to constant
			string_ref name = it1->first;
			it = e.function_vals.find(name);
			// cout << name;
			if(it != e.function_vals.end()){
				// cout << " already exists" << endl;
				continue;
			}

			bool isConstant = true;
			//expr_list vars = (it1->second)->get_vars();
			expr_list vars = z_ctx->expr_get_vars(it1->second);

			// cout << vars.size() << endl;

      for (auto const& var : vars) {
        ASSERT(var->is_var());
				string_ref const &n = var->get_name();
        bool isLocalVar = string_has_prefix(n->get_str(), "x!");
				if (isLocalVar)
					continue;
				it = e.function_vals.find(n);
				if(it == e.function_vals.end()){
					isConstant = false;
					break;
				}
			}
			if(isConstant){
				r = solve_func(name->get_str(),false);
			}
		}
	}
}

expr_ref solve_func(string name,bool as_array)
{
	map<string_ref,fn_value>::iterator it;
	map<string_ref,funcNode>::iterator it_meta;
	env& e = env_list.front();

	it = e.function_vals.find(mk_string_ref(name));
	it_meta = e.funcMeta.find(mk_string_ref(name));

	if(it!= e.function_vals.end()){
		if(!as_array)
			return (it->second).f;
		else
			return (it->second).a;
	}

	int arity = (it_meta->second).arity;
	expr_ref a,f;

	expr_ref v = e.funcMap[mk_string_ref(name)];
	expr_ref r = solve_val(v,arity);

	//function is of array type and always takes the default value
	if(arity > 0 && r->get_sort() == (it_meta->second.sort)->get_range_sort()){
                context *ctx = r->get_context();
                consts_struct_t const &cs = ctx->get_consts_struct();
		array_constant_ref arr_cons = z_ctx->mk_array_constant(map<vector<expr_ref>, expr_ref>(), r);
		sort_ref arr_sort = z_ctx->mk_array_sort((it_meta->second.sort)->get_domain_sort(),r->get_sort());
		a = z_ctx->mk_array_const(arr_cons,arr_sort);
		f = z_ctx->mk_array_const(arr_cons,it_meta->second.sort);
	}

	//function is a constant with no arguments
	else if(r->get_sort() == (it_meta->second.sort)->get_range_sort()){
		a = r;
                context *ctx = r->get_context();
                consts_struct_t const &cs = ctx->get_consts_struct();
		array_constant_ref arr_cons = z_ctx->mk_array_constant(map<vector<expr_ref>, expr_ref>(), r);
		f = z_ctx->mk_array_const(arr_cons,it_meta->second.sort);
	}

	//function is an array with atleast one mapping
	else{
		ASSERT(r->is_array_sort());
		context *ctx = r->get_context();
		consts_struct_t const &cs = ctx->get_consts_struct();
		array_constant_ref arr_cons = r->get_array_constant();
		a = r;
		f = z_ctx->mk_array_const(arr_cons,it_meta->second.sort);
	}

	fn_value tmp(f,a);
	e.function_vals.insert(make_pair(mk_string_ref(name),tmp));

	if(as_array){
		ASSERT(a->is_array_sort());
		return a;
	}
	else
		return f;
}

expr_ref get_bool_bv_cons(context *ctx, string const &cons)
{
    if (cons == "True") {
      return ctx->mk_bool_const(true);
    } else if (cons == "False") {
      return ctx->mk_bool_const(false);
    } else {
	  //convert cons to expr_ref
	  pair<int,bv_const> a = str_to_bvlen_val_pair(cons);
	  expr_ref res = ctx->mk_bv_const(a.first,a.second);
	  return res;
    }
}

expr_ref solve_val(expr_ref v,int arity)
{

	expr::operation_kind type = v->get_operation_kind();
	expr_ref r;

	if(type == expr::OP_CONST)
		return v;
	else if(type == expr::OP_ITE){
		r = solve_ite(v,arity);
	}
	else if(type == expr::OP_AND){
		r = solve_and(v);
	}
	else if(type == expr::OP_OR){
		r = solve_or(v);
	}
	else if(type == expr::OP_XOR){
		r = solve_xor(v);
	}
	else if(type == expr::OP_NOT){
		r = solve_not(v);
	}
	else if(type == expr::OP_BVNOT){
		r = solve_bvnot(v);
	}
	else if(type == expr::OP_BVADD){
		r = solve_bvadd(v);
	}
	else if(type == expr::OP_BVMUL){
		r = solve_bvmul(v);
	}
	else if(type == expr::OP_BVSLE){
		r = solve_bvsle(v);
	}
	else if(type == expr::OP_BVCONCAT)
		r = solve_concat(v);
	else if(type == expr::OP_VAR)
	  r = solve_func(v->get_name()->get_str(), true);
  else if(type == expr::OP_SELECT)
	  r = solve_select(v);
  else NOT_REACHED();

	return r;
}

expr_ref solve_ite(expr_ref v,int arity)
{
	env& e = env_list.front();
	vector<expr_ref> ite_args = v->get_args();	//to get v1 & v2
	expr::operation_kind type = ite_args[2]->get_operation_kind();
	context *ctx = v->get_context();
	consts_struct_t const &cs = ctx->get_consts_struct();

	expr_ref v1 = solve_val(ite_args[1],arity);
	expr_ref v2 = solve_val(ite_args[2],arity);
	sort_ref arr_sort;
	array_constant_ref arr_cons;
	expr_ref r;

	vector<expr_ref> case_args = resolve_bool_expr(ite_args[0]);

	if(case_args.size() == 1 && case_args[0]->is_bool_sort())
	{
		bool val = case_args[0]->get_bool_value();
		if(val)
			return v1;
		else
			return v2;
	}

	e.z_sort_vector.clear();
	for(size_t i=0;i<case_args.size();i++)
		e.z_sort_vector.push_back(case_args[i]->get_sort());

	ASSERT(arity == (int)case_args.size());

	if(type != expr::OP_ITE)	//second arg is default value
	{
		arr_cons = z_ctx->mk_array_constant(map<vector<expr_ref>, expr_ref>(), v2);
		e.z_sort_range = v2->get_sort();
		arr_sort = z_ctx->mk_array_sort(e.z_sort_vector,e.z_sort_range);
	}

	else{
		arr_cons = v2->get_array_constant();
		arr_sort = v2->get_sort();
	}

	arr_cons = arr_cons->array_set_elem(case_args,v1);
	ASSERT(v1->get_operation_kind() != expr::OP_ITE);
	r = z_ctx->mk_array_const(arr_cons,arr_sort);
	return r;
}

expr_ref solve_concat(expr_ref v)
{
	vector<expr_ref> args = v->get_args();
	expr_ref v1 = solve_val(args[0],-1);
	expr_ref v2 = solve_val(args[1],-1);
	ASSERT(v1->is_bv_sort() && v2->is_bv_sort());
	vector<mybitset> bitset_args;
	bitset_args.push_back(v1->get_mybitset_value());
	bitset_args.push_back(v2->get_mybitset_value());
	mybitset tmp;
	mybitset r = tmp.mybitset_bvconcat(bitset_args);
	int size = v1->get_sort()->get_size()+v2->get_sort()->get_size();
	expr_ref res = z_ctx->mk_bv_const(size,r);
	return res;
}

expr_ref solve_not(expr_ref v)
{
	vector<expr_ref> args = v->get_args();
	expr_ref tmp = solve_val(args[0],-1);
	ASSERT(tmp->is_bool_sort());
	bool r = tmp->is_const_bool_true();
	expr_ref res = z_ctx->mk_bool_const(r);
	return res;
}

expr_ref solve_and(expr_ref v)
{
	vector<expr_ref> args = v->get_args();
	expr_ref tmp1 = solve_val(args[0],-1);
	expr_ref tmp2 = solve_val(args[1],-1);
	ASSERT(tmp1->is_bool_sort() && tmp2->is_bool_sort());
	bool r1 = tmp1->is_const_bool_true();
	bool r2 = tmp2->is_const_bool_true();
	bool r = r1 & r2;
	expr_ref res = z_ctx->mk_bool_const(r);
	return res;
}

expr_ref solve_or(expr_ref v)
{
	vector<expr_ref> args = v->get_args();
	expr_ref tmp1 = solve_val(args[0],-1);
	expr_ref tmp2 = solve_val(args[1],-1);
	ASSERT(tmp1->is_bool_sort() && tmp2->is_bool_sort());
	bool r1 = tmp1->is_const_bool_true();
	bool r2 = tmp2->is_const_bool_true();
	bool r = r1 | r2;
	expr_ref res = z_ctx->mk_bool_const(r);
	return res;
}

expr_ref solve_xor(expr_ref v)
{
	vector<expr_ref> args = v->get_args();
	expr_ref tmp1 = solve_val(args[0],-1);
	expr_ref tmp2 = solve_val(args[1],-1);
	ASSERT(tmp1->is_bool_sort() && tmp2->is_bool_sort());
	bool r1 = tmp1->is_const_bool_true();
	bool r2 = tmp2->is_const_bool_true();
	bool r = r1 ^ r2;
	expr_ref res = z_ctx->mk_bool_const(r);
	return res;
}

expr_ref solve_bvnot(expr_ref v)
{
	vector<expr_ref> args = v->get_args();
	expr_ref tmp = solve_val(args[0],-1);
	ASSERT(tmp->is_bv_sort());
	mybitset r = tmp->get_mybitset_value();
	expr_ref res = z_ctx->mk_bv_const(tmp->get_sort()->get_size(), ~r);
	return res;
}

expr_ref solve_bvadd(expr_ref v)
{
	vector<expr_ref> args = v->get_args();
	expr_ref tmp1 = solve_val(args[0],-1);
	expr_ref tmp2 = solve_val(args[1],-1);
	ASSERT(tmp1->is_bv_sort() && tmp2->is_bv_sort());
	mybitset r1 = tmp1->get_mybitset_value();
	mybitset r2 = tmp2->get_mybitset_value();
	mybitset r = r1+r2;
	expr_ref res = z_ctx->mk_bv_const(tmp1->get_sort()->get_size(), r);
	return res;
}

expr_ref solve_bvmul(expr_ref v)
{
	vector<expr_ref> args = v->get_args();
	expr_ref tmp1 = solve_val(args[0],-1);
	expr_ref tmp2 = solve_val(args[1],-1);
	ASSERT(tmp1->is_bv_sort() && tmp2->is_bv_sort());
	mybitset r1 = tmp1->get_mybitset_value();
	mybitset r2 = tmp2->get_mybitset_value();
	mybitset r = r1*r2;
	expr_ref res = z_ctx->mk_bv_const(tmp1->get_sort()->get_size(), r);
	return res;
}

expr_ref solve_bvsle(expr_ref v)
{
	vector<expr_ref> args = v->get_args();
	expr_ref tmp1 = solve_val(args[0],-1);
	expr_ref tmp2 = solve_val(args[1],-1);
	ASSERT(tmp1->is_bv_sort() && tmp2->is_bv_sort());
	mybitset r1 = tmp1->get_mybitset_value();
	mybitset r2 = tmp2->get_mybitset_value();
	bool r = r1.bvsle(r2);
	expr_ref res = z_ctx->mk_bool_const(r);
	return res;
}

expr_ref solve_select(expr_ref v)
{
	//cout << __func__ << endl;
	vector<expr_ref> args = v->get_args();
	bool case_select =  args[4]->get_bool_value();
	array_constant_ref arr_cons;

	if(case_select){
		expr_ref tmp = solve_val(args[0]->get_array_constant()->get_default_value().get_default_value(),-1);
		arr_cons = tmp->get_array_constant();
	}
	else{
		expr_ref tmp = solve_val(args[0],-1);
		arr_cons = tmp->get_array_constant();
	}
	vector<expr_ref> key;
	key.push_back(solve_val(args[2],-1));
	expr_ref res = arr_cons->array_select(key,1,false);
	return res;
}

//returns vector of values as a key or a bool value
vector< pair<string,expr_ref> > solve_bool(expr_ref b)
{
	vector< pair<string,expr_ref> > t;
	vector<expr_ref> bool_expr_args = b->get_args();
	expr::operation_kind type = b->get_operation_kind();

	if(type == expr::OP_AND)
	{
		vector< pair<string,expr_ref> > tmp1 = solve_bool(bool_expr_args[0]);
		vector< pair<string,expr_ref> > tmp2 = solve_bool(bool_expr_args[1]);
		t.insert(t.end(), tmp1.begin(), tmp1.end());
		t.insert(t.end(), tmp2.begin(), tmp2.end());
	}
	else if(type == expr::OP_EQ)
	{
		expr_ref arg1 = bool_expr_args[0];
		string const &name = arg1->get_name()->get_str();
		bool is_arg = arg1->is_var() && name.size()>2 && ((name.substr(0,2)).compare("x!") == 0);

		expr_ref arg2 = solve_val(bool_expr_args[1],-1);
		if(is_arg){
			t.push_back(make_pair(name,arg2));
		}
		else
		{
			expr_ref val_arg1 = solve_val(arg1,-1);
			bool val = (*val_arg1 == *arg2);
			expr_ref res = z_ctx->mk_bool_const(val);
			t.push_back(make_pair("",res));
		}
	}
	return t;
}


//returns vector of values as a key or a bool value
vector<expr_ref> resolve_bool_expr(expr_ref b)
{
	vector<pair<string,expr_ref> > args = solve_bool(b);

	bool is_bool_const = true;
	bool val_bool = true;

	//check if res evaluates to constant
	//resolves value if constant
	for(size_t i = 0; i< args.size();i++)
	{
		string name = args[i].first;
		bool is_arg = (name.size()>2) && ((name.substr(0,2)).compare("x!") == 0);
		if(is_arg)
			is_bool_const = false;
		else{
			ASSERT(is_bool_const);
			val_bool &= (args[i].second)->get_bool_value();
		}
	}

	vector<expr_ref> res;
	if(is_bool_const)
	{
		res.push_back(z_ctx->mk_bool_const(val_bool));
		return res;
	}

	res.resize(args.size());

	//resolves any reorderings
	for (size_t i = 0; i < args.size(); ++i)
	{
		//extract index from name
		string s = args[i].first;
		int index = atoi((s.substr(2)).c_str());
        if (index > 0){
          index--;
        } else {
          index += args.size() - 1;
        }
        ASSERT(index >= 0);
		res[index] = args[i].second;
	}
	return res;
}


