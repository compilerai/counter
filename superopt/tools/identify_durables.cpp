#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/range.hpp>
#include <magic.h>

#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/globals_cpp.h"
#include "support/timers.h"
#include "support/read_source_files.h"
#include "support/src-defs.h"
#include "support/dyn_debug.h"

#include "expr/consts_struct.h"
#include "expr/z3_solver.h"

#include "rewrite/assembly_handling.h"

#include "tfg/tfg_llvm.h"

#include "ptfg/llptfg.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"

#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_asm_annotation.h"
#include "eq/function_cg_map.h"


#define DEBUG_TYPE "main"

using namespace std;

#define MAX_GUESS_LEVEL 1


static string const&
find_line_and_column_for_pc(map<pc, string> const& pc_line_and_column_map, pc const& p)
{
  for (auto const& plc : pc_line_and_column_map) {
    if (plc.first.get_index() == p.get_index() && plc.first.get_subindex() == p.get_subindex()) {
      return plc.second;
    }
  }
  NOT_REACHED();
}

static bool
memlabel_represents_durable_region(llptfg_t const& llptfg, memlabel_t const& ml)
{
  //cout << _FNLN_ << ": ml = " << ml.to_string() << endl;
  if (ml.memlabel_represents_symbol()) {
    graph_symbol_map_t symbol_map = llptfg.llptfg_get_symbol_map();
    symbol_id_t symbol_id = ml.memlabel_get_symbol_id();
    graph_symbol_t const& sym = symbol_map.at(symbol_id);
    string const& sname = sym.get_name()->get_str();
    return string_has_prefix(sname, PMEM_PREFIX);
  } else if (ml.memlabel_is_heap_alloc()) {
    allocstack_t allocstack = ml.memlabel_get_heap_alloc_id();
    allocsite_t const& allocsite = allocstack.allocstack_get_last_allocsite();
    string const& fname = allocstack.allocstack_get_last_fname()->get_str();
    pc const& p = allocsite.get_pc();
    dshared_ptr<tfg_llvm_t const> t = llptfg.llptfg_get_tfg_llvm_for_function_name(fname);
    pc fcall_end_pc(pc::insn_label, p.get_index(), p.get_subindex(), PC_SUBSUBINDEX_FCALL_END);
    list<dshared_ptr<tfg_edge const>> incoming = t->get_edges_incoming(fcall_end_pc);
    ASSERT(incoming.size() == 1);
    dshared_ptr<tfg_edge const> e = incoming.front();
    nextpc_id_t nextpc_id;
    if (!e->is_fcall_edge(nextpc_id)) {
      cout << _FNLN_ << ": e =\n" << e->to_string_concise() << endl;
      NOT_REACHED();
    }
    map<nextpc_id_t, string> const& nextpc_map = t->get_nextpc_map();
    ASSERT(nextpc_map.count(nextpc_id));
    string const& callee_name = nextpc_map.at(nextpc_id);
    bool ret = string_has_prefix(callee_name, PMEM_PREFIX);
    //cout << _FNLN_ << ": returning " << ret << " for " << callee_name << "(nextpc_id " << nextpc_id << " of " << fname << ")\n";
    return ret;
  }
  return false;
}

static set<memlabel_ref>
identify_durable_memlabels(llptfg_t const& llptfg)
{
  relevant_memlabels_t relevant_mls = llptfg.llptfg_get_relevant_memlabels();
  set<memlabel_ref> ret;
  for (auto const& mlr : relevant_mls.relevant_memlabels_get_ml_set()) {
    if (memlabel_represents_durable_region(llptfg, mlr->get_ml())) {
      ret.insert(mlr);
    }
  }
  return ret;
}

static set<memlabel_ref>
identify_transitive_closure(llptfg_t const& llptfg, set<memlabel_ref> const& mls)
{
  set<memlabel_ref> ret = mls;
  for (auto const& ft : llptfg.get_function_tfg_map()) {
    dshared_ptr<tfg> t = ft.second->get_ssa_tfg();
    DYN_DEBUG(identify_durables_debug, cout << _FNLN_ << ": looking at cc " << ft.first->call_context_to_string() << endl);
    set_union(ret, t->tfg_identify_reachable_memlabels_at_all_pcs(ret));
  }
  return ret;
}

