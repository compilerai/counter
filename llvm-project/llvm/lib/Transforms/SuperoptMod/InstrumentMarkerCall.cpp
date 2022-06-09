#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "support/debug.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/Dominators.h"
#include "tfg/tfg.h"
#include "tfg/tfg_llvm.h"
#include "graph/dfa.h"
#include "../lib/Analysis/Superopt/dfa_helper.h"
#include <sstream>

using namespace llvm;
using namespace std;
using namespace eqspace;

namespace {

using loop_depth_t = int;

static BasicBlock *
getBBfromName(Function *F, char const *bbname)
{
  for (auto& BB : *F) {
    if (get_basicblock_name(BB) == string("%") + bbname) {
      return &BB;
    }
  }
  dbgs() << __func__ << " " << __LINE__ << ": bbname = " << bbname << "\n";
  errs() << __func__ << " " << __LINE__ << ": bbname = " << bbname << "\n";
  dbgs().flush();
  errs().flush();
  assert(false); //not-reached
}


class distance_t
{
private:
  size_t m_val;
  bool m_is_top;
public:
  distance_t(size_t v) : m_val(v), m_is_top(false)
  { }
  size_t get() const { ASSERT(!m_is_top); return m_val; }
  void set(size_t v) { ASSERT(!m_is_top); m_val = v; }
  static distance_t top()
  {
    distance_t ret(0);
    ret.m_is_top = true;
    return ret;
  }
  bool meet(distance_t const& other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    if (m_val < other.m_val) {
      m_val = other.m_val;
      return true;
    }
    return false;
  }
  bool is_top() const { return m_is_top; }
  string to_string() const
  {
    if (m_is_top) {
      return "TOP";
    }
    stringstream ss;
    ss << m_val;
    return ss.str();
  }
};

template<dfa_dir_t DFA_DIR>
class distance_dfa_t : public data_flow_analysis<pc, tfg_node, tfg_edge, distance_t, DFA_DIR>
{
public:
  distance_dfa_t(dshared_ptr<tfg const> t, map<pc, distance_t>& init_vals)
      : data_flow_analysis<pc, tfg_node, tfg_edge, distance_t, DFA_DIR>(t, init_vals)
  { }
  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<tfg_edge const> const& e, distance_t const& in, distance_t &out) override
  {
    distance_t in_xfer = in;
    if (e->get_to_state().contains_function_call()) {
      in_xfer.set(0);
    } else {
      in_xfer.set(in_xfer.get() + 1);
    }
    return out.meet(in_xfer) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

class value_version_t
{
private:
  Value *m_I;
  BasicBlock *m_BB;
  bool m_is_phi; //m_is_phi is true IFF m_BB is non-null IFF m_I is null
public:
  value_version_t(Value *I, BasicBlock *BB, bool is_phi) : m_I(I), m_BB(BB), m_is_phi(is_phi)
  { }
  string to_string() const
  {
    if (m_is_phi) {
      return string("PHI ") + get_basicblock_name(*m_BB);
    } else {
      return sym_exec_common::get_value_name_using_srcdst_keyword(*m_I, G_SRC_KEYWORD);
    }
  }
  BasicBlock *getBB() const { return m_BB; }
  Value *getI() const { return m_I; }
  bool is_phi() const { return m_is_phi; }
  bool equals(value_version_t const& other) const
  {
    if (this->m_is_phi != other.m_is_phi) {
      return false;
    }
    if (this->m_is_phi) {
      assert(!m_I);
      assert(!other.m_I);
      return this->m_BB == other.m_BB;
    } else {
      assert(!m_BB);
      assert(!other.m_BB);
      return this->m_I == other.m_I;
    }
  }
};

class value_version_map_t
{
private:
  map<Value *, value_version_t> m_map;
  bool m_is_top;
public:
  value_version_map_t() : m_is_top(false)
  { }
  map<Value*, value_version_t> const& get_map() const { return m_map; }
  static value_version_map_t top()
  {
    value_version_map_t ret;
    ret.m_is_top = true;
    return ret;
  }
  string to_string() const
  {
    if (m_is_top) {
      return "TOP\n";
    } else {
      stringstream ss;
      for (auto const& m : m_map) {
        ss << sym_exec_common::get_value_name_using_srcdst_keyword(*m.first, G_SRC_KEYWORD) << " -> " << m.second.to_string() << "\n";
      }
      return ss.str();
    }
  }
  void add(Value *v, Value *I)
  {
    if (m_map.count(v)) {
      m_map.erase(v);
    }
    m_map.insert(make_pair(v, value_version_t(I, nullptr, false)));
  }
  bool meet(value_version_map_t const& other, BasicBlock *BB)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    bool changed = false;
    for (auto const& m : other.m_map) {
      if (!this->m_map.count(m.first)) { //ignore this because it is not available on all paths; please note that this does not mean that phi nodes will not get updated; this also means that do not maintain a substitution map for values that are not available on all paths because they will never be used at this point (SSA property)
        //m_map.insert(m);
        //changed = true;
      } else if (!this->m_map.at(m.first).equals(m.second)) {
        m_map.at(m.first) = value_version_t(nullptr, BB, true);
        changed = true;
      }
    }
    for (auto const& m : this->m_map) {
      if (!other.m_map.count(m.first)) {
        this->m_map.erase(m.first); //erase this because it is not available on all paths
        changed = true;
      }
    }
    return changed;
  }
};

class valversion_dfa_t : public data_flow_analysis<pc, tfg_node, tfg_edge, value_version_map_t, dfa_dir_t::forward>
{
private:
  map<shared_ptr<tfg_edge const>, Instruction *> const& m_eimap;
  vector<Value*> const& m_from;
  vector<Instruction*> const& m_to;
  BasicBlock *m_markerBB;
  Function *m_F;
public:
  valversion_dfa_t(dshared_ptr<tfg const> t, map<shared_ptr<tfg_edge const>, Instruction *> const& eimap, vector<Value*> const& from, vector<Instruction*> const& to, BasicBlock* BB, Function* F, map<pc, value_version_map_t>& init_vals)
      : data_flow_analysis<pc, tfg_node, tfg_edge, value_version_map_t, dfa_dir_t::forward>(t, init_vals), m_eimap(eimap), m_from(from), m_to(to), m_markerBB(BB), m_F(F)
  {
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
        dbgs() << __func__ << " " << __LINE__ << ": DFA initialized\n";
        dbgs() << "from_values =";
        for (auto f : from) {
          dbgs() << " " << sym_exec_common::get_value_name_using_srcdst_keyword(*f, G_SRC_KEYWORD);
        }
        dbgs() << "\n";
        dbgs() << "to_values =";
        for (auto f : to) {
          dbgs() << " " << sym_exec_common::get_value_name_using_srcdst_keyword(*f, G_SRC_KEYWORD);
        }
        dbgs() << "\n";
    );
    ASSERT(m_from.size() == m_to.size());
  }
  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<tfg_edge const> const& e, value_version_map_t const& in, value_version_map_t &out) override
  {
    pc const& to_p = e->get_to_pc();
    if (to_p.is_exit()) {
      return DFA_XFER_RETVAL_UNCHANGED;
    }
    value_version_map_t in_xfer = in;
    //dbgs() << __func__ << " " << __LINE__ << ": e = " << e->to_string() << "\n";
    BasicBlock* toBB = getBBfromName(m_F, to_p.get_index());
    if (m_eimap.count(e.get_shared_ptr())) {
      Instruction* I = m_eimap.at(e.get_shared_ptr());
      CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
          if (!I->getType()->isVoidTy()) {
            dbgs() << __func__ << " " << __LINE__ << ": I = " << sym_exec_common::get_value_name_using_srcdst_keyword(*I, G_SRC_KEYWORD) << "\n";
          }
      );
      if (vector_contains(m_from, (Value *)I)) {
        CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
            dbgs() << __func__ << " " << __LINE__ << ": adding " << sym_exec_common::get_value_name_using_srcdst_keyword(*I, G_SRC_KEYWORD) << " (identity mapping)\n";
        );
        in_xfer.add(I, I);
      } else if (vector_contains(m_to, I)) {
        size_t i = vector_find(m_to, I);
        in_xfer.add(m_from.at(i), m_to.at(i));
        CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
            dbgs() << __func__ << " " << __LINE__ << ": i = " << i << ", adding " << sym_exec_common::get_value_name_using_srcdst_keyword(*m_from.at(i), G_SRC_KEYWORD) << " -> " << sym_exec_common::get_value_name_using_srcdst_keyword(*m_to.at(i), G_SRC_KEYWORD) << "\n";
        );
      }
    }
    bool changed = out.meet(in_xfer, toBB);
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
        if (changed) {
          dbgs() << __func__ << " " << __LINE__ << ": e = " << e->to_string() << "\n";
          dbgs() << ": changed vals at to_pc " << e->get_to_pc().to_string() << " to:\n";
          dbgs() << out.to_string() << "\n";
        }
    );
    return changed ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

