#include "gsupport/suffixpath.h"
#include "gsupport/tfg_edge.h"

namespace eqspace {

using namespace std;

tfg_suffixpath_val_t::tfg_suffixpath_val_t()
  : m_path(mk_epsilon_ec<pc,tfg_edge>()), m_is_top(false)
{}

void 
print_suffixpaths(map<pc, tfg_suffixpath_t> const &suffixpaths)
{
  cout << __func__ << " " << __LINE__ << ": Printing suffix paths:\n";
  for (const auto &pc_ec : suffixpaths) {
    cout << pc_ec.first.to_string() << ": " << pc_ec.second->graph_edge_composition_to_string() << '\n';
  }
}

}
