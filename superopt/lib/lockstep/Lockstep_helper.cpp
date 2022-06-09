//This is a helper file for Lockstep.cpp; this file must not refer to
//any symbol defined in the Interpreter/ directory because for some LLVM
//utilities, the Intepreter library is not linked even when the ExecutionEngine
//library is linked
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
#include "support/dyn_debug.h"
//using namespace llvm;
using namespace std;

#if ARCH_SRC == ARCH_ETFG && ARCH_DST == ARCH_I386
//void *mem32 = NULL;
//void *mem32_start = (void *)0x700000000000;
dyn_debug_state_t dyn_debug_state_t::dyn_debug_state;
//malloc_map_t malloc_map_t::malloc_map;
//memory_regions_t memory_regions_t::memory_regions;
llvm_pc_t llvm_pc_t::cur_llvm_pc;
llvm_pc_stack_t llvm_pc_stack_t::llvm_pc_stack;

//void *
//malloc_at_addr(void *addr, Type *typ, size_t size)
//{
//  //ASSERT((unsigned long)addr < mem32_size);
//#if SYNC_MODE
//  void *full_addr = memspace_t::global_memspace.convert_to_ptr(addr);
//  //void *ret = (void *)(((unsigned long)mem32_start) + (unsigned long)addr);
//  void *ret = full_addr;
//#else
//  void *ret = addr;
//#endif
//  memory_regions_t::memory_regions.memory_regions_add_mapping(ret, typ, size);
//  return ret;
//}

//void
//free_at_addr(void *addr)
//{
//  NOT_IMPLEMENTED();
//}

bool
dyn_debug_state_elem_t::dd_llvm_values_match(uint64_t ddval, GenericValue const &llvm_val, TypeID typID)
{
  if (typID == IntegerTyID) {
    int bit_width = llvm_val.get_nbits();
    if ((ddval & ((1ULL << bit_width) - 1)) == llvm_val.getIntVal()) {
      return true;
    }
    return false;
  } else if (typID == PointerTyID) {
    if (ddval == ((unsigned long)llvm_val.getIntVal() & 0xffffffff)) {
      return true;
    }
    return false;
  } else {
    cout << __func__ << " " << __LINE__ << ": typID = " << typID << endl;
    NOT_IMPLEMENTED();
  }
}

void
dyn_debug_state_elem_t::dd_llvm_value_update(uint64_t ddval, GenericValue &llvm_val, TypeID typID)
{
  if (typID == VoidTyID) {
    //do nothing
  } else if (typID == IntegerTyID) {
    //int bit_width = llvm_val.IntVal.getBitWidth();
    //llvm_val.IntVal = (ddval & ((1ULL << bit_width) - 1));
    int bit_width = llvm_val.get_nbits();
    llvm_val.setIntVal(ddval & ((1ULL << bit_width) - 1));
  } else if (typID == PointerTyID) {
    //llvm_val.PointerVal = (PointerTy)(((unsigned long)llvm_val.PointerVal & 0xffffffff00000000) | ddval);
  } else {
    NOT_IMPLEMENTED();
  }
}

string
dyn_debug_state_elem_t::to_string() const
{
  stringstream ss;
  ss << "icount " << icount << ": " << pc_to_string() << "\n";
  for (const auto &kv : ddmap) {
    ss << kv.first << " : " << hex << kv.second << dec << "\n";
  }
  ss << DYN_DEBUG_STATE_END_OF_STATE_MARKER << "\n";
  return ss.str();
}

void
dyn_debug_state_elem_t::read_pc_indices_from_pc_line(string const &line)
{
  size_t colon = line.find(": ");
  ASSERT(colon != string::npos);

  istringstream iss(line.substr(colon + 2));
  iss >> hex >> dd_pc >> dec;

  size_t pos = line.find('L');
  ASSERT(pos != string::npos);
  size_t end = line.find("=>");
  ASSERT(end != string::npos);

  string pc_str = line.substr(pos, end);
  dd_pc_tfg = eqspace::pc::create_from_string(pc_str);

  //size_t dot1 = line.find('.', pos + 1);
  //ASSERT(dot1 != string::npos);
  //size_t dot2 = line.find('.', dot1 + 1);
  //ASSERT(dot2 != string::npos);
  //size_t end = line.find("=>", dot2 + 1);
  //ASSERT(end != string::npos);

  //dd_pc_index = stoi(line.substr(pos + 1, dot1));
  //dd_pc_subindex = stoi(line.substr(dot1 + 1, dot2));
  //dd_pc_subsubindex = stoi(line.substr(dot2 + 1, end));
}

