#include "graph/memlabel_assertions.h"
#include "expr/context.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/solver_interface.h"
#include "expr/eval.h"
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/globals.h"
#include "expr/graph_local.h"

namespace eqspace {


memlabel_assertions_t::memlabel_assertions_t(istream& is, context* ctx) : m_ctx(ctx)
{
  stats_num_memlabel_assertions_constructions++;
  string line;
  bool end;

  do {
    end = !getline(is, line);
    ASSERT(!end);
  } while (line == "");
  if (line != "=Memlabel assertions") {
    cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
  }
  ASSERT(line == "=Memlabel assertions");
  m_symbol_map = graph_symbol_map_t(is);
  m_locals_map = graph_locals_map_t(is);
  m_rodata_map = rodata_map_t(is);
  line = m_local_sprel_expr_guesses.local_sprel_expr_guesses_from_stream(is, ctx);
  ASSERTCHECK(is_line(line, "=assertion"), cout << _FNLN_ << ": line = " << line << endl);
  line = read_expr(is, m_assertion, ctx);
  ASSERT(is_line(line, "=concrete_assertion"));
  line = read_expr(is, m_concrete_assertion, ctx);
  string const prefix = "=use_concrete_assertion : ";
  ASSERT(is_line(line, prefix));
  m_use_concrete_assertion = (string_to_int(line.substr(prefix.length())) != 0);
  end = !getline(is, line);
  ASSERT(!end);
  ASSERT(line == "=Memlabel assertions done");

  m_relevant_memlabels = compute_relevant_memlabels();
  m_assertion = compute_assertion();
}

void
memlabel_assertions_t::memlabel_assertions_to_stream(ostream& os) const
{
  os << "=Memlabel assertions\n";
  m_symbol_map.graph_symbol_map_to_stream(os);
  m_locals_map.graph_locals_map_to_stream(os);
  m_rodata_map.rodata_map_to_stream(os);
  m_local_sprel_expr_guesses.local_sprel_expr_guesses_to_stream(os);
  os << "=assertion\n";
  os << m_ctx->expr_to_string_table(m_assertion);
  os << "\n=concrete_assertion\n";
  os << m_ctx->expr_to_string_table(m_concrete_assertion);
  os << "\n=use_concrete_assertion : " << m_use_concrete_assertion << "\n";
  os << "=Memlabel assertions done\n";
}

memlabel_t
memlabel_assertions_t::get_memlabel_all() const
{
  return memlabel_t::memlabel_union(this->get_relevant_memlabels());
}

memlabel_t
memlabel_assertions_t::get_memlabel_all_except_locals_and_stack() const
{
  set<memlabel_ref> const& relevant_mls = this->get_relevant_memlabels();
  vector<memlabel_ref> ret;
  for (auto const& mlr : relevant_mls) {
    if (   memlabel_t::memlabel_is_stack(&mlr->get_ml())
        || mlr->get_ml().memlabel_is_local()) {
      continue;
    }
    ret.push_back(mlr);
  }
  return memlabel_t::memlabel_union(ret);
}

memlabel_t
memlabel_assertions_t::get_memlabel_all_except_stack() const
{
  set<memlabel_ref> const& relevant_mls = this->get_relevant_memlabels();
  vector<memlabel_ref> ret;
  for (auto const& mlr : relevant_mls) {
    if (memlabel_t::memlabel_is_stack(&mlr->get_ml())) {
      continue;
    }
    ret.push_back(mlr);
  }
  return memlabel_t::memlabel_union(ret);
}

//void
//memlabel_assertions_t::assign_concrete_addresses_and_set_constraints()
//{
//  NOT_IMPLEMENTED();
//}

expr_ref
memlabel_assertions_t::gen_alignment_constraint_for_addr(context* ctx, expr_ref const& addr, align_t const align)
{
  uint64_t const mask    = round_up_to_power_of_two(align)-1;
  size_t const addr_size = addr->get_sort()->get_size();
  expr_ref align_expr    = ctx->mk_bv_const(addr_size, mask);
  return ctx->mk_eq(ctx->mk_bvand(addr, align_expr), ctx->mk_zerobv(addr_size));
}

expr_ref
memlabel_assertions_t::gen_start_addr_lb_eq_assertions(context *ctx, set<memlabel_ref> const &relevant_memlabels)
{
  //autostop_timer func_timer(__func__);
  sort_ref s = ctx->get_addr_sort();
  consts_struct_t const &cs = ctx->get_consts_struct();
  expr_ref ret = expr_true(ctx);
  list<memlabel_t> sorted_mls;
  for (auto const& mlref : relevant_memlabels) {
    sorted_mls.push_back(mlref->get_ml());
  }
  sorted_mls.sort();
  for (auto const& ml : sorted_mls) {
    //tuple<expr_ref, size_t, memlabel_t> t;
    expr_ref variable_expr;
    if (ml.memlabel_represents_symbol()) {
      variable_expr = cs.get_expr_value(reg_type_symbol, ml.memlabel_get_symbol_id());
    } else if (ml.memlabel_is_local()) {
      variable_expr = cs.get_expr_value(reg_type_local, ml.memlabel_get_local_id());
    } else {
      continue;
    }
    expr_ref const& lb = get_lb_expr_for_memlabel(ctx, s, ml);
    CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lb->get_id() = " << lb->get_id() << endl);
    expr_ref eq_expr = expr_eq(variable_expr, lb);
    CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": eq_expr->get_id() = " << eq_expr->get_id() << endl);
    ret = expr_and(ret, eq_expr);
    CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": ret->get_id() = " << ret->get_id() << endl);
  }
  return ret;
}

