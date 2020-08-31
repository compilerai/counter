#pragma once

#include "tfg/predicate.h"
#include "gsupport/pc.h"
#include "expr/state.h"
#include "graph/graph_cp_location.h"
#include "graph/callee_summary.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "support/log.h"
#include "graph/lr_map.h"
#include <list>
#include <map>
#include <stack>
#include "gsupport/sprel_map.h"
#include "gsupport/suffixpath.h"
#include "tfg/avail_exprs.h"
#include "gsupport/tfg_edge.h"
#include "tfg/edge_guard.h"
#include "graph/graph_with_execution.h"
#include "support/types.h"
#include "support/serpar_composition.h"
#include "rewrite/transmap.h"
//#include "rewrite/symbol_map.h"
#include "rewrite/nextpc_function_name_map.h"
#include "valtag/nextpc_map.h"
struct nextpc_function_name_map_t;
//#include "rewrite/translation_instance.h"
#include "graph/delta.h"

struct symbol_t;
struct symbol_map_t;

namespace eqspace {

using namespace std;
class rodata_map_t;
class symbol_or_section_id_t;
class slot
{
public:
  slot(tfg const *tfg, graph_loc_id_t loc_id) : m_tfg(tfg), m_loc_id(loc_id) {}
  expr_ref get_expr(/*pc const &p*/) const;
  string to_string(/*pc const &p*/) const;

private:
  tfg const *m_tfg;
  graph_loc_id_t m_loc_id;
};

/*
class slot
{
public:
  virtual expr_ref get_expr(const state& st) const = 0;
  virtual string to_string() const = 0;

private:
};

class slot_reg : public slot
{
public:
  slot_reg(const string& name) : m_reg_name(name) {}
  expr_ref get_expr(const eq::state& st) const override { return st.get_expr(m_reg_name); }
  string to_string() const override
  {
    return m_reg_name;
  }

private:
  string m_reg_name;
};

class slot_mem_addr : public slot
{
public:
  //slot_mem_addr(memlabel_t memlabel, expr_ref addr, unsigned count, bool bigendian) : m_memlabel(memlabel), m_addr(addr), m_count(count), m_bigendian(bigendian) {}
  slot_mem_addr(const tuple<memlabel_t, expr_ref, unsigned, bool>& tp, vector<memlabel_t> const &relevant_memlabels) : m_memlabel(get<0>(tp)), m_addr(get<1>(tp)), m_count(get<2>(tp)), m_bigendian(get<3>(tp)), m_relevant_memlabels(relevant_memlabels) {}
  expr_ref get_expr(const eq::state& st) const override
  {
    //expr_ref arr = st.get_expr(m_arr_reg_name);
    expr_ref arr = st.get_memory_expr();
    return arr->get_context()->mk_select(arr, m_memlabel, m_addr, m_count, m_bigendian, comment_t());
  }

  expr_list get_expr_masked_mem(const eq::state& st) const
  {
    //expr_ref arr = st.get_expr(m_arr_reg_name);
    expr_ref arr = st.get_memory_expr();
    context *ctx = arr->get_context();
    expr_list ret;
    for (auto rm : m_relevant_memlabels) {
      ret.push_back(ctx->mk_memmask(ctx->mk_store(arr, m_memlabel, m_addr, ctx->mk_zerobv(m_count * BYTE_LEN), m_count, m_bigendian, comment_t()), rm));
    }
    return ret;
  }

  string to_string() const override
  {
    static char as1[4096];
    stringstream ss;
    //ss << m_arr_reg_name << "addr: "  << m_addr->to_string() << " size: " << m_count;
    ss << "memory memlabel: " << eq::memlabel_to_string(&m_memlabel, as1, sizeof as1) << " addr: "  << m_addr->to_string() << " size: " << m_count;

    return ss.str();
  }

  unsigned get_count() const
  {
    return m_count;
  }

private:
  memlabel_t m_memlabel;
  expr_ref m_addr;
  unsigned m_count;
  bool m_bigendian;
  vector<memlabel_t> m_relevant_memlabels;
};

class slot_mem_mask : public slot
{
public:
  slot_mem_mask(memlabel_t memlabel, expr_ref addr, unsigned count) : m_memlabel(memlabel), m_addr(addr), m_count(count) {}
  slot_mem_mask(const tuple<memlabel_t, expr_ref, unsigned>& tp) : m_memlabel(get<0>(tp)), m_addr(get<1>(tp)), m_count(get<2>(tp)) {}
  expr_ref get_expr(const eq::state& st) const override
  {
    //expr_ref arr = st.get_expr(m_arr_reg_name);
    expr_ref arr = st.get_memory_expr();
    return arr->get_context()->mk_memmask(arr, m_memlabel);
  }

  string to_string() const override
  {
    static char as1[4096];
    stringstream ss;
    ss << "memmask memlabel: " << eq::memlabel_to_string(&m_memlabel, as1, sizeof as1) << " addr: "  << m_addr->to_string() << " size: " << m_count;
    return ss.str();
  }

  memlabel_t get_memlabel() const
  {
    return m_memlabel;
  }

private:
  memlabel_t m_memlabel;
  expr_ref m_addr;
  unsigned m_count;
};

class slot_fcall_retreg : public slot
{
public:
  slot_fcall_retreg(const string& name) : m_reg_name(name) {}
  expr_ref get_expr(const eq::state& st) const override { return st.get_expr(m_reg_name); }
  string to_string() const override
  {
    return m_reg_name;
  }

private:
  string m_reg_name;
};
*/

/*class edge_locid_pair {
  public:
  shared_ptr<tfg_edge const> m_edge;
  graph_loc_id_t m_loc_id;

  edge_locid_pair(shared_ptr<tfg_edge const> edge, int loc_id) : m_edge(edge), m_loc_id(loc_id) {}
};

struct edge_locid_pair_comp {
  bool operator()(edge_locid_pair const &left, edge_locid_pair const &right) const
  {
    if (left.m_loc_id < right.m_loc_id) {
      return true;
    }
    if (left.m_loc_id > right.m_loc_id) {
      return false;
    }
    assert(left.m_loc_id == right.m_loc_id);
    if (left.m_edge < right.m_edge) {
      return true;
    }
    if (right.m_edge > left.m_edge) {
      return false;
    }
    return false;
  }
};*/

typedef list<shared_ptr<tfg_edge const>> tfg_path;
using intrinsic_interpret_fn_t = std::function<list<shared_ptr<tfg_edge const>> (shared_ptr<tfg_edge const> const &/*, state const &start_state*/)>;

class tfg : public graph_with_execution<pc, tfg_node, tfg_edge, predicate>
{
public:
  //tfg(string_ref const &name, context* ctx, state const &start_state);
  tfg(string const &name, context* ctx);
  tfg(tfg const &other);
  tfg(istream& is, string const& name, context* ctx);

