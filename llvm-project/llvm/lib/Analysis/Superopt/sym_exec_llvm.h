#ifndef EQCHECKSYM_EXEC_LLVM_H
#define EQCHECKSYM_EXEC_LLVM_H

#include "expr/expr.h"

#include "tfg/tfg.h"
#include "tfg/tfg_llvm.h"

#include "ptfg/llvm_value_id.h"
#include "ptfg/ftmap.h"

#include "state_llvm.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/ScalarEvolution.h"
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
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/WasmEHFuncInfo.h"
#include "llvm/CodeGen/WinEHFuncInfo.h"
//#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "sym_exec_common.h"

using value_scev_map_t = map<string, scev_toplevel_t<pc>>;

namespace eqspace {
  class llptfg_t;
}

class sym_exec_llvm : public sym_exec_common
{
public:
  sym_exec_llvm(context* ctx, llvm::Module const *module, llvm::Function& F, dshared_ptr<tfg_llvm_t const> src_llvm_tfg/*, bool gen_callee_summary*/, unsigned memory_addressable_size, unsigned word_length, string const& srcdst_keyword) :
    sym_exec_common(ctx, make_dshared<list<pair<string, unsigned>> const>(sym_exec_common::get_fun_names(module)), make_dshared<graph_symbol_map_t const>(sym_exec_common::get_symbol_map(module, src_llvm_tfg)), make_dshared<map<pair<symbol_id_t, offset_t>, vector<char>> const>(sym_exec_common::get_string_contents(module, src_llvm_tfg)), /*gen_callee_summary, */memory_addressable_size, word_length, srcdst_keyword),
    m_module(module), m_function(F), m_rounding_mode_at_start_pc(ctx->mk_rounding_mode_const(rounding_mode_t::round_to_nearest_ties_to_even()))
  {}
  virtual ~sym_exec_llvm() {}

  void exec(const state& state_in, const llvm::Instruction& I/*, state& state_out, vector<control_flow_transfer>& cfts, bool &expand_switch_flag, unordered_set<predicate> &assumes*/, dshared_ptr<tfg_node> from_node, llvm::BasicBlock const &B, llvm::Function const &F, size_t next_insn_id, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, bool model_llvm_semantics, tfg &t/*, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> *function_tfg_map*/, map<llvm_value_id_t, string_ref>* value_to_name_map/*, set<string> const *function_call_chain*/, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format);

  string get_cur_rounding_mode_varname() const;
  expr_ref get_cur_rounding_mode_var() const;

  static llvm_value_id_t get_llvm_value_id_for_value(llvm::Value const* v);
  static const llvm::Function *getParent(const llvm::Value *V);

  //unsigned get_word_length() const { return m_word_length; }
  //unsigned get_memory_addressable_size() const { return m_memory_addressable_size; }

  //sort_ref get_mem_domain() const;
  //sort_ref get_mem_range() const;

  static dshared_ptr<tfg_llvm_t> get_tfg(llvm::Function& F, llvm::Module const *M, string const &name, context *ctx, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, bool model_llvm_semantics, map<llvm_value_id_t, string_ref>* value_to_name_map, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap, map<string, value_scev_map_t> const& scev_map, string const& srcdst_keyword, context::xml_output_format_t xml_output_format);

  static pc get_start_pc(llvm::Function const& F);

  unordered_set<expr_ref> gen_arg_assumes() const;
  //void sym_exec_preprocess_tfg(string const &name, tfg_llvm_t &t_src, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> &function_tfg_map, list<string> const& sorted_bbl_indices, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, context::xml_output_format_t xml_output_format);

  //static dshared_ptr<tfg_llvm_t> get_preprocessed_tfg(llvm::Function& F, llvm::Module const *M, string const &name, context *ctx, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> &function_tfg_map, map<llvm_value_id_t, string_ref>* value_to_name_map, set<string> function_call_chain, bool gen_callee_summary, bool DisableModelingOfUninitVarUB, map<string, value_scev_map_t> const& scev_map, string const& srcdst_keyword, context::xml_output_format_t xml_output_format);

