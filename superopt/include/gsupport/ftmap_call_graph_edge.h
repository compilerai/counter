#pragma once

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>

#include "expr/context.h"

#include "gsupport/edge_id.h"
#include "gsupport/edge.h"
#include "gsupport/te_comment.h"
#include "gsupport/ftmap_call_graph_pc.h"

namespace eqspace {

using namespace std;

class ftmap_call_graph_edge_t : public edge<ftmap_call_graph_pc_t> {
public:
  ftmap_call_graph_edge_t(ftmap_call_graph_pc_t const& from_pc, ftmap_call_graph_pc_t const& to_pc) : edge<ftmap_call_graph_pc_t>(from_pc, to_pc)
  { }
  static dshared_ptr<ftmap_call_graph_edge_t const> empty() { NOT_IMPLEMENTED(); }
  static shared_ptr<ftmap_call_graph_edge_t const> empty(ftmap_call_graph_pc_t const&) { NOT_REACHED(); }
  bool is_empty() const { NOT_IMPLEMENTED(); }
  expr_ref get_cond() const { NOT_REACHED(); }
  state const& get_to_state() const { NOT_REACHED(); }
  unordered_set<expr_ref> const& get_assumes() const { NOT_REACHED(); }
  te_comment_t const& get_te_comment() const { NOT_REACHED(); }
  void visit_exprs_const(function<void (ftmap_call_graph_pc_t const&, string const&, expr_ref)> f) const { NOT_REACHED(); }
  string to_string_concise() const { return this->to_string(); }
};

}