  static shared_ptr<tfg> tfg_rewrite(tfg const &t);
  std::set<pc> get_all_recursive_fcalls();
  set<pc> tfg_identify_anchor_pcs() const;
  bool tfg_has_anchor_free_cycle(std::set<pc> const &anchor_pcs) const;
  void tfg_perform_tail_recursion_optimization();
  void tfg_update_symbol_map_and_rename_expressions_at_zero_offset(graph_symbol_map_t const &new_symbol_map);
  void tfg_rename_srcdst_identifier(string const &from_id, string const &to_id);
  void append_to_input_keyword(string const &keyword);
  void rename_keys(map<string, string> const &key_map, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap);
  void add_edge_composite(shared_ptr<tfg_edge_composition_t> const &e);
  void remove_node_and_make_composite_edges(pc const &p);
  tuple<size_t, size_t, vector<size_t>, vector<size_t>> remove_nodes_compute_node_weight_tuple(list<shared_ptr<tfg_edge const>> const &incoming, list<shared_ptr<tfg_edge const>> const &outgoing) const;
  void collapse_tfg(bool preserve_branch_structure/*, bool collapse_function_calls*/);
  void replace_exit_fcalls_with_exit_edge();
  void preprocess();
  void rename_locals_to_stack_pointer();
  //void resize_farg_exprs_for_function_calling_conventions();
  //void rename_farg_ptr_memlabel_top_to_notstack();
  //void substitute_rodata_accesses(tfg const &reference_tfg_for_symbol_types);
  //void convert_esp_to_esplocals();
  //void add_outgoing_values(const pc& p, list<pair<string, expr_ref>> const &outgoing_values);
  //void add_sprel_assumes();
  //void populate_pred_dependencies_for_edgeconds();
  //map<pc, map<pc, list<pair<graph_loc_id_t, pair<expr_ref, expr_ref>>>>> do_constant_propagation();
  //void populate_cp_assumes();
  void add_offset_to_all_locs(graph_loc_id_t offset);
  //void set_fcall_read_write_memlabels_to_bottom();

  //virtual void add_edge(shared_ptr<tfg_edge> e) override
  //{
  //  this->graph<pc, tfg_node, tfg_edge>::add_edge(e);
  //  cout << __func__ << " " << __LINE__ << ": this->incoming =\n" << this->incoming_sizes_to_string() << endl;
  //}

  map<pc, map<string_ref, expr_ref>> tfg_llvm_get_return_reg_map_for_whole_functions() const;

  bool memlabel_is_memloc_symbol(memlabel_t const &ml) const;


  static void generate_dst_tfg_file(string const& outfile, string const& function_name, rodata_map_t const& rodata_map, tfg const& dst_tfg, vector<dst_insn_t> const& dst_iseq, vector<dst_ulong> const& dst_insn_pcs);

  static void
  gen_tfg_for_dst_iseq(char const *outfile,
                       char const *execfile,
                       struct dst_insn_t const *dst_iseq,
                       size_t dst_iseq_len,
                       regmap_t const& dst_regmap,
                       char const *tfg_llvm,
                       char const *function_name,
                       struct symbol_map_t const *dst_symbol_map,
                       nextpc_function_name_map_t& new_nextpc_function_name_map,
                       //char const *peep_comment,
                       size_t num_live_out,
                       vector<dst_ulong> const& insn_pcs,
                       class context *ctx);


  bool tfg_locs_equal_at_pcs(pc const &pc1, graph_cp_location const &loc1, pc const &pc2, graph_cp_location const &loc2) const;
  //void remove_trivial_nodes(bool collapse_function_calls);

  //vector<expr_ref> get_global_regs() const;

  const string& get_memory_reg() const { return m_memory_reg; }
  void set_memory_reg(const string& mem_reg) { m_memory_reg = mem_reg; }

  //virtual string tfg_to_string(bool details = false) const override;
  virtual string tfg_to_string_with_invariants() const;

  void to_dot(string filename, bool embed_info) const;
  virtual void graph_to_stream(ostream& os) const override;
  //string to_string_for_eq_llvm() const;
  //string to_string_for_eq(/*vector<pair<pc, list<expr_ref>>> const &exit_conds, cspace::symbol_map_t const *symbol_map, cspace::nextpc_function_name_map_t const *nextpc_function_name_map, bool src*/);
  void remove_dead_edges();

  //bool tfg_callee_summary_exists(string const &fun_name) const;
  //callee_summary_t tfg_callee_summary_get(string const &fun_name) const;
  virtual callee_summary_t get_callee_summary_for_nextpc(nextpc_id_t nextpc_num, size_t num_fargs) const override;
  //void tfg_callee_summary_add(string const &fun_name, callee_summary_t const &csum);

  //void populate_slots();
  //list<slot> const &get_slots(pc const &p) const;

  list<pair<memlabel_t, pair<expr_ref, pair<unsigned, bool> > > > get_store_expressions(shared_ptr<tfg_edge const> ed);
  list<pair<memlabel_t, pair<expr_ref, pair<unsigned, bool> > > > get_select_expressions(shared_ptr<tfg_edge const> ed);

  map<pc, map<graph_loc_id_t, bool>> determine_initialization_status_for_locs() const;
  bool return_register_is_uninit_at_exit(string return_reg_name, pc const &exit_pc) const;

  void tfg_add_location_slots_using_state_mem_acc_map(map<pc, list<eqspace::state::mem_access>> const &state_mem_acc_map/*map<shared_ptr<tfg_edge>, list<pair<memlabel_t, pair<expr_ref, pair<unsigned, pair<bool, comment_t>>>>>> const &mem_acc_map*/);