void *
dyn_debug_state_elem_t::compute_addr_using_ddmap_and_key(map<string, uint64_t> const &ddmap, string const &key, size_t &len)
{
  unsigned long addr;
  size_t lbrack = key.find('(');
  ASSERT(lbrack != string::npos);
  string disp_str = key.substr(0, lbrack);
  addr = stol(disp_str, NULL, 16);
  size_t comma = key.find(", ", lbrack);
  if (comma != string::npos) {
    NOT_IMPLEMENTED();
    comma += strlen(", ");
  } else {
    comma = lbrack + 1;
  }
  size_t rbrack = key.find(')');
  ASSERT(rbrack != string::npos);
  string reg_str = key.substr(comma + 1, rbrack);
  if (!ddmap.count(reg_str)) {
    cout << __func__ << " " << __LINE__ << ": could not find value for " << reg_str << " in ddmap\n";
    cout << "ddmap:\n";
    for (const auto &dv : ddmap) {
      stringstream ss;
      ss << dv.first << ": " << hex << dv.second << dec << "\n";
      cout << ss.str();
    }
  }
  ASSERT(ddmap.count(reg_str));
  addr += ddmap.at(reg_str);
  stringstream ss;
  ss << __func__ << ": returning " << hex << addr << dec << " for " << key << "\n";
  cout << ss.str();
  return (void *)addr;
}

void
dyn_debug_state_elem_t::add_mapping_using_line(string const &line)
{
  size_t colon = line.find(" : ");
  if (colon == string::npos) {
    return;
  }
  string key = line.substr(0, colon);
  size_t colon2 = line.rfind(" : ");
  if (colon2 == string::npos || colon2 == colon) {
    return;
  }
  string value_str = line.substr(colon2 + 3);
  istringstream iss(value_str);
  uint64_t value;
  iss >> hex >> value >> dec;
  if (key.find('(') != string::npos) {
    size_t len;
    void *addr = compute_addr_using_ddmap_and_key(ddmap, key, len);
    for (size_t i = 0; i < len; i++) {
      ddmemory[(uint8_t *)addr + i] = (value >> (i * BYTE_LEN)) & MAKE_MASK(BYTE_LEN);
    }
  } else {
    ddmap[key] = value;
  }
  //dbgs() << "line = " << line << "\n";
  //dbgs() << "key = " << key << ", value = " << value << "\n";
}

dyn_debug_state_elem_t
dyn_debug_state_elem_t::read_from_input()
{
  //bool gotline;
  string line;
  dyn_debug_state_elem_t ret;
  bool end;

  /*gotline = */getline(cin, line);
  //ASSERT(gotline);
  //dbgs() << "line = " << line << "\n";
  //dbgs().flush();

  while (line != DYN_DEBUG_STATE_END_OF_STATE_MARKER) {
    if (line.find("pc: ") == 0) {
      ret.read_pc_indices_from_pc_line(line);
    } else {
      ret.add_mapping_using_line(line);
    }
    /*gotline = */getline(cin, line);
    //ASSERT(gotline);
    //dbgs() << "line = " << line << "\n";
    //dbgs().flush();
  }
  end = !getline(cin, line);
  ASSERT(!end);
#define ICOUNT_PREFIX "icount = "
  ASSERT(string_has_prefix(line, ICOUNT_PREFIX));
  long long icount = string_to_longlong(line.substr(strlen(ICOUNT_PREFIX)));
  ret.icount = icount;
  //cout << "returning " << ret.to_string() << "\n";
  return ret;
}

bool
dyn_debug_state_elem_t::is_surely_a_sync_point() const
{
  //if (   dd_pc_subindex != 0 //real subindices start from PC_SUBINDEX_FIRST_INSN_IN_BB
  //    && dd_pc_subsubindex == 0) {
  //  return true;
  //}
  if (   dd_pc_tfg.get_subindex() != 0 //real subindices start from PC_SUBINDEX_FIRST_INSN_IN_BB
      && dd_pc_tfg.get_subsubindex() == PC_SUBSUBINDEX_DEFAULT) {
    return true;
  }
  return false;
}

string
dyn_debug_state_elem_t::pc_to_string() const
{
  stringstream ss;
  //ss << hex << dd_pc << dec << ": L" << dd_pc_index << "." << dd_pc_subindex << "." << dd_pc_subsubindex;
  ss << hex << dd_pc << dec << ": " << dd_pc_tfg.to_string();
  return ss.str();
}

string
dyn_debug_state_elem_t::icount_to_string() const
{
  stringstream ss;
  ss << icount;
  return ss.str();
}

