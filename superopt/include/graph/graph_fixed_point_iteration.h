#pragma once

#include "graph/graph.h"

using namespace std;

namespace eqspace {

template<typename T_PC, typename T_N, typename T_E>
void graph_forward_fixed_point_iteration(graph<T_PC, T_N, T_E> const& graph, function<bool (T_PC const &)> fn_per_pc)
{
  // we'll use a worklist for keeping track of PC
  // where values needs to be recalculated
  list<T_PC> worklist = graph.topological_sorted_labels();
  worklist.pop_front(); // remove entry
  set<T_PC> workset(worklist.begin(), worklist.end());

  while (!worklist.empty()) {
    // pop a PC to work on
    auto itr = worklist.begin();
    T_PC p = *itr;
    worklist.erase(itr);
    workset.erase(p);

    if (fn_per_pc(p)) { // true if value changed
      list<shared_ptr<T_E const>> outgoing;
      graph.get_edges_outgoing(p, outgoing);

      // add outgoing edges' to-PCs to worklist
      for (auto e : outgoing) {
        auto to_pc = e->get_to_pc();
        if (!workset.count(to_pc)) {// if not already present
          worklist.push_back(to_pc);
          workset.insert(to_pc);
        }
      }
    }
  }
}

}
