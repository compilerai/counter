#include "sym_exec_llvm.h"
#include "sym_exec_mir.h"
#include "dfa_helper.h"
//#include "../lib/Target/X86/X86CallLowering.h"
////#include "../lib/Target/X86/X86CallingConv.h"
//#include "../lib/Target/X86/X86ISelLowering.h"
//#include "../lib/Target/X86/X86InstrInfo.h"
//#include "../lib/Target/X86/X86RegisterInfo.h"
//#include "../lib/Target/X86/X86Subtarget.h"
#include "expr/expr.h"
#include "tfg/predicate.h"
#include "tfg/tfg_llvm.h"
#include "support/dyn_debug.h"
//#include "X86.h"
//#include "X86RegisterInfo.h"
//#include "X86Subtarget.h"
#include <tuple>

using namespace llvm;
static char as1[40960];


sort_ref sym_exec_common::get_mem_domain() const
{
  return m_ctx->mk_bv_sort(m_word_length);
}

sort_ref sym_exec_common::get_mem_range() const
{
  return m_ctx->mk_bv_sort(m_memory_addressable_size);
}

sort_ref sym_exec_common::get_mem_sort() const
{
  return m_ctx->mk_array_sort(get_mem_domain(), get_mem_range());
}

string prefix_identifier(const string& id, const string& prefix)
{
  return prefix + "." + id;
}

string
sym_exec_common::get_value_name(const Value& v)
{
  //errs() << "get_value_name: " << v << " ---- ";
  assert(!v.getType()->isVoidTy());

  string ret;
  raw_string_ostream ss(ret);
  v.printAsOperand(ss, false);
  ss.flush();
  //errs() << "name:" << ret << "\n";
  errs().flush();

  //if(m_value_name_map.count(ret) > 0)
  //  return m_value_name_map.at(ret);
  //else
  if (ret.substr(0, 1) == "@") {
    return ret; //ret.substr(1);
  } else {
    return string(G_SRC_KEYWORD "." G_LLVM_PREFIX) + "-" + ret;
  }
}

//expr_ref
//sym_exec_mir::get_mir_frameindex_var_expr(int frameindex)
//{
//  NOT_IMPLEMENTED();
//  //return m_ctx->mk_var(name, m_ctx->mk_bv_sort(DWORD_LEN));
//}

expr_ref
sym_exec_mir::mir_get_value_expr(MachineOperand const &v)
{
  NOT_IMPLEMENTED();
  //if (v.getType() == MachineOperand::MO_GlobalAddress) {
  //  string name = this->get_value_name(*v.getGlobal());
  //  ASSERT(name.substr(0, strlen(LLVM_GLOBAL_VARNAME_PREFIX)) == LLVM_GLOBAL_VARNAME_PREFIX);
  //  name = name.substr(strlen(LLVM_GLOBAL_VARNAME_PREFIX));
  //  sort_ref sr = get_value_type(*v.getGlobal());
  //  return get_symbol_expr_for_global_var(name, sr);
  //  //errs() << ": parsing global address: " << v << "\n";
  //} else if (v.getType() == MachineOperand::MO_FrameIndex) {
  //  NOT_IMPLEMENTED();
  //  //return get_mir_frameindex_var_expr(v.GetIndex());
  //} else {
  //  NOT_IMPLEMENTED();
  //}
}

void sym_exec_mir::exec_mir(const state& state_in, const llvm::MachineInstr& I/*, state& state_out, vector<control_flow_transfer>& cfts, bool &expand_switch_flag, unordered_set<predicate> &assumes*/, shared_ptr<tfg_node> from_node, llvm::MachineBasicBlock const &B, size_t next_insn_id, tfg &t, map<string, pair<callee_summary_t, unique_ptr<tfg>>> &function_tfg_map, set<string> const &function_call_chain)
{
  NOT_IMPLEMENTED();
#if 0
  //CPP_DBG_EXEC(LLVM2TFG, errs() << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": sym exec doing: " << I << "\n");
  state state_out = state_in;
  vector<control_flow_transfer> cfts;
  //unordered_set<predicate> assumes;
  //bool expand_switch_flag = false;
  //bool cft_processing_needed = true;
  MachineFunction const &F = m_machine_function;
  //string const &cur_function_name = F.getName(); //NOT_IMPLEMENTED
  pc pc_to = get_pc_from_bbindex_and_insn_id(get_basicblock_index(B), next_insn_id);
  errs() << "exec_mir: " << I << "\n";

  switch(I.getOpcode())
  {
  case X86::MOV32rm:
  case X86::MOV32ri: {
    const auto &dstOp = I.getOperand(0);
    ASSERT(dstOp.getType() == MachineOperand::MO_Register);
    const auto &srcOp = I.getOperand(1);
    expr_ref insn_expr = mir_get_value_expr(srcOp);
    mir_set_expr(dstOp, insn_expr, state_out);
  }
  case X86::MOV32r0: {
    const auto &dstOp = I.getOperand(0);
    ASSERT(dstOp.getType() == MachineOperand::MO_Register);
    mir_set_expr(dstOp, m_ctx->mk_zerobv(DWORD_LEN), state_out);
    break;
  }
  default: {
    errs() << "unsupported opcode: " << I << "\n";
    NOT_IMPLEMENTED();
    break;
  }
  }

  errs() << __func__ << " " << __LINE__ << ": state_out =\n" << state_out.to_string_for_eq() << "\n";
  process_cfts<llvm::MachineFunction, llvm::MachineBasicBlock, llvm::MachineInstr>(t, from_node, pc_to, state_out, cfts, B, F);
  //CPP_DBG_EXEC(LLVM2TFG, errs() << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": sym exec done\n");
  ////t.add_assumes(pc_from, assumes);
#endif
}

string
sym_exec_mir::get_basicblock_index(const llvm::MachineBasicBlock &v) const
{
  string s = get_machine_basicblock_name(v);
  ASSERT(m_basicblock_idx_map.count(s));
  ASSERT(s.substr(0, 1) == "%");
  string s1 = s.substr(1);
  return s1;
}

expr_ref
sym_exec_common::get_symbol_expr_for_global_var(string const &name, sort_ref const &sr)
{
  expr_ref ret;
  for (const auto &s : m_symbol_map->get_map()) {
    if (s.second.get_name()->get_str() == name) {
      symbol_id_t symbol_id = s.first;
      stringstream ss;
      ss << "symbol." << symbol_id;
      ret = m_ctx->mk_var(ss.str(), sr/*get_value_type(v)*/);
      m_touched_symbols.insert(symbol_id);
      break;
    }
  }
  return ret;
}

pair<expr_ref,unordered_set<expr_ref>>
sym_exec_llvm::get_const_value_expr(const llvm::Value& v, string vname, const state& state_in, unordered_set<expr_ref> const& state_assumes, shared_ptr<tfg_node> *from_node, pc const &pc_to, llvm::BasicBlock const *B, llvm::Function const *F, tfg &t)
{
  assert(isa<const llvm::Constant>(&v));
  if(const ConstantInt* c = dyn_cast<const ConstantInt>(&v))
  {
    unsigned size = c->getBitWidth();
    if(size > 1)
      return make_pair(m_ctx->mk_bv_const(size, c->getZExtValue()), state_assumes);
    else if(c->getZExtValue() == 0)
      return make_pair(m_ctx->mk_bool_false(), state_assumes);
    else
      return make_pair(m_ctx->mk_bool_true(), state_assumes);
  }
  else if (ConstantExpr const* ce = (ConstantExpr const*)dyn_cast<const ConstantExpr>(&v))
  {
    Instruction *i_sp = ce->getAsInstruction();
    ASSERT(from_node);
    ASSERT(B);
    ASSERT(F);
    vector<expr_ref> expr_args;
    unordered_set<expr_ref> assumes = state_assumes;
    tie(expr_args, assumes) = get_expr_args(*i_sp, vname, state_in, assumes, *from_node, pc_to, *B, *F, t);
    expr_ref ce_expr;
    tie(ce_expr, assumes) = exec_gen_expr(*i_sp, vname, expr_args, state_in, assumes, *from_node, pc_to, *B, *F, t);
    DYN_DEBUG(llvm2tfg, errs() << "ce_expr:" << m_ctx->expr_to_string(ce_expr) << "\n");
    i_sp->deleteValue();
    return make_pair(ce_expr, assumes);
  }
  else if(isa<const ConstantPointerNull>(&v))
  {
    return make_pair(m_ctx->mk_zerobv(get_word_length()), state_assumes);
  }
  else if(const GlobalValue* c = dyn_cast<const GlobalValue>(&v))
  {
    expr_ref ret;

    string name = get_value_name(v);
    sort_ref sr = get_value_type(v, m_module->getDataLayout());
    ASSERT(name.substr(0, strlen(LLVM_GLOBAL_VARNAME_PREFIX)) == LLVM_GLOBAL_VARNAME_PREFIX);
    name = name.substr(strlen(LLVM_GLOBAL_VARNAME_PREFIX));
    ASSERT(!ret);
    ret = get_symbol_expr_for_global_var(name, sr);
    if (!ret) {
      cout << __func__ << " " << __LINE__ << ": Warning: symbol not recognized. name = " << name << endl;
      cout << __func__ << " " << __LINE__ << ": sort = " << get_value_type(v, m_module->getDataLayout())->to_string() << endl;
      for (const auto &s : m_symbol_map->get_map()) {
        cout << __func__ << " " << __LINE__ << ": sym = " << s.second.get_name()->get_str() << endl;
      }
      NOT_REACHED(); //could be a function pointer!
      //ret = m_ctx->mk_var(name, get_value_type(v));
    }
    ASSERT(ret);

    //m_symbol_map[name] = make_pair(symbol_size, symbol_is_constant);
    return make_pair(ret, state_assumes);
  }
  else if(const UndefValue* c = dyn_cast<const UndefValue>(&v))
  {
    expr_ref ret = m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + get_value_name(v), get_value_type(v, m_module->getDataLayout()));
    return make_pair(ret, state_assumes);
  }

  DYN_DEBUG(llvm2tfg, errs() << "const to expr not handled:"  << get_value_name(v) << "\n");
  string unsupported_value_name = get_value_name(v);
  string_replace(unsupported_value_name, "-", ".minus.");
  string_replace(unsupported_value_name, "+", ".plus.");

  expr_ref ret = m_ctx->mk_var("unsupported." + unsupported_value_name, get_value_type(v, m_module->getDataLayout()));
  //assert(false && "const to expr not handled");
  return make_pair(ret, state_assumes);
}

sort_ref sym_exec_common::get_fun_type_sort(/*const llvm::Type* t, */sort_ref ret_sort, const vector<sort_ref>& args) const
{
  //if(const FunctionType* f = dyn_cast<const FunctionType>(t))
  {
    //sort_ref ret_sort = get_type_sort(f->getReturnType());
    /*if(!f->isVarArg())
    {
      vector<sort_ref> args;
      for(const Type* t_arg : f->params())
        args.push_back(get_type_sort(t_arg));
      return m_ctx->mk_function_sort(args, ret_sort);
    }
    else*/
      return m_ctx->mk_function_sort(args, ret_sort);
  }
  /*else
    unreachable("type unhandled");*/
}

sort_ref sym_exec_mir::get_mir_type_sort(const MachineOperand::MachineOperandType ty) const
{
  ASSERT(   false
         || ty == MachineOperand::MO_Register
         || ty == MachineOperand::MO_Immediate
         || ty == MachineOperand::MO_CImmediate
         || ty == MachineOperand::MO_FPImmediate
  );
  unsigned size = DWORD_LEN /*XXX: ty.getSizeInBits()*/;
  return m_ctx->mk_bv_sort(size);
}

vector<sort_ref> sym_exec_common::get_type_sort_vec(const llvm::Type* t, DataLayout const &dl) const
{
  vector<sort_ref> sv;
  if(const IntegerType* itype = dyn_cast<const IntegerType>(t))
  {
    unsigned size = itype->getBitWidth();
    if(size == 1) {
      sv.push_back(m_ctx->mk_bool_sort());
    } else {
      sv.push_back(m_ctx->mk_bv_sort(size));
    }
    return sv;
  }
  else if (isa<const PointerType>(t)) {
    sv.push_back(m_ctx->mk_bv_sort(get_word_length()));
    return sv;
  } else if (t->getTypeID() == Type::FloatTyID) {
    sv.push_back(m_ctx->mk_bv_sort(DWORD_LEN));
    return sv;
  } else if (t->getTypeID() == Type::DoubleTyID) {
    sv.push_back(m_ctx->mk_bv_sort(QWORD_LEN));
    return sv;
  } else if (t->getTypeID() == Type::StructTyID) {
    //const DataLayout &dl = m_module->getDataLayout();
    StructType const *sty = dyn_cast<StructType>(t);
    StructLayout const *sl = dl.getStructLayout((StructType *)sty);
    for (unsigned i = 0, e = sty->getNumElements(); i != e; i++) {
      Type *elTy = sty->getElementType(i);
      vector<sort_ref> elsv = get_type_sort_vec(elTy, dl);
      ASSERT(elsv.size() == 1);
      sv.push_back(elsv.at(0));
      //elTy->print(errs());
      //errs() << "\n";
    }
    return sv;
    //t->print(errs());
    //errs() << "\n";
    //errs().flush();
    //NOT_IMPLEMENTED();
  } else if (t->getTypeID() == Type::VectorTyID) {
    t->print(errs());
    errs().flush();
    NOT_IMPLEMENTED();
  } else {
    t->print(errs());
    errs().flush();
    unreachable("type unhandled");
  }
}

sort_ref sym_exec_common::get_type_sort(const llvm::Type* t, DataLayout const &dl) const
{
  vector<sort_ref> sv = get_type_sort_vec(t, dl);
  if (sv.size() != 1) {
    errs() << __func__ << " " << __LINE__ << ": get_type_sort_vec() returned " << sv.size() << "-sized vector for type ";
    t->print(errs());
    errs() << "\n";
    errs().flush();
  }
  ASSERT(sv.size() == 1);
  return sv.at(0);
}

string
sym_exec_llvm::getTypeString(Type *T)
{
  string Result;
  raw_string_ostream Tmp(Result);
  Tmp << *T;
  return Tmp.str();
}

unsigned sym_exec_common::get_bv_bool_size(sort_ref e) const
{
  if(e->is_bv_kind())
    return e->get_size();
  else if(e->is_bool_kind())
    return 1;
  else
    unreachable();
}

sort_ref sym_exec_common::get_value_type(const Value& v, DataLayout const &dl) const
{
  return get_type_sort(v.getType(), dl);
}

vector<sort_ref> sym_exec_common::get_value_type_vec(const Value& v, DataLayout const &dl) const
{
  return get_type_sort_vec(v.getType(), dl);
}



expr_ref sym_exec_common::mk_fresh_expr(const string& name, const string& prefix, sort_ref s) const
{
  string id = prefix_identifier(name, prefix);
  return m_ctx->mk_var(id, s);
}

string
sym_exec_common::gep_instruction_get_intermediate_value_name(string base_name, unsigned index_counter, int intermediate_value_num)
{
  stringstream ss;
  ASSERT(intermediate_value_num == 0 || intermediate_value_num == 1);
  if (intermediate_value_num == 0) {
    ss << base_name << "." << LLVM_INTERMEDIATE_VALUE_KEYWORD << "." << GEPOFFSET_KEYWORD << "." << index_counter << "." << "offset";
  } else {
    ss << base_name << ".gepoffset." << index_counter << "." << "total_offset";
  }
  //m_state_templ.push_back(make_pair(ss.str(), m_ctx->mk_bv_sort(DWORD_LEN)));
  return ss.str();
}

/*void
sym_exec_llvm::add_gep_intermediate_vals(Instruction const &I, string const &name)
{
  GetElementPtrInst const *gep = cast<GetElementPtrInst const>(&I);
  gep_type_iterator itype_begin = gep_type_begin(gep);
  gep_type_iterator itype_end = gep_type_end(gep);
  unsigned index_counter = 1;
  sort_ref offset_sort = m_ctx->mk_bv_sort(get_word_length());
  for (gep_type_iterator itype = itype_begin; itype != itype_end; itype++, index_counter++) {
    string offset_name = gep_instruction_get_intermediate_value_name(name, index_counter, 0);
    m_state_templ.push_back({offset_name, offset_sort});
    string total_offset_name = gep_instruction_get_intermediate_value_name(name, index_counter, 1);
    m_state_templ.push_back({total_offset_name, offset_sort});
    //errs() << "added " << offset_name << " and " << total_offset_name << " to state_templ\n";
  }
  string total_offset_name = gep_instruction_get_intermediate_value_name(name, index_counter, 1);
  m_state_templ.push_back({total_offset_name, offset_sort});
}*/