class liveness_val_t
{
private:
  list<Value *> m_vals;
  bool m_is_top;
public:
  liveness_val_t() : m_is_top(false)
  { }
  static liveness_val_t top() { auto ret = liveness_val_t(); ret.m_is_top = true; return ret; }
  bool meet(liveness_val_t const& other)
  {
    ASSERT(!other.m_is_top);
    if (m_is_top) {
      *this = other;
      return true;
    }
    auto old_vals = m_vals;
    list_union_append<Value *>(m_vals, other.m_vals);
    return old_vals != m_vals;
  }
  void remove(Value *v)
  {
    auto old_vals = m_vals;
    m_vals.clear();
    for (auto lv : old_vals) {
      if (lv == v) {
        continue;
      }
      m_vals.push_back(lv);
    }
  }
  void add(Value *v)
  {
    if (!list_contains(m_vals, v)) {
      m_vals.push_back(v);
    }
  }
  list<Value *> const& get_vals() const { return m_vals; }
};

class liveness_dfa_t : public data_flow_analysis<pc, tfg_node, tfg_edge, liveness_val_t, dfa_dir_t::backward>
{
private:
  map<shared_ptr<tfg_edge const>, Instruction *> const& m_eimap;
public:
  liveness_dfa_t(dshared_ptr<tfg const> t, map<shared_ptr<tfg_edge const>, Instruction *> const& eimap, map<pc, liveness_val_t>& init_vals)
      : data_flow_analysis<pc, tfg_node, tfg_edge, liveness_val_t, dfa_dir_t::backward>(t, init_vals), m_eimap(eimap)
  { }
  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<tfg_edge const> const& e, liveness_val_t const& in, liveness_val_t &out) override
  {
    liveness_val_t in_xferred = in;
    CPP_DBG_EXEC2(LLVM_LIVENESS, cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << endl);
    pc const& from_pc = e->get_from_pc();
    char const* from_name = from_pc.get_index();
    if (m_eimap.count(e.get_shared_ptr())) {
      Instruction * I = m_eimap.at(e.get_shared_ptr());
      if (isa<PHINode const>(I)) {
        PHINode const *phi = dyn_cast<PHINode const>(I);
        ASSERT(phi);
        for (int i = 0; i < phi->getNumIncomingValues(); i++) {
          BasicBlock const *inblock = phi->getIncomingBlock(i);
          string inblockname = get_basicblock_name(*inblock);
          ASSERT(inblockname.at(0) == '%');
          inblockname = inblockname.substr(1);
          if (inblockname != from_name) {
            CPP_DBG_EXEC2(LLVM_LIVENESS, cout << __func__ << " " << __LINE__ << ": inblockname = " << inblockname << ", from_name = " << from_name << ".\n");
            continue;
          }
          Value *o = phi->getIncomingValue(i);
          if (isa<const Argument>(o) || isa<const Instruction>(o)) {
            CPP_DBG_EXEC2(LLVM_LIVENESS, cout << __func__ << " " << __LINE__ << ": adding value " << sym_exec_common::get_value_name_using_srcdst_keyword(*o, G_SRC_KEYWORD) << endl);
            in_xferred.add(o);
          }
        }
      } else {
        for (int i = 0; i < I->getNumOperands(); i++) {
          Value *o = I->getOperand(i);
          if (isa<const Argument>(o) || isa<const Instruction>(o)) {
            CPP_DBG_EXEC2(LLVM_LIVENESS, cout << __func__ << " " << __LINE__ << ": adding value " << sym_exec_common::get_value_name_using_srcdst_keyword(*o, G_SRC_KEYWORD) << endl);
            in_xferred.add(o);
          }
        }
      }
      if (!I->getType()->isVoidTy()) {
        CPP_DBG_EXEC2(LLVM_LIVENESS, cout << __func__ << " " << __LINE__ << ": removing value " << sym_exec_common::get_value_name_using_srcdst_keyword(*I, G_SRC_KEYWORD) << endl);
        in_xferred.remove(I);
      }
    }
    return out.meet(in_xferred) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

class IdentifyUnbrokenLoop : public FunctionPass {
private:
  Module *M;
  Function *m_function;
  BasicBlock *m_BB;
  int m_insn_num;
public:
  static char ID;
  IdentifyUnbrokenLoop() : FunctionPass(ID), M(nullptr), m_function(nullptr), m_BB(nullptr), m_insn_num(-1)
  {
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
        dbgs() << "IdentifyUnbrokenLoop() constructor: ID = " << int(ID) << ", getPassID() = " << this->getPassID() << "\n";
    );
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override
  {
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
        dbgs() << "IdentifyUnbrokenLoop() getAnalysisUsage: ID = " << int(ID) << ", getPassID() = " << this->getPassID() << "\n";
    );
    AU.addRequired<LoopInfoWrapperPass>();
    AU.setPreservesAll();
  }

  virtual bool doInitialization(Module& m) override;

  virtual bool runOnFunction(Function &F) override;
  Function* get_function() const { return m_function; }
  BasicBlock* getBB() const { return m_BB; }
  int getInum() const { return m_insn_num; }

  StringRef getPassName() const { return "Identify unbroken loop"; }
private:
  Loop *identifyUnbrokenLoop(Function& F);
  void unbrokenLoopVisit(Loop *L, loop_depth_t cur_depth, loop_depth_t &depth_out, Loop* &loop_out);
};

