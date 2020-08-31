#include "support/types.h"
#include "graph/graph_with_guessing.h"
#include "expr/counter_example.h"
#include "graph/graphce.h"
#include "graph/nodece.h"
#include "graph/point_set.h"
#include "graph/invariant_state_eqclass.h"
#include "graph/invariant_state_eqclass_arr.h"
#include "graph/invariant_state_eqclass_bv.h"
#include "graph/invariant_state_eqclass_ineq.h"
#include "graph/invariant_state_eqclass_ineq_loose.h"
#include "eq/corr_graph.h"

namespace eqspace {

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
string
invariant_state_t<T_PC, T_N, T_E, T_PRED>::invariant_state_to_string(/*T_PC const &p, */string prefix, bool full) const
{
  if (this->m_is_top) {
    return "TOP";
  }
  stringstream ss;
  ss << prefix << (this->m_is_stable ? "" : "(UNSTABLE) ")
               << "Invariant state" << /*" at " << p.to_string() << */" (" << m_eqclasses.size() << " classes):\n";
  size_t i = 0;
  for (auto const& eqclass : m_eqclasses) {
    string new_prefix = prefix + "  ";
    ss << new_prefix << "eqclass " << ++i << ":\n";
    ss << eqclass->invariant_state_eqclass_to_string(new_prefix, full);
  }
  return ss.str();
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
invariant_state_t<T_PC, T_N, T_E, T_PRED>::invariant_state_get_asm_annotation(ostream& os) const
{
  for (auto const& eqclass : m_eqclasses) {
    eqclass->invariant_state_eqclass_get_asm_annotation(os);
  }
}

template class invariant_state_t<pcpair, corr_graph_node, corr_graph_edge, predicate>;
template class invariant_state_t<pc, tfg_node, tfg_edge, predicate>;

}
