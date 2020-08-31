#ifndef LLVM_EXECUTIONENGINE_LOCKSTEP_H
#define LLVM_EXECUTIONENGINE_LOCKSTEP_H
#include "rewrite/config-all.h"
#define ARCH_SRC ARCH_ETFG
#define ARCH_DST ARCH_I386
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
//#include "../lib/Analysis/Superopt/dfa_helper.h"
#include "gsupport/pc.h"
#include <sys/mman.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <list>
#include <stack>
//#include "Common.h"



//using namespace llvm;
using namespace std;

//#define NOT_IMPLEMENTED() do { dbgs() << __func__ << " " << __LINE__ << ": not-implemented\n"; assert(false); } while (0)
//#define NOT_REACHED() do { dbgs() << __func__ << " " << __LINE__ << ": not-reached\n"; assert(false); } while (0)
//#define ASSERT assert
//#define PC_SUBINDEX_FIRST_INSN_IN_BB 1
//#define DYN_DEBUG_STATE_END_OF_STATE_MARKER "--"
#define DD_LLVM_SRC_PREFIX "exr0.src.llvm-"
//#define LLVM_BASICBLOCK_COUNTING_IDX_START 0x100000
//#define BYTE_LEN 8
//#define MAKE_MASK(n) (((n) == 64)?0xffffffffffffffffULL:(1ULL << (n)) - 1)

typedef uint32_t dst_ulong;

//extern void *mem32;
//extern void *mem32_start;
//const size_t mem32_size = 0x100000000ULL;

/*static inline bool
myStringIsInteger(string const &s)
{
  if (   s.empty()
      || (   (!isdigit(s[0]))
          && (s[0] != '-') && (s[0] != '+'))) {
    return false ;
  }
  char *p;
  strtol(s.c_str(), &p, 10) ;
  return (*p == 0) ;
}*/



struct llvm_pc_t {
  static llvm_pc_t cur_llvm_pc;

  llvm_pc_t(/*BasicBlock *in_bbl*/) : m_pc()//pc_index(0), pc_subindex(0), pc_subsubindex(0)/*, bbl(in_bbl)*/
  {
  }
  llvm_pc_t(eqspace::pc const &p) : m_pc(p) {}
  static llvm_pc_t bbl_entry_pc(string const& bblname/*BasicBlock &bbl*/)
  {
    //llvm_pc_t ret(eqspace::pc(eqspace::pc::insn_label, get_basicblock_name_without_prefix(bbl).c_str(), PC_SUBINDEX_FIRST_INSN_IN_BB, PC_SUBSUBINDEX_DEFAULT));
    llvm_pc_t ret(eqspace::pc(eqspace::pc::insn_label, bblname.c_str(), PC_SUBINDEX_FIRST_INSN_IN_BB, PC_SUBSUBINDEX_DEFAULT));
    //ret.pc_index = get_basicblock_index(bbl);
    //ret.pc_subindex = PC_SUBINDEX_FIRST_INSN_IN_BB;
    //ret.pc_subsubindex = 0;
    return ret;
  }

  //static string
  //get_basicblock_name_without_prefix(BasicBlock const& v)
  //{
  //  string ret = get_basicblock_name(v);
  //  ASSERT(ret.substr(0, 1) == "%");
  //  ret = ret.substr(1);
  //  return ret;
  //}

