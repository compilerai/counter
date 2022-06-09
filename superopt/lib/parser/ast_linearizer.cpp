#include <numeric>
#include <sstream>

#include "parser/ast_linearizer.h"
#include "expr/range_utils.h"
#include "expr/eval.h"
#include "support/dyn_debug.h"

namespace eqspace {

// Itp->first is range_t
template<typename Itp>
static
bool ranges_are_non_overlapping(Itp it_a, Itp it_a_end, Itp it_b, Itp it_b_end)
{
  while (   it_a != it_a_end
         && it_b != it_b_end) {
    if (it_a->first.comes_before(it_b->first))
      ++it_a;
    else if (it_b->first.comes_before(it_a->first))
      ++it_b;
    else {
      ASSERT(it_a->first.has_intersection(it_b->first));
      return false;
    }
  }
  return true;
}

ExprLinearLazy
ExprLinearLazy::union_with(ExprLinearLazy const& other) const
{
  if (other.is_empty())
    return *this;
  if (this->is_empty())
    return other;

  ASSERT(this->get_sort() == other.get_sort());

  if (!ranges_are_non_overlapping(this->m_vals.cbegin(), this->m_vals.cend(), other.m_vals.cbegin(), other.m_vals.cend())) {
    NOT_IMPLEMENTED();
  }

  ExprLinearLazy ret(*this);
  map_union(ret.m_vals, other.m_vals);
  return ret;
}

// assumes that mask_ranges are sorted and are sub-range of m_vals' ranges
ExprLinearLazy 
ExprLinearLazy::masked(vector<range_t> const& mask_ranges) const
{
  ASSERT(mask_ranges.size());
  ASSERT(this->m_vals.size());
  map<range_t,expr_ref> new_vals;
  auto vitr = this->m_vals.cbegin();
  auto vitr_end = this->m_vals.cend();
  for (auto const& mr : mask_ranges) {
    while (   vitr != vitr_end
           && vitr->first.comes_before(mr)) {
      ++vitr;
    }
    ASSERT(vitr != vitr_end);
    ASSERT(vitr->first <= mr);
    if (mr.is_subrange_of(vitr->first)) {
      new_vals.emplace(mr, vitr->second);
      continue;
    }
    // multiple m_vals ranges for single mask range
    auto isect = vitr->first.range_intersect(mr);
    ASSERT(!isect.range_is_empty());
    new_vals.emplace(isect,vitr->second);
    auto prev_vitr = vitr;
    while (   ++vitr != vitr_end
           && vitr->first.has_intersection(mr)) {
      ASSERT(vitr->first.is_in_immediate_neighbourhood_of(isect));
      isect = vitr->first.range_intersect(mr);
      ASSERT(!isect.range_is_empty());
      new_vals.emplace(isect,vitr->second);
      prev_vitr = vitr;
    }
    ASSERT(isect.end() == mr.end());
    if (mr.ends_before(prev_vitr->first)) {
      // prev_vitr still has valid range outside mr
      vitr = prev_vitr;
    }
  }
  return ExprLinearLazy(new_vals);
}

expr_ref
ExprLinearLazy::eval_to_eager() const
{
  if (m_cached_eager_expr)
    return m_cached_eager_expr;

  vector<range_t> val_ranges = map_project_domain(this->m_vals);
  if (!range_t::ranges_give_full_coverage(val_ranges)) {
    cout << _FNLN_ << ": ranges do not give full coverage! val_ranges:" << endl;
    for (auto const& r : val_ranges) {
      cout << r.to_string() << endl;
    }
    NOT_IMPLEMENTED();
  }

  context* ctx = this->get_context();
  map<pair<expr_ref,expr_ref>, expr_ref> range_mapping;
  for (auto const& [r,v] : this->m_vals) {
    range_mapping.emplace(r.expr_pair_from_range(ctx), v);
  }
  array_constant_ref ret_ac = ctx->mk_array_constant(range_mapping);
  m_cached_eager_expr = ctx->mk_array_const(ret_ac, this->get_sort());
  return m_cached_eager_expr;
}

string
ExprLinearLazy::to_string() const
{
  ostringstream os;
  for (auto const& [r,v] : this->m_vals) {
    os << r.to_string() << " -> " << expr_string(v) << ", ";
  }
  return os.str();
}

string
ExprDualLazy::to_string() const
{
  ostringstream os;
  os << "Normal form: " << expr_string(m_normal);
  if (has_linear_eager_form()) {
    os << "; Linear eager form: " << expr_string(get_linear_eager());
  } else if (has_linear_lazy_form()) {
    os << "; Linear lazy form: " << get_linear_lazy().to_string();
  }
  return os.str();
}

bool
short_circuit_to_undefined(vector<ExprDualLazy> const& args)
{
  for (auto const& arg : args)
    if (arg.normal_form()->is_undefined())
      return true;
  return false;
}

ExprDualLazy
AstLinearizer::process_op(expr::operation_kind const& opk, vector<ExprDualLazy> const& args)
{
  ASSERT(args.size());
  if (short_circuit_to_undefined(args)) {
    return ExprDualLazy::normal(m_ctx->mk_undefined());
  }
  // lossy application -- discards linear form
  expr_vector args_e = vmap<ExprDualLazy,expr_ref>(args, [](ExprDualLazy const& ed) { return ed.normal_form(); });
  expr_ref ret = nullptr;
  if (opk == expr::OP_SELECT) {
    ASSERT(args_e.size() == 2);
    expr_ref const& mem = args_e.at(0);
    expr_ref mem_alloc = get_dummy_mem_alloc_expr(m_ctx->mk_array_sort(mem->get_sort()->get_domain_sort(), m_ctx->mk_memlabel_sort()));
    memlabel_t ml = memlabel_t::memlabel_top();
    ret = m_ctx->mk_select(mem, mem_alloc, ml, args_e.at(1), /*count*/1, /*endianness*/false);
  } else if (opk == expr::OP_STORE) {
    ASSERT(args_e.size() == 3);
    expr_ref const& mem = args_e.at(0);
    expr_ref mem_alloc = get_dummy_mem_alloc_expr(m_ctx->mk_array_sort(mem->get_sort()->get_domain_sort(), m_ctx->mk_memlabel_sort()));
    memlabel_t ml = memlabel_t::memlabel_top();
    ret = m_ctx->mk_store(mem, mem_alloc, ml, args_e.at(1), args_e.at(2), /*count*/1, /*endianness*/false);
  } else if (   opk == expr::OP_EQ
             && args[0].has_linear_form()
             && args[1].has_linear_form()) {
    ASSERT(args.size() == 2);
    auto arg0 = args[0].linear_form();
    auto arg1 = args[1].linear_form();
    //cout << _FNLN_ << ": OP_EQ: " << expr_string(args_e[0]) << ", " << expr_string(args_e[1]) << endl;
    //cout << "arg0 = " << expr_string(arg0) << endl;
    //cout << "arg1 = " << expr_string(arg1) << endl;
    if (arg0->is_array_sort() ^ arg1->is_array_sort()) {
      auto [ac_e,const_e] = swap_if_false(arg0->is_array_sort(), arg0, arg1);
      auto ret_lf_ac = ac_e->get_array_constant()->array_constant_map_value([ctx=m_ctx,c=const_e](expr_ref const& v) -> expr_ref {
          if (v == c) return ctx->mk_bool_true();
                 else return ctx->mk_bool_false();
        });
      sort_ref ret_lf_sort = m_ctx->mk_array_sort(ac_e->get_sort()->get_domain_sort(), m_ctx->mk_bool_sort());
      expr_ref ret_lf = m_ctx->mk_array_const(ret_lf_ac, ret_lf_sort);
      if (ret_lf_ac->is_non_rac_default_array_constant()) {
        // resolved to single value
        ret = ret_lf_ac->get_default_value();
      } else {
        ret = m_ctx->mk_app(opk, args_e);
      }
      return ExprDualLazy::dual(ret, ret_lf);
    }
  } else if (   opk == expr::OP_NOT
             && args[0].has_linear_form()) {
    ASSERT(args.size() == 1);
    auto arg0 = args[0].linear_form();
    //cout << _FNLN_ << ": OP_NOT: " << expr_string(args_e[0]) << endl;
    //cout << "arg0 = " << expr_string(arg0) << endl;
    if (arg0->is_array_sort()) {
      auto ret_lf_ac = arg0->get_array_constant()->array_constant_map_value([ctx=m_ctx](expr_ref const& v) {
          if (v->get_bool_value()) return ctx->mk_bool_false();
                              else return ctx->mk_bool_true();
          });
      sort_ref ret_lf_sort = m_ctx->mk_array_sort(arg0->get_sort()->get_domain_sort(), m_ctx->mk_bool_sort());
      expr_ref ret_lf = m_ctx->mk_array_const(ret_lf_ac, ret_lf_sort);
      if (ret_lf_ac->is_non_rac_default_array_constant()) {
        // resolved to single value
        ret = ret_lf_ac->get_default_value();
      } else {
        ret = m_ctx->mk_app(opk, args_e);
      }
      return ExprDualLazy::dual(ret, ret_lf);
    }
  }
  if (!ret) {
    ret = m_ctx->mk_app(opk, args_e);
  }
  ret = expr_evaluates_to_constant(ret);
  return ExprDualLazy::dual_if_const(ret);
}

ExprDualLazy
AstLinearizer::lookup_name(string_ref const& name) const
{
  if (auto r = lookup_letdecl(name)) { return *r; }
  if (auto r = lookup_lambda_param(name)) { return ExprDualLazy::normal(r); }
  if (auto r = lookup_func(name)) { return *r; }
  return ExprDualLazy::normal(m_ctx->mk_undefined());
}

static expr_ref
compose_array_constant_expr_pair(expr_ref const& a, expr_ref const& b)
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
compose_array_constant_exprs(vector<expr_ref> const& comps)
{
  expr_ref ret = comps.front();
  for (auto itr = comps.begin()+1; itr != comps.end(); ++itr) {
    ret = compose_array_constant_expr_pair(ret, *itr);
  }
  return ret;
}

ExprDualLazy
AstLinearizer::handle_apply(AstNodeRef const& ast)
{
  if (ast->get_args().size() == 1) {
    // can linearize composition of APPLYs on lambda param
    vector<string_ref> apply_nest_fns;
    AstNodeRef arg = ast;
    while (   arg->get_kind() == AstNode::APPLY
           && arg->get_args().size() == 1) {
      apply_nest_fns.push_back(arg->get_name());
      arg = arg->get_args().front();
    }
    expr_ref lambda_var;
    if (   arg->get_kind() == AstNode::VAR
        && (lambda_var = this->lookup_lambda_param(arg->get_name()))) {
      vector<expr_ref> fn_exprs;
      bool all_const = true;
      for (auto ritr = apply_nest_fns.rbegin(); ritr != apply_nest_fns.rend(); ++ritr) {
        ExprDualLazy fn_ed = this->lookup_name(*ritr);
        if (!fn_ed.has_linear_form()) {
          all_const = false;
          break;
        }
        fn_exprs.push_back(fn_ed.linear_form());
      }
      if (all_const) {
        expr_ref apply_ret = compose_array_constant_exprs(fn_exprs);
        if (apply_ret->is_array_sort()) {
          expr_ref ret_nf = apply_ret->get_array_constant()->array_constant_to_ite({ lambda_var });
          return ExprDualLazy::dual(ret_nf, apply_ret);
        } else {
          return ExprDualLazy::dual_if_const(apply_ret);
        }
      }
    }
  }
  ExprDualLazy func_ed = this->lookup_name(ast->get_name());
  vector<ExprDualLazy> args = { func_ed };
  vector<expr_ref> app_args;
  bool all_linear = func_ed.has_linear_form();
  for (auto const& arg : ast->get_args()) {
    ExprDualLazy argl = this->linearize(arg);
    all_linear = all_linear && argl.has_linear_form();
    if (all_linear) {
      app_args.push_back(argl.linear_form());
    }
    args.push_back(argl);
  }
  if (all_linear) {
    // if func is linear then use array ops
    ASSERT(app_args.size() == args.size()-1);
    expr_ref func_l = func_ed.linear_form();
    ASSERT(func_l->is_const());
    ASSERT(func_l->is_array_sort());
    expr_ref ret = func_l->get_array_constant()->array_select(app_args, 1, false);
    return ExprDualLazy::dual_if_const(ret);
  }
  return process_op(expr::OP_APPLY, args);
}

// var_val_ranges are assumed to be pairwise disjoint and in sorted order
optional<vector<range_t>>
get_valid_ranges_for_var_from_boolean_expr_subject_to_value_ranges(expr_ref const& var, expr_ref const& cond, vector<range_t> const& var_val_ranges)
{
  ASSERT(var->is_var());
  ASSERT(var->is_bv_sort());
  ASSERT(cond->is_bool_sort());
  ASSERT(var_val_ranges.size());
  context* ctx = var->get_context();
  if (cond->get_operation_kind() == expr::OP_BVULE) {
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
      // (bvule c1 ((_ bvextract U L) x1)) OR (bvule c1 x1)
      // construct corresponding range, intersection with var_val_ranges => true ranges
      //                              , difference                       => false ranges
      unsigned bvsize = var->get_sort()->get_size();
      vector<mybitset> minval_v, maxval_v;
      if (arg2_opk == expr::OP_BVEXTRACT) {
        ASSERT(arg2->get_args().at(0) == var);
        unsigned u = arg2->get_args().at(1)->get_int_value();
        unsigned l = arg2->get_args().at(2)->get_int_value();
        if (inverted) { // x1 <= c1
          if (u < bvsize-1) {
            mybitset zeros(0,bvsize-1-u);
            mybitset single_val_max = mybitset::mybitset_bvconcat({ zeros, ~mybitset::mybitset_zero(u+1) });
            if (!var_val_ranges.back().back().bvule(single_val_max)) {
              // a new range has to be created for each possibility of upper bits -- return failure
              return {};
            }
            // we can only have 0s in upper bits
            maxval_v.push_back(zeros);
          }
          maxval_v.push_back(arg1->get_mybitset_value());
          if (l > 0) {
            // 1s in lower bits of maximum possible value
            maxval_v.push_back(~mybitset(0, l));
          }
          minval_v.push_back(var_val_ranges.front().front());
        } else {        // c1 <= x1
          if (u < bvsize-1) {
            mybitset ones = ~mybitset(0, bvsize-1-u);
            mybitset single_val_min = mybitset::mybitset_bvconcat({ ones, mybitset(0, u+1) });
            if (!var_val_ranges.front().front().bvuge(single_val_min)) {
              // a new range has to be created for each possibility of upper bits -- return failure
              return {};
            }
            // we can only have 1s in upper bits
            minval_v.push_back(ones);
          }
          minval_v.push_back(arg1->get_mybitset_value());
          if (l > 0) {
            mybitset zeros(0, l);
            minval_v.push_back(zeros);
          }
          maxval_v.push_back(var_val_ranges.back().back());
        }
      } else {
        ASSERT(arg2 == var);
        if (inverted) { // x1 <= c1;
          minval_v.push_back(var_val_ranges.front().front());
          maxval_v.push_back(arg1->get_mybitset_value());
        } else {        // c1 <= x1
          minval_v.push_back(arg1->get_mybitset_value());
          maxval_v.push_back(var_val_ranges.back().back());
        }
      }
      mybitset minval = mybitset::mybitset_bvconcat(minval_v);
      mybitset maxval = mybitset::mybitset_bvconcat(maxval_v);
      auto this_range_r = minval.bvule(maxval) ? range_t::range_from_mybitset(minval, maxval) : range_t::empty_range(bvsize);
      return range_intersection(var_val_ranges, this_range_r);
    }
  }
  else if (cond->get_operation_kind() == expr::OP_BVSLE) {
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
      // (bvsle c1 ((_ bvextract U L) x1)) OR (bvsle c1 x1)
      // construct corresponding range, intersection with var_val_ranges => true ranges
      //                              , difference                       => false ranges
      unsigned bvsize = var->get_sort()->get_size();
      range_vector this_ranges;
      if (arg2_opk == expr::OP_BVEXTRACT) {
        // not implemented yet
        return nullopt;
      } else {
        auto const& c_mbs = arg1->get_mybitset_value();
        auto min_neg_val = mybitset::mybitset_int_min(bvsize);
        auto zero_val    = mybitset::mybitset_zero(bvsize);
        ASSERT(arg2 == var);
        if (inverted) { // x1 <=_s c1;
          auto minval = c_mbs.is_neg() ? min_neg_val
                                       : zero_val;
          auto maxval = c_mbs;
          this_ranges.push_back(range_t::range_from_mybitset(minval, maxval));
          if (!c_mbs.is_neg()) { // add the full negative range as well
            this_ranges.push_back(range_t::range_from_mybitset(min_neg_val, mybitset(-1,bvsize)));
          }
        } else {        // c1 <=_s x1
          auto max_pos_val = min_neg_val.dec();
          auto minval = c_mbs;
          auto maxval = c_mbs.is_neg() ? mybitset((int)-1, bvsize)
                                       : max_pos_val;
          if (c_mbs.is_neg()) { // add the full positive range as well
            this_ranges.push_back(range_t::range_from_mybitset(zero_val, max_pos_val));
          }
          this_ranges.push_back(range_t::range_from_mybitset(minval, maxval));
        }
      }
      return ranges_intersection({ var_val_ranges, this_ranges });
    }
  }
  else if (cond->get_operation_kind() == expr::OP_EQ) {
    expr_ref arg1 = cond->get_args().at(0);
    expr_ref arg2 = cond->get_args().at(1);
    expr::operation_kind arg1_opk = arg1->get_operation_kind();
    expr::operation_kind arg2_opk = arg2->get_operation_kind();
    if (   ((arg1->is_var() || arg1_opk == expr::OP_BVEXTRACT) && arg2->is_const())
        || ((arg2->is_var() || arg2_opk == expr::OP_BVEXTRACT) && arg1->is_const())) {
      if      (arg1->is_const()) { ASSERT(arg2->is_var() || arg2_opk == expr::OP_BVEXTRACT); }
      else if (arg2->is_const()) { ASSERT(arg1->is_var() || arg1_opk == expr::OP_BVEXTRACT); swap(arg1, arg2); }
      else    { NOT_REACHED(); }
      // (= c1 ((_ extract U L) x1)) OR (= c1 x1)
      // construct corresponding ranges, intersection with var_val_ranges => true ranges
      //                                 difference                       => false ranges
      unsigned bvsize = var->get_sort()->get_size();
      vector<mybitset> minval_v, maxval_v;
      if (arg2_opk == expr::OP_BVEXTRACT) {
        ASSERT(arg2->get_args().at(0) == var);
        unsigned u = arg2->get_args().at(1)->get_int_value();
        unsigned l = arg2->get_args().at(2)->get_int_value();
        if (bvsize-1 > u) {
          // multiple disjoint ranges will have to be created for each possibility of upper bits -- return failure
          return {};
        }
        minval_v.push_back(arg1->get_mybitset_value());
        maxval_v.push_back(arg1->get_mybitset_value());
        if (l > 0) {
          mybitset zeros(0, l);
          minval_v.push_back(zeros);
          maxval_v.push_back(~zeros);
        }
      } else {
        ASSERT(arg2 == var);
        minval_v.push_back(arg1->get_mybitset_value());
        maxval_v.push_back(arg1->get_mybitset_value());
      }
      mybitset minval = mybitset::mybitset_bvconcat(minval_v);
      mybitset maxval = mybitset::mybitset_bvconcat(maxval_v);
      auto this_range_r = minval.bvule(maxval) ? range_t::range_from_mybitset(minval, maxval) : range_t::empty_range(bvsize);
      return range_intersection(var_val_ranges, this_range_r);
    }
  }
  else if (cond->is_const_bool_true()) {
    return var_val_ranges;
  }
  else if (cond->is_const_bool_false()) {
    return vector<range_t>();
  }
  return {};
}

