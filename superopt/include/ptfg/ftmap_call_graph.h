#pragma once

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>

#include "gsupport/node.h"
#include "gsupport/ftmap_call_graph_pc.h"
#include "gsupport/ftmap_call_graph_edge.h"

#include "graph/graph.h"

namespace eqspace {

using namespace std;

class ftmap_call_graph_t : public graph<ftmap_call_graph_pc_t, node<ftmap_call_graph_pc_t>, ftmap_call_graph_edge_t> {
private:
  list<dshared_ptr<ftmap_call_graph_edge_t const>> m_retreating_edges;
public:
  void populate_retreating_edges(void);
  list<dshared_ptr<ftmap_call_graph_edge_t const>> const& ftmap_call_graph_get_retreating_edges(void) const { return m_retreating_edges; }
};

}
