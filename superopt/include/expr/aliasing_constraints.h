#pragma once

#include "expr/expr.h"
//#include "gsupport/precond.h"
#include "gsupport/predicate_set.h"
#include "support/debug.h"

namespace eqspace
{

class aliasing_constraint_t {
private:
  expr_ref m_guard;
  expr_ref m_mem_alloc;
  expr_ref m_addr;
  size_t m_count;
  memlabel_ref m_memlabel;
public:
  aliasing_constraint_t(expr_ref const& guard, expr_ref const& mem_alloc, expr_ref const& addr, size_t count, memlabel_ref const& ml_ref) : m_guard(guard), m_mem_alloc(mem_alloc), m_addr(addr), m_count(count), m_memlabel(ml_ref)
  {
    stats_num_aliasing_constraint_constructions++;
  }
  aliasing_constraint_t(aliasing_constraint_t const& other) : m_guard(other.m_guard), m_mem_alloc(other.m_mem_alloc), m_addr(other.m_addr), m_count(other.m_count), m_memlabel(other.m_memlabel)
  {
    stats_num_aliasing_constraint_constructions++;
  }
  ~aliasing_constraint_t()
  {
    stats_num_aliasing_constraint_destructions++;
  }
  expr_ref const& get_guard() const { return m_guard; }
  expr_ref const& get_mem_alloc() const { return m_mem_alloc; }
  expr_ref const& get_addr() const { return m_addr; }
  size_t get_count() const { return m_count; }
  memlabel_ref const& get_ml() const { return m_memlabel; }
  expr_ref aliasing_constraint_convert_to_expr() const;
  string aliasing_constraint_to_string() const;
  string aliasing_constraint_to_string_for_eq(bool use_orig_ids = false) const;
  bool operator==(aliasing_constraint_t const& other) const
  {
    return !(*this < other) && !(other < *this);
  }
  bool equals_except_guard(aliasing_constraint_t const& other) const
  {
    if (m_addr == other.m_addr && m_mem_alloc == other.m_mem_alloc && m_count == other.m_count && m_memlabel ==  other.m_memlabel )
      return true;
    return false;
  }
  bool operator<(aliasing_constraint_t const& other) const
  {
    if (m_guard->get_id() < other.m_guard->get_id()) {
      return true;
    }
    if (m_guard->get_id() > other.m_guard->get_id()) {
      return false;
    }
    if (m_mem_alloc->get_id() < other.m_mem_alloc->get_id()) {
      return true;
    }
    if (m_mem_alloc->get_id() > other.m_mem_alloc->get_id()) {
      return false;
    }
    if (m_addr->get_id() < other.m_addr->get_id()) {
      return true;
    }
    if (m_addr->get_id() > other.m_addr->get_id()) {
      return false;
    }
    if (m_count < other.m_count) {
      return true;
    }
    if (m_count > other.m_count) {
      return false;
    }
    if (m_memlabel->get_ml() < other.m_memlabel->get_ml()) {
      return true;
    }
    if (other.m_memlabel->get_ml() < this->m_memlabel->get_ml()) {
      return false;
    }
    return false;
  }
  void visit_exprs(std::function<expr_ref (expr_ref const&)> f)
  {
    m_guard = f(m_guard);
    m_mem_alloc = f(m_mem_alloc);
    m_addr = f(m_addr);
  }
  void set_guard(expr_ref const& guard)
  {
    m_guard = guard;
  }
  void add_guard(expr_ref const& guard)
  {
    context *ctx = m_guard->get_context();
    m_guard = ctx->expr_do_simplify(expr_and(m_guard, guard));
  }
  bool aliasing_constraint_is_trivial() const;
};

class aliasing_constraints_t {
private:
  set<aliasing_constraint_t> m_ls;
public:
  aliasing_constraints_t() { }
  aliasing_constraints_t(set<aliasing_constraint_t> const& alias_cons) : m_ls(alias_cons) { }

  set<aliasing_constraint_t> const& get_ls() const { return m_ls; }
  
  void add_constraint(aliasing_constraint_t const& cons)
  {
    m_ls.insert(cons);
  }
  void aliasing_constraints_union(aliasing_constraints_t const& b)
  {
    set<aliasing_constraint_t> done_preds, ret;
    for (const auto &b_elem : b.m_ls) {
      //dedup those that are identical by or'ing their edge guards, else add them to the set
      for (const auto &a_elem : m_ls) {
        if (a_elem.equals_except_guard(b_elem) && !set_belongs(done_preds, b_elem) && !set_belongs(done_preds, a_elem))
        {
          done_preds.insert(a_elem);
          done_preds.insert(b_elem);
          expr_ref new_guard = expr_or(a_elem.get_guard(), b_elem.get_guard());
          aliasing_constraint_t new_ls = aliasing_constraint_t(a_elem);
          new_ls.set_guard(new_guard);
          ret.insert(new_ls);
        }
      }
    }
    for (const auto &b_elem : b.m_ls) {
      if (!set_belongs(done_preds, b_elem)) {
        ret.insert(b_elem);
      }
    }
    for (const auto &a_elem : m_ls) {
      if (!set_belongs(done_preds, a_elem)) {
        ret.insert(a_elem);
      }
    }
    m_ls = ret;
  }

  void visit_exprs(std::function<expr_ref (expr_ref const&)> f)
  {
    set<aliasing_constraint_t> new_ls;
    for (auto& cons : m_ls) {
      auto new_cons = cons;
      new_cons.visit_exprs(f);
      new_ls.insert(new_cons);
    }
    m_ls = new_ls;
  }

  void add_guard_to_all_constraints(expr_ref const& guard)
  {
    set<aliasing_constraint_t> new_ls;
    for (auto& cons : m_ls) {
      auto new_cons = cons;
      new_cons.add_guard(guard);
      new_ls.insert(new_cons);
    }
    m_ls = new_ls;
  }

  expr_ref convert_to_expr(context* ctx) const;
  vector<expr_ref> get_region_agrees_with_memlabel_exprs() const;
  list<predicate_ref> get_region_agrees_with_memlabel_preds() const;
  string to_string() const;
  string to_string_for_eq(bool use_orig_ids = false) const;

  bool operator==(aliasing_constraints_t const& other) const
  {
    return this->m_ls == other.m_ls;
  }

  static string aliasing_constraints_from_stream(istream& in, context* ctx, aliasing_constraints_t& alias_cons);
};

aliasing_constraints_t generate_aliasing_constraints_from_expr(expr_ref const& e, graph_memlabel_map_t const& memlabel_map);
vector<expr_ref> generate_region_agrees_with_memlabel_constraints_from_expr(expr_ref const& e, graph_memlabel_map_t const& memlabel_map);
aliasing_constraints_t generate_aliasing_constraints_from_submap(map<expr_id_t,pair<expr_ref,expr_ref>> const& submap, graph_memlabel_map_t const& memlabel_map);
}
