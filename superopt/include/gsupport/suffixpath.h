#pragma once

#include <memory>
#include <map>

#include "expr/pc.h"

#include "gsupport/graph_ec.h"
#include "gsupport/tfg_edge.h"

namespace eqspace {

class tfg_edge;

using namespace std;
using tfg_suffixpath_t = graph_edge_composition_ref<pc,tfg_edge>;

void print_suffixpaths(map<pc, tfg_suffixpath_t> const &suffixpaths);

class tfg_suffixpath_val_t {
private:
  tfg_suffixpath_t m_path = nullptr;
  //bool m_is_top;
  /* m_incoming_edges_seen: is used to ensure that we propogate
     an out value only after all incoming values have been seen. 
     This helps in keeping the tfg_edge_composition small, because
     we no longer need to call mk_parallel() repeatedly on downstream
     nodes now due to updates to the current node).
     Consider the example where a node N has two incoming edges, E1
     and E2; and one downstream node M. Now, if we keep propagate
     the suffixpath the first time E1 fires, then M may have a
     non-top value even before E2 fires.  Thus, we will have to
     later do mk_parallel on E1.(N->M) and E2.(N->M). To do this
     correctly, we will need to factor out (N->M) to get (E1+E2).(N->M).
     To avoid such computation, we update M only after both E1
     and E2 have fired.
  */
  map<tfg_edge_ref, tfg_suffixpath_t> m_incoming_edges_seen;
public:
  tfg_suffixpath_val_t();
  //tfg_suffixpath_val_t(tfg_suffixpath_t const &pth) : m_path(pth)/*, m_is_top(false)*/ {}

  static tfg_suffixpath_val_t top()
  {
    tfg_suffixpath_val_t ret;
    //ret.m_is_top = true;
    return ret;
  }
  static tfg_suffixpath_val_t tfg_suffixpath_start_pc_value()
  {
    tfg_suffixpath_val_t ret;
    ret.m_path = graph_edge_composition_t<pc,tfg_edge>::mk_epsilon_ec();
    return ret;
  }
  
  tfg_suffixpath_t const &get_path() const { /*ASSERT(!m_is_top); */return m_path; }

  //returns if changed
  bool meet(tfg_suffixpath_t const& other, dshared_ptr<tfg_edge const> const& e, set<tfg_edge_ref> const& incoming_non_back_edges_at_pc);
};


}