bool
expr_contains_only_this_var(expr_ref const& var, expr_ref const& e)
{
  expr_list vars = e->get_context()->expr_get_vars(e);
  expr_set varset(vars.begin(), vars.end());
  if (varset.size() == 0)
    return true;
  return    varset.size() == 1
         && varset.count(var);
}

bool
node_represents_boolean_connective(AstNodeRef const& ast)
{
  if (ast->get_kind() != AstNode::COMPOSITE)
    return false;
  auto opk = ast->get_composite_operation_kind();
  return opk == expr::OP_AND || opk == expr::OP_OR || opk == expr::OP_NOT;
}

pair<ExprDualLazy,optional<vector<range_t>>>
AstLinearizer::linearize_boolean_ast_and_recover_valid_ranges_for_var(AstNodeRef const& cond, vector<range_t> const& input_val_ranges, expr_ref const& var)
{
  expr_ref condE;
  if (   cond->get_kind() == AstNode::COMPOSITE
      && cond->get_composite_operation_kind() == expr::OP_EQ) {
    // special handling of OP_EQ with APPLY or ITE or VAR
    AstNodeRef arg1 = cond->get_args().at(0);
    AstNodeRef arg2 = cond->get_args().at(1);
    auto arg1_kind = arg1->get_kind();
    auto arg2_kind = arg2->get_kind();
    auto arg1_kind_can_be_array_constant = (arg1_kind == AstNode::APPLY || arg1_kind == AstNode::ITE || arg1_kind == AstNode::VAR);
    auto arg2_kind_can_be_array_constant = (arg2_kind == AstNode::APPLY || arg2_kind == AstNode::ITE || arg2_kind == AstNode::VAR);
    if (   arg1_kind_can_be_array_constant
        || arg2_kind_can_be_array_constant) {
      auto arg1_ed = this->linearize(arg1);
      auto arg2_ed = this->linearize(arg2);
      if (   arg1_ed.has_linear_form()
          && arg2_ed.has_linear_form()) {
        auto arg1_e_is_array_const = arg1_ed.linear_form_sort()->is_array_kind();
        auto arg2_e_is_array_const = arg2_ed.linear_form_sort()->is_array_kind();
        if (   arg1_e_is_array_const
            || arg2_e_is_array_const) {
          auto arg1_e = arg1_ed.linear_form();
          auto arg2_e = arg2_ed.linear_form();
          if (arg2_e_is_array_const) { swap(arg1_e, arg2_e); }
          // (= (f x1) c1) or (= (ite ...) c1)
          // find all addr ranges in arg1 whose value equal c1
          auto equal_ranges = arg1_e->get_array_constant()->addrs_where_mapped_value_equals(arg2_e);;
          auto equal_ranges_r = vmap<expr_pair,range_t>(equal_ranges, range_t::range_from_expr_pair);
          auto ret = ExprDualLazy::dual_if_const(encode_ranges_as_contraints_over_var(equal_ranges, var));
          return make_pair(ret, ranges_intersection({ equal_ranges_r, input_val_ranges }));
        }
      }
      auto cond_ed = this->process_op(expr::OP_EQ, { arg1_ed, arg2_ed });
      condE = cond_ed.normal_form();
    }
  }
  else if (cond->get_kind() == AstNode::VAR) {
    auto cond_ed = this->linearize(cond);
    if (cond_ed.has_linear_form()) {
      auto cond_lf = cond_ed.linear_form();
      ASSERT(cond_lf->is_array_sort() && cond_lf->get_sort()->get_range_sort()->is_bool_kind());
      auto true_ranges = cond_lf->get_array_constant()->addrs_where_mapped_value_equals(m_ctx->mk_bool_true());
      auto true_ranges_r = vmap<expr_pair,range_t>(true_ranges, range_t::range_from_expr_pair);
      auto ret = ExprDualLazy::dual_if_const(encode_ranges_as_contraints_over_var(true_ranges, var));
      return make_pair(ret, ranges_intersection({ true_ranges_r, input_val_ranges }));
    }
    condE = cond_ed.normal_form();
    //cout << _FNLN_ << ": VAR " << *cond << " does not have linear form; linearization is bound to fail!" << endl;
  }
  if (!condE) {
    auto cond_ed = this->linearize(cond);
    condE = cond_ed.normal_form();
  }
  ASSERT(expr_contains_only_this_var(var, condE));
  auto op_val_ranges = get_valid_ranges_for_var_from_boolean_expr_subject_to_value_ranges(var, condE, input_val_ranges);
  return make_pair(ExprDualLazy::dual_if_const(condE), op_val_ranges);
}

