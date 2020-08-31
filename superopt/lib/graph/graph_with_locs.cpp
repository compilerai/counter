#include "graph/expr_loc_visitor.h"
#include "graph/graph_with_locs.h"
//#include "cg/corr_graph.h"
//#include "tfg/tfg.h"
#include "gsupport/tfg_edge.h"
#include "gsupport/corr_graph_edge.h"
#include "graph/liveness_dfa.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_locs<T_PC, T_N, T_E, T_PRED>::graph_to_stream(ostream& ss) const
{
  this->graph_with_predicates<T_PC, T_N, T_E, T_PRED>::graph_to_stream(ss);
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
graph_with_locs<T_PC, T_N, T_E, T_PRED>::graph_with_locs(istream& in, string const& name, context* ctx) : graph_with_predicates<T_PC, T_N, T_E, T_PRED>(in, name, ctx)
{
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
string graph_with_locs<T_PC, T_N, T_E, T_PRED>::read_locs(istream& in, string line)
{
  context *ctx = this->get_context();
  ASSERT(is_line(line, "=Locs in "));
  getline(in, line);
  map<graph_loc_id_t, graph_cp_location> locs;
  while (is_line(line, "=Loc ")) {
    size_t index = line.find(" in ");
    ASSERT(index != string::npos);
    graph_loc_id_t loc_id = stoi(line.substr(5, index));
    graph_cp_location loc;
    line = read_loc(in, loc);
    locs[loc_id] = loc;
  }
  while (line == "") {
    bool end = !getline(in, line);
    ASSERT(!end);
  }
  this->set_locs(locs);
  return line;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
string graph_with_locs<T_PC, T_N, T_E, T_PRED>::read_loc(istream& in, graph_cp_location &loc) const
{
  context *ctx = this->get_context();
  string line;
  getline(in, line);
  if (is_line(line, "REGMEM")) {
    loc.m_type = GRAPH_CP_LOCATION_TYPE_REGMEM;
    bool end = !getline(in, line);
    ASSERT(!end);
    loc.m_varname = mk_string_ref(line);
    expr_ref e;
    line = read_expr(in, e, ctx);
    //size_t remaining;
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    //size_t regname_start = line.find(G_REGULAR_REG_NAME);
    //ASSERT(regname_start != string::npos);
    //int group = stoi(line.substr(regname_start + strlen(G_REGULAR_REG_NAME)), &remaining);
    //ASSERT(line.at(regname_start + strlen(G_REGULAR_REG_NAME) + remaining) == '.');
    //int reg = stoi(line.substr(regname_start + strlen(G_REGULAR_REG_NAME) + remaining + 1));
    loc.m_var = e;
    //cout << __func__ << " " << __LINE__ << ": line =\n" << line << endl;
  } else if (is_line(line, "LLVMVAR")) {
    loc.m_type = GRAPH_CP_LOCATION_TYPE_LLVMVAR;
    bool end = !getline(in, line);
    ASSERT(!end);
    loc.m_varname = mk_string_ref(line);
    expr_ref e;
    line = read_expr(in, e, ctx);
    loc.m_var = e;
    //getline(in, line);
  /*} else if (is_line(line, "IO")) {
    expr_ref e;
    line = read_expr(in, e, ctx);
    loc.m_type = GRAPH_CP_LOCATION_TYPE_IO;
    loc.m_addr = e;*/
    //cout << __func__ << " " << __LINE__ << ": line =\n" << line << endl;
  } else if (is_line(line, "SLOT")) {
    memlabel_t ml;
    getline(in, line);
    ASSERT(is_line(line, "=memname"));
    getline(in, line);
    string memname = line;
    getline(in, line);
    ASSERT(is_line(line, "=addr"));
    expr_ref addr;
    line = read_expr(in, addr, ctx);
    ASSERT(is_line(line, "=nbytes"));
    getline(in, line);
    int nbytes = stoi(line);
    getline(in, line);
    ASSERT(is_line(line, "=bigendian"));
    getline(in, line);
    bool bigendian = stob(line);
    loc.m_type = GRAPH_CP_LOCATION_TYPE_MEMSLOT;
    loc.m_memname = mk_string_ref(memname);
    loc.m_reg_type = reg_type_memory;
    loc.m_reg_index_i = -1;
    loc.m_addr = addr;
    loc.m_nbytes = nbytes;
    loc.m_bigendian = bigendian;
    getline(in, line);
    //cout << __func__ << " " << __LINE__ << ": line =\n" << line << endl;
  } else if (is_line(line, "MASKED")) {
    getline(in, line);
    ASSERT(is_line(line, "=memname"));
    getline(in, line);
    string memname = line;
    getline(in, line);
    ASSERT(is_line(line, "=memlabel"));
    getline(in, line);
    memlabel_t ml;
    memlabel_t::memlabel_from_string(&ml, line.c_str());
    loc.m_type = GRAPH_CP_LOCATION_TYPE_MEMMASKED;
    loc.m_reg_type = reg_type_memory;
    loc.m_reg_index_i = -1;
    loc.m_memname = mk_string_ref(memname);
    loc.m_memlabel = mk_memlabel_ref(ml);
    loc.m_addr = ctx->mk_zerobv(DWORD_LEN);
    loc.m_nbytes = -1;
    getline(in, line);
    //cout << __func__ << " " << __LINE__ << ": line =\n" << line << endl;
  } else NOT_REACHED();/*{
    loc.m_type = GRAPH_CP_LOCATION_TYPE_REGMEM;
    loc.m_varname = mk_string_ref(line);
    getline(in, line);
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    //NOT_REACHED();
  }*/
  //cout << __func__ << " " << __LINE__ << ": line =\n" << line << endl;
  while (line == "") {
    bool end = !getline(in, line);
    ASSERT(!end);
  }
  return line;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
set<T_PC> graph_with_locs<T_PC, T_N, T_E, T_PRED>::get_nodes_where_live_locs_added(map<T_PC, set<graph_loc_id_t>> const &orig, map<T_PC, set<graph_loc_id_t>> const &cur)
{
  set<T_PC> ret;
  for (const auto &c : cur) {
    if (orig.count(c.first) == 0 || !set_contains(orig.at(c.first), c.second)) {
      ret.insert(c.first);
    }
  }
  return ret;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
bool graph_with_locs<T_PC, T_N, T_E, T_PRED>::loc_is_atomic(graph_cp_location const &loc)
{
  if (   loc.m_type == GRAPH_CP_LOCATION_TYPE_REGMEM
      || loc.m_type == GRAPH_CP_LOCATION_TYPE_LLVMVAR) {
    return true;
  }
  return false;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void graph_with_locs<T_PC, T_N, T_E, T_PRED>::graph_locs_visit_exprs(std::function<expr_ref (T_PC const &, const string&, expr_ref)> f, map<graph_loc_id_t, graph_cp_location> &locs_map)
{
  map<graph_loc_id_t, graph_cp_location> &locs = locs_map;

  for (auto &locm : locs) {
    graph_cp_location &loc = locm.second;
    if (loc.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
      loc.m_addr = f(T_PC::start()/*p*/, G_TFG_LOC_IDENTIFIER, loc.m_addr);
    }
  }
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
set<expr_ref>
graph_with_locs<T_PC, T_N, T_E, T_PRED>::get_live_loc_exprs_at_pc(T_PC const &p) const
{
  set<expr_ref> ret;
  for (const auto &locid : this->get_live_locids(p)) {
    ret.insert(this->graph_loc_get_value_for_node(locid));
  }
  return ret;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
set<expr_ref>
graph_with_locs<T_PC, T_N, T_E, T_PRED>::get_spreled_loc_exprs_at_pc(T_PC const &p) const
{
  const auto &sprel_map = this->get_sprels().at(p);
  set<expr_ref> ret;
  for (auto const& loc : this->get_locs()) {
    if (sprel_map.has_mapping_for_loc(loc.first)) {
      ret.insert(graph_loc_get_value_for_node(loc.first));
    }
  }
  return ret;
}

//template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
//set<expr_ref>
//graph_with_locs<T_PC, T_N, T_E, T_PRED>::get_live_loc_exprs_having_sprel_map_at_pc(T_PC const &p) const
//{
//  set<expr_ref> ret;
//  for (const auto &locid : this->get_live_locids(p)) {
//    if(this->loc_has_sprel_mapping_at_pc(p, locid))
//      ret.insert(this->graph_loc_get_value_for_node(locid));
//  }
//  return ret;
//}

//template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
//map<graph_loc_id_t, graph_cp_location>
//graph_with_locs<T_PC, T_N, T_E, T_PRED>::::get_live_locs_across_pcs(T_PC const& from_pc, T_PC const& to_pc) const
//{
//  set<T_PC> pcs = collect_all_intermediate_pcs(from_pc, to_pc);
//  if (from_pc != to_pc) {
//    pcs.erase(from_pc); // remove from_pc
//  }
//  map<graph_loc_id_t, graph_cp_location> ret;
//  for (auto const& pc : pcs) {
//    auto live_at_this = this->get_live_locs(pc);
//    ret.insert(live_at_this.begin(), live_at_this.end());
//  }
//  return ret;
//}

//template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
//template<typename T_MAP_VALUETYPE>
//void
//graph_with_locs<T_PC, T_N, T_E, T_PRED>::edgeloc_map_eliminate_unused_loc(map<pair<edge_id_t<T_PC>, graph_loc_id_t>, T_MAP_VALUETYPE> &elmap, graph_loc_id_t loc_id)
//{
//  list<pair<edge_id_t<T_PC>, graph_loc_id_t>> keys_to_remove;
//  for (const auto &ell : elmap) {
//    if (ell.first.second == loc_id) {
//      keys_to_remove.push_back(ell.first);
//    }
//  }
//  for (const auto &k : keys_to_remove) {
//    elmap.erase(k);
//  }
//}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
bool graph_with_locs<T_PC, T_N, T_E, T_PRED>::expr_is_possible_loc(expr_ref e)
{
  return e->is_var() || e->get_operation_kind() == expr::OP_SELECT || e->get_operation_kind() == expr::OP_MEMMASK;
}

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_locs<T_PC, T_N, T_E, T_PRED>::graph_locs_remove_duplicates_mod_comments(map<graph_loc_id_t, graph_cp_location> &locs_map)
{
  //autostop_timer func_timer(__func__);
  map<graph_loc_id_t, graph_cp_location> &start_graph_locs = locs_map; //m_locs; //.at(T_PC::start());
  map<graph_loc_id_t, bool> is_redundant;
  graph_locs_remove_duplicates_mod_comments_for_pc(start_graph_locs, is_redundant);

  CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << ": graph:\n" << this->to_string(/*cs*/) << endl);
  map<graph_loc_id_t, graph_cp_location> const& graph_locs_old = locs_map;
  map<graph_loc_id_t, graph_cp_location> graph_locs;
  for (const auto &loc : graph_locs_old) {
    if (is_redundant.at(loc.first)) {
      continue;
    }
    graph_locs[loc.first] = loc.second;
  }
  locs_map = graph_locs;
  CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << ": graph:\n" << this->to_string() << endl);
  CPP_DBG_EXEC2(TFG_LOCS,
      cout << __func__ << " " << __LINE__ << ": num graph locs: " << locs_map.size() << endl;
      for (auto const& loc : locs_map) {
        cout << loc.first << ": " << loc.second.to_string() << endl;
      }
  );
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class expr_get_locs_used_visitor : public expr_loc_visitor<T_PC, T_N, T_E, T_PRED, set<graph_loc_id_t>>
{
public:
  expr_get_locs_used_visitor(
      expr_ref const &e,
      map<graph_loc_id_t, graph_cp_location> const &locs,
      graph_with_locs<T_PC, T_N, T_E, T_PRED> const& graph
    ) : expr_loc_visitor<T_PC, T_N, T_E, T_PRED, set<graph_loc_id_t>>(e->get_context(), graph), m_locs(locs), m_ctx(e->get_context()), m_cs(m_ctx->get_consts_struct())
  {
    m_stackslot_locs = compute_stackslot_locs();
  }
  set<graph_loc_id_t> visit(expr_ref const &e, bool is_loc, set<graph_loc_id_t> const &locs, vector<set<graph_loc_id_t>> const &argvals)
  {
    if (is_loc) {
      return locs;
    } else {
      if (e->is_const() || e->is_var()) {
        return {};
      }
      set<graph_loc_id_t> locs;
      for (const auto &argval : argvals) {
        set_union(locs, argval);
      }

      if (   e->get_operation_kind() == expr::OP_FUNCTION_CALL
          && m_cs.expr_is_fcall_with_unknown_args(e->get_args().at(0))) {
        set_union(locs, m_stackslot_locs);
      }
      return locs;
    }
  }

private:
  set<graph_loc_id_t> compute_stackslot_locs() const
  {
    set<graph_loc_id_t> ret;
    for (auto const& l : m_locs) {
      if (   l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT
          && memlabel_t::memlabel_is_stack(&l.second.m_memlabel->get_ml())) {
        ret.insert(l.first);
      }
    }
    return ret;
  }

private:
  map<graph_loc_id_t, graph_cp_location> const &m_locs;
  set<graph_loc_id_t> m_stackslot_locs;
  context *m_ctx;
  consts_struct_t const &m_cs;
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
set<graph_loc_id_t>
graph_with_locs<T_PC, T_N, T_E, T_PRED>::get_locs_potentially_read_in_expr_using_locs_map(expr_ref const &e, map<graph_loc_id_t, graph_cp_location> const &locs) const
{
  expr_get_locs_used_visitor<T_PC, T_N, T_E, T_PRED> visitor(e, locs, *this);
  return visitor.visit_recursive(e, {});
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_locs<T_PC, T_N, T_E, T_PRED>::populate_loc_liveness()
{
  autostop_timer func_timer(__func__);
  map<T_PC, livelocs_val_t<T_PC, T_N, T_E, T_PRED>> vals;
  liveness_analysis<T_PC, T_N, T_E, T_PRED> l(this, vals);
  l.initialize(livelocs_val_t<T_PC, T_N, T_E, T_PRED>::get_boundary_value(this));
  l.solve();

  map<T_PC, set<graph_loc_id_t>> loc_liveness;
  for (auto const& v : vals) {
    loc_liveness.insert(make_pair(v.first, v.second.get_livelocs()));
  }
  this->set_loc_liveness(loc_liveness);
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_locs<T_PC, T_N, T_E, T_PRED>::collect_memory_accesses(set<T_PC> const &nodes_changed, map<T_PC, set<eqspace::state::mem_access>> &state_mem_acc_map)
{
  //cout << __func__ << " " << __LINE__ << ": nodes_changed.size() = " << nodes_changed.size() << endl;
  for (auto from_pc : nodes_changed) {
    list<shared_ptr<T_E const>> outgoing;
    this->get_edges_outgoing(from_pc, outgoing);

    state_mem_acc_map[from_pc] = set<eqspace::state::mem_access>();
    //cout << __func__ << " " << __LINE__ << ": looking at outgoing edges of node " << from_pc << ": outgoing.size() = " << outgoing.size() << endl;
    for (auto ed : outgoing) {
      T_PC const &from_pc = ed->get_from_pc();
      if (!this->get_sprels().count(from_pc)) {
        cout << __func__ << " " << __LINE__ << ": from_pc = " << from_pc.to_string() << endl;
      }
      const auto &sprel_map = this->get_sprels().at(from_pc);
      auto const& memlabel_map = this->get_memlabel_map();
      CPP_DBG_EXEC2(SPLIT_MEM, cout << __func__ << " " << __LINE__ << ": looking at edge " << ed->to_string(/*NULL*/) << endl);
      set<eqspace::state::mem_access> new_acc;
      state const &to_state = ed->get_to_state();
      expr_ref const &econd = ed->get_cond();
      list<expr_ref> exprs;
      string prefix = string("Edge:") + ed->to_string();
      to_state.get_regs(exprs);
      for (auto const& expr : exprs) {
        CPP_DBG_EXEC2(SPLIT_MEM, cout << __func__ << " " << __LINE__ << ": collecting memory exprs in original expr:\n" << this->get_context()->expr_to_string_table(expr) << endl);
        state::find_all_memory_exprs_for_all_ops(prefix + ".to_state", expr, new_acc);
        expr_ref expr_simplified = this->get_context()->expr_simplify_using_sprel_and_memlabel_maps(expr, sprel_map, memlabel_map);
        CPP_DBG_EXEC2(SPLIT_MEM, cout << __func__ << " " << __LINE__ << ": collecting memory exprs in simplified expr:\n" << this->get_context()->expr_to_string_table(expr_simplified) << endl);
        state::find_all_memory_exprs_for_all_ops(prefix + ".to_state", expr_simplified, new_acc);
      }
      state::find_all_memory_exprs_for_all_ops(prefix + ".cond", econd, new_acc);
      expr_ref econd_simplified = this->get_context()->expr_simplify_using_sprel_and_memlabel_maps(econd, sprel_map, memlabel_map);
      state::find_all_memory_exprs_for_all_ops(prefix + ".cond", econd_simplified, new_acc);
      //set<eqspace::state::mem_access> new_acc = ed->get_all_mem_accesses(string("Edge:") + ed->to_string());
      //state_mem_acc_map[from_pc].insert(state_mem_acc_map[from_pc].end(), new_acc.begin(), new_acc.end());
      set_union(state_mem_acc_map[from_pc], new_acc);
    }
  }

  for (auto p : nodes_changed) {
    predicate_set_t preds = this->get_assume_preds(p);
    unordered_set_union(preds, this->get_assert_preds(p));
    set<state::mem_access> ret;
    for (const auto &pr : preds) {
      string prefix = string("Node.") + p.to_string() + ".pred." + pr->get_comment();
      std::function<void (const string&, expr_ref const &)> f = [&prefix, &ret](string const &fieldname, expr_ref const &e)
      {
        state::find_all_memory_exprs_for_all_ops(prefix + "." + fieldname, e, ret);
      };
      pr->visit_exprs(f);
      //pair<expr_ref, pair<expr_ref, expr_ref>> ep = pr.get_expr_tuple();
      //state::find_all_memory_exprs_for_all_ops(prefix + ".guard", ep.first, ret);
      //state::find_all_memory_exprs_for_all_ops(prefix + ".lhs_expr", ep.second.first, ret);
      //state::find_all_memory_exprs_for_all_ops(prefix + ".rhs_expr", ep.second.second, ret);
    }
    set_union(state_mem_acc_map[p], ret);
    CPP_DBG_EXEC2(TFG_LOCS,
        cout << __func__ << " " << __LINE__ << ": collected memory accesses at " << p.to_string() << ":" << endl;
        for (const auto &ma : state_mem_acc_map.at(p)) {
        cout << ma.to_string() << endl;
        }
        );
  }
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_locs<T_PC, T_N, T_E, T_PRED>::refresh_graph_locs(map<graph_loc_id_t, graph_cp_location> const &llvm_locs)
{
  autostop_timer func_timer(__func__);
  //consts_struct_t const &cs = this->get_consts_struct();

  context* ctx = this->get_context();
  map<graph_loc_id_t, graph_cp_location> const old_locs = this->get_locs();
  map<graph_loc_id_t, graph_cp_location> locs_map;
  //m_locs.clear();

  //this->graph_init_locs_for_all_pcs();
  CPP_DBG_EXEC2(GENERAL, cout << __func__ << " " << __LINE__ << ": adding locs exvregs" << endl);
  this->graph_locs_add_all_exvregs(locs_map, old_locs);

  this->graph_locs_add_all_llvmvars(locs_map, old_locs);

  this->graph_locs_add_all_bools_and_bitvectors(locs_map, old_locs);

  this->graph_add_location_slots_and_memmasked_using_llvm_locs(locs_map, llvm_locs);

  //this->graph_locs_add_io();

  map<T_PC, set<state::mem_access>> state_mem_acc_map;

  CPP_DBG_EXEC2(GENERAL, cout << __func__ << " " << __LINE__ << ": collecting memory accesses" << endl);
  set<T_PC> all_pcs = this->get_all_pcs();
  this->collect_memory_accesses(all_pcs, state_mem_acc_map);

  CPP_DBG_EXEC2(GENERAL, cout << __func__ << " " << __LINE__ << ": adding location slots" << endl);
  graph_add_location_slots_using_state_mem_acc_map(locs_map, state_mem_acc_map, old_locs);

  vector<memlabel_ref> relevant_memlabels;
  this->graph_get_relevant_memlabels(relevant_memlabels);
  vector<string> memnames = this->get_start_state().get_memnames();
  for (auto const& memname : memnames) {
    for (auto const& ml : relevant_memlabels) {
      this->graph_locs_add_location_memmasked(locs_map, memname, ml, ctx->mk_zerobv(DWORD_LEN), MEMSIZE_MAX, old_locs);
    }
  }
  CPP_DBG_EXEC2(GENERAL, cout << __func__ << " " << __LINE__ << ": calling graph_locs_remove_duplicates_mod_comments" << endl);

  this->graph_locs_remove_duplicates_mod_comments(locs_map);

  this->set_locs(locs_map);

  DYN_DEBUG(tfg_locs,
      set<graph_loc_id_t> added;
      set<graph_loc_id_t> removed;
      set<graph_loc_id_t> const new_locids = map_get_keys(locs_map);
      set<graph_loc_id_t> const old_locids = map_get_keys(old_locs);
      set_difference(new_locids, old_locids, added);
      set_difference(old_locids, new_locids, removed);
      cout << _FNLN_ << ": added: " << loc_set_to_string(added) << endl;
      cout << _FNLN_ << ": removed: " << loc_set_to_string(removed) << endl;
   );
  DYN_DEBUG(tfg_locs, cout << _FNLN_ << ": new locs:\n"; this->print_graph_locs());

  CPP_DBG_EXEC2(EQGEN, cout << __func__ << " " << __LINE__ << ": returning, TFG:\n" << this->graph_to_string(/*true*/));
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_locs<T_PC, T_N, T_E, T_PRED>::init_graph_locs(graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs)
{
  autostop_timer func_timer(__func__);
  consts_struct_t const &cs = this->get_consts_struct();
  bool changed;

  int iter = 0;
  do {
    DYN_DEBUG2(tfg_locs, cout << __func__ << " " << __LINE__ << ": before refresh_graph_locs, TFG:\n" << this->graph_to_string(/*true*/));

    DYN_DEBUG(tfg_locs, cout << _FNLN_ << ": Calling refresh_graph_locs iter " << iter << '\n');
    refresh_graph_locs(llvm_locs);

    DYN_DEBUG2(tfg_locs, cout << __func__ << " " << __LINE__ << ": after refresh_graph_locs and before substituting sprels, TFG:\n" << this->graph_to_string(/*true*/));

    DYN_DEBUG(tfg_locs, cout << _FNLN_ << ": Calling graph_init_memlabels_to_top, iter " << iter << '\n');
    this->graph_init_memlabels_to_top(update_callee_memlabels);

    DYN_DEBUG(tfg_locs, cout << _FNLN_ << ": Calling propagate_sprels, iter " << iter << '\n');
    changed = populate_auxilliary_structures_dependent_on_locs();

    this->populate_loc_liveness(/*false*/); //redo this in light of loc update
    CPP_DBG_EXEC2(EQGEN, cout << __func__ << " " << __LINE__ << ": after populate_loc_liveness, TFG:\n" << this->graph_to_string(/*true*/));

    iter++;
  } while (changed);

  CPP_DBG_EXEC2(EQGEN, cout << __func__ << " " << __LINE__ << ": after fixpoint loop, TFG:\n" << this->graph_to_string());

  CPP_DBG_EXEC(EQGEN, cout << "num graph locs = " << this->get_locs().size() << endl);
  CPP_DBG_EXEC2(EQGEN, print_graph_locs());

  CPP_DBG_EXEC2(EQGEN, cout << __func__ << " " << __LINE__ << ": loc liveness =\n"; print_loc_liveness(););
  CPP_DBG(EQGEN, "%s", "done\n");
}



template class graph_with_locs<pc, tfg_node, tfg_edge, predicate>;
template class graph_with_locs<pcpair, corr_graph_node, corr_graph_edge, predicate>;

}