expr_ref
memlabel_assertions_t::gen_stack_pointer_range_assertion(context *ctx) const
{
  //autostop_timer func_timer(__func__);
  memlabel_t ml = memlabel_t::memlabel_stack();
  sort_ref s = ctx->get_addr_sort();
  consts_struct_t const& cs = ctx->get_consts_struct();
  expr_ref const& lb = get_lb_expr_for_memlabel(ctx, s, ml);
  expr_ref const& ub = get_ub_expr_for_memlabel(ctx, s, ml);
  ASSERT(lb->is_bv_sort());
  size_t bvlen = lb->get_sort()->get_size();
  expr_ref const& ret1 = expr_and(ctx->mk_bvule(lb, cs.get_input_stack_pointer_const_expr()),
                                  ctx->mk_bvule(cs.get_input_stack_pointer_const_expr(), ub));

  size_t headroom = m_arg_regs.get_num_bytes_on_stack_including_retaddr();
  //cout << __func__ << " " << __LINE__ << ": headroom = " << headroom << ", m_arg_regs.size() = " << m_arg_regs.size() << endl;
  if (m_locals_map.has_vararg()) {
    NOT_IMPLEMENTED();
  }
  //expr_ref const& ret2 = ctx->mk_bvule(ctx->mk_bvadd(lb, ctx->mk_bv_const(bvlen, MIN_STACK_TAILROOM)), cs.get_input_stack_pointer_const_expr());
  expr_ref const& ret3 = ctx->mk_eq(ctx->mk_bvadd(cs.get_input_stack_pointer_const_expr(), ctx->mk_bv_const(bvlen, headroom - 1)), ub);
  //return ctx->mk_and(ret1, ctx->mk_and(ret2, ret3));
  return ctx->mk_and(ret1, ret3);
}

expr_ref
memlabel_assertions_t::gen_memlabel_is_page_aligned_assertion(context* ctx, memlabel_t const& ml)
{
  sort_ref s = ctx->get_addr_sort();
  expr_ref const& lb = get_lb_expr_for_memlabel(ctx, s, ml);
  expr_ref const& ub = get_ub_expr_for_memlabel(ctx, s, ml);

  ASSERT(g_page_size_for_safety_checks > 0);
  ASSERT(is_power_of_two(g_page_size_for_safety_checks, nullptr));
  uint32_t mask = ~(g_page_size_for_safety_checks - 1);
  expr_ref const& mask_e    = ctx->mk_bv_const(s->get_size(), (uint64_t)mask);
  expr_ref const& ubplusone = expr_bvadd(ub, ctx->mk_bv_const(s->get_size(), (uint64_t)1));
  expr_ref const& ret1      = expr_eq(ctx->mk_bvand(lb, mask_e), lb);
  expr_ref const& ret2      = expr_eq(ctx->mk_bvand(ubplusone, mask_e), ubplusone);

  return expr_and(ret1, ret2);
}

