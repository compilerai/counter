#pragma once

#include <map>

#include "support/autostop_watch.h"
#include "support/dyn_debug.h"

#include "expr/expr.h"
#include "expr/eval.h"
#include "expr/context.h"
#include "expr/consts_struct.h"
#include "expr/sprel_map_pair.h"
#include "expr/dummy_class.h"
#include "expr/relevant_memlabels.h"

namespace eqspace {

//class precond_t;
class sprel_map_pair_t;
//class predicate;
//using predicate_ref = shared_ptr<predicate const>;

class expr_simplify
{
public:
  static bool is_sign_overflow_comparison(expr_ref a, expr_ref b, expr::operation_kind *found_op, expr_ref *arg1, expr_ref *arg2);
  static expr_ref expr_simplify_memalloc_using_memlabel(expr_ref const &mem_alloc, memlabel_t const& memlabel);

  template<typename T_PRED, typename T_PRECOND>
  static expr_ref simplify_select(expr_ref e, bool semantic_simplification, expr_ref const& lhs_expr, relevant_memlabels_t const& relevant_memlabels, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map)
  {
    DYN_DEBUG(expr_simplify_select_dbg, cout << _FNLN_ << ": entry\n");
    ASSERT(e->get_operation_kind() == expr::OP_SELECT);
    context* ctx = e->get_context();
  
    expr_ref ml_expr = e->get_args()[OP_SELECT_ARGNUM_MEMLABEL];
    if (ml_expr->is_var()) {
      return e;
    }
    ASSERT(ml_expr->is_const());
  
    expr_ref mem = e->get_args()[OP_SELECT_ARGNUM_MEM];
    expr_ref mem_alloc = e->get_args()[OP_SELECT_ARGNUM_MEM_ALLOC];
    memlabel_t cur_memlabel = ml_expr->get_memlabel_value();
    expr_ref addr = e->get_args()[OP_SELECT_ARGNUM_ADDR];
    unsigned nbytes = e->get_args()[OP_SELECT_ARGNUM_COUNT]->get_int_value();
    bool bigendian = e->get_args()[OP_SELECT_ARGNUM_ENDIANNESS]->get_bool_value();
  
    expr_ref ret = expr_simplify_select<T_PRED, T_PRECOND>(ctx, mem, mem_alloc, cur_memlabel, addr, nbytes, bigendian, semantic_simplification, lhs_expr, relevant_memlabels, lhs_set, precond, sprel_map_pair, symbol_map, locals_map);
    return ret;
  }


  template<typename T_PRED, typename T_PRECOND>
  static expr_ref simplify_store(expr_ref e, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map);

  //expr_ref simplify_function_argument_ptr(expr_ref e, consts_struct_t const &cs);
  static expr_ref simplify_bvadd(expr_ref e);
  static expr_ref simplify_bvsub(expr_ref e);
  static expr_ref simplify_bvmul(expr_ref const &e);
  static expr_ref simplify_bvmul_no_ite(expr_ref const &e);

  static expr_ref expr_get_atoms_for_reduction_operator(expr_ref const &e, expr::operation_kind op, std::function<expr_ref (expr_ref const &)> atomic_simplifier);

  static bool args_equal(expr_ref a, expr_ref b);
  static expr_ref simplify_iff_identify_signed_cmp_pattern(expr_ref e);
  static expr_ref simplify_donotsimplify_using_solver_subzero(expr_ref e);
  static expr_ref simplify_eq(expr_ref e);
  static expr_ref simplify_or_zero_cmp(expr_ref e);
  static expr_ref simplify_andnot1_zero_cmp(expr_ref e);
  static expr_ref simplify_or_andnot1_and(expr_ref e);
  static expr_ref simplify_or_andnot2_and(expr_ref e);
  static expr_ref simplify_or(expr_ref e);
  static expr_ref simplify_and(expr_ref e);
  static expr_ref simplify_andnot1(expr_ref e);
  static expr_ref simplify_andnot2(expr_ref e);
  static expr_ref simplify_iff(expr_ref e);
  static expr_ref simplify_implies(expr_ref e);
  static expr_ref simplify_not_cmp(expr_ref e);
  static int expr_bv_get_msb_const_numbits(expr_ref e, mybitset *const_val);
  static expr_ref simplify_bvconcat(expr_ref e);
  static expr_ref simplify_bvextract(expr_ref e);
  static expr_ref simplify_bvucmp(expr_ref e);
  static expr_ref simplify_const(expr_ref e);
  static expr_ref simplify_not_chain(expr_ref e, bool is_not);
  static expr_ref simplify_not(expr_ref e);
  static expr_ref simplify_bvle(expr_ref e, bool is_signed);
  static expr_ref simplify_bvgt(expr_ref e, bool is_signed);
  static expr_ref simplify_bvge(expr_ref e, bool is_signed);
  static expr_ref simplify_bvxor(expr_ref e);
  static expr_ref simplify(expr_ref, consts_struct_t const &cs);
  static expr_ref simplify_ite(expr_ref e);
  //static expr_ref simplify_store(expr_ref e);
  static expr_ref simplify_function_call(expr_ref e, consts_struct_t const &cs);
  static expr_ref simplify_memmask(expr_ref const& e);
  static expr_ref simplify_memmasks_are_equal(expr_ref const& e);
  //static expr_ref simplify_memmasks_are_equal_else(expr_ref const& e);
  static expr_ref simplify_axpred(expr_ref const& e);
  static expr_ref simplify_lambda(expr_ref const& e);
  static expr_ref simplify_mkstruct(expr_ref const& e);
  static expr_ref simplify_getfield(expr_ref const& e);
  static expr_ref simplify_increment_count(expr_ref const& e);
  static expr_ref simplify_memlabel_is_absent(expr_ref const& e);
  //static expr_ref simplify_memsplice_arg(expr_ref e, consts_struct_t const &cs);
  //static expr_ref simplify_memsplice(expr_ref e, consts_struct_t const &cs);
  //static expr_ref simplify_ismemlabel(expr_ref e, consts_struct_t const &cs);
  //static expr_ref simplify_switchmemlabel(expr_ref e, consts_struct_t const &cs);
  static expr_ref simplify_bvextrotate(expr_ref e);

  static expr_ref sort_arguments_to_canonicalize_expression(expr_ref e);
  static expr_ref prune_obviously_false_branches_using_assume_clause(expr_ref assume, expr_ref e);
  static expr_ref simplify_bvdivrem_numerator_no_ite(bool is_div, expr_ref const &numerator, expr_ref const &denominator, int op_kind, bool allow_const_specific_simplifications, consts_struct_t const &cs);
  static expr_ref simplify_bvsdivrem_numerator_no_ite_denominator_no_ite(bool s_div, expr_ref const &denominator, expr_ref const &numerator, bool allow_const_specific_simplifications, consts_struct_t const &cs);
  static expr_ref simplify_bvudivrem_numerator_no_ite_denominator_no_ite(bool is_div, expr_ref const &denominator, expr_ref const &numerator, bool allow_const_specific_simplifications, consts_struct_t const &cs);
  static expr_ref simplify_bvdivrem(expr_ref const &e, bool allow_const_specific_simplifications, consts_struct_t const &cs);
//  expr_ref simplify_bvmul(expr_ref e, consts_struct_t const *cs);
  //expr_ref simplify_bvsrem(expr_ref e, consts_struct_t const &cs);
  //expr_ref simplify_bvurem(expr_ref e, consts_struct_t const &cs);
  static expr_ref simplify_bvsign_ext(expr_ref e, consts_struct_t const &cs);
  static expr_ref simplify_bvexshl(expr_ref const &e, consts_struct_t const &cs);
  static expr_ref simplify_bvexashr(expr_ref const &e);
  static expr_ref simplify_bvsign_ext_no_ite(expr_ref e, int l);

  static expr_ref simplify_fptrunc(expr_ref const& e);
  static expr_ref simplify_fp_to_ubv(expr_ref const& e);
  static expr_ref simplify_fp_to_sbv(expr_ref const& e);
  static expr_ref simplify_ubv_to_fp(expr_ref const& e);
  static expr_ref simplify_sbv_to_fp(expr_ref const& e);
  static expr_ref simplify_ieee_bv_to_float(expr_ref const& e);
  static expr_ref simplify_float_to_ieee_bv(expr_ref const& e);
  static expr_ref simplify_ieee_bv_to_floatx(expr_ref const& e);
  static expr_ref simplify_floatx_to_ieee_bv(expr_ref const& e);

  template<class T>
  static bool isPowerOfTwo(T);
  static string power_of_two_bin_string(size_t shift_count);
  template <class sint, class uint>
  static void compute_signed_magic_info(sint D, sint &multiplier, unsigned &shift);
  template <class uint>
  static void compute_unsigned_magic_info(uint D, unsigned num_bits, uint &multiplier, unsigned &pre_shift, unsigned &post_shift, int &increment);

  static expr_vector get_arithmetic_mul_atoms(expr_ref const &e);

  static expr_vector get_arithmetic_addsub_atoms(expr_ref e);
  static expr_ref arithmetic_addsub_atoms_to_expr(expr_vector const &ev);