pair<ExprDualLazy,optional<vector<range_t>>>
AstLinearizer::linearize_ite_cond_and_recover_valid_ranges_for_var(AstNodeRef const& cond, vector<range_t> const& input_val_ranges, expr_ref const& var)
{
  if (!node_represents_boolean_connective(cond)) {
    return this->linearize_boolean_ast_and_recover_valid_ranges_for_var(cond, input_val_ranges, var);
  }
  // we proceed recursively for connectives (AND, OR, NOT)
  ASSERT(cond->get_kind() == AstNode::COMPOSITE);
  bool ranges_avail = true;
  expr_vector atoms;
  vector<vector<range_t>> val_ranges_vec;
  for (auto const& ast : cond->get_args()) {
    auto const& [cond_ed,op_val_ranges] = this->linearize_ite_cond_and_recover_valid_ranges_for_var(ast, input_val_ranges, var);
    atoms.push_back(cond_ed.normal_form());
    if (op_val_ranges && ranges_avail) {
      val_ranges_vec.push_back(*op_val_ranges);
    } else {
      ranges_avail = false;
    }
  }
  switch (cond->get_composite_operation_kind()) {
    case expr::OP_AND: {
      auto ret = ExprDualLazy::dual_if_const(accumulate(atoms.begin(), atoms.end(), m_ctx->mk_bool_true(), expr_and));
      if (!ranges_avail) {
        return make_pair(ret, nullopt);
      }
      return make_pair(ret, ranges_intersection(val_ranges_vec));
    }
    case expr::OP_OR: {
      auto ret = ExprDualLazy::dual_if_const(accumulate(atoms.begin(), atoms.end(), m_ctx->mk_bool_false(), expr_or));
      if (!ranges_avail) {
        return make_pair(ret, nullopt);
      }
      return make_pair(ret, ranges_union(val_ranges_vec));
    }
    case expr::OP_NOT: {
      ASSERT(atoms.size() == 1);
      auto ret = ExprDualLazy::dual_if_const(expr_not(atoms.front()));
      if (!ranges_avail) {
        return make_pair(ret, nullopt);
      }
      ASSERT(val_ranges_vec.size() == 1);
      return make_pair(ret, ranges_difference(input_val_ranges, val_ranges_vec.front()));
    }
    default:
      NOT_REACHED();
  }
}