expr_ref
memlabel_assertions_t::gen_dst_symbol_addrs_assertion(context* ctx, set<memlabel_ref> const& relevant_memlabels)
{
  sort_ref s = ctx->get_addr_sort();
  consts_struct_t const &cs = ctx->get_consts_struct();
  expr_ref ret = expr_true(ctx);
  list<memlabel_t> sorted_mls;
  for (auto const& mlref : relevant_memlabels) {
    sorted_mls.push_back(mlref->get_ml());
  }
  sorted_mls.sort();
  for (auto const& ml : sorted_mls) {
    //tuple<expr_ref, size_t, memlabel_t> t;
    if (!ml.memlabel_represents_symbol()) {
      continue;
    }
    expr_ref const& dst_sym_addr_e = cs.get_expr_value(reg_type_dst_symbol_addr, ml.memlabel_get_symbol_id());
    expr_ref const& symbol_addr = cs.get_expr_value(reg_type_symbol, ml.memlabel_get_symbol_id());
    expr_ref eq_expr = expr_eq(symbol_addr, dst_sym_addr_e);
    ret = expr_and(ret, eq_expr);
  }
  return ret;
}

void
remove_local_and_local_size_vars_from_submap(map<expr_id_t,pair<expr_ref,expr_ref>>& submap, context* ctx)
{
  consts_struct_t const& cs = ctx->get_consts_struct();
  for (auto itr = submap.begin(); itr != submap.end(); ) {
    expr_ref e = itr->second.first;
    if (   cs.expr_is_local_id(e)
        || expr_is_local_size(e)
        || expr_is_local_memlabel_bound(e)) {
      itr = submap.erase(itr);
    } else {
      ++itr;
    }
  }
}

expr_ref
memlabel_assertions_t::assign_concrete_addresses_and_compute_assertion(map<expr_id_t, pair<expr_ref, expr_ref>>& submap) const
{
  query_comment qc("memlabel_assertions_sat");
  list<counter_example_t> counter_examples;
  solver::solver_res res = m_ctx->expr_is_satisfiable(m_assertion, qc, set<memlabel_ref>(), counter_examples);
  if (res != solver::solver_res_true) {
    cout << __func__ << " " << __LINE__ << ": m_assertion =\n" << m_ctx->expr_to_string_table(m_assertion) << endl;
  }
  ASSERT(res == solver::solver_res_true);
  ASSERT(counter_examples.size() > 0);
  counter_example_t const& ce = counter_examples.front();
  submap = ce.get_submap();
  remove_local_and_local_size_vars_from_submap(submap, m_ctx);
  return m_ctx->submap_get_expr(submap);
}

