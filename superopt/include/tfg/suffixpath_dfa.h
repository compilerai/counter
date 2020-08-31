#pragma once
#include <memory>
#include <map>

#include "graph/dfa.h"
#include "gsupport/pc.h"
#include "gsupport/tfg_edge.h"
#include "gsupport/itfg_edge.h"
#include "gsupport/suffixpath.h"

namespace eqspace {

using namespace std;

class suffixpath_computation : public data_flow_analysis<pc,tfg_node, tfg_edge, tfg_suffixpath_val_t, dfa_dir_t::forward>
{
  public:
  suffixpath_computation(graph<pc, tfg_node, tfg_edge> const *g, map<pc, tfg_suffixpath_val_t> &init_vals)
    : data_flow_analysis<pc, tfg_node, tfg_edge, tfg_suffixpath_val_t, dfa_dir_t::forward>(g, init_vals)
  {
    //ASSERT(dynamic_cast<tfg const*>(g) != nullptr);
  }
  
  virtual bool xfer_and_meet(shared_ptr<tfg_edge const> const &e, tfg_suffixpath_val_t const& in, tfg_suffixpath_val_t &out) override;
};

}
