#pragma once

#include "support/types.h"
#include "support/dshared_ptr.h"
#include "support/dyn_debug.h"

#include "expr/graph_local.h"
#include "expr/graph_arg_regs.h"
#include "expr/relevant_memlabels.h"
#include "expr/graph_locals_map.h"

#include "gsupport/graph_symbol.h"
#include "gsupport/rodata_map.h"

namespace eqspace {


class memlabel_assertions_t {
protected:
  graph_symbol_map_t m_symbol_map;
  graph_locals_map_t m_locals_map;
  graph_arg_regs_t m_arg_regs;
  rodata_map_t m_rodata_map;
  //vector<expr_ref> m_mem_alloc_exprs;
  context* m_ctx;
  //expr_ref m_assertion;
  //expr_ref m_concrete_assertion;
  //set<memlabel_ref> m_relevant_memlabels;
  map<expr_id_t, pair<expr_ref, expr_ref>> m_global_submap;

  bool m_use_concrete_assertion;
public:
  memlabel_assertions_t(context* ctx) : m_ctx(ctx), m_use_concrete_assertion(true)
  {
    stats_num_memlabel_assertions_constructions++;
  }
  memlabel_assertions_t(string_ref const& fname, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, graph_arg_regs_t const& arg_regs, rodata_map_t const& rodata_map/*, vector<expr_ref> const& mem_alloc_exprs, set<local_id_t> const& preallocated_locals*/, context* ctx/*, bool has_stack*/, relevant_memlabels_t const& relevant_memlabels, vector<expr_ref> const& mem_alloc_exprs) : m_symbol_map(symbol_map), m_locals_map(locals_map), m_arg_regs(arg_regs), m_rodata_map(rodata_map)/*, m_mem_alloc_exprs(mem_alloc_exprs), m_preallocated_locals(preallocated_locals)*/, m_ctx(ctx), m_use_concrete_assertion(true)/*, m_has_stack(has_stack)*/
  {
    this->assign_concrete_addresses(fname, relevant_memlabels, mem_alloc_exprs);
    stats_num_memlabel_assertions_constructions++;
  }
  memlabel_assertions_t(istream& is, context* ctx);
  memlabel_assertions_t(memlabel_assertions_t const& other) : m_symbol_map(other.m_symbol_map),
                                                              m_locals_map(other.m_locals_map),
                                                              m_arg_regs(other.m_arg_regs),
                                                              m_rodata_map(other.m_rodata_map),
                                                              //m_mem_alloc_exprs(other.m_mem_alloc_exprs),
                                                              m_ctx(other.m_ctx),
                                                              //m_assertion(other.m_assertion),
                                                              //m_concrete_assertion(other.m_concrete_assertion),
                                                              //m_relevant_memlabels(other.m_relevant_memlabels),
                                                              m_global_submap(other.m_global_submap),
                                                              m_use_concrete_assertion(other.m_use_concrete_assertion)//,
  {
    stats_num_memlabel_assertions_constructions++;
  }
  virtual ~memlabel_assertions_t()
  {
    stats_num_memlabel_assertions_destructions++;
  }
//  expr_ref get_assertion() const
//  {
//    if (m_use_concrete_assertion) {
//      DYN_DEBUG(memlabel_assertions, cout << "using concrete assertion\n");
//      return m_concrete_assertion;
//    } else {
//      return m_assertion;
//    }
//  }
  //expr_ref get_concrete_assertion() const { return m_concrete_assertion; }
  map<expr_id_t, pair<expr_ref, expr_ref>> memlabel_assertions_get_submap() const
  {
    return m_use_concrete_assertion ? m_global_submap : map<expr_id_t, pair<expr_ref, expr_ref>>();
  }
  void memlabel_assertions_to_stream(ostream& os) const;
  void memlabel_assertions_use_abstract_assertion()
  {
    ASSERT(m_use_concrete_assertion);
    m_use_concrete_assertion = false;
  }
  void memlabel_assertions_use_concrete_assertion()
  {
    ASSERT(!m_use_concrete_assertion);
    m_use_concrete_assertion = true;
  }

  bool uses_concrete_assertion() const { return m_use_concrete_assertion; }

  context* get_context() const { return m_ctx; }
  expr_ref get_abstract_assertions_expr_for_inductive_proof(string_ref const& fname, relevant_memlabels_t const& relevant_memlabels, vector<expr_ref> const& mem_alloc_exprs) const;