  bool contains_float_op() const
  {
    return    check_if_any_edge_sets_flag("contains_float_op")
           || check_if_any_edge_sets_flag(G_LLVM_PREFIX "-contains_float_op");
  }
  bool contains_unsupported_op() const
  {
    return    check_if_any_edge_sets_flag("contains_unsupported_op")
           || check_if_any_edge_sets_flag(G_LLVM_PREFIX "-contains_unsupported_op");
  }

  //size_t get_num_tfg_addrs(pc const &p) const;

  void tfg_add_transmap_translations_for_input_arguments(graph_arg_regs_t const &args, transmap_t const &in_tmap, int stack_gpr);
  void dst_tfg_add_stack_pointer_translation_at_function_entry(exreg_id_t stack_gpr);
  void dst_tfg_add_retaddr_translation_at_function_entry(exreg_id_t stack_gpr);
  void dst_tfg_add_callee_save_translation_at_function_entry(vector<exreg_id_t> const& callee_save_gpr);
  void dst_tfg_add_single_assignments_for_register(exreg_id_t const& regid);
  //void dst_tfg_add_indir_target_translation_at_function_entry();
  void allocate_stack_memory_at_start_pc();
  //void mask_input_stack_and_locals_to_zero();
  void convert_to_locs(tfg &tfg_loc) const;
  void do_preprocess(consts_struct_t const &cs);
  //void populate_relevant_addr_refs(eq::consts_struct_t const &cs);

  //void relevant_addr_refs_union(tfg const *other);
  void eliminate_donotsimplify_using_solver_operators();
  void tfg_eliminate_memlabel_esp();
  void tfg_run_copy_propagation();
  void tfg_add_global_assumes_for_string_contents(bool is_dst_tfg);

  //void append_insn_id_to_comments(int insn_id);
  //void set_comments_to_zero(void);

  void substitute_tfg_locs_input_vars_with_node_vars(eqspace::consts_struct_t const &cs);
  void tfg_locs_remove_duplicates_mod_comments(eqspace::consts_struct_t const &cs);
  static void tfg_locs_remove_duplicates_mod_comments_for_pc(map<graph_loc_id_t, graph_cp_location> const &tfg_locs, map<graph_loc_id_t, bool> &is_redundant);

  void edge_set_to_pc(shared_ptr<tfg_edge const> e, pc const &new_pc/*, state const &new_state*/);

  void tfg_substitute_variables(map<expr_id_t, pair<expr_ref, expr_ref> > const &map, bool opt=true);

  //void tfg_annotate_locals(cspace::input_exec_t const *in, dwarf_map_t const *dwarf_map, struct eq::consts_struct_t const *cs, eq::context *ctx, sym_exec const &se, cspace::locals_info_t *locals_info);

  void clear_ismemlabel_assumes();
  void add_ismemlabel_assumes_for_locs(map<pc, map<graph_loc_id_t, lr_status_ref>> const &linear_relations);
  void add_ismemlabel_assumes_for_addrs(map<pc, map<graph_loc_id_t, lr_status_ref>> const &linear_relations);
  bool expr_contains_local_keyword_expr(expr_ref const &e);

  void append_tfg(tfg const &new_tfg/*, bool src*/, pc const &pc, eqspace::context *ctx);

  void eliminate_edges_with_false_pred(void);
  string read_from_ifstream(ifstream& db, context* ctx, state const &base_state);

  //void set_locals_info(struct cspace::locals_info_t *locals_info) { m_locals_info = locals_info; }

  virtual shared_ptr<tfg> tfg_copy(/*expr_vector const &function_arguments*/) const = 0;
  void convert_function_argument_local_memlabels(eqspace::consts_struct_t const &cs);
  //shared_ptr<tfg_edge> convert_jmp_nextpcs_to_callret(consts_struct_t const &cs/*, map<nextpc_id_t, pair<size_t, callee_summary_t>> const &nextpc_args_map*/, int r_esp, int r_eax);
  //void add_fcall_arguments_using_nextpc_args_map(map<nextpc_id_t, pair<size_t, callee_summary_t>> const &nextpc_args_map, int r_esp);
  void convert_jmp_nextpcs_to_callret(/*struct context *ctx, sym_exec const &se, map<nextpc_id_t, pair<size_t, callee_summary_t>> const &nextpc_args_map, */int r_esp, int r_eax/*, bool src*/);
  void get_all_function_calls(set<expr_ref> &ret) const;
  //void get_nextpc_args_map(map<nextpc_id_t, pair<size_t, callee_summary_t>> &out) const;
  void edge_rename_to_pc_using_nextpc_map(shared_ptr<tfg_edge const> e, nextpc_map_t const &nextpc_map, long cur_index, state const &start_state/*, bool src*/);
  void dfs_find_loop_heads_and_tails(pc const &p, map<pc, dfs_color> &visited, set<pc> &loop_heads, set<pc>& loop_tails) const;
  void find_loop_heads(set<pc> &loop_heads) const;
  void find_loop_tails(set<pc>& loop_tails) const;
  //void find_loop_heads_ignoring_fcall_edges(set<pc> &loop_heads) const;

  void find_loop_anchors_collapsing_into_unconditional_fcalls(set<pc> &loop_heads) const;
  void get_rid_of_non_loop_head_labels(set<pc> &labels_to_delete) const;
  //void get_rid_of_redundant_labels(set<pc> &labels_to_delete) const;
  //void delete_label_list(set<pc> const &labels_to_delete);
  void transform_function_arguments_to_avoid_stack_pointers();
  //void check_consistency_of_edges();
  void update_tfg_addrs(set<pc> const &node_changed_map_for_any_addr_ref);
  //void init_linear_relations_to_top_for_new_tfg_addrs(map<pc, lr_map_t, pc_comp> &linear_relations);
  //void read_tfg_llvm(ifstream &in, tfg &out, tfg const &tfg_src, struct context *ctx);
  void rename_llvm_symbols_to_keywords(context *ctx/*, tfg const &tfg_src*/);
  void add_exit_return_value_ptr_if_needed(tfg const &tfg_llvm);
  static expr_ref dst_get_return_reg_expr_using_out_tmap(context *ctx, state const &start_state, transmap_t const &out_tmap, string const &retreg_name, size_t size, int stack_gpr);

