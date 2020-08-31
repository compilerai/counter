#pragma once

#include "expr/expr.h"
//#include "src-defs.h"
//#include "eqcheck/common.h"
#include <vector>
#include <list>
#include "support/consts.h"
#include "support/types.h"
#include "memlabel.h"

namespace eqspace {
struct consts_struct_t;
class context;
extern consts_struct_t *g_consts_struct; //should be used only for C code; e.g., harvest/fbgen

using namespace std;

struct consts_struct_t
{
  consts_struct_t() : m_ctx(nullptr)
  {
    //relevant_memlabels = NULL;
    //relevant_memlabels_str = NULL;
    //num_relevant_memlabels = 0;
    //relevant_memlabels_bvsize = 0;
    g_consts_struct = this;
  }
  void set_expr_value(reg_type rt, unsigned i, expr_ref e);
  expr_ref get_expr_value(reg_type rt, unsigned i) const;
  expr_ref get_stack_size() const;
  string to_string() const;
  bool addr_refs_intersects_with_avars(list<expr_ref> const &vars) const;
  bool addr_refs_type_intersects_with_avars(list<expr_ref> const &vars, reg_type rt, long n) const;
  void init_consts_map(void);
  void init_addr_refs(void);
  long get_num_addr_ref(void) const { return m_addr_refs.size(); }
  pair<string, expr_ref> get_addr_ref(long i) const { return m_addr_refs[i]; }
  pair<reg_type, long> get_addr_ref_reg_type_and_id(long i) const { return m_addr_refs_reg_type_and_id[i]; }
  expr_ref get_input_stack_pointer_const_expr() const;
  long get_addr_ref_id_for_keyword(string keyword) const;
  //expr_ref get_input_memory_const_expr() const;
  expr_ref get_function_call_identifier_regout(int num_args) const { return m_function_call_identifier_regouts[num_args]; }
  expr_ref get_function_call_identifier_memout(int num_args) const { return m_function_call_identifier_memouts[num_args]; }
  expr_ref get_function_call_identifier_regout_unknown_args() const { return m_function_call_identifier_regout_unknown_args; }
  expr_ref get_function_call_identifier_memout_unknown_args() const { return m_function_call_identifier_memout_unknown_args; }
  //expr_ref get_function_call_identifier_io_out(int num_args) const { return m_function_call_identifier_io_outs[num_args]; }
  bool is_symbol(expr_ref const &e) const;
  bool is_dst_symbol_addr(expr_ref const &e) const;
  bool is_section(expr_ref const &e) const;
  bool is_canonical_constant(expr_ref const &e) const;
  //bool expr_is_argument_constant(expr_ref e) const;
  bool expr_is_retaddr_const(expr_ref const &e) const;
  bool expr_is_callee_save_const(expr_ref const &e) const;
  bool expr_is_fcall_with_unknown_args(expr_ref const &e) const;
  static symbol_id_t get_symbol_id_from_expr(expr_ref const &e);
  static symbol_id_t get_dst_symbol_addr_id_from_expr(expr_ref const &e);
  static int get_section_id_from_expr(expr_ref const &e);
  static int get_canonical_constant_id_from_expr(expr_ref const &e);
  static nextpc_id_t get_nextpc_id_from_expr(expr_ref const &e);

  //void solver_set_relevant_memlabels(vector<memlabel_ref> const &relevant_memlabels);
  //vector<memlabel_ref> const &solver_get_relevant_memlabels(/*vector<memlabel_t> &relevant_memlabels*/) const
  //{
  //  return relevant_memlabels;
  //}

  string relevant_addr_refs_to_string(set<cs_addr_ref_id_t> const &relevant_addr_refs) const;
  string read_relevant_addr_refs(ifstream &db, string const &line, set<cs_addr_ref_id_t> &relevant_addr_refs);

  memlabel_t get_memlabel_bottom() const;
  bool expr_represents_llvm_undef(expr_ref const &e) const;

  //void set_argument_constants(expr_vector const &args);
  //void set_retaddr_const(expr_ref const &e);
  //expr_vector const &get_argument_constants() const;
  expr_ref const & get_retaddr_const() const;
  vector<expr_ref> const& get_callee_save_consts() const;
  //void parse_consts_db(context *ctx);
  static string read_consts_struct(ifstream &db, consts_struct_t &cs, context *ctx);

