#include "parser/parse_tree_common.h"
#include "expr/context.h"
#include "expr/range.h"
#include "support/dyn_debug.h"
#include "support/utils.h"

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
    if (!e->is_const()) {
      cout << _FNLN_ << ": e =\n" << ctx->expr_to_string_table(e) << endl;
    }
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
      cout << __func__ << ':' << __LINE__ << " arg1: " << expr_string(arg1) << endl;
      cout << __func__ << ':' << __LINE__ << " arg2: " << expr_string(arg2) << endl;
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
  else if (op == expr::OP_NOT) {
    // must be bool
    ASSERT(e->is_bool_sort());
    ASSERT(e->get_args().front()->is_var());
    ret[e->get_args().front()->get_name()->get_str()] = e->get_context()->mk_bool_false();
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
static bool cond_is_range_type(expr_ref const& cond)
{
  if (cond->get_operation_kind() == expr::OP_BVULE)
    return true;
  if (cond->get_operation_kind() == expr::OP_EQ) {
    expr_ref arg1 = cond->get_args().at(0);
    expr_ref arg2 = cond->get_args().at(1);
    if (   arg1->get_operation_kind() == expr::OP_APPLY
        && arg2->is_const()) {
      ASSERT(arg1->get_args().at(0)->is_const()); // array is const
      return true;
    }
    if (   arg2->get_operation_kind() == expr::OP_APPLY
        && arg1->is_const()) {
      ASSERT(arg2->get_args().at(0)->is_const()); // array is const
      return true;
    }
  }
  return false;
}

static array_constant_ref
solve_ite_ladder(expr_ref const& e, vector<expr_ref>const& params, pair<expr_ref,expr_ref> const& dom_val_range);

static bool
put_op_eq_at_end_sort_fn(expr_ref const& a, expr_ref const& b)
{
  if (a->get_operation_kind() == b->get_operation_kind()) return a < b;
  if (b->get_operation_kind() == expr::OP_EQ) return true;
  if (a->get_operation_kind() == expr::OP_EQ) return false;
  return a < b;
}

static list<pair<pair<expr_ref,expr_ref>,expr_ref>>
get_range_val_pairs_from_cond(expr_ref const& cond, pair<expr_ref,expr_ref> const& in_range, expr_ref& then_expr, expr_ref& else_expr, expr_ref const& param_var)
{
  context* ctx = cond->get_context();
  list<pair<pair<expr_ref,expr_ref>,expr_ref>> ret;
  switch (cond->get_operation_kind()) {
    case expr::OP_BVULE: {
      expr_ref arg1 = cond->get_args().at(0);
      expr_ref arg2 = cond->get_args().at(1);
      expr::operation_kind arg1_opk = arg1->get_operation_kind();
      expr::operation_kind arg2_opk = arg2->get_operation_kind();
      if (   ((arg1->is_var() || arg1_opk == expr::OP_BVEXTRACT) && arg2->is_const())
          || ((arg2->is_var() || arg2_opk == expr::OP_BVEXTRACT) && arg1->is_const())) {
        bool inverted = false; // default: c1 <= x1, inverted: x1 <= c1
        if      (arg1->is_const()) { ASSERT(arg2->is_var() || arg2_opk == expr::OP_BVEXTRACT); }
        else if (arg2->is_const()) { ASSERT(arg1->is_var() || arg1_opk == expr::OP_BVEXTRACT); swap(arg1, arg2); inverted = true; }
        else    { NOT_REACHED(); }
        // (ite (bvule c1 ((_ bvextract U L) x1))) OR (ite (bvule c1 x1) ... ...)
        // construct corresponding range, intersection with in_range => then_expr
        //                              , difference => else_expr
        unsigned bvsize = param_var->get_sort()->get_size();
        vector<mybitset> minval_v, maxval_v;
        if (arg2_opk == expr::OP_BVEXTRACT) {
          ASSERT(arg2->get_args().at(0) == param_var);
          unsigned u = arg2->get_args().at(1)->get_int_value();
          unsigned l = arg2->get_args().at(2)->get_int_value();
          if (inverted) { // x1 <= c1
            if (bvsize-1 > u) {
              mybitset zeros(0, bvsize-1-u);
              mybitset single_val_max = mybitset::mybitset_bvconcat({ zeros, ~mybitset(0, u+1) });
              if (!in_range.second->get_mybitset_value().bvule(single_val_max)) {
                // a new range has to be created for each possibility of upper bits
                cout << "in_range max = " << expr_string(in_range.second) << "; single_val_max = " << single_val_max << endl;
                NOT_IMPLEMENTED();
              }
              // we can only have 0s in upper bits
              maxval_v.push_back(zeros);
            }
            maxval_v.push_back(arg1->get_mybitset_value());
            if (l > 0) {
              mybitset zeros(0, l);
              maxval_v.push_back(~zeros);
            }
            minval_v.push_back(in_range.first->get_mybitset_value());
          } else {        // c1 <= x1
            if (bvsize-1 > u) {
              mybitset zeros(0, bvsize-1-u);
              mybitset single_val_min = mybitset::mybitset_bvconcat({ ~zeros, mybitset(0, u+1) });
              if (!in_range.first->get_mybitset_value().bvuge(single_val_min)) {
                // a new range has to be created for each possibility of upper bits
                cout << "in_range min = " << expr_string(in_range.first) << "; single_val_min = " << single_val_min << endl;
                NOT_IMPLEMENTED();
              }
              // we can only have 1s in upper bits
              minval_v.push_back(~zeros);
            }
            minval_v.push_back(arg1->get_mybitset_value());
            if (l > 0) {
              mybitset zeros(0, l);
              minval_v.push_back(zeros);
            }
            maxval_v.push_back(in_range.second->get_mybitset_value());
          }
        } else {
          if (inverted) { // x1 <= c1;
            ASSERT(arg2 == param_var);
            minval_v.push_back(in_range.first->get_mybitset_value());
            maxval_v.push_back(arg1->get_mybitset_value());
          } else {        // c1 <= x1
            ASSERT(arg2 == param_var);
            minval_v.push_back(arg1->get_mybitset_value());
            maxval_v.push_back(in_range.second->get_mybitset_value());
          }
        }
        mybitset minval = mybitset::mybitset_bvconcat(minval_v);
        mybitset maxval = mybitset::mybitset_bvconcat(maxval_v);
        auto this_range_r = minval.bvule(maxval) ? range_t::range_from_mybitset(minval, maxval) : range_t::empty_range(bvsize);
        auto in_range_r   = range_t::range_from_expr_pair(in_range);
        auto then_range =  in_range_r.range_intersect(this_range_r);
        auto else_ranges = in_range_r.range_difference(this_range_r);
        if (!then_range.range_is_empty()) {
          ret.push_back(make_pair(then_range.expr_pair_from_range(ctx), then_expr));
        }
        for (auto const& er : else_ranges) {
          ret.push_back(make_pair(er.expr_pair_from_range(ctx), else_expr));
        }
        return ret;
      }
      else {
        cerr << __func__ << ':' << __LINE__ << ": arg1 = " << expr_string(arg1) << endl;
        cerr << __func__ << ':' << __LINE__ << ": arg2 = " << expr_string(arg2) << endl;
        NOT_IMPLEMENTED();
      }
    }
    break;
  case expr::OP_EQ: {
      expr_ref arg1 = cond->get_args().at(0);
      expr_ref arg2 = cond->get_args().at(1);
      expr::operation_kind arg1_opk = arg1->get_operation_kind();
      expr::operation_kind arg2_opk = arg2->get_operation_kind();
      if (   (arg1_opk == expr::OP_APPLY || arg1_opk == expr::OP_ITE)
          || (arg2_opk == expr::OP_APPLY || arg2_opk == expr::OP_ITE)) {
        if      (arg1->is_const()) { ASSERT(arg2_opk == expr::OP_APPLY || arg2_opk == expr::OP_ITE); swap(arg1, arg2); }
        else if (arg2->is_const()) { ASSERT(arg1_opk == expr::OP_APPLY || arg1_opk == expr::OP_ITE); }
        else    { NOT_REACHED(); }
        // (ite (= (f x1) c2) ... ...)
        // find all addr ranges in arg1 whose value equal c2
        vector<pair<expr_ref,expr_ref>> equal_ranges;
        if (arg1_opk == expr::OP_APPLY) {
          ASSERT(arg1->get_args().at(1) == param_var);
          equal_ranges = arg1->get_args().at(0)->get_array_constant()->addrs_where_mapped_value_equals(arg2);
        } else {
          array_constant_ref ite_ac = solve_ite_ladder(arg1, { param_var }, in_range);
          equal_ranges = ite_ac->addrs_where_mapped_value_equals(arg2);
        }
        range_t in_rng = range_t::range_from_expr_pair(in_range);
        unsigned bvsize = in_range.first->get_sort()->get_size();
        vector<range_t> then_ranges;
        for (auto const& re : equal_ranges) {
          range_t intx = in_rng.range_intersect(range_t::range_from_expr_pair(re));
          if (intx.range_is_empty())
            continue;
          ret.push_back(make_pair(
                make_pair(ctx->mk_bv_const(bvsize, intx.front()),
                          ctx->mk_bv_const(bvsize, intx.back())),
                then_expr));
          then_ranges.push_back(intx);
        }
        auto else_ranges = in_rng.range_difference(then_ranges);
        for (auto const& er : else_ranges) {
          ret.push_back(make_pair(
                make_pair(ctx->mk_bv_const(bvsize, er.front()),
                          ctx->mk_bv_const(bvsize, er.back())),
                else_expr));

        }
        return ret;
      } else if (   ((arg1->is_var() || arg1_opk == expr::OP_BVEXTRACT) && arg2->is_const())
                 || ((arg2->is_var() || arg2_opk == expr::OP_BVEXTRACT) && arg1->is_const())) {
        if      (arg1->is_const()) { ASSERT(arg2->is_var() || arg2_opk == expr::OP_BVEXTRACT); }
        else if (arg2->is_const()) { ASSERT(arg1->is_var() || arg1_opk == expr::OP_BVEXTRACT); swap(arg1, arg2); }
        else    { NOT_REACHED(); }
        // (ite (= c1 ((_ extract U L) x1)) ... ...) OR (ite (= c1 x1) ... ...)
        // construct corresponding range, intersection with in_range => then_expr
        //                              , difference => else_expr
        unsigned bvsize = param_var->get_sort()->get_size();
        vector<mybitset> minval_v, maxval_v;
        if (arg2_opk == expr::OP_BVEXTRACT) {
          ASSERT(arg2->get_args().at(0) == param_var);
          unsigned u = arg2->get_args().at(1)->get_int_value();
          unsigned l = arg2->get_args().at(2)->get_int_value();
          if (bvsize-1 > u) {
            NOT_IMPLEMENTED();
          }
          minval_v.push_back(arg1->get_mybitset_value());
          maxval_v.push_back(arg1->get_mybitset_value());
          if (l > 0) {
            mybitset zeros(0, l);
            minval_v.push_back(zeros);
            maxval_v.push_back(~zeros);
          }
        } else {
          ASSERT(arg2 == param_var);
          minval_v.push_back(arg1->get_mybitset_value());
          maxval_v.push_back(arg1->get_mybitset_value());
        }
        mybitset minval = mybitset::mybitset_bvconcat(minval_v);
        mybitset maxval = mybitset::mybitset_bvconcat(maxval_v);
        auto this_range_r = minval.bvule(maxval) ? range_t::range_from_mybitset(minval, maxval) : range_t::empty_range(bvsize);
        auto in_range_r   = range_t::range_from_expr_pair(in_range);
        auto then_range =  in_range_r.range_intersect(this_range_r);
        auto else_ranges = in_range_r.range_difference(this_range_r);
        if (!then_range.range_is_empty()) {
          ret.push_back(make_pair(then_range.expr_pair_from_range(ctx), then_expr));
        }
        for (auto const& er : else_ranges) {
          ret.push_back(make_pair(er.expr_pair_from_range(ctx), else_expr));
        }
        return ret;
      } else {
        cerr << __func__ << ':' << __LINE__ << ": OP_EQ: got unhandled args" << endl;
        cerr << __func__ << ':' << __LINE__ << ": arg1 = " << expr_string(arg1) << endl;
        cerr << __func__ << ':' << __LINE__ << ": arg2 = " << expr_string(arg2) << endl;
        NOT_IMPLEMENTED();
      }
    }
    break;
  case expr::OP_AND: {
      ASSERT(cond->get_args().size() >= 2);
      // we solve solve OP_AND recursively: pick one operand and recurse on the
      // remaining; with vector only pop_back() is available so we pick last element
      vector<expr_ref> args = cond->get_args();
      // sort so that OP_EQ is handled first; our implementation of OP_EQ is more
      // "complete" than others and resolving OP_EQ first narrows down the
      // available range and thereby increases the chance that we won't
      // trigger any unhandled edge case
      sort(args.begin(), args.end(), put_op_eq_at_end_sort_fn);
      // (ite (and cond1 cond2) then_expr else_expr)
      // rewrite to:
      // (ite (cond1) (ite (cond2) then_expr else_expr) else_expr)
      expr_ref cond1 = args.back();
      args.pop_back();
      auto ranges1 = get_range_val_pairs_from_cond(cond1, in_range, then_expr, else_expr, param_var);
      for (auto itr = ranges1.begin(); itr != ranges1.end(); ) {
        if (itr->second == else_expr) {
          // put else_expr ranges directly into ret
          ret.push_back(*itr);
          itr = ranges1.erase(itr);
        } else ++itr;
      }
      // for the remaining operands, recurse
      expr_ref const& remaining_cond = args.size() > 1 ? ctx->mk_and(args) : args.front();
      for (auto const& r : ranges1) {
        ASSERT(r.second == then_expr);
        auto ranges2 = get_range_val_pairs_from_cond(remaining_cond, r.first, then_expr, else_expr, param_var);
        list_append(ret, ranges2);
      }
      return ret;
    }
    break;
  case expr::OP_OR: {
      ASSERT(cond->get_args().size() >= 2);
      DYN_DEBUG(ce_parse, cout << __func__ << ": solving OP_OR " << expr_string(cond) << endl);

      // use same tricks as OP_AND
      vector<expr_ref> args = cond->get_args();
      sort(args.begin(), args.end(), put_op_eq_at_end_sort_fn);
      // (ite (or cond1 cond2) then_expr else_expr)
      // rewrite to:
      // (ite (cond2) then_expr (ite (cond1) then_expr else_expr))
      expr_ref const& cond1 = args.back();
      args.pop_back();
      expr_ref remaining_cond = args.size() > 1 ? ctx->mk_or(args) : args.front();
      expr_ref new_else_expr = ctx->mk_ite(remaining_cond, then_expr, else_expr);
      auto ranges1 = get_range_val_pairs_from_cond(cond1, in_range, then_expr, new_else_expr, param_var);
      for (auto const& r : ranges1) {
        ret.push_back(r);
      }
      return ret;
    }
    break;
  case expr::OP_NOT:
    return get_range_val_pairs_from_cond(cond->get_args().front(), in_range, else_expr, then_expr, param_var);
    break;
  default:
    cerr << __func__ << ':' << __LINE__ << ": cond = " << expr_string(cond) << endl;
    NOT_IMPLEMENTED();
  }
}

static array_constant_ref
solve_ite_ladder(expr_ref const& e, vector<expr_ref>const& params, pair<expr_ref,expr_ref> const& dom_val_range)
{
  /* We support two kinds of ITE ladders:
     type 1: cond expr contains conjunction of all params
     type 2: cond expr contains only one param and ITE is used for creating conjunction of params.  In this case, we require that all else clauses have same value
     Note that the first type is subsumed by second if # of params = 1

     For type 1, we further allow ranges in the condition
  */
  context* ctx = e->get_context();
  vector<sort_ref> dom_sort = vmap<expr_ref,sort_ref>(params, [](expr_ref const& e) { return e->get_sort(); });
  // base case
  if (e->get_operation_kind() != expr::OP_ITE) {
    ASSERT(e->is_const());
    return ctx->mk_array_constant(dom_sort, e);
  }

  ASSERT (e->get_operation_kind() == expr::OP_ITE);

  expr_ref const& cond = e->get_args().at(0);
  expr_ref then_expr = e->get_args().at(1);
  expr_ref else_expr = e->get_args().at(2);

  if (params.size() == 1 && params.front()->is_bv_sort()) {
    ASSERT(dom_val_range.first && dom_val_range.second);
    DYN_DEBUG(ce_parse, cout << "cond = " << expr_string(cond) << endl);
    list<pair<pair<expr_ref,expr_ref>,expr_ref>> ranges = get_range_val_pairs_from_cond(cond, dom_val_range, then_expr, else_expr, params.front());
    array_constant_ref ret_ac;
    for (auto it = ranges.begin(); it != ranges.end(); ++it) {
      array_constant_ref range_ac = solve_ite_ladder(it->second, params, it->first);
      ASSERT(range_ac);
      ret_ac = ret_ac ? ret_ac->apply_mask_and_overlay_array_constant(it->first, range_ac)
                      : range_ac;
    }
    ASSERT(ret_ac);
    return ret_ac;
  }
  else {
    map<string,expr_ref> name_value_map;
    get_name_value_pairs(cond, name_value_map);
    if (name_value_map.size() == params.size()) {
      // type 1: cond expr contains conjunction of all params
      vector<expr_ref> true_case_args = create_param_assignment_from_name_value_map(name_value_map, params);
      ASSERT(then_expr->is_const());
      /* recursively solve else case */
      array_constant_ref else_expr_ac = solve_ite_ladder(else_expr, params, dom_val_range);
      ASSERT(else_expr_ac);

      /* add then case to array constant */
      else_expr_ac = else_expr_ac->array_store(true_case_args, then_expr, 1, false);
      return else_expr_ac;
    }
    else if (name_value_map.size() == 1) {
      // type 2: cond expr contains only one param and ITE is used for creating conjunction of params.  In this case, we require that all else clauses have same value
      map<expr_ref,expr_ref> cond_else_pairs;
      expr_ref tail_then_expr = collect_cond_else_pairs_for_ite_ladder(e, cond_else_pairs);
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
      vector<expr_ref> true_case_args = create_param_assignment_from_name_value_map(name_value_ladder_map, params);
      array_constant_ref else_expr_ac = ctx->mk_array_constant(dom_sort, else_expr_solved);
      else_expr_ac = else_expr_ac->array_store(true_case_args, tail_then_expr, 1, false);
      return else_expr_ac;
    } else {
      cout << __func__ << ':' << __LINE__ << "name_value_map.size() == " << name_value_map.size() << endl;
      NOT_IMPLEMENTED();
    }
  }
}

static expr_ref
compose_ac_expr(expr_ref const& a, expr_ref const& b)
{
  ASSERT(a->is_array_sort() && b->is_array_sort());
  context* ctx = a->get_context();
  array_constant_ref a_ac = a->get_array_constant();
  array_constant_ref b_ac = b->get_array_constant();
  array_constant_ref ret = a_ac->compose(b_ac);
  ASSERT(ret);
  sort_ref ret_sort = ctx->mk_array_sort(a->get_sort()->get_domain_sort(), b->get_sort()->get_range_sort());
  return ctx->mk_array_const(ret, ret_sort);
}

static expr_ref
solve_composition(vector<expr_ref> const& comps)
{
  expr_ref ret = comps.front();
  for (auto itr = comps.begin()+1; itr != comps.end(); ++itr) {
    ret = compose_ac_expr(ret, *itr);
  }
  return ret;
}

static expr_ref
solve_apply(expr_ref const& e, sort_ref const& arr_sort, vector<expr_ref>const& params)
{
  ASSERT(e->get_operation_kind() == expr::OP_APPLY);
  if (params.size() > 1) {
    cout << _FNLN_ << ": cannot handle OP_APPLY with > 1 args: " << expr_string(e) << endl;
    NOT_REACHED();
  } else {
    // handle nested composition of apply(s)
    ASSERTCHECK((params.size() == 1), cout << __func__ << ": apply with > 1 arity not-supported" << endl);
    vector<expr_ref> comps;
    expr_ref ee = e;
    while (ee->get_operation_kind() == expr::OP_APPLY) {
      ASSERT(ee->get_args().size() == 2);
      comps.push_back(ee->get_args().at(0));
      ee = ee->get_args().at(1);
    }
    ASSERT(ee->is_var() && ee == params.front());
    reverse(comps.begin(), comps.end());
    expr_ref ret = solve_composition(comps);
    ASSERT(ret->is_array_sort() && ret->get_sort() == arr_sort);
    return ret;
  }
}

static expr_ref
solve_or(expr_ref const& e, sort_ref const& arr_sort, vector<expr_ref>const& params)
{
  ASSERT(e->get_operation_kind() == expr::OP_OR);
  ASSERT(arr_sort->get_range_sort()->is_bool_kind());

  // top-level boolean OR: (or (and ...) (and ...) (and ...) ...)
  // each operand must be AND with arity == params size.
  context* ctx = e->get_context();
  array_constant_ref ret_ac = ctx->mk_array_constant(arr_sort->get_domain_sort(), expr_false(ctx));
  for (auto const& arg : e->get_args()) {
    ASSERTCHECK((arg->get_operation_kind() == expr::OP_AND), cout << _FNLN_ << ": expected OP_AND, got " << expr_string(arg) << endl);
    map<string,expr_ref> name_value_map;
    get_name_value_pairs(arg, name_value_map);
    vector<expr_ref> true_case_args = create_param_assignment_from_name_value_map(name_value_map, params);
    ret_ac = ret_ac->array_store(true_case_args, expr_true(ctx), 1, false);
  }
  expr_ref ret = ctx->mk_array_const(ret_ac, arr_sort);
  return ret;
}

expr_ref resolve_params(expr_ref const& e, vector<sort_ref> const& dom, sort_ref const& range, vector<expr_ref>const& params, string const& name)
{
  context* ctx = e->get_context();
  expr_ref ret;
  sort_ref target_sort = ctx->mk_array_sort(dom, range);
  if (e->is_const()) {
    if (e->get_sort() == target_sort) {
      /* Nothing to be done */
      ret = e;
    } else if (e->get_sort() == range){
      ret = ctx->mk_array_const_with_def(target_sort, e);
    } else {
      cerr << __func__ << ':' << __LINE__ << ": unexpected e = " << expr_string(e) << endl;
      cerr << __func__ << ':' << __LINE__ << ": target_sort = " << target_sort->to_string() << endl;
      cerr << __func__ << ':' << __LINE__ << ": e->get_sort() = " << e->get_sort()->to_string() << endl;
      NOT_REACHED();
    }
  } else if (e->get_operation_kind() == expr::OP_ITE) {
    expr_ref dom_low, dom_high;
    if (dom.size() == 1 && dom.front()->is_bv_kind()) {
      dom_low  = ctx->mk_zerobv(dom.front()->get_size());
      dom_high = ctx->mk_minusonebv(dom.front()->get_size());
    }
    auto ret_ac = solve_ite_ladder(e, params, make_pair(dom_low, dom_high));
    ret = ctx->mk_array_const(ret_ac, target_sort);
  } else if (e->get_operation_kind() == expr::OP_APPLY) {
    ret = solve_apply(e, target_sort, params);
  } else if (e->get_operation_kind() == expr::OP_OR) {
    ret = solve_or(e, target_sort, params);
  } else {
    cout << _FNLN_ << ": unexpected top-level expr: " << expr_string(e) << endl;
    NOT_REACHED();
  }
  ASSERT(ret);
  ASSERT(ret->get_sort() == target_sort);
  return ret;
}

}
