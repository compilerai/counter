#ifndef EQCHECKSYM_EXEC_COMMON_H
#define EQCHECKSYM_EXEC_COMMON_H

#include "expr/expr.h"
#include "state_llvm.h"
#include "tfg/tfg.h"
#include "llvm/IR/Module.h"
/*#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"*/

#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/raw_ostream.h"
//#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
//#include "llvm/Analysis/sym_exec_common.h"

class sym_exec_common
{
public:
  sym_exec_common(context* ctx/*, consts_struct_t const &cs*/, shared_ptr<list<pair<string, unsigned>> const> fun_names, shared_ptr<graph_symbol_map_t const> symbol_map, shared_ptr<map<pair<symbol_id_t, offset_t>, vector<char>> const> string_contents, bool gen_callee_summary, unsigned memory_addressable_size, unsigned word_length) :
    m_ctx(ctx), m_cs(ctx->get_consts_struct()), m_fun_names(fun_names), m_symbol_map(symbol_map), m_string_contents(string_contents), m_gen_callee_summary(gen_callee_summary), m_memory_addressable_size(memory_addressable_size), m_word_length(word_length), m_mem_reg(G_SRC_KEYWORD "." LLVM_MEM_SYMBOL)/*, m_io_reg(LLVM_IO_SYMBOL)*/, m_local_num(graph_locals_map_t::first_non_arg_local()), m_memlabel_varnum(0)
  {
    //list<pair<string, unsigned>> fun_names;
    //sym_exec_common::get_fun_names(M, m_fun_names);

    //pair<graph_symbol_map_t, map<symbol_id_t, vector<char>>> symbol_map_and_string_contents = sym_exec_common::get_symbol_map_and_string_contents(M, fun_names);
    //m_symbol_map = make_shared<graph_symbol_map_t const>(symbol_map_and_string_contents.first);
    //m_string_contents = make_shared<map<symbol_id_t, vector<char>> const>(symbol_map_and_string_contents.second);
  }

  //void exec(const state& state_in, const llvm::Instruction& I/*, state& state_out, vector<control_flow_transfer>& cfts, bool &expand_switch_flag, unordered_set<predicate> &assumes*/, shared_ptr<tfg_node> from_node, llvm::BasicBlock const &B, llvm::Function const &F, size_t next_insn_id, tfg &t, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);

  unsigned get_word_length() const { return m_word_length; }
  unsigned get_memory_addressable_size() const { return m_memory_addressable_size; }

  sort_ref get_mem_domain() const;
  sort_ref get_mem_range() const;
  sort_ref get_mem_sort() const;

  virtual unique_ptr<tfg_llvm_t> get_tfg(map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> *function_tfg_map, set<string> const *function_call_chain, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap) = 0;
  virtual pc get_start_pc() const = 0;

  void get_tfg_common(tfg &t);

  static unique_ptr<tfg> get_preprocessed_tfg_using_callback_function(string const &name, context *ctx, list<pair<string, unsigned>> const &fun_names, graph_symbol_map_t const &symbol_map, map<pair<symbol_id_t, offset_t>, vector<char>> const &string_contents, consts_struct_t &cs, map<string, pair<callee_summary_t, unique_ptr<tfg>>> &function_tfg_map, set<string> function_call_chain, std::function<unique_ptr<tfg> (map<string, pair<callee_summary_t, unique_ptr<tfg>>> &, set<string> const &)> callback_f);

  static bool update_function_call_args_and_retvals_with_atlocals(unique_ptr<tfg> t_src);

  map<local_id_t, graph_local_t> const &get_local_refs() { return m_local_refs; }
  graph_symbol_map_t const &get_symbol_map() { return *m_symbol_map; }
  static string get_value_name(const llvm::Value& v);
  static list<pair<string, unsigned>> get_fun_names(llvm::Module const *M);
  static pair<graph_symbol_map_t, map<pair<symbol_id_t, offset_t>, vector<char>>> get_symbol_map_and_string_contents(llvm::Module const *M, list<pair<string, unsigned>> const &fun_names);
  static graph_symbol_map_t get_symbol_map(llvm::Module const *M);
  static map<pair<symbol_id_t, offset_t>, vector<char>> get_string_contents(llvm::Module const *M);
  static unsigned get_num_insn(const llvm::Function& f);
  context *get_context() const { return m_ctx; }
  list<pair<string, unsigned>> const &get_fun_names() const { return *m_fun_names; }
  bool gen_callee_summary() const { return m_gen_callee_summary; }

protected:
  te_comment_t phi_node_to_te_comment(bbl_order_descriptor_t const& bbo, int inum, llvm::Instruction const& I) const;
  te_comment_t instruction_to_te_comment(llvm::Instruction const& I, pc const& from_pc, bbl_order_descriptor_t const& bbo) const;
  expr_ref get_symbol_expr_for_global_var(string const &name, sort_ref const &sr);