  template<typename T_PRED, typename T_PRECOND>
  static void arithmetic_addsub_atoms_pair_minimize_using_lhs_set_and_precond(list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, expr_vector &ev1, expr_vector &ev2, context *ctx)
  {
    unsigned ev1_ignore_indices[ev1.size()];
    unsigned ev2_ignore_indices[ev2.size()];
    expr_vector ev1_min, ev2_min;
    unsigned ev1_i = 0, ev2_i = 0;
  
    CPP_DBG_EXEC(IS_EXPR_EQUAL_SYNTACTIC,
        for (auto const& ev1a : ev1) {
          cout << __func__ << " " << __LINE__ << ": ev1a = " << expr_string(ev1a) << endl;
        }
        for (auto const& ev2a : ev2) {
          cout << __func__ << " " << __LINE__ << ": ev2a = " << expr_string(ev2a) << endl;
        }
    );
  
    //iterate through lhs set to see if a precond of the form a + b + c + ... = d + e + f + ... exists, and a, b, c are a part of one ev, and d, e, f are a part of another ev. if so populate evN_ignore_indices accordingly
    for (auto const& l : lhs_set) {
      //expr_ref guard = l.first;
      //expr_ref src = l.second.first;
      //expr_ref dst = l.second.second;
      /*if (!is_expr_equal_syntactic(guard, expr_true(ctx))) {
        continue;
      }*/
      if (!precond.precond_implies(l->get_precond())) {
        continue;
      }
      expr_ref src = l->get_lhs_expr();
      expr_ref dst = l->get_rhs_expr();
      expr_vector src_atoms = get_arithmetic_addsub_atoms(src);
      expr_vector dst_atoms = get_arithmetic_addsub_atoms(dst);
      //CPP_DBG_EXEC4(IS_EXPR_EQUAL_SYNTACTIC, cout << "src: " << expr_string(src) << ", src_atoms:\n" << expr_vector_to_string(src_atoms) << endl);
      //CPP_DBG_EXEC4(IS_EXPR_EQUAL_SYNTACTIC, cout << "dst: " << expr_string(dst) << ", dst_atoms:\n" << expr_vector_to_string(dst_atoms) << endl);
      //CPP_DBG_EXEC4(IS_EXPR_EQUAL_SYNTACTIC, cout << "before arithmetic_addsub_atoms_pair_eliminate_using_another_atoms_pair: ev1:\n" << expr_vector_to_string(ev1) << endl);
      //CPP_DBG_EXEC4(IS_EXPR_EQUAL_SYNTACTIC, cout << "before arithmetic_addsub_atoms_pair_eliminate_using_another_atoms_pair: ev2:\n" << expr_vector_to_string(ev2) << endl);
      arithmetic_addsub_atoms_pair_eliminate_using_another_atoms_pair(ev1, ev2, src_atoms, dst_atoms);
      arithmetic_addsub_atoms_pair_eliminate_using_another_atoms_pair(ev1, ev2, dst_atoms, src_atoms);
      //CPP_DBG_EXEC4(IS_EXPR_EQUAL_SYNTACTIC, cout << "after arithmetic_addsub_atoms_pair_eliminate_using_another_atoms_pair: ev1:\n" << expr_vector_to_string(ev1) << endl);
      //CPP_DBG_EXEC4(IS_EXPR_EQUAL_SYNTACTIC, cout << "after arithmetic_addsub_atoms_pair_eliminate_using_another_atoms_pair: ev2:\n" << expr_vector_to_string(ev2) << endl);
    }
  
    for (unsigned i = 0; i < ev1.size(); i++) {
      for (unsigned j = 0; j < ev2.size(); j++) {
        if (array_search(ev2_ignore_indices, ev2_i, j) != -1) {
          continue;
        }
        if (is_expr_equal_syntactic_using_lhs_set_and_precond(ev1[i], ev2[j], lhs_set, precond, sprel_map_pair)) {
          CPP_DBG_EXEC(IS_EXPR_EQUAL_SYNTACTIC, cout << __func__ << " " << __LINE__ << ": is_expr_equal_syntactic() returned true for " << expr_string(ev1[i]) << " and " << expr_string(ev2[j]) << endl);
          ASSERT(ev1_i <= ev1.size());
          ASSERT(ev2_i <= ev2.size());
          ev1_ignore_indices[ev1_i] = i;
          ev2_ignore_indices[ev2_i] = j;
          ev1_i++;
          ev2_i++;
          break;
        }
        CPP_DBG_EXEC(IS_EXPR_EQUAL_SYNTACTIC, cout << __func__ << " " << __LINE__ << ": is_expr_equal_syntactic() returned false for " << expr_string(ev1[i]) << " and " << expr_string(ev2[j]) << endl << "ev1-table:\n" << ctx->expr_to_string_table(ev1[i]) << "\nev2-table:\n" << ctx->expr_to_string_table(ev2[j]) << endl);
      }
    }
  
    CPP_DBG_EXEC(IS_EXPR_EQUAL_SYNTACTIC, cout << __func__ << " " << __LINE__ << ": ev1_i = " << ev1_i << ", ev2_i = " << ev2_i << endl);
    bv_const ev1_const = 0;
    bv_const ev2_const = 0;
    for (unsigned i = 0; i < ev1.size(); i++) {
      if (array_search(ev1_ignore_indices, ev1_i, i) != -1) {
        continue;
      }
      expr_ref const_val;
      if (   ev1[i]->is_bv_sort()
          && ev1[i]->get_sort()->get_size() <= sizeof(int) * BYTE_SIZE
          && (const_val = expr_evaluates_to_constant(ev1[i]))->is_const()) {
        ev1_const += const_val->get_mybitset_value().get_signed_mpz();
      } else {
        ev1_min.push_back(ev1[i]);
      }
    }
    for (unsigned i = 0; i < ev2.size(); i++) {
      if (array_search(ev2_ignore_indices, ev2_i, i) != -1) {
        continue;
      }
      expr_ref const_val;
      if (   ev2[i]->is_bv_sort()
          && ev2[i]->get_sort()->get_size() <= sizeof(int) * BYTE_SIZE
          && (const_val = expr_evaluates_to_constant(ev2[i]))->is_const()) {
        ev2_const += const_val->get_mybitset_value().get_signed_mpz();
      } else {
        ev2_min.push_back(ev2[i]);
      }
    }
    if (ev1_const != ev2_const) {
      ASSERT(ev1.size() > 0 || ev2.size() > 0);
      size_t sz;
      if (ev1.size() > 0) {
        sz = ev1.at(0)->get_sort()->get_size();
      } else if (ev2.size() > 0) {
        sz = ev2.at(0)->get_sort()->get_size();
      } else NOT_REACHED();
      //ev1_min.push_back(ctx->mk_bv_const(ev1[0]->get_sort()->get_size(), ev1_const - ev2_const));
      ev1_min.push_back(ctx->mk_bv_const(sz, ev1_const));
      ev2_min.push_back(ctx->mk_bv_const(sz, ev2_const));
    }
  
  /*
    std::sort(ev1.begin(), ev1.end(), expr_ref_cmp);
    std::sort(ev2.begin(), ev2.end(), expr_ref_cmp);
    std::set_difference(ev1.begin(), ev1.end(), ev2.begin(), ev2.end(), std::back_inserter(ev1_min), expr_ref_cmp);
    std::set_difference(ev2.begin(), ev2.end(), ev1.begin(), ev1.end(), std::back_inserter(ev2_min), expr_ref_cmp);
    //cout << "ev1:\n" << expr_vector_to_string(ev1) << endl;
    //cout << "ev2:\n" << expr_vector_to_string(ev2) << endl;
    CPP_DBG_EXEC4(IS_EXPR_EQUAL_SYNTACTIC, cout << "ev1_min:\n" << expr_vector_to_string(ev1_min) << endl);
    CPP_DBG_EXEC4(IS_EXPR_EQUAL_SYNTACTIC, cout << "ev2_min:\n" << expr_vector_to_string(ev2_min) << endl);
  */
    ev1 = ev1_min;
    ev2 = ev2_min;
  }




  static void arithmetic_addsub_atoms_pair_minimize(expr_vector &ev1, expr_vector &ev2, context *ctx);
  static bool expr_function_argument_is_independent_of_memlabel(expr_ref const &e, memlabel_t const &memlabel);
  static void expr_convert_ite_to_cases(expr_vector &out_conds, expr_vector &out_exprs, expr_ref const &in);
  static expr_ref expr_convert_cases_to_ite(expr_vector const &conds, expr_vector const &exprs);

  template<typename T_PRED, typename T_PRECOND>
  static expr_ref expr_simplify_using_lhs_set_helper(expr_ref e, bool allow_const_specific_simplifications, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map)
  {
    autostop_timer func_timer(__func__);
    //context *ctx = e->get_context();
    //cout << __func__ << " " << __LINE__ << ": e = " << ctx->expr_to_string_table(e) << endl;
    //expr_ref e(this);
    //autostop_timer func_miss_timer(string(__func__) + ".miss");
    //set<pair<expr_ref, pair<expr_ref, expr_ref>>> lhs_set_substituted;
    //e = e->substitute_using_lhs_set(lhs_set, *cs, ctx);
    //lhs_set_substituted = lhs_set_substitute_until_fixpoint(lhs_set, *cs, ctx);
    //e = e->substitute_using_lhs_set(lhs_set_substituted, *cs, ctx);
    //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
  
    //expr_simplify_visitor visitor(e, lhs_set_substituted, local_sprel_expr_map, cs);
    expr_simplify_visitor<T_PRED, T_PRECOND> visitor(e, allow_const_specific_simplifications, /*semantic_simplifcation*/false, expr_true(e->get_context()), relevant_memlabels_t({}), lhs_set, precond, sprel_map_pair, symbol_map, locals_map);
    expr_ref ret = visitor.get_result();
    return ret;
  }

