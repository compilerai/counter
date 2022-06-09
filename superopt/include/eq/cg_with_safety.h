#pragma once

#include "support/log.h"
#include "support/mymemory.h"

#include <list>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include "tfg/tfg.h"
//#include "gsupport/edge_guard.h"
#include "expr/counter_example.h"
#include "graph/eqclasses.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_relocatable_memlabels.h"
#include "eq/unsafe_cond.h"

#include "graph/graph_with_guessing.h"
//#include "graph/predicate_acceptance.h"
#include "gsupport/corr_graph_edge.h"

namespace eqspace {

using namespace std;

class unsafe_vals_intersect_t;
class unsafe_vals_union_t;

class cg_with_safety : public cg_with_relocatable_memlabels
{
public:
  cg_with_safety(cg_with_relocatable_memlabels const &cgr) : cg_with_relocatable_memlabels(cgr)
  { }

  bool check_safety() const;
protected:
  bool check_safety_for(unsafe_condition_t const& unsafe_cond) const;
private:
  void print_unsafe_dfa_vals(map<pcpair, unsafe_vals_intersect_t> const& src_unsafe_vals, map<pcpair, unsafe_vals_union_t> const& dst_unsafe_vals) const;
};

}