string
sym_exec_common::gep_name_prefix(string const &name, pc const &from_pc, pc const &to_pc, int argnum)
{
  stringstream ss;
  ss << G_SRC_KEYWORD "." G_LLVM_PREFIX "-%" << name << "." << argnum << "." << from_pc.to_string() << "." << to_pc.to_string();
  return ss.str();
}

void sym_exec_common::populate_state_template_common()
{
  //for(const pair<string, sort_ref>& item : m_state_templ)
  //{
  //  if (return_reg_sort.get()) {
  //    assert(item.first != m_ret_reg && "return reg-name already used");
  //  }
  //  assert(item.first != m_mem_reg && "mem name already used");
  //}

  sort_ref mem_sort = m_ctx->mk_array_sort(get_mem_domain(), get_mem_range());

  //m_io_reg = G_IO_SYMBOL;
  //sort_ref io_sort = m_ctx->mk_io_sort();

  //if (return_reg_sort.get()) {
  //  m_state_templ.push_back({m_ret_reg, return_reg_sort});
  //  //m_value_name_map[m_ret_reg] = m_ret_reg;
  //}
  //m_state_templ.push_back({G_SRC_KEYWORD "." G_LLVM_HIDDEN_REGISTER_NAME, m_ctx->mk_bv_sort(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS))});
  m_state_templ.push_back({m_mem_reg, mem_sort});
  //m_state_templ.push_back({m_io_reg, io_sort});

  //m_state_templ.push_back({G_SRC_KEYWORD "." LLVM_CONTAINS_FLOAT_OP_SYMBOL, m_ctx->mk_bool_sort()});

  //m_state_templ.push_back({G_SRC_KEYWORD "." LLVM_CONTAINS_UNSUPPORTED_OP_SYMBOL, m_ctx->mk_bool_sort()});

  //for (size_t i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
  //  stringstream ss;
  //  ss << G_SRC_KEYWORD "." LLVM_CALLEE_SAVE_REGNAME "." << i;
  //  m_state_templ.push_back({ss.str(), m_ctx->mk_bv_sort(DWORD_LEN)});
  //}

  //DYN_DEBUG(llvm2tfg,
  //  errs() << "\n==State template:\n";
  //  for (const pair<string, sort_ref>& item : m_state_templ) {
  //    errs() << item.first << " : " << item.second->to_string() << "\n";
  //  }
  //);
  //m_state_templ.push_back({G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID, m_ctx->mk_bv_sort(DWORD_LEN)});
  //errs() << "\n==Name mapping:\n";
  //for(auto name_mapping : m_value_name_map)
  //  errs() << name_mapping.first << " : " << name_mapping.second << "\n";

  /*errs() << "\n==Basicblock name mapping:\n";
  for (auto name_mapping : m_basicblock_name_map) {
    errs() << name_mapping.first << " : " << name_mapping.second << "\n";
  }*/
//  assert(m_value_name_map.size() == m_state_templ.size());

}

void sym_exec_llvm::populate_state_template(const llvm::Function& F)
{
  argnum_t argnum = 0;
  for(Function::const_arg_iterator iter = F.arg_begin(); iter != F.arg_end(); ++iter)
  {
    const Value& v = *iter;
    if (isa<const Constant>(v)) {
      continue;
    }
    string name = get_value_name(v);
    sort_ref s = get_value_type(v, m_module->getDataLayout());
    string argname = name + SRC_INPUT_ARG_NAME_SUFFIX;
    expr_ref argvar = m_ctx->mk_var(string(G_INPUT_KEYWORD) + "." + argname, s);
    m_arguments[name] = make_pair(argnum, argvar);
    argnum++;
    //m_state_templ.push_back({argname, s});
  }

  //int bbnum = 1; //bbnum == 0 is reserved
  for (const BasicBlock& B : F) {
  //  m_basicblock_idx_map[get_basicblock_name(B)] = bbnum;
  //  bbnum++;
  //  int insn_id = 0;
    for(const Instruction& I : B)
    {
  //    //pc pc_from = get_pc_from_bb_and_insn_id(B, insn_id);
  //    if (!isa<const PHINode>(I)) {
  //      insn_id++;
  //    }
      //if(isa<ReturnInst>(I))
      //{
      //  //const ReturnInst* ret = dyn_cast<const ReturnInst>(dyn_cast<const TerminatorInst>(&I));
      //  const ReturnInst *ret = dyn_cast<ReturnInst>(&I);
      //  if(Value* ret_val = ret->getReturnValue())
      //  {
      //    //m_ret_reg = G_LLVM_RETURN_REGISTER_NAME;
      //    sort_ref return_reg_sort = get_value_type(*ret_val, m_module->getDataLayout());
      //    m_state_templ.push_back({G_LLVM_RETURN_REGISTER_NAME/*m_ret_reg*/, return_reg_sort});
      //  }
      //  continue;
      //}
  //    if (isa<SwitchInst>(I)) {
  //      const SwitchInst* SI =  cast<const SwitchInst>(&I);
  //      size_t varnum = 0;
  //      for (SwitchInst::ConstCaseIt i = SI->case_begin(), e = SI->case_end(); i != e; ++i) {
  //        stringstream ss;
  //        ss << G_SRC_KEYWORD "." G_LLVM_PREFIX "-" LLVM_SWITCH_TMPVAR_PREFIX << varnum;
  //        varnum++;
  //        m_state_templ.push_back({ss.str(), m_ctx->mk_bool_sort()});
  //      }
  //      continue;
  //    }
  //    /*if (I.getOpcode() == Instruction::GetElementPtr) {
  //      add_gep_intermediate_vals(I, gep_name_prefix("gep", pc_from, 0));
  //    } else {
  //      for(unsigned i = 0; i < I.getNumOperands(); ++i) {
  //        Value const *op = I.getOperand(i);
  //        if (ConstantExpr *ce = (ConstantExpr *)dyn_cast<const ConstantExpr>(op)) {
  //          Instruction const &CI = *ce->getAsInstruction();
  //          if (CI.getOpcode() == Instruction::GetElementPtr) {
  //            add_gep_intermediate_vals(CI, gep_name_prefix("const_operand", pc_from, i));
  //          }
  //        }

  //      }
  //    }*/
  //    if (I.getType()->isVoidTy())
  //      continue;
  //    string name = get_value_name(I);
  //    vector<sort_ref> sv = get_value_type_vec(I, m_module->getDataLayout());
  //    if (sv.size() == 1) {
  //      m_state_templ.push_back({name, sv.at(0)});
  //    } else {
  //      for (size_t i = 0; i < sv.size(); i++) {
  //        stringstream ss;
  //        ss << name << "." LLVM_FIELDNUM_PREFIX << i;
  //        m_state_templ.push_back({ss.str(), sv.at(i)});
  //      }
  //    }
  //    //errs() << "adding field " << name << "\n";
  //    if (isa<const PHINode>(I)) {
  //      if (sv.size() > 1) {
  //        NOT_IMPLEMENTED();
  //      }
  //      m_state_templ.push_back({name + PHI_NODE_TMPVAR_SUFFIX, sv.at(0)});
  //      //errs() << "adding field " << name + PHI_NODE_TMPVAR_SUFFIX << "\n";
  //    }
  //    //if (I.getOpcode() == Instruction::Call) {
  //    //  const CallInst* c =  cast<const CallInst>(&I);
  //    //  for (const auto &arg : c->arg_operands()) {
  //    //    string name_copy = LLVM_FCALL_ARG_COPY_PREFIX + get_value_name(*arg);
  //    //    sort_ref s = get_value_type(*arg);
  //    //    m_state_templ.push_back({name_copy, s});
  //    //  }
  //    //}
    }
  }
  populate_state_template_common();
}

void sym_exec_common::get_state_template(const pc& p, state& st)
{
  //string src_dst_prefix = m_is_src ? "src" : "dst";
  string src_dst_prefix = "src";
  string prefix = src_dst_prefix + p.to_string();
  if (p.is_start()) {
    //prefix = "input";
    prefix = G_INPUT_KEYWORD;
  }

  map<string_ref, expr_ref> value_expr_map = st.get_value_expr_map_ref();
  for (const pair<string, sort_ref>& item : m_state_templ) {
    string name = item.first;
    //if (m_value_name_map.count(name) > 0) {
    //  name = m_value_name_map.at(name);
    //}
    sort_ref s = item.second;
    value_expr_map.insert(make_pair(mk_string_ref(name), mk_fresh_expr(name, prefix, s)));
  }
  st.set_value_expr_map(value_expr_map);
}

pair<expr_ref,unordered_set<expr_ref>>
sym_exec_llvm::__get_expr_adding_edges_for_intermediate_vals_helper(const Value& v, string vname, const state& state_in, unordered_set<expr_ref> const& state_assumes, shared_ptr<tfg_node> *from_node, pc const &pc_to, llvm::BasicBlock const *B, llvm::Function const *F, tfg& t)
{
  if (isa<const UndefValue>(&v)) {
    sort_ref s = get_type_sort(v.getType(), m_module->getDataLayout());
    if (!(s->is_bv_kind() || s->is_bool_kind())) {
      cout << __func__ << " " << __LINE__ << ": s = " << s->to_string() << endl;
    }
    ASSERT(s->is_bv_kind() || s->is_bool_kind());
    expr_ref a = m_ctx->mk_var(LLVM_UNDEF_VARIABLE_NAME, s);
    return make_pair(a, state_assumes);
  } else if (isa<const Constant>(&v)) {
    return get_const_value_expr(v, vname, state_in, state_assumes, from_node, pc_to, B, F, t);
  } else if (isa<const Argument>(&v)) {
    ASSERT(m_arguments.count(get_value_name(v)));
    return make_pair(m_arguments.at(get_value_name(v)).second, state_assumes);
  } else if (isa<const Instruction>(&v)) {
    vector<sort_ref> sv = get_value_type_vec(v, m_module->getDataLayout());
    ASSERT(sv.size() == 1);
    return make_pair(state_get_expr(state_in, get_value_name(v), sv.at(0)), state_assumes);
  } else {
    assert(false && "value not converted to expr");
  }
}

pair<expr_ref,unordered_set<expr_ref>>
sym_exec_llvm::get_expr_adding_edges_for_intermediate_vals(const Value& v, string vname, const state& state_in, unordered_set<expr_ref> const& assumes, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t)
{
  return __get_expr_adding_edges_for_intermediate_vals_helper(v, vname, state_in, assumes, &from_node, pc_to, &B, &F, t);
}

void
sym_exec_common::state_set_expr(state &st, string const &key, expr_ref const &value)
{
  st.set_expr_in_map(key, value);
}

expr_ref
sym_exec_common::get_input_expr(string const &key, sort_ref const& s) const
{
  stringstream ss;
  ss << G_INPUT_KEYWORD "." << key;
  return m_ctx->mk_var(ss.str(), s);
}

expr_ref
sym_exec_common::state_get_expr(state const &st, string const &key, sort_ref const& s) const
{
  if (st.has_expr(key)) {
    return st.get_expr(key, st);
  } else {
    return get_input_expr(key, s);
  }
}

void sym_exec_llvm::set_expr(string const &name/*, const llvm::Value& v*/, expr_ref expr, state& st)
{
  //state_set_expr(st, get_value_name(v), expr);
  state_set_expr(st, name, expr);
}

void sym_exec_mir::mir_set_expr(const llvm::MachineOperand& v, expr_ref expr, state& st)
{
  NOT_IMPLEMENTED();
  //string name;
  //raw_string_ostream ss(name);
  //v.print(ss);
  //ss.flush();
  //MachineOperand::MachineOperandType type = v.getType();
  //sort_ref s = get_mir_type_sort(type);

  //m_state_templ.push_back({name, s});
  //state_set_expr(st, name, expr);
}



expr::operation_kind icmp_kind_to_expr_kind(ICmpInst::Predicate cmp_kind)
{
  switch(cmp_kind)
  {
  case ICmpInst::ICMP_EQ:
  case ICmpInst::ICMP_NE:
    return expr::OP_EQ;
  case ICmpInst::ICMP_UGT:
    return expr::OP_BVUGT;
  case ICmpInst::ICMP_UGE:
    return expr::OP_BVUGE;
  case ICmpInst::ICMP_ULT:
    return expr::OP_BVULT;
  case ICmpInst::ICMP_ULE:
    return expr::OP_BVULE;
  case ICmpInst::ICMP_SGT:
    return expr::OP_BVSGT;
  case ICmpInst::ICMP_SGE:
    return expr::OP_BVSGE;
  case ICmpInst::ICMP_SLT:
    return expr::OP_BVSLT;
  case ICmpInst::ICMP_SLE:
    return expr::OP_BVSLE;
  default:
    assert(false);
  }
}

expr::operation_kind binary_op_to_expr_kind(Instruction::BinaryOps kind, bool is_bool)
{
  switch(kind)
  {
  case BinaryOperator::Add:
    return expr::OP_BVADD;
  case BinaryOperator::Sub:
    return expr::OP_BVSUB;
  case BinaryOperator::Mul:
    return expr::OP_BVMUL;
  case BinaryOperator::SDiv:
    return expr::OP_BVSDIV;
  case BinaryOperator::UDiv:
    return expr::OP_BVUDIV;
  case BinaryOperator::SRem:
    return expr::OP_BVSREM;
  case BinaryOperator::URem:
    return expr::OP_BVUREM;
  case BinaryOperator::Shl:
    return expr::OP_BVEXSHL;
  case Instruction::LShr:
    return expr::OP_BVEXLSHR;
  case Instruction::AShr:
    return expr::OP_BVEXASHR;
  case Instruction::And:
    if(is_bool)
      return expr::OP_AND;
    else
      return expr::OP_BVAND;
  case Instruction::Or:
    if(is_bool)
      return expr::OP_OR;
    else
      return expr::OP_BVOR;
  case Instruction::Xor:
    if(is_bool)
      return expr::OP_XOR;
    else
      return expr::OP_BVXOR;
  default:
    assert(false);
  }
}

expr_ref sym_exec_llvm::icmp_to_expr(ICmpInst::Predicate cmp_kind, const vector<expr_ref>& args) const
{
  expr_ref ret = m_ctx->mk_app(icmp_kind_to_expr_kind(cmp_kind), args);

  if(cmp_kind == ICmpInst::ICMP_NE)
    ret = m_ctx->mk_not(ret);

  return ret;
}

pair<vector<expr_ref>, unordered_set<expr_ref>>
sym_exec_llvm::get_expr_args(const llvm::Instruction& I, string vname, const state& st, unordered_set<expr_ref> const& state_assumes, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t)
{
  vector<expr_ref> args;
  unordered_set<expr_ref> assumes = state_assumes;
  pc orig_from_pc = from_node->get_pc();
  for (unsigned i = 0; i < I.getNumOperands(); ++i) {
    string avname = vname;
    if (avname == "") {
      avname = gep_name_prefix("const_operand", orig_from_pc, pc_to, i);
    }
    expr_ref arg;
    tie(arg, assumes) = get_expr_adding_edges_for_intermediate_vals(*I.getOperand(i), avname, st, assumes, from_node, pc_to, B, F, t);
    args.push_back(arg);
  }
  return make_pair(args, assumes);
}