  //static dshared_ptr<tfg> get_preprocessed_tfg_for_machine_function(llvm::MachineFunction const &mf, const llvm::Function& F, llvm::Module const *M, string const &name, context *ctx, list<pair<string, unsigned>> const &fun_names, graph_symbol_map_t const &symbol_map, map<pair<symbol_id_t, offset_t>, vector<char>> const &string_contents, consts_struct_t &cs, map<string, pair<callee_summary_t, dshared_ptr<tfg>>> &function_tfg_map, set<string> function_call_chain, bool gen_callee_summary, bool DisableModelingOfUninitVarUB);

  //dshared_ptr<tfg_llvm_t> get_preprocessed_tfg_common(string const &name, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> &function_tfg_map, map<llvm_value_id_t, string_ref>* value_to_name_map, set<string> function_call_chain, list<string> const& sorted_bbl_indices, bool DisableModelingOfUninitVarUB, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format);

  //static bool update_function_call_args_and_retvals_with_atlocals(tfg *t_src);

  //list<pair<string, size_t>> const &get_local_refs() { return m_local_refs; }
  //map<symbol_id_t, tuple<string, size_t, bool>> const &get_symbol_map() { return m_symbol_map; }
  //static string get_value_name(const llvm::Value& v);
  //virtual void process_phi_nodes(tfg &t, const llvm::BasicBlock* B_from, const pc& p_to, shared_ptr<tfg_node> const &from_node, const llvm::Function& F, expr_ref edgecond) override;
  static dshared_ptr<ftmap_t> get_function_tfg_map(llvm::Module* M, set<string> FunNamesVec/*, bool DisableModelingOfUninitVarUB*/, context* ctx, dshared_ptr<llptfg_t const> const& src_llptfg, bool gen_scev, bool model_llvm_semantics, map<llvm_value_id_t, string_ref>* value_to_name_map = nullptr, context::xml_output_format_t xml_output_format = context::XML_OUTPUT_FORMAT_TEXT_NOCOLOR);
  static map<string, value_scev_map_t> sym_exec_populate_potential_scev_relations(llvm::Module* M, string const& srcdst_keyword);

  static scev_toplevel_t<pc> get_scev_toplevel(llvm::Instruction& I, llvm::ScalarEvolution * scev, llvm::LoopInfo const* loopinfo, string const& srcdst_keyword, size_t word_length);
private:
  string get_next_undef_varname();
  string get_poison_value_varname(string const& varname) const;
  expr_ref get_poison_value_var(string const& varname) const;
  void add_state_assume(string const& varname, expr_ref const& assume, state const& state_in, unordered_set<expr_ref>& assumes, dshared_ptr<tfg_node>& from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map);
  void transfer_poison_values(string const& varname, expr_ref const& e, unordered_set<expr_ref>& state_assumes, dshared_ptr<tfg_node>& node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map);
  void transfer_poison_value_on_load(string const& varname, expr_ref const& load_expr, unordered_set<expr_ref>& state_assumes, dshared_ptr<tfg_node>& node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map);
  void transfer_poison_value_on_store(expr_ref const& store_expr, unordered_set<expr_ref>& state_assumes, dshared_ptr<tfg_node>& node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map);
  static scev_op_t get_scev_op_from_scev_type(llvm::SCEVTypes scevtype);
  static mybitset get_mybitset_from_apint(llvm::APInt const& apint, uint32_t bitwidth, bool is_signed);
  static pair<mybitset, mybitset> get_bounds_from_range(llvm::ConstantRange const& crange, bool is_signed);
  static pc get_loop_pc(llvm::Loop const* L, string const& srcdst_keyword);
  static scev_with_bounds_t get_scev_with_bounds(llvm::ScalarEvolution& SE, llvm::SCEV const* scev, string const& srcdst_keyword, size_t word_length);
  static scev_ref get_scev(llvm::ScalarEvolution& SE, llvm::SCEV const* scev, string const& srcdst_keyword, size_t word_length);
  //virtual expr_ref phiInstructionGetIncomingBlockValue(llvm::Instruction const &I/*, state const &start_state*/, shared_ptr<tfg_node> &pc_to_phi_node, pc const &pc_to, llvm::BasicBlock const *B_from, llvm::Function const &F, tfg &t) override;
  pair<expr_ref,unordered_set<expr_ref>> phiInstructionGetIncomingBlockValue(llvm::Instruction const &I, dshared_ptr<tfg_node> &pc_to_phi_node/*, pc const &pc_to*/, llvm::BasicBlock const *B_from, llvm::Function const &F, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map);
  string functionGetName(llvm::Function const &F) const;
  static string get_basicblock_index(llvm::BasicBlock const &F);
  //virtual string get_basicblock_name(llvm::BasicBlock const &F) const override;
  bool instructionIsPhiNode(llvm::Instruction const &I, string &varname) const;