expr_ref
memlabel_assertions_t::compute_assertion() const
{
  //autostop_timer func_timer(__func__);
  // we need to generate range assertions for memlabels i.e.
  // (1) lb <= ub for all non-heap memlabels
  // (2) if size is known, lb+size-1 == ub -- all memlabels except heap have known sizes
  // (3) commented out (was sum of all memlabel sizes cover the entire address space (= 2**32))
  // (4) unsubsumed memlabels are non-overlapping -- this generates O(N^2) clauses!!
  // (5) subsumed memlabels are non-overlapping -- this generates O(N^2) clauses!!
  // (6) subsumed memlabels belong to one of potentially (corresponding) subsuming memlabels
  // (7) alignment constraints for symbols and locals
  // (8) start_addr == lb for symbols and locals
  // (9) input_stack_pointer belongs to the stack region
  // (10) rodata_map constraints
  // (11) stack region is page aligned
  // (12) dst_symbol_addr.N == symbol.N
  // (13) local sprel constraints

  // both heap and RODATA symbol must be present in relevant memlabels
  memlabel_ref mlr_heap = mk_memlabel_ref(memlabel_t::memlabel_heap());
  //memlabel_ref mlr_rodata = mk_memlabel_ref(memlabel_dst_rodata());
  //ASSERT(vector_find(relevant_memlabels, mlr_heap) != -1);
  //ASSERT(vector_find(relevant_memlabels, mlr_rodata) != -1);

  expr_ref ret = expr_true(m_ctx);
  sort_ref s = m_ctx->get_addr_sort();
  //variable_size_t total_size = 0;
  consts_struct_t const& cs = m_ctx->get_consts_struct();

  set<memlabel_ref> const& relevant_memlabels = this->get_relevant_memlabels();

  // (1) & (2): lb <= ub & lb+size-1 == ub, if size is known
  for (auto const& mlr : relevant_memlabels) {
    memlabel_t const& ml = mlr->get_ml();
    ASSERT(memlabel_t::memlabel_is_atomic(&ml));
    if (ml.memlabel_is_heap()) {
      continue;
    }
    expr_ref const& lb = get_lb_expr_for_memlabel(m_ctx, s, ml);
    expr_ref const& ub = get_ub_expr_for_memlabel(m_ctx, s, ml);
    ret = expr_and(ret, m_ctx->mk_bvule(lb, ub));

    // include size information
    // size information is available for symbols and stack
    expr_ref size;
    //tuple<expr_ref, size_t, memlabel_t> variable_size_memlabel_tuple;
    if (ml.memlabel_represents_symbol()) {
      symbol_id_t symbol_id = ml.memlabel_get_symbol_id();
      size = m_ctx->mk_bv_const(s->get_size(), m_symbol_map.symbol_map_get_size_for_symbol(symbol_id));
    } else if (ml.memlabel_is_stack()) {
      size = cs.get_stack_size();
    } else if (ml.memlabel_is_local()) {
      local_id_t local_id = ml.memlabel_get_local_id();
      ASSERT(m_locals_map.count(local_id));
      graph_local_t const& local = m_locals_map.at(local_id);
      if (local.is_varsize()) {
        size = m_ctx->get_local_size_expr_for_id(local_id);
      } else {
        size = m_ctx->mk_bv_const(s->get_size(), local.get_size());
      }
    } else {
      ASSERT(memlabel_t::memlabel_contains_heap(&ml));
      continue;
    }


    //bool memlabel_is_subsumed_in_another_memlabel = this->memlabel_is_subsumed_in_another_memlabel(ml);
    //ml.memlabel_is_local() || this->memlabel_represents_src_rodata_symbol(ml);

    /*if (size > 0) */{
      //if (!memlabel_is_subsumed_in_another_memlabel) {
      //  total_size += size;
      //}
      // ub == lb+size-1
      expr_ref const& ub_in_terms_of_lb = expr_bvadd(lb, expr_bvadd(size, m_ctx->mk_minusonebv(s->get_size())));
      ret = expr_and(ret, expr_eq(ub, ub_in_terms_of_lb));
    }
  }
  DYN_DEBUG2(memlabel_assertions, cout << __func__ << " " << __LINE__ << ": ret =\n" << m_ctx->expr_to_string_table(ret) << endl);

  // (3) heap gets the remaining space, thus covering the entire address space
  //variable_size_t remaining_size = (1ULL << DWORD_LEN) - total_size;
  //ASSERT(remaining_size > 0);

  //expr_ref const& heap_lb = get_lb_expr_for_memlabel(m_ctx, s, mlr_heap->get_ml());
  //expr_ref const& heap_ub = get_ub_expr_for_memlabel(m_ctx, s, mlr_heap->get_ml());
  //expr_ref const& heap_ub_in_terms_of_lb = expr_bvadd(heap_lb, m_ctx->mk_bv_const(s->get_size(), remaining_size-1));
  //ret = expr_and(ret, expr_eq(heap_ub, heap_ub_in_terms_of_lb));

  // (4) non-overlapping ranges for unsubsumed memlabels -- O(N^2) clauses
  ret = expr_and(ret, this->generate_non_overlapping_constraints(relevant_memlabels, false));
  DYN_DEBUG2(memlabel_assertions, cout << __func__ << " " << __LINE__ << ": ret =\n" << m_ctx->expr_to_string_table(ret) << endl);

  // (5) non-overlapping ranges for subsumed memlabels -- O(N^2) clauses
  ret = expr_and(ret, this->generate_non_overlapping_constraints(relevant_memlabels, true));
  DYN_DEBUG2(memlabel_assertions, cout << __func__ << " " << __LINE__ << ": ret =\n" << m_ctx->expr_to_string_table(ret) << endl);

  // (6) constraints for subsumed memlabels
  for (auto const& mlr : relevant_memlabels) {
    memlabel_t const& ml = mlr->get_ml();
    if (!this->memlabel_is_subsumed_in_another_memlabel(ml)) {
      continue;
    }
    vector<memlabel_t> subsuming_mls = this->get_memlabels_that_may_subsume(ml);
    DYN_DEBUG2(memlabel_assertions,
      cout << __func__ << " " << __LINE__ << ": subsuming memlabels for " << ml.to_string() << ":";
      for (auto const& sml : subsuming_mls) {
        cout << " " << sml.to_string();
      }
      cout << endl;
    );
    ASSERT(subsuming_mls.size());
    expr_ref s = this->get_subsume_constraints(ml, subsuming_mls);
    ret = expr_and(ret, s);
  }
  DYN_DEBUG2(memlabel_assertions, cout << __func__ << " " << __LINE__ << ": ret =\n" << m_ctx->expr_to_string_table(ret) << endl);

  // (7) alignment constraints
  for (auto const& mlr : relevant_memlabels) {
    memlabel_t const& ml = mlr->get_ml();
    if (ml.memlabel_is_local()) {
      local_id_t local_id = ml.memlabel_get_local_id();
      ASSERT(m_locals_map.count(local_id));
      graph_local_t const& local = m_locals_map.at(local_id);
      align_t align = local.get_alignment();
      expr_ref local_expr = cs.get_expr_value(reg_type_local, local_id);
      expr_ref align_constraint = gen_alignment_constraint_for_addr(m_ctx, local_expr, align);
      ret = expr_and(ret, align_constraint);
    } else if (ml.memlabel_represents_symbol()) {
      symbol_id_t symbol_id = ml.memlabel_get_symbol_id();
      ASSERT(m_symbol_map.count(symbol_id));
      graph_symbol_t const& sym = m_symbol_map.at(symbol_id);
      align_t align = sym.get_alignment();
      expr_ref sym_expr = cs.get_expr_value(reg_type_symbol, symbol_id);
      expr_ref align_constraint = gen_alignment_constraint_for_addr(m_ctx, sym_expr, align);
      ret = expr_and(ret, align_constraint);
    }
  }
  DYN_DEBUG2(memlabel_assertions, cout << __func__ << " " << __LINE__ << ": ret =\n" << m_ctx->expr_to_string_table(ret) << endl);

  // (8) start_addr == lb for symbols and locals
  ret = expr_and(ret, gen_start_addr_lb_eq_assertions(m_ctx, relevant_memlabels));
  DYN_DEBUG2(memlabel_assertions, cout << __func__ << " " << __LINE__ << ": ret =\n" << m_ctx->expr_to_string_table(ret) << endl);

  // (9) input_stack_pointer belongs to the stack region
  ret = expr_and(ret, gen_stack_pointer_range_assertion(m_ctx));
  DYN_DEBUG2(memlabel_assertions, cout << __func__ << " " << __LINE__ << ": ret =\n" << m_ctx->expr_to_string_table(ret) << endl);

  // (10) rodata_map constraints
  ret = expr_and(ret, m_rodata_map.get_assertions(m_ctx));

  // (11) stack region is page aligned
  ret = expr_and(ret, gen_memlabel_is_page_aligned_assertion(m_ctx, memlabel_t::memlabel_stack()));

  // (12) dst_symbol_addr.N == symbol.N
  ret = expr_and(ret, gen_dst_symbol_addrs_assertion(m_ctx, relevant_memlabels));

  DYN_DEBUG2(memlabel_assertions, cout << __func__ << " " << __LINE__ << ": ret =\n" << m_ctx->expr_to_string_table(ret) << endl);

  return ret;
}