  void populate_exit_return_values_using_out_tmaps(map<pc, map<string_ref, expr_ref>> const &return_reg_map, map<pc, transmap_t> const &out_tmaps, int stack_gpr);
  void populate_exit_return_values_for_llvm_method();

  shared_ptr<state const> get_start_state_with_all_input_exprs() const;

  //void reconcile_outputs_with_llvm_tfg(tfg &tfg_llvm);
  static bool symbol_is_const(struct symbol_t const *sym);
  src_ulong get_min_offset_with_which_symbol_is_used(symbol_id_t sid) const;
  rodata_map_t populate_symbol_map_using_llvm_symbols_and_exec(struct symbol_map_t const *symbol_map, graph_symbol_map_t const &llvm_symbol_map, map<pair<symbol_id_t, offset_t>, vector<char>> const &llvm_string_contents, input_exec_t const *in);
  void populate_nextpc_map(nextpc_function_name_map_t const *c_nextpc_function_name_map);
  void populate_symbol_map_using_memlocs_in_tmaps(transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_out_tmaps);
  //void set_symbol_map(string symbol_map_string);
  void set_string_contents(map<pair<symbol_id_t, offset_t>, vector<char>> const &string_contents) { m_string_contents = string_contents; }

  void set_symbol_map_for_touched_symbols(graph_symbol_map_t const &symbol_map, set<symbol_id_t> const &touched_symbols);
  void set_string_contents_for_touched_symbols_at_zero_offset(map<pair<symbol_id_t, offset_t>, vector<char>> const &string_contents, set<symbol_id_t> const &touched_symbols);

  virtual map<pair<symbol_id_t, offset_t>, vector<char>> const &get_string_contents() const { return m_string_contents; }
  //map<symbol_id_t, pair<string, size_t>> get_symbol_map_excluding_rodata() const;
  void set_nextpc_map(map<nextpc_id_t, string> const &nextpc_map) { m_nextpc_map = nextpc_map; }
  map<nextpc_id_t, string> const &get_nextpc_map() const { return m_nextpc_map; }
  nextpc_id_t get_nextpc_id_from_function_name(string const &fun_name) const;

  void replace_llvm_memoutput_with_memmasks(tfg const &tfg_src);
  void tfg_init_locs_for_all_pcs(void);
  //void canonicalize_llvm_symbols(graph_symbol_map_t const &symbol_map);
  //void set_string_contents(map<symbol_id_t, string> const &string_contents);
  void canonicalize_llvm_nextpcs();
  //void rename_symbols(context *ctx, tfg const &tfg_other);
  void remove_function_name_from_symbols(string const &function_name);

  /*void rename_nextpcs(context *ctx,
    struct nextpc_function_name_map_t *nextpc_function_name_map);*/
  void rename_nextpcs(context *ctx, map<nextpc_id_t, string> const &llvm_nextpc_map);
  //void rename_recursive_calls_to_nextpcs();
  static bool function_is_gcc_standard_function(char const *name);
  void expand_calls_to_gcc_standard_functions(nextpc_function_name_map_t *map, int r_esp);
  void expand_calls_to_gcc_standard_function(int nextpc_num, char const *function_name, int r_esp);
  void rename_recursive_calls_to_nextpcs(struct nextpc_function_name_map_t *nextpc_function_name_map);
  void replace_alloca_with_store_zero();
  void replace_alloca_with_nop();
  void set_input_caller_save_registers_to_zero();
  //void zero_out_llvm_temp_vars();
  void truncate_function_arguments_using_tfg(tfg const &tfg_other);
  string symbols_to_string();
  bool symbol_is_rodata(symbol_id_t symbol_id) const;
  //bool memlabel_is_rodata(memlabel_t const &ml) const;
  void debug_check(void) const;
  //virtual void graph_get_relevant_memlabels(vector<memlabel_ref> &relevant_memlabels) const override;
  //virtual void graph_get_relevant_memlabels_except_args(vector<memlabel_ref> &relevant_memlabels) const override;
  //virtual void graph_populate_relevant_addr_refs() override;
  void tighten_memlabels_using_switchmemlabel_and_simplify_until_fixpoint(consts_struct_t const &cs);
  //void collapse_rodata_symbols_into_one(void);
  //void remove_fcall_side_effects(void);
  //void make_fcalls_semantic(void);
  size_t max_expr_size(void) const;
  //bool contains_memlabel_bottom() const;

  size_t get_symbol_size_from_id(symbol_id_t symbol_id) const;
  bool symbol_represents_memloc(symbol_id_t symbol_id) const;
  variable_constness_t get_symbol_constness_from_id(int symbol_id) const;
  size_t get_local_size_from_id(int local_id) const;
  symbol_id_t get_symbol_id_from_name(string const &name) const;
  //bool contains_llvm_return_register(pc const &p, size_t &return_register_size) const;

  void add_assumes_to_start_edge(unordered_set<expr_ref> const& arg_assumes);
  void expand_fcall_edge(shared_ptr<tfg_edge const> &e);
  void add_extra_nodes_around_fcalls();
  void tfg_llvm_interpret_intrinsic_fcalls();
  void add_extra_nodes_at_basic_block_entries();
  bool pc_is_basic_block_entry(pc const &p) const;
  void add_extra_node_at_start_pc(pc const &pc_cur_start);
  map<string_ref, expr_ref> get_start_state_mem_pools_except_stack_locals_rodata_and_memlocs_as_return_values() const;
  //void add_outgoing_value(pc const &p, expr_ref e, string const &comment);
  //list<pair<string, expr_ref>> const &get_outgoing_values(pc const &p) const;

  //void mask_input_stack_memory_to_zero_at_start_pc();
  set<local_id_t> identify_address_taken_local_variables() const;
  set<expr_ref> identify_stack_pointer_relative_expressions() const;
  //set<pair<string, expr_ref>> get_outgoing_values_at_delta(pc const &p, delta_t const& delta) const;