class IdentifyMaxDistancePC : public FunctionPass {
private:
  Module *M;
  Function *m_function;
  BasicBlock *m_BB;
  int m_insn_num;
  distance_t m_max_distance;
  map<Function *, map<pc, distance_t>> fwd_distances, bwd_distances;
public:
  static char ID;
  IdentifyMaxDistancePC() : FunctionPass(ID), M(nullptr), m_function(nullptr), m_BB(nullptr), m_insn_num(-1), m_max_distance(0)
  {
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
        dbgs() << "IdentifyMaxDistancePC() constructor: ID = " << int(ID) << ", getPassID() = " << this->getPassID() << "\n";
    );
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override
  {
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
        dbgs() << "IdentifyMaxDistancePC() getAnalysisUsage: ID = " << int(ID) << ", getPassID() = " << this->getPassID() << "\n";
    );
    AU.addRequired<IdentifyUnbrokenLoop>();
    AU.setPreservesAll();
  }

  virtual bool doInitialization(Module& m) override;

  virtual bool runOnFunction(Function &F) override;
  Function* get_function() const { return m_function; }
  BasicBlock* getBB() const { return m_BB; }
  int getInum() const { return m_insn_num; }

  StringRef getPassName() const { return "Identify max flow PC"; }
private:
  static tuple<pc, distance_t> find_max_distance_pc(map<pc, distance_t> const& fwd_distances, map<pc, distance_t> const& bwd_distances);
  static void retain_only_basic_block_heads(map<pc, distance_t>& distances);
};

class InstrumentMarkerCall : public FunctionPass {
private:
  Module *M;
  //Function *m_function;
  //BasicBlock *m_BB;
  //int m_insn_num;
  Value *m_markerF;
public:
  static char ID;
  InstrumentMarkerCall() : FunctionPass(ID), M(nullptr)/*, m_function(nullptr), m_BB(nullptr), m_insn_num(-1)*/, m_markerF(nullptr)
  {
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
        dbgs() << "InstrumentMarkerCall() constructor: ID = " << int(ID) << ", getPassID() = " << this->getPassID() << "\n";
    );
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override
  {
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
        dbgs() << "InstrumentMarkerCall() getAnalysisUsage: ID = " << int(ID) << ", getPassID() = " << this->getPassID() << "\n";
    );
    //AU.addRequired<IdentifyUnbrokenLoop>();
    //AU.addRequired<IdentifyForwardDistanceFromMarker>();
    //AU.addRequired<IdentifyBackwardDistanceFromMarker>();
    AU.addRequired<IdentifyUnbrokenLoop>();
    AU.addRequired<IdentifyMaxDistancePC>();
    //AU.addRequired<DominatorTreeWrapperPass>();

    //AU.setPreservesAll();
  }

  virtual bool doInitialization(Module& m) override;

  virtual bool runOnFunction(Function &F) override;

  bool markerPresentInBasicBlock(Function *F, BasicBlock *BB);
  void addMarkerInBasicBlock(Function *F, BasicBlock *BB, int insn_subindex, list<Value*> const& live_values, bool breaking_loop);
  Value * getMarkerFunction();
  FunctionType* getMarkerFunctionType(vector<Type*> const& live_types);

  static vector<Type *> values_to_types_vec(list<Value*> const& live_values);
  StringRef getPassName() const { return "Instrument marker call"; }
  //Loop *identifyUnbrokenLoop(Function& F);
  //void unbrokenLoopVisit(Loop *L, loop_depth_t cur_depth, loop_depth_t &depth_out, Loop* &loop_out);
