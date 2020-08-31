#include "graph/lr_map.h"
#include "gsupport/pc.h"
//#include "tfg/tfg.h"
//#include "cg/corr_graph.h"
#include "gsupport/corr_graph_edge.h"
#include "gsupport/tfg_edge.h"
#include "expr/consts_struct.h"
#include "graph/expr_loc_visitor.h"
#include "graph/graph_with_aliasing.h"

#include "support/timers.h"
/*void
lr_map_t::add_lr_mappings(int cs_addr_ref_id, map<int, lr_status_t> const &loc_ids)
{
  m_map[cs_addr_ref_id] = loc_ids;
}*/

namespace eqspace {

/*alias_type
get_extra_type_using_regtype_NS( reg_type rt)
{
  ASSERT(rt != reg_type_memory);
  switch (rt) {
    case reg_type_stack:
      return alias_type_null;
    case reg_type_local:
      return alias_type_null;
    case reg_type_symbol:
      return alias_type_symbol;
    default:
      NOT_REACHED();
  }
}*/




/*memlabel_t
get_memlabel(eq::consts_struct_t const &cs, int lr_addr_ref_id, bool is_top, vector<cs_addr_ref_id_t>& bottom_set, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map)
{
  memlabel_t ret;
  //int i = 0, j = 0;
  string status;
  if (lr_addr_ref_id == -1) {
    if (is_top) {
      keyword_to_memlabel(&ret, G_MEMLABEL_TOP_SYMBOL, MEMSIZE_MAX);
      return ret;
    } else {
      size_t i = 0;
      keyword_to_memlabel(&ret, G_MEM_SYMBOL, MEMSIZE_MAX);
      //ASSERT(bottom_set.size() <= MAX_ALIAS_SET_SIZE);
      for (i = 0; i < bottom_set.size(); ++i) {
        pair<reg_type, long> p = cs.get_addr_ref_reg_type_and_id(bottom_set[i]);
        variable_id_t variable_id = get_extra_id_using_regtype_and_index(p.first, p.second);
        memlabel_t::alias_type altype = get_extra_type_using_regtype(p.first);
        variable_size_t variable_size = MEMSIZE_MAX;
        variable_constness_t variable_is_const = false;
        if (altype == memlabel_t::alias_type_symbol) {
          if (symbol_map.count(variable_id) == 0) {
            variable_size = 0;
          } else {
            variable_size = get<1>(symbol_map.at(variable_id));
            variable_is_const = get<2>(symbol_map.at(variable_id));
          }
        } else if (altype == memlabel_t::alias_type_local) {
          variable_size = locals_map.at(variable_id).second;
        }
        ret.m_alias_set.insert(make_tuple(altype, variable_id, variable_size, variable_is_const));
      }
      
      //ret.m_num_alias = bottom_set.size() + 1;
      //ret.m_extra[i] = -1;
      //ret.m_extra_type[i] = memlabel_t::alias_type_heap;
      ret.m_alias_set.insert(make_tuple(memlabel_t::alias_type_heap, -1, (variable_size_t)-1, false));
      return ret;
    }
  } else {
    pair<reg_type, long> p = cs.get_addr_ref_reg_type_and_id(lr_addr_ref_id);
    size_t memsize;
    variable_constness_t variable_is_const = false;
    if (p.first == reg_type_symbol) {
      if (symbol_map.count(p.second) == 0) {
        memsize = -1;
      } else {
        memsize = get<1>(symbol_map.at(p.second));
        variable_is_const = get<2>(symbol_map.at(p.second));
      }
    } else if (p.first == reg_type_local) {
      //memsize = tfg->get_local_size_from_id(p.second);
      if ((int)locals_map.size() <= p.second) {
        memsize = -1;
      } else {
        memsize = locals_map.at(p.second).second;
      }
    } else {
      memsize = MEMSIZE_MAX;
    }
    keyword_to_memlabel(&ret, G_MEM_SYMBOL, MEMSIZE_MAX);
    variable_id_t variable_id = get_extra_id_using_regtype_and_index(p.first, p.second);
    memlabel_t::alias_type altype = get_extra_type_using_regtype(p.first);
    variable_size_t variable_size = memsize;
    ret.m_alias_set.insert(make_tuple(altype, variable_id, variable_size, variable_is_const));
    return ret;
    //memlabel_populate_using_regtype_and_index(&ret, p.first, p.second, memsize);
  }
  return ret;
}*/

#if 0
long get_linearly_related_addr_ref_id_for_addr(map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t>> const &addr2status_map, addr_id_t addr_id, eq::consts_struct_t const &cs)
{
  stopwatch_run(&get_linearly_related_addr_ref_id_for_addr_timer);
  long ret = -1;
  long stack_addr_ref_id = cs.get_addr_ref_id_for_keyword(g_stack_symbol);
  for (map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t> >::const_iterator iter = addr2status_map.begin();
       iter != addr2status_map.end();
       iter++) {
    //cout << __func__ << " " << __LINE__ << ": addr_id = " << addr_id << ", iter->first = " << iter->first << ": " << cs.get_addr_ref(iter->first).first << endl;
    map<addr_id_t, lr_status_t> const &smap = iter->second;
    if (smap.at(addr_id) == LR_STATUS_LINEARLY_RELATED) {
      if (ret == -1 || ret == stack_addr_ref_id) {
        ret = iter->first;
        if (ret != stack_addr_ref_id) {
          stopwatch_stop(&get_linearly_related_addr_ref_id_for_addr_timer);
          return ret;
        }
      }
    }
    /*for (map<addr_id_t, lr_status_t>::const_iterator it = smap.begin();
         it != smap.end();
         it++) {
      if (it->first == addr_id && it->second == LR_STATUS_LINEARLY_RELATED) {
        if (ret == -1 || ret == cs.get_addr_ref_id_for_keyword(g_stack_symbol)) {
          ret = iter->first;
        }
      }
    }*/
  }
  stopwatch_stop(&get_linearly_related_addr_ref_id_for_addr_timer);
  return ret;
}

bool addr_is_independent_of_input_stack_pointer(map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t>> const &addr2status_map, addr_id_t addr_id, eq::consts_struct_t const &cs)
{
  bool stack_addr_ref_id_exists = false;
  stopwatch_run(&addr_is_independent_of_input_stack_pointer_timer);
  //long stack_pointer_addr_ref_id;
  for (map<int, map<addr_id_t, lr_status_t> >::const_iterator iter = addr2status_map.begin();
       iter != addr2status_map.end();
       iter++) {
    int cs_addr_ref_id = iter->first;
    pair<string, expr_ref> p = cs.get_addr_ref(cs_addr_ref_id);
    CPP_DBG_EXEC(linear_relations, cout << __func__ << " " << __LINE__ << ": addr-ref " << p.first << " addr_id " << addr_id << " : " << lr_status_to_string(iter->second.at(addr_id)) << endl);
    if (   iter->second.at(addr_id) == LR_STATUS_LINEARLY_RELATED
        && p.first != g_stack_symbol) {
      ASSERT(   p.first.substr(0, g_local_keyword.size()) == g_local_keyword
             || p.first.substr(0, g_symbol_keyword.size()) == g_symbol_keyword);
      stopwatch_stop(&addr_is_independent_of_input_stack_pointer_timer);
      return true;
    } else if (   iter->second.at(addr_id) == LR_STATUS_MUTUALLY_INDEPENDENT
               && p.first == g_stack_symbol) {
      stopwatch_stop(&addr_is_independent_of_input_stack_pointer_timer);
      return true;
    } else if (p.first == g_stack_symbol) {
      stack_addr_ref_id_exists = true;
    }
  }
  stopwatch_stop(&addr_is_independent_of_input_stack_pointer_timer);
  if (stack_addr_ref_id_exists) {
    return false;
  } else {
    return true;
  }
}

bool
addr_is_top(map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t>> const &addr2status_map, addr_id_t addr_id)
{
  stopwatch_run(&addr_is_top_timer);
  for (map<cs_addr_ref_id_t, map<addr_id_t , lr_status_t>>::const_iterator it = addr2status_map.begin();
       it != addr2status_map.end();
       it++) {
    ASSERT(it->second.count(addr_id) > 0);
    if (it->second.at(addr_id) != LR_STATUS_TOP) { // if loc is top wrt one addr_ref, it must be top wrt all addr_ref
      stopwatch_stop(&addr_is_top_timer);
      return false;
    }
  }
  stopwatch_stop(&addr_is_top_timer);
  return true;
}

bool
addr_is_bottom(map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t>> const &addr2status_map, addr_id_t addr_id)
{
  stopwatch_run(&addr_is_bottom_timer);
  for (map<int, map<addr_id_t, lr_status_t> >::const_iterator it = addr2status_map.begin();
       it != addr2status_map.end();
       it++) {
    if (it->second.at(addr_id) != LR_STATUS_BOTTOM) {
      stopwatch_stop(&addr_is_bottom_timer);
      return false;
    }
  }
  stopwatch_stop(&addr_is_bottom_timer);
  return true;
}

static void
get_bottom_addr_ref_ids_for_addr(map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t>> const &addr2status_map, addr_id_t addr_id, eqspace::consts_struct_t const &cs, vector<cs_addr_ref_id_t>& bottom_set)
{
//   stopwatch_run(&get_linearly_related_addr_ref_id_for_addr_timer);
  for (map<cs_addr_ref_id_t, map<addr_id_t, lr_status_t> >::const_iterator iter = addr2status_map.begin();
       iter != addr2status_map.end();
       iter++) {
    //cout << __func__ << " " << __LINE__ << ": iter->first = " << iter->first << endl;
    map<addr_id_t, lr_status_t> const &smap = iter->second;
    if (smap.at(addr_id) == LR_STATUS_BOTTOM) {
      bottom_set.push_back(iter->first);
    } else if (smap.at(addr_id) == LR_STATUS_LINEARLY_RELATED) {
      bottom_set.clear();
      return;
    }
  }
//   stopwatch_stop(&get_linearly_related_addr_ref_id_for_addr_timer);
}
#endif


memlabel_t
//address_to_memlabel(expr_ref addr, eqspace::consts_struct_t const &cs, map<expr_id_t, addr_id_t> const &expr2addr_map, map<addr_id_t, lr_status_t> const &addr2status_map, map<symbol_id_t, tuple<string, size_t, bool>> const &symbol_map, vector<pair<string, size_t>> const &locals_map)
address_to_memlabel(expr_ref addr, eqspace::consts_struct_t const &cs, map<expr_id_t, addr_id_t> const &expr2addr_map, map<addr_id_t, lr_status_ref> const &addr2status_map, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map)
{
  //addr_id_t addr_id = graph->get_addr_id_from_addr(p, addr);
  //ASSERT(m_addrs.at(p).second.count(addr->get_id()));
  if (expr2addr_map.count(addr->get_id()) == 0) {
    cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << endl;
  }
  addr_id_t addr_id = expr2addr_map.at(addr->get_id());

  //cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << ", addr_id = " << addr_id << endl;

  //bool is_top = this->addr_is_top(addr_id);
  //bool is_top = addr_is_top(addr2status_map, addr_id);
  ASSERT(addr2status_map.count(addr_id));
  return addr2status_map.at(addr_id)->to_memlabel(cs, symbol_map, locals_map);
  //lr_status_t const &addr_status = addr2status_map.at(addr_id);
  //bool is_bottom = this->addr_is_bottom(addr_id);
  //bool is_bottom = addr_is_bottom(addr2status_map, addr_id);

  //int lr_addr_ref_id = this->get_linearly_related_addr_ref_id_for_addr(addr_id, cs);
  /*cs_addr_ref_id_t lr_addr_ref_id = get_linearly_related_addr_ref_id_for_addr(addr2status_map, addr_id, cs);
  //bool independent_of_input_stack_pointer = addr_is_independent_of_input_stack_pointer(addr2status_map, addr_id, cs);

  //return get_memlabel(cs, lr_addr_ref_id, independent_of_input_stack_pointer, is_top, is_bottom, symbol_map, locals_map);

  vector<cs_addr_ref_id_t> bottom_set;
  if (lr_addr_ref_id == -1) {
    get_bottom_addr_ref_ids_for_addr(addr2status_map, addr_id, cs, bottom_set);
    std::sort(bottom_set.begin(), bottom_set.end());
  }
  return get_memlabel(cs, lr_addr_ref_id, is_top, bottom_set, symbol_map, locals_map);*/
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class expr_linear_relation_holds_visitor : public expr_loc_visitor<T_PC, T_N, T_E, T_PRED, lr_status_ref>
{
public:
  expr_linear_relation_holds_visitor(
      expr_ref const &e,
      map<graph_loc_id_t, graph_cp_location> const &locs,
      map<graph_loc_id_t, lr_status_ref> const &pred_locs,
      set<cs_addr_ref_id_t> const &relevant_addr_ref_ids,
      graph_arg_regs_t const &argument_regs,
      graph_symbol_map_t const &symbol_map,
      graph_locals_map_t const &locals_map,
      //bool update_callee_memlabels,
      //graph_memlabel_map_t &memlabel_map,
      context *ctx,
      eqspace::consts_struct_t const &cs,
      graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const& graph//,
      //std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn
      //std::function<callee_summary_t (nextpc_id_t, int)> get_callee_summary_fn
    )
  : expr_loc_visitor<T_PC, T_N, T_E, T_PRED, lr_status_ref>(ctx, graph/*expr2locid_fn*/)/*, m_expr(e), m_locs(locs)*/, m_pred_locs(pred_locs), m_relevant_addr_ref_ids(relevant_addr_ref_ids), m_argument_regs(argument_regs), m_symbol_map(symbol_map)/*, m_locals_map(locals_map), m_update_callee_memlabels(update_callee_memlabels), m_memlabel_map(memlabel_map)*/, m_ctx(ctx), m_cs(cs)/*, m_graph(graph), m_expr2locid_fn(expr2locid_fn), m_get_callee_summary_fn(get_callee_summary_fn)*/
  {
    //m_holds.clear();
    //m_visit_each_expr_only_once = true;
    //visit_recursive(e, {});
  }

  static lr_status_ref get_lr_status_for_loc_ids(
        set<graph_loc_id_t> const &loc_ids,
        //map<graph_loc_id_t, graph_cp_location> const &locs,
        //cs_addr_ref_id_t cs_addr_ref_id,
        //T_PC const &p,
        //map<T_PC, map<graph_loc_id_t, T_PC>> const &domdefs,
        //map<T_PC, lr_map_t<T_PC, T_N, T_E, T_PRED>> const &lr_map
        map<graph_loc_id_t, lr_status_ref> const &pred_locs,
        set<cs_addr_ref_id_t> const& relevant_addr_refs)
  {
    /*T_PC dominating_pc = p; //domdefs.at(p).at(loc_id);
    lr_status_t ret1, ret2;
    ASSERT(lr_map.count(dominating_pc));
    ASSERT(lr_map.at(dominating_pc).get_linearly_related_locs_for_addr_ref_id(cs_addr_ref_id).count(loc_id));
    ret1 = lr_map.at(dominating_pc).get_linearly_related_locs_for_addr_ref_id(cs_addr_ref_id).at(loc_id);*/
    /*ret2 = lr_map.at(p).get_linearly_related_locs_for_addr_ref_id(cs_addr_ref_id).at(loc_id);
    if (ret1 != ret2) {
      cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", loc_id = " << loc_id << ", dominating_pc = " << dominating_pc.to_string() << endl;
    }
    ASSERT(ret1 == ret2);*/
    lr_status_ref ret = lr_status_t::linearly_related(set<cs_addr_ref_id_t>());
    bool ret_init = false;
    for (const auto &loc_id : loc_ids) {
      lr_status_ref lret;
      if (pred_locs.count(loc_id) == 0) {
        //cout << __func__ << " " << __LINE__ << ": loc_id = " << loc_id << endl;
        // for some reason locid was determined to be dead so just conservatively assume bottom
        NOT_REACHED(); // XXX use start_pc_value() here
        //set<memlabel_t> bset;
        //bset.insert(memlabel_heap());
        //lret = lr_status_t::bottom(relevant_addr_refs, bset);
      }
      else {
        lret = pred_locs.at(loc_id);
      }
      if (!ret_init) {
        ret = lret;
        ret_init = true;
      } else {
        ret = lr_status_t::lr_status_union(ret, lret);
      }
    }
    return ret;
  }

  lr_status_ref visit(expr_ref const &e, bool is_loc, set<graph_loc_id_t> const &locs, vector<lr_status_ref> const &argvals) override;

  //lr_status_t get_status()
  //{
  //  ASSERT(m_holds.count(m_expr->get_id()) > 0);
  //  return m_holds[m_expr->get_id()];
  //}
  /*long expr_to_loc_id(expr_ref e) {
    return expr_to_loc_id_using_map(e, m_expr2locid_map);
  }*/

private:
  static bool
  expr_can_affect_aliasing(expr::operation_kind const& opk, size_t pos)
  {
    switch(opk) {
      case expr::OP_ITE:
        // test condition cannot affect aliasing
        return pos != 0;
      case expr::OP_SELECT:
        return pos == 0; //only the memory operand is relevant, because we can only access that stuff that is pointed-to by the memory from which we are selecting
      //case expr::OP_FUNCTION_CALL:
      //  return pos != OP_FUNCTION_CALL_ARGNUM_ORIG_MEMVAR;
      default:
        break;
    }
    return true;
  }

  /*static graph_loc_id_t expr_memmask_is_pred_dependency(map<graph_loc_id_t, graph_cp_location> const &start_pc_locs, memlabel_t const &ml, expr_ref addr, size_t memsize)
  {
    for (const auto &locm : start_pc_locs) {
      //eq::graph_cp_location const &loc = this->m_locs.at(pc::start()).at(i);
      eq::graph_cp_location const &loc = locm.second;
      if (   loc.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED
          && memlabels_equal(&ml, &loc.m_memlabel)
          && is_expr_equal_syntactic(loc.m_addr, addr)
          && (int)loc.m_nbytes == (int)memsize) {
        //cout << "found" << endl;
        return locm.first;
      }
    }
    return -1;
  }*/

  //tfg const *m_tfg;
  //eq::state const &m_from_state;
  //expr_ref const &m_expr;
  //cs_addr_ref_id_t m_cs_addr_ref_id;
  //T_PC const &m_pc;
  //map<T_PC, map<graph_loc_id_t, T_PC>> const &m_domdefs;
  //map<T_PC, lr_map_t<T_PC, T_N, T_E, T_PRED>> const &m_lr_map;
  //map<graph_loc_id_t, graph_cp_location> const &m_locs;
  map<graph_loc_id_t, lr_status_ref> const &m_pred_locs;
  //map<graph_loc_id_t, graph_cp_location> const &m_start_pc_locs;
  set<cs_addr_ref_id_t> const &m_relevant_addr_ref_ids;
  graph_arg_regs_t const &m_argument_regs;
  graph_symbol_map_t const &m_symbol_map;
  //graph_locals_map_t const &m_locals_map;
  //bool m_update_callee_memlabels;
  //graph_memlabel_map_t &m_memlabel_map;
  context *m_ctx;
  eqspace::consts_struct_t const &m_cs;
  //graph_with_predicates<T_PC, T_N, T_E, T_PRED> const& m_graph;
  //std::function<graph_loc_id_t (expr_ref const &)> m_expr2locid_fn;
  //std::function<callee_summary_t (nextpc_id_t, int)> m_get_callee_summary_fn;
  //map<int, lr_status_t> m_holds;
  //map<int, pair<expr_ref, int> > const &m_expr2locid_map;
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
lr_status_ref
expr_linear_relation_holds_visitor<T_PC, T_N, T_E, T_PRED>::visit(expr_ref const &e, bool is_loc, set<graph_loc_id_t> const &locs, vector<lr_status_ref> const &arg_holds)
{
  stopwatch_run(&expr_linear_relation_holds_visit_timer);
  set<cs_addr_ref_id_t> empty_set;
  expr::operation_kind op;
  //graph_loc_id_t e_loc_id;

  //ASSERT(m_holds.count(e->get_id()) == 0);
  DYN_DEBUG3(linear_relations,
      cout << "expr_linear_relation_holds_visitor::" << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
      cout << "expr_linear_relation_holds_visitor::" << __func__ << " " << __LINE__ << ": locs = [size " << locs.size() << "]:";
      for (auto l : locs) {
        cout << " " << l;
      }
      cout << endl;
  );
  //string cs_addr_ref_str = m_cs.get_addr_ref(m_cs_addr_ref_id).first;

  if (e->is_const()) {
    //m_holds[e->get_id()] = lr_status_t::linearly_related(empty_set);
    lr_status_ref ret = lr_status_t::linearly_related(empty_set);
#if 0
    if (e->is_memlabel_sort()) {
      memlabel_t const &ml = e->get_memlabel_value();
      /*if (memlabel_is_top(&ml)) {
        m_holds[e->get_id()] = lr_status_t::top();
      } else */{
        m_holds[e->get_id()] = lr_status_t::linearly_related(empty_set);
      }
    } else {
      m_holds[e->get_id()] = lr_status_t::linearly_related(empty_set);
    }
#endif
    //CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning mutually-independent for " << cs_addr_ref_str << " vs. " << expr_string(e) << endl);
    stopwatch_stop(&expr_linear_relation_holds_visit_timer);
    return ret;
  }

  /*if (e->is_array_sort()) {
    m_holds[e->get_id()] = LR_STATUS_MUTUALLY_INDEPENDENT;
    stopwatch_stop(&expr_linear_relation_holds_visit_timer);
    return;
    }*/

  /*if (cs_addr_ref_str == g_stack_symbol) {
    memlabel_t ml;
    bool found = false;
    if (e->get_operation_kind() == expr::OP_SELECT) {
    ml = e->get_args()[1]->get_memlabel_value();
    found = true;
    } else if (e->get_operation_kind() == expr::OP_FUNCTION_ARGUMENT_PTR) {
    ml = e->get_args()[1]->get_memlabel_value();
    found = true;
    }
    if (found && memlabel_is_independent_of_input_stack_pointer(&ml)) {
    m_holds[e->get_id()] = LR_STATUS_MUTUALLY_INDEPENDENT;
    stopwatch_stop(&expr_linear_relation_holds_visit_timer);
    CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning mutually-independent for " << cs_addr_ref_str << " vs. " << expr_string(e) << endl);
    return;
    }
    }*/
  long const_id;
  if (e->is_var()) {
    autostop_timer func_var_timer(string("expr_linear_relation_holds::") +  __func__ + ".var");
    for (auto cs_addr_ref_id : m_relevant_addr_ref_ids) {
      expr_ref addr_ref = m_cs.get_addr_ref(cs_addr_ref_id).second;
      //cout << __func__ << " " << __LINE__ << ": addr_ref = " << expr_string(addr_ref) << endl;
      if (is_expr_equal_syntactic(e, addr_ref)) {
        lr_status_ref lrs = lr_status_t::linearly_related(cs_addr_ref_id);
        //ASSERT2(lr_status_t::lr_status_meet(m_holds.at(e->get_id()), lrs).equals(lrs));
        //m_holds[e->get_id()] = lrs;
        lr_status_ref ret = lrs;
        DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << lrs->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
        stopwatch_stop(&expr_linear_relation_holds_visit_timer);
        return ret;
      }
    }
    if (m_cs.expr_represents_llvm_undef(e)) {
      //m_holds[e->get_id()] = lr_status_t::linearly_related(empty_set);
      lr_status_ref ret = lr_status_t::linearly_related(empty_set);
      stopwatch_stop(&expr_linear_relation_holds_visit_timer);
      return ret;
    }
    string arg_id;
    if (   (arg_id = m_argument_regs.expr_get_argname(e)) != ""
        || m_cs.expr_is_retaddr_const(e)
        || m_cs.expr_is_callee_save_const(e)) {
      //set<memlabel_t> memlabel_arg_set;
      //memlabel_arg_set.insert(memlabel_arg(arg_id));
      lr_status_ref lrs = lr_status_t::start_pc_value(e, m_relevant_addr_ref_ids, m_ctx, m_argument_regs, m_symbol_map);
      /*if (   cs_addr_ref_str == g_stack_symbol
        || cs_addr_ref_str.substr(0, g_local_keyword.size()) == g_local_keyword) {
        CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning mutually-independent for " << m_cs.get_addr_ref(m_cs_addr_ref_id).first << " vs. " << expr_string(e) << endl);
        m_holds[e->get_id()] = LR_STATUS_MUTUALLY_INDEPENDENT;
        } else {
        CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning bottom for " << m_cs.get_addr_ref(m_cs_addr_ref_id).first << " vs. " << expr_string(e) << endl);
        m_holds[e->get_id()] = LR_STATUS_BOTTOM;
        }*/
      DYN_DEBUG(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << lrs->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
      //m_holds[e->get_id()] = lrs;
      lr_status_ref ret = lrs;
      stopwatch_stop(&expr_linear_relation_holds_visit_timer);
      return ret;
    } else if (e->is_function_sort() || e->is_nextpc_const() || e->is_memlabel_sort()) {
      //m_holds[e->get_id()] = lr_status_t::linearly_related(set<cs_addr_ref_id_t>());
      lr_status_ref ret = lr_status_t::linearly_related(set<cs_addr_ref_id_t>());
      stopwatch_stop(&expr_linear_relation_holds_visit_timer);
      return ret;
    } else if (/*e->is_array_sort() || */m_cs.is_canonical_constant(e)) {
      set<memlabel_t> bset;
      bset.insert(memlabel_t::memlabel_heap());
      set<cs_addr_ref_id_t> relevant_addr_ref_ids_minus_memlocs = lr_status_t::subtract_memlocs_from_addr_ref_ids(m_relevant_addr_ref_ids, m_symbol_map, m_ctx);
      //m_holds[e->get_id()] = lr_status_t::bottom(relevant_addr_ref_ids_minus_memlocs, bset); //if e is array sort, this value should never be used in future; thus storing some junk value here.
      if (m_cs.is_dst_symbol_addr(e)) {
        cs_addr_ref_id_t esp_addr_ref_id = m_cs.get_addr_ref_id_for_keyword(g_stack_symbol);
        relevant_addr_ref_ids_minus_memlocs.erase(esp_addr_ref_id);
      }
      lr_status_ref ret = lr_status_t::bottom(relevant_addr_ref_ids_minus_memlocs, bset); //if e is array sort, this value should never be used in future; thus storing some junk value here.
      DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
      stopwatch_stop(&expr_linear_relation_holds_visit_timer);
      return ret;
    } else if (e->is_array_sort()) {
      ASSERT(is_loc);
      lr_status_ref new_lr_status = get_lr_status_for_loc_ids(locs, m_pred_locs, m_relevant_addr_ref_ids);
      //m_holds[e->get_id()] = new_lr_status;
      stopwatch_stop(&expr_linear_relation_holds_visit_timer);
      return new_lr_status;
    }
  }


  //the order is important; this if-condition should be AFTER all the previous checks have failed.
  if (is_loc) {
    ASSERT(locs.size() == 1);
    graph_loc_id_t locid = *locs.cbegin();
    DYN_DEBUG(linear_relations, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << ", locs.front() = " << *locs.begin() << endl);
    //ASSERT(m_pred_locs.count(e_loc_id) > 0);
    //lr_status_t new_lr_status = get_lr_status_for_loc_ids(locs/*, m_cs_addr_ref_id, m_pc, m_domdefs, m_lr_map*/, m_pred_locs, m_relevant_addr_ref_ids);
    lr_status_ref new_lr_status = lr_status_t::top();
    if (m_pred_locs.count(locid)) {
      DYN_DEBUG2(linear_relations, cout << __func__ << " " << __LINE__ << ": found in m_pred_locs\n");
      new_lr_status = m_pred_locs.at(locid);
    } else {
      DYN_DEBUG2(linear_relations, cout << __func__ << " " << __LINE__ << ": not in m_pred_locs; returning start_pc_value\n");
      new_lr_status = lr_status_t::start_pc_value(e, m_relevant_addr_ref_ids, m_ctx, m_argument_regs, m_symbol_map);
    }
    //ASSERT(lr_status_meet(m_holds[e->get_id()], m_pred_locs.at(e_loc_id)) == m_pred_locs.at(e_loc_id));
    //ASSERT2(lr_status_meet(m_holds.at(e->get_id()), new_lr_status) == new_lr_status);
    DYN_DEBUG2(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << new_lr_status->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
    //m_holds[e->get_id()] = new_lr_status/*m_pred_locs.at(e_loc_id)*/;
    stopwatch_stop(&expr_linear_relation_holds_visit_timer);
    return new_lr_status;
  }



  if (e->is_const() || e->is_var()) {
    cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
    NOT_REACHED();
  }
  ASSERT(!e->is_var());
  ASSERT(!e->is_const());

  //expr_ref orig_e = e;
  //e = m_ctx->expr_simplify_using_sprel_and_memlabel_maps(e, sprel_map, /*alias_map, *//*m_*/memlabel_map/*, m_locid2expr_map*/, m_relevant_addr_ref_ids, m_symbol_map, m_locals_map, m_argument_regs);
  //ASSERT(e->get_args().size() > 0);

  ASSERT(e->get_args().size() > 0);
  op = e->get_operation_kind();
  expr_ref arg0, arg1, arg2, base_arg;
  switch (op) {
    case expr::OP_BVADD:
    case expr::OP_BVSUB:
      //cur_status = LR_STATUS_MUTUALLY_INDEPENDENT;
      ASSERT(arg_holds.size() == e->get_args().size());
      for (unsigned i = 0; i < e->get_args().size(); i++) {
        int arg_id = e->get_args()[i]->get_id();
        //ASSERT(m_holds.count(arg_id) > 0);
        //ASSERT(m_holds[arg_id] != LR_STATUS_TOP);
        if (/*m_holds[arg_id].is_linearly_related_to_non_empty_addr_ref_ids()*/
            arg_holds[i]->is_linearly_related_to_non_empty_addr_ref_ids()
           ) {
          //m_holds[e->get_id()] = LR_STATUS_LINEARLY_RELATED;
          //m_holds[e->get_id()] = m_holds.at(arg_id);
          lr_status_ref ret = arg_holds.at(i);
          DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
          stopwatch_stop(&expr_linear_relation_holds_visit_timer);
          return ret;
        }
        /*ASSERT(m_holds.count(arg_id) > 0);
          if (m_holds[arg_id] == LR_STATUS_BOTTOM) {
          cur_status = LR_STATUS_BOTTOM;
          } else if (m_holds[arg_id] == LR_STATUS_MUTUALLY_INDEPENDENT) {
          continue;
          } else NOT_REACHED();*/
      }
      /*m_holds[e->get_id()] = cur_status;
        stopwatch_stop(&expr_linear_relation_holds_visit_timer);
        return;*/
      break;
    case expr::OP_ITE:
      arg1 = e->get_args()[1];
      arg2 = e->get_args()[2];
      //if (   m_holds[arg1->get_id()].is_linearly_related_to_non_empty_addr_ref_ids()
      //    && m_holds[arg2->get_id()].is_linearly_related_to_non_empty_addr_ref_ids())
      if (   arg_holds.at(1)->is_linearly_related_to_non_empty_addr_ref_ids()
          && arg_holds.at(2)->is_linearly_related_to_non_empty_addr_ref_ids()) {
        //CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning linearly-related for " << m_cs.get_addr_ref(m_cs_addr_ref_id).first << " vs. " << expr_string(e) << endl);
        //m_holds[e->get_id()] = LR_STATUS_LINEARLY_RELATED;
        //m_holds[e->get_id()] = lr_status_t::lr_status_union(m_holds.at(arg1->get_id()), m_holds.at(arg2->get_id()));
        //lr_status_t ret = lr_status_t::lr_status_union(m_holds.at(arg1->get_id()), m_holds.at(arg2->get_id()));
        lr_status_ref ret = lr_status_t::lr_status_union(arg_holds.at(1), arg_holds.at(2));
        DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
        stopwatch_stop(&expr_linear_relation_holds_visit_timer);
        return ret;
      }
      break;
    case expr::OP_BVAND: {
      int base_arg_id;
      //if (e->get_args().size() == 2) {
      //  if (e->get_args().at(1)->is_highorder_mask_const()) {
      //    base_arg = e->get_args().at(0);
      //    base_arg_id = 0;
      //  } else if (e->get_args().at(0)->is_highorder_mask_const()) {
      //    base_arg = e->get_args().at(1);
      //    base_arg_id = 1;
      //  } else {
      //    break;
      //  }
      //}
      for (base_arg_id = 0; base_arg_id < e->get_args().size(); ++base_arg_id) {
        if (e->get_args().at(base_arg_id)->is_highorder_mask_const()) {
          base_arg = e->get_args().at(base_arg_id);
          break;
        }
      }
      if (base_arg_id >= e->get_args().size())
        break;
      //if (m_holds.at(base_arg->get_id()).is_linearly_related_to_non_empty_addr_ref_ids())
      if (arg_holds.at(base_arg_id)->is_linearly_related_to_non_empty_addr_ref_ids()) {
        //CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning linearly-related for " << m_cs.get_addr_ref(m_cs_addr_ref_id).first << " vs. " << expr_string(e) << endl);
        //m_holds[e->get_id()] = LR_STATUS_LINEARLY_RELATED;
        //m_holds[e->get_id()] = m_holds.at(base_arg->get_id());
        lr_status_ref ret = arg_holds.at(base_arg_id);
        DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
        stopwatch_stop(&expr_linear_relation_holds_visit_timer);
        return ret;
      }
      break;
    }
    case expr::OP_BVOR: {
      int base_arg_id;
      //if (e->get_args().size() == 2) {
      //  if (e->get_args().at(1)->is_loworder_mask_const()) {
      //    //base_arg = e->get_args().at(0);
      //    base_arg_id = 0;
      //  } else if (e->get_args().at(0)->is_loworder_mask_const()) {
      //    //base_arg = e->get_args().at(1);
      //    base_arg_id = 1;
      //  } else {
      //    break;
      //  }
      //}
      for (base_arg_id = 0; base_arg_id < e->get_args().size(); ++base_arg_id) {
        if (e->get_args().at(base_arg_id)->is_loworder_mask_const()) {
          break;
        }
      }
      if (base_arg_id >= e->get_args().size())
        break;
      //if (   base_arg
      //    && m_holds.at(base_arg->get_id()).is_linearly_related_to_non_empty_addr_ref_ids())
      if (arg_holds.at(base_arg_id)->is_linearly_related_to_non_empty_addr_ref_ids()) {
        //CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning linearly-related for " << m_cs.get_addr_ref(m_cs_addr_ref_id).first << " vs. " << expr_string(e) << endl);
        //m_holds[e->get_id()] = LR_STATUS_LINEARLY_RELATED;
        //m_holds[e->get_id()] = m_holds.at(base_arg->get_id());
        lr_status_ref ret = arg_holds.at(base_arg_id);
        DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
        stopwatch_stop(&expr_linear_relation_holds_visit_timer);
        return ret;
      }
      break;
    }
    case expr::OP_BVEXTRACT:
      arg0 = e->get_args()[0];
      arg1 = e->get_args()[1];
      ASSERT(arg0->is_bv_sort());
      ASSERT(arg1->is_const());
      //if (   m_holds.at(arg0->get_id()).is_linearly_related_to_non_empty_addr_ref_ids()
      //    && arg1->get_int_value() == (int)arg0->get_sort()->get_size() - 1)
      if (   arg_holds.at(0)->is_linearly_related_to_non_empty_addr_ref_ids()
          && arg1->get_int_value() == (int)arg0->get_sort()->get_size() - 1) {
        //if bottom few bits are getting masked, it is still linearly related
        //CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning linearly-related for " << m_cs.get_addr_ref(m_cs_addr_ref_id).first << " vs. " << expr_string(e) << endl);
        //m_holds[e->get_id()] = LR_STATUS_LINEARLY_RELATED;
        //m_holds[e->get_id()] = m_holds.at(arg0->get_id());
        lr_status_ref ret = arg_holds.at(0);
        //CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << m_holds.at(e->get_id()).lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
        DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
        stopwatch_stop(&expr_linear_relation_holds_visit_timer);
        return ret;
      }
      break;
    case expr::OP_BVCONCAT:
      arg0 = e->get_args()[0];
      ASSERT(arg0->is_bv_sort());
      //if (m_holds.at(arg0->get_id()).is_linearly_related_to_non_empty_addr_ref_ids())
      if (arg_holds.at(0)->is_linearly_related_to_non_empty_addr_ref_ids()) {
        //if top few bits are linearly related, then the whole value is linearly related
        //CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning linearly-related for " << m_cs.get_addr_ref(m_cs_addr_ref_id).first << " vs. " << expr_string(e) << endl);
        //m_holds[e->get_id()] = LR_STATUS_LINEARLY_RELATED;
        //m_holds[e->get_id()] = m_holds.at(arg0->get_id());
        lr_status_ref ret = arg_holds.at(0);
        DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
        stopwatch_stop(&expr_linear_relation_holds_visit_timer);
        return ret;
      }
      break;
    case expr::OP_ALLOCA: {
      NOT_REACHED();
      //auto ret = arg_holds.at(0);
      //CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
      //stopwatch_stop(&expr_linear_relation_holds_visit_timer);
      //return ret;
      //break;
    }
    default:
      break;
  }

  /*if (op == expr::OP_FUNCTION_CALL) {
    m_holds[e->get_id()] = lr_status_t::start_pc_value(m_relevant_addr_ref_ids, m_cs); //XXX: do not consider function calls to have any dependence on fstack
    CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << m_holds.at(e->get_id()).lr_status_to_string() << " for " << expr_string(e) << endl);
    stopwatch_stop(&expr_linear_relation_holds_visit_timer);
    return;
    }*/

  set<cs_addr_ref_id_t> dep_addr_ref_ids;
  set<memlabel_t> dep_bottom_set;
  bool is_bottom = false;
  for (unsigned i = 0; i < e->get_args().size(); i++) {
    expr_ref const &arg = e->get_args()[i];
    //ASSERT(m_holds.count(arg->get_id()) != 0);
    //lr_status_t arg_status = m_holds.at(e->get_args()[i]->get_id());
    lr_status_ref arg_status = arg_holds.at(i);
    if (!arg->is_memlabel_sort() && arg_status->is_top()) {
      //m_holds[e->get_id()] = lr_status_t::top();
      lr_status_ref ret = lr_status_t::top();
      stopwatch_stop(&expr_linear_relation_holds_visit_timer);
      DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);
      return ret;
    } else if (!arg->is_memlabel_sort()) {
      if (!expr_can_affect_aliasing(e->get_operation_kind(), i))
        continue;
      //cout << __func__ << " " << __LINE__ << ": i = " << i << endl;
      //cout << __func__ << " " << __LINE__ << ": arg = " << expr_string(arg) << endl;
      //cout << __func__ << " " << __LINE__ << ": arg_status = " << arg_status.lr_status_to_string(m_cs) << endl;
      set_union(dep_addr_ref_ids, arg_status->get_cs_addr_ref_ids());
      set_union(dep_bottom_set, arg_status->get_bottom_set());
      if (arg_status->is_bottom()) {
        is_bottom = true;
      }
    }
  }
  lr_status_ref ret;
  if (is_bottom) {
    //m_holds[e->get_id()] = lr_status_t::bottom(dep_addr_ref_ids, dep_bottom_set);
    ret = lr_status_t::bottom(dep_addr_ref_ids, dep_bottom_set);
  } else {
    ASSERT(dep_bottom_set.empty());
    //m_holds[e->get_id()] = lr_status_t::linearly_related(dep_addr_ref_ids);
    ret = lr_status_t::linearly_related(dep_addr_ref_ids);
  }
  DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(m_cs) << " for " << expr_string(e) << endl);

  //if (   op == expr::OP_SELECT
  //    || op == expr::OP_STORE) {
  //  expr_ref const &addr = e->get_args().at(2);
  //  lr_status_t const &addr_status = m_holds.at(addr->get_id());
  //  ASSERT(e->get_args().at(1)->is_memlabel_sort());
  //  ASSERT(e->get_args().at(1)->is_var());
  //  mlvarname_t mlvarname = expr_string(e->get_args().at(1));
  //  memlabel_t ml = addr_status.to_memlabel(m_cs, m_symbol_map, m_locals_map);
  //  cout << __func__ << " " << __LINE__ << ": assigning value " << ml.to_string() << " to " << mlvarname << "\n";
  //  m_memlabel_map[mlvarname] = ml;
  //} else if (   m_update_callee_memlabels
  //           && op == expr::OP_FUNCTION_CALL) {
  //  vector<memlabel_t> farg_memlabels;
  //  size_t num_fargs = e->get_args().size() - OP_FUNCTION_CALL_ARGNUM_FARG0;
  //  //expr_ref orig_memvar = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_ORIG_MEMVAR);
  //  for (size_t i = OP_FUNCTION_CALL_ARGNUM_FARG0; i < e->get_args().size(); i++) {
  //    expr_ref arg = e->get_args().at(i);
  //    //memlabel_t addr_ml = get_memlabel_for_addr(e->get_args().at(i));
  //    memlabel_t addr_ml = m_holds.at(arg->get_id()).to_memlabel(m_cs, m_symbol_map, m_locals_map);
  //    //cout << __func__ << " " << __LINE__ << ": addr_ml = " << addr_ml.to_string() << endl;
  //    //cout << __func__ << " " << __LINE__ << ": orig_memvar = " << expr_string(orig_memvar) << endl;
  //    memlabel_t addr_reachable_ml;
  //    if (!addr_ml.memlabel_is_top()) {
  //      addr_reachable_ml = get_reachable_memlabels(addr_ml/*, orig_memvar*/);
  //    } else {
  //      addr_reachable_ml = addr_ml;
  //    }
  //    farg_memlabels.push_back(addr_reachable_ml);
  //    //cout << __func__ << " " << __LINE__ << ": farg = " << expr_string(e->get_args().at(i)) << endl;
  //    //cout << __func__ << " " << __LINE__ << ": addr_reachable_ml = " << addr_reachable_ml.to_string() << endl;
  //  }
  //  expr_ref nextpc = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_NEXTPC_CONST);
  //  callee_summary_t csum;
  //  if (nextpc->is_nextpc_const()) {
  //    unsigned nextpc_num = nextpc->get_nextpc_num();
  //    //csum = m_graph.get_callee_summary_for_nextpc(nextpc_num, num_fargs);
  //    csum = m_get_callee_summary_fn(nextpc_num, num_fargs);
  //    //cout << __func__ << " " << __LINE__ << ": nextpc_num = " << nextpc_num << "\n";
  //    //cout << __func__ << " " << __LINE__ << ": csum = " << csum.callee_summary_to_string_for_eq() << "\n";
  //  } else {
  //    csum = callee_summary_t::callee_summary_bottom(m_cs, m_relevant_addr_ref_ids, m_symbol_map, m_locals_map, num_fargs);
  //  }
  //  pair<memlabel_t, memlabel_t> pp = csum.get_readable_writeable_memlabels(farg_memlabels);
  //  memlabel_t ml_readable = get_reachable_memlabels(pp.first/*, orig_memvar*/);
  //  memlabel_t ml_writeable= get_reachable_memlabels(pp.second/*, orig_memvar*/);
  //  expr_ref ml_readable_var = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE);
  //  expr_ref ml_writeable_var = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE);
  //  ASSERT(ml_readable_var->is_var());
  //  ASSERT(ml_readable_var->is_memlabel_sort());
  //  ASSERT(ml_writeable_var->is_var());
  //  ASSERT(ml_writeable_var->is_memlabel_sort());
  //  mlvarname_t ml_readable_varname = expr_string(ml_readable_var);
  //  mlvarname_t ml_writeable_varname = expr_string(ml_writeable_var);
  //  cout << __func__ << " " << __LINE__ << ": assigning value " << ml_readable.to_string() << " to " << ml_readable_varname << "\n";
  //  cout << __func__ << " " << __LINE__ << ": assigning value " << ml_writeable.to_string() << " to " << ml_writeable_varname << "\n";
  //  m_memlabel_map[ml_readable_varname] = ml_readable;
  //  m_memlabel_map[ml_writeable_varname] = ml_writeable;
  //  //args_subs[OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE] = m_context->mk_memlabel_const(ml_readable);
  //  //args_subs[OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE] = m_context->mk_memlabel_const(ml_writeable);
  //  //m_map_from_to[e->get_id()] = m_context->mk_app(e->get_operation_kind(), args_subs);
  //}

  /*for (size_t i = 0; i < e->get_args().size(); i++) {
    expr_ref arg = e->get_args()[i];
    int arg_id = arg->get_id();
    ASSERT(m_holds.count(arg_id) > 0);
    ASSERT(!m_holds.at(arg_id).is_top())
    if (m_holds[arg_id] != LR_STATUS_MUTUALLY_INDEPENDENT) {
    m_holds[e->get_id()] = LR_STATUS_BOTTOM;
    stopwatch_stop(&expr_linear_relation_holds_visit_timer);
    CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << lr_status_to_string(m_holds[e->get_id()]) << " for " << cs_addr_ref_str << " vs. " << expr_string(e) << endl);
    return;
    }
    }
    m_holds[e->get_id()] = LR_STATUS_MUTUALLY_INDEPENDENT;*/
  stopwatch_stop(&expr_linear_relation_holds_visit_timer);
  //CPP_DBG_EXEC3(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << lr_status_to_string(m_holds[e->get_id()]) << " for " << cs_addr_ref_str << " vs. " << expr_string(e) << endl);
  return ret;
}



template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
lr_status_ref
graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::compute_lr_status_for_expr(expr_ref e, map<graph_loc_id_t, graph_cp_location> const &locs, map<graph_loc_id_t, lr_status_ref> const &prev_lr, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_arg_regs_t const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, eqspace::consts_struct_t const &cs) const
{
  autostop_timer func_timer(__func__);
  context *ctx = e->get_context();

  pair<reg_type, long> r;
  expr_ref r_expr;
  //lr_status_t ret;

  //e = ctx->expr_do_simplify(e);
  DYN_DEBUG(linear_relations, cout << __func__ << " " << __LINE__ << ": Checking if linear relation holds for: " << expr_string(e) << " id " << e->get_id() << endl);
  /*if (expr_contains_memlabel_top(e)) {
    CPP_DBG_EXEC(linear_relations, cout << "returning lr_status_top" << endl);
    return LR_STATUS_TOP;
  }*/

  expr_linear_relation_holds_visitor<T_PC, T_N, T_E, T_PRED> visitor(e, locs, /*cs_addr_ref_id, *//*p, domdefs, lr_map*/prev_lr/*, start_pc_locs*/, relevant_addr_refs, argument_regs, symbol_map, locals_map, ctx, cs, *this);
  //ret = visitor.get_status();
  lr_status_ref ret = visitor.visit_recursive(e, {});
  DYN_DEBUG(linear_relations, cout << __func__ << " " << __LINE__ << ": returning " << ret->lr_status_to_string(cs) << " for " << expr_string(e) << endl);
  return ret;
}

#if 0
lr_status_t
compute_lr_status_for_expr(expr_ref e, map<graph_loc_id_t, lr_status_t> const &prev_lr, set<cs_addr_ref_id_t> const &relevant_addr_refs, map<string, expr_ref> const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, eqspace::consts_struct_t const &cs)
{
  NOT_IMPLEMENTED();
  //graph_memlabel_map_t dummy;
  //return compute_lr_status_for_expr_updating_memlabel_map(e, prev_lr, relevant_addr_refs, argument_regs, symbol_map, locals_map, cs, true, dummy);
}
#endif

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class populate_memlabel_map_visitor : public expr_visitor
{
public:
  populate_memlabel_map_visitor(
      expr_ref const &e,
      map<graph_loc_id_t, graph_cp_location> const &locs,
      map<graph_loc_id_t, lr_status_ref> const &in,
      sprel_map_pair_t const &sprel_map_pair,
      set<cs_addr_ref_id_t> const &relevant_addr_refs,
      graph_arg_regs_t const &argument_regs,
      graph_symbol_map_t const &symbol_map,
      graph_locals_map_t const &locals_map,
      bool update_callee_memlabels,
      graph_memlabel_map_t &memlabel_map,
      graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const& graph/*,
      std::function<callee_summary_t (nextpc_id_t, int)> get_callee_summary_fn,
      std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn*/
    ) : /*m_expr(e),*/
        m_locs(locs),
        m_in(in),
        m_sprel_map_pair(sprel_map_pair),
        m_relevant_addr_refs(relevant_addr_refs),
        m_argument_regs(argument_regs),
        m_symbol_map(symbol_map),
        m_locals_map(locals_map),
        m_update_callee_memlabels(update_callee_memlabels),
        m_memlabel_map(memlabel_map),
        m_graph(graph),
        //m_get_callee_summary_fn(get_callee_summary_fn),
        //m_expr2locid_fn(expr2locid_fn),
        m_ctx(e->get_context()),
        m_cs(m_ctx->get_consts_struct())
  {
    for (auto ri : m_relevant_addr_refs) {
      pair<reg_type, long> p = m_cs.get_addr_ref_reg_type_and_id(ri);
      alias_region_t::alias_type altype = memlabel_t::get_extra_type_using_regtype(p.first);
      if (altype == alias_region_t::alias_type_stack) {
        continue;
      }
      m_relevant_addr_refs_minus_stack.insert(ri);
    }
    visit_recursive(e);
  }
  void visit(expr_ref const &e)
  {
    autostop_timer func_timer("populate_memlabel_map.visit");
    expr::operation_kind op = e->get_operation_kind();
    if (   op == expr::OP_SELECT
        || op == expr::OP_STORE) {
      expr_ref addr = e->get_args().at(2);
      expr_ref const &mlexpr = e->get_args().at(1);
      if (mlexpr->is_var()) {
        mlvarname_t mlvarname = mlexpr->get_name();

        addr = m_ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(addr, m_sprel_map_pair, m_memlabel_map);
        DYN_DEBUG2(alias_analysis, cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << endl);
        lr_status_ref addr_status = m_graph.compute_lr_status_for_expr(addr, m_locs, m_in, m_relevant_addr_refs, m_argument_regs, m_symbol_map, m_locals_map, m_ctx->get_consts_struct()/*, m_expr2locid_fn*/);
        memlabel_t ml = addr_status->to_memlabel(m_cs, m_symbol_map, m_locals_map);
        DYN_DEBUG2(alias_analysis, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl);
        DYN_DEBUG2(alias_analysis, cout << __func__ << " " << __LINE__ << ": assigning value " << ml.to_string() << " to " << mlvarname->get_str() << "\n");
        //if (memlabel_contains_stack(&ml) && memlabel_contains_heap(&ml)) {
        //  cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
        //  cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << endl;
        //  cout << __func__ << " " << __LINE__ << ": assigning value " << ml.to_string() << " to " << mlvarname->get_str() << "\n";
        //  cout << __func__ << " " << __LINE__ << ": ml = " << ml.to_string() << endl;
        //  cout << __func__ << " " << __LINE__ << ": m_sprel_map_pair = " << m_sprel_map_pair.to_string_for_eq() << endl;
        //  cout << __func__ << " " << __LINE__ << ": m_memlabel_map = " << memlabel_map_to_string(m_memlabel_map) << endl;
        //  NOT_REACHED();
        //}
        update_memlabel_map(mlvarname, ml);
        //m_memlabel_map[mlvarname] = ml;
      }
    } else if (   op == expr::OP_FUNCTION_CALL
               && (   m_cs.expr_is_fcall_with_unknown_args(e->get_args().at(0))
                   || m_update_callee_memlabels)) {
      memlabel_t ml_readable, ml_writeable;
      memlabel_t ml_fcall_bottom = callee_summary_t::memlabel_fcall_bottom(m_cs, m_relevant_addr_refs_minus_stack, m_symbol_map, m_locals_map);
      if (m_cs.expr_is_fcall_with_unknown_args(e->get_args().at(0))) {
        ml_readable = ml_fcall_bottom;
        ml_writeable = ml_fcall_bottom;
      } else if (m_update_callee_memlabels) {
        autostop_timer atimer("populate_memlabel_map.visit.fcall");
        vector<memlabel_t> farg_memlabels;
        size_t num_fargs = e->get_args().size() - OP_FUNCTION_CALL_ARGNUM_FARG0;
        for (size_t i = OP_FUNCTION_CALL_ARGNUM_FARG0; i < e->get_args().size(); i++) {
          expr_ref arg = e->get_args().at(i);

          arg = m_ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(arg, m_sprel_map_pair, m_memlabel_map);
          lr_status_ref arg_status = m_graph.compute_lr_status_for_expr(arg, m_locs, m_in, m_relevant_addr_refs, m_argument_regs, m_symbol_map, m_locals_map, m_cs/*, m_expr2locid_fn*/);
          memlabel_t arg_ml = arg_status->to_memlabel(m_cs, m_symbol_map, m_locals_map);
          DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl);
          DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": argnum = " << (i - OP_FUNCTION_CALL_ARGNUM_FARG0) << ", simplified arg = " << expr_string(arg) << ", arg_ml = " << arg_ml.to_string() << endl);
          //if (memlabel_contains_stack(&arg_ml)) {
          //  NOT_REACHED();
          //}
          memlabel_t arg_reachable_ml;
          if (!arg_ml.memlabel_is_top()) {
            arg_reachable_ml = get_reachable_memlabels(arg_ml);
          } else {
            NOT_REACHED();
            arg_reachable_ml = arg_ml;
          }
          farg_memlabels.push_back(arg_reachable_ml);
          DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": farg = " << expr_string(e->get_args().at(i)) << endl);
          DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": arg_reachable_ml = " << arg_reachable_ml.to_string() << endl);
        }
        expr_ref nextpc = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_NEXTPC_CONST);
        callee_summary_t csum;
        if (nextpc->is_nextpc_const()) {
          unsigned nextpc_num = nextpc->get_nextpc_num();
          //csum = m_get_callee_summary_fn(nextpc_num, num_fargs);
          csum = m_graph.get_callee_summary_for_nextpc(nextpc_num, num_fargs);
          DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": nextpc_num = " << nextpc_num << "\n");
          DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": csum = " << csum.callee_summary_to_string_for_eq() << "\n");
        } else {
          csum = callee_summary_t::callee_summary_bottom(m_cs, m_relevant_addr_refs, m_symbol_map, m_locals_map, num_fargs);
        }
        pair<memlabel_t, memlabel_t> pp = csum.get_readable_writeable_memlabels(farg_memlabels, ml_fcall_bottom);
        ml_readable = get_reachable_memlabels(pp.first/*, orig_memvar*/);
        ml_writeable = get_reachable_memlabels(pp.second/*, orig_memvar*/);
      }
      expr_ref const &ml_readable_var = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE);
      expr_ref const &ml_writeable_var = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE);
      ASSERT(ml_readable_var->is_var());
      ASSERT(ml_readable_var->is_memlabel_sort());
      ASSERT(ml_writeable_var->is_var());
      ASSERT(ml_writeable_var->is_memlabel_sort());
      mlvarname_t const &ml_readable_varname = ml_readable_var->get_name();
      mlvarname_t const &ml_writeable_varname = ml_writeable_var->get_name();
      DYN_DEBUG(linear_relations, cout << __func__ << " " << __LINE__ << ": assigning value " << ml_readable.to_string() << " to " << ml_readable_varname->get_str() << "\n");
      DYN_DEBUG(linear_relations, cout << __func__ << " " << __LINE__ << ": assigning value " << ml_writeable.to_string() << " to " << ml_writeable_varname->get_str() << "\n");
      update_memlabel_map(ml_readable_varname, ml_readable);
      update_memlabel_map(ml_writeable_varname, ml_writeable);
      //m_memlabel_map[ml_readable_varname] = ml_readable;
      //m_memlabel_map[ml_writeable_varname] = ml_writeable;
      //args_subs[OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE] = m_context->mk_memlabel_const(ml_readable);
      //args_subs[OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE] = m_context->mk_memlabel_const(ml_writeable);
      //m_map_from_to[e->get_id()] = m_context->mk_app(e->get_operation_kind(), args_subs);
    }
  }
