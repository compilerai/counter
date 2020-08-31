#include "parser/parse_tree_common.h"
#include "expr/context.h"

using namespace std;

namespace eqspace {

pair<int,bv_const> str_to_bvlen_val_pair(string const& cons)
{
  if (cons.substr(0, 2) == "#x") {
	  return make_pair(4*(cons.size()-2), bv_const(cons.c_str()+2, 16)); //mybitsetstrtoull(cons.c_str()+2, nullptr, 16));
  } else if (cons.substr(0, 2) == "#b") {
	  return make_pair(cons.size()-2, bv_const(cons.c_str()+2,2)); //strtoull(cons.c_str()+2, nullptr, 2));
  } else {
    cout << __func__ << " " << __LINE__ << ": cons = " << cons << endl;
    NOT_IMPLEMENTED();
  }
}

static expr_ref collect_cond_else_pairs_for_ite_ladder(expr_ref const& e, map<expr_ref,expr_ref>& cond_else_pairs)
{
  // base case -- e is constant
  context* ctx = e->get_context();
  if (e->get_operation_kind() != expr::OP_ITE) {
    ASSERT(e->is_const());
    return e;
  }

  ASSERT (e->get_operation_kind() == expr::OP_ITE);

  expr_ref const& cond = e->get_args().at(0);
  expr_ref const& then_expr = e->get_args().at(1);
  expr_ref const& else_expr = e->get_args().at(2);

  ASSERT(else_expr->is_const());
  cond_else_pairs.insert(make_pair(cond, else_expr));
  return collect_cond_else_pairs_for_ite_ladder(then_expr, cond_else_pairs);
}

static void get_name_value_pairs(expr_ref const& e, map<string,expr_ref>& ret)
{
  auto op = e->get_operation_kind();
  if (op == expr::OP_EQ) {
    ASSERT(e->get_args().size() == 2);
    auto arg1 = e->get_args()[0];
    auto arg2 = e->get_args()[1];
    if (arg1->is_var()) {
      // the other must be const
      ASSERT(arg2->is_const());
      ret[arg1->get_name()->get_str()] = arg2;
    }
    else if(arg2->is_var()) {
      // the other must be const
      ASSERT(arg1->is_const());
      ret[arg2->get_name()->get_str()] = arg1;
    }
    else {
      cout << __func__ << ':' << __LINE__ << " at least one argument must be var\n";
      ASSERT(false);
    }
  }
  else if (op == expr::OP_AND) {
    for (auto a : e->get_args()) {
      get_name_value_pairs(a, ret);
    }
  }
  else if (op == expr::OP_VAR) {
    // must be bool
    ASSERT(e->is_bool_sort());
    ret[e->get_name()->get_str()] = e->get_context()->mk_bool_const(true);
  }
  else {
    cout << __func__ << ':' << __LINE__ << " unexpected operation kind for expr: " << expr_string(e) << endl;
    ASSERT(false);
  }

  ASSERT(ret.size());
}

static vector<expr_ref> create_param_assignment_from_name_value_map(map<string,expr_ref> const& name_value_map, vector<expr_ref> const& params)
{
  vector<expr_ref> ret;
  ASSERT(name_value_map.size() == params.size());
  for (auto const& p : params) {
    auto const& pname = p->get_name()->get_str();
    auto it = name_value_map.find(pname);
    if (it == name_value_map.end()) {
      cout << __func__ << ':' << __LINE__ << ": " << pname << " param not found in ite cond" << endl;
      ASSERT(false);
    }
    ret.push_back(it->second);
  }
  return ret;
}

static expr_ref solve_ite_ladder(expr_ref const& e, vector<sort_ref> const& dom, sort_ref range, vector<expr_ref>const& params)
{
  /* We support two kinds of ITE ladders:
     type 1: cond expr contains conjunction of all params
     type 2: cond expr contains only one param and ITE is used for creating conjunction of params.
     In this case, we require that the all else clauses have same value
     Note that the first type is subsumed by second if # of params = 1
   */
  // base case
  context* ctx = e->get_context();
  sort_ref arr_sort = ctx->mk_array_sort(dom, range);
  if (e->get_operation_kind() != expr::OP_ITE) {
    ASSERT(e->is_const());
    ASSERT(e->get_sort() == range);

    return ctx->mk_array_const_with_def(arr_sort, e);
  }

  ASSERT (e->get_operation_kind() == expr::OP_ITE);

  expr_ref const& cond = e->get_args().at(0);
  expr_ref const& then_expr = e->get_args().at(1);
  expr_ref const& else_expr = e->get_args().at(2);

  map<string,expr_ref> name_value_map;
  get_name_value_pairs(cond, name_value_map);
  if (name_value_map.size() == params.size()) {
    // type 1
    vector<expr_ref> const& true_case_args = create_param_assignment_from_name_value_map(name_value_map, params);
    ASSERT(then_expr->is_const());
    ASSERT(then_expr->get_sort() == range); // `then` cannot be array
    /* recursively solve else case */
    expr_ref const& else_expr_solved = solve_ite_ladder(else_expr, dom, range, params);

    /* add then case to array constant */
    array_constant_ref else_array_const = else_expr_solved->get_array_constant();
    else_array_const = else_array_const->array_set_elem(true_case_args, then_expr);
    return ctx->mk_array_const(else_array_const, arr_sort);
  } else if (name_value_map.size() == 1) {
    // type 2
    map<expr_ref,expr_ref> cond_else_pairs;
    expr_ref const& tail_then_expr = collect_cond_else_pairs_for_ite_ladder(e, cond_else_pairs);
    ASSERT(cond_else_pairs.size() == params.size());

    expr_ref else_expr_solved;
    map<string,expr_ref> name_value_ladder_map;
    for (auto const& p : cond_else_pairs) {
      if (!else_expr_solved) {
        else_expr_solved = p.second;
      }
      ASSERT(else_expr_solved == p.second);
      get_name_value_pairs(p.first, name_value_ladder_map);
    }
    vector<expr_ref> const& true_case_args = create_param_assignment_from_name_value_map(name_value_ladder_map, params);
    array_constant_ref else_array_const = ctx->mk_array_const_with_def(arr_sort, else_expr_solved)->get_array_constant();
    else_array_const = else_array_const->array_set_elem(true_case_args, tail_then_expr);
    return ctx->mk_array_const(else_array_const, arr_sort);
  } else {
    cout << __func__ << ':' << __LINE__ << "name_value_map.size() == " << name_value_map.size() << endl;
    NOT_IMPLEMENTED();
  }
}

expr_ref resolve_params(expr_ref const& e, vector<sort_ref> const& dom, sort_ref range, vector<expr_ref>const& params)
{
  context* ctx = e->get_context();
  sort_ref target_sort = ctx->mk_array_sort(dom, range);
  if (e->is_const()) {
    if (e->get_sort() == target_sort) {
      /* Nothing to be done */
      return e;
    } else if(e->get_sort() == range){
      return ctx->mk_array_const_with_def(target_sort, e);
    } else NOT_REACHED();
  }
  else{
    /* now e can only have top level ITE */
    ASSERT(e->get_operation_kind() == expr::OP_ITE);
    return solve_ite_ladder(e, dom, range, params);
  }
}

}
