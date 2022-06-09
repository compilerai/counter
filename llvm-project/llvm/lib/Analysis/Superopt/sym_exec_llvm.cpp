#include <tuple>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/Debugify.h"

#include "support/dyn_debug.h"
#include "support/globals.h"
#include "support/dshared_ptr.h"
#include "support/mybitset.h"

#include "expr/expr.h"

#include "gsupport/predicate.h"
#include "gsupport/scev.h"

#include "tfg/tfg_llvm.h"

#include "ptfg/llvm_value_id.h"
#include "ptfg/llptfg.h"

#include "sym_exec_llvm.h"
//#include "sym_exec_mir.h"
#include "dfa_helper.h"

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

sort_ref sym_exec_common::get_mem_alloc_sort() const
{
  return m_ctx->mk_array_sort(get_mem_alloc_domain(), get_mem_alloc_range());
}

sort_ref sym_exec_common::get_mem_alloc_domain() const
{
  return m_ctx->mk_bv_sort(m_word_length);
}

sort_ref sym_exec_common::get_mem_alloc_range() const
{
  return m_ctx->mk_memlabel_sort();
}

sort_ref sym_exec_common::get_mem_poison_sort() const
{
  return m_ctx->mk_array_sort(get_mem_poison_domain(), get_mem_poison_range());
}

sort_ref sym_exec_common::get_mem_poison_domain() const
{
  return m_ctx->mk_bv_sort(m_word_length);
}

sort_ref sym_exec_common::get_mem_poison_range() const
{
  return m_ctx->mk_bool_sort();
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
sym_exec_common::get_value_name(const Value& v) const
{
  return get_value_name_using_srcdst_keyword(v, m_srcdst_keyword);
}

string
sym_exec_common::get_value_name_using_srcdst_keyword(const Value& v, string const& srcdst_keyword)
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
    return srcdst_keyword + string("." G_LLVM_PREFIX) + "-" + ret;
  }
}

//expr_ref
//sym_exec_mir::get_mir_frameindex_var_expr(int frameindex)
//{
//  NOT_IMPLEMENTED();
//  //return m_ctx->mk_var(name, m_ctx->mk_bv_sort(DWORD_LEN));
//}

//expr_ref
//sym_exec_mir::mir_get_value_expr(MachineOperand const &v)
//{
//  NOT_IMPLEMENTED();
//  //if (v.getType() == MachineOperand::MO_GlobalAddress) {
//  //  string name = this->get_value_name(*v.getGlobal());
//  //  ASSERT(name.substr(0, strlen(LLVM_GLOBAL_VARNAME_PREFIX)) == LLVM_GLOBAL_VARNAME_PREFIX);
//  //  name = name.substr(strlen(LLVM_GLOBAL_VARNAME_PREFIX));
//  //  sort_ref sr = get_value_type(*v.getGlobal());
//  //  return get_symbol_expr_for_global_var(name, sr);
//  //  //errs() << ": parsing global address: " << v << "\n";
//  //} else if (v.getType() == MachineOperand::MO_FrameIndex) {
//  //  NOT_IMPLEMENTED();
//  //  //return get_mir_frameindex_var_expr(v.GetIndex());
//  //} else {
//  //  NOT_IMPLEMENTED();
//  //}
//}

//void sym_exec_mir::exec_mir(const state& state_in, const llvm::MachineInstr& I/*, state& state_out, vector<control_flow_transfer>& cfts, bool &expand_switch_flag, unordered_set<predicate> &assumes*/, shared_ptr<tfg_node> from_node, llvm::MachineBasicBlock const &B, size_t next_insn_id, tfg &t, map<string, pair<callee_summary_t, unique_ptr<tfg>>> &function_tfg_map, set<string> const &function_call_chain)
//{
//  NOT_IMPLEMENTED();
//#if 0
//  //CPP_DBG_EXEC(LLVM2TFG, errs() << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": sym exec doing: " << I << "\n");
//  state state_out = state_in;
//  vector<control_flow_transfer> cfts;
//  //unordered_set<predicate> assumes;
//  //bool expand_switch_flag = false;
//  //bool cft_processing_needed = true;
//  MachineFunction const &F = m_machine_function;
//  //string const &cur_function_name = F.getName(); //NOT_IMPLEMENTED
//  pc pc_to = get_pc_from_bbindex_and_insn_id(get_basicblock_index(B), next_insn_id);
//  errs() << "exec_mir: " << I << "\n";
//
//  switch(I.getOpcode())
//  {
//  case X86::MOV32rm:
//  case X86::MOV32ri: {
//    const auto &dstOp = I.getOperand(0);
//    ASSERT(dstOp.getType() == MachineOperand::MO_Register);
//    const auto &srcOp = I.getOperand(1);
//    expr_ref insn_expr = mir_get_value_expr(srcOp);
//    mir_set_expr(dstOp, insn_expr, state_out);
//  }
//  case X86::MOV32r0: {
//    const auto &dstOp = I.getOperand(0);
//    ASSERT(dstOp.getType() == MachineOperand::MO_Register);
//    mir_set_expr(dstOp, m_ctx->mk_zerobv(DWORD_LEN), state_out);
//    break;
//  }
//  default: {
//    errs() << "unsupported opcode: " << I << "\n";
//    NOT_IMPLEMENTED();
//    break;
//  }
//  }
//
//  errs() << __func__ << " " << __LINE__ << ": state_out =\n" << state_out.to_string_for_eq() << "\n";
//  process_cfts<llvm::MachineFunction, llvm::MachineBasicBlock, llvm::MachineInstr>(t, from_node, pc_to, state_out, cfts, B, F);
//  //CPP_DBG_EXEC(LLVM2TFG, errs() << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": sym exec done\n");
//  ////t.add_assumes(pc_from, assumes);
//#endif
//}

//string
//sym_exec_mir::get_basicblock_index(const llvm::MachineBasicBlock &v) const
//{
//  string s = get_machine_basicblock_name(v);
//  ASSERT(m_basicblock_idx_map.count(s));
//  ASSERT(s.substr(0, 1) == "%");
//  string s1 = s.substr(1);
//  return s1;
//}

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
sym_exec_llvm::get_const_value_expr(const llvm::Value& v, string const& vname, const state& state_in, unordered_set<expr_ref> const& state_assumes, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map)
{
  assert(isa<const llvm::Constant>(&v));
  if(const ConstantInt* c = dyn_cast<const ConstantInt>(&v))
  {
    unsigned size = c->getBitWidth();
    if (size > 1) {
      APInt const& ai = c->getValue();
      return make_pair(m_ctx->mk_bv_const(size, get_mybitset_from_apint(ai, ai.getBitWidth(), false)), state_assumes);
    } else if(c->getZExtValue() == 0) {
      return make_pair(m_ctx->mk_bool_false(), state_assumes);
    } else {
      return make_pair(m_ctx->mk_bool_true(), state_assumes);
    }
  }
  else if(const ConstantFP* c = dyn_cast<const ConstantFP>(&v))
  {
    sort_ref s = get_type_sort(v.getType(), m_module->getDataLayout());

    if (s->is_float_kind()) {
      float_max_t d = c->getValueAPF().convertToDouble();
      return make_pair(m_ctx->mk_float_const(s->get_size(), d), state_assumes);
      //return make_pair(m_ctx->mk_float_const(ai.getBitWidth(), mbs), state_assumes);
    } else if (s->is_floatx_kind()) {
      APInt ai = c->getValueAPF().bitcastToAPInt();
      mybitset mbs = get_mybitset_from_apint(ai, ai.getBitWidth(), false);
      return make_pair(m_ctx->mk_floatx_const(ai.getBitWidth(), mbs), state_assumes);
    } else NOT_REACHED();
  }
  else if (ConstantExpr const* ce = (ConstantExpr const*)dyn_cast<const ConstantExpr>(&v))
  {
    Instruction *i_sp = ce->getAsInstruction();
    //ASSERT(from_node);
    //ASSERT(B);
    //ASSERT(F);
    vector<expr_ref> expr_args;
    unordered_set<expr_ref> assumes = state_assumes;
    tie(expr_args, assumes) = get_expr_args(*i_sp, vname, state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    expr_ref ce_expr;
    tie(ce_expr, assumes) = exec_gen_expr(*i_sp, vname, expr_args, state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    string ce_key_name = constexpr_instruction_get_name(*i_sp);

    state state_to_intermediate_val;
    state_set_expr(state_to_intermediate_val, ce_key_name, ce_expr);
    dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx), {}, this->instruction_to_te_comment(*i_sp, from_node->get_pc())));
    t.add_edge(e);
    from_node = intermediate_node;

    ce_expr = mk_fresh_expr(ce_key_name, G_INPUT_KEYWORD, ce_expr->get_sort());

    if (value_to_name_map) {
      llvm_value_id_t llvm_value_id = sym_exec_llvm::get_llvm_value_id_for_value((Value const*)i_sp);

      DYN_DEBUG(value_to_name_map_dbg, std::cout << "llvm_value_id = " << llvm_value_id.llvm_value_id_to_string() << ": ce_key_name = " << ce_key_name << "\n");

      if (value_to_name_map->count(llvm_value_id) && value_to_name_map->at(llvm_value_id) != mk_string_ref(ce_key_name)) {
        errs() << "llvm_value_id = " << llvm_value_id.llvm_value_id_to_string() << ": ce_key_name = " << ce_key_name << "\n";
        errs() << "llvm_value_id = " << llvm_value_id.llvm_value_id_to_string() << ": value_to_name_map->at(llvm_value_id) = " << value_to_name_map->at(llvm_value_id)->get_str() << "\n";
      }
      ASSERT(!value_to_name_map->count(llvm_value_id) || value_to_name_map->at(llvm_value_id) == mk_string_ref(ce_key_name));
      value_to_name_map->insert(make_pair(llvm_value_id, mk_string_ref(ce_key_name)));
    }

    DYN_DEBUG2(llvm2tfg, errs() << "ce_expr:" << m_ctx->expr_to_string(ce_expr) << "\n");
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
      //cout << __func__ << " " << __LINE__ << ": Warning: symbol not recognized. name = " << name << endl;
      //cout << __func__ << " " << __LINE__ << ": sort = " << get_value_type(v, m_module->getDataLayout())->to_string() << endl;
      //for (const auto &s : m_symbol_map->get_map()) {
      //  cout << __func__ << " " << __LINE__ << ": sym = " << s.second.get_name()->get_str() << endl;
      //}
      //NOT_REACHED(); //could be a function pointer!
      ret = m_ctx->mk_var(string(LLVM_FUNCTION_NAME_PREFIX) + name, get_value_type(v, m_module->getDataLayout()));
    }
    ASSERT(ret);

    //m_symbol_map[name] = make_pair(symbol_size, symbol_is_constant);
    return make_pair(ret, state_assumes);
  }
  else if(const UndefValue* c = dyn_cast<const UndefValue>(&v))
  {
    string undef_key_name = get_next_undef_varname();
    string undef_poison_varname = get_poison_value_varname(undef_key_name);

    sort_ref s = get_value_type(v, m_module->getDataLayout());

    state state_to_intermediate_val;
    state_set_expr(state_to_intermediate_val, undef_poison_varname, expr_true(m_ctx));
    state_set_expr(state_to_intermediate_val, undef_key_name, m_ctx->mk_var(LLVM_UNDEF_VALUE_NAME, s));
    dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    pc from_pc = from_node->get_pc();
    shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(from_pc, intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx), {}, te_comment_t(false, from_pc.get_subindex(), "treat-undef-as-poison")));
    t.add_edge(e);
    from_node = intermediate_node;

    m_poison_varnames_seen.insert(undef_poison_varname);
    expr_ref undef_expr = mk_fresh_expr(undef_key_name, G_INPUT_KEYWORD, s);

    return make_pair(undef_expr, state_assumes);

    //expr_ref ret = m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + get_value_name(v), get_value_type(v, m_module->getDataLayout()));
    //return make_pair(ret, state_assumes);
  }

  DYN_DEBUG2(llvm2tfg, errs() << "const to expr not handled:"  << get_value_name(v) << "\n");
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
//
//sort_ref sym_exec_mir::get_mir_type_sort(const MachineOperand::MachineOperandType ty) const
//{
//  ASSERT(   false
//         || ty == MachineOperand::MO_Register
//         || ty == MachineOperand::MO_Immediate
//         || ty == MachineOperand::MO_CImmediate
//         || ty == MachineOperand::MO_FPImmediate
//  );
//  unsigned size = DWORD_LEN /*XXX: ty.getSizeInBits()*/;
//  return m_ctx->mk_bv_sort(size);
//}

vector<sort_ref> sym_exec_common::get_type_sort_vec(llvm::Type* t, DataLayout const &dl) const
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
    //cout << __func__ << " " << __LINE__ << ": float size in bits = " << dl.getTypeSizeInBits(t) << endl;
    size_t size = dl.getTypeSizeInBits(t);
    ASSERT(size == DWORD_LEN);
    sv.push_back(m_ctx->mk_float_sort(size));
    //sv.push_back(m_ctx->mk_bv_sort(DWORD_LEN));
    return sv;
  } else if (t->getTypeID() == Type::DoubleTyID) {
    //cout << __func__ << " " << __LINE__ << ": double size in bits = " << dl.getTypeSizeInBits(t) << endl;
    size_t size = dl.getTypeSizeInBits(t);
    ASSERT(size == QWORD_LEN);
    sv.push_back(m_ctx->mk_float_sort(size));
    //sv.push_back(m_ctx->mk_bv_sort(QWORD_LEN));
    return sv;
  } else if (t->getTypeID() == Type::X86_FP80TyID) {
    //cout << __func__ << " " << __LINE__ << ": double size in bits = " << dl.getTypeSizeInBits(t) << endl;
    size_t size = dl.getTypeSizeInBits(t);
    ASSERT(size == 80);
    sv.push_back(m_ctx->mk_floatx_sort(size));
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
  } else if (   t->getTypeID() == Type::FixedVectorTyID
             || t->getTypeID() == Type::ScalableVectorTyID
            ) {
    t->print(errs());
    errs().flush();
    errs() << "\n";
    NOT_IMPLEMENTED();
  } else {
    t->print(errs());
    errs() << "\n";
    errs().flush();
    unreachable("type unhandled");
  }
}

sort_ref sym_exec_common::get_type_sort(llvm::Type* t, DataLayout const &dl) const
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
sym_exec_llvm::llvm_instruction_get_md5sum_name(Instruction const& I) const
{
  string istr;
  raw_string_ostream rso(istr);
  rso << I;
  return m_srcdst_keyword + string("." G_LLVM_PREFIX "-%") + md5_checksum(istr);
}

string
sym_exec_llvm::constexpr_instruction_get_name(Instruction const& I) const
{
  string base_name = llvm_instruction_get_md5sum_name(I);
  return base_name + "." + CONSTEXPR_KEYWORD;
}