pair<expr_ref,unordered_set<expr_ref>>
sym_exec_llvm::exec_gen_expr_casts(const llvm::CastInst& I, expr_ref arg, unordered_set<expr_ref> const& state_assumes, pc const &from_pc, tfg &t)
{
  unsigned src_size = get_bv_bool_size(get_type_sort(I.getSrcTy(), m_module->getDataLayout()));
  unsigned dst_size = get_bv_bool_size(get_type_sort(I.getDestTy(), m_module->getDataLayout()));
  expr_ref src_expr = arg;
  if(arg->is_bool_sort())
    src_expr = expr_bool_to_bv1(src_expr);

  switch(I.getOpcode())
  {
  case Instruction::SExt:
  {
    return make_pair(m_ctx->mk_bvsign_ext(src_expr, dst_size - src_size), state_assumes);
  }
  case Instruction::ZExt:
  {
    return make_pair(m_ctx->mk_bvzero_ext(src_expr, dst_size - src_size), state_assumes);
  }
  case Instruction::Trunc:
  {
    expr_ref ret_expr = m_ctx->mk_bvextract(src_expr, dst_size - 1, 0);
    if (ret_expr->get_sort()->get_size() == 1) {
      ret_expr = expr_bv1_to_bool(ret_expr);
    }
    return make_pair(ret_expr, state_assumes);
  }
  case Instruction::BitCast:  // it's a nop cast
  {
    assert(src_size == dst_size);
    unordered_set<expr_ref> assumes = state_assumes;
    //Value *v = I.getOperand(0);
    //Type *typ = I.getType();
    //string typeString = getTypeString(typ);
    //langtype_ref lt = mk_langtype_ref(typeString);
    //predicate p(precond_t(m_ctx), m_ctx->mk_islangtype(src_expr, lt), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_BITCAST_AFTERCAST_ISLANGTYPE, predicate::assume);
    //t.add_assume_pred(from_pc, p);
    //assumes.insert(p);
    //typeString = getTypeString(v->getType());
    //langtype_ref lt2 = mk_langtype_ref(typeString);
    //predicate p2(precond_t(m_ctx), m_ctx->mk_islangtype(src_expr, lt2), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_BITCAST_SRC_EXPR_ISLANGTYPE, predicate::assume);
    //t.add_assume_pred(from_pc, p2);
    //assumes.insert(p2);
    return make_pair(src_expr, assumes);
  }
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  {
    expr_ref after_cast = src_expr;
    if(dst_size < src_size)
      after_cast = m_ctx->expr_do_simplify(m_ctx->mk_bvextract(src_expr, dst_size - 1, 0));
    else if(dst_size > src_size)
      after_cast = m_ctx->expr_do_simplify(m_ctx->mk_bvzero_ext(src_expr, dst_size - src_size));

    unordered_set<expr_ref> assumes = state_assumes;
    Value *v = I.getOperand(0);
    //Type *typ = I.getType();
    //string typeString = getTypeString(typ);
    //langtype_ref lt = mk_langtype_ref(typeString);
    //string comment_opc;
    //if (I.getOpcode() == Instruction::PtrToInt) {
    //  comment_opc = "ptr2int";
    //} else if (I.getOpcode() == Instruction::IntToPtr) {
    //  comment_opc = "int2ptr";
    //} else NOT_REACHED();
    //predicate p(precond_t(m_ctx), m_ctx->mk_islangtype(after_cast, lt), expr_true(m_ctx), string(UNDEF_BEHAVIOUR_ASSUME_INTPTR_AFTERCAST_ISLANGTYPE "-") + comment_opc, predicate::assume);
    //t.add_assume_pred(from_pc, p);
    //assumes.insert(p);
    //typeString = getTypeString(v->getType());
    //langtype_ref lt2 = mk_langtype_ref(typeString);
    //predicate p2(precond_t(m_ctx), m_ctx->mk_islangtype(src_expr, lt2), expr_true(m_ctx), string(UNDEF_BEHAVIOUR_ASSUME_INTPTR_SRC_EXPR_ISLANGTYPE "-") + comment_opc, predicate::assume);
    //t.add_assume_pred(from_pc, p2);
    //assumes.insert(p2);

    return make_pair(after_cast, assumes);
  }
  case Instruction::AddrSpaceCast:
  default:
    printf("WARNING : casts not handled");
    return make_pair(src_expr, state_assumes);
  }
}

pair<unordered_set<expr_ref>,unordered_set<expr_ref>>
sym_exec_llvm::apply_memcpy_function(const CallInst* c, expr_ref fun_name_expr, string const &fun_name, Function *F, state const &state_in, state &state_out, unordered_set<expr_ref> const& state_assumes, string const &cur_function_name, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &curF, tfg &t, map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> *function_tfg_map, set<string> const *function_call_chain)
{
  const auto& memcpy_dst = c->getArgOperand(0);
  const auto& memcpy_src = c->getArgOperand(1);
  const auto& memcpy_nbytes = c->getArgOperand(2);
  const auto& memcpy_align = c->getArgOperand(3);
  pc const &from_pc = from_node->get_pc();
  //expr_ref memcpy_src_expr = get_expr_adding_edges_for_intermediate_vals(*memcpy_src, "", state_out, from_node, pc_to, B, curF, t/*, assumes*/);
  //expr_ref memcpy_dst_expr = get_expr_adding_edges_for_intermediate_vals(*memcpy_dst, "", state_out, from_node, pc_to, B, curF, t/*, assumes*/);
  //expr_ref memcpy_nbytes_expr = get_expr_adding_edges_for_intermediate_vals(*memcpy_nbytes, "", state_out, from_node, pc_to, B, curF, t/*, assumes*/);
  //expr_ref memcpy_align_expr = get_expr_adding_edges_for_intermediate_vals(*memcpy_align, "", state_out, from_node, pc_to, B, curF, t/*, assumes*/);
  //memlabel_t ml_top;
  unordered_set<expr_ref> assumes = state_assumes;
  expr_ref memcpy_src_expr, memcpy_dst_expr, memcpy_nbytes_expr, memcpy_align_expr;

  tie(memcpy_src_expr, assumes)    = get_expr_adding_edges_for_intermediate_vals(*memcpy_src, "", state_out, assumes, from_node, pc_to, B, curF, t);
  tie(memcpy_dst_expr, assumes)    = get_expr_adding_edges_for_intermediate_vals(*memcpy_dst, "", state_out, assumes, from_node, pc_to, B, curF, t);
  tie(memcpy_nbytes_expr, assumes) = get_expr_adding_edges_for_intermediate_vals(*memcpy_nbytes, "", state_out, assumes, from_node, pc_to, B, curF, t);
  tie(memcpy_align_expr, assumes)  = get_expr_adding_edges_for_intermediate_vals(*memcpy_align, "", state_out, assumes, from_node, pc_to, B, curF, t);

  int memcpy_align_int;
  if (memcpy_align_expr->is_bool_sort() || memcpy_align_expr->get_sort()->get_size() == 1) { //seems like the align operand was omitted; unsure if this is the correct check though.
    memcpy_align_int = 1;
  } else {
    ASSERT(memcpy_align_expr->is_const());
    int memcpy_align_int = memcpy_align_expr->get_int_value();
    if (memcpy_align_int == 0) {
      cout << __func__ << " " << __LINE__ << ": memcpy_nbytes_expr = " << expr_string(memcpy_nbytes_expr) << endl;
      cout << __func__ << " " << __LINE__ << ": memcpy_align_expr = " << expr_string(memcpy_align_expr) << endl;
    }
    ASSERT(memcpy_align_int > 0);
  }

  memlabel_t ml_top;
  memlabel_t::keyword_to_memlabel(&ml_top, G_MEMLABEL_TOP_SYMBOL, MEMSIZE_MAX);

  unordered_set<expr_ref> succ_assumes;
  if (memcpy_nbytes_expr->is_const()) {
    //predicate p_src(m_ctx->mk_islangaligned(memcpy_src_expr, memcpy_align_int), m_ctx->mk_bool_const(true), UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-align-memcpy-src-assume", predicate::assume);
    int count = memcpy_nbytes_expr->get_int_value();
    if (count != 0) {
      expr_ref const& src_isaligned_assume = m_ctx->mk_islangaligned(memcpy_src_expr, count /*XXX: not sure*/);
      assumes.insert(src_isaligned_assume);
      //predicate p_src(precond_t(m_ctx), isaligned_assume, expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ALIGN_MEMCPY_SRC, predicate::assume);
      //t.add_assume_pred(from_pc, p_src);
      ////predicate p_dst(m_ctx->mk_islangaligned(memcpy_dst_expr, memcpy_align_int), m_ctx->mk_bool_const(true), UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-align-memcpy-dst-assume", predicate::assume);
      expr_ref const& dst_isaligned_assume = m_ctx->mk_islangaligned(memcpy_dst_expr, count /*XXX: not sure*/);
      assumes.insert(dst_isaligned_assume);
      //predicate p_dst(precond_t(m_ctx), dst_isaligned_assume, expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ALIGN_MEMCPY_DST);
      //t.add_assume_pred(from_pc, p_dst);
    }

    for (int i = 0; i < count; i += memcpy_align_int) {
      expr_ref offset = m_ctx->mk_bv_const(DWORD_LEN, i);
      //cout << __func__ << " " << __LINE__ << ": count = " << count << ", memcpy_align_int = " << memcpy_align_int << ", i = " << i << endl;
      expr_ref inbytes = m_ctx->mk_select(state_get_expr(state_out, m_mem_reg, this->get_mem_sort()), ml_top, m_ctx->mk_bvadd(memcpy_src_expr, offset), memcpy_align_int, false/*, comment_t()*/);
      expr_ref out_mem = state_get_expr(state_out, m_mem_reg, this->get_mem_sort());
      out_mem = m_ctx->mk_store(out_mem, ml_top, m_ctx->mk_bvadd(memcpy_dst_expr, offset), inbytes, memcpy_align_int, false/*, comment_t()*/);
      state_set_expr(state_out, m_mem_reg, out_mem);
    }
  } else {
    //cout << __func__ << " " << __LINE__ << ": memcpy_nbytes_expr = " << expr_string(memcpy_nbytes_expr) << endl;
    tie(assumes, succ_assumes) = apply_general_function(c, fun_name_expr, fun_name, F, state_in, state_out, assumes, cur_function_name, from_node, pc_to, B, curF, t, function_tfg_map, function_call_chain);
  }
  return make_pair(state_assumes, succ_assumes);
}

size_t
sym_exec_llvm::function_get_num_bbls(const Function *F)
{
  size_t ret = 0;
  for(const BasicBlock& B : *F) {
    ret++;
  }
  return ret;
}

pair<callee_summary_t, unique_ptr<tfg_llvm_t>> const&
sym_exec_llvm::get_callee_summary(Function *F, string const &fun_name/*, map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &symbol_map*/, map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> &function_tfg_map, set<string> const &function_call_chain)
{
  //cout.flush();
  //cout << __func__ << " " << __LINE__ << endl;
  //cout.flush();
  if (function_tfg_map.count(fun_name)) {
    //cout << __func__ << " " << __LINE__ << endl;
    return function_tfg_map.at(fun_name);
  }
  //cout << __func__ << " " << __LINE__ << endl;
  context *ctx = g_ctx; //new context(context::config(600, 600));
  ctx->parse_consts_db(CONSTS_DB_FILENAME);
  //ctx->parse_consts_db("~/superopt/consts_db.in"); //XXX: fixme

  cout << __func__ << " " << __LINE__ << ": getting callee summary for " << fun_name << ": function_call_chain.size() = " << function_call_chain.size() << ", chain:";
  for (const auto &f : function_call_chain) {
    cout << " " << f;
  }
  cout << endl;
  unique_ptr<tfg_llvm_t> t = sym_exec_llvm::get_preprocessed_tfg(*F, m_module, fun_name, ctx, function_tfg_map, function_call_chain, this->gen_callee_summary(), false);
 cout << __func__ << " " << __LINE__ << ": done callee summary for " << fun_name << ": function_call_chain.size() = " << function_call_chain.size() << ", chain:";
  for (const auto &f : function_call_chain) {
    cout << " " << f;
  }
  cout << endl;
  //cout << __func__ << " " << __LINE__ << ": TFG =\n" << t->graph_to_string() << endl;
  auto ret = make_pair(t->get_summary_for_calling_functions(), std::move(t));
  function_tfg_map.insert(make_pair(fun_name, std::move(ret)));
  //delete ctx; //we are returning tfg which contains references to ctx, so do not delete ctx
  return function_tfg_map.at(fun_name);
}

bool
sym_exec_common::function_belongs_to_program(string const &fun_name) const
{
  for (auto f : *m_fun_names) {
    if (string(LLVM_FUNCTION_NAME_PREFIX) + f.first == fun_name) {
      return true;
    }
  }
  return false;
}

pair<unordered_set<expr_ref>,unordered_set<expr_ref>>
sym_exec_llvm::apply_general_function(const CallInst* c, expr_ref fun_name_expr, string const &fun_name, Function *F, state const &state_in, state &state_out, unordered_set<expr_ref> const& state_assumes, string const &cur_function_name, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &curF, tfg &t, map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> *function_tfg_map, set<string> const *function_call_chain)
{
  vector<expr_ref> args;
  vector<sort_ref> args_type;
  pc const &from_pc = from_node->get_pc();

  callee_summary_t csum;
  string fname = fun_name == "" ? "" : fun_name.substr(string(LLVM_FUNCTION_NAME_PREFIX).size());
  if (m_callee_summaries.count(fun_name)) {
    csum = m_callee_summaries.at(fun_name);
  } else if (   F != NULL
             && function_call_chain
             && function_belongs_to_program(fun_name)
             && !set_belongs(*function_call_chain, fname)
             && this->gen_callee_summary()) {
    ASSERT(fun_name.substr(0, string(LLVM_FUNCTION_NAME_PREFIX).size()) == LLVM_FUNCTION_NAME_PREFIX);
    pair<callee_summary_t, unique_ptr<tfg_llvm_t>> const& csum_callee_tfg = get_callee_summary(F, fname, *function_tfg_map, *function_call_chain);
    DYN_DEBUG(llvm2tfg, cout << "Doing resume: " << t.get_name() << endl);
    csum = csum_callee_tfg.first;
    tfg const& callee_tfg = *csum_callee_tfg.second;
    for (auto const& s : callee_tfg.get_symbol_map().get_map()) {
      if (!s.second.is_const()) { //do not include callee's const symbols because they are inconsequential (but are complicated to handle)
        m_touched_symbols.insert(s.first);
      }
    }
    m_callee_summaries[fname] = csum;
  } else {
    DYN_DEBUG(llvm2tfg, cout << "Could not get callee summary for " << fun_name << ": F = " << F << endl);
  }

  expr_ref ml_read_expr = m_ctx->memlabel_var(t.get_name()->get_str(), m_memlabel_varnum); //csum.get_read_memlabel();
  m_memlabel_varnum++;
  expr_ref ml_write_expr= m_ctx->memlabel_var(t.get_name()->get_str(), m_memlabel_varnum); //csum.get_write_memlabel();
  m_memlabel_varnum++;

  expr_ref mem = state_get_expr(state_in, m_mem_reg, this->get_mem_sort());
  expr_ref zerobv = m_ctx->mk_zerobv(DWORD_LEN);
  args.push_back(ml_read_expr);
  args_type.push_back(ml_read_expr->get_sort());
  args.push_back(ml_write_expr);
  args_type.push_back(ml_write_expr->get_sort());
  args.push_back(mem);
  args_type.push_back(mem->get_sort());


  args.push_back(fun_name_expr); //nextpc_const.<n>
  args_type.push_back(fun_name_expr->get_sort());
  expr_ref eax_regid = m_ctx->mk_regid_const(G_FUNCTION_CALL_REGNUM_EAX);
  size_t regnum_pos = args.size();
  args.push_back(eax_regid); //regnum eax
  args_type.push_back(eax_regid->get_sort());

  int argnum = 0;
  unordered_set<expr_ref> assumes = state_assumes;
  for (const auto& arg : c->arg_operands()) {
    expr_ref expr;
    tie(expr, assumes) = get_expr_adding_edges_for_intermediate_vals(*arg, gep_name_prefix("const_operand", from_pc, pc_to, argnum), state_in, assumes, from_node, pc_to, B, curF, t);
    args.push_back(expr);
    args_type.push_back(expr->get_sort());
    argnum++;
  }

  unordered_set<expr_ref> succ_assumes;
  if (!c->getType()->isVoidTy()) {
    Type *retType;
    if (F) {
      const llvm::FunctionType* ft = F->getFunctionType();
      const FunctionType* f = dyn_cast<const FunctionType>(ft);
      retType = f->getReturnType();
    } else {
      retType = c->getType();
    }
    const auto &dl = m_module->getDataLayout();
    vector<sort_ref> ret_sort_vec = get_type_sort_vec(retType, dl);
    for (size_t r = 0; r < ret_sort_vec.size(); r++) {
      sort_ref ret_sort = ret_sort_vec.at(r);
      size_t bvsize;
      if (ret_sort->is_bv_kind()) {
        bvsize = ret_sort->get_size();
      } else if (ret_sort->is_bool_kind()) {
        bvsize = 1;
      } else NOT_REACHED();

      expr_ref c_expr;
      expr_ref fun = m_ctx->get_fun_expr(args_type, ret_sort);
      expr_ref field_regid = m_ctx->mk_regid_const(G_FUNCTION_CALL_REGNUM_RETVAL(r));
      args[regnum_pos] = field_regid;
      c_expr = m_ctx->mk_function_call(fun, args);
      string Elname = get_value_name(*c);
      Type *ElTy = (*c).getType();
      if (ret_sort_vec.size() > 1) {
        stringstream ss;
        ss << Elname << "." LLVM_FIELDNUM_PREFIX << r;
        Elname = ss.str();
        StructType const *sty = dyn_cast<StructType>(ElTy);
        StructLayout const *sl = dl.getStructLayout((StructType *)sty);
        ASSERT(r < ret_sort_vec.size());
        ElTy = sty->getElementType(r);
      }

      set_expr(Elname, c_expr, state_out);
      //add_align_assumes(Elname, ElTy, c_expr, pc_to, t);
      unordered_set_union(succ_assumes, gen_align_assumes(Elname, ElTy, c_expr->get_sort()));
      DYN_DEBUG(llvm2tfg, errs() << "\n\nfun sort: " << m_ctx->expr_to_string_table(fun) << "\n");
    }
  }
  //sort_ref memfun_sort = get_fun_type_sort(/*ft, */mem->get_sort(), args_type);
  //sort_ref io_fun_sort = get_fun_type_sort(io->get_sort(), args_type);
  //string memfun_name = smt_fun_name + "." + G_MEM_SYMBOL;
  expr_ref memfun = m_ctx->get_fun_expr(args_type, mem->get_sort()); //m_ctx->mk_var(memfun_name, memfun_sort);
  //string io_fun_name = smt_fun_name + "." + G_IO_SYMBOL;
  //expr_ref io_fun = m_ctx->mk_var(io_fun_name, io_fun_sort);
  ASSERT(args[FUNCTION_CALL_REGNUM_ARG_INDEX]->is_const());
  //args[FUNCTION_CALL_REGNUM_ARG_INDEX] = zerobv; //regnum 0
  args[FUNCTION_CALL_REGNUM_ARG_INDEX] = m_ctx->mk_regid_const(0); //regnum 0
  expr_ref ret = m_ctx->mk_function_call(memfun, args);
  //cout << __func__ << " " << __LINE__ << ": ret = " << endl << ret->to_string_table() << endl;
  state_set_expr(state_out, m_mem_reg, ret);
  //ret = m_ctx->mk_function_call(io_fun, args);
  //state_out.set_expr(m_io_reg, ret);
  return make_pair(assumes, succ_assumes);
}