  //set<graph_loc_id_t> get_pred_min_dependencies_set_for_composite_edge_loc(shared_ptr<tfg_edge_composite> e, graph_loc_id_t loc_id) const;
  //tfg_edge_composite path_to_composite_edge(list<shared_ptr<tfg_edge>> const &path) const;
  map<pc, map<graph_loc_id_t, mybitset>> compute_dst_locs_assigned_to_constants_map() const;
  callee_summary_t get_summary_for_calling_functions() const;
  void set_callee_summaries(map<nextpc_id_t, callee_summary_t> const &callee_summaries);
  map<nextpc_id_t, callee_summary_t> const &get_callee_summaries() const;
  //bool tfg_is_nop() const;
  //bool tfg_reads_heap_or_symbols() const;
  //bool tfg_writes_heap_or_symbols() const;
  set<memlabel_ref> tfg_get_read_memlabels() const;
  set<memlabel_ref> tfg_get_write_memlabels() const;
  //void model_nop_callee_summaries(map<nextpc_id_t, callee_summary_t> const &callee_summaries);
  list<shared_ptr<tfg_edge const>> get_fcall_edges(nextpc_id_t nextpc_id) const;
  //void replace_fcalls_with_nop(nextpc_id_t nextpc_id);
  shared_ptr<tfg_edge const> get_incident_fcall_edge(pc const &p) const;
  expr_ref get_incident_fcall_nextpc(pc const &p) const;
  nextpc_id_t get_incident_fcall_nextpc_id(pc const &p) const;
  memlabel_t get_incident_fcall_memlabel_writeable(pc const &p) const;

  void get_reachable_pcs_stopping_at_fcalls(pc const &p, set<pc> &reachable_pcs) const;
  bool is_pc_pair_compatible(pc const &p1, int p2_subsubindex, expr_ref const& p2_incident_fcall_nextpc) const;
  virtual map<pair<pc, int>, shared_ptr<tfg_edge_composition_t>> get_composite_path_between_pcs_till_unroll_factor(pc const& from_pc, set<pc> const& to_pcs, map<pc,int> const& visited_count, int unroll_factor, set<pc> const& stop_pcs) const;

  bool is_tfg_ec_mutex(shared_ptr<tfg_edge_composition_t> const &ec) const;

  void get_unrolled_loop_paths_from(pc from_pc, list<shared_ptr<tfg_edge_composition_t>>& pths, int max_occurrences_of_a_node) const;

  void get_unrolled_paths_from(pc const &pc_from, map<pair<pc,int>, shared_ptr<tfg_edge_composition_t>>& pths, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, int max_occurrences_of_a_node) const;

//  void generate_unrolled_suffix_paths(shared_ptr<tfg_edge_composition_t> const &comp_pth, map<int, shared_ptr<tfg_edge_composition_t>> const &unrolled_pths_n_n, map<pc, list<shared_ptr<tfg_edge_composition_t>>> const &compound_pths_0, map< pair<pc,int>, list<shared_ptr<tfg_edge_composition_t>> > &pths, int max_occurrences_of_an_edge) const;

  //bool fcall_nextpc_writes_heap_or_symbol(nextpc_id_t nextpc_num) const;
  bool pc_has_incident_fcall(pc const &p) const;
  predicate_set_t collect_preds_on_composite_edge(shared_ptr<tfg_edge_composition_t> e) const;
  list<pair<edge_guard_t, predicate_ref>> apply_trans_funs_old(shared_ptr<tfg_edge_composition_t> const& ec, predicate_ref const &pred, bool simplified) const;
  list<pair<edge_guard_t, predicate_ref>> apply_trans_funs(shared_ptr<tfg_edge_composition_t> const& ec, predicate_ref const &pred, bool simplified) const;

