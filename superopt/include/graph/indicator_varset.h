#pragma once
#include <vector>
#include <list>
#include "expr/expr.h"
#include "support/consts.h"
#include "support/types.h"
#include "support/debug.h"
#include "support/utils.h"
#include "expr/consts_struct.h"
#include "graph/indicator_var.h"

namespace eqspace {

class indicator_varset_t {
private:
  list<indicator_var_t> m_vars;
public:
  void push_back(indicator_var_t const& ivar) { m_vars.push_back(ivar); }
  expr_ref indicator_varset_get_expr(context *ctx) const
  {
    expr_ref ret = expr_true(ctx);
    for (auto const& iv : m_vars) {
      ret = expr_and(ret, iv.indicator_var_get_expr(ctx));
    }
    return ret;
  }
  bool equals(indicator_varset_t const& other) const
  {
    return    this->contains(other)
           && other.contains(*this);
  }
  static bool indicator_varset_compare_ids(indicator_varset_t const& a, indicator_varset_t const& b)
  {
    if (a.size() < b.size()) {
      return true;
    } else if (a.size() > b.size()) {
      return false;
    }
    auto a_iter = a.m_vars.cbegin();
    auto b_iter = b.m_vars.cbegin();
    for (; a_iter != a.m_vars.cend() && b_iter != b.m_vars.cend(); a_iter++, b_iter++) {
      bool r_is_less = indicator_var_t::indicator_var_compare_ids(*a_iter, *b_iter);
      if (r_is_less) {
        return true;
      }
      bool r_is_greater = indicator_var_t::indicator_var_compare_ids(*b_iter, *a_iter);
      if (r_is_greater) {
        return false;
      }
    }
    return false;
  }
  bool indicator_varset_implies(indicator_varset_t const& other) const
  {
    for (auto const& other_var : other.m_vars) {
      if (!this->indicator_varset_implies_indicator_var(other_var)) {
        return false;
      }
    }
    return true;
  }
  string to_string() const
  {
    std::stringstream ss;
    ss << "[";
    for (auto const& var : m_vars) {
      ss << var.to_string() << ",";
    }
    ss << "]";
    return ss.str();
  }
  string to_string_for_eq(bool use_orig_ids) const
  {
    std::stringstream ss;
    size_t n = 0;
    for (auto const& var : m_vars) {
      ss << "=IndicatorVar" << n << '\n';
      ss << var.to_string_for_eq(use_orig_ids);
    }
    return ss.str();
  }
  size_t size() const { return m_vars.size(); }
  void indicator_varset_clear() { m_vars.clear(); }
  indicator_varset_t visit_exprs(std::function<expr_ref (const string &, expr_ref const &)> f) const
  {
    indicator_varset_t ret;
    for (auto const& var : m_vars) {
      ret.push_back(var.visit_exprs(f));
    }
    return ret;
  }
  void visit_exprs(std::function<void (const string &, expr_ref const &)> f) const
  {
    for (auto const& var : m_vars) {
      var.visit_exprs(f);
    }
  }

  bool indicator_varset_eval_on_counter_example(set<memlabel_ref> const& relevant_memlabels, bool& eval_result, counter_example_t& counter_example) const
  {
    eval_result = true;
    for (auto const& var : m_vars) {
      if (!var.indicator_var_eval_on_counter_example(relevant_memlabels, eval_result, counter_example)) {
        return false;
      }
      if (!eval_result) {
        return true;
      }
    }
    return true;
  }
  static string read_indicator_varset(istream& in, context* ctx, indicator_varset_t& indvarset)
  {
    indvarset.indicator_varset_clear();
    string line;
    bool end = !!getline(in, line);
    if (!end) {
      return line;
    }
    while (is_line(line, "=IndicatorVar")) {
      indicator_var_t indvar;
      line = indicator_var_t::read_indicator_var(in, ctx, indvar);
      indvarset.push_back(indvar);
    }
    return line;
  }
private:
  bool indicator_varset_implies_indicator_var(indicator_var_t const& other_var) const
  {
    for (auto const& var : m_vars) {
      if (var.indicator_var_implies(other_var)) {
        return true;
      }
    }
    return false;
  }
  bool contains(indicator_varset_t const& other) const
  {
    for (auto const& other_var : other.m_vars) {
      bool found = false;
      for (auto const& var : this->m_vars) {
        if (var == other_var) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
    return true;
  }
};

}

namespace std{
using namespace eqspace;
template<>
struct hash<indicator_varset_t>
{
  std::size_t operator()(indicator_varset_t const &ivset) const
  {
    size_t seed = 0;
    myhash_combine<size_t>(seed, ivset.size());
    return seed;
  }
};

}


