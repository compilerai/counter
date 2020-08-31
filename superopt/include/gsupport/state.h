#pragma once

#include "expr/expr.h"
#include "expr/consts_struct.h"
#include "expr/memlabel.h"
#include "expr/memlabel_map.h"
#include "gsupport/pc.h"
//#include "graph/lr_map.h"
#include <vector>
#include <list>
#include "support/debug.h"
//#include "rewrite/regmap.h"
//#include "rewrite/symbol_map.h"
#include "i386/regs.h"
#include "ppc/regs.h"
#include "graph/state_submap.h"
#include "gsupport/fsignature.h"

class regmap_t;
class reg_identifier_t;
namespace eqspace {
class tfg;
class sprel_map_t;
class sym_exec;

using namespace std;

class slot_mem_addr;

class state_value_expr_map_t
{
private:
  map<string_ref, expr_ref> m_value_expr_map;
  map<string, expr_ref> m_deterministic_value_expr_map;
public:
  void set_map(map<string_ref, expr_ref> const& m)
  {
    m_value_expr_map = m;
    m_deterministic_value_expr_map = determinize_value_expr_map(m); 
  }
  void insert(pair<string_ref, expr_ref> const& p)
  {
    //m_value_expr_map.insert(p);
    //m_deterministic_value_expr_map.insert(make_pair(p.first->get_str(), p.second));
    m_value_expr_map[p.first] = p.second;
    m_deterministic_value_expr_map[p.first->get_str()] = p.second;
  }
  map<string_ref, expr_ref> const& get_map() const
  {
    return m_value_expr_map;
  }
  map<string, expr_ref> const& get_det_map() const
  {
    return m_deterministic_value_expr_map;
  }
  void clear()
  {
    m_value_expr_map.clear();
    m_deterministic_value_expr_map.clear();
  }
  void erase(string_ref const& k)
  {
    m_value_expr_map.erase(k);
    m_deterministic_value_expr_map.erase(k->get_str());
  }
private:
  static map<string, expr_ref> determinize_value_expr_map(map<string_ref, expr_ref> const& value_expr_map);
};

class state
{
public:
  class mem_access
  {
  public:
    mem_access(string const &comment, expr_ref data, bool is_farg) : m_comment(mk_string_ref(comment)), m_data(data), m_is_farg(is_farg) { }
    bool is_select() const;
    bool is_store() const;
    bool is_farg() const;
    expr_ref const &get_expr() const { return m_data; }
    expr_ref const &get_address() const;
    expr_ref const &get_store_data() const;
    string const &get_comment() const { return m_comment->get_str(); }
    //expr::operation_kind get_operation_kind() const;
    //expr::operation_kind ma_get_operation_kind() const;
    string get_memname() const;
    memlabel_t get_memlabel() const;
    unsigned get_nbytes() const;
    bool get_bigendian() const;
    //expr_ref get_expr() const;
    //void set_expr(expr_ref e);
    bool operator<(mem_access const &other) const
    {
      if (m_data < other.m_data) {
        return true;
      }
      if (other.m_data < m_data) {
        return false;
      }
      if (!m_is_farg && other.m_is_farg) {
        return true;
      }
      return false;
    }
    bool operator==(mem_access const& other) const
    {
      return    m_data == other.m_data
             && m_is_farg == other.m_is_farg
      ;
    }

    string to_string() const;

  private:
    string_ref m_comment;
    expr_ref m_data;
    bool m_is_farg;
  };


  void clear() { m_value_exprs.clear();/* m_num_exreg_groups = 0; m_num_exregs.clear(); m_num_invisible_regs = 0; m_num_hidden_regs = 0; */}
  void state_set_memlabels_to_top_or_constant_values(map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &symbol_map, graph_locals_map_t const &locals_map/*, set<cs_addr_ref_id_t> const &relevant_addr_refs*/, consts_struct_t const &cs, state const &start_state, bool update_callee_memlabels, sprel_map_t const *sprel_map, map<graph_loc_id_t, expr_ref> const &locid2expr_map, string const &graph_name, long &mlvarnum, graph_memlabel_map_t &memlabel_map) const;

  void set_value_expr_map(map<string_ref, expr_ref> const &m) {  m_value_exprs.set_map(m); }
  //map<string, expr_ref> get_value_expr_map(void) const {  return m_value_expr_map; }
  map<string_ref, expr_ref> const &get_value_expr_map_ref(void) const {  return m_value_exprs.get_map(); }

