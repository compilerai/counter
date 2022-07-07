#pragma once

#include "expr/fallocsite.h"
#include "expr/call_context.h"

namespace eqspace {

class allocstack_t
{
private:
  list<fallocsite_t> m_stack;
  bool m_has_any_prefix = false;
public:
  allocstack_t()
  { }
  allocstack_t(list<fallocsite_t> const& stck, bool has_any_prefix);

  list<fallocsite_t> const& allocstack_get_stack() const { return m_stack; }
  bool allocstack_has_any_prefix() const { return m_has_any_prefix; }

  bool operator<(allocstack_t const& other) const
  {
    if (m_stack.size() != other.m_stack.size()) {
      return m_stack.size() < other.m_stack.size();
    }
    for (auto iter1 = m_stack.begin(), iter2 = other.m_stack.begin();
         iter1 != m_stack.end() && iter2 != other.m_stack.end();
         iter1++, iter2++) {
      if (*iter1 != *iter2) {
        return *iter1 < *iter2;
      }
    }
    return false;
  }

  bool operator>(allocstack_t const& other) const
  {
    return other < *this;
  }

  bool operator==(allocstack_t const& other) const
  {
    if (m_stack.size() != other.m_stack.size()) {
      return false;
    }
    for (auto iter1 = m_stack.begin(), iter2 = other.m_stack.begin();
         iter1 != m_stack.end() && iter2 != other.m_stack.end();
         iter1++, iter2++) {
      if (*iter1 != *iter2) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(allocstack_t const& other) const
  {
    return !(*this == other);
  }
  string allocstack_to_string() const;
  bool allocstack_is_empty() const;
  bool allocstack_is_singleton() const;

  fallocsite_t const& allocstack_get_last_fallocsite() const;

  allocsite_t const& allocstack_get_last_allocsite() const;
  allocsite_t const& allocstack_get_only_allocsite() const;

  string_ref const& allocstack_get_last_fname() const;

  bool allocstack_contains_call_context_and_allocsite(call_context_ref const& cc, string_ref const& fname, allocsite_t const& asite) const;

  static allocstack_t allocstack_from_string(string const& s);
  static allocstack_t allocstack_singleton(string const& fname, allocsite_t const& asite);
  static bool allocstack_from_stream(istream& is, allocstack_t& allocstack);
  static bool string_represents_allocstack(string const& s);

  static allocstack_t allocstack_from_call_context(call_context_ref const& cc);
  static allocstack_t allocstack_from_longlong(long long l);
private:

};

}
