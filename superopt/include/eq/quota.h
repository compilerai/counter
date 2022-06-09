#pragma once

#include <sys/time.h>
#include <sys/resource.h>
#include "tfg/tfg.h"
#include "expr/expr.h"
#include "expr/local_sprel_expr_guesses.h"
#include "gsupport/rodata_map.h"
#include "expr/consts_struct.h"

class quota
{
public:
  quota(unsigned unroll_factor = 1, unsigned smt_query_timeout = 600, unsigned sage_query_timeout = 600, unsigned total_timeout = 3600, unsigned memory_threshold_gb = 128, rlim_t max_memory_usage_in_bytes = -1) :
    m_unroll_factor(unroll_factor), m_smt_query_timeout(smt_query_timeout), m_sage_query_timeout(sage_query_timeout), m_total_timeout(total_timeout),
    m_memory_threshold_gb(memory_threshold_gb)
  {
    struct rlimit address_space_limit = { (rlim_t)max_memory_usage_in_bytes, (rlim_t)-1 };
    setrlimit(RLIMIT_AS, &address_space_limit);
  }

  unsigned get_unroll_factor() const;
  unsigned get_smt_query_timeout() const;
  unsigned get_sage_query_timeout() const;
  unsigned get_total_timeout() const;
  unsigned get_memory_threshold_gb() const { return m_memory_threshold_gb; }

  void set_unroll_factor(unsigned v);
  void set_smt_query_timeout(unsigned v);
  void set_sage_query_timeout(unsigned v);
  void set_total_timeout(unsigned v);
  void set_memory_threshold_gb(unsigned v) { m_memory_threshold_gb = v; }

  string to_string() const;

private:
  unsigned m_unroll_factor;
  unsigned m_smt_query_timeout;
  unsigned m_sage_query_timeout;
  unsigned m_total_timeout;
  unsigned m_memory_threshold_gb;
};
