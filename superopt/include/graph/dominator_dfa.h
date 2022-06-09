#pragma once

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <list>
#include <stack>
#include <memory>

#include "graph/pc_list_val.h"
#include "graph/graph.h"
#include "graph/graph_bbl.h"
#include "graph/graph_loop.h"
#include "graph/graph_region.h"
#include "graph/dfa_without_regions.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, dfa_dir_t DIR>
class find_dominators_dfa_t : public dfa_without_regions_t<T_PC, T_N, T_E, dominator_val_t<T_PC>, DIR>
{
public:
  find_dominators_dfa_t(dshared_ptr<graph<T_PC, T_N, T_E> const> g, map<T_PC, dominator_val_t<T_PC>> &init_vals) :
                   dfa_without_regions_t<T_PC, T_N, T_E, dominator_val_t<T_PC>, DIR>(g, init_vals)
  { }
  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, dominator_val_t<T_PC> const &in, dominator_val_t<T_PC> &out)
  {
    if (in.is_top()) {
      cout << _FNLN_ << ": found top in value at " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << endl;
    }
    ASSERT(!in.is_top());
    if (DIR == dfa_dir_t::forward) {
      return out.meet(in.add(e->get_to_pc()));
    } else if (DIR == dfa_dir_t::backward) {
      return out.meet(in.add(e->get_from_pc()));
    } else NOT_REACHED();
  }
};

}