expr_ref
memlabel_assertions_t::generate_non_overlapping_constraints(set<memlabel_ref> const& relevant_memlabels, bool consider_subsumed_memlabels) const
{
  sort_ref s = m_ctx->get_addr_sort();
  expr_ref ret = expr_true(m_ctx);
  for (auto const& mlri : relevant_memlabels) {
    memlabel_t const& mli = mlri->get_ml();
    if (mli.memlabel_is_heap()) {
      continue;
    }
    if (!consider_subsumed_memlabels && this->memlabel_is_subsumed_in_another_memlabel(mli)) {
      continue;
    }
    if (consider_subsumed_memlabels && !this->memlabel_is_subsumed_in_another_memlabel(mli)) {
      continue;
    }
    expr_ref mli_lb = get_lb_expr_for_memlabel(m_ctx, s, mli);
    expr_ref mli_ub = get_ub_expr_for_memlabel(m_ctx, s, mli);

    for (auto const& mlrj : relevant_memlabels) {
      memlabel_t const& mlj = mlrj->get_ml();
      if (mlj.memlabel_is_heap()) {
        continue;
      }
      if (   mlri == mlrj
          || this->memlabel_is_subsumed_in_another_memlabel(mli)) {
        continue;
      }
      if (!consider_subsumed_memlabels && this->memlabel_is_subsumed_in_another_memlabel(mlj)) {
        continue;
      }
      if (consider_subsumed_memlabels && !this->memlabel_is_subsumed_in_another_memlabel(mlj)) {
        continue;
      }
      expr_ref mlj_lb = get_lb_expr_for_memlabel(m_ctx, s, mlj);
      expr_ref mlj_ub = get_ub_expr_for_memlabel(m_ctx, s, mlj);

      // for each pair (mli, mlj)
      expr_ref i_not_in_j = expr_and(
                              expr_not(
                               expr_and(m_ctx->mk_bvuge(mli_lb, mlj_lb),
                                        m_ctx->mk_bvule(mli_lb, mlj_ub))),
                              expr_not(
                               expr_and(m_ctx->mk_bvuge(mli_ub, mlj_lb),
                                        m_ctx->mk_bvule(mli_ub, mlj_ub))));
      ret = expr_and(ret, i_not_in_j);
    }
  }
  return ret;
}

