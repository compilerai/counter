#include <fstream>
#include <iostream>
#include <string>

#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/src-defs.h"
#include "support/timers.h"
#include "support/binary_search.h"

#include "expr/consts_struct.h"
#include "expr/z3_solver.h"
#include "expr/expr.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"

#include "gsupport/parse_edge_composition.h"

#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_inductive_preds.h"
#include "eq/cg_with_relocatable_memlabels.h"

#define DEBUG_TYPE "main"

using namespace std;

static set<expr_group_t::expr_idx_t>
get_remaining_exprs(set<expr_group_t::expr_idx_t> const& minimized_bv_exprs, size_t total_num_bv_exprs)
{
  set<expr_group_t::expr_idx_t> ret;
  for (expr_group_t::expr_idx_t i = expr_group_t::expr_idx_begin(); i < total_num_bv_exprs; i++) {
    if (!set_belongs(minimized_bv_exprs, i)) {
      ret.insert(i);
    }
  }
  return ret;
}

static pair<dshared_ptr<cg_with_asm_annotation>, shared_ptr<corr_graph_edge const>>
read_update_invariant_state_over_edge(string const& filename, context* ctx)
{
  ifstream in(filename);
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << filename << "." << endl;
    exit(1);
  }

  string line;
  bool end;

  auto cg = cg_with_asm_annotation::corr_graph_from_stream(in, ctx);

  tfg const& src_tfg = cg->get_src_tfg();
  tfg const& dst_tfg = cg->get_dst_tfg();

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=edge");
  shared_ptr<cg_edge_composition_t> pth;
  pth = cg->graph_edge_composition_from_stream(in/*, line*/, "=update_invariant_state_over_edge"/*, ctx, pth*/);
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(is_line(line, "=end"));

  ASSERT(pth->is_atom());
  auto e = pth->get_atom()->edge_with_unroll_get_edge();

  in.close();
  return make_pair(cg, e);
}


class binary_search_on_bv_exprs_t : public binary_search_t
{
public:
  binary_search_on_bv_exprs_t(context* ctx, string const& filename, size_t start_check_at_num_ces_count, size_t total_num_bv_exprs) : m_ctx(ctx), m_filename(filename), m_start_check_at_num_ces_count(start_check_at_num_ces_count), m_total_num_bv_exprs(total_num_bv_exprs)
  { }

private:
  bool test(set<size_t> const& indices) const override
  {
    cout << _FNLN_ << ": indices =";
    for (auto i : indices) { cout << " " << i; }
    cout << "...\n";
    set<expr_group_t::expr_idx_t> expr_idxs;
    for (auto const& i : indices) { expr_idxs.insert((expr_group_t::expr_idx_t)(i + expr_group_t::expr_idx_begin())); }
    set<expr_group_t::expr_idx_t> remaining = get_remaining_exprs(expr_idxs, m_total_num_bv_exprs);

    pair<dshared_ptr<cg_with_asm_annotation>, shared_ptr<corr_graph_edge const>> pr = read_update_invariant_state_over_edge(m_filename, m_ctx);
    auto cg = pr.first;
    auto to_pc = pr.second->get_to_pc();
    bool ret = !cg->check_monotonicity_of_invariants(to_pc, m_start_check_at_num_ces_count, remaining);
    cout << "  returning " << (ret ? "true" : "false") << endl;
    return ret;
  }
private:
  context* m_ctx;
  string const& m_filename;
  size_t m_start_check_at_num_ces_count;
  size_t m_total_num_bv_exprs;
};

int main(int argc, char **argv)
{
  // command line args processing
  cl::cl cmd("decide_hoare_triple query");
  cl::arg<string> query_file(cl::implicit_prefix, "", "path to input file");
  cmd.add_arg(&query_file);
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 60, "Timeout per query (s)");
  cmd.add_arg(&smt_query_timeout);
  cl::arg<unsigned> start_check_at_num_ces(cl::explicit_prefix, "start-check-at-num-CEs", 1, "Start checking monotonicity only after these many CEs have been added");
  cmd.add_arg(&start_check_at_num_ces);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&debug);

  cl::arg<string> record_filename(cl::explicit_prefix, "record", "", "generate record log of SMT queries to given filename");
  cmd.add_arg(&record_filename);
  cl::arg<string> replay_filename(cl::explicit_prefix, "replay", "", "replay log of SMT queries from given filename");
  cmd.add_arg(&replay_filename);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  //DYN_DBG_ELEVATE(prove_dump, 1);

  context::config cfg(smt_query_timeout.get_value(), 100/*sage_query_timeout*/);
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init(record_filename.get_value(), replay_filename.get_value(), "");

  g_query_dir_init();
  src_init();
  dst_init();
  
  dshared_ptr<cg_with_asm_annotation> cg;
  shared_ptr<corr_graph_edge const> e;
  tie(cg, e) = read_update_invariant_state_over_edge(query_file.get_value(), &ctx);
  pcpair to_pc = e->get_to_pc();

  //cout << __func__ << " " << __LINE__ << ": cg =\n";
  //cg->graph_to_stream(cout);
  cout << "=edge\n" << e->to_string() << endl;

  size_t start_check_at_num_ces_count = start_check_at_num_ces.get_value();
  if (cg->check_monotonicity_of_invariants(to_pc, start_check_at_num_ces_count, set<expr_group_t::expr_idx_t>())) {
    cout << "Successfully checked monotonicity of invariants at " << to_pc.to_string() << endl;
  } else {
    size_t total_num_bv_exprs = cg->get_invariant_state_at_pc(to_pc, reason_for_counterexamples_t::inductive_invariants()).get_num_bv_exprs_correlated();
    //set<size_t> start_bv_expr_ids = { 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 24, 28, 30, 37, 42, 47, 48, 49 };
    set<expr_group_t::expr_idx_t> minimized_bv_exprs = { 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 19, 20, 21, 24, 28, 30, 37, 42, 47, 48, 49 };
    cout << "Monotonicity check failed for the invariants at " << to_pc.to_string() << ", doing binary search on " << total_num_bv_exprs << " bv-exprs to minimize the bug report..." << endl;
    binary_search_on_bv_exprs_t bsearch(&ctx, query_file.get_value(), start_check_at_num_ces_count, total_num_bv_exprs);
    //minimized_bv_exprs = bsearch.do_binary_search(start_bv_expr_ids);
    DYN_DBG_ELEVATE(eqclass_bv, 2);
    DYN_DBG_ELEVATE(bv_solve_debug, 2);
    set<expr_group_t::expr_idx_t> remaining_exprs = get_remaining_exprs(minimized_bv_exprs, total_num_bv_exprs);

    pair<dshared_ptr<cg_with_asm_annotation>, shared_ptr<corr_graph_edge const>> pr = read_update_invariant_state_over_edge(query_file.get_value(), &ctx);
    cg = pr.first;
    ASSERT(to_pc == pr.second->get_to_pc());

    bool b = cg->check_monotonicity_of_invariants(to_pc, start_check_at_num_ces_count, remaining_exprs);
    ASSERT(!b);
  }

  solver_kill();
  call_on_exit_function();

  exit(0);
}
