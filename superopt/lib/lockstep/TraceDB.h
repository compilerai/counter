#ifndef LLVM_EXECUTIONENGINE_TRACEDB_H
#define LLVM_EXECUTIONENGINE_TRACEDB_H
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
#include "lockstep/Common.h"
#include "lockstep/mempool.h"


//using namespace llvm;
using namespace std;

using tracedb_rank_t = int;
using tracedb_iteration_t = int;

class tracedb_val_t {
private:
  GenericValue m_val;
  TypeID m_typeID;
public:
  tracedb_val_t(GenericValue const &v, TypeID typID) : m_val(v), m_typeID(typID) {}
  bool operator<(tracedb_val_t const &other) const
  {
    NOT_IMPLEMENTED();
    //if (m_typeID < other.m_typeID) {
    //  return true;
    //}
    //if (m_typeID > other.m_typeID) {
    //  return false;
    //}
    //for (int i = 0; i < 8; i++) {
    //  if (m_val.Untyped[i] < other.m_val.Untyped[i]) {
    //    return true;
    //  }
    //  if (m_val.Untyped[i] > other.m_val.Untyped[i]) {
    //    return false;
    //  }
    //}
    //if (m_val.IntVal.ult(other.m_val.IntVal)) {
    //  return true;
    //}
    //if (other.m_val.IntVal.ult(m_val.IntVal)) {
    //  return false;
    //}
    //return false;
  }
  string tracedb_val_to_string() const
  {
    //return generic_value_to_string(m_val);
    return GenericValToString(m_val, m_typeID);
  }
};

class tracedb_state_t {
private:
  map<string, tracedb_val_t> m_map;
  map<dst_ulong, vector<uint8_t>> m_mallocs;
public:
  void tracedb_state_add_mapping(string const &k, TypeID typID, GenericValue const &v)
  {
    if (m_map.count(k)) {
      m_map.erase(k);
    }
    m_map.insert(make_pair(k, tracedb_val_t(v, typID)));
  }
  void tracedb_state_add_memory_region_snapshot(mempool_snapshot_t const &snapshot)
  {
    map<void *, vector<uint8_t>> const &new_mallocs = snapshot.get_mallocs();
    for (const auto &m : new_mallocs) {
      void *addr = m.first;
      vector<uint8_t> const &data = m.second;
      m_mallocs.insert(make_pair((dst_ulong)(uint64_t)addr, data));
    }
    //m_mallocs.insert(make_pair(addr, data));
  }
  bool operator<(tracedb_state_t const &other) const
  {
    if (m_map.size() < other.m_map.size()) {
      return true;
    } else if (other.m_map.size() < m_map.size()) {
      return false;
    }

    //compare keys
    auto iter1 = m_map.begin();;
    auto iter2 = other.m_map.begin();
    for (; iter1 != m_map.end() && iter2 != m_map.end(); iter1++, iter2++) {
      if (iter1->first < iter2->first) {
        return true;
      }
      if (iter1->first > iter2->first) {
        return false;
      }
    }

    iter1 = m_map.begin();;
    iter2 = other.m_map.begin();
    //compare values
    for (; iter1 != m_map.end() && iter2 != m_map.end(); iter1++, iter2++) {
      if (iter1->second < iter2->second) {
        return true;
      }
      if (iter2->second < iter1->second) {
        return false;
      }
    }
    return false;
  }
  void tracedb_state_print(ofstream &ofs) const
  {
    ofs << "=state begin\n";
    ofs << "=regs\n";
    for (const auto &p : m_map) {
      ofs << p.first << " : " << p.second.tracedb_val_to_string() << endl;
    }
    ofs << "=mem\n";
    for (const auto &p : m_mallocs)  {
      ofs << hex << p.first << dec << " :";
      data_vector_print(ofs, p.second);
      ofs << "\n";
    }
    ofs << "=state end\n";
  }
  static void data_vector_print(ofstream &ofs, vector<uint8_t> const &data)
  {
    for (const auto &b : data) {
      ofs << " " << hex << (int)b << dec;
    }
  }
};

