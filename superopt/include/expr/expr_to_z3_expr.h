#pragma once

#include "z3++.h"

#include "support/debug.h"
#include "support/dyn_debug.h"

#include "expr/context.h"
#include "expr/expr.h"
#include "expr/relevant_memlabels.h"

namespace eqspace {

class expr_to_z3_expr : public expr_visitor
{
public:
  expr_to_z3_expr(expr_ref const &e, relevant_memlabels_t const& relevant_memlabels);

  z3::expr compute_z3_expr();
  string get_logic_string(string const& solver_name) const;
  bool has_quantifiers() const;
  bool involves_fp_to_ieee_bv() const { return m_involves_fp_to_ieee_bv; }
  static Z3_sort get_fp_sort_for_bvlen(z3::context& z3_ctx, unsigned bvlen);

  relevant_memlabels_t const& get_relevant_memlabels() const { return m_relevant_memlabels; }
  map<expr_ref,memlabel_ref> get_id_in_solver_to_memlabel_map() const { return m_id_in_solver_to_memlabel; }

protected:

  z3::expr get_memlabel_id_in_solver_z3(memlabel_t const& ml) const;
  sort_ref get_memlabel_id_in_solver_sort() const { ASSERT(m_memlabel_id_in_solver_sort); return m_memlabel_id_in_solver_sort; }

private:

  virtual void visit(expr_ref const &e) override;

  z3::expr get_z3_rounding_mode(rounding_mode_t const& rm);
  static unsigned get_fresh_id();

  z3::func_decl struct_field_getter(sort_ref const& struct_sort, int fieldnum);
  z3::func_decl struct_constructor(sort_ref const& struct_sort);
  void populate_struct_constructors_and_getters(sort_ref const& struct_sort);

  z3::expr gen_memmask_expr(z3::expr mem, z3::expr mem_alloc, memlabel_t const& ml);
  z3::expr gen_memsplice_expr(memlabel_t const& ml, z3::expr mem_alloc, z3::expr mem_belonging_to_ml, z3::expr mem_outside_ml);

  z3::expr get_axioms_expr() const;
  z3::expr get_supplement_expr(z3::expr const& e) const;

  void set_result_expr();
  z3::expr get_result() const { return m_result; }
  //z3::expr get_axioms_expr_for_op(expr::operation_kind op) const;
  //z3::expr get_axioms_expr_for_islast_in_container(container_type_ref const& container_type) const;
  z3::expr get_z3_args(expr_ref e) const
  {
    if (!m_map.count(e->get_id())) {
      context* ctx = e->get_context();
      cout << _FNLN_ << ": do not have mapping for " << expr_string(e) << ":\n" << ctx->expr_to_string_table(e) << endl;
      cout << _FNLN_ << ": m_expr = " << expr_string(m_expr) << ":\n" << ctx->expr_to_string_table(m_expr) << endl;
    }
    z3::ast a = m_map.at(e->get_id());
    return z3::expr(a.ctx(), a);
  }

  z3::func_decl get_z3_func_decl(expr_ref e) const
  {
    if (!m_map.count(e->get_id())) {
      context* ctx = e->get_context();
      cout << _FNLN_ << ": do not have mapping for " << expr_string(e) << ":\n" << ctx->expr_to_string_table(e) << endl;
      cout << _FNLN_ << ": m_expr = " << expr_string(m_expr) << ":\n" << ctx->expr_to_string_table(m_expr) << endl;
    }
    z3::ast a = m_map.at(e->get_id());
    Z3_ast A = a;
    Z3_func_decl F = Z3_to_func_decl(a.ctx(), A);
    return z3::func_decl(a.ctx(), F);
  }

  z3::expr urem(z3::expr const &a, z3::expr const &b) { return to_expr(a.ctx(), Z3_mk_bvurem(a.ctx(), a, b)); }
  z3::expr srem(z3::expr const &a, z3::expr const &b) { return to_expr(a.ctx(), Z3_mk_bvsrem(a.ctx(), a, b)); }