  static expr_ref expr_semantically_simplify_using_lhs_expr(expr_ref const& lhs_expr, expr_ref const& target_expr, relevant_memlabels_t const& relevant_memlabels);

  template<typename T_PRED, typename T_PRECOND>
  static expr_ref expr_do_simplify_using_lhs_set_and_precond(expr_ref const &e, bool allow_const_specific_simplifications, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map)
  {
    autostop_timer func_timer(__func__);
    //context *ctx = e->get_context();
  
    //expr_query_cache_t<expr_ref> &cache = ctx->m_cache->m_simplify_using_lhs_set;
    //if (!allow_const_specific_simplifications) {
    //  cache = ctx->m_cache->m_simplify_using_lhs_set_without_const_specific_simplifications;
    //}
    //bool solve = false;
    //expr_ref e(this);
  
    //if (!ctx->disable_caches()) {
    //  bool lb_found, ub_found;
    //  expr_ref lb, ub;
  
    //  cache.find_bounds(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, e, lb_found, lb, ub_found, ub);
    //  if (lb_found && ub_found && lb == ub) {
    //    expr_ref ret = lb;
    //    CPP_DBG_EXEC(EXPR_SIZE_DURING_SIMPLIFY,
    //        if (expr_size(ret) > expr_size(e)) {
    //          cout << __func__ << " " << __LINE__ << ": size increased from " << expr_size(e) << " to " << expr_size(ret) << ", e->op = " << op_kind_to_string(e->get_operation_kind()) << endl;
    //        }
    //    );
    //    CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": returning\n" << expr_string(ret) << endl);
    //    return ret;
    //  }
    //}
  
    //map<local_id_t, expr_ref> local_sprel_expr_map = lhs_set_get_local_sprel_expr_map(ctx, cs, lhs_set, guard);
    expr_ref ret = expr_simplify_using_lhs_set_helper(e, allow_const_specific_simplifications, lhs_set, precond, sprel_map_pair, symbol_map, locals_map);
    //if (!ctx->disable_caches()) {
    //  ASSERT(ret);
    //  cache.add(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, e, ret);
    //}
    //cout << __func__ << " " << __LINE__ << ": returning\n" << ret->to_string_table(true) << endl;
    CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": returning\n" << expr_string(ret) << endl);
    return ret;
  }


  template<typename T_PRED, typename T_PRECOND>
  static bool is_expr_equal_syntactic_using_lhs_set_and_precond(expr_ref const &e1_in, expr_ref const &e2_in, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair)
  {
    //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
    context *ctx = e1_in->get_context();
    if (e1_in->get_sort() != e2_in->get_sort()) {
      return false;
    }
    map<expr_id_t, pair<expr_ref, expr_ref>> const &sprel_submap = sprel_map_pair.sprel_map_pair_get_submap();
    expr_ref e1, e2;
    e1 = e1_in;
    e2 = e2_in;
    if (sprel_submap.size()) {
      e1 = ctx->expr_do_simplify(ctx->expr_substitute(e1, sprel_submap));
      e2 = ctx->expr_do_simplify(ctx->expr_substitute(e2, sprel_submap));
    }
    //map<expr_id_t, pair<expr_ref, expr_ref>> lsprel_submap = precond.get_local_sprel_expr_submap();
    //e1 = ctx->expr_substitute(e1, lsprel_submap);
    //e2 = ctx->expr_substitute(e2, lsprel_submap);
  
    //cout << __func__ << " " << __LINE__ << ": e1_in = " << expr_string(e1) << endl;
    //cout << __func__ << " " << __LINE__ << ": e2_in = " << expr_string(e2) << endl;
    //cout << __func__ << " " << __LINE__ << ": e1 = " << expr_string(e1) << endl;
    //cout << __func__ << " " << __LINE__ << ": e2 = " << expr_string(e2) << endl;
    //cout << __func__ << " " << __LINE__ << ": sprel_map_pair =\n" << sprel_map_pair.to_string_for_eq() << endl;
    if (e1 == e2) {
      return true;
    }
    //expr_query_cache_t<bool> &cache = ctx->m_cache->m_is_expr_equal_syntactic_using_lhs_set;
    //if (!ctx->disable_caches()) {
    //  bool lb_found, ub_found;
    //  bool lb, ub;
    //  cache.find_bounds(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, e1, e2, lb_found, lb, ub_found, ub);
    //  //cout << __func__ << " " << __LINE__ << ": lhs_set.size() = " << lhs_set.size() << endl;
    //  //cout << __func__ << " " << __LINE__ << ": e1 = " << expr_string(e1) << endl;
    //  //cout << __func__ << " " << __LINE__ << ": e2 = " << expr_string(e2) << endl;
  
    //  if (ub_found && ub) {
    //    //cout << __func__ << " " << __LINE__ << ": returning true\n";
    //    //cache_result_found = true;
    //    //cache_result = true;
    //    return true;
    //  }
    //  if (lb_found && !lb) {
    //    //cout << __func__ << " " << __LINE__ << ": returning false\n";
    //    //cache_result_found = true;
    //    //cache_result = false;
    //    return false;
    //  }
    //  ASSERT(!lb_found || !ub_found || lb || !ub);
    //}
    consts_struct_t const &cs = ctx->get_consts_struct();
  
    //cout << __func__ << " " << __LINE__ << ": e1 = " << expr_string(e1) << endl;
    //cout << __func__ << " " << __LINE__ << ": e2 = " << expr_string(e2) << endl;
    //cout << __func__ << " " << __LINE__ << ": precond = " << precond.precond_to_string_for_eq(true) << endl;
    map<expr_id_t, expr_ref> e1_eqset = gen_equivalence_class_using_lhs_set_and_precond(lhs_set, precond, sprel_map_pair, e1, cs);
    map<expr_id_t, expr_ref> e2_eqset = gen_equivalence_class_using_lhs_set_and_precond(lhs_set, precond, sprel_map_pair, e2, cs);
    for (auto e1_expr : e1_eqset) {
      if (e2_eqset.count(e1_expr.first)) {
        /*cout << __func__ << " " << __LINE__ << ": returning true for " << expr_string(e1) << " vs. " << expr_string(e2) << endl;
        cout << "e1_eqset =";
        for (auto e1e : e1_eqset) {
          cout << " " << expr_string(e1e.second);
        }
        cout << "\n";
        cout << "e2_eqset =";
        for (auto e2e : e2_eqset) {
          cout << " " << expr_string(e2e.second);
        }
        cout << "\n";*/
        //ASSERT(!cache_result_found || cache_result);
        //cout << __func__ << " " << __LINE__ << ": returning true\n";
        return true;
      }
    }
  
    bool ret;
  
    expr_vector ev1, ev2;
    ev1 = get_arithmetic_addsub_atoms(e1);
    ev2 = get_arithmetic_addsub_atoms(e2);
  
    CPP_DBG_EXEC3(IS_EXPR_EQUAL_SYNTACTIC,
        cout << __func__ << " " << __LINE__ << ": before minimize, ev1:\n";
        for (unsigned i = 0; i < ev1.size(); i++) {
          cout << i << ". " << expr_string(ev1[i]) << endl;
        }
        cout << __func__ << " " << __LINE__ << ": before minimize, ev2:\n";
        for (unsigned i = 0; i < ev2.size(); i++) {
          cout << i << ". " << expr_string(ev2[i]) << endl;
        }
    );
  
    if (ev1.size() == 1 && ev2.size() == 1) {
      expr_ref const &e1 = ev1.at(0);
      expr_ref const &e2 = ev2.at(0);
      if (e1 == e2) {
        ret = true;
      } else if (e1->is_var() || e2->is_var()) {
        ret = false;
      } else if (e1->is_const() || e2->is_const()) {
        ret = false;
      } else if (e1->get_operation_kind() != e2->get_operation_kind()) {
        ret = false;
      } else {
        if (e1->get_args().size() != e2->get_args().size()) {
          /*if (e1->get_operation_kind() != expr::OP_FUNCTION_CALL) {
            cout << __func__ << " " << __LINE__ << "e1 = " << expr_string(e1) << endl;
            cout << __func__ << " " << __LINE__ << "e2 = " << expr_string(e2) << endl;
          }
          ASSERT(e1->get_operation_kind() == expr::OP_FUNCTION_CALL);*/
          ret = false;
        } else {
          ASSERT(e1->get_args().size() == e2->get_args().size());
          ret = true;
          for (unsigned i = 0; i < e1->get_args().size(); i++) {
            if (!is_expr_equal_syntactic_using_lhs_set_and_precond(e1->get_args()[i], e2->get_args()[i], lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/)) {
              ret = false;
              break;
            }
          }
        }
      }
    } else {
      arithmetic_addsub_atoms_pair_minimize_using_lhs_set_and_precond(lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/, ev1, ev2, ctx);
      CPP_DBG_EXEC3(IS_EXPR_EQUAL_SYNTACTIC,
          cout << __func__ << " " << __LINE__ << ": after minimize, ev1:\n";
          for (unsigned i = 0; i < ev1.size(); i++) {
            cout << i << ". " << expr_string(ev1[i]) << endl;
          }
          cout << __func__ << " " << __LINE__ << ": after minimize, ev2:\n";
          for (unsigned i = 0; i < ev2.size(); i++) {
            cout << i << ". " << expr_string(ev2[i]) << endl;
          }
      );
  
      ret = ((ev1.size() == 0) && (ev2.size() == 0));
    }
    //cache[make_pair(e1->get_id(), e2->get_id())] = make_pair(make_pair(e1, e2), ret);
    //if (!ctx->disable_caches()) {
    //  cache.add(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, e1, e2, ret);
    //}
    /*if (cache_result_found && cache_result != ret) {
      cout << __func__ << " " << __LINE__ << ": lhs_set=\n";
      for (auto p : lhs_set) {
        cout << p.to_string(true) << endl;
      }
      cout << __func__ << " " << __LINE__ << ": guard = " << guard.edge_guard_to_string() << endl;
      cout << __func__ << " " << __LINE__ << ": e1 = " << expr_string(e1) << endl;
      cout << __func__ << " " << __LINE__ << ": e2 = " << expr_string(e2) << endl;
      cout << __func__ << " " << __LINE__ << ": returning " << (ret ? "true" : "false") << ". cache_result_found = " << cache_result_found <<". cache_result = " << cache_result << "\n";
    }
    ASSERT(!cache_result_found || cache_result == ret);*/
    //cout << __func__ << " " << __LINE__ << ": returning " << ret << "\n";
    return ret;
  }