optional<map<expr_ref,expr_ref>>
try_recovering_name_value_assignment_from_boolean_expr(expr_ref const& e)
{
  map<expr_ref,expr_ref> ret;
  auto opk = e->get_operation_kind();
  if (opk == expr::OP_EQ) {
    ASSERT(e->get_args().size() == 2);
    auto arg1 = e->get_args()[0];
    auto arg2 = e->get_args()[1];
    if (   arg1->is_var()
        && arg2->is_const()) {
      ret[arg1] = arg2;
      return ret;
    }
    else if (   arg2->is_var()
             && arg1->is_const()) {
      ret[arg2] = arg1;
      return ret;
    }
  } else if (opk == expr::OP_AND) {
    for (auto a : e->get_args()) {
      auto op_ret = try_recovering_name_value_assignment_from_boolean_expr(a);
      if (!op_ret)
        return nullopt;
      map_union(ret, *op_ret);
    }
    return ret;
  } else if (opk == expr::OP_VAR) {
    if (e->is_bool_sort()) {
      ret[e] = e->get_context()->mk_bool_true();
      return ret;
    }
  } else if (opk == expr::OP_NOT) {
    if (   e->is_bool_sort()
        && e->get_args().front()->is_var()) {
      ret[e->get_args().front()] = e->get_context()->mk_bool_false();
      return ret;
    }
  }
  return nullopt;
}

