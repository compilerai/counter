#include "llvm/ADT/Statistic.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include <fstream>
#include <string>
#include <algorithm>
#include <cmath>
#include <set>
#include <dlfcn.h>
#include "support/utils.h"
#include "support/types.h"
#include "../lib/Analysis/Superopt/dfa_helper.h"
using namespace llvm;
using namespace std;

#define DEBUG_TYPE "lockstep"
#define REGISTER_FUN_FUNCTION_NAME "lockstep_register_fun"
#define REGISTER_BBL_FUNCTION_NAME "lockstep_register_bbl"
#define REGISTER_INST_FUNCTION_NAME "lockstep_register_inst"
#define REGISTER_FCALL_FUNCTION_NAME "lockstep_register_fcall"
#define CLEAR_FUN_FUNCTION_NAME "lockstep_clear_fun"

namespace {
  struct Lockstep : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    map<string, Value*> m_existing_globals;
    int m_nglobal;
    Lockstep() : ModulePass(ID), m_nglobal(0) {}

    static inline string get_value_name(const Value& v)
    {
      if (v.getType()->isVoidTy()) {
        return "";
      }
      assert(!v.getType()->isVoidTy());

      string ret;
      raw_string_ostream ss(ret);
      v.printAsOperand(ss, false);
      ss.flush();
      errs().flush();

      return ret;
    }

    static vector<Value*> llvm_insn_get_args(Instruction& I)
    {
      return vector<Value*>(); //NOT_IMPLEMENTED()
    }

    Value* find_or_add_string(Module& M, IRBuilder<>& Builder, string const& s)
    {
      if (m_existing_globals.count(s)) {
        return m_existing_globals.at(s);
      }
      LLVMContext& ctx = M.getContext();
      stringstream ss;
      ss << "glob" << m_nglobal++;
      auto *GlobalPtr =
          cast<GlobalVariable>(M.getOrInsertGlobal(ss.str(), ArrayType::get(Type::getInt8Ty(ctx), s.size() + 1)));
      GlobalPtr->setLinkage(GlobalValue::LinkageTypes::InternalLinkage);

      Constant *string_const = ConstantDataArray::getString(ctx, s.c_str());
      GlobalPtr->setInitializer(string_const);

      llvm::Value *cast = Builder.CreatePointerCast(GlobalPtr, Builder.getInt8PtrTy());

      //Value *glob = new GlobalVariable(M, ArrayType::get(Type::getInt8Ty(ctx), s.size()), GlobalValue::InternalLinkage, c);
      return cast;
    }

    static Value* convertValToInt64(IRBuilder<>& Builder, Value *v, LLVMContext& ctx)
    {
      switch (v->getType()->getTypeID()) {
        case Type::VoidTyID: {
          return ConstantInt::getSigned(Type::getInt64Ty(ctx), LLVM_LOCKSTEP_DEFAULT_VOID_VAL);
        }
        case Type::IntegerTyID: {
          if (v->getType() == Type::getInt64Ty(ctx)) {
            return v;
          } else {
            return Builder.CreateIntCast(v, Type::getInt64Ty(ctx), false);
          }
        }
        case Type::StructTyID: {
          errs() << __func__ << " " << __LINE__ << ": struct not implemented\n";
          assert(0);
        }
        case Type::ScalableVectorTyID:
        case Type::FixedVectorTyID: {
          errs() << __func__ << " " << __LINE__ << ": vector not implemented\n";
          assert(0);
        }
        case Type::FunctionTyID: {
          errs() << __func__ << " " << __LINE__ << ": function not implemented\n";
          assert(0);
        }
        case Type::PointerTyID: {
          return Builder.CreatePtrToInt(v, Type::getInt64Ty(ctx));
        }
        case Type::FloatTyID:
        case Type::DoubleTyID:
        case Type::X86_FP80TyID:
        case Type::FP128TyID:
        case Type::PPC_FP128TyID: {
          errs() << __func__ << " " << __LINE__ << ": float not implemented\n";
          assert(0);
        }
        default: {
          errs() << __func__ << " " << __LINE__ << ": unrecognized type id " << v->getType()->getTypeID() << "\n";
          assert(0);
        }
      }
    }