  template<typename T_PRED, typename T_PRECOND>
  static bool is_overlapping_syntactic_using_lhs_set_and_precond(context* ctx, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, expr_ref const &a1_in, int a1nbytes, expr_ref const &b1_in, int b1nbytes)
  {
    autostop_watch aw(&is_overlapping_syntactic_using_lhs_set_and_precond_timer);
    map<expr_id_t, pair<expr_ref, expr_ref>> const &sprel_submap = sprel_map_pair.sprel_map_pair_get_submap();
    expr_ref a1 = a1_in;
    expr_ref b1 = b1_in;
    if (sprel_submap.size()) {
      a1 = ctx->expr_do_simplify(ctx->expr_substitute(a1, sprel_submap));
      b1 = ctx->expr_do_simplify(ctx->expr_substitute(b1, sprel_submap));
    }
    expr_vector a1_atoms, b1_atoms;
  
    a1_atoms = get_arithmetic_addsub_atoms(a1);
    b1_atoms = get_arithmetic_addsub_atoms(b1);
  
    arithmetic_addsub_atoms_pair_minimize_using_lhs_set_and_precond(lhs_set, precond, sprel_map_pair, a1_atoms, b1_atoms, ctx);
  
    return is_overlapping_atoms_pair_syntactic(ctx, a1_atoms, a1nbytes, b1_atoms, b1nbytes);
  }


  static bool is_overlapping_syntactic(context* ctx, expr_ref const &a1, int a1nbytes, expr_ref const &b1, int b1nbytes);

  template<typename T_PRED, typename T_PRECOND>
  static bool is_overlapping_using_lhs_set_and_precond(context* ctx, expr_ref const &a1, int a1nbytes, expr_ref const &b1, int b1nbytes, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, consts_struct_t const &cs)
  {
    if (!is_overlapping_considering_type_constraints(ctx, a1, a1nbytes, b1, b1nbytes, lhs_set, precond, sprel_map_pair, cs)) {
      return false;
    }
    bool ret = is_overlapping_syntactic_using_lhs_set_and_precond(ctx, lhs_set, precond, sprel_map_pair, a1, a1nbytes, b1, b1nbytes);
    return ret;
  }

  static bool is_expr_equal_syntactic(expr_ref const &e1, expr_ref const &e2);
  static bool is_expr_not_equal_syntactic(expr_ref const &e1, expr_ref const &e2);
  static bool is_expr_bv_greatereq_syntactic(expr_ref const &e1, expr_ref const &e2);
  static bool is_expr_bv_lesseq_syntactic(expr_ref const &e1, expr_ref const &e2);

  static expr_ref get_memmask_from_mem(expr_ref const& e, expr_ref const& mem_alloc, memlabel_t const& memlabel);

  template<typename T_PRED, typename T_PRECOND>
  static bool is_contained_in_using_lhs_set_and_precond(context* ctx, memlabel_t const &a1ml, expr_ref const &a1, int a1nbytes, memlabel_t const &b1ml, expr_ref const &b1, int b1nbytes, bool &is_symbol_local_loc, int &start_byte, int &stop_byte, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map)
  {
    is_symbol_local_loc = false;
    if (memlabel_t::memlabels_are_independent(&a1ml, &b1ml)) {
      CPP_DBG(IS_CONTAINED_IN, "memlabels %s and %s are independent. returning false\n", a1ml.to_string().c_str(), b1ml.to_string().c_str());
      return false;
    }
    if (   memlabel_t::memlabels_equal(&a1ml, &b1ml)
        && (a1ml.memlabel_represents_symbol() || a1ml.memlabel_is_local())) {
      if (loc_represents_entire_symbol_or_local(b1, b1nbytes, symbol_map, locals_map)) {
        is_symbol_local_loc = true;
        return true;
      }
    }
    bool ret = is_contained_in_syntactic_using_lhs_set_and_precond(ctx, lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/, a1, a1nbytes, b1, b1nbytes, start_byte, stop_byte);
    return ret;
  }


  static bool is_contained_in(context* ctx, memlabel_t const &a1ml, expr_ref const &a1, int a1nbytes, memlabel_t const &b1ml, expr_ref const &b1, int b1nbytes, bool &is_symbol_local_loc, int &start_byte, int &stop_byte, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map);
  static bool op_is_commutative(expr::operation_kind opk);

