#include <sstream>
#include <iostream>

#include "graph/delta.h"

namespace eqspace {

delta_t::delta_t(int max_lookahead, unsigned int max_unroll)
: m_lookahead(0),
  m_max_lookahead(max_lookahead),
  m_max_unroll(max_unroll)
{
  ASSERT(max_lookahead >= 0); 
	ASSERT(max_unroll > 0);
}


delta_t::delta_t(int max_lookahead)
: delta_t(max_lookahead, /*max unroll factor*/1)
{ }

string 
delta_t::to_string() const
{
  stringstream ss;
  ss << "lookahead: " << m_lookahead << "  max_lookahead: " << m_max_lookahead << "max_unroll: " << m_max_unroll;
  return ss.str();
}

string 
delta_t::to_concise_string() const
{
  stringstream ss;
  ss << m_lookahead << '.' << m_max_unroll;
  return ss.str();
}

bool
delta_t::increment_delta()
{
  if (m_lookahead >= m_max_lookahead) {
    return false;
  }

  ++m_lookahead;
  return true;
}

}