bool
all_names_are_assigned_in_map(expr_vector const& vars, map<expr_ref,expr_ref> const& name_value_map, expr_vector& param_vals)
{
  param_vals.clear();
  for (auto const& v : vars) {
    auto itr = name_value_map.find(v);
    if (itr == name_value_map.end())
      return false;
    param_vals.push_back(itr->second);
  }
  return true;
}

template<typename T, typename V>
V
op_bind(optional<T> const& op_val, function<V(T const&)> f, V const& def)
{
  if (op_val) {
    return f(*op_val);
  }
  return def;
}

optional<ExprDualLazy>
AstLinearizer::linearize_with_refined_ranges(range_vector const& refined_ranges, AstNodeRef const& ast)
{
  if (refined_ranges.empty())
    return {};
  auto old_ranges = this->get_enclosing_lambda_param_ranges();
  this->set_refined_lambda_param_ranges(refined_ranges);
  auto ast_ed = this->linearize(ast);
  this->restore_lambda_param_ranges(old_ranges);
  return ast_ed;
}

ExprDualLazy
AstLinearizer::handle_ite(AstNodeRef const& condA, AstNodeRef const& thenA, AstNodeRef const& elseA)
{
  // ite can be linearized to array_constant if
  // (1) cond refers param of unit-arity lambda, and is in form amicable to
  //     array constant linearization
  // (2) cond contains simple assignment constraints for all params of lambda
  //
  // During linearization, cond is viewed as constraining the lambda param
  // values (or value ranges) and the branches are viewed as values of the
  // linearized array constant.

  // helpers
  auto ac_domsort = this->get_array_constant_domain_sort_for_enclosing_lambda();
  auto is_leaf = [&ac_domsort](ExprDualLazy const& ed) -> bool
    {
      sort_ref s = ed.linear_form_sort();
      return !(s->is_array_kind() && s->get_domain_sort() == ac_domsort);
    };
  auto opportunistically_promote_to_lazy_form = [is_leaf](ExprDualLazy const& ed, range_vector const& ranges)
    {
      if (   ed.has_linear_form()
          && is_leaf(ed)) {
        auto ed_ll = ExprLinearLazy(ranges, ed.linear_form());
        return ExprDualLazy::dual_lazy(ed.normal_form(), ed_ll);
      }
      return ed;
    };
  auto ed_get_array_constant = [ctx=this->m_ctx,ac_domsort](ExprDualLazy const& ed, bool is_leaf)
    {
      ASSERT(ed.has_linear_form());
      auto lf = ed.linear_form();
      return is_leaf ? ctx->mk_array_constant(ac_domsort, lf)
                     : lf->get_array_constant();
    };

  DYN_DEBUG2(ast_linearizer, cout << "Doing ITE with cond = " << *condA << "; then = " << *thenA << "; else = " << *elseA << endl);

  ExprDualLazy cond_ed = ExprDualLazy::normal(m_ctx->mk_undefined());
  if (auto op_var = this->enclosing_lambda_is_unit_arity_bv_sorted()) {
    expr_ref var = *op_var;
    auto old_ranges = this->get_enclosing_lambda_param_ranges();
    auto p = this->linearize_ite_cond_and_recover_valid_ranges_for_var(condA, old_ranges, var);
    cond_ed = p.first;
    DYN_DEBUG2(ast_linearizer, cout << "cond = " << cond_ed.to_string() << "\nParam ranges = " << range_vector_to_string(old_ranges) << endl);
    if (p.second) {
      ASSERT(ac_domsort.size());
      auto const& true_ranges = *p.second;
      auto false_ranges = ranges_difference(old_ranges, true_ranges);

      if (true_ranges.empty() || false_ranges.empty()) {
        DYN_DEBUG2(ast_linearizer,
            if (true_ranges.empty())  cout << "WARNING! true_ranges are empty for cond" << endl;
            if (false_ranges.empty()) cout << "WARNING! false_ranges are empty for cond" << endl;
            if (!false_ranges.empty()) { cout << "false_ranges: " << range_vector_to_string(false_ranges) << endl; }
            if (!true_ranges.empty()) { cout << "true_ranges: " << range_vector_to_string(true_ranges) << endl; }
        );
      }

      auto op_then_ed = this->linearize_with_refined_ranges(true_ranges, thenA);
      auto op_else_ed = this->linearize_with_refined_ranges(false_ranges, elseA);

      function<string(ExprDualLazy const&)> ed_to_str = [](ExprDualLazy const& ed) { return ed.to_string(); };
      DYN_DEBUG2(ast_linearizer, cout << "then = " << op_bind(op_then_ed, ed_to_str, string("(empty-range)")) << "\nelse = " << op_bind(op_else_ed, ed_to_str, string("(empty-range)")) << endl);

      if (!op_then_ed) {
        ASSERT(op_else_ed);
        return opportunistically_promote_to_lazy_form(*op_else_ed, false_ranges);
      }
      if (!op_else_ed) {
        ASSERT(op_then_ed);
        return opportunistically_promote_to_lazy_form(*op_then_ed, true_ranges);
      }

      ASSERT(true_ranges.size());
      ASSERT(false_ranges.size());
      ASSERT(op_then_ed && op_else_ed);

      auto then_ed = *op_then_ed;
      auto else_ed = *op_else_ed;

      auto ret_nf = this->process_op(expr::OP_ITE, { cond_ed, then_ed, else_ed });
      if (!(   then_ed.has_linear_form()
            && else_ed.has_linear_form())) {
        return ret_nf;
      }

      bool then_is_leaf = is_leaf(then_ed);
      bool else_is_leaf = is_leaf(else_ed);

      if (   (then_ed.linear_form_is_lazy() || then_is_leaf)
          && (else_ed.linear_form_is_lazy() || else_is_leaf)) {
        auto then_ll = then_is_leaf ? ExprLinearLazy(true_ranges, then_ed.linear_form())
                                    : then_ed.linear_lazy_form().masked(true_ranges);
        auto else_ll = else_is_leaf ? ExprLinearLazy(false_ranges, else_ed.linear_form())
                                    : else_ed.linear_lazy_form().masked(false_ranges);
        auto ret_ll = then_ll.union_with(else_ll);
        ASSERT(ret_ll.get_sort()->get_domain_sort() == ac_domsort);
        auto ret = ExprDualLazy::dual_lazy(ret_nf.normal_form(), ret_ll);
        DYN_DEBUG2(ast_linearizer, cout << "ret = " << ret.to_string() << endl);
        return ret;
      }

      array_constant_ref thenAC = ed_get_array_constant(then_ed, then_is_leaf);
      array_constant_ref elseAC = ed_get_array_constant(else_ed, else_is_leaf);
      ASSERT(thenAC->get_result_sort() == elseAC->get_result_sort());
      // overlay thenAC over elseAC for true_ranges
      array_constant_ref retAC = elseAC;
      for (auto const& r : true_ranges) {
        expr_pair er = r.expr_pair_from_range(m_ctx);
        retAC = retAC->apply_mask_and_overlay_array_constant(er, thenAC);
      }
      expr_ref ite_lf = m_ctx->mk_array_const(retAC, m_ctx->mk_array_sort(ac_domsort, retAC->get_result_sort()));
      auto ret = ExprDualLazy::dual(ret_nf.normal_form(), ite_lf);
      DYN_DEBUG2(ast_linearizer, cout << "ret = " << ret.to_string() << endl);
      return ret;
    }
  } else if (auto op_vars = this->enclosing_lambda_has_arity_more_than_one()) {
    ASSERT(ac_domsort.size());
    // try pattern matching for (2)
    auto const& vars = *op_vars;
    auto cond_ed = this->linearize(condA);
    auto op_name_value_map = try_recovering_name_value_assignment_from_boolean_expr(cond_ed.normal_form());
    expr_vector param_vals;
    if (   op_name_value_map
        && all_names_are_assigned_in_map(vars, *op_name_value_map, param_vals)) {
      auto then_ed = this->linearize(thenA);
      auto else_ed = this->linearize(elseA);
      if (   then_ed.has_linear_form()
          && else_ed.has_linear_form()) {
        array_constant_ref elseAC = ed_get_array_constant(else_ed, is_leaf(else_ed));
        array_constant_ref retAC = elseAC->array_store(param_vals, then_ed.linear_form(), 1, false);
        expr_ref ite_nf = m_ctx->mk_ite(cond_ed.normal_form(), then_ed.normal_form(), else_ed.normal_form());
        expr_ref ite_lf = m_ctx->mk_array_const(retAC, m_ctx->mk_array_sort(ac_domsort, retAC->get_result_sort()));
        return ExprDualLazy::dual(ite_nf, ite_lf);
      } else {
        return this->process_op(expr::OP_ITE, { cond_ed, then_ed, else_ed });
      }
    }
  } else {
    cond_ed = this->linearize(condA);
  }
  auto then_ed = this->linearize(thenA);
  auto else_ed = this->linearize(elseA);
  return this->process_op(expr::OP_ITE, { cond_ed, then_ed, else_ed });
}