void sym_exec_llvm::exec(const state& state_in, const llvm::Instruction& I, shared_ptr<tfg_node> from_node, llvm::BasicBlock const &B, llvm::Function const &F, size_t next_insn_id, tfg &t, map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> *function_tfg_map, set<string> const *function_call_chain, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  DYN_DEBUG(llvm2tfg, errs() << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": sym exec doing: " << I << "\n");
  unordered_set<expr_ref> state_assumes;
  vector<control_flow_transfer> cfts;
  state state_out = state_in;
  string const &cur_function_name = F.getName().str();
  string const& bbindex = get_basicblock_index(B);
  bbl_order_descriptor_t const& bbo = this->m_bbl_order_map.at(mk_string_ref(bbindex));
  pc pc_to = get_pc_from_bbindex_and_insn_id(bbindex, next_insn_id);
  te_comment_t te_comment = this->instruction_to_te_comment(I, from_node->get_pc(), bbo);

  //cout << __func__ << " " << __LINE__ << ": t.incoming =\n" << t.incoming_sizes_to_string() << endl;
  switch(I.getOpcode())
  {
  case Instruction::Br:
  {
    const BranchInst* i =  cast<const BranchInst>(&I);
    if(i->isUnconditional())
    {
      control_flow_transfer cft(from_node->get_pc(), get_pc_from_bbindex_and_insn_id(get_basicblock_index(*i->getSuccessor(0)), 0), m_ctx->mk_bool_true());
      cfts.push_back(cft);
    }
    else if(i->isConditional())
    {
      expr_ref e;
      unordered_set<expr_ref> cond_assumes;
      tie(e, cond_assumes) = get_expr_adding_edges_for_intermediate_vals(*i->getCondition(), "", state_in, state_assumes, from_node, pc_to, B, F, t);
      ASSERT(e->is_bool_sort());
      control_flow_transfer cft1(from_node->get_pc(), get_pc_from_bbindex_and_insn_id(get_basicblock_index(*i->getSuccessor(0)), 0), e, cond_assumes);
      cfts.push_back(cft1);
      control_flow_transfer cft2(from_node->get_pc(), get_pc_from_bbindex_and_insn_id(get_basicblock_index(*i->getSuccessor(1)), 0), m_ctx->mk_not(e), cond_assumes);
      cfts.push_back(cft2);
    }
    else
      assert(false);
    break;
  }
  case Instruction::Switch:
  {
    /* Switch is expanded like this:
     * (S) -------- () --------- () ------- () ---------- ... () ---------- () --------- ()
     *        T      \   ~case1        T     \    ~case2       \   ~caseN        T (default)
     *          case1 \                case2  \                 \
     *                 \                       \                 \
     *
     */
    const SwitchInst* SI =  cast<const SwitchInst>(&I);

    Value* Cond = SI->getCondition();
    Type *ElTy = Cond->getType();
    expr_ref CondVal;
    unordered_set<expr_ref> cond_assumes;
    tie(CondVal, cond_assumes) = get_expr_adding_edges_for_intermediate_vals(*Cond, "", state_in, state_assumes, from_node, pc_to, B, F, t);
    //string typeString = getTypeString(ElTy);
    //langtype_ref lt = mk_langtype_ref(typeString);
    //predicate p(precond_t(m_ctx), m_ctx->mk_islangtype(CondVal, lt), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_SWITCH_ISLANGTYPE, predicate::assume);
    //t.add_assume_pred(from_node->get_pc(), p);
    //assumes.insert(p);

    vector<expr_ref> matched_any_cond;

    // Check to see if any of the cases match...
    for (auto Case : SI->cases()) {
      expr_ref CaseVal;
      unordered_set<expr_ref> CaseAssumes;
      tie(CaseVal, CaseAssumes) = get_expr_adding_edges_for_intermediate_vals(*Case.getCaseValue(), "", state_in, state_assumes, from_node, pc_to, B, F, t);
      ASSERT(CondVal->get_sort() == CaseVal->get_sort());
      expr_ref cond = m_ctx->mk_eq(CondVal, CaseVal);
      control_flow_transfer cft(from_node->get_pc(), get_pc_from_bbindex_and_insn_id(get_basicblock_index(*Case.getCaseSuccessor()), 0), cond, CaseAssumes);
      cfts.push_back(cft);
      matched_any_cond.push_back(cond);
      //cout << __func__ << " " << __LINE__ << ": cft cond = " << expr_string(cond) << ", dest = " << get_basicblock_name(*i.getCaseSuccessor()) << endl;
    }
    expr_ref remaining_cond;
    if (matched_any_cond.size() == 0) {
      remaining_cond = expr_true(m_ctx);
    } else if (matched_any_cond.size() == 1) {
      remaining_cond = m_ctx->mk_not(matched_any_cond.at(0));
    } else {
      remaining_cond = m_ctx->mk_not(m_ctx->mk_app(expr::OP_OR, matched_any_cond));
    }
    control_flow_transfer cft(from_node->get_pc(), get_pc_from_bbindex_and_insn_id(get_basicblock_index(*SI->getDefaultDest()), 0), remaining_cond);
    cfts.push_back(cft);
    cfts = expand_switch(t, from_node, cfts, state_out, cond_assumes, te_comment, (Instruction *)&I, B, F, eimap);
    //cft_processing_needed = false;
    break;
  }
  case Instruction::Ret:
  {
    const ReturnInst* i = cast<const ReturnInst>(&I);
    if(Value* ret = i->getReturnValue())
    {
      Type *ElTy = ret->getType();

      expr_ref dst_val;
      tie(dst_val, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*ret, "", state_in, state_assumes, from_node, pc_to, B, F, t);


      //string typeString = getTypeString(ElTy);
      //langtype_ref lt = mk_langtype_ref(typeString);
      //predicate p(precond_t(m_ctx), m_ctx->mk_islangtype(dst_val, lt), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_RET_ISLANGTYPE, predicate::assume);
      //assumes.insert(p);
      //t.add_assume_pred(from_node->get_pc(), p);

      state_set_expr(state_out, G_LLVM_RETURN_REGISTER_NAME/*m_ret_reg*/, dst_val);
    }
    for (size_t i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
      stringstream ss;
      ss << G_INPUT_KEYWORD "." G_SRC_KEYWORD "." LLVM_CALLEE_SAVE_REGNAME << "." << i;
      expr_ref csreg = m_ctx->mk_var(ss.str(), m_ctx->mk_bv_sort(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS)));
      state_set_expr(state_out, G_SRC_KEYWORD "." G_LLVM_HIDDEN_REGISTER_NAME, m_ctx->mk_bvxor(state_get_expr(state_out, G_SRC_KEYWORD "." G_LLVM_HIDDEN_REGISTER_NAME, csreg->get_sort()), csreg));
    }
    control_flow_transfer cft(from_node->get_pc(), pc(pc::exit), m_ctx->mk_bool_true(), m_cs.get_retaddr_const(), {});
    cfts.push_back(cft);
    break;
  }
  case Instruction::Alloca:
  {
    const AllocaInst* a =  cast<const AllocaInst>(&I);
    string name = get_value_name(*a);
    size_t local_id = m_local_num++;
    stringstream ss0;
    ss0 << G_LOCAL_KEYWORD << '.' << local_id;
    string local_name = ss0.str();
    const DataLayout &dl = m_module->getDataLayout();
    Type *ElTy = a->getAllocatedType();
    uint64_t local_size = dl.getTypeAllocSize(ElTy);
    unsigned align = a->getAlignment();
    bool is_varsize = !(a->getAllocationSizeInBits(dl).hasValue());

    m_local_refs.insert(make_pair(local_id, graph_local_t(name, local_size, align, is_varsize)));
    expr_ref local_addr = m_ctx->mk_var(local_name, m_ctx->mk_bv_sort(get_word_length()));
    memlabel_t ml_local;
    stringstream ss;
    ss <<  local_name << "." << local_size;
    string local_name_with_size = ss.str();
    memlabel_t::keyword_to_memlabel(&ml_local, local_name_with_size.c_str(), local_size);

    //string typeString = getTypeString(ElTy);
    //typeString = typeString + "*";
    //langtype_ref lt = mk_langtype_ref(typeString);
    //predicate p(precond_t(m_ctx), m_ctx->mk_islangtype(local_addr, lt), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ALLOCA_ISLANGTYPE, predicate::assume);
    //assumes.insert(p);
    //t.add_assume_pred(from_node->get_pc(), p);

    ostringstream ss2;
    ss2 << G_LOCAL_SIZE_KEYWORD << '.' << local_id;
    expr_ref local_size_expr;
    if (is_varsize) {
      expr_ref varsize_expr;
      Value const* ArraySize = a->getArraySize();
      tie(varsize_expr, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*ArraySize, "", state_in, state_assumes, from_node, pc_to, B, F, t);
      local_size_expr = m_ctx->mk_bvmul(varsize_expr, m_ctx->mk_bv_const(varsize_expr->get_sort()->get_size(), local_size));
      ASSERT(local_size_expr->get_sort()->get_size() == get_word_length());
    } else {
      local_size_expr = m_ctx->mk_bv_const(get_word_length(), local_size);
    }

    state_set_expr(state_out, m_mem_reg, m_ctx->mk_alloca(state_get_expr(state_in, m_mem_reg, this->get_mem_sort()), ml_local, local_addr, local_size_expr));
    state_set_expr(state_out, name, local_addr);
    state_set_expr(state_out, ss2.str(), local_size_expr);
    break;
  }
  case Instruction::Store:
  {
    const StoreInst* s =  cast<const StoreInst>(&I);
    memlabel_t ml_top;
    memlabel_t::keyword_to_memlabel(&ml_top, G_MEMLABEL_TOP_SYMBOL, MEMSIZE_MAX);
    Value const *Addr = s->getPointerOperand();
    Value const *Val = s->getValueOperand();
    size_t align = s->getAlignment();

    if (   Val->getType()->getTypeID() == Type::FloatTyID
        || Val->getType()->getTypeID() == Type::DoubleTyID) {
      state_set_expr(state_out, G_SRC_KEYWORD "." LLVM_CONTAINS_FLOAT_OP_SYMBOL, m_ctx->mk_bool_const(true));
      break;
    }

    expr_ref addr, val;
    tie(addr, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*Addr, "", state_in, state_assumes, from_node, pc_to, B, F, t);
    tie(val, state_assumes)  = get_expr_adding_edges_for_intermediate_vals(*Val, "", state_in, state_assumes, from_node, pc_to, B, F, t);
    if (val->is_bool_sort()) {
      string val_str = expr_string(val);
      /*if (val_str.find("unsupported") != string::npos) */{
        state_set_expr(state_out, G_SRC_KEYWORD "." LLVM_CONTAINS_FLOAT_OP_SYMBOL, m_ctx->mk_bool_const(true));
        break;
      }
    }
    ASSERTCHECK(val->is_bv_sort(), cout << __func__ << " " << __LINE__ << ": val = " << expr_string(val) << endl);
    unsigned count = val->get_sort()->get_size()/get_memory_addressable_size();
    state_set_expr(state_out, m_mem_reg, m_ctx->mk_store(state_get_expr(state_in, m_mem_reg, this->get_mem_sort()), ml_top, addr, val, count, false/*, comment_t()*/));
    //add_dereference_assume(addr, assumes);

    //Type *ElTy = Addr->getType();
    //string typeString = getTypeString(ElTy);
    //langtype_ref lt_addr = mk_langtype_ref(typeString);
    //predicate p1(precond_t(m_ctx), m_ctx->mk_islangtype(addr, lt_addr), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_STORE_ADDR_ISLANGTYPE, predicate::assume);
    //assumes.insert(p1);
    //t.add_assume_pred(from_node->get_pc(), p1);

    //ElTy = Val->getType();
    //typeString = getTypeString(ElTy);
    //langtype_ref lt_val = mk_langtype_ref(typeString);
    //predicate p2(precond_t(m_ctx), m_ctx->mk_islangtype(val, lt_val), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_STORE_DATA_ISLANGTYPE, predicate::assume);
    //assumes.insert(p2);
    //t.add_assume_pred(from_node->get_pc(), p2);

    if (align != 0) {
      expr_ref const& isaligned_assume = m_ctx->mk_islangaligned(addr, align);
      state_assumes.insert(isaligned_assume);
      //predicate p3(precond_t(m_ctx), m_ctx->mk_islangaligned(addr, align), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ALIGN_ISLANGALIGNED, predicate::assume);
      //assumes.insert(p3);
      //t.add_assume_pred(from_node->get_pc(), p3);
    }
    break;
  }
  case Instruction::Load:
  {
    const LoadInst* l =  cast<const LoadInst>(&I);
    Value const *Addr = l->getPointerOperand();

    expr_ref addr;
    tie(addr, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*Addr, gep_name_prefix("const_operand", from_node->get_pc(), pc_to, 0), state_in, state_assumes, from_node, pc_to, B, F, t);
    if (   l->getType()->getTypeID() == Type::FloatTyID
        || l->getType()->getTypeID() == Type::DoubleTyID) {
      state_set_expr(state_out, G_SRC_KEYWORD "." LLVM_CONTAINS_FLOAT_OP_SYMBOL, m_ctx->mk_bool_const(true));
      break;
    }
    sort_ref value_type = get_value_type(*l, m_module->getDataLayout());
    ASSERTCHECK(value_type->is_bv_kind(), cout << __func__ << " " << __LINE__ << ": value_type = " << value_type->to_string() << endl);
    memlabel_t ml_top;
    memlabel_t::keyword_to_memlabel(&ml_top, G_MEMLABEL_TOP_SYMBOL, MEMSIZE_MAX);
    unsigned count = value_type->get_size()/get_memory_addressable_size();
    expr_ref read_value = m_ctx->mk_select(state_get_expr(state_in, m_mem_reg, this->get_mem_sort()), ml_top, addr, count, false);
    set_expr(get_value_name(*l), read_value, state_out);

    //add_dereference_assume(addr, assumes);

    //Type *ElTy = Addr->getType();
    //string typeString = getTypeString(ElTy);
    //langtype_ref lt_addr = mk_langtype_ref(typeString);
    //predicate p1(precond_t(m_ctx), m_ctx->mk_islangtype(addr, lt_addr), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_LOAD_ISLANGTYPE, predicate::assume);
    //assumes.insert(p1);
    //t.add_assume_pred(from_node->get_pc(), p1);

    size_t align = l->getAlignment();
    if (align != 0) {
      expr_ref const& isaligned_assume = m_ctx->mk_islangaligned(addr, align);
      state_assumes.insert(isaligned_assume);
      //predicate p2(precond_t(m_ctx), m_ctx->mk_islangaligned(addr, align), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ALIGN_ISLANGALIGNED, predicate::assume);
      //assumes.insert(p2);
      //t.add_assume_pred(from_node->get_pc(), p2);
    }
    string lname = get_value_name(*l);
    Type *lTy = (*l).getType();
    //add_align_assumes(lname, lTy, read_value, pc_to/*from_node->get_pc()*/, t);
    // create extra edge for adding assumes related to the load target
    shared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    tfg_edge_ref e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_out, expr_true(m_ctx)/*, t.get_start_state()*/, state_assumes, te_comment));
    t.add_edge(e);

    state_assumes = gen_align_assumes(lname, lTy, read_value->get_sort());
    state_out = state_in;
    from_node = t.find_node(intermediate_node->get_pc());
    ASSERT(from_node);
    break;
  }
  case Instruction::Call:
  {
    const CallInst* c =  cast<const CallInst>(&I);

    Function *calleeF = c->getCalledFunction();
    string fun_name;
    expr_ref fun_expr;
    if (calleeF == NULL) {
      Value const *v = c->getCalledValue();
      Value const *sv = v->stripPointerCasts();
      fun_name = string(sv->getName());
      if (fun_name == "") {
        //fun_expr = get_expr_adding_edges_for_intermediate_vals(*sv, "", state_in, from_node, pc_to, B, F, t/*, assumes*/);
        tie(fun_expr, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*sv, "", state_in, state_assumes, from_node, pc_to, B, F, t);
      } else {
        fun_name = string(LLVM_FUNCTION_NAME_PREFIX) + fun_name;
        fun_expr = m_ctx->mk_var(fun_name, m_ctx->mk_bv_sort(DWORD_LEN));
      }
    } else {
      fun_name = calleeF->getName().str();
      fun_name = string(LLVM_FUNCTION_NAME_PREFIX) + fun_name;
      fun_expr = m_ctx->mk_var(fun_name, m_ctx->mk_bv_sort(DWORD_LEN));
    }
    if (   fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_DBG_DECLARE_FUNCTION
        || fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_DBG_VALUE_FUNCTION
        || fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_STACKSAVE_FUNCTION
        || fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_STACKRESTORE_FUNCTION) {
      break;
    }
    if (string_has_prefix(fun_name, LLVM_FUNCTION_NAME_PREFIX G_LLVM_LIFETIME_FUNCTION_PREFIX)) {
      break;
    }
    if (fun_name == string(LLVM_FUNCTION_NAME_PREFIX) + cur_function_name) {
      fun_name = LLVM_FUNCTION_NAME_PREFIX G_SELFCALL_IDENTIFIER;
      fun_expr = m_ctx->mk_var(fun_name, m_ctx->mk_bv_sort(DWORD_LEN));
    }
    string memcpy_fn = LLVM_FUNCTION_NAME_PREFIX G_LLVM_MEMCPY_FUNCTION;
    unordered_set<expr_ref> succ_assumes;
    if (fun_name.substr(0, memcpy_fn.length()) == memcpy_fn) {
      tie(state_assumes, succ_assumes) = apply_memcpy_function(c, fun_expr, fun_name, calleeF, state_in, state_out, state_assumes, cur_function_name, from_node, pc_to, B, F, t, function_tfg_map, function_call_chain);
    } else {
      tie(state_assumes, succ_assumes) = apply_general_function(c, fun_expr, fun_name, calleeF, state_in, state_out, state_assumes, cur_function_name, from_node, pc_to, B, F, t, function_tfg_map, function_call_chain);
    }

    if (succ_assumes.size()) {
      // create extra edge for adding assumes related to the function return value target
      shared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
      tfg_edge_ref e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_out, expr_true(m_ctx)/*, t.get_start_state()*/, state_assumes, te_comment));
      t.add_edge(e);

      state_assumes = succ_assumes;
      state_out = state_in;
      from_node = t.find_node(intermediate_node->get_pc());
      ASSERT(from_node);
    }

    break;
  }
  case Instruction::IndirectBr:
    assert(false);
  case Instruction::Invoke:
    assert(false);
  case Instruction::Resume:
    assert(false);
  case Instruction::Unreachable:
    //assert(false);
    break;
  case Instruction::FDiv:
  case Instruction::FMul:
  case Instruction::FAdd:
  case Instruction::FCmp:
  case Instruction::FSub:
  case Instruction::FRem:
  case Instruction::FPToUI:
  case Instruction::FPToSI:
  case Instruction::UIToFP:
  case Instruction::SIToFP:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
    state_set_expr(state_out, G_SRC_KEYWORD "." LLVM_CONTAINS_FLOAT_OP_SYMBOL, m_ctx->mk_bool_const(true));
    break;
  case Instruction::ExtractValue: {
    ASSERT(I.getNumOperands() == 1);
    Value const &op0 = *I.getOperand(0);
    string iname = get_value_name(I);
    string op0name = get_value_name(op0);
    sort_ref op0_sort = get_value_type(op0, m_module->getDataLayout());
    sort_ref s = get_value_type(I, m_module->getDataLayout());
    const ExtractValueInst *EVI = cast<ExtractValueInst>(&I);
    vector<int> indices;
    for (auto idx = EVI->idx_begin(); idx != EVI->idx_end(); idx++) {
      indices.push_back(*idx);
    }
    if (indices.size() > 1) {
      errs() << "op0name = " << op0name << "\n";
      errs().flush();
      NOT_IMPLEMENTED();
    } else {
      stringstream ss;
      if (op0_sort == s) { //indicates that the struct has only one field; so we do not need a field selector
        ss << G_INPUT_KEYWORD "." << op0name;
      } else {
        ss << G_INPUT_KEYWORD "." << op0name << "." << LLVM_FIELDNUM_PREFIX << indices.at(0);
      }
      state_set_expr(state_out, iname, m_ctx->mk_var(ss.str(), s));
    }
    break;
  }
  default:
    vector<expr_ref> expr_args;
    tie(expr_args, state_assumes) = get_expr_args(I, "", state_in, state_assumes, from_node, pc_to, B, F, t);
    expr_ref insn_expr;
    tie(insn_expr, state_assumes) = exec_gen_expr(I, "", expr_args, state_in, state_assumes, from_node, pc_to, B, F, t);
    set_expr(get_value_name(I), insn_expr, state_out);
    break;
  }
  DYN_DEBUG(llvm2tfg, errs() << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": sym exec done\n");
  process_cfts<llvm::Function, llvm::BasicBlock, llvm::Instruction>(t, from_node, pc_to, state_out, state_assumes, te_comment, (Instruction *)&I, cfts, B, F, eimap);
}

