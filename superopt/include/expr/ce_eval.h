#pragma once

#include "expr/eval.h"

namespace eqspace {

class ce_eval_expr_visitor : public expr_evaluates_to_constant_visitor {
public:
  ce_eval_expr_visitor(expr_ref const& in, counter_example_t const &ce, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels, bool check_bounds)
    : expr_evaluates_to_constant_visitor(in, false), // disable cache
      m_ce(ce),
      m_rand_vals(rand_vals),
      m_check_bounds(check_bounds),
      m_relevant_memlabels(relevant_memlabels),
      m_is_out_of_bound(false)
  { }

  bool evaluate_var(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_select(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_store(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_memmask(expr_ref const& e, expr_ref& const_val) const override;
  //bool evaluate_memsplice(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_function_call(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_ismemlabel(expr_ref const& e, expr_ref& const_val) const override;
  bool evaluate_memmasks_are_equal(expr_ref const& e, expr_ref& const_val) const override;

  bool is_out_of_bound() const{ return m_is_out_of_bound;}

private:
  bool addr_within_memlabel_bounds(expr_ref const& addr, int nbytes, memlabel_t const &composite_ml) const;
  bool expr_constant_value_in_heap(mybitset const& addr, int nbytes, sort_ref const& addr_sort) const;
  array_constant_ref overlay_heap(array_constant_ref const& orig, array_constant_ref const& overlay) const;
  expr_ref set_stack_memory_to_default(expr_ref const& mem) const;

  counter_example_t m_ce;
  counter_example_t &m_rand_vals;
  bool m_check_bounds;
  set<memlabel_ref> const& m_relevant_memlabels;
  mutable bool m_is_out_of_bound;
};

}