private:
  static list<Value *> get_live_values(dshared_ptr<tfg_llvm_t const> t, map<shared_ptr<tfg_edge const>, Instruction *> const& eimap, Function const * function, BasicBlock const* BB, int insn_num);
  void replaceUsesInBBAfterMarkerCall(Value *from, Value *to, BasicBlock *BB, Instruction *markerCallEnd);
  void replaceGlobalUsesOfValue(vector<Value *> const& from, vector<Instruction *> const& to, BasicBlock *BB, Function *F, dshared_ptr<tfg const> t, map<shared_ptr<tfg_edge const>, Instruction *> const& eimap);

  bool addPhiNodesToBBForValues(BasicBlock* BB, set<Value*> const& new_vvs, map<pc, value_version_map_t> const& vals, Function* F, dshared_ptr<tfg const> t, map<BasicBlock*, map<Value*, PHINode*>>& nodesMap);
  map<BasicBlock *, map<Value *, PHINode *>> addPhiNodesToBBIfRequired(map<pc, value_version_map_t> const& vals, Function *F, dshared_ptr<tfg const> t);
  map<Value *, Value*> getValueRenameMap(BasicBlock* BB, int inum, map<pc, value_version_map_t> const& vals, map<BasicBlock *, map<Value*, PHINode*>> const& new_phi_nodes);
  void replaceUsesOfValuesInBB(BasicBlock* BB, map<pc, value_version_map_t> const& vals, map<BasicBlock *, map<Value *, PHINode *>> &new_phi_nodes);

  value_version_map_t const& getValueVersionMapForBBAtInum(map<pc, value_version_map_t> const& vals, BasicBlock *BB, int inum);

  static pc getPCinBBAtInum(BasicBlock *BB, int inum);
};

}

char InstrumentMarkerCall::ID = 0;
char IdentifyUnbrokenLoop::ID = 0;
char IdentifyMaxDistancePC::ID = 0;

static RegisterPass<IdentifyUnbrokenLoop> UnbrokenLoop("identifyUnbrokenLoop", "Identify an unbroken loop");
static RegisterPass<IdentifyMaxDistancePC> MaxDistancePC("identifyMaxDistancePC", "Identify max flow PC");
static RegisterPass<InstrumentMarkerCall> MarkerCall("instrumentMarkerCall", "Instrument a marker call");

//static RegisterStandardPasses Y(
//    PassManagerBuilder::EP_LoopOptimizerEnd,
//    [](PassManagerBuilder const&  Builder,
//       legacy::PassManagerBase &PM) {
//    //PM.add(new IdentifyUnbrokenLoop());
//    //PM.add(new IdentifyForwardDistanceFromMarker());
//    /*PM.add(new IdentifyBackwardDistanceFromMarker()); */
//    PM.add(new InstrumentMarkerCall());
//});

//INITIALIZE_PASS_BEGIN(InstrumentMarkerCall, "instrumentMarkerCall", "Instrument a marker call", false, true)
//INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
//INITIALIZE_PASS_END(InstrumentMarkerCall, "instrumentMarkerCall", "Instrument a marker call", false, true)

static bool
function_is_defined_in_module(string const& fun_name, Module *M)
{
  for (auto const& F : *M) {
    string fname = F.getName().str();
    if (fname == fun_name) {
      return true;
    }
  }
  return false;
}

static string
get_function_name(Instruction const& I)
{
  const CallInst* c =  cast<const CallInst>(&I);
  Function *F = c->getCalledFunction();
  if (F == NULL) {
    Value const *v = c->getCalledOperand();
    Value const *sv = v->stripPointerCasts();
    return string(sv->getName());
  } else {
    return F->getName().str();
  }
}

static bool
basic_block_contains_marker(BasicBlock *BB, Module *M)
{
  assert(M);
  for (Instruction const& I : *BB) {
    if (I.getOpcode() != Instruction::Call) {
      continue;
    }
    string fun_name = get_function_name(I);
    if (fun_name == "") { //indirect call
      return true;
    }
    if (!function_is_defined_in_module(fun_name, M)) {
      return true;
    }
    Function *F = BB->getParent();
    if (F->getName() == fun_name) {
      return true;
    }
  }
  return false;
}

void
IdentifyUnbrokenLoop::unbrokenLoopVisit(Loop *L, loop_depth_t cur_depth, loop_depth_t &depth_out, Loop* &loop_out)
{
  bool is_unbroken = true;
  for (Loop::block_iterator BB = L->block_begin(),
                            BBE = L->block_end();
       BB != BBE; ++BB) {
    if (basic_block_contains_marker(*BB, M)) {
      is_unbroken = false;
      break;
    }
  }
  if (is_unbroken) {
    if (depth_out < cur_depth) {
      depth_out = cur_depth;
      loop_out = L;
    }
  }
  std::vector<Loop*> subLoops = L->getSubLoops();
  for (auto subL : subLoops) {
    unbrokenLoopVisit(subL, cur_depth + 1, depth_out, loop_out);
  }
}

Loop *
IdentifyUnbrokenLoop::identifyUnbrokenLoop(Function& F)
{
  loop_depth_t max_unbroken_loop_depth = 0;
  Loop* unbroken_loop = nullptr;

  auto & LIWP = getAnalysis<LoopInfoWrapperPass>();
  LoopInfo &LI = LIWP.getLoopInfo();
  for (LoopInfo::iterator i = LI.begin(); i != LI.end(); ++i) {
    unbrokenLoopVisit(*i, 1, max_unbroken_loop_depth, unbroken_loop);
  }
  return unbroken_loop;
}

bool
IdentifyUnbrokenLoop::runOnFunction(Function &F)
{
  string const& function_name = get_global_function_name();
  string fname = F.getName().str();
  if (fname != function_name) {
    return false;
  }

  Loop *L = nullptr;
  if ((L = identifyUnbrokenLoop(F))) {
    m_function = &F;
    m_BB = L->getHeader();
    m_insn_num = PC_SUBINDEX_FIRST_INSN_IN_BB;
  }
  return false;
}

bool
IdentifyUnbrokenLoop::doInitialization(Module& m)
{
  M = &m;
  return false;
}

