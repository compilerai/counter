#include "tfg/precond.h"
#include "tfg/tfg.h"
#include "expr/consts_struct.h"

#include "support/timers.h"

namespace eqspace {

bool
precond_t::precond_is_trivial() const
{
  return    m_guard.edge_guard_is_trivial()
         && m_indicator_vars.size() == 0
         && m_local_sprel_expr_assumes.local_sprel_expr_guesses_is_trivial();
}

bool
precond_t::precond_implies(precond_t const &other) const
{
  return    m_guard.edge_guard_implies(other.m_guard)
         && m_indicator_vars.indicator_varset_implies(other.m_indicator_vars)
         && m_local_sprel_expr_assumes.lsprel_guesses_imply(other.m_local_sprel_expr_assumes);
}

bool
precond_t::preconds_are_incompatible(precond_t const &other) const
{
  NOT_IMPLEMENTED();
}

bool
precond_t::operator==(precond_t const &other) const
{
  return m_guard == other.m_guard && m_indicator_vars.equals(other.m_indicator_vars); //XXX: 25/10/19: wonder why local_sprel_expr_assumes are not included here; need a comment at least
}

/*bool
precond_t::equals_mod_lsprels(precond_t const &other) const
{
  return m_guard == other.m_guard;
}*/

bool
precond_t::operator<(precond_t const &other) const
{
  if (m_guard < other.m_guard) {
    return true;
  } else if (other.m_guard < m_guard) {
    return false;
  }
  // stronger is smaller
  if (m_indicator_vars.indicator_varset_implies(other.m_indicator_vars)) {
    return true;
  }
  if (other.m_indicator_vars.indicator_varset_implies(m_indicator_vars)) {
    return false;
  }
  return m_local_sprel_expr_assumes.lsprel_guesses_imply(other.m_local_sprel_expr_assumes);
}

bool
precond_t::equals(precond_t const &other) const
{
  return    m_guard == other.m_guard
         && m_indicator_vars.equals(other.m_indicator_vars)
         && m_local_sprel_expr_assumes.equals(other.m_local_sprel_expr_assumes);
}

expr_ref
precond_t::precond_get_expr(tfg const &t, bool simplified) const
{
  //autostop_timer func_timer(__func__); // funcion called too frequently, use low overhead timers!
  context *ctx = t.get_context();
  expr_ref local_sprel_expr_guard = m_local_sprel_expr_assumes.get_expr();
  expr_ref ret = m_guard.edge_guard_get_expr(t, simplified);
  ret = expr_and(ret, m_indicator_vars.indicator_varset_get_expr(ctx));
  if (!local_sprel_expr_guard) {
    return ret;
  } else {
    return expr_and(ret, local_sprel_expr_guard);
  }
}

expr_ref
precond_t::precond_get_expr_for_lsprels(context *ctx) const
{
  expr_ref local_sprel_expr_guard = m_local_sprel_expr_assumes.get_expr();
  if (!local_sprel_expr_guard) {
    local_sprel_expr_guard = expr_true(ctx);
  }
  return local_sprel_expr_guard;
}

void
precond_t::subtract_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &guesses)
{
  m_local_sprel_expr_assumes.minus(guesses);
}

void
precond_t::add_local_sprel_expr_assumes_as_guards(local_sprel_expr_guesses_t const &local_sprel_expr_assumes)
{
  m_local_sprel_expr_assumes.set_union(local_sprel_expr_assumes);
}

string
precond_t::precond_to_string() const
{
  stringstream ss;
  ss << ", local_sprel_expr_assumes = " << m_local_sprel_expr_assumes.to_string();
  ss << ", indicator_vars = " << m_indicator_vars.to_string();
  ss << ", guard = " << m_guard.edge_guard_to_string();
  return ss.str();
}

