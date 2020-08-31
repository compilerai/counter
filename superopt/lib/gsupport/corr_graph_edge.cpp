#include <string>
#include <sstream>

#include "support/debug.h"
#include "support/consts.h"
#include "gsupport/corr_graph_edge.h"
//#include "cg/corr_graph.h"
#include "support/utils.h"
#include "expr/expr.h"
#include "support/log.h"
#include "support/globals.h"
//#include "parser/parse_edge_composition.h"

namespace eqspace {

corr_graph_edge::~corr_graph_edge()
{
  if (m_is_managed) {
    corr_graph_edge::get_corr_graph_edge_manager()->deregister_object(*this);
  }
}

manager<corr_graph_edge> *
corr_graph_edge::get_corr_graph_edge_manager()
{
  static manager<corr_graph_edge> *ret = nullptr;
  if (!ret) {
    ret = new manager<corr_graph_edge>;
  }
  return ret;
}

corr_graph_edge_ref
mk_corr_graph_edge(shared_ptr<corr_graph_node const> from, shared_ptr<corr_graph_node const> to, shared_ptr<tfg_edge_composition_t> const& src_edge, shared_ptr<tfg_edge_composition_t> const& dst_edge)
{
  corr_graph_edge cge(from, to, src_edge, dst_edge);
  return corr_graph_edge::get_corr_graph_edge_manager()->register_object(cge);
}

corr_graph_edge_ref
mk_corr_graph_edge(pcpair const& from, pcpair const& to, shared_ptr<tfg_edge_composition_t> const& src_edge, shared_ptr<tfg_edge_composition_t> const& dst_edge)
{
  corr_graph_edge cge(from, to, src_edge, dst_edge);
  return corr_graph_edge::get_corr_graph_edge_manager()->register_object(cge);
}

/*bool
corr_graph_edge::contains_function_call(void) const
{
  return m_src_edge->contains_function_call() || m_dst_edge->contains_function_call();
}*/

string
corr_graph_edge::to_string(/*state const* start_state*/) const
{
  stringstream ss;
  ss << edge::to_string();
  ss << "[src: " << this->get_src_edge()->to_string_concise() << "], ";
  ss << "[dst: " << this->get_dst_edge()->to_string_concise() << "]";
  return ss.str();
}

string
corr_graph_edge::to_string_concise() const
{
  //return m_from->get_src_pc().to_string() + "_" + m_from->get_dst_pc().to_string() + "=>" + m_to->get_src_pc().to_string() + m_to->get_dst_pc().to_string();
  return this->get_from_pc().to_string() + "=>" + this->get_to_pc().to_string();
}

shared_ptr<corr_graph_edge const>
corr_graph_edge::visit_exprs(std::function<expr_ref (string const&, expr_ref const&)> update_expr_fn, bool opt/*, state const& start_state*/) const
{
  NOT_IMPLEMENTED();
}

shared_ptr<corr_graph_edge const>
corr_graph_edge::visit_exprs(std::function<expr_ref (expr_ref const&)> update_expr_fn) const
{
  std::function<expr_ref (string const&, expr_ref)> f = [&update_expr_fn](string const& k, expr_ref e)
  {
    return update_expr_fn(e);
  };
  return this->visit_exprs(f);
}

void
corr_graph_edge::visit_exprs_const(function<void (string const&, expr_ref)> f) const
{
  typedef bool dont_care_t;
  function<dont_care_t (dont_care_t const&, expr_ref const&, state const&, unordered_set<expr_ref> const&)> atom_f =
    [&f](dont_care_t const& postorder_val, expr_ref const& econd, state const& to_state, unordered_set<expr_ref> const& assumes)
    {
      f(G_EDGE_COND_IDENTIFIER, econd);
      to_state.state_visit_exprs(f);
      unordered_set_visit_exprs_const(assumes, f);
      return postorder_val;
    };
  function<dont_care_t (dont_care_t const&, dont_care_t const&)> meet_f =
    [](dont_care_t const& a, dont_care_t const& __unused)
    {
      return a;
    };
  this->template xfer<dont_care_t,xfer_dir_t::forward>(dont_care_t(), atom_f, meet_f, /*simplified*/false);
}

void
corr_graph_edge::visit_exprs_const(function<void (pcpair const&, string const&, expr_ref)> f) const
{
  NOT_REACHED();
}

bool
corr_graph_edge::cg_is_fcall_edge(corr_graph const &cg) const
{
//  tfg const &dst_tfg = cg.get_dst_tfg();
//  shared_ptr<tfg_edge_composition_t> ec = this->get_dst_edge();
//  state const &to_state = dst_tfg.tfg_edge_composition_get_to_state(ec, dst_tfg.get_start_state());
//  return to_state.contains_function_call();
  return this->get_to_pc().is_fcall_pc();
}

string
corr_graph_edge::graph_edge_from_stream(istream &in, string const& first_line, string const &prefix/*, state const &start_state*/, context* ctx, shared_ptr<corr_graph_edge const>& te)
{
  pcpair from_pc, to_pc;
  string comment;
  string line;
  if (first_line.size() <= prefix.size()) {
    cout << __func__ << " " << __LINE__ << ": first_line = " << first_line << ", prefix = " << prefix << ".\n";
    NOT_REACHED();
  }
  edge<pcpair>::edge_read_pcs_and_comment(first_line.substr(prefix.size() + 1 /* for colon*/), from_pc, to_pc, comment);
  ASSERT(comment == "");
  bool end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line  == prefix + ".src_edge_composition");
  end = !getline(in, line);
  ASSERT(!end);
  shared_ptr<tfg_edge_composition_t> src_edge;
  line = graph_edge_composition_t<pc,tfg_edge>::graph_edge_composition_from_stream(in, line, prefix/*, start_state*/, ctx, src_edge);
  ASSERT(line  == prefix + ".dst_edge_composition");
  end = !getline(in, line);
  ASSERT(!end);
  shared_ptr<tfg_edge_composition_t> dst_edge;
  line = graph_edge_composition_t<pc,tfg_edge>::graph_edge_composition_from_stream(in, line, prefix/*, start_state*/, ctx, dst_edge);
  te = mk_corr_graph_edge(from_pc, to_pc, src_edge, dst_edge);
  return line;
}

string
corr_graph_edge::to_string_for_eq(string const& prefix) const
{
  string ret;
  ret += this->edge<pcpair>::to_string_for_eq(prefix);
  ret += prefix + ".src_edge_composition\n";
  ret += m_src_edge->graph_edge_composition_to_string_for_eq(prefix);
  ret += prefix + ".dst_edge_composition\n";
  ret += m_dst_edge->graph_edge_composition_to_string_for_eq(prefix);
  return ret;
}


}
