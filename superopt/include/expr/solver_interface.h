#pragma once

#include "support/query_comment.h"

#include "expr/expr.h"
#include "expr/relevant_memlabels.h"
#include "expr/proof_result.h"

namespace eqspace
{

class solver
{
  uint8_t m_solver_output_mask = 0;  // all enabled by default
  uint8_t m_solver_disable_mask = 0; // all enabled by default

public:
  virtual ~solver() {}

  virtual proof_result_t solver_satisfiable(expr_ref e, const query_comment& qc, relevant_memlabels_t const& relevant_memlabels/*, list<counter_example_t> &counter_example*/) = 0;
  virtual proof_result_t solver_provable(expr_ref e, const query_comment& qc, relevant_memlabels_t const& relevant_memlabels/*, list<counter_example_t> &counter_example*/) = 0;
  virtual expr_ref solver_simplify(expr_ref e, consts_struct_t const &cs) = 0;
  virtual string stats() const = 0;
  //virtual void print_stats(void) const = 0;
  virtual void clear_cache(void) = 0;
  static string solver_id_to_string(solver_id_t solver_id)
  {
    if (solver_id == SOLVER_ID_Z3) return "z3";
    else if (solver_id == SOLVER_ID_Z3v487) return "z3v487";
    else if (solver_id == SOLVER_ID_YICES) return "yices";
    else if (solver_id == SOLVER_ID_CVC4) return "cvc4";
    else if (solver_id == SOLVER_ID_BOOLECTOR) return "boolector";
    else NOT_REACHED();
  }
  static string solver_res_to_string(solver_res sr) {
    if (sr == solver_res_true) {
      return "SOLVER_RES_TRUE";
    } else if (sr == solver_res_false) {
      return "SOLVER_RES_FALSE";
    } else if (sr == solver_res_timeout) {
      return "SOLVER_RES_TIMEOUT";
    } else if (sr == solver_res_unknown) {
      return "SOLVER_RES_UNKNOWN";
    /*} else if (sr == solver_res_undef) {
      return "solver_res_undef";*/
    }
    assert(0);
  }
  //virtual void set_timeout(unsigned timeout_secs) = 0;
  //virtual unsigned get_timeout() const = 0;
  virtual bool timeout_occured_after_last_set_timeout() = 0;
  void unmask_yices_output() { m_solver_output_mask &= ~(uint8_t)(1 << SOLVER_ID_YICES); }
  void unmask_z3_output()    { m_solver_output_mask &= ~(uint8_t)(1 << SOLVER_ID_Z3); }
  void unmask_cvc4_output()  { m_solver_output_mask &= ~(uint8_t)(1 << SOLVER_ID_CVC4); }
  //void unmask_boolector_output() { m_solver_output_mask &= ~(uint8_t)(1 << SOLVER_ID_BOOLECTOR); }
  void mask_yices_output()   { m_solver_output_mask |= (1 << SOLVER_ID_YICES); }
  void mask_z3_output()      { m_solver_output_mask |= (1 << SOLVER_ID_Z3); }
  void mask_cvc4_output()    { m_solver_output_mask |= (1 << SOLVER_ID_CVC4); }
  //void mask_boolector_output() { m_solver_output_mask |= (1 << SOLVER_ID_BOOLECTOR); }

  void set_solver_output_mask(uint8_t mask) { m_solver_output_mask = mask; }
  uint8_t get_solver_output_mask() const    { return m_solver_output_mask; }

  void enable_yices() { m_solver_disable_mask &= ~(uint8_t)(1 << SOLVER_ID_YICES); }
  void enable_z3()    { m_solver_disable_mask &= ~(uint8_t)(1 << SOLVER_ID_Z3); }
  void enable_cvc4()  { m_solver_disable_mask &= ~(uint8_t)(1 << SOLVER_ID_CVC4); }
  void disable_yices()  { m_solver_disable_mask |= (1 << SOLVER_ID_YICES); }
  void disable_z3()     { m_solver_disable_mask |= (1 << SOLVER_ID_Z3); }
  void disable_cvc4()   { m_solver_disable_mask |= (1 << SOLVER_ID_CVC4); }

  void set_solver_disable_mask(uint8_t mask) { m_solver_disable_mask = mask; }
  uint8_t get_solver_disable_mask() const    { return m_solver_disable_mask; }

  proof_result_t expr_is_satisfiable(expr_ref const &e, query_comment const &comment, relevant_memlabels_t const& relevant_memlabels/*, list<counter_example_t> &counter_examples*/);
  proof_result_t expr_is_provable(expr_ref const &e, query_comment const &comment, relevant_memlabels_t const& relevant_memlabels/*, list<counter_example_t> &counter_examples*/);
};


}
