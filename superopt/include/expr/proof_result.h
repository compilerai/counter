#pragma once

#include "expr/context.h"
#include "expr/counter_example.h"
#include "expr/proof_status.h"

namespace eqspace {

using namespace std;

class proof_result_t
{
private:
  //layout the fields in the order in which they are populated on the way back from the proof path
  proof_status_t m_proof_status = proof_status_t::proof_status_proven;
  list<counter_example_t> m_counterexamples;
  bool m_trivially_resolved = false;
  int m_num_quantifiers = 0;
  bool m_involves_fp_to_ieee_bv = false;
  bool m_stack_modeled_as_separate_mem = false;
  bool m_nonstack_modeled_as_common_mem = false;
public:
  static proof_result_t proof_result_dummy() { return proof_result_t(); }
  static proof_result_t proof_result_create(proof_status_t proof_status, list<counter_example_t> const& counterexamples, bool trivially_resolved = false, int num_quantifiers = 0, bool involves_fp_to_ieee_bv = false, bool stack_modeled_as_separate_mem = false, bool nonstack_modeled_as_common_mem = false)
  {
    return proof_result_t(proof_status, counterexamples, trivially_resolved, num_quantifiers, involves_fp_to_ieee_bv, stack_modeled_as_separate_mem, nonstack_modeled_as_common_mem);
  }

  proof_status_t get_proof_status() const { return m_proof_status; }
  list<counter_example_t> const& get_counterexamples() const { return m_counterexamples; }
  int get_num_quantifiers() const { return m_num_quantifiers; }
  bool involved_fp_to_ieee_bv() const { return m_involves_fp_to_ieee_bv; }
  bool was_trivially_resolved() const { return m_trivially_resolved; }
  bool stack_modeled_as_separate_mem() const { return m_stack_modeled_as_separate_mem; }
  bool nonstack_modeled_as_common_mem() const { return m_nonstack_modeled_as_common_mem; }

  string get_comment_str() const
  {
    if (m_trivially_resolved) {
      return "-trivial";
    }
    string ret;
    ret += (m_num_quantifiers > 0) ? "-used-quantifiers" : "";
    ret += m_involves_fp_to_ieee_bv ? "-fp-to-ieee-bv" : "";
    ret += m_stack_modeled_as_separate_mem ? "" : "-stack-not-modeled-as-separate-mem";
    ret += m_nonstack_modeled_as_common_mem ? "" : "-nonstack-not-modeled-as-common-mem";
    if (ret == "") {
      ret = "-all-proof-path-optimizations";
    }
    return ret;
  }
  static proof_status_t
  solver_res_to_proof_status(solver_res sr)
  {
    switch (sr) {
      case solver_res_true:
        return proof_status_proven;
      case solver_res_false:
      case solver_res_unknown:
        return proof_status_disproven;
      case solver_res_timeout:
        return proof_status_timeout;
      default:
        NOT_REACHED();
    }
  }
private:
  proof_result_t() {}
  proof_result_t(proof_status_t proof_status, list<counter_example_t> const& counterexamples, bool trivially_resolved, int num_quantifiers, bool involves_fp_to_ieee_bv, bool stack_modeled_as_separate_mem, bool nonstack_modeled_as_common_mem)
    : m_proof_status(proof_status),
      m_counterexamples(counterexamples),
      m_trivially_resolved(trivially_resolved),
      m_num_quantifiers(num_quantifiers),
      m_involves_fp_to_ieee_bv(involves_fp_to_ieee_bv),
      m_stack_modeled_as_separate_mem(stack_modeled_as_separate_mem),
      m_nonstack_modeled_as_common_mem(nonstack_modeled_as_common_mem)
  { }
};

}
