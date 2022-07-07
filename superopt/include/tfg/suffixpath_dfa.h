#pragma once
#include "expr/pc.h"

#include "gsupport/tfg_edge.h"
#include "gsupport/itfg_edge.h"
#include "gsupport/suffixpath.h"

#include "graph/dfa.h"

namespace eqspace {

using namespace std;

class suffixpath_computation : public data_flow_analysis<pc,tfg_node, tfg_edge, tfg_suffixpath_val_t, dfa_dir_t::forward>
{
private:
  map<pc, set<tfg_edge_ref>> m_incoming_non_back_edges;
private:
  void populate_incoming_non_back_edges_at_pc(pc const& p, graph<pc, tfg_node, tfg_edge> const &g);
public:
  suffixpath_computation(dshared_ptr<graph_with_regions_t<pc, tfg_node, tfg_edge> const> g, map<pc, tfg_suffixpath_val_t> &init_vals)
    : data_flow_analysis<pc, tfg_node, tfg_edge, tfg_suffixpath_val_t, dfa_dir_t::forward>(g, init_vals)
  {
    for (auto const& p : g->get_all_pcs()) {
      this->populate_incoming_non_back_edges_at_pc(p, *g);
    }
    //ASSERT(dynamic_cast<tfg const*>(g) != nullptr);
  }
  
  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<tfg_edge const> const &e, tfg_suffixpath_val_t const& in, tfg_suffixpath_val_t &out) override;
};

}
