#ifndef STACK_OFFSET_H
#define STACK_OFFSET_H
#include <map>
#include <algorithm>
#include <set>
#include "support/debug.h"
#include "support/types.h"
#include "valtag/reg_identifier.h"

#define STACK_SLOT_SIZE 4
#define STACK_ARG_SIZE 4

class stack_offset_t
{
private:
  bool m_is_arg;
  dst_long m_offset;
public:
  stack_offset_t(bool is_arg, dst_long off) : m_is_arg(is_arg), m_offset(off) {}
  static stack_offset_t stack_offset_arg(int i)
  {
    stack_offset_t ret(true, i*STACK_ARG_SIZE);
    return ret;
  }
  static stack_offset_t stack_offset_nonarg(int i)
  {
    stack_offset_t ret(false, i*STACK_ARG_SIZE);
    return ret;
  }

  std::string stack_offset_to_string() const
  {
    std::stringstream ss;
    ss << "(" << (m_is_arg ? "arg, " : "") << m_offset << ")";
    return ss.str();
  }
  void set_offset(bool is_arg, dst_long off)
  {
    m_is_arg = is_arg;
    m_offset = off;
  }
  bool get_is_arg() const { return m_is_arg; }
  dst_long get_offset() const { return m_offset; }
  bool operator<(stack_offset_t const &other) const
  {
    if (other.m_is_arg && !this->m_is_arg) {
      return true;
    }
    if (!other.m_is_arg && this->m_is_arg) {
      return false;
    }
    ASSERT(other.m_is_arg == this->m_is_arg);
    return m_offset < other.m_offset;
  }
  bool operator==(stack_offset_t const &other) const
  {
    return other.m_is_arg == this->m_is_arg && other.m_offset == this->m_offset;
  }
  static stack_offset_t get_free_slot(std::set<stack_offset_t> const &used_slots)
  {
    dst_long cur_offset = 0;
    for (const auto &s : used_slots) {
      ASSERT(s.get_offset() >= cur_offset);
      if (!s.get_is_arg() && s.get_offset() > cur_offset) {
        ASSERT(s.get_offset() >= cur_offset + STACK_SLOT_SIZE);
        return stack_offset_t(false, cur_offset);
      }
      cur_offset += STACK_SLOT_SIZE;
    }
    return stack_offset_t(false, cur_offset);
  }
  /*string to_string() const
  {
    return this->stack_offset_to_string();
  }*/
  friend ostream &operator<<(ostream &out, stack_offset_t const &off)
  {
    out << off.stack_offset_to_string();
    return out;
  }
  static stack_offset_t get_default_object(size_t off)
  {
    return stack_offset_t(false, off * STACK_SLOT_SIZE);
  }
  static stack_offset_t get_invalid_object()
  {
    return stack_offset_t(false, 0);
  }
};

#endif