uint64_t
dyn_debug_state_elem_t::get_value_for_key(string const &key) const
{
  string llvm_key = string(DD_LLVM_SRC_PREFIX) + key;
  if (!ddmap.count(llvm_key)) {
    cout << __func__ << " " << __LINE__ << ": this =\n" << this->to_string() << endl;
    cout << __func__ << " " << __LINE__ << ": llvm_key = " << llvm_key << endl;
  }
  ASSERT(ddmap.count(llvm_key));
  return ddmap.at(llvm_key);
}

bool
dyn_debug_state_elem_t::matches_llvm_pc(llvm_pc_t const &llvm_pc) const
{
  //return llvm_pc.matches(this->dd_pc_index, this->dd_pc_subindex, this->dd_pc_subsubindex);
  bool ret = llvm_pc.matches(this->dd_pc_tfg);
  //cout << __func__ << " " << __LINE__ << ": dd_pc_tfg = " << dd_pc_tfg.to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": llvm_pc = " << llvm_pc.to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": icount = " << icount << endl;
  //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
  return ret;
}

string
dyn_debug_state_t::pc_list_to_string() const
{
  stringstream ss;
  ss << "[";
  for (auto iter = q.begin(); iter != q.end(); iter++) {
    ss << iter->pc_to_string() << ", ";
  }
  ss << "]";
  return ss.str();
}

string
dyn_debug_state_t::icount_list_to_string() const
{
  stringstream ss;
  ss << "[";
  for (auto iter = q.begin(); iter != q.end(); iter++) {
    ss << iter->icount_to_string() << ", ";
  }
  ss << "]";
  return ss.str();
}

bool
dyn_debug_state_t::dd_matches_pc(llvm_pc_t const &llvm_pc) const
{
  for (auto iter = q.begin(); iter != q.end(); iter++) {
    if (iter->matches_llvm_pc(llvm_pc)) {
      return true;
    }
  }
  return false;
}

void
dyn_debug_state_t::clear_till_pc(llvm_pc_t const &llvm_pc)
{
  //dbgs() << __func__ << ": llvm_pc = " << llvm_pc.to_string() << ": before:\n" << this->pc_list_to_string() << "\n";
  while (q.size() && !q.front().matches_llvm_pc(llvm_pc)) {
    q.pop_front();
  }
  if (q.size() && q.front().matches_llvm_pc(llvm_pc)) {
    q.pop_front();
  }
  //dbgs() << __func__ << ": after:\n" << this->pc_list_to_string() << "\n";
}

bool
dyn_debug_state_t::contains_sync_point() const
{
  for (auto iter = q.begin(); iter != q.end(); iter++) {
    if (iter->is_surely_a_sync_point()) {
      return true;
    }
  }
  return false;
}

void
dyn_debug_state_t::populate_from_input()
{
  if (this->contains_sync_point()) {
    return;
  }
  dyn_debug_state_elem_t dde;
  do {
    dde = dyn_debug_state_elem_t::read_from_input();
    q.push_back(dde);
  } while (!dde.is_surely_a_sync_point());
  CPP_DBG_EXEC(LOCKSTEP,
    cout << __func__ << " " << __LINE__ << ": dyn_debug_state: q.size() = " << q.size() << endl;
    size_t i = 0;
    for (auto const& e : q) {
      cout << "q elem " << i << ":\n" << e.to_string() << endl;
      i++;
    }
  );
}

uint64_t
dyn_debug_state_t::get_value_for_key_at_pc(string const &key, llvm_pc_t const &llvm_pc)
{
  for (auto iter = q.begin(); iter != q.end(); iter++) {
    if (llvm_pc.matches(iter->dd_pc_tfg/*iter->dd_pc_index, iter->dd_pc_subindex, iter->dd_pc_subsubindex*/)) {
      return iter->get_value_for_key(key);
    }
  }
  cout << __func__ << " " << __LINE__ << ": could not get value for key " << key << " at " << llvm_pc.to_string() << "\n";
  cout << "dyn_debug_state =\n" << this->pc_list_to_string() << "\n";
  cout.flush();
  NOT_REACHED();
}

//void
//malloc_map_t::malloc_map_add_mapping(void *replaced, void *orig, size_t sz)
//{
//  ASSERT(m_map.count(replaced) == 0);
//  m_map[replaced] = make_pair(orig, sz);
//  //dbgs() << "malloc_map.size() = " << m_map.size() << "\n";
//}

//void *
//malloc_map_t::get_mapping(void *replaced)
//{
//  ASSERT(m_map.count(replaced));
//  return m_map.at(replaced).first; //return orig for replaced
//}