void
IdentifyMaxDistancePC::retain_only_basic_block_heads(map<pc, distance_t>& distances)
{
  set<pc> to_erase;
  for (auto const& d : distances) {
    if (   d.first.get_subindex() != PC_SUBINDEX_FIRST_INSN_IN_BB
        || d.first.get_subsubindex() != PC_SUBSUBINDEX_DEFAULT) {
      to_erase.insert(d.first);
    }
  }
  for (auto const& p : to_erase) {
    distances.erase(p);
  }
}

bool
IdentifyMaxDistancePC::runOnFunction(Function &F)
{
  string const& function_name = get_global_function_name();
  string fname = F.getName().str();
  if (fname != function_name) {
    return false;
  }
  auto & idUnbrokenLoop = getAnalysis<IdentifyUnbrokenLoop>();
  if (idUnbrokenLoop.get_function()) {
    return false;
  }
  map<shared_ptr<tfg_edge const>, Instruction *> eimap;
  dshared_ptr<tfg_llvm_t> t = function2tfg(&F, M, eimap);
  map<pc, distance_t> fdistances, bdistances;
  distance_dfa_t<dfa_dir_t::forward> fdistance_dfa(t, fdistances);
  fdistance_dfa.initialize(distance_t(0));
  fdistance_dfa.solve();
  distance_dfa_t<dfa_dir_t::backward> bdistance_dfa(t, bdistances);
  bdistance_dfa.initialize(distance_t(0));
  bdistance_dfa.solve();

  retain_only_basic_block_heads(fdistances);
  retain_only_basic_block_heads(bdistances);

  CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
      dbgs() << __func__ << " " << __LINE__ << ": Printing forward distances for function " << F.getName() << ".\n";
      for (auto const& pd : fdistances) {
        dbgs() << pd.first.to_string() << " : " << pd.second.to_string() << "\n";
      }
      dbgs() << "\n";
      dbgs() << __func__ << " " << __LINE__ << ": Printing backward distances for function " << F.getName() << ".\n";
      for (auto const& pd : bdistances) {
        dbgs() << pd.first.to_string() << " : " << pd.second.to_string() << "\n";
      }
      dbgs() << "\n";
  );

  tuple<pc, distance_t> function_pc_max_distance = find_max_distance_pc(fdistances, bdistances);
  distance_t const& max_distance = get<1>(function_pc_max_distance);
  if (!m_function || max_distance.get() > m_max_distance.get()) {
    pc const& pc_max_distance = get<0>(function_pc_max_distance);
    m_function = &F;
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
        dbgs() << __func__ << " " << __LINE__ << ": pc_max_distance = " << pc_max_distance.to_string() << " in function " << m_function->getName() << "\n";
    );
    m_BB = getBBfromName(m_function, pc_max_distance.get_index());
    assert(m_BB);
    m_insn_num = pc_max_distance.get_subindex();
    m_max_distance = max_distance;
  }

  return false;
}

bool
IdentifyMaxDistancePC::doInitialization(Module& m)
{
  M = &m;
  return false;
}

bool
InstrumentMarkerCall::doInitialization(Module& m)
{
  M = &m;
  return false;
}

Value *
InstrumentMarkerCall::getMarkerFunction()
{
  if (!m_markerF) {
    assert(M);
    string markerName = QCC_MARKER_PREFIX "F";
    m_markerF = M->getNamedGlobal(markerName);
    if (!m_markerF) {
      m_markerF = new GlobalVariable(*M, PointerType::get(Type::getInt8Ty(M->getContext()), M->getDataLayout().getProgramAddressSpace()), false, GlobalValue::ExternalLinkage, nullptr, markerName);
    }
  }
  return m_markerF;
  //if (live_types.size() == 0) {
  //  return Function::Create(
  //      FunctionType::get(Type::getVoidTy(M->getContext()), false),
  //      GlobalValue::ExternalLinkage,
  //      M->getDataLayout().getProgramAddressSpace(),
  //      "markerF", M);
  //} else {
  //  return Function::Create(
  //      FunctionType::get(StructType::create(live_types), live_types, false),
  //      GlobalValue::ExternalLinkage,
  //      M->getDataLayout().getProgramAddressSpace(),
  //      "markerF", M);
  //}
}

FunctionType *
InstrumentMarkerCall::getMarkerFunctionType(vector<Type*> const& live_types)
{
  if (live_types.size() == 0) {
    return FunctionType::get(Type::getVoidTy(M->getContext()), false);
  } else {
    static int typenum = 0;
    stringstream ss;
    ss << "structType." << typenum;
    typenum++;
    //return FunctionType::get(StructType::create(live_types, ss.str()), live_types, false);
    return FunctionType::get(Type::getVoidTy(M->getContext()), live_types, false);
  }
}

vector<Type *>
InstrumentMarkerCall::values_to_types_vec(list<Value*> const& live_values)
{
  vector<Type *> ret;
  for (auto v : live_values) {
    ret.push_back(v->getType());
  }
  return ret;
}