  string to_string() const
  {
    //stringstream ss;
    //ss << "L" << pc_index << "." << pc_subindex << "." << pc_subsubindex;
    //return ss.str();
    return m_pc.to_string();
  }
  //static int
  //get_basicblock_index(const llvm::BasicBlock &v)
  //{
  //  int counting_index = get_counting_index_for_basicblock(v);
  //  string s = get_basicblock_name(v);
  //  ASSERT(s.substr(0, 1) == "%");
  //  string s1 = s.substr(1);
  //  int ret;
  //  if (stringIsInteger(s1)) {
  //    ret = stoi(s1);
  //    if (counting_index == 0 && ret != 0) {
  //      ret = counting_index;
  //    }
  //  } else {
  //    ret = counting_index;
  //    if (ret != 0) {
  //      ret += LLVM_BASICBLOCK_COUNTING_IDX_START;
  //    }
  //  }
  //  return ret;
  //}
  llvm_pc_t get_next_pc(/*BasicBlock *next_bbl*/) const
  {
    llvm_pc_t ret = *this;
    //ret.pc_subindex++;
    //ret.pc_subsubindex = 0;
    ret.m_pc.set_subindex(ret.m_pc.get_subindex() + 1);
    ret.m_pc.set_subsubindex(PC_SUBSUBINDEX_DEFAULT);
    return ret;
  }
  //bool matches(int index, int subindex, int subsubindex) const
  //{
  //  return    index == pc_index
  //         && subindex == pc_subindex
  //         && subsubindex == pc_subsubindex;
  //}
  bool matches(eqspace::pc const &p) const
  {
    return p.get_type() == m_pc.get_type() && p.get_index() == m_pc.get_index() && p.get_subindex() == m_pc.get_subindex(); //ignore subsubindex
    //return m_pc == p;
  }
  bool equals(llvm_pc_t const &other) const
  {
    //return matches(other.pc_index, other.pc_subindex, other.pc_subsubindex);
    return matches(other.m_pc);
  }
  bool operator<(llvm_pc_t const &other) const
  {
    return m_pc < other.m_pc;
    //return    this->pc_index < other.pc_index
    //       || (   this->pc_index == other.pc_index
    //           && this->pc_subindex < other.pc_subindex)
    //       || (   this->pc_index == other.pc_index
    //           && this->pc_subindex == other.pc_subindex
    //           && this->pc_subsubindex < other.pc_subsubindex
    //          );
  }
  bool is_uninitialized() const { return m_pc.get_index() == nullptr; }
  bool is_bbl_entry() const { return m_pc.get_subindex() == PC_SUBINDEX_FIRST_INSN_IN_BB; }
private:
  //int pc_index, pc_subindex, pc_subsubindex;
  eqspace::pc m_pc;
  //BasicBlock *bbl;
};

struct llvm_pc_stack_t {
  static llvm_pc_stack_t llvm_pc_stack;

  void push(llvm_pc_t const &llvm_pc)
  {
    //cout << __func__ << " " << __LINE__ << ": llvm_pc_stack pushing " << llvm_pc.to_string() << endl;
    m_stack.push(llvm_pc);
  }
  llvm_pc_t pop()
  {
    //cout << __func__ << " " << __LINE__ << ": pop called\n";
    llvm_pc_t ret = m_stack.top();
    //cout << __func__ << " " << __LINE__ << ": popping " << ret.to_string() << endl;
    m_stack.pop();
    //cout << __func__ << " " << __LINE__ << ": popped " << ret.to_string() << endl;
    return ret;
  }
private:
  stack<llvm_pc_t> m_stack;
};

class GenericValue
{
private:
  uint64_t m_val;
  int m_nbits;
public:
  GenericValue(uint64_t val, int nbits) : m_val(val), m_nbits(nbits)
  { }
  void *getPtrVal() const { return (void *)m_val; }
  uint64_t getIntVal() const { return m_val; }
  void setIntVal(uint64_t v) { m_val = v; }
  int get_nbits() const { return m_nbits; }
};

class ExecutionContext
{
private:
  string m_fname;
  string m_cur_bb_name;
  long   m_cur_insn_num;
  map<string, pair<TypeID, GenericValue>> m_vals;    //LLVM values used in this invocation
  std::vector<GenericValue> m_varargs; //Values passed through an ellipsis
  std::vector<void *> m_allocas;       //LLVM allocas
public:
  ExecutionContext(vector<GenericValue> const& varargs) : m_varargs(varargs)
  { }
  void setFname(string const& fun_name) { m_fname = fun_name; }
  string const& getFname() const { return m_fname; }
  void setBB(string const& bbname) { m_cur_bb_name = bbname; m_cur_insn_num = 0; }
  std::vector<void *>& get_allocas() { return m_allocas; }
  map<string, pair<TypeID, GenericValue>> const& get_values() const { return m_vals; }
  map<string, pair<TypeID, GenericValue>>& get_values_ref() { return m_vals; }
  void register_update(char const* name, TypeID typID, int nbits, uint64_t val);
};