  //static string gep_name_prefix(string const &name, pc const &from_pc, pc const &pc_to, int argnum);
  //expr_ref __get_expr_adding_edges_for_intermediate_vals_helper(const llvm::Value& v, string vname, const state& state_in, shared_ptr<tfg_node> *from_node, pc const &pc_to, llvm::BasicBlock const *B, llvm::Function const *F, tfg& t);
  //pair<expr_ref,unordered_set<expr_ref>> __get_expr_adding_edges_for_intermediate_vals_helper(const llvm::Value& v/*, string vname*/, const state& state_in, unordered_set<expr_ref> const& state_assumes, shared_ptr<tfg_node> *from_node, pc const &pc_to, llvm::BasicBlock const *B, llvm::Function const *F, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map);
  //bool function_belongs_to_program(string const &fun_name) const;
  //string gep_instruction_get_intermediate_value_name(string base_name, unsigned index_counter, int intermediate_value_num);
  string llvm_instruction_get_md5sum_name(llvm::Instruction const& I) const;

  string gep_instruction_get_intermediate_value_name(llvm::Instruction const& I/*string base_name*/, unsigned index_counter, int intermediate_value_num) const;
  string constexpr_instruction_get_name(llvm::Instruction const& I) const;

  //llvm::BasicBlock const *get_basic_block_for_pc(const llvm::Function& F, pc const &p);

  pair<unordered_set<expr_ref>,unordered_set<expr_ref>> apply_memcpy_function(const llvm::CallInst* c, expr_ref fun_name_expr, string const &fun_name, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, llvm::Function *F, state const &state_in, state &state_out, unordered_set<expr_ref> const& assumes, string const &cur_function_name, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, llvm::Function const &curF, tfg &t/*, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> *function_tfg_map*/, map<llvm_value_id_t, string_ref>* value_to_name_map/*, set<string> const *function_call_chain*/, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format);

  static size_t function_get_num_bbls(const llvm::Function *F);
  pair<callee_summary_t, dshared_ptr<tfg_llvm_t>> const& get_callee_summary(llvm::Function *F, string const &fun_name, dshared_ptr<tfg_llvm_t const> src_llvm_tfg/*, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> &function_tfg_map*/, map<llvm_value_id_t, string_ref>* value_to_name_map/*, set<string> const &function_call_chain*/, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format);
  pair<unordered_set<expr_ref>,unordered_set<expr_ref>> apply_general_function(const llvm::CallInst* c, expr_ref fun_name_expr, string const &fun_name, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, llvm::Function *F, state const &state_in, state &state_out, unordered_set<expr_ref> const& assumes, string const &cur_function_name, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg &t/*, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> *function_tfg_map*/, map<llvm_value_id_t, string_ref>* value_to_name_map/*, set<string> const *function_call_chain*/, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format);
  //void add_align_assumes(std::string const &elname, llvm::Type *elTy/*llvm::Value const &arg*/, expr_ref a, pc const&from_pc, tfg &t);
  pair<unordered_set<expr_ref>,unordered_set<expr_ref>> apply_va_start_function(const llvm::CallInst* c, state const& state_in, state &state_out, unordered_set<expr_ref> const& state_assumes, string const& cur_function_name, dshared_ptr<tfg_node>& from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t,string_ref>* value_to_name_map);
  pair<unordered_set<expr_ref>,unordered_set<expr_ref>> apply_va_copy_function(const llvm::CallInst* c, state const& state_in, state &state_out, unordered_set<expr_ref> const& state_assumes, string const& cur_function_name, dshared_ptr<tfg_node>& from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t,string_ref>* value_to_name_map);