  static z3::expr get_z3_fp_from_bv(z3::expr a);
  static z3::expr get_z3_bv_from_fp(z3::expr a);

  static sort_ref compute_memlabel_solver_ids_and_sort(context* ctx, relevant_memlabels_t const& relevant_memlabels, map<memlabel_ref,unsigned>& ml_to_id, map<expr_ref,memlabel_ref>& id_to_ml);

  z3::expr gen_alloca_eq_z3_expr_uf(expr_ref const& arg0, expr_ref const& arg1);
  z3::expr gen_alloca_z3_expr(z3::expr const& mem_alloc, memlabel_t const& ml, z3::expr const& addr, z3::expr const& size);
  z3::expr gen_dealloc_z3_expr(z3::expr const& mem_alloc, memlabel_t const& ml, z3::expr const& addr, z3::expr const& size);
  z3::expr gen_heap_alloc_z3_expr(z3::expr const& mem_alloc, memlabel_t const& ml, z3::expr const& addr, z3::expr const& size);
  z3::expr gen_heap_dealloc_z3_expr(z3::expr const& mem_alloc, memlabel_t const& ml, z3::expr const& addr, z3::expr const& size);

  z3::expr gen_memmasks_are_equal_z3_expr(z3::expr const& mem1, z3::expr const& mem_alloc1, z3::expr const& mem2_or_const_val, z3::expr const& mem_alloc2, memlabel_t const& ml, bool mem2_is_const);
  z3::expr gen_memmasks_are_equal_z3_expr_uf(expr_ref const& mem1, expr_ref const& mem_alloc1, expr_ref const& mem2, expr_ref const& mem_alloc2, memlabel_t const& ml);
  z3::expr gen_memmasks_are_equal_else_z3_expr(z3::expr const& mem1, z3::expr const& mem_alloc1, z3::expr const& mem2, z3::expr const& mem_alloc2, memlabel_t const& ml, z3::expr const& defval);
  //z3::expr gen_ismemlabel_constraints_for_z3(z3::expr const& mem_alloc, z3::expr const& addr, z3::expr const& count, memlabel_t const &cml);
  z3::expr gen_iscontiguous_memlabel_constraints_for_z3(z3::expr const& mem_alloc, z3::expr const& lb, z3::expr const& ub, memlabel_t const &cml);
  z3::expr gen_isprobably_contiguous_memlabel_constraints_for_z3(z3::expr const& mem_alloc, z3::expr const& lb, z3::expr const& ub, memlabel_t const &cml);
  z3::expr gen_issubsuming_memlabel_for_constraints_for_z3(z3::expr const& mem_alloc, z3::expr const& subsuming_lb, z3::expr const& subsuming_ub, memlabel_t const &subsuming_ml, memlabel_t const& subsumed_mls);
  z3::expr gen_belongs_to_same_region_constraints_for_z3(z3::expr const& mem_alloc, z3::expr const& lb, z3::expr const& count) const;
  z3::expr gen_region_agrees_with_memlabel_constraints_for_z3(z3::expr const& mem_alloc, z3::expr const& lb, z3::expr const& count, memlabel_t const& mls) const;
  z3::expr gen_memlabel_is_absent_constraints_for_z3(z3::expr const& mem_alloc, memlabel_t const& ml);
  z3::expr gen_store_uninit_z3_expr(z3::expr const& mem, z3::expr const& addr, z3::expr const& count, z3::expr const& nonce);

  z3::expr gen_function_call_z3_expr(expr_ref const& e);

  z3::expr get_z3_fp_from_floatx(expr_ref const& a);
  z3::expr get_z3_floatx_bv_from_fp(expr_ref const& a);

  z3::sort memlabel_sort() const;
  z3::sort_vector function_call_sort_vector_to_z3_sort_vector(const vector<sort_ref>& s) const;
  z3::sort sort_to_z3_sort(sort_ref const& s) const;