ExprDualLazy
AstLinearizer::handle_lambda(AstNodeRef const& ast)
{
  ASSERT(ast->get_args().size() >= 1);
  if (ast->get_args().size() == 1) {
    // zero arg lambda
    return linearize(ast->get_args().front());
  } else {
    expr_vector params;
    for (size_t i = 0; i < ast->get_args().size()-1; ++i) {
      auto const& arg = ast->get_args().at(i);
      ASSERT(arg->get_kind() == AstNode::ATOM);
      params.push_back(arg->get_atom());
    }
    this->push_lambda_params(params);
    ExprDualLazy body = linearize(ast->get_args().back());
    this->pop_lambda_params();

    expr_vector args = params;
    args.push_back(body.normal_form());
    expr_ref lambda_nf = m_ctx->mk_app(expr::OP_LAMBDA, args);

    if (body.has_linear_form()) {
      expr_ref lambda_lf = body.linear_form();
      sort_ref lambda_arr_sort = m_ctx->mk_array_sort(expr_vector_to_sort_vector(params), body.normal_form()->get_sort());
      if (lambda_lf->get_sort() != lambda_arr_sort) {
        // we keep linear form in array sort only
        ASSERT(lambda_lf->get_sort() == lambda_arr_sort->get_range_sort());
        lambda_lf = m_ctx->mk_array_const_with_def(lambda_arr_sort, lambda_lf);
      }
      return ExprDualLazy::dual(lambda_nf, lambda_lf);
    } else {
      return ExprDualLazy::normal(lambda_nf);
    }
  }
}