    static llvm_insn_type_t get_insn_type(Instruction& I)
    {
      if (isa<PHINode>(I)) {
        return llvm_insn_phinode;
      } else if (I.getOpcode() == Instruction::Alloca) {
        return llvm_insn_alloca;
      } else if (I.getOpcode() == Instruction::Ret) {
        return llvm_insn_ret;
      } else if (I.getOpcode() == Instruction::Call) {
        return llvm_insn_call;
      } else if (I.getOpcode() == Instruction::Invoke) {
        return llvm_insn_invoke;
      } else if (I.getOpcode() == Instruction::Load) {
        return llvm_insn_load;
      } else if (I.getOpcode() == Instruction::Store) {
        return llvm_insn_store;
      } else {
        return llvm_insn_other;
      }
    }

    static int get_num_bits_for_type(Type* t)
    {
      if (t->isSized()) {
        return t->getPrimitiveSizeInBits();
      } else {
        return 0;
      }
    }

    static Instruction* get_first_non_instrumented_insn_in_bb(BasicBlock* bb, set<Value*>& instrumented_instructions)
    {
      Instruction* ret = bb->getFirstNonPHIOrDbg();
      while (ret && set_belongs(instrumented_instructions, (Value*)ret)) {
        ret = ret->getNextNode();
      }
      return ret;
    }

    static TypeID get_type_id(Type::TypeID t)
    {
      if (t == Type::VoidTyID) {
        return VoidTyID;
      } else if (t == Type::IntegerTyID) {
        return IntegerTyID;
      } else if (t == Type::PointerTyID) {
        return PointerTyID;
      } else {
        cout << __func__ << " " << __LINE__ << ": t = " << t << endl;
        NOT_IMPLEMENTED();
      }
    }