bool
InstrumentMarkerCall::addPhiNodesToBBForValues(BasicBlock* BB, set<Value*> const& vvs, map<pc, value_version_map_t> const& vals, Function* F, dshared_ptr<tfg const> t, map<BasicBlock*, map<Value*, PHINode*>>& nodesMap)
{
  bool changed = false;
  pc p = getPCinBBAtInum(BB, PC_SUBINDEX_FIRST_INSN_IN_BB);
  list<dshared_ptr<tfg_edge const>> incoming;
  t->get_edges_incoming(p, incoming);
  for (auto v : vvs) {
    ASSERT(vals.count(p));
    ASSERT(vals.at(p).get_map().count(v));
    ASSERT(vals.at(p).get_map().at(v).is_phi());
    ASSERT(vals.at(p).get_map().at(v).getBB() == BB);
    map<BasicBlock const*, Value*> incomingValues;
    for (auto const& e : incoming) {
      pc const& from_p = e->get_from_pc();
      BasicBlock* fromBB = getBBfromName(F, from_p.get_index());
      ASSERT(vals.count(from_p));
      ASSERT(vals.at(from_p).get_map().count(v));
      value_version_t const& vv = vals.at(from_p).get_map().at(v);
      if (vv.is_phi()) {
        BasicBlock* vBB = vv.getBB();
        if (nodesMap.count(vBB)) {
          ASSERT(nodesMap.at(vBB).count(v));
          Instruction *I = nodesMap.at(vBB).at(v);
          incomingValues[fromBB] = I;
        } else {
          incomingValues[fromBB] = v;
        }
      } else {
        incomingValues[fromBB] = vv.getI();
      }
    }

    PHINode* phiInstruction = nullptr;
    if (nodesMap.count(BB) && nodesMap.at(BB).count(v)) {
      phiInstruction = nodesMap.at(BB).at(v);
    } else {
      for (auto& I : *BB) {
        if (!isa<PHINode const>(&I)) {
          continue;
        }
        PHINode *phi = dyn_cast<PHINode>(&I);
        assert(phi);
        for (int i = 0; i < phi->getNumIncomingValues(); i++) {
          Value *o = phi->getIncomingValue(i);
          BasicBlock const *fromBB = phi->getIncomingBlock(i);
          if (o == v || o == incomingValues.at(fromBB)) {
            ASSERT(!phiInstruction);
            phiInstruction = phi;
            break;
          }
        }
      }
      if (!phiInstruction) {
        changed = true;
        BasicBlock::iterator IP = BB->getFirstInsertionPt();
        phiInstruction = PHINode::Create(v->getType(), incomingValues.size(), "", &(*IP));
        for (auto const& iv : incomingValues) {
          phiInstruction->addIncoming(iv.second, (BasicBlock*)iv.first);
        }
        CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
           dbgs() << __func__ << " " << __LINE__ << ": Inserting " << get_basicblock_name(*BB) << " -> (" << sym_exec_common::get_value_name_using_srcdst_keyword(*v, G_SRC_KEYWORD) << " -> " << sym_exec_common::get_value_name_using_srcdst_keyword(*phiInstruction, G_SRC_KEYWORD) << ") to nodesMap\n";
        );
        nodesMap[BB].insert(make_pair(v, phiInstruction));
      }
    }
    for (int i = 0; i < phiInstruction->getNumIncomingValues(); i++) {
      BasicBlock const *fromBB = phiInstruction->getIncomingBlock(i);
      Value *o = phiInstruction->getIncomingValue(i);
      if (o != incomingValues.at(fromBB)) {
        phiInstruction->setIncomingValue(i, incomingValues.at(fromBB));
        changed = true;
      }
    }
  }
  return changed;
}

map<BasicBlock *, map<Value *, PHINode *>>
InstrumentMarkerCall::addPhiNodesToBBIfRequired(map<pc, value_version_map_t> const& vals, Function *F, dshared_ptr<tfg const> t)
{
  map<BasicBlock *, map<Value *, PHINode *>> ret;
  map<BasicBlock *, set<Value*>> new_valversions;
  for (auto const& pv : vals) {
    pc const& p = pv.first;
    if (p.is_start() || p.is_exit()) {
      continue;
    }
    //dbgs() << __func__ << " " << __LINE__ << ": p = " << p.to_string() << "\n";
    BasicBlock *BB = getBBfromName(F, p.get_index());
    map<Value*, value_version_t> const& vversion = pv.second.get_map();
    for (auto const& vv : vversion) {
      if (vv.second.is_phi()) {
        new_valversions[vv.second.getBB()].insert(vv.first);
      }
    }
  }

  bool something_changed = true;
  while (something_changed) {
    something_changed = false;
    for (auto const& bbvs : new_valversions) {
      BasicBlock *BB = bbvs.first;
      set<Value*> const& vvs = bbvs.second;
      something_changed = addPhiNodesToBBForValues(BB, vvs, vals, F, t, ret) || something_changed;
    }
  }
  return ret;
}

pc
InstrumentMarkerCall::getPCinBBAtInum(BasicBlock *BB, int inum)
{
  string bbname = get_basicblock_name(*BB);
  ASSERT(bbname.at(0) == '%');
  bbname = bbname.substr(1);
  return pc(pc::insn_label, bbname.c_str(), inum, PC_SUBSUBINDEX_DEFAULT);
}

value_version_map_t const&
InstrumentMarkerCall::getValueVersionMapForBBAtInum(map<pc, value_version_map_t> const& vals, BasicBlock *BB, int inum)
{
  pc p = getPCinBBAtInum(BB, inum);
  return vals.at(p);
}

map<Value *, Value*>
InstrumentMarkerCall::getValueRenameMap(BasicBlock* BB, int inum, map<pc, value_version_map_t> const& vals, map<BasicBlock *, map<Value*, PHINode*>> const &new_phi_nodes)
{
  map<Value *, Value *> ret;
  value_version_map_t const& vvmap = getValueVersionMapForBBAtInum(vals, BB, inum);
  for (auto const& m : vvmap.get_map()) {
    if (m.second.is_phi()) {
      ASSERT(new_phi_nodes.count(m.second.getBB()));
      //dbgs() << __func__ << " " << __LINE__ << ": m.second.getBB() = " << get_basicblock_name(*m.second.getBB()) << "\n";
      auto const& bbPhiNodes = new_phi_nodes.at(m.second.getBB());
      ASSERT(bbPhiNodes.count(m.first));
      ret.insert(make_pair(m.first, bbPhiNodes.at(m.first)));
    } else {
      ret.insert(make_pair(m.first, m.second.getI()));
    }
  }
  return ret;
}