  template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
  pair<shared_ptr<tfg_node>, map<std::string, sort_ref>>
  process_cft_first_half(tfg &t, shared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, state const &state_to, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, llvm::Instruction * I, const BASICBLOCK/*llvm::BasicBlock*/& B, const FUNCTION/*llvm::Function*/& F, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
  void
  process_cft_second_half(tfg &t, shared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, llvm::Instruction * I, const BASICBLOCK& B, const FUNCTION& F, map<std::string, sort_ref> const &phi_regnames, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);


  template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
  void process_cfts(tfg &t, shared_ptr<tfg_node> const &from_node, pc const &pc_to, state const &state_to, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, llvm::Instruction * I, vector<control_flow_transfer> const &cfts, BASICBLOCK const &B, FUNCTION const &F, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
  pair<shared_ptr<tfg_node>, map<std::string, sort_ref>>
  process_phi_nodes_first_half(tfg &t, const BASICBLOCK/*llvm::MachineBasicBlock*/* B_from, const pc& p_to, shared_ptr<tfg_node> const &from_node, const FUNCTION/*llvm::MachineFunction*/& F, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
  void process_phi_nodes_second_half(tfg &t, const BASICBLOCK* B_from, const pc& p_to, shared_ptr<tfg_node> const &from_node, const FUNCTION& F, expr_ref edgecond, map<string, sort_ref> const &phi_regnames, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, llvm::Instruction * I, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  virtual pair<expr_ref,unordered_set<expr_ref>> phiInstructionGetIncomingBlockValue(llvm::Instruction const &I/*, state const &start_state*/, shared_ptr<tfg_node> &pc_to_phi_node, pc const &pc_to, llvm::BasicBlock const *B_from, llvm::Function const &F, tfg &t)
  {
    NOT_REACHED(); //should be either overwritten or never called
  }
  virtual expr_ref phiInstructionGetIncomingBlockValue(llvm::MachineInstr const &I/*, state const &start_state*/, shared_ptr<tfg_node> &pc_to_phi_node, pc const &pc_to, llvm::MachineBasicBlock const *B_from, llvm::MachineFunction const &F, tfg &t)
  {
    NOT_REACHED(); //should be either overwritten or never called
  }
  template<typename FUNCTION, typename BASICBLOCK>
  BASICBLOCK const *get_basic_block_for_pc(const FUNCTION& F, pc const &p);
  virtual string functionGetName(llvm::Function const &F) const
  {
    NOT_REACHED();
  }
  virtual string functionGetName(llvm::MachineFunction const &F) const
  {
    NOT_REACHED();
  }
  virtual string get_basicblock_index(llvm::BasicBlock const &F) const
  {
    NOT_REACHED();
  }
  virtual string get_basicblock_index(llvm::MachineBasicBlock const &F) const
  {
    NOT_REACHED();
  }
  //virtual string get_basicblock_name(llvm::BasicBlock const &F) const
  //{
  //  NOT_REACHED();
  //}
  //virtual string get_basicblock_name(llvm::MachineBasicBlock const &F) const
  //{
  //  NOT_REACHED();
  //}
  virtual bool instructionIsPhiNode(llvm::Instruction const &I, string &varname) const
  {
    NOT_REACHED();
  }
  virtual bool instructionIsPhiNode(llvm::MachineInstr const &I, string &varname) const
  {
    NOT_REACHED();
  }

  static string gep_name_prefix(string const &name, pc const &from_pc, pc const &pc_to, int argnum);
  //expr_ref __get_expr_adding_edges_for_intermediate_vals_helper(const llvm::Value& v, string vname, const state& state_in, shared_ptr<tfg_node> *from_node, pc const &pc_to, llvm::BasicBlock const *B, llvm::Function const *F, tfg *t);
  bool function_belongs_to_program(string const &fun_name) const;
  string gep_instruction_get_intermediate_value_name(string base_name, unsigned index_counter, int intermediate_value_num);

  //llvm::BasicBlock const *get_basic_block_for_pc(const llvm::Function& F, pc const &p);

  //void apply_memcpy_function(const llvm::CallInst* c, expr_ref fun_name_expr, string const &fun_name, llvm::Function *F, state const &state_in, state &state_out/*, unordered_set<predicate> &assumes*/, string const &cur_function_name, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &curF, tfg &t, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);

  //static size_t function_get_num_bbls(const llvm::Function *F);
  //pair<callee_summary_t, tfg *> get_callee_summary(llvm::Function *F, string const &fun_name/*, map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &symbol_map*/, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);
  //void apply_general_function(const llvm::CallInst* c, expr_ref fun_name_expr, string const &fun_name, llvm::Function *F, state const &state_in, state &state_out/*, unordered_set<predicate> &assumes*/, string const &cur_function_name, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &curF, tfg &t, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);

  //void add_type_and_align_assumes(llvm::Value const &arg, expr_ref a, pc const&from_pc, tfg &t/*, unordered_set<predicate> &assumes*/, string langtype_comment);
  sort_ref get_fun_type_sort(/*const llvm::Type* t, */sort_ref ret_sort, const vector<sort_ref>& args) const;
  sort_ref get_type_sort(const llvm::Type* t, const llvm::DataLayout &dl) const;
  vector<sort_ref> get_type_sort_vec(const llvm::Type* t, const llvm::DataLayout &dl) const;
  sort_ref get_value_type(const llvm::Value& v, const llvm::DataLayout &dl) const;
  vector<sort_ref> get_value_type_vec(const llvm::Value& v, const llvm::DataLayout &dl) const;
  //string get_basicblock_name(const llvm::BasicBlock& v) const;
  //string get_basicblock_index(const llvm::BasicBlock& v) const;

  unsigned get_bv_bool_size(sort_ref e) const;

  expr_ref mk_fresh_expr(const string& name, const string& prefix, sort_ref s) const;

  //expr_ref get_const_value_expr(const llvm::Value& v, string vname, const state& state_in, shared_ptr<tfg_node> *from_node, pc const &pc_to, llvm::BasicBlock const *B, llvm::Function const *F, tfg *t);

  //expr_ref get_expr_adding_edges_for_intermediate_vals(const llvm::Value& v, string vname, const state& state_in, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t);

  void state_set_expr(state &st, string const &key, expr_ref const &value);
  expr_ref get_input_expr(string const &key, sort_ref const& s) const;
  expr_ref state_get_expr(state const &st, string const &key, sort_ref const& s) const;

  //void set_expr(const llvm::Value& v, expr_ref expr, state& st);
  //vector<expr_ref> get_expr_args(const llvm::Instruction& I, string vname, const state& st, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t/*, unordered_set<predicate> &assumes*/);

  //void add_gep_intermediate_vals(llvm::Instruction const &I, string const &name);
  void populate_state_template_common();
  void get_state_template(const pc& p, state& st);
  //expr_ref icmp_to_expr(llvm::ICmpInst::Predicate k, const vector<expr_ref>& args) const;

  pc get_pc_from_bbindex_and_insn_id(string const &bbindex, size_t insn_id) const;
  //pc get_pc_from_bb_and_insn_id(llvm::BasicBlock const &B, size_t insn_id);
  //vector<control_flow_transfer> expand_switch(tfg &t, shared_ptr<tfg_node> const &from_node, vector<control_flow_transfer> const &cfts, state const &state_to, const llvm::BasicBlock& B, const llvm::Function& F);

  //void process_cft(tfg &t, shared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, state const &state_to, const llvm::BasicBlock& B, const llvm::Function& F);
  //void add_edges(const llvm::BasicBlock& B, tfg& t, const llvm::Function& F, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);
  //void process_phi_nodes(tfg &t, const llvm::BasicBlock* B_from, const pc& p_to, shared_ptr<tfg_node> const &from_node, const llvm::Function& F, expr_ref edgecond);
  shared_ptr<tfg_node> get_next_intermediate_subsubindex_pc_node(tfg &t, shared_ptr<tfg_node> const &from_node);

  //expr_ref exec_gen_expr_casts(const llvm::CastInst& I, expr_ref arg, pc const &from_pc, tfg &t/*, unordered_set<predicate> &assumes*/);
  //static string getTypeString(llvm::Type *t);

  //expr_ref exec_gen_expr(const llvm::Instruction& I, string Iname, const vector<expr_ref>& args, state const &state_in, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t/*, map<string, expr_ref> &intermediate_values, unordered_set<predicate> &assumes*/);

  context* m_ctx;
  consts_struct_t const &m_cs;
  //const std::unique_ptr<llvm::Module>& m_module;
  shared_ptr<list<pair<string, unsigned>> const> m_fun_names;
  //std::map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &m_symbol_map;
  shared_ptr<graph_symbol_map_t const> m_symbol_map;
  shared_ptr<std::map<pair<symbol_id_t, offset_t>, vector<char>> const> m_string_contents;
  bool m_gen_callee_summary;
  unsigned m_memory_addressable_size;
  unsigned m_word_length;

  string m_mem_reg;
  //string m_io_reg;
  int m_local_num;
  //string m_ret_reg;
  map<local_id_t, graph_local_t> m_local_refs;
  //map<string, string> m_basicblock_name_map;
  map<string, int> m_basicblock_idx_map;
  list<pair<string, sort_ref>> m_state_templ;
  //map<string, string> m_value_name_map;
  map<string, pair<argnum_t, expr_ref>> m_arguments;
  map<string, callee_summary_t> m_callee_summaries;
  std::set<symbol_id_t> m_touched_symbols;
  int m_memlabel_varnum;
  //map<pc, pc> m_next_phi_pc;
  map<pc, int> m_intermediate_subsubindex_map;

  map<string_ref, bbl_order_descriptor_t> m_bbl_order_map; //map from bbl name to bbl_order_descriptor_t
};



#endif