  template<typename T_PRED, typename T_PRECOND>
  static expr_ref expr_simplify_syntactic(expr_ref e, bool allow_const_specific_simplifications, bool semantic_simplification, expr_ref const& lhs_expr, relevant_memlabels_t const& relevant_memlabels, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map)
  {
    //autostop_timer func_timer("expr_simplify_syntactic()");
    //autostop_watch aw(&expr_simplify_syntactic_timer);
  
    context *ctx = e->get_context();
    consts_struct_t const& cs = ctx->get_consts_struct();
    //expr_query_cache_t<expr_ref> &cache = ctx->m_cache->m_simplify_syntactic;
  
    //if (!ctx->disable_caches()) {
    //  bool lb_found, ub_found;
    //  expr_ref lb, ub;
  
    //  cache.find_bounds(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, e, lb_found, lb, ub_found, ub);
    //  if (lb_found && ub_found && lb == ub) {
    //    expr_ref ret = lb;
    //    CPP_DBG_EXEC(EXPR_SIZE_DURING_SIMPLIFY,
    //      if (expr_size(ret) > expr_size(e)) {
    //        cout << __func__ << " " << __LINE__ << ": size increased from " << expr_size(e) << " to " << expr_size(ret) << ", e->op = " << op_kind_to_string(e->get_operation_kind()) << endl;
    //      }
    //    );
    //    /*if (ret.get_ptr() != e.get_ptr()) {
    //      cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
    //      cout << __func__ << " " << __LINE__ << ": ret = " << expr_string(ret) << endl;
    //      ASSERT(ret->to_string_table() != e->to_string_table());
    //      ASSERT(expr_string(ret) != expr_string(e));
    //    }*/
  
    //    //cout << __func__ << " " << __LINE__ << ":  returning " << expr_string(ret) << " for " << expr_string(e) << endl;
    //    return ret;
    //  }
    //}
    CPP_DBG_EXEC3(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": simplifying syntactically " << op_kind_to_string(e->get_operation_kind()) << endl);
    expr_ref ret;
    switch (e->get_operation_kind()) {
      case expr::OP_CONST: ret = expr_simplify::simplify_const(e); break;
      case expr::OP_EQ:
          ret = expr_simplify::sort_arguments_to_canonicalize_expression(e);
          ret = expr_simplify::simplify_eq(ret);
          break;
      case expr::OP_IFF:
          ret = expr_simplify::sort_arguments_to_canonicalize_expression(e);
          ret = expr_simplify::simplify_iff(ret);
          break;
      case expr::OP_OR:
          ret = expr_simplify::sort_arguments_to_canonicalize_expression(e);
          ret = expr_simplify::simplify_or(ret);
          break;
      case expr::OP_AND:
          ret = expr_simplify::sort_arguments_to_canonicalize_expression(e);
          ret = expr_simplify::simplify_and(ret);
          break;
      case expr::OP_BVOR:
      case expr::OP_BVAND:
          ret = expr_simplify::sort_arguments_to_canonicalize_expression(e);
          break;
      case expr::OP_ANDNOT1: ret = expr_simplify::simplify_andnot1(e); break;
      case expr::OP_ANDNOT2: ret = expr_simplify::simplify_andnot2(e); break;
      case expr::OP_NOT: ret = expr_simplify::simplify_not(e); break;
      case expr::OP_IMPLIES: ret = expr_simplify::simplify_implies(e); break;
      case expr::OP_ITE: ret = expr_simplify::simplify_ite(e); break;
      case expr::OP_BVXOR: ret = expr_simplify::simplify_bvxor(e); break;
      case expr::OP_BVCONCAT: ret = expr_simplify::simplify_bvconcat(e); break;
      case expr::OP_BVEXTRACT: ret = expr_simplify::simplify_bvextract(e); break;
      case expr::OP_BVEXTROTATE_LEFT:
      case expr::OP_BVEXTROTATE_RIGHT:
        ret = expr_simplify::simplify_bvextrotate(e);
        break;
      case expr::OP_BVULE:
      case expr::OP_BVUGE:
      case expr::OP_BVULT:
      case expr::OP_BVUGT:
        ret = expr_simplify::simplify_bvucmp(e); break;
      case expr::OP_SELECT:
        ret = expr_simplify::simplify_select(e, semantic_simplification, lhs_expr, relevant_memlabels, lhs_set, precond, sprel_map_pair, symbol_map, locals_map); break;
      case expr::OP_STORE:
        ret = expr_simplify::simplify_store(e, lhs_set, precond, sprel_map_pair, symbol_map, locals_map); break;
      /*case expr::OP_FUNCTION_ARGUMENT_PTR:
        ret = expr_simplify::simplify_function_argument_ptr(e, cs); break;*/
      case expr::OP_BVSUB:
        ret = expr_simplify::simplify_bvsub(e);
        break;
      case expr::OP_BVADD:
        ret = expr_simplify::sort_arguments_to_canonicalize_expression(e);
        ret = expr_simplify::simplify_bvadd(ret);
        break;
      case expr::OP_BVMUL:
        ret = expr_simplify::sort_arguments_to_canonicalize_expression(e);
        ret = expr_simplify::simplify_bvmul(ret);
        break;
      case expr::OP_FUNCTION_CALL:
        ret = expr_simplify::simplify_function_call(e, cs);
        break;
      case expr::OP_MEMMASK:
        ret = expr_simplify::simplify_memmask(e);
        break;
      case expr::OP_MEMMASKS_ARE_EQUAL:
        ret = expr_simplify::simplify_memmasks_are_equal(e);
        break;
      //case expr::OP_MEMMASKS_ARE_EQUAL_ELSE:
      //  ret = expr_simplify::simplify_memmasks_are_equal_else(e);
      //  break;
      // TODO: axpred need better simplification logic
      case expr::OP_AXPRED:
        ret = expr_simplify::simplify_axpred(e);
        break;
      case expr::OP_LAMBDA:
        ret = expr_simplify::simplify_lambda(e);
        break;
      case expr::OP_MKSTRUCT:
        ret = expr_simplify::simplify_mkstruct(e);
        break;
      case expr::OP_GETFIELD:
        ret = expr_simplify::simplify_getfield(e);
        break;

      case expr::OP_INCREMENT_COUNT:
        ret = expr_simplify::simplify_increment_count(e);
        break;

      case expr::OP_MEMLABEL_IS_ABSENT:
        ret = expr_simplify::simplify_memlabel_is_absent(e);
        break;
      /*case expr::OP_MEMJOIN:
        ret = expr_simplify::simplify_memjoin(e, cs);
        break;*/
      //case expr::OP_MEMSPLICE:
      //  ret = expr_simplify::simplify_memsplice(e, cs);
      //  break;
      case expr::OP_DONOTSIMPLIFY_USING_SOLVER_SUBZERO:
        ret = expr_simplify::simplify_donotsimplify_using_solver_subzero(e);
        break;
      case expr::OP_BVSREM:
      case expr::OP_BVUREM:
      case expr::OP_BVSDIV:
      case expr::OP_BVUDIV:
        ret = expr_simplify::simplify_bvdivrem(e, allow_const_specific_simplifications, cs);
        break;
      case expr::OP_BVSIGN_EXT:
        ret = expr_simplify::simplify_bvsign_ext(e, cs);
        break;
      case expr::OP_BVEXSHL:
        ret = expr_simplify::simplify_bvexshl(e, cs);
        break;
      case expr::OP_BVEXASHR:
        ret = expr_simplify::simplify_bvexashr(e);
        break;

      case expr::OP_FPTRUNC:
        ret = expr_simplify::simplify_fptrunc(e);
        break;
      case expr::OP_FP_TO_UBV:
        ret = expr_simplify::simplify_fp_to_ubv(e);
        break;
      case expr::OP_FP_TO_SBV:
        ret = expr_simplify::simplify_fp_to_sbv(e);
        break;
      case expr::OP_UBV_TO_FP:
        ret = expr_simplify::simplify_ubv_to_fp(e);
        break;
      case expr::OP_SBV_TO_FP:
        ret = expr_simplify::simplify_sbv_to_fp(e);
        break;

      case expr::OP_IEEE_BV_TO_FLOAT:
        ret = expr_simplify::simplify_ieee_bv_to_float(e);
        break;
      case expr::OP_FLOAT_TO_IEEE_BV:
        ret = expr_simplify::simplify_float_to_ieee_bv(e);
        break;
      case expr::OP_IEEE_BV_TO_FLOATX:
        ret = expr_simplify::simplify_ieee_bv_to_floatx(e);
        break;
      case expr::OP_FLOATX_TO_IEEE_BV:
        ret = expr_simplify::simplify_floatx_to_ieee_bv(e);
        break;
      default:
        ret = e;
        break;
    }
    /*if (ret != e) {
      //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
      //cout << __func__ << " " << __LINE__ << ": ret = " << expr_string(ret) << endl;
      ASSERT(ctx->expr_to_string_table(ret) != ctx->expr_to_string_table(e));
      //ASSERT(expr_string(ret) != expr_string(e));
    }*/
    CPP_DBG_EXEC(EXPR_SIZE_DURING_SIMPLIFY,
      if (expr_size(ret) > expr_size(e)) {
        cout << __func__ << " " << __LINE__ << ": expr_size(ret) " << expr_size(ret) << " expr_size(e) " << expr_size(e) << " e->get_operation_kind() = " << op_kind_to_string(e->get_operation_kind()) << endl;
        cout << "e = " << endl << expr_string(e) << endl;
        cout << "ret = " << endl << expr_string(ret) << endl;
      }
    );
    /*CPP_DBG_EXEC(ASSERTCHECKS,
        query_comment qc(query_comment::prover, "prove-assertcheck");
        counter_example_t counter_example;
        if (!expr_contains_memlabel_top(e)
            && ret->is_const()) {
          ASSERT(expr_eq(e, ret)->is_provable(qc, false, counter_example));
        }
    );*/
    /*if ((lhs_list.size() == 0)) {
      lhs_list_zero_cache[e->get_id()] = make_pair(ret, e);
    }*/
    //if (!ctx->disable_caches()) {
    //  cache.add(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, e, ret);
    //}
    DYN_DEBUG2(expr_simplify, cout << __func__ << " " << __LINE__ << ": done simplifying syntactically " << op_kind_to_string(e->get_operation_kind()) << endl);
    DYN_DEBUG2(expr_simplify, cout << __func__ << " " << __LINE__ << ":  returning\n" << expr_string(ret) << "\nfor\n" << expr_string(e) << endl);
    return ret;
  }

  static expr_ref expr_simplify_solver(expr_ref e, bool allow_const_specific_simplifications, consts_struct_t const &cs);
  static expr_ref canonicalize_expr_tree(expr_ref const& e);

private:
  static expr_ref expr_reconcile_sorts_for_replacement_vars_after_solver_simplify(expr_ref const& e, map<expr_id_t, pair<expr_ref, expr_ref>> const& submap);
  static expr_ref simplify_eq_ite(expr_ref const& arg0, expr_ref const& arg1);
  static void arithmetic_addsub_atoms_pair_eliminate_using_another_atoms_pair(expr_vector &ev1, expr_vector &ev2, expr_vector const &another1, expr_vector const &another2);
  static int64_t expr_vector_addsub_eval(expr_vector const &ev, consts_struct_t const &cs);
  static bool is_overlapping_atoms_pair_syntactic(context *ctx, expr_vector const &ev1, int nbytes1, expr_vector const &ev2, int nbytes2);

  template<typename T_PRED, typename T_PRECOND>
  static bool expr_is_variable_start(list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair/*, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs*/, expr_ref const &e)
  {
    //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
    context *ctx = e->get_context();
    if (e->get_operation_kind() == expr::OP_FUNCTION_CALL) {
      return true;
    }
    for (const auto &lp : lhs_set) {
      //expr_ref l = lp.second.first;
      expr_ref l = lp->get_lhs_expr();
      //cout << __func__ << " " << __LINE__ << ": l = " << expr_string(l) << endl;
      if (l->get_operation_kind() != expr::OP_IS_MEMACCESS_LANGTYPE) {
        continue;
      }
      /*if (   !is_expr_equal_syntactic(lp.first, expr_true(ctx))
          || !is_expr_equal_syntactic(lp.second.second, expr_true(ctx))) {
        continue;
      }*/
      if (   !precond.precond_implies(lp->get_precond())
          || !expr_simplify::is_expr_equal_syntactic(lp->get_rhs_expr(), expr_true(ctx))) {
        continue;
      }
      ASSERT(l->get_args().size() == 2);
      if (expr_simplify::is_expr_equal_syntactic_using_lhs_set_and_precond(l->get_args().at(0), e, lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/)) {
        //cout << __func__ << " " << __LINE__ << ": returning true for e = " << expr_string(e) << endl;
        return true;
      }
    }
    //cout << __func__ << " " << __LINE__ << ": returning false for e = " << expr_string(e) << endl;
    return false;
  }





