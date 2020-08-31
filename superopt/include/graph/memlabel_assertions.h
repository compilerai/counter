#pragma once
#include "expr/graph_symbol.h"
#include "expr/graph_local.h"
#include "support/types.h"
#include "gsupport/rodata_map.h"
#include "expr/graph_arg_regs.h"

namespace eqspace {


class memlabel_assertions_t {
private:
  graph_symbol_map_t m_symbol_map;
  graph_locals_map_t m_locals_map;
  graph_arg_regs_t m_arg_regs;
  rodata_map_t m_rodata_map;
  local_sprel_expr_guesses_t m_local_sprel_expr_guesses;
  context* m_ctx;
  expr_ref m_assertion;
  expr_ref m_concrete_assertion;
  set<memlabel_ref> m_relevant_memlabels;
  map<expr_id_t, pair<expr_ref, expr_ref>> m_submap;

  bool m_use_concrete_assertion;
public:
  memlabel_assertions_t(context* ctx) : m_ctx(ctx), m_use_concrete_assertion(true)
  {
    stats_num_memlabel_assertions_constructions++;
  }
  memlabel_assertions_t(graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, graph_arg_regs_t const& arg_regs, rodata_map_t const& rodata_map, local_sprel_expr_guesses_t const& lsprels, context* ctx) : m_symbol_map(symbol_map), m_locals_map(locals_map), m_arg_regs(arg_regs), m_rodata_map(rodata_map), m_local_sprel_expr_guesses(lsprels), m_ctx(ctx), m_use_concrete_assertion(true)
  {
    stats_num_memlabel_assertions_constructions++;
    this->populate_assertions_and_relevant_memlabels();
  }
  memlabel_assertions_t(istream& is, context* ctx);
  memlabel_assertions_t(memlabel_assertions_t const& other) : m_symbol_map(other.m_symbol_map), m_locals_map(other.m_locals_map), m_arg_regs(other.m_arg_regs), m_rodata_map(other.m_rodata_map), m_local_sprel_expr_guesses(other.m_local_sprel_expr_guesses), m_ctx(other.m_ctx), m_assertion(other.m_assertion), m_concrete_assertion(other.m_concrete_assertion), m_relevant_memlabels(other.m_relevant_memlabels), m_submap(other.m_submap), m_use_concrete_assertion(other.m_use_concrete_assertion)
  {
    stats_num_memlabel_assertions_constructions++;
  }
  ~memlabel_assertions_t()
  {
    stats_num_memlabel_assertions_destructions++;
  }
  expr_ref get_assertion() const
  {
    return m_use_concrete_assertion ? m_concrete_assertion : m_assertion;
  }
  //expr_ref get_concrete_assertion() const { return m_concrete_assertion; }
  map<expr_id_t, pair<expr_ref, expr_ref>> memlabel_assertions_get_submap() const
  {
    return m_use_concrete_assertion ? m_submap : map<expr_id_t, pair<expr_ref, expr_ref>>();
  }
  void memlabel_assertions_to_stream(ostream& os) const;
  memlabel_t get_memlabel_all() const;
  memlabel_t get_memlabel_all_except_locals_and_stack() const;
  memlabel_t get_memlabel_all_except_stack() const;
  set<memlabel_ref> const& get_relevant_memlabels() const
  {
    return m_relevant_memlabels;
  }
  void memlabel_assertions_use_abstract_assertion()
  {
    ASSERT(m_use_concrete_assertion);
    m_use_concrete_assertion = false;
  }
private:
  expr_ref compute_assertion() const;
  expr_ref assign_concrete_addresses_and_compute_assertion(map<expr_id_t, pair<expr_ref, expr_ref>>& submap) const;
  //void assign_concrete_addresses_and_set_constraints();
  expr_ref get_subsume_constraints(memlabel_t const& needle, vector<memlabel_t> const& haystack) const;
  vector<memlabel_t> get_memlabels_that_may_subsume(memlabel_t const& ml) const;
  bool memlabel_is_subsumed_in_another_memlabel(memlabel_t const& ml) const;
  set<memlabel_ref> compute_relevant_memlabels() const;
  expr_ref generate_non_overlapping_constraints(set<memlabel_ref> const& relevant_memlabels, bool consider_subsumed_memlabels) const;
  expr_ref compute_heap_constraint_expr() const;
  expr_ref gen_stack_pointer_range_assertion(context *ctx) const;
  static expr_ref gen_start_addr_lb_eq_assertions(context *ctx, set<memlabel_ref> const &relevant_memlabels);
  static expr_ref gen_memlabel_is_page_aligned_assertion(context *ctx, memlabel_t const& ml);
  static expr_ref gen_dst_symbol_addrs_assertion(context* ctx, set<memlabel_ref> const& relevant_memlabels);
  static expr_ref gen_alignment_constraint_for_addr(context *ctx, expr_ref const& addr, align_t const align);
  void populate_assertions_and_relevant_memlabels();
};

}