class tracedb_state_space_t {
private:
  list<tracedb_state_t> m_states;
  tracedb_rank_t m_rank;
  static size_t const max_num_states_per_pc = 2;
public:
  tracedb_state_space_t() : m_rank(0) {}
  tracedb_rank_t get_rank() const { return m_rank; }
  bool tracedb_state_space_add(tracedb_state_t const &s)
  {
    m_states.push_back(s);
    int new_rank = tracedb_states_compute_rank(m_states);
    ASSERT(new_rank >= m_rank);
    if (new_rank == m_rank) {
      m_states.pop_back();
      return false;
    } else {
      m_rank = new_rank;
      return true;
    }
  }
  static int tracedb_states_compute_rank(list<tracedb_state_t> const &ls)
  {
    //NOT_IMPLEMENTED();
    //for now, just limit the max number of states per pc
    if (ls.size() < tracedb_state_space_t::max_num_states_per_pc) {
      return ls.size();
    } else {
      return tracedb_state_space_t::max_num_states_per_pc;
    }
  }
  void tracedb_state_space_print(ofstream &ofs) const
  {
    ofs << "=rank " << m_rank << endl;
    size_t n = 0;
    for (const auto &s : m_states) {
      ofs << "=state " << n << endl;
      s.tracedb_state_print(ofs);
      n++;
    }
  }
};

class tracedb_coverage_t {
private:
  map<llvm_pc_t, set<tracedb_iteration_t>> m_map;
  map<llvm_pc_t, tracedb_iteration_t> m_iteration_map;
public:
  static map<llvm_pc_t, tracedb_state_space_t> global_map;
  static void print_global_coverage_stats(ofstream &ofs)
  {
    for (const auto &m : global_map) {
      ofs << "LLVM_PC: " << m.first.to_string() << ": " << m.second.get_rank() << "\n";
    }
  }
  tracedb_coverage_t() {}
  /*void clear()
  {
    m_map.clear();
    m_iteration_map.clear();
  }*/
  bool is_empty() const
  {
    return m_map.size() == 0;
  }
  void add(llvm_pc_t const &p, tracedb_state_t const &s)
  {
    if (global_map[p].tracedb_state_space_add(s)) {
      m_map[p].insert(m_iteration_map[p]);
    }
    m_iteration_map[p]++;
    //m_last_state = s;
  }
  void tracedb_coverage_print(ofstream &ofs) const
  {
    for (const auto &m : m_map) {
      ofs << "=coverage_pc " << m.first.to_string() << ": iters";
      for (const auto &i : m.second) {
        ofs << " " << i;
      }
      ofs << endl;
    }
  }
};



class tracedb_state_set_t {
private:
  map<tracedb_state_t, tracedb_coverage_t> m_set;
  static const int m_max_states = 1024;
public:
  void insert(tracedb_state_t const &s, tracedb_coverage_t const &coverage)
  {
    if (m_set.size() < m_max_states) {
      m_set.insert(make_pair(s, coverage));
    } else {
      //need logic to replace an existing stage that is not adding to the coverage
      //for now, just ignore
    }
  }
  void tracedb_state_set_print(ofstream &ofs) const
  {
    size_t n = 0;
    for (const auto &p : m_set) {
      ofs << "=state " << n << endl;
      p.first.tracedb_state_print(ofs);
      ofs << "=coverage for state " << n << endl;
      p.second.tracedb_coverage_print(ofs);
      n++;
      ofs << endl;
    }
  }
};

class tracedb_t;

class tracedb_entry_t {
private:
  pair<llvm_pc_t, tracedb_state_t> m_last_candidate;
  tracedb_coverage_t m_coverage_of_last_candidate;
  friend class tracedb_t;
};

class tracedb_t {
private:
  stack<tracedb_entry_t> m_stack; //tracks the currently active traces; stack allows nested caller/callees to be tracked simultaneously
  map<llvm_pc_t, tracedb_state_set_t> m_map;
  string m_filename;
public:
  static unique_ptr<tracedb_t> tracedb;
  tracedb_t() = delete;
  tracedb_t(string const &filename) : m_filename(filename) {}
  void register_initial_state_candidate(llvm_pc_t const &p, tracedb_state_t const &s)
  {
    tracedb_entry_t e;
    e.m_last_candidate = make_pair(p, s);
    //e.m_coverage_of_last_candidate.clear();
    m_stack.push(e);
  }
  void register_coverage_point(llvm_pc_t const &p, tracedb_state_t const &s)
  {
    //dbgs() << __func__ << " " << __LINE__ << ": p = " << p.to_string() << "\n";
    ASSERT(m_stack.size() > 0);
    m_stack.top().m_coverage_of_last_candidate.add(p, s);
  }
  void commit_last_candidate_if_coverage_is_improved()
  {
    ASSERT(m_stack.size() > 0);
    if (!m_stack.top().m_coverage_of_last_candidate.is_empty()) {
      llvm_pc_t const &p = m_stack.top().m_last_candidate.first;
      tracedb_state_t const &s = m_stack.top().m_last_candidate.second;
      //XXX: also include any memory regions that may have been allocated (through alloca) before commit in s.
      m_map[p].insert(s, m_stack.top().m_coverage_of_last_candidate);
    }
    m_stack.pop();
  }
  void tracedb_print() const
  {
    ofstream ofs;
    ofs.open(m_filename);
    ofs << "Coverage stats:\n";
    tracedb_coverage_t::print_global_coverage_stats(ofs);
    ofs << endl;
    ofs << "Traces:\n";
    for (const auto &m : m_map) {
      ofs << "LLVM_PC: " << m.first.to_string() << "\n";
      m.second.tracedb_state_set_print(ofs);
      ofs << "\n";
    }
    ofs.close();
  }

