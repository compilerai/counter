#pragma once

#include <memory>
#include <map>

#include "gsupport/pc.h"
#include "gsupport/graph_ec.h"
#include "gsupport/tfg_edge.h"

namespace eqspace {

class tfg_edge;

using namespace std;
using tfg_suffixpath_t = graph_edge_composition_ref<pc,tfg_edge>;

void print_suffixpaths(map<pc, tfg_suffixpath_t> const &suffixpaths);

class tfg_suffixpath_val_t {
private:
  tfg_suffixpath_t m_path;
  bool m_is_top;
public:
  tfg_suffixpath_val_t();
  tfg_suffixpath_val_t(tfg_suffixpath_t const &pth) : m_path(pth), m_is_top(false) {}

  static tfg_suffixpath_val_t top()
  {
    tfg_suffixpath_val_t ret;
    ret.m_is_top = true;
    return ret;
  }
  
  tfg_suffixpath_t const &get_path() const { ASSERT(!m_is_top); return m_path; }

  //returns if changed
  bool meet(tfg_suffixpath_val_t const &other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    assert(other.m_path != nullptr);
    assert(this->m_path != nullptr);
    //auto suffixpath_after_meet = a->get_graph().mk_parallel(a, b);
    //corr_graph const *cg = dynamic_cast<corr_graph const *>(m_graph);
    auto suffixpath_after_meet = mk_parallel(this->m_path, other.m_path/*suffixpath_edge*/);
    if (suffixpath_after_meet != this->m_path/*out*/) {
      this->m_path = suffixpath_after_meet;
      return true;
    } else {
      return false;
    }
  }
};


}
