#ifndef EQCHECKSTATE_LLVM_H
#define EQCHECKSTATE_LLVM_H

#include "gsupport/pc.h"
#include "expr/expr.h"

#include <unordered_set>

using namespace eqspace;

class control_flow_transfer
{
public:
  control_flow_transfer(pc const &p_from, pc const &p_to, expr_ref const& cond, expr_ref const& tgt = nullptr, std::unordered_set<expr_ref> const& assumes = {})
    : m_pc_from(p_from), m_pc_to(p_to),
      m_condition(cond),
      m_target(tgt),
      m_assumes(assumes)
  { }

  control_flow_transfer(pc const &p_from, pc const &p_to, expr_ref const& cond, std::unordered_set<expr_ref> const& assumes)
    : control_flow_transfer(p_from, p_to, cond, nullptr, assumes)
  { }

  pc const &get_from_pc() const { return m_pc_from; }
  pc const &get_to_pc() const { return m_pc_to; }
  expr_ref const &get_target() const { return m_target; }
  expr_ref const &get_condition() const { ASSERT(m_condition); return m_condition; }
  bool is_indir_type() const { return m_target != nullptr; }
  std::unordered_set<expr_ref> const& get_assumes() const { return m_assumes; }

private:
  pc m_pc_from;
  pc m_pc_to;
  expr_ref m_condition;
  expr_ref m_target;
  std::unordered_set<expr_ref> m_assumes;
};

#endif
