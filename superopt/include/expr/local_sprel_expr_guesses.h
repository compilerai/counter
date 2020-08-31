#pragma once

//#include "expr/context.h"
#include <vector>
#include <list>
#include "support/consts.h"
#include "support/types.h"
#include "support/debug.h"
#include "expr/consts_struct.h"

namespace eqspace {

class local_sprel_expr_guesses_t {
private:
  set<pair<local_id_t, expr_ref>> m_local_sprel_expr_set;
public:
  local_sprel_expr_guesses_t() {}

  local_sprel_expr_guesses_t(vector<pair<local_id_t, expr_ref>> const &init_pairs)
  {
    for (auto ip : init_pairs) {
      m_local_sprel_expr_set.insert(ip);
    }
  }

  local_sprel_expr_guesses_t(local_sprel_expr_guesses_t const &other) : m_local_sprel_expr_set(other.m_local_sprel_expr_set)
  {
  }

  bool local_sprel_expr_guesses_is_trivial() const
  {
    return m_local_sprel_expr_set.size() == 0;
  }

  void operator=(local_sprel_expr_guesses_t const &other)
  {
    m_local_sprel_expr_set = other.m_local_sprel_expr_set;
  }

  string to_string_for_eq(bool use_orig_ids = false) const;

  local_sprel_expr_guesses_t visit_exprs(std::function<expr_ref (const string &, expr_ref const &)> f)
  {
    local_sprel_expr_guesses_t new_local_sprel_expr_set;
    for (auto &lse : m_local_sprel_expr_set) {
      pair<local_id_t, expr_ref> new_entry = make_pair(lse.first, f("local_sprel_guess", lse.second));
      new_local_sprel_expr_set.m_local_sprel_expr_set.insert(new_entry);
    }
    //m_local_sprel_expr_set = new_local_sprel_expr_set;
    return new_local_sprel_expr_set;
  }

  void visit_exprs(std::function<void (const string &, expr_ref const &)> f) const
  {
    for (const auto &lse : m_local_sprel_expr_set) {
      f("local_sprel_guess", lse.second);
    }
  }

  void add_local_sprel_expr_guess(local_id_t local, expr_ref sprel_expr)
  {
    m_local_sprel_expr_set.insert(make_pair(local, sprel_expr));
  }

  set<pair<local_id_t, expr_ref>> const &get_local_sprel_expr_pairs() const
  {
    return m_local_sprel_expr_set;
  }

  bool is_empty() const
  {
    return m_local_sprel_expr_set.size() == 0;
  }

  expr_ref get_assertions(context* ctx) const;

  bool contains_conflicting_local_sprel_expr_guess(pair<local_id_t, expr_ref> const &local_sprel_expr_guess, graph_locals_map_t const &locals_map) const
  {
    local_sprel_expr_guesses_t new_guesses = *this;
    new_guesses.m_local_sprel_expr_set.insert(local_sprel_expr_guess);
    return new_guesses.is_incompatible(locals_map);
    /*for (auto ge : m_local_sprel_expr_set) {
      if (   ge.first == local_sprel_expr_guess.first
          && !is_expr_equal_syntactic(ge.second, local_sprel_expr_guess.second)) {
        return true;
      }
    }
    return false;*/
  }

  bool remove_local_sprel_expr_guess_if_it_exists(pair<local_id_t, expr_ref> const &local_sprel_expr_guess)
  {
    set<pair<local_id_t, expr_ref>> local_sprel_exprs_to_be_removed;
    for (auto ge : m_local_sprel_expr_set) {
      if (   ge.first == local_sprel_expr_guess.first
          && is_expr_equal_syntactic(ge.second, local_sprel_expr_guess.second)) {
        local_sprel_exprs_to_be_removed.insert(ge);
      }
    }

    bool ret = false;
    for (auto ge : local_sprel_exprs_to_be_removed) {
      m_local_sprel_expr_set.erase(ge);
      ret = true;
    }
    return ret;
  }

  static expr_ref local_sprel_expr_tuple_to_expr(pair<local_id_t, expr_ref> const &local_sprel_expr_pair);

  expr_ref get_expr() const
  {
    expr_ref ret;
    for (auto ge : m_local_sprel_expr_set) {
      if (!ret) {
        ret = local_sprel_expr_tuple_to_expr(ge);
      } else {
        ret = expr_and(ret, local_sprel_expr_tuple_to_expr(ge));
      }
    }
    return ret;
  }