  bool has_expr(const string& val_name) const;
  string has_expr_substr(const string& val_name) const;
  expr_ref get_expr(const string& val_name, state start_state = state()) const;
  expr_ref get_expr_for_input_expr(const string& val_name, expr_ref const& input_expr) const;
  bool key_exists(const string& key_name/*, state const &start_state*/) const;
  void remove_key(string const &val_name);

  void set_memory_expr(expr_ref val);
  //bool get_memory_expr(state const &start_state, expr_ref &ret) const;
  vector<string> get_memnames() const;
  string get_memname() const;
  bool get_memory_expr(state const &start_state, expr_ref &ret) const;
  vector<expr_ref> get_memory_exprs(state const &start_state) const;

  //string to_string() const;
  string to_string(/*const state& relative, */const string& sep = "\n") const;
  string to_string_for_eq() const;

  void state_substitute_variables(/*const state& from, */const state& to);
  //bool substitute_variables_using_submap_slow(state const &start_state, map<expr_id_t, pair<expr_ref, expr_ref>> const &map);
  bool substitute_variables_using_state_submap(/*state const &start_state, */state_submap_t const &state_submap);
  void state_simplify();
  void merge_state(expr_ref pred, state const &st_in/*, state const &start_state*/);

  static string read_state(istream& in, map<string_ref, expr_ref> & st_value_expr_map, context* ctx/*, state const *start_state*/);

  //void get_substitution_vars(vector<expr_ref>& vars) const;

  static std::function<bool (pair<string_ref, expr_ref> const&, pair<string_ref, expr_ref> const&)> value_expr_map_deterministic_sort_fn();
  bool state_visit_exprs_for_key(std::function<expr_ref (const string&, expr_ref)> f/*, state const &start_state*/, string const &key);
  bool state_visit_exprs(std::function<expr_ref (const string&, expr_ref)> f);
  void state_visit_exprs(std::function<void (const string&, expr_ref)> f) const;
  bool state_visit_exprs_with_start_state(std::function<expr_ref (const string&, expr_ref)> f/*, state const& start_state*/);
  void state_visit_exprs_with_start_state(std::function<void (const string&, expr_ref)> f/*, state const& start_state*/) const;

  void get_regs(list<expr_ref>& regs) const;
  void get_regs(vector<expr_ref>& regs) const;
  void get_names(list<string>& names) const;

  void get_fun_calls(list<expr_ref>& calls) const;
  void get_mem_selects_and_stores(list<expr_ref>& es) const;
  //void get_mem_slots(list<tuple<expr_ref, memlabel_t, expr_ref, unsigned, bool>>& addrs) const;
  //void get_reg_slots(set<string>& names) const;
  //void get_fcall_retregs(set<string>& names) const;

  //string get_name_from_expr(expr_ref e) const;
  //void state_transform_function_arguments_to_avoid_stack_pointers(consts_struct_t const &cs);
  void state_rename_locals_to_stack_pointer(consts_struct_t const &cs);
  //void state_rename_farg_ptr_memlabel_top_to_notstack(consts_struct_t const &cs);
  //void state_substitute_rodata_reads(consts_struct_t const &cs);
  //void state_set_memlabels_to_top(map<symbol_id_t, pair<string, size_t>> const &symbol_map, graph_locals_map_t const &locals_map, consts_struct_t const &cs);

  //void populate_mem_reads_writes(expr_ref esp, expr_ref pred);
  //void find_memory_exprs(expr_ref e, expr_ref esp, expr::operation_kind k, map<expr_id_t, expr_ref> &found_es);
  static void add_to_mem_reads_write(set<expr_ref, expr_deterministic_cmp_t> const &found_es, list<state::mem_access>& mas);

  //void state_substitute_using_sprels(tfg const *tfg, pc const &from_pc, sprel_map_t const &sprels, map<unsigned, expr_ref> &sprels_subcache, eq::consts_struct_t const &cs, map<pair<pc, expr_id_t>, graph_loc_id_t> &expr_represents_loc_in_state_cache);

  //expr_ref convert_function_argument_ptr_to_select_expr(expr_ref e) const;
  //vector<expr_ref> get_mem_pools_except_stack_and_locals(tfg const *tfg) const;//, cspace::symbol_map_t const *symbol_map, eq::consts_struct_t const &cs) const;

