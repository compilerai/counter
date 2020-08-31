#pragma once

#include "expr.h"
#include "support/query_comment.h"

namespace eqspace
{

class solver
{
public:
  enum solver_res
  {
    solver_res_true,
    solver_res_false,
    solver_res_timeout,
    //solver_res_undef,
  };

  uint8_t m_mask = 0;

  virtual ~solver() {}

  virtual solver_res solver_satisfiable(expr_ref e, const query_comment& qc, set<memlabel_ref> const& relevant_memlabels, list<counter_example_t> &counter_example, consts_struct_t const &cs) = 0;
  virtual solver_res solver_provable(expr_ref e, const query_comment& qc, set<memlabel_ref> const& relevant_memlabels, list<counter_example_t> &counter_example, consts_struct_t const &cs) = 0;
  virtual expr_ref solver_simplify(expr_ref e, consts_struct_t const &cs) = 0;
  virtual string stats() const = 0;
  //virtual void print_stats(void) const = 0;
  virtual void clear_cache(void) = 0;
  static string solver_res_to_string(solver_res sr) {
    if (sr == solver_res_true) {
      return "solver_res_true";
    } else if (sr == solver_res_false) {
      return "solver_res_false";
    } else if (sr == solver_res_timeout) {
      return "solver_res_timeout";
    /*} else if (sr == solver_res_undef) {
      return "solver_res_undef";*/
    }
    assert(0);
  }
  //virtual void set_timeout(unsigned timeout_secs) = 0;
  //virtual unsigned get_timeout() const = 0;
  virtual bool timeout_occured_after_last_set_timeout() = 0;
  void unmask_yices() {
    m_mask &= ~(uint8_t)(1 << SOLVER_ID_YICES);
  }
  void mask_yices() {
    m_mask |= (1 << SOLVER_ID_YICES);
  }
  void unmask_z3() {
    m_mask &= ~(uint8_t)(1 << SOLVER_ID_Z3);
  }
  void mask_z3() {
    m_mask |= (1 << SOLVER_ID_Z3);
  }
  void unmask_cvc4() {
    m_mask &= ~(uint8_t)(1 << SOLVER_ID_CVC4);
  }
  void mask_cvc4() {
    m_mask |= (1 << SOLVER_ID_CVC4);
  }
  void unmask_boolector() {
    m_mask &= ~(uint8_t)(1 << SOLVER_ID_BOOLECTOR);
  }
  void mask_boolector() {
    m_mask |= (1 << SOLVER_ID_BOOLECTOR);
  }
};

}