  //set<pair<expr_ref, pair<expr_ref, expr_ref>>> get_expr_set() const
  //set<predicate> get_pred_set() const;

  void set_union(local_sprel_expr_guesses_t const &other)
  {
    ::set_union(m_local_sprel_expr_set, other.m_local_sprel_expr_set);
  }

  bool equals(local_sprel_expr_guesses_t const &other) const
  {
    if (m_local_sprel_expr_set.size() != other.m_local_sprel_expr_set.size()) {
      return false;
    }
    set<pair<local_id_t, expr_ref>>::const_iterator iter1, iter2;
    for (iter1 = m_local_sprel_expr_set.begin(), iter2 = other.m_local_sprel_expr_set.begin();
         iter1 != m_local_sprel_expr_set.end() && iter2 != other.m_local_sprel_expr_set.end();
         iter1++, iter2++) {
      if (iter1->first != iter2->first) {
        return false;
      }
      if (iter1->second != iter2->second) {
        return false;
      }
    }
    return true;
  }

  bool operator<(local_sprel_expr_guesses_t const &other) const
  {
    if (m_local_sprel_expr_set.size() < other.m_local_sprel_expr_set.size()) {
      return true;
    }
    if (m_local_sprel_expr_set.size() > other.m_local_sprel_expr_set.size()) {
      return false;
    }
    set<pair<local_id_t, expr_ref>>::const_iterator iter1, iter2;
    for (iter1 = m_local_sprel_expr_set.begin(), iter2 = other.m_local_sprel_expr_set.begin();
         iter1 != m_local_sprel_expr_set.end() && iter2 != other.m_local_sprel_expr_set.end();
         iter1++, iter2++) {
      if (iter1->first < iter2->first) {
        return true;
      }
      if (iter1->first > iter2->first) {
        return false;
      }
      if (iter1->second < iter2->second) {
        return true;
      }
      if (iter2->second < iter1->second) {
        return false;
      }
    }
    return false;
  }

  bool is_weaker_than(local_sprel_expr_guesses_t const &other) const
  {
    return ::set_contains(other.m_local_sprel_expr_set, m_local_sprel_expr_set);
  }

  bool lsprel_guesses_imply(local_sprel_expr_guesses_t const &other) const
  {
    return other.is_weaker_than(*this);
  }

  bool contains_conflicting_local_sprel_expr_guess(local_sprel_expr_guesses_t const &other, graph_locals_map_t const &locals_map) const
  {
    local_sprel_expr_guesses_t new_guesses = *this;
    new_guesses.set_union(other);
    return new_guesses.is_incompatible(locals_map);

    /*for (auto ge : m_local_sprel_expr_set) {
      for (auto oge : other.m_local_sprel_expr_set) {
        if (   ge.first == oge.first
            && ge.second != oge.second) {
          return true;
        }
      }
    }
    return false;*/
  }

  void minus(local_sprel_expr_guesses_t const &other)
  {
    ::set_difference(m_local_sprel_expr_set, other.m_local_sprel_expr_set);
  }

  string to_string() const
  {
    stringstream ss;
    for (auto p : m_local_sprel_expr_set) {
      ss << "EQA(local." << p.first << ", " << expr_string(p.second) << "),";
    }
    return ss.str();
  }

  void local_sprel_expr_guesses_to_stream(ostream& os) const;
  string local_sprel_expr_guesses_from_stream(istream& is, context* ctx);

  size_t size() const
  {
    return m_local_sprel_expr_set.size();
  }

  bool is_incompatible(graph_locals_map_t const &locals_map);
  void lsprels_clear();
  map<local_id_t, expr_ref> get_local_sprel_expr_map() const;
  map<expr_id_t, pair<expr_ref, expr_ref>> get_local_sprel_expr_submap() const;

  bool local_sprel_expr_guesses_eval_on_counter_example(bool &eval_result, counter_example_t &counter_example) const;

  static bool local_sprel_expr_guesses_compare_ids(local_sprel_expr_guesses_t const &a, local_sprel_expr_guesses_t const &b)
  {
    //compare_ids function needs to have a deterministic output, e.g., cannot depend on pointer values
    bool ret = a.to_string() < b.to_string();
    ASSERT(!ret);
    return ret;
  }
};

}
