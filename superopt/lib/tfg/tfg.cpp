#include "tfg/tfg.h"
#include <boost/algorithm/string.hpp>
#include "expr/solver_interface.h"
#include "eq/corr_graph.h"
/* #include "graph_cp.h" */

#include <iostream>
#include "support/log.h"
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/utils.h"
#include "support/globals_cpp.h"
#include "expr/memlabel.h"
#include "expr/consts_struct.h"
#include "expr/expr_simplify.h"
#include "expr/local_sprel_expr_guesses.h"
#include "graph/lr_map.h"
#include "gsupport/sprel_map.h"
#include "tfg/parse_input_eq_file.h"
#include "graph/graph_fixed_point_iteration.h"
#include "graph/bav_dfa.h"
#include "graph/liveness_dfa.h"
#include "graph/loc_def_dfa.h"
//#include "gsupport/sym_exec.h"
//#include "rewrite/locals_info_cpp.h"
#include "rewrite/transmap.h"
#include "rewrite/static_translate.h"
#include "rewrite/translation_instance.h"
#include <fstream>
#include <boost/algorithm/string/replace.hpp>
#include "support/timers.h"
#include "support/stopwatch.h"
#include "support/debug.h"
#include "support/distinct_sets.h"
#include "expr/context_cache.h"
#include "graph/expr_loc_visitor.h"
#include "rewrite/translation_instance.h"
#include "rewrite/symbol_map.h"
#include "expr/graph_local.h"
#include "rewrite/symbol_or_section_id.h"
#include "rewrite/cfg.h"
#include "rewrite/transmap_set.h"
#include "gsupport/gsupport_cache.h"
#include "rewrite/dst-insn.h"
#include "rewrite/src-insn.h"
#include "tfg/suffixpath_dfa.h"
#include "codegen/etfg_insn.h"

#define SEM_PRUNNING_THRESHOLD 256
#define TFG_ENUMERATION_MAX_PATH_LENGTH 80
#define CHECK_FEASIBLE_PATH_THRESHOLD 40

#define PREDICATE_MERGE_THRESHOLD 64

static char as1[163840];
//extern char const *peeptab_filename;

#undef DEBUG_TYPE
#define DEBUG_TYPE "tfg"
namespace eqspace {
unsigned g_page_size_for_safety_checks = 4096;


bool COMPARE_IO_ELEMENTS = true;


/*static shared_ptr<tfg_edge>
find_edge_using_string(const string& str, tfg const * t)
{
  string s(str);
  trim(s);
  unsigned pos = s.find("=>");
  string pc1 = s.substr(0, pos);
  string pc2 = s.substr(pos+2);
  pc p1 = pc::create_from_string(pc1);
  pc p2 = pc::create_from_string(pc2);

  return t->get_edge(p1, p2);
}*/

/*void tfg_edge_composite::add_edge(shared_ptr<tfg_edge> e, state const &start_state)
{
  assert(is_next_edge_in_series(e) && is_next_edge_in_parallel(e) && "ambiguity in adding to composite edge");

  if (is_next_edge_in_series(e)) {
    add_edge_in_series(e, start_state, this->get_consts_struct());
  } else if(is_next_edge_in_parallel(e)) {
    add_edge_in_parallel(e, start_state, this->get_consts_struct());
  } else {
    assert(false);
  }
}

void tfg_edge_composite::add_edge_in_series(shared_ptr<tfg_edge> e, state const &start_state, consts_struct_t const &cs)
{
  tfg_edge::add_edge_in_series(e, start_state, cs);

  shared_ptr<serpar_composition_node_t<tfg_edge_ptr>> incoming_composition;
  if (shared_ptr<tfg_edge_composite> e_composite = dynamic_pointer_cast<tfg_edge_composite>(e)) {
    incoming_composition = e_composite->m_composition_node;
  } else {
    incoming_composition = make_shared<serpar_composition_node_t<tfg_edge_ptr>>(e);
  }
  m_composition_node = make_shared<serpar_composition_node_t<tfg_edge_ptr>>(serpar_composition_node_t<tfg_edge_ptr>::series, m_composition_node, incoming_composition);
}

void tfg_edge_composite::add_edge_in_parallel(shared_ptr<tfg_edge> e, state const &start_state, consts_struct_t const &cs)
{
  tfg_edge::add_edge_in_parallel(e, start_state, cs);

  shared_ptr<serpar_composition_node_t<tfg_edge_ptr>> incoming_composition;
  if (shared_ptr<tfg_edge_composite> e_composite = dynamic_pointer_cast<tfg_edge_composite>(e)) {
    incoming_composition = e_composite->m_composition_node;
  } else {
    incoming_composition = make_shared<serpar_composition_node_t<tfg_edge_ptr>>(e);
  }
  m_composition_node = make_shared<serpar_composition_node_t<tfg_edge_ptr>>(serpar_composition_node_t<tfg_edge_ptr>::parallel, m_composition_node, incoming_composition);
}

bool tfg_edge_composite::is_next_edge_in_series(shared_ptr<tfg_edge> next_e)
{
  if (get_to_pc() == next_e->get_from_pc()) {
    return true;
  }
  return false;
}

bool tfg_edge_composite::is_next_edge_in_parallel(shared_ptr<tfg_edge> next_e)
{
  if(get_from_pc() == next_e->get_from_pc() && get_to_pc() == next_e->get_to_pc())
    return true;

  return false;
}

shared_ptr<tfg_edge_composite>
tfg_edge_composite::create_from_parallel_paths(list<shared_ptr<tfg_edge_composite>> paths, state const &start_state, consts_struct_t const &cs)
{
  assert(paths.size() > 0);
  shared_ptr<tfg_edge_composite> first = paths.front();
  //cout << __func__ << " " << __LINE__ << ": adding edge in parallel " << first->to_string() << ": to_state:\n" << first->get_to_state().to_string() << endl;
  paths.pop_front();

  shared_ptr<tfg_edge_composite> ret = make_shared<tfg_edge_composite>(first);
  for (auto path : paths) {
    //cout << __func__ << " " << __LINE__ << ": adding edge in parallel " << path->to_string() << ": to_state:\n" << path->get_to_state().to_string() << endl;
    ret->add_edge_in_parallel(path, start_state, cs);
  }
  return ret;
}

shared_ptr<tfg_edge_composite> tfg_edge_composite::create_from_composition(composition_node const &c, state const &start_state, consts_struct_t const &cs)
{
  switch(c.m_type)
  {
    case composition_node::single_edge:
      return make_shared<tfg_edge_composite>(c.m_e);
    case composition_node::series:
      {
        shared_ptr<tfg_edge_composite> e = create_from_composition(*c.m_a, start_state, cs);
        e->add_edge_in_series(create_from_composition(*c.m_b, start_state, cs), start_state, cs);
        return e;
      }
    case composition_node::parallel:
      {
        shared_ptr<tfg_edge_composite> e = create_from_composition(*c.m_a, start_state, cs);
        e->add_edge_in_parallel(create_from_composition(*c.m_b, start_state, cs), start_state, cs);
        return e;
      }
  }
  unreachable();
}*/

//tfg::tfg(string_ref const &name, context* ctx, state const &start_state) : graph_with_paths<pc, tfg_node, tfg_edge, predicate>(name, ctx, start_state)
tfg::tfg(string const &name, context* ctx) : graph_with_execution<pc, tfg_node, tfg_edge, predicate>(name, ctx)
{ }

tfg::tfg(tfg const &other) : graph_with_execution<pc, tfg_node, tfg_edge, predicate>(other),
                             m_memory_reg(other.m_memory_reg),
                             //m_outgoing_values(other.m_outgoing_values),
                             //m_locals_info(other.m_locals_info),
                             m_nextpc_map(other.m_nextpc_map),
                             m_callee_summaries(other.m_callee_summaries),
                             m_string_contents(other.m_string_contents),
                             m_max_mlvarnum(other.m_max_mlvarnum)
{ }

tfg::tfg(istream& in, string const &name, context* ctx) : graph_with_execution<pc, tfg_node, tfg_edge, predicate>(in, name, ctx)
{
  string line;
  bool end;
  do {
    end = !getline(in, line);
    ASSERT(!end);
  } while (line == "");
  if (!is_line(line, "=String-contents:")) {
    cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
  }

  this->compute_max_memlabel_varnum();

  ASSERT(is_line(line, "=String-contents:"));
  map<pair<symbol_id_t, offset_t>, vector<char>> string_contents;
  map<nextpc_id_t, string> nextpc_map;
  map<nextpc_id_t, callee_summary_t> callee_summaries;
  getline(in, line);
  while (!is_line(line, "=Nextpc-map:")) {
    ASSERT(line.substr(0, strlen("C_SYMBOL")) == "C_SYMBOL");
    size_t remaining;
    int symbol_id = stoi(line.substr(strlen("C_SYMBOL")), &remaining);
    ASSERT(symbol_id >= 0);
    size_t colon1 = line.find(':');
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    ASSERT(colon1 != string::npos);
    size_t colon2 = line.find(':', colon1 + 1);
    ASSERT(colon2 != string::npos);
    string off_str = line.substr(colon1 + 1, colon2);
    offset_t off = string_to_int(off_str, -1);
    ASSERT(off != -1);
    string remaining_line = line.substr(colon2 + 1);
    //cout << __func__ << " " << __LINE__ << ": remaining_line = " << remaining_line << endl;
    char const *remaining_str = remaining_line.data();
    //cout << __func__ << " " << __LINE__ << ": remaining string is: " << remaining_str << endl;
    while (*remaining_str == ' ') {
      remaining_str++;
    }
    vector<char> s;
    while (remaining_str[0] == '\\') {
      int a = strtol(remaining_str + 1, (char **)&remaining_str, 10);
      s.push_back(char(a));
    }
    //cout << __func__ << " " << __LINE__ << ": remaining string is: " << remaining_str << endl;
    //cout << __func__ << " " << __LINE__ << ": remaining string[0] is: " << (int)remaining_str[0] << endl;
    ASSERT(remaining_str[0] == '\0');
    string_contents[make_pair(symbol_id, off)] = s;
    getline(in, line);
  }

  //nextpc_map.push_back(G_SELFCALL_IDENTIFIER);
  ASSERT(is_line(line, "=Nextpc-map:"));
  getline(in, line);
  while (is_line(line, "C_NEXTPC")) {
    ASSERT(line.substr(0, strlen("C_NEXTPC")) == "C_NEXTPC");
    size_t remaining;
    int nextpc_id = stoi(line.substr(strlen("C_NEXTPC")), &remaining);
    ASSERT(nextpc_id >= 0);
    size_t colon = line.find(':');
    //ASSERT(colon == remaining + strlen("C_NEXTPC"));
    ASSERT(colon != string::npos);
    string nextpc_name = line.substr(colon + 1);
    colon = nextpc_name.find(':');
    callee_summary_t csum;
    bool csum_exists = false;
    if (colon != string::npos) {
      string callee_summary_str = nextpc_name.substr(colon + 1);
      nextpc_name = nextpc_name.substr(0, colon);
      csum.callee_summary_read_from_string(callee_summary_str);
      csum_exists = true;
    }
    trim(nextpc_name);
    //nextpc_map.push_back(nextpc_name);
    nextpc_map[nextpc_id] = nextpc_name;
    if (csum_exists) {
      callee_summaries[nextpc_id] = csum;
      //cout << __func__ << " " << __LINE__ << ": nextpc_id = " << nextpc_id << endl;
      //cout << __func__ << " " << __LINE__ << ": csum = " << csum.callee_summary_to_string_for_eq() << endl;
    }
    getline(in, line);
  }

  this->set_string_contents(string_contents);
  this->set_nextpc_map(nextpc_map);
  this->set_callee_summaries(callee_summaries);

  this->populate_transitive_closure();
  this->populate_reachable_and_unreachable_incoming_edges_map();

  ASSERT(line == FUNCTION_FINISH_IDENTIFIER);
}

predicate_set_t
tfg::tfg_evaluate_pred_on_edge_composition(shared_ptr<tfg_edge_composition_t> const& ec, predicate_ref const &pred) const
{
  predicate_set_t ret;
  if (!ec) {
    ret.insert(pred);
    return ret;
  }
  list<pair<edge_guard_t, predicate_ref>> preds_apply = this->apply_trans_funs(ec, pred, false);
  for (auto epa : preds_apply) {
    edge_guard_t eg = epa.first;
    predicate_ref pa = epa.second;
    edge_guard_t eguard = pa->get_edge_guard();
    eg.add_guard_in_series(eguard);
    pa = pa->set_edge_guard(eguard);
    ret.insert(pa);
  }
  return ret;
}

void
tfg::tfg_evaluate_dst_preds_on_edge_and_add_to_src_node(shared_ptr<tfg_edge const> in_e)
{
  pc const &p = in_e->get_to_pc();
  predicate_set_t const &preds = this->get_assume_preds(p);
  pc src_pc = in_e->get_from_pc();
  for (const auto &pred : preds) {
    predicate_set_t preds_evaluated = this->tfg_evaluate_pred_on_edge_composition(mk_edge_composition<pc,tfg_edge>(in_e), pred);
    for (auto epa : preds_evaluated) {
      this->add_assume_pred(src_pc, epa);
    }
  }
}

void
tfg::remove_node_and_make_composite_edges(pc const &p)
{
  list<shared_ptr<tfg_edge const>> out;
  list<shared_ptr<tfg_edge const>> in;
  get_edges_outgoing(p, out);
  get_edges_incoming(p, in);

  list<pair<shared_ptr<tfg_edge const>, shared_ptr<tfg_edge const>>> pdt;
  cartesian_product<shared_ptr<tfg_edge const>>(in, out, pdt);
  if (pdt.size() > 1) {
    cout << __func__ << " " << __LINE__ << ": Warning : removing " << p.to_string() << ", pdt.size() = " << pdt.size() << " (> 1)" << ", incoming.size " << in.size() << ", outgoing.size " << out.size() << endl << ", max_expr_size = " << this->max_expr_size() << endl;
  }

  for (const auto &in_e : in) {
    tfg_evaluate_dst_preds_on_edge_and_add_to_src_node(in_e);
  }

  graph_remove_preds_for_pc(p);

  for (const auto& edge_pair : pdt) {
    shared_ptr<tfg_edge const> in_e = edge_pair.first;
    shared_ptr<tfg_edge const> out_e = edge_pair.second;

    assert(in_e->get_from_pc() != in_e->get_to_pc() &&
        out_e->get_from_pc() != out_e->get_to_pc() &&
        "can't compose self loop");
    shared_ptr<tfg_edge_composition_t> in_edge_c = mk_edge_composition<pc,tfg_edge>(in_e);
    shared_ptr<tfg_edge_composition_t> out_edge_c = mk_edge_composition<pc,tfg_edge>(out_e);
    shared_ptr<tfg_edge_composition_t> new_edge = mk_series(in_edge_c, out_edge_c);
    shared_ptr<tfg_edge const> existing_edge = get_edge(in_e->get_from_pc(), out_e->get_to_pc());
    if (existing_edge) {
      new_edge = mk_parallel(mk_edge_composition<pc,tfg_edge>(existing_edge), new_edge);
      add_edge_composite(new_edge);
      remove_edge(existing_edge);
    } else {
      add_edge_composite(new_edge);
    }
  }

  remove_edges(in);
  remove_edges(out);
  remove_node(p);
}

pc
tfg::remove_nodes_get_min_incoming_outgoing(map<pc, tuple<size_t, size_t, vector<size_t>, vector<size_t>>> const &remove_nodes) const
{
  size_t min_weight = SIZE_MAX;
  size_t min_cost = SIZE_MAX;
  pc ret;
  for (auto e : remove_nodes) {
    size_t cur_weight = get<0>(e.second) * get<1>(e.second);
    vector<size_t> incoming = get<2>(e.second);
    vector<size_t> outgoing = get<3>(e.second);
    size_t max_incoming_cost = vector_max(incoming);
    size_t max_outgoing_cost = vector_max(outgoing);
    size_t cur_cost = min(max_incoming_cost, max_outgoing_cost);

    //cout << __func__ << " " << __LINE__ << ": " << e.first.to_string() << ": weight " << cur_weight << ", incoming_cost " << max_incoming_cost << ", outgoing_cost " << max_outgoing_cost << endl;

    if (   cur_weight < min_weight
        || (cur_weight == min_weight && cur_cost < min_cost)) {
      min_weight = cur_weight;
      min_cost = cur_cost;
      ret = e.first;
    }
  }
  ASSERT(min_weight != LONG_MAX);
  return ret;
}

tuple<size_t, size_t, vector<size_t>, vector<size_t>>
tfg::remove_nodes_compute_node_weight_tuple(list<shared_ptr<tfg_edge const>> const &incoming, list<shared_ptr<tfg_edge const>> const &outgoing) const
{
  vector<size_t> incoming_weights, outgoing_weights;
  for (const auto &e : incoming) {
    incoming_weights.push_back(e->get_collapse_cost());
  }
  for (const auto &e : outgoing) {
    outgoing_weights.push_back(e->get_collapse_cost());
  }
  return make_tuple(incoming.size(), outgoing.size(), incoming_weights, outgoing_weights);
}

void
tfg::remove_nodes_update_incoming_outgoing(map<pc, tuple<size_t, size_t, vector<size_t>, vector<size_t>>> &remove_nodes) const
{
  for (auto e : remove_nodes) {
    list<shared_ptr<tfg_edge const>> incoming, outgoing;
    pc const &p = e.first;
    get_edges_incoming(p, incoming);
    get_edges_outgoing(p, outgoing);
    remove_nodes[p] = remove_nodes_compute_node_weight_tuple(incoming, outgoing);
    //remove_nodes[p] = make_pair(incoming.size(), outgoing.size());
  }
}

  static bool
tfg_edge_compare_src_dst_pcs(shared_ptr<tfg_edge const> const &a, shared_ptr<tfg_edge const> const &b)
{
  pc_comp pc_comp;
  if (pc_comp(a->get_from_pc(), b->get_from_pc())) {
    return true;
  } else if (pc_comp(b->get_from_pc(), a->get_from_pc())) {
    return false;
  }
  if (pc_comp(a->get_to_pc(), b->get_to_pc())) {
    return true;
  } else if (pc_comp(b->get_to_pc(), a->get_to_pc())) {
    return false;
  }
  return false;
}

  bool
tfg::node_has_only_nop_outgoing_edge(shared_ptr<tfg_node> n)
{
  list<shared_ptr<tfg_edge const>> outgoing;
  get_edges_outgoing(n->get_pc(), outgoing);
  if (outgoing.size() != 1) {
    return false;
  }
  for (auto e : outgoing) {
    if (!e->is_nop_edge(/*this->get_start_state()*/)) {
      return false;
    }
  }
  return true;
}

void tfg::collapse_nop_edges()
{
  list<shared_ptr<tfg_node>> nodes, nodes_to_remove;
  this->get_nodes(nodes);
  for (auto n : nodes) {
    if (   n->get_pc().is_start()
        || n->get_pc().is_exit()
        || n->get_pc().get_subsubindex() == PC_SUBSUBINDEX_BASIC_BLOCK_ENTRY
        || (n->get_pc().get_subsubindex() == PC_SUBSUBINDEX_FCALL_END && this->pc_has_incident_fcall(n->get_pc()))) {
      continue;
    }
    if (node_has_only_nop_outgoing_edge(n)) {
      nodes_to_remove.push_back(n);
    }
  }
  for (auto n : nodes_to_remove) {
    remove_node_and_make_composite_edges(n->get_pc());
  }
}

void tfg::combine_edges_with_same_src_dst_pcs()
{
  list<list<shared_ptr<tfg_edge const>>> edges_to_be_modified; //list of groups of edges; each group of edges will be replaced by a single composite edge
  list<shared_ptr<tfg_edge const>> edges;
  get_edges(edges);
  edges.sort(tfg_edge_compare_src_dst_pcs);
  for (list<shared_ptr<tfg_edge const>>::const_iterator iter = edges.begin();
      iter != edges.end();) {
    list<shared_ptr<tfg_edge const>> similar_edges;
    list<shared_ptr<tfg_edge const>>::const_iterator iter2 = iter;

    for (iter2++;
           iter2 != edges.end()
        && (*iter2)->get_from_pc() == (*iter)->get_from_pc()
        && (*iter2)->get_to_pc() == (*iter)->get_to_pc();
        iter2++) {
      similar_edges.push_back(*iter2);
    }
    if (similar_edges.size() != 0) {
      similar_edges.push_back(*iter);
      edges_to_be_modified.push_back(similar_edges);
    }
    iter = iter2;
  }
  for (auto mls : edges_to_be_modified) {
    ASSERT(mls.size() >= 2);
    list<shared_ptr<tfg_edge const>>::const_iterator iter = mls.begin();
    //shared_ptr<tfg_edge_composition_t> new_edge = make_shared<tfg_edge_composition_t>(*iter, *this);
    shared_ptr<tfg_edge_composition_t> new_edge = mk_edge_composition<pc,tfg_edge>(*iter);
    remove_edge(*iter);
    for (iter++; iter != mls.end(); iter++) {
      //new_edge = make_shared<tfg_edge_composition_t>(tfg_edge_composition_t::parallel, new_edge, make_shared<tfg_edge_composition_t>(*iter, *this), *this);
      new_edge = mk_parallel(new_edge, mk_edge_composition<pc,tfg_edge>(*iter));
      //new_edge->add_edge_in_parallel(*iter, this->get_start_state(), this->get_consts_struct());
      remove_edge(*iter);
    }
    add_edge_composite(new_edge);
  }
}

bool tfg::is_fcall_pc(pc const &p)
{
  int subsubindex = p.get_subsubindex();
  return subsubindex >= PC_SUBSUBINDEX_FCALL_START && subsubindex <= PC_SUBSUBINDEX_FCALL_END;
}

bool tfg::ignore_fcall_edges(shared_ptr<tfg_edge const> const &e)
{
  if (   is_fcall_pc(e->get_from_pc())
      || is_fcall_pc(e->get_to_pc())) {
    return false;
  }
  return true;
}

void tfg::collapse_tfg(bool preserve_branch_structure/*, bool collapse_function_calls*/)
{
  //ASSERT(preserve_branch_structure);
  //ASSERT(!collapse_function_calls || !preserve_branch_structure); //both cannot be true
  // cout << __func__ << " " << __LINE__ << ": before collapsing nop edges" << endl << this->graph_to_string() << endl;
  context *ctx = this->get_context();
  collapse_nop_edges();

  // cout << __func__ << " " << __LINE__ << ": before combining edges_with_same_src_dst_pcs " << endl << this->graph_to_string() << endl;
  combine_edges_with_same_src_dst_pcs();

  // cout << __func__ << " " << __LINE__ << ": after combining edges_with_same_src_dst_pcs " << endl << this->graph_to_string() << endl;


  populate_backedges();

  // get nodes to remove, along with the number of incoming/outgoing edges
  map<pc, tuple<size_t, size_t, vector<size_t>, vector<size_t>>> remove_nodes;
  list<shared_ptr<tfg_node>> ns;
  this->get_nodes(ns);
  for (shared_ptr<tfg_node> n : ns) {
    if (n->get_pc().is_start() || n->get_pc().is_exit()) {
      continue;
    }
    list<shared_ptr<tfg_edge const>> incoming, outgoing;
    get_edges_incoming(n->get_pc(), incoming);
    get_edges_outgoing(n->get_pc(), outgoing);

    bool keep_node = false;
    if (preserve_branch_structure) {
      keep_node = keep_node || (incoming.size() > 1) || (outgoing.size() > 1);
      for (auto e : incoming) {
        pc const &from_pc = e->get_from_pc();
        list<shared_ptr<tfg_edge const>> from_outgoing;
        get_edges_outgoing(from_pc, from_outgoing);
        if (from_outgoing.size() > 1) {
          keep_node = true;
        }
      }
    }

    /*if (!collapse_function_calls) */{
      //keep_node = keep_node || (n->get_pc().get_subsubindex() == PC_SUBSUBINDEX_FCALL_END);
      //keep_node = keep_node || (n->get_pc().get_subsubindex() == PC_SUBSUBINDEX_FCALL_END && this->pc_has_incident_fcall(n->get_pc()));
      //for (auto e : outgoing) {
      //  pc const &to_pc = e->get_to_pc();
      //  if (to_pc.get_subsubindex() == PC_SUBSUBINDEX_FCALL_END && this->pc_has_incident_fcall(to_pc)) {
      //    keep_node = true;
      //  }
      //}
      keep_node = keep_node || n->get_pc().is_fcall();
    }

    set<pc> transitive_closure = this->compute_all_reachable_pcs_using_edge_select_fn(n->get_pc(), ignore_fcall_edges);
    if (set_belongs(transitive_closure, n->get_pc())) { //if we have a cycle starting at this node
      for (const auto& e : incoming) {
        keep_node = keep_node || e->is_back_edge(); //and this node is the target of a back edge, then keep this node
      }
    }
    //for (const auto& e : incoming) {
    //  // if (e->is_back_edge()) {
    //  //   cout << __func__ << " " << __LINE__ << ": " << e->to_string() << " is back edge\n";
    //  // }
    //  keep_node = keep_node || e->is_back_edge();
    //  ///*if (!collapse_function_calls) */{
    //  //  keep_node = keep_node || e->contains_function_call();
    //  //}
    //}
    /*if (!collapse_function_calls) */{
      //for (const auto& e : outgoing) {
      //  keep_node = keep_node || (/*incoming.size() > 1 && */e->contains_function_call());
      //}
    }
    if (!keep_node) {
      //cout << __func__ << " " << __LINE__ << ": removing node " << n->get_pc().to_string() << endl;
      remove_nodes[n->get_pc()] = remove_nodes_compute_node_weight_tuple(incoming, outgoing);
    } else {
      //cout << __func__ << " " << __LINE__ << ": keeping node " << n->get_pc().to_string() << endl;
    }
  }

  // remove nodes and make composite edges
  while (remove_nodes.size()) {
    pc const &p = remove_nodes_get_min_incoming_outgoing(remove_nodes);
    //cout << __func__ << " " << __LINE__ << ": removing node " << p.to_string() << endl;
    remove_node_and_make_composite_edges(p);
    // cout << __func__ << " " << __LINE__ << ": after removing pc " << p << " with " << get<0>(remove_nodes.at(p)) << " incoming, and " << get<1>(remove_nodes.at(p)) << " outgoing edges" << endl << this->graph_to_string() << endl;
    remove_nodes.erase(p);
    //cout << __func__ << __LINE__ << ": after remove_nodes.erase(p)" << endl;
    remove_nodes_update_incoming_outgoing(remove_nodes);
    //cout << __func__ << __LINE__ << ": after remove_nodes_update_incoming_outgoing()" << endl;
  }
  populate_backedges();
  if (!preserve_branch_structure) {
    //remove all nodes that do not have a self-cycle
    list<shared_ptr<tfg_node>> ns;
    this->get_nodes(ns);
    for (shared_ptr<tfg_node> n : ns) {
      if (n->get_pc().is_start() || n->get_pc().is_exit()) {
        continue;
      }
      list<shared_ptr<tfg_edge const>> incoming;
      get_edges_incoming(n->get_pc(), incoming);
      bool keep_node = false;
      for (auto e : incoming) {
        if (e->get_from_pc() == n->get_pc() || e->is_back_edge()) {
          keep_node = true;
          break;
        }
      }

      /*if (!collapse_function_calls) */{
        //keep_node = keep_node || (n->get_pc().get_subsubindex() == PC_SUBSUBINDEX_FCALL_END);
        //keep_node = keep_node || (n->get_pc().get_subsubindex() == PC_SUBSUBINDEX_FCALL_END && this->pc_has_incident_fcall(n->get_pc()));
        //list<shared_ptr<tfg_edge>> outgoing;
        //get_edges_outgoing(n->get_pc(), outgoing);
        //for (const auto& e : outgoing) {
        //  keep_node = keep_node || (/*incoming.size() > 1 && */e->contains_function_call());
        //}
        keep_node = keep_node || n->get_pc().is_fcall();
      }
      if (!keep_node) {
        remove_node_and_make_composite_edges(n->get_pc());
      }
      // cout << __func__ << " " << __LINE__ << ": after remove_node_and_make_composite_edges " << n->get_pc() << endl << this->graph_to_string() << endl;
    }
  }
}

/*void tfg::remove_trivial_nodes(bool collapse_function_calls)
{
  collapse_nop_edges();
  stats::get().set_value("eq-state", "remove_trivial_nodes");
  list<scc<pc>> sccs;
  get_sccs(sccs);
  list<pc> remove_nodes;
  for (scc<pc>& s : sccs) {
    if (   s.is_component_without_backedge()
        && !s.get_first_pc().is_exit()
        && !s.get_first_pc().is_start()
        && s.get_first_pc().get_subsubindex() == 0) {
      list<shared_ptr<tfg_edge>> incoming, outgoing;
      pc const &p = s.get_first_pc();
      get_edges_incoming(p, incoming);
      get_edges_outgoing(p, outgoing);
      bool keep_node = false;
      for (const auto& e : incoming) {
        if (!collapse_function_calls) {
          keep_node = keep_node || e->contains_function_call();
        }
        keep_node = keep_node || (e->get_from_pc().get_index() != p.get_index());
      }
      for (const auto& e : outgoing) {
        if (!collapse_function_calls) {
          keep_node = keep_node || e->contains_function_call();
        }
        keep_node = keep_node || (e->get_to_pc().get_index() != p.get_index());
      }
      if (!keep_node) {
        remove_nodes.push_back(s.get_first_pc());
      }
    }
  }

  for(const pc& p : remove_nodes) {
    //cout << __func__ << " " << __LINE__ << ": removing node " << p.to_string() << endl;
    remove_node_and_make_composite_edges(p);
  }
}*/

/*void tfg::specialize_edges()
{
  INFO("specialization edges start\n");
  list<shared_ptr<tfg_edge>> es;
  get_edges(es);
  for(auto& e : es)
  {
    query_comment qc(query_comment::specialize);
    e->get_to_state().specialize(e->get_cond(), qc);
  }
  INFO("specialization edges end\n");
}*/

//bool SPECIALIZE_EDGES = true;
void tfg::preprocess()
{
  //simplify_edges();
  //CPP_DBG_EXEC(EQGEN, cout << __func__ << " " << __LINE__ << ": after-simplify-edges:\n" << this->to_string(&this->get_start_state()) << endl);
  /*`if(SPECIALIZE_EDGES)
    specialize_edges();*/
  //simplify_edges();
  //CPP_DBG_EXEC(EQGEN, cout << __func__ << __LINE__ << ": after-simplify-edges2:\n" << this->to_string(&this->get_start_state()) << endl);
  populate_backedges();
}

/*string tfg::to_string(bool details) const
{
  stringstream ss;
  list<shared_ptr<const tfg_node>> ns;
  this->get_nodes(ns);
  ss << "NODES: ";
  for(auto n : ns)
    ss << n->to_string() << ", ";

  list<shared_ptr<tfg_edge>> es;
  get_edges(es);
  ss << "\nEDGES: ";
  for(auto e : es)
  {
    if(details)
      ss << "\nEDGE: " << e->to_string(details) << "\n";
    else
      ss << e->to_string(details) << ", ";
  }

  ss << "Relation:\n";
  ss << to_string_preds(true, false);
  return ss.str();
}*/

/*string
get_string_status(consts_struct_t const &cs, int lr_addr_ref_id, bool independent_of_input_stack_pointer, bool is_top, bool is_bottom)
{
  string status;
  if (lr_addr_ref_id == -1) {
    if (independent_of_input_stack_pointer) {
      status = "status-mem";
    } else if (is_top) {
      status = "status-top";
    } else if (is_bottom) {
      status = "status-mem";
    }
  } else {
    string addr_ref_name = cs.get_addr_ref(lr_addr_ref_id).first;
    status = "status-" + addr_ref_name;
  }
  return status;
}*/

//string
//tfg::tfg_to_string(bool details) const
//{
//  //map<pc, lr_map_t<pc, tfg_node, tfg_edge, predicate>> lrelations;
//  map<pc, map<graph_loc_id_t, lr_status_ref>> lrelations;
//  list<shared_ptr<tfg_node>> nodes;
//  stringstream ss;
//  context *ctx = this->get_context();
//
//  for (auto arg : get_argument_regs()) {
//    ss << "=Input: " << arg.first->get_str() << "\n" << ctx->expr_to_string(arg.second) << "\n";
//  }
//
//  this->get_nodes(nodes);
//  for (auto n : nodes) {
//    pc const &p = n->get_pc();
//    if (!p.is_exit()) {
//      continue;
//    }
//    if (!pc_has_return_regs(p)) {
//      continue;
//    }
//    for (auto ret : get_return_regs_at_pc(p)) {
//      ss << "=" << n->get_pc().to_string() << ": Output: " << ret.first->get_str() << "\n" << ctx->expr_to_string(ret.second) << "\n";
//    }
//  }
//
//  ss << this->to_string_with_linear_relations(lrelations, details);
//
//  return ss.str();
//}

string
tfg::tfg_to_string_with_invariants() const
{
  stringstream ss;
  list<shared_ptr<tfg_node>> ns;
  get_nodes(ns);
  //ss << m_guessing.guessing_state_to_string() << endl;
  ss << "NODES: ";
  for (auto n : ns) {
    ss << n->to_string() << ", ";
  }
  list<shared_ptr<tfg_edge const>> es;
  get_edges(es);
  ss << "\nEDGES:\n";
  ss << "  num_edges = " << es.size() << endl;
  size_t i = 0;
  for (auto e : es) {
    ss << "  EDGE " << i << ".\n" << e->to_string() << endl;
    i++;
  }
  ss << "\nProvable invariants:\n";
  for (auto n : ns) {
    ss << n->to_string() << ":\n";
    NOT_IMPLEMENTED(); //ss << this->provable_preds_to_string(n->get_pc(), "  ");
  }
  return ss.str();
}

void tfg::to_dot(string filename, bool embed_info) const
{
  ofstream fo;
  fo.open(filename.data());
  ASSERT(fo);
  fo << "digraph tfg {" << endl;
  for(auto e : this->get_edges_ordered()) {
    stringstream label;
    if(embed_info)
    {
      if(e->is_back_edge())
        label << "B" << "\\l";
      label << "Cond: " << expr_string(e->get_cond()) << "\\l";
      label << "TF: " << e->get_to_state().to_string(/*this->get_start_state(), */"\\l    ");
    }

    string label_str = label.str();
    boost::algorithm::replace_all(label_str, "\n", "\\l    ");
    fo << "\"" << e->get_from_pc().to_string() << "\"" << "->" << "\"" << e->get_to_pc().to_string() << "\""
       << "[label=" << "\"" << label_str  << "\"]" << endl;
       // "fontsize=5 " <<
       // "fontname=mono" <<
       // "]" << endl;
  }
  fo << "}" << endl;
  fo.close();
}

static bool
pred_compare_comment(predicate_ref const &a, predicate_ref const &b)
{
  return (a->get_comment() < b->get_comment());
}

void
tfg::graph_to_stream(ostream& ss) const
{
  autostop_timer func_timer(__func__);
  context *ctx = this->get_context();
  ss << "=TFG:\n";
  this->graph_with_paths<pc, tfg_node, tfg_edge, predicate>::graph_to_stream(ss);
  ss << "=String-contents:\n";
  for (auto const& s : m_string_contents) {
    ss << "C_SYMBOL" << s.first.first << " : " << s.first.second << " : ";
    for (size_t i = 0; i < s.second.size(); i++) {
      ss << "\\"  << unsigned((unsigned char)s.second.at(i));
    }
    ss << endl;
  }

  ss << "=Nextpc-map:\n";
  for (auto nextpc : m_nextpc_map) {
    ss << "C_NEXTPC" << nextpc.first << " : " << nextpc.second;
    if (m_callee_summaries.count(nextpc.first)) {
      ss << " : " << m_callee_summaries.at(nextpc.first).callee_summary_to_string_for_eq();
    }
    ss << endl;
  }

  //if (this->pred_dependencies_are_available()) {
  //  ss << endl << "=Dependencies" << endl;
  //  for (auto e : es) {
  //    ss << "=Dependencies for edge: " << e->to_string_for_eq() << "\n";
  //    for (const auto &loc : locs) {
  //      if (this->pred_min_dependencies_expr_for_edge_loc_is_default(e, loc.first)) {
  //        continue;
  //      }
  //      ss << "=Dependencies expr for loc " << loc.first << "\n";
  //      expr_ref ex = this->get_pred_min_dependencies_expr_for_edge_loc(e, loc.first);
  //      ss << ctx->expr_to_string_table(ex) << "\n";
  //      ss << "=Dependencies for loc " << loc.first << "\n";
  //      set<graph_loc_id_t> const &locset = this->get_pred_min_dependencies_set_for_edge_loc(e, loc.first);
  //      for (auto l : locset) {
  //        ss << l << ", ";
  //      }
  //      ss << "\n";
  //    }
  //  }
  //}
  ss << FUNCTION_FINISH_IDENTIFIER << '\n';
  //return ss.str();
}

static bool
mem_slots_less(tuple<memlabel_t, expr_ref, unsigned, bool> const &a,
               tuple<memlabel_t, expr_ref, unsigned, bool> const &b)
{
  if (memlabel_t::memlabel_less(&(get<0>(a)), &(get<0>(b)))) {
    return true;
  }
  if (memlabel_t::memlabel_less(&(get<0>(b)), &(get<0>(a)))) {
    return false;
  }
  if (get<1>(a)->get_id() < get<1>(b)->get_id()) {
    return true;
  }
  if (get<1>(a)->get_id() > get<1>(b)->get_id()) {
    return false;
  }
  if (get<2>(a) < get<2>(b)) {
    return true;
  }
  if (get<2>(a) > get<2>(b)) {
    return false;
  }
  if (get<3>(a) < get<3>(b)) {
    return true;
  }
  if (get<3>(a) > get<3>(b)) {
    return false;
  }
  return false;
}

bool tfg::check_if_any_edge_sets_flag(const string& flag_name) const
{
  list<shared_ptr<tfg_edge const>> es;
  get_edges(es);
  for (shared_ptr<tfg_edge const> e : es) {
    if(e->get_to_state().key_exists(flag_name/*, this->get_start_state()*/)) {
      if (e->get_to_state().get_expr(flag_name/*, this->get_start_state()*/) == expr_true(get_context())) {
        return true;
      }
    }
  }
  return false;
}

/*
void
tfg::substitute_rodata_accesses(tfg const &reference_tfg_for_symbol_types)
{
  autostop_timer func_timer(__func__);
  consts_struct_t const &cs = this->get_consts_struct();
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&reference_tfg_for_symbol_types, &cs, ctx](pc const &p, const string& key, expr_ref e) -> expr_ref { return ctx->expr_substitute_rodata_accesses(e, reference_tfg_for_symbol_types, cs); };
  tfg_visit_exprs(f);
  map<graph_loc_id_t, graph_cp_location> &locs = this->get_locs_ref();
  for (auto &l : locs) {
    if (l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
      l.second.m_memlabel = memlabel_remove_rodata_symbols(l.second.m_memlabel, reference_tfg_for_symbol_types);
    }
  }
  this->populate_locid2expr_map();
  this->update_edge2loc_map_and_dependencies(this->get_all_pcs());
}*/

void
tfg::tfg_add_transmap_translations_for_input_arguments(graph_arg_regs_t const &args, transmap_t const &in_tmap, int stack_gpr)
{
  context *ctx = this->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  list<shared_ptr<tfg_edge const>> start_edges;
  //pc start_pc(pc::insn_label, 0, PC_SUBINDEX_FIRST_INSN_IN_BB, 0);
  //get_edges_outgoing(start_pc, start_edges);
  get_edges_outgoing(pc::start(), start_edges);
  ASSERT(start_edges.size() == 1);

  //memlabel_t ml_stack;
  //keyword_to_memlabel(&ml_stack, G_STACK_SYMBOL, MEMSIZE_MAX);
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  //map<string_ref, expr_ref> name_submap;
  expr_ref mem;
  state const &start_state = this->get_start_state();
  bool ret = start_state.get_memory_expr(this->get_start_state(), mem);
  ASSERT(ret);
  //expr_ref iesp = cs.get_input_stack_pointer_const_expr();
  expr_ref mem_masked = mem; //cs.get_input_memory_const_expr();
  //expr_ref addr = iesp;
  //expr_ref retaddr_const = cs.get_retaddr_const();
  //mem_masked = ctx->mk_store(mem_masked, ml_stack, addr, retaddr_const, retaddr_const->get_sort()->get_size() / BYTE_LEN, false);
  //size_t cur_offset = 0;
  //cout << __func__ << " " << __LINE__ << ": in_tmap = " << transmap_to_string(&in_tmap, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": args.size() = " << args.size() << endl;
  for (const auto &a: args.get_map()) {
    string const &argname = a.first->get_str();
    expr_ref const &arg = a.second;
    if (argname == G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID) {
      string_ref name_dst = mk_string_ref(G_INPUT_KEYWORD "." G_DST_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID);
      string_ref name_src = mk_string_ref(G_INPUT_KEYWORD "." G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID);
      expr_ref from = ctx->mk_var(name_dst, ctx->mk_bv_sort(DWORD_LEN));
      expr_ref to = ctx->mk_var(name_src, ctx->mk_bv_sort(DWORD_LEN));
      submap[from->get_id()] = make_pair(from, to);
      //name_submap[name_dst] = to;
      continue;
    }
    //cout << __func__ << " " << __LINE__ << ": argname = " << argname << endl;
    //cout << __func__ << " " << __LINE__ << ": arg = " << expr_string(arg) << endl;
    exreg_group_id_t group;
    reg_identifier_t reg_id = reg_identifier_t::reg_identifier_invalid();
    if (argname.substr(0, 4) == "exvr") {
      size_t dot = argname.find('.');
      ASSERT(dot != string::npos && dot > 4);
      string groupname = argname.substr(4, dot);
      string regname = argname.substr(dot + 1);
      group = stoi(groupname);
      reg_id = get_reg_identifier_for_regname(regname);
    } else {
      group = ETFG_EXREG_GROUP_GPRS;
      //reg_id = reg_identifier_t(argname);
      reg_id = get_reg_identifier_for_regname(argname);
    }
    transmap_entry_t const *tmap_entry = transmap_get_elem(&in_tmap, group, reg_id);
    //cout << __func__ << " " << __LINE__ << ": num_locs = " << tmap_entry->get_locs().size() << endl;
    for (const auto &i : tmap_entry->get_locs()) {
      ASSERT(arg->is_bv_sort());
      //cout << __func__ << " " << __LINE__ << ": loc[" << i << "] = " << int(tmap_entry->loc[i]) << endl;
      if (i.get_loc() == TMAP_LOC_SYMBOL) {
        expr_ref stack_gpr_expr;
        if (stack_gpr != -1) {
          ASSERT(stack_gpr >= 0 && stack_gpr < DST_NUM_GPRS);
          stack_gpr_expr = start_state.get_expr_value(start_state, reg_type_exvreg, DST_EXREG_GROUP_GPRS, stack_gpr);
          expr_ref addr = ctx->mk_bvadd(stack_gpr_expr, ctx->mk_bv_const(DWORD_LEN, (int)i.get_regnum()));
          memlabel_t ml_stack;
          memlabel_t::keyword_to_memlabel(&ml_stack, G_STACK_SYMBOL, MEMSIZE_MAX);
          ASSERT((arg->get_sort()->get_size() % BYTE_LEN) == 0);
          mem_masked = ctx->mk_store(mem_masked, ml_stack, addr, arg, arg->get_sort()->get_size() / BYTE_LEN, false);
        } else {
          symbol_id_t symbol_id = i.get_regnum();
          expr_ref symbol_expr = cs.get_expr_value(reg_type_symbol, symbol_id);
          memlabel_t ml_symbol;
          stringstream ss;
          size_t symbol_size = this->get_symbol_size_from_id(symbol_id);
          ss << G_SYMBOL_KEYWORD "." << symbol_id << "." << symbol_size << ".0";
          memlabel_t::keyword_to_memlabel(&ml_symbol, ss.str().c_str(), MEMSIZE_MAX);
          ASSERT((arg->get_sort()->get_size() % BYTE_LEN) == 0);
          mem_masked = ctx->mk_store(mem_masked, ml_symbol, symbol_expr, arg, arg->get_sort()->get_size() / BYTE_LEN, false);
        }
      } else {
        ASSERT(i.get_loc() >= TMAP_LOC_EXREG(0) && i.get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
        expr_ref const &from = start_state.get_expr_value(start_state, reg_type_exvreg, i.get_loc() - TMAP_LOC_EXREG(0), i.get_reg_id().reg_identifier_get_id());
        ASSERT(from->is_bv_sort());
        ASSERT(from->get_sort()->get_size() <= arg->get_sort()->get_size());
        expr_ref argx;
        if (from->get_sort()->get_size() < arg->get_sort()->get_size()) {
          argx = ctx->mk_bvextract(arg, from->get_sort()->get_size() - 1, 0);
        } else {
          argx = arg;
        }
        //cout << __func__ << " " << __LINE__ << ": submap[" << expr_string(from) << "] = " << expr_string(argx) << endl;
        submap[from->get_id()] = make_pair(from, argx);
        //name_submap[from->get_name()] = argx;
      }
    }
  }
  submap[mem->get_id()] = make_pair(mem, mem_masked);
  //name_submap[mem->get_name()] = mem_masked;

  shared_ptr<tfg_edge const> e = *start_edges.begin();
  auto new_e = e->substitute_variables_at_input(start_state, submap, ctx);
  this->remove_edge(e);
  this->add_edge(new_e);

  this->set_argument_regs(args);
  //cout << __func__ << " " << __LINE__ << ": after adding transmap translations, tfg=\n" << this->graph_to_string() << endl;
}

/*void
tfg::mask_input_stack_and_locals_to_zero(void)
{
  vector<memlabel_t> relevant_memlabels;
  consts_struct_t const &cs = this->get_consts_struct();
  this->graph_get_relevant_memlabels(relevant_memlabels);

  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&cs, &relevant_memlabels](pc const &p, const string& key, expr_ref e) -> expr_ref { return e->mask_input_stack_to_zero(relevant_memlabels, cs); };
  tfg_visit_exprs(f);
}*/

#if 0
void
tfg::do_preprocess(consts_struct_t const &cs)
{
  //INFO(__func__ << " " << __LINE__ << ": calling simplify_edges" << endl);
  simplify_edges(/*cs*/);
  CPP_DBG_EXEC(TFG_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": == TFG after simplify_edges1 (have not removed donotsimplify_ ops ==\n" << this->to_string(/*cs*/) << endl);
  eliminate_donotsimplify_using_solver_operators();
  simplify_edges(/*cs*/);

  CPP_DBG_EXEC(TFG_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": == TFG after simplify_edges ==\n" << this->to_string(/*cs*/) << endl);
  /*if(SPECIALIZE_EDGES)
    specialize_edges();*/
  //INFO(__func__ << " " << __LINE__ << ": calling simplify_edges again" << endl);
  simplify_edges(/*cs*/);

  //vector<string> keywords;
  //boost::copy(m_addr_ref | boost::adaptors::map_keys, std::back_inserter(keywords));

  /*for(map<string,expr_ref>::const_iterator it = m_addr_ref.begin(); it != m_addr_ref.end(); ++it) {
    keywords.push_back(it->first);
  //cout << it->first << "\n";
  }
  for (long i = 0; i < this->num_relevant_addr_refs(); i++) {
  keywords.push_back(cs.get_addr_ref(this->get_relevant_addr_ref(i)).first);
  }*/

  //INFO(__func__ << " " << __LINE__ << ": calling populate_select_expressions_per_edge" << endl);
  //populate_select_expressions_per_edge(cs, keywords);
  //INFO(__func__ << " " << __LINE__ << ": calling populate_store_expressions_per_edge" << endl);
  //populate_store_expressions_per_edge(cs, keywords);
  //INFO(__func__ << " " << __LINE__ << ": calling populate_stack_accesses" << endl);
  //populate_stack_accesses(cs);
  //INFO(__func__ << " " << __LINE__ << ": calling populate_local_accesses" << endl);
  //populate_local_accesses(cs);
  //INFO(__func__ << " " << __LINE__ << ": calling populate_mem_reads_writes" << endl);
  //populate_mem_reads_writes();
  //INFO(__func__ << " " << __LINE__ << ": calling populate_backedge_info" << endl);
  //populate_backedge_info();
  //INFO(__func__ << " " << __LINE__ << ": calling populate_all_array_accesses" << endl);
  //populate_all_array_accesses(cs);
  //INFO(__func__ << " " << __LINE__ << ": calling populate_all_used_regs" << endl);
  //populate_all_used_regs();
  //INFO(__func__ << " " << __LINE__ << ": done" << endl);
  //INFO("== TFG after do_preprocess ==\n" << this->to_string(cs) << endl);
}
#endif


/*
void
tfg::tfg_add_addrs_using_locs(void)
{
  list<shared_ptr<tfg_node const>> nodes;
  this->get_nodes(nodes);
  for (auto n : nodes) {
    pc const &p = n->get_pc();
    vector<graph_cp_location> const &locs = m_locs.at(p);
    for (auto loc : locs) {
      if (loc.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
        tfg_add_addr(p, loc.m_addr);
      }
    }
  }
}
*/

void
tfg::clear_ismemlabel_assumes()
{
  set<string> comments;
  comments.insert(PRED_ISMEMLABEL_LOC_COMMENT_PREFIX);
  comments.insert(PRED_ISMEMLABEL_ADDR_COMMENT_PREFIX);
  this->remove_assume_preds_with_comments(comments);
}

void
tfg::add_ismemlabel_assumes_for_addrs(map<pc, map<graph_loc_id_t, lr_status_ref>> const &linear_relations)
{
  NOT_IMPLEMENTED();
#if 0
  consts_struct_t const &cs = this->get_consts_struct();
  list<shared_ptr<tfg_node>> nodes;
  this->get_nodes(nodes);
  for (auto n : nodes) {
    pc const &p = n->get_pc();
    if (p.is_exit()) {
      continue;
    }
    unordered_set<predicate> assumes;
    map<addr_id_t, expr_ref> const &addrs_map = get_addrs_map(p);
    lr_map_t<pc, tfg_node, tfg_edge, predicate> const &lr_map = linear_relations.at(p);
    for (auto am : addrs_map) {
      expr_ref e = am.second;
      if (e->is_bv_sort() && e->get_sort()->get_size() == DWORD_LEN) {
        context *ctx = e->get_context();
        memlabel_t ml;
        //ml = lr_map.address_to_memlabel(e, this->get_consts_struct(), this, p);
        ml = address_to_memlabel(e, cs, this->get_addrs_map_reverse(p), lr_map.get_addr_map(), this->get_symbol_map(), this->get_locals_map());
        //memlabel_init_to_top(&ml);
        stringstream ss;
        //ss << "ismemlabel-addr." << am.first;
        ss << PRED_ISMEMLABEL_ADDR_COMMENT_PREFIX << am.first;
        //vector<memlabel_t> atomic_mls;
        //get_atomic_memlabels(ml, atomic_mls);
        //ASSERT(atomic_mls.size() > 0);
        //expr_ref ml_assume = expr_false(ctx);
        //for (auto atomic_ml : atomic_mls) {
        //ml_assume = expr_or(ml_assume, ctx->mk_ismemlabel(e, atomic_ml));
        //}
        //predicate pred(expr_true(ctx), ml_assume, expr_true(ctx), ss.str(), predicate::assume);
        //ASSERT(!memlabel_is_top(&ml));
        if (memlabel_is_top(&ml) || memlabel_is_empty(&ml)) {
          continue;
        }
        ASSERT(e->is_bv_sort() && e->get_sort()->get_size() == DWORD_LEN);
        predicate pred(precond_t(*this), ctx->mk_ismemlabel(e, ml), expr_true(ctx), ss.str(), predicate::assume);
        assumes.insert(pred);
      }
    }
    add_assumes(p, assumes);
  }
#endif
}

#if 0
void
//tfg::add_ismemlabel_assumes_for_locs(map<pc, lr_map_t<pc, tfg_node, tfg_edge, predicate>> const &linear_relations)
tfg::add_ismemlabel_assumes_for_locs(map<pc, map<graph_loc_id_t, lr_status_t>> const &linear_relations)
{
  consts_struct_t const &cs = this->get_consts_struct();
  list<shared_ptr<tfg_node>> nodes;
  this->get_nodes(nodes);
  //map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &symbol_map = this->get_symbol_map();
  graph_locals_map_t const &locals_map = this->get_locals_map();
  context *ctx = this->get_context();
  for (auto n : nodes) {
    pc const &p = n->get_pc();
    if (p.is_exit()) {
      continue;
    }
    unordered_set<predicate> assumes;
    /*map<graph_loc_id_t, graph_cp_location> const &locs = this->get_locs();
    lr_map_t<pc, tfg_node, tfg_edge, predicate> const &lr_map = linear_relations.at(p);
    for (const auto &l : locs) {
      if (!this->loc_is_live_at_pc(p, l.first)) {
        continue;
      }
      expr_ref e = this->get_loc_expr(l.first);
      if (e->is_bv_sort() && e->get_sort()->get_size() == DWORD_LEN) {
        memlabel_t ml = lr_map.get_loc_status(l.first).to_memlabel(cs, symbol_map, locals_map);
        stringstream ss;
        //ss << "ismemlabel-loc." << l.first;
        ss << PRED_ISMEMLABEL_LOC_COMMENT_PREFIX << l.first;
        if (memlabel_is_top(&ml) || memlabel_is_empty(&ml)) {
          continue;
        }
        predicate pred(precond_t(*this), ctx->mk_ismemlabel(e, ml), expr_true(ctx), ss.str(), predicate::assume, cs);
        assumes.insert(pred);
      }
    }*/
    for (size_t i = 0; i < locals_map.size(); i++) {
      memlabel_t ml = memlabel_local(i, locals_map.at(i).second);
      expr_ref local_expr = cs.get_expr_value(reg_type_local, i);
      stringstream ss;
      ss << "ismemlabel-local." << i;
      predicate pred(precond_t(*this), ctx->mk_ismemlabel(local_expr, ml), expr_true(ctx), ss.str(), predicate::assume);
      assumes.insert(pred); //these are needed for handling alias decisions in presence of local-sprel expr guesses
    }
    add_assumes(p, assumes);
  }
}
#endif



void
tfg::eliminate_donotsimplify_using_solver_operators()
{
  //transform_all_exprs(&expr_remove_donotsimplify_operators_callback, NULL);
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [ctx](pc const &p, const string& key, expr_ref e) -> expr_ref { return ctx->expr_remove_donotsimplify_using_solver_operators(e); };
  tfg_visit_exprs(f);
}

void
tfg::tfg_visit_exprs(std::function<expr_ref (pc const &, const string&, expr_ref)> f, bool opt)
{
  this->graph_visit_exprs(f, opt);

  map<string_ref, expr_ref> new_arg_regs;
  for (auto &pr : this->get_argument_regs().get_map()) {
    expr_ref e = pr.second;
    expr_ref new_e = f(pc::start(), G_TFG_INPUT_IDENTIFIER, e);
    new_arg_regs[pr.first] = new_e;
  }
  this->set_argument_regs(new_arg_regs);
}

void
tfg::tfg_visit_exprs_const(std::function<void (pc const &, const string&, expr_ref)> f, bool opt) const
{
  //autostop_timer func_timer(__func__);
  this->graph_visit_exprs_const(f, opt);
}

void
tfg::tfg_visit_exprs_const(std::function<void (string const&, expr_ref)> f, bool opt) const
{
  this->graph_visit_exprs_const(f, opt);
}

//shared_ptr<tfg>
//tfg::tfg_copy() const
//{
//  shared_ptr<tfg> ret = make_shared<tfg>(this->get_name()->get_str(), get_context());
//  ret->set_start_state(this->get_start_state());
//  list<shared_ptr<tfg_node>> nodes;
//  this->get_nodes(nodes);
//  for (auto n : nodes) {
//    shared_ptr<tfg_node> new_node = make_shared<tfg_node>(*n/*->add_function_arguments(function_arguments)*/);
//    ret->add_node(new_node);
//    predicate_set_t const &theos = this->get_assume_preds(n->get_pc());
//    ret->add_assume_preds_at_pc(n->get_pc(), theos);
//  }
//  list<shared_ptr<tfg_edge const>> edges;
//  get_edges(edges);
//  for (auto e : edges) {
//    //ASSERT(e->get_from_pc() == e->get_from()->get_pc());
//    //ASSERT(e->get_to_pc() == e->get_to()->get_pc());
//    shared_ptr<tfg_node> from_node = ret->find_node(e->get_from_pc());
//    shared_ptr<tfg_node> to_node = ret->find_node(e->get_to_pc());
//    ASSERT(from_node);
//    ASSERT(to_node);
//    state to_state = e->get_to_state();
//    //to_state = to_state.add_function_arguments(function_arguments);
//    shared_ptr<tfg_edge const> new_edge = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), to_node->get_pc(), to_state, e->get_cond(), this->get_start_state(), e->get_assumes(), e->get_te_comment()));
//    ASSERT(from_node->get_pc() == new_edge->get_from_pc());
//    ASSERT(to_node->get_pc() == new_edge->get_to_pc());
//    ret->add_edge(new_edge);
//  }
//  //ret->m_arg_regs = m_arg_regs;
//  ret->set_argument_regs(this->get_argument_regs());
//  ret->set_return_regs(this->get_return_regs());
//  ret->set_symbol_map(this->get_symbol_map());
//  ret->set_locals_map(this->get_locals_map());
//  ret->set_nextpc_map(this->get_nextpc_map());
//  //ret->m_locs = m_locs;
//  ret->set_locs(this->get_locs());
//  ret->set_assume_preds_map(get_assume_preds_map());
//  //ret->set_preds_rejection_cache_map(get_preds_rejection_cache_map());
//  return ret;
//}


/*size_t
tfg::get_num_tfg_addrs(pc const &p) const
{
  if (m_addrs.count(p) == 0) {
    return 0;
  }
  ASSERT(m_addrs.count(p) > 0);
  return m_addrs.at(p).first.size();
}*/

#if 0
  void
tfg::tfg_locs_add_location_slots(/*reg_type rt, int i, int j*/)
{
  list<pair<memlabel_t, pair<expr_ref, pair<unsigned, pair<bool,comment_t> > > > > const accs = this->get_select_store_accesses_for_reg_type(/*rt, i, j*/);

  list<pair<memlabel_t, pair<expr_ref, pair<unsigned, pair<bool,comment_t> > > > >::const_iterator iter;
  for (iter = accs.begin(); iter != accs.end(); iter++) {
    tfg_add_loc(iter->first, iter->second.first, iter->second.second.first, iter->second.second.second.first, iter->second.second.second.second);
  }
}
#endif

list<pair<memlabel_t, pair<expr_ref, pair<unsigned, bool> > > >
tfg::get_select_expressions(shared_ptr<tfg_edge const> ed)
{
  autostop_timer func_timer(__func__);
  list<pair<memlabel_t, pair<expr_ref, pair<unsigned, bool> > > > ret;
  set<state::mem_access> mas = ed->get_all_mem_accesses(string("Edge.") + ed->to_string());
  for (set<state::mem_access>::const_iterator iter = mas.begin();
       iter != mas.end();
       iter++) {
    if (!iter->is_select()) {
      continue;
    }
    //ASSERT(iter->get_expr()->get_operation_kind() == expr::OP_SELECT);
    /*if (!expr_array_matches_keyword(iter->get_mem(), keyword)) {
      continue;
    }*/
    //expr_ref e = iter->get_expr();
    //e = expr_substitute(e, find_node(ed->get_from_pc())->get_state(), find_node(pc::start())->get_state());
    //ASSERT(e->get_args().size() == 6);
    memlabel_t memlabel = iter->get_memlabel(); //e->get_args()[1]->get_memlabel_value();
    expr_ref addr = iter->get_address(); //e->get_args()[2];
    int nbytes = iter->get_nbytes(); //e->get_args()[3]->get_int_value();
    bool bigendian = iter->get_bigendian(); //e->get_args()[4]->get_bool_value();
    //comment_t comment = e->get_args()[5]->get_comment_value();
    CPP_DBG_EXEC(TFG_LOCS, cout << "Adding select expr " << expr_string(addr) << " due to edge " << ed->to_string() << endl);
    ret.push_back(make_pair(memlabel, make_pair(addr, make_pair(nbytes, bigendian))));
  }

  return ret;
}

list<pair<memlabel_t, pair<expr_ref, pair<unsigned, bool>>>>
tfg::get_store_expressions(shared_ptr<tfg_edge const> ed)
{
  autostop_timer func_timer(__func__);
  list<pair<memlabel_t, pair<expr_ref, pair<unsigned, bool> > > > ret;
  set<state::mem_access> mas = ed->get_all_mem_accesses(string("Edge.") + ed->to_string());
  for (set<state::mem_access>::const_iterator iter = mas.begin();
      iter != mas.end();
      iter++) {
    if (!iter->is_store()) {
      continue;
    }
    //ASSERT(iter->get_expr()->get_operation_kind() == expr::OP_STORE);
    /*if (!expr_array_matches_keyword(iter->get_mem(), keyword)) {
      continue;
      }*/
    //expr_ref e = iter->get_expr();
    //e = expr_substitute(e, find_node(ed->get_from_pc())->get_state(), find_node(pc::start())->get_state());
    //ASSERT(e->get_args().size() == 7);
    memlabel_t memlabel = iter->get_memlabel(); //e->get_args()[1]->get_memlabel_value();
    expr_ref addr = iter->get_address(); //e->get_args()[2];
    int nbytes = iter->get_nbytes(); //e->get_args()[4]->get_int_value();
    bool bigendian = iter->get_bigendian(); //e->get_args()[5]->get_bool_value();
    //comment_t comment = e->get_args()[6]->get_comment_value();
    CPP_DBG_EXEC(TFG_LOCS, cout << "Adding store expr " << expr_string(addr) << " due to edge " << ed->to_string() << endl);
    ret.push_back(make_pair(memlabel, make_pair(addr, make_pair(nbytes, bigendian))));
  }

  return ret;
}


/*
void
tfg::set_tfg_locs_memlabels_to_top(bool reset_function_argument_ptr_memlabels)
{
  for (map<pc, vector<graph_cp_location>, pc_comp>::iterator iter = m_locs.begin();
      iter != m_locs.end();
      iter++) {
    //pc const &p = iter->first;
    vector<graph_cp_location> &locs = iter->second;

    for (unsigned i = 0; i < locs.size(); i++) {
      graph_cp_location &loc = locs[i];
      if (loc.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
        memlabel_init_to_top(&loc.m_memlabel);
        loc.m_addr = expr_set_memlabels_to_top(loc.m_addr, reset_function_argument_ptr_memlabels);
      }
    }
  }
}

void
tfg::set_tfg_addrs_memlabels_to_top(bool reset_function_argument_ptr_memlabels)
{
  for (map<pc, pair<vector<expr_ref>, map<unsigned, int>>, pc_comp>::iterator iter = m_addrs.begin();
      iter != m_addrs.end();
      iter++) {
    //pc const &p = iter->first;
    vector<expr_ref> &addrs = iter->second.first;
    map<unsigned, int> &addrs_map = iter->second.second;
    for (unsigned i = 0; i < addrs.size(); i++) {
      expr_ref cur_addr = addrs[i];
      expr_ref new_addr = expr_set_memlabels_to_top(cur_addr, reset_function_argument_ptr_memlabels);
      if (cur_addr != new_addr) {
        addrs_map.erase(cur_addr->get_id());
        addrs[i] = new_addr;
        addrs_map[new_addr->get_id()] = i;
      }
    }
  }
}
*/

/*void
tfg::convert_function_argument_local_memlabels(consts_struct_t const &cs)
{
  autostop_timer func_timer(__func__);
  //transform_all_exprs(&expr_convert_function_argument_local_memlabels, &cs);
  consts_struct_t const *csp = &cs;
  std::function<expr_ref (pc const &, const string&, expr_ref)> f = [&csp](pc const &p, string const & key, expr_ref e) -> expr_ref { return expr_convert_function_argument_local_memlabels(e, csp); };
  tfg_visit_exprs(f);
}*/



/*bool
tfg::is_memmask_loc(graph_loc_id_t loc_id) const
{
  //ASSERT(loc_id >= 0 && loc_id < (int)this->get_num_tfg_locs());
  return (this->m_locs.at(pc::start()).at(loc_id).m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED);
}*/

bool
tfg::expr_is_array_variable(expr_ref const &e, const void *dummy)
{
  ASSERT(dummy == NULL);
  if (e->is_var() && e->is_array_sort()) {
    return true;
  }
  return false;
}

bool
tfg::expr_contains_array_variable(expr_ref const &e)
{
  expr_contains_elem visitor(e, &expr_is_array_variable, NULL, NULL, NULL);
  return visitor.satisfies();
}



/*bool
tfg::tfg_loc_contains_memlabel_top(graph_loc_id_t loc_id) const
{
  ASSERT(m_locs.count(pc::start()) > 0);
  vector<graph_cp_location> const &tfg_locs = m_locs.at(pc::start());
  ASSERT(loc_id >= 0 && loc_id < (int)tfg_locs.size());
  if (tfg_locs.at(loc_id).m_type != GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
    return false;
  }
  return memlabel_is_top(&tfg_locs.at(loc_id).m_memlabel);
}*/

void
tfg::remove_dead_edges()
{
  bool something_changed = true;
  while (something_changed) {
    something_changed = false;
    for (auto p : this->get_all_pcs()) {
      list<shared_ptr<tfg_edge const>> incoming;
      this->get_edges_incoming(p, incoming);
      //CPP_DBG_EXEC2(EQGEN, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", incoming.size() = " << incoming.size() << endl);
      if (!p.is_start() && incoming.size() == 0) {
        //CPP_DBG_EXEC2(EQGEN, cout << __func__ << " " << __LINE__ << ": removing node " << p.to_string() << endl);
        this->remove_node_and_make_composite_edges(p);
        something_changed = true;
      }
    }
  }
}

void
tfg::tfg_substitute_variables(map<expr_id_t, pair<expr_ref, expr_ref>> const &map, bool opt)
{
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&map, ctx](pc const &p, const string& key, expr_ref e) -> expr_ref {
    expr_ref ret = ctx->expr_substitute(e, map);
    //cout << __func__ << " " << __LINE__ << ": returning " << expr_string(ret) << " for " << expr_string(e) << endl;
    return ret;
  };
  //cout << __func__ << " " << __LINE__ << ": calling tfg_visit_exprs" << endl;
  tfg_visit_exprs(f, opt);
}

/*void
tfg::tighten_memlabels_using_switchmemlabel_and_simplify_until_fixpoint(consts_struct_t const &cs)
{
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &, const string&, expr_ref)> f = [&cs, this, ctx](pc const &from_pc, const string& key, expr_ref e) -> expr_ref
  {
    expr_ref new_e = e;
    do {
      e = new_e;
      //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
      //expr_ref e1 = e->tighten_memlabels_using_switchmemlabel();
      expr_ref e1 = e;
      //cout << __func__ << " " << __LINE__ << ": e1 = " << expr_string(e1) << endl;
      expr_ref e2 = ctx->expr_do_simplify(e1);
      expr_ref e3 = e2; //ctx->expr_substitute_rodata_accesses(e2, *this, cs);
      new_e = ctx->expr_do_simplify(e3);
    } while (e != new_e);
    return new_e;
  };
  tfg_visit_exprs(f);
  //this->substitute_rodata_reads();
}*/

bool
expr_is_local_keyword(expr_ref const &e, const void *dummy)
{
  ASSERT(dummy == NULL);
  string name = expr_string(e);
  if (name.substr(0, g_local_keyword.size()) == g_local_keyword) {
    return true;
  }
  return false;
}

bool
tfg::expr_contains_local_keyword_expr(expr_ref const &e)
{
  expr_contains_elem visitor(e, &expr_is_local_keyword, NULL, NULL, NULL);
  return visitor.satisfies();
}

//#define PATH_MAX_NUM_EDGES_TO_SUBST 32

/*
expr_ref
tfg::substitute_notstack_locs_to_zero(expr_ref addr, pc const &from_pc, state const &from_state, consts_struct_t const &cs, map<pc_exprid_pair, graph_loc_id_t, pc_exprid_pair_comp> &expr_represents_loc_in_state_cache) const
{
  expr_vector ev;
  context *ctx;
  long offset;

  ctx = addr->get_context();

  ev = get_arithmetic_addsub_atoms(addr);

  offset = 0;
  expr_ref new_addr;// = ctx->mk_bv_const(DWORD_LEN, 0);
  for (size_t i = 0; i < ev.size(); i++) {
    long loc_id;
    //ASSERT(ev[i]->is_bv_sort());
    if (ev[i]->is_bv_sort() && ev[i]->is_const() && ev[i]->get_sort()->get_size() <= DWORD_LEN) {
      //ASSERT(ev[i]->get_sort()->get_size() <= DWORD_LEN);
      offset += ev[i]->get_int_value();
      continue;
    }
    //CPP_DBG_EXEC(TMP, cout << "looking at addsub_atom: " << expr_string(ev[i]) << endl);
    loc_id = expr_represents_loc_in_state(ev[i], from_pc, from_state, cs, expr_represents_loc_in_state_cache);
    //CPP_DBG_EXEC(TMP, cout << "loc_id = " << loc_id << endl);
    expr_ref e, e_tmp;
    if (   loc_id != -1
        && ev[i]->is_bv_sort()
        //&& !lr_map.loc_is_linearly_related_to_input_stack_pointer(loc_id, cs)
       ) {
      e = ctx->mk_zerobv(ev[i]->get_sort()->get_size());
    } else {
      e = ev[i];
    }
    if (!e->is_var() && !e->is_const()) {
      expr_vector new_args;
      ASSERT(e->get_args().size() > 0);
      for (size_t a = 0; a < e->get_args().size(); a++) {
        expr_ref new_e;
        new_e = this->substitute_notstack_locs_to_zero(e->get_args()[a], from_pc, from_state, cs, expr_represents_loc_in_state_cache);
        new_args.push_back(new_e);
      }
      e = ctx->mk_app(e->get_operation_kind(), new_args);
    }
    if (new_addr.get_ptr()) {
      new_addr = ctx->mk_bvadd(new_addr, e);
    } else {
      new_addr = e;
    }
  }
  if (offset) {
    if (new_addr.get_ptr()) {
      ASSERT(new_addr->is_bv_sort());
      new_addr = ctx->mk_bvadd(new_addr, ctx->mk_bv_const(new_addr->get_sort()->get_size(), offset));
      ASSERT(new_addr->get_sort() == addr->get_sort());
    } else {
      new_addr = ctx->mk_bv_const(addr->get_sort()->get_size(), offset);
      ASSERT(new_addr->get_sort() == addr->get_sort());
    }
  } else if (!new_addr.get_ptr()) {
    ASSERT(addr->is_bv_sort());
    ASSERT(addr->get_sort()->get_size() <= DWORD_LEN);
    new_addr = ctx->mk_bv_const(addr->get_sort()->get_size(), offset);
    ASSERT(new_addr->get_sort() == addr->get_sort());
  }
  ASSERT(new_addr.get_ptr());
  //CPP_DBG_EXEC(TMP, cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << ", new_addr = " << expr_string(new_addr) << endl);
  if (!new_addr->is_const() && !new_addr->is_var() && new_addr != addr) {
    new_addr = new_addr->simplify(&cs);
  }
  return new_addr;
}*/

void
tfg::append_tfg(tfg const &new_tfg/*, bool src*/, ::pc const &cur_pc, context *ctx)
{
  //#define MAX_NODES_PER_INSN 10
  tfg *tfg = this;
  //int inum;

  //DEBUG(__func__ << ": entry tfg:\n" << tfg->to_string() << endl);
  //long start_pc = cur_pc.get_index()/* * MAX_NODES_PER_INSN*/;
  //shared_ptr<tfg_node const> start_node = new_tfg->find_node(pc::start());
  //ASSERT(start_node);
  //state start_state(this->get_start_state()/*start_node->get_state()*/);

  list<shared_ptr<tfg_edge const>> cur_edges;
  tfg->get_edges_incoming(pc::start(), cur_edges);
  //DEBUG("cur_edges.size() = " << cur_edges.size() << endl);
  for (auto e : cur_edges) {
    //cout << __func__ << " " << __LINE__ << ": setting to_pc for edge " << e->to_string(true) << endl;
    //pc new_pc = pc(pc::insn_label, cur_pc.get_index(), cur_pc.get_subindex(), cur_pc.get_subsubindex());
    //state new_pc_state = start_state;
    //new_pc_state.append_pc_string_to_state_elems(new_pc, src);
    tfg->edge_set_to_pc(e, cur_pc/*, new_pc_state*/);
  }
  ASSERT(cur_pc.get_type() == pc::insn_label);


  //inum = 0;

  list<shared_ptr<tfg_edge const>> edges;
  new_tfg.get_edges(edges);
  for (auto e : edges) {
    pc from_pc = e->get_from_pc();
    pc to_pc = e->get_to_pc();

    pc new_from_pc(from_pc);
    pc new_to_pc(to_pc);

    ASSERT(!new_from_pc.is_exit());
    ASSERT(!strcmp(new_from_pc.get_index(), "0"));
    new_from_pc.set_index(cur_pc.get_index(), new_from_pc.get_subindex() + PC_SUBINDEX_FIRST_INSN_IN_BB, new_from_pc.get_subsubindex());

    if (   !new_to_pc.is_exit()
        && !new_to_pc.is_start()) {
      if (!strcmp(new_to_pc.get_index(), "0")) {
        new_to_pc.set_index(cur_pc.get_index(), new_to_pc.get_subindex() + PC_SUBINDEX_FIRST_INSN_IN_BB, new_to_pc.get_subsubindex());
      } else {
        new_to_pc.set_index(new_to_pc.get_index(), new_to_pc.get_subindex() + PC_SUBINDEX_FIRST_INSN_IN_BB, new_to_pc.get_subsubindex());
      }
    }
    append_tfg_add_node_pc(new_from_pc/*, start_state, src*/);
    append_tfg_add_node_pc(new_to_pc/*, start_state, src*/);

    //DEBUG("e->get_to_state()->to_string():\n" << e->get_to_state()->to_string() << endl);
    state edge_to_state(e->get_to_state());
    //edge_to_state.append_pc_string_to_state_elems(new_from_pc, src);
    expr_ref new_pred;
    //new_pred = expr_append_pc_string(e->get_cond(), src, new_from_pc);
    new_pred = e->get_cond();
    unordered_set<expr_ref> new_assumes = e->get_assumes();
    shared_ptr<tfg_node> new_from_node = find_node(new_from_pc);
    shared_ptr<tfg_node> new_to_node = find_node(new_to_pc);
    shared_ptr<tfg_edge const> new_e = mk_tfg_edge(mk_itfg_edge(new_from_node->get_pc(), new_to_node->get_pc(), edge_to_state, new_pred/*, this->get_start_state()*/, new_assumes, e->get_te_comment()));
    //DEBUG("new_e->get_to_state()->to_string():\n" << new_e->get_to_state()->to_string() << endl);
    add_edge(new_e);
  }
}

void
tfg::append_tfg_add_node_pc(pc const &new_pc/*, state const &start_state, bool src*/)
{
  //state node_state(start_state);
  //node_state.append_pc_string_to_state_elems(new_pc, src);
  CPP_DBG_EXEC(SYM_EXEC, cout << __func__ << " " << __LINE__ << ": tfg = " << this->graph_to_string(/*true*/) << endl);
  CPP_DBG_EXEC(SYM_EXEC, cout << __func__ << " " << __LINE__ << ": new_pc = " << new_pc.to_string() << ", find_node(new_pc) = " << !!find_node(new_pc) << endl);
  if (find_node(new_pc) == 0) {
    shared_ptr<tfg_node> new_node = make_shared<tfg_node>(new_pc/*, node_state*/);
    CPP_DBG_EXEC(SYM_EXEC, cout << __func__ << " " << __LINE__ << ": adding node at " << new_pc.to_string() << ":\n" << new_node->to_string());
    add_node(new_node);
  }
}

/*bool
tfg::contains_llvm_return_register(pc const &p, size_t &return_register_size) const
{
  shared_ptr<tfg_node const> n = this->find_node(p);
  bool ret = n->get_state().contains(G_LLVM_RETURN_REGISTER_NAME);
  if (ret) {
    expr_ref retreg = n->get_state().get_expr(G_LLVM_RETURN_REGISTER_NAME);
    ASSERT(retreg->is_bv_sort());
    return_register_size = retreg->get_sort()->get_size();
  }
  return ret;
}*/

map<string_ref, expr_ref>
tfg::get_start_state_mem_pools_except_stack_locals_rodata_and_memlocs_as_return_values() const
{
  consts_struct_t const &cs = this->get_consts_struct();
  map<string_ref, expr_ref> ret;

  expr_ref mem;
  bool bret;
  bret = this->get_start_state().get_memory_expr(this->get_start_state(), mem);
  ASSERT(bret);
  context *ctx = mem->get_context();

  CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": m_symbol_map.get_map().size() = " << this->get_symbol_map().get_map().size() << endl);

  for (auto s : this->get_symbol_map().get_map()) {
    //size_t k = this->get_relevant_addr_ref(i);
    //string addr_ref_string = cs.get_addr_ref(k).first;
    //string prefix = string(G_SYMBOL_KEYWORD) + ".";

    //cout << __func__ << " " << __LINE__ << ": addr_ref_string = " << addr_ref_string << endl;
    symbol_id_t symbol_id = s.first;//stoi(addr_ref_string.substr(prefix.length()));
    CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": symbol_id = " << symbol_id << endl);
    if (this->symbol_represents_memloc(symbol_id)) {
      continue;
    }
    size_t symbol_size = this->get_symbol_size_from_id(symbol_id);
    variable_constness_t symbol_is_const = this->get_symbol_constness_from_id(symbol_id);
    if (symbol_is_const) {
      continue;
    }
    stringstream ss;
    ss << string(G_SYMBOL_KEYWORD) << "." << s.first << /*"." << symbol_size << */"." << symbol_is_const;
    string addr_ref_string = ss.str();
    memlabel_t ml;
    memlabel_t::keyword_to_memlabel(&ml, addr_ref_string.c_str(), symbol_size);
    //mlvarname_t mlvarname = ctx->memlabel_varname(this->get_name(), m_max_mlvarnum);
    //m_max_mlvarnum++;
    CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": addr_ref_string = " << addr_ref_string << ": ml = " << memlabel_t::memlabel_to_string(&ml, as1, sizeof as1) << endl);

    //ASSERT(ml.m_type == MEMLABEL_MEM);
    //ASSERT(ml.m_num_alias == 1);
    if (symbol_size <= MAX_SCALAR_SIZE_IN_BYTES) {
      if (symbol_size > 0) {
        ASSERT(memlabel_t::memlabel_is_atomic(&ml));
        //ret[addr_ref_string] = ctx->mk_select(ctx->mk_memmask(mem, ml), ml, cs.get_expr_value(reg_type_symbol, s.first), symbol_size, false);
        ret[mk_string_ref(addr_ref_string)] = ctx->mk_select(mem, ml, cs.get_expr_value(reg_type_symbol, s.first), symbol_size, false);
        //ret.push_back(ctx->mk_select(mem, mlvarname, cs.get_expr_value(reg_type_symbol, s.first), symbol_size, false));
      }
    } else {
      ASSERT(memlabel_t::memlabel_is_atomic(&ml));
      ret[mk_string_ref(addr_ref_string)] = ctx->mk_memmask(mem, ml);
    }
  }
  memlabel_t ml;
  memlabel_t::keyword_to_memlabel(&ml, G_HEAP_KEYWORD, MEMSIZE_MAX);
  ret[mk_string_ref(G_HEAP_KEYWORD)] = ctx->mk_memmask(mem, ml);

  return ret;
}



static void
transmap_mapping_add(map<unsigned, pair<expr_ref, expr_ref> > &out, state const &from_state, uint8_t loc1, int regnum1, uint8_t loc2, int regnum2)
{
  if (loc1 == TMAP_LOC_NONE || loc2 == TMAP_LOC_NONE) {
    return;
  }
  if (loc1 == TMAP_LOC_SYMBOL || loc2 == TMAP_LOC_SYMBOL) {
    NOT_IMPLEMENTED();
  }
  ASSERT(loc1 >= TMAP_LOC_EXREG(0) && loc1 < TMAP_LOC_EXREG(MAX_NUM_EXREG_GROUPS));
  ASSERT(loc2 >= TMAP_LOC_EXREG(0) && loc2 < TMAP_LOC_EXREG(MAX_NUM_EXREG_GROUPS));
  ASSERT(loc1 == loc2);
  int group = loc1 - TMAP_LOC_EXREG(0);
  ASSERT(group >= 0 && group < MAX_NUM_EXREG_GROUPS);
  ASSERT(regnum1 >= 0 && regnum1 < MAX_NUM_EXREGS(group));
  ASSERT(regnum2 >= 0 && regnum2 < MAX_NUM_EXREGS(group));

  //cout << __func__ << " " << __LINE__ << ": from_state:\n" << from_state->to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": group = " << group << ", regnum1 = " << regnum1 << ", regnum2 = " << regnum2 << endl;
  expr_ref from = from_state.get_expr_value(from_state, reg_type_exvreg, group, regnum1);
  expr_ref to = from_state.get_expr_value(from_state, reg_type_exvreg, group, regnum2);
  out[from->get_id()] = make_pair(from, to);
}

static void
transmap_get_inv_submap(map<unsigned, pair<expr_ref, expr_ref> > &out, transmap_t const *tmap, state const &from_state)
{
  //cout << __func__ << " " << __LINE__ << ": from_state:\n" << from_state->to_string() << endl;
  for (int i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    for (int j = 0; j < MAX_NUM_EXREGS(i); j++) {
      transmap_entry_t const *e;
      //e = &tmap->extable[i][j];
      e = transmap_get_elem(tmap, i, j);
      for (const auto &k : e->get_locs()) {
        transmap_mapping_add(out, from_state, k.get_loc(), k.get_regnum(), TMAP_LOC_EXREG(i), j);
      }
    }
  }
}

/*static void
tfg_edge_apply_transmap(shared_ptr<tfg_edge> e, state const &from_state, consts_struct_t const &cs, context *ctx)
{
  map<unsigned, pair<expr_ref, expr_ref> > submap;
  //transmap_get_inv_submap(submap, tmap, from_state);
  e->substitute_variables_at_input(from_state, submap, ctx);
}*/


static void
tfg_edge_apply_inv_transmap(shared_ptr<tfg_edge const> e/*, cspace::transmap_t const *tmap, state const &start_state*/)
{
  //e->substitute_variables_at_output(tmap, start_state);
}

/*void
tfg::dst_tfg_add_indir_target_translation_at_function_entry()
{
  return;
  context *ctx = this->get_context();
  expr_ref from = ctx->mk_var(G_INPUT_KEYWORD "." G_DST_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID, ctx->mk_bv_sort(DWORD_LEN));
  expr_ref to = ctx->mk_var(G_INPUT_KEYWORD "." G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID, ctx->mk_bv_sort(DWORD_LEN));
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  submap[from->get_id()] = make_pair(from, to);
  list<shared_ptr<tfg_edge>> out;
  this->get_edges_outgoing(pc::start(), out);
  ASSERT(out.size() == 1);
  shared_ptr<tfg_edge> e = *out.begin();
  state &st = e->get_to_state();
  expr_ref cond = e->get_cond();
  st.substitute_variables_using_submap(this->get_start_state(), submap);
  expr_ref cond_new = ctx->expr_substitute(cond, submap);
  e->set_cond(cond_new);
}*/

void
tfg::dst_tfg_add_stack_pointer_translation_at_function_entry(exreg_id_t stack_gpr)
{
  ASSERT(stack_gpr >= 0 && stack_gpr < DST_NUM_GPRS);
  tfg *tfg = this;
  context *ctx = tfg->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  list<shared_ptr<tfg_edge const>> out;
  tfg->get_edges_outgoing(pc::start(), out);
  ASSERT(out.size() == 1);
  shared_ptr<tfg_edge const> e = *out.begin();
  //tfg_edge_apply_transmap(e, tfg->get_start_state(), cs, ctx);

  //map<string_ref, expr_ref> name_submap;
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  expr_ref iesp = cs.get_input_stack_pointer_const_expr();
  string regname = state::reg_name(DST_EXREG_GROUP_GPRS, stack_gpr);
  state const& start_state = tfg->get_start_state();
  expr_ref from = start_state.get_expr_value(tfg->get_start_state(), reg_type_exvreg, DST_EXREG_GROUP_GPRS, stack_gpr);
  submap[from->get_id()] = make_pair(from, iesp);

  auto new_e = e->substitute_variables_at_input(start_state, submap, ctx);
  this->remove_edge(e);
  this->add_edge(new_e);
}

void
tfg::dst_tfg_add_retaddr_translation_at_function_entry(exreg_id_t stack_gpr)
{
  tfg *tfg = this;
  context *ctx = tfg->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  list<shared_ptr<tfg_edge const>> out;
  tfg->get_edges_outgoing(pc::start(), out);
  ASSERT(out.size() == 1);
  shared_ptr<tfg_edge const> e = *out.begin();

  memlabel_t ml_stack;
  memlabel_t::keyword_to_memlabel(&ml_stack, G_STACK_SYMBOL, MEMSIZE_MAX);
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  //map<string_ref, expr_ref> name_submap;
  expr_ref mem;
  state const& start_state = tfg->get_start_state();
  bool ret = start_state.get_memory_expr(tfg->get_start_state(), mem);
  ASSERT(ret);
  ASSERT(mem->is_var());
  string memname = mem->get_name()->get_str();
  ASSERT(string_has_prefix(memname, G_INPUT_KEYWORD "."));
  memname = memname.substr(strlen(G_INPUT_KEYWORD "."));
  expr_ref iesp = cs.get_input_stack_pointer_const_expr();
  expr_ref mem_masked = mem; //cs.get_input_memory_const_expr();
  expr_ref addr = iesp;
  expr_ref retaddr_const = cs.get_retaddr_const();
  mem_masked = ctx->mk_store(mem_masked, ml_stack, addr, retaddr_const, retaddr_const->get_sort()->get_size() / BYTE_LEN, false);
  submap[mem->get_id()] = make_pair(mem, mem_masked);
  //name_submap[mk_string_ref(memname)] = mem_masked;


  auto new_e = e->substitute_variables_at_input(start_state, submap, ctx);
  this->remove_edge(e);
  this->add_edge(new_e);
}

void
tfg::dst_tfg_add_callee_save_translation_at_function_entry(vector<exreg_id_t> const& callee_save_gpr)
{
  tfg *tfg = this;
  context *ctx = tfg->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  list<shared_ptr<tfg_edge const>> out;
  tfg->get_edges_outgoing(pc::start(), out);
  ASSERT(out.size() == 1);
  shared_ptr<tfg_edge const> e = *out.begin();

  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  //map<string_ref, expr_ref> name_submap;
  vector<expr_ref> const& callee_save_consts_exprs = cs.get_callee_save_consts();
  ASSERT(callee_save_gpr.size() <= callee_save_consts_exprs.size());
  state const& start_state = tfg->get_start_state();
  for (size_t i = 0; i < callee_save_gpr.size(); ++i) {
    expr_ref to = callee_save_consts_exprs[i];
    expr_ref from = start_state.get_expr_value(start_state, reg_type_exvreg, DST_EXREG_GROUP_GPRS, callee_save_gpr[i]);
    submap[from->get_id()] = make_pair(from, to);
  }

  auto new_e = e->substitute_variables_at_input(start_state, submap, ctx);
  this->remove_edge(e);
  this->add_edge(new_e);
}

void
tfg::dst_tfg_add_single_assignments_for_register(exreg_id_t const& regid)
{
  ASSERT(regid == R_ESP); // XXX new register name is defined for esp only
  size_t num = 0;
  state const& start_state = this->get_start_state();
  string regname = string(G_DST_KEYWORD ".") + state::reg_name(DST_EXREG_GROUP_GPRS, regid);
  list<tfg_edge_ref> edges = this->get_edges_ordered();
  for (auto const& edge : edges) {
    state const& st = edge->get_to_state();
    if (st.has_expr(regname)) {
      state new_st = st;
      stringstream ss;
      ss << ESP_VERSION_REGISTER_NAME_PREFIX << num++;
      string const& keyname = ss.str();
      expr_ref val = st.get_expr(regname, start_state);
      new_st.set_expr_in_map(keyname, val);

      auto new_edge = edge->tfg_edge_set_state(new_st);
      this->remove_edge(edge);
      this->add_edge(new_edge);
      // update start state as well
      //this->graph_add_key_to_start_state(keyname, val->get_sort());
    }
  }
}

/*static void
preprocess_tfg(tfg * tfg, string name, consts_struct_t const &cs)
{
  //g_esp = tfg->find_node(pc::start())->get_state()->get_esp();
  //string str = is_src ? "SRC" : "DST";
  //string str = boost::to_upper_copy<std::string>(name);
  //string str_small = name;
  //INFO("==" + str + " TFG before preprocess==\n" << tfg->to_string() << endl);
  //tfg->to_dot(str_small+ "-original.dot", false);
  //CPP_DBG(GENERAL, "%s", "Calling do_preprocess\n");
  tfg->do_preprocess(cs);
  //CPP_DBG(GENERAL, "%s", "Calling do_constprop_and_invariant_gen\n");
  //tfg->do_constprop_and_invariant_gen(cs);

  //INFO("==" + str + " TFG after preprocess==\n" << tfg->to_string() << endl);
  //tfg->to_dot(str_small+ "-processed-label.dot", true);
  //tfg->to_dot(str_small+ "-processed.dot", false);
}*/

/*void
tfg::delete_label_list(set<pc> const &labels_to_delete)
{
  NOT_IMPLEMENTED();
}*/

/*
void
tfg::tfg_set_memlabels_to_top()
{
  std::function<expr_ref (pc const &, const string&, expr_ref)> f = [](pc const &p, string const & key, expr_ref e) -> expr_ref { return expr_set_memlabels_to_top(e); };
  tfg_visit_exprs(f);
}
*/

//string
//tfg::incoming_sizes_to_string() const
//{
//  stringstream  ss;
//  list<pc> pcs;
//  for (auto const& p : this->get_all_pcs()) {
//    pcs.push_front(p);
//  }
//  for (auto const& p : pcs) {
//    list<shared_ptr<tfg_edge>> incoming;
//    this->get_edges_incoming(p, incoming);
//    ss << p.to_string() << ": incoming.size() = " << incoming.size() << endl;
//  }
//  return ss.str();
//}

void
tfg::tfg_preprocess(bool is_dst_tfg, list<string> const& sorted_bbl_indices, map<graph_loc_id_t, graph_cp_location> const &llvm_locs)
{
  tfg *tfg = this;
  autostop_timer func_timer(__func__);
  autostop_timer func_name_timer(string(__func__) + "." + tfg->get_name()->get_str());

  tfg->graph_populate_relevant_addr_refs();

  //tfg->to_dot(name + ".dot", false);
  stats::get().get_timer("total-preprocess")->start();

  tfg->replace_exit_fcalls_with_exit_edge();

  DYN_DEBUG2(eqgen, cout << __func__ << " " << __LINE__ << ": before add_extra_nodes_after_fcalls: TFG:\n" << tfg->graph_to_string() << endl);

  tfg->add_extra_nodes_around_fcalls();
  tfg->add_extra_nodes_at_basic_block_entries();

  DYN_DEBUG2(eqgen, cout << __func__ << " " << __LINE__ << ": after add_extra_nodes_after_fcalls: TFG:\n" << tfg->graph_to_string() << endl);

  tfg->remove_dead_edges();

  DYN_DEBUG2(eqgen, cout << __func__ << " " << __LINE__ << ": after remove_dead_edges: TFG:\n" << tfg->graph_to_string() << endl);

  DYN_DEBUG(eqgen, cout << _FNLN_ << ": " << timestamp() << ": TFG labeling memlabels with mlvars.\n");
  tfg->tfg_assign_mlvars_to_memlabels();

  stats::get().get_timer("total-preprocess-split_memory")->start();
  tfg->split_memory_in_graph_and_propagate_sprels_until_fixpoint(!is_dst_tfg, llvm_locs);
  stats::get().get_timer("total-preprocess-split_memory")->stop();

  DYN_DEBUG2(eqgen, cout << __func__ << " " << __LINE__ << ": After split_memory: TFG:\n" << tfg->graph_to_string() << endl);

  //tfg->to_dot(name + "1.dot", false);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": TFG:\n" << tfg->graph_to_string() << endl);
  DYN_DEBUG(eqgen,
      cout << "sorted_bbl_indices =\n";
      for (auto const& bbl_index : sorted_bbl_indices) {
         cout << bbl_index << endl;
      }
  );

  stats::get().get_timer("total-preprocess")->stop();

  tfg->sort_edges(tfg::gen_pc_cmpF(sorted_bbl_indices));

  tfg->tfg_add_global_assumes_for_string_contents(is_dst_tfg);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": TFG:\n" << tfg->graph_to_string(/*cs*/) << endl);
  if (!is_dst_tfg) {
    tfg->tfg_eliminate_memlabel_esp();
    DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": TFG:\n" << tfg->graph_to_string(/*cs*/) << endl);
    tfg->tfg_run_copy_propagation();
    DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": after copy propagation, TFG:\n" << tfg->graph_to_string() << endl);
    tfg->sort_edges(tfg::gen_pc_cmpF(sorted_bbl_indices)); //call again because copy_propagation may have changed the order of edges
  }
}

//void
//tfg::set_fcall_read_write_memlabels_to_bottom()
//{
//  context *ctx = this->get_context();
//  graph_memlabel_map_t &memlabel_map = this->get_memlabel_map_ref();
//  std::function<void (pc const &p, const string&, expr_ref)> f = [ctx, &memlabel_map](pc const &p, const string& key, expr_ref e) -> void
//  {
//    ctx->expr_set_fcall_read_write_memlabels_to_bottom(e, memlabel_map);
//  };
//  tfg_visit_exprs_const(f);
//}

void
tfg::construct_tmap_add_transmap_entry_for_stack_offset(transmap_t &in_tmap, string_ref const &argname, int cur_offset)
{
  exreg_group_id_t group = ETFG_EXREG_GROUP_GPRS;
  //reg_identifier_t regid(argname);
  reg_identifier_t regid = get_reg_identifier_for_regname(argname->get_str());
  transmap_entry_t tmap_entry;
  //tmap_entry.loc[0] = TMAP_LOC_SYMBOL;
  //tmap_entry.regnum[0] = cur_offset;
  //tmap_entry.num_locs = 1;
  tmap_entry.set_to_symbol_loc(TMAP_LOC_SYMBOL, cur_offset);
  in_tmap.extable[group][regid] = tmap_entry;
}

void
tfg::construct_tmap_add_transmap_entry_for_gpr(transmap_t &in_tmap, string_ref const &argname, int regnum, transmap_loc_modifier_t mod)
{
  exreg_group_id_t group = ETFG_EXREG_GROUP_GPRS;
  //reg_identifier_t regid(argname);
  reg_identifier_t regid = get_reg_identifier_for_regname(argname->get_str());
  transmap_entry_t tmap_entry;
  //tmap_entry.loc[0] = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
  //tmap_entry.regnum[0] = regnum;
  //tmap_entry.num_locs = 1;
  tmap_entry.set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), regnum, mod);
  in_tmap.extable[group][regid] = tmap_entry;
}

transmap_t
tfg::construct_out_tmap_for_exit_pc_using_function_calling_conventions(map<string_ref, expr_ref> const &return_regs, int const r_eax)
{
  transmap_t ret;
  for (const auto &rr : return_regs) {
    if (rr.first->get_str().find(G_LLVM_RETURN_REGISTER_NAME) != string::npos) {
      construct_tmap_add_transmap_entry_for_gpr(ret, rr.first, r_eax, TMAP_LOC_MODIFIER_SRC_SIZED);
    }
  }
  //cout << __func__ << " " << __LINE__ << ": returning " << transmap_to_string(&ret, as1, sizeof as1) << endl;
  return ret;
}

pair<transmap_t, map<pc, transmap_t>>
tfg::construct_transmaps_using_function_calling_conventions(graph_arg_regs_t const &argument_regs, map<pc, map<string_ref, expr_ref>> const &llvm_return_reg_map, int const r_eax)
{
  map<pc, transmap_t> out_tmaps;
  transmap_t in_tmap;

  //construct in_tmap
  int cur_offset = 0;
  for (size_t i = 0; i < argument_regs.size(); i++) {
    stringstream ss;
    ss << LLVM_METHOD_ARG_PREFIX << i;
    string_ref argname = mk_string_ref(ss.str());
    if (argument_regs.count(argname)) {
      int offset = (cur_offset + 1) * (DWORD_LEN/BYTE_LEN);
      construct_tmap_add_transmap_entry_for_stack_offset(in_tmap, argname, offset);
      expr_ref arg = argument_regs.at(argname);
      if (arg->is_bv_sort()) {
        ASSERT((arg->get_sort()->get_size() % BYTE_LEN) == 0);
        int nwords = (arg->get_sort()->get_size() + DWORD_LEN - 1)/DWORD_LEN;
        cur_offset += nwords;
      }
    }
  }

  //construct out_tmaps
  for (const auto &rr : llvm_return_reg_map) {
    out_tmaps[rr.first] = construct_out_tmap_for_exit_pc_using_function_calling_conventions(rr.second, r_eax);
  }
  return make_pair(in_tmap, out_tmaps);
}

string
parse_insn_pc_from_node_id_string(ifstream& db, int nextpc_indir, context *ctx, pc &pc_ret, bool &is_indir_exit, expr_ref &indir_tgt)
{
  string line;
  //bool more;

  if (!getline(db, line)) {
    NOT_REACHED();
  }

  if (line.find("StateNodeId_start") != string::npos) {
    pc_ret = pc(pc::insn_label, "0", PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);

    if (!getline(db, line)) {
      NOT_REACHED();
    }
    return line;
  }
  if (line.find("StateNodeId_internal") != string::npos) {
    string node_num_str = line.substr(string("StateNodeId_internal").size());
    istringstream ss(node_num_str);
    long node_num;
    ss >> node_num;
    pc_ret = pc(pc::insn_label, "0", node_num, PC_SUBSUBINDEX_DEFAULT);

    if (!getline(db, line)) {
      NOT_REACHED();
    }
    return line;
  }
  if (line.find("StateNodeId_nextpc") != string::npos) {
    string nextpc_num_str = line.substr(string("StateNodeId_nextpc").size());
    istringstream ss(nextpc_num_str);
    long nextpc_num;
    ss >> nextpc_num;
    pc_ret = pc(pc::exit, int_to_string(nextpc_num).c_str(), PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);

    if (!getline(db, line)) {
      NOT_REACHED();
    }
    return line;
  }
  if (line.find("StateNodeId_indir") != string::npos) {
    pc_ret = pc(pc::exit, int_to_string(nextpc_indir).c_str(), PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);
    line = read_expr(db, indir_tgt, ctx);
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    is_indir_exit = true;
    return line;
  }
  NOT_REACHED();
}



string
tfg::read_from_ifstream(ifstream& db, context* ctx, state const &base_state)
{
  int nextpc_indir;
  string line;
  //bool more;

  nextpc_indir = 0; //assume that nextpc0 represents indir
  if (!getline(db, line)) {
    NOT_REACHED();
  }

  predicate_set_t theos;
  while (line.find("=precondition.") != string::npos) {
    predicate_ref pred;
    line = predicate::predicate_from_stream(db/*, this->get_start_state()*/, ctx, pred);
    theos.insert(pred);
  }

  while (line.find("=edge.") != string::npos) {
    pc from_pc, to_pc;
    expr_ref pred;
    bool is_indir_exit;
    expr_ref indir_tgt;

    map<string_ref, expr_ref> to_state_value_expr_map;

    if (!getline(db, line)) {
      NOT_REACHED();
    }
    assert(line.find("=from") != string::npos);

    is_indir_exit = false;
    line = parse_insn_pc_from_node_id_string(db, nextpc_indir, ctx, from_pc, is_indir_exit, indir_tgt);
    ASSERT(!is_indir_exit); //from pc cannot be indir
    //more = getline(db, line);
    //assert(more);
    assert(line.find("=to") != string::npos);

    is_indir_exit = false;
    line = parse_insn_pc_from_node_id_string(db, nextpc_indir, ctx, to_pc, is_indir_exit, indir_tgt);
    //more = getline(db, line);
    //assert(more);
    assert(line.find("=econd") != string::npos);

    line = read_expr(db, pred, ctx);
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    assert(line.find("=etf") != string::npos);

    line = state::read_state(db, to_state_value_expr_map, ctx/*, &base_state*/);
    string k;
    if (   is_indir_exit
        //&& !to_state.key_exists(G_STATE_INDIR_TARGET_KEY_ID, to_state)
        //&& (k = to_state.has_expr_substr(LLVM_STATE_INDIR_TARGET_KEY_ID)) == ""
       ) {
      //to_state.set_expr(G_STATE_INDIR_TARGET_KEY_ID, indir_tgt); // This is a hack. Ideally all indir targets should be set already
      //to_state.set_expr(G_STATE_INDIR_TARGET_KEY_ID, to); // This is a hack. Ideally all indir targets should be set already
      //to_state.set_expr(LLVM_STATE_INDIR_TARGET_KEY_ID, indir_tgt);
      to_state_value_expr_map.insert(make_pair(mk_string_ref(LLVM_STATE_INDIR_TARGET_KEY_ID), indir_tgt));
    }
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    /*assert(   (line.find("=edge.") != string::npos)
      || (line.find("=insn") != string::npos));*/
    shared_ptr<tfg_node> from_node, to_node;

    from_node = find_node(from_pc);
    if (!from_node) {
      from_node = make_shared<tfg_node>(from_pc/*, base_state*/);
      add_node(from_node);
    }

    to_node = find_node(to_pc);
    if (!to_node) {
      to_node = make_shared<tfg_node>(to_pc/*, base_state*/);
      add_node(to_node);
    }

    state to_state(base_state);
    to_state.set_value_expr_map(to_state_value_expr_map);
    to_state.populate_reg_counts(/*&base_state*/);

    shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), to_node->get_pc(), to_state, pred/*, this->get_start_state()*/, {}, te_comment_t::te_comment_not_implemented()));
    add_edge(e);
  }

  // XXX should be added on entry edge
  this->add_assume_preds_at_pc(pc::start(), theos);

  //assert(line.find("=insn") != string::npos);
  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  return line;
}

/*void
tfg::append_insn_id_to_comments(int insn_id)
{
  list<shared_ptr<tfg_edge>> edges;
  get_edges(edges);
  for (auto e : edges) {
    //cout << __func__ << " " << __LINE__ << ": before appending insn_id " << insn_id << " to edge " << e->to_string() << ", tfg:\n" << this->to_string(true) << endl;
    e->get_to_state().append_insn_id_to_comments(insn_id);
    expr_ref pred = e->get_cond();
    expr_append_insn_id_to_comments(pred, insn_id);
    e->set_cond(pred);
    if (e->is_indir_exit()) {
      expr_ref indir_tgt = e->get_indir_tgt();
      expr_append_insn_id_to_comments(indir_tgt, insn_id);
      e->set_indir_tgt(indir_tgt);
    }
    //cout << __func__ << " " << __LINE__ << ": after appending insn_id " << insn_id << " to edge " << e->to_string() << ", tfg:\n" << this->to_string(true) << endl;
  }
}

void
tfg::set_comments_to_zero(void)
{
  list<shared_ptr<tfg_edge>> edges;
  get_edges(edges);
  for (auto e : edges) {
    e->get_to_state().set_comments_to_zero();
    expr_ref pred = e->get_cond();
    expr_set_comments_to_zero(pred);
    e->set_cond(pred);
    if (e->is_indir_exit()) {
      expr_ref indir_tgt = e->get_indir_tgt();
      expr_set_comments_to_zero(indir_tgt);
      e->set_indir_tgt(indir_tgt);
    }
  }
}*/

void
tfg::eliminate_edges_with_false_pred(void)
{
  list<shared_ptr<tfg_edge const>> edges_to_be_removed, edges;
  get_edges(edges);
  for (auto e : edges) {
    if (e->get_cond() == expr_false(e->get_cond()->get_context())) {
      //DEBUG(__func__ << " " << __LINE__ << ": removing edge " << e->to_string() << endl);
      edges_to_be_removed.push_back(e);
    }
  }
  for (auto e : edges_to_be_removed) {
    remove_edge(e);
  }
}

void
tfg::edge_rename_to_pc_using_nextpc_map(shared_ptr<tfg_edge const> e, nextpc_map_t const &nextpc_map, long cur_index, state const &start_state/*, bool src*/)
{
  shared_ptr<tfg_node> to_node = this->find_node(e->get_to_pc());
  if (to_node->get_pc() == pc::start()) {
    return;
  }

  ASSERT(to_node->get_pc().is_exit());
  int exit_num = atoi(to_node->get_pc().get_index());
  ASSERT(exit_num >= 0);
  if (exit_num < nextpc_map.num_nextpcs_used) {
    if (nextpc_map.table[exit_num].tag == tag_var) {
      pc new_pc(pc::exit, int_to_string(nextpc_map.table[exit_num].val).c_str(), PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);
      shared_ptr<tfg_node> to_node = find_node(new_pc);
      if (!to_node) {
        //state new_pc_state = start_state;
        //new_pc_state.append_pc_string_to_state_elems(new_pc, src);
        to_node = make_shared<tfg_node>(new_pc/*, new_pc_state*/);
        add_node(to_node);
      }
      ASSERT(to_node);
      edge_set_to_pc(e, new_pc/*, this->get_start_state(), to_node->get_state()*/);
    } else {
      ASSERT(nextpc_map.table[exit_num].tag == tag_const);
      if (nextpc_map.table[exit_num].val == PC_JUMPTABLE) {
        //NOT_IMPLEMENTED(); do nothing for now; ideally should identify the nextpc_indir and rename accordingly
      } else {
        pc new_pc(pc::insn_label, int_to_string(cur_index + 1 + nextpc_map.table[exit_num].val).c_str(), PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);
        shared_ptr<tfg_node> to_node = find_node(new_pc);
        if (!to_node) {
          //state new_pc_state = start_state;
          //new_pc_state.append_pc_string_to_state_elems(new_pc, src);
          to_node = make_shared<tfg_node>(new_pc/*, new_pc_state*/);
          add_node(to_node);
        }
        ASSERT(to_node);
        edge_set_to_pc(e, new_pc/*, this->get_start_state()to_node->get_state()*/);
      }
    }
  }
}

void
tfg::get_all_function_calls(set<expr_ref> &ret) const
{
  std::function<void (string const&, expr_ref)> f = [&ret](string const& key, expr_ref e) -> void
  {
    expr_vector fcalls = expr_get_function_calls(e);
    ret.insert(fcalls.begin(), fcalls.end());
  };
  tfg_visit_exprs_const(f);
}

//out: map from nextpc_id to num_args
/*void
tfg::get_nextpc_args_map(map<nextpc_id_t, pair<size_t, callee_summary_t>> &out) const
{
  set<expr_ref> fcalls;

  this->get_all_function_calls(fcalls);

  for (auto e : fcalls) {
    ASSERT(e->get_operation_kind() == expr::OP_FUNCTION_CALL);
    nextpc_id_t nextpc_id;
    size_t num_args;
    ASSERT(e->get_args().size() >= 5);
    nextpc_id = consts_struct_t::get_nextpc_id_from_expr(e->get_args()[OP_FUNCTION_CALL_ARGNUM_NEXTPC_CONST]);
    //cout << __func__ << " " << __LINE__ << ": nextpc_id = " << nextpc_id << " for expr " << expr_string(e->get_args()[2]) << endl;
    if (nextpc_id == -1) {
      continue;
    }
    num_args = e->get_args().size() - OP_FUNCTION_CALL_ARGNUM_FARG0;
    string nextpc_name = m_nextpc_map.at(nextpc_id);
    callee_summary_t csum;
    if (this->tfg_callee_summary_exists(nextpc_name)) {
      csum = this->tfg_callee_summary_get(nextpc_name);
    }
    if (out.count(nextpc_id) == 0) {
      out[nextpc_id] = make_pair(num_args, csum);
    } else {
      if (out[nextpc_id].first != num_args) {
        printf("Warning : nextpc_id = %d, out[nextpc_id] = %zu, num_args = %zu\n", nextpc_id, out[nextpc_id].first, num_args);
      }
      out[nextpc_id] = make_pair(std::min(out[nextpc_id].first, num_args), csum);
      //ASSERT(out[nextpc_id] == num_args);
    }
  }
}*/

//out: nextpc_args_map from nextpc_id to num_args
void
tfg::convert_jmp_nextpcs_to_callret(int r_esp, int r_eax)
{
  list<shared_ptr<tfg_edge const>> edges_to_remove, edges_to_add, edges;
  consts_struct_t const &cs = this->get_consts_struct();
  get_edges(edges);
  for (auto e : edges) {
    shared_ptr<tfg_edge const> new_e = e->convert_jmp_nextpcs_to_callret(cs/*, nextpc_args_map*/, r_esp, r_eax, this);
    if (!new_e) {
      continue;
    }
    edges_to_remove.push_back(e);
    edges_to_add.push_back(new_e);
  }
  for (auto e : edges_to_remove) {
    remove_edge(e);
  }
  for (auto new_e : edges_to_add) {
    pc to_pc = new_e->get_to_pc();
    shared_ptr<tfg_node> to_node = find_node(to_pc);
    if (!to_node) {
      NOT_IMPLEMENTED();
      /*
      string prefix;
      if(to_pc.is_start())
        prefix = G_INPUT_KEYWORD;
      else if (src)
        prefix = "src" + to_pc.to_string();
      else
        prefix = "dst" + to_pc.to_string();

      state st(prefix, src ? se.src_get_base_state() : se.dst_get_base_state(), ctx);
      shared_ptr<tfg_node> new_node = make_shared<tfg_node>(to_pc, st);
      add_node(new_node);
      */
    }
    ASSERT(find_node(to_pc));
    add_edge(new_e);
  }
  list<shared_ptr<tfg_node>> nodes_to_remove, nodes;
  this->get_nodes(nodes);
  for (auto n : nodes) {
    list<shared_ptr<tfg_edge const>> incoming, outgoing;
    pc const &p = n->get_pc();
    get_edges_incoming(p, incoming);
    get_edges_outgoing(p, outgoing);
    if (incoming.size() == 0 && outgoing.size() == 0) {
      nodes_to_remove.push_back(n);
    }
  }
  for (auto n : nodes_to_remove) {
    remove_node(n);
  }
}

/*void
tfg::transform_function_arguments_to_avoid_stack_pointers(list<string> const &local_refs, consts_struct_t const &cs)
{
  autostop_timer func_timer(__func__);
  //transform_all_exprs(&expr_transform_function_arguments_to_avoid_stack_pointers, &cs);
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&local_refs, &cs](pc const &p, const string& key, expr_ref e) -> expr_ref { return e->expr_transform_function_arguments_to_avoid_stack_pointers(local_refs, cs); };
  tfg_visit_exprs(f);
}
*/

void
tfg::dfs_find_loop_heads_and_tails(pc const &p, map<pc, dfs_color> &visited, set<pc> &loop_heads, set<pc>& loop_tails) const
{
  if(visited.count(p) == 0) {
    visited[p] = dfs_color_white;
  }

  switch(visited.at(p)) {
    case dfs_color_black:
      break;
    case dfs_color_gray:
      break;
    case dfs_color_white:
      visited[p] = dfs_color_gray;
      list<shared_ptr<tfg_edge const>> out_edges;
      get_edges_outgoing(p, out_edges);

      for (auto out_edge : out_edges) {
        pc p_next = out_edge->get_to_pc();

        if(visited.count(p_next) > 0 && visited.at(p_next) == dfs_color_gray) {
          loop_heads.insert(p_next);
          loop_tails.insert(p);
        }

        dfs_find_loop_heads_and_tails(p_next, visited, loop_heads, loop_tails);
      }
      visited[p] = dfs_color_black;
      break;
  }
}

bool
tfg::find_fcall_pc_along_maximal_linear_path_from_loop_head(pc const& loop_head, set<pc>& fcall_heads) const
{
  pc pp = loop_head;
  list<shared_ptr<tfg_edge const>> out_edges;
  list<shared_ptr<tfg_edge const>> in_edges;
  // first pc is loop head, do not check incoming edges here
  this->get_edges_outgoing(pp, out_edges);
  if (out_edges.size() != 1)
    return false;

  do {
    pp = out_edges.front()->get_to_pc();
    if (this->is_fcall_pc(pp)) {
      fcall_heads.insert(pp);
      return true;
    }
    out_edges.clear(); in_edges.clear();
    this->get_edges_outgoing(pp, out_edges);
    this->get_edges_incoming(pp, in_edges);
  } while (out_edges.size() == 1 && in_edges.size() == 1);
  return false;
}

bool
tfg::find_fcall_pc_along_maximal_linear_path_to_loop_tail(pc const& loop_tail, set<pc>& fcall_heads) const
{
  pc pp = loop_tail;
  list<shared_ptr<tfg_edge const>> out_edges;
  list<shared_ptr<tfg_edge const>> in_edges;
  // first pc is loop tail, do not check outgoing edges here
  this->get_edges_incoming(pp, in_edges);
  if (in_edges.size() != 1)
    return false;
  do {
    pp = in_edges.front()->get_from_pc();
    if (this->is_fcall_pc(pp)) {
      fcall_heads.insert(pp);
      return true;
    }
    out_edges.clear(); in_edges.clear();
    this->get_edges_outgoing(pp, out_edges);
    this->get_edges_incoming(pp, in_edges);
  } while (out_edges.size() == 1 && in_edges.size() == 1);
  return false;
}


void
tfg::find_loop_anchors_collapsing_into_unconditional_fcalls(set<pc> &loop_pcs) const
{
  set<pc> fcall_heads;
  if (this->get_context()->get_config().anchor_loop_tail) {
    this->find_loop_tails(loop_pcs);
    for (auto itr = loop_pcs.begin(); itr != loop_pcs.end(); ) {
      if (this->find_fcall_pc_along_maximal_linear_path_to_loop_tail(*itr, fcall_heads)) {
        itr = loop_pcs.erase(itr);
      } else { ++itr; }
    }
  }
  else {
    // use loop heads as anchor points
    this->find_loop_heads(loop_pcs);
    for (auto itr = loop_pcs.begin(); itr != loop_pcs.end(); ) {
      if (this->find_fcall_pc_along_maximal_linear_path_from_loop_head(*itr, fcall_heads)) {
        itr = loop_pcs.erase(itr);
      } else { ++itr; }
    }
  }
  loop_pcs.insert(fcall_heads.begin(), fcall_heads.end());
}

void
tfg::get_path_enumeration_stop_pcs(set<pc> &stop_pcs) const
{
  get_anchor_pcs(stop_pcs);
}

//In this implementation, DST_TO_PC_SUBSUBINDEX, DST_TO_PC_INCIDENT_FCALL_NEXTPC is unused; however, this is a virtual function that is overridden in tfg_ssa_t
void
tfg::get_path_enumeration_reachable_pcs(pc const &from_pc, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, set<pc> &reachable_pcs) const
{
  get_reachable_pcs_stopping_at_fcalls(from_pc, reachable_pcs);
  set<pc> anchor_pcs;
  get_anchor_pcs(anchor_pcs);
  set_intersect(reachable_pcs, anchor_pcs); //this implementation is only invoked for DST TFG, so return only anchor PCs
}

void
tfg::get_anchor_pcs(set<pc>& anchor_pcs) const
{
  this->find_loop_anchors_collapsing_into_unconditional_fcalls(anchor_pcs);
  for (auto const& pp : this->get_all_pcs()) {
    if (pp.is_exit() || is_fcall_pc(pp)) {
      anchor_pcs.insert(pp);
    }
  }
}

void
tfg::find_loop_tails(set<pc>& loop_tails) const
{
  map<pc, dfs_color> visited;
  set<pc> unused;
  dfs_find_loop_heads_and_tails(pc::start(), visited, unused, loop_tails);
}

void
tfg::find_loop_heads(set<pc> &loop_heads) const
{
  map<pc, dfs_color> visited;
  set<pc> unused;
  dfs_find_loop_heads_and_tails(pc::start(), visited, loop_heads, unused);
}

void
tfg::get_rid_of_non_loop_head_labels(set<pc> &labels_to_delete) const
{
  list<shared_ptr<tfg_node>> nodes;
  set<pc> loop_heads;

  labels_to_delete.clear();
  find_loop_heads(loop_heads);
  this->get_nodes(nodes);
  for (auto n : nodes) {
    pc const &p = n->get_pc();
    if (p.is_start() || p.is_exit()) {
      continue;
    }
    if (loop_heads.count(p) == 0) {
      labels_to_delete.insert(p);
    }
  }
}

/*void
tfg::get_rid_of_redundant_labels(set<pc> &labels_to_delete) const
{
  list<shared_ptr<tfg_node>> nodes;
  this->get_nodes(nodes);

  labels_to_delete.clear();
  for (auto n : nodes) {
    list<shared_ptr<tfg_edge>> in, out;
    pc p = n->get_pc();

    if (p.is_start() || p.is_exit()) {
      continue;
    }

    get_edges_incoming(p, in);

    if (in.size() != 1) {
      continue;
    }
    shared_ptr<tfg_edge> e = *(in.begin());
    pc const &from_pc = e->get_from_pc();
    get_edges_outgoing(from_pc, out);
    ASSERT(out.size() >= 1);
    if (out.size() != 1) {
      continue;
    }

    shared_ptr<tfg_edge> e2 = *(out.begin());
    if (!(   from_pc.get_index() + 1 == e2->get_to_pc().get_index()
          && e->get_to_pc() == p)) {
      continue;
    }
    labels_to_delete.insert(p);
  }
}*/

/*
void trans_fun_graph::find_reachable_nodes(::pc p, map<::pc, bool, ::pc_comp>& visited)
{
  if(visited.count(p) > 0)
    return;

  visited[p] = true;

  list<edge*> out;
  find_outgoing_edges(p, out);
  for(list<edge*>::const_iterator iter = out.begin(); iter != out.end(); ++iter)
  {
    edge* e = *iter;
    find_reachable_nodes(e->get_to_pc(), visited);
  }
}

void tfg::remove_unreachable_nodes()
{
  map<pc, bool, pc_comp> visited;
  find_reachable_nodes(pc::start(), visited);

  list<node*> ns;
  for(map<::pc, node*, ::pc_comp>::const_iterator iter = m_nodes.begin(); iter != m_nodes.end(); ++iter)
  {
    pc p = iter->first;
    if(visited.count(iter->first) == 0)
    {
      ns.push_back(iter->second);
      list<edge*> out;
      find_outgoing_edges(p, out);
      list<edge*> in;
      find_incoming_edges(p, in);
      out.splice(out.end(), in);
      set<edge*> es;
      for(list<edge*>::const_iterator iter = out.begin(); iter != out.end(); ++iter)
      {
        edge* e = *iter;
        es.insert(e);
      }
      //INFO(__func__ << ": calling remove_edges" << endl);
      remove_edges(es);
    }
  }

  for(list<node*>::const_iterator iter = ns.begin(); iter!= ns.end(); ++iter)
    remove_node(*iter);
}
*/

//void
//tfg::check_consistency_of_edges()
//{
//  list<shared_ptr<tfg_edge>> edges;
//  get_edges(edges);
//  /*for (auto e : edges) {
//    ASSERT(e->get_from_pc() == e->get_from()->get_pc());
//    ASSERT(e->get_to_pc() == e->get_to()->get_pc());
//    }*/
//  list<shared_ptr<tfg_node>> nodes;
//  this->get_nodes(nodes);
//  for (auto n : nodes) {
//    graph_check_consistency_of_addrs_at_pc(n->get_pc());
//  }
//}

void
tfg::edge_set_to_pc(shared_ptr<tfg_edge const> e, pc const &new_pc/*, state const &new_state*/)
{
  this->remove_edge(e);
  shared_ptr<tfg_node> to_node = this->find_node(new_pc);
  if (!to_node) {
    to_node = make_shared<tfg_node>(new_pc);
    this->add_node(to_node);
  }
  shared_ptr<tfg_edge const> new_e = mk_tfg_edge(mk_itfg_edge(e->get_from_pc(), new_pc, e->get_to_state(), e->get_cond()/*, state()*/, e->get_assumes(), e->get_te_comment()));
  this->add_edge(new_e);
}

void
tfg::rename_llvm_symbols_to_keywords(context *ctx)
{
  graph_symbol_map_t const &symbols = this->get_symbol_map();
  map<unsigned, pair<expr_ref, expr_ref>> submap;
  for (auto s : symbols.get_map()) {
    expr_ref symbol_id, symbol_name;
    string symbol_id_str;
    stringstream ss;
    string llvm_str;

    ss << "symbol." << s.first; //symbol_num;
    symbol_id_str = ss.str();
    symbol_id = ctx->mk_var(symbol_id_str, ctx->mk_bv_sort(DWORD_LEN));
    llvm_str = string("@") + s.second.get_name()->get_str();
    symbol_name = ctx->mk_var(llvm_str, ctx->mk_bv_sort(DWORD_LEN));

    submap[symbol_name->get_id()] = make_pair(symbol_name, symbol_id);
  }
  this->tfg_substitute_variables(submap);
}

/*static bool
output_exists_in_llvm_regs(vector<expr_ref> const &llvm_regs, consts_struct_t const &cs, expr_ref const &e, expr_ref &llvm_expr)
{
  string e_str = expr_string(e);
  context *ctx = e->get_context();
  if (   e_str.find("reg.0.0") != string::npos
      && e_str.find("reg.0.3") != string::npos) {
    return false;
  }
  if (e_str.find("reg.0.5") != string::npos) {
    for (size_t i = 0; i < llvm_regs.size(); i++) {
      if (expr_string(llvm_regs.at(i)).find(G_LLVM_RETURN_REGISTER_NAME) != string::npos) {
        if (llvm_regs.at(i)->is_bv_sort()) {
          if (llvm_regs.at(i)->get_sort()->get_size() <= DWORD_LEN) {
            llvm_expr = llvm_regs.at(i);
          } else {
            llvm_expr = ctx->mk_bvextract(llvm_regs.at(i), DWORD_LEN - 1, 0);
          }
          return true;
        } else {
          return false;
        }
        NOT_REACHED();
      }
    }
  }
  if (e_str.find("reg.0.6") != string::npos) {
    for (size_t i = 0; i < llvm_regs.size(); i++) {
      if (expr_string(llvm_regs.at(i)).find(G_LLVM_RETURN_REGISTER_NAME) != string::npos) {
        if (llvm_regs.at(i)->is_bv_sort()) {
          if (llvm_regs.at(i)->get_sort()->get_size() <= DWORD_LEN) {
            return false;
          } else {
            llvm_expr = ctx->mk_bvextract(llvm_regs.at(i), llvm_regs.at(i)->get_sort()->get_size() - 1, DWORD_LEN);
            return true;
          }
          NOT_REACHED();
        } else {
          return false;
        }
        NOT_REACHED();
      }
    }
  }
  if (e_str.find(LLVM_STATE_INDIR_TARGET_KEY_ID) != string::npos) {
    for (size_t i = 0; i < llvm_regs.size(); i++) {
      if (expr_string(llvm_regs.at(i)).find(LLVM_STATE_INDIR_TARGET_KEY_ID) != string::npos) {
        llvm_expr = llvm_regs.at(i);
        return true;
      }
    }
  }
  if (COMPARE_IO_ELEMENTS) {
    if (e_str.find(".io") != string::npos) {
      for (size_t i = 0; i < llvm_regs.size(); i++) {
        if (expr_string(llvm_regs.at(i)).find(".io") != string::npos) {
          llvm_expr = llvm_regs.at(i);
          return true;
        }
      }
    }
  }
  if (e->get_operation_kind() != expr::OP_MEMMASK && e->get_operation_kind() != expr::OP_SELECT) {
    return false;
  }
  if (e->get_operation_kind() == expr::OP_MEMMASK) {
    memlabel_t e_ml = e->get_args()[1]->get_memlabel_value();
    for (size_t i = 0; i < llvm_regs.size(); i++) {
      expr_ref l = llvm_regs.at(i);
      if (l->get_operation_kind() == e->get_operation_kind()) {
        memlabel_t l_ml = l->get_args()[1]->get_memlabel_value();
        if (memlabels_equal(&e_ml, &l_ml)) {
          llvm_expr = llvm_regs.at(i);
          return true;
        }
      }
    }
  } else if (e->get_operation_kind() == expr::OP_SELECT) {
    expr_ref addr = e->get_args().at(2);
    for (size_t i = 0; i < llvm_regs.size(); i++) {
      expr_ref l = llvm_regs.at(i);
      if (l->get_operation_kind() == e->get_operation_kind()) {
        expr_ref llvm_addr = l->get_args().at(2);
        if (addr == llvm_addr) {
          llvm_expr = llvm_regs.at(i);
          return true;
        }
      }
    }
  } else NOT_REACHED();
  return false;
}*/

#if 0
void
tfg::add_exit_return_value_ptr_if_needed(tfg const &tfg_llvm)
{
  context *ctx = this->get_context();
  list<pc> exit_pcs;
  tfg_llvm.get_exit_pcs(exit_pcs);

  for (auto exit_pc : exit_pcs) {
    shared_ptr<tfg_node> exit_node = find_node(exit_pc);
    if (exit_node == NULL) {
      continue;
    }
    //cout << __func__ << " " << __LINE__ << ": exit_pc = " << exit_pc.to_string() << endl;
    //expr_ref mem = exit_node->get_state().get_memory_expr();
    expr_ref mem;
    bool ret;
    ret = this->get_start_state().get_memory_expr(this->get_start_state(), mem);
    ASSERT(ret);
    memlabel_t ml_top;
    keyword_to_memlabel(&ml_top, G_MEMLABEL_TOP_SYMBOL, MEMSIZE_MAX);
    expr_vector const &return_regs = tfg_llvm.get_return_regs(exit_pc);
    for (auto r : return_regs) {
      if (   r->is_bv_sort()
          && expr_string(r).find(G_LLVM_RETURN_REGISTER_NAME) != string::npos) {
        size_t return_register_size = r->get_sort()->get_size();
        /*if (return_register_size < 0 || return_register_size > DWORD_LEN) {
          cout << __func__ << " " << __LINE__ << ": return_register_size = " << return_register_size << endl;
          }
          ASSERT(return_register_size >= 0 && return_register_size <= QWORD_LEN);*/
        if (return_register_size == DWORD_LEN) {
          list<shared_ptr<tfg_edge>> incoming;
          //cout << __func__ << " " << __LINE__ << ": exit_pc = " << exit_pc.to_string() << endl;
          get_edges_incoming(exit_pc, incoming);
          for (auto e : incoming) {
            //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << endl;
            expr_ref to_r5;
            to_r5 = e->get_to_state().get_expr("reg.0.5", this->get_start_state());
            if (to_r5->get_operation_kind() != expr::OP_RETURN_VALUE_PTR) {
              expr_ref to_mem;
              bool ret;
              ret = e->get_to_state().get_memory_expr(this->get_start_state(), to_mem);
              ASSERT(ret);
              ASSERT(to_mem);
              e->get_to_state().set_expr("reg.0.5", ctx->mk_return_value_ptr(to_r5, ml_top, to_mem));
            }
          }
        }
      }
    }
  }
}
#endif

expr_ref
tfg::dst_get_return_reg_expr_using_out_tmap(context *ctx, state const &start_state, transmap_t const &out_tmap, string const &retreg_name, size_t size, int stack_gpr)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  //cout << __func__ << " " << __LINE__ << ": retreg_name = " << retreg_name << endl;
  if (retreg_name == G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID) {
    return ctx->mk_var(G_INPUT_KEYWORD "." G_DST_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID, ctx->mk_bv_sort(size));
  }
  exreg_group_id_t group = DST_EXREG_GROUP_GPRS;
  //reg_identifier_t regid(retreg_name);
  reg_identifier_t regid = get_reg_identifier_for_regname(retreg_name);
  if (retreg_name.substr(0, 4) == "exvr") {
    size_t dot = retreg_name.find('.');
    ASSERT(dot != string::npos);
    group = stoi(retreg_name.substr(4, dot));
    regid = get_reg_identifier_for_regname(retreg_name.substr(dot + 1));
  }
  transmap_entry_t const *tmap_entry = transmap_get_elem(&out_tmap, group, regid);
  if (tmap_entry->get_locs().size() == 0) {
    cout << __func__ << " " << __LINE__ << ": num_locs is zero in out_tmap for group " << group << ", regid " << regid.reg_identifier_to_string() << endl;
    cout << __func__ << " " << __LINE__ << ": out_tmap =\n" << transmap_to_string(&out_tmap, as1, sizeof as1) << endl;
  }
  ASSERT(tmap_entry->get_locs().size() >= 1);
  if (tmap_entry->get_locs().size() > 1) {
    NOT_IMPLEMENTED(); //may want to return a vector of exprs in this case
  }
  if (tmap_entry->get_locs().at(0).get_loc() == TMAP_LOC_SYMBOL) {
    expr_ref mem;
    bool success = start_state.get_memory_expr(start_state, mem);
    ASSERT(success);
    expr_ref data;
    if (stack_gpr != -1) {
      expr_ref addr = ctx->mk_bv_const(DWORD_LEN, (int)tmap_entry->get_locs().at(0).get_regnum());
      expr_ref stack_reg = start_state.get_expr_value(start_state, reg_type_exvreg, DST_EXREG_GROUP_GPRS, stack_gpr);
      addr = ctx->mk_bvadd(addr, stack_reg);
      data = ctx->mk_select(mem, memlabel_t::memlabel_top(), addr, DWORD_LEN/BYTE_LEN, false);
    } else {
      symbol_id_t symbol_id = (int)tmap_entry->get_locs().at(0).get_regnum();
      expr_ref addr = cs.get_expr_value(reg_type_symbol, symbol_id);
      data = ctx->mk_select(mem, memlabel_t::memlabel_top(), addr, DWORD_LEN/BYTE_LEN, false);
    }
    if (size > DWORD_LEN) {
      return ctx->mk_bvzero_ext(data, size - DWORD_LEN);
    } else if (size == DWORD_LEN) {
      return data;
    } else {
      return ctx->mk_bvextract(data, size - 1, 0);
    }
  } else {
    if (!(tmap_entry->get_locs().at(0).get_loc() >= TMAP_LOC_EXREG(0) && tmap_entry->get_locs().at(0).get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS))) {
      cout << __func__ << " " << __LINE__ << ": tmap_entry->loc[0] = " << int(tmap_entry->get_locs().at(0).get_loc()) << endl;
    }
    ASSERT(tmap_entry->get_locs().at(0).get_loc() >= TMAP_LOC_EXREG(0) && tmap_entry->get_locs().at(0).get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
    expr_ref e = start_state.get_expr_value(start_state, reg_type_exvreg, tmap_entry->get_locs().at(0).get_loc() - TMAP_LOC_EXREG(0), tmap_entry->get_locs().at(0).get_reg_id().reg_identifier_get_id());
    //ASSERT(e->get_sort()->get_size() == DWORD_LEN);
    size_t esize = e->get_sort()->get_size();
    if (size > esize) {
      expr_ref ret = ctx->mk_bvzero_ext(e, size - esize);
      ASSERT(ret->get_sort()->get_size() == size);
      return ret;
      //NOT_IMPLEMENTED();
    } else if (size == esize) {
      return e;
    } else if (size < esize) {
      return ctx->mk_bvextract(e, size - 1, 0);
    } else NOT_REACHED();
  }
  /*
     expr_ref r5 = this->get_start_state().get_expr("reg.0.5", this->get_start_state());
     expr_ref r6 = this->get_start_state().get_expr("reg.0.6", this->get_start_state());
     vector<expr_ref> ret;
     if (size == QWORD_LEN) {
     ret.push_back(r5);
     ret.push_back(r6);
     return ret;
     } else if (size == DWORD_LEN) {
     ret.push_back(r5);
     return ret;
     } else if (size < DWORD_LEN) {
     ret.push_back(ctx->mk_bvextract(r5, size - 1, 0));
     return ret;
     } else {
     cout << __func__ << " " << __LINE__ << ": size = " << size << endl;
     NOT_IMPLEMENTED();
     }
     */
}

/*void
tfg::reconcile_outputs_with_llvm_tfg(tfg &tfg_llvm)
{
  map<pc, vector<expr_ref>> new_dst_return_regs_map, new_llvm_return_regs_map;
  consts_struct_t const &cs = this->get_consts_struct();
  list<shared_ptr<tfg_node>> nodes;

  this->get_nodes(nodes);

  for (auto n : nodes) {
    pc const &p = n->get_pc();
    if (!p.is_exit()) {
      continue;
    }
    vector<expr_ref> const &dst_return_regs = this->get_return_regs(p);
    vector<expr_ref> const &llvm_return_regs = tfg_llvm.get_return_regs(p);
    vector<expr_ref> new_dst_return_regs, new_llvm_return_regs;

    for (auto e : dst_return_regs) {
      expr_ref llvm_expr;
      if (output_exists_in_llvm_regs(llvm_return_regs, cs, e, llvm_expr)) {
        //cout << __func__ << " " << __LINE__ << ": return reg. llvm = " << expr_string(llvm_expr) << ", e = " << expr_string(e) << endl;
        new_dst_return_regs.push_back(e);
        new_llvm_return_regs.push_back(llvm_expr);
      }
    }
    new_dst_return_regs_map[p] = new_dst_return_regs;
    new_llvm_return_regs_map[p] = new_llvm_return_regs;
  }

  this->set_return_regs(new_dst_return_regs_map);
  tfg_llvm.set_return_regs(new_llvm_return_regs_map);
}*/

/*void
tfg::replace_llvm_memoutput_with_memmasks(tfg const &tfg_src)
{
  map<pc, vector<expr_ref>> new_return_regs_map;
  list<shared_ptr<tfg_node const>> nodes;
  this->get_nodes(nodes);

  for (auto n : nodes) {
    pc const &p = n->get_pc();
    if (!p.is_exit()) {
      continue;
    }

    vector<expr_ref> const &return_regs = this->get_return_regs(p);
    vector<expr_ref> new_return_regs;
    for (auto r : return_regs) {
      if (r->is_array_sort()) {
        //int num_symbols = tfg_src.get_num_symbols();
        context *ctx = r->get_context();
        memlabel_t ml_heap;
        expr_ref masked;
        for (auto symbol : m_symbol_map) {
          //expr_ref symbol_addr;
          memlabel_t ml_symbol;
          string symbol_name;
          stringstream ss;
          ss << g_symbol_keyword << "." << symbol.first;
          symbol_name = ss.str();
          keyword_to_memlabel(&ml_symbol, symbol_name.c_str(), symbol.second.second);
          //ml_symbol.m_size = symbol.second.second;
          masked = ctx->mk_memmask(r, ml_symbol);
          new_return_regs.push_back(masked);
        }
        keyword_to_memlabel(&ml_heap, G_HEAP_KEYWORD, MEMSIZE_MAX);
        masked = ctx->mk_memmask(r, ml_heap);
        new_return_regs.push_back(masked);
      } else {
        new_return_regs.push_back(r);
      }
    }
    new_return_regs_map[p] = new_return_regs;
  }
  this->set_return_regs(new_return_regs_map);
}*/

/*void
tfg::canonicalize_llvm_symbols(graph_symbol_map_t const &symbol_map)
{
  autostop_timer func_timer(__func__);
  context *ctx = this->get_context();

  std::function<expr_ref (pc const &p, const string&, expr_ref)> f =
    [&symbol_map, ctx]
    (pc const &p, const string& key, expr_ref e) -> expr_ref
    {
      return ctx->expr_canonicalize_llvm_symbols(e, symbol_map);
    };
  tfg_visit_exprs(f);
  this->set_symbol_map(symbol_map);
}*/

symbol_id_t
tfg::get_symbol_id_from_name(string const &name) const
{
  for (const auto &s : this->get_symbol_map().get_map()) {
    if (s.second.get_name()->get_str() == name) {
      return s.first;
    }
  }
  return SYMBOL_ID_INVALID;
}

/*void
tfg::set_string_contents(map<symbol_id_t, string> const &string_contents)
{
  for (const auto &s : string_contents) {
    symbol_id_t sid = get_symbol_id_from_name(s.first.substr(1));
    ASSERT(sid != SYMBOL_ID_INVALID);
    m_string_contents[sid] = s.second;
  }
}*/

size_t
tfg::get_num_symbols(void) const
{
  return this->get_symbol_map().size();
}

void
tfg::canonicalize_llvm_nextpcs()
{
  autostop_timer func_timer(__func__);
  map<nextpc_id_t, string> nextpc_map;
  context *ctx = this->get_context();

  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&nextpc_map, ctx](pc const &p, const string& key, expr_ref e) -> expr_ref { return ctx->expr_canonicalize_llvm_nextpcs(e, nextpc_map); };
  tfg_visit_exprs(f);
  this->set_nextpc_map(nextpc_map);

  /*list<shared_ptr<tfg_edge>> edges;
    vector<string> nextpc_map;
    get_edges(edges);
    for (auto e : edges) {
    e->get_to_state().state_canonicalize_llvm_nextpcs(nextpc_map);
    e->set_cond(e->get_cond()->expr_canonicalize_llvm_nextpcs(nextpc_map));
    }*/
}

static bool
check_rodata_str(string const &a, string const &b)
{
  if (   (a.substr(0, 7) == ".rodata")
      && (b.substr(0, 4) == ".str")) {
    return true;
  }
  if (   (a.substr(0, 6) == ".L.str")
      && (b.substr(0, 4) == ".str")) {
    return true;
  }

  return false;
}

static bool
check_prefix(string const &a, string const &b, char const *prefix, bool strip_numeric_suffix)
{
  size_t len = strlen(prefix);
  if (   a.substr(0, len) == prefix
      && (   a.substr(len) == b
        || (strip_numeric_suffix && a.substr(len, b.length()) == b))) {
    return true;
  }
  return false;
}

static bool
check_data_prefix(string const &a, string const &b, bool strip_numeric_suffix)
{
  return check_prefix(a, b, ".data.", strip_numeric_suffix);
}

static bool
check_bss_prefix(string const &a, string const &b, bool strip_numeric_suffix)
{
  return check_prefix(a, b, ".bss.", strip_numeric_suffix);
}

static bool
check_rodata_prefix(string const &a, string const &b, bool strip_numeric_suffix)
{
  return check_prefix(a, b, ".rodata.", strip_numeric_suffix);
}



static bool
check_atat_substr(string const &a, string const &b)
{
  size_t atat = a.find("@@");
  if (atat == string::npos) {
    return false;
  }
  return (a.substr(0, atat) == b);
}

static bool
check_double_underscore_prefix(string const &a, string const &b)
{
  if (   a.substr(0, 2) == "__"
      && a.substr(2) == b) {
    return true;
  }
  if (a == "tolower" && b == "__ctype_tolower_loc") {
    return true;
  }
  if (a == "toupper" && b == "__ctype_toupper_loc") {
    return true;
  }
  return false;
}

int
tfg::symbol_map_find_symbol_index(graph_symbol_map_t const &llvm_symbol_map, string symbol_name, bool strip_numeric_suffix)
{
  size_t atat, dot;
  dot = symbol_name.find('.');
  atat = symbol_name.find("@@");
  if (atat != string::npos) {
    symbol_name = symbol_name.substr(0, atat);
  } else if (dot != string::npos) {
    if (dot != 0) {
      symbol_name = symbol_name.substr(dot + 1);
    }
  }
  size_t lastdot;
  if (((lastdot = symbol_name.rfind("."))) != string::npos) {
    //cout << "lastdot = " << lastdot << endl;
    size_t remaining;
    if (isdigit(symbol_name.at(lastdot + 1))) {
      int suffix_unused = stoi(symbol_name.substr(lastdot + 1), &remaining);
      //cout << "remaining = " << remaining << endl;
      if (remaining == symbol_name.substr(lastdot + 1).length()) {
        symbol_name = symbol_name.substr(0, lastdot);
      }
    }
  }
  for (auto s : llvm_symbol_map.get_map()) {
    string sname = s.second.get_name()->get_str();
    if (strip_numeric_suffix) {
      char const *sname_c = sname.c_str();
      char const *ptr = sname_c + strlen(sname_c) - 1;
      while (*ptr >= '0' && *ptr <= '9') {
        ptr--;
      }
      size_t len = ptr - sname_c + 1;
      sname = sname.substr(0, len);
    }
    CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": checking " << symbol_name << " against " << sname << endl);
    if (sname == symbol_name) {
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": returning " << s.first << endl);
      return s.first;
    }
    if (   check_atat_substr(sname, symbol_name)
        || check_atat_substr(symbol_name, sname)) {
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": returning " << s.first << endl);
      return s.first;
    }
    if (   check_data_prefix(sname, symbol_name, strip_numeric_suffix)
        || check_data_prefix(symbol_name, sname, strip_numeric_suffix)) {
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": returning " << s.first << endl);
      return s.first;
    }
    if (   check_rodata_prefix(sname, symbol_name, strip_numeric_suffix)
        || check_rodata_prefix(symbol_name, sname, strip_numeric_suffix)) {
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": returning " << s.first << endl);
      return s.first;
    }
    if (   check_bss_prefix(sname, symbol_name, strip_numeric_suffix)
        || check_bss_prefix(symbol_name, sname, strip_numeric_suffix)) {
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": returning " << s.first << endl);
      return s.first;
    }
    if (   check_double_underscore_prefix(sname, symbol_name)
        || check_double_underscore_prefix(symbol_name, sname)) {
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": returning " << s.first << endl);
      return s.first;
    }
    if (   check_rodata_str(sname, symbol_name)
        || check_rodata_str(symbol_name, sname)) {
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": returning " << s.first << endl);
      return s.first;
    }
    CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": not matched" << endl);
    //i++;
  }
  return -1;
}

bool
tfg::nextpc_names_are_equivalent(string const &a, string const &b)
{
  if (a == b) {
    return true;
  }
  if (a == "llvm.memset.p0i8.i32" && b == "memset") {
    return true;
  }
  if (a == "llvm.memcpy.p0i8.p0i8.i32" && b == "memcpy") {
    return true;
  }
  return false;
}

int
tfg::nextpc_map_find_nextpc_index(map<nextpc_id_t, string> const &nextpc_map, string nextpc_name)
{
  for (auto s : nextpc_map) {
    //cout << __func__ << " " << __LINE__ << ": comparing with " << s << endl;
    if (nextpc_names_are_equivalent(s.second, nextpc_name)) {
      return s.first;
    }
    if (   check_double_underscore_prefix(s.second, nextpc_name)
        || check_double_underscore_prefix(nextpc_name, s.second)) {
      return s.first;
    }
  }
  return -1;
}

void
tfg::expand_calls_to_gcc_standard_functions(nextpc_function_name_map_t *map, int r_esp)
{
  for (int i = 0; i < map->num_nextpcs; i++) {
    if (function_is_gcc_standard_function(map->table[i].name)) {
      this->expand_calls_to_gcc_standard_function(i, map->table[i].name, r_esp);
      map->table[i].name[0] = '\0';
    }
  }
}

bool
tfg::function_is_gcc_standard_function(char const *name)
{
  return !strcmp(name, "__divdi3") || !strcmp(name, "__udivdi3");
}

void
tfg::expand_calls_to_gcc_standard_function(int nextpc_num, char const *function_name, int r_esp)
{
  expr_ref esp_expr = this->get_start_state().get_expr_value(this->get_start_state(), reg_type_exvreg, DST_EXREG_GROUP_GPRS, r_esp);
  //graph_memlabel_map_t &memlabel_map = this->get_memlabel_map_ref();
  string graph_name = this->get_name()->get_str();
  long &max_memlabel_varnum = this->get_max_memlabel_varnum_ref();
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [nextpc_num, function_name, &esp_expr/*, &memlabel_map*/, &graph_name, &max_memlabel_varnum](pc const &p, const string& key, expr_ref e) -> expr_ref
  {
    return expr_expand_calls_to_gcc_standard_function(e, nextpc_num, function_name, esp_expr/*, memlabel_map*/, graph_name, max_memlabel_varnum);
  };
  tfg_visit_exprs(f);
}

void
tfg::rename_recursive_calls_to_nextpcs(struct nextpc_function_name_map_t *nextpc_function_name_map)
{
  //substitute nextpc_const.0 with a new nextpc_const that represents G_SELFCALL_ID. Add
  stringstream ss1, ss2;
  int nextpc_num;

  nextpc_num = nextpc_function_name_map_add(nextpc_function_name_map, G_SELFCALL_IDENTIFIER);

  ss1 << NEXTPC_CONST_PREFIX << 0;
  ss2 << NEXTPC_CONST_PREFIX << nextpc_num;
  context *ctx = this->get_context();

  expr_ref nextpc1 = ctx->mk_var(ss1.str(), ctx->mk_bv_sort(DWORD_LEN));
  expr_ref nextpc2 = ctx->mk_var(ss2.str(), ctx->mk_bv_sort(DWORD_LEN));

  map<expr_id_t, pair<expr_ref, expr_ref>> rename_submap;
  rename_submap[nextpc1->get_id()] = make_pair(nextpc1, nextpc2);

  this->tfg_substitute_variables(rename_submap);
}

/*void
tfg::rename_nextpcs(context *ctx,
struct cspace::nextpc_function_name_map_t *nextpc_function_name_map)
{
  map<unsigned, pair<expr_ref, expr_ref>> rename_submap;
  for (size_t i = 0 ; i < m_nextpc_map.size(); i++) {
    string nextpc_name = m_nextpc_map.at(i);
    size_t idx;
    idx = nextpc_function_name_map_find_function_name(nextpc_function_name_map, nextpc_name.c_str());
    if ((int)idx == -1) {
      //cout << __func__ << " " << __LINE__ << ": nextpc_name = " << nextpc_name << endl;
      //cout << __func__ << " " << __LINE__ << ": nextpc_function_name_map = " << endl << nextpc_function_name_map_to_string(nextpc_function_name_map, as1, sizeof as1) << endl;
      idx = nextpc_function_name_map_add(nextpc_function_name_map, nextpc_name.c_str());
    }
    ASSERT((int)idx != -1);
    //cout << __func__ << " " << __LINE__ << ": " << i + 1 << " --> " << idx << endl;
    stringstream ss;
    ss << NEXTPC_CONST_PREFIX << (i + 1);
    string this_nextpc_name = ss.str();
    ss.str(std::string());
    ss << NEXTPC_CONST_PREFIX << idx;
    string other_nextpc_name = ss.str();
    expr_ref this_nextpc = ctx->mk_var(this_nextpc_name, ctx->mk_bv_sort(DWORD_LEN));
    expr_ref other_nextpc = ctx->mk_var(other_nextpc_name, ctx->mk_bv_sort(DWORD_LEN));
    rename_submap[this_nextpc->get_id()] = make_pair(this_nextpc, other_nextpc);
  }
  this->substitute_variables(rename_submap);
}*/

void
tfg::rename_nextpcs(context *ctx, map<nextpc_id_t, string> const &llvm_nextpc_map)
{
  map<unsigned, pair<expr_ref, expr_ref>> rename_submap;
  map<int, string> new_nextpc_map;
  for (auto npc : m_nextpc_map) {
    trim(npc.second);
    if (npc.second == "") { //indir branch
      continue;
    }
    int idx = nextpc_map_find_nextpc_index(llvm_nextpc_map, npc.second);
    if (idx == -1 && npc.second == G_SELFCALL_IDENTIFIER) {
      continue;
    }
    if (idx == -1) {
      cout << __func__ << " " << __LINE__ << ": " << npc.second << ": " << npc.first << " --> " << idx  << endl;
      cout << "llvm_nextpc_map:\n";
      for (const auto &lnpc : llvm_nextpc_map) {
        cout << lnpc.first << " --> " << lnpc.second << "\n";
      }
    }
    ASSERT(idx != -1);
    stringstream ss;
    ss << NEXTPC_CONST_PREFIX << npc.first;
    string this_nextpc_name = ss.str();
    ss.str(std::string());
    ss << NEXTPC_CONST_PREFIX << idx;
    string other_nextpc_name = ss.str();
    expr_ref this_nextpc = ctx->mk_var(this_nextpc_name, ctx->mk_bv_sort(DWORD_LEN));
    expr_ref other_nextpc = ctx->mk_var(other_nextpc_name, ctx->mk_bv_sort(DWORD_LEN));
    rename_submap[this_nextpc->get_id()] = make_pair(this_nextpc, other_nextpc);
    new_nextpc_map[idx] = npc.second;
  }
  this->tfg_substitute_variables(rename_submap);
  this->set_nextpc_map(new_nextpc_map);
}

void
tfg::replace_alloca_with_store_zero()
{
  autostop_timer func_timer(__func__);
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [ctx](pc const &p, const string& key, expr_ref e) -> expr_ref { return ctx->expr_replace_alloca_with_store_zero(e); };
  tfg_visit_exprs(f);
}

void
tfg::replace_alloca_with_nop()
{
  //autostop_timer func_timer(__func__);
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [ctx](pc const &p, const string& key, expr_ref e) -> expr_ref { return ctx->expr_replace_alloca_with_nop(e); };
  tfg_visit_exprs(f);
}

//void
//tfg::set_input_caller_save_registers_to_zero()
//{
//  autostop_timer func_timer(string(__func__));
//  context *ctx = this->get_context();
//  list<shared_ptr<tfg_edge const>> start_edges;
//  pc start_pc(pc::insn_label, int_to_string(0).c_str(), PC_SUBINDEX_FIRST_INSN_IN_BB, PC_SUBSUBINDEX_DEFAULT); //find_entry_node()->get_pc();
//
//  get_edges_outgoing(start_pc, start_edges);
//  //state const &start_state = this->get_start_state();
//
//  std::function<expr_ref (const string&, expr_ref)> f = [/*&start_state, */ctx](const string& key, expr_ref e) -> expr_ref { return ctx->expr_set_caller_save_registers_to_zero(e/*, start_state*/); };
//
//
//  std::function<expr_ref (pc const&, expr_ref const&)> fcond = [ctx, &start_state](pc const& p, expr_ref const& e)
//  {
//    return ctx->expr_set_caller_save_registers_to_zero(e, start_state);
//  };
//  std::function<void (pc const&, state&)> fstate = [/*&start_state, */f](pc const& p, state &s)
//  {
//    //s.substitute_variables_using_state_submap(start_state, state_submap_t(submap));
//    s.state_visit_exprs_with_start_state(f/*, start_state*/);
//  };
//
//  for (list<shared_ptr<tfg_edge const>>::const_iterator iter = start_edges.begin();
//      iter != start_edges.end();
//      iter++) {
//    shared_ptr<tfg_edge const> e = *iter;
//
//    //ASSERT(e->tfg_edge_is_atomic());
//
//    auto new_e = e->tfg_edge_update_edgecond(fcond);
//    new_e = new_e->tfg_edge_update_state(fstate);
//    this->remove_edge(e);
//    this->add_edge(new_e);
//  }
//}

/*void
tfg::get_nextpc_num_args_map(map<unsigned, pair<size_t, set<expr_ref, expr_ref_cmp_t>>> &out) const
{
  autostop_timer func_timer(__func__);
  out.clear();
  std::function<void (pc const &p, const string&, expr_ref)> f = [&out](pc const &p, const string& key, expr_ref e) -> void { expr_get_nextpc_num_args_map(e, out); };
  tfg_visit_exprs(f);
}

void
tfg::truncate_function_arguments_using_nextpc_num_args_map(map<unsigned, pair<size_t, set<expr_ref, expr_ref_cmp_t>>> const &nextpc_num_args_map)
{
  autostop_timer func_timer(__func__);
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&nextpc_num_args_map](pc const &p, const string& key, expr_ref e) -> expr_ref { return expr_truncate_function_arguments_using_nextpc_num_args_map(e, nextpc_num_args_map); };
  tfg_visit_exprs(f);
}

void
tfg::truncate_function_arguments_using_tfg(tfg const &tfg_other)
{
  map<unsigned, pair<size_t, set<expr_ref, expr_ref_cmp_t>>> nextpc_num_args_map; //nextpc_num --> num args
  tfg_other.get_nextpc_num_args_map(nextpc_num_args_map);
  this->truncate_function_arguments_using_nextpc_num_args_map(nextpc_num_args_map);
}*/

string
tfg::symbols_to_string(void)
{
  stringstream ss;
  for (auto s : this->get_symbol_map().get_map()) {
    ss << s.first << " : " << s.second.get_name()->get_str() << " : " << s.second.get_size() << " : " << s.second.get_alignment() << " : " << s.second.is_const() << endl;
  }
  return ss.str();
}

/*bool
tfg::memlabel_is_rodata(memlabel_t const &ml) const
{
  if (!memlabel_represents_symbol(&ml)) {
    //cout << __func__ << " " << __LINE__ << ": ml = " << memlabel_to_string(&ml, as1, sizeof as1) << ", ret = false" << endl;
    return false;
  }
  bool ret;
  ret = symbol_is_rodata(memlabel_get_symbol_id(&ml));
  //cout << __func__ << " " << __LINE__ << ": ml = " << memlabel_to_string(&ml, as1, sizeof as1) << ", ret = " << ret << endl;
  return ret;
}*/

/*void
tfg::add_fcall_arguments_using_nextpc_args_map(map<nextpc_id_t, pair<size_t, callee_summary_t>> const &nextpc_args_map, int r_esp)
{
  autostop_timer func_timer(__func__);
  consts_struct_t const &cs = this->get_consts_struct();
  context *ctx = this->get_context();
  state const &from_state = this->get_start_state();
  //tfg const *this_tfg = this;
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&nextpc_args_map, &r_esp, &from_state, &cs, ctx]
    (pc const &p, const string& key, expr_ref e) -> expr_ref
    {
      //shared_ptr<tfg_node const> n = this_tfg->find_node(p);
      //state const &from_state = n->get_state();
      stringstream ss;
      ss << "reg.0." << r_esp;
      expr_ref esp_expr = from_state.get_expr(ss.str(), from_state);
      return ctx->expr_add_fcall_arguments_using_nextpc_args_map(e, nextpc_args_map, esp_expr, cs);
    };
  tfg_visit_exprs(f);
}*/

/*bool
tfg::symbol_is_const(cspace::symbol_t const *sym)
{
  NOT_IMPLEMENTED();
}*/

src_ulong
tfg::get_min_offset_with_which_symbol_is_used(symbol_id_t sid) const
{
  return 0; //NOT_IMPLEMENTED()
}

void
tfg::update_symbol_rename_submap(map<expr_id_t, pair<expr_ref, expr_ref>> &rename_submap, size_t from, size_t to, src_ulong to_offset, context *ctx)
{
  stringstream ss;
  ss << g_symbol_keyword << "." << from;
  string this_symbol_name = ss.str();
  ss.str(std::string());
  ss << g_symbol_keyword << "." << to;
  string other_symbol_name = ss.str();
  expr_ref this_symbol = ctx->mk_var(this_symbol_name, ctx->mk_bv_sort(DWORD_LEN));
  expr_ref other_symbol = ctx->mk_var(other_symbol_name, ctx->mk_bv_sort(DWORD_LEN));
  if (to_offset != 0) {
    other_symbol = ctx->mk_bvadd(other_symbol, ctx->mk_bv_const(DWORD_LEN, (uint64_t)to_offset));
  }
  rename_submap[this_symbol->get_id()] = make_pair(this_symbol, other_symbol);
}

list<section_idx_t>
get_rodata_sections(input_exec_t const* in)
{
  list<section_idx_t> ret;
  for (int i = 0; i < in->num_input_sections; i++) {
    input_section_t const *isec = &in->input_sections[i];
    if (strstart(isec->name, RODATA_SECTION_NAME, nullptr)) {
      //ASSERT(isec->size <= SYMBOL_ID_DST_RODATA_SIZE);
      //return isec->addr;
      ret.push_back(i);
    }
  }
  //return (src_ulong)-1;
  return ret;
}

rodata_map_t
tfg::populate_symbol_map_using_llvm_symbols_and_exec(symbol_map_t const *symbol_map, graph_symbol_map_t const &llvm_symbol_map, map<pair<symbol_id_t, offset_t>, vector<char>> const &llvm_string_contents, input_exec_t const *in)
{
  //map<expr_id_t, pair<expr_ref, expr_ref>> rename_submap;
  map<symbol_or_section_id_t, symbol_id_t> rename_submap;
  rodata_map_t rodata_map;
  context *ctx = this->get_context();
  consts_struct_t const& cs = ctx->get_consts_struct();
  //bool rodata_symbol_added = false;
  //list<section_idx_t> dst_rodata_sections = get_rodata_sections(in);
  //CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": dst_rodata_sections.size() = " << dst_rodata_sections.size() << endl);
  //if (dst_rodata_addr == (src_ulong)-1) {
  //  return rodata_map;
  //}

  this->dst_tfg_rename_rodata_symbols_to_their_section_names_and_offsets(in, *symbol_map);
  set<symbol_or_section_id_t> referenced_symbol_or_section_ids = this->tfg_identify_used_symbols_and_sections();

  CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": symbol_map->num_symbols = " << symbol_map->num_symbols << endl);
  for (int i = 0; i < symbol_map->num_symbols; i++) {
    if (!set_belongs(referenced_symbol_or_section_ids, symbol_or_section_id_t(symbol_or_section_id_t::symbol, i))) {
      continue;
    }
    if (in->symbol_is_rodata(symbol_map->table[i])) {
      continue;
    }
    {
      int idx = symbol_map_find_symbol_index(llvm_symbol_map, symbol_map->table[i].name, false);
      src_ulong offset = 0;
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": " << symbol_map->table[i].name << ": " << i << " --> " << idx << endl);
      if (idx == -1) {
        idx = symbol_map_find_symbol_index(llvm_symbol_map, symbol_map->table[i].name, true);
      }
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": " << symbol_map->table[i].name << ": " << i << " --> " << idx << endl);
      //if (idx == -1/* && dst_rodata_sections.size() > 0*/) {
      //  /*if (!this->is_rodata_symbol_string(symbol_map->table[i].name)) {
      //    cout << __func__ << " " << __LINE__ << ": Could not find a matching symbol for dst symbol " << symbol_map->table[i].name << endl;
      //    cout << "tfg_llvm.m_symbol_map:" << endl;
      //    for (auto osymbol : tfg_llvm.m_symbol_map) {
      //    cout << "  " << get<0>(osymbol.second) << endl;
      //    }
      //    }
      //    ASSERT(this->is_rodata_symbol_string(symbol_map->table[i].name));*/
      //  //check with strings
      //  //idx = SYMBOL_ID_DST_RODATA;
      //  //offset = symaddr - dst_rodata_addr;

      //  //ASSERT(idx == -1);
      //  //for (auto sid : dst_rodata_sections) {
      //  //  ASSERT(sid >= 0 && sid < in->num_input_sections);
      //  //  if (   symaddr >= in->input_sections[sid].addr
      //  //      && symaddr < in->input_sections[sid].addr + in->input_sections[sid].size) {
      //  //    idx = sid;
      //  //    break;
      //  //  }
      //  //}
      //  //ASSERT(idx != -1);
      //  //offset = symaddr - in->input_sections[idx].addr;
      //  //CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": adding '" << symbol_map->table[i].name << "' with i = " << i << " as rodata symbol with name " << in->input_sections[idx].name << " offset " << offset << endl);
      //  //rodata_offset_t rodata_offset(in->input_sections[idx].name, offset);
      //  //expr_ref from = cs.get_expr_value(reg_type_symbol, i);
      //  //rename_submap.insert(make_pair(from->get_id(), make_pair(from, rodata_offset.get_expr(ctx))));
      //}
      if (idx != -1) {
        //ASSERT(idx != -1);
        //update_symbol_rename_submap(rename_submap, i, idx, offset, ctx);
        rename_submap.insert(make_pair(symbol_or_section_id_t(symbol_or_section_id_t::symbol, i), idx));
      } else {
        cout << __func__ << " " << __LINE__ << ": Warning: found read-write symbol '" << symbol_map->table[i].name << "' in executable that could not be matched.\n";
      }
    }
  }
  graph_symbol_map_t dst_symbol_map = llvm_symbol_map;
  this->dst_tfg_add_unrenamed_symbols_and_sections_to_symbol_map(dst_symbol_map, rename_submap, referenced_symbol_or_section_ids, in);


  for (int i = 0; i < in->num_input_sections; i++) {
    if (!set_belongs(referenced_symbol_or_section_ids, symbol_or_section_id_t(symbol_or_section_id_t::section, i))) {
      continue;
    }
    if (strstart(in->input_sections[i].name, RODATA_SECTION_NAME, nullptr)) {
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": checking '" << in->input_sections[i].name << "' with tfg_llvm.m_string_contents\n");
      //src_ulong symaddr = symbol_map->table[i].value;
      //src_ulong symsize = symbol_map->table[i].size;
      for (const auto &lstring : llvm_string_contents) {
        vector<char> const &string_contents = lstring.second;
        //cout << __func__ << " " << __LINE__ << ": string_contents = " << string_contents << endl;
        char string_contents_arr[string_contents.size()];
        for (size_t i = 0; i < string_contents.size(); i++) {
          string_contents_arr[i] = string_contents.at(i);
        }
        ASSERT(lstring.first.second == 0);
        input_exec_scan_rodata_region_and_populate_rodata_map(in, i, string_contents_arr, string_contents.size(), &rodata_map, lstring.first.first, rename_submap);
        /*if (rodata_map.count(lstring.first)) {
          cout << __func__ << " " << __LINE__ << ": Error: looks like two identical strings appear in two different rodata symbols in the executable (no dedup was done by the compiler!). This violates the assumptions we use during symbol renaming.\n";
          cout << __func__ << " " << __LINE__ << ": symbol name = " << lstring.first << endl;
          cout << __func__ << " " << __LINE__ << ": symbol contents = " << lstring.second << endl;
          cout << __func__ << " " << __LINE__ << ": previous symaddr = " << hex << rodata_map.at(lstring.first) << endl;
          cout << __func__ << " " << __LINE__ << ": new symaddr = " << hex << symaddr + offset << endl;
          }
          ASSERT(!rodata_map.count(lstring.first));
          cout << __func__ << " " << __LINE__ << ": found match for addr " << symaddr + offset << " with llvm symbol " << lstring.first << endl;*/
        //rodata_map[lstring.first] = symaddr + offset;
        //rodata_map.add_mapping(lstring.first, symaddr + offset);
      }
    }
  }

  map<expr_id_t, pair<expr_ref, expr_ref>> expr_rename_submap = symbol_or_section_id_t::get_expr_rename_submap(rename_submap, in, ctx);
  DYN_DEBUG(eqgen, cout << _FNLN_ << ": expr_rename_submap =\n" << expr_submap_to_string(expr_rename_submap) << endl);

  this->tfg_substitute_variables(expr_rename_submap);
  this->set_symbol_map(dst_symbol_map);
  return rodata_map;
}

void
tfg::dst_tfg_rename_rodata_symbols_to_their_section_names_and_offsets(input_exec_t const* in, symbol_map_t const& symbol_map)
{
  context* ctx = this->get_context();
  std::function<expr_ref (pc const&, string const&, expr_ref)> f = [in, &symbol_map, ctx](pc const& p, string const& k, expr_ref e)
  {
    return ctx->expr_rename_rodata_symbols_to_their_section_names_and_offsets(e, in, symbol_map);
  };
  tfg_visit_exprs(f);
  //cout << __func__ << " " << __LINE__ << ": this =\n";
  //this->graph_to_stream(cout);
}

set<symbol_or_section_id_t>
tfg::tfg_identify_used_symbols_and_sections() const
{
  context* ctx = this->get_context();
  set<symbol_or_section_id_t> ret;
  std::function<void (string const&, expr_ref)> f = [&ret, ctx](string const& k, expr_ref e)
  {
    set<symbol_or_section_id_t> s = ctx->expr_get_used_symbols_and_sections(e);
    set_union(ret, s);
  };
  tfg_visit_exprs_const(f);
  return ret;
}

void
tfg::dst_tfg_add_unrenamed_symbols_and_sections_to_symbol_map(graph_symbol_map_t& dst_symbol_map, map<symbol_or_section_id_t, symbol_id_t>& rename_submap, set<symbol_or_section_id_t> const& referenced_symbol_or_section_ids, input_exec_t const* in)
{
  symbol_id_t cur_symbol_id = dst_symbol_map.find_max_symbol_id() + 1;
  for (auto const& ssid : referenced_symbol_or_section_ids) {
    if (rename_submap.count(ssid)) {
      continue;
    }
    CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": adding symbol or section " << ssid.to_string() << " to dst_symbol_map" << endl);
    rename_submap.insert(make_pair(ssid, cur_symbol_id));
    graph_symbol_t gsym = in->get_graph_symbol_for_symbol_or_section_id(ssid);
    gsym.set_name(mk_string_ref(string(G_DST_SYMBOL_PREFIX) + gsym.get_name()->get_str()));
    dst_symbol_map.insert(make_pair(cur_symbol_id, gsym));
    cur_symbol_id++;
  }
}

void
tfg::populate_symbol_map_using_memlocs_in_tmap(transmap_t const *tmap)
{
  for (const auto &g : tmap->extable) {
    for (const auto &r : g.second) {
      vector<transmap_loc_t> const &locs = r.second.get_locs();
      for (const auto &loc : locs) {
        if (loc.is_symbol()) {
          symbol_id_t symbol_id = loc.get_regnum();
          stringstream ss;
          ss << PEEPTAB_MEM_PREFIX << "." << G_REGULAR_REG_NAME << g.first << "." << r.first.reg_identifier_to_string();
          string symbol_name = ss.str();
          size_t symbol_size = DIV_ROUND_UP(SRC_EXREG_LEN(g.first), BYTE_LEN);
          ASSERT(symbol_size != 0);
          ASSERT(this->get_symbol_map().count(symbol_id) == 0 || this->get_symbol_map().at(symbol_id).get_name()->get_str() == symbol_name);
          if (this->get_symbol_map().count(symbol_id) == 0) {
            this->get_symbol_map().insert(make_pair(symbol_id, graph_symbol_t(mk_string_ref(symbol_name), symbol_size, ALIGNMENT_FOR_TRANSMAP_MAPPING_TO_SYMBOL, false)));
          }
        }
      }
    }
  }
}

void
tfg::populate_symbol_map_using_memlocs_in_tmaps(transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_out_tmaps)
{
  this->populate_symbol_map_using_memlocs_in_tmap(in_tmap);
  for (size_t i = 0; i < num_out_tmaps; i++) {
    this->populate_symbol_map_using_memlocs_in_tmap(&out_tmaps[i]);
  }
}

void
tfg::populate_nextpc_map(nextpc_function_name_map_t const *nextpc_map)
{
  m_nextpc_map.clear();
  for (int i = 0; i < nextpc_map->num_nextpcs; i++) {
    m_nextpc_map[i] = nextpc_map->table[i].name;
  }
}

void
tfg::debug_check(void) const
{
  consts_struct_t const *cs = &this->get_consts_struct();
  std::function<void (string const&, expr_ref)> f = [&cs](string const& key, expr_ref e) -> void { expr_debug_check(e, cs); };
  tfg_visit_exprs_const(f);
}

//bool
//tfg::symbols_need_to_be_collapsed(string a, string b) const
//{
//  if (memlabel_t::is_rodata_symbol_string(a) && memlabel_t::is_rodata_symbol_string(b)) {
//    return true;
//  }
//  return false;
//}

/*void
tfg::collapse_rodata_symbols_into_one(void)
{
  map<size_t, size_t> symbols_to_replace;
  map<int, tuple<string, size_t, variable_constness_t>> new_symbol_map;
  for (map<symbol_id_t, tuple<string, size_t, variable_constness_t>>::const_iterator iter = m_symbol_map.begin();
      iter != m_symbol_map.end();
      iter++) {
    if (symbols_to_replace.count(iter->first) > 0) {
      continue;
    }
    map<symbol_id_t, tuple<string, size_t, variable_constness_t>>::const_iterator iter2 = iter;
    for (iter2++;
        iter2 != m_symbol_map.end();
        iter2++) {
      if (symbols_to_replace.count(iter2->first) > 0) {
        continue;
      }
      //cout << __func__ << " " << __LINE__ << ": checking " << iter->second << " and " << iter2->second << endl;
      if (symbols_need_to_be_collapsed(get<0>(iter->second), get<0>(iter2->second))) {
        symbols_to_replace[iter2->first] = iter->first;
        //cout << __func__ << " " << __LINE__ << ": replacing " << iter2->second << " " << iter2->first << " with " << iter->second << " " << iter->first << endl;
      }
    }
  }

  map<unsigned, pair<expr_ref, expr_ref>> rename_submap;
  for (auto symbol : m_symbol_map) {
    if (symbols_to_replace.count(symbol.first) > 0) {
      size_t j = symbols_to_replace.at(symbol.first);
      update_symbol_rename_submap(rename_submap, symbol.first, j, get<1>(symbol.second), RODATA_SYMBOL_SIZE, get<2>(symbol.second), true, this->get_context());
    } else {
      string symbol_name = get<0>(symbol.second);
      size_t symbol_size = get<1>(symbol.second);
      if (this->is_rodata_symbol_string(symbol_name)) {
        symbol_size = RODATA_SYMBOL_SIZE;
      }
      new_symbol_map[symbol.first] = symbol.second; //make_pair(symbol_name, symbol_size, get<2>(symbol.second));
    }
  }
  this->substitute_variables(rename_submap);
  this->set_symbol_map(new_symbol_map);
}*/

/*void
tfg::remove_fcall_side_effects(void)
{
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &, const string&, expr_ref)> f = [ctx](pc const &from_pc, const string& key, expr_ref e) -> expr_ref
  {
    return ctx->expr_remove_fcall_side_effects(e);
  };
  tfg_visit_exprs(f);
}*/

/*void
tfg::make_fcalls_semantic(void)
{
  consts_struct_t const &cs = this->get_consts_struct();
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &, const string&, expr_ref)> f = [&cs, ctx](pc const &from_pc, const string& key, expr_ref e) -> expr_ref
  {
    return ctx->expr_make_fcalls_semantic(e, cs);
  };
  tfg_visit_exprs(f);
}*/

size_t
tfg::max_expr_size(void) const
{
  size_t size = 0;
  function<void (string const&, expr_ref)> f = [&size](string const& key, expr_ref e) -> void
  {
    if (size < expr_size(e)) {
      size = expr_size(e);
    }
  };
  tfg_visit_exprs_const(f);
  return size;
}

/*bool
tfg::contains_memlabel_bottom(void) const
{
  bool ret = false;
  std::function<void (pc const &, const string&, expr_ref)> f = [&ret](pc const &from_pc, const string& key, expr_ref e) -> void
  {
    ret = ret || expr_contains_memlabel_bottom(e);
  };
  tfg_visit_exprs(f);
  return ret;
}*/

/*void
tfg::zero_out_llvm_temp_vars()
{
  list<shared_ptr<tfg_node>> nodes;
  this->get_nodes(nodes);

  for (auto n : nodes) {
    list<shared_ptr<tfg_edge>> in;
    pc const &p = n->get_pc();
    set<string> vars;

    if (!p.is_exit()) {
      continue;
    }

    //state const &exit_state = find_node(p)->get_state();
    state const &exit_state = this->get_start_state();

    exit_state.get_live_vars_from_exprs(vars, get_return_regs(p));
    get_edges_incoming(p, in);
    for (auto in_e : in) {
      in_e->get_to_state().zero_out_dead_vars(vars);
    }
  }
}*/

void
tfg::remove_function_name_from_symbols(string const &function_name)
{
  this->get_symbol_map().symbol_map_remove_function_name_from_symbols(function_name);
}

size_t
tfg::get_symbol_size_from_id(int symbol_id) const
{
  if (this->get_symbol_map().count(symbol_id) == 0) {
    cout << __func__ << " " << __LINE__ << ": symbol_id = " << symbol_id << endl;
    cout << "symbol_map:" << endl;
    for (auto symbol : this->get_symbol_map().get_map()) {
      cout << symbol.first << " --> " << symbol.second.get_name()->get_str() << ", " << symbol.second.get_size() << endl;
    }
  }
  ASSERT(this->get_symbol_map().count(symbol_id) > 0);
  return this->get_symbol_map().at(symbol_id).get_size();
}

variable_constness_t
tfg::get_symbol_constness_from_id(int symbol_id) const
{
  ASSERT(symbol_id >= 0);
  if (this->get_symbol_map().count(symbol_id) == 0) {
    cout << __func__ << " " << __LINE__ << ": symbol_id = " << symbol_id << endl;
    cout << "symbol_map:" << endl;
    for (auto symbol : this->get_symbol_map().get_map()) {
      cout << symbol.first << " --> " << symbol.second.get_name()->get_str() << ", " << symbol.second.get_size() << endl;
    }
  }
  ASSERT(this->get_symbol_map().count(symbol_id) > 0);
  return this->get_symbol_map().at(symbol_id).is_const();
}



size_t
tfg::get_local_size_from_id(int local_id) const
{
  ASSERT(this->get_locals_map().count(local_id));
  return this->get_locals_map().at(local_id).get_size();
}

void
tfg::replace_exit_fcalls_with_exit_edge()
{
  list<shared_ptr<tfg_node>> nodes;
  this->get_nodes(nodes);

  //shared_ptr<tfg_node const> start_node = find_node(pc::start());
  //state start_state(start_node->get_state());
  //state start_state(this->get_start_state());

  for (auto n : nodes) {
    list<shared_ptr<tfg_edge const>> outgoing;
    get_edges_outgoing(n->get_pc(), outgoing);
    for (auto e : outgoing) {
      int nextpc_num;
      if (e->contains_exit_fcall(this->get_nextpc_map(), &nextpc_num)) {
        pc pc_exit(pc::exit, int_to_string(nextpc_num).c_str(), PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);
        //state exit_state = start_state;
        //exit_state.append_pc_string_to_state_elems(pc_exit, name == G_LLVM_PREFIX);
        if (!this->pc_has_return_regs(pc_exit)) {
          this->set_return_regs_at_pc(pc_exit, this->get_start_state_mem_pools_except_stack_locals_rodata_and_memlocs_as_return_values());
          //this->set_return_regs(pc_exit, start_state.get_memory_exprs(start_state));
        }
        this->edge_set_to_pc(e, pc_exit/*, exit_state*/);
      }
    }
  }
}


expr_ref
slot::get_expr(/*pc const &p*/) const
{
  //return m_tfg->get_loc_expr(/*p, */m_loc_id);
  return m_tfg->graph_loc_get_value_for_node(m_loc_id);
}

string
slot::to_string(/*pc const &p*/) const
{
  return m_tfg->get_loc(/*p, */m_loc_id).to_string();
}

/*list<slot> const &
  tfg::get_slots(pc const &p) const
  {
  if (m_slots.count(p) == 0) {
  cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << endl;
  }
  ASSERT(m_slots.count(p));
  return m_slots.at(p);
  }*/

void
tfg::convert_to_locs(tfg &tfg_loc) const
{
  /*
     list<shared_ptr<tfg_node const>> nodes;
     this->get_nodes(nodes);
     for (auto n : nodes) {
     shared_ptr<tfg_node> new_node = make_shared<tfg_node>(n->add_function_arguments(function_arguments));
     tfg_loc.add_node(new_node);
     }
     list<shared_ptr<tfg_edge>> edges;
     get_edges(edges);
     for (auto e : edges) {
     ASSERT(e->get_from_pc() == e->get_from()->get_pc());
     ASSERT(e->get_to_pc() == e->get_to()->get_pc());
     shared_ptr<tfg_node const> from_node = ret->find_node(e->get_from_pc());
     shared_ptr<tfg_node const> to_node = ret->find_node(e->get_to_pc());
     ASSERT(from_node);
     ASSERT(to_node);
     state to_state = e->get_to_state();
     to_state = to_state.add_function_arguments(function_arguments);
     shared_ptr<tfg_edge> new_edge = make_shared<tfg_edge>(from_node, to_node, to_state, e->get_cond(), e->get_is_indir_exit(), e->get_indir_tgt());
     ASSERT(from_node->get_pc() == new_edge->get_from_pc());
     ASSERT(to_node->get_pc() == new_edge->get_to_pc());
     ret->add_edge(new_edge);
     }
     ret->m_arg_regs = m_arg_regs;
     ret->m_return_regs = m_return_regs;
     ret->m_symbol_map = m_symbol_map;
     ret->m_nextpc_map = m_nextpc_map;
  //ret->m_cp_assumes = m_cp_assumes;
  ret->m_locs = m_locs;
  //ret->m_slots = m_slots;
  ret->set_preds_map(get_preds_map());
  //ret->set_preds_rejection_cache_map(get_preds_rejection_cache_map());
  return ret;
  */
}

bool
tfg::pc_is_basic_block_entry(pc const &p) const
{
  list<shared_ptr<tfg_edge const>> incoming;
  get_edges_incoming(p, incoming);
  if (incoming.size() > 1) {
    return true;
  } else {
    for (const auto &e : incoming) {
      pc const &from_pc = e->get_from_pc();
      list<shared_ptr<tfg_edge const>> outgoing;
      get_edges_outgoing(from_pc, outgoing);
      if (outgoing.size() > 1) {
        return true;
      }
    }
  }
  return false;
}

void
tfg::add_extra_nodes_at_basic_block_entries()
{
  list<shared_ptr<tfg_node>> nodes;
  context *ctx = this->get_context();
  this->get_nodes(nodes);

  //state start_state(this->get_start_state());

  for (const auto &n : nodes) {
    if (   !pc_is_basic_block_entry(n->get_pc())
        || n->get_pc().get_subsubindex() != PC_SUBSUBINDEX_DEFAULT
        || !n->get_pc().is_label()) {
      continue;
    }
    //cout << "n = " << n->get_pc().to_string() << endl;
    pc pc_extra(pc::insn_label, n->get_pc().get_index()/*, n->get_pc().get_bblnum()*/, n->get_pc().get_subindex(), PC_SUBSUBINDEX_BASIC_BLOCK_ENTRY);
    ASSERT(find_node(pc_extra) == 0);
    shared_ptr<tfg_node> extra_node = make_shared<tfg_node>(pc_extra);
    this->add_node(extra_node);
    list<shared_ptr<tfg_edge const>> incoming;
    this->get_edges_incoming(n->get_pc(), incoming);
    //cout << __func__ << " " << __LINE__ << ": this->incoming =\n" << this->incoming_sizes_to_string() << endl;
    for (const auto &e : incoming) {
      shared_ptr<tfg_edge const> new_e1 = mk_tfg_edge(mk_itfg_edge(e->get_from_pc(), extra_node->get_pc(), e->get_to_state(), e->get_cond()/*, state()*/, e->get_assumes(), e->get_te_comment()));
      this->remove_edge(e);
      this->add_edge(new_e1);
    }
    //cout << __func__ << " " << __LINE__ << ": this->incoming =\n" << this->incoming_sizes_to_string() << endl;
    shared_ptr<tfg_edge const> new_e2 = mk_tfg_edge(mk_itfg_edge(extra_node->get_pc(), n->get_pc(), state(), expr_true(ctx)/*, state()*/, {}, te_comment_t::te_comment_bb_entry()));
    this->add_edge(new_e2);
    //cout << __func__ << " " << __LINE__ << ": this->incoming =\n" << this->incoming_sizes_to_string() << endl;
  }
}

void
tfg::expand_fcall_edge(shared_ptr<tfg_edge const> &e)
{
  context *ctx = this->get_context();
  shared_ptr<tfg_node> n = this->find_node(e->get_from_pc());
  ASSERT(n != nullptr);
  state const &to_state = e->get_to_state();
  ASSERT(to_state.contains_function_call());
  vector<expr_ref> args = to_state.state_get_fcall_args();
  pc pc_extra1(pc::insn_label, n->get_pc().get_index()/*, n->get_pc().get_bblnum()*/, n->get_pc().get_subindex(), PC_SUBSUBINDEX_FCALL_START);
  ASSERT(find_node(pc_extra1) == 0);
  shared_ptr<tfg_node> extra_node1 = make_shared<tfg_node>(pc_extra1);
  this->add_node(extra_node1);
  shared_ptr<tfg_edge const> new_e1 = mk_tfg_edge(mk_itfg_edge(e->get_from_pc(), extra_node1->get_pc(), state(), expr_true(ctx)/*, state()*/, {}, te_comment_t::te_comment_fcall_edge_start()));
  this->add_edge(new_e1);
  //cout << __func__ << " " << __LINE__ << ": this->incoming =\n" << this->incoming_sizes_to_string() << endl;
  shared_ptr<tfg_node> last_node = extra_node1;
  vector<expr_ref> new_args;
  size_t i = 0;
  for (const auto &arg : args) {
    state to_state;
    string new_argname;
    string argname = expr_string(arg);
    if (arg->is_const()) {
      new_argname = string(G_SRC_KEYWORD "." LLVM_FCALL_ARG_COPY_PREFIX) + int_to_string(i) + "." + argname + "." + arg->get_sort()->to_string();
      string_replace(new_argname, ":", "_");
    } else {
      if (string_has_prefix(argname, G_INPUT_KEYWORD ".")) {
        //cout << __func__ << " " << __LINE__ << ": argname = " << argname << endl;
        new_argname = string(LLVM_FCALL_ARG_COPY_PREFIX) + int_to_string(i) + "." + argname.substr(strlen(G_INPUT_KEYWORD "."));
      } else {
        new_argname = string(LLVM_FCALL_ARG_COPY_PREFIX) + int_to_string(i) + "." + argname;
      }
    }
    //this->graph_add_key_to_start_state(new_argname, arg->get_sort());
    to_state.set_expr_in_map(new_argname, arg);
    expr_ref new_arg_expr = ctx->mk_var(string(G_INPUT_KEYWORD ".") + new_argname, arg->get_sort());
    new_args.push_back(new_arg_expr);
    pc pc_extra(pc::insn_label, n->get_pc().get_index()/*, n->get_pc().get_bblnum()*/, n->get_pc().get_subindex(), PC_SUBSUBINDEX_FCALL_MIDDLE(i));
    shared_ptr<tfg_node> extra_node = make_shared<tfg_node>(pc_extra);
    this->add_node(extra_node);
    shared_ptr<tfg_edge const> new_e = mk_tfg_edge(mk_itfg_edge(last_node->get_pc(), extra_node->get_pc(), to_state, expr_true(ctx)/*, state()*/, {}, te_comment_t::te_comment_fcall_edge_arg()));
    last_node = extra_node;
    this->add_edge(new_e);
    i++;
  }
  //cout << __func__ << " " << __LINE__ << ": this->incoming =\n" << this->incoming_sizes_to_string() << endl;
  pc pc_extra2(pc::insn_label, n->get_pc().get_index()/*, n->get_pc().get_bblnum()*/, n->get_pc().get_subindex(), PC_SUBSUBINDEX_FCALL_END);
  ASSERT(find_node(pc_extra2) == 0);
  shared_ptr<tfg_node> extra_node2 = make_shared<tfg_node>(pc_extra2);
  this->add_node(extra_node2);
  pc fcall_pc = last_node->get_pc();
  state new_to_state = e->get_to_state();
  new_to_state.replace_fcall_args(args, new_args);
  shared_ptr<tfg_edge const> new_e2 = mk_tfg_edge(mk_itfg_edge(fcall_pc, extra_node2->get_pc(), new_to_state, e->get_cond()/*, state()*/, e->get_assumes(), e->get_te_comment()));
  this->add_edge(new_e2);

  shared_ptr<tfg_edge const> new_e3 = mk_tfg_edge(mk_itfg_edge(extra_node2->get_pc(), e->get_to_pc(), state(), expr_true(ctx)/*, state()*/, {}, te_comment_t::te_comment_fcall_edge_end()));
  this->add_edge(new_e3);
  this->remove_edge(e);
  this->transfer_preds(e->get_from_pc(), fcall_pc); //transfer any preds at from_pc (due to fcall) to pc_extra1
}

void
tfg::add_extra_nodes_around_fcalls()
{
  list<shared_ptr<tfg_node>> nodes;
  context *ctx = this->get_context();
  this->get_nodes(nodes);

  for (auto n : nodes) {
    list<shared_ptr<tfg_edge const>> outgoing;
    get_edges_outgoing(n->get_pc(), outgoing);
    for (auto e : outgoing) {
      //int nextpc_num;
      if (e->contains_function_call()) {
        this->expand_fcall_edge(e);
      }
    }
  }
}

void
tfg::add_extra_node_at_start_pc(pc const &pc_cur_start)
{
  context *ctx = this->get_context();

  auto n = this->find_node(pc_cur_start);
  ASSERT(n);

  if (find_node(pc::start()) == 0) {
    shared_ptr<tfg_node> extra_node = make_shared<tfg_node>(pc::start());
    this->add_node(extra_node);
  }

  list<shared_ptr<tfg_edge const>> outgoing;
  get_edges_outgoing(pc::start(), outgoing);
  if (outgoing.size() == 0) {
    shared_ptr<tfg_edge const> new_e1 = mk_tfg_edge(mk_itfg_edge(pc::start(), pc_cur_start, state(), expr_true(ctx)/*, state()*/, {}, te_comment_t::te_comment_start_pc_edge()));
    this->add_edge(new_e1);
  }
}

void
tfg::add_assumes_to_start_edge(unordered_set<expr_ref> const& arg_assumes)
{
  list<tfg_edge_ref> outgoing;
  get_edges_outgoing(pc::start(), outgoing);
  ASSERT(outgoing.size() == 1);
  tfg_edge_ref old_start_edge = *outgoing.begin();

  unordered_set<expr_ref> assumes = arg_assumes;
  unordered_set_union(assumes, old_start_edge->get_assumes());

  tfg_edge_ref new_start_edge = old_start_edge->tfg_edge_update_assumes(assumes);
  this->remove_edge(old_start_edge);
  this->add_edge(new_start_edge);
}

list<shared_ptr<tfg_edge const>>
tfg::get_edges_for_intrinsic_call(shared_ptr<tfg_edge const> const &e, nextpc_id_t nextpc_id)
{
  string const &fname = m_nextpc_map.at(nextpc_id);
  ASSERT(interpret_intrinsic_fn.count(fname));
  intrinsic_interpret_fn_t f = interpret_intrinsic_fn.at(fname);
  ASSERT(f);
  //cout << "calling function object\n";
  //fflush(stdout);
  return f(e/*, this->get_start_state()*/);
}

void
tfg::tfg_llvm_interpret_intrinsic_fcall_for_edge(shared_ptr<tfg_edge const> const &e, nextpc_id_t nextpc_id)
{
  context *ctx = this->get_context();
  //state const &start_state = this->get_start_state();
  pc const &orig_from_pc = e->get_from_pc();
  pc const &orig_to_pc = e->get_to_pc();

  list<shared_ptr<tfg_edge const>> intrinsic_edges = get_edges_for_intrinsic_call(e, nextpc_id);
  //cout << __func__ << " " << __LINE__ << ": intrinsic_edges.size() = " << intrinsic_edges.size() << "\n";

  this->remove_edge(e);

  ASSERT(intrinsic_edges.size() > 0);
  pc pc_extra = (*intrinsic_edges.begin())->get_from_pc();
  ASSERT(find_node(pc_extra) == 0);
  shared_ptr<tfg_node> extra_node = make_shared<tfg_node>(pc_extra);
  this->add_node(extra_node);
  shared_ptr<tfg_edge const> extra_edge = mk_tfg_edge(mk_itfg_edge(orig_from_pc, pc_extra, state(), expr_true(ctx)/*, state()*/, {}, te_comment_t::te_comment_intrinsic_fcall_begin()));
  this->add_edge(extra_edge);

  for (const auto &ie : intrinsic_edges) {
    pc const &pc_to = ie->get_to_pc();
    ASSERT(find_node(pc_to) == 0);
    shared_ptr<tfg_node> to_node = make_shared<tfg_node>(pc_to);
    this->add_node(to_node);
    this->add_edge(ie);
    pc_extra = pc_to;
  }

  shared_ptr<tfg_edge const> extra_edge2 = mk_tfg_edge(mk_itfg_edge(pc_extra, orig_to_pc, state(), expr_true(ctx)/*, state()*/, {}, te_comment_t::te_comment_intrinsic_fcall_end()));
  this->add_edge(extra_edge2);
}

void
tfg::tfg_llvm_interpret_intrinsic_fcalls_for_nextpc_id(nextpc_id_t nextpc_id)
{
  //cout << __func__ << " " << __LINE__ << ": nextpc_id = " << nextpc_id << "\n";
  list<shared_ptr<tfg_edge const>> fcall_edges = this->get_fcall_edges(nextpc_id);
  for (const auto &e : fcall_edges) {
    this->tfg_llvm_interpret_intrinsic_fcall_for_edge(e, nextpc_id);
  }
}

void
tfg::tfg_llvm_interpret_intrinsic_fcalls()
{
  context *ctx = this->get_context();
  //state const &start_state = this->get_start_state();

  for (const auto &n : m_nextpc_map) {
    if (llvm_function_is_intrinsic(n.second)) {
      this->tfg_llvm_interpret_intrinsic_fcalls_for_nextpc_id(n.first);
    }
  }
}

bool
tfg::llvm_function_is_intrinsic(string const &fname)
{
  return interpret_intrinsic_fn.count(fname);
}

/*void
tfg::graph_populate_relevant_addr_refs()
{
}*/

/*map<symbol_id_t, pair<string, size_t>>
  tfg::get_symbol_map_excluding_rodata() const
  {
  map<symbol_id_t, pair<string, size_t>> ret;
  for (auto sm : m_symbol_map) {
  if (!symbol_is_rodata(sm.first)) {
  ret[sm.first] = sm.second;
  }
  }
  return ret;
  }*/

/*string
  tfg::llvm_get_callee_save_regname(size_t callee_save_regnum)
  {
  stringstream ss;
  ss << LLVM_CALLEE_SAVE_REGNAME << "." << callee_save_regnum;
  return ss.str();
  }*/

void
tfg::populate_exit_return_values_using_out_tmaps(map<pc, map<string_ref, expr_ref>> const &llvm_return_reg_map, map<pc, transmap_t> const &out_tmaps, int stack_gpr)
{
  context *ctx = this->get_context();
  state const &start_state = this->get_start_state();
  for (const auto &p : llvm_return_reg_map) {
    pc const &exit_pc = p.first;
    map<string_ref, expr_ref> const &return_regs = p.second;
    map<string_ref, expr_ref> dst_return_regs = this->get_start_state_mem_pools_except_stack_locals_rodata_and_memlocs_as_return_values();
    //cout << __func__ << " " << __LINE__ << ": dst_return_regs =\n";
    //for (auto const& dret : dst_return_regs) {
    //  cout << "dret.first = " << dret.first->get_str() << ": " << expr_string(dret.second) << endl;
    //}
    if (!out_tmaps.count(exit_pc)) {
      cout << "this =\n"; this->graph_to_stream(cout); cout << endl;
      cout << __func__ << " " << __LINE__ << ": exit_pc = " << exit_pc.to_string() << endl;
    }
    ASSERT(out_tmaps.count(exit_pc));
    //cout << __func__ << " " << __LINE__ << ": exit_pc = " << exit_pc.to_string() << endl;
    transmap_t const &out_tmap = out_tmaps.at(exit_pc);
    for (const auto &rr : return_regs) {
      expr_ref const &r = rr.second;
      //cout << __func__ << " " << __LINE__ << ": rr.first = " << rr.first->get_str() << ", rr.second = " << expr_string(r) << endl;
      //cout << "r = " << expr_string(r) << endl;
      //cout << "rr.first->get_str().find(G_SYMBOL_KEYWORD) = " << rr.first->get_str().find(G_SYMBOL_KEYWORD) << endl;
      if ((r->is_bv_sort() || r->is_bool_sort()) && rr.first->get_str().find(G_SYMBOL_KEYWORD) == string::npos) { //non-symbol
        size_t return_register_size = r->is_bool_sort() ? 1 : r->get_sort()->get_size();
        ASSERT(return_register_size >= 0 && return_register_size <= QWORD_LEN);
        expr_ref dst_ret = dst_get_return_reg_expr_using_out_tmap(ctx, start_state, out_tmap, rr.first->get_str(), return_register_size, stack_gpr);
        dst_return_regs[rr.first] = dst_ret;
        /*vector<expr_ref> dst_rets = dst_return_reg_expr(get_context(), exit_pc, return_register_size);
          if (dst_rets.size() > 1) {
          NOT_IMPLEMENTED(); //do not yet know what string name to use for different regs.
          }
          for (auto dst_ret : dst_rets) {
          dst_return_regs[rr.first] = dst_ret;
          }*/
      } else if (dst_return_regs.count(rr.first) == 0) {
        dst_return_regs[rr.first] = r;
      }
    }
    set_return_regs_at_pc(exit_pc, dst_return_regs);
  }
}

//void
//tfg::add_outgoing_value(pc const &p, expr_ref outval, string const &comment)
//{
//  if (m_outgoing_values.count(p) == 0) {
//    m_outgoing_values[p] = list<pair<string, expr_ref>>();
//  }
//  for (auto ov : m_outgoing_values.at(p)) {
//    if (is_expr_equal_syntactic(ov.second, outval)) {
//      return;
//    }
//  }
//  m_outgoing_values[p].push_back(make_pair(comment, outval));
//}

//void
//tfg::add_outgoing_values(const pc& p, list<pair<string, expr_ref>> const &outgoing_values)
//{
//  m_outgoing_values[p] = outgoing_values;
//}
//
//list<pair<string, expr_ref>> const &
//tfg::get_outgoing_values(pc const &p) const
//{
//  static list<pair<string, expr_ref>> empty_list;
//  if (m_outgoing_values.count(p) == 0) {
//    return empty_list;
//  }
//  return m_outgoing_values.at(p);
//}

/*void
tfg::mask_input_stack_memory_to_zero_at_start_pc()
{
  //pc start_pc = find_entry_node()->get_pc();

  vector<memlabel_t> relevant_memlabels;
  consts_struct_t const &cs = this->get_consts_struct();
  this->graph_get_relevant_memlabels(relevant_memlabels);
  ASSERT(relevant_memlabels.size() > 0);

  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&cs, &relevant_memlabels](pc const &p, const string& key, expr_ref e) -> expr_ref { return e->mask_input_stack_to_zero(relevant_memlabels, cs); };

  this->tfg_visit_exprs(f);
}*/

void
tfg::allocate_stack_memory_at_start_pc()
{
  autostop_timer func_timer(string(__func__));
  /*vector<memlabel_t> relevant_memlabels;
    consts_struct_t const &cs = this->get_consts_struct();
    this->graph_get_relevant_memlabels(relevant_memlabels);
    ASSERT(relevant_memlabels.size() > 0);

    std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&cs, &relevant_memlabels](pc const &p, const string& key, expr_ref e) -> expr_ref { return e->mask_input_stack_to_zero(relevant_memlabels, cs); };

    this->tfg_visit_exprs(f);*/

  consts_struct_t const &cs = this->get_consts_struct();
  expr_ref input_mem;
  state const &start_state = this->get_start_state();
  bool m = start_state.get_memory_expr(start_state, input_mem);
  ASSERT(m);
  memlabel_t ml_stack = memlabel_t::memlabel_stack();
  context *ctx = input_mem->get_context();

  std::function<expr_ref (const string&, expr_ref)> f = [&cs, &input_mem, &ml_stack, &ctx](const string& key, expr_ref e) -> expr_ref {
    expr_ref new_input_mem;
    expr_ref stack_size = ctx->mk_bv_const(DWORD_LEN, MAX_STACK_SIZE);
    expr_ref start_addr = ctx->mk_bvsub(cs.get_input_stack_pointer_const_expr(), stack_size);
    new_input_mem = ctx->mk_alloca(input_mem, ml_stack, start_addr, stack_size);
    return expr_substitute(e, input_mem, new_input_mem);
  };

  list<shared_ptr<tfg_edge const>> out;
  get_edges_outgoing(pc::start(), out);
  ASSERT(out.size() == 1);
  auto e = *out.begin();

  //ASSERT(e->tfg_edge_is_atomic());

  std::function<expr_ref (pc const&, expr_ref const&)> fcond = [f](pc const&, expr_ref const& e)
  {
    return f("edgecond", e);
  };
  std::function<void (pc const&, state&)> fstate = [/*&start_state, */f](pc const&, state &s)
  {
    s.state_visit_exprs_with_start_state(f/*, start_state*/);
  };
  auto new_e = e->tfg_edge_update_state(fstate);
  new_e = new_e->tfg_edge_update_edgecond(fcond);
  this->remove_edge(e);
  this->add_edge(new_e);
}

//the following function is called from llvm2tfg
/*void
tfg::add_sprel_assumes()
{
  map<pc, sprel_map_t> const &sprels = this->get_sprels();
  //cout << __func__ << " " << __LINE__ << ": sprels.size() = " << sprels.size() << endl;
  for (auto ps : sprels) {
    pc const &p = ps.first;
    if (p.is_exit()) {
      continue;
    }
    unordered_set<predicate> preds = ps.second.get_assumes(this->get_locid2expr_map(), *this);
    for (auto pred : preds) {
      //cout << __func__ << " " << __LINE__ << ": adding pred" << endl;
      add_pred_no_check(p, pred);
    }
  }
}*/

set<local_id_t>
tfg::identify_address_taken_local_variables() const
{
  autostop_timer func_timer(string(__func__));
  context *ctx = this->get_context();
  consts_struct_t const &cs = this->get_consts_struct();
  set<local_id_t> locals;
  map<pc, sprel_map_t> const &sprels = this->get_sprels();
  //map<graph_loc_id_t, expr_ref> const &locid2expr_map = this->get_locid2expr_map();
  //set<cs_addr_ref_id_t> const &relevant_addr_refs = this->graph_get_relevant_addr_refs();
  //graph_symbol_map_t const &symbol_map = this->get_symbol_map();
  //graph_locals_map_t const &locals_map = this->get_locals_map();
  //map<string, expr_ref> const &argument_regs = this->get_argument_regs();
  graph_memlabel_map_t const &memlabel_map = this->get_memlabel_map();
  std::function<void (pc const &p, const string&, expr_ref)> f = [&locals, ctx, &cs, &sprels, &memlabel_map/*, &locid2expr_map, &relevant_addr_refs, &symbol_map, &locals_map, &argument_regs*/](pc const &p, const string& key, expr_ref e) -> void
  {
    expr_ref ee = e;
    //sprel_map_t sprel_map(cs);
    //sprel_map_t const *sprel_map = NULL;
    //if (sprels.count(p)) {
    //  sprel_map = &sprels.at(p);
    //}
    sprel_map_t const &sprel_map = sprels.at(p);
    //map<expr_ref, memlabel_t> alias_map;
    ee = ctx->expr_simplify_using_sprel_and_memlabel_maps(ee, sprel_map, memlabel_map);
    //cout << __func__ << " " << __LINE__ << ": ee = " << expr_string(ee) << endl;
    expr_identify_address_taken_local_variables(ee, locals, cs);
  };

  list<shared_ptr<tfg_edge const>> edges;
  this->get_edges(edges);
  for (auto e : edges) {
    pc const &p = e->get_from_pc();
    f(p, G_EDGE_COND_IDENTIFIER, e->get_cond());
    std::function<void (const string &, expr_ref)> sf = [&f, &p](const string &key, expr_ref e) -> void { f(p, key, e); };
    e->get_to_state().state_visit_exprs_with_start_state(sf/*, this->get_start_state()*/);
  }
  return locals;
}

set<expr_ref>
tfg::identify_stack_pointer_relative_expressions() const
{
  //autostop_timer func_timer(string(__func__));
  context *ctx = this->get_context();
  consts_struct_t const &cs = this->get_consts_struct();
  set<expr_ref> exprs;
  std::function<void (pc const &p, const string&, expr_ref)> f = [this, &exprs, &cs](pc const &p, const string& key, expr_ref e) -> void
  {
    expr_ref ee = e;
    ee = this->expr_simplify_at_pc(ee, p);
    expr_identify_stack_pointer_relative_expressions(ee, exprs, cs);
  };

  list<shared_ptr<tfg_edge const>> edges;
  this->get_edges(edges);
  for (auto e : edges) {
    pc const &p = e->get_from_pc();
    f(p, G_EDGE_COND_IDENTIFIER, e->get_cond());
    std::function<void (const string &, expr_ref)> sf = [&f, &p](const string &key, expr_ref e) -> void { f(p, key, e); };
    e->get_to_state().state_visit_exprs_with_start_state(sf/*, this->get_start_state()*/);
  }
  return exprs;
}

//set<pair<string, expr_ref>>
//tfg::get_outgoing_values_at_delta(pc const &p, delta_t const& delta) const
//{
//  ASSERT(delta.get_lookahead() >= 0);
//
//  context *ctx = this->get_context();
//  set<pair<string, expr_ref>> ret;
//  sprel_map_t const &sprel_map = this->get_sprel_map(p);
//  set<cs_addr_ref_id_t> const &relevant_addr_refs = this->graph_get_relevant_addr_refs();
//
//  auto relevant_locs = this->get_locs();
//  if (delta.get_lookahead() == 0) {
//    for (auto loc : relevant_locs) {
//      if (!this->loc_is_defined_at_pc(p,loc.first))
//        continue;
//      stringstream ss;
//      ss << "loc" << loc.first;
//      ret.insert(make_pair(ss.str(), this->graph_loc_get_value_for_node(loc.first)));
//    }
//    return ret;
//  }
//  list<shared_ptr<tfg_edge_composition_t>> outgoing_paths = this->get_outgoing_paths_at_lookahead_and_max_unrolls(p, delta.get_lookahead(), delta.get_max_unroll());
//  for (auto loc : relevant_locs) {
//    stringstream ss;
//    ss << "loc" << loc.first;
//    expr_ref loc_expr = this->graph_loc_get_value_for_node(loc.first);
//
//    for (const auto &opath : outgoing_paths) {
//      if (!this->loc_is_defined_at_pc(opath->graph_edge_composition_get_to_pc(),loc.first))
//        continue;
//      predicate ep(precond_t(*this), loc_expr, loc_expr, "tmp", predicate::not_provable);
//      unordered_set<pair<edge_guard_t, predicate>> epreds = this->apply_trans_funs(opath/*, map<shared_ptr<tfg_edge>, state>()*/, ep);
//      for (auto eepa : epreds) {
//        predicate epa = eepa.second;
//        expr_ref e = epa.get_lhs_expr();
//        //map<expr_ref, memlabel_t> alias_map;
//        expr_ref e_simplified = ctx->expr_simplify_using_sprel_and_memlabel_maps(e, sprel_map, this->get_memlabel_map());
//        //e = ctx->expr_do_simplify(e);
//        if (!ctx->expr_contains_function_call(e_simplified)) {
//          ret.insert(make_pair(opath->graph_edge_composition_get_to_pc().to_string() + "." + ss.str(), e));
//        }
//      }
//    }
//  }
//  return ret;
//}

/*list<shared_ptr<tfg_edge>>
tfg_edge_composite::get_constituent_edges(state const &start_state, consts_struct_t const &cs) const
{
  list<tfg_edge_ptr> ls = m_composition_node->get_constituent_atoms();
  list<shared_ptr<tfg_edge>> ret;
  for (auto e : ls) {
    ret.push_back(e.get_edge_ptr());
  }
  return ret;
}*/

/*list<list<shared_ptr<tfg_edge>>>
tfg_edge_composite::get_paths(state const &start_state, consts_struct_t const &cs) const
{
  list<list<tfg_edge_ptr>> paths = m_composition_node->get_paths();
  if (paths.size() > 1024) {
    cout << __func__ << " " << __LINE__ << ": Warning: paths.size() = " << paths.size() << endl;
  }
  list<list<shared_ptr<tfg_edge>>> ret;
  for (const auto &path : paths) {
    list<shared_ptr<tfg_edge>> ret_path;
    for (const auto &e : path) {
      ret_path.push_back(e.get_edge_ptr());
    }
    ret.push_back(ret_path);
  }
  return ret;
}*/

/*set<graph_loc_id_t>
tfg::get_pred_min_dependencies_set_for_composite_edge_loc(shared_ptr<tfg_edge_composite> e, graph_loc_id_t loc_id) const
{
  if (e->is_atomic()) {
    return this->get_pred_min_dependencies_set_for_edge_loc(e->get_atomic_edge(), loc_id);
  }
  state const &start_state = this->get_start_state();
  consts_struct_t const &cs = this->get_consts_struct();
  if (e->is_parallel_composition()) {
    set<graph_loc_id_t> ret_a = this->get_pred_min_dependencies_set_for_edge_loc(e->get_composition_edgeA(start_state, cs), loc_id);
    set<graph_loc_id_t> ret_b = this->get_pred_min_dependencies_set_for_edge_loc(e->get_composition_edgeB(start_state, cs), loc_id);
    set<graph_loc_id_t> ret;
    set_union(ret_a, ret_b, ret);
    return ret;
  }
  if (e->is_series_composition()) {
    set<graph_loc_id_t> ret;
    set<graph_loc_id_t> ret_b = this->get_pred_min_dependencies_set_for_edge_loc(e->get_composition_edgeB(start_state, cs), loc_id);
    for (auto b : ret_b) {
      set<graph_loc_id_t> ret_a = this->get_pred_min_dependencies_set_for_edge_loc(e->get_composition_edgeA(start_state, cs), b);
      set_union(ret, ret_a);
    }
    return ret;
  }
  NOT_REACHED();
}*/

/*tfg_edge_composite
tfg::path_to_composite_edge(list<shared_ptr<tfg_edge>> const &path) const
{
  typename list<shared_ptr<tfg_edge>>::const_iterator iter = path.begin();
  state const &start_state = this->get_start_state();
  tfg_edge_composite ret(*iter);
  //cout << __func__ << " " << __LINE__ << ": ret = " << ret.to_string(&start_state) << endl;
  for (iter++; iter != path.end(); iter++) {
    shared_ptr<tfg_edge> e = *iter;
    //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string(&start_state) << endl;
    ret.add_edge_in_series(e, start_state, this->get_consts_struct());
    //cout << __func__ << " " << __LINE__ << ": ret = " << ret.to_string(&start_state) << endl;
  }
  return ret;
}*/

/*string
tfg_edge_composite::to_string_for_eq() const
{
  stringstream ss;
  ss << this->get_from_pc().to_string() << " => " << this->get_to_pc().to_string() << " [" << m_composition_node->to_string() << "]";
  return ss.str();
}*/

map<pc, map<graph_loc_id_t, mybitset>>
tfg::compute_dst_locs_assigned_to_constants_map() const
{
  map<pc, map<graph_loc_id_t, mybitset>> ret;
  list<shared_ptr<tfg_edge const>> es;
  this->get_edges(es);
  for (auto ed : es) {
    map<graph_loc_id_t, mybitset> ret_at_to_pc = ret[ed->get_to_pc()];
    map<graph_loc_id_t, expr_ref> locs_written = this->get_locs_definitely_written_on_edge(ed);
    for (const auto &le : locs_written) {
      graph_loc_id_t loc_id = le.first;
      expr_ref const &e = le.second;
      if (e->is_const() && e->is_bv_sort()) {
        mybitset cval = e->get_mybitset_value();
        if (cval != mybitset(0,cval.get_numbits())) {
          ret_at_to_pc[loc_id] = cval;
          //cout << __func__ << " " << __LINE__ << ": pc = " << ed->get_to_pc().to_string() << ", loc_id = " << loc_id << ", cval = " << cval << endl;
        }
      }
    }
    //for (auto loc : this->get_locs()) {
    //  graph_loc_id_t loc_id = loc.first;
    //  //expr_ref e = this->get_pred_min_dependencies_expr_for_edge_loc(ed, loc_id);
    //  //if (e->is_const() && e->is_bv_sort()) {
    //  //  mybitset cval = e->get_mybitset_value();
    //  //  if (cval != mybitset(0,cval.get_numbits())) {
    //  //    ret_at_to_pc[loc_id] = cval;
    //  //    //cout << __func__ << " " << __LINE__ << ": pc = " << ed->get_to_pc().to_string() << ", loc_id = " << loc_id << ", cval = " << cval << endl;
    //  //  }
    //  //}
    //}
    ret[ed->get_to_pc()] = ret_at_to_pc;
  }
  return ret;
}

void
tfg::tfg_eliminate_memlabel_esp()
{
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [ctx](pc const &p, const string& key, expr_ref e) -> expr_ref
  {
    return ctx->expr_eliminate_memlabel_esp(e);
  };
  tfg_visit_exprs(f);
}

class copy_prop_relationship_t
{
private:
  bool m_is_top;
public:
  map<expr_id_t, pair<expr_ref, expr_ref>> m;
  copy_prop_relationship_t() : m_is_top(false) {}

  static copy_prop_relationship_t
  top()
  {
    copy_prop_relationship_t ret;
    ret.m_is_top = true;
    return ret;
  }

  string to_string() const
  {
    if (m_is_top) {
      return "TOP";
    }
    stringstream ss;
    for (const auto &p : m) {
      ss << expr_string(p.second.first) << " -> " << expr_string(p.second.second) << endl;
    }
    return ss.str();
  }


  bool meet(copy_prop_relationship_t const &out2)
  {
    ASSERT(!out2.m_is_top);
    if (this->m_is_top) {
      *this = out2;
      return true;
    }
    set<expr_id_t> to_erase;
    for (auto &p : this->m) {
      if (   !out2.m.count(p.first)
          || out2.m.at(p.first).second != p.second.second) {
        to_erase.insert(p.first);
      }
    }
    for (const auto &k : to_erase) {
      this->m.erase(k);
    }
    return to_erase.size() > 0;
  }

  map<expr_id_t, pair<expr_ref, expr_ref>> const& get_submap() const { return m; }

  //map<expr_id_t, pair<expr_ref, expr_ref>> get_submap(state const &start_state) const
  //{
  //  ASSERT(!m_is_top || this->m.size() == 0);
  //  map<expr_id_t, pair<expr_ref, expr_ref>> ret;
  //  map<string_ref, expr_ref> const &ke = start_state.get_value_expr_map_ref();
  //  for (const auto &p : this->m) {
  //    ASSERT(ke.count(mk_string_ref(p.first)));
  //    ASSERT(ke.count(mk_string_ref(p.second)));
  //    expr_ref const &from = ke.at(mk_string_ref(p.first));
  //    expr_ref const &to = ke.at(mk_string_ref(p.second));
  //    ret[from->get_id()] = make_pair(from, to);
  //  }
  //  return ret;
  //}
};

class copy_prop_dfa_t : public data_flow_analysis<pc, tfg_node, tfg_edge, copy_prop_relationship_t, dfa_dir_t::forward>
{
private:
  //state const &m_start_state;
public:
  copy_prop_dfa_t(tfg const &t, map<pc, copy_prop_relationship_t> &init_vals) : data_flow_analysis<pc, tfg_node, tfg_edge, copy_prop_relationship_t, dfa_dir_t::forward>(&t, init_vals)//, m_start_state(t.get_start_state())
  {
  }
  bool xfer_and_meet(shared_ptr<tfg_edge const> const &e, copy_prop_relationship_t const &in, copy_prop_relationship_t &out) override
  {
    copy_prop_relationship_t ret = in;
    state const &st = e->get_to_state();
    map<string_ref, expr_ref> const &m = st.get_value_expr_map_ref();
    for (const auto &p : m) {
      context *ctx = p.second->get_context();
      expr_ref from_expr = ctx->get_input_expr_for_key(p.first, p.second->get_sort());
      ret.m.erase(from_expr->get_id());
      if (ctx->expr_is_input_expr(p.second)) {
        expr_ref to_expr = p.second;
        //string key = identify_key_for_expr(p.second);
        //if (key != "") {
          if (ret.m.count(to_expr->get_id())) {
            to_expr = ret.m.at(to_expr->get_id()).second;
          }
          ret.m.insert(make_pair(from_expr->get_id(), make_pair(from_expr, to_expr)));
        //}
      }
    }
    return out.meet(ret);
  }
private:
  //string identify_key_for_expr_using_start_state(expr_ref const &e) const
  //{
  //  map<string_ref, expr_ref> const &m = m_start_state.get_value_expr_map_ref();
  //  for (const auto &p : m) {
  //    if (e == p.second) {
  //      return p.first->get_str();
  //    }
  //  }
  //  return "";
  //}
};

void
tfg::substitute_at_outgoing_edges_and_node_predicates(pc const &p, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap)
{
  context *ctx = this->get_context();
  //state const &start_state = this->get_start_state();
  list<shared_ptr<tfg_edge const>> outgoing;
  this->get_edges_outgoing(p, outgoing);
  std::function<expr_ref (pc const &, string const &, expr_ref const &)> f = [ctx, &submap](pc const &, string const &, expr_ref const &e)
  {
    return ctx->expr_substitute(e, submap);
  };
  std::function<expr_ref (expr_ref const &)> fe = [ctx, &submap](expr_ref const &e)
  {
    return ctx->expr_substitute(e, submap);
  };
  for (const auto &e : outgoing) {
    //auto new_e = e->substitute_variables_at_input(e->get_to_state(), submap, ctx);
    auto new_e = e->visit_exprs(fe);
    this->remove_edge(e);
    this->add_edge(new_e);
    set<pc> pcs = { p };
    this->graph_preds_visit_exprs(pcs, f);
  }
}

void
tfg::tfg_run_copy_propagation()
{
  map<pc, copy_prop_relationship_t> sol;
  copy_prop_dfa_t copy_prop_dfa(*this, sol);
  copy_prop_dfa.initialize(copy_prop_relationship_t());
  copy_prop_dfa.solve();
  //map<pc, copy_prop_relationship_t> sol = copy_prop_dfa.get_values();
  DYN_DEBUG(copy_prop,
    cout << __func__ << " " << __LINE__ << ": Printing copy prop relationships\n";
    for (const auto &pcpr : sol) {
      cout << pcpr.first.to_string() << ":\n" << pcpr.second.to_string() << endl;
    }
  );
  for (const auto &cpr : sol) {
    map<expr_id_t, pair<expr_ref, expr_ref>> const& submap = cpr.second.get_submap();
    this->substitute_at_outgoing_edges_and_node_predicates(cpr.first, submap);
  }
}

//class init_status_lattice_t
//{
//public:
//  class lattice_elem_t
//  {
//    public:
//      typedef enum { INIT_STATUS_INIT, INIT_STATUS_UNINIT } status_t;
//      status_t m_status;
//      lattice_elem_t(status_t status) : m_status(status) {}
//      bool is_uninit() const { return m_status == INIT_STATUS_UNINIT; }
//      bool is_bottom() const { return m_status == INIT_STATUS_UNINIT; }
//      bool is_top() const { return m_status == INIT_STATUS_INIT; }
//      bool operator==(lattice_elem_t const &other) const { return m_status == other.m_status; }
//      bool operator!=(lattice_elem_t const &other) const { return m_status != other.m_status; }
//  };
//
//  class expr_identify_init_status_visitor : public expr_loc_visitor<expr_ref> {
//    public:
//      expr_identify_init_status_visitor(expr_ref const &e, map<graph_loc_id_t, lattice_elem_t> const &pred_map, std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn/*, consts_struct_t const &cs*/) : expr_loc_visitor<expr_ref>(e->get_context(), expr2locid_fn), m_in(e), m_pred_map(pred_map), m_expr2locid_fn(expr2locid_fn)
//    {
//      //m_visit_each_expr_only_once = true;
//      m_ctx = e->get_context();
//      //visit_recursive(m_in);
//    }
//
//      expr_ref visit(expr_ref const &e, bool represents_loc, set<graph_loc_id_t> const &locs, vector<expr_ref> const &argvals) override
//      {
//        if (e->is_const()) {
//          //m_map[e->get_id()] = e;
//          //return;
//          return e;
//        }
//        if (represents_loc) {
//        //graph_loc_id_t e_loc_id;
//        //if ((e_loc_id = m_expr2locid_fn(e)) != GRAPH_LOC_ID_INVALID)
//          bool is_uninit = true;
//          for (auto e_loc_id : locs) {
//            lattice_elem_t pred_val = m_pred_map.at(e_loc_id);
//            if (!pred_val.is_uninit()) {
//              is_uninit = false;
//            }
//          }
//          if (is_uninit) {
//            return m_ctx->mk_var(UNINIT_KEYWORD, e->get_sort());
//          } else {
//            return e;
//          }
//        }
//        if (e->is_var()) {
//          //graph_loc_id_t e_loc_id = expr_var_is_pred_dependency(e);
//          //if (e_loc_id != -1) {
//          //}
//          //m_map[e->get_id()] = e;
//          //cout << __func__ << " " << __LINE__ << ": returning " << expr_string(m_map.at(e->get_id())) << " for " << expr_string(e) << endl;
//          return e;
//        }
//        expr_vector new_args;
//        for (auto const& argval : argvals) {
//          //new_args.push_back(m_map.at(arg->get_id()));
//          new_args.push_back(argval);
//        }
//        expr_ref new_e = m_ctx->mk_app(e->get_operation_kind(), new_args);
//        //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << ", new_e = " << expr_string(new_e) << endl;
//        if (new_e->get_operation_kind() == expr::OP_ALLOCA) {
//          memlabel_t const &ml = new_args.at(1)->get_memlabel_value();
//          expr_ref const &addr = new_args.at(2);
//          size_t nbytes = new_args.at(3)->get_int_value();
//          expr_ref uninit_var = m_ctx->mk_var(UNINIT_KEYWORD, m_ctx->mk_bv_sort(nbytes * BYTE_LEN));
//          new_e = m_ctx->mk_store(new_e, ml, addr, uninit_var, nbytes, false/*, comment_t()*/);
//        }/* else if (new_e->get_operation_kind() == expr::OP_RETURN_VALUE_PTR) {
//            expr_ref arg0 = e->get_args().at(0);
//            if (expr_is_uninit(arg0)) {
//            new_e = m_map.at(arg0->get_id());
//            }
//            }*/
//        //m_map[e->get_id()] = new_e;
//        return new_e;
//        //cout << __func__ << " " << __LINE__ << ": returning " << expr_string(m_map.at(e->get_id())) << " for " << expr_string(e) << endl;
//      }
//
//      //bool expr_is_uninit(expr_ref const &in_e) const
//      //{
//      //  ASSERT(m_map.count(in_e->get_id()) > 0);
//      //  expr_ref e = m_map.at(in_e->get_id());
//      //  e = m_ctx->expr_do_simplify(e);
//      //  if (e->is_var() && expr_string(e) == UNINIT_KEYWORD) {
//      //    return true;
//      //  }
//      //  return false;
//      //}
//
//    private:
//      expr_ref const &m_in;
//      map<graph_loc_id_t, lattice_elem_t> const &m_pred_map;
//      std::function<graph_loc_id_t (expr_ref const &)> m_expr2locid_fn;
//      //map<expr_id_t, expr_ref> m_map;
//      context *m_ctx;
//  };
//
//  lattice_elem_t top() const
//  {
//    return lattice_elem_t(lattice_elem_t::INIT_STATUS_INIT);
//  }
//
//  lattice_elem_t start_pc_value() const
//  {
//    return lattice_elem_t(lattice_elem_t::INIT_STATUS_INIT);
//  }
//
//
//  lattice_elem_t meet(lattice_elem_t const &a, lattice_elem_t const &b) const
//  {
//    if (a.is_bottom()) {
//      return a;
//    }
//    if (b.is_bottom()) {
//      return b;
//    }
//    //both are top; return any
//    return a;
//  }
//  lattice_elem_t transfer_function(expr_ref e, map<graph_loc_id_t, lattice_elem_t> const &pred_map, consts_struct_t const &cs, std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn) const
//  {
//    context *ctx = e->get_context();
//    expr_identify_init_status_visitor visitor(e, pred_map, expr2locid_fn);
//    expr_ref s = visitor.visit_recursive(e, {});
//    s = ctx->expr_do_simplify(s);
//    if (s->is_var() && s->get_name() == UNINIT_KEYWORD) {
//      lattice_elem_t ret(lattice_elem_t::INIT_STATUS_UNINIT);
//      return ret;
//    } else {
//      lattice_elem_t ret(lattice_elem_t::INIT_STATUS_INIT);
//      return ret;
//    }
//  }
//};

//map<pc, map<graph_loc_id_t, bool>>
//tfg::determine_initialization_status_for_locs() const
//{
//  init_status_lattice_t init_status_lattice;
//  map<pc, map<graph_loc_id_t, init_status_lattice_t::lattice_elem_t>> result;
//  result = this->graph_forward_fixed_point_computation_on_locs(init_status_lattice);
//  map<pc, map<graph_loc_id_t, bool>> ret;
//  for (const auto &pr : result) {
//    pc const &p = pr.first;
//    //cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << endl;
//    for (auto lb : pr.second) {
//      graph_loc_id_t loc_id = lb.first;
//      ret[p][loc_id] = lb.second.is_bottom();
//      /*if (ret.at(p).at(loc_id)) {
//        cout << __func__ << " " << __LINE__ << ": pc " << p.to_string() << ": " << loc_id << ": UNINIT" << endl;
//        } else {
//        cout << __func__ << " " << __LINE__ << ": pc " << p.to_string() << ": " << loc_id << ": INIT" << endl;
//        }*/
//    }
//  }
//  return ret;
//}

bool
tfg::return_register_is_uninit_at_exit(string return_reg_name, pc const &exit_pc/*, map<pc, map<graph_loc_id_t, bool>> const &init_status*/) const
{
  //map<graph_loc_id_t, bool> const &init_status_at_pc = init_status.at(exit_pc);
  ASSERT(this->get_loc_definedness().count(exit_pc));
  set<graph_loc_id_t> const &def_locs = this->get_loc_definedness().at(exit_pc);
  for (auto li : def_locs) {
    expr_ref loc_expr = this->graph_loc_get_value_for_node(li);
    string loc_string = expr_string(loc_expr);
    //cout << __func__ << " " << __LINE__ << ": loc_string = " << loc_string << endl;
    if (loc_string == string(G_INPUT_KEYWORD) + "." + return_reg_name) {
      return false/*li.second*/;
    }
  }
  //cout << __func__ << " " << __LINE__ << ": return_reg_name = " << return_reg_name << endl;
  return true;
}

/*bool
tfg::tfg_callee_summary_exists(string const &fun_name) const
{
  return m_callee_summaries.count(fun_name) > 0;
}

  void
tfg::tfg_callee_summary_add(string const &fun_name, callee_summary_t const &csum)
{
  m_callee_summaries[fun_name] = csum;
}

callee_summary_t
tfg::tfg_callee_summary_get(string const &fun_name) const
{
  ASSERT(m_callee_summaries.count(fun_name));
  return m_callee_summaries.at(fun_name);
}*/


/*bool
tfg::tfg_is_nop() const
{
  for (auto p : this->get_all_pcs()) {
    if (!p.is_start() && !p.is_exit()) {
      return false;
    }
  }
  list<pc> exit_pcs;
  this->get_exit_pcs(exit_pcs);
  if (exit_pcs.size() != 1) {
    return false;
  }
  auto exit_pc = *exit_pcs.begin();
  list<shared_ptr<tfg_edge>> incoming;
  this->get_edges_incoming(exit_pc, incoming);
  ASSERT(incoming.size() == 1);
  shared_ptr<tfg_edge> incoming_edge = *incoming.begin();
  expr_vector const &return_regs = this->get_return_regs(exit_pc);
  for (auto r : return_regs) {
    if (   r->is_bv_sort()
        && expr_string(r).find(G_LLVM_RETURN_REGISTER_NAME) != string::npos) {
      return false;
    }
  }
  set<graph_loc_id_t> const &ret_locs = this->get_live_locs(exit_pc);
  for (auto ret_loc : ret_locs) {
    set<graph_loc_id_t> const &pred_locs = this->get_pred_min_dependencies_set_for_edge_loc(incoming_edge, ret_loc);
    if (pred_locs.size() > 1 || (*pred_locs.begin()) != ret_loc) {
      return false;
    }
    expr_ref pred_expr = this->get_pred_min_dependencies_expr_for_edge_loc(incoming_edge, ret_loc);
    if (pred_expr != this->get_loc_expr(ret_loc)) {
      return false;
    }
  }
  return true;
}*/

/*bool
tfg::tfg_reads_heap_or_symbols() const
{
  bool ret = false;
  std::function<void (pc const &p, const string&, expr_ref)> f = [&ret](pc const &p, const string& key, expr_ref e) -> void
  {
    ret = ret || expr_reads_heap_or_symbols(e);
  };
  tfg_visit_exprs(f);
  //cout << __func__ << " " << __LINE__ << ": returning " << ret;
  return ret;
}

bool
tfg::tfg_writes_heap_or_symbols() const
{
  bool ret = false;
  std::function<void (pc const &p, const string&, expr_ref)> f = [&ret](pc const &p, const string& key, expr_ref e) -> void
  {
    ret = ret || expr_writes_heap_or_symbols(e);
  };
  tfg_visit_exprs(f);
  //cout << __func__ << " " << __LINE__ << ": this =\n"  << this->tfg_to_string(true) << endl;
  //cout << __func__ << " " << __LINE__ << ": returning " << ret;
  return ret;
}*/


set<memlabel_ref>
tfg::tfg_get_read_memlabels() const
{
  autostop_timer func_timer(__func__);
  context *ctx = this->get_context();
  set<cs_addr_ref_id_t> const &relevant_addr_refs = this->graph_get_relevant_addr_refs();
  set<mlvarname_t> mlvars;
  std::function<void (string const&, expr_ref)> f = [&mlvars, ctx/*, this, &relevant_addr_refs*/](string const& key, expr_ref e) -> void
  {
    set_union(mlvars, ctx->expr_get_read_mlvars(e/*_simplified*/));
  };
  tfg_visit_exprs_const(f);
  set<memlabel_ref> ret = get_memlabel_set_from_mlvarnames(this->get_memlabel_map(), mlvars);
  return ret;
}

set<memlabel_ref>
tfg::tfg_get_write_memlabels() const
{
  autostop_timer func_timer(__func__);
  context *ctx = this->get_context();
  set<cs_addr_ref_id_t> const &relevant_addr_refs = this->graph_get_relevant_addr_refs();
  set<mlvarname_t> mlvars;
  function<void (string const&, expr_ref)> f = [&mlvars, ctx/*, this, &relevant_addr_refs*/](string const& key, expr_ref e) -> void
  {
    set_union(mlvars, ctx->expr_get_write_mlvars(e/*_simplified*/));
  };
  tfg_visit_exprs_const(f);
  //cout << __func__ << " " << __LINE__ << ": mlvarnames (size " << mlvars.size() << "):";
  //for (const auto& mlvar : mlvars) {
  //  cout << " " << mlvar;
  //}
  //cout << endl;

  set<memlabel_ref> ret = get_memlabel_set_from_mlvarnames(this->get_memlabel_map(), mlvars);
  return ret;
}

callee_summary_t
tfg::get_summary_for_calling_functions() const
{
  autostop_timer func_timer(__func__);
  callee_summary_t ret;
  //ret.set_is_nop(this->tfg_is_nop());
  //cout << __func__ << " " << __LINE__ << ": TFG =\n" << this->graph_to_string() << endl;
  set<memlabel_ref> reads = this->tfg_get_read_memlabels();
  set<memlabel_ref> writes = this->tfg_get_write_memlabels();
  reads = memlabel_t::eliminate_locals_and_stack(reads);
  reads = memlabel_t::memlabel_set_eliminate_ro_symbols(reads); //ro symbols require special handling because they are hard to correlate across src and dst programs; their access in the callee has little consequence in the caller so eliminate them in the callee summary to avoid related handling headaches
  writes = memlabel_t::eliminate_locals_and_stack(writes);
  writes = memlabel_t::memlabel_set_eliminate_ro_symbols(writes); //similar to reads
  set<memlabel_t> read_mls, write_mls;
  for (auto const& r : reads) {
    read_mls.insert(r->get_ml());
  }
  for (auto const& w : writes) {
    write_mls.insert(w->get_ml());
  }
  ret.set_reads(read_mls);
  ret.set_writes(write_mls);
  return ret;
}

void
tfg::set_callee_summaries(map<nextpc_id_t, callee_summary_t> const &callee_summaries)
{
  m_callee_summaries = callee_summaries;
}

map<nextpc_id_t, callee_summary_t> const &
tfg::get_callee_summaries() const
{
  return m_callee_summaries;
}

list<shared_ptr<tfg_edge const>>
tfg::get_fcall_edges(nextpc_id_t nextpc_id) const
{
  list<shared_ptr<tfg_edge const>> es, ret;
  this->get_edges(es);
  for (auto e : es) {
    nextpc_id_t e_nextpc_id;
    if (!e->is_fcall_edge(e_nextpc_id)) {
      continue;
    }
    if (e_nextpc_id != nextpc_id) {
      continue;
    }
    ret.push_back(e);
  }
  return ret;
}

/*void
  tfg::replace_fcalls_with_nop(nextpc_id_t nextpc_id)
  {
  list<shared_ptr<tfg_edge>> fcall_edges = this->get_fcall_edges(nextpc_id);
  state state_nop;
  for (auto e : fcall_edges) {
  e->set_to_state(state_nop);
  }
  }*/

/*void
  tfg::model_nop_callee_summaries(map<nextpc_id_t, callee_summary_t> const &callee_summaries)
  {
  for (auto e: callee_summaries) {
//string const &fun_name = e.first;
callee_summary_t const &csum = e.second;
nextpc_id_t nextpc_id = e.first; //get_nextpc_id_from_function_name(fun_name);
ASSERT(nextpc_id != -1);
if (csum.is_nop()) {
this->replace_fcalls_with_nop(nextpc_id);
}
}
}*/

nextpc_id_t
tfg::get_nextpc_id_from_function_name(string const &fun_name) const
{
  for (auto npc : m_nextpc_map) {
    if (npc.second == fun_name) {
      return npc.first;
    }
  }
  return -1;
}

shared_ptr<tfg_edge const>
tfg::get_incident_fcall_edge(pc const &p) const
{
  shared_ptr<tfg_edge const> incident_edge;
  if (p.get_subsubindex() == PC_SUBSUBINDEX_FCALL_START) {
    list<shared_ptr<tfg_edge const>> outgoing;
    this->get_edges_outgoing(p, outgoing);
    ASSERT(outgoing.size() == 1);
    return *outgoing.begin();
  } else if (p.get_subsubindex() == PC_SUBSUBINDEX_FCALL_END) {
    list<shared_ptr<tfg_edge const>> incoming;
    this->get_edges_incoming(p, incoming);
    ASSERT(incoming.size() == 1);
    return *incoming.begin();
  } else {
    return nullptr;
  }
}

expr_ref
tfg::get_incident_fcall_nextpc(pc const &p) const
{
  shared_ptr<tfg_edge const> incident_edge = this->get_incident_fcall_edge(p);
  //cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", incident_edge = " << incident_edge->to_string() << endl;
  if (!incident_edge) {
    expr_ref null_expr;
    return null_expr;
  }
  expr_ref ret = incident_edge->get_function_call_nextpc();
  //cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", ret = " << ret << endl;
  return ret;
}

nextpc_id_t
tfg::get_incident_fcall_nextpc_id(pc const &p) const
{
  expr_ref nextpc = this->get_incident_fcall_nextpc(p);
  if (!nextpc->is_nextpc_const()) {
    return -1;
  }
  return nextpc->get_nextpc_num();
}

memlabel_t
tfg::get_incident_fcall_memlabel_writeable(pc const &p) const
{
  shared_ptr<tfg_edge const> incident_edge = this->get_incident_fcall_edge(p);
  ASSERT(incident_edge);
  return incident_edge->get_function_call_memlabel_writeable(this->get_memlabel_map());
}

bool
tfg::pc_has_incident_fcall(pc const &p) const
{
  expr_ref incident_fcall_nextpc = this->get_incident_fcall_nextpc(p);
  if (incident_fcall_nextpc) {
    return true;
  } else {
    return false;
  }
}

bool
tfg::incident_fcall_nextpcs_are_compatible(expr_ref nextpc1, expr_ref nextpc2)
{
  //cout << __func__ << " " << __LINE__ << ": nextpc1 = " << expr_string(nextpc1) << endl;
  //cout << __func__ << " " << __LINE__ << ": nextpc2 = " << expr_string(nextpc2) << endl;
  if (   nextpc1->is_nextpc_const()
      || nextpc2->is_nextpc_const()) {
    return nextpc1 == nextpc2;
  }
  return true;
}

// The PCs p1 and p2 can belong to different tfgs i.e. p1 to src-tfg and p2 to dst-tfg. So can't pass p2 directly to this function
bool
tfg::is_pc_pair_compatible(pc const &p1, int p2_subsubindex, expr_ref const& p2_incident_fcall_nextpc) const
{
  expr_ref p1_incident_fcall_nextpc = this->get_incident_fcall_nextpc(p1);
  int p1_to_subsubindex = p1.get_subsubindex();
  //dst_pc is fcall pc
  if (   (   p2_subsubindex == PC_SUBSUBINDEX_FCALL_START
          || p2_subsubindex == PC_SUBSUBINDEX_FCALL_END)
      && p2_incident_fcall_nextpc) {
    //src_pc is fcall pc
    if (   p1_to_subsubindex == p2_subsubindex
        && incident_fcall_nextpcs_are_compatible(p1_incident_fcall_nextpc, p2_incident_fcall_nextpc))
      return true;
  } else {
    //dst_pc is not fcall pc and  src pc is not fcall pc
    if(!is_fcall_pc(p1))
      return true;
  }
  return false;
}

void
tfg::get_reachable_pcs_stopping_at_fcalls(pc const &p, set<pc> &reachable_pcs) const
{
  list<shared_ptr<tfg_edge const>> out_edges;
  get_edges_outgoing(p, out_edges);

  for (auto out_edge : out_edges) {
    pc p_next = out_edge->get_to_pc();
    if(!reachable_pcs.count(p_next)) {
      reachable_pcs.insert(p_next);
      if(!this->is_fcall_pc(p_next))
        get_reachable_pcs_stopping_at_fcalls(p_next, reachable_pcs);
    }
  }
}

// max number of times any pc can appear as to-pc - unroll factor
// number of times a pc \in to_pcs appears as to-pc in an ec - delta
// We make parallel of all paths with same delta and different values of unroll factor
// The output is an ec for each value for delta from 1 to unroll factor
map<pair<pc, int>,  shared_ptr<tfg_edge_composition_t>>
tfg::get_composite_path_between_pcs_till_unroll_factor_helper(pc const& from_pc, set<pc> const& to_pcs, map<pc,int> const& visited_count, int pth_length, int unroll_factor, set<pc> const& stop_pcs) const
{
  //cout << __func__ << ':' << __LINE__ << ": visited_count.size() = " << visited_count.size() << endl;
  map<pair<pc, int>, list<shared_ptr<tfg_edge_composition_t>>> parallel_paths;
  map<pair<pc, int>, shared_ptr<tfg_edge_composition_t>> ret_paths;
  if (pth_length > TFG_ENUMERATION_MAX_PATH_LENGTH) {
    return ret_paths;
  }
  list<shared_ptr<const tfg_edge>> outgoing_edges;
  this->get_edges_outgoing(from_pc, outgoing_edges);
  for (auto const& out_e : outgoing_edges){
    map<pair<pc,int>, list<shared_ptr<tfg_edge_composition_t>>> parallel_paths_for_this_edge;
    pc const& e_to_pc = out_e->get_to_pc();
    if (   visited_count.count(e_to_pc)
        && visited_count.at(e_to_pc) >= unroll_factor) { // next addition would increase this visited_count
      continue;
    }

    shared_ptr<tfg_edge_composition_t> left_part = mk_edge_composition<pc,tfg_edge>(out_e);
    if (to_pcs.count(e_to_pc)) {
      parallel_paths_for_this_edge[make_pair(e_to_pc, 1)].push_back(mk_epsilon_ec<pc,tfg_edge>());
    }
    if (!set_belongs(stop_pcs,e_to_pc)) {
      map<pc,int> new_visited_count = visited_count;
      new_visited_count.at(e_to_pc)++;
      map<pair<pc, int>, shared_ptr<tfg_edge_composition_t>> right_part = get_composite_path_between_pcs_till_unroll_factor_helper(e_to_pc, to_pcs, new_visited_count, pth_length+1, unroll_factor, stop_pcs);
      for (auto const& r : right_part) {
        pc const& pth_to_pc = r.first.first;
        int rep = r.first.second;
        auto const &ec = r.second;
        if (pth_to_pc == e_to_pc) {
          rep++;
        }
        parallel_paths_for_this_edge[make_pair(pth_to_pc, rep)].push_back(ec);
      }
    }
    for (auto const &p : parallel_paths_for_this_edge) {
      parallel_paths[p.first].push_back(mk_series(left_part, graph_edge_composition_construct_edge_from_parallel_edgelist(p.second)));
    }
  }
  for (auto const &p : parallel_paths) {
    ret_paths[p.first] = graph_edge_composition_construct_edge_from_parallel_edgelist(p.second);
  }
  return ret_paths;
}

bool
tfg::is_tfg_ec_mutex(shared_ptr<tfg_edge_composition_t> const &ec) const
{
  if(!ec)
    return true;
  ASSERT(ec);
  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> sterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_serial_terms(ec);
  ASSERT(sterms.size() > 0);
  bool is_mutex = true;
  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> back_pterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_parallel_terms(sterms.back());
  for (auto const& bpc : back_pterms) {
    if (bpc->is_atom() && bpc->get_atom()->is_empty()) {
      is_mutex = false;
      break;
    }
  }
  return is_mutex;
}


void
tfg::get_unrolled_loop_paths_from(pc from_pc, list<shared_ptr<tfg_edge_composition_t>>& pths, int max_occurrences_of_a_node) const
{

  map< pair<pc,int>, shared_ptr<tfg_edge_composition_t> > tfg_paths_to_pcs;
  int from_pc_subsubindex = from_pc.get_subsubindex();
  expr_ref incident_fcall_nextpc = this->get_incident_fcall_nextpc(from_pc);
  this->get_unrolled_paths_from(from_pc, tfg_paths_to_pcs, from_pc_subsubindex, incident_fcall_nextpc, max_occurrences_of_a_node);
  for(auto const& pth : tfg_paths_to_pcs)
  {
    if(pth.first.first == from_pc) {
      auto const& simpl_pth = semantically_simplify_tfg_edge_composition(pth.second);
      if(simpl_pth)
        pths.push_back(simpl_pth);
    }
  }
}


//This is a virtual function.  This implementation is only called for the DST TFG.  For the SRC TFG, tfg_ssa_t's implementation is invoked
map<pair<pc, int>,  shared_ptr<tfg_edge_composition_t>>
tfg::get_composite_path_between_pcs_till_unroll_factor(pc const& from_pc, set<pc> const& to_pcs, map<pc,int> const& visited_count, int unroll_factor, set<pc> const& stop_pcs) const
{
  map<pair<pc, int>, shared_ptr<tfg_edge_composition_t>> ret_pths;
  for(auto const& to_pc : to_pcs)
  {
    set<pc> new_stop_pcs = stop_pcs;
    ASSERT(new_stop_pcs.count(to_pc)); //because for DST TFG, we only enumerate paths till anchor nodes, and anchor nodes are also present in STOP_PCS
    if(!is_fcall_pc(to_pc))
      new_stop_pcs.erase(to_pc);
    map<pair<pc, int>, shared_ptr<tfg_edge_composition_t>> to_pc_pths = get_composite_path_between_pcs_till_unroll_factor_helper(from_pc, {to_pc}, visited_count, 0, unroll_factor, new_stop_pcs);
    ret_pths.insert(to_pc_pths.begin(), to_pc_pths.end());
  }
  return ret_pths;
}

void
tfg::get_unrolled_paths_from(pc const &pc_from, map<pair<pc,int>, shared_ptr<tfg_edge_composition_t>>& pths, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, int max_occurrences_of_a_node) const
{
  autostop_timer func_timer(__func__);
  pths.clear();
  ASSERT(max_occurrences_of_a_node >= 1);

  DYN_DEBUG2(correlate, cout << _FNLN_ << ": computing src paths from " << pc_from.to_string() << " with max_occurrences_of_a_node = " << max_occurrences_of_a_node << endl);
  set<pc> waypost_to_pcs;
  get_path_enumeration_reachable_pcs(pc_from, dst_to_pc_subsubindex, dst_to_pc_incident_fcall_nextpc, waypost_to_pcs);
  DYN_DEBUG2(correlate, cout << _FNLN_ << ": waypost_to_pcs.size() = " << waypost_to_pcs.size() << endl);
  set<pc> stop_pcs;
  get_path_enumeration_stop_pcs(stop_pcs); //returns anchor pcs for DST; for SRC, just the fcalls and exit nodes
  set<pc> new_waypost_to_pcs;
  for (auto const &p: waypost_to_pcs) {
    auto const& cache_index = make_tuple(pc_from, p, max_occurrences_of_a_node);
    if(m_from_pc_unroll_factor_to_paths_cache.count(cache_index))
      pths.insert(m_from_pc_unroll_factor_to_paths_cache.at(cache_index).begin(), m_from_pc_unroll_factor_to_paths_cache.at(cache_index).end());
    else
      new_waypost_to_pcs.insert(p);
  }
  if(!new_waypost_to_pcs.size())
    return;
  map<pc, int> init_visited_count;
  set<pc> all_pcs = this->get_all_pcs();
  for(auto const& p: all_pcs)
    init_visited_count.insert(make_pair(p,0));

  map<pair<pc, int>, shared_ptr<tfg_edge_composition_t>> ret_pths = get_composite_path_between_pcs_till_unroll_factor(pc_from, new_waypost_to_pcs, init_visited_count, max_occurrences_of_a_node, stop_pcs);
  pths.insert(ret_pths.begin(), ret_pths.end());
  // 'ret_pths' can have 1 to max_occurrences_of_a_node number of paths to same 'to_pc', each having a different value of delta.
  // when inserting these paths to cache, we iterate over the 'ret_pths' and insert all paths with same 'to_pc' (and different value of delta) to same entry in cache.
  for (auto const &pth : ret_pths) {
    pc const& to_pc = pth.first.first;
    DYN_DEBUG(print_pathsets, cout << _FNLN_ << ": from_pc: " << pc_from << ", (mu, delta): (" << max_occurrences_of_a_node << ", " << pth.first.second << "), pathset: " << pth.second->to_string_concise() << endl);
    auto const& cache_index = make_tuple(pc_from, to_pc, max_occurrences_of_a_node);
    m_from_pc_unroll_factor_to_paths_cache[cache_index].insert(pth);
  }
}

list<pair<edge_guard_t, predicate_ref>>
tfg::apply_trans_funs(shared_ptr<tfg_edge_composition_t> const& ec/*, map<shared_ptr<tfg_edge>, state> const &fcall_edge_overrides*/, predicate_ref const &pred, bool simplified) const
{
  list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> ret_ec = this->graph_apply_trans_funs(ec, pred, simplified);
  list<pair<edge_guard_t, predicate_ref>> ret;
  for (auto const& ep : ret_ec) {
    ret.push_back(make_pair(edge_guard_t(ep.first), ep.second));
  }
  //ASSERT(ret == this->apply_trans_funs_old(ec, pred, simplified));
  return ret;
}

//unordered_set<pair<expr_ref, predicate>>
//tfg::edge_composition_apply_trans_funs_on_pred(predicate const &pred, shared_ptr<tfg_edge_composition_t> const& ec) const
//{
//  unordered_set<pair<expr_ref, predicate>> ret;
//  for (auto psd : this->apply_trans_funs(ec, pred)) {
//    ret.insert(make_pair(psd.first.edge_guard_get_expr(*this), psd.second));
//  }
//  return ret;
//}

void
tfg::add_edge_composite(shared_ptr<tfg_edge_composition_t> const &e)
{
  shared_ptr<tfg_edge const> te = mk_tfg_edge(e, *this);
  this->add_edge(te);
}

expr_ref
tfg::tfg_edge_composition_get_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified) const
{
  context* ctx = this->get_context();
  return this->graph_get_cond_for_edge_composition(src_ec, ctx, simplified, /*substituted*/true);
}

expr_ref
tfg::tfg_edge_composition_get_unsubstituted_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified) const
{
  context* ctx = this->get_context();
  return this->graph_get_cond_for_edge_composition(src_ec, ctx, simplified, /*substituted*/false);
}

expr_ref
tfg::tfg_suffixpath_get_expr_helper(tfg_suffixpath_t const &sfpath) const
{
  autostop_timer ft(__func__);
  return this->tfg_edge_composition_get_unsubstituted_edge_cond(sfpath, true);
}

void
tfg::populate_suffixpath2expr_map() const
{
  m_suffixpath2expr.clear();
  for (auto const& ps : m_suffixpaths) {
    tfg_suffixpath_t const& sfpath = ps.second;
    expr_ref const& sfpath_expr = this->tfg_suffixpath_get_expr(sfpath);
    m_suffixpath2expr.insert(make_pair(sfpath, sfpath_expr));
  }
}

expr_ref
tfg::tfg_suffixpath_get_expr(tfg_suffixpath_t const &sfpath) const
{
  if (!m_suffixpath2expr.count(sfpath)) {
    // this can happen if a suffixpath generated by CG is passed to us
    expr_ref const& sfpath_expr = this->tfg_suffixpath_get_expr_helper(sfpath);
    m_suffixpath2expr.insert(make_pair(sfpath, sfpath_expr));
  }
  ASSERT(m_suffixpath2expr.count(sfpath));
  return m_suffixpath2expr.at(sfpath);
}

state
tfg::tfg_edge_composition_get_to_state(shared_ptr<tfg_edge_composition_t> const &src_ec/*, state const &start_state*/, bool simplified) const
{
  autostop_timer ft(__func__);

  context *ctx = this->get_context();
  if (!src_ec) {
    return state();//start_state;
  }
  ASSERT(src_ec);
  std::function<pair<expr_ref, state> (tfg_edge_ref const &)> f = [ctx, simplified](tfg_edge_ref const &e)
  {
    if (e->is_empty()) {
      return make_pair(expr_true(ctx), state());
    }
    expr_ref const& cond  = simplified ? e->get_simplified_cond()     : e->get_cond();
    state const& to_state = simplified ? e->get_simplified_to_state() : e->get_to_state();
    return make_pair(cond, to_state);
  };
  std::function<pair<expr_ref, state> (tfg_edge_composition_t::type, pair<expr_ref, state> const &, pair<expr_ref, state> const &)> fold_f = [/*&start_state, */simplified, ctx](tfg_edge_composition_t::type ctype, pair<expr_ref, state> const &a, pair<expr_ref, state> const &b)
  {
    if (ctype == tfg_edge_composition_t::series) {
      state ret = b.second;
      ret.state_substitute_variables(/*start_state, */a.second);
      expr_ref const& acond = a.first;
      expr_ref const& bcond = expr_substitute_using_state(b.first/*, start_state*/, a.second);
      expr_ref new_sub_cond = expr_and(acond, bcond);
      if (simplified) {
        ret.state_simplify();
        new_sub_cond = ctx->expr_do_simplify(new_sub_cond);
      }
      return make_pair(new_sub_cond, ret);
    } else if (ctype == tfg_edge_composition_t::parallel) {
      state ret = a.second;
      expr_ref new_sub_cond = expr_or(a.first, b.first);
      ret.merge_state(b.first, b.second/*, start_state*/);
      if (simplified) {
        ret.state_simplify();
        new_sub_cond = ctx->expr_do_simplify(new_sub_cond);
      }
      return make_pair(new_sub_cond, ret);
    } else NOT_REACHED();
  };
  return src_ec->visit_nodes(f, fold_f).second;
}

unordered_set<expr_ref>
tfg::tfg_edge_composition_get_assumes(shared_ptr<tfg_edge_composition_t> const &src_ec) const
{
  if (!src_ec) {
    return unordered_set<expr_ref>();
  }

  context* ctx = this->get_context();
  std::function<predicate_set_t (tfg_edge_ref const&)> pf = [ctx](tfg_edge_ref const& te)
  {
    predicate_set_t ret;
    unordered_set<expr_ref> assumes = te->get_assumes();
    for (auto const& a : assumes) {
      predicate_ref p = predicate::mk_predicate_ref(precond_t(), a, expr_true(ctx), "assumes");
      ret.insert(p);
    }
    return ret;
  };
  predicate_set_t xferred_assume_preds = pth_collect_preds_using_atom_func(src_ec, pf, {}, /*simplified*/false);

  unordered_set<expr_ref> ret;
  for (auto const& p : xferred_assume_preds) {
    expr_ref precond_e = p->get_edge_guard().edge_guard_get_expr(*this, /*simplified*/false);
    ASSERT(p->get_rhs_expr() == expr_true(ctx));
    expr_ref assume_e = expr_implies(precond_e, p->get_lhs_expr());
    ret.insert(assume_e);
  }
  return ret;
}

te_comment_t
tfg::tfg_edge_composition_get_te_comment(shared_ptr<tfg_edge_composition_t> const &src_ec) const
{
  return src_ec->graph_edge_composition_get_comment(*this);
}

bool
tfg::tfg_edge_composition_involves_fcall(shared_ptr<tfg_edge_composition_t> const &src_ec) const
{
  NOT_IMPLEMENTED();
}

map<graph_loc_id_t,expr_ref>
tfg::tfg_edge_composition_get_potentially_written_locs(shared_ptr<tfg_edge_composition_t> const& src_ec) const
{
  autostop_timer ft(__func__);
  typedef pair<expr_ref,map<graph_loc_id_t,expr_ref>> retval_t;

  context* ctx = this->get_context();
  retval_t postcons = make_pair(expr_true(ctx),map<graph_loc_id_t,expr_ref>());
  if (!src_ec || src_ec->is_atom()) {
    return postcons.second;
  }

  //state const& start_state = this->get_start_state();
  std::function<retval_t (tfg_edge_ref const&, retval_t const &postorder_val)> fold_atom_f = [this/*,&start_state*/,ctx](tfg_edge_ref const& e, retval_t const &postorder_val)
  {
    if (e->is_empty()) {
      return postorder_val;
    }
    state const& to_state = e->get_simplified_to_state();

    expr_ref ret_cond = expr_and(e->get_simplified_cond(), expr_substitute_using_state(postorder_val.first/*, start_state*/, to_state));
    map<graph_loc_id_t,expr_ref> ret_locs = this->get_locs_potentially_written_on_edge(e);
    for (const auto &loc_expr : postorder_val.second) {
      expr_ref esub = expr_substitute_using_state(loc_expr.second/*, start_state*/, to_state);
      esub = ctx->expr_do_simplify(esub);
      ret_locs.insert(make_pair(loc_expr.first, esub));
    }
    return make_pair(ret_cond, ret_locs);
  };

  std::function<retval_t (retval_t const &, retval_t const &)> fold_parallel_f =
    [ctx](retval_t const &a, retval_t const &b)
    {
      expr_ref ret_cond = expr_or(a.first, b.first);
      map<graph_loc_id_t,expr_ref> ret_locs;
      for (auto const& loc_expr : b.second) {
        graph_loc_id_t locid = loc_expr.first;
        expr_ref const& ex   = loc_expr.second;

        auto it = ret_locs.find(locid);
        if (it != ret_locs.end()) {
          expr_ref e = expr_ite(b.first, ex, it->second);
          e = ctx->expr_do_simplify(e);
          ret_locs[locid] = e;
        } else {
          ret_locs.insert(make_pair(locid, ex));
        }
      }
      return make_pair(ret_cond, ret_locs);
    };
  return src_ec->template visit_nodes_postorder_series<retval_t>(fold_atom_f, fold_parallel_f, postcons).second;
}

/*pair<bool, expr_ref>
tfg::tfg_edge_composition_get_is_indir_tgt(shared_ptr<tfg_edge_composition_t> const &src_ec)
{
  shared_ptr<tfg_edge> r = src_ec->get_rightmost_atom();
  return make_pair(r->get_is_indir_exit(), r->get_indir_tgt());
}*/

shared_ptr<tfg_edge_composition_t>
tfg::tfg_edge_composition_create_from_path(list<shared_ptr<tfg_edge const>> const &path) const
{
  shared_ptr<tfg_edge_composition_t> ret;
  for (auto e : path) {
    if (!ret) {
      //ret = make_shared<tfg_edge_composition_t>(e, *this);
      ret = mk_edge_composition<pc,tfg_edge>(e);
    } else {
      ASSERT(ret->graph_edge_composition_get_to_pc() == e->get_from_pc());
      //ret = make_shared<tfg_edge_composition_t>(tfg_edge_composition_t::series, ret, make_shared<tfg_edge_composition_t>(e, *this), *this);
      ret = mk_series(ret, mk_edge_composition<pc,tfg_edge>(e));
    }
  }
  return ret;
}

/*void
tfg::eliminate_io_arguments_from_function_calls()
{
  consts_struct_t const &cs = this->get_consts_struct();
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [&cs](pc const &p, const string& key, expr_ref e) -> expr_ref
  {
    return expr_eliminate_io_arguments_from_function_calls(e, cs);
  };
  tfg_visit_exprs(f);
}*/

/*bool
tfg::graph_substitute_using_sprels(map<pc, sprel_map_t> const &sprels_map)
{
  consts_struct_t const &cs = this->get_consts_struct();
  context *ctx = this->get_context();
  state const &start_state = this->get_start_state();
  map<graph_loc_id_t, expr_ref> const &locid2expr_map = m_locid2expr_map;
  bool changed = false;

  std::function<expr_ref (pc const &, const string&, expr_ref)> f = [&sprels_map, &cs, &start_state, &locid2expr_map, &changed, ctx](pc const &p, string const &key, expr_ref e) -> expr_ref
  {
    map<expr_id_t, pair<expr_ref, expr_ref>> submap = sprels_map.at(p).sprel_map_get_submap(locid2expr_map);

    expr_ref ret = ctx->expr_substitute(e, submap);
    if (ret != e) {
      changed = true;
    }
    return ret;
  };
  graph_visit_exprs(f);
  return changed;
}*/

map<pc, sprel_map_t>
tfg::compute_sprel_relations() const
{
  autostop_timer ftimer(__func__);
  consts_struct_t const &cs = this->get_consts_struct();
  map<pc, avail_exprs_t> tfg_avail_exprs = this->compute_loc_avail_exprs();
  DYN_DEBUG2(sprels, print_loc_avail_expr(tfg_avail_exprs));
  map<pc, sprel_map_t> ret;
  set<graph_loc_id_t> arg_locids = this->get_argument_regs_locids();
  for (auto p : this->get_all_pcs()) {
    sprel_map_t sprel_map(this->get_locid2expr_map(), cs);
    sprel_map.set_sprel_mappings(get_sprel_status_mapping_from_avail_exprs(tfg_avail_exprs.at(p), arg_locids));
    ret.insert(make_pair(p, sprel_map));
  }
  return ret;
}

map<graph_loc_id_t, sprel_status_t>
tfg::get_sprel_status_mapping_from_avail_exprs(avail_exprs_t const &avail_exprs, set<graph_loc_id_t> const& arg_locids) const
{
  map<graph_loc_id_t, sprel_status_t> mappings;
  context *ctx = this->get_context();
  consts_struct_t const &cs = this->get_consts_struct();
  bool changed = false;
  do {
    changed = false;
    for (auto const& ave : avail_exprs) {
      expr_ref val = ave.second;
      graph_cp_location const& loc = this->get_loc(ave.first);
      if (   mappings.count(ave.first) == 0
          && !loc.graph_cp_location_represents_hidden_var(ctx)
          && expr_contains_only_constants_or_arguments_or_esp_versions(val, cs, this->get_argument_regs())
          && this->get_context()->expr_contains_only_sprel_compatible_ops(val)) {
        CPP_DBG_EXEC4(TFG_LOCS, cout << __func__ << ':' << __LINE__ << " adding " << ave.first << ": " << expr_string(val) << endl;);
        changed = true;
        mappings[ave.first] = sprel_status_t::constant(val);
      }
    }
  } while (changed);

  return mappings;
}

/*bool
tfg::tfg_edge_composition_edge_cond_evaluates_to_true(shared_ptr<tfg_edge_composition_t const> const &src_ec) const
{
  if (src_ec == NULL) {
    return true;
  }
  cout << __func__ << " " << __LINE__ << ": src_ec = " << src_ec->graph_edge_composition_to_string() << endl;
  list<shared_ptr<serpar_composition_node_t<shared_ptr<tfg_edge>> const>> pterms = serpar_composition_node_t<shared_ptr<tfg_edge>>::serpar_composition_get_parallel_terms(src_ec);
  set<shared_ptr<tfg_edge>> incident_edges;
  for (auto pterm : pterms) {
    shared_ptr<tfg_edge_composition_t const> ptermg = dynamic_pointer_cast<tfg_edge_composition_t const>(pterm);
    list<shared_ptr<serpar_composition_node_t<shared_ptr<tfg_edge>> const>> terms = serpar_composition_node_t<shared_ptr<tfg_edge>>::serpar_composition_get_serial_terms(ptermg);
    list<shared_ptr<serpar_composition_node_t<shared_ptr<tfg_edge>> const>>::const_iterator iter;
    iter = terms.begin();
    ASSERT(iter != terms.end());
    shared_ptr<tfg_edge_composition_t const> incident_ec = dynamic_pointer_cast<tfg_edge_composition_t const>(*iter);
    if (incident_ec->is_atom()) {
      shared_ptr<tfg_edge> incident_edge = incident_ec->get_atom();
      incident_edges.insert(incident_edge);
      iter++;
    }
    for (; iter != terms.end(); iter++) {
      shared_ptr<tfg_edge_composition_t const> ec = dynamic_pointer_cast<tfg_edge_composition_t const>(*iter);
      if (!tfg_edge_composition_edge_cond_evaluates_to_true(ec)) {
        //cout << __func__ << " " << __LINE__ << ": returning false for " << src_ec->graph_edge_composition_to_string() << endl;
        return false;
      }
    }
  }
  pc from_pc = src_ec->graph_edge_composition_get_from_pc();
  list<shared_ptr<tfg_edge>> outgoing;
  this->get_edges_outgoing(from_pc, outgoing);
  for (const auto &e : outgoing) {
    if (!set_belongs(incident_edges, e)) {
      //cout << __func__ << " " << __LINE__ << ": returning false for " << src_ec->graph_edge_composition_to_string() << endl;
      return false;
    }
  }
  //cout << __func__ << " " << __LINE__ << ": returning true for " << src_ec->graph_edge_composition_to_string() << endl;
  return true;
}*/

pair<bool, list<shared_ptr<tfg_edge const>>>
tfg::tfg_edge_composition_represents_incoming_edges_at_to_pc(shared_ptr<tfg_edge_composition_t> const &src_ec, list<shared_ptr<tfg_edge const>> const &required_incoming_edges_at_to_pc) const
{
  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> pterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_parallel_terms(src_ec);
  set<shared_ptr<tfg_edge_composition_t const>> incident_ecs;
  set<tfg_edge_ref> incident_edges;
  for (auto pterm : pterms) {
    shared_ptr<tfg_edge_composition_t const> ptermg = dynamic_pointer_cast<tfg_edge_composition_t const>(pterm);
    list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> terms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_serial_terms(ptermg);
    list<shared_ptr<serpar_composition_node_t<tfg_edge> const>>::const_reverse_iterator iter;
    iter = terms.rbegin();
    ASSERT(iter != terms.rend());
    shared_ptr<tfg_edge_composition_t const> incident_ec = dynamic_pointer_cast<tfg_edge_composition_t const>(*iter);
    if (incident_ec->is_atom()) {
      tfg_edge_ref incident_edge = incident_ec->get_atom();
      incident_edges.insert(incident_edge);
    } else {
      incident_ecs.insert(incident_ec);
    }
    for (iter++; iter != terms.rend(); iter++) {
      shared_ptr<tfg_edge_composition_t const> ec = dynamic_pointer_cast<tfg_edge_composition_t const>(*iter);
      if (!tfg_edge_composition_represents_all_possible_paths(ec)) {
        //cout << __func__ << " " << __LINE__ << ": returning false for " << src_ec->graph_edge_composition_to_string() << endl;
        return make_pair(false, list<shared_ptr<tfg_edge const>>());
      }
    }
  }
  list<shared_ptr<tfg_edge const>> remaining_edges;
  for (const auto &e : required_incoming_edges_at_to_pc) {
    if (!set_belongs(incident_edges, e)) {
      //cout << __func__ << " " << __LINE__ << ": returning false for " << src_ec->graph_edge_composition_to_string() << endl;
      remaining_edges.push_back(e);
    }
  }
  //cout << __func__ << " " << __LINE__ << ": returning true for " << src_ec->graph_edge_composition_to_string() << endl;
  for (const auto &ec : incident_ecs) {
    pair<bool, list<shared_ptr<tfg_edge const>>> ecret = tfg_edge_composition_represents_incoming_edges_at_to_pc(ec, remaining_edges);
    if (!ecret.first) {
      //cout << __func__ << " " << __LINE__ << ": returning false for " << src_ec->graph_edge_composition_to_string() << endl;
      return make_pair(false, list<shared_ptr<tfg_edge const>>());
    }
    remaining_edges = ecret.second;
  }
  return make_pair(true, remaining_edges);
}

bool
tfg::tfg_edge_composition_represents_all_possible_paths(shared_ptr<tfg_edge_composition_t> const &src_ec) const
{
  if (src_ec == NULL) {
    return true;
  }
  //cout << __func__ << " " << __LINE__ << ": src_ec = " << src_ec->graph_edge_composition_to_string() << endl;
  pc to_pc = src_ec->graph_edge_composition_get_to_pc();
  list<shared_ptr<tfg_edge const>> incoming = this->get_edges_incoming_unreachable(to_pc);
  pair<bool, list<shared_ptr<tfg_edge const>>> ecret = this->tfg_edge_composition_represents_incoming_edges_at_to_pc(src_ec, incoming);
  if (!ecret.first) {
    //cout << __func__ << " " << __LINE__ << ": returning false for " << src_ec->graph_edge_composition_to_string() << endl;
    return false;
  }
  bool ret = (ecret.second.size() == 0);
  //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for " << src_ec->graph_edge_composition_to_string() << endl;
  return ret;
}

/*
 * tfg_edge_composition_implies() is over-approximate: if it returns TRUE, implication is provable. if it returns FALSE, that just means that
 * we were unable to determine implication even if it might exist. An example where we may incorrectly return FALSE:
 *    a :  0->1, 0->2, 1->3, 2->3, 3->4
 *    b :  0->1, 1->3, 3->4
 * In this case the serial terms would be:
 *    a: 0->3, 3->4
 *    b: 0->1, 1->3, 3->4
 * list_contains_sequence() will return false in this case!
 */
bool
tfg::tfg_edge_composition_implies(shared_ptr<tfg_edge_composition_t> const &a, shared_ptr<tfg_edge_composition_t> const &b)
{
  if (a == b) {
    return true;
  }

  if (!b || graph_edge_composition_is_epsilon(b)) {
    return true;
  }
  else if (!a || graph_edge_composition_is_epsilon(a)) {
    return false;
  }

  // XXX using global context
  if (   g_ctx != nullptr
      && !g_ctx->disable_caches()) {
    auto& cache = g_gsupport_cache.m_tfg_edge_composition_implies;
    auto p = make_pair(a,b);
    if (cache.lookup(p))
      return cache.get(p);
  }

  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> aterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_serial_terms(a);
  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> bterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_serial_terms(b);

  std::function<bool (shared_ptr<serpar_composition_node_t<tfg_edge> const> const &, shared_ptr<serpar_composition_node_t<tfg_edge> const> const &)> f_contains = [](shared_ptr<serpar_composition_node_t<tfg_edge> const> const &a, shared_ptr<serpar_composition_node_t<tfg_edge> const> const &b)
  {
    shared_ptr<graph_edge_composition_t<pc, tfg_edge> const> const &ag = dynamic_pointer_cast<graph_edge_composition_t<pc, tfg_edge> const>(a);
    shared_ptr<graph_edge_composition_t<pc, tfg_edge> const> const &bg = dynamic_pointer_cast<graph_edge_composition_t<pc, tfg_edge> const>(b);
    if (ag->graph_edge_composition_get_from_pc() != bg->graph_edge_composition_get_from_pc()) {
      return false;
    }
    if (ag->graph_edge_composition_get_to_pc() != bg->graph_edge_composition_get_to_pc()) {
      return false;
    }
    return tfg_edge_composition_implies(ag, bg);
  };

  bool ret;
  if (aterms.size() < bterms.size()) {
    ret = false;
    goto update_cache;
  } else if (aterms.size() == 1) {
    ASSERT(bterms.size() == 1);
    list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> a_pterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_parallel_terms(a);
    list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> b_pterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_parallel_terms(b);
    if (a_pterms.size() == 1 && b_pterms.size() == 1) {
      ret = *a_pterms.begin() == *b_pterms.begin();
      goto update_cache;
    }
    for (const auto &a_pterm : a_pterms) {
      shared_ptr<tfg_edge_composition_t> const &a_ptermg = dynamic_pointer_cast<tfg_edge_composition_t>(a_pterm);
      bool found = false;
      for (const auto &b_pterm : b_pterms) {
        shared_ptr<tfg_edge_composition_t> const &b_ptermg = dynamic_pointer_cast<tfg_edge_composition_t>(b_pterm);
        if (tfg_edge_composition_implies(a_ptermg, b_ptermg)) {
          found = true;
          break;
        }
      }
      if (!found) {
        ret = false;
        goto update_cache;
      }
    }
    ret = true;
    goto update_cache;
  } else {
    ret = sequence_is_prefix_of_list(aterms, bterms, f_contains);
    goto update_cache;
  }

update_cache:
  if (   g_ctx != nullptr
      && !g_ctx->disable_caches()) {
    auto p = make_pair(a,b);
    g_gsupport_cache.m_tfg_edge_composition_implies.add(p,ret);
  }
  return ret;
}

bool
tfg::tfg_suffixpath_implies(shared_ptr<tfg_edge_composition_t> const &a, shared_ptr<tfg_edge_composition_t> const &b)
{
  autostop_timer ft(__func__);
  return tfg_edge_composition_implies(a, b);
}


shared_ptr<tfg_edge_composition_t>
tfg::tfg_edge_composition_conjunct(shared_ptr<tfg_edge_composition_t> const &a, shared_ptr<tfg_edge_composition_t> const &b) const
{
  ASSERT(a->graph_edge_composition_get_from_pc() == b->graph_edge_composition_get_from_pc());
  if (a->graph_edge_composition_get_to_pc() != b->graph_edge_composition_get_to_pc()) {
    cout << __func__ << " " << __LINE__ << ": a = " << a->graph_edge_composition_to_string() << endl;
    cout << __func__ << " " << __LINE__ << ": b = " << b->graph_edge_composition_to_string() << endl;
  }
  ASSERT(a->graph_edge_composition_get_to_pc() == b->graph_edge_composition_get_to_pc());

  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> aterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_serial_terms(a);
  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> bterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_serial_terms(b);

  /*std::function<shared_ptr<tfg_edge_composition_t> (shared_ptr<serpar_composition_node_t<shared_ptr<tfg_edge>> const> const &, shared_ptr<serpar_composition_node_t<shared_ptr<tfg_edge>> const> const &)> f_contains = [this](shared_ptr<serpar_composition_node_t<shared_ptr<tfg_edge>> const> const &a, shared_ptr<serpar_composition_node_t<shared_ptr<tfg_edge>> const> const &b)
    {
    shared_ptr<graph_edge_composition_t<pc, tfg_node, tfg_edge> const> const &ag = dynamic_pointer_cast<graph_edge_composition_t<pc, tfg_node, tfg_edge> const>(a);
    shared_ptr<graph_edge_composition_t<pc, tfg_node, tfg_edge> const> const &bg = dynamic_pointer_cast<graph_edge_composition_t<pc, tfg_node, tfg_edge> const>(b);
    ASSERT(ag->graph_edge_composition_get_from_pc() == bg->graph_edge_composition_get_from_pc());
    ASSERT(ag->graph_edge_composition_get_to_pc() == bg->graph_edge_composition_get_to_pc());
    return this->tfg_edge_composition_conjunct(ag, bg);
    };*/

  ASSERT(aterms.size() >= 1);
  ASSERT(bterms.size() >= 1);

  if (aterms.size() == 1 && bterms.size() == 1) {
    list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> a_pterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_parallel_terms(a);
    list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> b_pterms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_parallel_terms(b);
    if (a_pterms.size() == 1 && b_pterms.size() == 1) {
      shared_ptr<tfg_edge_composition_t> const &a_ptermg = dynamic_pointer_cast<tfg_edge_composition_t>(*a_pterms.begin());
      shared_ptr<tfg_edge_composition_t> const &b_ptermg = dynamic_pointer_cast<tfg_edge_composition_t>(*b_pterms.begin());
      if (a_ptermg == b_ptermg) {
        return a_ptermg;
      } else {
        return nullptr;
      }
    } else {
      list<shared_ptr<tfg_edge_composition_t>> new_pterms;
      for (const auto &a_pterm : a_pterms) {
        shared_ptr<tfg_edge_composition_t> const &a_ptermg = dynamic_pointer_cast<tfg_edge_composition_t>(a_pterm);
        bool found_new_pterm = false;
        for (const auto &b_pterm : b_pterms) {
          shared_ptr<tfg_edge_composition_t> const &b_ptermg = dynamic_pointer_cast<tfg_edge_composition_t>(b_pterm);
          shared_ptr<tfg_edge_composition_t> ret = this->tfg_edge_composition_conjunct(a_ptermg, b_ptermg);
          if (ret) {
            found_new_pterm = true;
            new_pterms.push_back(ret);
          }
        }
        if (!found_new_pterm) {
          return nullptr;
        }
      }
      return graph_edge_composition_construct_edge_from_parallel_edgelist(new_pterms);
    }
  }

  list<shared_ptr<tfg_edge_composition_t>> new_terms;
  for (const auto &aterm : aterms) {
    shared_ptr<tfg_edge_composition_t> const &atermg = dynamic_pointer_cast<tfg_edge_composition_t>(aterm);
    bool found_new_term = false;
    for (const auto &bterm : bterms) {
      shared_ptr<tfg_edge_composition_t> const &btermg = dynamic_pointer_cast<tfg_edge_composition_t>(bterm);
      if (atermg->graph_edge_composition_get_from_pc() != btermg->graph_edge_composition_get_from_pc()) {
        continue;
      }
      if (atermg->graph_edge_composition_get_to_pc() != btermg->graph_edge_composition_get_to_pc()) {
        return nullptr; //trying to conjunct x->y and x->z will return nullptr
      }
      shared_ptr<tfg_edge_composition_t> ret = this->tfg_edge_composition_conjunct(atermg, btermg);
      if (ret) {
        found_new_term = true;
        new_terms.push_back(ret);
        break;
      } else {
        return nullptr;
      }
    }
    if (!found_new_term) {
      return nullptr;
    }
  }
  return graph_edge_composition_construct_edge_from_serial_edgelist(new_terms);
}

callee_summary_t
tfg::get_callee_summary_for_nextpc(nextpc_id_t nextpc_id, size_t num_fargs) const
{
  autostop_timer func_timer(__func__);
  //ASSERT(m_nextpc_map.count(nextpc_num));
  //string nextpc_str = m_nextpc_map.at(nextpc_num);
  //cout << __func__ << " " << __LINE__ << ": nextpc_str = " << nextpc_str << endl;
  if (!m_callee_summaries.count(nextpc_id)) {
    set<cs_addr_ref_id_t> const &relevant_addr_refs = this->graph_get_relevant_addr_refs();
    consts_struct_t const &cs = this->get_consts_struct();
    set<cs_addr_ref_id_t> relevant_addr_refs_minus_memlocs = lr_status_t::subtract_memlocs_from_addr_ref_ids(relevant_addr_refs, this->get_symbol_map(), this->get_context());
    callee_summary_t ret = callee_summary_t::callee_summary_bottom(cs, relevant_addr_refs_minus_memlocs, this->get_symbol_map(), this->get_locals_map(), num_fargs);
    //cout << __func__ << " " << __LINE__ << ": ret = " << ret.callee_summary_to_string_for_eq() << endl;
    return ret;
  }
  //cout << __func__ << " " << __LINE__ << ": nextpc_id = " << nextpc_num << endl;
  //cout << __func__ << " " << __LINE__ << ": nextpc_str = " << nextpc_str << endl;
  return m_callee_summaries.at(nextpc_id);
}

void
tfg::set_symbol_map_for_touched_symbols(graph_symbol_map_t const &symbol_map, set<symbol_id_t> const &touched_symbols)
{
  this->get_symbol_map().clear();
  for (auto sid : touched_symbols) {
    //if (sid == SYMBOL_ID_DST_RODATA) {
    //  continue;
    //}
    if (!symbol_map.count(sid)) {
      cout << __func__ << " " << __LINE__ << ": sid = " << sid << endl;
    }
    this->get_symbol_map().insert(make_pair(sid, symbol_map.at(sid)));
  }
  //ASSERT(!m_symbol_map.count(SYMBOL_ID_DST_RODATA));
  //m_symbol_map.insert(make_pair(SYMBOL_ID_DST_RODATA, graph_symbol_t(mk_string_ref(SYMBOL_ID_DST_RODATA_NAME), SYMBOL_ID_DST_RODATA_SIZE, ALIGNMENT_FOR_RODATA_SYMBOL, true)));
}

void
tfg::set_string_contents_for_touched_symbols_at_zero_offset(map<pair<symbol_id_t, offset_t>, vector<char>> const &string_contents, set<symbol_id_t> const &touched_symbols)
{
  m_string_contents.clear();
  for (auto sid : touched_symbols) {
    //cout << __func__ << " " << __LINE__ << ": sid = " << sid << ", string_contents.count(sid) = " << string_contents.count(sid) << ", string_contents.size() = " << string_contents.size() << endl;
    if (string_contents.count(make_pair(sid, 0))) {
      m_string_contents[make_pair(sid, 0)] = string_contents.at(make_pair(sid, 0));
    }
  }
}

/*void
tfg::substitute_symbols_using_rodata_map(rodata_map_t const &rodata_map)
{
  //map<expr_id_t, pair<expr_ref, expr_ref>> rename_submap;
  //context *ctx = this->get_context();
  //for (auto rm : rodata_map) {
  //  update_symbol_rename_submap(rename_submap, rm.first, SYMBOL_ID_DST_RODATA, rm.second, ctx);
  //}
  //this->substitute_variables(rename_submap);
  map<expr_id_t, pair<expr_ref, expr_ref>> rodata_submap;
  rodata_submap = rodata_map.rodata_map_get_submap();
  this->substitute_variables(rodata_submap);
}*/

/*unordered_set<predicate>
tfg::get_sprel_preds(pc const &p) const
{
  ASSERT(m_sprels.count(p));
  return m_sprels.at(p).sprel_map_get_assumes(this->get_locid2expr_map());
}*/

void
input_exec_scan_rodata_region_and_populate_rodata_map(struct input_exec_t const *in, int input_section_index, char const *contents, size_t contents_len, rodata_map_t *rodata_map, symbol_id_t llvm_string_symbol_id, map<symbol_or_section_id_t, symbol_id_t> const& rename_submap)
{
  src_ulong j;
  //cout << __func__ << " " << __LINE__ << ": addr = " << addr << endl;
  input_section_t const& isec = in->input_sections[input_section_index];

  char const* name = isec.name;
  src_ulong addr = isec.addr;
  src_ulong addr_contents_len = isec.size;

  //cout << __func__ << " " << __LINE__ << ": addr = " << addr << endl;
  //cout << __func__ << " " << __LINE__ << ": addr_contents_len = " << addr_contents_len << endl;
  //cout << __func__ << " " << __LINE__ << ": isec->addr = " << isec->addr << endl;
  //cout << __func__ << " " << __LINE__ << ": isec->size = " << isec->size << endl;
  //cout << __func__ << " " << __LINE__ << ": isec->addr + isec->size = " << isec->addr + isec->size << endl;
  for (j = addr; j < addr + addr_contents_len/*isec->addr + isec->size*/; j++) {
    if (!memcmp(&isec.data[j - isec.addr], contents, contents_len)) {
      //*ret_offset = j - addr;
      //return true;
      symbol_or_section_id_t ssid(symbol_or_section_id_t::section, input_section_index);
      ASSERT(rename_submap.count(ssid));
      rodata_map->rodata_map_add_mapping(llvm_string_symbol_id, rodata_offset_t(rename_submap.at(ssid), j - isec.addr));
    }
  }
}

void
tfg::tfg_assign_mlvars_to_memlabels()
{
  this->get_memlabel_map_ref().clear();
  long mlvarnum = 0;
  string tfg_name = this->get_name()->get_str();
  map<pc, pair<mlvarname_t, mlvarname_t>> fcall_mlvars;
  map<pc, map<expr_ref, mlvarname_t>> expr_mlvars;
  std::function<expr_ref (pc const &, const string&, expr_ref)> f = [&fcall_mlvars, &expr_mlvars, &tfg_name, &mlvarnum](pc const &p, string const & key, expr_ref e) -> expr_ref
  {
    return expr_assign_mlvars_to_memlabels(e, fcall_mlvars, expr_mlvars, tfg_name, mlvarnum, p);
  };
  tfg_visit_exprs(f);
  this->m_max_mlvarnum = mlvarnum;
}

void
tfg::compute_max_memlabel_varnum()
{
  this->m_max_mlvarnum = this->get_memlabel_map().size();
}

//list<expr_ref>
//void
//tfg::resize_farg_exprs_for_function_calling_conventions(/*expr_ref e*/)
//{
//  NOT_REACHED();
//  //context *ctx = this->get_context();
//  //std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [ctx](pc const &p, const string& key, expr_ref e) -> expr_ref
//  //{
//  //  return ctx->expr_resize_farg_expr_for_function_calling_conventions(e);
//  //};
//  //tfg_visit_exprs(f);
//}

void
tfg::add_transmap_translation_edges_at_entry_and_exit(transmap_t const *in_tmap, transmap_t const *out_tmaps, int num_out_tmaps)
{
  NOT_IMPLEMENTED();
}

void
tfg::tfg_populate_arguments_using_live_regset(regset_t const &live_in)
{
  //cout << __func__ << " " << __LINE__ << ": live_in = " << regset_to_string(&live_in, as1, sizeof as1) << endl;
  context *ctx = this->get_context();
  //state const &start_state = this->get_start_state();
  map<string_ref, expr_ref> argument_regs;
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  for (const auto &g : live_in.excregs) {
    for (const auto &r : g.second) {
      stringstream ss1, ss2;
      ss1 << "exvr" << g.first << "." << r.first.reg_identifier_to_string();
      ss2 << G_INPUT_KEYWORD << "." << G_SRC_KEYWORD << "." << G_REGULAR_REG_NAME << g.first << "." << r.first.reg_identifier_to_string();
      size_t argsize = ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS);
      expr_ref reg_expr = ctx->mk_var(ss2.str(), ctx->mk_bv_sort(argsize));
      //expr_ref arg_expr = ctx->mk_var(ss1.str() + SRC_INPUT_ARG_NAME_SUFFIX, ctx->mk_bv_sort(argsize));
      argument_regs[mk_string_ref(ss1.str())] = reg_expr;
      //argument_regs[ss1.str()] = arg_expr;
      //cout << __func__ << " " << __LINE__ << ": reg_expr = " << expr_string(reg_expr) << endl;
      //cout << __func__ << " " << __LINE__ << ": arg_expr = " << expr_string(arg_expr) << endl;
      //submap[reg_expr->get_id()] = make_pair(reg_expr, arg_expr);
    }
  }
  string indir_tgt_key = G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID;
  expr_ref indir_tgt_expr = ctx->mk_var(string(G_INPUT_KEYWORD ".") + indir_tgt_key, ctx->mk_bv_sort(DWORD_LEN));
  argument_regs[mk_string_ref(indir_tgt_key)] = indir_tgt_expr;
  this->set_argument_regs(argument_regs);
  //list<shared_ptr<tfg_edge>> out;
  //this->get_edges_outgoing(pc::start(), out);
  //ASSERT(out.size() == 1);
  //shared_ptr<tfg_edge> e = *out.begin();
  //cout << __func__ << " " << __LINE__ << ": live_in = " << regset_to_string(&live_in, as1, sizeof as1) << endl;
  //e->substitute_variables_at_input(start_state, submap, ctx);
}

void
tfg::tfg_rename_srcdst_identifier(string const &from_id, string const &to_id)
{
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  map<string, string> keys_map;
  string from_pattern = from_id + ".";
  string to_pattern = to_id + ".";
  for (const auto &kv : this->get_start_state().get_value_expr_map_ref()) {
    string const &key = kv.first->get_str();
    expr_ref const &e = kv.second;
    context *ctx = e->get_context();
    string new_key = key;
    string_replace(new_key, from_pattern, to_pattern);
    string new_e_name = string(G_INPUT_KEYWORD ".") + new_key;
    submap[e->get_id()] = make_pair(e, ctx->mk_var(new_e_name, e->get_sort()));
    keys_map[key] = new_key;
  }
  this->rename_keys(keys_map, submap);
  this->tfg_substitute_variables(submap);
}

void
tfg::append_to_input_keyword(string const &keyword)
{
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  map<string, string> keys_map;
  for (const auto &kv : this->get_start_state().get_value_expr_map_ref()) {
    string const &key = kv.first->get_str();
    expr_ref const &e = kv.second;
    context *ctx = e->get_context();
    string new_key = keyword + "." + key;
    string new_e_name = string(G_INPUT_KEYWORD ".") + new_key;
    submap[e->get_id()] = make_pair(e, ctx->mk_var(new_e_name, e->get_sort()));
    keys_map[key] = new_key;
  }
  //tfg ret = *this;
  this->rename_keys(keys_map, submap);
  this->tfg_substitute_variables(submap);
  //this->populate_auxilliary_structures_dependent_on_locs();
  //cout << __func__ << " " << __LINE__ << ": keyword = " << keyword << ": this=\n" << this->graph_to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": keyword = " << keyword << ": ret =\n" << ret.graph_to_string() << endl;
  //return ret;
}

void
tfg::rename_keys(map<string, string> const &key_map, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap)
{
  map<string_ref, expr_ref> new_value_expr_map;
  context *ctx = this->get_context();
  for (auto &kv : this->get_start_state().get_value_expr_map_ref()) {
    expr_ref e = kv.second;
    context *ctx = e->get_context();
    e = ctx->expr_substitute(e, submap);
    new_value_expr_map[kv.first] = e;
  }
  this->get_start_state().set_value_expr_map(new_value_expr_map);
  this->get_start_state().state_rename_keys(key_map);
  for (const auto &p : this->get_all_pcs()) {
    if (!p.is_exit()) {
      continue;
    }
    map<string_ref, expr_ref> const &retregs_at_pc = this->get_return_regs().at(p);
    map<string_ref, expr_ref> new_retregs_at_pc;
    for (const auto &ee : retregs_at_pc) {
      //string new_key = key_map.count(ee.first) ? key_map.at(ee.first) : ee.first;
      //new_retregs_at_pc[new_key] = ctx->expr_substitute(ee.second, submap);
      new_retregs_at_pc[ee.first] = ctx->expr_substitute(ee.second, submap);
    }
    this->set_return_regs_at_pc(p, new_retregs_at_pc);
  }
  set<shared_ptr<tfg_edge const>> es = this->get_edges();
  std::function<void (pc const&, state&)> fstate = [&key_map](pc const&, state& st)
  {
    st.state_rename_keys(key_map);
  };
  for (auto e : es) {
    //e->get_to_state().state_rename_keys(key_map);
    auto new_e = e->tfg_edge_update_state(fstate);
    this->remove_edge(e);
    this->add_edge(new_e);
  }
  auto locs = this->get_locs();
  for (auto &ll : locs) {
    auto &loc = ll.second;
    if (   loc.m_type == GRAPH_CP_LOCATION_TYPE_REGMEM
        || loc.m_type == GRAPH_CP_LOCATION_TYPE_LLVMVAR) {
      if (key_map.count(loc.m_varname->get_str())) {
        loc.m_varname = mk_string_ref(key_map.at(loc.m_varname->get_str()));
        NOT_IMPLEMENTED(); //rename m_var
      }
    } else if (loc.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
      if (key_map.count(loc.m_memname->get_str())) {
        loc.m_memname = mk_string_ref(key_map.at(loc.m_memname->get_str()));
      }
    } else if (loc.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
      if (key_map.count(loc.m_memname->get_str())) {
        loc.m_memname = mk_string_ref(key_map.at(loc.m_memname->get_str()));
      }
    }
  }
  this->set_locs(locs);
}

static size_t
compute_reg_len_from_bitmap(uint64_t bitmap)
{
  int ret = 0;
  for (int i = 0; i < 64; i++) {
    if (bitmap & (1ULL << i)) {
      ret = i + 1;
    }
  }
  return ret;
}

map<pc, map<string_ref, expr_ref>>
get_return_reg_map_from_live_out(regset_t const *live_out, size_t num_live_out, context *ctx)
{
  map<pc, map<string_ref, expr_ref>> ret;
  for (size_t i = 0; i < num_live_out; i++) {
    pc exit_pc(pc::exit, int_to_string(i).c_str(), PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);
    stringstream ss;
    ss << G_INPUT_KEYWORD << "." << G_SRC_KEYWORD << "." << LLVM_MEM_SYMBOL;
    ret[exit_pc][mk_string_ref(G_HEAP_KEYWORD)] = ctx->mk_memmask(ctx->mk_var(ss.str(), ctx->mk_array_sort(ctx->mk_bv_sort(DWORD_LEN), ctx->mk_bv_sort(BYTE_LEN))), memlabel_t::memlabel_heap());
    ret[exit_pc][mk_string_ref(G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID)] = ctx->mk_var(G_INPUT_KEYWORD "." G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID, ctx->mk_bv_sort(DWORD_LEN));
    for (const auto &g : live_out->excregs) {
      for (const auto &r : g.second) {
        stringstream ss1, ss2;
        ss1 << "exvr" << g.first << "." << r.first.reg_identifier_to_string();
        string name = ss1.str();
        ss2 << G_INPUT_KEYWORD << "." << G_SRC_KEYWORD << "." << G_REGULAR_REG_NAME << g.first << "." << r.first.reg_identifier_to_string();
        string expr_name = ss2.str();
        size_t etfg_reg_len = ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS);
        size_t reg_len = compute_reg_len_from_bitmap(r.second);
        expr_ref e = ctx->mk_bvextract(ctx->mk_var(expr_name, ctx->mk_bv_sort(etfg_reg_len)), reg_len - 1, 0);
        ret[exit_pc][mk_string_ref(name)] = e;
      }
    }
  }
  return ret;
}

void
tfg::tfg_introduce_src_dst_prefix(char const *prefix)
{
  list<shared_ptr<tfg_edge const>> edges;
  this->get_edges(edges);

  std::function<void (pc const&, state&)> fstate = [prefix](pc const&, state& st)
  {
    st.state_introduce_src_dst_prefix(prefix, true);
  };
  std::function<expr_ref (pc const&, expr_ref const&)> fcond = [prefix](pc const&, expr_ref const& e)
  {
    return expr_introduce_src_dst_prefix(e, prefix);
  };

  for (const auto &e : edges) {
    pc const &p = e->get_from_pc();
    auto new_e = e->tfg_edge_update_state(fstate);
    new_e = new_e->tfg_edge_update_edgecond(fcond);
    this->remove_edge(e);
    this->add_edge(new_e);
  }
}

/*void
tfg_edge::tfg_edge_truncate_register_sizes_using_regset(regset_t const &regset)
{
  get_to_state().state_truncate_register_sizes_using_regset(regset);
}*/

bool
tfg::symbol_represents_memloc(symbol_id_t symbol_id) const
{
  ASSERT(this->get_symbol_map().count(symbol_id));
  return this->get_symbol_map().at(symbol_id).get_name()->get_str().find(PEEPTAB_MEM_PREFIX) == 0;
}

bool
tfg::memlabel_is_memloc_symbol(memlabel_t const &ml) const
{
  if (!memlabel_t::memlabel_represents_symbol(&ml)) {
    return false;
  }
  symbol_id_t symbol_id = ml.memlabel_get_symbol_id();
  return this->symbol_represents_memloc(symbol_id);
}

sprel_map_pair_t
tfg::get_sprel_map_pair(pc const &p) const
{
  sprel_map_t const &sprel_map = get_sprel_map(p);
  return sprel_map_pair_t(sprel_map);
}

void
tfg::tfg_visit_exprs_full(std::function<expr_ref (pc const &, const string&, expr_ref)> f)
{
  this->tfg_visit_exprs(f);

  //visit return regs;
  auto return_regs = this->get_return_regs();
  list<shared_ptr<tfg_node>> nodes;
  this->get_nodes(nodes);
  for (auto n : nodes) {
    pc const &p = n->get_pc();
    if (!p.is_exit()) {
      continue;
    }
    if (return_regs.count(p) == 0) {
      continue;
    }
    for (auto &pr : return_regs.at(p)) {
      expr_ref const &e = pr.second;
      pr.second = f(p, G_TFG_OUTPUT_IDENTIFIER, e);
    }
  }
  this->set_return_regs(return_regs);

  //visit dependent expressions.
}



void
tfg::tfg_update_symbol_map_and_rename_expressions_at_zero_offset(graph_symbol_map_t const &new_symbol_map)
{
  context *ctx = this->get_context();
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  map<string, string> symbol_name_map;
  map<symbol_id_t, symbol_id_t> symbol_id_map;
  map<pair<symbol_id_t, offset_t>, vector<char>> new_string_contents;
  for (const auto &nsym : new_symbol_map.get_map()) {
    symbol_id_t found = -1;
    for (const auto &sym : this->get_symbol_map().get_map()) {
      if (sym.second.get_name() == nsym.second.get_name()) {
        ASSERT(sym.second.get_size() == nsym.second.get_size());
        ASSERT(sym.second.get_alignment() == nsym.second.get_alignment());
        ASSERT(sym.second.is_const() == nsym.second.is_const());
        found = sym.first;
        break;
      }
    }
    ASSERT(found != -1);
    size_t symbol_size = this->get_symbol_size_from_id(found);
    stringstream ss1, ss2;
    ss1 << G_SYMBOL_KEYWORD "." << found;// << "." << symbol_size;
    ss2 << G_SYMBOL_KEYWORD "." << nsym.first;// << "." << symbol_size;
    string sym1_name = ss1.str();
    string sym2_name = ss2.str();
    symbol_name_map[sym1_name] = sym2_name;
    symbol_id_map[found] = nsym.first;
    //cout << __func__ << " " << __LINE__ << ": sym1_name->sym2name = " << sym1_name << " --> " << sym2_name << endl;
    expr_ref from = ctx->mk_var(sym1_name, ctx->mk_bv_sort(DWORD_LEN));
    expr_ref to = ctx->mk_var(sym2_name, ctx->mk_bv_sort(DWORD_LEN));
    submap[from->get_id()] = make_pair(from, to);
    if (this->get_string_contents().count(make_pair(found, 0))) {
      new_string_contents[make_pair(nsym.first, 0)] = this->get_string_contents().at(make_pair(found, 0));
    }
  }
  this->tfg_substitute_variables(submap);
  this->set_symbol_map(new_symbol_map);
  this->set_string_contents(new_string_contents);
  auto return_regs = this->get_return_regs();
  for (auto &psym : return_regs) {
    map<string_ref, expr_ref> new_return_regs_at_pc;
    for (auto &sym : psym.second) {
      bool found = false;
      for (const auto &m : symbol_name_map) {
        if (string_has_prefix(sym.first->get_str(), m.first + ".")) {
          string new_sym = sym.first->get_str();
          //cout << __func__ << " " << __LINE__ << ": before replace, new_sym = " << new_sym << endl;
          //cout << __func__ << " " << __LINE__ << ": m.first = " << m.first << endl;
          //cout << __func__ << " " << __LINE__ << ": m.second = " << m.second << endl;
          string_replace(new_sym, m.first + ".", m.second + ".");
          //cout << __func__ << " " << __LINE__ << ": after replace, new_sym = " << new_sym << endl;
          new_return_regs_at_pc[mk_string_ref(new_sym)] = sym.second;
          //cout << __func__ << " " << __LINE__ << ": new_sym = " << new_sym << endl;
          found = true;
          break;
        }
      }
      if (!found) {
        new_return_regs_at_pc.insert(sym);
      }
    }
    psym.second = new_return_regs_at_pc;
  }
  this->set_return_regs(return_regs);
  std::function<expr_ref (pc const &, string const &, expr_ref const &)> f = [ctx, &symbol_id_map](pc const &, string const &, expr_ref const &e)
  {
    expr_ref ret = ctx->expr_rename_memlabels(e, alias_region_t::alias_type_symbol, symbol_id_map);
    ret = ctx->expr_rename_memlabel_vars_prefix(ret, G_MEMLABEL_VAR_PREFIX G_LLVM_PREFIX, G_MEMLABEL_VAR_PREFIX G_DST_PREFIX);
    ret = ctx->expr_rename_memlabel_vars_prefix(ret, G_MEMLABEL_VAR_PREFIX_FOR_FCALL G_LLVM_PREFIX, G_MEMLABEL_VAR_PREFIX_FOR_FCALL G_DST_PREFIX);
    return ret;
  };
  this->tfg_visit_exprs_full(f);
  auto memlabel_map = this->get_memlabel_map();
  graph_memlabel_map_t new_memlabel_map;
  for (const auto &m : memlabel_map) {
    string name = m.first->get_str();
    string_replace(name, G_MEMLABEL_VAR_PREFIX G_LLVM_PREFIX, G_MEMLABEL_VAR_PREFIX G_DST_PREFIX);
    string_replace(name, G_MEMLABEL_VAR_PREFIX_FOR_FCALL G_LLVM_PREFIX, G_MEMLABEL_VAR_PREFIX_FOR_FCALL G_DST_PREFIX);
    memlabel_t ml = m.second->get_ml();
    ml.memlabel_rename_variable_ids(alias_region_t::alias_type_symbol, symbol_id_map);
    new_memlabel_map[mk_string_ref(name)] = mk_memlabel_ref(ml);
  }
  this->set_memlabel_map(new_memlabel_map);

  auto locs_map = this->get_locs();
  for (auto &l : locs_map) {
    memlabel_t ml = l.second.m_memlabel->get_ml();
    ml.memlabel_rename_variable_ids(alias_region_t::alias_type_symbol, symbol_id_map);
    l.second.m_memlabel = mk_memlabel_ref(ml);
  }
  this->set_locs(locs_map);
  this->graph_populate_relevant_addr_refs();
}

intrinsic_interpret_fn_t
tfg::llvm_intrinsic_interpret_usub_with_overflow_i32 = [](shared_ptr<tfg_edge const> const &ed/*, state const &start_state*/) -> list<shared_ptr<tfg_edge const>>
{
  pc const &from_pc = ed->get_from_pc();
  state const &state_to = ed->get_to_state();
  map<string_ref, expr_ref> const &m = state_to.get_value_expr_map_ref();
  int cur_subsubindex = PC_SUBSUBINDEX_INTRINSIC_MIDDLE(0);
  pc pc_cur(pc::insn_label, from_pc.get_index()/*, from_pc.get_bblnum()*/, from_pc.get_subindex(), cur_subsubindex);
  list<shared_ptr<tfg_edge const>> ret;
  for (const auto &ve : m) {
    expr_ref const &e = ve.second;
    context *ctx = e->get_context();
    if (   e->is_bv_sort()
        && e->get_sort()->get_size() == DWORD_LEN
        && e->get_operation_kind() == expr::OP_FUNCTION_CALL) {
      pc pc_next(pc::insn_label, from_pc.get_index()/*, from_pc.get_bblnum()*/, from_pc.get_subindex(), cur_subsubindex + 1);
      state to_state;
      ASSERT(e->get_args().size() == OP_FUNCTION_CALL_ARGNUM_FARG0 + 2);
      expr_ref op0 = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_FARG0);
      expr_ref op1 = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_FARG0 + 1);
      to_state.set_expr_in_map(ve.first->get_str(), ctx->mk_bvsub(op0, op1));
      shared_ptr<tfg_edge const> new_e = mk_tfg_edge(mk_itfg_edge(pc_cur, pc_next, to_state, expr_true(ctx)/*, state()*/, {}, te_comment_t::te_comment_intrinsic_usub_result()));
      ret.push_back(new_e);
      cur_subsubindex++;
      pc_cur = pc_next;
    } else if (e->is_bool_sort()) {
      expr_vector ev = expr_get_function_calls(e);
      ASSERT(ev.size() == 1);
      expr_ref fe = ev.at(0);
      expr_ref op0 = fe->get_args().at(OP_FUNCTION_CALL_ARGNUM_FARG0);
      expr_ref op1 = fe->get_args().at(OP_FUNCTION_CALL_ARGNUM_FARG0 + 1);
      pc pc_next(pc::insn_label, from_pc.get_index()/*, from_pc.get_bblnum()*/, from_pc.get_subindex(), cur_subsubindex + 1);
      state to_state;
      to_state.set_expr_in_map(ve.first->get_str(), ctx->mk_bvult(op0, op1));
      shared_ptr<tfg_edge const> new_e = mk_tfg_edge(mk_itfg_edge(pc_cur, pc_next, to_state, expr_true(ctx)/*, state()*/, {}, te_comment_t::te_comment_intrinsic_usub_carry()));
      ret.push_back(new_e);
      cur_subsubindex++;
      pc_cur = pc_next;
    } else if (e->is_array_sort()) {
      //expr_vector ev = expr_get_function_calls(e);
      //ASSERT(ev.size() == 1);
      //expr_ref const &f = ev.at(0);
      //ASSERT(f->get_operation_kind() == expr::OP_FUNCTION_CALL);
      continue;
    } else NOT_REACHED();
  }
  return ret;
};

//this definition must be after all the function objects have been defined
map<string, intrinsic_interpret_fn_t> tfg::interpret_intrinsic_fn = {
  {"llvm.usub.with.overflow.i32", tfg::llvm_intrinsic_interpret_usub_with_overflow_i32},
};

//set<pc>
//tfg::tfg_identify_anchor_pcs() const
//{
//  NOT_IMPLEMENTED();
//}

//bool
//tfg::tfg_has_anchor_free_cycle(set<pc> const &anchor_pcs) const
//{
//  NOT_IMPLEMENTED();
//}

bool
tfg::tfg_edge_composition_contains_fcall(shared_ptr<tfg_edge_composition_t> const &tfg_ec) const
{
  state to_state = this->tfg_edge_composition_get_to_state(tfg_ec/*, this->get_start_state()*/);
  return to_state.contains_function_call();
}

set<expr_ref>
tfg::get_live_loc_exprs_having_sprel_map_at_pc(pc const &p) const
{
  set<expr_ref> ret;
  for (const auto &locid : this->get_live_locids(p)) {
    if(this->loc_has_sprel_mapping_at_pc(p, locid))
      ret.insert(this->graph_loc_get_value_for_node(locid));
  }
  return ret;
}

set<expr_ref>
tfg::get_live_loc_exprs_ignoring_memslot_symbols_at_pc(pc const &p) const
{
  set<expr_ref> ret;
  for (const auto &l : this->get_live_locs(p)) {
    if(!l.second.is_memslot_symbol() && !l.second.is_memslot_pure_stack() && !l.second.is_memslot_arg())
      ret.insert(this->graph_loc_get_value_for_node(l.first));
  }
  return ret;
}

set<expr_ref>
tfg::get_interesting_exprs_at_delta(pc const &p, delta_t const& delta) const
{
  ASSERT(delta.get_lookahead() >= 0);
  if(m_pc_delta_to_interesting_exprs_cache.count(make_pair(p, delta)))
  { 
    return m_pc_delta_to_interesting_exprs_cache.at(make_pair(p, delta));
  }

  context *ctx = this->get_context();
  graph_memlabel_map_t const& memlabel_map = this->get_memlabel_map();
  sprel_map_t const& sprel_map = this->get_sprel_map(p);

  //set<pair<expr_ref,edge_guard_t>> ret_exprs_egs;
  set<expr_ref> ret_exprs;
  list<shared_ptr<tfg_edge_composition_t>> outgoing_paths;
  if (delta.get_lookahead() == 0) {
    for (auto loc : this->get_locs()) {
      if (!this->loc_is_defined_at_pc(p,loc.first))
        continue;
      expr_ref loc_expr = this->graph_loc_get_value_for_node(loc.first);
      expr_ref se = ctx->expr_simplify_using_sprel_and_memlabel_maps(loc_expr, sprel_map, memlabel_map);
      if (!se->is_const()) {
        //ret_exprs_egs.insert(make_pair(se,edge_guard_t()));
        ret_exprs.insert(se);
      }
    }
    if(!ctx->get_config().disable_loop_path_exprs){
      int max_unroll = delta.get_max_unroll();
      get_unrolled_loop_paths_from(p, outgoing_paths, max_unroll);
    }
  }
  else {
    outgoing_paths = this->get_outgoing_paths_at_lookahead_and_max_unrolls(p, delta.get_lookahead(), delta.get_max_unroll());
  }

  for (auto loc : this->get_locs()) {
    expr_ref loc_expr = this->graph_loc_get_value_for_node(loc.first);

    for (const auto &opath : outgoing_paths) {
      if (!this->loc_is_defined_at_pc(opath->graph_edge_composition_get_to_pc(),loc.first))
        continue;
      predicate_ref ep = predicate::mk_predicate_ref(precond_t(), loc_expr, loc_expr, "tmp");
      list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> epreds = this->graph_apply_trans_funs(opath, ep);
      for (auto const& eepa : epreds) {
        predicate_ref epa = eepa.second;
        expr_ref e = epa->get_lhs_expr();
        expr_ref e_simplified = ctx->expr_simplify_using_sprel_and_memlabel_maps(e, sprel_map, memlabel_map);
        if (   !ctx->expr_contains_function_call(e_simplified)
            && !e_simplified->is_const()
            && e_simplified->get_operation_kind() != expr::OP_MEMMASK) {
          //ret_exprs_egs.insert(make_pair(e_simplified,edge_guard_t(eepa.first)));
          ret_exprs.insert(e_simplified);
        }
      }
    }
  }
  //set<expr_ref> ret_exprs;
  //transform(ret_exprs_egs.begin(), ret_exprs_egs.end(),
  //          std::inserter(ret_exprs, ret_exprs.end()),
  //          [](pair<expr_ref,edge_guard_t>const& pe) { return pe.first; });
  m_pc_delta_to_interesting_exprs_cache.insert(make_pair(make_pair(p, delta), ret_exprs));
  return ret_exprs;
}

void expr_edge_guard_set_union(set<pair<expr_ref,edge_guard_t>> &res, set<pair<expr_ref,edge_guard_t>> const& other)
{
  // dedup pairs with common expr_ref, preferring the existing pair
  set<pair<expr_ref,edge_guard_t>> ret = res;
  for (auto const& other_p : other) {
    bool already_present = false;
    for (auto const& res_p : res) {
      if (other_p.first == res_p.first) {
        already_present = true;
        break;
      }
    }
    if (!already_present) {
      ret.insert(other_p);
    }
  }
  res.swap(ret);
}

expr_vector
get_arithmetic_affine_atoms(expr_ref const& e)
{
  // NOTE: assumes simplified input s.t. (const*const) can never be in input
  expr_vector ret;
  for (auto const& addsub_atom : get_arithmetic_addsub_atoms(e)) {
    if (   addsub_atom->get_operation_kind() == expr::OP_BVMUL
        && addsub_atom->get_args().size() == 2
        && (   addsub_atom->get_args()[0]->is_const()
            || addsub_atom->get_args()[1]->is_const())) {
      expr_ref const_arg     = addsub_atom->get_args()[0];
      expr_ref non_const_arg = addsub_atom->get_args()[1];
      if (!const_arg->is_const()) {
        swap(const_arg, non_const_arg);
      }
      ASSERT(const_arg->is_const());
      ret.push_back(const_arg);
      expr_vector non_const_atoms = get_arithmetic_affine_atoms(non_const_arg);
      ret.insert(ret.end(), non_const_atoms.begin(), non_const_atoms.end());
    } else {
      ret.push_back(addsub_atom);
    }
  }
  return ret;
}

set<expr_ref>
expr_set_get_affine_atoms(set<expr_ref> const& in_set)
{
  set<expr_ref> ret;
  for (auto const& e : in_set) {
    expr_vector affine_atoms = get_arithmetic_affine_atoms(e);
    for (auto const& atom : affine_atoms) {
      if (atom->is_const())
        continue;
      ret.insert(atom);
    }
  }
  //cout << _FNLN_ << ": size reduction = " << in_set.size() - ret.size() << endl;
  //if (in_set.size() > ret.size() + 10) {
  //  cout << "in [" << in_set.size() << "] = ";
  //  for (auto const& e : in_set) { cout << expr_string(e) << " ; "; }
  //  cout << endl;
  //  cout << "ret [" << ret.size() << "] = ";
  //  for (auto const& e : ret) { cout << expr_string(e) << " ; "; }
  //  cout << endl;
  //}
  return ret;
}

set<expr_ref>
tfg::get_interesting_exprs_till_delta(pc const &p, delta_t delta) const
{
  set<expr_ref> ret;
  do {
    //cout << __func__ << ':' << __LINE__ << ": adding exprs at delta: " << delta.to_string() << endl;
    set_union(ret, this->get_interesting_exprs_at_delta(p, delta));
  } while (delta.increment_delta()); // order is important, we want to pick an expr at its smallest edge_guard

  // remove exprs which are affine form of other exprs
  ret = expr_set_get_affine_atoms(ret);

  // remove memmask(arg)
  set_erase_if<expr_ref>(ret, [](expr_ref const& p) -> bool {
      return    p->get_operation_kind() == expr::OP_MEMMASK
             && p->get_args().at(1)->get_memlabel_value().memlabel_is_arg() != -1;
  });

  // add retaddr_const
  expr_ref const& retaddr_expr = this->get_context()->get_consts_struct().get_retaddr_const();
  // remove retaddr if already present
  set_erase_if<expr_ref>(ret, [&retaddr_expr](expr_ref const& p) -> bool {
      return p == retaddr_expr;
  });
  ret.insert(retaddr_expr);

  return ret;
}

void
tfg::tfg_add_unknown_arg_to_fcalls()
{
  context *ctx = this->get_context();
  std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [ctx](pc const &p, const string& key, expr_ref e) -> expr_ref
  {
    return ctx->expr_add_unknown_arg_to_fcalls(e);
  };
  tfg_visit_exprs(f);
}

void
tfg::tfg_identify_rodata_accesses_and_populate_string_contents(input_exec_t const& in)
{
  context *ctx = this->get_context();
  map<rodata_offset_t, size_t> rodata_accesses;
  graph_symbol_map_t const& symbol_map = this->get_symbol_map();
  std::function<void (pc const &p, const string&, expr_ref)> f =
      [this, ctx, &symbol_map, &rodata_accesses]
      (pc const &p, string const& key, expr_ref e) -> void
  {
    sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(p);
    auto const& memlabel_map = this->get_memlabel_map();
    e = ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(e, sprel_map_pair, memlabel_map);
    ctx->expr_identify_rodata_accesses(e, symbol_map, rodata_accesses);
  };
  tfg_visit_exprs_const(f);

  for (auto const& ro_offset : rodata_accesses) {
    DYN_DEBUG(rodata_string_content, cout << __func__ << " " << __LINE__ << ": ro_offset = " << ro_offset.first.rodata_offset_to_string() << ", size " << ro_offset.second << endl);
    this->add_string_contents_for_ro_offset_using_input_exec(ro_offset.first, ro_offset.second, in);
  }
}

void
tfg::add_string_contents_for_ro_offset_using_input_exec(rodata_offset_t const& ro_offset, size_t nbytes, input_exec_t const& in)
{
  graph_symbol_map_t const& symbol_map = this->get_symbol_map();
  symbol_id_t symbol_id = ro_offset.get_symbol_id();
  src_ulong section_off = ro_offset.get_offset();
  ASSERT(symbol_map.count(symbol_id));
  ASSERT(symbol_map.at(symbol_id).is_dst_const_symbol());
  string const& section_name = symbol_map.at(symbol_id).get_name()->get_str();
  //cout << __func__ << " " << __LINE__ << ": dst section name = '" << section_name << "'\n";
  int section_id = get_input_section_by_name(&in, section_name.substr(strlen(G_DST_SYMBOL_PREFIX)).c_str());
  ASSERT(section_id >= 0 && section_id < in.num_input_sections);

  input_section_t const& isec = in.input_sections[section_id];
  ASSERT(section_off >= 0 && section_off < isec.size);
  ASSERT(section_off + nbytes <= isec.size);
  ASSERT(isec.data);
  map<pair<symbol_id_t, offset_t>, vector<char>> scontents = this->get_string_contents();
  vector<char> contents;
  for (size_t i = 0; i < nbytes; i++) {
    contents.push_back(isec.data[section_off + i]);
  }
  scontents.insert(make_pair(make_pair(symbol_id, section_off), contents));
  this->set_string_contents(scontents);
}

bool
tfg::path_represented_by_tfg_edge_composition_is_not_feasible(shared_ptr<tfg_edge_composition_t> const& ec) const
{
  if (!ec || ec->is_epsilon()) {
    return false;
  }
  context* ctx = this->get_context();
  expr_ref econd = this->tfg_edge_composition_get_edge_cond(ec, /*simplified*/true);
  if (econd == expr_true(ctx))
    return false;

  autostop_timer func_timer(__func__);
  stringstream ss;
  ss << __func__; // << "_" << ec->graph_edge_composition_to_string();
  query_comment qc(ss.str());
  list<counter_example_t> unused_ces;
  expr_ref not_econd = expr_not(econd);
  bool ret = ctx->expr_is_provable(not_econd, qc, set<memlabel_ref>(), unused_ces) == solver::solver_res_true;
  //cout << __func__ << ':' << __LINE__ << ": comment = " << ss.str() << endl;
  //cout << __func__ << ':' << __LINE__ << ": e = " << expr_string(not_econd) << ", result = " << ret << endl;
  return ret;
}

shared_ptr<tfg_edge_composition_t>
tfg::semantically_simplify_tfg_edge_composition(shared_ptr<tfg_edge_composition_t> const& ec) const
{
  autostop_timer ft(__func__);
  if (!ec || ec->is_epsilon()) {
    return ec;
  }
  CPP_DBG_EXEC2(STATS, cout << __func__ << ':' << edge_and_path_stats_string_for_edge_composition(ec) << endl);
  int num_feasible = 0;

  shared_ptr<tfg_edge_composition_t> ret;
  list<shared_ptr<tfg_edge_composition_t>> res_list = { mk_epsilon_ec<pc,tfg_edge>() };
  for (auto const& s_term : serpar_composition_node_t<tfg_edge>::serpar_composition_get_serial_terms(ec)) {
    shared_ptr<tfg_edge_composition_t> s_ec = dynamic_pointer_cast<tfg_edge_composition_t>(s_term);
    // get all parallel paths out of this edge composition (SoP form)
    list<shared_ptr<tfg_edge_composition_t>> pterms = graph_edge_composition_get_parallel_paths(s_ec);
    if (pterms.size() == 1) {
      for (auto& res : res_list) {
        res = mk_series(res, s_ec);
      }
      continue;
    }
    list<shared_ptr<tfg_edge_composition_t>> p_list;
    for (auto const& res : res_list) {
      for (auto const& pt_ec : pterms) {
        shared_ptr<tfg_edge_composition_t> se = mk_series(res, pt_ec);
        if (path_represented_by_tfg_edge_composition_is_not_feasible(se)) {
          continue;
        }
        num_feasible++;
        if(num_feasible > SEM_PRUNNING_THRESHOLD) {
          ret = ec;
          goto end;
        }
        p_list.push_back(se);
      }
    }
    res_list = p_list;
  }
  if (res_list.empty()) {
    CPP_DBG_EXEC2(STATS, cout << __func__ << ':' << edge_and_path_stats_string_for_edge_composition(ret) << endl);
    return nullptr;
  }
  ASSERT (res_list.size());
  ret = graph_edge_composition_construct_edge_from_parallel_edgelist(res_list);
  ASSERT(!ret->is_epsilon()); // assuming the input edge composition is used for correlation with a not provably false dst edge composition, the src edge composition must also be not provably false

end:
  CPP_DBG_EXEC2(STATS, cout << __func__ << ':' << edge_and_path_stats_string_for_edge_composition(ret) << endl);
  return ret;
}

bool
getNextFunctionInTfgFile(ifstream &in, string &nextFunctionName)
{
  string line;
  do {
    if (!getline(in, line)) {
      cout << __func__ << " " << __LINE__ << ": reached end of file\n";
      return false;
    }
  } while (line == "");
  if (!string_has_prefix(line, FUNCTION_NAME_FIELD_IDENTIFIER)) {
    cout << "line = " << line << ".\n";
    fflush(stdout);
    NOT_REACHED();
    return false;
  }
  nextFunctionName = line.substr(strlen(FUNCTION_NAME_FIELD_IDENTIFIER " "));
  return true;
}

void
tfg::tfg_preprocess_before_eqcheck(set<string> const &undef_behaviour_to_be_ignored, predicate_set_t const& input_assumes, bool populate_suffixpaths_and_pred_avail_exprs)
{
  context *ctx = this->get_context();
  consts_struct_t const& cs = ctx->get_consts_struct();
  this->remove_assume_preds_with_comments(undef_behaviour_to_be_ignored);
  this->replace_alloca_with_nop();

  if (!input_assumes.empty()) {
    for (auto const& assume : input_assumes) {
      this->add_global_assume(assume);
    }
  }
  this->collapse_tfg(true/*collapse_flag_preserve_branch_structure*/);
  this->populate_transitive_closure();
  this->populate_reachable_and_unreachable_incoming_edges_map();
  this->graph_init_sprels();
  //if (collapse_flag_preserve_branch_structure) {
  //  this->resize_farg_exprs_for_function_calling_conventions();
  //}
  this->populate_auxilliary_structures_dependent_on_locs();
  this->populate_simplified_edgecond(nullptr);
  this->populate_simplified_to_state(nullptr);
  this->populate_simplified_assumes(nullptr);
  this->populate_simplified_assert_map(this->get_all_pcs());
  this->populate_loc_definedness();
  this->populate_branch_affecting_locs();

  if (populate_suffixpaths_and_pred_avail_exprs) {
    this->populate_suffixpaths();
    this->populate_suffixpath2expr_map();
    this->populate_pred_avail_exprs();
  }
}

void
tfg::generate_dst_tfg_file(string const& outfile, string const& function_name, rodata_map_t const& rodata_map, tfg const& dst_tfg, vector<dst_insn_t> const& dst_iseq, vector<dst_ulong> const& dst_insn_pcs)
{
  ofstream fo;
  fo.open(outfile, std::ofstream::out | std::ofstream::app);
  if (fo.is_open()) {
    //fo << boost::replace_all_copy(src_tfg->to_string_for_eq(exit_conds, src_symbol_map, nextpc_function_name_map, true), ": INT", ": BV:32") << endl;
    //fo << boost::replace_all_copy(dst_tfg->graph_to_string(/*exit_conds, dst_symbol_map, &new_nextpc_function_name_map, false*/), ": INT", ": BV:32") << endl;
    fo << "=FunctionName: " << function_name << "\n";
    fo << "=Rodata-map:\n";
    fo << rodata_map.to_string_for_eq();
    fo << "=dst_iseq\n";
    fo << dst_insn_vec_to_string(dst_iseq, as1, sizeof as1);
    fo << "=dst_iseq done\n";
    fo << "=dst_insn_pcs\n";
    dst_insn_pcs_to_stream(fo, dst_insn_pcs);
    fo << dst_tfg.graph_to_string() << endl;
    fo.close();
    cout << "successfully-generated: " << outfile << endl;
  } else {
    cout << __func__ << " " << __LINE__ << ": could not open: " << outfile << endl;
    NOT_REACHED();
  }
}

void
tfg::tfg_add_global_assumes_for_string_contents(bool is_dst_tfg)
{
  context* ctx = this->get_context();
  consts_struct_t const& cs = ctx->get_consts_struct();
  expr_ref mem;
  bool found;
  state const& start_state = this->get_start_state();
  found = start_state.get_memory_expr(start_state, mem);
  ASSERT(found);
  ASSERT(mem);
  for (auto const& s : m_string_contents) {
    auto const& sym = this->get_symbol_map().get_map().at(s.first.first);
    reg_type rt = is_dst_tfg ? reg_type_dst_symbol_addr : reg_type_symbol;
    expr_ref sym_expr = cs.get_expr_value(rt, s.first.first);
    memlabel_t ml = memlabel_t::memlabel_symbol(s.first.first/*, sym.get_size()*/, sym.is_const());
    for (size_t i = 0; i < s.second.size(); i++) {
      ostringstream oss;
      size_t full_offset = s.first.second + i;
      oss << "symbol" << s.first.first << ".char" << full_offset;
      expr_ref val = ctx->mk_select(mem, ml, ctx->mk_bvadd(sym_expr, ctx->mk_bv_const(sym_expr->get_sort()->get_size(), full_offset)), 1, false);
      expr_ref c = ctx->mk_bv_const(BYTE_SIZE, s.second.at(i));
      predicate_ref pred = predicate::mk_predicate_ref(precond_t(), val, c, oss.str());
      this->add_global_assume(pred);
    }
  }
}

void
tfg::tfg_dst_add_marker_call_argument_setup_and_destroy_instructions(set<pc>& marker_pcs) const
{
  return; //NOT_IMPLEMENTED();
}

bool
tfg::tfg_dst_pc_represents_marker_call_instruction(pc const& p) const
{
  if (p.get_subsubindex() != PC_SUBSUBINDEX_FCALL_START) {
    CPP_DBG_EXEC(REMOVE_MARKER_CALL, cout << __func__ << " " << __LINE__ << ": returning false for " << p.to_string() << "\n");
    return false;
  }
  context* ctx = this->get_context();
  consts_struct_t const& cs = ctx->get_consts_struct();
  shared_ptr<tfg_edge const> incident_edge = this->get_incident_fcall_edge(p);
  ASSERT(incident_edge);
  expr_ref nextpc = incident_edge->get_function_call_nextpc();
  ASSERT(nextpc);
  if (nextpc->get_operation_kind() != expr::OP_SELECT) {
    return false;
  }
  expr_ref addr = nextpc->get_args().at(2);
  if (!cs.is_symbol(addr)) {
    CPP_DBG_EXEC(REMOVE_MARKER_CALL, cout << __func__ << " " << __LINE__ << ": returning false for " << p.to_string() << ": nextpc = " << expr_string(nextpc) << ", addr = " << expr_string(addr) << "\n");
    return false;
  }
  symbol_id_t symbol_id = cs.get_symbol_id_from_expr(addr);
  graph_symbol_map_t const& symbol_map = this->get_symbol_map();
  bool ret = symbol_map.at(symbol_id).is_marker_fcall_symbol();
  CPP_DBG_EXEC(REMOVE_MARKER_CALL, cout << __func__ << " " << __LINE__ << ": returning " << ret << " for " << p.to_string() << "\n");
  return ret;
}

set<dst_ulong>
tfg::tfg_dst_get_marker_call_instructions(vector<dst_ulong> const& insn_pcs) const
{
  set<pc> marker_pcs;
  bool changed = false;

  for (auto const& p : this->get_all_pcs()) {
    if (this->tfg_dst_pc_represents_marker_call_instruction(p)) {
      marker_pcs.insert(p);
    }
  }

  for (auto const& marker_pc : marker_pcs) {
    this->tfg_dst_add_marker_call_argument_setup_and_destroy_instructions(marker_pcs);
  }

  set<dst_ulong> ret;
  for (auto const& p : marker_pcs) {
    if (!p.is_label()) {
      continue;
    }
    char const* pc_index = p.get_index();
    int insn_index = strtol(pc_index, nullptr, 10);
    ASSERT(insn_index >= 0 && insn_index < insn_pcs.size());
    ret.insert(insn_pcs.at(insn_index));
  }
  return ret;
}

aliasing_constraints_t
tfg::get_aliasing_constraints_for_edge(tfg_edge_ref const& e) const
{
	autostop_timer ft(__func__);

	pc const& from_pc  = e->get_from_pc();
	state const& to_state = e->get_simplified_to_state();
	expr_ref const& econd = e->get_simplified_cond();

	aliasing_constraints_t ret;
	ret.aliasing_constraints_union(this->graph_generate_aliasing_constraints_for_edgecond_and_to_state(econd, to_state));

  std::function<expr_ref (expr_ref const&)> f = [this, &from_pc](expr_ref const& e)
  {
    return this->expr_simplify_at_pc(e, from_pc);
  };
  ret.visit_exprs(f);
	return ret;
}

aliasing_constraints_t
tfg::graph_apply_trans_funs_on_aliasing_constraints(tfg_edge_ref const& e, aliasing_constraints_t const& als) const
{
  aliasing_constraints_t ret;
  context *ctx = this->get_context();
  expr_ref const& edgecond = e->get_simplified_cond();
  for (auto const& cons : als.get_ls()) {
    expr_ref const& cons_guard = cons.get_guard();
    expr_ref const& cons_addr = cons.get_addr();
    predicate_ref guard_pred = predicate::mk_predicate_ref(precond_t(), cons_guard, expr_true(ctx), "aliasing_constraint_guard");
    predicate_ref addr_pred = predicate::mk_predicate_ref(precond_t(), cons_addr, expr_true(ctx), "aliasing_constraint_addr");
    list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> guard_ls = this->graph_apply_trans_funs(mk_edge_composition<pc,tfg_edge>(e), guard_pred, true);
    list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> addr_ls = this->graph_apply_trans_funs(mk_edge_composition<pc,tfg_edge>(e), addr_pred, true);
    ASSERT(guard_ls.size() == 1);
    ASSERT(addr_ls.size() == 1);
    predicate_ref const& pred_guard = guard_ls.front().second;
    predicate_ref const& pred_addr = addr_ls.front().second;
    ASSERT(pred_guard->get_rhs_expr() == expr_true(ctx));
    ASSERT(pred_addr->get_rhs_expr() == expr_true(ctx));
    ASSERT(pred_guard->get_edge_guard() == pred_addr->get_edge_guard());
    expr_ref new_guard = ctx->expr_do_simplify(expr_and(edgecond, pred_guard->get_lhs_expr()));
    expr_ref new_addr = pred_addr->get_lhs_expr();
    aliasing_constraint_t new_cons(new_guard, new_addr, cons.get_count(), cons.get_ml());
    ret.add_constraint(new_cons);
  }
  return ret;
}

std::function<bool (pc const&, pc const&)>
tfg::gen_pc_cmpF(list<string> const& sorted_bbl_indices)
{
  std::function<bool (pc const&, pc const&)> f = [sorted_bbl_indices](pc const& a, pc const& b)
  {
    //cout << __func__ << " " << __LINE__ << ": a = " << a.to_string() << ", b = " << b.to_string() << endl;
    if (a.get_index() == b.get_index()) {
      //cout << __func__ << " " << __LINE__ << ": return false\n";
      return false;
    }
    for (auto const& s : sorted_bbl_indices) {
      if (a.get_index() == s) {
        //cout << __func__ << " " << __LINE__ << ": return true\n";
        return true;
      }
      if (b.get_index() == s) {
        //cout << __func__ << " " << __LINE__ << ": return false\n";
        return false;
      }
    }
    //cout << __func__ << " " << __LINE__ << ": return false\n";
    return false;
  };
  return f;
}

pair<bool,bool>
tfg::counter_example_translate_on_edge(counter_example_t const &counter_example, tfg_edge_ref const &edge, counter_example_t &ret, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels) const
{
  autostop_timer ft(__func__);
  PRINT_PROGRESS("%s %s() %d: entry. counter_example.size() = %zd\n", __FILE__, __func__, __LINE__, counter_example.size());

  DYN_DEBUG(ce_translate, cout << __func__ << " " << __LINE__ << ": attempting propagation of counterexample across " << edge->to_string() << ":\n" << counter_example.counter_example_to_string() << endl);

  pc const &from_pc = edge->get_from_pc();
  context *ctx = this->get_context();

  // incoming counter example must satisfy the edge cond
  // in order to be propagated
  expr_ref const &econd = edge->get_simplified_cond();
  expr_ref const_val;
  bool success = counter_example.evaluate_expr_assigning_random_consts_as_needed(econd, const_val, rand_vals, relevant_memlabels);
  if (!success) {
    DYN_DEBUG(ce_translate, cout << __func__ << " " << __LINE__ << ": propagation across " << edge->to_string() << " failed due to edgecond evaluation failure. econd = " << expr_string(econd) << "\n");
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return make_pair(false, false);
  }
  //ASSERT(const_val->is_undefined() || const_val->is_const());
  ASSERT(const_val->is_const());
  ASSERT(const_val->is_bool_sort());
  if (!const_val->get_bool_value()) {
    DYN_DEBUG(ce_translate, cout << __func__ << " " << __LINE__ << ": propagation across " << edge->to_string() << " failed because edgecond (successfully) evaluated to false.\n");
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return make_pair(false, true);
  }

  predicate_set_t const &from_assumes = this->collect_assumes_around_edge(edge);
  if (!this->counter_example_satisfies_preds_at_pc(counter_example, from_assumes, rand_vals, relevant_memlabels)) {
    DYN_DEBUG(ce_translate, cout << __func__ << " " << __LINE__ << ": propagation across " << edge->to_string() << " failed because it does not satisfy the required predicates at " << from_pc.to_string() << ".\n");
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return make_pair(false, false);
  }

  //cout << __func__ << ':' << __LINE__ << ": input ce : " << counter_example.to_string() << endl;
  map<string_ref, expr_ref> ret_mapping = counter_example.get_mapping();
  for (auto const& ve : this->graph_get_vars_written_on_edge(edge)) {
    expr_ref const_val;
    DYN_DEBUG2(ce_translate, cout << __func__ << " " << __LINE__ << ": evaluating counter-example for " << ve.first->get_str() << " on expr " << expr_string(ve.second) << ":\n" << counter_example.counter_example_to_string() << endl);
    bool success = counter_example.evaluate_expr_assigning_random_consts_as_needed(ve.second, const_val, rand_vals, relevant_memlabels);
    if (!success) {
      DYN_DEBUG(ce_translate, cout << __func__ << " " << __LINE__ << ": propagation across " << edge->to_string() << " failed due to evaluation failure of state-element " << ve.first->get_str() << " = " << expr_string(ve.second) << ".\n");
      PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
      return make_pair(false, false);
    }
    ASSERT(const_val->is_const());
    DYN_DEBUG2(ce_translate, cout << __func__ << ':' << __LINE__ << ": " << ve.first->get_str() << ": " << expr_string(ve.second) << " = " << expr_string(const_val) << endl);
    ret_mapping[ve.first] = const_val;
  }


  ret = counter_example_t(ret_mapping, counter_example.get_rodata_submap(), ctx);
  PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
  return make_pair(true, true);
}

void
tfg::populate_branch_affecting_locs()
{
  autostop_timer func_timer(__func__);
  map<pc, bavlocs_val_t> vals;
  branch_affecting_vars_analysis l(this, vals);
  l.initialize(bavlocs_val_t::get_boundary_value(this));
  l.solve();

  map<pc,set<graph_loc_id_t>> bavlocs;
  for (auto const& v : vals) {
    bavlocs.insert(make_pair(v.first, v.second.get_bavlocs()));
  }
  this->set_bavlocs(bavlocs);
  DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": " << this->get_name()->get_str() << ": branch affecting locs for tfg:\n"; this->print_branch_affecting_locs(););
}

list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>>
tfg::graph_apply_trans_funs(graph_edge_composition_ref<pc,tfg_edge> const &pth, predicate_ref const &pred, bool simplified) const
{

  list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> ret_ec = this->graph_apply_trans_funs_helper(pth, pred, simplified);
  if (!ret_ec.size()) {
    ret_ec = this->graph_apply_trans_funs_helper(pth, pred, simplified, /*always_merge*/true);
  }
  ASSERT(ret_ec.size());
  return ret_ec;
}

list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>>
tfg::graph_apply_trans_funs_helper(graph_edge_composition_ref<pc,tfg_edge> const &pth, predicate_ref const &pred, bool simplified, bool always_merge) const
{
  autostop_timer func_timer(__func__);
  //state const &start_state = this->get_start_state();

  if (always_merge) {
    list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> ret_preds;
    state to_state = this->tfg_edge_composition_get_to_state(pth, simplified);
    predicate_ref new_pred = pred->pred_substitute(/*start_state, */to_state);
    if (simplified) {
      new_pred = new_pred->simplify();
    }
    DYN_DEBUG2(apply_trans_funs, cout << _FNLN_ << " new pred = " << pth->graph_edge_composition_to_string() << "\n" << new_pred->to_string(true) << endl);
    ret_preds.push_back(make_pair(pth, new_pred));
    return ret_preds;
  }

  typedef list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> retval_t;

  //XXX: should reuse predicate_set_union for the dedup logic in get_distinct_retval and fold_parallel_f
  std::function<retval_t (retval_t const &)> get_distinct_retval =
    [](retval_t const &in) {
      distinct_set_t<size_t> eqclasses;
      size_t i;

      // create singelton sets
      i = 0;
      for (auto const& elem : in) {
        eqclasses.distinct_set_make(i++);
      }
      // union sets with same predicate
      i = 0; auto itr = in.begin();
      for ( ; itr != in.end(); ++itr, ++i) {
        size_t j = 0; auto itr2 = next(itr);
        for ( ; itr2 != in.end(); ++itr2, ++j) {
          if (itr->second == itr2->second) {
            eqclasses.distinct_set_union(i, j);
          }
        }
      }
      // get all sets with same predicate and merge their edgeguards
      map<size_t, pair<graph_edge_composition_ref<pc,tfg_edge>,predicate_ref>> m;
      i = 0;
      for (auto const& elem : in) {
        size_t rep = eqclasses.distinct_set_find(i);
        if (!m.count(rep)) {
          m.insert(make_pair(rep, elem));
        } else {
          graph_edge_composition_ref<pc,tfg_edge> new_guard = mk_parallel(elem.first, m.at(rep).first);
          m.insert(make_pair(rep, make_pair(new_guard, elem.second)));
        }
        ++i;
      }
      retval_t ret;
      for (auto const& p : m) {
        ret.push_back(p.second);
      }
      return ret;
    };

  std::function<retval_t (shared_ptr<tfg_edge const> const&, retval_t const &postorder_val)> fold_atom_f = [/*&start_state, */simplified, get_distinct_retval, always_merge](shared_ptr<tfg_edge const> const& e, retval_t const &postorder_val)
  {
    retval_t ret;
    if(postorder_val.size() == 0)
    {
      ASSERT(!always_merge);
      return ret;
    }
    if (e->is_empty()) {
      return postorder_val;
    }
    for (const auto &pv : postorder_val) {
      graph_edge_composition_ref<pc,tfg_edge> const &pveguard = pv.first;
      predicate_ref const &pvpred = pv.second;
      predicate_ref new_pred = pvpred->pred_substitute(/*start_state, */(simplified ? e->get_simplified_to_state() : e->get_to_state()));
      if (simplified) {
        new_pred = new_pred->simplify();
      }
      graph_edge_composition_ref<pc,tfg_edge> new_guard = mk_series(mk_edge_composition<pc,tfg_edge>(e), pveguard);
      ret.push_back(make_pair(new_guard, new_pred));
    }
    return get_distinct_retval(ret);
  };

  std::function<retval_t (retval_t const &, retval_t const &)> fold_parallel_f =
    [always_merge](retval_t const &a, retval_t const &b)
    {
      retval_t ret;
      if(a.size() == 0 || b.size() == 0)
      {
        ASSERT(!always_merge);
        return ret;
      }
      predicate_set_t done_preds;
      for (const auto &b_elem : b) {
        //dedup those that have identical lhs/rhs exprs, by or'ing their edge guards, else add them to the set
        for (const auto &a_elem : a) {
          if (a_elem.second == b_elem.second && !unordered_set_belongs(done_preds, a_elem.second) && !unordered_set_belongs(done_preds, b_elem.second))
          {
            done_preds.insert(a_elem.second);
            graph_edge_composition_ref<pc,tfg_edge> new_guard = mk_parallel(a_elem.first, b_elem.first);
            ret.push_back(make_pair(new_guard, a_elem.second));
          }
        }
      }
      for (const auto &b_elem : b) {
        if (!unordered_set_belongs(done_preds, b_elem.second)) {
          ret.push_back(b_elem);
        }
      }
      for (const auto &a_elem : a) {
        if (!unordered_set_belongs(done_preds, a_elem.second)) {
          ret.push_back(a_elem);
        }
      }
      
      if(ret.size() > PREDICATE_MERGE_THRESHOLD)
        ret.clear();
      return ret;
    };

  list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> start_set;
  start_set.push_back(make_pair(mk_epsilon_ec<pc,tfg_edge>(), pred));
  return pth->template visit_nodes_postorder_series<retval_t>(fold_atom_f, fold_parallel_f, start_set);
}

void
tfg::populate_loc_definedness()
{
  autostop_timer ft(__func__);
  map<pc, defined_locs_val_t> vals;
  def_analysis definedness_analysis(this, vals);
  definedness_analysis.initialize(defined_locs_val_t::get_boundary_value(this));
  definedness_analysis.solve();

  map<pc,set<graph_loc_id_t>> loc_definedness;
  for (const auto &v : vals) {
    loc_definedness.insert(make_pair(v.first, v.second.get_defined_locs()));
  }
  this->set_loc_definedness(loc_definedness);
}

predicate_set_t
tfg::collect_assumes_for_edge(tfg_edge_ref const& te) const
{
  predicate_set_t from_preds;
  if (!te->is_empty()) {
    from_preds = this->get_assume_preds(te->get_from_pc());
    ASSERT(from_preds.empty() || te->get_from_pc().is_start());
    for (auto const& assume_e : te->get_simplified_assumes()) {
      predicate_ref p = predicate::ub_assume_predicate_from_expr(assume_e);
      from_preds.insert(p);
    }

    DYN_DEBUG2(decide_hoare_triple,
      cout << _FNLN_ << ": returning:\n";
      for (auto const& fpred : from_preds) {
        cout << fpred->pred_to_string("  ") << "\n";
      }
      cout << "\n";
    );
  }
  return from_preds;
}

predicate_set_t
tfg::collect_assumes_around_edge(tfg_edge_ref const& te) const
{
  return this->collect_assumes_for_edge(te);
}

void
tfg::populate_simplified_edgecond(tfg_edge_ref const& e) const
{
  set<tfg_edge_ref> es;
  if (e != nullptr) {
    es.insert(e);
  } else {
    for (auto const& e : this->get_edges()) {
      es.insert(e);
    }
  }
  context* ctx = this->get_context();
  graph_memlabel_map_t memlabel_map = this->get_memlabel_map();
  for (auto const& e : es) {
    expr_ref const& econd = e->get_cond();
    sprel_map_pair_t const& sprel_map_pair = this->get_sprel_map_pair(e->get_from_pc());
    expr_ref const& simplified_econd = ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(econd, sprel_map_pair, memlabel_map);
    e->set_simplified_cond(simplified_econd);
  }
}

void
tfg::populate_simplified_assumes(tfg_edge_ref const& e) const
{
  set<tfg_edge_ref> es;
  if (e != nullptr) {
    es.insert(e);
  } else {
    for (auto const& e : this->get_edges()) {
      es.insert(e);
    }
  }
  context* ctx = this->get_context();
  graph_memlabel_map_t const& memlabel_map = this->get_memlabel_map();
  for (auto const& e : es) {
    sprel_map_pair_t const& sprel_map_pair = this->get_sprel_map_pair(e->get_from_pc());
    unordered_set<expr_ref> new_assumes;
    for (auto const& assume_e : e->get_assumes()) {
      expr_ref const& simplified_assume_e = ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(assume_e, sprel_map_pair, memlabel_map);
      new_assumes.insert(simplified_assume_e);
    }
    e->set_simplified_assumes(new_assumes);
  }
}

void
tfg::populate_simplified_to_state(tfg_edge_ref const& e) const
{
  autostop_timer func_timer(__func__);
  set<tfg_edge_ref> es;
  if (e) {
    es.insert(e);
  } else {
    es = this->get_edges();
  }
  vector<memlabel_ref> relevant_memlabels_ref;
  this->graph_get_relevant_memlabels_except_args(relevant_memlabels_ref);
  vector<memlabel_ref> relevant_memlabels;
  for (auto const& mlr : relevant_memlabels_ref) {
    relevant_memlabels.push_back(mlr);
  }
  auto const& memlabel_map = this->get_memlabel_map();
  state const& start_state = this->get_start_state();
  context* ctx = this->get_context();
  for (auto &edge : es) {
    sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(edge->get_from_pc());
    auto const& loc2expr_map = this->get_locs_potentially_written_on_edge(edge);

    map<string_ref, expr_ref> m;
    state simplified_to_state = edge->get_to_state();
    for (auto const& l : simplified_to_state.get_value_expr_map_ref()) {
      //if (!start_state.get_value_expr_map_ref().count(l.first))
      //  continue;
      if (ctx->key_represents_hidden_var(l.first)) {
        continue;
      }
      expr_ref e = ctx->get_input_expr_for_key(l.first, l.second->get_sort()); //start_state.get_value_expr_map_ref().at(l.first);
      expr_ref ex;
      graph_loc_id_t locid;
      locid = this->graph_expr_to_locid(e);
      if (locid != GRAPH_LOC_ID_INVALID) {
        if (!loc2expr_map.count(locid)) {
          continue; //this can happen if the loc has been sprel'd away, or the simplified version of the output value is identical to the input value
        } else {
          ex = loc2expr_map.at(locid);
        }
      } else if (e->is_array_sort()) {
        // for memory, take memsplice of all relevant memmasks
        vector<expr_ref> args;
        memlabel_t ml = memlabel_t::memlabel_union(relevant_memlabels);
        ex = ctx->mk_memmask(l.second, ml);
        ex = ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(ex, sprel_map_pair, memlabel_map);
      } else {
        cout << __func__ << ':' << __LINE__ << ": " << l.first  << " -> " << expr_string(l.second) << endl;
        NOT_REACHED();
      }
      ASSERT(ex);
      m.insert(make_pair(l.first, ex));
    }
    simplified_to_state.set_value_expr_map(m);
    edge->set_simplified_to_state(simplified_to_state);
  }
}

void
tfg::populate_locs_potentially_modified_on_edge(tfg_edge_ref const &e) const
{
  autostop_timer func_timer(__func__);

  set<tfg_edge_ref> es;
  if (e) {
    es.insert(e);
  } else {
    es = this->get_edges();
  }

  auto const& memlabel_map = this->get_memlabel_map();
  set<graph_loc_id_t> const& locids = map_get_keys(this->get_locs());

  map<edge_id_t<pc>,map<graph_loc_id_t,expr_ref>>& edge_to_loc_to_expr_map = this->get_edge_to_locs_written_map();
  for (const auto &e : es) {
    map<graph_loc_id_t, expr_ref> const& m = this->compute_simplified_loc_exprs_for_edge(e, locids);
    edge_to_loc_to_expr_map[e->get_edge_id()] =  m;
  }
}

map<graph_loc_id_t,expr_ref>
tfg::compute_simplified_loc_exprs_for_edge(tfg_edge_ref const& e, set<graph_loc_id_t> const& locids, bool force_update) const
{
  pc const &p = e->get_from_pc();
  state const &to_state = e->get_to_state();
  map<string, expr_ref> mem_simplified = this->graph_loc_get_mem_simplified(p, to_state);
  map<graph_loc_id_t, expr_ref> ret;
  for (auto l : locids) {
    expr_ref const& ex = graph_loc_get_value_simplified_using_mem_simplified(p, to_state, l, mem_simplified);
    if (!ex || (!force_update && ex == this->get_locid2expr_map().at(l))) {
      continue;
    }
    //cout << __func__ << " " << __LINE__ << ": " << e->to_string() << ": locid " << l << ": ex = " << expr_string(ex) << endl;
    ret.insert(make_pair(l, ex));
  }
  return ret;
}

map<string_ref,expr_ref>
tfg::graph_get_vars_written_on_edge(tfg_edge_ref const& e) const
{
  map<string_ref, expr_ref> ret;
  state const& to_state = e->get_simplified_to_state();
  map<expr_id_t, pair<expr_ref, expr_ref>> state_submap = state::create_submap_from_state(/*this->get_start_state(), */to_state).get_expr_submap();
  for (auto const& pe : state_submap) {
    ret.insert(make_pair(pe.second.first->get_name(), pe.second.second));
  }
  return ret;
}

predicate_set_t
tfg::get_simplified_non_mem_assumes(tfg_edge_ref const& e) const
{
  predicate_set_t ret;
  for (auto const& assume_e : e->get_simplified_assumes()) {
    if (!expr_has_mem_ops(assume_e)) {
      predicate_ref pred = predicate::ub_assume_predicate_from_expr(assume_e);
      ret.insert(pred);
    }
  }
  return ret;
}

shared_ptr<state const>
tfg::get_start_state_with_all_input_exprs() const
{
  context* ctx = this->get_context();
  map<string_ref, expr_ref> m;
  for (auto const& edge : this->get_edges()) {
    function<void (string const&, expr_ref)> func =
      [&m, ctx](string const& name, expr_ref const& e) -> void
    {
        if (e->is_bv_sort() || e->is_bool_sort()) {
          string_ref key = mk_string_ref(name);
          m.insert(make_pair(key, ctx->get_input_expr_for_key(key, e->get_sort())));
        }
    };
    edge->visit_exprs_const(func);
  }
  for (auto const& ke : this->get_argument_regs().get_map()) {
    ASSERT(ke.second->is_var());
    string const& name = ke.second->get_name()->get_str();
    string key = name.substr(strlen(G_INPUT_KEYWORD "."));
    //llvm_vars.insert(make_pair(key, ke.second->get_sort()));
    m.insert(make_pair(mk_string_ref(key), ke.second));
  }
  for (size_t i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
    stringstream ss;
    ss << G_SRC_KEYWORD "." LLVM_CALLEE_SAVE_REGNAME << "." << i;
    string key = ss.str();
    expr_ref csreg = ctx->mk_var(key, ctx->mk_bv_sort(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS)));
    m.insert(make_pair(mk_string_ref(key), csreg));
  }

  shared_ptr<state> ret = make_shared<state>();
  ret->set_value_expr_map(m);
  return dynamic_pointer_cast<state const>(ret);
}

void
tfg::populate_suffixpaths()
{
  autostop_timer func_timer(__func__);
  map<pc, tfg_suffixpath_val_t> vals;
  suffixpath_computation sfp_analysis(this, vals);
  sfp_analysis.initialize(mk_epsilon_ec<pc,tfg_edge>());
  sfp_analysis.solve();
  
  m_suffixpaths.clear();
  for (const auto &v : vals) {
    m_suffixpaths.insert(make_pair(v.first, v.second.get_path()));
  }
}



}