  //edge_guard_t tfg_edge_composition_get_edge_guard(shared_ptr<tfg_edge_composition_t> src_ec);
  //bool tfg_edge_composition_edge_cond_evaluates_to_true(shared_ptr<tfg_edge_composition_t> const &src_ec) const;
  bool tfg_edge_composition_represents_all_possible_paths(shared_ptr<tfg_edge_composition_t> const &src_ec) const;
  virtual expr_ref tfg_edge_composition_get_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified = false) const;
  virtual expr_ref tfg_edge_composition_get_unsubstituted_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified = false) const;
  state tfg_edge_composition_get_to_state(shared_ptr<tfg_edge_composition_t> const &src_ec/*, state const &start_state*/, bool simplified = false) const;
  unordered_set<expr_ref> tfg_edge_composition_get_assumes(shared_ptr<tfg_edge_composition_t> const &src_ec) const;
  te_comment_t tfg_edge_composition_get_te_comment(shared_ptr<tfg_edge_composition_t> const &src_ec) const;
  shared_ptr<tfg_edge_composition_t> tfg_edge_composition_create_from_path(list<shared_ptr<tfg_edge const>> const &path) const;
  shared_ptr<tfg_edge_composition_t> tfg_edge_composition_compose_in_parallel_factoring_out_common_subpaths(shared_ptr<tfg_edge_composition_t> const &a, shared_ptr<tfg_edge_composition_t> const &b) const;
  static pair<bool, expr_ref> tfg_edge_composition_get_is_indir_tgt(shared_ptr<tfg_edge_composition_t> const &src_ec);
  //void eliminate_io_arguments_from_function_calls();

  //extern std::function<shared_ptr<eg_cnode_t> (tfg_edge_composition_t::type t, shared_ptr<eg_cnode_t> const &a, shared_ptr<eg_cnode_t> const &b)> tfg_edge_composition_fold_edge_guard_ops;
  //virtual void init_graph_locs(graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>()) override;

  void compute_max_memlabel_varnum();
  void tfg_assign_mlvars_to_memlabels();

  bool tfg_edge_composition_contains_fcall(shared_ptr<tfg_edge_composition_t> const &tfg_ec) const;
  static bool tfg_edge_composition_implies(shared_ptr<tfg_edge_composition_t const> const &a, shared_ptr<tfg_edge_composition_t const> const &b);
  static bool tfg_suffixpath_implies(shared_ptr<tfg_edge_composition_t> const &a, shared_ptr<tfg_edge_composition_t> const &b);
  shared_ptr<tfg_edge_composition_t const> tfg_edge_composition_conjunct(shared_ptr<tfg_edge_composition_t const> const &a, shared_ptr<tfg_edge_composition_t const> const &b) const;
  //void substitute_symbols_using_rodata_map(rodata_map_t const &rodata_map);

  static void update_symbol_rename_submap(map<expr_id_t, pair<expr_ref, expr_ref>> &rename_submap, size_t from, size_t to, src_ulong to_offset, context *ctx);
  long &get_max_memlabel_varnum_ref() { return m_max_mlvarnum; }

  map<pc, avail_exprs_t> compute_loc_avail_exprs() const;
  void print_pred_avail_exprs(map<pc, pred_avail_exprs_t> const &pred_avail_exprs) const;
  void print_loc_avail_expr(map<pc, avail_exprs_t> loc_avail_exprs) const;

  void populate_suffixpaths();
  void populate_pred_avail_exprs();

  tfg_suffixpath_t get_suffixpath_ending_at_pc(pc const &p) const
  {
    ASSERT(m_suffixpaths.count(p));
    return m_suffixpaths.at(p);
  }

  pred_avail_exprs_t const &get_pred_avail_exprs_at_pc(pc const &p) const
  {
    ASSERT(m_pred_avail_exprs.count(p));
    return m_pred_avail_exprs.at(p);
  }

  expr_ref tfg_suffixpath_get_expr(tfg_suffixpath_t const &src_ec) const;
  void populate_suffixpath2expr_map() const;

  sprel_map_pair_t get_sprel_map_pair(pc const &p) const override;

  pred_avail_exprs_t const &get_src_pred_avail_exprs_at_pc(pc const &p) const
  {
    return this->get_pred_avail_exprs_at_pc(p);
  }

  edge_guard_t
  graph_get_edge_guard_from_edge_composition(shared_ptr<tfg_edge_composition_t> const &ec) const override
  {
    //NOT_IMPLEMENTED();
    return edge_guard_t(ec);
  }

  void add_transmap_translation_edges_at_entry_and_exit(transmap_t const *in_tmap, transmap_t const *out_tmaps, int num_out_tmaps);
  void tfg_populate_arguments_using_live_regset(regset_t const &live_in);
  //void populate_exit_return_values_using_live_regset(regset_t const *live_out, size_t num_live_out);
  void tfg_introduce_src_dst_prefix(char const *prefix);
  void tfg_visit_exprs(std::function<expr_ref (pc const &, const string&, expr_ref)> f, bool opt = true);
  void tfg_visit_exprs_const(std::function<void (pc const &, const string&, expr_ref)> f, bool opt = true) const;
  void tfg_visit_exprs_const(std::function<void (string const&, expr_ref)> f, bool opt = true) const;

  void tfg_visit_exprs_full(std::function<expr_ref (pc const &, const string&, expr_ref)> f);

  void substitute_gen_set_exprs_using_avail_exprs(map<graph_loc_id_t, expr_ref> &gen_set/*, map<expr_ref, set<graph_loc_id_t>> &expr2deps_cache*/, avail_exprs_t const& this_avail_exprs) const;
  void populate_gen_and_kill_sets_for_edge(tfg_edge_ref const &e, map<graph_loc_id_t, expr_ref> &gen_set, set<graph_loc_id_t> &killed_locs/*, map<expr_ref, set<graph_loc_id_t>> &expr2deps_cache*/) const;

  virtual set<expr_ref> get_interesting_exprs_till_delta(pc const& p, delta_t delta) const;
  set<expr_ref> get_live_loc_exprs_having_sprel_map_at_pc(pc const &p) const;
  set<expr_ref> get_live_loc_exprs_ignoring_memslot_symbols_at_pc(pc const &p) const;

  void tfg_add_unknown_arg_to_fcalls();
  void tfg_identify_rodata_accesses_and_populate_string_contents(input_exec_t const& in);

  virtual void tfg_preprocess(bool is_dst_tfg, list<string> const& sorted_bbl_indices = {}, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>());
  void tfg_preprocess_before_eqcheck(set<string> const &undef_behaviour_to_be_ignored, predicate_set_t const& input_assumes, bool populate_suffixpaths_and_pred_avail_exprs = false);

  shared_ptr<tfg_edge_composition_t> semantically_simplify_tfg_edge_composition(shared_ptr<tfg_edge_composition_t> const& ec) const;

  virtual expr_ref edge_guard_get_expr(edge_guard_t const& eg, bool simplified) const override
  {
    return eg.edge_guard_get_expr(*this, simplified);
  }

  set<dst_ulong> tfg_dst_get_marker_call_instructions(vector<dst_ulong> const& insn_pcs) const;
  void reconcile_state_elements_sort();
  
  tuple<expr_ref, expr_ref, state> tfg_edge_composition_get_to_state_and_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, state const &start_state, bool simplified, bool mask_expr_simplify = false) const;
  map<graph_loc_id_t,expr_ref> tfg_edge_composition_get_potentially_written_locs(shared_ptr<tfg_edge_composition_t> const& src_ec) const;

  tfg const &get_src_tfg() const override //src_tfg is the tfg against which the edge-guards are computed
  {
    return *this;
  }

  static std::function<bool (pc const&, pc const&)> gen_pc_cmpF(list<string> const& sorted_bbl_indices);

  pair<bool,bool> counter_example_translate_on_edge(counter_example_t const &counter_example, tfg_edge_ref const &edge, counter_example_t &ret, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels) const override;

  list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> graph_apply_trans_funs(graph_edge_composition_ref<pc,tfg_edge> const &pth, predicate_ref const &pred, bool simplified = false) const override;
  
  list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> graph_apply_trans_funs_helper(graph_edge_composition_ref<pc,tfg_edge> const &pth, predicate_ref const &pred, bool simplified = false, bool always_merge = false) const;

  virtual predicate_set_t collect_assumes_for_edge(tfg_edge_ref const& te) const override;
  virtual predicate_set_t collect_assumes_around_edge(tfg_edge_ref const& te) const override;
  predicate_set_t get_simplified_non_mem_assumes(tfg_edge_ref const& e) const override;

  virtual aliasing_constraints_t get_aliasing_constraints_for_edge(tfg_edge_ref const& e) const override;
  virtual aliasing_constraints_t graph_apply_trans_funs_on_aliasing_constraints(tfg_edge_ref const& e, aliasing_constraints_t const& als) const override;

  void populate_simplified_edgecond(tfg_edge_ref const& e) const override;
  void populate_simplified_assumes(tfg_edge_ref const& e) const override;
  void populate_simplified_to_state(tfg_edge_ref const &e) const override;
  void populate_locs_potentially_modified_on_edge(tfg_edge_ref const &e) const override;

  void populate_branch_affecting_locs() override;
  //void populate_loc_liveness() override;
  void populate_loc_definedness() override;

  //get reachable_pcs starting at FROM_PC that are "compatible" with TO_PC
  virtual void get_path_enumeration_reachable_pcs(pc const &from_pc, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, set<pc> &reachable_pcs) const;

