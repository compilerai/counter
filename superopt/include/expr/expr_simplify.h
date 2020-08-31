#pragma once

#include "expr/expr.h"
#include <map>

namespace eqspace {

class precond_t;
class sprel_map_pair_t;
class predicate;
using predicate_ref = shared_ptr<predicate const>;

// why use a namespace?  ¯\_(ツ)_/¯
namespace expr_simplify
{
  bool is_sign_overflow_comparison(expr_ref a, expr_ref b, expr::operation_kind *found_op, expr_ref *arg1, expr_ref *arg2);
  expr_ref simplify_select(expr_ref e, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, consts_struct_t const &cs);
  expr_ref simplify_store(expr_ref e, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, consts_struct_t const &cs);
  //expr_ref simplify_function_argument_ptr(expr_ref e, consts_struct_t const &cs);
  expr_ref simplify_bvadd(expr_ref e);
  expr_ref simplify_bvsub(expr_ref e);
  expr_ref simplify_bvmul(expr_ref const &e);
  expr_ref simplify_bvmul_no_ite(expr_ref const &e);

  expr_ref expr_get_atoms_for_reduction_operator(expr_ref const &e, expr::operation_kind op, std::function<expr_ref (expr_ref const &)> atomic_simplifier);

  bool args_equal(expr_ref a, expr_ref b);
  expr_ref simplify_iff_identify_signed_cmp_pattern(expr_ref e);
  expr_ref simplify_donotsimplify_using_solver_subzero(expr_ref e);
  expr_ref simplify_eq(expr_ref e);
  expr_ref simplify_or_zero_cmp(expr_ref e);
  expr_ref simplify_andnot1_zero_cmp(expr_ref e);
  expr_ref simplify_or_andnot1_and(expr_ref e);
  expr_ref simplify_or_andnot2_and(expr_ref e);
  expr_ref simplify_or(expr_ref e);
  expr_ref simplify_and(expr_ref e);
  expr_ref simplify_andnot1(expr_ref e);
  expr_ref simplify_andnot2(expr_ref e);
  expr_ref simplify_iff(expr_ref e);
  expr_ref simplify_implies(expr_ref e);
  expr_ref simplify_not_cmp(expr_ref e);
  int expr_bv_get_msb_const_numbits(expr_ref e, mybitset *const_val);
  expr_ref simplify_bvconcat(expr_ref e);
  expr_ref simplify_bvextract(expr_ref e, consts_struct_t const &cs);
  expr_ref simplify_bvucmp(expr_ref e);
  expr_ref simplify_const(expr_ref e);
  expr_ref simplify_not_chain(expr_ref e, bool is_not);
  expr_ref simplify_not(expr_ref e);
  expr_ref simplify_bvle(expr_ref e, bool is_signed);
  expr_ref simplify_bvgt(expr_ref e, bool is_signed);
  expr_ref simplify_bvge(expr_ref e, bool is_signed);
  expr_ref simplify_bvxor(expr_ref e);
  expr_ref simplify(expr_ref, consts_struct_t const &cs);
  expr_ref simplify_ite(expr_ref e);
  //expr_ref simplify_store(expr_ref e);
  expr_ref simplify_function_call(expr_ref e, consts_struct_t const &cs);
  expr_ref simplify_memmask(expr_ref e);
  expr_ref simplify_memmasks_are_equal(expr_ref e);
  //expr_ref simplify_memjoin(expr_ref e, consts_struct_t const &cs);
  expr_ref simplify_memsplice_arg(expr_ref e, consts_struct_t const &cs);
  expr_ref simplify_memsplice(expr_ref e, consts_struct_t const &cs);
  expr_ref simplify_ismemlabel(expr_ref e, consts_struct_t const &cs);
  //expr_ref simplify_switchmemlabel(expr_ref e, consts_struct_t const &cs);
  expr_ref simplify_bvextrotate(expr_ref e);

