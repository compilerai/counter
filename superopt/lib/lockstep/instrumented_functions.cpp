#include <chrono>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <stack>
#include "lockstep/instrumented_functions.h"
#include "lockstep/Lockstep.h"
#include "lockstep/TraceDB.h"
#include "support/dyn_debug.h"
#include "lockstep/Common.h"
using namespace std;

class frequency_histogram_t
{
private:
  string m_filename;
  map<string, long long> m_counts;
  map<pair<string, string>, long long> m_edge_counts;
  string m_last_pc_string;
  struct greater_count {
    bool operator()(pair<string, long long> const &a, pair<string, long long> const &b) const
    {
      return a.second > b.second;
    }
  };
  struct edge_greater_count {
    bool operator()(pair<pair<string, string>, long long> const &a, pair<pair<string, string>, long long> const &b) const
    {
      return a.second > b.second;
    }
  };

public:
  static frequency_histogram_t* frequency_histogram;

  frequency_histogram_t(string const &filename) : m_filename(filename)
  {
    m_last_pc_string = "PROGRAM_START";
  }
  void register_execution(string const &function_name, llvm_pc_t const &llvm_pc)
  {
    string key = function_name + ":" + llvm_pc.to_string();
    if (m_counts.count(key)) {
      m_counts.at(key)++;
    } else {
      m_counts.insert(make_pair(key, 1));
    }
    pair<string, string> edge_string = make_pair(m_last_pc_string, key);
    if (m_edge_counts.count(edge_string)) {
      m_edge_counts.at(edge_string)++;
    } else {
      m_edge_counts.insert(make_pair(edge_string, 1));
    }
    m_last_pc_string = key;
  }
  void output()
  {
    vector<pair<string, long long>> counts(m_counts.begin(), m_counts.end());
    std::sort(counts.begin(), counts.end(), greater_count());
    ofstream ofs(m_filename);
    //dbgs() << ": opened freqhist file" << m_filename << "\n";
    ofs << "Printing pc counts:\n";
    for (const auto &c : counts) {
      //dbgs() << ": output\n";
      ofs << c.first << ": " << c.second << endl;
    }
    //dbgs() << ": closed freqhist file\n";
    vector<pair<pair<string, string>, long long>> ecounts(m_edge_counts.begin(), m_edge_counts.end());
    std::sort(ecounts.begin(), ecounts.end(), edge_greater_count());
    ofs << "Printing edge counts:\n";
    for (const auto &c : ecounts) {
      //dbgs() << ": output\n";
      ofs << c.first.first << " -> " << c.first.second << " : " << c.second << endl;
    }
    ofs.close();
  }
};

frequency_histogram_t* frequency_histogram_t::frequency_histogram;

extern "C"
void lockstep_register_fun(char const* fun_name, int64_t is_small_function)
{
  CPP_DBG_EXEC(LOCKSTEP, cout << __func__ << " " << __LINE__ << ": " << fun_name << ": entry\n");
  if (!strcmp(fun_name, "main")) {
    if (getTraceDB() != "") {
      tracedb_t::tracedb = make_unique<tracedb_t>(getTraceDB());
    }
    if (getLockstep()) {
      //BasicBlock *lastBB = NULL;
      dyn_debug_state_t::dyn_debug_state.populate_from_input();
    }
    if (genFreqHist() != "") {
      frequency_histogram_t::frequency_histogram = new frequency_histogram_t(genFreqHist());
    }
    llvm_machine_state_t::ECStack.push(ExecutionContext(vector<GenericValue>()));
  }
  ASSERT(llvm_machine_state_t::ECStack.size() > 0);
  llvm_machine_state_t::ECStack.top().setFname(fun_name);
  llvm_pc_t::cur_llvm_pc = pc::start();
  dyn_debug_state_t::dyn_debug_state.populate_from_input();
  beginTracePoint();
}