  static const unsigned long heap_begin = 0x10000000;
  static const unsigned long heap_end = 0xb0000000;
  static const unsigned long globals_begin = 0xb0000000;
  static const unsigned long globals_end = 0xb0f00000;
  static const unsigned long locals_begin = 0xb0f00000;
  static const unsigned long locals_end = 0xbf000000;
  static const unsigned long stack_begin = 0xbf000000;
  static const unsigned long stack_end = 0xbffff000;

  //static mempool_t heap_mempool, stack_mempool, globals_mempool, locals_mempool;

  //static pair<globals_t, argvs_t>
  //tracedb_pick_addresses_for_globals_and_argvs(std::vector<std::string> const &argv, ExecutionEngineState::GlobalAddressMapTy const &globalsMap, ExecutionEngine *EE)
  //{
  //  argvs_t argvs_ret;
  //  char **argv_arr = (char **)locals_mempool.mempool_malloc(argv.size() * sizeof(char *));
  //  char **argv_arr_ptr = (char **)memspace_t::global_memspace.convert_to_ptr(argv_arr);
  //  argvs_ret.argv_addr = (dst_ulong)(uint64_t)argv_arr;
  //  for (size_t i = 0; i < argv.size(); i++) {
  //    size_t sz = argv.at(i).size();
  //    argv_arr_ptr[i] = (char *)locals_mempool.mempool_malloc(sz + 1);
  //    char *argv_ptr = (char *)memspace_t::global_memspace.convert_to_ptr(argv_arr_ptr[i]);
  //    memcpy(argv_ptr, argv.at(i).c_str(), sz);
  //    argv_ptr[sz] = '\0';
  //    argvs_ret.argv_values.push_back(make_pair((dst_ulong)(uint64_t)argv_arr_ptr[i], argv.at(i)));
  //  }
  //  globals_t globals_ret;
  //  for (auto iter = globalsMap.begin(); iter != globalsMap.end(); iter++) {
  //    string name = iter->first();
  //    uint64_t orig_addr = iter->second;
  //    GlobalValue const *GV = EE->getGlobalValueAtAddress((void *)orig_addr);
  //    Type *ElTy = GV->getValueType();
  //    DataLayout const &TD = EE->getDataLayout();
  //    size_t GVSize = (size_t)TD.getTypeAllocSize(ElTy);
  //    void *new_addr = globals_mempool.mempool_malloc(GVSize);
  //    globals_ret.m.insert(make_pair(name, (dst_ulong)(uint64_t)new_addr));
  //  }
  //  return make_pair(globals_ret, argvs_ret);
  //}
  //static void *heap_malloc(void *orig, size_t sz)
  //{
  //  return heap_mempool.mempool_malloc(sz);
  //}
  //static void heap_free(void *ptr)
  //{
  //  heap_mempool.mempool_free(ptr);
  //}
  //static void *locals_malloc(/*Type *typ, */size_t MemToAlloc)
  //{
  //  return locals_mempool.mempool_malloc(MemToAlloc);
  //}
  //static void locals_free(void *ptr)
  //{
  //  locals_mempool.mempool_free(ptr);
  //}
  //void snapshot_memory_regions_to_tracedb_state(tracedb_state_t &st) const
  //{
  //  st.tracedb_state_add_memory_region_snapshot(heap_mempool.mempool_get_snapshot());
  //  st.tracedb_state_add_memory_region_snapshot(globals_mempool.mempool_get_snapshot());
  //  st.tracedb_state_add_memory_region_snapshot(locals_mempool.mempool_get_snapshot());
  //  st.tracedb_state_add_memory_region_snapshot(stack_mempool.mempool_get_snapshot());
  //}
};

void beginTracePoint();
//tracedb_state_t tracedb_state_from_EC(ExecutionContext const &ec);

#endif
