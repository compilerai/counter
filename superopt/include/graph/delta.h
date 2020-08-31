#pragma once

#include <string>
#include "support/consts.h"
#include "support/debug.h"

using namespace std;

namespace eqspace {

class delta_t
{
private:
  int m_lookahead;
  int m_max_lookahead;
  unsigned int m_max_unroll;

public:
  delta_t(int max_lookahead);
  delta_t(int max_lookahead, unsigned int max_unroll);
  bool operator==(delta_t const& other) const
  {
    return (m_lookahead == other.m_lookahead && m_max_unroll == other.m_max_unroll);
  }
  bool operator<(delta_t const& other) const
  {
    if (m_lookahead < other.m_lookahead)
     return true;
    else if (m_lookahead > other.m_lookahead)
      return false;
    if (m_max_unroll < other.m_max_unroll)
     return true;
    else if (m_max_unroll > other.m_max_unroll)
      return false;
    return false;
  }
  int get_lookahead() const { return m_lookahead; }
  unsigned int get_max_unroll() const { return m_max_unroll; }

  bool increment_delta();

  string to_string() const;
  string to_concise_string() const;
};

}