static void
update_source_file_using_durable_memlabels(llptfg_t const& llptfg, set<memlabel_ref> const& durable_mls)
{
  cout << "Durable regions:\n";
  graph_symbol_map_t symbol_map = llptfg.llptfg_get_symbol_map();
  for (auto const& mlr : durable_mls) {
    if (mlr->get_ml().memlabel_represents_symbol()) {
      symbol_id_t sym_id = mlr->get_ml().memlabel_get_symbol_id();
      graph_symbol_t const& sym = symbol_map.at(sym_id);
      cout << "  global variable '" << sym.get_name()->get_str() << "'\n";
    }
    if (mlr->get_ml().memlabel_is_heap_alloc()) {
      allocstack_t allocstack = mlr->get_ml().memlabel_get_heap_alloc_id();
      cout << "  objects allocated at: {\n";
      for (auto const& fallocsite : allocstack.allocstack_get_stack()) {
        string const& fname = fallocsite.get_function_name()->get_str();
        allocsite_t const& allocsite = fallocsite.get_allocsite();
        pc const& p = allocsite.get_pc();
        dshared_ptr<tfg_llvm_t const> t = llptfg.llptfg_get_tfg_llvm_for_function_name(fname);
        map<pc, string> const& pc_line_and_column_map = t->tfg_llvm_get_pc_line_and_column_map();

        string const& line_and_column = find_line_and_column_for_pc(pc_line_and_column_map, p);
        cout << "    " << fname << " : " << line_and_column << endl;
      }
      cout << "  }\n";
    }
  }
}

int main(int argc, char **argv)
{
  autostop_timer func_timer(__func__);

  cl::cl cmd("Equivalence checker: checks equivalence of given .eq file");

  // command line args processing
  cl::arg<string> etfg_file(cl::implicit_prefix, "", "path to .bc/.etfg file");
  cmd.add_arg(&etfg_file);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1,dumptfg,decide_hoare_triple,update_invariant_state_over_edge");
  cmd.add_arg(&debug);
  cl::arg<unsigned> call_context_depth(cl::explicit_prefix, "call-context-depth", CALL_CONTEXT_DEPTH_ZERO, "Call context depth used for alias analysis.");
  cmd.add_arg(&call_context_depth);

  CPP_DBG_EXEC(ARGV_PRINT,
      for (int i = 0; i < argc; i++) {
        cout << "argv[" << i << "] = " << argv[i] << endl;
      }
  );
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());
  g_query_dir_init();
  src_init();
  dst_init();
  init_timers();

  //tfg_asm_t::init_asm_regnames();
  //quota qt(1, smt_query_timeout.get_value(), sage_query_timeout.get_value(), global_timeout.get_value(), memory_threshold_gb.get_value(), max_address_space_size.get_value());

  g_ctx_init();
  context *ctx = g_ctx;
  //ctx->parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx->get_consts_struct();

  string source_filename = etfg_file.get_value();

  bool model_llvm_semantics = false;
  string etfg_filename = convert_source_file_to_etfg(source_filename, "", "text-color", "ALL", model_llvm_semantics, call_context_depth.get_value(), "", "");

  ifstream in_llvm(etfg_filename);
  if (!in_llvm.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open etfg_filename '" << etfg_filename << "'\n";
  }
  llptfg_t llptfg(in_llvm, ctx);
  in_llvm.close();

  set<memlabel_ref> durable_memlabels = identify_durable_memlabels(llptfg);
  cout << _FNLN_ << ":\n";
  cout << "durable memlabels =\n";
  for (auto const& dm : durable_memlabels) {
    cout << dm->get_ml().to_string() << endl;
  }
  cout << "\n";
  durable_memlabels = identify_transitive_closure(llptfg, durable_memlabels);
  cout << "transitive closure of durable memlabels =\n";
  for (auto const& dm : durable_memlabels) {
    cout << dm->get_ml().to_string() << endl;
  }
  cout << "\n";
  update_source_file_using_durable_memlabels(llptfg, durable_memlabels);

  //for (auto const& fname_tfg : llptfg.get_function_tfg_map()) {
  //  string cur_function_name = fname_tfg.first;
  //  dshared_ptr<tfg> t = fname_tfg.second;
  //  dshared_ptr<tfg_llvm_t> t_llvm = dynamic_pointer_cast<tfg_llvm_t>(t);
  //  ASSERT(t_llvm);
  //}

  call_on_exit_function();
}
