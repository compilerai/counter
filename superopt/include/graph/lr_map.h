#pragma once
//#include "eqcheck/common.h"
#include "expr/context.h"
#include "expr/consts_struct.h"
//#include "graph/pc.h"
#include "support/debug.h"
#include "graph/graph_cp_location.h"
#include "graph/callee_summary.h"
#include "expr/state.h"
#include "support/types.h"
#include "expr/memlabel.h"
#include "support/timers.h"
#include "graph/lr_status.h"
#include "expr/graph_arg_regs.h"

namespace eqspace {
//class tfg;
struct consts_struct_t;

//#define CONSTANTS_LOC_ID -1

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_predicates;

//lr_status_t lr_status_meet(lr_status_t const &a, lr_status_t const &b);
//string lr_status_to_string(lr_status_t const &lrs);
//string get_string_status(eq::consts_struct_t const &cs, cs_addr_ref_id_t lr_addr_ref_id, bool independent_of_input_stack_pointer, bool is_top, bool is_bottom);
//lr_status_t compute_lr_status_for_expr(expr_ref e, eq::state const &from_state, int cs_addr_ref_id, map<int, lr_status_t> const &prev_lr, map<graph_loc_id_t, graph_cp_location> const &start_pc_locs, eq::consts_struct_t const &cs);


#if 0
template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class lr_map_t
{
private:
  typedef graph_with_predicates<T_PC, T_N, T_E, T_PRED> graph_with_predicates_instance;
  map<graph_loc_id_t, lr_status_t> m_loc_map;
  map<addr_id_t, lr_status_t> m_addr_map;
public:
  map<graph_loc_id_t, lr_status_t> const &get_loc_map() const { return m_loc_map; }
  map<graph_loc_id_t, lr_status_t> &get_loc_map() { return m_loc_map; }
  map<addr_id_t, lr_status_t> const &get_addr_map() const { return m_addr_map; }
  map<addr_id_t, lr_status_t> &get_addr_map() { return m_addr_map; }
  lr_status_t const &get_loc_status(graph_loc_id_t loc_id) const { return m_loc_map.at(loc_id); }
  lr_status_t const &get_addr_status(addr_id_t addr_id) const { return m_addr_map.at(addr_id); }
  bool loc_exists(graph_loc_id_t loc_id) const { return m_loc_map.count(loc_id) != 0; }
  bool addr_exists(addr_id_t addr_id) const { return m_addr_map.count(addr_id) != 0; }

  /*static void print_linear_relations(graph_with_predicates_instance const &graph, map<T_PC, lr_map_t<T_PC, T_N, T_E, T_PRED>> const &linear_relations)
  {
    for (const auto &lr : linear_relations) {
      cout << lr.first.to_string() << ":\n";
      cout << lr.second.to_string(&graph, lr.first, graph.get_all_pcs(), graph.get_consts_struct(), "  ");
    }
  }*/


  /*map<graph_loc_id_t, lr_status_t> const &get_linearly_related_locs_for_addr_ref_id(cs_addr_ref_id_t cs_addr_ref_id) const
  {
    return m_loc_map.at(cs_addr_ref_id);
  }

  map<graph_loc_id_t, lr_status_t> &get_linearly_related_addrs_for_addr_ref_id(cs_addr_ref_id_t cs_addr_ref_id)
  {
    if (m_addr_map.count(cs_addr_ref_id) == 0) {
      m_addr_map[cs_addr_ref_id] = map<addr_id_t, lr_status_t>();
    }
    return m_addr_map.at(cs_addr_ref_id);
  }

  //map<graph_loc_id_t, lr_status_t> const &get_linearly_related_addrs_for_addr_ref_id(cs_addr_ref_id_t cs_addr_ref_id) const;

  void set_linearly_related_locs_for_addr_ref_id(cs_addr_ref_id_t cs_addr_ref_id, map<graph_loc_id_t, lr_status_t> const &lr_status)
  {
    m_loc_map[cs_addr_ref_id] = lr_status;
  }

  void set_linearly_related_addrs_for_addr_ref_id(cs_addr_ref_id_t cs_addr_ref_id, map<addr_id_t, lr_status_t> const &lr_status)
  {
    m_addr_map[cs_addr_ref_id] = lr_status;
  }*/

  void set_addrs_map_to_top_for_new_addrs(graph_with_predicates_instance const *graph, T_PC const &p)
  {
    for (auto &pr : graph->get_addrs_map(p)) {
      addr_id_t i = pr.first;
      //cout << __func__ << " " << __LINE__ << ": setting lr status to top for addr_id " << i << ", addr_ref " << k << " (" << graph->get_consts_struct().get_addr_ref(k).first << ")" << endl;
      if (m_addr_map.count(i) == 0) {
        //node_changed_map[k].insert(p);
        m_addr_map[i] = lr_status_t::top();
      }
    }
  }

  void set_locs_map_to_start_pc_values(graph_with_predicates_instance const *graph, eqspace::consts_struct_t const &cs)
  {
    autostop_timer func_timer(__func__);

    set<cs_addr_ref_id_t> const &relevant_addr_refs = graph->graph_get_relevant_addr_refs();
    context *ctx = graph->get_context();
    for (auto loc : graph->get_locs()) {
      //pc_locid_pair plp(::pc::start(), i);
      if (   m_loc_map.count(loc.first) == 0
          && graph->loc_is_live_at_pc(T_PC::start(), loc.first)) {
        m_loc_map[loc.first] = lr_status_t::start_pc_value(graph->graph_loc_get_value_for_node(loc.first), relevant_addr_refs, ctx, graph->get_argument_regs(), graph->get_symbol_map());
      } else if (!graph->loc_is_live_at_pc(T_PC::start(), loc.first)) {
        m_loc_map.erase(loc.first);
      }
    }
  }

  void set_locs_map_to_top(/*map<int, map<pc, set<int>, pc_comp>> &changed_map, */set<T_PC> &node_changed_map, graph_with_predicates_instance const *graph, T_PC const &p, list<shared_ptr<T_E>> const &incoming, eqspace::consts_struct_t const &cs)
  {
    bool some_loc_changed = false;
    for (auto loc : graph->get_locs()) {
      //pc_locid_pair plp(::pc::start(), i);
      bool loc_is_live = graph->loc_is_live_at_pc(p, loc.first);
      //cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", loc.first = " << loc.first << ", loc_is_live = " << loc_is_live << endl;
      if (m_loc_map.count(loc.first) == 0 && loc_is_live) {
        m_loc_map[loc.first] = lr_status_t::top();
        some_loc_changed = true;
      } else if (!graph->loc_is_live_at_pc(p, loc.first)) {
        m_loc_map.erase(loc.first);
      }
    }
    for (auto e : incoming) {
      node_changed_map.insert(e->get_from_pc());
    }
  }

  /*long get_linearly_related_addr_ref_id_for_loc(long loc_id, eq::consts_struct_t const &cs) const
  {
    long ret = -1;
    for (map<cs_addr_ref_id_t, map<graph_loc_id_t, lr_status_t> >::const_iterator iter = m_loc_map.begin();
         iter != m_loc_map.end();
         iter++) {
      map<graph_loc_id_t, lr_status_t> const &smap = iter->second;
      for (map<graph_loc_id_t, lr_status_t>::const_iterator it = smap.begin();
           it != smap.end();
           it++) {
        if (it->first == loc_id && it->second == LR_STATUS_LINEARLY_RELATED) {
          if (ret == -1 || ret == cs.get_addr_ref_id_for_keyword(g_stack_symbol)) {
            ret = iter->first;
          }
        }
      }
    }
    return ret;
  }

  void get_bottom_addr_ref_ids_for_loc(graph_loc_id_t loc_id, eq::consts_struct_t const &cs, vector<cs_addr_ref_id_t> &bottom_set) const
  {
    bottom_set.clear();
    for (map<cs_addr_ref_id_t, map<graph_loc_id_t, lr_status_t> >::const_iterator iter = m_loc_map.begin();
         iter != m_loc_map.end();
         iter++) {
      map<graph_loc_id_t, lr_status_t> const &smap = iter->second;
      for (map<graph_loc_id_t, lr_status_t>::const_iterator it = smap.begin();
           it != smap.end();
           it++) {
        if (it->first == loc_id && it->second == LR_STATUS_BOTTOM) {
          bottom_set.push_back(iter->first);
        }
      }
    }
    std::sort(bottom_set.begin(), bottom_set.end());
  }

  void get_bottom_addr_ref_ids_for_addr(addr_id_t addr_id, eq::consts_struct_t const &cs, vector<cs_addr_ref_id_t> &bottom_set) const
  {
    bottom_set.clear();
    for (map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t> >::const_iterator iter = m_addr_map.begin();
         iter != m_addr_map.end();
         iter++) {
      map<addr_id_t, lr_status_t> const &smap = iter->second;
      for (map<addr_id_t, lr_status_t>::const_iterator it = smap.begin();
           it != smap.end();
           it++) {
        if (it->first == addr_id && it->second == LR_STATUS_BOTTOM) {
          bottom_set.push_back(iter->first);
        }
      }
    }
    std::sort(bottom_set.begin(), bottom_set.end());
  }*/


  /*bool loc_is_linearly_related_to_input_stack_pointer(long loc_id, eq::consts_struct_t const &cs) const
  {
    NOT_IMPLEMENTED();
    //int stack_pointer_addr_ref_id = cs.get_addr_ref_id_for_keyword(g_stack_symbol);
    //map<int, lr_status_t> const &smap = m_loc_map.at(stack_pointer_addr_ref_id);
    //return (smap.at(loc_id) == LR_STATUS_LINEARLY_RELATED);
  }*/

#if 0
  bool loc_is_independent_of_input_stack_pointer(long loc_id, eq::consts_struct_t const &cs) const
  {
    NOT_IMPLEMENTED();
    /*bool stack_addr_ref_id_exists = false;
    for (map<int, map<int, lr_status_t> >::const_iterator iter = m_loc_map.begin();
         iter != m_loc_map.end();
         iter++) {
      int cs_addr_ref_id = iter->first;
      pair<string, expr_ref> p = cs.get_addr_ref(cs_addr_ref_id);
      CPP_DBG_EXEC3(LINEAR_RELATIONS, cout << __func__ << " " << __LINE__ << ": addr-ref " << p.first << " loc_id " << loc_id << " : " << lr_status_to_string(iter->second.at(loc_id)) << endl);
      if (   iter->second.at(loc_id) == LR_STATUS_LINEARLY_RELATED
          && p.first != g_stack_symbol) {
        ASSERT(   p.first.substr(0, g_local_keyword.size()) == g_local_keyword
               || p.first.substr(0, g_symbol_keyword.size()) == g_symbol_keyword);
        return true;
      } else if (   iter->second.at(loc_id) == LR_STATUS_MUTUALLY_INDEPENDENT
                 && p.first == g_stack_symbol) {
        return true;
      } else if (p.first == g_stack_symbol) {
        stack_addr_ref_id_exists = true;
      }
    }
    if (stack_addr_ref_id_exists) {
      return false; //stack_addr_ref_id_exists but could not find apt LR_STATUS
    } else {
      return true; //stack_addr_ref_id does not exist; so of course, mutually independent
    }*/
  }

  bool loc_is_top(long loc_id) const
  {
    NOT_IMPLEMENTED();
/*
    stopwatch_run(&loc_is_top_timer);
    //long stack_pointer_addr_ref_id;
    //stack_pointer_addr_ref_id = cs.get_addr_ref_id_for_keyword(g_stack_symbol);
    //map<int, lr_status_t> const &smap = m_map.at(stack_pointer_addr_ref_id);
    //bool ret = (smap.at(loc_id) == LR_STATUS_TOP);
    CPP_DBG_EXEC(ASSERTCHECKS, is_consistent_for_loc(loc_id));
    for (map<cs_addr_ref_id_t, map<graph_loc_id_t, lr_status_t>>::const_iterator it = m_loc_map.begin();
         it != m_loc_map.end();
         it++) {
      if (it->second.count(loc_id) && it->second.at(loc_id) != LR_STATUS_TOP) { // if loc is top wrt one addr_ref, it must be top wrt all addr_ref
        stopwatch_stop(&loc_is_top_timer);
        return false;
      }
    }
    stopwatch_stop(&loc_is_top_timer);
    return true;
*/
  }

  bool loc_is_bottom(long loc_id) const
  {
    NOT_IMPLEMENTED();
/*
    stopwatch_run(&loc_is_bottom_timer);
    for (map<cs_addr_ref_id_t, map<graph_loc_id_t, lr_status_t> >::const_iterator it = m_loc_map.begin();
         it != m_loc_map.end();
         it++) {
      if (it->second.at(loc_id) != LR_STATUS_BOTTOM) {
        stopwatch_stop(&loc_is_bottom_timer);
        return false;
      }
    }
    stopwatch_stop(&loc_is_bottom_timer);
    return true;
*/
  }

  bool is_consistent_for_loc(long loc_id) const
  {
    NOT_IMPLEMENTED();
/*
    stopwatch_run(&is_consistent_for_loc_timer);
    bool seen_top = false;
    bool seen_nontop = false;
    for (map<cs_addr_ref_id_t, map<graph_loc_id_t, lr_status_t> >::const_iterator it = m_loc_map.begin();
         it != m_loc_map.end();
         it++) {
      if (it->second.at(loc_id) == LR_STATUS_TOP) { // if loc is top wrt one addr_ref, it must be top wrt all addr_ref
        if (seen_nontop) {
          CPP_DBG_EXEC(ERR, cout << __func__ << " " << __LINE__ << ": it->first = " << it->first << endl);
          stopwatch_stop(&is_consistent_for_loc_timer);
          return false;
        }
        seen_top = true;
      } else {
        if (seen_top) {
          CPP_DBG_EXEC(ERR, cout << __func__ << " " << __LINE__ << ": it->first = " << it->first << endl);
          stopwatch_stop(&is_consistent_for_loc_timer);
          return false;
        }
        seen_nontop = true;
      }
    }
    stopwatch_stop(&is_consistent_for_loc_timer);
    return true;
*/
  }
#endif

  string to_string(graph_with_predicates_instance const *graph, T_PC const &p/*, set<T_PC> const &node_changed_map, eq::consts_struct_t const &cs*/, string prefix) const
  {
    graph_cp_location loc;
    stringstream ss;
    ss << "locs:" << endl;
    for (map<graph_loc_id_t, lr_status_t>::const_iterator it = m_loc_map.begin();
         it != m_loc_map.end();
         it++) {
      loc = graph->get_loc(/*p, */it->first);
      lr_status_t lrs = it->second;
      ss << prefix << " loc" << it->first << ": " << loc.to_string() << ": " << lrs.lr_status_to_string(graph->get_consts_struct()) << endl;
    }
    ss << "addrs:" << endl;
    for (map<addr_id_t, lr_status_t>::const_iterator it = m_addr_map.begin();
         it != m_addr_map.end();
         it++) {
      expr_ref addr = graph->get_addr(p, it->first);
      lr_status_t lrs = it->second;
      ss << prefix << " addr " << it->first << ". " << expr_string(addr) << " : " << lrs.lr_status_to_string(graph->get_consts_struct()) << endl;
    }
    return ss.str();
  }

};
#endif

class sort_cmp_on_loc_size {
public:
  sort_cmp_on_loc_size(map<graph_loc_id_t, graph_cp_location> const &locs) : m_locs(locs) {}
  bool operator() (graph_loc_id_t i, graph_loc_id_t j)
  {
    graph_cp_location loc_i, loc_j;
    //loc_i = m_graph->get_loc(m_pc, i);
    loc_i = m_locs.at(i);
    //loc_j = m_graph->get_loc(m_pc, j);
    loc_j = m_locs.at(j);

    if (loc_i.m_type < loc_j.m_type) {
      return true;
    } else if (loc_i.m_type > loc_j.m_type) {
      return false;
    }
    ASSERT(loc_i.m_type == loc_j.m_type);
    if (   loc_i.m_type == GRAPH_CP_LOCATION_TYPE_REGMEM
        || loc_i.m_type == GRAPH_CP_LOCATION_TYPE_LLVMVAR/*
        || loc_i.m_type == GRAPH_CP_LOCATION_TYPE_IO*/) {
      return i < j;
    }
    if (loc_i.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
      return loc_i.m_nbytes < loc_j.m_nbytes;
    }
    if (loc_i.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
      if (memlabel_t::memlabel_less(&loc_i.m_memlabel->get_ml(), &loc_j.m_memlabel->get_ml())) {
        return true;
      }
      if (memlabel_t::memlabel_less(&loc_j.m_memlabel->get_ml(), &loc_i.m_memlabel->get_ml())) {
        return false;
      }
      return loc_i.m_nbytes < loc_j.m_nbytes;
    }
    NOT_REACHED();
  }
private:
  //graph_with_predicates<T_PC, T_N, T_E, T_PRED> const *m_graph;
  //T_PC const &m_pc;
  map<graph_loc_id_t, graph_cp_location> const &m_locs;
};


//long get_linearly_related_addr_ref_id_for_addr(map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t>> const &addr2status_map, addr_id_t addr_id, eq::consts_struct_t const &cs);
//bool addr_is_independent_of_input_stack_pointer(map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t>> const &addr2status_map, addr_id_t addr_id, eq::consts_struct_t const &cs);
//bool addr_is_top(map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t>> const &addr2status_map, addr_id_t addr_id);
//bool addr_is_bottom(map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t>> const &addr2status_map, addr_id_t addr_id);
//bool loc_is_bottom(map<cs_addr_ref_id_t, map<graph_loc_id_t, lr_status_t>> const &loc2status_map, long loc_id);
//memlabel_t address_to_memlabel(expr_ref addr, eqspace::consts_struct_t const &cs, map<expr_id_t, addr_id_t> const &expr2addr_map, map<addr_id_t, lr_status_t> const &addr2status_map, map<symbol_id_t, tuple<string, size_t, bool>> const &symbol_map, vector<pair<string, size_t>> const &locals_map);
memlabel_t address_to_memlabel(expr_ref addr, eqspace::consts_struct_t const &cs, map<expr_id_t, addr_id_t> const &expr2addr_map, map<addr_id_t, lr_status_ref> const &addr2status_map, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map);
//memlabel_t get_memlabel(eq::consts_struct_t const &cs, int lr_addr_ref_id/*, bool independent_of_input_stack_pointer*/, bool is_top/*, bool is_bottom*/, vector<cs_addr_ref_id_t>& bottom_set, map<symbol_id_t, pair<string, size_t>> const &symbol_map, graph_locals_map_t const &locals_map);


//template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
//lr_status_t compute_lr_status_for_expr(expr_ref e/*, eq::state const &from_state*//*, cs_addr_ref_id_t cs_addr_ref_id*//*, T_PC const &p, map<T_PC, map<graph_loc_id_t, T_PC>> const &domdefs, map<T_PC, lr_map_t<T_PC, T_N, T_E, T_PRED>> const &lr_map*/, map<graph_loc_id_t, lr_status_t> const &prev_lr/*, map<graph_loc_id_t, graph_cp_location> const &start_pc_locs*/, set<cs_addr_ref_id_t> const &relevant_addr_refs, map<string, expr_ref> const &argument_regs, map<symbol_id_t, tuple<string, size_t, bool>> const &symbol_map, eqspace::consts_struct_t const &cs);
//lr_status_t compute_lr_status_for_expr(expr_ref e/*, eq::state const &from_state*//*, cs_addr_ref_id_t cs_addr_ref_id*//*, T_PC const &p, map<T_PC, map<graph_loc_id_t, T_PC>> const &domdefs, map<T_PC, lr_map_t<T_PC, T_N, T_E, T_PRED>> const &lr_map*/, map<graph_loc_id_t, lr_status_t> const &prev_lr/*, map<graph_loc_id_t, graph_cp_location> const &start_pc_locs*/, set<cs_addr_ref_id_t> const &relevant_addr_refs, map<string, expr_ref> const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, eqspace::consts_struct_t const &cs/*, std::function<callee_summary_t (nextpc_id_t, int)> get_callee_summary_fn*/);

lr_status_ref compute_lr_status_for_expr(expr_ref e/*, eq::state const &from_state*//*, cs_addr_ref_id_t cs_addr_ref_id*//*, T_PC const &p, map<T_PC, map<graph_loc_id_t, T_PC>> const &domdefs, map<T_PC, lr_map_t<T_PC, T_N, T_E, T_PRED>> const &lr_map*/, map<graph_loc_id_t, graph_cp_location> const &locs, map<graph_loc_id_t, lr_status_ref> const &prev_lr/*, map<graph_loc_id_t, graph_cp_location> const &start_pc_locs*/, sprel_map_pair_t const &sprel_map_pair, graph_memlabel_map_t const &memlabel_map, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_arg_regs_t const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, eqspace::consts_struct_t const &cs, std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn);

#if 0
template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class expr_label_visitor : public expr_visitor
{
public:
  expr_label_visitor(
      map<expr_ref, memlabel_t> &alias_map,
      graph_memlabel_map_t &memlabel_map,
      expr_ref const &e,
      map<addr_id_t, lr_status_t> const &addr2status_map,
      map<graph_loc_id_t, lr_status_t> const &loc2status_map,
      graph_with_predicates<T_PC, T_N, T_E, T_PRED> const *graph,
      state const &from_state,
      map<expr_id_t, addr_id_t> const &expr2addr_map,
      graph_symbol_map_t const &symbol_map,
      graph_locals_map_t const &locals_map,
      set<cs_addr_ref_id_t> const &relevant_addr_refs,
      context *ctx,
      bool update_callee_memlabels,
      //map<graph_loc_id_t, expr_ref> const &locid2expr_map,
      sprel_map_t const *sprel_map) :
    //m_alias_map(alias_map),
    m_memlabel_map(memlabel_map),
    m_expr(e),
    m_addr2status_map(addr2status_map),
    m_loc2status_map(loc2status_map),
    m_graph(graph),
    m_from_state(from_state),
    m_expr2addr_map(expr2addr_map),
    m_symbol_map(symbol_map),
    m_locals_map(locals_map),
    m_relevant_addr_refs(relevant_addr_refs),
    m_ctx(ctx),
    m_cs(ctx->get_consts_struct()),
    m_update_callee_memlabels(update_callee_memlabels),
    //m_locid2expr_map(locid2expr_map),
    m_sprel_map(sprel_map)
  {
    //m_visit_each_expr_only_once = true;
    m_context = e->get_context();
    visit_recursive(m_expr);
  }
  /*expr_ref get_final_expr(void) {
    return m_map_from_to.at(m_expr->get_id());
  }*/
private:
  virtual void visit(expr_ref const &e)
  {
    //ASSERT(m_map_from_to.count(e->get_id()) == 0);
  
    if (e->is_var() || e->is_const()) {
      //m_map_from_to[e->get_id()] = e;
      return;
    }
  
    /*expr_vector args_subs;
    for (unsigned i = 0; i < e->get_args().size(); ++i) {
      expr_ref arg = e->get_args()[i];
      ASSERT(m_map_from_to.count(arg->get_id()) > 0);
      args_subs.push_back(m_map_from_to.at(arg->get_id()));
    }*/

    if (e->get_operation_kind() == expr::OP_STORE) {
      expr_ref addr = e->get_args()[2];
      memlabel_t memlabel = get_memlabel_for_addr(addr);
      ASSERT(e->get_args().at(1)->is_memlabel_sort());
      ASSERT(e->get_args().at(1)->is_var());
      m_memlabel_map[expr_string(e->get_args().at(1))] = memlabel;
      //m_alias_map[addr] = memlabel;
      //args_subs[1] = ctx->mk_memlabel_const(memlabel);
      //m_map_from_to[e->get_id()] = m_context->mk_app(e->get_operation_kind(), args_subs);
      return;
    }
  
    if (e->get_operation_kind() == expr::OP_SELECT) {
      expr_ref addr = e->get_args()[2];
      memlabel_t memlabel = get_memlabel_for_addr(addr);
      ASSERT(e->get_args().at(1)->is_memlabel_sort());
      if (!e->get_args().at(1)->is_var()) {
        cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
      }
      ASSERT(e->get_args().at(1)->is_var());
      m_memlabel_map[expr_string(e->get_args().at(1))] = memlabel;
      //m_alias_map[addr] = memlabel;
      //args_subs[1] = ctx->mk_memlabel_const(memlabel);
      //m_map_from_to[e->get_id()] = m_context->mk_app(e->get_operation_kind(), args_subs);
      return;
    }
  
    if (   m_update_callee_memlabels
        && e->get_operation_kind() == expr::OP_FUNCTION_CALL) {
      vector<memlabel_t> farg_memlabels;
      size_t num_fargs = e->get_args().size() - OP_FUNCTION_CALL_ARGNUM_FARG0;
      expr_ref orig_memvar = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_ORIG_MEMVAR);
      for (size_t i = OP_FUNCTION_CALL_ARGNUM_FARG0; i < e->get_args().size(); i++) {
        memlabel_t addr_ml = get_memlabel_for_addr(e->get_args().at(i));
        //cout << __func__ << " " << __LINE__ << ": addr_ml = " << addr_ml.to_string() << endl;
        //cout << __func__ << " " << __LINE__ << ": orig_memvar = " << expr_string(orig_memvar) << endl;
        memlabel_t addr_reachable_ml;
        if (!addr_ml.memlabel_is_top()) {
          addr_reachable_ml = get_reachable_memlabels(addr_ml, orig_memvar);
        } else {
          addr_reachable_ml = addr_ml;
        }
        farg_memlabels.push_back(addr_reachable_ml);
        //cout << __func__ << " " << __LINE__ << ": farg = " << expr_string(e->get_args().at(i)) << endl;
        //cout << __func__ << " " << __LINE__ << ": addr_reachable_ml = " << addr_reachable_ml.to_string() << endl;
      }
      expr_ref nextpc = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_NEXTPC_CONST);
      callee_summary_t csum;
      if (nextpc->is_nextpc_const()) {
        unsigned nextpc_num = nextpc->get_nextpc_num();
        csum = m_graph->get_callee_summary_for_nextpc(nextpc_num, num_fargs);
        //cout << __func__ << " " << __LINE__ << ": nextpc_num = " << nextpc_num << "\n";
        //cout << __func__ << " " << __LINE__ << ": csum = " << csum.callee_summary_to_string_for_eq() << "\n";
      } else {
        csum = callee_summary_t::callee_summary_bottom(m_cs, m_relevant_addr_refs, m_symbol_map, m_locals_map, num_fargs);
      }
      pair<memlabel_t, memlabel_t> pp = csum.get_readable_writeable_memlabels(farg_memlabels);
      memlabel_t ml_readable = get_reachable_memlabels(pp.first, orig_memvar);
      memlabel_t ml_writeable= get_reachable_memlabels(pp.second, orig_memvar);
      expr_ref ml_readable_var = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE);
      expr_ref ml_writeable_var = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE);
      ASSERT(ml_readable_var->is_var());
      ASSERT(ml_readable_var->is_memlabel_sort());
      ASSERT(ml_writeable_var->is_var());
      ASSERT(ml_writeable_var->is_memlabel_sort());
      mlvarname_t ml_readable_varname = expr_string(ml_readable_var);
      mlvarname_t ml_writeable_varname = expr_string(ml_writeable_var);
      m_memlabel_map[ml_readable_varname] = ml_readable;
      m_memlabel_map[ml_writeable_varname] = ml_writeable;
      //args_subs[OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE] = m_context->mk_memlabel_const(ml_readable);
      //args_subs[OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE] = m_context->mk_memlabel_const(ml_writeable);
      //m_map_from_to[e->get_id()] = m_context->mk_app(e->get_operation_kind(), args_subs);
      return;
    }
    /*if (   e->get_operation_kind() == expr::OP_ITE
        && args_subs[1] == args_subs[2]) {
      m_map_from_to[e->get_id()] = args_subs[1];
    } else {
      m_map_from_to[e->get_id()] = m_context->mk_app(e->get_operation_kind(), args_subs);
    }*/
  }

  //memlabel_t get_memlabel_for_addr(expr_ref addr) const
  //{
  //  memlabel_t ret;
  //  //expr_ref addr_simplified = m_context->expr_do_simplify(addr);
  //  expr_ref addr_simplified = m_context->expr_simplify_using_sprel_and_memlabel_maps(addr, m_sprel_map_pair, m_memlabel_map);
  //  if (m_context->expr_contains_only_consts_struct_constants_or_arguments(addr_simplified, m_graph->get_argument_regs())) {
  //    ret = get_memlabel_for_addr_containing_only_consts_struct_constants(addr_simplified, m_symbol_map, m_locals_map, m_relevant_addr_refs, m_graph->get_argument_regs(), m_cs/*, m_locid2expr_map, m_sprel_map*/);
  //  } else {
  //    ret = address_to_memlabel(addr, m_cs, m_expr2addr_map, m_addr2status_map, m_symbol_map, m_locals_map);
  //  }
  //  return ret;
  //}
  //set<memlabel_t> get_reachable_memlabels(memlabel_t const &ml, expr_ref orig_memvar) const
  memlabel_t get_reachable_memlabels(memlabel_t const &ml, expr_ref orig_memvar) const
  {
    set<memlabel_ref> reachable_mls;
    set<memlabel_ref> new_reachable_mls = ml.get_atomic_memlabels();
    do {
      reachable_mls = new_reachable_mls;
      for (const auto &aml : reachable_mls) {
        ASSERT(memlabel_is_atomic(&aml));
        expr_ref masked_mem = m_ctx->mk_memmask(orig_memvar, aml);
        graph_loc_id_t loc_id = m_graph->get_loc_id_for_masked_mem_expr(masked_mem);
        memlabel_t rml = m_loc2status_map.at(loc_id).to_memlabel(m_cs, m_symbol_map, m_locals_map);
        if (!rml.memlabel_is_top()) {
          set<memlabel_t> s = rml.get_atomic_memlabels();
          for (auto const& ml : s) {
            new_reachable_mls.insert(mk_memlabel_ref(ml));
          }
        }
      }
    } while (reachable_mls.size() != new_reachable_mls.size());
    return memlabel_union(new_reachable_mls);
  }

  //map<expr_ref, memlabel_t> &m_alias_map;
  graph_memlabel_map_t &m_memlabel_map;
  expr_ref const &m_expr;
  map<addr_id_t, lr_status_ref> const &m_addr2status_map;
  map<graph_loc_id_t, lr_status_ref> const &m_loc2status_map;
  graph_with_predicates<T_PC, T_N, T_E, T_PRED> const *m_graph;
  state const &m_from_state;
  map<expr_id_t, addr_id_t> const &m_expr2addr_map;
  graph_symbol_map_t const &m_symbol_map;
  graph_locals_map_t const &m_locals_map;
  set<cs_addr_ref_id_t> const &m_relevant_addr_refs;
  context *m_ctx;
  eqspace::consts_struct_t const &m_cs;
  bool m_update_callee_memlabels;
  //map<graph_loc_id_t, expr_ref> const &m_locid2expr_map;
  sprel_map_pair_t const &m_sprel_map_pair;
  //map<unsigned, expr_ref> m_map_from_to;
  struct eqspace::context *m_context;
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
expr_split_mem(
    map<expr_ref, memlabel_t> &alias_map,
    graph_memlabel_map_t &memlabel_map,
    expr_ref e,
    //lr_map_t const &lr_map,
    map<addr_id_t, lr_status_ref> const &addr2status_map,
    map<graph_loc_id_t, lr_status_ref> const &loc2status_map,
    graph_with_predicates<T_PC, T_N, T_E, T_PRED> const *graph,
    //pc const &from_pc,
    state const &from_state,
    map<expr_id_t, addr_id_t> const &expr2addr_map,
    graph_symbol_map_t const &symbol_map,
    graph_locals_map_t const &locals_map,
    set<cs_addr_ref_id_t> const &relevant_addr_refs,
    context *ctx, bool update_callee_memlabels,
    //map<graph_loc_id_t, expr_ref> const &locid2expr_map,
    sprel_map_pair_t const &sprel_map_pair)
{
  ASSERT(alias_map.size() == 0);
  CPP_DBG_EXEC(SPLIT_MEM, cout << __func__ << " " << __LINE__ << ": e:\n" << ctx->expr_to_string_table(e) << endl);
  expr_label_visitor<T_PC, T_N, T_E, T_PRED> visitor(alias_map, memlabel_map, e, addr2status_map, loc2status_map, graph, /*from_pc, */from_state, expr2addr_map, symbol_map, locals_map, relevant_addr_refs, ctx, update_callee_memlabels/*, locid2expr_map*/, sprel_map_pair);
  //expr_ref new_e = visitor.get_final_expr();
  //CPP_DBG_EXEC(SPLIT_MEM, cout << __func__ << " " << __LINE__ << ": new_e:\n" << ctx->expr_to_string_table(new_e) << endl);
  //return new_e;
  ASSERT(alias_map.size() == 0);
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
state_split_mem(
    map<expr_ref, memlabel_t> &alias_map,
    graph_memlabel_map_t &memlabel_map,
    state const &st,
    map<addr_id_t, lr_status_ref> const &addr2status_map,
    map<graph_loc_id_t, lr_status_ref> const &loc2status_map,
    graph_with_predicates<T_PC, T_N, T_E, T_PRED> const *graph,
    state const &from_state,
    map<expr_id_t, addr_id_t> const &expr2addr_map,
    graph_symbol_map_t const &symbol_map,
    graph_locals_map_t const &locals_map,
    set<cs_addr_ref_id_t> const &relevant_addr_refs,
    context *ctx, bool update_callee_memlabels,
    //map<graph_loc_id_t, expr_ref> const &locid2expr_map,
    sprel_map_pair_t const &sprel_map_pair)
{
  autostop_timer func_timer(__func__);
  //bool changed = false;

  std::function<void (const string&, expr_ref)> f =
    [&alias_map, &memlabel_map, &from_state, &expr2addr_map, &addr2status_map, &loc2status_map, graph, &symbol_map, &locals_map, &relevant_addr_refs, ctx, update_callee_memlabels/*, &locid2expr_map*/, &sprel_map_pair/*, &changed*/]
    (const string& key, expr_ref e) -> void
  {
    CPP_DBG_EXEC(SPLIT_MEM, cout << __func__ << " " << __LINE__ << ": calling expr_split_mem on key " << key << endl);
    expr_split_mem<T_PC, T_N, T_E, T_PRED>(alias_map, memlabel_map, e, addr2status_map, loc2status_map, graph, from_state, expr2addr_map, symbol_map, locals_map, relevant_addr_refs, ctx, update_callee_memlabels/*, locid2expr_map*/, sprel_map_pair);
    /*if (e != new_e) {
      changed = true;
    }*/
    //return new_e;
  };
  st.state_visit_exprs(f);
  //return changed;
}
#endif

//memlabel_t get_memlabel_for_addr_ref_ids(set<cs_addr_ref_id_t> const &addr_ref_ids, consts_struct_t const &cs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map);

//void
//populate_memlabel_map_for_expr_and_loc_status(expr_ref const &new_e, map<graph_loc_id_t, graph_cp_location> const &locs, map<graph_loc_id_t, lr_status_ref> const &in, sprel_map_pair_t const &sprel_map_pair, set<cs_addr_ref_id_t> const &relevant_addr_refs, map<string_ref, expr_ref> const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, graph_memlabel_map_t &memlabel_map, std::function<callee_summary_t (nextpc_id_t, int)> get_callee_summary_fn, std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn);
}
