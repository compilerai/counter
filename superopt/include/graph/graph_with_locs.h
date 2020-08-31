#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "graph/graph_loc_id.h"
#include "graph/graph_with_predicates.h"
#include "gsupport/sprel_map.h"
#include "graph/locset.h"
#include "support/timers.h"
#include "tfg/parse_input_eq_file.h"

#include <map>
#include <list>
#include <string>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_locs : public graph_with_predicates<T_PC, T_N, T_E, T_PRED>
{
public:
  graph_with_locs(string const &name, context* ctx) : graph_with_predicates<T_PC,T_N,T_E,T_PRED>(name, ctx), m_locs(make_shared<map<graph_loc_id_t, graph_cp_location>>())
  {
  }

  graph_with_locs(graph_with_locs const &other) : graph_with_predicates<T_PC,T_N,T_E,T_PRED>(other),
                                                  m_loc_liveness(other.m_loc_liveness),
                                                  m_loc_definedness(other.m_loc_definedness),
                                                  m_bavlocs(other.m_bavlocs),
                                                  m_locs(other.m_locs),
                                                  m_sprels(other.m_sprels),
                                                  m_locid2expr_map(other.m_locid2expr_map),
                                                  m_exprid2locid_map(other.m_exprid2locid_map)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_locs(istream& in, string const& name, context* ctx);
  virtual ~graph_with_locs() = default;

  void print_graph_locs() const
  {
    cout << "Printing graph locs (" << get_num_graph_locs() << " total)\n";
    cout << this->graph_locs_to_string();
    cout << "done printing locs" << endl;
  }

  expr_ref const& graph_loc_get_value_for_node(graph_loc_id_t loc_index) const { return m_locid2expr_map.at(loc_index); }

  void set_locs(map<graph_loc_id_t, graph_cp_location> const &locs) { m_locs = make_shared<map<graph_loc_id_t, graph_cp_location>>(locs); }
  virtual map<graph_loc_id_t, graph_cp_location> const &get_locs() const { return *m_locs; }
  graph_cp_location const &get_loc(graph_loc_id_t loc_id) const { return m_locs->at(loc_id); }

  void get_all_loc_ids(set<graph_loc_id_t> &out) const
  {
    for (auto const& loc : *m_locs) {
      out.insert(loc.first);
    }
  }

  expr_ref get_loc_expr(graph_loc_id_t loc_id) const
  {
    return this->graph_loc_get_value(T_PC::start(), this->get_start_state(), loc_id);
  }

  static void graph_add_loc(map<graph_loc_id_t, graph_cp_location> &locs_map, graph_loc_id_t loc_id, graph_cp_location const &loc) { locs_map[loc_id] = loc; }

  static graph_loc_id_t get_max_loc_id(map<graph_loc_id_t, graph_cp_location> const &old_locs, map<graph_loc_id_t, graph_cp_location> const &new_locs)
  {
    graph_loc_id_t ret = 1000;
    for (auto const& loc : new_locs) {
      ret = max(ret, loc.first);
    }
    for (auto const& loc : old_locs) {
      ret = max(ret, loc.first);
    }
    return ret;
  }

  graph_loc_id_t get_loc_id_for_newloc(graph_cp_location const &newloc, map<graph_loc_id_t, graph_cp_location> const &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs)
  {
    for (auto const& old_loc : old_locs) {
      if (   old_loc.second.equals_mod_comment(newloc)
          && !new_locs.count(old_loc.first)) {
        return old_loc.first;
      }
    }
    graph_loc_id_t ret = this->get_max_loc_id(old_locs, new_locs) + 1;
    return ret;
  }

  void graph_add_new_loc(map<graph_loc_id_t, graph_cp_location> &locs_map, graph_cp_location const &newloc, map<graph_loc_id_t, graph_cp_location> const &old_locs, graph_loc_id_t suggested_loc_id = -1)
  {
    graph_loc_id_t fresh_id = this->get_loc_id_for_newloc(newloc, locs_map, old_locs);
    if (   suggested_loc_id != -1
        && locs_map.count(suggested_loc_id) == 0) {
      fresh_id = suggested_loc_id;
    }
    ASSERT(locs_map.count(fresh_id) == 0 || locs_map.at(fresh_id).equals_mod_comment(newloc));
    locs_map[fresh_id] = newloc;
  }

  void graph_add_loc(map<graph_loc_id_t, graph_cp_location> &new_locs, string const &memname, expr_ref addr, int nbytes, bool bigendian, map<graph_loc_id_t, graph_cp_location> const &old_locs)
  {
    CPP_DBG_EXEC2(TFG_LOCS, cout << "trying to add graph loc slot. addr = " << expr_string(addr) << endl);
    if (expr_num_op_occurrences(addr, expr::OP_STORE) > 0) { //ignore any address that contains a STORE
      CPP_DBG_EXEC2(TFG_LOCS, cout << "ignoring because store occurs in addr" << endl);
      return;
    }
    if (expr_num_op_occurrences(addr, expr::OP_SELECT) > 0) { //ignore any address that contains more than zero selects
      CPP_DBG_EXEC2(TFG_LOCS, cout << "ignoring because select occurs in addr more than once" << endl);
      return;
    }
    /*if (expr_contains_select_on_reg_type(addr, rt, i, j)) { //ignore any address that contains a select to the same reg type
      CPP_DBG_EXEC(TFG_LOCS, cout << "ignoring because select occurs on same reg type" << endl);
      return;
    }*/
    graph_cp_location newloc;
    newloc.m_memname = mk_string_ref(memname);
    newloc.m_addr = addr;
    newloc.m_nbytes = nbytes;
    newloc.m_bigendian = bigendian;
    newloc.m_memlabel = mk_memlabel_ref(memlabel_t::memlabel_top());
    newloc.m_type = GRAPH_CP_LOCATION_TYPE_MEMSLOT;
    newloc.m_reg_type = reg_type_memory;
    newloc.m_reg_index_i = -1;
    newloc.m_reg_index_j = -1;
    newloc.m_bigendian = false;

    CPP_DBG_EXEC2(TFG_LOCS, cout << "Adding graph_cp_loc slot " << newloc.to_string() << endl);
    graph_add_new_loc(new_locs, newloc, old_locs);
  }

  void graph_locs_add_location_llvmvar(map<graph_loc_id_t, graph_cp_location> &new_locs, string const& varname, sort_ref const &sort, map<graph_loc_id_t, graph_cp_location> const &old_locs)
  {
    context* ctx = this->get_context();
    graph_cp_location newloc;
    newloc.m_type = GRAPH_CP_LOCATION_TYPE_LLVMVAR;
    newloc.m_varname = mk_string_ref(varname);
    newloc.m_var = ctx->mk_var(string(G_INPUT_KEYWORD ".") + varname, sort);

    string expected_prefix = G_LLVM_PREFIX "-%";
    graph_loc_id_t varnum = -1;
    if (string_has_prefix(varname, expected_prefix)) {
      string varnum_str = varname.substr(expected_prefix.length());
      try {
        varnum = stoi(varnum_str);
      } catch (std::invalid_argument const& _unused) {
        //do nothing
      }
    }
    graph_add_new_loc(new_locs, newloc, old_locs, varnum);
  }

  void graph_locs_add_location_reg(map<graph_loc_id_t, graph_cp_location> &new_locs, string const& name, sort_ref const &sort, map<graph_loc_id_t, graph_cp_location> const &old_locs)
  {
    context* ctx = this->get_context();
    graph_cp_location newloc;
    newloc.m_type = GRAPH_CP_LOCATION_TYPE_REGMEM;
    newloc.m_varname = mk_string_ref(name);
    newloc.m_var = ctx->mk_var(string(G_INPUT_KEYWORD ".") + name, sort);
    graph_loc_id_t suggested_loc_id = -1;
    size_t dot;
    if ((dot = name.rfind('.')) != string::npos) {
      if (name.substr(0, dot) == G_DST_KEYWORD "." G_REGULAR_REG_NAME "0") {
        string suffix = name.substr(dot + 1);
        istringstream ss(suffix);
        if (!(ss >> suggested_loc_id)) {
          suggested_loc_id = -1;
        }
      }
    }

    graph_add_new_loc(new_locs, newloc, old_locs, suggested_loc_id);
  }

  void graph_locs_add_all_llvmvars(map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs)
  {
    autostop_timer func_timer(__func__);
    //shared_ptr<T_N> start_node = this->find_node(T_PC::start());
    //ASSERT(start_node);
    //state const &start_state = this->get_start_state();
    //list<string> llvm_vars;
    //for(const auto& n_r : start_state.get_value_expr_map_ref()) {
    //  if (   context::is_llvmvarname(n_r.first->get_str())
    //      && (n_r.second->is_bv_sort() || n_r.second->is_bool_sort())) {
    //    llvm_vars.push_back(n_r.first->get_str());
    //  }
    //}
    //llvm_vars.sort();
    map<string, sort_ref> llvm_vars;
    for (auto const& edge : this->get_edges()) {
      function<void (string const&, expr_ref)> func =
        [&llvm_vars](string const& name, expr_ref const& e) -> void {
          if (   context::is_llvmvarname(name)
              && (e->is_bv_sort() || e->is_bool_sort())) {
            llvm_vars.insert(make_pair(name, e->get_sort()));
          }
        };
      edge->visit_exprs_const(func);
    }
    for (auto const& ke : this->get_argument_regs().get_map()) {
      ASSERT(ke.second->is_var());
      string const& name = ke.second->get_name()->get_str();
      string key = name.substr(strlen(G_INPUT_KEYWORD "."));
      llvm_vars.insert(make_pair(key, ke.second->get_sort()));
    }
    for (auto const& str_e : llvm_vars) {
      graph_locs_add_location_llvmvar(new_locs, str_e.first, str_e.second, old_locs);
    }
  }

  void graph_locs_add_all_bools_and_bitvectors(map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs)
  {
    //state const &start_state = this->get_start_state();
    //map<string_ref, expr_ref> const &m = start_state.get_value_expr_map_ref();
    //for (const auto &ve : m) {
    //  string const &name = ve.first->get_str();
    //  if (context::is_llvmvarname(name)) {
    //    continue;
    //  }
    //  if (   name.find(string(".") + g_regular_reg_name) != string::npos
    //      || name.find(g_regular_reg_name) == 0) {
    //    continue;
    //  }
    //  if (ve.second->is_bool_sort() || ve.second->is_bv_sort()) {
    //    graph_locs_add_location_reg(new_locs, ve.first->get_str(), old_locs);
    //  }
    //}
    map<string, sort_ref> names;
    for (auto const& edge : this->get_edges()) {
      function<void (string const&, expr_ref)> func =
        [&names](string const& name, expr_ref const& e) -> void {
          if (context::is_llvmvarname(name)) {
            return;
          }
          if (   name.find(string(".") + g_regular_reg_name) != string::npos
              || name.find(g_regular_reg_name) == 0) {
            return;
          }
          if (e->is_bool_sort() || e->is_bv_sort()) {
            names.insert(make_pair(name, e->get_sort()));
          }
        };
      edge->visit_exprs_const(func);
    }
    for (auto const& name_e : names) {
      graph_locs_add_location_reg(new_locs, name_e.first, name_e.second, old_locs);
    }
  }

  void graph_locs_add_all_exvregs(map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs)
  {
    autostop_timer func_timer(__func__);
    //INFO("add_all_exvregs" << endl);
    //list<string> names;
    //shared_ptr<T_N> start_node = this->find_node(T_PC::start());
    //ASSERT(start_node);
    //state const &start_state = this->get_start_state();
    //start_state.get_names(names);
    //names.sort();
    //for (auto const &name : names) {
    //  if (   name.find(string(".") + g_regular_reg_name) != string::npos
    //      || name.find(g_regular_reg_name) == 0) {
    //    graph_locs_add_location_reg(new_locs, name, old_locs);
    //  }
    //}
    map<string, sort_ref> reg_names;
    for (auto const& edge : this->get_edges()) {
      function<void (string const&, expr_ref)> func =
        [&reg_names](string const& name, expr_ref const& e) -> void {
          if (   name.find(string(".") + g_regular_reg_name) != string::npos
              || name.find(g_regular_reg_name) == 0) {
            reg_names.insert(make_pair(name, e->get_sort()));
          }
        };
      edge->visit_exprs_const(func);
    }
    for (auto const& name_e : reg_names) {
      graph_locs_add_location_reg(new_locs, name_e.first, name_e.second, old_locs);
    }
  }

  void graph_locs_add_location_memmasked(map<graph_loc_id_t, graph_cp_location> &new_locs, string const &memname, memlabel_ref const& ml, expr_ref addr, size_t memsize, map<graph_loc_id_t, graph_cp_location> const &old_locs)
  {
    autostop_timer func_timer(__func__);
    graph_cp_location newloc;
    newloc.m_type = GRAPH_CP_LOCATION_TYPE_MEMMASKED;
    newloc.m_memname = mk_string_ref(memname);
    newloc.m_reg_type = reg_type_memory;
    newloc.m_reg_index_i = -1;
    newloc.m_reg_index_j = -1;
    newloc.m_memlabel = ml;
    newloc.m_addr = addr;
    newloc.m_nbytes = memsize;

    graph_add_new_loc(new_locs, newloc, old_locs);
  }

  string graph_locs_to_string() const
  {
    stringstream ss;
    map<graph_loc_id_t, graph_cp_location> const &graph_locs = *m_locs;
    for (map<graph_loc_id_t, graph_cp_location>::const_iterator iter = graph_locs.begin();
        iter != graph_locs.end();
        iter++) {
      ss << "  LOC" << iter->first << ". " << iter->second.to_string() << endl;
    }
    return ss.str();
  }

  void graph_add_location_slots_and_memmasked_using_llvm_locs(map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &llvm_locs)
  {
    for (const auto &l : llvm_locs) {
      if (   l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED
          || l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
        graph_cp_location newloc = l.second;
        string memname = newloc.m_memname->get_str();
        string_replace(memname, G_SRC_KEYWORD "." G_LLVM_PREFIX "-", G_DST_PREFIX ".");
        newloc.m_memname = mk_string_ref(memname);
        if (l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
          newloc.m_addr = expr_rename_symbols_to_dst_symbol_addrs(newloc.m_addr);
        }
        this->graph_add_loc(new_locs, l.first, newloc);
      }
    }
  }

  void graph_add_location_slots_using_state_mem_acc_map(map<graph_loc_id_t, graph_cp_location> &new_locs, map<T_PC, set<eqspace::state::mem_access>> const &state_mem_acc_map, map<graph_loc_id_t, graph_cp_location> const &old_locs)
  {
    autostop_timer func_timer(__func__);
    list<pair<string, pair<expr_ref, pair<unsigned, bool>>>> ret;
    list<shared_ptr<T_N>> nodes;
    this->get_nodes(nodes);
    for (auto n : nodes) {
      T_PC const &p = n->get_pc();
      set<eqspace::state::mem_access> const &mem_acc = state_mem_acc_map.at(p);
      CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << ": printing memory accesses for " << p.to_string() << endl);
      for (auto const& ma : mem_acc) {
        expr_ref addr = ma.get_address();
        const auto &sprel_map = this->get_sprels().at(p);
        auto const& memlabel_map = this->get_memlabel_map();
        //expr::operation_kind op = ma.get_operation_kind();
        CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << " " << p.to_string() << ": looking at mem_acc " << expr_string(ma.get_expr()) << endl);
        CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << " " << p.to_string() << ": looking at mem_acc addr " << expr_string(addr) << endl);
        if (ma.is_select() || ma.is_store()) {
          vector<expr_ref> addr_leaves = this->get_context()->expr_get_ite_leaves(addr);
          for (auto addr_leaf : addr_leaves) {
            CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << " " << p.to_string() << ": addr_leaf " << expr_string(addr_leaf) << endl);
            expr_ref addr_leaf_simplified = this->get_context()->expr_simplify_using_sprel_and_memlabel_maps(addr_leaf, sprel_map, memlabel_map);
            CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << " " << p.to_string() << ": addr_leaf_simplified " << expr_string(addr_leaf_simplified) << endl);
            if (this->get_context()->expr_contains_only_consts_struct_constants_or_arguments_or_esp_versions(addr_leaf_simplified, this->get_argument_regs())) {
              CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << " " << p.to_string() << ": addr_leaf_simplified " << expr_string(addr_leaf_simplified) << " contains only consts_struct constants" << endl);
              CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << " " << p.to_string() << ": expr " << ma.to_string() << endl);
              string memname = ma.get_memname();
              CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << ": memname = " << memname << endl);
              pair<string, pair<expr_ref, pair<unsigned, bool>>> pp = make_pair(memname, make_pair(addr_leaf_simplified, make_pair(ma.get_nbytes(), ma.get_bigendian())));
              ret.push_back(pp);
            }
          }
        }
      }
    }
    CPP_DBG_EXEC2(TFG_LOCS, cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl);
    ret.sort(mem_accesses_cmp);
    CPP_DBG_EXEC2(TFG_LOCS,
      cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl;
      int i = 0;
      for (auto iter = ret.begin(); iter != ret.end(); iter++, i++) {
        cout << "mem_acc[" << i << "] = addr " << expr_string(iter->second.first) << ", nbytes " << iter->second.second.first << endl;
      }
    );
    ret.unique(mem_accesses_eq);
    DYN_DEBUG2(tfg_locs,
      cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl;
      int i = 0;
      for (auto iter = ret.begin(); iter != ret.end(); iter++, i++) {
        cout << "mem_acc[" << i << "] = addr " << expr_string(iter->second.first) << ", nbytes " << iter->second.second.first << endl;
      }
    );

    for (list<pair<string, pair<expr_ref, pair<unsigned, bool>>>>::const_iterator iter = ret.begin();
         iter != ret.end();
         iter++) {
      this->graph_add_loc(new_locs, iter->first, iter->second.first, iter->second.second.first, iter->second.second.second, old_locs);
    }
  }

  void graph_locs_remove_duplicates_mod_comments_for_pc(map<graph_loc_id_t, graph_cp_location> const &graph_locs, map<graph_loc_id_t, bool> &is_redundant)
  {
    //autostop_timer func_timer(__func__);
    for (auto graph_loc : graph_locs) {
      is_redundant[graph_loc.first] = false;
    }
    for (typename map<graph_loc_id_t, graph_cp_location>::const_iterator iter = graph_locs.begin();
         iter != graph_locs.end();
         iter++) {
      if (is_redundant.at(iter->first)) {
        continue;
      }
      typename map<graph_loc_id_t, graph_cp_location>::const_iterator iter2 = iter;
      for (iter2++; iter2 != graph_locs.end(); iter2++) {
        if (is_redundant.at(iter2->first)) {
          continue;
        }
        if (iter->second.equals_mod_comment(iter2->second)) {
          //cout << __func__ << " " << __LINE__ << ": EQUAL" << endl;
          is_redundant[iter2->first] = true;
        } else {
          //cout << __func__ << " " << __LINE__ << ": NOT EQUAL" << endl;
        }
      }
    }
  }

  void graph_locs_remove_duplicates_mod_comments(map<graph_loc_id_t, graph_cp_location> &locs_map);

  void populate_locid2expr_map()
  {
    autostop_timer func_timer(__func__);
    m_locid2expr_map.clear();
    m_exprid2locid_map.clear();
    for (auto const& loc : this->get_locs()) {
      expr_ref e = this->get_loc_expr(loc.first);
      DYN_DEBUG(tfg_locs, cout << __func__ << " " << __LINE__ << ": loc-id " << loc.first << ": " << expr_string(e) << endl);
      m_locid2expr_map[loc.first] = e;
      m_exprid2locid_map[e->get_id()] = loc.first;
    }
  }

  void populate_loc_liveness();
  virtual void populate_loc_definedness() = 0;
  virtual void populate_branch_affecting_locs() = 0;

  set<graph_loc_id_t> const& get_branch_affecting_locs_at_pc(T_PC const& pp) const
  {
    if (!m_bavlocs.count(pp)) {
      static set<graph_loc_id_t> empty;
      return empty;
    }
    return m_bavlocs.at(pp);
  }

  expr_ref graph_loc_get_value(T_PC const &p, state const &s, graph_loc_id_t loc_index) const
  {
    stopwatch_run(&graph_loc_get_value_timer);
    context *ctx = this->get_context();

    graph_cp_location l;
    expr_ref ret;

    map<graph_loc_id_t, graph_cp_location> const &graph_locs = *m_locs;
    ASSERT(graph_locs.count(loc_index) > 0);
    l = graph_locs.at(loc_index);

    expr_ref memvar;
    bool bret;
    bret = this->get_start_state().get_memory_expr(this->get_start_state(), memvar);
    ASSERT(bret);

    if (l.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
      expr_ref m;

      m = s.get_expr_for_input_expr(l.m_memname->get_str(), memvar);
      expr_ref addr = l.m_addr;
      addr = expr_substitute_using_state(l.m_addr/*, this->get_start_state()*/, s);
      ASSERT(!l.m_bigendian);
      ret = ctx->mk_select(m, l.m_memlabel->get_ml(), addr, l.m_nbytes, l.m_bigendian/*, l.m_comment*/);
    } else if (l.m_type == GRAPH_CP_LOCATION_TYPE_LLVMVAR) {
      ret = s.get_expr_for_input_expr(l.m_varname->get_str(), l.m_var);
    //} else if (l.m_type == GRAPH_CP_LOCATION_TYPE_IO) {
    //  expr_ref io_expr = l.m_addr;
    //  io_expr = expr_substitute_using_states(l.m_addr, this->get_start_state(), s);
    //  ret = io_expr;
    } else if (l.m_type == GRAPH_CP_LOCATION_TYPE_REGMEM) {
      //ret = s.get_expr(l.m_varname->get_str(), this->get_start_state());
      ret = s.get_expr_for_input_expr(l.m_varname->get_str(), l.m_var);
    } else if (l.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
      context *ctx;
      expr_ref m;
      //m = s.get_expr(l.m_memname->get_str(), this->get_start_state());
      m = s.get_expr_for_input_expr(l.m_memname->get_str(), memvar);
      ctx = m->get_context();
      expr_ref addr;
      ASSERT(memlabel_t::memlabel_is_atomic(&l.m_memlabel->get_ml()));
      ret = ctx->mk_memmask(m, l.m_memlabel->get_ml());
    }
    stopwatch_stop(&graph_loc_get_value_timer);
    return ret;
  }

  expr_ref expr_simplify_at_pc(expr_ref const& e, T_PC const& p) const
  {
    sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(p);
    auto const& memlabel_map = this->get_memlabel_map();
    return this->get_context()->expr_simplify_using_sprel_pair_and_memlabel_maps(e, sprel_map_pair, memlabel_map);
  }

  size_t get_num_graph_locs() const { return m_locs->size(); }

  static bool sprels_are_different(map<T_PC, sprel_map_t> const &sprels1, map<T_PC, sprel_map_t> const &sprels2)
  {
    if (sprels1.size() != sprels2.size()) {
      DYN_DEBUG(sprels, cout << _FNLN_ << ": returning true because sprels1.size() = " << sprels1.size() << " =/= sprels2.size() = " << sprels2.size() << endl);
      return true;
    }
    for (const auto &s1 : sprels1) {
      if (sprels2.count(s1.first) == 0) {
        DYN_DEBUG(sprels, cout << __func__ << " " << __LINE__ << ": returning true because s2.count(" << s1.first.to_string() << ") == 0\n");
        return true;
      }
      if (!s1.second.equals(sprels2.at(s1.first))) {
        DYN_DEBUG(sprels, cout << __func__ << " " << __LINE__ << ": returning true because sprel_maps not equal at " << s1.first.to_string() << "\n");
        DYN_DEBUG2(sprels, cout << _FNLN_ << ": s1.second =\n" << s1.second.to_string() << "s2.second =\n" << sprels2.at(s1.first).to_string() << endl);
        return true;
      }
    }
    return false;
  }

  virtual map<T_PC, sprel_map_t> compute_sprel_relations() const { return {}; }

  void print_sp_relations(map<T_PC, sprel_map_t> const &sprel_map, consts_struct_t const &cs) const
  {
    cout << "Printing sp relations" << endl;
    for (typename map<T_PC, sprel_map_t>::const_iterator iter = sprel_map.begin();
        iter != sprel_map.end();
        iter++) {
      T_PC p = iter->first;
      stringstream ss;
      string s;
      ss << "pc " << p.to_string();
      s = ss.str();
      sprel_map_t const &sprel = iter->second;
      string sprel_status_string;
      sprel_status_string = sprel.to_string(this->get_locs()/*.at(p)*/, cs, s);
      if (sprel_status_string != "") {
        cout << sprel_status_string << endl;
      }
    }
    cout << "done Printing sp relations" << endl;
  }

  virtual bool propagate_sprels()
  {
    autostop_timer ftimer(__func__);
    consts_struct_t const &cs = this->get_consts_struct();
    map<T_PC, sprel_map_t> old_sprels = m_sprels;
    m_sprels = this->compute_sprel_relations();
    DYN_DEBUG(sprels, cout << __func__ << " " << __LINE__ << ": sp relations used to substitute:\n"; print_sp_relations(m_sprels, cs));

    //XXX: the following loop is not really required; should get rid of this at some point
    //for (const auto &pc_sprel_map : old_sprels) {
    //  if (!m_sprels.count(pc_sprel_map.first)) {
    //    m_sprels.insert(pc_sprel_map);
    //  } else {
    //    m_sprels.at(pc_sprel_map.first).sprel_map_union(pc_sprel_map.second);
    //  }
    //}
    bool changed = this->sprels_are_different(old_sprels, m_sprels);
    return changed;
  }

  bool loc_has_sprel_mapping_at_pc(T_PC const &p, graph_loc_id_t loc_id) const { return m_sprels.at(p).has_mapping_for_loc(loc_id); }

  void init_graph_locs(graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs);

  set<graph_loc_id_t> const &get_live_locids(T_PC const &p) const
  {
    static set<graph_loc_id_t> empty_set = set<graph_loc_id_t>();
    if (m_loc_liveness.count(p) == 0) {
      return empty_set;
    }
    return m_loc_liveness.at(p);
  }

  map<graph_loc_id_t, graph_cp_location> get_live_locs(T_PC const &p) const
  {
    map<graph_loc_id_t, graph_cp_location> ret;
    if (m_loc_liveness.count(p) == 0) {
      return ret;
    }

    for (auto const& loc : this->get_locs()) {
      if (loc_is_live_at_pc(p, loc.first))
        ret.insert(loc);
    }
    return ret;
  }

  void add_live_locs(T_PC const &p, set<graph_loc_id_t> const &locs, graph_loc_id_t start_loc_offset = 0)
  {
    for (auto l : locs) {
      graph_loc_id_t el =  l + start_loc_offset;
      if (m_locs->count(el)) {
        m_loc_liveness[p].insert(el);
      }
    }
  }

  void add_live_loc(T_PC const &p, graph_loc_id_t loc)
  {
    m_loc_liveness[p].insert(loc);
  }

  void print_loc_liveness() const
  {
    cout << __func__ << " " << __LINE__ << ": Printing loc liveness:\n";
    for (const auto &pls : m_loc_liveness) {
      T_PC const &p = pls.first;
      cout << p.to_string() << ": " << loc_set_to_string(pls.second) << endl;
    }
  }

  void print_loc_definedness() const
  {
    cout << __func__ << " " << __LINE__ << ": TFG:\n" << this->graph_to_string(/*true*/) << endl;
    cout << __func__ << " " << __LINE__ << ": Printing loc definedness:\n";
    for (const auto &pls : m_loc_definedness) {
      T_PC const &p = pls.first;
      cout << p.to_string() << ": " << loc_set_to_string(pls.second) << endl;
    }
  }

  static string loc_set_to_expr_string(graph_with_locs<T_PC,T_N,T_E,T_PRED> const& g, set<graph_loc_id_t> const &loc_set)
  {
    stringstream ss;
    bool first = true;
    for (auto loc_id : loc_set) {
      if (!first) {
        ss << ", ";
      }
      ss << expr_string(g.graph_loc_get_value_for_node(loc_id));
      first = false;
    }
    return ss.str();
  }

  void print_branch_affecting_locs() const
  {
    cout << __func__ << " " << __LINE__ << ": Printing branch affecting vars:\n";
    for (const auto &pls : m_bavlocs) {
      T_PC const &p = pls.first;
      cout << p.to_string() << ": " << loc_set_to_string(pls.second) << "[ " << loc_set_to_expr_string(*this, pls.second) << " ]" << endl;
    }
  }

  void clear_loc_liveness() { m_loc_liveness.clear(); }
  map<T_PC, set<graph_loc_id_t>> const &get_loc_liveness() const { return m_loc_liveness; }
  map<T_PC, set<graph_loc_id_t>> const &get_loc_definedness() const { return m_loc_definedness; }

  bool loc_is_live_at_pc(T_PC const &p, graph_loc_id_t loc_id) const { return m_loc_liveness.at(p).count(loc_id) != 0; }
  bool loc_is_defined_at_pc(T_PC const &p, graph_loc_id_t loc_id) const { return m_loc_definedness.at(p).count(loc_id) != 0; }


  void graph_init_sprels()
  {
    for (const auto &p : this->get_all_pcs()) {
      if (!this->m_sprels.count(p)) {
        this->m_sprels.insert(make_pair(p, sprel_map_t(m_locid2expr_map, this->get_consts_struct())));
      }
    }
  }

  map<graph_loc_id_t, expr_ref> const &get_locid2expr_map() const { return m_locid2expr_map; }
  map<T_PC, sprel_map_t> const &get_sprels() const { return m_sprels; }

  sprel_map_t const &get_sprel_map(T_PC const &p) const { return this->get_sprels().at(p); }

  void add_sprel_mapping(T_PC const &p, graph_loc_id_t loc_id, expr_ref const &e)
  {
    if (!m_sprels.count(p)) {
      m_sprels.insert(make_pair(p, sprel_map_t(m_locid2expr_map, this->get_consts_struct())));
    }
    m_sprels.at(p).sprel_map_add_constant_mapping(loc_id, e);
  }

  graph_loc_id_t get_loc_id_for_masked_mem_expr(expr_ref const &e) const
  {
    for (auto const& loc : *m_locs) {
      if (loc.second.m_type != GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
        continue;
      }
      //expr_ref le = this->get_loc_expr(loc.first);
      expr_ref le = this->graph_loc_get_value_for_node(loc.first);
      //cout << __func__ << " " << __LINE__ << ": le = " << expr_string(le) << endl;
      if (e == le) {
        return loc.first;
      }
    }
    cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
    NOT_REACHED();
  }

  string memlabel_map_to_string_for_eq() const
  {
    graph_memlabel_map_t const &memlabel_map = this->get_memlabel_map();
    return memlabel_map_to_string(memlabel_map);
  }

  set<graph_loc_id_t> get_argument_regs_locids() const
  {
    set<graph_loc_id_t> ret;
    set<expr_ref> arg_reg_exprs;
    for (auto const& pse : this->get_argument_regs().get_map()) {
      arg_reg_exprs.insert(pse.second);
    }
    for (auto const& loc : this->get_locs()) {
      expr_ref loc_expr = this->graph_loc_get_value_for_node(loc.first);
      if (arg_reg_exprs.count(loc_expr)) {
        ret.insert(loc.first);
      }
    }
    return ret;
  }

  set<graph_loc_id_t> get_return_regs_locids() const
  {
    set<graph_loc_id_t> ret;
    set<expr_ref> ret_reg_exprs;
    for (auto const& p_pse : this->get_return_regs()) {
      for (auto const& pse : p_pse.second) {
        if (pse.second->get_operation_kind() == expr::OP_SELECT) {
          // XXX special handling of select-on-symbol
          // add the memmask instead of select since loc is formed for memmask only
          ret_reg_exprs.insert(pse.second->get_args().at(0));
        }
        ret_reg_exprs.insert(pse.second);
      }
    }
    for (auto const& loc : this->get_locs()) {
      expr_ref loc_expr = this->graph_loc_get_value_for_node(loc.first);
      if (ret_reg_exprs.count(loc_expr)) {
        ret.insert(loc.first);
      }
    }
    return ret;
  }

  graph_loc_id_t get_ret_reg_locid() const
  {
    for (auto const& loc : this->get_locs()) {
      if (   loc.second.is_var()
          && loc.second.to_string().find(G_LLVM_RETURN_REGISTER_NAME) != string::npos) {
        return loc.first;
      }
    }
    return -1;
  }

  void expand_locset_to_include_all_memmasks(set<graph_loc_id_t> &locset) const
  {
    for (const auto &loc : *m_locs) {
      if (loc.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
        locset.insert(loc.first);
      }
    }
  }

  static void relevant_memlabels_add(vector<memlabel_ref> &relevant_memlabels, memlabel_ref const &ml_input)
  {
    bool ml_input_exists = false;
    for (auto ml : relevant_memlabels) {
      if (ml == ml_input) {
        ml_input_exists = true;
      }
    }
    if (!ml_input_exists) {
      relevant_memlabels.push_back(ml_input);
    }
  }


  void graph_get_relevant_memlabels(vector<memlabel_ref> &relevant_memlabels) const
  {
    graph_get_relevant_memlabels_except_args(relevant_memlabels);

    for (int i = 0; i < (int)this->get_argument_regs().size(); i++) {
      relevant_memlabels_add(relevant_memlabels, mk_memlabel_ref(memlabel_t::memlabel_arg(i)));
    }
  }

  void graph_get_relevant_memlabels_except_args(vector<memlabel_ref> &relevant_memlabels) const
  {
    //autostop_timer func_timer(__func__);

    relevant_memlabels_add(relevant_memlabels, mk_memlabel_ref(memlabel_t::memlabel_heap()));
    relevant_memlabels_add(relevant_memlabels, mk_memlabel_ref(memlabel_t::memlabel_stack()));

    for (auto const& symbol : this->get_symbol_map().get_map()) {
      memlabel_t ml = memlabel_t::memlabel_symbol(symbol.first/*, symbol.second.get_size()*/, symbol.second.is_const());
      relevant_memlabels_add(relevant_memlabels, mk_memlabel_ref(ml));
    }

    for (auto const& local : this->get_locals_map().get_map()) {
      memlabel_t ml = memlabel_t::memlabel_local(local.first/*, this->get_locals_map().at(i).second*/);
      relevant_memlabels_add(relevant_memlabels, mk_memlabel_ref(ml));
    }
  }

  void expand_locset_to_include_slots_for_memmask(set<graph_loc_id_t> &locset) const
  {
    set<memlabel_ref> mls;
    for (auto loc_id : locset) {
      if (m_locs->at(loc_id).m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
        mls.insert(m_locs->at(loc_id).m_memlabel);
      }
    }
    if (mls.empty())
      return;

    memlabel_t mlu = memlabel_t::memlabel_union(mls);
    for (const auto &loc : *m_locs) {
      if (loc.second.m_type != GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
        continue;
      }
      if (!memlabel_t::memlabel_intersection_is_empty(mlu, loc.second.m_memlabel->get_ml())) {
       locset.insert(loc.first);
      }
    }
  }

  set<graph_loc_id_t> get_locs_potentially_read_in_expr(expr_ref const &e) const
  {
    set<graph_loc_id_t> ret = this->get_locs_potentially_read_in_expr_using_locs_map(e, *m_locs);
    if (this->get_context()->expr_contains_memlabel_top(e)) {
      expand_locset_to_include_all_memmasks(ret);
    }
    expand_locset_to_include_slots_for_memmask(ret);
    return ret;
  }

  bool expr_indicates_definite_write(graph_loc_id_t loc_id, expr_ref const &new_expr) const
  {
    if (m_locs->at(loc_id).m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
      return false;
    }
    if (m_locs->at(loc_id).m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
      expr_find_op expr_stores_visitor(new_expr, expr::OP_STORE);
      expr_vector expr_stores = expr_stores_visitor.get_matched_expr();
      return expr_stores.empty(); // can make this even more precise by looking at target addr of store; if the target addr is different from slot's addr then return true
      //return false; //can make this more precise by looking at new_expr; e.g., if new_expr does not contain a store, we can return true
    }
    return true;
  }

  /* for CG we may want union of both src and dst */
  virtual sprel_map_pair_t get_sprel_map_pair(T_PC const &p) const = 0;

  //set<graph_loc_id_t> graph_get_locs_potentially_belonging_to_memlabel(memlabel_t const &ml)
  //{
  //  set<graph_loc_id_t> ret;
  //  for (const auto &l : *this->m_locs) {
  //    if (   (   l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED
  //            && (ml.memlabel_is_top() || memlabel_contains(&ml, &l.second.m_memlabel->get_ml())))
  //        || (   l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT
  //            && (ml.memlabel_is_top() || memlabel_contains(&ml, &l.second.m_memlabel->get_ml())))) {
  //      ret.insert(l.first);
  //    }
  //  }
  //  return ret;
  //}

  graph_loc_id_t graph_expr_to_locid(expr_ref const &e) const
  {
    if (m_exprid2locid_map.count(e->get_id())) {
      return m_exprid2locid_map.at(e->get_id());
    } else {
      return GRAPH_LOC_ID_INVALID;
    }
  }

  virtual set<expr_ref> get_live_vars(T_PC const& pp) const
  {
    set<expr_ref> ret;
    for (auto const& loc : this->get_live_locs(pp)) {
      if (   loc.second.is_var()
          || loc.second.is_reg())
        ret.insert(this->get_loc_expr(loc.first));
    }
    return ret;
  }

  set<expr_ref> get_live_loc_exprs_at_pc(T_PC const& p) const;
  set<expr_ref> get_spreled_loc_exprs_at_pc(T_PC const& p) const;
  //set<expr_ref> get_live_loc_exprs_having_sprel_map_at_pc(T_PC const &p) const;
  //map<graph_loc_id_t, graph_cp_location> get_live_locs_across_pcs(T_PC const& from_pc, T_PC const& to_pc) const;

