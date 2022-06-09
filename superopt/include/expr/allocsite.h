#pragma once

#include "expr/pc.h"

namespace eqspace {

class allocsite_t
{
private:
  pc m_pc;
public:
  allocsite_t()
  { }

  allocsite_t(pc const& p);

  bool allocsite_is_null() const
  {
    return m_pc == pc();
  }

  pc const& get_pc() const { return m_pc; }

  bool operator<(allocsite_t const& other) const
  {
    return m_pc < other.m_pc;
  }

  bool operator>(allocsite_t const& other) const
  {
    return other.m_pc < m_pc;
  }

  bool operator==(allocsite_t const& other) const
  {
    return m_pc == other.m_pc;
  }
  bool operator!=(allocsite_t const& other) const
  {
    return !(*this == other);
  }
  string allocsite_to_string() const;
  long long allocsite_to_longlong() const;

  allocsite_t increment_by_one() const;

  bool allocsite_represents_farg_local() const;
  graph_arg_id_t allocsite_arg_get_argnum() const;

  static bool string_represents_allocsite(string const& s);

  static allocsite_t allocsite_for_vararg_local();
  static allocsite_t allocsite_arg(argnum_t a);

  static allocsite_t allocsite_from_string(string const& s);
  static bool allocsite_from_stream(istream& is, allocsite_t& allocsite);

  static allocsite_t allocsite_from_longlong(long long l);
};

}