  expr_ref get_abstract_assertions_expr_for_preallocated_memlabels(relevant_memlabels_t const& relevant_memlabels, vector<expr_ref> const& mem_alloc_exprs) const;

protected:
  void assign_concrete_addresses(string_ref const& fname, relevant_memlabels_t const& relevant_memlabels, vector<expr_ref> const& mem_alloc_exprs);
  expr_ref generate_non_overlapping_assertions(string_ref const& fname, relevant_memlabels_t const& relevant_memlabels, vector<expr_ref> const& mem_alloc_exprs, bool ignore_symbols_non_overlapping_constraints) const;
  expr_ref gen_size_assertion_for_memlabel(memlabel_t const& ml, expr_ref const& lb, expr_ref const& ub) const;
  expr_ref gen_contiguity_assertion_for_memlabel(vector<expr_ref> const& mem_alloc_exprs, memlabel_t const& ml, expr_ref const& lb, expr_ref const& ub) const;

  expr_ref get_memlabel_size(memlabel_t const& ml) const;
  vector<memlabel_t> get_memlabels_that_may_subsume(memlabel_t const& ml) const;
  bool memlabel_is_subsumed_in_another_memlabel(memlabel_t const& ml) const;

private:
  expr_ref get_abstract_assertions_expr(string_ref const& fname, relevant_memlabels_t const& relevant_memlabels, vector<expr_ref> const& mem_alloc_exprs, bool ignore_symbols_non_overlapping_constraints = false) const;
//  expr_ref compute_assertion(string_ref const& fname, relevant_memlabels_t const& relevant_memlabels) const;
  void remove_non_global_mappings_from_submap(map<expr_id_t,pair<expr_ref,expr_ref>>& submap) const;
  //void assign_concrete_addresses_and_set_constraints();
  //set<memlabel_ref> compute_relevant_memlabels() const;
  expr_ref generate_bounds_and_size_assertions(relevant_memlabels_t const& relevant_memlabels, vector<expr_ref> const& mem_alloc_exprs) const;
  expr_ref generate_memlabel_alignment_assertions(relevant_memlabels_t const &relevant_memlabels) const;
  expr_ref gen_stack_pointer_range_assertion() const;
  expr_ref gen_start_addr_lb_eq_assertions(relevant_memlabels_t const &relevant_memlabels) const;

  bool memlabel_refers_to_arg_local(memlabel_t const& ml) const;
  bool memlabel_refers_to_non_arg_local(memlabel_t const& ml) const;
  bool memlabel_refers_to_vararg_local(memlabel_t const& ml) const;
  bool memlabel_refers_to_rodata_mapped_src_ro_symbol(memlabel_t const& ml) const;

  expr_ref generate_non_overlapping_constraints(relevant_memlabels_t const& relevant_memlabels, bool consider_subsumed_memlabels, bool ignore_symbols_non_overlapping_constraints) const;
  expr_ref generate_subsumed_memlabels_constraints(relevant_memlabels_t const& relevant_memlabels) const;
  expr_ref get_subsume_constraints(memlabel_t const& needle, vector<memlabel_t> const& haystack) const;
  expr_ref generate_stack_subsumes_locals_constraints(relevant_memlabels_t const& relevant_memlabels, vector<expr_ref> const& mem_alloc_exprs) const;

  static expr_ref gen_memlabel_is_page_aligned_assertion(context *ctx, memlabel_t const& ml);
  static expr_ref gen_dst_symbol_addrs_assertion(context* ctx, relevant_memlabels_t const& relevant_memlabels);
  static expr_ref gen_alignment_constraint_for_addr(context *ctx, expr_ref const& addr, align_t const align);
};

dshared_ptr<memlabel_assertions_t> mk_memlabel_assertions(context* ctx);
dshared_ptr<memlabel_assertions_t> mk_memlabel_assertions(istream& is, context* ctx);
dshared_ptr<memlabel_assertions_t> mk_memlabel_assertions(string_ref const& fname, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, graph_arg_regs_t const& arg_regs, rodata_map_t const& rodata_map/*, vector<expr_ref> const& mem_alloc_exprs, set<local_id_t> const& preallocated_locals*/, context* ctx/*, bool has_stack*/, relevant_memlabels_t const& relevant_memlabels, vector<expr_ref> const& mem_alloc_exprs);
dshared_ptr<memlabel_assertions_t> mk_memlabel_assertions(dshared_ptr<memlabel_assertions_t> const other);
dshared_ptr<memlabel_assertions_t> mk_memlabel_assertions(memlabel_assertions_t const& other);

string_ref local_id_refers_to_arg(allocsite_t const& lid, graph_arg_regs_t const& arg_regs, graph_locals_map_t const& locals_map);

}
