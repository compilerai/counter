#pragma once

#include "graph/pc_list_val.h"
#include "graph/graph.h"
#include "graph/graph_bbl.h"
#include "graph/graph_loop.h"
#include "graph/graph_region.h"
#include "graph/dfa_without_regions.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E>
class find_reaching_pcs_dfa_t : public dfa_without_regions_t<T_PC, T_N, T_E, reaching_pcs_val_t<T_PC>, dfa_dir_t::forward>
{
public:
  find_reaching_pcs_dfa_t(dshared_ptr<graph<T_PC, T_N, T_E> const> g, map<T_PC, reaching_pcs_val_t<T_PC>> &init_vals) :
                   dfa_without_regions_t<T_PC, T_N, T_E, reaching_pcs_val_t<T_PC>, dfa_dir_t::forward>(g, init_vals)
  { }
  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, reaching_pcs_val_t<T_PC> const &in, reaching_pcs_val_t<T_PC> &out)
  {
    //cout << _FNLN_ << ": e = " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << endl;
    if (in.is_top()) {
      cout << _FNLN_ << ": found top in value at " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << endl;
    }
    ASSERT(!in.is_top());
    auto ret = out.meet(in.add(e->get_to_pc()));
    //cout << _FNLN_ << ": returning " << ret.to_string() << endl;
    //cout << _FNLN_ << ": all edges:";
    //for (auto const& e : this->m_graph->get_edges()) {
    //  cout << " " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string();
    //}
    //cout << endl;
    //cout << _FNLN_ << ": outgoing edges at " << e->get_to_pc().to_string() << ":";
    //for (auto const& o : this->m_graph->get_edges_outgoing(e->get_to_pc())) {
    //  cout << " " << o->get_from_pc().to_string() << "=>" << o->get_to_pc().to_string();
    //}
    //cout << endl;
    return ret;
  }
};

}