string
sym_exec_llvm::gep_instruction_get_intermediate_value_name(Instruction const& I/*string base_name*/, unsigned index_counter, int intermediate_value_num) const
{
  string base_name = llvm_instruction_get_md5sum_name(I);
  stringstream ss;
  ASSERT(intermediate_value_num == 0 || intermediate_value_num == 1);
  if (intermediate_value_num == 0) {
    ss << base_name << "." << LLVM_INTERMEDIATE_VALUE_KEYWORD << "." << GEPOFFSET_KEYWORD << "." << index_counter << "." << "offset";
  } else {
    ss << base_name << "." GEPOFFSET_KEYWORD << "." << index_counter << "." << GEPTOTALOFFSET_KEYWORD;
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
sym_exec_common::gep_name_prefix(string const &name, pc const &from_pc, pc const &to_pc, int argnum) const
{
  stringstream ss;
  ss << m_srcdst_keyword << "." G_LLVM_PREFIX "-%" << name << "." << argnum << "." << from_pc.to_string() << "." << to_pc.to_string();
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
  m_state_templ.push_back({m_mem_alloc_reg, get_mem_alloc_sort()});
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

void
sym_exec_llvm::populate_state_template(const llvm::Function& F, bool model_llvm_semantics)
{
  argnum_t argnum = 0;
  const DataLayout &dl = m_module->getDataLayout();
  for (Function::const_arg_iterator iter = F.arg_begin(); iter != F.arg_end(); ++iter) {
    const Value& v = *iter;
    if (isa<const Constant>(v)) {
      continue;
    }
    string name = get_value_name(v);
    sort_ref s = get_value_type(v, m_module->getDataLayout());
    string argname = name/* + SRC_INPUT_ARG_NAME_SUFFIX*/;
    expr_ref argvar = m_ctx->get_input_expr_for_key(mk_string_ref(argname), s);
    m_arguments[name] = make_pair(argnum, argvar);

    allocsite_t allocsite = allocsite_t::allocsite_arg(argnum);
    argnum++;
    //m_state_templ.push_back({argname, s});

    Type* ty = v.getType();
    unsigned size = dl.getTypeAllocSize(ty);
    unsigned align = dl.getPrefTypeAlignment(ty);
    //m_local_refs.insert(make_pair(m_local_num++, graph_local_t(argname, size, align)));
    m_local_refs.insert(make_pair(allocsite, graph_local_t(argname, size, align)));
    //m_local_num = m_local_num.increment_by_one();

    if (model_llvm_semantics) {
      string arg_poison_varname = get_poison_value_varname(name);
      expr_ref arg_poison_var = m_ctx->get_input_expr_for_key(mk_string_ref(arg_poison_varname), m_ctx->mk_bool_sort());
      m_arguments[arg_poison_varname] = make_pair(argnum, arg_poison_var);
      allocsite_t allocsite = allocsite_t::allocsite_arg(argnum);
      argnum++;
      m_poison_varnames_seen.insert(arg_poison_varname);
      m_local_refs.insert(make_pair(allocsite, graph_local_t(arg_poison_varname, 1, 0)));
    }
  }
  if (F.isVarArg()) {
    expr_ref argvar = m_ctx->get_vararg_local_expr();
    string_ref name = m_ctx->get_key_from_input_expr(argvar);
    m_arguments[name->get_str()] = make_pair(argnum, argvar);

    m_local_refs.insert(make_pair(graph_locals_map_t::vararg_local_id(), graph_local_t::vararg_local()));
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
sym_exec_llvm::get_expr_adding_edges_for_intermediate_vals(const Value& v, string const& vname, const state& state_in, unordered_set<expr_ref> const& state_assumes, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map)
{
  if (isa<const UndefValue>(&v)) {

    string undef_key_name = get_next_undef_varname();
    string undef_poison_varname = get_poison_value_varname(undef_key_name);

    sort_ref s = get_value_type(v, m_module->getDataLayout());
    if (!(s->is_bv_kind() || s->is_bool_kind())) {
      cout << __func__ << " " << __LINE__ << ": s = " << s->to_string() << endl;
    }

    state state_to_intermediate_val;
    state_set_expr(state_to_intermediate_val, undef_poison_varname, expr_true(m_ctx));
    state_set_expr(state_to_intermediate_val, undef_key_name, m_ctx->mk_var(LLVM_UNDEF_VALUE_NAME, s));
    dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    pc from_pc = from_node->get_pc();
    shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(from_pc, intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx), {}, te_comment_t(false, from_pc.get_subindex(), "treat-undef-as-poison")));
    t.add_edge(e);
    from_node = intermediate_node;

    m_poison_varnames_seen.insert(undef_poison_varname);

    ASSERT(s->is_bv_kind() || s->is_bool_kind());
    expr_ref undef_expr = mk_fresh_expr(undef_key_name, G_INPUT_KEYWORD, s);

    return make_pair(undef_expr, state_assumes);
  } else if (isa<const Constant>(&v)) {
    return get_const_value_expr(v, vname, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
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

//pair<expr_ref,unordered_set<expr_ref>>
//sym_exec_llvm::get_expr_adding_edges_for_intermediate_vals(const Value& v/*, string vname*/, const state& state_in, unordered_set<expr_ref> const& assumes, shared_ptr<tfg_node> &from_node, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map)
//{
//  return __get_expr_adding_edges_for_intermediate_vals_helper(v/*, vname*/, state_in, assumes, &from_node, pc_to, &B, &F, t, value_to_name_map);
//}

void
sym_exec_common::state_set_expr(state &st, string const &key, expr_ref const &value) const
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
    return st.get_expr(key);
  } else {
    return get_input_expr(key, s);
  }
}

void sym_exec_llvm::set_expr(string const &name/*, const llvm::Value& v*/, expr_ref expr, state& st)
{
  //state_set_expr(st, get_value_name(v), expr);
  state_set_expr(st, name, expr);
}

//void sym_exec_mir::mir_set_expr(const llvm::MachineOperand& v, expr_ref expr, state& st)
//{
//  NOT_IMPLEMENTED();
//  //string name;
//  //raw_string_ostream ss(name);
//  //v.print(ss);
//  //ss.flush();
//  //MachineOperand::MachineOperandType type = v.getType();
//  //sort_ref s = get_mir_type_sort(type);
//
//  //m_state_templ.push_back({name, s});
//  //state_set_expr(st, name, expr);
//}



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

expr_ref
sym_exec_llvm::fcmp_to_expr(FCmpInst::Predicate cmp_kind, const vector<expr_ref>& args) const
{
  switch (cmp_kind) {
    case CmpInst::FCMP_FALSE:
      return m_ctx->mk_bool_false();
    case CmpInst::FCMP_OEQ:
      return m_ctx->mk_fcmp_oeq(args.at(0), args.at(1));
    case CmpInst::FCMP_OGT:
      return m_ctx->mk_fcmp_ogt(args.at(0), args.at(1));
    case CmpInst::FCMP_OGE:
      return m_ctx->mk_fcmp_oge(args.at(0), args.at(1));
    case CmpInst::FCMP_OLT:
      return m_ctx->mk_fcmp_olt(args.at(0), args.at(1));
    case CmpInst::FCMP_OLE:
      return m_ctx->mk_fcmp_ole(args.at(0), args.at(1));
    case CmpInst::FCMP_ONE:
      return m_ctx->mk_fcmp_one(args.at(0), args.at(1));
    case CmpInst::FCMP_ORD:
      return m_ctx->mk_fcmp_ord(args.at(0), args.at(1));
    case CmpInst::FCMP_UNO:
      return m_ctx->mk_fcmp_uno(args.at(0), args.at(1));
    case CmpInst::FCMP_UEQ:
      return m_ctx->mk_fcmp_ueq(args.at(0), args.at(1));
    case CmpInst::FCMP_UGT:
      return m_ctx->mk_fcmp_ugt(args.at(0), args.at(1));
    case CmpInst::FCMP_UGE:
      return m_ctx->mk_fcmp_uge(args.at(0), args.at(1));
    case CmpInst::FCMP_ULT:
      return m_ctx->mk_fcmp_ult(args.at(0), args.at(1));
    case CmpInst::FCMP_ULE:
      return m_ctx->mk_fcmp_ule(args.at(0), args.at(1));
    case CmpInst::FCMP_UNE:
      return m_ctx->mk_fcmp_une(args.at(0), args.at(1));
    case CmpInst::FCMP_TRUE:
      return m_ctx->mk_bool_true();
    default:
      NOT_REACHED();
  }
  NOT_REACHED();
}

expr_ref sym_exec_llvm::icmp_to_expr(ICmpInst::Predicate cmp_kind, const vector<expr_ref>& args) const
{
  expr_ref ret = m_ctx->mk_app(icmp_kind_to_expr_kind(cmp_kind), args);

  if(cmp_kind == ICmpInst::ICMP_NE)
    ret = m_ctx->mk_not(ret);

  return ret;
}

pair<vector<expr_ref>, unordered_set<expr_ref>>
sym_exec_llvm::get_expr_args(const llvm::Instruction& I, string const& vname, const state& st, unordered_set<expr_ref> const& state_assumes, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map)
{
  vector<expr_ref> args;
  unordered_set<expr_ref> assumes = state_assumes;
  pc orig_from_pc = from_node->get_pc();
  for (unsigned i = 0; i < I.getNumOperands(); ++i) {
    //string avname = vname;
    //if (avname == "") {
    //  avname = gep_name_prefix("const_operand", orig_from_pc, pc_to, i);
    //}
    expr_ref arg;
    tie(arg, assumes) = get_expr_adding_edges_for_intermediate_vals(*I.getOperand(i), vname, st, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    args.push_back(arg);
  }
  return make_pair(args, assumes);
}

pair<expr_ref,unordered_set<expr_ref>>
sym_exec_llvm::exec_gen_expr_casts(const llvm::CastInst& I, expr_ref arg, unordered_set<expr_ref> const& state_assumes, pc const &from_pc, bool model_llvm_semantics, tfg &t)
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
    if (src_expr->is_bv_sort() && src_expr->get_sort()->get_size() == 1) {
      src_expr = expr_bv1_to_bool(src_expr);
    }
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
sym_exec_llvm::apply_memcpy_function(const CallInst* c, expr_ref fun_name_expr, string const &fun_name, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, Function *F, state const &state_in, state &state_out, unordered_set<expr_ref> const& state_assumes, string const &cur_function_name, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, llvm::Function const &curF, tfg &t/*, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> *function_tfg_map*/, map<llvm_value_id_t, string_ref>* value_to_name_map/*, set<string> const *function_call_chain*/, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format)
{
  const auto& memcpy_dst = c->getArgOperand(0);
  const auto& memcpy_src = c->getArgOperand(1);
  const auto& memcpy_nbytes = c->getArgOperand(2);
  const auto& memcpy_align = c->getArgOperand(3);
  pc const &from_pc = from_node->get_pc();
  unordered_set<expr_ref> assumes = state_assumes;
  expr_ref memcpy_src_expr, memcpy_dst_expr, memcpy_nbytes_expr, memcpy_align_expr;

  tie(memcpy_src_expr, assumes)    = get_expr_adding_edges_for_intermediate_vals(*memcpy_src, "", state_out, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
  tie(memcpy_dst_expr, assumes)    = get_expr_adding_edges_for_intermediate_vals(*memcpy_dst, "", state_out, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
  tie(memcpy_nbytes_expr, assumes) = get_expr_adding_edges_for_intermediate_vals(*memcpy_nbytes, "", state_out, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
  tie(memcpy_align_expr, assumes)  = get_expr_adding_edges_for_intermediate_vals(*memcpy_align, "", state_out, assumes, from_node, model_llvm_semantics, t, value_to_name_map);

  int memcpy_align_int;
  if (memcpy_align_expr->is_bool_sort() || memcpy_align_expr->get_sort()->get_size() == 1) { //seems like the align operand was omitted; unsure if this is the correct check though.
    memcpy_align_int = 1;
  } else {
    ASSERT(memcpy_align_expr->is_const());
    int memcpy_align_int = memcpy_align_expr->get_int64_value();
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
    uint64_t memcpy_dst_align = F->getParamAlign(0).valueOrOne().value();
    uint64_t memcpy_src_align = F->getParamAlign(1).valueOrOne().value();
    uint64_t memcpy_align_int = max(memcpy_src_align, memcpy_dst_align); // satisfies alignment requirement of both src and dst
    DYN_DEBUG(llvm2tfg_memcpy, cout << "memcpy_src_align = " << memcpy_src_align << "; memcpy_dst_align = " << memcpy_dst_align << endl);
    int count = memcpy_nbytes_expr->get_int64_value();
    if (count != 0) {
      expr_ref const& src_isaligned_assume = m_ctx->mk_islangaligned(memcpy_src_expr, memcpy_src_align);
      add_state_assume("", src_isaligned_assume, state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
      expr_ref const& dst_isaligned_assume = m_ctx->mk_islangaligned(memcpy_dst_expr, memcpy_dst_align);
      //assumes.insert(dst_isaligned_assume);
      add_state_assume("", dst_isaligned_assume, state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    }
    pc cur_pc = from_node->get_pc();
    dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(cur_pc, intermediate_node->get_pc(), state_out, expr_true(m_ctx), assumes, this->instruction_to_te_comment(*c, from_pc)));
    t.add_edge(e);
    DYN_DEBUG(llvm2tfg_memcpy, cout << "added edge " << e->to_string_concise() << " for alignment assume" << endl);
    cur_pc = intermediate_node->get_pc();

    for (int i = 0; i < count; i += memcpy_align_int) {
      state_out = state_in;

      expr_ref offset = m_ctx->mk_bv_const(get_word_length(), i);
      expr_ref mem = state_get_expr(state_out, m_mem_reg, this->get_mem_sort());
      expr_ref mem_alloc = state_get_expr(state_out, m_mem_alloc_reg, this->get_mem_alloc_sort());
      expr_ref inbytes = m_ctx->mk_select(mem, mem_alloc, ml_top, m_ctx->mk_bvadd(memcpy_src_expr, offset), memcpy_align_int, false/*, comment_t()*/);

      expr_ref out_mem = state_get_expr(state_out, m_mem_reg, this->get_mem_sort());
      expr_ref out_mem_alloc = state_get_expr(state_out, m_mem_alloc_reg, this->get_mem_alloc_sort());
      out_mem = m_ctx->mk_store(out_mem, out_mem_alloc, ml_top, m_ctx->mk_bvadd(memcpy_dst_expr, offset), inbytes, memcpy_align_int, false/*, comment_t()*/);
      state_set_expr(state_out, m_mem_reg, out_mem);

      dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);

      shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(cur_pc, intermediate_node->get_pc(), state_out, expr_true(m_ctx), {}, this->instruction_to_te_comment(*c, from_pc)));
      t.add_edge(e);
      DYN_DEBUG(llvm2tfg_memcpy, cout << "added edge " << e->to_string_concise() << " for memcpy of bytes " << i << "-" << (i+memcpy_align_int-1) << endl);
      cur_pc = intermediate_node->get_pc();
    }
    state_out = state_in;
    from_node = t.find_node(cur_pc);
    ASSERT(from_node);
  } else {
    //cout << __func__ << " " << __LINE__ << ": memcpy_nbytes_expr = " << expr_string(memcpy_nbytes_expr) << endl;
    tie(assumes, succ_assumes) = apply_general_function(c, fun_name_expr, fun_name, src_llvm_tfg, F, state_in, state_out, assumes, cur_function_name, from_node, model_llvm_semantics, t/*, function_tfg_map*/, value_to_name_map/*, function_call_chain*/, scev_map, xml_output_format);
  }
  return make_pair(state_assumes, succ_assumes);
}

pair<unordered_set<expr_ref>,unordered_set<expr_ref>>
sym_exec_llvm::apply_va_start_function(const CallInst* c, state const& state_in, state &state_out, unordered_set<expr_ref> const& state_assumes, string const& cur_function_name, dshared_ptr<tfg_node>& from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t,string_ref>* value_to_name_map)
{
  const auto& va_list_ptr = c->getArgOperand(0);
  expr_ref va_list_ptr_expr;
  string_ref const& fname = t.get_name();

  ASSERT(va_list_ptr);
  unordered_set<expr_ref> assumes = state_assumes;
  // store vararg addr at the location pointed to by va_list_ptr
  tie(va_list_ptr_expr, assumes) = get_expr_adding_edges_for_intermediate_vals(*va_list_ptr, "", state_out, assumes, from_node, model_llvm_semantics, t, value_to_name_map);

  //expr_ref vararg_addr = m_ctx->get_consts_struct().get_expr_value(reg_type_local, graph_locals_map_t::vararg_local_id());
  allocstack_t allocstack = allocstack_t::allocstack_singleton(cur_function_name, graph_locals_map_t::vararg_local_id());
  expr_ref vararg_addr = m_ctx->get_consts_struct().get_local_addr(reg_type_local, allocstack, m_srcdst_keyword);
  expr_ref mem_alloc = state_get_expr(state_in, this->m_mem_alloc_reg, this->get_mem_alloc_sort());
  memlabel_t ml_top = memlabel_t::memlabel_top();
  unsigned count = get_word_length()/get_memory_addressable_size();
  state_set_expr(state_out, m_mem_reg, m_ctx->mk_store(state_get_expr(state_in, m_mem_reg, this->get_mem_sort()), mem_alloc, ml_top, va_list_ptr_expr, vararg_addr, count, false));
  return make_pair(assumes, unordered_set<expr_ref>());
}

pair<unordered_set<expr_ref>,unordered_set<expr_ref>>
sym_exec_llvm::apply_va_copy_function(const CallInst* c, state const& state_in, state &state_out, unordered_set<expr_ref> const& state_assumes, string const& cur_function_name, dshared_ptr<tfg_node>& from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t,string_ref>* value_to_name_map)
{
  const auto& dst_va_list_ptr = c->getArgOperand(0);
  const auto& src_va_list_ptr = c->getArgOperand(1);
  expr_ref src_va_list_ptr_expr, dst_va_list_ptr_expr;
  ASSERT(dst_va_list_ptr);
  ASSERT(src_va_list_ptr);
  // copy word-legnth bytes from [src_va_list_ptr] to [dst_va_list_ptr]
  unordered_set<expr_ref> assumes = state_assumes;
  tie(src_va_list_ptr_expr, assumes) = get_expr_adding_edges_for_intermediate_vals(*src_va_list_ptr, "", state_out, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
  tie(dst_va_list_ptr_expr, assumes) = get_expr_adding_edges_for_intermediate_vals(*dst_va_list_ptr, "", state_out, assumes, from_node, model_llvm_semantics, t, value_to_name_map);

  memlabel_t ml_top = memlabel_t::memlabel_top();
  unsigned count = get_word_length()/get_memory_addressable_size();
  expr_ref mem_expr = state_get_expr(state_in, m_mem_reg, this->get_mem_sort());
  expr_ref mem_alloc = state_get_expr(state_in, m_mem_alloc_reg, this->get_mem_alloc_sort());
  expr_ref va_list_expr = m_ctx->mk_select(mem_expr, mem_alloc, ml_top, src_va_list_ptr_expr, count, false);
  state_set_expr(state_out, m_mem_reg, m_ctx->mk_store(mem_expr, mem_alloc, ml_top, dst_va_list_ptr_expr, va_list_expr, count, false));

  return make_pair(assumes, unordered_set<expr_ref>());
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

//pair<callee_summary_t, dshared_ptr<tfg_llvm_t>> const&
//sym_exec_llvm::get_callee_summary(Function *F, string const &fun_name, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> &function_tfg_map, map<llvm_value_id_t, string_ref>* value_to_name_map, set<string> const &function_call_chain, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format)
//{
//  //cout.flush();
//  //cout << __func__ << " " << __LINE__ << endl;
//  //cout.flush();
//  if (function_tfg_map.count(fun_name)) {
//    //cout << __func__ << " " << __LINE__ << endl;
//    return function_tfg_map.at(fun_name);
//  }
//  //cout << __func__ << " " << __LINE__ << endl;
//  context *ctx = g_ctx; //new context(context::config(600, 600));
//  //ctx->parse_consts_db(CONSTS_DB_FILENAME);
//  //ctx->parse_consts_db("~/superopt/consts_db.in"); //XXX: fixme
//
//  cout << __func__ << " " << __LINE__ << ": getting callee summary for " << fun_name << ": function_call_chain.size() = " << function_call_chain.size() << ", chain:";
//  for (const auto &f : function_call_chain) {
//    cout << " " << f;
//  }
//  cout << endl;
//  dshared_ptr<tfg_llvm_t> t = sym_exec_llvm::get_preprocessed_tfg(*F, m_module, fun_name, ctx, src_llvm_tfg, function_tfg_map, value_to_name_map, function_call_chain, this->gen_callee_summary(), false, scev_map, this->get_srcdst_keyword(), xml_output_format);
// cout << __func__ << " " << __LINE__ << ": done callee summary for " << fun_name << ": function_call_chain.size() = " << function_call_chain.size() << ", chain:";
//  for (const auto &f : function_call_chain) {
//    cout << " " << f;
//  }
//  cout << endl;
//  //cout << __func__ << " " << __LINE__ << ": TFG =\n" << t->graph_to_string() << endl;
//  auto ret = make_pair(t->get_summary_for_calling_functions(), std::move(t));
//  function_tfg_map.insert(make_pair(fun_name, std::move(ret)));
//  //delete ctx; //we are returning tfg which contains references to ctx, so do not delete ctx
//  return function_tfg_map.at(fun_name);
//}

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

//string
//sym_exec_llvm::get_local_alloc_count_varname() const
//{
//  return this->get_srcdst_keyword() + "." + G_LOCAL_ALLOC_COUNT_VARNAME/*m_local_alloc_count_varname*/;
//}

//string
//sym_exec_llvm::get_local_alloc_count_ssa_varname(pc const& p) const
//{
//  return this->get_srcdst_keyword() + "." + G_LOCAL_ALLOC_COUNT_SSA_VARNAME + "." + p.to_string();
//}



pair<unordered_set<expr_ref>,unordered_set<expr_ref>>
sym_exec_llvm::apply_general_function(const CallInst* c, expr_ref fun_name_expr, string const &fun_name, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, Function *F, state const &state_in, state &state_out, unordered_set<expr_ref> const& state_assumes, string const &cur_function_name, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg &t/*, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> *function_tfg_map*/, map<llvm_value_id_t, string_ref>* value_to_name_map/*, set<string> const *function_call_chain*/, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format)
{
  vector<expr_ref> args;
  vector<sort_ref> args_type;
  pc const &from_pc = from_node->get_pc();

  //callee_summary_t csum;
  //string fname = fun_name == "" ? "" : fun_name.substr(string(LLVM_FUNCTION_NAME_PREFIX).size());
  //if (m_callee_summaries.count(fun_name)) {
  //  csum = m_callee_summaries.at(fun_name);
  //}
#if 0
  else if (   F != NULL
             && function_call_chain
             && function_belongs_to_program(fun_name)
             && !set_belongs(*function_call_chain, fname)/*
             && this->gen_callee_summary()*/) {
    ASSERT(fun_name.substr(0, string(LLVM_FUNCTION_NAME_PREFIX).size()) == LLVM_FUNCTION_NAME_PREFIX);
    pair<callee_summary_t, dshared_ptr<tfg_llvm_t>> const& csum_callee_tfg = get_callee_summary(F, fname, src_llvm_tfg, *function_tfg_map, value_to_name_map, *function_call_chain, scev_map, xml_output_format);
    DYN_DEBUG(llvm2tfg, cout << "Doing resume: " << t.get_name() << endl);
    csum = csum_callee_tfg.first;
    tfg const& callee_tfg = *csum_callee_tfg.second;
    for (auto const& s : callee_tfg.get_symbol_map().get_map()) {
      if (!s.second.graph_symbol_is_const()) { //do not include callee's const symbols because they are inconsequential (but are complicated to handle)
        m_touched_symbols.insert(s.first);
      }
    }
    m_callee_summaries[fname] = csum;
  }
#endif
  //else {
  //  DYN_DEBUG(llvm2tfg, cout << "Could not get callee summary for " << fun_name << ": F = " << F << endl);
  //}

  expr_ref ml_read_expr = m_ctx->memlabel_var(t.get_name()->get_str(), m_memlabel_varnum); //csum.get_read_memlabel();
  m_memlabel_varnum++;
  expr_ref ml_write_expr= m_ctx->memlabel_var(t.get_name()->get_str(), m_memlabel_varnum); //csum.get_write_memlabel();
  m_memlabel_varnum++;

  expr_ref mem = state_get_expr(state_in, m_mem_reg, this->get_mem_sort());
  expr_ref mem_alloc = state_get_expr(state_in, m_mem_alloc_reg, this->get_mem_alloc_sort());
  //expr_ref mem_alloc = get_corresponding_mem_alloc_from_mem_expr(mem);
  //expr_ref zerobv = m_ctx->mk_zerobv(DWORD_LEN);
  args.push_back(ml_read_expr);
  args_type.push_back(ml_read_expr->get_sort());
  args.push_back(ml_write_expr);
  args_type.push_back(ml_write_expr->get_sort());
  args.push_back(mem);
  args_type.push_back(mem->get_sort());
  args.push_back(mem_alloc);
  args_type.push_back(mem_alloc->get_sort());

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
    tie(expr, assumes) = get_expr_adding_edges_for_intermediate_vals(*arg, "", state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    args.push_back(expr);
    args_type.push_back(expr->get_sort());
    argnum++;

    if (model_llvm_semantics) {
      if (m_ctx->expr_is_input_expr(expr)) {
        string const& val_key = m_ctx->get_key_from_input_expr(expr)->get_str();
        string val_key_poison = get_poison_value_varname(val_key);

        if (set_belongs(m_poison_varnames_seen, val_key_poison)) {
          args.push_back(m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + val_key_poison, m_ctx->mk_bool_sort()));
        } else {
          args.push_back(m_ctx->mk_bool_const(false));
        }
      } else {
        args.push_back(m_ctx->mk_bool_const(false));
      }
      args_type.push_back(m_ctx->mk_bool_sort());
      argnum++;
    }
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
      //size_t bvsize;
      //if (ret_sort->is_bv_kind() || ret_sort->is_float_kind() || ret_sort->is_floatx_kind()) {
      //  bvsize = ret_sort->get_size();
      //} else if (ret_sort->is_bool_kind()) {
      //  bvsize = 1;
      //} else NOT_REACHED();

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
      DYN_DEBUG2(llvm2tfg, errs() << "\n\nfun sort: " << m_ctx->expr_to_string_table(fun) << "\n");
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
  return make_pair(assumes, succ_assumes);
}

expr::operation_kind
sym_exec_llvm::farith_to_operation_kind(unsigned opcode, expr_vector const& args)
{
  if (opcode == Instruction::FDiv) {
    return expr::OP_FDIV;
  } else if (opcode == Instruction::FMul) {
    return expr::OP_FMUL;
  } else if (opcode == Instruction::FAdd) {
    return expr::OP_FADD;
  } else if (opcode == Instruction::FSub) {
    return expr::OP_FSUB;
  } else if (opcode == Instruction::FRem) {
    return expr::OP_FREM;
  } else if (opcode == Instruction::FNeg) {
    return expr::OP_FNEG;
  }
  NOT_REACHED();
}

static expr_ref
gen_no_mul_overflow_assume_expr(expr_ref const& a, expr_ref const& b, bool a_is_positive)
{
  ASSERT(a->get_sort()->get_size() == b->get_sort()->get_size());
  unsigned bvlen = a->get_sort()->get_size();
  context* ctx = a->get_context();

  expr_ref se_a        = ctx->mk_bvsign_ext(a, bvlen);
  expr_ref ze_b        = ctx->mk_bvzero_ext(b, bvlen);
  expr_ref mul_expr    = expr_bvmul(se_a, ze_b);
  expr_ref msb_expr    = expr_bvextract(2*bvlen-1, bvlen, mul_expr);
  expr_ref msb_is_zero = ctx->mk_eq(msb_expr, ctx->mk_zerobv(bvlen));
  if (a_is_positive) {
    return msb_is_zero;
  } else {
    expr_ref is_neg = ctx->mk_bvslt(a, ctx->mk_zerobv(bvlen));
    expr_ref msb_is_minusone = ctx->mk_eq(msb_expr, ctx->mk_minusonebv(bvlen));
    return ctx->mk_ite(is_neg, msb_is_minusone, msb_is_zero);
  }
}

void sym_exec_llvm::exec(const state& state_in, const llvm::Instruction& I, dshared_ptr<tfg_node> from_node, llvm::BasicBlock const &B, llvm::Function const &F, size_t next_insn_id, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map/*, set<string> const *function_call_chain*/, map<shared_ptr<tfg_edge const>, Instruction *>& eimap, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format)
{
  DYN_DEBUG(llvm2tfg, errs() << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": sym exec doing: " << I << "\n");
  unordered_set<expr_ref> state_assumes;
  vector<control_flow_transfer> cfts;
  state state_out = state_in;
  string const &cur_function_name = F.getName().str();
  string const& bbindex = get_basicblock_index(B);
  //bbl_order_descriptor_t const& bbo = this->m_bbl_order_map.at(mk_string_ref(bbindex));
  pc pc_to = get_pc_from_bbindex_and_insn_id(bbindex, next_insn_id);
  te_comment_t te_comment = this->instruction_to_te_comment(I, from_node->get_pc()/*, bbo*/);
  const DataLayout &dl = m_module->getDataLayout();

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
      tie(e, cond_assumes) = get_expr_adding_edges_for_intermediate_vals(*i->getCondition(), "", state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
      ASSERT(e->is_bool_sort());
      control_flow_transfer cft1(from_node->get_pc(), get_pc_from_bbindex_and_insn_id(get_basicblock_index(*i->getSuccessor(0)), 0), e, cond_assumes);
      cfts.push_back(cft1);
      control_flow_transfer cft2(from_node->get_pc(), get_pc_from_bbindex_and_insn_id(get_basicblock_index(*i->getSuccessor(1)), 0), m_ctx->mk_not(e), cond_assumes);
      cfts.push_back(cft2);

      transfer_poison_values("", e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
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
    tie(CondVal, cond_assumes) = get_expr_adding_edges_for_intermediate_vals(*Cond, "", state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
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
      tie(CaseVal, CaseAssumes) = get_expr_adding_edges_for_intermediate_vals(*Case.getCaseValue(), "", state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
      ASSERT(CondVal->get_sort() == CaseVal->get_sort());
      expr_ref cond = m_ctx->mk_eq(CondVal, CaseVal);
      control_flow_transfer cft(from_node->get_pc(), get_pc_from_bbindex_and_insn_id(get_basicblock_index(*Case.getCaseSuccessor()), 0), cond, CaseAssumes);
      cfts.push_back(cft);
      matched_any_cond.push_back(cond);
      //cout << __func__ << " " << __LINE__ << ": cft cond = " << expr_string(cond) << ", src = " << cft.get_from_pc() << ", dest = " << cft.get_to_pc() << endl;
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
    DYN_DEBUG2(llvm2tfg, cout << _FNLN_ << ": before expand_switch, CFTs:\n"; for (auto const& cft : cfts) cout << '\t' << cft << '\n';);
    cfts = expand_switch(t, value_to_name_map, from_node, cfts, state_out, cond_assumes, te_comment, (Instruction *)&I, B, F, eimap);
    //cout << __func__ << " " << __LINE__ << ": cft cond = " << expr_string(cft.get_condition()) << ", src = " << cft.get_from_pc() << ", dest = " << cft.get_to_pc() << endl;
    DYN_DEBUG2(llvm2tfg, cout << _FNLN_ << ": after expand_switch,  CFTs:\n"; for (auto const& cft : cfts) cout << '\t' << cft << '\n';);

    transfer_poison_values("", CondVal, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    break;
  }
  case Instruction::Ret:
  {
    const ReturnInst* i = cast<const ReturnInst>(&I);
    if(Value* ret = i->getReturnValue())
    {
      Type *ElTy = ret->getType();

      expr_ref dst_val;
      tie(dst_val, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*ret, "", state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);


      //string typeString = getTypeString(ElTy);
      //langtype_ref lt = mk_langtype_ref(typeString);
      //predicate p(precond_t(m_ctx), m_ctx->mk_islangtype(dst_val, lt), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_RET_ISLANGTYPE, predicate::assume);
      //assumes.insert(p);
      //t.add_assume_pred(from_node->get_pc(), p);

      state_set_expr(state_out, G_LLVM_RETURN_REGISTER_NAME/*m_ret_reg*/, dst_val);
    }
    for (size_t i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
      stringstream ss;
      ss << G_INPUT_KEYWORD "." << m_srcdst_keyword << "." LLVM_CALLEE_SAVE_REGNAME << "." << i;
      expr_ref csreg = m_ctx->mk_var(ss.str(), m_ctx->mk_bv_sort(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS)));
      state_set_expr(state_out, m_srcdst_keyword + ("." G_LLVM_HIDDEN_REGISTER_NAME), m_ctx->mk_bvxor(state_get_expr(state_out, m_srcdst_keyword + "." G_LLVM_HIDDEN_REGISTER_NAME, csreg->get_sort()), csreg));
    }
    control_flow_transfer cft(from_node->get_pc(), pc(pc::exit), m_ctx->mk_bool_true(), m_cs.get_retaddr_const(), {});
    cfts.push_back(cft);
    break;
  }
  case Instruction::Alloca:
  {
    const AllocaInst* a =  cast<const AllocaInst>(&I);
    string iname = get_value_name(*a);
    Type *ElTy = a->getAllocatedType();
    Value const* ArraySize = a->getArraySize();
    unsigned align = a->getAlignment();
    bool is_varsize = !(a->getAllocationSizeInBits(dl).hasValue());

    uint64_t local_type_alloc_size = dl.getTypeAllocSize(ElTy);
    uint64_t local_size = local_type_alloc_size;
    if (const ConstantInt* constArraySize = dyn_cast<const ConstantInt>(ArraySize)) {
      local_size *= constArraySize->getZExtValue();
    }

    allocsite_t local_id(from_node->get_pc());
    allocstack_t local_id_stack = allocstack_t::allocstack_singleton(cur_function_name, local_id);
    memlabel_t ml_local = memlabel_t::memlabel_local(local_id_stack);
    expr_ref local_addr_var = m_cs.get_local_addr(reg_type_local, local_id_stack, m_srcdst_keyword);

    string local_addr_key = m_ctx->get_key_from_input_expr(local_addr_var)->get_str();
    m_local_refs.insert(make_pair(local_id, graph_local_t(iname, local_size, align, is_varsize)));

    expr_ref local_size_val;
    if (is_varsize) {
      expr_ref varsize_expr;
      tie(varsize_expr, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*ArraySize, iname, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
      unsigned bvlen = varsize_expr->get_sort()->get_size();
      ASSERT(bvlen == get_word_length());
      expr_ref local_type_alloc_size_expr = m_ctx->mk_bv_const(bvlen, local_type_alloc_size);
      local_size_val = m_ctx->mk_bvmul(varsize_expr, local_type_alloc_size_expr);

      bool is_c_alloca = false; // TODO
      if (is_c_alloca) {
        // add size != 0 assume
        expr_ref size_is_nonzero = m_ctx->mk_not(m_ctx->mk_eq(local_size_val, m_ctx->mk_zerobv(bvlen)));
        //state_assumes.insert(size_is_nonzero);
        add_state_assume(iname, size_is_nonzero, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map); //state_assumes.insert(size_is_positive_assume);
      } else {
        // add size > 0 assume
        expr_ref size_is_positive_assume = m_ctx->mk_bvsgt(local_size_val, m_ctx->mk_zerobv(bvlen));
        //state_assumes.insert(size_is_positive_assume);
        add_state_assume(iname, size_is_positive_assume, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map); //state_assumes.insert(size_is_positive_assume);
      }
      // add no overflow assume for (varsize_expr * local_type_alloc_size)
      expr_ref no_overflow = gen_no_mul_overflow_assume_expr(varsize_expr, local_type_alloc_size_expr, /*varsize_expr is positive*/true);
      //state_assumes.insert(no_overflow);
      add_state_assume(iname, no_overflow, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map); //state_assumes.insert(no_overflow);
    } else {
      local_size_val = m_ctx->mk_bv_const(get_word_length(), local_size);
    }
    // == intermediate edge 0 ==
    string local_alloc_count_varname = m_ctx->get_local_alloc_count_varname(this->get_srcdst_keyword())->get_str();
    expr_ref local_alloc_count_var = state_get_expr(state_in, local_alloc_count_varname, m_ctx->mk_count_sort());
    expr_ref local_size_var = m_ctx->get_local_size_expr_for_id(local_id/*, local_alloc_count_var*/, m_ctx->mk_bv_sort(get_word_length()), m_srcdst_keyword);
    // local_size.id <- size expr
    state_set_expr(state_out, m_ctx->get_key_from_input_expr(local_size_var)->get_str(), local_size_val);
    dshared_ptr<tfg_node> intermediate_node0 = get_next_intermediate_subsubindex_pc_node(t, from_node);
    ASSERT(intermediate_node0);
    tfg_edge_ref e0 = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node0->get_pc(), state_out, expr_true(m_ctx), state_assumes, te_comment));
    t.add_edge(e0);

    // == intermediate edge 1 ==
    from_node = intermediate_node0;
    state_out = state_in;
    state_assumes.clear();

    expr_ref mem_e = state_get_expr(state_in, m_mem_reg, this->get_mem_sort());
    expr_ref mem_alloc_e = state_get_expr(state_in, m_mem_alloc_reg, this->get_mem_alloc_sort());
    // local.<id>            <- alloca_ptr
    // local.alloc.count.ssa <- local.alloc.count
    state_set_expr(state_out, local_addr_key, m_ctx->get_local_ptr_expr_for_id(local_id, local_alloc_count_var, mem_alloc_e, ml_local, local_size_var));
    state_set_expr(state_out, m_ctx->get_local_alloc_count_ssa_varname(this->get_srcdst_keyword(), local_id)->get_str(), local_alloc_count_var);
    dshared_ptr<tfg_node> intermediate_node1 = get_next_intermediate_subsubindex_pc_node(t, from_node);
    ASSERT(intermediate_node1);
    tfg_edge_ref e1 = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node1->get_pc(), state_out, expr_true(m_ctx), state_assumes, te_comment));
    t.add_edge(e1);

    // == intermediate edge 2 ==
    from_node = intermediate_node1;
    state_out = state_in;
    state_assumes.clear();

    //The following assumes are now removed; they will be a part of the WFconds
    //// alloca returned addr can never be 0
    //expr_ref ret_addr_non_zero = m_ctx->mk_not(m_ctx->mk_eq(local_addr_var, m_ctx->mk_zerobv(get_word_length())));
    //state_assumes.insert(ret_addr_non_zero);
    //// alloca returned addr does not cause overflow: alloca_ptr <= (alloca_ptr + size - 1)
    //expr_ref ret_addr_no_overflow = m_ctx->mk_bvule(local_addr_var, m_ctx->mk_bvadd(local_addr_var, m_ctx->mk_bvsub(local_size_var, m_ctx->mk_onebv(get_word_length()))));
    //state_assumes.insert(ret_addr_no_overflow);
    //if (align != 0) {
    //  // alloca returned addr is aligned
    //  expr_ref isaligned = m_ctx->mk_islangaligned(local_addr_var, align);
    //  state_assumes.insert(isaligned);
    //}
    // <llvm-var> <- local.<id>
    state_set_expr(state_out, iname, local_addr_var);
    dshared_ptr<tfg_node> intermediate_node2 = get_next_intermediate_subsubindex_pc_node(t, from_node);
    ASSERT(intermediate_node2);
    tfg_edge_ref e2 = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node2->get_pc(), state_out, expr_true(m_ctx), state_assumes, te_comment));
    t.add_edge(e2);

    // == intermediate edge 3 ==
    from_node = intermediate_node2;
    state_out = state_in;
    state_assumes.clear();

    // mem.alloc <- alloca
    // mem       <- store_unint
    expr_ref new_mem_alloc = m_ctx->mk_alloca(mem_alloc_e, ml_local, local_addr_var, local_size_var);
    state_set_expr(state_out, m_mem_alloc_reg, new_mem_alloc);
    state_set_expr(state_out, m_mem_reg, m_ctx->mk_store_uninit(mem_e, new_mem_alloc, ml_local, local_addr_var, local_size_var, local_alloc_count_var));
    dshared_ptr<tfg_node> intermediate_node3 = get_next_intermediate_subsubindex_pc_node(t, from_node);
    ASSERT(intermediate_node3);
    tfg_edge_ref e3 = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node3->get_pc(), state_out, expr_true(m_ctx), state_assumes, te_comment));
    t.add_edge(e3);

    // == intermediate edge 4 ==
    from_node = intermediate_node3;
    state_out = state_in;
    state_assumes.clear();

    // local.alloc.count <- local.alloc.count+1
    //state_set_expr(state_out, m_ctx->get_local_alloc_count_varname(this->get_srcdst_keyword())->get_str(), m_ctx->mk_increment_count(local_alloc_count_var));
    state_set_expr(state_out, local_alloc_count_varname, m_ctx->mk_increment_count(local_alloc_count_var));
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

    //if (   Val->getType()->getTypeID() == Type::FloatTyID
    //    || Val->getType()->getTypeID() == Type::DoubleTyID) {
    //  state_set_expr(state_out, G_SRC_KEYWORD "." LLVM_CONTAINS_FLOAT_OP_SYMBOL, m_ctx->mk_bool_const(true));
    //  break;
    //}

    expr_ref addr, val;
    tie(addr, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*Addr, "", state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    tie(val, state_assumes)  = get_expr_adding_edges_for_intermediate_vals(*Val, "", state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    if (val->is_bool_sort()) {
      val = m_ctx->mk_bool_to_bv(val);
    }
    if (val->is_float_sort()) {
      val = m_ctx->mk_float_to_ieee_bv(val);
    } else if (val->is_floatx_sort()) {
      val = m_ctx->mk_floatx_to_ieee_bv(val);
    }
    ASSERTCHECK(val->is_bv_sort(), cout << __func__ << " " << __LINE__ << ": val = " << expr_string(val) << endl);
    unsigned mem_addressable_sz = get_memory_addressable_size();
    unsigned count = DIV_ROUND_UP(val->get_sort()->get_size(), mem_addressable_sz);
    unsigned addressable_write_size = ROUND_UP(val->get_sort()->get_size(), mem_addressable_sz);
    if (addressable_write_size != val->get_sort()->get_size()) {
      ASSERT(addressable_write_size > val->get_sort()->get_size());
      val = m_ctx->mk_bvzero_ext(val, addressable_write_size - val->get_sort()->get_size());
    }
    expr_ref mem = state_get_expr(state_in, m_mem_reg, this->get_mem_sort());
    expr_ref mem_alloc = state_get_expr(state_in, m_mem_alloc_reg, this->get_mem_alloc_sort());
    expr_ref new_mem = m_ctx->mk_store(mem, mem_alloc, ml_top, addr, val, count, false/*, comment_t()*/);
    state_set_expr(state_out, m_mem_reg, new_mem);

    transfer_poison_value_on_store(new_mem, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

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
      //state_assumes.insert(isaligned_assume);
      add_state_assume("", isaligned_assume, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map); //state_assumes.insert(isaligned_assume);
      //predicate p3(precond_t(m_ctx), m_ctx->mk_islangaligned(addr, align), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ALIGN_ISLANGALIGNED, predicate::assume);
      //assumes.insert(p3);
      //t.add_assume_pred(from_node->get_pc(), p3);
    }

    transfer_poison_values("", addr, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    break;
  }
  case Instruction::Load:
  {
    const LoadInst* l =  cast<const LoadInst>(&I);
    Value const *Addr = l->getPointerOperand();
    string lname = get_value_name(*l);

    expr_ref addr;
    tie(addr, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*Addr, lname, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    //if (   l->getType()->getTypeID() == Type::FloatTyID
    //    || l->getType()->getTypeID() == Type::DoubleTyID) {
    //  state_set_expr(state_out, G_SRC_KEYWORD "." LLVM_CONTAINS_FLOAT_OP_SYMBOL, m_ctx->mk_bool_const(true));
    //  break;
    //}
    sort_ref value_type = get_value_type(*l, m_module->getDataLayout());
    if (value_type->is_bool_kind()) {
      value_type = m_ctx->mk_bv_sort(1);
    }
    ASSERTCHECK((value_type->is_bv_kind() || value_type->is_float_kind() || value_type->is_floatx_kind()), cout << __func__ << " " << __LINE__ << ": value_type = " << value_type->to_string() << endl);
    memlabel_t ml_top;
    memlabel_t::keyword_to_memlabel(&ml_top, G_MEMLABEL_TOP_SYMBOL, MEMSIZE_MAX);
    unsigned count = DIV_ROUND_UP(value_type->get_size(), get_memory_addressable_size());
    size_t addressable_sz = ROUND_UP(value_type->get_size(), get_memory_addressable_size());
    expr_ref mem = state_get_expr(state_in, m_mem_reg, this->get_mem_sort());
    expr_ref mem_alloc = state_get_expr(state_in, m_mem_alloc_reg, this->get_mem_alloc_sort());
    expr_ref read_value = m_ctx->mk_select(mem, mem_alloc, ml_top, addr, count, false);

    transfer_poison_value_on_load(lname, read_value, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    if (addressable_sz != value_type->get_size()) {
      ASSERT(addressable_sz > value_type->get_size());
      read_value = m_ctx->mk_bvextract(read_value, value_type->get_size() - 1, 0);
    }
    if (read_value->get_sort()->get_size() == 1) {
      read_value = m_ctx->mk_eq(read_value, m_ctx->mk_bv_const(1, 1));
    }
    if (value_type->is_float_kind()) {
      read_value = m_ctx->mk_ieee_bv_to_float(read_value);
    } else if (value_type->is_floatx_kind()) {
      read_value = m_ctx->mk_ieee_bv_to_floatx(read_value);
    }
    set_expr(lname, read_value, state_out);

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
      //state_assumes.insert(isaligned_assume);
      add_state_assume(lname, isaligned_assume, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map); //state_assumes.insert(isaligned_assume);
      //predicate p2(precond_t(m_ctx), m_ctx->mk_islangaligned(addr, align), expr_true(m_ctx), UNDEF_BEHAVIOUR_ASSUME_ALIGN_ISLANGALIGNED, predicate::assume);
      //assumes.insert(p2);
      //t.add_assume_pred(from_node->get_pc(), p2);
    }
    Type *lTy = (*l).getType();
    //add_align_assumes(lname, lTy, read_value, pc_to/*from_node->get_pc()*/, t);
    // create extra edge for adding assumes related to the load target
    dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    tfg_edge_ref e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_out, expr_true(m_ctx)/*, t.get_start_state()*/, state_assumes, te_comment));
    t.add_edge(e);

    unordered_set<expr_ref> align_assumes = gen_align_assumes(lname, lTy, read_value->get_sort());
    for (auto const& align_assume : align_assumes) {
      add_state_assume(lname, align_assume, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map); //state_assumes.insert(align_assume);
    }
    state_out = state_in;
    from_node = t.find_node(intermediate_node->get_pc());
    ASSERT(from_node);

    transfer_poison_values("", addr, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    break;
  }
  case Instruction::Call:
  {
    const CallInst* c =  cast<const CallInst>(&I);

    Function *calleeF = c->getCalledFunction();
    string fun_name;
    expr_ref fun_expr;
    if (calleeF == NULL) {
      Value const *v = c->getCalledOperand();
      Value const *sv = v->stripPointerCasts();
      fun_name = string(sv->getName());
      tie(fun_expr, state_assumes) = get_expr_adding_edges_for_intermediate_vals(*v, "", state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
      if (fun_name != "") {
        fun_name = string(LLVM_FUNCTION_NAME_PREFIX) + fun_name;
        //fun_expr = m_ctx->mk_var(fun_name, m_ctx->mk_bv_sort(DWORD_LEN)); //shortcut the expression (we are assuming the casts are meaningless). This is a hack, and should be removed at some point.
      }

    } else {
      fun_name = calleeF->getName().str();
      fun_name = string(LLVM_FUNCTION_NAME_PREFIX) + fun_name;
      fun_expr = m_ctx->mk_var(fun_name, m_ctx->mk_bv_sort(get_word_length()));
    }
    if (   fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_DBG_DECLARE_FUNCTION
        || fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_DBG_VALUE_FUNCTION) {
      break;
    }
    if (string_has_prefix(fun_name, LLVM_FUNCTION_NAME_PREFIX G_LLVM_LIFETIME_FUNCTION_PREFIX)) {
      break;
    }

    if (fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_STACKSAVE_FUNCTION) {
      //cast<CallInst>(I).getIntrinsicID() == Intrinsic::stacksave
      state_out = this->parse_stacksave_intrinsic(I, t, from_node->get_pc());
      break;
    }
    if (fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_STACKRESTORE_FUNCTION) {
      //cast<CallInst>(I).getIntrinsicID() == Intrinsic::stackrestore
      state_out = this->parse_stackrestore_intrinsic(I, t, from_node->get_pc());
      break;
    }

    if (fun_name == string(LLVM_FUNCTION_NAME_PREFIX) + cur_function_name) {
      fun_name = LLVM_FUNCTION_NAME_PREFIX G_SELFCALL_IDENTIFIER;
      fun_expr = m_ctx->mk_var(fun_name, m_ctx->mk_bv_sort(get_word_length()));
    }
    string memcpy_fn = LLVM_FUNCTION_NAME_PREFIX G_LLVM_MEMCPY_FUNCTION;
    unordered_set<expr_ref> succ_assumes;
    if (fun_name.substr(0, memcpy_fn.length()) == memcpy_fn) {
      tie(state_assumes, succ_assumes) = apply_memcpy_function(c, fun_expr, fun_name, src_llvm_tfg, calleeF, state_in, state_out, state_assumes, cur_function_name, from_node, model_llvm_semantics, F, t/*, function_tfg_map*/, value_to_name_map/*, function_call_chain*/, scev_map, xml_output_format);
    } else if (fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_VA_START_FUNCTION) {
      tie(state_assumes, succ_assumes) = apply_va_start_function(c, state_in, state_out, state_assumes, cur_function_name, from_node, model_llvm_semantics, t, value_to_name_map);
    } else if (fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_VA_COPY_FUNCTION) {
      tie(state_assumes, succ_assumes) = apply_va_copy_function(c, state_in, state_out, state_assumes, cur_function_name, from_node, model_llvm_semantics, t, value_to_name_map);
    } else if (fun_name == LLVM_FUNCTION_NAME_PREFIX G_LLVM_VA_END_FUNCTION) {
      //NOP
    } else {
      tie(state_assumes, succ_assumes) = apply_general_function(c, fun_expr, fun_name, src_llvm_tfg, calleeF, state_in, state_out, state_assumes, cur_function_name, from_node, model_llvm_semantics, t/*, function_tfg_map*/, value_to_name_map/*, function_call_chain*/, scev_map, xml_output_format);
    }

    string fname;
    if (fun_name != "") {
      if (!string_has_prefix(fun_name, LLVM_FUNCTION_NAME_PREFIX)) {
        cout << _FNLN_ << ": fun_name = '" << fun_name << "'\n";
      }
      ASSERT(string_has_prefix(fun_name, LLVM_FUNCTION_NAME_PREFIX));
      fname = fun_name.substr(strlen(LLVM_FUNCTION_NAME_PREFIX));
    }
    auto fcall_state = state_out;
    if (succ_assumes.size()) {
      // create extra edge for adding assumes related to the function return value target
      dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
      tfg_edge_ref e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_out, expr_true(m_ctx), state_assumes, te_comment));
      t.add_edge(e);

      state_assumes = succ_assumes;
      state_out = state_in;
      from_node = t.find_node(intermediate_node->get_pc());
      ASSERT(from_node);
    }

    state state_out_heap_allocfree;
    unordered_set<expr_ref> heap_allocfree_assumes;
    if (ftmap_t::function_name_represents_mallocfree(cur_function_name, fname, from_node->get_pc(), fcall_state, m_mem_alloc_reg, m_mem_reg, m_word_length, state_out_heap_allocfree, heap_allocfree_assumes)) {
      dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
      tfg_edge_ref e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_out, expr_true(m_ctx), state_assumes, te_comment));
      t.add_edge(e);

      state_assumes = heap_allocfree_assumes;
      state_out = state_out_heap_allocfree;
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
  case Instruction::FSub:
  case Instruction::FRem:
  {
    sort_ref isort = get_value_type(I, dl);
    ASSERT(isort->is_float_kind() || isort->is_floatx_kind());
    string iname = get_value_name(I);

    Value const &op0 = *I.getOperand(0);
    Value const &op1 = *I.getOperand(1);

    expr_ref e0, e1;
    tie(e0, state_assumes) = get_expr_adding_edges_for_intermediate_vals(op0, iname, state(), state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    tie(e1, state_assumes) = get_expr_adding_edges_for_intermediate_vals(op1, iname, state(), state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    if (e0->is_floatx_sort()) {
      e0 = m_ctx->mk_floatx_to_float(e0);
    }

    if (e1->is_floatx_sort()) {
      e1 = m_ctx->mk_floatx_to_float(e1);
    }

    expr_vector args;
    args.push_back(this->get_cur_rounding_mode_var());
    args.push_back(e0);
    args.push_back(e1);

    expr_ref e = m_ctx->mk_app(farith_to_operation_kind(I.getOpcode(), args), args);

    if (isort->is_floatx_kind()) {
      e = m_ctx->mk_float_to_floatx(e);
    }

    transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    state_set_expr(state_out, iname, e);
    break;
  }
  case Instruction::FNeg: {
    sort_ref isort = get_value_type(I, dl);
    ASSERT(isort->is_float_kind() || isort->is_floatx_kind());
    string iname = get_value_name(I);

    Value const &op0 = *I.getOperand(0);

    expr_ref e0;
    tie(e0, state_assumes) = get_expr_adding_edges_for_intermediate_vals(op0, iname, state(), state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    if (e0->is_floatx_sort()) {
      e0 = m_ctx->mk_floatx_to_float(e0);
    }

    expr_vector args;
    args.push_back(e0);

    expr_ref e = m_ctx->mk_app(farith_to_operation_kind(I.getOpcode(), args), args);

    if (isort->is_floatx_kind()) {
      e = m_ctx->mk_float_to_floatx(e);
    }
    transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    state_set_expr(state_out, iname, e);
    break;
  }
  case Instruction::FCmp: {
    FCmpInst const *FI = cast<FCmpInst const>(&I);
    ASSERT(FI);
    sort_ref isort = get_value_type(I, dl);
    ASSERT(isort->is_bool_kind());
    string iname = get_value_name(I);

    Value const &op0 = *I.getOperand(0);
    Value const &op1 = *I.getOperand(1);

    expr_ref e0, e1;
    tie(e0, state_assumes) = get_expr_adding_edges_for_intermediate_vals(op0, iname, state(), state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    tie(e1, state_assumes) = get_expr_adding_edges_for_intermediate_vals(op1, iname, state(), state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    if (e0->is_floatx_sort()) {
      e0 = m_ctx->mk_floatx_to_float(e0);
    }

    if (e1->is_floatx_sort()) {
      e1 = m_ctx->mk_floatx_to_float(e1);
    }

    expr_vector args;
    args.push_back(e0);
    args.push_back(e1);

    expr_ref e = fcmp_to_expr(FI->getPredicate(), args);

    transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    state_set_expr(state_out, iname, e);
    break;
  }
  case Instruction::FPToUI: {
    //FPToUIInst const *FI = cast<FPToUIInst const>(&I);
    //ASSERT(FI);
    sort_ref isort = get_value_type(I, dl);
    ASSERT(isort->is_bool_kind() || isort->is_bv_kind());
    size_t target_size = isort->is_bool_kind() ? 1 : isort->get_size();
    string iname = get_value_name(I);

    Value const &op0 = *I.getOperand(0);

    expr_ref e0;
    tie(e0, state_assumes) = get_expr_adding_edges_for_intermediate_vals(op0, iname, state(), state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    float_max_t max_limit = powl((float_max_t)2, target_size);
    float_max_t min_limit = 0;

    if (e0->is_floatx_sort()) {
      e0 = m_ctx->mk_floatx_to_float(e0);
    }

    ASSERT(e0->is_float_sort());
    unordered_set<expr_ref> float_assumes;
    //add to float_assumes the conditions that op0 is within limits
    float_assumes.insert(m_ctx->mk_fcmp_oge(e0, m_ctx->mk_float_const(e0->get_sort()->get_size(), min_limit)));
    float_assumes.insert(m_ctx->mk_fcmp_olt(e0, m_ctx->mk_float_const(e0->get_sort()->get_size(), max_limit)));

    for (auto const& float_assume : float_assumes) {
      add_state_assume(iname, float_assume, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    }

    expr_ref e = m_ctx->mk_fp_to_ubv(m_ctx->mk_rounding_mode_const(rounding_mode_t::round_towards_zero_aka_truncate()), e0, target_size);

    transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    state_set_expr(state_out, iname, e);
    break;
  }
  case Instruction::FPToSI: {
    //FPToUIInst const *FI = cast<FPToUIInst const>(&I);
    //ASSERT(FI);
    sort_ref isort = get_value_type(I, dl);
    ASSERT(isort->is_bool_kind() || isort->is_bv_kind());
    size_t target_size = isort->is_bool_kind() ? 1 : isort->get_size();
    string iname = get_value_name(I);

    Value const &op0 = *I.getOperand(0);

    expr_ref e0;
    tie(e0, state_assumes) = get_expr_adding_edges_for_intermediate_vals(op0, iname, state(), state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    float_max_t max_limit = powl((float_max_t)2, target_size - 1);
    float_max_t min_limit = powl((float_max_t)-2, target_size - 1);

    expr_ref min_limit_expr = m_ctx->mk_float_const(e0->get_sort()->get_size(), min_limit);
    expr_ref max_limit_expr = m_ctx->mk_float_const(e0->get_sort()->get_size(), max_limit);

    //cout << _FNLN_ << ": min_limit_expr = " << expr_string(min_limit_expr) << endl;
    //cout << _FNLN_ << ": max_limit_expr = " << expr_string(max_limit_expr) << endl;

    if (e0->is_floatx_sort()) {
      e0 = m_ctx->mk_floatx_to_float(e0);
    }

    ASSERT(e0->is_float_sort());
    unordered_set<expr_ref> float_assumes;
    //add to float_assumes the conditions that op0 is within limits
    float_assumes.insert(m_ctx->mk_fcmp_oge(e0, min_limit_expr));
    float_assumes.insert(m_ctx->mk_fcmp_olt(e0, max_limit_expr));

    for (auto const& float_assume : float_assumes) {
      add_state_assume(iname, float_assume, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    }

    expr_ref e = m_ctx->mk_fp_to_sbv(m_ctx->mk_rounding_mode_const(rounding_mode_t::round_towards_zero_aka_truncate()), e0, target_size);

    transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    state_set_expr(state_out, iname, e);
    break;
  }
  case Instruction::UIToFP: {
    sort_ref isort = get_value_type(I, dl);
    ASSERT(isort->is_float_kind() || isort->is_floatx_kind());
    size_t target_size = isort->is_float_kind() ? isort->get_size() : isort->get_size() - 1;
    string iname = get_value_name(I);
    Value const &op0 = *I.getOperand(0);
    expr_ref e0;
    tie(e0, state_assumes) = get_expr_adding_edges_for_intermediate_vals(op0, iname, state(), state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    expr_ref e = m_ctx->mk_ubv_to_fp(this->get_cur_rounding_mode_var(), e0, target_size);
    if (isort->is_floatx_kind()) {
      e = m_ctx->mk_float_to_floatx(e);
    }

    transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    state_set_expr(state_out, iname, e);
    break;
  }
  case Instruction::SIToFP: {
    sort_ref isort = get_value_type(I, dl);
    ASSERT(isort->is_float_kind() || isort->is_floatx_kind());
    size_t target_size = isort->is_float_kind() ? isort->get_size() : isort->get_size() - 1;
    string iname = get_value_name(I);
    Value const &op0 = *I.getOperand(0);
    expr_ref e0;
    tie(e0, state_assumes) = get_expr_adding_edges_for_intermediate_vals(op0, iname, state(), state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    expr_ref e = m_ctx->mk_sbv_to_fp(this->get_cur_rounding_mode_var(), e0, target_size);
    if (isort->is_floatx_kind()) {
      e = m_ctx->mk_float_to_floatx(e);
    }

    transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    state_set_expr(state_out, iname, e);
    break;
  }
  case Instruction::FPTrunc: {
    FPTruncInst const *FI = cast<FPTruncInst const>(&I);
    ASSERT(FI);
    //Type* typ = FI->getType();
    //size_t resultBitwidth = dl.getTypeSizeInBits(typ);
    ASSERT(I.getNumOperands() == 1);
    Value const &op0 = *I.getOperand(0);
    string iname = get_value_name(I);
    string op0name = get_value_name(op0);
    sort_ref op0_sort = get_value_type(op0, dl);
    ASSERT(op0_sort->is_float_kind() || op0_sort->is_floatx_kind());
    sort_ref isort = get_value_type(I, dl);
    ASSERT(isort->is_float_kind());
    size_t target_size = isort->get_size();
    int ebits, sbits;
    tie(ebits, sbits) = mybitset::floating_point_get_ebits_and_sbits_from_size(target_size);
    stringstream ss;
    ss << G_INPUT_KEYWORD "." << op0name;
    expr_ref op0_e = m_ctx->mk_var(ss.str(), op0_sort);
    if (op0_e->is_floatx_sort()) {
      op0_e = m_ctx->mk_floatx_to_float(op0_e);
    }

    expr_ref e = m_ctx->mk_fptrunc(this->get_cur_rounding_mode_var(), op0_e, ebits, sbits);
    transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    state_set_expr(state_out, iname, e);
    //cout << __func__ << " " << __LINE__ << ": FPTrunc: state_out =\n" << state_out.to_string_for_eq() << endl;
    break;
  }
  case Instruction::FPExt: {
    FPExtInst const *FI = cast<FPExtInst const>(&I);
    ASSERT(FI);
    //Type* typ = FI->getType();
    //size_t resultBitwidth = dl.getTypeSizeInBits(typ);
    ASSERT(I.getNumOperands() == 1);
    Value const &op0 = *I.getOperand(0);
    string iname = get_value_name(I);
    string op0name = get_value_name(op0);
    sort_ref op0_sort = get_value_type(op0, dl);
    ASSERT(op0_sort->is_float_kind());
    sort_ref isort = get_value_type(I, dl);
    ASSERT(isort->is_float_kind() || isort->is_floatx_kind());
    size_t target_size = isort->get_size();
    if (isort->is_floatx_kind()) {
      target_size--;
    }
    int ebits, sbits;
    tie(ebits, sbits) = mybitset::floating_point_get_ebits_and_sbits_from_size(target_size);

    stringstream ss;
    ss << G_INPUT_KEYWORD "." << op0name;
    expr_ref op0_e = m_ctx->mk_var(ss.str(), op0_sort);

    expr_ref e = m_ctx->mk_fpext(op0_e, ebits, sbits);
    if (isort->is_floatx_kind()) {
      e = m_ctx->mk_float_to_floatx(e);
    }
    transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    state_set_expr(state_out, iname, e);
    //cout << __func__ << " " << __LINE__ << ": FPExt: state_out =\n" << state_out.to_string_for_eq() << endl;
    break;
  }
  case Instruction::ExtractValue: {
    ASSERT(I.getNumOperands() == 1);
    Value const &op0 = *I.getOperand(0);
    string iname = get_value_name(I);
    string op0name = get_value_name(op0);
    sort_ref op0_sort = get_value_type(op0, dl);
    sort_ref s = get_value_type(I, dl);
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
      expr_ref e = m_ctx->mk_var(ss.str(), s);
      transfer_poison_values(iname, e, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
      state_set_expr(state_out, iname, e);
    }
    break;
  }
  default: {
    string iname = get_value_name(I);
    vector<expr_ref> expr_args;
    tie(expr_args, state_assumes) = get_expr_args(I, iname, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    expr_ref insn_expr;
    tie(insn_expr, state_assumes) = exec_gen_expr(I, iname, expr_args, state_in, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);

    transfer_poison_values(iname, insn_expr, state_assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    set_expr(iname, insn_expr, state_out);
    break;
  }
  }
  DYN_DEBUG(llvm2tfg, errs() << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": sym exec done\n");
  process_cfts(t, value_to_name_map, from_node, pc_to, state_out, state_assumes, model_llvm_semantics, te_comment, (Instruction *)&I, cfts, B, F, eimap);
}

te_comment_t
sym_exec_common::phi_node_to_te_comment(/*bbl_order_descriptor_t const& bbo, */int inum, llvm::Instruction const& I) const
{
  string s;
  raw_string_ostream ss(s);
  //cout << __func__ << " " << __LINE__ << ": from_index = " << from_index << ", from_subindex = " << from_subindex << ", from_subsubindex = " << from_subsubindex << ", bbo = " << bbo.get_bbl_num() << ", to_pc = " << to_pc.to_string() << endl;
  ss << I;
  return te_comment_t(/*bbo, */true, inum, ss.str());
}


te_comment_t
sym_exec_common::instruction_to_te_comment(llvm::Instruction const& I, pc const& from_pc/*, bbl_order_descriptor_t const& bbo*/) const
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
  return te_comment_t(/*bbo, */false, from_subindex, ss.str());
}

//exec_gen_expr(): this is shared common logic for general instruction computation and constant-expression computation;
//thus these common opcodes are encapsulated in a separate function
//Returns: resulting expression EXPR_REF, set of UB assumes UNORDERED_SET<EXPR_REF>
pair<expr_ref,unordered_set<expr_ref>>
sym_exec_llvm::exec_gen_expr(const llvm::Instruction& I, string const& Iname, const vector<expr_ref>& args, state const &state_in, unordered_set<expr_ref> const& state_assumes, dshared_ptr<tfg_node> &from_node, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map)
{
  //errs() << "exec_gen_expr: " << I << " (function " << F.getName() << ")\n";
  pc const &from_pc = from_node->get_pc();
  //string const& bbindex = get_basicblock_index(B);
  //bbl_order_descriptor_t const& bbo = this->m_bbl_order_map.at(mk_string_ref(bbindex));
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
      //assumes.insert(gen_shiftcount_assume_expr(args[1], args[0]->get_sort()->get_size()));
      add_state_assume(Iname, gen_shiftcount_assume_expr(args[1], args[0]->get_sort()->get_size()), state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
    }
    if (   I.getOpcode() == Instruction::UDiv
        || I.getOpcode() == Instruction::SDiv
        || I.getOpcode() == Instruction::URem
        || I.getOpcode() == Instruction::SRem) {
      ASSERT(args[0]->is_bv_sort());
      //assumes.insert(gen_no_divbyzero_assume_expr(args[1]));
      add_state_assume(Iname, gen_no_divbyzero_assume_expr(args[1]), state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
      if (   I.getOpcode() == Instruction::SDiv
          || I.getOpcode() == Instruction::SRem) {
        //assumes.insert(gen_div_no_overflow_assume_expr(args[0], args[1]));
        add_state_assume(Iname, gen_div_no_overflow_assume_expr(args[0], args[1]), state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
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
    return exec_gen_expr_casts(cast<CastInst>(I), args[0], state_assumes, from_node->get_pc(), model_llvm_semantics, t);
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
    //string gnp = (Iname == "" ? gep_name_prefix("gep", from_node->get_pc(), pc_to, 0) : Iname);
    //expr_ref total_offset_expr = m_ctx->mk_bv_const(ptr->get_sort()->get_size(), 0);
    for(gep_type_iterator itype = itype_begin; itype != itype_end; ++itype, ++index_counter)
    {
      Type* typ = itype.getIndexedType();

      expr_ref index = args[index_counter];
      assert(index->is_bv_sort());
      if (index->get_sort()->get_size() > get_word_length()) {
        cout << _FNLN_ << ": index =\n" << m_ctx->expr_to_string_table(index) << endl;
        cout << _FNLN_ << ": get_word_length() = " << get_word_length() << endl;
      }
      assert(index->get_sort()->get_size() <= get_word_length());
      if (index->get_sort()->get_size() < get_word_length()) {
        index = m_ctx->mk_bvzero_ext(index, get_word_length() - index->get_sort()->get_size());
        index = m_ctx->expr_do_simplify(index);
      }
      ASSERT(index->get_sort()->get_size() == get_word_length());
      expr_ref offset_expr;
      unordered_set<expr_ref> assumes = state_assumes;

      if (itype.isStruct()) {
        //StructType* s = dyn_cast<StructType>(typ)
        StructType* s = itype.getStructType();
        if (!index->is_const()) {
          errs() << _FNLN_ << ": index =\n" << m_ctx->expr_to_string_table(index) << "\n";
        }
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
          bool index_is_positive = itype.isBoundedSequential(); // if base is array then index must be positive
          expr_ref overflow_expr = gen_no_mul_overflow_assume_expr(index, size_expr, index_is_positive);
          expr_ref assume = m_ctx->mk_isindexforsize(overflow_expr, size);
          //assumes.insert(assume);
          add_state_assume(Iname, assume, state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
        }
      } else {
        unreachable();
      }
      ASSERT(offset_expr.get());

      expr_ref new_cur_expr = m_ctx->mk_bvadd(cur_expr, offset_expr);

      string offset_name       = gep_instruction_get_intermediate_value_name(I/*gnp*/, index_counter, 0);
      string total_offset_name = gep_instruction_get_intermediate_value_name(I/*gnp*/, index_counter, 1);

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
        //assumes.insert(assume);
        add_state_assume(Iname, assume, state_in, assumes, from_node, model_llvm_semantics, t, value_to_name_map);
      }

      dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
      shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(cur_pc, intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx)/*, t.get_start_state()*/, assumes, this->instruction_to_te_comment(I, from_node->get_pc()/*, bbo*/)));
      t.add_edge(e);

      cur_expr = mk_fresh_expr(total_offset_name, G_INPUT_KEYWORD, m_ctx->mk_bv_sort(get_word_length()));
      cur_pc = intermediate_node->get_pc();
    }

    string total_offset_name = gep_instruction_get_intermediate_value_name(I/*gnp*/, index_counter, 1);

    state state_to_intermediate_val;
    state_set_expr(state_to_intermediate_val, total_offset_name, m_ctx->mk_bvadd(ptr, cur_expr));

    dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(cur_pc, intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx)/*, t.get_start_state()*/, {}, this->instruction_to_te_comment(I, from_node->get_pc()/*, bbo*/)));
    t.add_edge(e);

    cur_expr = mk_fresh_expr(total_offset_name, G_INPUT_KEYWORD, m_ctx->mk_bv_sort(get_word_length()));
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

scev_op_t
sym_exec_llvm::get_scev_op_from_scev_type(SCEVTypes scevtype)
{
  switch (scevtype) {
    case scConstant: return scev_op_constant;
    case scTruncate: return scev_op_truncate;
    case scZeroExtend: return scev_op_zeroext;
    case scSignExtend: return scev_op_signext;
    case scAddRecExpr: return scev_op_addrec;
    case scAddExpr: return scev_op_add;
    case scMulExpr: return scev_op_mul;
    case scUMaxExpr: return scev_op_umax;
    case scSMaxExpr: return scev_op_smax;
    case scUMinExpr: return scev_op_umin;
    case scSMinExpr: return scev_op_smin;
    case scUDivExpr: return scev_op_udiv;
    case scUnknown: return scev_op_unknown;
    case scCouldNotCompute: return scev_op_couldnotcompute;
    default: NOT_REACHED();
  }
}

scev_ref
sym_exec_llvm::get_scev(ScalarEvolution& SE, SCEV const* scev, string const& srcdst_keyword, size_t word_length)
{
  SCEVTypes scevtype = static_cast<SCEVTypes>(scev->getSCEVType());
  switch (scevtype) {
    case scConstant: {
      APInt const& apint = cast<SCEVConstant>(scev)->getAPInt();
      return mk_scev(scev_op_constant, get_mybitset_from_apint(apint, apint.getBitWidth(), false), {});
    }
    case scTruncate: {
      const SCEVTruncateExpr *Trunc = cast<SCEVTruncateExpr>(scev);
      const SCEV *Op = Trunc->getOperand();
      //OS << "(trunc " << *Op->getType() << " " << *Op << " to "
      //   << *Trunc->getType() << ")";
      return mk_scev(scev_op_truncate, mybitset(), { get_scev(SE, Op, srcdst_keyword, word_length) });
    }
    case scZeroExtend: {
      const SCEVZeroExtendExpr *ZExt = cast<SCEVZeroExtendExpr>(scev);
      const SCEV *Op = ZExt->getOperand();
      //OS << "(zext " << *Op->getType() << " " << *Op << " to "
      //   << *ZExt->getType() << ")";
      return mk_scev(scev_op_zeroext, mybitset(), { get_scev(SE, Op, srcdst_keyword, word_length) });
    }
    case scSignExtend: {
      const SCEVSignExtendExpr *SExt = cast<SCEVSignExtendExpr>(scev);
      const SCEV *Op = SExt->getOperand();
      return mk_scev(scev_op_signext, mybitset(), { get_scev(SE, Op, srcdst_keyword, word_length) });
    }
    case scAddRecExpr: {
      const SCEVAddRecExpr *AR = cast<SCEVAddRecExpr>(scev);
      //OS << "{" << *AR->getOperand(0);
      vector<scev_ref> scev_args;
      for (unsigned i = 0, e = AR->getNumOperands(); i != e; ++i) {
        //OS << ",+," << *AR->getOperand(i);
        scev_args.push_back(get_scev(SE, AR->getOperand(i), srcdst_keyword, word_length));
      }
      //OS << "}<";
      scev_overflow_flag_t scev_overflow_flag;
      if (AR->hasNoUnsignedWrap()) {
        //OS << "nuw><";
        scev_overflow_flag.add(scev_overflow_flag_t::scev_overflow_flag_nuw);
      }
      if (AR->hasNoSignedWrap()) {
        //OS << "nsw><";
        //flag_nsw = true;
        scev_overflow_flag.add(scev_overflow_flag_t::scev_overflow_flag_nsw);
      }
      if (AR->hasNoSelfWrap() &&
          !AR->getNoWrapFlags((SCEV::NoWrapFlags)(SCEV::FlagNUW | SCEV::FlagNSW))) {
        //OS << "nw><";
        //flag_nw = true;
        scev_overflow_flag.add(scev_overflow_flag_t::scev_overflow_flag_nw);
      }
      pc loop_pc = get_loop_pc(AR->getLoop(), srcdst_keyword);
      return mk_scev(scev_op_addrec, mybitset(), scev_args, loop_pc, scev_overflow_flag);
      //AR->getLoop()->getHeader()->printAsOperand(OS, /*PrintType=*/false);
      //OS << ">";
      //return;
    }
    case scAddExpr:
    case scMulExpr:
    case scUMaxExpr:
    case scSMaxExpr:
    case scUMinExpr:
    case scSMinExpr: {
      const SCEVNAryExpr *NAry = cast<SCEVNAryExpr>(scev);
      vector<scev_ref> scev_args;
      for (int i = 0; i < NAry->getNumOperands(); i++) {
        scev_ref scev_arg = get_scev(SE, NAry->getOperand(i), srcdst_keyword, word_length);
        scev_args.push_back(scev_arg);
      }
      scev_overflow_flag_t scev_overflow_flag;
      if (NAry->getSCEVType() == scAddExpr || NAry->getSCEVType() == scMulExpr) {
        if (NAry->hasNoUnsignedWrap()) {
          scev_overflow_flag.add(scev_overflow_flag_t::scev_overflow_flag_nuw);
        }
        if (NAry->hasNoSignedWrap()) {
          scev_overflow_flag.add(scev_overflow_flag_t::scev_overflow_flag_nsw);
        }
      }
      return mk_scev(get_scev_op_from_scev_type(scevtype), mybitset(), scev_args, pc::start(), scev_overflow_flag);
    }
    case scUDivExpr: {
      const SCEVUDivExpr *UDiv = cast<SCEVUDivExpr>(scev);
      //OS << "(" << *UDiv->getLHS() << " /u " << *UDiv->getRHS() << ")";
      vector<scev_ref> scev_args;
      scev_args.push_back(get_scev(SE, UDiv->getLHS(), srcdst_keyword, word_length));
      scev_args.push_back(get_scev(SE, UDiv->getRHS(), srcdst_keyword, word_length));

      return mk_scev(scev_op_udiv, mybitset(), scev_args);
    }
    case scUnknown: {
      const SCEVUnknown *U = cast<SCEVUnknown>(scev);
      Type *AllocTy;

      string ret;
      raw_string_ostream ss(ret);
      string name;
      if (U->isSizeOf(AllocTy)) {
        ss << "sizeof(" << *AllocTy << ")";
        name = ss.str();
      } else if (U->isAlignOf(AllocTy)) {
        ss << "alignof(" << *AllocTy << ")";
        name = ss.str();
      } else {
        Type *CTy;
        Constant *FieldNo;
        if (U->isOffsetOf(CTy, FieldNo)) {
          ss << "offsetof(" << *CTy << ", ";
          FieldNo->printAsOperand(ss, false);
          ss << ")";
          name = ss.str();
        }
      }

      if (name == "") {
        U->getValue()->printAsOperand(ss, false);
        name = string(G_INPUT_KEYWORD ".") + srcdst_keyword + ("." G_LLVM_PREFIX "-") + ss.str();
      }
      if (name == "") {
        name = "unknown-notimplemented";
      }
      size_t bitwidth;
      Type const* typ = U->getType();
      if (IntegerType const* itype = dyn_cast<IntegerType const>(typ)) {
        bitwidth = itype->getBitWidth();
      } else if (isa<PointerType const>(typ)) {
        bitwidth = word_length;
      } else NOT_REACHED();
      return mk_scev(scev_op_unknown, name, bitwidth);
    }
    case scCouldNotCompute: {
      return mk_scev(scev_op_couldnotcompute, mybitset(), {});
    }
    //default: {
    //  NOT_REACHED();
    //}
  }
  NOT_REACHED();
}

mybitset
sym_exec_llvm::get_mybitset_from_apint(llvm::APInt const& apint, uint32_t bitwidth, bool is_signed)
{
  mybitset ret;
  if (bitwidth <= QWORD_LEN) {
    if (is_signed) {
      int64_t I = apint.getSExtValue();
      ret = mybitset((long long)I, bitwidth);
    } else {
      uint64_t N = apint.getZExtValue();
      ret = mybitset((unsigned long long)N, bitwidth);
    }
    {
      SmallString<4096> s;
      apint.toStringUnsigned(s);
      mybitset a(s.c_str(), bitwidth);
      ASSERT(ret == a);
    }
    return ret;
  } else {
    SmallString<4096> s;
    apint.toStringUnsigned(s);
    return mybitset(s.c_str(), bitwidth);
  }
}

pair<mybitset, mybitset>
sym_exec_llvm::get_bounds_from_range(llvm::ConstantRange const& crange, bool is_signed)
{
  APInt const& lower = crange.getLower();
  APInt const& upper = crange.getUpper();
  uint32_t bitwidth = crange.getBitWidth();
  mybitset lmbs = get_mybitset_from_apint(lower, bitwidth, is_signed);
  mybitset umbs = get_mybitset_from_apint(upper, bitwidth, is_signed);
  return make_pair(lmbs, umbs);
}

scev_with_bounds_t
sym_exec_llvm::get_scev_with_bounds(ScalarEvolution& SE, SCEV const* scev, string const& srcdst_keyword, size_t word_length)
{
  pair<mybitset, mybitset> unsigned_bounds = get_bounds_from_range(SE.getUnsignedRange(scev), false);
  pair<mybitset, mybitset> signed_bounds = get_bounds_from_range(SE.getSignedRange(scev), true);
  scev_ref scevr = get_scev(SE, scev, srcdst_keyword, word_length);
  return scev_with_bounds_t(scevr, unsigned_bounds.first, unsigned_bounds.second, signed_bounds.first, signed_bounds.second);
}

pc
sym_exec_llvm::get_loop_pc(Loop const* L, string const& srcdst_keyword)
{
  if (!L) {
    return pc::start();
  }
  BasicBlock* Header = L->getHeader();
  ASSERT(Header);
  return get_pc_from_bbindex_and_insn_id(sym_exec_llvm::get_basicblock_index(*Header), 0);
}

scev_toplevel_t<pc>
sym_exec_llvm::get_scev_toplevel(Instruction& I, ScalarEvolution * scev, LoopInfo const* loopinfo, string const& srcdst_keyword, size_t word_length)
{
  SCEV const* sv = scev->getSCEV(&I);
  Loop const* L = loopinfo->getLoopFor(I.getParent());
  SCEV const* atuse_sv = scev->getSCEVAtScope(sv, L);
  SCEV const* atexit_sv = L ? scev->getSCEVAtScope(sv, L->getParentLoop()) : nullptr;

  scev_with_bounds_t val_scevb = get_scev_with_bounds(*scev, sv, srcdst_keyword, word_length);
  scev_with_bounds_t atuse_scevb = get_scev_with_bounds(*scev, atuse_sv, srcdst_keyword, word_length);
  pc loop_pc = get_loop_pc(L, srcdst_keyword);
  scev_ref atexit_scev = atexit_sv ? get_scev(*scev, atexit_sv, srcdst_keyword, word_length) : nullptr;
  return scev_toplevel_t<pc>(val_scevb, atuse_scevb, atexit_scev, loop_pc);
}

void
sym_exec_llvm::sym_exec_populate_tfg_scev_map(tfg_llvm_t& t_src, value_scev_map_t const& value_scev_map) const
{
  for (auto const& vs : value_scev_map) {
    string const& iname = vs.first;
    scev_toplevel_t<pc> const& scev_toplevel = vs.second;
    t_src.add_scev_mapping(mk_string_ref(iname), scev_toplevel);
  }
}

dshared_ptr<tfg_llvm_t>
sym_exec_llvm::get_tfg(llvm::Function& F, llvm::Module const *M, string const &name, context *ctx, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, bool model_llvm_semantics, map<llvm_value_id_t, string_ref>* value_to_name_map, map<shared_ptr<tfg_edge const>, Instruction *>& eimap, map<string, value_scev_map_t> const& scev_map, string const& srcdst_keyword, context::xml_output_format_t xml_output_format)
{
  autostop_timer func_timer(__func__);

  const DataLayout &dl = M->getDataLayout();
  unsigned pointer_size = dl.getPointerSize();
  //cout << __func__ << " " << __LINE__ << ": pointer_size = " << pointer_size << endl;
  ASSERT(pointer_size == DWORD_LEN/BYTE_LEN || pointer_size == QWORD_LEN/BYTE_LEN);
  sym_exec_llvm se(ctx, M, F, src_llvm_tfg/*, gen_callee_summary*/, BYTE_LEN, pointer_size * BYTE_LEN, srcdst_keyword);

  list<string> sorted_bbl_indices;
  for (BasicBlock const& BB: F) {
    sorted_bbl_indices.push_back(se.get_basicblock_index(BB));
  }

  //llvm::Function const &F = m_function;
  se.populate_state_template(F, model_llvm_semantics);
  state start_state;
  se.get_state_template(pc::start(), start_state);
  string fname = se.functionGetName(F);
  stringstream ss;
  ss << srcdst_keyword << "." G_LLVM_PREFIX "." << fname;
  dshared_ptr<tfg_llvm_t> t = make_dshared<tfg_llvm_t>(ss.str(), fname, ctx);
  ASSERT(t);
  t->set_start_state(start_state);

  //this->populate_bbl_order_map();

  for(const BasicBlock& B : F) {
    se.add_edges(B, src_llvm_tfg, model_llvm_semantics, *t, F/*, function_tfg_map*/, value_to_name_map/*, function_call_chain*/, eimap, scev_map, xml_output_format);
  }

  //for (const auto& arg : F.args()) {
  //  pair<argnum_t, expr_ref> const &a = m_arguments.at(get_value_name(arg));
  //  string Elname = get_value_name(arg) + SRC_INPUT_ARG_NAME_SUFFIX;
  //  Type *ElTy = arg.getType();
  //  add_type_and_align_assumes(Elname, ElTy, a.second, pc::start(), *t/*, assumes*/, UNDEF_BEHAVIOUR_ASSUME_ARG_ISLANGTYPE);
  //}

  se.get_tfg_common(*t);

  if (scev_map.count(name)) {
    se.sym_exec_populate_tfg_scev_map(*t, scev_map.at(name));
  }

  map<allocsite_t, graph_local_t> const& local_refs = se.get_local_refs();

  //pc start_pc = this->get_start_pc();
  pc start_pc = sym_exec_llvm::get_start_pc(se.m_function);
  t->add_extra_node_at_start_pc(start_pc);

  unordered_set<expr_ref> const& arg_assumes = se.gen_arg_assumes();
  t->add_assumes_to_start_edge(arg_assumes);

  t->tfg_initialize_rounding_mode_on_start_edge(se.get_cur_rounding_mode_varname(), se.m_rounding_mode_at_start_pc);

  //t->set_symbol_map_for_touched_symbols(*se.m_symbol_map, se.m_touched_symbols);
  //t->set_string_contents_for_touched_symbols_at_zero_offset(*se.m_string_contents, se.m_touched_symbols);

  set<symbol_id_t> all_symbols = map_get_keys(se.m_symbol_map->get_map());
  t->set_symbol_map_for_touched_symbols(*se.m_symbol_map, all_symbols);
  t->set_string_contents_for_touched_symbols_at_zero_offset(*se.m_string_contents, all_symbols);

  t->remove_function_name_from_symbols(name);
  t->populate_exit_return_values_for_llvm_method();
  t->canonicalize_llvm_nextpcs(src_llvm_tfg);
  t->tfg_llvm_interpret_intrinsic_fcalls();

  ASSERT(t->get_locals_map().size() == 0);
  t->set_locals_map(local_refs);
  t->tfg_initialize_uninit_nonce_on_start_edge(map_get_keys(local_refs), se.m_srcdst_keyword);

  t->tfg_llvm_set_sorted_bbl_indices(sorted_bbl_indices);
  t->tfg_preprocess(false/*, src_llvm_tfg*//*, xml_output_format*/);

  return t;
}

pc
sym_exec_common::get_pc_from_bbindex_and_insn_id(string const &bbindex/*llvm::BasicBlock const &B*/, size_t insn_id)
{
  pc ret(pc::insn_label, /*get_basicblock_index(B)*/bbindex.c_str()/*, m_basicblock_idx_map.at(string("%") + bbindex)*/, insn_id + PC_SUBINDEX_FIRST_INSN_IN_BB, PC_SUBSUBINDEX_DEFAULT);
  return ret;
}

vector<control_flow_transfer>
sym_exec_llvm::expand_switch(tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map, dshared_ptr<tfg_node> const &from_node, vector<control_flow_transfer> const &cfts, state const &state_to, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, llvm::Instruction * I, const llvm::BasicBlock& B, const llvm::Function& F, map<shared_ptr<tfg_edge const>, llvm::Instruction *>& eimap)
{
  unordered_set<expr_ref> cond_assumes = assumes;
  vector<control_flow_transfer> new_cfts;
  pc from_pc = from_node->get_pc();
  dshared_ptr<tfg_node> cur_node = from_node;
  size_t varnum = 0;
  for (size_t i = 0; i < cfts.size() - 1; i++) {
    const auto &cft = cfts.at(i);
    ASSERT(cft.get_target() == nullptr);
    ASSERT(cft.get_from_pc() == from_pc);

    pc to_pc = cft.get_to_pc();
    if (!t.find_node(to_pc)) {
      t.add_node(make_dshared<tfg_node>(to_pc));
    }
    dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, cur_node);
    pc const& intermediate_pc = intermediate_node->get_pc();
    dshared_ptr<tfg_node> next_case_node = get_next_intermediate_subsubindex_pc_node(t, intermediate_node);
    pc const& next_case_pc = next_case_node->get_pc();

    stringstream ss;
    ss << m_srcdst_keyword << "." G_LLVM_PREFIX "-" LLVM_SWITCH_TMPVAR_PREFIX << varnum++;
    string switch_tmpvar_name = ss.str();
    expr_ref switch_tmpvar = get_input_expr(switch_tmpvar_name, m_ctx->mk_bool_sort());

    // edge for setting switch tmpvar
    unordered_set<expr_ref> assumes = cft.get_assumes();
    unordered_set_union(assumes, cond_assumes);
    state state_to_newvar;
    state_set_expr(state_to_newvar, switch_tmpvar_name, cft.get_condition());
    shared_ptr<tfg_edge const> e1 = mk_tfg_edge(mk_itfg_edge(cur_node->get_pc(), intermediate_pc, state_to_newvar, expr_true(m_ctx), assumes, te_comment));
    t.add_edge(e1);
    DYN_DEBUG2(llvm2tfg, cout << __func__ << ':' << __LINE__ << ": Added edge e1 = " << e1->to_string() << endl);

    // edge for ~case jump
    shared_ptr<tfg_edge const> enp = mk_tfg_edge(mk_itfg_edge(intermediate_pc, next_case_pc, state_to, m_ctx->mk_not(switch_tmpvar), {}, te_comment));
    t.add_edge(enp);
    DYN_DEBUG2(llvm2tfg, cout << __func__ << ':' << __LINE__ << ": Added edge enp = " << enp->to_string() << endl);

    // create CFT for case jump
    control_flow_transfer case_cft(intermediate_pc, to_pc, switch_tmpvar);
    new_cfts.push_back(case_cft);

    cur_node = next_case_node;
    cond_assumes = {};
  }
  const auto &cft = cfts.at(cfts.size() - 1);
  ASSERT(cft.get_from_pc() == from_pc);
  pc to_pc = cft.get_to_pc();
  control_flow_transfer def_cft(cur_node->get_pc(), to_pc, expr_true(m_ctx), cond_assumes);
  new_cfts.push_back(def_cft);
  return new_cfts;
}

//template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
//pair<shared_ptr<tfg_node>, map<string, sort_ref>>
void
sym_exec_llvm::process_cft(tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map, dshared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, state const &state_to, unordered_set<expr_ref> const& assumes, bool model_llvm_semantics, te_comment_t const& te_comment, Instruction * I, const llvm::BasicBlock& B, const llvm::Function& F, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  DYN_DEBUG2(llvm2tfg, errs() << _FNLN_ << ": state_to: " << state_to.state_to_string_for_eq() << "\n");

  state state_to_cft(state_to);
  pc const &from_pc = from_node->get_pc();
  if (!t.find_node(pc_to)) {
    t.add_node(make_dshared<tfg_node>(pc_to));
  }
  if (target) {
    state_set_expr(state_to_cft, m_srcdst_keyword + "." LLVM_STATE_INDIR_TARGET_KEY_ID, target);
    auto e = mk_tfg_edge(mk_itfg_edge(from_pc, pc_to, state_to_cft, to_condition/*, t.get_start_state()*/, assumes, te_comment));
    eimap.insert(make_pair(e, I));
    t.add_edge(e);
    return;//make_pair(from_node, map<string, sort_ref>());
  } else if (pc_to.is_label() && pc_to.get_subindex() == PC_SUBINDEX_FIRST_INSN_IN_BB) {
    process_phi_nodes(t, value_to_name_map, &B, pc_to, from_node, model_llvm_semantics, to_condition, assumes, te_comment, I, F, eimap);
    return;
  } else {
    auto e = mk_tfg_edge(mk_itfg_edge(from_pc, pc_to, state_to_cft, to_condition/*, t.get_start_state()*/, assumes, te_comment));
    eimap.insert(make_pair(e, I));
    t.add_edge(e);
    return;//make_pair(from_node, map<string, sort_ref>());
  }
}

//template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
//void
//sym_exec_llvm::process_cft_second_half(tfg &t, shared_ptr<tfg_node> const &from_node, pc const &pc_to, expr_ref target, expr_ref to_condition, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, Instruction * I, const llvm::BasicBlock& B, const llvm::Function& F, map<std::string, sort_ref> const &phi_regnames, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
//{
//  if (pc_to.is_label() && pc_to.get_subindex() == PC_SUBINDEX_FIRST_INSN_IN_BB) {
//    process_phi_nodes_second_half(t, &B, pc_to, from_node, F, to_condition, phi_regnames, assumes, te_comment, I, eimap);
//  }
//}

//template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
void
sym_exec_llvm::process_cfts(tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map, dshared_ptr<tfg_node> const &from_node, pc const &pc_to, state const &state_to, unordered_set<expr_ref> const& state_assumes, bool model_llvm_semantics, te_comment_t const& te_comment, Instruction * I, vector<control_flow_transfer> const &cfts, llvm::BasicBlock const &B, llvm::Function const &F, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  map<pc, map<string, sort_ref>> pc_to_phi_regnames_map;
  if (cfts.size() == 0) {
    //pc pc_to(pc::insn_label, get_pc_from_bb_and_insn_id(B, next_insn_id));
    /*auto n = */process_cft(t, value_to_name_map, from_node, pc_to, nullptr, expr_true(m_ctx), state_to, state_assumes, model_llvm_semantics, te_comment, I, B, F, eimap);
    //ASSERT(n.first == from_node);
    //ASSERT(n.second.size() == 0);
  } else {
    //pc const &bb_last_pc = cfts.at(cfts.size() - 1).get_from_pc();
    //dshared_ptr<tfg_node> bb_last_node = t.find_node(bb_last_pc);
    //ASSERT(bb_last_node);
    for(const control_flow_transfer& cft : cfts) {
      //first half performs all the computation required in the successor phi nodes, as part of the current basic block
      unordered_set<expr_ref> assumes = state_assumes;
      unordered_set_union(assumes, cft.get_assumes());
      dshared_ptr<tfg_node> cft_from_node = t.find_node(cft.get_from_pc());
      ASSERT(cft_from_node);
      /*auto r = */process_cft(t, value_to_name_map, cft_from_node, cft.get_to_pc(), cft.get_target(), cft.get_condition(), state_to, assumes, model_llvm_semantics, te_comment, I, B, F, eimap);
      //pc_to_phi_regnames_map.insert(make_pair(cft.get_to_pc(), r.second));
      //bb_last_node = r.first;
    }
    //for(const control_flow_transfer& cft : cfts) {
    //  //second half creates control-flow edges from the current basic block to the successor(s), while copying the computed phi expressions from their temporary registers to their actual registers. This division of labour between first-half and second-half mimics LLC's code generator
    //  process_cft_second_half(t, (bb_last_pc == cft.get_from_pc()) ? bb_last_node : t.find_node(cft.get_from_pc()), cft.get_to_pc(), cft.get_target(), cft.get_condition(), cft.get_assumes(), te_comment, I, B, F, pc_to_phi_regnames_map.at(cft.get_to_pc()), eimap);
    //}
  }
}

void
sym_exec_llvm::parse_dbg_declare_intrinsic(Instruction const& I, tfg_llvm_t& t, pc const& pc_from) const
{
  const CallInst& CI = cast<CallInst>(I);
  const DbgDeclareInst &DI = cast<DbgDeclareInst>(CI);
  assert(DI.getVariable() && "Missing variable");

  const Value *Address = DI.getAddress();
  if (!Address || isa<UndefValue>(Address)) {
    DYN_DEBUG(dbg_declare_intrinsic, dbgs() << "Dropping debug info for " << DI << "\n");
    return;
  }

  auto AI = dyn_cast<AllocaInst>(Address);
  if (!AI) {
    DYN_DEBUG(dbg_declare_intrinsic, dbgs() << "Dropping debug info for " << DI << " because it is NOT an alloca\n");
    return;
  }
  string source_varname = DI.getVariable()->getName().str() + (m_srcdst_keyword == G_DST_KEYWORD ? "'" : "");
  string llvm_varname = get_value_name(*Address);
  DYN_DEBUG(dbg_declare_intrinsic, std::cout << "DI.getVariable().getName() = " << source_varname << "\n");
  DYN_DEBUG(dbg_declare_intrinsic, std::cout << "value_name = " << llvm_varname << "\n");
  t.tfg_llvm_add_source_declaration_to_llvm_varname_mapping_at_pc(source_varname, llvm_varname, pc_from);
}

void
sym_exec_llvm::parse_dbg_value_intrinsic(Instruction const& I, tfg_llvm_t& t, pc const& pc_from) const
{
  const CallInst& CI = cast<CallInst>(I);
  const DbgValueInst &DI = cast<DbgValueInst>(CI);

  Value* v = DI.getValue();
  if (!v) {
    return;
  } else if (const auto *CI = dyn_cast<Constant>(v)) {
    return;
  }
  string llvm_varname = string(G_INPUT_KEYWORD ".") + get_value_name(*v);
  string source_varname = DI.getVariable()->getName().str() + (m_srcdst_keyword == G_DST_KEYWORD ? "'" : "");
  DIExpression* die = DI.getExpression(); //not used so far
  DYN_DEBUG(dbg_declare_intrinsic, std::cout << "source_varname = " << source_varname << "\n");
  DYN_DEBUG(dbg_declare_intrinsic, std::cout << "llvm_varname = " << llvm_varname << "\n");
  DYN_DEBUG(dbg_declare_intrinsic, std::cout << "pc_from = " << pc_from << "\n");
  t.tfg_llvm_add_source_to_llvm_varname_mapping_at_pc(source_varname, llvm_varname, pc_from);
}

state
sym_exec_llvm::parse_stacksave_intrinsic(Instruction const& I, tfg& t, pc const& pc_from)
{
  const CallInst& CI = cast<CallInst>(I);
  // llvm.stacksave returns an opaque pointer value which can be passed to stackrestore.
  string opaque_keyname = get_value_name(CI);
  string opaque_varname = string(G_INPUT_KEYWORD ".") + opaque_keyname;

  //let's save the current state of mem.alloc in an SSA var
  state state_in, state_out;
  expr_ref mem_alloc_e = state_get_expr(state_in, m_mem_alloc_reg, this->get_mem_alloc_sort());
  string memalloc_ssa_varname = opaque_keyname + "." + G_MEMALLOC_SSA_VARNAME_SUFFIX;
  expr_ref memalloc_ssa_var = m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + memalloc_ssa_varname, this->get_mem_alloc_sort());
  m_opaque_varname_to_memalloc_map.insert(make_pair(opaque_varname, memalloc_ssa_var));
  //cout << _FNLN_ << ": opaque_varname = " << opaque_varname << ", memalloc_ssa_var = " << expr_string(memalloc_ssa_var) << endl;

  state_set_expr(state_out, memalloc_ssa_varname, mem_alloc_e);
  return state_out;
}

state
sym_exec_llvm::parse_stackrestore_intrinsic(Instruction const& I, tfg& t, pc const& pc_from)
{
  const CallInst& CI = cast<CallInst>(I);
  state state_in, state_out;

  Value* v = *CI.arg_operands().begin();
  if (!v) {
    return state_in;
  } else if (const auto *CI = dyn_cast<Constant>(v)) {
    return state_in;
  }
  string opaque_varname = string(G_INPUT_KEYWORD ".") + get_value_name(*v);

  if (!m_opaque_varname_to_memalloc_map.count(opaque_varname)) {
    cout << _FNLN_ << ": opaque_varname = " << opaque_varname << endl;
    cout << "m_opaque_varname_memalloc_map (size " << m_opaque_varname_to_memalloc_map.size() << ") =\n";
    for (auto const& v : m_opaque_varname_to_memalloc_map) {
      cout << v.first << " -> " << expr_string(v.second) << endl;
    }
  }

  //restore the state of mem.alloc using the memalloc map
  ASSERT(m_opaque_varname_to_memalloc_map.count(opaque_varname));
  expr_ref saved_memalloc = m_opaque_varname_to_memalloc_map.at(opaque_varname);
  //cout << _FNLN_ << ": opaque_varname = " << opaque_varname << ", saved_memalloc = " << expr_string(saved_memalloc) << endl;

  state_set_expr(state_out, this->m_mem_alloc_reg, saved_memalloc);
  return state_out;
}

void
sym_exec_llvm::add_edges(const llvm::BasicBlock& B, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, bool model_llvm_semantics, tfg_llvm_t& t, const llvm::Function& F/*, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> *function_tfg_map*/, map<llvm_value_id_t, string_ref>* value_to_name_map/*, set<string> const *function_call_chain*/, map<shared_ptr<tfg_edge const>, Instruction *>& eimap, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format)
{
  //errs() << "Doing BB: " << get_basicblock_name(B) << "\n";
  //errs() << "t.get_edges().size() = " << t.get_edges().size() << "\n";
  size_t insn_id = 0;
  bool pc_is_start = (t.get_edges().size() == 0);

  for (const Instruction& I : B) {
    if (isa<PHINode const>(I)) {
      ASSERT(!pc_is_start);
      continue;
    }
    if (   false
        || (isa<CallInst>(I) && cast<CallInst>(I).getIntrinsicID() == Intrinsic::dbg_addr)
        || (isa<CallInst>(I) && cast<CallInst>(I).getIntrinsicID() == Intrinsic::dbg_label)
       ) {
      continue;
    }

    pc pc_from = get_pc_from_bbindex_and_insn_id(get_basicblock_index(B), insn_id);
    pc pc_from_dbg;
    if (pc_is_start) {
      pc_from_dbg = pc::start();
    } else {
      pc_from_dbg = pc_from;
    }

    if (isa<CallInst>(I) && cast<CallInst>(I).getIntrinsicID() == Intrinsic::dbg_declare) {//declare will be replaced with addr in future revisions; so watch out for this change
      pc pc_from_for_dbg_parsing;
      if (pc_is_start) {
        pc_from_for_dbg_parsing = pc_from_dbg;
      } else {
        pc_from_for_dbg_parsing = pc(pc_from_dbg.get_type(), pc_from_dbg.get_index(), pc_from_dbg.get_subindex(), 0);
      }
      this->parse_dbg_declare_intrinsic(I, t, pc_from_for_dbg_parsing);
      continue;
    }

    if (isa<CallInst>(I) && cast<CallInst>(I).getIntrinsicID() == Intrinsic::dbg_value) {
      pc pc_from_for_dbg_parsing;
      if (pc_is_start) {
        pc_from_for_dbg_parsing = pc_from_dbg;
      } else {
        pc_from_for_dbg_parsing = pc(pc_from_dbg.get_type(), pc_from_dbg.get_index(), pc_from_dbg.get_subindex(), 0);
      }
      this->parse_dbg_value_intrinsic(I, t, pc_from_for_dbg_parsing);
      continue;
    }

    if (isa<CallInst>(I) && cast<CallInst>(I).getIntrinsicID() == Intrinsic::unseq_noalias) {
      //NOT_IMPLEMENTED();
      continue;
    }
    if (pc_is_start) {
      pc_is_start = false;
      //errs() << "setting pc_is_start to false\n";
    }

    dshared_ptr<tfg_node> from_node = make_dshared<tfg_node>(pc_from);
    if (!t.find_node(pc_from)) {
      t.add_node(from_node);
    }
    {
      DebugLoc const& dl = I.getDebugLoc();
      DILocation* diloc = dl.get();
      //cout << __func__ << " " << __LINE__ << ": pc_from = " << pc_from.to_string() << ", diloc = " << diloc << endl;
      if (diloc) {
        unsigned linenum = diloc->getLine();
        unsigned column_num = diloc->getColumn();
        t.tfg_llvm_add_pc_line_number_mapping(pc_from, linenum, column_num);
        //cout << __func__ << " " << __LINE__ << ": pc_from = " << pc_from.to_string() << ", linenum = " << linenum << endl;
      }
    }
    insn_id++;

    //exec(t.get_start_state(), I, from_node, B, F, insn_id, t, function_tfg_map, function_call_chain, eimap);
    exec(state(), I, from_node, B, F, insn_id, src_llvm_tfg, model_llvm_semantics, t, value_to_name_map/*, function_call_chain*/, eimap, scev_map, xml_output_format);
  }
}

llvm::BasicBlock const *
sym_exec_llvm::get_basic_block_for_pc(const llvm::Function& F, pc const &p)
{
  stringstream ss;
  //ss << string(F.getName()) << "." << p.get_index();
  ss << functionGetName(F) << "." << p.get_index();
  string id = ss.str();
  if (m_pc2bb_cache.count(id) > 0) {
    return m_pc2bb_cache.at(id);
  }
  //const BasicBlock* ret = NULL;
  for (const BasicBlock& B : F) {
    stringstream cur_ss;
    cur_ss << string(F.getName()) << "." << get_basicblock_index(B);
    string cur_id = cur_ss.str();
    m_pc2bb_cache[cur_id] = &B;
  }

  if (m_pc2bb_cache.count(id) > 0) {
    return m_pc2bb_cache.at(id);
  } else {
    /*if(get_basicblock_name(B) == p.get_id()) {
      ret = &B;
      return ret;
    }*/
    return NULL;
  }
}

dshared_ptr<tfg_node>
sym_exec_common::get_next_intermediate_subsubindex_pc_node(tfg &t, dshared_ptr<tfg_node> const &from_node)
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
    t.add_node(make_dshared<tfg_node>(ret));
  }
  return t.find_node(ret);
}

//template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
//pair<shared_ptr<tfg_node>, map<string, sort_ref>>
void
sym_exec_llvm::process_phi_nodes(tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map, const llvm::BasicBlock* B_from, const pc& pc_to, dshared_ptr<tfg_node> const &from_node, bool model_llvm_semantics, expr_ref edgecond, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, Instruction * I, const llvm::Function& F, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": searching for BB representing " << pc_to.to_string() << endl);
  const llvm::BasicBlock* B_to = 0;
  B_to = get_basic_block_for_pc(F, pc_to);
  assert(B_to);

  dshared_ptr<tfg_node> pc_to_phi_node = from_node;

  dshared_ptr<tfg_node> pc_to_phi_start_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
  auto e1 = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to_phi_start_node->get_pc(), state(), edgecond, assumes, te_comment));
  eimap.insert(make_pair(e1, I));
  t.add_edge(e1);
  pc_to_phi_node = pc_to_phi_start_node;


  DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": from_node = " << from_node->get_pc() << endl);
  DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": pc_to_phi_start_node = " << pc_to_phi_start_node->get_pc() << endl);

  //shared_ptr<tfg_node> pc_to_phi_start_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
  //auto e1 = make_dshared<tfg_edge>(pc_to_phi_node->get_pc(), pc_to_phi_start_node->get_pc(), t.get_start_state(), edgecond, t.get_start_state());
  //auto e1 = make_shared<tfg_edge>(pc_to_phi_node->get_pc(), pc_to_phi_start_node->get_pc(), t.get_start_state(), expr_true(m_ctx), t.get_start_state());
  //t.add_edge(e1);
  //pc_to_phi_node = pc_to_phi_start_node;


  DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": B_to->getInstList().size() = " << B_to->getInstList().size() << endl);
  map<string, sort_ref> changed_varnames;
  map<string, string> phi_tmpvarname;
  map<string, string> phi_tmpvarname_poison;
  int inum = 0;
  for (const llvm::Instruction& I : *B_to) {
    string varname;
    if (!instructionIsPhiNode(I, varname)) {
      continue;
    }
    inum++;
    DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": phi varname = " << varname << endl);

    state state_out;
    unordered_set<expr_ref> state_assumes;
    expr_ref val;
    tie(val, state_assumes) = phiInstructionGetIncomingBlockValue(I/*, t.get_start_state()*/, pc_to_phi_node/*, pc_to*/, B_from, F, model_llvm_semantics, t, value_to_name_map);
    DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": found phi instruction: from_node = " << from_node->get_pc() << ": pc_to_phi_node = " << pc_to_phi_node->get_pc() << endl);
    //expr_ref e = get_expr_adding_edges_for_intermediate_vals(*val/*phi->getIncomingValue(i)*/, "", t.get_start_state(), pc_to_phi_node, pc_to, *B_from, F, t);
    DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": after adding expr edges: from_node = " << from_node->get_pc() << ": pc_to_phi_node = " << pc_to_phi_node->get_pc() << ", val = " << expr_string(val) << endl);
    //string varname = get_value_name(*phi);
    changed_varnames.insert(make_pair(varname, val->get_sort()));
    phi_tmpvarname[varname] = varname + PHI_NODE_TMPVAR_SUFFIX + "." + get_basicblock_index(*B_from);
    state_set_expr(state_out, phi_tmpvarname.at(varname), val); //first update the tmpvars (do not want updates of one phi-node to influene the rhs of another phi-node.

    if (model_llvm_semantics) {
      //cout << _FNLN_ << ": val = " << expr_string(val) << endl;
      if (m_ctx->expr_is_input_expr(val)) {
        string const& val_key = m_ctx->get_key_from_input_expr(val)->get_str();
        //cout << _FNLN_ << ": val key = " << val_key << endl;
        string val_key_poison = get_poison_value_varname(val_key);
        //cout << _FNLN_ << ": val key poison = " << val_key_poison << endl;

        if (set_belongs(m_poison_varnames_seen, val_key_poison)) {
          phi_tmpvarname_poison[varname] = get_poison_value_varname(phi_tmpvarname.at(varname));
          m_poison_varnames_seen.insert(phi_tmpvarname_poison.at(varname));
          state_set_expr(state_out, phi_tmpvarname_poison.at(varname), m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + val_key_poison, m_ctx->mk_bool_sort()));
        }
      }
    }

    dshared_ptr<tfg_node> pc_to_phi_dst_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": from_node = " << from_node->get_pc() << ": pc_to_phi_node = " << pc_to_phi_node->get_pc() << ", pc_to_phi_dst_node = " << pc_to_phi_dst_node->get_pc() << endl);
    string const& bbindex = get_basicblock_index(*B_to);
    //bbl_order_descriptor_t const& bbo = this->m_bbl_order_map.at(mk_string_ref(bbindex));
    auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to_phi_dst_node->get_pc(), state_out, expr_true(m_ctx)/*, t.get_start_state()*/, state_assumes, phi_node_to_te_comment(/*bbo, */inum, I)));
    eimap.insert(make_pair(e, (Instruction *)&I));
    t.add_edge(e);
    DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": adding phi edge: " << e->to_string(/*&t.get_start_state()*/) << endl);
    pc_to_phi_node = pc_to_phi_dst_node;
  }
  //return make_pair(pc_to_phi_node, changed_varnames);

  auto pc_to_start2_phi_node = get_next_intermediate_subsubindex_pc_node(t, pc_to_phi_node);
  auto e2 = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to_start2_phi_node->get_pc(), state(), expr_true(m_ctx), {}, te_comment));
  eimap.insert(make_pair(e2, (Instruction*)&I));
  t.add_edge(e2);
  //cout << __func__ << " " << __LINE__ << ": t.incoming =\n" << t.incoming_sizes_to_string() << endl;
  pc_to_phi_node = pc_to_start2_phi_node;

  for (const auto &cvarname_sort : changed_varnames) {
    string const &cvarname = cvarname_sort.first;
    sort_ref const &cvarsort = cvarname_sort.second;
    string const& phi_tmpvarname_for_cvarname = phi_tmpvarname.at(cvarname);

    state state_out;
    state_set_expr(state_out, cvarname, m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + phi_tmpvarname_for_cvarname, cvarsort));

    if (model_llvm_semantics) {
      if (phi_tmpvarname_poison.count(cvarname)) {
        string cvarname_poison = get_poison_value_varname(cvarname);
        m_poison_varnames_seen.insert(cvarname_poison);
        state_set_expr(state_out, cvarname_poison, m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + phi_tmpvarname_poison.at(cvarname), m_ctx->mk_bool_sort())); //first update the tmpvars (do not want updates of one phi-node to influene the rhs of another phi-node.
      }
    }

    dshared_ptr<tfg_node> pc_to_phi_dst_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to_phi_dst_node->get_pc(), state_out, expr_true(m_ctx)/*, t.get_start_state()*/, {}, te_comment));
    eimap.insert(make_pair(e, (Instruction*)&I));
    t.add_edge(e);
    //cout << __func__ << " " << __LINE__ << ": t.incoming =\n" << t.incoming_sizes_to_string() << endl;
    DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": adding phi edge: " << e->to_string(/*&t.get_start_state()*/) << endl);
    pc_to_phi_node = pc_to_phi_dst_node;
  }
  auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to, state(), expr_true(m_ctx), {}, te_comment));
  DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": final edge: " << e->to_string() << endl);
  DYN_DEBUG2(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": all done" << endl);
  eimap.insert(make_pair(e, (Instruction *)&I));
  t.add_edge(e);
}