void
InstrumentMarkerCall::replaceUsesOfValuesInBB(BasicBlock* BB, map<pc, value_version_map_t> const& vals, map<BasicBlock *, map<Value*, PHINode*>> &new_phi_nodes)
{
  map<Value *, Value*> renameMap;
  int inum = PC_SUBINDEX_FIRST_INSN_IN_BB;
  for (auto& I : *BB) {
    if (isa<PHINode const>(&I)) {
      continue;
    }
    renameMap = getValueRenameMap(BB, inum, vals, new_phi_nodes);
    for (int i = 0; i < I.getNumOperands(); i++) {
      Value *o = I.getOperand(i);
      if (renameMap.count(o)) {
        I.setOperand(i, renameMap.at(o));
      }
    }
    inum++;
  }
  //renameMap now represents the map at the last instruction of BB (deliberately); because the last instruction is always a branch (does not create a value) and because PHI node assignments are not effective, we can use this renameMap to update all the phi nodes in the successor blocks
  for (succ_iterator SI = succ_begin(BB); SI != succ_end(BB); SI++) {
    BasicBlock* nextBB = *SI;
    for (auto& I : *nextBB) {
      if (!isa<PHINode const>(&I)) {
        continue;
      }
      PHINode *phi = dyn_cast<PHINode>(&I);
      ASSERT(phi);
      for (int i = 0; i < phi->getNumIncomingValues(); i++) {
        if (phi->getIncomingBlock(i) != BB) {
          continue;
        }
        Value *o = phi->getIncomingValue(i);
        if (renameMap.count(o)) {
          phi->setIncomingValue(i, renameMap.at(o));
        }
      }
    }
  }
}

void
InstrumentMarkerCall::replaceGlobalUsesOfValue(vector<Value *> const& from, vector<Instruction *> const& to, BasicBlock *markerBB, Function *F, dshared_ptr<tfg const> t, map<shared_ptr<tfg_edge const>, Instruction *> const& eimap)
{
  map<pc, value_version_map_t> vals;
  valversion_dfa_t vvdfa(t, eimap, from, to, markerBB, F, vals);
  value_version_map_t start_pc_val;
  for (auto iter = F->arg_begin(); iter != F->arg_end(); iter++) {
    start_pc_val.add(iter, iter);
  }
  vvdfa.initialize(start_pc_val);
  vvdfa.solve();

  CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL,
    for (auto const& v : vals) {
      dbgs() << "solution at pc " << v.first.to_string() << " :\n";
      dbgs() << v.second.to_string() << "\n";
    }
  );

  map<BasicBlock *, map<Value *, PHINode *>> new_phi_nodes = addPhiNodesToBBIfRequired(vals, F, t);
  for (auto& BB : *F) {
    replaceUsesOfValuesInBB(&BB, vals, new_phi_nodes);
  }
}

void
InstrumentMarkerCall::replaceUsesInBBAfterMarkerCall(Value *from, Value *to, BasicBlock *BB, Instruction *markerCallEnd)
{
  bool seenMarkerCallEnd = false;
  for (auto& I : *BB) {
    if (markerCallEnd == &I) {
      seenMarkerCallEnd = true;
    }
    if (!seenMarkerCallEnd) {
      continue;
    }
    for (int i = 0; i < I.getNumOperands(); i++) {
      Value *o = I.getOperand(i);
      if (o == from) {
        I.setOperand(i, to);
      }
    }
  }
}

bool
InstrumentMarkerCall::markerPresentInBasicBlock(Function *F, BasicBlock *BB)
{
  for (auto const& I : *BB) {
    if (I.getOpcode() != Instruction::Call) {
      continue;
    }
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": found call instruction in BB " << get_basicblock_name(*BB) << ": " << I << "\n");
    CallInst const* c = cast<CallInst const>(&I);
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": c = " << c << "\n");
    Function *calleeF = c->getCalledFunction();
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": calleeF = " << calleeF << "\n");
    if (calleeF) {
      continue;
    }
    Value const *v = c->getCalledOperand();
    if (!isa<Instruction const>(v)) {
      continue;
    }
    Instruction const* calleeI = cast<Instruction const>(v);
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": calleeI = " << calleeI << "\n");
    if (!calleeI) {
      continue;
    }
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": *calleeI = " << *calleeI << "\n");
    if (calleeI->getOpcode() != Instruction::BitCast) {
      continue;
    }
    CastInst const* ci = cast<CastInst>(calleeI);
    Value *ci_v = ci->getOperand(0);
    if (!isa<Instruction const>(ci_v)) {
      continue;
    }
    Instruction const* ci_i = cast<Instruction const>(ci_v);
    if (!ci_i || ci_i->getOpcode() != Instruction::Load) {
      continue;
    }
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": ci_i->getOpcodel = load\n");
    const LoadInst* l = cast<const LoadInst>(ci_i);
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": l = " << l << "\n");
    if (!l) {
      continue;
    }
    Value const *Addr = l->getPointerOperand();
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": Addr = " << sym_exec_common::get_value_name_using_srcdst_keyword(*Addr, G_SRC_KEYWORD) << "\n");
    CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": m_markerF = " << (m_markerF ? sym_exec_common::get_value_name_using_srcdst_keyword(*m_markerF, G_SRC_KEYWORD) : "null") << "\n");
    if (sym_exec_common::get_value_name_using_srcdst_keyword(*Addr, G_SRC_KEYWORD) == string("@") + QCC_MARKER_PREFIX "F") {
      return true;
    }
  }
  return false;
}