protected:

  map<graph_loc_id_t,expr_ref> compute_simplified_loc_exprs_for_edge(tfg_edge_ref const& e, set<graph_loc_id_t> const& locids, bool force_update = false) const override;
  map<string_ref,expr_ref> graph_get_vars_written_on_edge(tfg_edge_ref const& e) const override;
  static bool is_fcall_pc(pc const &p);
  map<pair<pc, int>, shared_ptr<tfg_edge_composition_t>> get_composite_path_between_pcs_till_unroll_factor_helper(pc const& from_pc, set<pc> const& to_pcs, map<pc,int> const& visited_count, int pth_length, int unroll_factor, set<pc> const& stop_pcs) const;

private:

  virtual void get_path_enumeration_stop_pcs(set<pc> &stop_pcs) const;
  expr_ref tfg_suffixpath_get_expr_helper(tfg_suffixpath_t const &src_ec) const;

  bool tfg_dst_pc_represents_marker_call_instruction(pc const& p) const;
  void tfg_dst_add_marker_call_argument_setup_and_destroy_instructions(set<pc>& marker_pcs) const;

  bool find_fcall_pc_along_maximal_linear_path(pc const& p) const;

  bool find_fcall_pc_along_maximal_linear_path_from_loop_head(pc const& loop_head, set<pc>& fcall_heads) const;
  bool find_fcall_pc_along_maximal_linear_path_to_loop_tail(pc const& loop_tail, set<pc>& fcall_heads) const;
  void get_anchor_pcs(set<pc>& anchor_pcs) const;
  static bool ignore_fcall_edges(shared_ptr<tfg_edge const> const &e);
  set<expr_ref> get_interesting_exprs_at_delta(pc const &p, delta_t const& delta) const;

  static bool nextpc_names_are_equivalent(string const &a, string const &b);
  void tfg_llvm_interpret_intrinsic_fcall_for_edge(shared_ptr<tfg_edge const> const &e, nextpc_id_t nextpc_id);
  list<shared_ptr<tfg_edge const>> get_edges_for_intrinsic_call(shared_ptr<tfg_edge const> const &e, nextpc_id_t nextpc_id);
  void tfg_llvm_interpret_intrinsic_fcalls_for_nextpc_id(nextpc_id_t nextpc_id);
  static bool llvm_function_is_intrinsic(string const &fname);


  void substitute_at_outgoing_edges_and_node_predicates(pc const &p, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap);
  tfg *get_prefix_tfg(pc const &p) const;
  tfg *get_suffix_tfg_if_single_entry_and_single_exit(pc const &p) const;
  std::set<graph_loc_id_t> tfg_edge_get_modified_locs(shared_ptr<tfg_edge const> const &e) const;
  std::set<graph_loc_id_t> tfg_get_modified_locs_for_sub_tfg(tfg const *sub_tfg) const;
  bool tfg_output_circuits_are_associative_on_inputs(tfg const *sub,
      set<graph_loc_id_t> const &fcall_locs_modified,
      set<graph_loc_id_t> const &suffix_tfg_remaining_locs,
      map<graph_loc_id_t, expr_ref> const &fcall_loc_identity_values) const;

  void dst_tfg_rename_rodata_symbols_to_their_section_names_and_offsets(input_exec_t const* in, symbol_map_t const& symbol_map);
  set<symbol_or_section_id_t> tfg_identify_used_symbols_and_sections() const;
  void dst_tfg_add_unrenamed_symbols_and_sections_to_symbol_map(graph_symbol_map_t& dst_symbol_map, map<symbol_or_section_id_t, symbol_id_t>& rename_submap, set<symbol_or_section_id_t> const& referenced_symbols, input_exec_t const* in);
  void populate_symbol_map_using_memlocs_in_tmap(transmap_t const *tmap);
  //static string llvm_get_callee_save_regname(size_t call_save_regnum);
  pair<bool, list<shared_ptr<tfg_edge const>>> tfg_edge_composition_represents_incoming_edges_at_to_pc(shared_ptr<tfg_edge_composition_t> const &src_ec, list<shared_ptr<tfg_edge const>> const &required_incoming_edges_at_to_pc) const;
  bool determine_loc_avail_exprs_at_pc(pc const &p, map<pc, avail_exprs_t> &loc_avail_exprs, map<expr_ref, set<graph_loc_id_t>> &expr2deps_cache) const;
  bool determine_pred_avail_exprs_at_pc(pc const &p, map<pc, tfg_suffixpath_t> const &suffixpaths, map<pc, map<graph_loc_id_t, set<pair<tfg_suffixpath_t, expr_ref>>>> &pred_avail_exprs, map<expr_ref, set<graph_loc_id_t>> &expr2deps_cache) const;

  void add_string_contents_for_ro_offset_using_input_exec(rodata_offset_t const& ro_offset, size_t nbytes, input_exec_t const& in);

  map<expr_id_t, pair<expr_ref, expr_ref>> get_pred_dependency_to_constant_expr_submap_using_sprel_mapping_and_argument_locids(map<graph_loc_id_t, sprel_status_t> const& mappings, set<graph_loc_id_t> const& arg_locids) const;
  map<graph_loc_id_t, sprel_status_t> get_sprel_status_mapping_from_avail_exprs(avail_exprs_t const &avail_exprs, set<graph_loc_id_t> const& arg_locids) const;
  map<pc, sprel_map_t> compute_sprel_relations() const override;
  //bool graph_substitute_using_sprels(map<pc, sprel_map_t> const &sprels_map);
  bool tfg_edge_composition_involves_fcall(shared_ptr<tfg_edge_composition_t> const &src_ec) const;
  static bool incident_fcall_nextpcs_are_compatible(expr_ref nextpc1, expr_ref nextpc2);
  //bool is_rodata_symbol_string(string s) const;
  //bool symbols_need_to_be_collapsed(string a, string b) const;
  void tfg_evaluate_dst_preds_on_edge_and_add_to_src_node(shared_ptr<tfg_edge const> e);
  predicate_set_t tfg_evaluate_pred_on_edge_composition(shared_ptr<tfg_edge_composition_t> const &ec, predicate_ref const &pred) const;

  static transmap_t construct_out_tmap_for_exit_pc_using_function_calling_conventions(map<string_ref, expr_ref> const &return_regs, int const r_eax);
  static void construct_tmap_add_transmap_entry_for_stack_offset(transmap_t &in_tmap, string_ref const &argname, int cur_offset);
  static void construct_tmap_add_transmap_entry_for_gpr(transmap_t &in_tmap, string_ref const &argname, int regnum, transmap_loc_modifier_t mod);

  static pair<transmap_t, map<pc, transmap_t>> construct_transmaps_using_function_calling_conventions(graph_arg_regs_t const &argument_regs, map<pc, map<string_ref, expr_ref>> const &llvm_return_reg_map, int const r_eax);

  //void get_nextpc_num_args_map(map<unsigned, pair<size_t, set<expr_ref, expr_ref_cmp_t>>> &out) const; //get_nextpc_args_map already exists; perhaps dup implementation
  void truncate_function_arguments_using_nextpc_num_args_map(map<unsigned, pair<size_t, set<expr_ref, expr_ref_cmp_t>>> const &nextpc_num_args_map);

  static int symbol_map_find_symbol_index(graph_symbol_map_t const &llvm_symbol_map, string symbol_name, bool strip_numeric_suffix);
  //int find_symbol_index(string const &symbol_name, bool strip_numeric_suffix) const;
  static int nextpc_map_find_nextpc_index(map<nextpc_id_t, string> const &nextpc_map, string nextpc_name);
  //int find_nextpc_index(string nextpc_name) const;
  size_t get_num_symbols(void) const;
  bool node_has_only_nop_outgoing_edge(shared_ptr<tfg_node> n);
  void collapse_nop_edges();
  void combine_edges_with_same_src_dst_pcs();
  void tfg_addrs_remove_duplicates(pc const &p);
  size_t tfg_count_addrs(pc const &p) const;
  //void tfg_add_addrs_using_locs();
  void tfg_check_consistency_of_addrs_at_pc(pc const &p);
  void tfg_add_addrs_using_state_mem_acc_map(map<pc, list<eqspace::state::mem_access>> const &state_mem_acc_map);
  pc remove_nodes_get_min_incoming_outgoing(map<pc, tuple<size_t, size_t, vector<size_t>, vector<size_t>>> const &remove_nodes) const;
  void remove_nodes_update_incoming_outgoing(map<pc, tuple<size_t, size_t, vector<size_t>, vector<size_t>>> &remove_nodes) const;
  //bool is_local_addr_helper(expr_ref addr, int insn_index, map<pc, sprel_map_t, pc_comp> const &sprels, map<pc, map<unsigned, expr_ref>, pc_comp> &sprels_subcache_map/*, map<pc, lr_map_t, pc_comp> const &linear_relations*/, expr_ref l, int l_index, size_t lsize, context *ctx, sym_exec const &se, map<pc_exprid_pair, graph_loc_id_t, pc_exprid_pair_comp> &expr_represents_loc_in_state_cache, eq::consts_struct_t const *cs, expr_ref &new_addr) const;

  void append_tfg_add_node_pc(pc const &new_pc/*, state const &start_state, bool src*/);

  static bool expr_is_array_variable(expr_ref const &e, const void *dummy);
  static bool expr_contains_array_variable(expr_ref const &e);
  void find_dependencies(expr_ref e, pc const &from_pc/*, state const *from_state*/,
    vector<graph_loc_id_t> const &sorted_loc_indices, pair<expr_ref, vector<graph_loc_id_t>> &ret, eqspace::consts_struct_t const &cs) const;
  bool check_if_any_edge_sets_flag(const string& flag_name) const;
  bool path_represented_by_tfg_edge_composition_is_not_feasible(shared_ptr<tfg_edge_composition_t> const& ec) const;

  static intrinsic_interpret_fn_t llvm_intrinsic_interpret_usub_with_overflow_i32;
  static map<string, intrinsic_interpret_fn_t> interpret_intrinsic_fn;

  string m_memory_reg;

  map<nextpc_id_t, string> m_nextpc_map;
  map<nextpc_id_t, callee_summary_t> m_callee_summaries;
  map<pair<symbol_id_t, offset_t>, vector<char>> m_string_contents;
  long m_max_mlvarnum;

  map<pc, tfg_suffixpath_t> m_suffixpaths;
  map<pc, pred_avail_exprs_t> m_pred_avail_exprs;
  mutable map<tfg_suffixpath_t, expr_ref> m_suffixpath2expr;
  mutable map<tuple<pc, pc, int>, map<pair<pc,int>, shared_ptr<tfg_edge_composition_t>>> m_from_pc_unroll_factor_to_paths_cache;
  mutable map<pair<pc, delta_t>, set<expr_ref>> m_pc_delta_to_interesting_exprs_cache;
};

extern bool COMPARE_IO_ELEMENTS;

void input_exec_scan_rodata_region_and_populate_rodata_map(struct input_exec_t const *in, int input_section_index, char const *contents, size_t contents_len, rodata_map_t *rodata_map, symbol_id_t llvm_string_symbol_id, map<symbol_or_section_id_t, symbol_id_t> const& rename_submap);
map<pc, map<string_ref, expr_ref>> get_return_reg_map_from_live_out(regset_t const *live_out, size_t num_live_out, context *ctx);

bool getNextFunctionInTfgFile(ifstream &in, string &nextFunctionName);

}