string
precond_t::precond_to_string_for_eq(bool use_orig_ids) const
{
  stringstream ss;
  ss << "=LocalSprelAssumptions:\n" << m_local_sprel_expr_assumes.to_string_for_eq(use_orig_ids);
  if (m_indicator_vars.size()) {
    ss << "=IndicatorVars:\n" << m_indicator_vars.to_string_for_eq(use_orig_ids);
  }
  ss << m_guard.edge_guard_to_string_for_eq();
  return ss.str();
}

local_sprel_expr_guesses_t const &
precond_t::get_local_sprel_expr_assumes() const
{
  return m_local_sprel_expr_assumes;
}

void
precond_t::set_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &lsprel_guesses)
{
  m_local_sprel_expr_assumes = lsprel_guesses;
}

void
precond_t::precond_clear()
{
  m_guard.edge_guard_clear();
  m_indicator_vars.indicator_varset_clear();
  m_local_sprel_expr_assumes.lsprels_clear();
}

edge_guard_t const &
precond_t::precond_get_guard() const
{
  return m_guard;
}

void
precond_t::precond_set_guard(edge_guard_t const &eg)
{
  m_guard = eg;
}

map<local_id_t, expr_ref>
precond_t::get_local_sprel_expr_map() const
{
  return m_local_sprel_expr_assumes.get_local_sprel_expr_map();
}

map<expr_id_t, pair<expr_ref, expr_ref>>
precond_t::get_local_sprel_expr_submap() const
{
  return m_local_sprel_expr_assumes.get_local_sprel_expr_submap();
}

void
precond_t::visit_exprs(std::function<expr_ref (const string &, expr_ref const &)> f)
{
  m_indicator_vars = m_indicator_vars.visit_exprs(f);
  m_local_sprel_expr_assumes = m_local_sprel_expr_assumes.visit_exprs(f);
}

void
precond_t::visit_exprs(std::function<void (const string &, expr_ref const &)> f) const
{
  m_indicator_vars.visit_exprs(f);
  m_local_sprel_expr_assumes.visit_exprs(f);
}

bool
precond_t::precond_eval_on_counter_example(set<memlabel_ref> const& relevant_memlabels, bool &eval_result, counter_example_t &counter_example) const
{
  if (!m_guard.is_true()) {
    return false; //we don't know how to evaluate the edge guard; simply return evaluation failure.
  }
  bool success = m_indicator_vars.indicator_varset_eval_on_counter_example(relevant_memlabels, eval_result, counter_example);
  if (!success) {
    return false;
  }
  if (!eval_result) {
    return true;
  }
  return m_local_sprel_expr_assumes.local_sprel_expr_guesses_eval_on_counter_example(eval_result, counter_example);
}

bool
precond_t::precond_try_unioning_edge_guards(precond_t const& other)
{
  if (   m_indicator_vars.equals(other.m_indicator_vars)
      && m_local_sprel_expr_assumes.equals(other.m_local_sprel_expr_assumes)) {
    auto ec = m_guard.get_edge_composition();
    auto other_ec = other.m_guard.get_edge_composition();
    if (   !ec
        || !other_ec
        || (   ec->graph_edge_composition_get_from_pc() == other_ec->graph_edge_composition_get_from_pc()
            && ec->graph_edge_composition_get_to_pc() == other_ec->graph_edge_composition_get_to_pc())) {
      m_guard.add_guard_in_parallel(other.m_guard);
      return true;
    }
  }
  return false;
}

string
precond_t::read_precond(istream& in/*, state const& start_state*/, context* ctx, precond_t& precond)
{
  string line;
  getline(in, line);
  ASSERT(is_line(line, "=LocalSprelAssumptions:"));
  line = read_local_sprel_expr_assumptions(in, ctx, precond.m_local_sprel_expr_assumes);
  if (is_line(line, "=IndicatorVars:")) {
    ASSERT(is_line(line, "=IndicatorVars:"));
    line = indicator_varset_t::read_indicator_varset(in, ctx, precond.m_indicator_vars);
  }
  line = edge_guard_t::read_edge_guard(in, line/*, start_state*/, ctx, precond.m_guard);
  return line;
}

}
