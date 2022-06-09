#pragma once

#include "sym_exec_llvm.h"
#include "llvm/IR/Function.h"

using namespace std;
using namespace llvm;
dshared_ptr<tfg_llvm_t> function2tfg(Function *F, Module *M, map<shared_ptr<tfg_edge const>, Instruction *>& eimap);
string get_basicblock_name(const llvm::BasicBlock& v);
int get_counting_index_for_basicblock(llvm::BasicBlock const& v);

void set_global_function_name(string const& fname);
string const& get_global_function_name();
