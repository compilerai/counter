#pragma once

#include "expr/pc.h"

#include "gsupport/corr_graph_edge.h"
#include "gsupport/suffixpath.h"

#include "graph/dfa.h"

namespace eqspace {

using namespace std;


class cg_suffixpath_computation : public data_flow_analysis<pcpair,corr_graph_node, corr_graph_edge, tfg_suffixpath_val_t, dfa_dir_t::forward>
{
  public:
  cg_suffixpath_computation(dshared_ptr<graph_with_regions_t<pcpair, corr_graph_node, corr_graph_edge> const> g, map<pcpair, tfg_suffixpath_val_t> & in_vals)
    : data_flow_analysis<pcpair, corr_graph_node, corr_graph_edge, tfg_suffixpath_val_t, dfa_dir_t::forward>(g/*,mk_epsilon_ec<pc,tfg_node,tfg_edge>()*/, in_vals)
  { }
  
  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<corr_graph_edge const> const &cge, tfg_suffixpath_val_t const& in, tfg_suffixpath_val_t &out) override;
};

}
