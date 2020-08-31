#ifndef LLVM_EXECUTIONENGINE_COMMON_H
#define LLVM_EXECUTIONENGINE_COMMON_H

#include <map>
#include <set>
//#include "llvm/ExecutionEngine/ExecutionEngine.h"
//#include "llvm/ExecutionEngine/GenericValue.h"
//#include "llvm/IR/CallSite.h"
//#include "llvm/IR/DataLayout.h"
//#include "llvm/IR/Function.h"
//#include "llvm/IR/InstVisitor.h"
//#include "llvm/Support/DataTypes.h"
//#include "llvm/Support/ErrorHandling.h"
//#include "llvm/Support/raw_ostream.h"
#include "gsupport/pc.h"
#include <sys/mman.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <list>
#include <stack>

//using namespace llvm;
using namespace std;



//static inline string
//get_value_name(const Value& v)
//{
//  assert(!v.getType()->isVoidTy());
//
//  string ret;
//  raw_string_ostream ss(ret);
//  v.printAsOperand(ss, false);
//  ss.flush();
//  errs().flush();
//
//  return ret;
//}

static inline string
generic_value_to_string(GenericValue const &v)
{
  stringstream ss;
  //ss << "int 0x" << v.IntVal.toString(16, false) << " void* " << hex << intptr_t(v.PointerVal) << dec << " float " << v.FloatVal << " double " << v.DoubleVal;
  ss << "int 0x" << hex << v.getIntVal() << dec << " nbits " << v.get_nbits();
  return ss.str();
}

static inline std::string GenericValToString(GenericValue const &Val, TypeID typID)
{
  NOT_IMPLEMENTED();
  //stringstream ss;
  //switch (typID) {
  //default: llvm_unreachable("Invalid GenericValue Type");
  //case Type::VoidTyID:    ss << "void"; break;
  //case Type::FloatTyID:   ss << "float " << Val.FloatVal; break;
  //case Type::DoubleTyID:  ss << "double " << Val.DoubleVal; break;
  //case Type::PointerTyID: ss << "void* " << hex << intptr_t(Val.PointerVal) << dec; break;
  //case Type::IntegerTyID: 
  //  ss << "i" << Val.IntVal.getBitWidth() << " "
  //        << Val.IntVal.toString(10, false)
  //        << " (0x" << Val.IntVal.toString(16, false) << ")";
  //  break;
  //}
  //return ss.str();
}

#endif