string
AstLinearizer::fresh_varname() const
{
  static unsigned long long cnt = 0;
  ostringstream os;
  os << "ast-linearizer-forall-fresh-" << cnt++;
  return os.str();
}

ExprDualLazy
AstLinearizer::handle_forall(AstNodeRef const& ast)
{
  ASSERT(ast->get_args().size() >= 2);
  // forall is treated as an apply on lambda where the args to apply are "free".
  // For linearization we require the lambda to transform to a constant so that
  // the result is independent of the "free" variables.  If linearization fails
  // then we return lambda+apply form with freshly created vars as args
  auto lambda_ed = this->handle_lambda(ast);
  if (lambda_ed.has_linear_form()) {
    auto lf_ac = lambda_ed.linear_form()->get_array_constant();
    if (lf_ac->is_default_array_constant()) {
      return ExprDualLazy::dual_if_const(lf_ac->get_default_value());
    } else {
      // different output for different values of free var
      return ExprDualLazy::dual_if_const(m_ctx->mk_bool_false());
    }
  }
  vector<expr_ref> free_vars;
  for (size_t i = 0; i < ast->get_args().size()-1; ++i) {
    auto const& arg = ast->get_args().at(i);
    ASSERT(arg->get_kind() == AstNode::ATOM);
    auto bound_var = arg->get_atom();
    expr_ref free_var = m_ctx->mk_var(fresh_varname(), bound_var->get_sort());
    free_vars.push_back(free_var);
  }
  expr_ref ret = m_ctx->mk_apply(lambda_ed.normal_form(), free_vars);
  return ExprDualLazy::normal(ret);
}

