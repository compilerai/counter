#include "graph/graph_with_predicates.h"
#include "gsupport/corr_graph_edge.h"
#include "gsupport/tfg_edge.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_predicates<T_PC, T_N, T_E, T_PRED>::extract_align_assumes(unordered_set<shared_ptr<T_PRED const>> const &theos, unordered_set<shared_ptr<T_PRED const>> &alignment_assumes)
{
  for(auto it = theos.begin(); it != theos.end(); ++it) {
    if((*it)->predicate_is_ub_langaligned_assume())
      alignment_assumes.insert(*it);
  }
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_predicates<T_PC, T_N, T_E, T_PRED>::graph_to_stream(ostream& ss) const
{
  this->graph<T_PC, T_N, T_E>::graph_to_stream(ss);
  context *ctx = this->get_context();
  for (auto arg : get_argument_regs().get_map()) {
    ss << "=Input: " << arg.first->get_str() << "\n" << ctx->expr_to_string_table(arg.second) << "\n";
  }
  list<shared_ptr<T_N>> ns;
  this->get_nodes(ns);

  for (auto n : ns) {
    T_PC const &p = n->get_pc();
    if (!p.is_exit()) {
      continue;
    }
    ss << "=Node outputs: " << p.to_string() << endl;
    if (pc_has_return_regs(p)) {
      for (auto ret : get_return_regs_at_pc(p)) {
        ss << "=Output: " << ret.first->get_str() << "\n" << ctx->expr_to_string_table(ret.second) << "\n";
      }
    }
    ss << "=Node outputs done for " << p.to_string() << "\n";
  }

  ss << "=input_outputs done\n";

  m_symbol_map.graph_symbol_map_to_stream(ss);
  m_locals_map.graph_locals_map_to_stream(ss);

  ss << "=StartState:\n";
  ss << this->get_start_state().to_string_for_eq() << "\n";

  for(shared_ptr<T_N> n : ns) {
    T_PC const &p = n->get_pc();
    ss << "=Node assumes and asserts: " << p.to_string() << "\n";
    string prefix = p.to_string() + " assume ";
    predicate::predicate_set_to_stream(ss, prefix, this->get_assume_preds(p));
    //ss << this->preds_to_string_for_eq(p);
    prefix = p.to_string() + " assert ";
    predicate::predicate_set_to_stream(ss, prefix, this->get_assert_preds(p));
  }

  ss << "=Global assumes\n";
  T_PRED::predicate_set_to_stream(ss, "global assume ", m_global_assumes);

  ss << this->memlabel_map_to_string_for_eq();

  list<shared_ptr<T_E const>> es;
  this->get_edges(es);
  for (shared_ptr<T_E const> e : es) {
    ss << e->to_string_for_eq("=Edge") << "\n";
  }
  ss << "=graph_with_predicates_done\n";
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
graph_with_predicates<T_PC, T_N, T_E, T_PRED>::graph_with_predicates(istream& in, string const& name, context* ctx) : graph<T_PC, T_N, T_E>(in), m_name(mk_string_ref(name)), m_ctx(ctx), m_cs(ctx->get_consts_struct())
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  map<string_ref, expr_ref> args;
  map<T_PC, map<string_ref, expr_ref>> rets;
  string line;
  read_in_out(in, ctx, args, rets);
  this->set_argument_regs(args);
  this->set_return_regs(rets);

  line = this->read_symbol_map_nodes_and_edges(in/*, line*/);
  while (line == "") {
    getline(in, line);
  }
  ASSERTCHECK((line == "=graph_with_predicates_done"), cout << "line = " << line << endl);
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_predicates<T_PC, T_N, T_E, T_PRED>::read_in_out(istream& in, context* ctx, map<string_ref, expr_ref> &args, map<T_PC, map<string_ref, expr_ref>> &rets)
{
  //vector<expr_ref> args;
  //map<pc, vector<expr_ref>> rets;
  string line;
  bool end;
  end = !getline(in, line);
  ASSERT(!end);
  while (is_line(line, "=Input:")) {
    expr_ref e;
    string_ref name = mk_string_ref(line.substr(strlen("=Input: ")));
    line = read_expr(in, e, ctx);
    ASSERT(args.count(name) == 0);
    args[name] = e;
  }
  static string const prefix = "=Node outputs: ";
  while (string_has_prefix(line, prefix)) {
    //DEBUG("reading node for input-ouput: " <<  line << endl);
    T_PC p = T_PC::create_from_string(line.substr(prefix.length()));
    //ASSERT(p.is_start() || p.is_exit());
    ASSERT(p.is_exit());
    map<string_ref, expr_ref> retmap;

    read_out_values(in, ctx, retmap);
    rets[p] = retmap;
    end = !getline(in, line);
    ASSERT(!end);
  }
  if (line != "=input_outputs done") {
    cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
  }
  ASSERT(line == "=input_outputs done");
  //t.set_argument_regs(args);
  //t.set_return_regs(rets);
  //return line;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_predicates<T_PC, T_N, T_E, T_PRED>::read_out_values(istream& in, context* ctx, map<string_ref, expr_ref> &rets)
{
  string line;
  getline(in, line);
  while(true) {
    //bool input = is_line(line, "=Input:");
    bool out = is_line(line, "=Output:");
    //cout << __func__ << " " << __LINE__ << ": line = " << line << ", rets.size() = " << rets.size() << endl;
    if (!(/*input || */out)) {
      //t.set_argument_regs(args);
      //t.set_return_regs(rets);
      break;
      //return line;
    }
    string_ref s = mk_string_ref(line.substr(strlen("=Output: ")));

    expr_ref e;
    line = read_expr(in, e, ctx);
    /*if (input)
      args.push_back(e);
    else*/
    ASSERT(!rets.count(s));
      rets[s] = e;
  }
  //cout << __func__ << " " << __LINE__ << ": rets.size() = " << rets.size() << endl;
  if (!string_has_prefix(line, "=Node outputs done for ")) {
    cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
  }
  ASSERT(string_has_prefix(line, "=Node outputs done for "));
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
string
graph_with_predicates<T_PC, T_N, T_E, T_PRED>::read_symbol_map_nodes_and_edges(istream& in/*, string line*/)
{
  context *ctx = this->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  
  graph_symbol_map_t symbol_map(in);

  graph_locals_map_t local_map(in);

  string line;
  bool end;

  end = !getline(in, line);
  ASSERT(!end);
  //cout << "symbol_map = " << symbol_map << endl;
  assert(is_line(line, "=StartState:"));
  //cout << __func__ << " " << __LINE__ << ": reading start_state, line = " << line << endl;
  map<string_ref, expr_ref> start_state_value_expr_map;
  line = state::read_state(in, start_state_value_expr_map, ctx/*, NULL*/);
  //cout << __func__ << " " << __LINE__ << ": st =\n" << st.to_string() << endl;
  state st;
  st.set_value_expr_map(start_state_value_expr_map);
  st.populate_reg_counts(/*NULL*/);

  this->set_start_state(st);

  this->set_symbol_map(symbol_map);
  this->set_locals_map(local_map);

  unordered_set<shared_ptr<T_PRED const>> alignment_assumes;

  string const prefix = "=Node assumes and asserts: ";
  while(is_line(line, prefix)) {
    //cout << __func__ << " " << __LINE__ << endl;
    //DEBUG("reading node: " <<  line << endl);
    T_PC p = T_PC::create_from_string(line.substr(prefix.length()));
    //assert(is_next_line(in, "=State:"));
    //state st;
    //line = read_state(in, st, ctx);
    //getline(in, line);
    unordered_set<shared_ptr<T_PRED const>> theos, asserts;
    map<expr_ref, memlabel_t> alias_map;
    //cout << __func__ << " " << __LINE__ << endl;
    //line = read_alias_map(in, ctx, line, alias_map);
    //cout << __func__ << " " << __LINE__ << endl;
    //line = this->read_assumes(in, line, theos);
    string prefix = p.to_string() + " assume ";
    theos = T_PRED::predicate_set_from_stream(in/*, this->get_start_state()*/, ctx, prefix);

    prefix = p.to_string() + " assert ";
    asserts = T_PRED::predicate_set_from_stream(in/*, this->get_start_state()*/, ctx, prefix);
    bool end = !getline(in, line);
    ASSERT(!end);
    //cout << __func__ << " " << __LINE__ << endl;
    //list<pair<string, expr_ref>> outgoing_values;
    //cout << __func__ << " " << __LINE__ << endl;
    //line = this->read_outgoing_values(in, line, outgoing_values);
    //cout << __func__ << " " << __LINE__ << endl;
    //cout << __func__ << " " << __LINE__ << ": adding node with pc " << p.to_string() << endl;
    shared_ptr<T_N> new_node = make_shared<T_N>(p/*, st*/);
    //cout << __func__ << " " << __LINE__ << ": new_node = " << new_node << endl;
    this->add_node(new_node);
    //cout << __func__ << " " << __LINE__ << endl;
    //(*t)->add_alias_map(p, alias_map);
    //cout << __func__ << " " << __LINE__ << endl;
    if(alignment_assumes.size() > 0)
      this->add_assume_preds_at_pc(p, alignment_assumes);
    this->add_assume_preds_at_pc(p, theos);
    this->add_assert_preds_at_pc(p, asserts);
    if(p.is_start())
      this->extract_align_assumes(theos, alignment_assumes);
    //cout << __func__ << " " << __LINE__ << endl;
    //(*t)->add_outgoing_values(p, outgoing_values);
    //cout << __func__ << " " << __LINE__ << endl;
  }

  ASSERT(line == "=Global assumes");
  m_global_assumes = T_PRED::predicate_set_from_stream(in/*, this->get_start_state()*/, ctx, "global assume ");

  end = !getline(in, line);
  ASSERT(!end);
  //cout << __func__ << " " << __LINE__ << endl;

  while (line == "") {
    //cout << __func__ << " " << __LINE__ << endl;
    getline(in, line);
  }
  //cout << __func__ << " " << __LINE__ << endl;

  graph_memlabel_map_t memlabel_map;
  line = this->read_memlabel_map(in, line, memlabel_map);
  this->set_memlabel_map(memlabel_map);

  while (line == "") {
    //cout << __func__ << " " << __LINE__ << endl;
    getline(in, line);
  }

  string const graph_edge_prefix = "=Edge";
  while (is_line(line, graph_edge_prefix + ":")) {
    //cout << __func__ << " " << __LINE__ << ": reading edge: " << line << endl;
    line = this->read_graph_edge(in, line, graph_edge_prefix);
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    while (line == "") {
      //cout << __func__ << " " << __LINE__ << endl;
      bool end = !getline(in, line);
      ASSERT(!end);
    }
  }
  //cout << "done reading edges: line = " << line << endl;
  return line;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
string
graph_with_predicates<T_PC, T_N, T_E, T_PRED>::read_memlabel_map(istream& in, string line, graph_memlabel_map_t &memlabel_map)
{
  //context* ctx =  this->get_context();
  while (is_line(line, "=memlabel_map")) {
    //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
    getline(in, line);
    //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
    size_t colon = line.find(':');
    string mlvarname_str = line.substr(0, colon);
    trim(mlvarname_str);
    string ml_str = line.substr(colon + 1);
    memlabel_t ml;
    memlabel_t::memlabel_from_string(&ml, ml_str.c_str());
    memlabel_map[mk_string_ref(mlvarname_str)] = mk_memlabel_ref(ml);
    getline(in, line);
  }
  return line;
}

//string read_outgoing_values(istream& in, string line, list<pair<string, expr_ref>> &outgoing_values) const
//{
//  context* ctx = this->get_context();
//  //cout << "reading assumes:" << endl;
//  while(true)
//  {
//    if (line == "") {
//      getline(in, line);
//      continue;
//    }
//    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//    if (is_line(line, "=StateTo") || is_line(line, "=Edge")  || is_line(line, "=Node:")|| is_line(line, "=TFG:") || is_line(line, "=memlabel_map") || is_line(line, "=Locs"))
//    {
//      return line;
//    }
//
//    if (is_line(line, "OutgoingValue"))
//    {
//      string comment;
//      bool end;
//
//      //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//      end = !!getline(in, line);
//      ASSERT(end);
//      //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//      ASSERT(is_line(line, "Comment"));
//      end = !!getline(in, line);
//      ASSERT(end);
//      comment = line;
//      //cout << __func__ << " " << __LINE__ << ": comment = " << comment << endl;
//      end = !!getline(in, line);
//      ASSERT(end);
//      ASSERT(is_line(line, "Expr"));
//      //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//      expr_ref e;
//      line = read_expr(in, e, ctx);
//      //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//      outgoing_values.push_back(make_pair(comment, e));
//    }
//    if(!is_line(line, "="))
//      return "";
//  }
//  assert(false);
//}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
string
graph_with_predicates<T_PC, T_N, T_E, T_PRED>::read_graph_edge(istream& in, string const& first_line, string const& prefix)
{
  context *ctx = this->get_context();
  //expr_ref cond/*, indir_tgt*/;
  //bool is_indir_exit;
  //state state_to;

  shared_ptr<T_E const> te;
  string line = T_E::graph_edge_from_stream(in, first_line, prefix/*, this->get_start_state()*/, ctx, te);
  //string line = read_graph_edge_using_start_state(in, first_line, prefix, this->get_start_state(), te);
  ASSERT(te);
  this->add_edge(te);

  return line;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
string graph_with_predicates<T_PC, T_N, T_E, T_PRED>::read_assumes(istream& in, string line, unordered_set<shared_ptr<T_PRED const>> &theos) const
{
  context *ctx = this->get_context();
  //cout << "reading assumes:" << endl;
  while(true)
  {
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    if (line == "") {
      getline(in, line);
      continue;
    } else if (is_line(line, "=StateTo") || is_line(line, "=Edge")  || is_line(line, "=Node:")|| is_line(line, "=TFG:") || is_line(line, "OutgoingValue") || is_line(line, "=memlabel_map") || is_line(line, "=Locs")) {
      return line;
    } else if (is_line(line, "CP-theorem-suggested:") || is_line(line, "Invariant")) {
      shared_ptr<T_PRED const> pred;
      line = T_PRED::predicate_from_stream(in/*, this->get_start_state()*/, ctx, pred);
      theos.insert(pred);
      //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    }
    if (!is_line(line, "=")) {
      return "";
    } else {
      //cout << __func__ << " " << __LINE__ << ": could not parse line: " << line << endl;
    }
  }
  assert(false);
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
set<T_PC>
graph_with_predicates<T_PC, T_N, T_E, T_PRED>::get_to_pcs_for_edges(set<shared_ptr<T_E const>> const &edges)
{
  set<T_PC> ret;
  for (auto e : edges) {
    ret.insert(e->get_to_pc());
  }
  return ret;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
expr_ref graph_with_predicates<T_PC, T_N, T_E, T_PRED>::setup_debug_probes(expr_ref e)
{
  context *ctx = e->get_context();
  if (e->get_operation_kind() == expr::OP_EQ) {
    ASSERT(e->get_args().size() == 2);
    expr_ref output_src, output_dst;
    output_src = ctx->mk_var("output_src", e->get_args().at(0)->get_sort());
    output_dst = ctx->mk_var("output_dst", e->get_args().at(1)->get_sort());

    expr_ref arg0 = e->get_args()[0];
    expr_ref arg1 = e->get_args()[1];
    set<expr_ref> arg0_probes = m_ctx->expr_substitute_function_arguments_with_debug_probes(arg0, "src");
    set<expr_ref> arg1_probes = m_ctx->expr_substitute_function_arguments_with_debug_probes(arg1, "dst");

    expr_list lhs;
    lhs.push_back(ctx->mk_eq(output_src, arg0));
    lhs.push_back(ctx->mk_eq(output_dst, arg1));
    for (auto e : arg0_probes) {
      lhs.push_back(e);
    }
    for (auto e : arg1_probes) {
      lhs.push_back(e);
    }

    expr_ref ret = ctx->mk_or(ctx->mk_not(expr_list_and(lhs, expr_true(ctx))), ctx->mk_eq(arg0, arg1));
    return ret;
  }
  return e;
}

template class graph_with_predicates<pc, tfg_node, tfg_edge, predicate>;
template class graph_with_predicates<pcpair, corr_graph_node, corr_graph_edge, predicate>;

}