//template<typename FUNCTION, typename BASICBLOCK, typename INSTRUCTION>
//void
//sym_exec_llvm::process_phi_nodes_second_half(tfg &t, const llvm::BasicBlock* B_from, const pc& pc_to, shared_ptr<tfg_node> const &from_node, const llvm::Function& F, expr_ref edgecond, map<string, sort_ref> const &phi_regnames, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment, Instruction * I, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
//{
//  shared_ptr<tfg_node> pc_to_phi_node = from_node;
//
//  shared_ptr<tfg_node> pc_to_phi_start_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
//  auto e1 = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to_phi_start_node->get_pc(), state()/*t.get_start_state()*/, edgecond/*, t.get_start_state()*/, assumes, te_comment));
//  eimap.insert(make_pair(e1, I));
//  t.add_edge(e1);
//  //cout << __func__ << " " << __LINE__ << ": t.incoming =\n" << t.incoming_sizes_to_string() << endl;
//  pc_to_phi_node = pc_to_phi_start_node;
//
//  for (const auto &cvarname_sort : phi_regnames) {
//    string const &cvarname = cvarname_sort.first;
//    sort_ref const &cvarsort = cvarname_sort.second;
//    state state_out;
//    state_set_expr(state_out, cvarname, m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + cvarname + PHI_NODE_TMPVAR_SUFFIX, cvarsort));
//    shared_ptr<tfg_node> pc_to_phi_dst_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
//    auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to_phi_dst_node->get_pc(), state_out, expr_true(m_ctx)/*, t.get_start_state()*/, {}, te_comment));
//    eimap.insert(make_pair(e, I));
//    t.add_edge(e);
//    //cout << __func__ << " " << __LINE__ << ": t.incoming =\n" << t.incoming_sizes_to_string() << endl;
//    DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": adding phi edge: " << e->to_string(/*&t.get_start_state()*/) << endl);
//    pc_to_phi_node = pc_to_phi_dst_node;
//  }
//  DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << " " << get_timestamp(as1, sizeof as1) << ": all done" << endl);
//  //auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to, t.get_start_state(), expr_true(m_ctx)/*, t.get_start_state()*/, {}, te_comment));
//  auto e = mk_tfg_edge(mk_itfg_edge(pc_to_phi_node->get_pc(), pc_to, state()/*t.get_start_state()*/, expr_true(m_ctx)/*, t.get_start_state()*/, {}, te_comment));
//  eimap.insert(make_pair(e, I));
//  t.add_edge(e);
//  //cout << __func__ << " " << __LINE__ << ": t.incoming =\n" << t.incoming_sizes_to_string() << endl;
//}



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
  autostop_timer func_timer(__func__);
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