te_comment_t
sym_exec_common::phi_node_to_te_comment(bbl_order_descriptor_t const& bbo, int inum, llvm::Instruction const& I) const
{
  string s;
  raw_string_ostream ss(s);
  //cout << __func__ << " " << __LINE__ << ": from_index = " << from_index << ", from_subindex = " << from_subindex << ", from_subsubindex = " << from_subsubindex << ", bbo = " << bbo.get_bbl_num() << ", to_pc = " << to_pc.to_string() << endl;
  ss << I;
  return te_comment_t(bbo, true, inum, ss.str());
}


te_comment_t
sym_exec_common::instruction_to_te_comment(llvm::Instruction const& I, pc const& from_pc, bbl_order_descriptor_t const& bbo) const
{
  string s;
  raw_string_ostream ss(s);
  char const* from_index = from_pc.get_index();
  int from_subindex = from_pc.get_subindex();
  int from_subsubindex = from_pc.get_subsubindex();
  //bbl_order_descriptor_t const& bbo = m_bbl_order_map.at(from_index);
  //cout << __func__ << " " << __LINE__ << ": from_index = " << from_index << ", from_subindex = " << from_subindex << ", from_subsubindex = " << from_subsubindex << ", bbo = " << bbo.get_bbl_num() << ", to_pc = " << to_pc.to_string() << endl;
  ss << I;
  string str = ss.str();
  if (string_has_prefix(str, "  <badref>")) {
    return te_comment_t::te_comment_badref();
  }
  return te_comment_t(bbo, false, from_subindex, ss.str());
}

pair<expr_ref,unordered_set<expr_ref>>
sym_exec_llvm::exec_gen_expr(const llvm::Instruction& I, string Iname, const vector<expr_ref>& args, state const &state_in, unordered_set<expr_ref> const& state_assumes, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t)
{
  //errs() << "exec_gen_expr: " << I << "\n";
  pc const &from_pc = from_node->get_pc();
  string const& bbindex = get_basicblock_index(B);
  bbl_order_descriptor_t const& bbo = this->m_bbl_order_map.at(mk_string_ref(bbindex));
  switch(I.getOpcode())
  {
  case Instruction::Add:
  case Instruction::Mul:
  case Instruction::Sub:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
  {
    const BinaryOperator* i = cast<const BinaryOperator>(&I);
    //errs() << "arg0: " << args[0]->to_string_table() << "\n";
    //errs() << "arg1: " << args[1]->to_string_table() << "\n";
    assert(args[0]->get_sort() == args[1]->get_sort());
    expr_ref ret = m_ctx->mk_app(binary_op_to_expr_kind(i->getOpcode(), args[0]->is_bool_sort()), args);
    unordered_set<expr_ref> assumes = state_assumes;
    if (   I.getOpcode() == Instruction::Shl
        || I.getOpcode() == Instruction::LShr
        || I.getOpcode() == Instruction::AShr) {
      ASSERT(args[0]->is_bv_sort());
      assumes.insert(gen_shiftcount_assume_expr(args[1], args[0]->get_sort()->get_size()));
      //add_shiftcount_assume(args[1], args[0]->get_sort()->get_size(), from_node->get_pc(), t/*assumes*/);
    }
    if (   I.getOpcode() == Instruction::UDiv
        || I.getOpcode() == Instruction::SDiv
        || I.getOpcode() == Instruction::URem
        || I.getOpcode() == Instruction::SRem) {
      ASSERT(args[0]->is_bv_sort());
      assumes.insert(gen_no_divbyzero_assume_expr(args[1]));
      //add_divbyzero_assume(args[1], from_node->get_pc(), t/*assumes*/);
      if (   I.getOpcode() == Instruction::SDiv
          || I.getOpcode() == Instruction::SRem) {
        assumes.insert(gen_div_no_overflow_assume_expr(args[0], args[1]));
        //add_div_no_overflow_assume(args[0], args[1], from_node->get_pc(), t/*assumes*/);
      }
    }
    return make_pair(ret, assumes);
  }
  case Instruction::SExt:
  case Instruction::ZExt:
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast:
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::Trunc:
  {
    assert(args.size() == 1);
    return exec_gen_expr_casts(cast<CastInst>(I), args[0], state_assumes, from_node->get_pc(), t);
  }
  case Instruction::Select:
    return make_pair(m_ctx->mk_app(expr::OP_ITE, args), state_assumes);
  case Instruction::ICmp:
  {
    const ICmpInst* i =  cast<const ICmpInst>(&I);
    return make_pair(icmp_to_expr(i->getPredicate(), args), state_assumes);
  }
  case Instruction::InsertValue:
  case Instruction::ExtractValue:
    assert(false && "insert extract not handled");
  case Instruction::GetElementPtr:
  {
    const GetElementPtrInst* gep =  cast<const GetElementPtrInst>(&I);
    bool inbounds = gep->isInBounds();
    expr_ref ptr = args[0];

    const DataLayout& dl = m_module->getDataLayout();
    gep_type_iterator itype_begin = gep_type_begin(gep);
    gep_type_iterator itype_end = gep_type_end(gep);
    unsigned index_counter = 1;
    uint64_t total_offset = 0;
    //bool var_index = false;
    assert(ptr->is_bv_sort());
    pc cur_pc = from_pc;
    expr_ref cur_expr = m_ctx->mk_zerobv(ptr->get_sort()->get_size());
    string gnp = (Iname == "" ? gep_name_prefix("gep", from_node->get_pc(), pc_to, 0) : Iname);
    //expr_ref total_offset_expr = m_ctx->mk_bv_const(ptr->get_sort()->get_size(), 0);
    for(gep_type_iterator itype = itype_begin; itype != itype_end; ++itype, ++index_counter)
    {
      Type* typ = itype.getIndexedType();

      expr_ref index = args[index_counter];
      assert(index->is_bv_sort() && index->get_sort()->get_size() == get_word_length());
      expr_ref offset_expr;
      unordered_set<expr_ref> assumes = state_assumes;

      if (itype.isStruct()) {
        //StructType* s = dyn_cast<StructType>(typ)
        StructType* s = itype.getStructType();
        assert(index->is_const());
        int64_t c = index->get_int64_value();
        assert(c >= 0);
        uint64_t offset = dl.getStructLayout(s)->getElementOffset(c);
        total_offset += offset;
        offset_expr = m_ctx->mk_bv_const(get_word_length(), offset);
        //total_offset_expr = expr_bvadd(total_offset_expr, offset_expr);
      } else if (itype.isSequential()) {
        uint64_t size = dl.getTypeAllocSize(typ);
        expr_ref size_expr = m_ctx->mk_bv_const(get_word_length(), size);
        offset_expr = expr_bvmul(index, size_expr);

        if (inbounds) { // gep calculation can have undefined result only if it is inbounds
          // scaling cannot have signed overflow
          unsigned bvlen = get_word_length();

          expr_ref se_index    = m_ctx->mk_bvsign_ext(index, bvlen);
          expr_ref ze_size     = m_ctx->mk_bvzero_ext(size_expr, bvlen);
          expr_ref mul_expr    = expr_bvmul(se_index, ze_size);
          expr_ref msb_expr    = expr_bvextract(2*bvlen-1, bvlen, mul_expr);
          expr_ref msb_is_zero = m_ctx->mk_eq(msb_expr, m_ctx->mk_zerobv(bvlen));
          expr_ref overflow_expr;
          if (itype.isBoundedSequential()) {
            // if base is array then index must be positive
            overflow_expr = msb_is_zero;
          } else {
            expr_ref is_neg = m_ctx->mk_bvslt(index, m_ctx->mk_zerobv(bvlen));
            expr_ref msb_is_minusone = m_ctx->mk_eq(msb_expr, m_ctx->mk_minusonebv(bvlen));
            overflow_expr = m_ctx->mk_ite(is_neg, msb_is_minusone, msb_is_zero);
          }
          expr_ref assume = m_ctx->mk_isindexforsize(overflow_expr, size);
          assumes.insert(assume);
          //predicate p(precond_t(m_ctx), assume, expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ISINDEXFORSIZE, predicate::assume);
          //t.add_assume_pred(cur_pc, p);
        }
      } else {
        unreachable();
      }
      ASSERT(offset_expr.get());

      expr_ref new_cur_expr = m_ctx->mk_bvadd(cur_expr, offset_expr);

      string offset_name       = gep_instruction_get_intermediate_value_name(gnp, index_counter, 0);
      string total_offset_name = gep_instruction_get_intermediate_value_name(gnp, index_counter, 1);

      state state_to_intermediate_val;
      state_set_expr(state_to_intermediate_val, offset_name, offset_expr);
      state_set_expr(state_to_intermediate_val, total_offset_name, new_cur_expr);

      if (inbounds) {
        // Model undefined behaviour due to signed overflow in offset calculation
        // Ref: https://llvm.org/docs/GetElementPtr.html#what-happens-if-a-gep-computation-overflows
        // TL;DR The result of the additions within the offset calculation cannot have signed overflow,
        // but when applied to the base pointer, there can be signed overflow.
        expr_ref gep_expr = m_ctx->mk_bvadd(ptr, new_cur_expr);
        expr_ref assume = m_ctx->mk_isgepoffset(gep_expr, offset_expr);
        assumes.insert(assume);
        //predicate p(precond_t(m_ctx), assume, expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ISGEPOFFSET, predicate::assume);
        //t.add_assume_pred(cur_pc, p);
      }

      shared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
      shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(cur_pc, intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx)/*, t.get_start_state()*/, assumes, this->instruction_to_te_comment(I, from_node->get_pc(), bbo)));
      t.add_edge(e);

      cur_expr = mk_fresh_expr(total_offset_name, G_INPUT_KEYWORD, m_ctx->mk_bv_sort(DWORD_LEN));
      cur_pc = intermediate_node->get_pc();
    }

    string total_offset_name = gep_instruction_get_intermediate_value_name(gnp, index_counter, 1);

    state state_to_intermediate_val;
    state_set_expr(state_to_intermediate_val, total_offset_name, m_ctx->mk_bvadd(ptr, cur_expr));

    shared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(cur_pc, intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx)/*, t.get_start_state()*/, {}, this->instruction_to_te_comment(I, from_node->get_pc(), bbo)));
    t.add_edge(e);

    cur_expr = mk_fresh_expr(total_offset_name, G_INPUT_KEYWORD, m_ctx->mk_bv_sort(DWORD_LEN));
    cur_pc = intermediate_node->get_pc();

    from_node = t.find_node(cur_pc);
    ASSERT(from_node);

    return make_pair(cur_expr, unordered_set<expr_ref>());
  }
  default:
    errs() << I << "\n";
    assert(false && "syn_exec: opcode not handled");
  }
}

