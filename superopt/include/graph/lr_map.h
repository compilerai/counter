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
    if (loc_i.m_type == GRAPH_CP_LOCATION_TYPE_REGMEM) {
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

//lr_status_ref compute_lr_status_for_expr(expr_ref e/*, eq::state const &from_state*//*, cs_addr_ref_id_t cs_addr_ref_id*//*, T_PC const &p, map<T_PC, map<graph_loc_id_t, T_PC>> const &domdefs, map<T_PC, lr_map_t<T_PC, T_N, T_E, T_PRED>> const &lr_map*/, map<graph_loc_id_t, graph_cp_location> const &locs, map<graph_loc_id_t, lr_status_ref> const &prev_lr/*, map<graph_loc_id_t, graph_cp_location> const &start_pc_locs*/, sprel_map_pair_t const &sprel_map_pair, graph_memlabel_map_t const &memlabel_map, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_arg_regs_t const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, eqspace::consts_struct_t const &cs, std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn);
//memlabel_t get_memlabel_for_addr_ref_ids(set<cs_addr_ref_id_t> const &addr_ref_ids, consts_struct_t const &cs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map);

//void
//populate_memlabel_map_for_expr_and_loc_status(expr_ref const &new_e, map<graph_loc_id_t, graph_cp_location> const &locs, map<graph_loc_id_t, lr_status_ref> const &in, sprel_map_pair_t const &sprel_map_pair, set<cs_addr_ref_id_t> const &relevant_addr_refs, map<string_ref, expr_ref> const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, graph_memlabel_map_t &memlabel_map, std::function<callee_summary_t (nextpc_id_t, int)> get_callee_summary_fn, std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn);
}