extern "C"
void lockstep_register_bbl(char const* bbl_name, int64_t is_small_function)
{
  CPP_DBG_EXEC(LOCKSTEP, cout << __func__ << " " << __LINE__ << ": " << bbl_name << ": entry\n");
  //llvm_pc_t::cur_llvm_pc = llvm_pc_t::bbl_entry_pc(*SF.CurBB);
  ASSERT(bbl_name[0] == '%');
  llvm_pc_t::cur_llvm_pc = llvm_pc_t::bbl_entry_pc(&bbl_name[1]);
  lockstep_register_inst("", IntegerTyID, 0, LLVM_LOCKSTEP_DEFAULT_VOID_VAL, llvm_insn_bblentry, 0, nullptr, nullptr);
  CPP_DBG_EXEC(LOCKSTEP, cout << __func__ << " " << __LINE__ << ": after calling register_inst for bbl " << bbl_name << ": pc = " << llvm_pc_t::cur_llvm_pc.to_string() << "\n");
}

extern "C"
void lockstep_register_fcall()
{
  //populate varargs using the call instruction
  vector<GenericValue> varargs;
  //for (int i = 0; i < nargs; i++) {
  //  varargs.push_back(GenericValue(args[i], arg_nbits[i]));
  //}
  llvm_machine_state_t::ECStack.push(ExecutionContext(varargs));
  //auto retaddr = llvm_pc_t::cur_llvm_pc.get_next_pc();
  auto retaddr = llvm_pc_t::cur_llvm_pc.is_bbl_entry() ? llvm_pc_t::cur_llvm_pc.get_next_pc() : llvm_pc_t::cur_llvm_pc;
  CPP_DBG_EXEC(LOCKSTEP, cout << __func__ << " " << __LINE__ << ": pushing retaddr llvm pc = " << retaddr.to_string() << "\n");
  llvm_pc_stack_t::llvm_pc_stack.push(retaddr);
}