expr_ref
sym_exec_llvm::gen_dereference_assume_expr(expr_ref const& a) const
{
  ASSERT(a->is_bv_sort());
  return m_ctx->mk_not(m_ctx->mk_eq(a, m_ctx->mk_zerobv(a->get_sort()->get_size())));
}

//void sym_exec_llvm::add_dereference_assume(expr_ref a, pc const &from_pc, tfg &t/*unordered_set<predicate> &assumes*/)
//{
//  predicate p(precond_t(m_ctx), this->gen_dereference_assume_expr(a), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_DEREFERENCE);
//  //assumes.insert(p);
//  t.add_assume_pred(from_pc, p);
//}


expr_ref
sym_exec_llvm::gen_shiftcount_assume_expr(expr_ref const& a, size_t shifted_val_size) const
{
  return m_ctx->mk_isshiftcount(a, shifted_val_size);
}

//void sym_exec_llvm::add_shiftcount_assume(expr_ref a, size_t shifted_val_size, pc const &from_pc, tfg &t) const
//{
//  predicate p(precond_t(m_ctx), this->gen_shiftcount_assume_expr(a, shifted_val_size), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ISSHIFTCOUNT);
//  //assumes.insert(p);
//  t.add_assume_pred(from_pc, p);
//}

expr_ref
sym_exec_llvm::gen_no_divbyzero_assume_expr(expr_ref const& a) const
{
  return m_ctx->mk_not(m_ctx->mk_eq(a, m_ctx->mk_zerobv(a->get_sort()->get_size())));
}

//void
//sym_exec_llvm::add_divbyzero_assume(expr_ref a, pc const &from_pc, tfg &t/*, unordered_set<predicate> &assumes*/) const
//{
//  ASSERT(a->is_bv_sort());
//  predicate p(precond_t(m_ctx), this->gen_no_divbyzero_assume_expr(a), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_DIVBYZERO);
//  t.add_assume_pred(from_pc, p);
//}

expr_ref
sym_exec_llvm::gen_div_no_overflow_assume_expr(expr_ref const& dividend, expr_ref const& divisor) const
{
  ASSERT(dividend->is_bv_sort());
  ASSERT(divisor->is_bv_sort());
  int bvsize = divisor->get_sort()->get_size();
  ASSERT(bvsize <=  64);
  unsigned long max_negative_value = 0x1<<(bvsize-1);
  // XXX verify this
  return m_ctx->mk_not(m_ctx->mk_and(m_ctx->mk_eq(divisor, m_ctx->mk_zerobv(bvsize)), m_ctx->mk_eq(dividend, m_ctx->mk_bv_const(bvsize, max_negative_value))));
}

//void sym_exec_llvm::add_div_no_overflow_assume(expr_ref dividend, expr_ref divisor, pc const &from_pc, tfg &t) const
//{
//  predicate p(precond_t(m_ctx), this->gen_div_no_overflow_assume_expr(dividend, divisor), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_DIV_NO_OVERFLOW);
//  //assumes.insert(p);
//  t.add_assume_pred(from_pc, p);
//}

unordered_set<expr_ref>
sym_exec_llvm::gen_align_assumes(string const &Elname, Type *ElTy, sort_ref const& s) const
{
  unordered_set<expr_ref> ret;
  expr_ref Elexpr = m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + Elname, s);
  if (isa<const PointerType>(ElTy)) {
    PointerType *ElTyPointer = cast<PointerType>(ElTy);
    Type *ElTy2 = ElTyPointer->getElementType();
    //string typeString2 = getTypeString(ElTy2);
    //cout << __func__ << " " << __LINE__ << ": typeString2 = " << typeString2 << endl;
    if (ElTy2->isSized()) {
      const DataLayout& dl = m_module->getDataLayout();
      size_t elty2_size = dl.getTypeAllocSize(ElTy2);
      if (elty2_size != 0) {
        expr_ref assume2 = m_ctx->mk_islangaligned(Elexpr, elty2_size);
        ret.insert(assume2);
        //predicate p2(precond_t(m_ctx), m_ctx->mk_islangaligned(Elexpr, elty2_size), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ALIGN_ISLANGALIGNED, predicate::assume);
        //cout << __func__ << " " << __LINE__ << ": adding assume " << expr_string(assume2) << endl;
        //assumes.insert(p2);
        //t.add_assume_pred(pc_to, p2);
      }
    }
  }
  return ret;
}

//void sym_exec_llvm::add_align_assumes(string const &Elname, Type *ElTy/*llvm::Value const &arg*/, expr_ref a, pc const &pc_to, tfg &t) const
//{
//  unordered_set<expr_ref> const& assumes = gen_align_assumes(Elname, ElTy, a->get_sort());
//  for (auto const& a_expr : assumes) {
//    string comment;
//    if (a_expr->get_operation_kind() == expr::OP_ISLANGALIGNED) { comment = UNDEF_BEHAVIOUR_ASSUME_ALIGN_ISLANGALIGNED; }
//    else { NOT_REACHED(); }
//
//    predicate p(precond_t(m_ctx), a_expr, expr_true(m_ctx), comment, predicate::assume);
//    t.add_assume_pred(pc_to, p);
//  }
//}

unique_ptr<tfg_llvm_t>
sym_exec_llvm::get_tfg(map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> *function_tfg_map, set<string> const *function_call_chain, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  llvm::Function const &F = m_function;
  populate_state_template(F);
  state start_state;
  get_state_template(pc::start(), start_state);
  stringstream ss;
  ss << "llvm." << functionGetName(F);
  unique_ptr<tfg_llvm_t> t = make_unique<tfg_llvm_t>(ss.str(), m_ctx);
  ASSERT(t);
  t->set_start_state(start_state);

  this->populate_bbl_order_map();

  for(const BasicBlock& B : F) {
    add_edges(B, *t, F, function_tfg_map, function_call_chain, eimap);
  }

  //for (const auto& arg : F.args()) {
  //  pair<argnum_t, expr_ref> const &a = m_arguments.at(get_value_name(arg));
  //  string Elname = get_value_name(arg) + SRC_INPUT_ARG_NAME_SUFFIX;
  //  Type *ElTy = arg.getType();
  //  add_type_and_align_assumes(Elname, ElTy, a.second, pc::start(), *t/*, assumes*/, UNDEF_BEHAVIOUR_ASSUME_ARG_ISLANGTYPE);
  //}

  this->get_tfg_common(*t);
  return t;
}

pc
sym_exec_common::get_pc_from_bbindex_and_insn_id(string const &bbindex/*llvm::BasicBlock const &B*/, size_t insn_id) const
{
  pc ret(pc::insn_label, /*get_basicblock_index(B)*/bbindex.c_str()/*, m_basicblock_idx_map.at(string("%") + bbindex)*/, insn_id + PC_SUBINDEX_FIRST_INSN_IN_BB, PC_SUBSUBINDEX_DEFAULT);
  return ret;
}

vector<control_flow_transfer>
sym_exec_llvm::expand_switch(tfg &t, shared_ptr<tfg_node> const &from_node, vector<control_flow_transfer> const &cfts, state const &state_to, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, llvm::Instruction * I, const llvm::BasicBlock& B, const llvm::Function& F, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap)
{
  unordered_set<expr_ref> cond_assumes = assumes;
  vector<control_flow_transfer> new_cfts;
  pc from_pc = from_node->get_pc();
  pc cur_pc = from_pc;
  size_t varnum = 0;
  for (size_t i = 0; i < cfts.size() - 1; i++) {
    const auto &cft = cfts.at(i);
    ASSERT(cft.get_target() == nullptr);
    ASSERT(cft.get_from_pc() == from_pc);
    pc to_pc = cft.get_to_pc();
    expr_ref const& edgecond = cft.get_condition();
    unordered_set<expr_ref> assumes = cft.get_assumes();
    unordered_set_union(assumes, cond_assumes);
    pc new_cur_pc1 = cur_pc.increment_subindex();
    pc new_cur_pc2 = new_cur_pc1.increment_subindex();
    new_cur_pc1.set_subsubindex(PC_SUBSUBINDEX_SWITCH_INTERMEDIATE);
    new_cur_pc2.set_subsubindex(PC_SUBSUBINDEX_SWITCH_INTERMEDIATE);
    stringstream ss;
    ss << G_SRC_KEYWORD "." G_LLVM_PREFIX "-" LLVM_SWITCH_TMPVAR_PREFIX << varnum;
    varnum++;
    string new_varname = ss.str();
    state state_to_newvar;
    state_set_expr(state_to_newvar, new_varname, edgecond);
    shared_ptr<tfg_edge const> e1 = mk_tfg_edge(mk_itfg_edge(cur_pc, new_cur_pc1, state_to_newvar, expr_true(m_ctx)/*, t.get_start_state()*/, assumes, te_comment));

    //expr_ref new_var = state_get_expr(t.get_start_state(), new_varname, m_ctx->mk_bool_sort());
    expr_ref new_var = get_input_expr(new_varname, m_ctx->mk_bool_sort());
    new_cfts.push_back(control_flow_transfer(new_cur_pc1, to_pc, new_var));
    shared_ptr<tfg_edge const> e3 = mk_tfg_edge(mk_itfg_edge(new_cur_pc1, new_cur_pc2, state_to, m_ctx->mk_not(new_var)/*, t.get_start_state()*/, {}, te_comment));
    if (!t.find_node(new_cur_pc1)) {
      t.add_node(make_shared<tfg_node>(new_cur_pc1));
    }
    if (!t.find_node(new_cur_pc2)) {
      t.add_node(make_shared<tfg_node>(new_cur_pc2));
    }
    t.add_edge(e1);
    eimap.insert(make_pair(e3, I));
    t.add_edge(e3);
    cur_pc = new_cur_pc2;
    cond_assumes = {};
  }
  const auto &cft = cfts.at(cfts.size() - 1);
  pc to_pc = cft.get_to_pc();
  control_flow_transfer new_cft(cur_pc, to_pc, expr_true(m_ctx), cond_assumes);
  new_cfts.push_back(new_cft);
  return new_cfts;
}

template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
pair<shared_ptr<tfg_node>, map<string, sort_ref>>
sym_exec_common::process_cft_first_half(tfg &t, shared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, state const &state_to, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, Instruction * I, const BASICBLOCK& B, const FUNCTION& F, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  DYN_DEBUG2(llvm2tfg, errs() << _FNLN_ << ": state_to: " << state_to.to_string() << "\n");

  state state_to_cft(state_to);
  pc const &from_pc = from_node->get_pc();
  if (!t.find_node(pc_to)) {
    t.add_node(shared_ptr<tfg_node>(new tfg_node(pc_to)));
  }
  if (target) {
    state_set_expr(state_to_cft, G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID, target);
    auto e = mk_tfg_edge(mk_itfg_edge(from_pc, pc_to, state_to_cft, to_condition/*, t.get_start_state()*/, assumes, te_comment));
    eimap.insert(make_pair(e, I));
    t.add_edge(e);
    return make_pair(from_node, map<string, sort_ref>());
  } else if (pc_to.is_label() && pc_to.get_subindex() == PC_SUBINDEX_FIRST_INSN_IN_BB) {
    return process_phi_nodes_first_half<FUNCTION, BASICBLOCK, INSTRUCTION>(t, &B, pc_to, from_node, F, eimap);
  } else {
    auto e = mk_tfg_edge(mk_itfg_edge(from_pc, pc_to, state_to_cft, to_condition/*, t.get_start_state()*/, assumes, te_comment));
    eimap.insert(make_pair(e, I));
    t.add_edge(e);
    return make_pair(from_node, map<string, sort_ref>());
  }
}

template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
void
sym_exec_common::process_cft_second_half(tfg &t, shared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, Instruction * I, const BASICBLOCK& B, const FUNCTION& F, map<std::string, sort_ref> const &phi_regnames, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  if (pc_to.is_label() && pc_to.get_subindex() == PC_SUBINDEX_FIRST_INSN_IN_BB) {
    process_phi_nodes_second_half<FUNCTION, BASICBLOCK, INSTRUCTION>(t, &B, pc_to, from_node, F, to_condition, phi_regnames, assumes, te_comment, I, eimap);
  }
}

template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
void
sym_exec_common::process_cfts(tfg &t, shared_ptr<tfg_node> const &from_node, pc const &pc_to, state const &state_to, unordered_set<expr_ref> const& state_assumes, te_comment_t const& te_comment, Instruction * I, vector<control_flow_transfer> const &cfts, BASICBLOCK const &B, FUNCTION const &F, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  map<pc, map<string, sort_ref>> pc_to_phi_regnames_map;
  if (cfts.size() == 0) {
    //pc pc_to(pc::insn_label, get_pc_from_bb_and_insn_id(B, next_insn_id));
    auto n = process_cft_first_half<FUNCTION, BASICBLOCK, INSTRUCTION>(t, from_node, pc_to, nullptr, expr_true(m_ctx), state_to, state_assumes, te_comment, I, B, F, eimap);
    ASSERT(n.first == from_node);
    ASSERT(n.second.size() == 0);
  } else {
    pc const &bb_last_pc = cfts.at(cfts.size() - 1).get_from_pc();
    shared_ptr<tfg_node> bb_last_node = t.find_node(bb_last_pc);
    ASSERT(bb_last_node);
    for(const control_flow_transfer& cft : cfts) {
      //first half performs all the computation required in the successor phi nodes, as part of the current basic block
      unordered_set<expr_ref> assumes = state_assumes;
      unordered_set_union(assumes, cft.get_assumes());
      auto r = process_cft_first_half<FUNCTION, BASICBLOCK, INSTRUCTION>(t, bb_last_node, cft.get_to_pc(), cft.get_target(), cft.get_condition(), state_to, assumes, te_comment, I, B, F, eimap);
      pc_to_phi_regnames_map.insert(make_pair(cft.get_to_pc(), r.second));
      bb_last_node = r.first;
    }
    for(const control_flow_transfer& cft : cfts) {
      //second half creates control-flow edges from the current basic block to the successor(s), while copying the computed phi expressions from their temporary registers to their actual registers. This division of labour between first-half and second-half mimics LLC's code generator
      process_cft_second_half<FUNCTION, BASICBLOCK, INSTRUCTION>(t, (bb_last_pc == cft.get_from_pc()) ? bb_last_node : t.find_node(cft.get_from_pc()), cft.get_to_pc(), cft.get_target(), cft.get_condition(), cft.get_assumes(), te_comment, I, B, F, pc_to_phi_regnames_map.at(cft.get_to_pc()), eimap);
    }
  }
}

void
sym_exec_llvm::add_edges(const llvm::BasicBlock& B, tfg& t, const llvm::Function& F, map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> *function_tfg_map, set<string> const *function_call_chain, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  //errs() << "Doing BB: " << get_basicblock_name(B) << "\n";
  size_t insn_id = 0;

  for (const Instruction& I : B) {
    if (isa<PHINode const>(I)) {
      continue;
    }

    pc pc_from = get_pc_from_bbindex_and_insn_id(get_basicblock_index(B), insn_id);
    shared_ptr<tfg_node> from_node = make_shared<tfg_node>(pc_from);
    if (!t.find_node(pc_from)) {
      t.add_node(from_node);
    }
    insn_id++;

    //exec(t.get_start_state(), I, from_node, B, F, insn_id, t, function_tfg_map, function_call_chain, eimap);
    exec(state(), I, from_node, B, F, insn_id, t, function_tfg_map, function_call_chain, eimap);
  }
}