    set<Value*> handle_args(Module& M, Function& F, LLVMContext& ctx, IRBuilder<>& Builder, set<Value*>& instrumented_instructions){
      string fun_name = F.getName().str();
      int argnum = 0;
      for (Function::arg_iterator AI = F.arg_begin(), E = F.arg_end(); AI != E; ++AI) {
        argnum++;
        BasicBlock* FirstBasicBlock = NULL;
        for (Function::iterator b = F.begin(), be = F.end(); b != be; ++b){
          FirstBasicBlock = dyn_cast<BasicBlock>(&*b);
          break;
        }

        if ( FirstBasicBlock == NULL ) continue;
        Instruction* FirstNonPHIInst = get_first_non_instrumented_insn_in_bb(FirstBasicBlock, instrumented_instructions);
        if (FirstNonPHIInst == NULL) continue;

        stringstream ss;
        ss << fun_name << ".arg" << argnum;
        string argname = ss.str();

        Builder.SetInsertPoint(FirstNonPHIInst);
        //Builder.CreateCall(ValueFn, ValueArgs);
        //Value* ptr_in_int = Builder.CreatePtrToInt(&*AI, Type::getInt64Ty(ctx));
        Value* val_in_int = convertValToInt64(Builder, &*AI, ctx);
        if (val_in_int) {
          if (val_in_int != &*AI) {
            instrumented_instructions.insert(val_in_int);
          }
          Function *FuncToCall= M.getFunction(REGISTER_INST_FUNCTION_NAME);
          std::vector<Value*> Args;
          string valname = get_value_name(*AI);
          TypeID typID = get_type_id((*AI).getType()->getTypeID());
          int nbits = get_num_bits_for_type((*AI).getType());
          Value* valstr = find_or_add_string(M, Builder, argname);
          int nargs = 0;
          Args.push_back(valstr);
          Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), typID));
          Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), nbits));
          Args.push_back(val_in_int);
          Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), llvm_insn_farg));
          Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), nargs));
          Args.push_back(ConstantPointerNull::get(Type::getInt64PtrTy(ctx)));
          Args.push_back(ConstantPointerNull::get(Type::getInt64PtrTy(ctx)));
          Value* callv = Builder.CreateCall(FuncToCall, Args); // Result is void
          instrumented_instructions.insert(callv);
        }
      }
      return instrumented_instructions;
    }

    void insn_replace_malloc_with_mymalloc(Module& M, CallInst* c)
    {
      Function *calleeF = c->getCalledFunction();
      if (!calleeF) {
        return;
      }
      string fun_name = calleeF->getName().str();
      string new_fun_name = convert_malloc_callee_to_mymalloc(fun_name.c_str());
      if (new_fun_name == fun_name) {
        return;
      }
      FunctionCallee new_callee = M.getOrInsertFunction(new_fun_name, calleeF->getFunctionType());
      Value* calleeV = c->getCalledOperand();
      Value* new_calleeV = new_callee.getCallee();
      //Constant* new_calleeV = M.getOrInsertFunction(new_fun_name, calleeF->getFunctionType());
      Instruction* I = c;
      for (int i = 0; i < I->getNumOperands(); i++) {
        Value* o = I->getOperand(i);
        if (o == calleeV) {
          I->setOperand(i, new_calleeV);
        }
      }
    }

    void handle_function(Module& M, Function& F, LLVMContext& ctx, IRBuilder<>& Builder, set<Value*> instrumented_instructions){
      //errs() << __func__ << " " << __LINE__ << ": F = " << F.getName() << "\n";
      DataLayout const& DL = M.getDataLayout();
      int bbnum = 0;
      for (BasicBlock &BB : F) {
        bool first_insn_in_BB = true;
        //errs() << __func__ << " " << __LINE__ << ": F = " << F.getName() << " bb" << bbnum++ << "\n";
        for (auto insn_iter = BB.begin(); insn_iter != BB.end(); insn_iter++) {
          Instruction &I = *insn_iter;
          if (set_belongs(instrumented_instructions, (Value *)&I)) {
            continue;
          }

          //errs() << __func__ << " " << __LINE__ << ": I = " << I << "\n";
          auto FirstNonPHIInst = &I; //first non-phi inst starting at I
          if (isa<PHINode>(I)) {
            FirstNonPHIInst = get_first_non_instrumented_insn_in_bb(&BB, instrumented_instructions);
            if (FirstNonPHIInst == NULL) {
              //errs() << __LINE__ << "\n";
              continue;
            }

            //FirstNonPHIInst = FirstNonPHIInst->getPrevNode();
            //if( FirstNonPHIInst == NULL ) {
            //  //errs() << __LINE__ << "\n";
            //  continue;
            //}
          }
          //Builder.SetInsertPoint(FirstNonPHIInst->getNextNode());
          if (first_insn_in_BB) {
            Builder.SetInsertPoint(FirstNonPHIInst);
            string bblname = get_basicblock_name(BB);
            Value* bblstr = find_or_add_string(M, Builder, bblname);
            Function *FuncToCall= M.getFunction(REGISTER_BBL_FUNCTION_NAME);
            std::vector<Value*> Args;
            Args.push_back(bblstr);
            Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), 0));
            Instruction *callv = Builder.CreateCall(FuncToCall, Args); // Result is void
            instrumented_instructions.insert(callv);
            first_insn_in_BB = false;
          }

          auto NextInsn = (FirstNonPHIInst == &I) ? I.getNextNode() : FirstNonPHIInst;
          if (isa<ReturnInst>(I)) {
            Builder.SetInsertPoint(&I);
            Function *FuncToCall= M.getFunction(CLEAR_FUN_FUNCTION_NAME);
            std::vector<Value*> Args;
            if(F.getName().str() == "main")
              Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), 1 ));
            else
              Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), 0 ));
            Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), 0));
            Value *callv = Builder.CreateCall(FuncToCall, Args); // Result is void
            instrumented_instructions.insert(callv);
            continue;
          } else if (isa<CallInst>(I)) {
            Builder.SetInsertPoint(&I);
            Function *FuncToCall= M.getFunction(REGISTER_FCALL_FUNCTION_NAME);
            std::vector<Value*> Args;
            Value *callv = Builder.CreateCall(FuncToCall, Args); // Result is void
            instrumented_instructions.insert(callv);

            CallInst* c =  cast<CallInst>(&I);
            insn_replace_malloc_with_mymalloc(M, c);
          }

          if (!NextInsn) {
            continue;
          }

          Builder.SetInsertPoint(NextInsn);

          //Builder.SetInsertPoint(FirstNonPHIInst->getNextNode());
          //Builder.CreateCall(ValueFn, ValueArgs);
          //Value* ptr_in_int = Builder.CreatePtrToInt(&I, Type::getInt64Ty(ctx));
          Value* val_in_int = convertValToInt64(Builder, &I, ctx);
          if (val_in_int) {
            string valname = get_value_name(I);
            if (val_in_int != &I) {
              instrumented_instructions.insert(val_in_int);
            }
            llvm_insn_type_t llvm_insn_type = get_insn_type(I);
            //errs() << __func__ << " " << __LINE__ << ": creating call to register_inst\n";
            Function *FuncToCall= M.getFunction(REGISTER_INST_FUNCTION_NAME);

            vector<Value*> args = llvm_insn_get_args(I);
            Value* args_sz = ConstantInt::get(Type::getInt32Ty(ctx), args.size());

            llvm::Value* local_var = nullptr;
            if (args.size()) {
              local_var = Builder.CreateAlloca(Builder.getInt64Ty(), args_sz);
              instrumented_instructions.insert(local_var);
            }
            vector<Value*> args_int;
            for (size_t i = 0; i < args.size(); i++) {
              auto arg = args.at(i);
              llvm::Value *cast;
              if (arg->getType()->getTypeID() == Type::IntegerTyID) {
                if (arg->getType() == Type::getInt64Ty(ctx)) {
                  cast = arg;
                } else {
                  cast = Builder.CreateIntCast(arg, Builder.getInt64Ty(), false);
                }
              } else if (arg->getType()->getTypeID() == Type::PointerTyID) {
                cast = Builder.CreatePtrToInt(arg, Builder.getInt64Ty());
              } else {
                NOT_IMPLEMENTED();
              }
              if (cast != arg) {
                instrumented_instructions.insert(cast);
              }
              Value* idxVal = ConstantInt::getSigned(Type::getInt64Ty(ctx), i);
              ArrayRef<Value *> idxList({idxVal});
              llvm::Value* ptr = Builder.CreateGEP(Builder.getInt64Ty(), local_var, idxList);
              instrumented_instructions.insert(ptr);
              llvm::Value* store_insn = Builder.CreateStore(cast, ptr);
              instrumented_instructions.insert(store_insn);
            }

            TypeID typID = get_type_id(I.getType()->getTypeID());
            int nbits = get_num_bits_for_type(I.getType());
            std::vector<Value*> Args;
            Args.push_back(find_or_add_string(M, Builder, valname));
            Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), typID));
            Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), nbits));
            Args.push_back(val_in_int);
            Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), llvm_insn_type));
            Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), args.size()));
            Args.push_back(ConstantPointerNull::get(Type::getInt64PtrTy(ctx)));
            Args.push_back(ConstantPointerNull::get(Type::getInt64PtrTy(ctx)));
            Value *callv = Builder.CreateCall(FuncToCall, Args); // Result is void
            instrumented_instructions.insert(callv);
          }
        }
      }
    }
    int numberOfPointerInst(Function& F){
      int counter = 0;
      for (Function::arg_iterator AI = F.arg_begin(), E = F.arg_end(); AI != E; ++AI) {
        if ((*AI).getType()->isPointerTy()) counter++;
      }
      for(BasicBlock& BB : F){
        for (Instruction& I: BB){
          if(I.getType()->isPointerTy()) counter++;
        }
      }
      return counter;
    }

    bool runOnModule(Module &M) override
    {
      LLVMContext& ctx = M.getContext();
      //std::string errStr;
      //ExecutionEngine* EE = EngineBuilder(&M).setErrorStr(&errStr).create();
      //if (!EE) {
      //  errs() << __func__ << " " << __LINE__ << ": lockstep: failed to "
      //       "construct ExecutionEngine: " << errStr << "\n";
      //  return false;
      //}

      // register_fun function
      Type* voidType0[2] = { Type::getInt8PtrTy(ctx), Type::getInt64Ty(ctx) };
      ArrayRef<Type*> voidTypeARef0 (voidType0, 2);
      FunctionType* signature0 = FunctionType::get(Type::getVoidTy(ctx), voidTypeARef0, false);
      /*FunctionCallee register_fun_func = */M.getOrInsertFunction(REGISTER_FUN_FUNCTION_NAME, signature0);

      // register_bbl function
      /*FunctionCallee register_bbl_func = */M.getOrInsertFunction(REGISTER_BBL_FUNCTION_NAME, signature0);

      // register_inst function
      Type* voidType1[8] = { Type::getInt8PtrTy(ctx), Type::getInt64Ty(ctx), Type::getInt64Ty(ctx), Type::getInt64Ty(ctx), Type::getInt64Ty(ctx), Type::getInt64Ty(ctx), Type::getInt64PtrTy(ctx), Type::getInt64PtrTy(ctx) };
      ArrayRef<Type*> voidTypeARef1(voidType1, 8);
      FunctionType* signature1 = FunctionType::get(Type::getVoidTy(ctx), voidTypeARef1, false);
      /*FunctionCallee register_inst_func = */M.getOrInsertFunction(REGISTER_INST_FUNCTION_NAME, signature1);

      // clear_fun function
      Type* voidType2[2] = { Type::getInt64Ty(ctx), Type::getInt64Ty(ctx) };
      ArrayRef<Type*> voidTypeARef2 (voidType2, 2);
      FunctionType* signature2 = FunctionType::get(Type::getVoidTy(ctx), voidTypeARef2, false);
      /*FunctionCallee clear_fun_func = */M.getOrInsertFunction(CLEAR_FUN_FUNCTION_NAME, signature2);

      // register_fcall function
      ArrayRef<Type*> voidTypeARef3({});
      FunctionType* signature3 = FunctionType::get(Type::getVoidTy(ctx), voidTypeARef3, false);
      M.getOrInsertFunction(REGISTER_FCALL_FUNCTION_NAME, signature3);

      // ------------------------------------------------------------- //
      IRBuilder<> Builder(ctx);
      for (Function &F : M) {
        set<Value*> instrumented_instructions;

        string fun_name = F.getName().str();
        Value* funstr = find_or_add_string(M, Builder, fun_name);

        BasicBlock* FirstBasicBlock = NULL;
        for (Function::iterator b = F.begin(), be = F.end(); b != be; ++b){
          FirstBasicBlock = dyn_cast<BasicBlock>(&*b);
          break;
        }
        if (FirstBasicBlock == NULL) continue;
        Instruction* FirstNonPHIInst = get_first_non_instrumented_insn_in_bb(FirstBasicBlock, instrumented_instructions);
        if (FirstNonPHIInst == NULL) {
          //errs() << __LINE__ << "\n";
          continue;
        }
        IRBuilder<> Builder(FirstNonPHIInst);
        Function *FuncToCall= M.getFunction(REGISTER_FUN_FUNCTION_NAME);
        std::vector<Value*> Args;
        Args.push_back(funstr);
        Args.push_back(ConstantInt::getSigned(Type::getInt64Ty(ctx), 0));
        Value *callv = Builder.CreateCall(FuncToCall, Args); // Result is void
        instrumented_instructions.insert(callv);

        handle_args(M, F, ctx, Builder, instrumented_instructions);

        handle_function(M, F, ctx, Builder, instrumented_instructions);
      }
      return true;
    }
  };
}

char Lockstep::ID = 0;
static RegisterPass<Lockstep> Y("lockstep", "Run the executable in lockstep mode (used for debugging codegen)");
