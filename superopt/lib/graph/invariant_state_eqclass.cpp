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
#include "graph/invariant_state_eqclass_houdini.h"
#include "eq/corr_graph.h"

namespace eqspace {


template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>*
invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>::invariant_state_eqclass_from_stream(istream& is, string const& inprefix/*, state const& start_state*/, context* ctx, map<graph_loc_id_t, graph_cp_location> const& locs)
{
  string line;
  bool end;
  end = !getline(is, line);
  ASSERT(!end);
  string const prefix = inprefix + "type ";
  string const line_prefix = string("=") + prefix;
  if (!string_has_prefix(line, line_prefix)) {
    cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
    cout << __func__ << " " << __LINE__ << ": prefix = '" << prefix << "'\n";
  }
  ASSERT(string_has_prefix(line, line_prefix));
  string type = line.substr(line_prefix.length());
  if (type == "arr") {
    return new invariant_state_eqclass_arr_t<T_PC, T_N, T_E, T_PRED>(is, prefix + "arr "/*, start_state*/, ctx, locs);
  } else if (type == "bv") {
    return new invariant_state_eqclass_bv_t<T_PC, T_N, T_E, T_PRED>(is, prefix + "bv "/*, start_state*/, ctx);
  } else if (type == "ineq") {
    return new invariant_state_eqclass_ineq_t<T_PC, T_N, T_E, T_PRED>(is, prefix + "ineq "/*, start_state*/, ctx);
  } else if (type == "ineq_loose") {
    return new invariant_state_eqclass_ineq_loose_t<T_PC, T_N, T_E, T_PRED>(is, prefix + "ineq_loose "/*, start_state*/, ctx);
  } else if (type == "houdini") {
    return new invariant_state_eqclass_houdini_t<T_PC, T_N, T_E, T_PRED>(is, prefix + "houdini "/*, start_state*/, ctx);
  } else NOT_REACHED();
  NOT_REACHED();
}

//template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
//void
//invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>::replay_counter_examples(list<nodece_ref<T_PC, T_N, T_E>> const& counterexamples)
//{
//  NOT_IMPLEMENTED();
//}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
string
invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>::invariant_state_eqclass_to_string(string const &prefix, bool with_points) const
{
  stringstream ss;
  ss << prefix << eqclass_exprs_to_string() << endl;
  ss << prefix << m_point_set.size() << " points:\n";
  if (with_points) {
    ss << m_point_set.point_set_to_string(prefix);
    /*size_t n = 0;
      for (auto const& ps : m_point_set) {
      string new_prefix = prefix + "  ";
      ss << prefix << n << ".\n";
      n++;
      }*/
  }
  ss << this->eqclass_preds_to_string(prefix, m_preds);
  return ss.str();
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>::invariant_state_eqclass_get_asm_annotation(ostream& os) const
{
  for (auto const& pred : m_preds) {
    os << " ";
    pred->predicate_get_asm_annotation(os);
    os << ";";
  }
}

template class invariant_state_eqclass_t<pcpair, corr_graph_node, corr_graph_edge, predicate>;
template class invariant_state_eqclass_t<pc, tfg_node, tfg_edge, predicate>;

}