  template<typename T_PRED, typename T_PRECOND>
  static bool is_gep_inbounds_offset(expr_ref const &addr, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair/*, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs*/, int &offset)
  {
    //cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << endl;
    //cout << __func__ << " " << __LINE__ << ": addr = " << addr->to_string_table(true) << endl;
    context *ctx = addr->get_context();
    for (auto t : lhs_set) {
      //expr_ref guard = t.first;
      //expr_ref lhs = t.second.first;
      expr_ref lhs = t->get_lhs_expr();
      //expr_ref rhs = t.second.second;
      expr_ref rhs = t->get_rhs_expr();
      if (   precond.precond_implies(t->get_precond())
          && rhs == expr_true(ctx)
          && lhs->get_operation_kind() == expr::OP_ISGEPOFFSET
          && lhs->get_args().at(1)->is_const()) {
        //cout << __func__ << " " << __LINE__ << ": addr = " << addr->to_string_table(true) << endl;
        //cout << __func__ << " " << __LINE__ << ": lhs->get_args().at(0) = " << lhs->get_args().at(0)->to_string_table(true) << endl;
        if (expr_simplify::is_expr_equal_syntactic_using_lhs_set_and_precond(lhs->get_args().at(0), addr, lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/)) {
          //cout << __func__ << " " << __LINE__ << ": equal_syntactic returns true\n";
          ASSERT(lhs->get_args().at(1)->is_const());
          offset = lhs->get_args().at(1)->get_int_value();
          //cout << __func__ << " " << __LINE__ << ": returning true due to " << expr_string(lhs->get_args().at(0)) << endl;
          //cout << __func__ << " " << __LINE__ << ": returning true for " << addr->get_id() << " due to " << lhs->get_args().at(0)->get_id() << endl;
          return true;
        }
        //cout << __func__ << " " << __LINE__ << ": equal_syntactic returns false for " << lhs->get_args().at(0)->get_id() << "(is gep offset) vs. " << addr->get_id() << "(addr)\n";
      }
    }
    //cout << __func__ << " " << __LINE__ << ": returning false for " << addr->get_id() << "\n";
    return false;
  }


  template<typename T_PRED, typename T_PRECOND>
  static int find_offset_wrt_typed_variable_pointer(expr_ref const &addr, bool &is_symbol, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair/*, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs*/, consts_struct_t const &cs)
  {
    //cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << endl;
    is_symbol = false;
    int offset = 0;
    if (is_gep_inbounds_offset(addr, lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/, offset)) {
      //cout << __func__ << " " << __LINE__ << ": is_gep_inbounds_offset() returned true\n";
      return offset;
    }
    expr_vector addr_atoms;
    addr_atoms = expr_simplify::get_arithmetic_addsub_atoms(addr);
    bool found_unknown = false;
  
    offset = 0;
    for (size_t i = 0; i < addr_atoms.size(); i++) {
      if (cs.is_symbol(addr_atoms.at(i))) {
        is_symbol = true;
        continue;
      }
      if (addr_atoms.at(i)->is_const()) {
        offset += addr_atoms.at(i)->get_int64_value();
        continue;
      }
      found_unknown = true;
    }
    if (is_symbol) {
      if (found_unknown) {
        //cout << __func__ << " " << __LINE__ << ": returning -1 (is_symbol) for " << expr_string(addr) << endl;
        return -1;
      } else {
        //cout << __func__ << " " << __LINE__ << ": returning " << offset << " (is_symbol) for " << expr_string(addr) << endl;
        return offset;
      }
    }
    offset = 0;
    found_unknown = false;
    for (size_t i = 0; i < addr_atoms.size(); i++) {
      if (expr_is_variable_start(lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/, addr_atoms.at(i))) {
        continue;
      }
      if (addr_atoms.at(i)->is_const()) {
        offset += addr_atoms.at(i)->get_int64_value();
        continue;
      }
      found_unknown = true;
    }
    if (found_unknown) {
      //cout << __func__ << " " << __LINE__ << ": returning -1 (!is_symbol) for " << expr_string(addr) << endl;
      return -1;
    } else {
      //cout << __func__ << " " << __LINE__ << ": returning " << offset << " (!is_symbol) for " << expr_string(addr) << endl;
      return offset;
    }
  }


  template<typename T_PRED, typename T_PRECOND>
  static bool is_overlapping_considering_type_constraints(context* ctx, expr_ref const &a1, int a1nbytes, expr_ref const &b1, int b1nbytes, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair/*, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs*/, consts_struct_t const &cs)
  {
    //cout << __func__ << " " << __LINE__ << ": a1 = " << expr_string(a1) << ", a1nbytes = " << a1nbytes << endl;
    //cout << __func__ << " " << __LINE__ << ": b1 = " << expr_string(b1) << ", b1nbytes = " << b1nbytes << endl;
    //cout << __func__ << " " << __LINE__ << ": lhs_set.size() = " << lhs_set.size() << endl;
    if (a1nbytes != b1nbytes) {
      //return false; //XXX: not sure if this is correct. gap/ElmrVecFFE fails if this is commented out; perlbmk/Perl_my_htonl fails if this is uncommented
    }
    bool a1_is_symbol, b1_is_symbol;
    int a1_offset, b1_offset;
    a1_offset = find_offset_wrt_typed_variable_pointer(a1, a1_is_symbol, lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/, cs);
    //cout << __func__ << " " << __LINE__ << ": a1_offset = " << a1_offset << endl;
    if (a1_offset == -1) {
      return true;
    }
    b1_offset = find_offset_wrt_typed_variable_pointer(b1, b1_is_symbol, lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/, cs);
    //cout << __func__ << " " << __LINE__ << ": b1_offset = " << b1_offset << endl;
    if (b1_offset == -1) {
      return true;
    }
  #define NO_OVERLAP(a_offset, b_offset, b_nbytes) (a_offset >= b_offset + b_nbytes);
    //cout << __func__ << " " << __LINE__ << ": a1_is_symbol = " << a1_is_symbol << endl;
    //cout << __func__ << " " << __LINE__ << ": b1_is_symbol = " << b1_is_symbol << endl;
    if (a1_is_symbol) {
      bool ret = !NO_OVERLAP(b1_offset, a1_offset, a1nbytes);
      //cout << __func__ << " " << __LINE__ << ": returning " << ret << endl;
      return ret;
    } else if (b1_is_symbol) {
      bool ret = !NO_OVERLAP(a1_offset, b1_offset, b1nbytes);
      //cout << __func__ << " " << __LINE__ << ": returning " << ret << endl;
      return ret;
    } else {
      return true;
    }
    return true;
  }



  static bool is_contained_in_atoms_pair_syntactic(expr_vector const &ev1, int ev1_nbytes, expr_vector const &ev2, int ev2_nbytes, int &start_byte, int &stop_byte, consts_struct_t const &cs);

  template<typename T_PRED, typename T_PRECOND>
  static bool is_contained_in_syntactic_using_lhs_set_and_precond(context* ctx, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair/*, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs*/, expr_ref const &a1_in, int a1nbytes, expr_ref const &b1_in, int b1nbytes, int &start_byte, int &stop_byte)
  {
    consts_struct_t const& cs = ctx->get_consts_struct();
    map<expr_id_t, pair<expr_ref, expr_ref>> const &sprel_submap = sprel_map_pair.sprel_map_pair_get_submap();
    expr_ref a1 = a1_in;
    expr_ref b1 = b1_in;
    if (sprel_submap.size()) {
      a1 = ctx->expr_do_simplify(ctx->expr_substitute(a1, sprel_submap));
      b1 = ctx->expr_do_simplify(ctx->expr_substitute(b1, sprel_submap));
    }
    expr_vector a1_atoms, b1_atoms;
  
    a1_atoms = expr_simplify::get_arithmetic_addsub_atoms(a1);
    b1_atoms = expr_simplify::get_arithmetic_addsub_atoms(b1);
  
    expr_simplify::arithmetic_addsub_atoms_pair_minimize_using_lhs_set_and_precond(lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/, a1_atoms, b1_atoms, ctx);
  
    bool ret = is_contained_in_atoms_pair_syntactic(a1_atoms, a1nbytes, b1_atoms, b1nbytes, start_byte, stop_byte, cs);
    //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for " << expr_string(a1_in) << " a1nbytes " << a1nbytes << " and " << expr_string(b1) << " b1nbytes " << b1nbytes << ": start_byte = " << start_byte << ", stop_byte = " << stop_byte << endl;
    return ret;
  }



  static bool addr_belongs_to_memrange(expr_ref const &addr, int start, int stop);

  //template<typename T_PRED, typename T_PRECOND>
  //static void memlabel_intersect_using_lhs_set_and_addr(memlabel_t &ml, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair/*, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs*/, expr_ref const &addr, int nbytes, consts_struct_t const &cs)
  //{
  //  context *ctx = addr->get_context();
  //  {
  //    memlabel_t addr_ml = memlabel_t::memlabel_top();
  //    for (const auto &t : lhs_set) {
  //      expr_ref src = t->get_lhs_expr();
  //      expr_ref dst = t->get_rhs_expr();
  //
  //      if (   src->get_operation_kind() == expr::OP_ISMEMLABEL // put fast checks first
  //          && dst == expr_true(ctx)
  //          && precond.precond_implies(t->get_precond())) {
  //        expr_ref arg0 = src->get_args().at(0);
  //        if (expr_simplify::is_expr_equal_syntactic_using_lhs_set_and_precond(arg0, addr, lhs_set, precond, sprel_map_pair)) {
  //          memlabel_t arg_ml = src->get_args().at(2)->get_memlabel_value();
  //          if (addr_ml.memlabel_is_top())
  //            addr_ml = arg_ml;
  //          else
  //            memlabel_t::memlabels_intersect(&addr_ml, &arg_ml);
  //          //cout << __func__ << " " << __LINE__ << ": arg_ml = " << memlabel_to_string(&arg_ml, as1, sizeof as1) << endl;
  //          //cout << __func__ << " " << __LINE__ << ": after intersection, ml = " << memlabel_to_string(&ml, as1, sizeof as1) << endl;
  //        }
  //      }
  //    }
  //    //if (   !ctx->disable_caches()
  //    //    && !addr_ml.memlabel_is_top()) {
  //    //  cache.add(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, addr, addr_ml);
  //    //}
  //    memlabel_t::memlabels_intersect(&ml, &addr_ml); // if addr_ml is TOP then ml remains same
  //  }
  //}