  expr_ref sort_arguments_to_canonicalize_expression(expr_ref e);
  expr_ref prune_obviously_false_branches_using_assume_clause(expr_ref assume, expr_ref e);
  expr_ref simplify_bvdivrem_numerator_no_ite(bool is_div, expr_ref const &numerator, expr_ref const &denominator, int op_kind, bool allow_const_specific_simplifications, consts_struct_t const &cs);
  expr_ref simplify_bvsdivrem_numerator_no_ite_denominator_no_ite(bool s_div, expr_ref const &denominator, expr_ref const &numerator, bool allow_const_specific_simplifications, consts_struct_t const &cs);
  expr_ref simplify_bvudivrem_numerator_no_ite_denominator_no_ite(bool is_div, expr_ref const &denominator, expr_ref const &numerator, bool allow_const_specific_simplifications, consts_struct_t const &cs);
  expr_ref simplify_bvdivrem(expr_ref const &e, bool allow_const_specific_simplifications, consts_struct_t const &cs);
//  expr_ref simplify_bvmul(expr_ref e, consts_struct_t const *cs);
  //expr_ref simplify_bvsrem(expr_ref e, consts_struct_t const &cs);
  //expr_ref simplify_bvurem(expr_ref e, consts_struct_t const &cs);
  expr_ref simplify_bvsign_ext(expr_ref e, consts_struct_t const &cs);
  expr_ref simplify_bvexshl(expr_ref const &e, consts_struct_t const &cs);
  expr_ref simplify_bvexashr(expr_ref const &e);
  expr_ref simplify_bvsign_ext_no_ite(expr_ref e, int l);
  template<class T>
  bool isPowerOfTwo(T);
  string power_of_two_bin_string(size_t shift_count);
  template <class sint, class uint>
  void compute_signed_magic_info(sint D, sint &multiplier, unsigned &shift);
  template <class uint>
  void compute_unsigned_magic_info(uint D, unsigned num_bits, uint &multiplier, unsigned &pre_shift, unsigned &post_shift, int &increment);

  expr_vector get_arithmetic_mul_atoms(expr_ref const &e);
}

expr_vector get_arithmetic_addsub_atoms(expr_ref e);
expr_ref arithmetic_addsub_atoms_to_expr(expr_vector const &ev);
void arithmetic_addsub_atoms_pair_minimize_using_lhs_set_and_precond(list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, expr_vector &ev1, expr_vector &ev2, context *ctx);
void arithmetic_addsub_atoms_pair_minimize(expr_vector &ev1, expr_vector &ev2, context *ctx);
bool expr_function_argument_is_independent_of_memlabel(expr_ref const &e, memlabel_t const &memlabel);
//expr_ref expr_simplify_memmask(context* ctx, expr_ref const &mem, memlabel_t memlabel/*, expr_ref addr, size_t memsize*/, consts_struct_t const &cs);
void expr_convert_ite_to_cases(expr_vector &out_conds, expr_vector &out_exprs, expr_ref const &in);
expr_ref expr_convert_cases_to_ite(expr_vector const &conds, expr_vector const &exprs);
expr_ref expr_simplify_using_lhs_set_helper(expr_ref e, bool allow_const_specific_simplifications, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair);
expr_ref expr_do_simplify_using_lhs_set_and_precond(expr_ref const &e, bool allow_const_specific_simplifications, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair);
bool is_expr_equal_syntactic_using_lhs_set_and_precond(expr_ref const &e1, expr_ref const &e2, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair);


bool is_overlapping_using_lhs_set_and_precond(context* ctx, memlabel_t const &a1ml, expr_ref const &a1, int a1nbytes, memlabel_t const &b1ml, expr_ref const &b1, int b1nbytes, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, consts_struct_t const *cs);

bool is_overlapping_syntactic_using_lhs_set_and_precond(context* ctx, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, expr_ref const &a1_in, int a1nbytes, expr_ref const &b1_in, int b1nbytes);

}