  void reorder_registers_using_regmap(state const &from_state, struct regmap_t const &regmap, state const &start_state);
  void reorder_state_elements_using_transmap(struct transmap_t const *tmap, state const &start_state);

  //void state_annotate_locals(state const &instate, vector<pair<unsigned long, pair<string, unsigned long>>> const &symbol_vars, struct dwarf_map_t const *dwarf_map, struct eq::consts_struct_t const *cs, eq::context *ctx, sym_exec const &se, tfg const *tfg, pc const &from_pc, cspace::locals_info_t *locals_info, map<pc, sprel_map_t, pc_comp> const &sprels, map<pc, map<unsigned, expr_ref>, pc_comp> &sprels_subcache, map<pc, lr_map_t, pc_comp> const &linear_relations, map<pc_exprid_pair, int, pc_exprid_pair_comp> &expr_represents_loc_in_state_cache);

  //void append_pc_string_to_state_elems(pc const &pc, bool src);

  //state add_function_arguments(expr_vector const &function_arguments) const;

  /*bool split_mem(
    //lr_map_t const &lr_map,
    map<addr_id_t, lr_status_t> const &addr2status_map,
    //tfg const *tfg,
    //pc const &from_pc,
    state const &from_state,
    map<expr_id_t, addr_id_t> const &expr2addr_map,
    map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &symbol_map,
    graph_locals_map_t const &locals_map,
    set<cs_addr_ref_id_t> const &relevant_addr_refs,
    eq::consts_struct_t const &cs);*/

  void append_insn_id_to_comments(int insn_id);
  void set_comments_to_zero();

  static string reg_name(int i, int j) { return string(G_REGULAR_REG_NAME) + int_to_string(i) + "." + int_to_string(j); }

  state()
  : m_value_exprs()//,
    //m_num_exreg_groups(0),
    //m_num_exregs(),
    //m_num_invisible_regs(0),
    //m_num_hidden_regs(0)
  {}

  state(string prefix, context *ctx);

  bool get_fcall_expr(expr_ref& fcall_expr) const;

  vector<farg_t> state_get_fcall_retvals() const;
  tuple<nextpc_id_t, vector<farg_t>, mlvarname_t, mlvarname_t> get_nextpc_args_memlabel_map_for_edge(state const &start_state, context *ctx) const;
  state add_nextpc_retvals(vector<farg_t> const& fretvals, expr_ref const& dst_fcall_retval_buffer, expr_ref const& mem, expr_ref const& ml_callee_readable, expr_ref const& ml_callee_writeable, expr_ref const& nextpc, string const& dst_stackpointer_key, expr_ref const &stackpointer, state const &start_state, context *ctx, string const &graph_name, long &max_memlabel_varnum) const;
  state add_nextpc_args_and_retvals_for_edge(fsignature_t const& fsignature, string const& dst_stackpointer_key, expr_ref const &stackpointer/*, memlabel_t const &ml_esplocals*/, state const &start_state, context *ctx/*, graph_memlabel_map_t &memlabel_map*/, string const &graph_name, long &max_memlabel_varnum) const;

  void zero_out_dead_vars(set<string> const &live_regs);
  void get_live_vars_from_exprs(set<string> &vars, vector<expr_ref> const &live_exprs) const;

  //int get_num_exreg_groups() const { return m_num_exreg_groups; }
  //int get_num_exregs(int group) const { return m_num_exregs.at(group); }
  //int get_num_invisible_regs() const { return m_num_invisible_regs; }
  //int get_num_hidden_regs() const {
  //  int i;
  //  for (i = 0;; i++) {
  //    string regname = string(G_HIDDEN_REG_NAME) + int_to_string(i);
  //    if (m_value_exprs.get_map().count(mk_string_ref(regname)) == 0) {
  //      break;
  //    }
  //  }
  //  return i;
  //}
  //bool expr_value_exists(reg_type rt, int i = 0, int j = 0) const;
  expr_ref get_expr_value(state const &start_state, reg_type rt, int i = 0, int j = 0) const;
  //expr_ref get_invisible_reg(state const &start_state, int i) const { return get_expr(string(G_INVISIBLE_REG_NAME) + int_to_string(i), start_state); }
  //expr_ref get_hidden_reg(state const &start_state, int i) const { return get_expr(string(G_HIDDEN_REG_NAME) + int_to_string(i), start_state); }
  //void set_num_exreg_groups(int n) { m_num_exreg_groups = n; }
  //void set_num_exregs(size_t group, size_t n) {
  //  ASSERT(group <= m_num_exregs.size());
  //  if (group == m_num_exregs.size()) {
  //    m_num_exregs.push_back(n);
  //  } else {
  //    m_num_exregs[group] = n;
  //  }
  //}
  //void set_num_invisible_regs(int n) { m_num_invisible_regs = n; }
  //void set_num_hidden_regs(int n) { m_num_hidden_regs = n; }
  void set_expr_value(reg_type rt, int i, int j, expr_ref e, state const &start_state);
  vector<expr_ref> const get_all_function_calls() const;
  void apply_function_call(expr_ref nextpc_id, unsigned num_args, int r_esp, int r_eax, state const &start_state);
  void set_expr_in_map(const string& val_name, expr_ref val) { set_expr(val_name, val); }

