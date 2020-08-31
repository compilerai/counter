#ifndef EQCHECKSYM_EXEC_MIR_H
#define EQCHECKSYM_EXEC_MIR_H

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
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/WasmEHFuncInfo.h"
#include "llvm/CodeGen/WinEHFuncInfo.h"
//#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "sym_exec_common.h"


class sym_exec_mir : public sym_exec_common
{
public:
  sym_exec_mir(context* ctx, consts_struct_t const &cs, llvm::Module const *module, const llvm::Function& F, const llvm::MachineFunction &MF, list<pair<string, unsigned>> const &fun_names, graph_symbol_map_t const &symbol_map, map<pair<symbol_id_t, offset_t>, vector<char>> const &string_contents, bool gen_callee_summary, unsigned memory_addressable_size, unsigned word_length) :
    sym_exec_common(ctx, make_shared<list<pair<string, unsigned>> const>(sym_exec_common::get_fun_names(module)), make_shared<graph_symbol_map_t const>(sym_exec_common::get_symbol_map(module)), make_shared<map<pair<symbol_id_t, offset_t>, vector<char>> const>(sym_exec_common::get_string_contents(module)), gen_callee_summary, memory_addressable_size, word_length),
    m_module(module), m_function(F), m_machine_function(MF)
  {}

  //void exec(const state& state_in, const llvm::Instruction& I, shared_ptr<tfg_node> from_node, llvm::BasicBlock const &B, llvm::Function const &F, size_t next_insn_id, tfg &t, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);

  virtual unique_ptr<tfg_llvm_t> get_tfg(map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> *function_tfg_map, set<string> const *function_call_chain, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap);
  virtual pc get_start_pc() const;

  //static tfg *get_preprocessed_tfg(const llvm::Function& F, llvm::Module const *M, string const &name, context *ctx, list<pair<string, unsigned>> const &fun_names, map<symbol_id_t, tuple<string, size_t, bool>> const &symbol_map, map<symbol_id_t, vector<char>> const &string_contents, consts_struct_t &cs, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> function_call_chain, bool DisableModelingOfUninitVarUB);

  //static tfg *get_preprocessed_tfg_for_machine_function(llvm::MachineFunction const &mf, const llvm::Function& F, llvm::Module const *M, string const &name, context *ctx, list<pair<string, unsigned>> const &fun_names, map<symbol_id_t, tuple<string, size_t, bool>> const &symbol_map, map<symbol_id_t, vector<char>> const &string_contents, consts_struct_t &cs, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> function_call_chain, bool DisableModelingOfUninitVarUB);

  //static tfg *get_preprocessed_tfg_common(sym_exec_common &se, string const &name, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> function_call_chain, bool DisableModelingOfUninitVarUB);

  //static string get_value_name(const llvm::Value& v);

private:
  //virtual void process_phi_nodes(tfg &t, const llvm::MachineBasicBlock* B_from, const pc& p_to, shared_ptr<tfg_node> const &from_node, const llvm::MachineFunction& F, expr_ref edgecond) override;

  expr_ref mir_get_value_expr(llvm::MachineOperand const &v);

  void exec_mir(const state& state_in, const llvm::MachineInstr& I/*, state& state_out, vector<control_flow_transfer>& cfts, bool &expand_switch_flag, unordered_set<predicate> &assumes*/, shared_ptr<tfg_node> from_node, llvm::MachineBasicBlock const &B, size_t next_insn_id, tfg &t, map<string, pair<callee_summary_t, unique_ptr<tfg>>> &function_tfg_map, set<string> const &function_call_chain);
  std::string get_machine_basicblock_name(llvm::MachineBasicBlock const &mb) const;
  void populate_state_template_for_mf(llvm::MachineFunction const &mf);
  //expr_ref __get_expr_adding_edges_for_intermediate_vals_helper(const llvm::Value& v, string vname, const state& state_in, shared_ptr<tfg_node> *from_node, pc const &pc_to, llvm::BasicBlock const *B, llvm::Function const *F, tfg *t);
  