  template<typename T_PRED, typename T_PRECOND>
  static expr_ref expr_simplify_select_on_store(context* ctx, expr_ref const& mem, expr_ref const& mem_alloc, memlabel_t const& memlabel, expr_ref const& addr, unsigned nbytes, bool bigendian/*, comment_t comment*/, bool semantic_simplification, expr_ref const& lhs_expr, relevant_memlabels_t const& relevant_memlabels, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map);


  static std::function<expr_ref (expr_ref const &cond, expr_ref const &left, expr_ref const &right)> fold_ite_expr;
  static std::function<expr_ref (expr_ref const &cond, expr_ref const &left, expr_ref const &right)> fold_ite_and_simplify_expr;

  template<typename T_PRED, typename T_PRECOND>
  static expr_ref expr_simplify_select(context* ctx, expr_ref const& mem, expr_ref const& mem_alloc, memlabel_t const& memlabel, expr_ref const& addr, unsigned nbytes, bool bigendian, /*comment_t comment, */bool semantic_simplification, expr_ref const& lhs_expr, relevant_memlabels_t const& relevant_memlabels, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map)
  {
    DYN_DEBUG(expr_simplify_select_dbg, cout << _FNLN_ << ": entry\n");
    return expr_simplify_select_on_store<T_PRED, T_PRECOND>(ctx, mem, mem_alloc, memlabel, addr, nbytes, bigendian, /*commeyynt, */semantic_simplification, lhs_expr, relevant_memlabels, lhs_set, precond, sprel_map_pair, symbol_map, locals_map);
  }

  static expr_ref expr_simplify_memmask(context *ctx, expr_ref const &mem, expr_ref const& mem_alloc, memlabel_t const& memlabel);
  static expr_ref expr_simplify_memsplice_no_ite(expr_ref e, consts_struct_t const &cs);
  static bool memlabel_vector_find(vector<memlabel_t> const &memlabels, memlabel_t const &ml);
  static expr_ref expr_simplify_bvextract(context* ctx, expr_ref bv, int l, int r);
  static expr_ref convert_div_to_rem(expr_ref const &numerator, expr_ref const &denominator, expr_ref const &quotient);

  template<typename T_PRED, typename T_PRECOND>
  static map<expr_id_t, expr_ref> gen_equivalence_class_using_lhs_set_and_precond(list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, expr_ref e, consts_struct_t const &cs)
  {
    //context *ctx = e->get_context();
    //expr_query_cache_t<map<expr_id_t,expr_ref>> &cache = ctx->m_cache->m_gen_equivalence_class_using_lhs_set_and_precond;
    //if (!ctx->disable_caches()) {
    //  bool lb_found, ub_found;
    //  map<expr_id_t,expr_ref> lb, ub;
    //  cache.find_bounds(lhs_set, precond, sprel_map_pair, nullptr, pred_avail_exprs_t(), e, lb_found, lb, ub_found, ub);
    //  if (lb_found && ub_found && lb == ub) {
    //    //CPP_DBG_EXEC(EXPR_SIMPLIFY_SELECT, cout << __func__ << ':' << __LINE__ << ": memlabel returned by cache: " << lb.to_string() << endl);
    //    return lb;
    //  }
    //}
  
    map<expr_id_t, expr_ref> ret, new_ret;
    new_ret[e->get_id()] = e;
  
    /*if (cs.expr_is_local_id(e)) {
      local_id_t local_id = cs.expr_get_local_id(e);
      map<local_id_t, expr_ref> lsprels = precond.get_local_sprel_expr_map();
      if (lsprels.count(local_id)) {
        expr_ref sprel_expr = lsprels.at(local_id);
        new_ret[sprel_expr->get_id()] = sprel_expr;
      }
      for (auto le : lsprels) {
        if (e == le.second) {
          expr_ref l = cs.get_expr_value(reg_type_local, le.first);
          new_ret[l->get_id()] = l;
        }
      }
    }*/
  
    while (new_ret.size() > ret.size()) {
      ret = new_ret;
      for (const auto &l : lhs_set) {
        //expr_ref guard = l.first;
        //expr_ref src = l.second.first;
        //expr_ref dst = l.second.second;
        /*if (guard != expr_true(ctx)) {
          continue;
        }*/
        expr_ref src = l->get_lhs_expr();
        expr_ref dst = l->get_rhs_expr();
        if (!precond.precond_implies(l->get_precond())) {
          continue;
        }
        for (auto r : ret) {
          if (   new_ret.count(src->get_id()) == 0
              || new_ret.count(dst->get_id()) == 0) {
            if (   expr_simplify::is_expr_equal_syntactic(e, src)
                || expr_simplify::is_expr_equal_syntactic(e, dst)) {
              //cout << __func__ << " " << __LINE__ << ": src = " << expr_string(src) << endl;
              //cout << __func__ << " " << __LINE__ << ": dst = " << expr_string(dst) << endl;
              new_ret[src->get_id()] = src;
              new_ret[dst->get_id()] = dst;
            }
          }
        }
      }
    }
    //if (!ctx->disable_caches()) {
    //    cache.add(lhs_set, precond, sprel_map_pair, nullptr, pred_avail_exprs_t(), e, ret);
    //}
    return ret;
  }

  static bool is_concat_extract_pair(expr_ref e, expr_ref &orig_expr, expr_ref &new_value_for_masked_bits);
  static expr_ref expr_get_linear_lower_bound(expr_ref e);
  static expr_ref expr_get_linear_upper_bound(expr_ref e);
  static bool loc_represents_entire_symbol_or_local(expr_ref const& var, uint64_t size, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map);

  template<typename T_PRED, typename T_PRECOND>
  class expr_simplify_visitor : public expr_visitor {
    using T_PRED_REF = shared_ptr<T_PRED const>;
  public:
    expr_simplify_visitor(expr_ref const &e, bool allow_const_specific_simplifications, bool semantic_simplification, expr_ref const& lhs_expr, relevant_memlabels_t const& relevant_memlabels, list<T_PRED_REF> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map) : m_in(e), m_allow_const_specific_simplifications(allow_const_specific_simplifications), m_semantic_simplifcation(semantic_simplification), m_lhs_expr(lhs_expr), m_relevant_memlabels(relevant_memlabels), m_lhs_set(lhs_set), m_precond(precond), m_sprel_map_pair(sprel_map_pair), m_symbol_map(symbol_map), m_locals_map(locals_map), m_ctx(e->get_context()), m_cs(m_ctx->get_consts_struct())
    {
      ASSERT(IMPLIES(m_semantic_simplifcation, !!m_lhs_expr));
      //m_visit_each_expr_only_once = true;
      //cout << _FNLN_ << ": m_in = " << expr_string(m_in) << endl;
      visit_recursive(m_in);
    }
  
    expr_ref get_result() const
    {
      ASSERT(m_map.count(m_in->get_id()) > 0);
      expr_ref ret = m_map.at(m_in->get_id());
      return ret;
    }
  
