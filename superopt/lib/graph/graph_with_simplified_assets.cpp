#include "graph/graph_with_simplified_assets.h"
#include "gsupport/corr_graph_edge.h"
#include "gsupport/tfg_edge.h"
//#include "cg/corr_graph.h"
//#include "tfg/tfg.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
expr_ref
graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED>::graph_loc_get_value_simplified_using_mem_simplified(T_PC const &p, state const &to_state, graph_loc_id_t loc_id, map<string,expr_ref> const &mem_simplified) const
{
  //autostop_timer func_timer(__func__);
  //cout << __func__ << " " << __LINE__ << ": pc " << p.to_string() << ": to_state =\n" << to_state.to_string() << endl;
  map<string_ref, expr_ref> const &state_map = to_state.get_value_expr_map_ref();
  graph_cp_location const &l = this->get_loc(loc_id);
  expr_ref ex;
  if (l.m_type == GRAPH_CP_LOCATION_TYPE_REGMEM || l.m_type == GRAPH_CP_LOCATION_TYPE_LLVMVAR) {
    //autostop_timer func_timer(string(__func__) + ".var");
    if (state_map.count(l.m_varname) == 0) {
      return nullptr;
    }
    //cout << __func__ << " " << __LINE__ << ": pc " << p.to_string() << ": simplifying loc " << loc_id << endl;
    ex = this->graph_loc_get_value_simplified(p, to_state, loc_id);
    //cout << __func__ << " " << __LINE__ << ": pc " << p.to_string() << ": loc " << loc_id << ": ex = " << expr_string(ex) << endl;
  } else if (l.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
    //autostop_timer func_timer(string(__func__) + ".slot");
    if (!mem_simplified.count(l.m_memname->get_str())) {
      return nullptr;
    }
    ex = this->get_context()->mk_select(mem_simplified.at(l.m_memname->get_str()), l.m_memlabel->get_ml(), l.m_addr, l.m_nbytes, l.m_bigendian);
    ex = this->get_context()->expr_do_simplify(ex);
  } else if (l.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
    //autostop_timer func_timer(string(__func__) + ".memmask");
    if (!mem_simplified.count(l.m_memname->get_str())) {
      return nullptr;
    }
    ex = this->get_context()->mk_memmask(mem_simplified.at(l.m_memname->get_str()), l.m_memlabel->get_ml());
    ex = this->get_context()->expr_do_simplify(ex);
  } else NOT_REACHED();
  return ex;
}

template class graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate>;
template class graph_with_simplified_assets<pcpair, corr_graph_node, corr_graph_edge, predicate>;

}