  unordered_set<expr_ref> gen_align_assumes(std::string const &elname, llvm::Type *elTy, sort_ref const& s) const;
  //void add_align_assumes(std::string const &elname, llvm::Type *elTy, expr_ref a, pc const&from_pc, tfg &t) const;

  expr_ref gen_shiftcount_assume_expr(expr_ref const& a, size_t shifted_val_size) const;
  expr_ref gen_dereference_assume_expr(expr_ref const& a) const;

  expr_ref gen_no_divbyzero_assume_expr(expr_ref const& a) const;
  expr_ref gen_div_no_overflow_assume_expr(expr_ref const& dividend, expr_ref const& divisor) const;

  void add_shiftcount_assume(expr_ref a, size_t shifted_val_size, pc const &from_pc, tfg &t) const;
  void add_dereference_assume(expr_ref a, pc const &from_pc, tfg &t);
  void add_divbyzero_assume(expr_ref a, pc const &from_pc, tfg &t) const;
  void add_div_no_overflow_assume(expr_ref dividend, expr_ref divisor, pc const &from_pc, tfg &t) const;

  llvm::BasicBlock const *get_basic_block_for_pc(const llvm::Function& F, pc const &p);

  //pair<shared_ptr<tfg_node>, map<std::string, sort_ref>>
  void
  process_phi_nodes(tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map, const llvm::BasicBlock* B_from, const pc& p_to, dshared_ptr<tfg_node> const &from_node, bool model_llvm_semantics, expr_ref edgecond, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, llvm::Instruction * I, const llvm::Function& F, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  //pair<shared_ptr<tfg_node>, map<std::string, sort_ref>>
  void
  process_cft(tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map, dshared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, state const &state_to, unordered_set<expr_ref> const& assumes, bool model_llvm_semantics, te_comment_t const& te_comment, llvm::Instruction * I, const llvm::BasicBlock& B, const llvm::Function& F, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  void
  process_cft_second_half(tfg &t, dshared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, llvm::Instruction * I, const llvm::BasicBlock & B, const llvm::Function& F, map<std::string, sort_ref> const &phi_regnames, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  void process_cfts(tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map, dshared_ptr<tfg_node> const &from_node, pc const &pc_to, state const &state_to, unordered_set<expr_ref> const& assumes, bool model_llvm_semantics, te_comment_t const& te_comment, llvm::Instruction * I, vector<control_flow_transfer> const &cfts, llvm::BasicBlock const &B, llvm::Function const &F, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  //void process_phi_nodes_second_half(tfg &t, const llvm::BasicBlock* B_from, const pc& p_to, shared_ptr<tfg_node> const &from_node, const llvm::Function& F, expr_ref edgecond, map<string, sort_ref> const &phi_regnames, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, llvm::Instruction * I, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  //sort_ref get_fun_type_sort(/*const llvm::Type* t, */sort_ref ret_sort, const vector<sort_ref>& args) const;
  //sort_ref get_type_sort(const llvm::Type* t) const;
  //sort_ref get_value_type(const llvm::Value& v) const;
  //virtual string get_basicblock_name(const llvm::BasicBlock& v) const override;
  //virtual string get_basicblock_index(const llvm::BasicBlock& v) const override;

  //unsigned get_bv_bool_size(sort_ref e) const;

  //expr_ref mk_fresh_expr(const string& name, const string& prefix, sort_ref s) const;

  pair<expr_ref,unordered_set<expr_ref>> get_const_value_expr(const llvm::Value& v, string const& vname, const state& state_in, unordered_set<expr_ref> const& state_assumes, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map);

  pair<expr_ref,unordered_set<expr_ref>> get_expr_adding_edges_for_intermediate_vals(const llvm::Value& v, string const& vname, const state& state_in, unordered_set<expr_ref> const& state_assumes, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map);

  //void state_set_expr(state &st, string const &key, expr_ref const &value);
  //expr_ref state_get_expr(state const &st, string const &key);

  void set_expr(string const &name/*const llvm::Value& v*/, expr_ref expr, state& st);
  pair<vector<expr_ref>,unordered_set<expr_ref>> get_expr_args(const llvm::Instruction& I, string const& vname, const state& st, unordered_set<expr_ref> const& state_assumes, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map);

  void add_gep_intermediate_vals(llvm::Instruction const &I, string const &name);
  void populate_state_template(const llvm::Function& F, bool model_llvm_semantics);
  expr_ref fcmp_to_expr(llvm::FCmpInst::Predicate cmp_kind, const vector<expr_ref>& args) const;
  expr_ref icmp_to_expr(llvm::ICmpInst::Predicate k, const vector<expr_ref>& args) const;
  expr::operation_kind farith_to_operation_kind(unsigned opcode, expr_vector const& args);

  //pc get_pc_from_bb_and_insn_id(llvm::BasicBlock const &B, size_t insn_id) const;
  vector<control_flow_transfer> expand_switch(tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map, dshared_ptr<tfg_node> const &from_node, vector<control_flow_transfer> const &cfts, state const &state_to, unordered_set<expr_ref> const& cond_assumes, te_comment_t const& te_comment, llvm::Instruction * I, const llvm::BasicBlock& B, const llvm::Function& F, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);

  void process_cft(tfg &t, dshared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, state const &state_to, unordered_set<expr_ref> const& assumes, const llvm::BasicBlock& B, const llvm::Function& F);
  void add_edges(const llvm::BasicBlock& B, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, bool model_llvm_semantics, tfg_llvm_t& t, const llvm::Function& F/*, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> *function_tfg_map*/, map<llvm_value_id_t, string_ref>* value_to_name_map/*, set<string> const *function_call_chain*/, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format);

  void sym_exec_populate_tfg_scev_map(tfg_llvm_t& t_src, value_scev_map_t const& value_scev_map) const;

  void parse_dbg_value_intrinsic(llvm::Instruction const& I, tfg_llvm_t& t, pc const& pc_from) const;
  void parse_dbg_declare_intrinsic(llvm::Instruction const& I, tfg_llvm_t& t, pc const& pc_from) const;
  void parse_dbg_label_intrinsic(llvm::Instruction const& I, tfg_llvm_t& t) const;
  //shared_ptr<tfg_node> get_next_intermediate_subsubindex_pc_node(tfg &t, shared_ptr<tfg_node> const &from_node);
  state parse_stacksave_intrinsic(llvm::Instruction const& I, tfg& t, pc const& pc_from);
  state parse_stackrestore_intrinsic(llvm::Instruction const& I, tfg& t, pc const& pc_from);
  //string get_local_alloc_count_varname() const;
  //string get_local_alloc_count_ssa_varname(pc const& p) const;

  pair<expr_ref,unordered_set<expr_ref>> exec_gen_expr_casts(const llvm::CastInst& I, expr_ref arg, unordered_set<expr_ref> const& state_assumes, pc const &from_pc, bool model_llvm_semantics, tfg &t);
  static string getTypeString(llvm::Type *t);

  pair<expr_ref,unordered_set<expr_ref>> exec_gen_expr(const llvm::Instruction& I, string const& Iname, const vector<expr_ref>& args, state const &state_in, unordered_set<expr_ref> const& state_assumes, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map);

  void populate_bbl_order_map();

  static map<nextpc_id_t, callee_summary_t> get_callee_summaries_for_tfg(map<nextpc_id_t, string> const &nextpc_map, map<string, callee_summary_t> const &callee_summaries);

  //const std::dshared_ptr<llvm::Module>& m_module;
  llvm::Module const *m_module;
  llvm::Function &m_function;
  map<string, llvm::BasicBlock const *> m_pc2bb_cache;

  map<string, expr_ref> m_opaque_varname_to_memalloc_map;

  //see https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-160 where it says that the value is ROUND_TO_NEAREST at the start of program execution (x86 calling conventions).  XXX: We are taking some liberty here by extending this assumption to the start of every function; a more precise way to model this would involve using a variable (instead of a constant) for the rounding mode at the start pc
  expr_ref m_rounding_mode_at_start_pc;

  set<string> m_poison_varnames_seen;

  int m_cur_undef_varname_idx = 0;
  //string const m_local_alloc_count_varname = "local_alloc_count";
};

#endif