template<typename FUNCTION, typename BASICBLOCK>
BASICBLOCK/*llvm::BasicBlock*/ const *
sym_exec_common::get_basic_block_for_pc(const FUNCTION/*llvm::Function*/& F, pc const &p)
{
  static map<string, BASICBLOCK/*llvm::BasicBlock*/ const *> cache;

  stringstream ss;
  //ss << string(F.getName()) << "." << p.get_index();
  ss << functionGetName(F) << "." << p.get_index();
  string id = ss.str();
  if (cache.count(id) > 0) {
    return cache.at(id);
  }
  //const BasicBlock* ret = NULL;
  for (const BASICBLOCK/*BasicBlock*/& B : F) {
    stringstream cur_ss;
    cur_ss << string(F.getName()) << "." << get_basicblock_index(B);
    string cur_id = cur_ss.str();
    cache[cur_id] = &B;
  }

  if (cache.count(id) > 0) {
    return cache.at(id);
  } else {
    /*if(get_basicblock_name(B) == p.get_id()) {
      ret = &B;
      return ret;
    }*/
    return NULL;
  }
}

shared_ptr<tfg_node>
sym_exec_common::get_next_intermediate_subsubindex_pc_node(tfg &t, shared_ptr<tfg_node> const &from_node)
{
  char const *index = from_node->get_pc().get_index();
  //int bblnum = from_node->get_pc().get_bblnum();
  int subindex = from_node->get_pc().get_subindex();
  pc p(pc::insn_label, index/*, bblnum*/, subindex, PC_SUBSUBINDEX_DEFAULT);
  DYN_DEBUG3(llvm2tfg, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << endl);
  if (m_intermediate_subsubindex_map.count(p) == 0) {
    DYN_DEBUG3(llvm2tfg, cout << __func__ << " " << __LINE__ << ": intermediate_subsubindex_map.count(p) = 0" << endl);
    m_intermediate_subsubindex_map[p] = PC_SUBSUBINDEX_INTERMEDIATE_VAL(0);
  } else {
    DYN_DEBUG3(llvm2tfg, cout << __func__ << " " << __LINE__ << ": intermediate_subsubindex_map.count(p) = " << m_intermediate_subsubindex_map.at(p) << endl);
    m_intermediate_subsubindex_map[p]++;
  }
  pc ret(pc::insn_label, index/*, bblnum*/, subindex, m_intermediate_subsubindex_map.at(p));
  if (t.find_node(ret) == 0) {
    t.add_node(make_shared<tfg_node>(ret));
  }
  return t.find_node(ret);
}

template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
pair<shared_ptr<tfg_node>, map<string, sort_ref>>
sym_exec_common::process_phi_nodes_first_half(tfg &t, const BASICBLOCK* B_from, const pc& pc_to, shared_ptr<tfg_node> const &from_node, const FUNCTION& F, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": searching for BB representing " << pc_to.to_string() << endl);
  const BASICBLOCK* B_to = 0;
  B_to = get_basic_block_for_pc<FUNCTION, BASICBLOCK>(F, pc_to);
  assert(B_to);
  DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": copying state" << endl);

  DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": searching for phi instruction" << endl);
  shared_ptr<tfg_node> pc_to_phi_node = from_node;

  //shared_ptr<tfg_node> pc_to_phi_start_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
  //auto e1 = make_shared<tfg_edge>(pc_to_phi_node->get_pc(), pc_to_phi_start_node->get_pc(), t.get_start_state(), edgecond, t.get_start_state());
  //auto e1 = make_shared<tfg_edge>(pc_to_phi_node->get_pc(), pc_to_phi_start_node->get_pc(), t.get_start_state(), expr_true(m_ctx), t.get_start_state());
  //t.add_edge(e1);
  //pc_to_phi_node = pc_to_phi_start_node;

  DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": pc_to_phi = " << pc_to_phi_node->get_pc().to_string() << endl);
  map<string, sort_ref> changed_varnames;
  int inum = 0;
  for (const INSTRUCTION& I : *B_to) {
    string varname;
    if (!instructionIsPhiNode(I, varname)) {
      continue;
    }
    inum++;

    state state_out;
    unordered_set<expr_ref> state_assumes;
    expr_ref val;
    tie(val, state_assumes) = phiInstructionGetIncomingBlockValue(I/*, t.get_start_state()*/, pc_to_phi_node, pc_to, B_from, F, t);
    DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": found phi instruction: from_node = " << from_node->get_pc() << ": pc_to_phi_node = " << pc_to_phi_node->get_pc() << endl);
    //expr_ref e = get_expr_adding_edges_for_intermediate_vals(*val/*phi->getIncomingValue(i)*/, "", t.get_start_state(), pc_to_phi_node, pc_to, *B_from, F, t);
    DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": after adding expr edges: from_node = " << from_node->get_pc() << ": pc_to_phi_node = " << pc_to_phi_node->get_pc() << ", val = " << expr_string(val) << endl);
    //string varname = get_value_name(*phi);
    changed_varnames.insert(make_pair(varname, val->get_sort()));
    state_set_expr(state_out, varname + PHI_NODE_TMPVAR_SUFFIX, val); //first update the tmpvars (do not want updates of one phi-node to influene the rhs of another phi-node.

    shared_ptr<tfg_node> pc_to_phi_dst_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": from_node = " << from_node->get_pc() << ": pc_to_phi_node = " << pc_to_phi_node->get_pc() << ", pc_to_phi_dst_node = " << pc_to_phi_dst_node->get_pc() << endl);
    string const& bbindex = get_basicblock_index(*B_to);
    bbl_order_descriptor_t const& bbo = this->m_bbl_order_map.at(mk_string_ref(bbindex));
    auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to_phi_dst_node->get_pc(), state_out, expr_true(m_ctx)/*, t.get_start_state()*/, state_assumes, phi_node_to_te_comment(bbo, inum, I)));
    eimap.insert(make_pair(e, (Instruction *)&I));
    t.add_edge(e);
    DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": adding phi edge: " << e->to_string(/*&t.get_start_state()*/) << endl);
    pc_to_phi_node = pc_to_phi_dst_node;
  }
  return make_pair(pc_to_phi_node, changed_varnames);
}

template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
void
sym_exec_common::process_phi_nodes_second_half(tfg &t, const BASICBLOCK* B_from, const pc& pc_to, shared_ptr<tfg_node> const &from_node, const FUNCTION& F, expr_ref edgecond, map<string, sort_ref> const &phi_regnames, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, Instruction * I, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  shared_ptr<tfg_node> pc_to_phi_node = from_node;

  shared_ptr<tfg_node> pc_to_phi_start_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
  auto e1 = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to_phi_start_node->get_pc(), state()/*t.get_start_state()*/, edgecond/*, t.get_start_state()*/, assumes, te_comment));
  eimap.insert(make_pair(e1, I));
  t.add_edge(e1);
  //cout << __func__ << " " << __LINE__ << ": t.incoming =\n" << t.incoming_sizes_to_string() << endl;
  pc_to_phi_node = pc_to_phi_start_node;

  for (const auto &cvarname_sort : phi_regnames) {
    string const &cvarname = cvarname_sort.first;
    sort_ref const &cvarsort = cvarname_sort.second;
    state state_out;
    state_set_expr(state_out, cvarname, m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + cvarname + PHI_NODE_TMPVAR_SUFFIX, cvarsort));
    shared_ptr<tfg_node> pc_to_phi_dst_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to_phi_dst_node->get_pc(), state_out, expr_true(m_ctx)/*, t.get_start_state()*/, {}, te_comment));
    eimap.insert(make_pair(e, I));
    t.add_edge(e);
    //cout << __func__ << " " << __LINE__ << ": t.incoming =\n" << t.incoming_sizes_to_string() << endl;
    DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": adding phi edge: " << e->to_string(/*&t.get_start_state()*/) << endl);
    pc_to_phi_node = pc_to_phi_dst_node;
  }
  DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": all done" << endl);
  //auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to, t.get_start_state(), expr_true(m_ctx)/*, t.get_start_state()*/, {}, te_comment));
  auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to, state()/*t.get_start_state()*/, expr_true(m_ctx)/*, t.get_start_state()*/, {}, te_comment));
  eimap.insert(make_pair(e, I));
  t.add_edge(e);
  //cout << __func__ << " " << __LINE__ << ": t.incoming =\n" << t.incoming_sizes_to_string() << endl;
}



/*pc
sym_exec_llvm::get_next_phi_pc(pc const &p)
{
  ASSERT(p.get_subsubindex() == 0);
  if (m_next_phi_pc.count(p)) {
    return m_next_phi_pc.at(p);
  }
  pc p_phi = p;
  p_phi.set_subsubindex(PC_SUBSUBINDEX_PHI_NODE(0));
  return p_phi;
}*/

/*void
sym_exec_llvm::set_next_phi_pc(pc const &p, pc const &phi_pc)
{
  m_next_phi_pc[p] = phi_pc;
}*/

map<nextpc_id_t, callee_summary_t>
sym_exec_llvm::get_callee_summaries_for_tfg(map<nextpc_id_t, string> const &nextpc_map, map<string, callee_summary_t> const &callee_summaries)
{
  map<nextpc_id_t, callee_summary_t> ret;
  for (auto ncsum : callee_summaries) {
    string nextpc_str = ncsum.first;
    callee_summary_t csum = ncsum.second;
    nextpc_id_t nextpc_id = -1;
    for (auto npc : nextpc_map) {
      if (npc.second == nextpc_str) {
        nextpc_id = npc.first;
        break;
      }
    }
    ASSERT(nextpc_id != -1);
    ret[nextpc_id] = csum;
  }
  return ret;
}

void
sym_exec_llvm::sym_exec_preprocess_tfg(string const &name, tfg_llvm_t& t_src, map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> &function_tfg_map, list<string> const& sorted_bbl_indices)
{
  autostop_timer func_timer(__func__);
  context* ctx = this->get_context();
  consts_struct_t &cs = ctx->get_consts_struct();
  map<local_id_t, graph_local_t> local_refs = this->get_local_refs();

  pc start_pc = this->get_start_pc();
  t_src.add_extra_node_at_start_pc(start_pc);

  unordered_set<expr_ref> const& arg_assumes = this->gen_arg_assumes();
  t_src.add_assumes_to_start_edge(arg_assumes);

  t_src.set_symbol_map_for_touched_symbols(*m_symbol_map, m_touched_symbols);
  t_src.set_string_contents_for_touched_symbols_at_zero_offset(*m_string_contents, m_touched_symbols);
  t_src.remove_function_name_from_symbols(name);
  t_src.populate_exit_return_values_for_llvm_method();
  t_src.canonicalize_llvm_nextpcs();
  t_src.tfg_llvm_interpret_intrinsic_fcalls();

  map<nextpc_id_t, callee_summary_t> nextpc_id_csum = sym_exec_llvm::get_callee_summaries_for_tfg(t_src.get_nextpc_map(), m_callee_summaries);
  t_src.set_callee_summaries(nextpc_id_csum);

  ASSERT(t_src.get_locals_map().size() == 0);
  //vector<pair<string_ref, size_t>> locals_map;
  //for (auto const &local : local_refs) {
  //  locals_map.push_back(local);
  //}
  //t_src.set_locals_map(locals_map);
  t_src.set_locals_map(local_refs);
  t_src.tfg_preprocess(false, sorted_bbl_indices);
  CPP_DBG_EXEC(EQGEN, cout << __func__ << " " << __LINE__ << ": after preprocess, TFG:\n" << t_src.graph_to_string() << endl);
}

bool
sym_exec_common::update_function_call_args_and_retvals_with_atlocals(unique_ptr<tfg> t_src)
{
  return false; //NOT_IMPLEMENTED();
}

unordered_set<expr_ref>
sym_exec_llvm::gen_arg_assumes() const
{
  unordered_set<expr_ref> arg_assumes;
  for (const auto& arg : m_function.args()) {
    pair<argnum_t, expr_ref> const &a = m_arguments.at(get_value_name(arg));
    string Elname = get_value_name(arg) + SRC_INPUT_ARG_NAME_SUFFIX;
    Type *ElTy = arg.getType();
    unordered_set_union(arg_assumes, gen_align_assumes(Elname, ElTy, a.second->get_sort()));
  }
  return arg_assumes;
}

unique_ptr<tfg_llvm_t>
sym_exec_llvm::get_preprocessed_tfg_common(string const &name, map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> &function_tfg_map, set<string> function_call_chain, list<string> const& sorted_bbl_indices, bool DisableModelingOfUninitVarUB)
{
  autostop_timer func_timer(__func__);
  DYN_DEBUG(llvm2tfg,
    errs() << "Doing: " << name << "\n";
    errs().flush();
    outs() << "Doing: " << name << "\n";
    outs().flush();
  );
  function_call_chain.insert(name);
  map<shared_ptr<tfg_edge const>, Instruction *> eimap;
  unique_ptr<tfg_llvm_t> t_src = this->get_tfg(&function_tfg_map, &function_call_chain, eimap);
  this->sym_exec_preprocess_tfg(name, *t_src, function_tfg_map, sorted_bbl_indices);
  DYN_DEBUG(llvm2tfg,
    errs() << "Doing done: " << name << "\n";
    errs().flush();
    outs() << "Doing done: " << name << "\n";
    outs().flush();
  );

  if (!DisableModelingOfUninitVarUB) {
    map<pc, map<graph_loc_id_t, bool>> init_status;
    list<pc> exit_pcs;
    t_src->populate_loc_definedness();
    //init_status = t_src->determine_initialization_status_for_locs();
    t_src->get_exit_pcs(exit_pcs);
    for (auto exit_pc : exit_pcs) {
      if (t_src->return_register_is_uninit_at_exit(G_LLVM_RETURN_REGISTER_NAME, exit_pc/*, init_status*/)) {
        t_src->eliminate_return_reg_at_exit(G_LLVM_RETURN_REGISTER_NAME, exit_pc);
      }
    }
  }
  return t_src;
}

unique_ptr<tfg_llvm_t>
sym_exec_llvm::get_preprocessed_tfg(Function const &f, Module const *M, string const &name, context *ctx, map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> &function_tfg_map, set<string> function_call_chain, bool gen_callee_summary, bool DisableModelingOfUninitVarUB)
{
  autostop_timer func_timer(__func__);

  //list<pair<string, unsigned>> fun_names;
  //sym_exec_common::get_fun_names(M, fun_names);

  //pair<graph_symbol_map_t, map<symbol_id_t, vector<char>>> symbol_map_and_string_contents = sym_exec_common::get_symbol_map_and_string_contents(M, fun_names);
  //graph_symbol_map_t const& symbol_map = symbol_map_and_string_contents.first;
  //map<symbol_id_t, vector<char>> const &string_contents = symbol_map_and_string_contents.second;
  //consts_struct_t const& cs = ctx->get_consts_struct();

  sym_exec_llvm se(ctx/*, cs*/, M, f/*, fun_names, symbol_map, string_contents*/, gen_callee_summary, BYTE_LEN, DWORD_LEN);

  list<string> sorted_bbl_indices;
  for (BasicBlock const& BB: f) {
    sorted_bbl_indices.push_back(se.get_basicblock_index(BB));
  }

  return se.get_preprocessed_tfg_common(name, function_tfg_map, function_call_chain, sorted_bbl_indices, DisableModelingOfUninitVarUB);
}

unique_ptr<tfg>
sym_exec_llvm::get_preprocessed_tfg_for_machine_function(MachineFunction const &mf, Function const &f, Module const *M, string const &name, context *ctx, list<pair<string, unsigned>> const &fun_names, graph_symbol_map_t const &symbol_map, map<pair<symbol_id_t, offset_t>, vector<char>> const &string_contents, consts_struct_t &cs, map<string, pair<callee_summary_t, unique_ptr<tfg>>> &function_tfg_map, set<string> function_call_chain, bool gen_callee_summary, bool DisableModelingOfUninitVarUB)
{
  NOT_IMPLEMENTED();
  //sym_exec_mir se(ctx, cs, M, f, mf, fun_names, symbol_map, string_contents, gen_callee_summary, BYTE_LEN, DWORD_LEN);
  //return get_preprocessed_tfg_common(se, name, function_tfg_map, function_call_chain, DisableModelingOfUninitVarUB);
}