string
sym_exec_llvm::get_cur_rounding_mode_varname() const
{
  return m_ctx->get_cur_rounding_mode_varname_for_prefix(m_srcdst_keyword + ".");
}

expr_ref
sym_exec_llvm::get_cur_rounding_mode_var() const
{
  return m_ctx->mk_var(string(G_INPUT_KEYWORD ".") + this->get_cur_rounding_mode_varname(), m_rounding_mode_at_start_pc->get_sort());
}

//void
//sym_exec_llvm::sym_exec_preprocess_tfg(string const &name, tfg_llvm_t& t_src, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> &function_tfg_map, list<string> const& sorted_bbl_indices, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, context::xml_output_format_t xml_output_format)
//{
//  autostop_timer func_timer(__func__);
//  DYN_DEBUG(llvm2tfg, cout << _FNLN_ << ": name = " << name << endl);
//  //context* ctx = this->get_context();
//  //consts_struct_t &cs = ctx->get_consts_struct();
//  map<allocsite_t, graph_local_t> const& local_refs = this->get_local_refs();
//
//  //pc start_pc = this->get_start_pc();
//  pc start_pc = sym_exec_llvm::get_start_pc(m_function);
//  t_src.add_extra_node_at_start_pc(start_pc);
//
//  unordered_set<expr_ref> const& arg_assumes = this->gen_arg_assumes();
//  t_src.add_assumes_to_start_edge(arg_assumes);
//
//  t_src.tfg_initialize_rounding_mode_on_start_edge(this->get_cur_rounding_mode_varname(), m_rounding_mode_at_start_pc);
//
//  t_src.set_symbol_map_for_touched_symbols(*m_symbol_map, m_touched_symbols);
//  t_src.set_string_contents_for_touched_symbols_at_zero_offset(*m_string_contents, m_touched_symbols);
//  t_src.remove_function_name_from_symbols(name);
//  t_src.populate_exit_return_values_for_llvm_method();
//  t_src.canonicalize_llvm_nextpcs(src_llvm_tfg);
//  t_src.tfg_llvm_interpret_intrinsic_fcalls();
//
//  map<nextpc_id_t, callee_summary_t> nextpc_id_csum = sym_exec_llvm::get_callee_summaries_for_tfg(t_src.get_nextpc_map(), m_callee_summaries);
//  t_src.set_callee_summaries(nextpc_id_csum);
//
//  ASSERT(t_src.get_locals_map().size() == 0);
//  t_src.set_locals_map(local_refs);
//  t_src.tfg_initialize_uninit_nonce_on_start_edge(map_get_keys(local_refs), m_srcdst_keyword);
//
//  t_src.tfg_llvm_add_start_pc_preconditions(m_srcdst_keyword);
//
//  //DYN_DEBUG(llvm2tfg, cout << _FNLN_ << ": name = " << name << ": calling tfg_preprocess()\n");
//  //t_src.tfg_preprocess(false, src_llvm_tfg, sorted_bbl_indices, {}, xml_output_format);
//  //DYN_DEBUG(llvm2tfg, cout << _FNLN_ << ": name = " << name << ": done tfg_preprocess().\n" << endl);
//  //DYN_DEBUG2(llvm2tfg, cout << _FNLN_ << ": name = " << name << ": after tfg_preprocess(), TFG:\n" << t_src.graph_to_string() << endl);
//  //cout << _FNLN_ << ": " << get_timestamp(as1, sizeof as1) << ": returning\n";
//}