expr_ref
memlabel_assertions_t::get_subsume_constraints(memlabel_t const& needle, vector<memlabel_t> const& haystack) const
{
  expr_ref ret = expr_false(m_ctx);
  sort_ref s = m_ctx->mk_bv_sort(DWORD_LEN);
  for (auto const& ml_subsuming : haystack) {
    expr_ref ml_lb = get_lb_expr_for_memlabel(m_ctx, s, needle);
    expr_ref ml_ub = get_ub_expr_for_memlabel(m_ctx, s, needle);

    expr_ref mls_lb = get_lb_expr_for_memlabel(m_ctx, s, ml_subsuming);
    expr_ref mls_ub = get_ub_expr_for_memlabel(m_ctx, s, ml_subsuming);

    expr_ref iret = expr_true(m_ctx);
    iret = expr_and(iret, m_ctx->mk_bvuge(ml_lb, mls_lb));
    iret = expr_and(iret, m_ctx->mk_bvule(ml_ub, mls_ub));
    ret = expr_or(ret, iret);
  }
  return ret;
}

vector<memlabel_t>
memlabel_assertions_t::get_memlabels_that_may_subsume(memlabel_t const& ml) const
{
  if (ml.memlabel_is_local()) {
    return {memlabel_t::memlabel_stack()};
  }
  if (memlabel_t::memlabel_represents_symbol(&ml)) {
    symbol_id_t symbol_id = ml.memlabel_get_symbol_id();
    bool is_const = m_symbol_map.at(symbol_id).is_const();
    if (is_const) {
      align_t align = m_symbol_map.at(symbol_id).get_alignment();
      vector<memlabel_t> ret;
      for (auto const& sym : m_symbol_map.get_map()) {
        symbol_id_t other_symbol_id = sym.first;
        bool other_is_dst_const_symbol = m_symbol_map.at(other_symbol_id).is_dst_const_symbol();
        align_t other_align = m_symbol_map.at(other_symbol_id).get_alignment();
        //align_t other_symbol_size = m_symbol_map.at(other_symbol_id).get_size();
        if (other_is_dst_const_symbol && other_align >= align) {
          ret.push_back(memlabel_t::memlabel_symbol(other_symbol_id/*, other_symbol_size*/, true));
        }
      }
      return ret;
    }
  }
  return {};
}

