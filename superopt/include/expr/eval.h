#pragma once

#include "expr/expr.h"
#include "expr/context.h"

namespace eqspace {

class expr_evaluates_to_constant_visitor : public expr_visitor
{
public:
  expr_evaluates_to_constant_visitor(expr_ref e, bool enable_cache = true)
    : m_ctx(e->get_context()),
      m_cs(e->get_context()->get_consts_struct()),
      m_in(e),
      m_cache_enabled(enable_cache && !m_ctx->disable_caches())
  { }

  pair<expr_eval_status_t, expr_ref> expr_evaluate();

protected:
  virtual bool evaluate_var(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_select(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_store(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_store_uninit(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_memmask(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_memmasks_are_equal(expr_ref const& e, expr_ref& const_val) const;
  //virtual bool evaluate_memsplice(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_function_call(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_issubsuming_memlabel_for(expr_ref const& e, expr_ref& const_val) const;
  //virtual bool evaluate_const(expr_ref const& e, expr_ref& const_val) const;
  //virtual bool evaluate_eq(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_alloca(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_heap_alloc(expr_ref const& e, expr_ref& const_val) const;
  //virtual bool evaluate_alloca_ptr(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_iscontiguous_memlabel(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_isprobably_contiguous_memlabel(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_dealloc(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_heap_dealloc(expr_ref const& e, expr_ref& const_val) const;

  bool evaluate_select_shadow_bool(expr_ref const& e, expr_ref& const_val) const;
  bool evaluate_store_shadow_bool(expr_ref const& e, expr_ref& const_val) const;

  //bool addr_within_memlabel_bounds(expr_ref const& mem_alloc, expr_ref const& addr_begin, expr_ref const& addr_end, memlabel_t const &composite_ml_in) const;
  //bool addr_within_memlabel_bounds(expr_ref const& mem_alloc, expr_ref const& addr_begin, int count, memlabel_t const &composite_ml_in) const;
  bool addr_range_has_memlabels(expr_ref const& mem_alloc, expr_ref const &addr_begin, int count, memlabel_t const &composite_ml_in) const;
  bool addr_range_has_memlabels(expr_ref const& mem_alloc, expr_ref const &addr_begin, expr_ref const& addr_end, memlabel_t const &composite_ml_in) const;


  context *m_ctx;
  consts_struct_t const &m_cs;
  map<unsigned, pair<expr_eval_status_t, expr_ref>> m_map;

private:
  //bool addr_within_atomic_memlabel_bounds(expr_ref const& mem_alloc, expr_ref const& addr_begin, expr_ref const& addr_end, memlabel_t const& atomic_ml) const;

  bool evaluate_eq(expr_ref const& e, expr_ref& const_val) const;
  bool evaluate_const(expr_ref const& e, expr_ref& const_val) const;
  //bool evaluate_ismemlabel(expr_ref const& e, expr_ref& const_val) const;
  bool evaluate_belongs_to_same_region(expr_ref const& e, expr_ref& const_val) const;
  bool evaluate_region_agrees_with_memlabel(expr_ref const& e, expr_ref& const_val) const;
  bool evaluate_memlabel_is_absent(expr_ref const& e, expr_ref& const_val) const;

  static bool fcmp_op_on_consts(expr::operation_kind op, float_max_t f0, float_max_t f1, sort_ref const& fsort);
  static long double farith_op_on_consts(expr::operation_kind op, rounding_mode_t const& rm, float_max_t f0, float_max_t f1, sort_ref const& fsort);
  static float_max_t float_truncate_to_sort(float_max_t f0, sort_ref const& fsort);
  void previsit(expr_ref const &e, int interesting_args[], int &num_interesting_args);
  void visit(expr_ref const &e);
  static string eval_retval_to_string(expr_eval_status_t const& status, expr_ref const& const_val);

  expr_ref m_in;
  bool m_cache_enabled;
};

expr_ref expr_evaluates_to_constant(expr_ref const &e/*, expr_ref &const_val*/);

}