//void *
//memory_regions_t::extend_replaced_value_if_needed(void *replaced)
//{
//  unsigned long rtrunc = (unsigned long)replaced & 0xffffffff;
//  //dbgs() << "memory_regions.size() = " << m_map.size() << "\n";
//  for (const auto &m : m_map) {
//    unsigned long mtrunc = (unsigned long)m.first & 0xffffffff;
//    size_t msize = m.second.second;
//    stringstream ss;
//    //ss << "comparing " << hex << rtrunc << " with " << mtrunc << dec << ", size " << msize << endl;
//    //dbgs() << ss.str();
//    if (rtrunc >= mtrunc && rtrunc < mtrunc + msize) {
//      return (char *)m.first + (rtrunc - mtrunc);
//    }
//  }
//  return replaced;
//}

//bool
//malloc_map_t::is_malloc_family_function_for_alloc(Function *F)
//{
//#if SYNC_MODE
//  ASSERT(F);
//  string const &function_name = F->getName();
//  return function_name == "malloc"; //realloc should also figure here
//#else
//  return false;
//#endif
//}
//
//bool
//malloc_map_t::is_malloc_family_function_for_free(Function *F)
//{
//#if SYNC_MODE
//  ASSERT(F);
//  string const &function_name = F->getName();
//  return function_name == "free"; //realloc should also figure here because we need to convert the first arg to orig
//#else
//  return false;
//#endif
//}

//void
//memory_regions_t::memory_regions_add_mapping(void *addr, Type *typ, size_t sz)
//{
//  stringstream ss;
//  ss << "Adding memory region (" << hex << addr << dec << ", ";
//  dbgs() << ss.str();
//  if (typ) {
//    typ->print(dbgs());
//  } else {
//    dbgs() << "unknown-type";
//  }
//  dbgs() << ", " << sz << ")\n";
//  m_map[addr] = make_pair(typ, sz);
//  dbgs() << "done adding\n";
//}

//void
//memory_regions_t::memory_region_remove(void *addr)
//{
//  ASSERT(m_map.count(addr));
//  stringstream ss;
//  ss << "Removing memory region (" << hex << addr << dec << ", ";
//  dbgs() << ss.str();
//  if (m_map.at(addr).first) {
//    m_map.at(addr).first->print(dbgs());
//  } else {
//    dbgs() << "unknown-type";
//  }
//  dbgs() << ", " << m_map.at(addr).second << ")\n";
//  m_map.erase(addr);
//}

//map<void *, pair<Type *, size_t>> const &
//memory_regions_t::get_map() const
//{
//  return m_map;
//}


pair<globals_t, argvs_t>
dyn_debug_state_t::dyn_debug_state_read_globals_argc_argv(void)
{
  string line;
  getline(cin, line);
  stringstream iss(line);
  string prefix;
  int argc;
  iss >> prefix;
  globals_t globals;
  //dbgs() << "prefix = " << prefix << ".\n";
  while (prefix != "argc:") {
    ASSERT(prefix.at(prefix.size() - 1) == ':');
    string global_name = prefix.substr(0, prefix.size() - 1);
    long global_addr;
    iss >> hex >> global_addr >> dec;
    getline(cin, line);
    iss.str(line);
    iss.clear();
    iss >> prefix;
    globals.m.insert(make_pair(global_name, global_addr));
  }
  ASSERT(prefix == "argc:");
  iss >> argc;
  ASSERT(argc >= 1);
  getline(cin, line);
  iss.str(line);
  iss.clear();
  iss >> prefix;
  //dbgs() << "prefix = " << prefix << ".\n";
  ASSERT(prefix == "argv:");
  dst_ulong argv_addr;
  iss >> hex >> argv_addr >> dec;
  vector<pair<dst_ulong, string>> argv_values;
  for (int i = 0; i < argc; i++) {
    getline(cin, line);
    iss.str(line);
    iss.clear();
    char ch;
    iss >> prefix;
    iss >> ch;
    ASSERT(ch == '=');
    dst_ulong arg_addr;
    iss >> hex >> arg_addr >> dec;
    iss >> ch;
    ASSERT(ch == ':');
    string arg_str;
    iss >> arg_str;
    ASSERT(arg_str.size() > 2);
    ASSERT(arg_str.at(0) == '"');
    ASSERT(arg_str.at(arg_str.size() - 1) == '"');
    arg_str = arg_str.substr(1, arg_str.size() - 2);
    argv_values.push_back(make_pair(arg_addr, arg_str));
  }
  argvs_t argvs;
  argvs.argv_addr = argv_addr;
  argvs.argv_values = argv_values;
  return make_pair(globals, argvs);
}
#endif
