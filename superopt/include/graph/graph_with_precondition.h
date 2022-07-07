#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/sp_version.h"
#include "expr/relevant_memlabels.h"

#include "gsupport/sprel_map.h"

#include "graph/graph_loc_id.h"
#include "graph/graph_with_predicates.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_precondition : public graph_with_predicates<T_PC, T_N, T_E, T_PRED>
{
private:
  //for tfg: this may include constraints on arguments' locations (using mem.alloc)
  //for cg: this may include equality of the input arguments of the two tfgs
  unordered_set<predicate_ref> m_start_pc_preconditions;
public:
  graph_with_precondition(string const &name, string const& fname, context* ctx) : graph_with_predicates<T_PC,T_N,T_E,T_PRED>(name, fname, ctx)
  { }

  graph_with_precondition(graph_with_precondition const &other)
                                                : graph_with_predicates<T_PC,T_N,T_E,T_PRED>(other),
                                                  m_start_pc_preconditions(other.m_start_pc_preconditions)
  { }
  
  void graph_ssa_copy(graph_with_precondition const& other) 
  {
    graph_with_predicates<T_PC, T_N, T_E, T_PRED>::graph_ssa_copy(other);
    m_start_pc_preconditions = other.m_start_pc_preconditions;
  }

  void graph_to_stream(ostream& ss, string const& prefix) const override;

  graph_with_precondition(istream& in, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx);

  void set_start_pc_preconditions(unordered_set<predicate_ref> const& preconditions);
  unordered_set<predicate_ref> const& get_start_pc_preconditions() const;

};

}
