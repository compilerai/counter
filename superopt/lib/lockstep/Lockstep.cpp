//#include "Interpreter.h"
//#include "llvm/ExecutionEngine/Lockstep.h"
//#include "llvm/ExecutionEngine/memspace.h"
//#include "llvm/ADT/APInt.h"
//#include "llvm/ADT/Statistic.h"
//#include "llvm/CodeGen/IntrinsicLowering.h"
//#include "llvm/IR/Constants.h"
//#include "llvm/IR/DerivedTypes.h"
//#include "llvm/IR/GetElementPtrTypeIterator.h"
//#include "llvm/IR/Instructions.h"
//#include "llvm/Support/CommandLine.h"
//#include "llvm/Support/Debug.h"
//#include "llvm/Support/ErrorHandling.h"
//#include "llvm/Support/MathExtras.h"
//#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cmath>
#include <sys/mman.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <list>
#include <stack>
#include "lockstep/Lockstep.h"
#include "lockstep/TraceDB.h"
#include "lockstep/mempool.h"
#include "support/dyn_debug.h"
//using namespace llvm;
using namespace std;

bool
dyn_debug_state_elem_t::match_execution_context(ExecutionContext &SF) const
{
  //cout << "matches_execution_context returning true for:\n" << this->to_string() << "\nand\n";
  bool found_mismatch = false;
  for (auto &vv : SF.get_values_ref()) {
    //Value *key = vv.first;
    //string e_key = string(DD_LLVM_SRC_PREFIX) + get_value_name(*key);
    string e_key = string(DD_LLVM_SRC_PREFIX) + vv.first;
    CPP_DBG_EXEC2(LOCKSTEP, cout << __func__ << " " << __LINE__ << ": e_key = " << e_key << endl);
    uint64_t ddval;
    if (   ddmap.count(e_key)
        && !dd_llvm_values_match((ddval = ddmap.at(e_key)), vv.second.second, vv.second.first)) {
      TypeID typID = vv.second.first;
      string llvm_val_str = generic_value_to_string(vv.second.second);
      stringstream ss;
      ss << __func__ << " " << __LINE__ << ": mismatch for key " << e_key << ": codegen value " << hex << ddval << dec << " vs. dbg-gen value " << llvm_val_str << "\n";
      cout << ss.str();

      ss.str("");
      /*if (   typID == Type::IntegerTyID
          && vv.second.IntVal.getBitWidth() == 1) {
        ss << __func__ << " " << __LINE__ << ": NOT updating value for key " << e_key << ": " << hex << ddval << dec << " vs. " << llvm_val_str << " because it is boolean type\n";
      } else */{
        ss << __func__ << " " << __LINE__ << ": updating value for key " << e_key << ": " << hex << ddval << dec << " vs. " << llvm_val_str << "\n";
        dd_llvm_value_update(ddval, vv.second.second, typID);
      }
      cout << ss.str();
      found_mismatch = true;
    }
    //string llvm_val_str = generic_value_to_string(vv.second);
    //cout << e_key << " : " << llvm_val_str << "\n";
  }
  for (auto &dm : ddmemory) {
    //uint8_t *addr = (uint8_t *)mem32_start + (unsigned long)dm.first;
    //uint8_t *addr = (uint8_t *)memspace_t::global_memspace.convert_to_ptr(dm.first);
    uint8_t *addr = (uint8_t *)dm.first;
    if (dm.second != *addr) {
      stringstream ss;
      ss << __func__ << " " << __LINE__ << ": mismatch at addr " << hex << *addr << dec << " vs. " << hex << dm.second << dec << "\n";
      cout << ss.str();
      cout.flush();
      ss.clear();
      ss << __func__ << " " << __LINE__ << ": updating value at addr " << addr << " from " << hex << *addr << dec << " to " << hex << dm.second << dec << "\n";
      cout << ss.str();
      cout.flush();
      *addr = dm.second;
      found_mismatch = true;
    }
  }
  return !found_mismatch;
}

void
dyn_debug_state_t::dd_match_execution_context_at_pc(ExecutionContext &SF, llvm_pc_t const &llvm_pc) const
{
  for (auto iter = q.begin(); iter != q.end(); iter++) {
    if (iter->matches_llvm_pc(llvm_pc)) {
      if (!iter->match_execution_context(SF)) {
        cout << "Found mismatch at " << llvm_pc_t::cur_llvm_pc.to_string() << "\n";
        //cout << "dyn_debug_state pc list = " << this->pc_list_to_string() << "\n";
      }
      return;
    }
  }
  return;
}

void
beginTracePoint()
{
  if (!tracedb_t::tracedb) {
    return;
  }
  ExecutionContext &SF = llvm_machine_state_t::ECStack.top();
  //tracedb_state_t s = tracedb_state_from_EC(SF);
  //tracedb_t::tracedb->register_initial_state_candidate(llvm_pc_t::cur_llvm_pc, s);
}

void
registerCoverageAtPC()
{
  if (!tracedb_t::tracedb) {
    return;
  }
  if (llvm_machine_state_t::ECStack.empty()) {
    return;
  }
  ExecutionContext &SF = llvm_machine_state_t::ECStack.top();
  //tracedb_state_t s = tracedb_state_from_EC(SF);
  //tracedb_t::tracedb->register_coverage_point(llvm_pc_t::cur_llvm_pc, s);
}

tracedb_state_t
tracedb_state_from_EC(ExecutionContext const &SF)
{
  tracedb_state_t st;
  for (const auto &vv : SF.get_values()) {
    string e_key = string(DD_LLVM_SRC_PREFIX) + vv.first/*get_value_name(*vv.first)*/;
    st.tracedb_state_add_mapping(e_key, vv.second.first/*vv.first->getType()->getTypeID()*/, vv.second.second);
  }
  ASSERT(tracedb_t::tracedb);
  //tracedb_t::tracedb->snapshot_memory_regions_to_tracedb_state(st);
  return st;
}

void
ExecutionContext::register_update(char const* name, TypeID typID, int nbits, uint64_t val)
{
  m_vals.erase(name);
  m_vals.insert(make_pair(name, make_pair(typID, GenericValue(val, nbits))));
}

std::string getTraceDB() { return ""; }
bool getLockstep() { return true; }
string genFreqHist() { return ""; }

stack<ExecutionContext> llvm_machine_state_t::ECStack;
unique_ptr<tracedb_t> tracedb_t::tracedb;
map<llvm_pc_t, tracedb_state_space_t> tracedb_coverage_t::global_map;
//mempool_t tracedb_t::heap_mempool;
//mempool_t tracedb_t::stack_mempool;
//mempool_t tracedb_t::globals_mempool;
//mempool_t tracedb_t::locals_mempool;