ExprDualLazy
AstLinearizer::linearize(AstNodeRef const& ast)
{
  switch (ast->get_kind()) {
    case AstNode::ATOM: {
      expr_ref atom = ast->get_atom();
      return ExprDualLazy::dual_if_const(atom);
    }
    case AstNode::COMPOSITE: {
      vector<ExprDualLazy> args;
      for (auto const& arg : ast->get_args()) {
        auto arg_expr = linearize(arg);
        args.push_back(arg_expr);
      }
      return process_op(ast->get_composite_operation_kind(), args);
    }
    case AstNode::VAR: {
      return this->lookup_name(ast->get_name());
    }
    case AstNode::ARRAY_INTERP: {
      // interpret function as an array -- apply() becomes select()
      auto const& fname = ast->get_name();
      ExprDualLazy fn_d = this->lookup_name(fname);
      if (!fn_d.has_linear_form()) {
        // couldn't convert to array -- return undefined to denote failure
        return ExprDualLazy::normal(m_ctx->mk_undefined());
      }
      expr_ref fn_expr = fn_d.linear_form();
      if (fn_expr->is_array_sort()) {
        return ExprDualLazy::dual_if_const(fn_expr);
      } else {
        // linearized expr is not of array sort -- assume that it represents an
        // array with only default value
        // XXX: perhaps this should never happen because we do similar
        // transformation in handle_lambda() where we ensure that the sort of
        // the linear expr matches the array of the function
        auto fn_sort = m_env.get_func_sort(fname);
        sort_ref arr_sort = m_ctx->mk_array_sort(fn_sort->get_domain_sort(), fn_sort->get_range_sort());
        return ExprDualLazy::dual_if_const(m_ctx->mk_array_const_with_def(arr_sort, fn_expr));
      }
    }
    case AstNode::AS_CONST: {
      auto const& arg_ast = ast->get_args().front();
      auto arg_expr = linearize(arg_ast);
      if (!arg_expr.has_linear_form()) {
        // couldn't convert to const -- return undefined to denote failure
        return ExprDualLazy::normal(m_ctx->mk_undefined());
      }
      return ExprDualLazy::dual_if_const(m_ctx->mk_array_const_with_def(ast->get_sort(), arg_expr.linear_form()));
    }
    case AstNode::LET: {
      ASSERT(ast->get_args().size() == 2);
      auto const& let_assign = ast->get_args().front();
      auto const& let_body   = ast->get_args().back();
      auto let_assign_expr = linearize(let_assign);
      DYN_DEBUG(ast_linearizer, cout << "LET: " << ast->get_name()->get_str() << " = " << let_assign_expr.to_string() << endl);
      this->push_letdecl(ast->get_name(), let_assign_expr);
      ExprDualLazy ret = linearize(let_body);
      this->pop_letdecl();
      return ret;
    }
    case AstNode::LAMBDA: {
      return this->handle_lambda(ast);
    }
    case AstNode::FORALL: {
      return this->handle_forall(ast);
    }
    case AstNode::APPLY: {
      return this->handle_apply(ast);
    }
    case AstNode::ITE: {
      auto const& condA = ast->get_args().at(0);
      auto const& thenA = ast->get_args().at(1);
      auto const& elseA = ast->get_args().at(2);
      return this->handle_ite(condA, thenA, elseA);
    }
    default:
      cout << _FNLN_ << ": unhandled ast kind: " << *ast << endl;
      NOT_IMPLEMENTED();
  }
}

}