  private:
    virtual void visit(expr_ref const &e)
    {
      context *ctx = e->get_context();
      //expr_query_cache_t<expr_ref> &cache = m_allow_const_specific_simplifications ? ctx->m_cache->m_simplify_visitor : ctx->m_cache->m_simplify_visitor_without_const_specific_simplifications;
    
      //if (!ctx->disable_caches()) {
      //  bool lb_found, ub_found;
      //  expr_ref lb, ub;
      //  cache.find_bounds(m_lhs_set, m_precond, m_sprel_map_pair, m_src_suffixpath, m_src_pred_avail_exprs, e, lb_found, lb, ub_found, ub);
      //  if (lb_found && ub_found && lb == ub) {
      //    expr_ref ret = lb;
      //    CPP_DBG_EXEC(EXPR_SIZE_DURING_SIMPLIFY,
      //        if (expr_size(ret) > expr_size(e)) {
      //          cout << __func__ << " " << __LINE__ << ": size increased from " << expr_size(e) << " to " << expr_size(ret) << ", e->op = " << op_kind_to_string(e->get_operation_kind()) << endl;
      //        }
      //    );
      //    m_map[e->get_id()] = ret;
      //    return;
      //  }
      //}
    
      if (e->is_const()) {
        //if (!ctx->disable_caches()) {
        //  cache.add(m_lhs_set, m_precond, m_sprel_map_pair, m_src_suffixpath, m_src_pred_avail_exprs, e, e);
        //}
        m_map[e->get_id()] = e;
        return;
      } else if (e->is_var()) {
        //if (!ctx->disable_caches()) {
        //  cache.add(m_lhs_set, m_precond, m_sprel_map_pair, m_src_suffixpath, m_src_pred_avail_exprs, e, e);
        //}
        m_map[e->get_id()] = e;
        return;
      }
      ASSERT(e->get_args().size() > 0);
      //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
      expr_ref ret = expr_evaluates_to_constant(e);
      if (ret->is_const()) {
        //if (!ctx->disable_caches()) {
        //  cache.add(m_lhs_set, m_precond, m_sprel_map_pair, m_src_suffixpath, m_src_pred_avail_exprs, e, ret);
        //}
        ASSERT(expr_size(ret) <= expr_size(e));
        m_map[e->get_id()] = ret;
        //check_expr_size(e, ret);
        CPP_DBG_EXEC2(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl);
        CPP_DBG_EXEC2(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": ret = " << expr_string(ret) << endl);
        return;
      }
      expr_vector new_args;
      for (unsigned i = 0; i < e->get_args().size(); i++) {
        expr_ref arg = e->get_args().at(i);
        expr_ref new_arg = m_map.at(arg->get_id());
        if (arg->get_sort() != new_arg->get_sort()) {
          cout << _FNLN_ << ": arg =\n" << ctx->expr_to_string_table(arg) << endl;
          cout << _FNLN_ << ": new_arg =\n" << ctx->expr_to_string_table(new_arg) << endl;
        }
        ASSERT(arg->get_sort() == new_arg->get_sort());
        new_args.push_back(new_arg);
    
        CPP_DBG_EXEC(EXPR_SIZE_DURING_SIMPLIFY,
            if (expr_size(new_arg) > expr_size(arg)) {
              cout << __func__ << " " << __LINE__ << ": arg[" << i << "] size increased from " << expr_size(arg) << " to " << expr_size(new_arg) << ", arg->op = " << op_kind_to_string(arg->get_operation_kind()) << endl;
            }
        );
        //cout << __func__ << " " << __LINE__ << ": pushed new_arg " << new_args.at(i)->get_id() << endl;
      }
      ret = m_ctx->mk_app(e->get_operation_kind(), new_args);
    
      CPP_DBG_EXEC(EXPR_SIZE_DURING_SIMPLIFY,
          if (expr_size(ret) > expr_size(e)) {
            cout << __func__ << " " << __LINE__ << ": size increased from " << expr_size(e) << " to " << expr_size(ret) << ", e->op = " << op_kind_to_string(e->get_operation_kind()) << endl;
            cout << __func__ << " " << __LINE__ << ": e = " << endl << m_ctx->expr_to_string_table(e) << endl;
            cout << __func__ << " " << __LINE__ << ": ret = " << endl << m_ctx->expr_to_string_table(ret) << endl;
          }
      );
      //cout << __func__ << " " << __LINE__ << ": ret = " << expr_string(ret) << endl;
      expr_ref prev_ret;
      int iter = 0;
      do {
        prev_ret = ret;

        DYN_DEBUG2(expr_simplify_debug, cout << __func__ << " " << __LINE__ << ": iter = " << iter << "; ret =\n" << ctx->expr_to_string_table(ret, true) << endl);
    
        expr_ref const_ret = expr_evaluates_to_constant(ret);
        ASSERT(const_ret->get_sort() == ret->get_sort());
        DYN_DEBUG2(expr_simplify_debug, cout << __func__ << " " << __LINE__ << ": iter = " << iter << "; const_ret =\n" << ctx->expr_to_string_table(const_ret, true) << endl);
        expr_ref syntactic_ret = expr_simplify::expr_simplify_syntactic(const_ret, m_allow_const_specific_simplifications, m_semantic_simplifcation, m_lhs_expr, m_relevant_memlabels, m_lhs_set, m_precond, m_sprel_map_pair, m_symbol_map, m_locals_map);
        ASSERT(syntactic_ret->get_sort() == const_ret->get_sort());
        DYN_DEBUG2(expr_simplify_debug, cout << __func__ << " " << __LINE__ << ": iter = " << iter << "; syntactic_ret =\n" << ctx->expr_to_string_table(syntactic_ret, true) << endl);
    
        CPP_DBG_EXEC(EXPR_SIZE_DURING_SIMPLIFY,
            if (expr_size(syntactic_ret) > expr_size(const_ret)) {
              cout << __func__ << " " << __LINE__ << ": iter = " << iter << ", size increased from " << expr_size(const_ret) << " to " << expr_size(syntactic_ret) << ", ret->op = " << op_kind_to_string(const_ret->get_operation_kind()) << endl;
            }
        );

        expr_ref solver_ret = syntactic_ret;
        if (!solver_ret->is_function_sort()) {
          DYN_DEBUG2(expr_simplify_debug, cout << __func__ << " " << __LINE__ << ": iter = " << iter << "; solver_ret =\n" << ctx->expr_to_string_table(solver_ret, true) << endl);
          solver_ret = expr_simplify::expr_simplify_solver(solver_ret, m_allow_const_specific_simplifications, m_cs);
          ASSERT(syntactic_ret->get_sort() == solver_ret->get_sort());
          DYN_DEBUG2(expr_simplify_debug, cout << __func__ << " " << __LINE__ << ": iter = " << iter << "; solver_ret =\n" << ctx->expr_to_string_table(solver_ret, true) << endl);
        }
        //DYN_DEBUG(expr_simplify_solver,
            //cout << __func__ << " " << __LINE__ << ": iter = " << iter << "; ret =\n" << ctx->expr_to_string_table(ret, true) << "\n, prev_ret =\n" << ctx->expr_to_string_table(prev_ret, true) << "\nsyntactic_ret =\n" << ctx->expr_to_string_table(syntactic_ret, true) << "\nsolver_ret =\n" << ctx->expr_to_string_table(solver_ret, true) << endl;
        //);
        solver_ret = expr_simplify::canonicalize_expr_tree(solver_ret);
        ASSERT(syntactic_ret->get_sort() == solver_ret->get_sort());
        DYN_DEBUG2(expr_simplify_debug, cout << __func__ << " " << __LINE__ << ": iter = " << iter << "; after canonicalization, solver_ret =\n" << ctx->expr_to_string_table(solver_ret, true) << endl);
        if (iter > SIMPLIFY_FIXED_POINT_SYNTACTIC_CONVERGENCE_MAX_ITERS && syntactic_ret == prev_ret) {
          //syntactic simplification is making no changes but perhaps solver simplification is oscillating; let's simply return the current answer
          ret = syntactic_ret;
          break;
        }
    
        if (expr_size(solver_ret) <= expr_size(syntactic_ret)) {
          ret = solver_ret;
        } else {
          ret = syntactic_ret;
        }
        iter++;
        if (iter > SIMPLIFY_FIXED_POINT_CONVERGENCE_MAX_ITERS) {
          cout << __func__ << " " << __LINE__ << ": iter = " << iter << "; ret =\n" << ctx->expr_to_string_table(ret, true) << "\n, prev_ret =\n" << ctx->expr_to_string_table(prev_ret, true) << "\nsyntactic_ret =\n" << ctx->expr_to_string_table(syntactic_ret, true) << "\nsolver_ret =\n" << ctx->expr_to_string_table(solver_ret, true) << endl;
          cout << "This may happen if the argument order changes in every iteration, for example" << endl;
          NOT_REACHED();
          break;
        }
      } while (ret != prev_ret);
    
      /*if (expr_size(ret) > expr_size(e)) {
        cout << __func__ << " " << __LINE__ << ": iter = " << iter << ", size increased from " << expr_size(e) << " to " << expr_size(ret) << ", e->op = " << op_kind_to_string(e->get_operation_kind()) << endl;
      }*/
    
      //if (!ctx->disable_caches()) {
      //  cache.add(m_lhs_set, m_precond, m_sprel_map_pair, m_src_suffixpath, m_src_pred_avail_exprs, e, ret);
      //}
      //cout << __func__ << " " << __LINE__ << ": returning " << ret->get_id() << " for " << e->get_id() << endl;
      //cout << __func__ << " " << __LINE__ << ": e = " << endl << e->to_string_table() << endl;
      //cout << __func__ << " " << __LINE__ << ": ret = " << endl << ret->to_string_table() << endl;
      //check_expr_size(e, ret);
      m_map[e->get_id()] = ret;
    }

    expr_ref const &m_in;
    bool m_allow_const_specific_simplifications;
    bool m_semantic_simplifcation;
    expr_ref const& m_lhs_expr;
    relevant_memlabels_t const& m_relevant_memlabels;
    list<T_PRED_REF> const &m_lhs_set;
    //edge_guard_t const &m_guard;
    T_PRECOND const &m_precond;
    sprel_map_pair_t const &m_sprel_map_pair;
    graph_symbol_map_t const& m_symbol_map;
    graph_locals_map_t const& m_locals_map;
    context *m_ctx;
    consts_struct_t const &m_cs;
    map<unsigned, expr_ref> m_map;
  };

};



//bool is_overlapping_using_lhs_set_and_precond(context* ctx, memlabel_t const &a1ml, expr_ref const &a1, int a1nbytes, memlabel_t const &b1ml, expr_ref const &b1, int b1nbytes, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, consts_struct_t const *cs);

//bool is_overlapping_syntactic_using_lhs_set_and_precond(context* ctx, list<predicate_ref> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, expr_ref const &a1_in, int a1nbytes, expr_ref const &b1_in, int b1nbytes);

}