bool
sym_exec_common::update_function_call_args_and_retvals_with_atlocals(dshared_ptr<tfg> t_src)
{
  return false; //NOT_IMPLEMENTED();
}

unordered_set<expr_ref>
sym_exec_llvm::gen_arg_assumes() const
{
  unordered_set<expr_ref> arg_assumes;
  for (const auto& arg : m_function.args()) {
    pair<argnum_t, expr_ref> const &a = m_arguments.at(get_value_name(arg));
    string Elname = get_value_name(arg)/* + SRC_INPUT_ARG_NAME_SUFFIX*/;
    Type *ElTy = arg.getType();
    unordered_set_union(arg_assumes, gen_align_assumes(Elname, ElTy, a.second->get_sort()));
  }
  return arg_assumes;
}

//dshared_ptr<tfg_llvm_t>
//sym_exec_llvm::get_preprocessed_tfg_common(string const &name, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> &function_tfg_map, map<llvm_value_id_t, string_ref>* value_to_name_map, set<string> function_call_chain, list<string> const& sorted_bbl_indices, bool DisableModelingOfUninitVarUB, map<string, value_scev_map_t> const& scev_map, context::xml_output_format_t xml_output_format)
//{
//  autostop_timer func_timer(__func__);
//  DYN_DEBUG(llvm2tfg,
//    errs() << "Doing: " << name << "\n";
//    errs().flush();
//    outs() << "Doing: " << name << "\n";
//    outs().flush();
//  );
//  function_call_chain.insert(name);
//  map<shared_ptr<tfg_edge const>, Instruction *> eimap;
//  DYN_DEBUG(llvm2tfg, outs() << _FNLN_ << ": " << get_timestamp(as1, sizeof as1) << ": Calling get_tfg() on " << name << ".\n");
//  dshared_ptr<tfg_llvm_t> t_src = this->get_tfg(src_llvm_tfg, &function_tfg_map, value_to_name_map, &function_call_chain, eimap, scev_map, xml_output_format);
//  DYN_DEBUG(llvm2tfg, outs() << _FNLN_ << ": " << get_timestamp(as1, sizeof as1) << ": Done get_tfg() on " << name << ". Calling sym_exec_preprocess_tfg()\n");
//  this->sym_exec_preprocess_tfg(name, *t_src, function_tfg_map, sorted_bbl_indices, src_llvm_tfg, xml_output_format);
//  //cout << _FNLN_ << ": " << get_timestamp(as1, sizeof as1) << ": returned from sym_exec_preprocess_tfg\n";
//  if (scev_map.count(name)) {
//    this->sym_exec_populate_tfg_scev_map(*t_src, scev_map.at(name));
//  }
//  DYN_DEBUG(llvm2tfg,
//    errs() << "Doing done: " << name << "\n";
//    errs().flush();
//    outs() << "Doing done: " << name << "\n";
//    outs().flush();
//  );
//
//  if (!DisableModelingOfUninitVarUB) {
//    //map<pc, map<graph_loc_id_t, bool>> init_status;
//    list<pc> exit_pcs;
//    //t_src->populate_loc_definedness();
//    //init_status = t_src->determine_initialization_status_for_locs();
//    t_src->get_exit_pcs(exit_pcs);
//    for (auto exit_pc : exit_pcs) {
//      if (t_src->return_register_is_uninit_at_exit(G_LLVM_RETURN_REGISTER_NAME, exit_pc/*, init_status*/)) {
//        t_src->eliminate_return_reg_at_exit(G_LLVM_RETURN_REGISTER_NAME, exit_pc);
//      }
//    }
//  }
//  //cout << _FNLN_ << ": " << get_timestamp(as1, sizeof as1) << ": returning\n";
//  return t_src;
//}