list<pair<string, unsigned>>
sym_exec_common::get_fun_names(llvm::Module const *M)
{
  list<pair<string, unsigned>> fun_names;
  assert(M);
  for(const auto& f : *M)
  {
    unsigned insn_count = get_num_insn(f);
    if(insn_count == 0)
      continue;
    fun_names.push_back({f.getName().str(), insn_count});
  }

  fun_names.sort([](const pair<string, unsigned>& a, const pair<string, unsigned>& b){ return a.second < b.second; });
  return fun_names;
}

pair<graph_symbol_map_t, map<pair<symbol_id_t, offset_t>, vector<char>>>
sym_exec_common::get_symbol_map_and_string_contents(Module const *M, list<pair<string, unsigned>> const &fun_names)
{
  graph_symbol_map_t smap;
  map<pair<symbol_id_t, offset_t>, vector<char>> scontents;
  symbol_id_t symbol_id = 1;
  for (auto &g : M->getGlobalList()) {
    const DataLayout &dl = M->getDataLayout();
    Type *ElTy = g.getType()->getElementType();
    /*if (   ElTy->getTypeID() == Type::FunctionTyID
        || (   ElTy->getTypeID() == Type::PointerTyID
            && cast<PointerType>(ElTy)->getElementType()->getTypeID() == Type::FunctionTyID)) {
      continue;
    }*/
    uint64_t symbol_size = 0;
    unsigned symbol_alignment = 0;
    bool symbol_is_constant = false;
    assert(g.hasName());
    string name = sym_exec_llvm::get_value_name(g);
    ASSERT(name.substr(0, strlen(LLVM_GLOBAL_VARNAME_PREFIX)) == LLVM_GLOBAL_VARNAME_PREFIX);
    name = name.substr(strlen(LLVM_GLOBAL_VARNAME_PREFIX));
    //cout << __func__ << " " << __LINE__ << ": symbol_id = " << symbol_id << endl;
    //cout << __func__ << " " << __LINE__ << ": name = " << name << endl;
    //cout << __func__ << " " << __LINE__ << ": ElTyID = " << ElTy->getTypeID() << endl;
    const GlobalVariable *cv = dyn_cast<const GlobalVariable>(&g);
    ASSERT(cv);
    symbol_size = dl.getTypeAllocSize(ElTy);
    symbol_alignment = dl.getPreferredAlignment(cv);
    symbol_is_constant = cv->isConstant();
    //cout << __func__ << " " << __LINE__ << ": symbol_is_constant = " << symbol_is_constant << endl;
    smap.insert(make_pair(symbol_id, graph_symbol_t(mk_string_ref(name), symbol_size, symbol_alignment, symbol_is_constant)));
    if (symbol_is_constant && cv->hasInitializer()) {
      StringRef str;
      const ConstantDataArray *Array;
      const ConstantInt *Int;

      //cout << __func__ << " " << __LINE__ << ": Array = " << Array << endl;
      //XXX: handle all cases using lib/IR/AsmWriter.cpp:WriteConstantInternal()
      if ((Array = dyn_cast<ConstantDataArray>(cv->getInitializer()))/* && Array->isString()*/) {
        // Get the number of elements in the array
        //uint64_t NumElts = Array->getType()->getArrayNumElements();

        // Start out with the entire array in the StringRef.
        //str = Array->getAsString();
        str = Array->getRawDataValues();
        //cout << __func__ << " " << __LINE__ << ": name = " << name << ", str = " << str.data() << "\n";
        //m_string_contents[name] = str;
        vector<char> v;
        for (size_t i = 0; i < str.size(); i++) {
          v.push_back(str[i]);
        }
        scontents[make_pair(symbol_id, 0)] = v;
      } else if ((Int = dyn_cast<ConstantInt>(cv->getInitializer()))) {
        unsigned bitwidth = Int->getBitWidth();
        ASSERT(bitwidth <= QWORD_LEN);
        ASSERT((bitwidth % BYTE_LEN) == 0);
        uint64_t val = Int->getZExtValue();
        vector<char> v;
        for (unsigned i = 0; i < bitwidth; i += BYTE_LEN) {
          v.push_back(val & MAKE_MASK(BYTE_LEN));
          val = val >> BYTE_LEN;
        }
        //cout << __func__ << " " << __LINE__ << ": name = " << name << ", str = " << str.data() << ", val = " << val << ", bitwidth = " << bitwidth << "\n";
        scontents[make_pair(symbol_id, 0)] = v;
      }
    }
    symbol_id++;
  }
  for (const auto &fun_name : fun_names) {
    smap.insert(make_pair(symbol_id, graph_symbol_t(mk_string_ref(fun_name.first), fun_name.second, ALIGNMENT_FOR_FUNCTION_SYMBOL, false)));
    symbol_id++;
  }
  return make_pair(smap, scontents);
}

graph_symbol_map_t
sym_exec_common::get_symbol_map(Module const *M)
{
  list<pair<string, unsigned>> fun_names = get_fun_names(M);

  pair<graph_symbol_map_t, map<pair<symbol_id_t, offset_t>, vector<char>>> pr = get_symbol_map_and_string_contents(M, fun_names);
  return pr.first;
}

map<pair<symbol_id_t, offset_t>, vector<char>>
sym_exec_common::get_string_contents(Module const *M)
{
  list<pair<string, unsigned>> fun_names = get_fun_names(M);

  pair<graph_symbol_map_t, map<pair<symbol_id_t, offset_t>, vector<char>>> pr = get_symbol_map_and_string_contents(M, fun_names);
  return pr.second;
}

unsigned
sym_exec_common::get_num_insn(const Function& f)
{
  unsigned ret = 0;
  for(const BasicBlock& B : f)
    ret += B.getInstList().size();

  return ret;
}

pc
sym_exec_llvm::get_start_pc() const
{
  return this->get_pc_from_bbindex_and_insn_id(get_basicblock_index(*m_function.begin()), 0);
}

void
sym_exec_mir::populate_state_template_for_mf(MachineFunction const &mf)
{
  NOT_REACHED();
#if 0
  argnum_t argnum = 0;
  MachineFrameInfo const &MFI = mf.getFrameInfo();
  int FI = MFI.getObjectIndexBegin();
  for (; MFI.isFixedObjectIndex(FI); ++FI) {
    //int64_t ObjBegin = MFI.getObjectOffset(FI);
    //int64_t ObjEnd = ObjBegin + MFI.getObjectSize(FI);
    //if (ObjBegin <= PartBegin && PartEnd <= ObjEnd)
    //  break;
    stringstream ss;
    ss << MIR_FIXED_STACK_SLOT_PREFIX "." << argnum;
    string argname = ss.str();
    sort_ref s = m_ctx->mk_bv_sort(DWORD_LEN);
    expr_ref argvar = m_ctx->mk_var(string(G_INPUT_KEYWORD) + "." + argname, s);
    m_arguments[argname] = make_pair(argnum, argvar);
    //cout << __func__ << " " << __LINE__ << ": adding " << argname << endl;
    m_state_templ.push_back({argname, s});
    argnum++;
  }

  MachineConstantPool const *MCP = mf.getConstantPool();
  for (unsigned i = 0; i != MCP->getConstants().size(); i++) {
    NOT_IMPLEMENTED();
  }
  TargetInstrInfo const &TII = *mf.getSubtarget().getInstrInfo();
  const MachineRegisterInfo &MRI = mf.getRegInfo();


  for (const MachineBasicBlock& B : mf) {
    m_basicblock_idx_map[get_machine_basicblock_name(B)] = B.getNumber() + 1;  //bbnum == 0 is reserved
    //bbnum++;
    int insn_id = 0;
    for(const MachineInstr& I : B)
    {
      if (I.getOpcode() != TargetOpcode::PHI) {//!isa<const PHINode>(I)
        insn_id++;
      }
      if (TII.getName(I.getOpcode()) != MIR_RET_NODE_OPCODE) {//isa<ReturnInst>(I)
      //if (I.getOpcode() == X86::RET)
        m_ret_reg = G_LLVM_RETURN_REGISTER_NAME;
        //return_reg_sort = get_value_type(*ret_val);
        sort_ref return_reg_sort;
        return_reg_sort = m_ctx->mk_bv_sort(DWORD_LEN);
        //cout << __func__ << " " << __LINE__ << ": adding " << m_ret_reg << endl;
        m_state_templ.push_back({m_ret_reg, return_reg_sort});
        continue;
      }
      /*if (isa<SwitchInst>(I)) { //XXX: TODO
        const SwitchInst* SI =  cast<const SwitchInst>(&I);
        size_t varnum = 0;
        for (SwitchInst::ConstCaseIt i = SI->case_begin(), e = SI->case_end(); i != e; ++i) {
          stringstream ss;
          ss << LLVM_SWITCH_TMPVAR_PREFIX << varnum;
          varnum++;
          m_state_templ.push_back({ss.str(), m_ctx->mk_bool_sort()});
        }
        continue;
      }*/

      for (unsigned StartOp = 0; StartOp < I.getNumOperands(); StartOp++) {
        MachineOperand const &MO = I.getOperand(StartOp);
        if (!MO.isReg() || !MO.isDef() || MO.isImplicit()) {
          continue;
        }
        unsigned Reg = MO.getReg();
        if (TargetRegisterInfo::isVirtualRegister(Reg)) {
          string name = MRI.getVRegName(Reg);
          if (name != "") {
            name = string("%") + name;
          } else {
            stringstream ss;
            ss << "%" << TargetRegisterInfo::virtReg2Index(Reg);
            name = ss.str();
          }
          LLT type = MRI.getType(Reg);
          //sort_ref s = get_value_type(I);
          sort_ref s = m_ctx->mk_bv_sort(DWORD_LEN);
          //cout << __func__ << " " << __LINE__ << ": adding " << name << endl;
          m_state_templ.push_back({name, s});
          //if (TII.getName(I.getOpcode()) != MIR_PHI_NODE_OPCODE)
          if (I.getOpcode() != TargetOpcode::PHI) {
            m_state_templ.push_back({name + PHI_NODE_TMPVAR_SUFFIX, s});
          }
        }
      }
    }
  }
  populate_state_template_common();
#endif
}

void sym_exec_mir::add_edges_for_mir(const llvm::MachineBasicBlock& B, tfg& t, const llvm::MachineFunction& F, map<string, pair<callee_summary_t, unique_ptr<tfg>>> &function_tfg_map, set<string> const &function_call_chain)
{
  size_t insn_id = 0;

  TargetInstrInfo const &TII = *m_machine_function.getSubtarget().getInstrInfo();
  for (const MachineInstr& I : B) {
    if (I.getOpcode() == TargetOpcode::PHI) {
      continue;
    }

    pc pc_from = get_pc_from_bbindex_and_insn_id(get_basicblock_index(B), insn_id);

    //state state_start;
    //get_state_template(pc_from, state_start);

    shared_ptr<tfg_node> from_node((tfg_node*)(new tfg_node(pc_from)));
    if (!t.find_node(pc_from)) {
      t.add_node(from_node);
    }
    //state state_to(t.get_start_state());
    insn_id++;

    //errs() << "from state:\n" << state_to.to_string();
    //exec(state_to, I, state_to, cfts, expand_switch_flag, assumes, F.getName(), t, function_tfg_map);
    //errs() << "to state:\n" << state_to.to_string();

    exec_mir(/*t.get_start_state()*/state(), I/*, state_to, cfts, expand_switch_flag, assumes*/, from_node, B/*, F*/, insn_id, t, function_tfg_map, function_call_chain);

    //t.add_assumes(pc_from, assumes);
    //errs() << __func__ << " " << __LINE__ << ": insn_id = " << insn_id << "\n";
  }
}

void
sym_exec_common::get_tfg_common(tfg &t)
{
  map<string_ref, expr_ref> arg_exprs;
  //unordered_set<predicate> assumes;
  for (const auto& arg : m_arguments) {
    pair<argnum_t, expr_ref> const &a = arg.second;
    stringstream ss;
    ss << LLVM_METHOD_ARG_PREFIX << a.first;
    arg_exprs.insert(make_pair(mk_string_ref(ss.str()), a.second));
  }
  //t->add_assumes(pc::start(), assumes);

  state start_state;
  get_state_template(pc::start(), start_state);
  //arg_exprs.push_back(t.find_entry_node()->get_state().get_expr(m_mem_reg));
  t.set_argument_regs(arg_exprs);
  get_state_template(pc::start(), start_state);
  t.set_start_state(start_state);
}

unique_ptr<tfg_llvm_t>
sym_exec_mir::get_tfg(map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> *function_tfg_map, set<string> const *function_call_chain, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  NOT_REACHED();
/*
  llvm::MachineFunction const &F = m_machine_function;
  populate_state_template_for_mf(m_machine_function);
  state start_state;
  get_state_template(pc::start(), start_state);
  unique_ptr<tfg> t = make_unique<tfg>("mir", m_ctx);
  t->set_start_state(start_state);
  for(const MachineBasicBlock& B : F) {
    this->add_edges_for_mir(B, *t, F, *function_tfg_map, *function_call_chain);
  }

  this->get_tfg_common(*t);
  return t;
*/
}

pc
sym_exec_mir::get_start_pc() const
{
  return this->get_pc_from_bbindex_and_insn_id(get_basicblock_index(*m_machine_function.begin()), 0);
}

string
sym_exec_mir::get_machine_basicblock_name(MachineBasicBlock const &mb) const
{
  stringstream ss;
  ss << "%bb." << mb.getNumber() + 1;
  return ss.str();
}

pair<expr_ref, unordered_set<expr_ref>>
sym_exec_llvm::phiInstructionGetIncomingBlockValue(llvm::Instruction const &I, shared_ptr<tfg_node> &pc_to_phi_node, pc const &pc_to, llvm::BasicBlock const *B_from, llvm::Function const &F, tfg &t)
{
  const PHINode* phi = dyn_cast<const PHINode>(&I);
  ASSERT(phi);

  for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
    if (phi->getIncomingBlock(i) != B_from) {
      continue;
    }
    expr_ref e;
    unordered_set<expr_ref> assumes;
    tie(e, assumes) = get_expr_adding_edges_for_intermediate_vals(*phi->getIncomingValue(i), "", state()/*t.get_start_state()*/, assumes, pc_to_phi_node, pc_to, *B_from, F, t);
    return make_pair(e, assumes);
  }
  NOT_REACHED();
}

string
sym_exec_llvm::functionGetName(llvm::Function const &F) const
{
  return F.getName().str();
}

//string
//sym_exec_llvm::get_basicblock_index(llvm::BasicBlock const &F) const
//{
//  NOT_IMPLEMENTED();
//}

//string
//sym_exec_llvm::get_basicblock_name(llvm::BasicBlock const &F) const
//{
//  NOT_IMPLEMENTED();
//}

bool
sym_exec_llvm::instructionIsPhiNode(llvm::Instruction const &I, string &varname) const
{
  const PHINode* phi = dyn_cast<const PHINode>(&I);
  if(!phi) {
    return false;
  }
  varname = get_value_name(*phi);
  return true;
}

void
sym_exec_llvm::populate_bbl_order_map()
{
  llvm::Function const &F = m_function;
  int bblnum = 0;
  for(BasicBlock const& B : F) {
    string const& bbindex = get_basicblock_index(B);
    this->m_bbl_order_map.insert(make_pair(mk_string_ref(bbindex), bbl_order_descriptor_t(bblnum, bbindex)));
    bblnum++;
  }
}

string
sym_exec_llvm::get_basicblock_index(const llvm::BasicBlock &v) const
{
  string s = get_basicblock_name(v);
  //ASSERT(m_basicblock_idx_map.count(s));
  ASSERT(s.substr(0, 1) == "%");
  string s1 = s.substr(1);
  /*int ret;
  if (stringIsInteger(s1)) {
    ret = stoi(s1);
    if (m_basicblock_idx_map.at(s) == 0 && ret != 0) {
      ret = m_basicblock_idx_map.at(s);
    }
  } else {
    ret = m_basicblock_idx_map.at(s);
    if (ret != 0) {
      ret += LLVM_BASICBLOCK_COUNTING_IDX_START;
    }
  }
  return ret;*/
  return s1;
}