void
InstrumentMarkerCall::addMarkerInBasicBlock(Function *F, BasicBlock *BB, int insn_subindex, list<Value*> const& live_values, bool breaking_loop)
{
  //auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  vector<Type *> live_types = values_to_types_vec(live_values);
  Value *markerF = this->getMarkerFunction();
  FunctionType *markerFT = this->getMarkerFunctionType(live_types);
  CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": adding marker in BB " << get_basicblock_name(*BB) << "\n");
  BasicBlock::iterator IP = BB->getFirstInsertionPt();
  Value *markerFload = new LoadInst(markerFT, markerF, "", false, Align(4), &(*IP));
  //CastInst *markerFcast = CastInst::CreatePointerCast(markerFload, PointerType::get(PointerType::get(markerFT, M->getDataLayout().getProgramAddressSpace()), M->getDataLayout().getProgramAddressSpace()), "", &(*IP));
  CastInst *markerFcast = CastInst::CreatePointerCast(markerFload, PointerType::get(markerFT, M->getDataLayout().getProgramAddressSpace()), "", &(*IP));
  vector<Value *> live_vals_vec = list_to_vector(live_values);
  vector<Instruction *> eInst_vec;
  CallInst* markerInst = CallInst::Create(markerFT, markerFcast, live_vals_vec, "", &(*IP));
  //for (size_t i = 0; i < live_values.size(); i++) {
  //  unsigned int arr[1] = {(unsigned)i};
  //  ArrayRef<unsigned> idx(arr, 1);
  //  ExtractValueInst* eInst = ExtractValueInst::Create(markerInst, idx, "", &(*IP));
  //  replaceUsesInBBAfterMarkerCall(live_vals_vec.at(i), eInst, BB, &(*IP));
  //  eInst_vec.push_back(eInst);
  //}
  CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": done inserting marker in BB " << get_basicblock_name(*BB) << "\n");
  map<shared_ptr<tfg_edge const>, Instruction *> eimap;
  dshared_ptr<tfg_llvm_t> t = function2tfg(F, M, eimap);
  CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": done function2tfg after inserting marker in BB " << get_basicblock_name(*BB) << "\n");
  //replaceGlobalUsesOfValue(live_vals_vec, eInst_vec, BB, F, t.get(), eimap);
  CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": done replaceGlobalUsesOfValue after inserting marker in BB " << get_basicblock_name(*BB) << "\n");

  CPP_DBG_EXEC2(INSTRUMENT_MARKER_CALL,
      dbgs() << "========================after replaceGlobalUsesOfValue===================\n";
      legacy::PassManager PM;
      PM.add(createPrintModulePass(dbgs()));
      PM.run(*F->getParent());
  );
}


tuple<pc, distance_t>
IdentifyMaxDistancePC::find_max_distance_pc(map<pc, distance_t> const& fwd_distances, map<pc, distance_t> const& bwd_distances)
{
  ASSERT(fwd_distances.size());
  pc ret = fwd_distances.begin()->first;
  distance_t max_distance_so_far = fwd_distances.begin()->second;
  ASSERT(fwd_distances.size());
  for (auto const &pd : fwd_distances) {
    if (pd.first.is_start() || pd.first.is_exit()) {
      continue;
    }
    if (!bwd_distances.count(pd.first)) {
      continue;
    }
    size_t fd_eff_distance, bd_eff_distance;;
    if (pd.second.is_top()) {
      fd_eff_distance = 0;
    } else {
      fd_eff_distance = pd.second.get();
    }
    distance_t const& bd = bwd_distances.at(pd.first);
    if (bd.is_top()) {
      CPP_DBG_EXEC(INSTRUMENT_MARKER_CALL, dbgs() << __func__ << " " << __LINE__ << ": error: backward distance is top at pc " << pd.first.to_string() << "\n");
      bd_eff_distance = 1;
    } else {
      bd_eff_distance = bd.get();
    }
    unsigned long new_distance = fd_eff_distance * bd_eff_distance;
    if (new_distance > max_distance_so_far.get()) {
      max_distance_so_far = distance_t(new_distance);
      ret = pd.first;
    }
  }
  //ASSERT(max_distance_so_far.get() > 0);
  return make_pair(ret, max_distance_so_far);
}


list<Value *>
InstrumentMarkerCall::get_live_values(dshared_ptr<tfg_llvm_t const> t, map<shared_ptr<tfg_edge const>, Instruction *> const& eimap, Function const * function, BasicBlock const* BB, int insn_num)
{
  map<pc, liveness_val_t> live_vals;
  liveness_dfa_t ldfa(t, eimap, live_vals);
  ldfa.initialize(liveness_val_t());
  ldfa.solve();
  CPP_DBG_EXEC(LLVM_LIVENESS,
      cout << __func__ << " " << __LINE__ << ": function " << function->getName().str() << endl;
      for (auto const& pl : live_vals) {
        cout << pl.first.to_string() << " : ";
        for (auto const& v : pl.second.get_vals()) {
          cout << " " << sym_exec_common::get_value_name_using_srcdst_keyword(*v, G_SRC_KEYWORD);
        }
        cout << endl;
      }
  );
  string bname = get_basicblock_name(*BB);
  assert(bname.at(0) == '%');
  bname = bname.substr(1);
  pc p(pc::insn_label, bname.c_str(), insn_num, PC_SUBSUBINDEX_DEFAULT);
  if (live_vals.count(p) == 0) {
    cout << __func__ << " " << __LINE__ << ": live_vals.count = 0 for pc = " << p.to_string() << "\n";
    return list<Value *>();
  }
  return live_vals.at(p).get_vals();
}

bool
InstrumentMarkerCall::runOnFunction(Function &F)
{
  string const& function_name = get_global_function_name();
  string fname = F.getName().str();
  if (fname != function_name) {
    return false;
  }
  //DBG_SET(INSTRUMENT_MARKER_CALL, 2);
  auto & idUnbrokenLoop = getAnalysis<IdentifyUnbrokenLoop>();
  Function *function = nullptr;
  BasicBlock *BB = nullptr;
  map<shared_ptr<tfg_edge const>, Instruction *> eimap;
  int insn_num;
  bool breaking_loop = false;
  if (!idUnbrokenLoop.get_function()) {
    auto & idMaxDistancePC = getAnalysis<IdentifyMaxDistancePC>();
    function = idMaxDistancePC.get_function();
    BB = idMaxDistancePC.getBB();
    insn_num = idMaxDistancePC.getInum();
  } else {
    function = idUnbrokenLoop.get_function();
    BB = idUnbrokenLoop.getBB();
    insn_num = idUnbrokenLoop.getInum();
    breaking_loop = true;
  }

  ASSERT(function == nullptr || function == &F); //because we only consider global_function_name
  if (function != &F) {
    return false;
  }
  ASSERT(function->getName() == function_name);
  if (markerPresentInBasicBlock(function, BB)) {
    return false;
  }
  assert(insn_num >= 0);
  dshared_ptr<tfg_llvm_t> t = function2tfg(&F, M, eimap);
  //identify live vars at F/BB/insns_num
  list<Value *> live_values = get_live_values(t, eimap, function, BB, insn_num);
  addMarkerInBasicBlock(function, BB, insn_num, live_values, breaking_loop);

  return true;
}