  bool memlabel_is_preallocated(memlabel_t const& ml) const;

  z3::expr gen_memlabel_constraint_for_z3_using_func(memlabel_t const& ml, z3::sort s, function<z3::expr(z3::expr const&,z3::expr const&)> func) const;
  z3::expr gen_memlabels_constraint_for_z3_using_func(set<memlabel_ref> const& ml_set, z3::sort s, function<z3::expr(z3::expr const&,z3::expr const&)> func) const;
  //z3::expr gen_isheap_constraint_for_z3(z3::expr addr, z3::expr addr_last, relevant_memlabels_t const& relevant_memlabels, z3::sort s);
  //z3::expr gen_range_does_not_belong_to_memlabels_constraint_for_z3(z3::expr addr, z3::expr addr_last, set<memlabel_ref> const& ml_set, z3::sort s);
  z3::expr gen_isheap_constraint_for_z3(z3::expr addr, z3::sort s) const;
  z3::expr gen_isheap_constraint_for_z3(z3::expr addr, z3::expr addr_last, z3::sort s) const;
  z3::expr gen_addr_constraint_for_preallocated_memlabel(z3::expr const& addr, memlabel_t const& aml) const;
  z3::expr gen_addr_range_constraint_for_preallocated_memlabel(z3::expr const& addr, z3::expr const& addr_last, memlabel_t const& aml) const;

  void add_supplement_expr_for(z3::expr const& ez, z3::expr const& se);

private:
  z3::expr gen_alloca_dealloc_z3_expr_uf(z3::expr const& mem_alloc, memlabel_t const& from_ml, memlabel_t const& to_ml, z3::expr const& addr, z3::expr const& size);
  z3::expr gen_alloca_dealloc_z3_expr(z3::expr const& mem_alloc, memlabel_t const& from_ml, memlabel_t const& to_ml, z3::expr const& addr, z3::expr const& size);
  z3::expr gen_alloca_z3_expr_uf(z3::expr const& mem_alloc, memlabel_t const& ml, z3::expr const& addr, z3::expr const& size);
  z3::expr gen_dealloc_z3_expr_uf(z3::expr const& mem_alloc, memlabel_t const& ml, z3::expr const& addr, z3::expr const& size);
  z3::expr z3_expr_agrees_with_memlabel(z3::expr const& e, memlabel_t const &ml);

private:
  static unsigned last_id;

  context* m_ctx;
  relevant_memlabels_t const m_relevant_memlabels;
  bool m_involves_fp_to_ieee_bv = false;
  map<sort_ref, pair<z3::func_decl, z3::func_decl_vector>> m_struct_constructors_and_getters;
  map<unsigned,z3::expr> m_supplement_exprs;
  expr_ref const &m_expr;
  z3::expr m_result;
  map<expr_id_t, z3::ast> m_map;
  map<memlabel_ref,unsigned> m_memlabel_to_id_in_solver;
  map<expr_ref,memlabel_ref> m_id_in_solver_to_memlabel;
  sort_ref m_memlabel_id_in_solver_sort;
};

class Z3_expr_visitor
{
public:
  Z3_expr_visitor() = default;
  void visit_recursive(z3::expr const& e);
protected:
  virtual void visit(z3::expr const& e) = 0;
private:
  set<unsigned> m_visited;
};

class Z3_expr_metadata : public Z3_expr_visitor
{
  bool m_has_array       = false;
  bool m_has_uf          = false;
  bool m_has_fp          = false;
  unsigned m_quantifiers = 0;
protected:
  virtual void visit(z3::expr const& e);
public:
  Z3_expr_metadata(z3::expr const& e)
  {
    visit_recursive(e);
  }
  bool has_array() const { return m_has_array; }
  bool has_uf() const { return m_has_uf; }
  bool has_fp() const { return m_has_fp; }
  unsigned num_quanitifers() const { return m_quantifiers; }
};

}