private:
  void update_memlabel_map(mlvarname_t const &mlvarname, memlabel_t const &ml)
  {
    if (   m_memlabel_map.count(mlvarname) == 0
        || m_memlabel_map.at(mlvarname)->get_ml().memlabel_is_top()) {
      m_memlabel_map[mlvarname] = mk_memlabel_ref(ml);
    } else {
      memlabel_t existing_ml = m_memlabel_map.at(mlvarname)->get_ml();
      memlabel_t::memlabels_union(&existing_ml, &ml); //union is required because it is possible for the same mlvarname to appear in two different places (e.g., for CG), and so we should take the union of both places
      m_memlabel_map.at(mlvarname) = mk_memlabel_ref(existing_ml);
    }
  }
  graph_loc_id_t get_loc_id_for_memmask(memlabel_t const &ml) const
  {
    ASSERT(memlabel_t::memlabel_is_atomic(&ml));
    for (const auto &l : m_locs) {
      if (   l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED
          && memlabel_t::memlabels_equal(&l.second.m_memlabel->get_ml(), &ml)) {
        return l.first;
      }
    }
    NOT_REACHED();
  }

  memlabel_t get_reachable_memlabels(memlabel_t const &ml) const
  {
    set<memlabel_ref> reachable_mls;
    set<memlabel_ref> new_reachable_mls = ml.get_atomic_memlabels();
    do {
      reachable_mls = new_reachable_mls;
      for (const auto &amlr : reachable_mls) {
        auto const& aml = amlr->get_ml();
        ASSERT(memlabel_t::memlabel_is_atomic(&aml));
        graph_loc_id_t loc_id = get_loc_id_for_memmask(aml);
        //memlabel_t rml = m_loc2status_map.at(loc_id).to_memlabel(m_cs, m_symbol_map, m_locals_map);
        memlabel_t rml = m_in.at(loc_id)->to_memlabel(m_cs, m_symbol_map, m_locals_map);
        if (!rml.memlabel_is_top()) {
          set<memlabel_ref> s = rml.get_atomic_memlabels();
          for (auto const& ml : s) {
            new_reachable_mls.insert(ml);
          }
        }
      }
    } while (reachable_mls.size() != new_reachable_mls.size());
    return memlabel_t::memlabel_union(new_reachable_mls);
  }

  //expr_ref const &m_expr;
  map<graph_loc_id_t, graph_cp_location> const &m_locs;
  map<graph_loc_id_t, lr_status_ref> const &m_in;
  sprel_map_pair_t const &m_sprel_map_pair;
  set<cs_addr_ref_id_t> const &m_relevant_addr_refs;
  graph_arg_regs_t const &m_argument_regs;
  graph_symbol_map_t const &m_symbol_map;
  graph_locals_map_t const &m_locals_map;
  bool m_update_callee_memlabels;
  graph_memlabel_map_t &m_memlabel_map;
  graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const& m_graph;
  //std::function<callee_summary_t (nextpc_id_t, int)> m_get_callee_summary_fn;
  //std::function<graph_loc_id_t (expr_ref const &)> m_expr2locid_fn;

  set<cs_addr_ref_id_t> m_relevant_addr_refs_minus_stack;
  context *m_ctx;
  consts_struct_t const &m_cs;
  //map<expr_id_t, set<graph_loc_id_t>> m_locs_map;
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::populate_memlabel_map_for_expr_and_loc_status(expr_ref const &e, map<graph_loc_id_t, graph_cp_location> const &locs, map<graph_loc_id_t, lr_status_ref> const &in, sprel_map_pair_t const &sprel_map_pair, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_arg_regs_t const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, graph_memlabel_map_t &memlabel_map/*, std::function<callee_summary_t (nextpc_id_t, int)> get_callee_summary_fn, std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn*/) const
{
  //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
  populate_memlabel_map_visitor<T_PC, T_N, T_E, T_PRED> visitor(e, locs, in, sprel_map_pair, relevant_addr_refs, argument_regs, symbol_map, locals_map, update_callee_memlabels, memlabel_map, *this/*get_callee_summary_fn, expr2locid_fn*/);
}


template class graph_with_aliasing<pc, tfg_node, tfg_edge, predicate>;
template class graph_with_aliasing<pcpair, corr_graph_node, corr_graph_edge, predicate>;

}
