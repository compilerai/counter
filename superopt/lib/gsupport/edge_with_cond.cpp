#include "expr/state.h"
#include "gsupport/edge_with_cond.h"
//#include "cg/corr_graph.h"
//#include "tfg/tfg.h"

namespace eqspace {

template<typename T_PC>
unordered_set<expr_ref>
edge_with_cond<T_PC>::read_assumes_from_stream(istream& in, string const& prefix, context* ctx)
{
  unordered_set<expr_ref> ret;
  string line;
  bool end = !getline(in, line);
  if (end)
    return {};

  while (!is_line(line, "Assumes.end")) {
    ASSERT(is_line(line, "Assume."));
    expr_ref e;
    line = read_expr(in, e, ctx);
    ret.insert(e);
  }
  return ret;
}

template <typename T_PC>
string
edge_with_cond<T_PC>::read_edge_with_cond_using_start_state(istream &in, string const& first_line, string const &prefix/*, state const &start_state*/, context* ctx, T_PC& p1, T_PC& p2, expr_ref& cond, state& state_to, unordered_set<expr_ref>& assumes)
{
  string line/*, comment*/;
  string comment;

  //cout << __func__ << " " << __LINE__ << ": first_line = " << first_line << ".\n";
  if (first_line.size() <= prefix.size()) {
    cout << __func__ << " " << __LINE__ << ": first_line = " << first_line << ", prefix = " << prefix << ".\n";
    NOT_REACHED();
  }
  edge<T_PC>::edge_read_pcs_and_comment(first_line.substr(prefix.size() + 1 /* for colon*/), p1, p2, comment);
  ASSERT(comment == "");

  getline(in, line);
  if (!is_line(line, prefix + ".EdgeCond:")) {
    cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
    cout << __func__ << " " << __LINE__ << ": prefix = " << prefix << ".\n";
  }
  assert(is_line(line, prefix + ".EdgeCond:"));
  //expr_ref cond;
  line = read_expr(in, cond, ctx);

  assert(is_line(line, prefix + ".StateTo:"));
  //state state_to;
  //cout << __func__ << " " << __LINE__ << ": reading state_to, line = " << line << endl;
  map<string_ref, expr_ref> state_to_value_expr_map = state_to.get_value_expr_map_ref();
  line = state::read_state(in, state_to_value_expr_map, ctx/*, &start_state*/);
  state_to.set_value_expr_map(state_to_value_expr_map);
  state_to.populate_reg_counts(/*&start_state*/);

  if (is_line(line, prefix + ".Assumes.begin:")) {
    ASSERTCHECK(is_line(line, prefix + ".Assumes.begin:"), cout << "line = " << line << ", prefix = " << prefix << endl);
    assumes = read_assumes_from_stream(in, prefix, ctx);
    bool end = !getline(in, line);
    ASSERT(!end);
  } else {
    assumes = {};
  }

  return line;
}

template<typename T_PC>
string
edge_with_cond<T_PC>::to_string(/*state const *start_state*/) const
{
  ostringstream ss;
  ss << this->edge<T_PC>::to_string();

  /*if (start_state) */{
    ss << "\n";
    context *ctx = this->get_cond()->get_context();
    ss << "Pred: " << expr_string(this->get_cond()) << "\n";
    ss << "State:\n" << this->get_to_state().to_string(/**start_state*/);
    ss << "Assumes:\n";
    int i = 0;
    for (auto const& assume_e : this->get_assumes()) {
      ss << "Assume." << i++ << ": " << expr_string(assume_e) << '\n';
    }
  }
  return ss.str();
}

template<typename T_PC>
string
edge_with_cond<T_PC>::to_string_for_eq(string const& prefix) const
{
  ostringstream ss;
  context *ctx = this->get_cond()->get_context();
  ss << this->edge<T_PC>::to_string_for_eq(prefix);
  ss << prefix << ".EdgeCond: \n" << ctx->expr_to_string_table(this->get_cond()) << "\n";
  ss << prefix << ".StateTo: \n" << this->get_to_state().to_string_for_eq();
  ss << prefix << ".Assumes.begin:\n";
  int i = 0;
  for (auto const& assume_e : this->get_assumes()) {
    ss << prefix << "Assume." << i++ << '\n';
    ss << ctx->expr_to_string_table(assume_e) << '\n';
  }
  ss << prefix << ".Assumes.end\n";

  return ss.str();
}

template class edge_with_cond<pc>;
template class edge_with_cond<pcpair>;

}