  static void get_exreg_details_from_name(string name, int *group, int *regnum);
  static state_submap_t create_submap_from_value_expr_map(/*map<string_ref, expr_ref> const &from_value_expr_map, */map<string_ref, expr_ref> const &to_value_expr_map);
  static state_submap_t create_submap_from_state(/*state const& from_state, */state const& to_state);
  //void state_canonicalize_llvm_symbols(vector<string> &symbol_map);
  //void state_canonicalize_llvm_nextpcs(vector<string> &nextpc_map);
  expr_ref get_function_call_expr() const;
  bool contains_function_call(void) const;
  static bool is_exit_function(string const &fun_name);
  bool contains_exit_fcall(map<int, string> const &nextpc_map, int *nextpc_num) const;
  bool is_fcall_edge(nextpc_id_t &nextpc_num) const;
  void get_function_call_nextpc(expr_ref &ret) const;
  void get_function_call_memlabel_writeable(memlabel_t &ret, graph_memlabel_map_t const &memlabel_map) const;
  static state state_union(state const &a, state const &b);
  void debug_check(consts_struct_t const *cs) const;

  void intersect_and_update_callee_memlabels_at_fcalls(state const &st, graph_memlabel_map_t &memlabel_map, context *ctx) const;

  state state_minus(state const &other) const;
  void populate_reg_counts(/*state const *start_state*/);
  static void find_all_memory_exprs_for_all_ops(string comment, expr_ref e, set<state::mem_access>& mas);
  void state_rename_keys(map<string, string> const &key_map);
  void state_introduce_src_dst_prefix(char const *prefix, bool update_exprs);
  void state_truncate_register_sizes_using_regset(regset_t const &regset);
  void eliminate_hidden_regs();
  static bool key_represents_exreg_key(string const &k, exreg_group_id_t &group, reg_identifier_t &reg_id);

  vector<expr_ref> state_get_fcall_args() const;
  void replace_fcall_args(vector<expr_ref> const &from, vector<expr_ref> const &to);

  static bool states_equal(state const& a, state const& b)
  {
    return a.m_value_exprs.get_map() == b.m_value_exprs.get_map()
           /*&& a.m_num_exreg_groups == b.m_num_exreg_groups // don't need to compare these as they are derived structures
           && a.m_num_exregs == b.m_num_exregs
           && a.m_num_invisible_regs == b.m_num_invisible_regs
           && a.m_num_hidden_regs == b.m_num_hidden_regs*/
    ;
  }

private:
  static string get_reg_name(const string& line);
  //bool state_visit_exprs_with_start_state_helper(std::function<expr_ref (const string&, expr_ref)> f, state const& start_state);
  static void find_all_memory_exprs(string comment, expr_ref e, expr::operation_kind k, set<state::mem_access>& mas);
  void get_expr_with_op_kind(expr::operation_kind k, list<expr_ref>& es) const;
  expr_ref reg_apply_function_call(expr_ref nextpc_id, unsigned num_args, int r_esp, int reg_id);
  void set_expr(const string& val_name, expr_ref val);

private:
  //map<string_ref, expr_ref> m_value_expr_map;
  //map<string, expr_ref> m_determinized_value_expr_map;
  state_value_expr_map_t m_value_exprs;

  //int m_num_exreg_groups;
  //vector<int> m_num_exregs;
  //int m_num_invisible_regs;
  //int m_num_hidden_regs;
};

expr_ref expr_append_pc_string(expr_ref in, bool src, pc const &pc);

}