  //llvm::BasicBlock const *get_basic_block_for_pc(const llvm::Function& F, pc const &p);

  //void apply_memcpy_function(const llvm::CallInst* c, expr_ref fun_name_expr, string const &fun_name, llvm::Function *F, state const &state_in, state &state_out/*, unordered_set<predicate> &assumes*/, string const &cur_function_name, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &curF, tfg &t, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);

  //static size_t function_get_num_bbls(const llvm::Function *F);
  //pair<callee_summary_t, tfg *> get_callee_summary(llvm::Function *F, string const &fun_name, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);
  //void apply_general_function(const llvm::CallInst* c, expr_ref fun_name_expr, string const &fun_name, llvm::Function *F, state const &state_in, state &state_out, string const &cur_function_name, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &curF, tfg &t, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);
  //void add_type_and_align_assumes(llvm::Value const &arg, expr_ref a, pc const&from_pc, tfg &t, string langtype_comment);
  sort_ref get_mir_type_sort(const llvm::MachineOperand::MachineOperandType ty) const;
  void mir_set_expr(const llvm::MachineOperand& v, expr_ref expr, state& st);
  //sort_ref get_value_type(const llvm::Value& v) const;
  //string get_basicblock_name(const llvm::BasicBlock& v) const;
  string get_basicblock_index(const llvm::MachineBasicBlock& v) const;

  //expr_ref get_const_value_expr(const llvm::Value& v, string vname, const state& state_in, shared_ptr<tfg_node> *from_node, pc const &pc_to, llvm::BasicBlock const *B, llvm::Function const *F, tfg *t);

  //expr_ref get_expr_adding_edges_for_intermediate_vals(const llvm::Value& v, string vname, const state& state_in, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t);

  //void set_expr(const llvm::Value& v, expr_ref expr, state& st);
  //vector<expr_ref> get_expr_args(const llvm::Instruction& I, string vname, const state& st, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t/*, unordered_set<predicate> &assumes*/);

  //void add_gep_intermediate_vals(llvm::Instruction const &I, string const &name);
  //void populate_state_template(const llvm::Function& F);
  //expr_ref icmp_to_expr(llvm::ICmpInst::Predicate k, const vector<expr_ref>& args) const;

  //pc get_pc_from_bb_and_insn_id(llvm::BasicBlock const &B, size_t insn_id) const;
  //vector<control_flow_transfer> expand_switch(tfg &t, shared_ptr<tfg_node> const &from_node, vector<control_flow_transfer> const &cfts, state const &state_to, const llvm::BasicBlock& B, const llvm::Function& F);

  //void process_cft(tfg &t, shared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, state const &state_to, const llvm::BasicBlock& B, const llvm::Function& F);
  //void add_edges(const llvm::BasicBlock& B, tfg& t, const llvm::Function& F, map<string, pair<callee_summary_t, tfg *>> &function_tfg_map, set<string> const &function_call_chain);

  void add_edges_for_mir(const llvm::MachineBasicBlock& B, tfg& t, const llvm::MachineFunction& F, map<string, pair<callee_summary_t, unique_ptr<tfg>>> &function_tfg_map, set<string> const &function_call_chain);

  //expr_ref exec_gen_expr_casts(const llvm::CastInst& I, expr_ref arg, pc const &from_pc, tfg &t);
  //static string getTypeString(llvm::Type *t);

  //expr_ref exec_gen_expr(const llvm::Instruction& I, string Iname, const vector<expr_ref>& args, state const &state_in, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t);

  //static map<nextpc_id_t, callee_summary_t> get_callee_summaries_for_tfg(map<nextpc_id_t, string> const &nextpc_map, map<string, callee_summary_t> const &callee_summaries);

  llvm::Module const *m_module;
  llvm::Function const &m_function;
  llvm::MachineFunction const &m_machine_function;
};

#endif