//dshared_ptr<tfg_llvm_t>
//sym_exec_llvm::get_preprocessed_tfg(Function &f, Module const *M, string const &name, context *ctx, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> &function_tfg_map, map<llvm_value_id_t, string_ref>* value_to_name_map, set<string> function_call_chain, bool gen_callee_summary, bool DisableModelingOfUninitVarUB, map<string, value_scev_map_t> const& scev_map, string const& srcdst_keyword, context::xml_output_format_t xml_output_format)
//{
//  return se.get_preprocessed_tfg_common(name, src_llvm_tfg, function_tfg_map, value_to_name_map, function_call_chain, sorted_bbl_indices, DisableModelingOfUninitVarUB, scev_map, xml_output_format);
//}
//
//dshared_ptr<tfg>
//sym_exec_llvm::get_preprocessed_tfg_for_machine_function(MachineFunction const &mf, Function const &f, Module const *M, string const &name, context *ctx, list<pair<string, unsigned>> const &fun_names, graph_symbol_map_t const &symbol_map, map<pair<symbol_id_t, offset_t>, vector<char>> const &string_contents, consts_struct_t &cs, map<string, pair<callee_summary_t, dshared_ptr<tfg>>> &function_tfg_map, set<string> function_call_chain, bool gen_callee_summary, bool DisableModelingOfUninitVarUB)
//{
//  NOT_IMPLEMENTED();
//  //sym_exec_mir se(ctx, cs, M, f, mf, fun_names, symbol_map, string_contents, gen_callee_summary, BYTE_LEN, DWORD_LEN);
//  //return get_preprocessed_tfg_common(se, name, function_tfg_map, function_call_chain, DisableModelingOfUninitVarUB);
//}


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