  void clear()
  {
    m_consts.clear();
    m_symbols.clear();
    m_dst_symbol_addrs.clear();
    m_locals.clear();
    m_nextpc_consts.clear();
    m_insn_id_consts.clear();
    m_addr_refs.clear();
    m_addr_refs_reg_type_and_id.clear();
    m_function_call_identifier_regouts.clear();
    m_function_call_identifier_memouts.clear();
    m_callee_save_consts.clear();
    m_sections.clear();
    //m_function_call_identifier_io_outs.clear();
  }

  void init_sizes()
  {
    m_consts.resize(NUM_CANON_CONSTANTS);
    m_symbols.resize(NUM_CANON_SYMBOLS);
    m_dst_symbol_addrs.resize(NUM_CANON_DST_SYMBOL_ADDRS);
    m_locals.resize(MAX_NUM_LOCALS);
    m_nextpc_consts.resize(MAX_NEXTPC_CONSTANTS);
    m_insn_id_consts.resize(MAX_INSN_ID_CONSTANTS);
    m_function_call_identifier_regouts.resize(FUNCTION_CALL_MAX_ARGS + 1);
    m_function_call_identifier_memouts.resize(FUNCTION_CALL_MAX_ARGS + 1);
    m_callee_save_consts.resize(MAX_CALLEE_SAVE_CONSTANTS);
    m_sections.resize(NUM_CANON_SECTIONS);
    //m_function_call_identifier_io_outs.resize(FUNCTION_CALL_MAX_ARGS + 1);
    //m_consts.resize(16);
  }

  //void get_submap_for_zero_symbols(map<expr_id_t, pair<expr_ref, expr_ref> > &submap) const;
  //void get_submap_for_local_neg_exprs(map<unsigned, pair<expr_ref, expr_ref> > &submap, struct cspace::locals_info_t const *locals_info) const;
  bool expr_is_local_id(expr_ref const &e) const;
  local_id_t expr_get_local_id(expr_ref const &e) const;
  expr_ref get_memzero() const;
  expr_ref get_memlabel_dummy_const_val() const;

  context *m_ctx;
  vector<expr_ref> m_consts;
  vector<expr_ref> m_symbols;
  vector<expr_ref> m_dst_symbol_addrs;
  vector<expr_ref> m_locals;
  vector<expr_ref> m_nextpc_consts;
  vector<expr_ref> m_insn_id_consts;
  vector<pair<string, expr_ref> > m_addr_refs;
  vector<pair<reg_type, long> > m_addr_refs_reg_type_and_id;
  expr_ref m_nextpc_indir_target_const;
  expr_ref m_input_stack_pointer_const;
  //expr_ref m_memzero;
  vector<expr_ref> m_function_call_identifier_regouts;
  vector<expr_ref> m_function_call_identifier_memouts;
  expr_ref m_function_call_identifier_regout_unknown_args;
  expr_ref m_function_call_identifier_memout_unknown_args;
  //vector<expr_ref> m_function_call_identifier_io_outs;
  map<expr_id_t, pair<expr_ref, cs_addr_ref_id_t>> m_expr_to_addr_ref_id_map; //expr_id --> pair(expr, addr_ref_id)
  map<expr_id_t, pair<expr_ref, constant_id_t>> m_expr_to_constant_id_map; //expr_id --> pair(expr, constant_id)
  //vector<expr_ref> m_argument_constants;
  expr_ref m_retaddr_const;
  vector<expr_ref> m_callee_save_consts;
  expr_ref m_input_memory_const;
  vector<expr_ref> m_sections;

  vector<memlabel_ref> relevant_memlabels;
  vector<string> relevant_memlabels_str;
  //relevant_memlabel_idx_t num_relevant_memlabels;
  //size_t relevant_memlabels_bvsize;
};

long expr_is_consts_struct_constant(expr_ref e, consts_struct_t const &cs);
long expr_is_consts_struct_addr_ref(expr_ref e, consts_struct_t const &cs);
bool const_id_is_independent_of_addr_ref_id(long const_id, long addr_ref_id, consts_struct_t const &cs);

extern string g_symbol_keyword;
extern string g_dst_symbol_addr_keyword;
extern string g_local_keyword;
extern string g_mem_symbol;
extern string g_stack_symbol;
extern string g_notstack_keyword;
extern string g_memlabel_top_symbol;
extern string g_regular_reg_name;
extern string g_invisible_reg_name;
extern string g_hidden_reg_name;

string reg_type_to_string(reg_type rt);

}