bool
memlabel_assertions_t::memlabel_is_subsumed_in_another_memlabel(memlabel_t const& ml) const
{
  if (ml.memlabel_is_local()) {
    return true;
  }
  if (memlabel_t::memlabel_represents_symbol(&ml)) {
    symbol_id_t symbol_id = ml.memlabel_get_symbol_id();
    bool is_const = m_symbol_map.at(symbol_id).is_const();
    bool is_dst_const_symbol = m_symbol_map.at(symbol_id).is_dst_const_symbol();
    if (is_const && !is_dst_const_symbol) {
      return true;
    }
  }
  return false;
}

set<memlabel_ref>
memlabel_assertions_t::compute_relevant_memlabels() const
{
  set<memlabel_ref> ret;
  for (auto const& sym : m_symbol_map.get_map()) {
    ret.insert(mk_memlabel_ref(memlabel_t::memlabel_symbol(sym.first/*, sym.second.get_size()*/, sym.second.is_const())));
  }
  for (auto const& lp :m_locals_map.get_map()) {
    ret.insert(mk_memlabel_ref(memlabel_t::memlabel_local(lp.first/*, m_locals_map.at(i).second*/)));
  }
  ret.insert(mk_memlabel_ref(memlabel_t::memlabel_stack()));
  ret.insert(mk_memlabel_ref(memlabel_t::memlabel_heap()));
  return ret;
}

//expr_ref
//memlabel_assertions_t::compute_heap_constraint_expr() const
//{
//  expr_ref ret = expr_true(m_ctx);
//  expr_ref addr = m_ctx->mk_var(HEAP_CONSTRAINT_EXPR_HOLE_VARNAME, m_ctx->mk_bv_sort(DWORD_LEN));
//  vector<memlabel_ref> relevant_memlabels = this->compute_relevant_memlabels();
//  for (auto const& mlr : relevant_memlabels) {
//    memlabel_t const& ml = mlr->get_ml();
//    if (ml.memlabel_is_heap()) {
//      continue;
//    }
//    expr_ref is_ml = m_ctx->mk_ismemlabel(addr, 0, mlr);
//    ret = expr_and(ret, expr_not(is_ml));
//  }
//  return ret;
//}

void
memlabel_assertions_t::populate_assertions_and_relevant_memlabels()
{
  m_relevant_memlabels = compute_relevant_memlabels();
  m_assertion = compute_assertion();
  DYN_DEBUG(memlabel_assertions, cout << _FNLN_ << ": m_assertion =\n" << m_ctx->expr_to_string_table(m_assertion) << endl);
  m_concrete_assertion = assign_concrete_addresses_and_compute_assertion(m_submap);
  m_concrete_assertion = expr_and(m_concrete_assertion,
                                  expr_evaluates_to_constant(m_ctx->expr_substitute(m_assertion, m_submap)));
  DYN_DEBUG(memlabel_assertions, cout << _FNLN_ << ": m_concrete_assertion =\n" << m_ctx->expr_to_string_table(m_concrete_assertion) << endl);
}


}
