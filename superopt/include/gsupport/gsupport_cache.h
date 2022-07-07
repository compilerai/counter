#pragma once
#include "support/debug.h"
#include "support/lru.h"
#include "expr/expr.h"
#include "expr/context_cache.h"
#include "gsupport/graph_ec.h"
#include "gsupport/tfg_edge.h"

namespace eqspace {

class gsupport_cache_t
{
public:
  lookup_cache_t<pair<shared_ptr<tfg_edge_composition_t>,shared_ptr<tfg_edge_composition_t>>,bool> m_tfg_edge_composition_implies;
  void clear() {
    m_tfg_edge_composition_implies.clear();
  }
};

extern gsupport_cache_t g_gsupport_cache;

}
