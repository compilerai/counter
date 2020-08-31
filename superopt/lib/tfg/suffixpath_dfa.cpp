#include "support/debug.h"
#include "support/utils.h"
//#include "graph/tfg.h"
#include "tfg/suffixpath_dfa.h"

namespace eqspace {

bool
suffixpath_computation::xfer_and_meet(shared_ptr<tfg_edge const> const &e, tfg_suffixpath_val_t const& in, tfg_suffixpath_val_t &out)
{
  CPP_DBG_EXEC4(SUFFIXPATH_DFA, cout << __func__ << ':' << __LINE__ << ": xfer " << e->to_string() << " in: " << in.get_path()->graph_edge_composition_to_string() << endl);
  return out.meet(mk_suffixpath_series_with_edge(in.get_path(), e));
}

//tfg_suffixpath_t
//suffixpath_computation::meet(tfg_suffixpath_t const& a, tfg_suffixpath_t const& b)
//{
//  ASSERT(a != nullptr);
//  ASSERT(b != nullptr);
//  CPP_DBG_EXEC4(SUFFIXPATH_DFA, cout << __func__ << ':' << __LINE__ << ": meet: a: " << a->graph_edge_composition_to_string() << " b: " << b->graph_edge_composition_to_string() << endl);
//  auto suffixpath_after_meet = mk_parallel(a, b);
//  return suffixpath_after_meet;
//}

//map<pc, tfg_suffixpath_t> const&
//suffixpath_computation::get_suffixpaths_perpc()
//{
//  this->solve();
//  return this->get_values();
//}

}