symbol_id_t
sym_exec_common::get_symbol_id_for_name(string const& name, dshared_ptr<tfg_llvm_t const> src_llvm_tfg, symbol_id_t input_symbol_id)
{
  if (!src_llvm_tfg) {
    return input_symbol_id;
  }
  graph_symbol_map_t const& symbol_map = src_llvm_tfg->get_symbol_map();
  map<symbol_id_t, graph_symbol_t> const& smap = symbol_map.get_map();

  symbol_id_t max_sym_id = -1;
  for (auto const& id_sym : smap) {
    symbol_id_t sym_id = id_sym.first;
    graph_symbol_t const& sym = id_sym.second;
    if (sym.get_name()->get_str() == name) {
      return sym_id;
    }
    max_sym_id = max(sym_id, max_sym_id);
  }
  if (symbol_map.count(input_symbol_id)) {
    return max_sym_id + 1;
  } else {
    return input_symbol_id;
  }
}

pair<graph_symbol_map_t, map<pair<symbol_id_t, offset_t>, vector<char>>>
sym_exec_common::get_symbol_map_and_string_contents(Module const *M, list<pair<string, unsigned>> const &fun_names, dshared_ptr<tfg_llvm_t const> src_llvm_tfg)
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
    string name = sym_exec_llvm::get_value_name_using_srcdst_keyword(g, G_SRC_KEYWORD);
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
    symbol_id = get_symbol_id_for_name(name, src_llvm_tfg, symbol_id);
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
    symbol_id = get_symbol_id_for_name(fun_name.first, src_llvm_tfg, symbol_id);
    smap.insert(make_pair(symbol_id, graph_symbol_t(mk_string_ref(fun_name.first), fun_name.second, ALIGNMENT_FOR_FUNCTION_SYMBOL, false)));
    symbol_id++;
  }
  ASSERT(symbol_id < NUM_CANON_SYMBOLS);
  return make_pair(smap, scontents);
}

graph_symbol_map_t
sym_exec_common::get_symbol_map(Module const *M, dshared_ptr<tfg_llvm_t const> src_llvm_tfg)
{
  list<pair<string, unsigned>> fun_names = get_fun_names(M);

  pair<graph_symbol_map_t, map<pair<symbol_id_t, offset_t>, vector<char>>> pr = get_symbol_map_and_string_contents(M, fun_names, src_llvm_tfg);
  return pr.first;
}

