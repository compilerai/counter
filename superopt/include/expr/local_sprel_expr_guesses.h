#pragma once

#include <vector>
#include <list>
#include <optional>

#include "support/consts.h"
#include "support/types.h"
#include "support/debug.h"

#include "expr/consts_struct.h"
#include "expr/allocsite.h"

namespace eqspace {

class local_sprel_expr_guesses_t {
  map<allocsite_t,expr_ref> m_allocsite_to_addr_map;
public:
  local_sprel_expr_guesses_t() {}
  local_sprel_expr_guesses_t(vector<pair<allocsite_t, expr_ref>> const &init_pairs)
  {
    for (auto const& ip : init_pairs) {
      bool success = this->add_local_sprel_expr_guess(ip.first, ip.second);
      ASSERT(success);
    }
  }

  local_sprel_expr_guesses_t(allocsite_t const& lid, expr_ref lsprel)
  {
    bool success = this->add_local_sprel_expr_guess(lid, lsprel);
    ASSERT(success);
  }

  local_sprel_expr_guesses_t visit_exprs(std::function<expr_ref (const string &, expr_ref const &)> f) const
  {
    local_sprel_expr_guesses_t new_local_sprel_expr_set;
    for (auto const& lse : m_allocsite_to_addr_map) {
      auto new_e = f("local_sprel_guess", lse.second);
      bool success = new_local_sprel_expr_set.add_local_sprel_expr_guess(lse.first, new_e);
      ASSERT(success);
    }
    return new_local_sprel_expr_set;
  }

  void visit_exprs(std::function<void (const string &, expr_ref const &)> f) const
  {
    for (const auto &lse : m_allocsite_to_addr_map) {
      f("local_sprel_guess", lse.second);
    }
  }

  bool add_local_sprel_expr_guess(allocsite_t const& local, expr_ref sprel_expr)
  {
    if (m_allocsite_to_addr_map.count(local))
      return false;
    m_allocsite_to_addr_map.insert(make_pair(local, sprel_expr));
    return true;
  }

  map<allocsite_t,expr_ref> const& get_local_sprel_expr_map() const
  {
    return m_allocsite_to_addr_map;
  }

  local_sprel_expr_guesses_t get_common_mappings(local_sprel_expr_guesses_t const& other) const
  {
    local_sprel_expr_guesses_t ret;
    ret.m_allocsite_to_addr_map = map_intersect(this->get_local_sprel_expr_map(), other.get_local_sprel_expr_map());
    return ret;
  }

  bool is_empty() const { return m_allocsite_to_addr_map.size() == 0; }
  size_t size() const   { return m_allocsite_to_addr_map.size(); }

  bool union_would_create_conflict(local_sprel_expr_guesses_t const &other) const
  {
    return !static_cast<bool>(try_union(other));
  }

  bool union_would_create_conflict(pair<allocsite_t, expr_ref> const &local_sprel_expr_guess) const
  {
    return union_would_create_conflict(local_sprel_expr_guesses_t(local_sprel_expr_guess.first, local_sprel_expr_guess.second));
  }

  void local_sprel_expr_guesses_union(local_sprel_expr_guesses_t const &other)
  {
    bool success = check_conflict_and_union(other);
    ASSERT(success);
  }

  bool check_conflict_and_union(local_sprel_expr_guesses_t const &other)
  {
    auto ret = try_union(other);
    if (!ret)
      return false;
    *this = *ret;
    return true;
  }

  bool operator<(local_sprel_expr_guesses_t const &other) const
  {
    if (m_allocsite_to_addr_map.size() != other.m_allocsite_to_addr_map.size()) {
      return m_allocsite_to_addr_map.size() < other.m_allocsite_to_addr_map.size();
    }
    for (auto iter1 = m_allocsite_to_addr_map.begin(), iter2 = other.m_allocsite_to_addr_map.begin();
         iter1 != m_allocsite_to_addr_map.end() && iter2 != other.m_allocsite_to_addr_map.end();
         iter1++, iter2++) {
      if (*iter1 != *iter2)
        return *iter1 < *iter2;
    }
    return false;
  }

  bool is_weaker_than(local_sprel_expr_guesses_t const &other) const
  {
    for (auto const& p : other.m_allocsite_to_addr_map) {
      auto itr = this->m_allocsite_to_addr_map.find(p.first);
      if (   itr == this->m_allocsite_to_addr_map.end()
          || itr->second != p.second)
        return false;
    }
    return true;
  }

  bool lsprel_guesses_imply(local_sprel_expr_guesses_t const &other) const
  {
    return other.is_weaker_than(*this);
  }

  string to_string() const
  {
    stringstream ss;
    for (auto p : m_allocsite_to_addr_map) {
      ss << "EQA(local." << p.first.allocsite_to_string() << ", " << expr_string(p.second) << "),";
    }
    return ss.str();
  }

  void local_sprel_expr_guesses_to_stream(ostream& os) const;
  string local_sprel_expr_guesses_from_stream(istream& is, context* ctx);

  bool addr_will_conflict_with_lsprel_exprs(expr_ref const& addr) const;
  bool lsprel_exprs_are_all_different() const;

  bool local_sprel_expr_guesses_eval_on_counter_example(bool &eval_result, counter_example_t &counter_example) const;

  bool have_mapping_for_local(allocsite_t const& lid) const;
  expr_ref get_mapping_for_local(allocsite_t const& lid) const;

private:
  std::optional<local_sprel_expr_guesses_t> try_union(local_sprel_expr_guesses_t const &other) const
  {
    local_sprel_expr_guesses_t new_guesses = *this;
    for (auto const& p : other.get_local_sprel_expr_map()) {
      bool success = new_guesses.add_local_sprel_expr_guess(p.first, p.second);
      if (!success)
        return {};
    }
    if (!new_guesses.lsprel_exprs_are_all_different())
      return {};
    return new_guesses;
  }

};

string read_local_sprel_expr_assumptions(istream &in, context *ctx, local_sprel_expr_guesses_t &lsprel_guesses);

}
