#include <map>
#include "i386/regs.h"
#include "ppc/regs.h"
#include "eqcheck.h"
#include "correlate.h"
#include "support/sig_handling.h"
#include "config-host.h"
#include "support/log.h"
#include "tfg/predicate.h"
#include "tfg/avail_exprs.h"
#include "gsupport/suffixpath.h"

unsigned quota::get_unroll_factor() const
{
  return m_unroll_factor;
}

unsigned quota::get_smt_query_timeout() const
{
  return m_smt_query_timeout;
}

unsigned quota::get_sage_query_timeout() const
{
  return m_sage_query_timeout;
}

unsigned quota::get_total_timeout() const
{
  return m_total_timeout;
}

/*void quota::set_guess_level(unsigned v)
{
  m_guess_level = v;
}*/

void quota::set_unroll_factor(unsigned v)
{
  m_unroll_factor = v;
}

void quota::set_smt_query_timeout(unsigned v)
{
  m_smt_query_timeout = v;
}

void quota::set_sage_query_timeout(unsigned v)
{
  m_sage_query_timeout = v;
}

void quota::set_total_timeout(unsigned v)
{
  m_total_timeout = v;
}

string quota::to_string() const
{
  stringstream ss;
  ss << /*"{guess_level: " << m_guess_level << ", */"{unroll_factor: " << m_unroll_factor <<
        ", smt_query_timeout: " << m_smt_query_timeout <<
        ", sage_query_timeout: " << m_sage_query_timeout <<
        ", total_timeout: " << m_total_timeout <<
        ", memory_threshold_gb: " << m_memory_threshold_gb <<
        "}";
  return ss.str();
}