map<pair<symbol_id_t, offset_t>, vector<char>>
sym_exec_common::get_string_contents(Module const *M, dshared_ptr<tfg_llvm_t const> src_llvm_tfg)
{
  list<pair<string, unsigned>> fun_names = get_fun_names(M);

  pair<graph_symbol_map_t, map<pair<symbol_id_t, offset_t>, vector<char>>> pr = get_symbol_map_and_string_contents(M, fun_names, src_llvm_tfg);
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
sym_exec_llvm::get_start_pc(Function const& f)
{
  return get_pc_from_bbindex_and_insn_id(get_basicblock_index(*f.begin()), 0);
}

//void
//sym_exec_mir::populate_state_template_for_mf(MachineFunction const &mf)
//{
//  NOT_REACHED();
//#if 0
//  argnum_t argnum = 0;
//  MachineFrameInfo const &MFI = mf.getFrameInfo();
//  int FI = MFI.getObjectIndexBegin();
//  for (; MFI.isFixedObjectIndex(FI); ++FI) {
//    //int64_t ObjBegin = MFI.getObjectOffset(FI);
//    //int64_t ObjEnd = ObjBegin + MFI.getObjectSize(FI);
//    //if (ObjBegin <= PartBegin && PartEnd <= ObjEnd)
//    //  break;
//    stringstream ss;
//    ss << MIR_FIXED_STACK_SLOT_PREFIX "." << argnum;
//    string argname = ss.str();
//    sort_ref s = m_ctx->mk_bv_sort(DWORD_LEN);
//    expr_ref argvar = m_ctx->mk_var(string(G_INPUT_KEYWORD) + "." + argname, s);
//    m_arguments[argname] = make_pair(argnum, argvar);
//    //cout << __func__ << " " << __LINE__ << ": adding " << argname << endl;
//    m_state_templ.push_back({argname, s});
//    argnum++;
//  }
//
//  MachineConstantPool const *MCP = mf.getConstantPool();
//  for (unsigned i = 0; i != MCP->getConstants().size(); i++) {
//    NOT_IMPLEMENTED();
//  }
//  TargetInstrInfo const &TII = *mf.getSubtarget().getInstrInfo();
//  const MachineRegisterInfo &MRI = mf.getRegInfo();
//
//
//  for (const MachineBasicBlock& B : mf) {
//    m_basicblock_idx_map[get_machine_basicblock_name(B)] = B.getNumber() + 1;  //bbnum == 0 is reserved
//    //bbnum++;
//    int insn_id = 0;
//    for(const MachineInstr& I : B)
//    {
//      if (I.getOpcode() != TargetOpcode::PHI) {//!isa<const PHINode>(I)
//        insn_id++;
//      }
//      if (TII.getName(I.getOpcode()) != MIR_RET_NODE_OPCODE) {//isa<ReturnInst>(I)
//      //if (I.getOpcode() == X86::RET)
//        m_ret_reg = G_LLVM_RETURN_REGISTER_NAME;
//        //return_reg_sort = get_value_type(*ret_val);
//        sort_ref return_reg_sort;
//        return_reg_sort = m_ctx->mk_bv_sort(DWORD_LEN);
//        //cout << __func__ << " " << __LINE__ << ": adding " << m_ret_reg << endl;
//        m_state_templ.push_back({m_ret_reg, return_reg_sort});
//        continue;
//      }
//      /*if (isa<SwitchInst>(I)) { //XXX: TODO
//        const SwitchInst* SI =  cast<const SwitchInst>(&I);
//        size_t varnum = 0;
//        for (SwitchInst::ConstCaseIt i = SI->case_begin(), e = SI->case_end(); i != e; ++i) {
//          stringstream ss;
//          ss << LLVM_SWITCH_TMPVAR_PREFIX << varnum;
//          varnum++;
//          m_state_templ.push_back({ss.str(), m_ctx->mk_bool_sort()});
//        }
//        continue;
//      }*/
//
//      for (unsigned StartOp = 0; StartOp < I.getNumOperands(); StartOp++) {
//        MachineOperand const &MO = I.getOperand(StartOp);
//        if (!MO.isReg() || !MO.isDef() || MO.isImplicit()) {
//          continue;
//        }
//        unsigned Reg = MO.getReg();
//        if (TargetRegisterInfo::isVirtualRegister(Reg)) {
//          string name = MRI.getVRegName(Reg);
//          if (name != "") {
//            name = string("%") + name;
//          } else {
//            stringstream ss;
//            ss << "%" << TargetRegisterInfo::virtReg2Index(Reg);
//            name = ss.str();
//          }
//          LLT type = MRI.getType(Reg);
//          //sort_ref s = get_value_type(I);
//          sort_ref s = m_ctx->mk_bv_sort(DWORD_LEN);
//          //cout << __func__ << " " << __LINE__ << ": adding " << name << endl;
//          m_state_templ.push_back({name, s});
//          //if (TII.getName(I.getOpcode()) != MIR_PHI_NODE_OPCODE)
//          if (I.getOpcode() != TargetOpcode::PHI) {
//            m_state_templ.push_back({name + PHI_NODE_TMPVAR_SUFFIX, s});
//          }
//        }
//      }
//    }
//  }
//  populate_state_template_common();
//#endif
//}

//void sym_exec_mir::add_edges_for_mir(const llvm::MachineBasicBlock& B, tfg& t, const llvm::MachineFunction& F, map<string, pair<callee_summary_t, unique_ptr<tfg>>> &function_tfg_map, set<string> const &function_call_chain)
//{
//  size_t insn_id = 0;
//
//  TargetInstrInfo const &TII = *m_machine_function.getSubtarget().getInstrInfo();
//  for (const MachineInstr& I : B) {
//    if (I.getOpcode() == TargetOpcode::PHI) {
//      continue;
//    }
//
//    pc pc_from = get_pc_from_bbindex_and_insn_id(get_basicblock_index(B), insn_id);
//
//    //state state_start;
//    //get_state_template(pc_from, state_start);
//
//    shared_ptr<tfg_node> from_node((tfg_node*)(new tfg_node(pc_from)));
//    if (!t.find_node(pc_from)) {
//      t.add_node(from_node);
//    }
//    //state state_to(t.get_start_state());
//    insn_id++;
//
//    //errs() << "from state:\n" << state_to.to_string();
//    //exec(state_to, I, state_to, cfts, expand_switch_flag, assumes, F.getName(), t, function_tfg_map);
//    //errs() << "to state:\n" << state_to.to_string();
//
//    exec_mir(/*t.get_start_state()*/state(), I/*, state_to, cfts, expand_switch_flag, assumes*/, from_node, B/*, F*/, insn_id, t, function_tfg_map, function_call_chain);
//
//    //t.add_assumes(pc_from, assumes);
//    //errs() << __func__ << " " << __LINE__ << ": insn_id = " << insn_id << "\n";
//  }
//}

void
sym_exec_common::get_tfg_common(tfg &t)
{
  map<string_ref, graph_arg_t> arg_exprs;
  //unordered_set<predicate> assumes;
  for (const auto& arg : m_arguments) {
    pair<argnum_t, expr_ref> const &a = arg.second;
    string argname = graph_arg_regs_t::get_argname_from_argnum(a.first);

    allocsite_t allocsite = m_ctx->is_vararg_local_expr(a.second) ? graph_locals_map_t::vararg_local_id()
                                                                  : allocsite_t::allocsite_arg(a.first);
    allocstack_t allocstack = allocstack_t::allocstack_singleton(t.get_function_name()->get_str(), allocsite);
    stringstream ss;
    ss << string(G_INPUT_KEYWORD ".") << m_srcdst_keyword << "." << G_LOCAL_KEYWORD << "." << allocstack.allocstack_to_string();
    expr_ref arg_addr = m_ctx->mk_var(ss.str(), m_ctx->get_addr_sort());
    arg_exprs.insert(make_pair(mk_string_ref(argname), graph_arg_t(arg_addr, a.second)));
  }
  //t->add_assumes(pc::start(), assumes);

  state start_state;
  get_state_template(pc::start(), start_state);
  //arg_exprs.push_back(t.find_entry_node()->get_state().get_expr(m_mem_reg));
  t.set_argument_regs(arg_exprs);
  get_state_template(pc::start(), start_state);
  t.set_start_state(start_state);
}

//unique_ptr<tfg_llvm_t>
//sym_exec_mir::get_tfg(map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> *function_tfg_map, set<string> const *function_call_chain, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
//{
//  NOT_REACHED();
///*
//  llvm::MachineFunction const &F = m_machine_function;
//  populate_state_template_for_mf(m_machine_function);
//  state start_state;
//  get_state_template(pc::start(), start_state);
//  unique_ptr<tfg> t = make_unique<tfg>("mir", m_ctx);
//  t->set_start_state(start_state);
//  for(const MachineBasicBlock& B : F) {
//    this->add_edges_for_mir(B, *t, F, *function_tfg_map, *function_call_chain);
//  }
//
//  this->get_tfg_common(*t);
//  return t;
//*/
//}

//pc
//sym_exec_mir::get_start_pc() const
//{
//  return this->get_pc_from_bbindex_and_insn_id(get_basicblock_index(*m_machine_function.begin()), 0);
//}

//string
//sym_exec_mir::get_machine_basicblock_name(MachineBasicBlock const &mb) const
//{
//  stringstream ss;
//  ss << "%bb." << mb.getNumber() + 1;
//  return ss.str();
//}

pair<expr_ref, unordered_set<expr_ref>>
sym_exec_llvm::phiInstructionGetIncomingBlockValue(llvm::Instruction const &I, dshared_ptr<tfg_node> &pc_to_phi_node/*, pc const &pc_to*/, llvm::BasicBlock const *B_from, llvm::Function const &F, bool model_llvm_semantics, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map)
{
  const PHINode* phi = dyn_cast<const PHINode>(&I);
  ASSERT(phi);

  for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
    if (phi->getIncomingBlock(i) != B_from) {
      continue;
    }
    expr_ref e;
    unordered_set<expr_ref> assumes;
    tie(e, assumes) = get_expr_adding_edges_for_intermediate_vals(*phi->getIncomingValue(i), "", state()/*t.get_start_state()*/, assumes, pc_to_phi_node, model_llvm_semantics, t, value_to_name_map);
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

//void
//sym_exec_llvm::populate_bbl_order_map()
//{
//  llvm::Function const &F = m_function;
//  int bblnum = 0;
//  for(BasicBlock const& B : F) {
//    string const& bbindex = get_basicblock_index(B);
//    this->m_bbl_order_map.insert(make_pair(mk_string_ref(bbindex), bbl_order_descriptor_t(bblnum, bbindex)));
//    bblnum++;
//  }
//}

string
sym_exec_llvm::get_basicblock_index(const llvm::BasicBlock &v)
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

struct FunctionPassPopulateTfgScev : public FunctionPass {
  static char ID;
  const PassInfo *PassInfo_v;
  const class PassInfo *PassInfo_LI;
  //raw_ostream &Out;
  map<string, value_scev_map_t>& scev_map;
  string const& m_srcdst_keyword;
  size_t m_word_length;
  //map<Function const*, LoopInfo const*>& loopinfo_map;
  std::string PassName;

  FunctionPassPopulateTfgScev(class PassInfo const *PI, class PassInfo const* PI_loopinfo, map<string, value_scev_map_t>& scev_map, string const& srcdst_keyword, size_t word_length)
      : FunctionPass(ID), PassInfo_v(PI), PassInfo_LI(PI_loopinfo), scev_map(scev_map), m_srcdst_keyword(srcdst_keyword), m_word_length(word_length) {
    PassName = "FunctionPass PopulateTfgScev";
  }

  bool runOnFunction(Function& F) override {
    // Get pass and populate scev ...
    ScalarEvolutionWrapperPass& P = getAnalysisID<ScalarEvolutionWrapperPass>(PassInfo_v->getTypeInfo());
    LoopInfoWrapperPass& LP = getAnalysisID<LoopInfoWrapperPass>(PassInfo_LI->getTypeInfo());
    LoopInfo& LI = LP.getLoopInfo();
    ScalarEvolution& SE = P.getSE();
    string fname = F.getName().data();

    //errs() << "Printing analysis '" << PassInfo_LI->getPassName() << "' for "
    //    << "function: '" << F.getName() << "':\n";
    //LI->print(errs());

    //errs() << "Printing analysis '" << PassInfo->getPassName() << "' for "
    //    << "function: '" << F.getName() << "':\n";
    //SE->print(errs());

    for (BasicBlock& B : F) {
      for(Instruction& I : B) {
        if (SE.isSCEVable(I.getType()) && !isa<CmpInst>(I)) {
          string iname = sym_exec_llvm::get_value_name_using_srcdst_keyword(I, m_srcdst_keyword);
          scev_toplevel_t<pc> st = sym_exec_llvm::get_scev_toplevel(I, &SE, &LI, m_srcdst_keyword, m_word_length);
          scev_map[fname].insert(make_pair(iname, st));
        }
      }
    }
    //scev_map.insert(make_pair((Function const*)&F, SE));
    //loopinfo_map.insert(make_pair((Function const*)&F, LI));
    return false;
  }

  StringRef getPassName() const override { return PassName; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequiredID(PassInfo_v->getTypeInfo());
    AU.addRequiredID(PassInfo_LI->getTypeInfo());
    AU.setPreservesAll();
  }
};

char FunctionPassPopulateTfgScev::ID = 0;

//static FunctionPass *
//createFunctionPassPopulateTfgScev(const PassInfo *PI, map<Function const*, ScalarEvolution const*>& scev_map) {
//  return new FunctionPassPopulateTfgScev(PI, scev_map);
//}

map<string, value_scev_map_t>
sym_exec_llvm::sym_exec_populate_potential_scev_relations(Module* M, string const& srcdst_keyword)
{
  PassInfo const* PI = PassRegistry::getPassRegistry()->getPassInfo(StringRef("scalar-evolution"));
  PassInfo const* PI_loopinfo = PassRegistry::getPassRegistry()->getPassInfo(StringRef("loops"));
  //errs() << __func__ << " " << __LINE__ << ": PI = " << PI << "\n";
  Pass *P, *P_loopinfo;
  if (PI->getNormalCtor()) {
    P = PI->getNormalCtor()();
  } else {
    NOT_REACHED();
  }
  if (PI_loopinfo->getNormalCtor()) {
    P_loopinfo = PI_loopinfo->getNormalCtor()();
  } else {
    NOT_REACHED();
  }

  //std::error_code EC;
  //auto Out = std::make_unique<ToolOutputFile>("-", EC,
  //                                       sys::fs::OF_None);

  map<string, value_scev_map_t> scev_map;
  //map<Function const*, LoopInfo const*> loopinfo_map;
  legacy::PassManager Passes;
  Passes.add(P);
  Passes.add(P_loopinfo);
  //Passes.add(createRegionPassPrinter(PI, Out->os()));
  Passes.add(new FunctionPassPopulateTfgScev(PI, PI_loopinfo, scev_map, srcdst_keyword, M->getDataLayout().getPointerSize() * BYTE_LEN));

  Passes.run(*M);
  return scev_map;
}

dshared_ptr<ftmap_t>
sym_exec_llvm::get_function_tfg_map(Module* M, set<string> FunNamesVec/*, bool DisableModelingOfUninitVarUB*/, context* ctx, dshared_ptr<llptfg_t const> const& src_llptfg, bool gen_scev, bool model_llvm_semantics, map<llvm_value_id_t, string_ref>* value_to_name_map, context::xml_output_format_t xml_output_format)
{
  //map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>> function_tfg_map;
  map<call_context_ref, dshared_ptr<tfg_ssa_t>> function_tfg_map;

  DYN_DEBUG3(llvm2tfg,
    cout.flush();
    M->print(dbgs(), nullptr, true);
    dbgs().flush();
  );
  string srcdst_keyword = src_llptfg ? G_DST_KEYWORD : G_SRC_KEYWORD;

  map<string, value_scev_map_t> scev_map;
  //map<Function const*, LoopInfo const*> loopinfo_map;
  if (gen_scev) {
    scev_map = sym_exec_populate_potential_scev_relations(M, srcdst_keyword);
  }
  //for (auto const& li : loopinfo_map) {
  //  errs() << "loop info map for " << li.first->getName() << "\n";
  //  li.second->print(errs());
  //}
  //for (auto const& sc : scev_map) {
  //  errs() << "scev map for " << sc.first->getName() << "\n";
  //  sc.second->print(errs());
  //}
  for (Function& f : *M) {
    if (   FunNamesVec.size() != 0
        //&& (FunNamesVec.size() != 1 || *FunNamesVec.begin() != "ALL")
        && !set_belongs(FunNamesVec, string(f.getName().data()))) {
      DYN_DEBUG(llvm2tfg, errs() << "Ignoring " << string(f.getName().data()) << "\n");
      continue;
    }
    if (sym_exec_common::get_num_insn(f) == 0) {
      continue;
    }

    string fname = f.getName().str();
    autostop_timer total_timer(string(__func__));
    autostop_timer func_timer(string(__func__) + "." + fname);

    ASSERT(!function_tfg_map.count(call_context_t::empty_call_context(fname)));

    dshared_ptr<tfg_llvm_t const> src_llvm_tfg = dshared_ptr<tfg_llvm_t const>::dshared_nullptr();
    if (src_llptfg) {
      map<call_context_ref, dshared_ptr<tfg_ssa_t>> const& src_fname_tfg_map = src_llptfg->get_function_tfg_map();
      if (src_fname_tfg_map.count(call_context_t::empty_call_context(fname))) {
        src_llvm_tfg = dynamic_pointer_cast<tfg_llvm_t>(src_fname_tfg_map.at(call_context_t::empty_call_context(fname))->get_ssa_tfg());
        ASSERT(src_llvm_tfg);
      }
    }

    {
      stringstream ss;
      ss << "Converting LLVM IR bitcode to Transfer Function Graph (TFG) for function " << fname;
      MSG(ss.str().c_str());
    }
    //bool gen_callee_summary = (FunNamesVec.size() == 0);
    //set<string> function_call_chain;

    DYN_DEBUG(llvm2tfg, cout << __func__ << " " << __LINE__ << ": Doing " << fname << endl; cout.flush());
    map<shared_ptr<tfg_edge const>, Instruction *> eimap;

    dshared_ptr<tfg_llvm_t> t_src = sym_exec_llvm::get_tfg(f, M, fname, ctx, src_llvm_tfg, model_llvm_semantics, value_to_name_map, eimap, scev_map, srcdst_keyword, xml_output_format);

    //dshared_ptr<tfg_ssa_t> t_src_ssa = tfg_ssa_t::tfg_ssa_construct_from_non_ssa_tfg(t_src, dshared_ptr<tfg const>::dshared_nullptr());
    //XXX: hack begin
    t_src->set_graph_as_ssa();
    dshared_ptr<tfg_ssa_t> t_src_ssa = tfg_ssa_t::tfg_ssa_construct_from_an_already_ssa_tfg(t_src/*, dshared_ptr<tfg const>::dshared_nullptr()*/);
    //XXX: hack end

    //dshared_ptr<tfg_llvm_t> t_src = sym_exec_llvm::get_preprocessed_tfg(f, M, fname, ctx, src_llvm_tfg, function_tfg_map, value_to_name_map, function_call_chain, gen_callee_summary, DisableModelingOfUninitVarUB, scev_map, srcdst_keyword, xml_output_format);

    //callee_summary_t csum = t_src->get_summary_for_calling_functions();
    //function_tfg_map.insert(make_pair(fname, make_pair(csum, std::move(t_src))));
    function_tfg_map.insert(make_pair(call_context_t::empty_call_context(fname), t_src_ssa));
  }

  DYN_DEBUG(get_function_tfg_map_debug,
    for (auto const& p : function_tfg_map) {
      cout << __func__ << " " << __LINE__ << ": TFG for " << p.first << ":\n";
      p.second->graph_to_stream(cout); cout << endl;
    }
  );
  return make_dshared<ftmap_t>(function_tfg_map);
}

llvm_value_id_t
sym_exec_llvm::get_llvm_value_id_for_value(Value const* v)
{
  ASSERT(v);
  string valname;
  if (Instruction const* I = (Instruction const*)dyn_cast<Instruction>(v)) {
    raw_string_ostream ss(valname);
    ss << *I;
  } else if (ConstantExpr const* ce = (ConstantExpr const*)dyn_cast<const ConstantExpr>(v)) {
    Instruction* I = ce->getAsInstruction();
    raw_string_ostream ss(valname);
    ss << *I;
    I->deleteValue();
  } else {
    valname = sym_exec_common::get_value_name_using_srcdst_keyword(*v, G_SRC_KEYWORD);
  }
  llvm::Function const* F = sym_exec_llvm::getParent(v);
  string fname = F ? F->getName().str() : FNAME_GLOBAL_SPACE;
  return llvm_value_id_t(fname, valname);
}

const Function *
sym_exec_llvm::getParent(const Value *V) {
  if (const Instruction *inst = dyn_cast<Instruction>(V)) {
    if (!inst->getParent()) {
      return nullptr;
    }
    return inst->getParent()->getParent();
  }

  if (const Argument *arg = dyn_cast<Argument>(V))
    return arg->getParent();

  return nullptr;
}

//string
//sym_exec_llvm::get_poison_value_name(Value const& I) const
//{
//  string base_name =  get_value_name(I);
//  stringstream ss;
//  ss << base_name << ".poison";
//  return ss.str();
//}
//
//vector<expr_ref>
//sym_exec_llvm::get_poison_args(const llvm::Instruction& I/*, string vname*/, const state& st, unordered_set<expr_ref> const& state_assumes, dshared_ptr<tfg_node> &from_node/*, pc const &pc_to, llvm::BasicBlock const &B, llvm::Function const &F*/, tfg &t, map<llvm_value_id_t, string_ref>* value_to_name_map)
//{
//  vector<expr_ref> args;
//  for (unsigned i = 0; i < I.getNumOperands(); ++i) {
//    const auto& v = *I.getOperand(i);
//    if (isa<const Instruction>(&v)) {
//      vector<sort_ref> sv = get_value_type_vec(v, m_module->getDataLayout());
//      auto poison_name = get_poison_value_name(v);
//      if (m_poison_set.find(poison_name) != m_poison_set.end()) {
//        args.push_back(state_get_expr(st, get_poison_value_name(v), m_ctx->mk_bool_sort()));
//      }
//      else {
//        args.push_back(expr_false(m_ctx));
//      }
//    }
//    else {
//      // We need this because sometimes we need to check only some args for poison possibility
//      args.push_back(expr_false(m_ctx));
//    }
//  }
//  return args;
//}


string
sym_exec_llvm::get_poison_value_varname(string const& varname) const
{
  stringstream ss;
  ss << varname << ".poison";
  return ss.str();
}


expr_ref
sym_exec_llvm::get_poison_value_var(string const& varname) const
{
  string poison_varname = get_poison_value_varname(varname);
  return get_input_expr(poison_varname, m_ctx->mk_bool_sort());
}

void
sym_exec_llvm::transfer_poison_values(string const& varname, expr_ref const& e, unordered_set<expr_ref>& state_assumes, dshared_ptr<tfg_node>& from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map)
{
  if (!model_llvm_semantics) {
    return;
  }

  expr_list vars = m_ctx->expr_get_vars(e);
  set<expr_ref> poison_vars;
  for (auto const& v : vars) {
    string varname = v->get_name()->get_str();
    if (!string_has_prefix(varname, G_INPUT_KEYWORD ".")) {
      continue;
    }
    varname = varname.substr(strlen(G_INPUT_KEYWORD "."));
    string poison_varname = get_poison_value_varname(varname);
    if (!set_belongs(m_poison_varnames_seen, poison_varname)) {
      continue;
    }
    expr_ref poison_var = get_input_expr(poison_varname, m_ctx->mk_bool_sort());
    poison_vars.insert(poison_var);
  }

  expr_ref poison_expr;
  for (auto const& poison_var : poison_vars) {
    if (varname == "") {
      state_assumes.insert(m_ctx->mk_not(poison_var));
    } else {
      if (poison_expr) {
        poison_expr = expr_or(poison_expr, poison_var);
      } else {
        poison_expr = poison_var;
      }
    }
  }

  if (varname != "" && poison_expr) {
    string poison_varname = get_poison_value_varname(varname);
    //if (!set_belongs(m_poison_varnames_seen, varname)) {
    //  expr_ref poison_var = get_input_expr(poison_varname, m_ctx->mk_bool_sort());
    //  poison_expr = expr_or(poison_var, poison_expr);
    //}
    m_poison_varnames_seen.insert(poison_varname);

    state state_to_intermediate_val;
    state_set_expr(state_to_intermediate_val, poison_varname, poison_expr);
    dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx), {}, te_comment_t(-1,-1,"poison")));
    t.add_edge(e);
    from_node = intermediate_node;
  }
}

void
sym_exec_llvm::add_state_assume(string const& varname, expr_ref const& assume, state const& state_in, unordered_set<expr_ref>& assumes, dshared_ptr<tfg_node>& from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map)
{
  if (model_llvm_semantics && varname != "") {
    string poison_varname = get_poison_value_varname(varname);
    expr_ref poison_expr;

    if (set_belongs(m_poison_varnames_seen, poison_varname)) {
      expr_ref poison_var = get_input_expr(poison_varname, m_ctx->mk_bool_sort());
      poison_expr = m_ctx->mk_or(poison_var, assume);
    } else {
      poison_expr = assume;
    }
    m_poison_varnames_seen.insert(poison_varname);

    state state_to_intermediate_val;
    state_set_expr(state_to_intermediate_val, poison_varname, poison_expr);
    dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
    shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx), {}, te_comment_t(-1,-1,"poison")));
    t.add_edge(e);
    from_node = intermediate_node;
  } else {
    assumes.insert(assume);
  }
}

void
sym_exec_llvm::transfer_poison_value_on_load(string const& varname, expr_ref const& load_expr, unordered_set<expr_ref>& state_assumes, dshared_ptr<tfg_node>& from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map)
{
  if (!model_llvm_semantics) {
    return;
  }
  ASSERT(load_expr->get_operation_kind() == expr::OP_SELECT);
  ASSERT(load_expr->get_args().at(OP_SELECT_ARGNUM_MEM)->is_var());
  ASSERT(load_expr->get_args().at(OP_SELECT_ARGNUM_MEM)->get_name()->get_str() == string(G_INPUT_KEYWORD ".") + m_mem_reg);

  string poison_varname = get_poison_value_varname(varname);
  m_poison_varnames_seen.insert(poison_varname);

  expr_ref mem_poison = m_ctx->mk_var(m_mem_poison_reg, this->get_mem_poison_sort());

  int count = load_expr->get_args().at(OP_SELECT_ARGNUM_COUNT)->get_int_value();
  ASSERT(count > 0);
  expr_ref poison_expr = m_ctx->mk_select_shadow_bool(mem_poison, load_expr->get_args().at(OP_SELECT_ARGNUM_ADDR), count);

  state state_to_intermediate_val;
  state_set_expr(state_to_intermediate_val, poison_varname, poison_expr);
  dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
  shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx), {}, te_comment_t(-1,-1,"poison")));
  t.add_edge(e);
  from_node = intermediate_node;
}


void
sym_exec_llvm::transfer_poison_value_on_store(expr_ref const& store_expr, unordered_set<expr_ref>& state_assumes, dshared_ptr<tfg_node>& from_node, bool model_llvm_semantics, tfg& t, map<llvm_value_id_t, string_ref>* value_to_name_map)
{
  if (!model_llvm_semantics) {
    return;
  }
  ASSERT(store_expr->get_operation_kind() == expr::OP_STORE);
  ASSERT(store_expr->get_args().at(OP_STORE_ARGNUM_MEM)->is_var());
  ASSERT(store_expr->get_args().at(OP_STORE_ARGNUM_MEM)->get_name()->get_str() == string(G_INPUT_KEYWORD ".") + m_mem_reg);

  expr_ref data = store_expr->get_args().at(OP_STORE_ARGNUM_DATA);
  expr_list vars = m_ctx->expr_get_vars(data);

  expr_ref poison_expr;
  for (auto const& v : vars) {
    string const& vname = v->get_name()->get_str();
    string poison_vname = get_poison_value_varname(vname);
    if (!set_belongs(m_poison_varnames_seen, poison_vname)) {
      continue;
    }
    expr_ref poison_v = get_input_expr(poison_vname, m_ctx->mk_bool_sort());

    if (!poison_expr) {
      poison_expr = poison_v;
    } else {
      poison_expr = expr_or(poison_expr, poison_v);
    }
  }
  if (!poison_expr) {
    poison_expr = expr_false(m_ctx);
  }
  ASSERT(poison_expr->is_bool_sort());

  expr_ref mem_poison = m_ctx->mk_var(m_mem_poison_reg, this->get_mem_poison_sort());
  int count = store_expr->get_args().at(OP_STORE_ARGNUM_COUNT)->get_int_value();
  ASSERT(count > 0);

  expr_ref addr = store_expr->get_args().at(OP_STORE_ARGNUM_ADDR);
  mem_poison = m_ctx->mk_store_shadow_bool(mem_poison, addr, poison_expr, count);

  state state_to_intermediate_val;
  state_set_expr(state_to_intermediate_val, this->m_mem_poison_reg, mem_poison);
  dshared_ptr<tfg_node> intermediate_node = get_next_intermediate_subsubindex_pc_node(t, from_node);
  shared_ptr<tfg_edge const> e = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), intermediate_node->get_pc(), state_to_intermediate_val, expr_true(m_ctx), {}, te_comment_t(-1,-1,"poison")));
  t.add_edge(e);
  from_node = intermediate_node;
}

string
sym_exec_llvm::get_next_undef_varname()
{
  stringstream ss;
  ss << m_srcdst_keyword + "." G_LLVM_PREFIX "-%" UNDEF_VARIABLE_NAME << this->m_cur_undef_varname_idx;
  this->m_cur_undef_varname_idx++;
  return ss.str();
}
