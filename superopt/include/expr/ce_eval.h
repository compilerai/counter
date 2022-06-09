#pragma once

#include "expr/eval.h"
#include "support/dyn_debug.h"

namespace eqspace {

class ce_eval_expr_visitor : public expr_evaluates_to_constant_visitor {
public:
  ce_eval_expr_visitor(expr_ref const& in, counter_example_t &ce, counter_example_t &rand_vals, relevant_memlabels_t const& relevant_memlabels/*, bool check_bounds*/)
    : expr_evaluates_to_constant_visitor(in, false), // disable cache
      m_ce(ce),
      m_relevant_memlabels(relevant_memlabels),
      m_rand_vals(rand_vals),
      //m_check_bounds(check_bounds),
      m_is_out_of_bound(false)
  { }

  bool is_out_of_bound() const { return m_is_out_of_bound;}

protected:
  bool evaluate_var(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_select(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_store(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_store_uninit(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_function_call(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_memmask(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_memmasks_are_equal(expr_ref const& e, expr_ref& const_val) const override;

  //virtual bool addr_within_atomic_memlabel_bounds(expr_ref const& addr, int nbytes, memlabel_t const& atomic_ml) const = 0;
  //virtual bool expr_constant_value_in_heap(expr_ref const& addr, int nbytes) const = 0;
  //virtual vector<pair<expr_ref,expr_ref>> get_mask_ranges_for_memlabel(memlabel_t const& ml) const = 0;
  //virtual array_constant_ref overlay_heap(array_constant_ref const& orig, array_constant_ref const& overlay) const = 0;
  //virtual bool evaluate_iscontiguous_memlabel(expr_ref const& e, expr_ref& const_val) const override = 0;
  //virtual bool evaluate_isprobably_contiguous_memlabel(expr_ref const& e, expr_ref& const_val) const override = 0;
  //expr_eval_status_t evaluate_islast_in_container(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_alloca(expr_ref const& e, expr_ref& const_val) const override;
  //bool evaluate_alloca_ptr(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_dealloc(expr_ref const& e, expr_ref& const_val) const override;

  bool evaluate_heap_alloc(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_heap_dealloc(expr_ref const& e, expr_ref& const_val) const override;

private:
  vector<pair<expr_ref,expr_ref>> get_mask_ranges_for_memlabel(expr_ref const& mem_alloc, memlabel_t const& ml) const;
  array_constant_ref get_alloca_ptr_array_constant() const;

  static bool lambda_body_has_only_lambda_vars(expr_ref const& e);
  static size_t expr_const_subtract(expr_ref const& a, expr_ref const& b);
  static expr_ref expr_const_add(expr_ref const& a, expr_ref const& b);
  static expr_ref expr_const_add(expr_ref const& a, size_t b);
  static bool expr_const_ulesseq(expr_ref const& a, expr_ref const& b);
  static expr_ref lambda_apply(expr_ref const& elambda, vector<expr_ref> const& econst);

protected:
  counter_example_t &m_ce;
  relevant_memlabels_t const& m_relevant_memlabels;

private:
  counter_example_t &m_rand_vals;
  //bool m_check_bounds;
  mutable bool m_is_out_of_bound;
};

//class ml_ranges_ce_eval_expr_visitor : public ce_eval_expr_visitor {
//public:
//  ml_ranges_ce_eval_expr_visitor(expr_ref const& in, counter_example_t &ce, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels/*, bool check_bounds*/)
//    : ce_eval_expr_visitor(in, ce, rand_vals, relevant_memlabels)
//  { }
//
//protected:
//  //bool addr_within_atomic_memlabel_bounds(expr_ref const& addr, int nbytes, memlabel_t const& atomic_ml) const override;
//  //bool expr_constant_value_in_heap(expr_ref const& addr, int nbytes) const override;
//  //vector<pair<expr_ref,expr_ref>> get_mask_ranges_for_memlabel(memlabel_t const& ml) const override;
//  //array_constant_ref overlay_heap(array_constant_ref const& orig, array_constant_ref const& overlay) const override;
//  //bool evaluate_iscontiguous_memlabel(expr_ref const& e, expr_ref& const_val) const override;
//  //bool evaluate_isprobably_contiguous_memlabel(expr_ref const& e, expr_ref& const_val) const override;
//private:
//  bool addr_overlaps_with_atomic_memlabel(expr_ref const& addr, int nbytes, memlabel_t const& atomic_ml) const;
//};
//
//class pointsto_uf_ce_eval_expr_visitor : public ce_eval_expr_visitor {
//public:
//  pointsto_uf_ce_eval_expr_visitor(expr_ref const& in, counter_example_t &ce, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels/*, bool check_bounds*/)
//    : ce_eval_expr_visitor(in, ce, rand_vals, relevant_memlabels)
//  {
//    compute_and_set_memlabel_ids_map(relevant_memlabels);
//  }
//
//protected:
//  //bool virtual addr_within_atomic_memlabel_bounds(expr_ref const& addr, int nbytes, memlabel_t const& atomic_ml) const override;
//  //bool virtual expr_constant_value_in_heap(expr_ref const& addr, int nbytes) const override;
//  //vector<pair<expr_ref,expr_ref>> get_mask_ranges_for_memlabel(memlabel_t const& ml) const override;
//  //array_constant_ref overlay_heap(array_constant_ref const& orig, array_constant_ref const& overlay) const override;
//  //bool evaluate_iscontiguous_memlabel(expr_ref const& e, expr_ref& const_val) const override;
//  //bool evaluate_isprobably_contiguous_memlabel(expr_ref const& e, expr_ref& const_val) const override;
//
//private:
//  //array_constant_ref get_pointsto_array_constant() const;
//  void compute_and_set_memlabel_ids_map(set<memlabel_ref> const& relevant_memlabels);
//  //bool all_bytes_in_range_agree_with_memlabel(memlabel_t const& ml, expr_ref const& size) const;
//  //bool all_bytes_outside_range_disagree_with_memlabel(memlabel_t const& ml) const;
//
//  map<memlabel_ref,unsigned> m_memlabel_ids;
//  map<unsigned,memlabel_ref> m_memlabel_ids_r;
//};

}