extern "C"
void lockstep_register_inst(char const* name, int64_t typID_i, int64_t nbits_i, uint64_t val, int64_t t_i, int64_t nargs_i, uint64_t const* args, uint64_t const* arg_nbits)
{
  TypeID typID = (TypeID)typID_i;
  int nbits = (int)nbits_i;
  llvm_insn_type_t t = (llvm_insn_type_t)t_i;
  int nargs = (int)nargs_i;
  if (llvm_pc_t::cur_llvm_pc.is_bbl_entry() && t != llvm_insn_phinode && t != llvm_insn_bblentry) {
    llvm_pc_t::cur_llvm_pc = llvm_pc_t::cur_llvm_pc.get_next_pc(); //if this is the first non-phi instruction in the BB, increment the llvm pc
  }
  CPP_DBG_EXEC(LOCKSTEP, cout << __func__ << " " << __LINE__ << ": " << llvm_pc_t::cur_llvm_pc.to_string() << ": " << name << ": t = " << llvm_insn_type_to_string(t) << ", nargs = " << nargs << ": val = 0x" << hex << val << dec << ": entry\n");
  llvm_pc_t prev_llvm_pc = llvm_pc_t::cur_llvm_pc;
  ASSERT(llvm_machine_state_t::ECStack.size() > 0);
  ExecutionContext& SF = llvm_machine_state_t::ECStack.top();
  if (strlen(name) > 0) {
    SF.register_update(name, typID, nbits, val);
  }

  if (t == llvm_insn_alloca) {
    void *Memory;
    if (getLockstep() || getTraceDB() != "") {
      if (getLockstep()) {
        Memory = (void *)dyn_debug_state_t::dyn_debug_state.get_value_for_key_at_pc(name, llvm_pc_t::cur_llvm_pc.get_next_pc(/*SF.CurBB*/));
      } else if (getTraceDB() != "") {
        ASSERT(nargs == 1);
        NOT_IMPLEMENTED(); //Memory = args[0];
        //Memory = tracedb_t::tracedb->locals_malloc(/*I.getType(), */args[0]);
      } else NOT_REACHED();
      //Memory = malloc_at_addr(Memory, I.getType(), MemToAlloc);
    } else {
      Memory = (void*)val;
    }
    llvm_machine_state_t::ECStack.top().get_allocas().push_back(Memory);
  } else if (t == llvm_insn_call) {
    llvm_pc_t::cur_llvm_pc = llvm_pc_stack_t::llvm_pc_stack.pop();
    CPP_DBG_EXEC(LOCKSTEP, cout << __func__ << " " << __LINE__ << ": after stack pop, new llvm pc = " << llvm_pc_t::cur_llvm_pc.to_string() << "\n");
    llvm_machine_state_t::ECStack.pop();
  }


  if (getLockstep()) {
    //dbgs() << __LINE__ << ": cur_llvm_pc = " << cur_llvm_pc.to_string() << "\n";

    //LLVM_DEBUG(dbgs() << "About to interpret: " << I);
    if (dyn_debug_state_t::dyn_debug_state.dd_matches_pc(llvm_pc_t::cur_llvm_pc)) {
      if (t != llvm_insn_bblentry && t != llvm_insn_phinode) {
        dyn_debug_state_t::dyn_debug_state.dd_match_execution_context_at_pc(SF, llvm_pc_t::cur_llvm_pc);
      }
        //dbgs() << "Found mismatch at " << llvm_pc_t::cur_llvm_pc.to_string() << "\n";
        //dbgs() << "dyn_debug_state pc list = " << dyn_debug_state_t::dyn_debug_state.pc_list_to_string() << "\n";
      dyn_debug_state_t::dyn_debug_state.clear_till_pc(llvm_pc_t::cur_llvm_pc);
      dyn_debug_state_t::dyn_debug_state.populate_from_input();
      //num_llvm_insns_skipped = 0;
    } else {
      if (t != llvm_insn_bblentry && t != llvm_insn_farg && t != llvm_insn_phinode/* && (num_llvm_insns_skipped % 1) == 0*/)
      {
        cout << "Warning: skipped insn without a sync point for " << dyn_debug_state_t::dyn_debug_state.pc_list_to_string() << ": llvm pc = " << llvm_pc_t::cur_llvm_pc.to_string() << "\n";
      }
      //num_llvm_insns_skipped++;
    }
  }

  if (genFreqHist() != "") {
    //Function *curF = llvm_machine_state_t::ECStack.size() == 0 ? NULL : llvm_machine_state_t::ECStack.back().CurFunction;
    string curF = llvm_machine_state_t::ECStack.size() == 0 ? "unknown" : llvm_machine_state_t::ECStack.top().getFname();
    //string name = curF ? curF->getName() : "unknown";
    ASSERT(frequency_histogram_t::frequency_histogram);
    frequency_histogram_t::frequency_histogram->register_execution(name, llvm_pc_t::cur_llvm_pc);
  }
  if (/*llvm_pc_t::cur_llvm_pc.equals(prev_llvm_pc) && */t != llvm_insn_bblentry && t != llvm_insn_farg && t != llvm_insn_phinode/* && t != llvm_insn_bblentry*/) {
    llvm_pc_t::cur_llvm_pc = llvm_pc_t::cur_llvm_pc.get_next_pc();
  }

  DYN_DEBUG(LOCKSTEP,
     if (t != llvm_insn_call && t != llvm_insn_invoke &&
         name != nullptr) {
       cout << name << "  --> ";
       const GenericValue &Val = SF.get_values().at(name).second;
       cout << GenericValToString(Val, typID) << "\n";
     });
  registerCoverageAtPC();
}

extern "C"
void lockstep_clear_fun(int64_t is_main, int64_t is_small_function)
{
  CPP_DBG_EXEC(LOCKSTEP, cout << __func__ << " " << __LINE__ << ": " << llvm_pc_t::cur_llvm_pc.to_string() << "\n");
  if (llvm_machine_state_t::ECStack.empty()) {
    if (genFreqHist() != "") {
      //dbgs() << "outputting frequency histogram\n";
      ASSERT(frequency_histogram_t::frequency_histogram);
      frequency_histogram_t::frequency_histogram->output();
    }
    if (tracedb_t::tracedb) {
      tracedb_t::tracedb->tracedb_print();
    }
  }
  //for (auto ptr : llvm_machine_state_t::ECStack.top().get_allocas()) {
  //  if (getTraceDB() != "") {
  //    tracedb_t::tracedb->locals_free(ptr);
  //  }
  //}
}
