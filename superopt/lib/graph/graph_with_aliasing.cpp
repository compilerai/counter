#include "graph/graph_loc_id.h"
#include "graph/graph_with_aliasing.h"
#include "eq/corr_graph.h"
//#include "tfg/tfg.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::graph_to_stream(ostream& ss) const
{
  this->graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED>::graph_to_stream(ss);

  ss << endl << "=Locs in " + this->get_name()->get_str() << endl;
  for (const auto &loc : this->get_locs()) {
    ss << "=Loc " << loc.first << " in " << this->get_name()->get_str() + "." << endl << loc.second.to_string_for_eq() << endl;
  }

  list<shared_ptr<T_N>> ns;
  this->get_nodes(ns);
  ss << endl << "=Liveness in " << this->get_name()->get_str() << endl;
  for (auto n : ns) {
    ss << "=live locs at " << n->get_pc().to_string() << endl;
    ss << loc_set_to_string(this->get_live_locids(n->get_pc())) << endl;
  }

  ss << endl << "=sprel_maps in " + this->get_name()->get_str() << endl;
  for (auto n : ns) {
    ss << "=sprel_map at " + n->get_pc().to_string() + " in " + this->get_name()->get_str() << endl;
    if (!this->get_sprels().count(n->get_pc())) {
      continue;
    }
    ss << this->get_sprel_map(n->get_pc()).to_string_for_eq();
  }
  ss << "=graph_with_aliasing_done\n";
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::graph_with_aliasing(istream& in, string const& name, context* ctx) : graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED>(in, name, ctx), m_alias_analysis(this, true, this->get_memlabel_map_ref())
{
  string line;

  getline(in, line);
  while (line == "") {
    getline(in, line);
  }

  if (!is_line(line, "=Locs in ")) {
    cout << "line = " << line << endl;
  }
  ASSERT(is_line(line, "=Locs"));
  line = this->read_locs(in, line);

  while (line == "") {
    getline(in, line);
  }

  ASSERT(is_line(line, "=Liveness in "));

  getline(in, line);
  string live_at_pc_prefix = "=live locs at ";
  while (is_line(line, live_at_pc_prefix)) {
    T_PC p = T_PC::create_from_string(line.substr(live_at_pc_prefix.length()));
    getline(in, line);
    this->add_live_locs(p, loc_set_from_string(line));
    getline(in, line);
  }

  while (line == "") {
    getline(in, line);
  }

  auto locs_map = this->get_locs();
  this->graph_populate_relevant_addr_refs();
  this->graph_locs_compute_and_set_memlabels(locs_map);
  this->set_locs(locs_map);
  this->populate_locid2expr_map();

  ASSERT(is_line(line, "=sprel_maps in "));
  getline(in, line);

  string sprel_map_at_pc_prefix = "=sprel_map at ";
  while (is_line(line, sprel_map_at_pc_prefix)) {
    size_t index = line.find(" in ");
    ASSERT(index != string::npos);
    T_PC p = T_PC::create_from_string(line.substr(sprel_map_at_pc_prefix.size(), index));
    getline(in, line);
    string loc_prefix = "=loc ";
    while (is_line(line, loc_prefix)) {
      graph_loc_id_t loc_id = string_to_int(line.substr(loc_prefix.length()));
      expr_ref e;
      line = read_expr(in, e, ctx);
      this->add_sprel_mapping(p, loc_id, e);
    }
    this->set_remaining_sprel_mappings_to_bottom(p);
  }
  ASSERT(is_line(line, "=graph_with_aliasing_done"));
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
string graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::to_string_with_linear_relations(map<T_PC, map<graph_loc_id_t, lr_status_ref>> const &linear_relations, bool details) const
{
  list<shared_ptr<T_N>> nodes;
  consts_struct_t const &cs = this->get_consts_struct();
  stringstream ss;

  this->get_nodes(nodes);

  for (auto n : nodes) {
    T_PC const &p = n->get_pc();
    ss << p.to_string() << endl;
    map<graph_loc_id_t, graph_cp_location> const &graph_locs = this->get_locs();
    ss << "  graph locs" << endl;
    for (const auto &graph_loc : graph_locs) {
      if (this->get_loc_liveness().count(p) && this->get_loc_liveness().at(p).count(graph_loc.first) == 0) {
        continue;
      } else {
        ss << "  loc " << graph_loc.first << ". " << graph_loc.second.to_string();
        if (linear_relations.count(p)) {
          map<graph_loc_id_t, lr_status_ref> const &lr_map = linear_relations.at(p);
          if (lr_map.count(graph_loc.first)) {
            memlabel_t ml = lr_map.at(graph_loc.first)->to_memlabel(cs, this->get_symbol_map(), this->get_locals_map());
            ss << " : " << ml.to_string();
          }
        }
        ss << endl;
      }
    }
    unordered_set<shared_ptr<T_PRED const>> const &preds = this->get_assume_preds(p);
    size_t pred_num = 0;
    ss << "  assumes" << endl;
    for (const auto &pred : preds) {
      ss << "  " << pred_num << ".\n" << pred->to_string(true) << endl;
      pred_num++;
    }
    unordered_set<shared_ptr<T_PRED const>> const &asserts = this->get_assert_preds(p);
    if (preds.size() > 0) {
      size_t pred_num = 0;
      ss << "  asserts" << endl;
      for (const auto &pred : asserts) {
        ss << "  " << pred_num << ".\n" << pred->to_string(true) << endl;
        pred_num++;
      }
    }
  }

  list<shared_ptr<T_E const>> edges;
  this->get_edges(edges);
  for (auto e : edges) {
    if (details) {
      ss << "\nEdge: " << e->to_string(/*&this->get_start_state()*/) << "\n";
    } else {
      ss << e->to_string(/*NULL*/) << ", ";
    }
  }

  ss << "\nSprels\n";
  for (const auto &pc_sprel_map : this->get_sprels()) {
    ss << pc_sprel_map.first.to_string() << ":\n";
    ss << pc_sprel_map.second.to_string();
  }

  ss << "\nMemlabel-map\n";
  ss << this->memlabel_map_to_string_for_eq() << endl;

  return ss.str();
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::graph_populate_relevant_addr_refs()
{
  consts_struct_t const &cs = this->get_consts_struct();
  m_relevant_addr_refs.clear();
  for (cs_addr_ref_id_t i = 0; i < cs.get_num_addr_ref(); i++) {
    pair<string, expr_ref> pp = cs.get_addr_ref(i);
    string const &name = pp.first;
    if (name.substr(0, g_symbol_keyword.size()) == g_symbol_keyword) {
      string prefix = g_symbol_keyword + ".";
      symbol_id_t symbol_id = stoi(name.substr(prefix.size()));
      if (   this->get_symbol_map().count(symbol_id)/*
          && !m_symbol_map.at(symbol_id).is_dst_const_symbol()*/) {
        m_relevant_addr_refs.insert(i);
      }
    } else if (name.substr(0, g_local_keyword.size()) == g_local_keyword) {
      string prefix = g_local_keyword + ".";
      size_t local_id = stoi(name.substr(prefix.size()));
      if (this->get_locals_map().count(local_id)) {
        m_relevant_addr_refs.insert(i);
      }
    } else if (name.substr(0, g_stack_symbol.size()) == g_stack_symbol) {
      m_relevant_addr_refs.insert(i);
    } else NOT_REACHED();
  }
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::split_memory_in_graph_and_propagate_sprels_until_fixpoint(bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs)
{
  autostop_timer func_timer(__func__);

  bool changed;
  int iter = 0;
  this->populate_transitive_closure();
  this->populate_reachable_and_unreachable_incoming_edges_map();

  split_memory_in_graph_initialize(update_callee_memlabels, llvm_locs);

  set<shared_ptr<T_E const>> all_edges = this->get_edges();
  set<T_PC> all_from_pcs = this->get_from_pcs_for_edges(all_edges);
  map<T_PC, sprel_map_t> prev_sprels;
  do {
    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": iteration " << iter << " started.\n");
    changed = false;
    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": calling split_memory_in_graph iter " << iter << endl);

    split_memory_in_graph(nullptr);

    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": after split_memory_in_graph, iter " << iter << ", memlabel_map:\n" << memlabel_map_to_string(this->get_memlabel_map()) << endl);
    CPP_DBG_EXEC2(GENERAL, cout << __func__ << " " << __LINE__ << ": calling simplify_edges iter " << iter << endl);
    CPP_DBG_EXEC2(GENERAL, cout << __func__ << " " << __LINE__ << ": calling propagate_sprels iter " << iter << endl);
    this->populate_locs_potentially_modified_on_edge(nullptr);
    // populate_loc_liveness uses simplified edgecond in its computation
    this->populate_simplified_edgecond(nullptr);
    this->propagate_sprels();
    changed = this->sprels_are_different(prev_sprels, this->get_sprels()) || changed;
    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": after propagate_sprels, iter " << iter << ", memlabel_map:\n" << memlabel_map_to_string(this->get_memlabel_map()) << endl);
    if (changed) {
      prev_sprels = this->get_sprels();
      DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": calling split_memory_in_graph_initialize iter " << iter << endl);
      split_memory_in_graph_initialize(update_callee_memlabels, llvm_locs);
    }
    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": iteration " << iter << " finished.\n");
    iter++;
    ASSERT(iter < 50);
  } while (changed);

  // update simplified counterparts in view of updated alias analysis
  this->populate_simplified_edgecond(nullptr);
  this->populate_simplified_to_state(nullptr);
  this->populate_simplified_assumes(nullptr);
  this->populate_simplified_assert_map(this->get_all_pcs());
  this->populate_branch_affecting_locs();
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::edge_update_memlabel_map_for_mlvars(shared_ptr<T_E const> const& e, map<graph_loc_id_t, lr_status_ref> const& in, bool update_callee_memlabels, graph_memlabel_map_t& memlabel_map) const
{
  autostop_timer func_timer(__func__);

  T_PC const &p = e->get_from_pc();
  sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(p);

  DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << ", to_state =\n" << e->get_to_state().to_string() << endl);
  {
    autostop_timer func1_timer(string(__func__) + ".populate_memlabel_map");
    std::function<void (string const &, expr_ref)> f = [this,update_callee_memlabels,&memlabel_map,&in,&sprel_map_pair](string const &key, expr_ref e)
    {
      if (e->is_const() || e->is_var()) { //common case
        return;
      }
      this->populate_memlabel_map_for_expr_and_loc_status(e, this->get_locs(), in, sprel_map_pair, this->graph_get_relevant_addr_refs(), this->get_argument_regs(), this->get_symbol_map(), this->get_locals_map(), update_callee_memlabels, memlabel_map/*, this->m_get_callee_summary_fn, this->m_expr2locid_fn*/);
    };
    e->visit_exprs_const(f);

    autostop_timer func2_timer(string(__func__) + ".populate_memlabel_map.update_potentially_mod_locs");
    map<graph_loc_id_t, expr_ref> &loc2expr_map = this->get_locs_potentially_written_on_edge(e);
    for (auto const& loc_expr : this->compute_simplified_loc_exprs_for_edge(e, this->get_locids_potentially_written_on_edge(e), /*force_update*/true)) {
      graph_loc_id_t locid = loc_expr.first;
      expr_ref const& ex   = loc_expr.second;
      loc2expr_map[locid] = ex;
    }
  }
}

template class graph_with_aliasing<pc, tfg_node, tfg_edge, predicate>;
template class graph_with_aliasing<pcpair, corr_graph_node, corr_graph_edge, predicate>;

}