struct dyn_debug_state_elem_t
{
  map<string, uint64_t> ddmap;
  map<void *, uint8_t> ddmemory;
  //int dd_pc_index, dd_pc_subindex, dd_pc_subsubindex;
  eqspace::pc dd_pc_tfg;
  int dd_pc;
  long long icount;

  static bool dd_llvm_values_match(uint64_t ddval, GenericValue const &llvm_val, TypeID typID);
  static void dd_llvm_value_update(uint64_t ddval, GenericValue &llvm_val, TypeID typID);
  bool match_execution_context(ExecutionContext &SF) const;
  string to_string() const;
  void read_pc_indices_from_pc_line(string const &line);
  static void *compute_addr_using_ddmap_and_key(map<string, uint64_t> const &ddmap, string const &key, size_t &len);
  void add_mapping_using_line(string const &line);
  static dyn_debug_state_elem_t read_from_input();
  bool is_surely_a_sync_point() const;
  string pc_to_string() const;
  string icount_to_string() const;
  uint64_t get_value_for_key(string const &key) const;
  bool matches_llvm_pc(llvm_pc_t const &llvm_pc) const;
};

struct argvs_t {
  dst_ulong argv_addr;
  vector<pair<dst_ulong, string>> argv_values;
};

struct globals_t {
  map<string, dst_ulong> m;
};

struct dyn_debug_state_t
{
  static dyn_debug_state_t dyn_debug_state;
  string pc_list_to_string() const;
  string icount_list_to_string() const;
  bool dd_matches_pc(llvm_pc_t const &llvm_pc) const;
  void dd_match_execution_context_at_pc(ExecutionContext &SF, llvm_pc_t const &llvm_pc) const;
  void clear_till_pc(llvm_pc_t const &llvm_pc);
  bool contains_sync_point() const;
  void populate_from_input();
  uint64_t get_value_for_key_at_pc(string const &key, llvm_pc_t const &llvm_pc);
  static pair<globals_t, argvs_t> dyn_debug_state_read_globals_argc_argv(void);

private:
  list<dyn_debug_state_elem_t> q;
};

class llvm_machine_state_t
{
public:
  static stack<ExecutionContext> ECStack;
};

std::string getTraceDB();
bool getLockstep();
string genFreqHist();

void registerCoverageAtPC();

//class malloc_map_t
//{
//private:
//  map<void *, pair<void *, size_t>> m_map;
//public:
//  static malloc_map_t malloc_map;
//  void malloc_map_add_mapping(void *replaced, void *orig, size_t sz);
//  void *get_mapping(void *replaced);
//  //static bool is_malloc_family_function_for_alloc(Function *F);
//  //static bool is_malloc_family_function_for_free(Function *F);
//};

//class memory_regions_t
//{
//private:
//  map<void *, pair<Type *, size_t>> m_map;
//public:
//  static memory_regions_t memory_regions;
//  void memory_regions_add_mapping(void *addr, Type *typ, size_t sz);
//  void memory_region_remove(void *addr);
//  void *extend_replaced_value_if_needed(void *replaced);
//  //map<void *, pair<Type *, size_t>> const &get_map() const;
//};

//void *malloc_at_addr(void *addr, Type *typ, size_t size);
//inline void *MY_GVTOP(GenericValue const &GV)
//{
//  void *ret = GVTOP(GV);
//  //cout << __func__ << " " << __LINE__ << ": before ret = " << ret << endl;
//  ret = memory_regions_t::memory_regions.extend_replaced_value_if_needed(ret);
//  //cout << __func__ << " " << __LINE__ << ": after ret = " << ret << endl;
//  return ret;
//}





#endif