protected:

  void set_remaining_sprel_mappings_to_bottom(T_PC const &p)
  {
    if (!m_sprels.count(p)) {
      m_sprels.insert(make_pair(p, sprel_map_t(m_locid2expr_map, this->get_consts_struct())));
    }
    m_sprels.at(p).sprel_map_set_remaining_mappings_to_bottom(*m_locs);
  }

  virtual void graph_to_stream(ostream& ss) const override;
  string read_locs(istream& in, string line);

  void set_loc_liveness(map<T_PC, set<graph_loc_id_t>> const& loc_liveness) { m_loc_liveness = loc_liveness; }
  void set_loc_definedness(map<T_PC, set<graph_loc_id_t>> const& loc_definedness) { m_loc_definedness = loc_definedness; }
  void set_bavlocs(map<T_PC, set<graph_loc_id_t>> const& bavlocs) { m_bavlocs = bavlocs; }
  virtual bool populate_auxilliary_structures_dependent_on_locs() = 0;
  virtual void graph_init_memlabels_to_top(bool update_callee_memlabels) = 0;

private:
  void collect_memory_accesses(set<T_PC> const &nodes_changed, map<T_PC, set<eqspace::state::mem_access>> &state_mem_acc_map);
  void refresh_graph_locs(map<graph_loc_id_t, graph_cp_location> const &llvm_locs);

  set<graph_loc_id_t> get_locs_potentially_read_in_expr_using_locs_map(expr_ref const &e, map<graph_loc_id_t, graph_cp_location> const &locs) const;

  string read_loc(istream& in, graph_cp_location &loc) const;
  set<T_PC> get_nodes_where_live_locs_added(map<T_PC, set<graph_loc_id_t>> const &orig, map<T_PC, set<graph_loc_id_t>> const &cur);
  static bool loc_is_atomic(graph_cp_location const &loc);
  void eliminate_unused_loc_in_start_state_and_all_edges(graph_cp_location const &loc);
  void graph_locs_visit_exprs(std::function<expr_ref (T_PC const &, const string&, expr_ref)> f, map<graph_loc_id_t, graph_cp_location> &locs_map);
  //template<typename T_MAP_VALUETYPE>
  //static void edgeloc_map_eliminate_unused_loc(map<pair<edge_id_t<T_PC>, graph_loc_id_t>, T_MAP_VALUETYPE> &elmap, graph_loc_id_t loc_id);
  static bool expr_is_possible_loc(expr_ref e);

private:

  map<T_PC, set<graph_loc_id_t>> m_loc_liveness;     //for each PC, the set of live locs
  map<T_PC, set<graph_loc_id_t>> m_loc_definedness;  //for each PC, the set of defined locs
  map<T_PC, set<graph_loc_id_t>> m_bavlocs;          //for each PC, the set of branch affecting (variables) locs

  shared_ptr<map<graph_loc_id_t, graph_cp_location> const> m_locs;
  map<T_PC, sprel_map_t> m_sprels;
  map<graph_loc_id_t, expr_ref>  m_locid2expr_map;
  map<expr_id_t, graph_loc_id_t> m_exprid2locid_map;
};

}
