#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/relevant_memlabels.h"

#include "support/timers.h"
#include "expr/z3_solver.h"

#include "graph/prove.h"
#include "expr/counter_example.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

class expr_get_relevant_memlabels_visitor : public expr_visitor {
public:
  expr_get_relevant_memlabels_visitor(expr_ref const &e, set<memlabel_ref> &relevant_memlabels) : m_in(e), m_relevant_memlabels(relevant_memlabels)
  {
    //m_visit_each_expr_only_once = true;
    m_ctx = e->get_context();
    visit_recursive(e);
  }

private:
  virtual void visit(expr_ref const &e);

  expr_ref m_in;
  set<memlabel_ref> &m_relevant_memlabels;
  context *m_ctx;
};

void
expr_get_relevant_memlabels_visitor::visit(expr_ref const &e)
{
  if (e->is_memlabel_sort()) {
    memlabel_t ml;
    ml = e->get_memlabel_value();
    if (!ml.memlabel_is_top()) {
      set<memlabel_ref> amls = ml.get_atomic_memlabels();
      for (auto const& aml : amls) {
        m_relevant_memlabels.insert(aml);
      }
    }
  }
}

static set<memlabel_ref>
expr_get_relevant_memlabels(expr_ref const& e)
{
  set<memlabel_ref> ret;
  expr_get_relevant_memlabels_visitor visitor(e, ret);
  ret.insert(mk_memlabel_ref(memlabel_t::memlabel_heap()));
  return ret;
}


int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to .expr file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::arg<string> counter_example_infile(cl::explicit_prefix, "ce", "", "Counter example filename; if specified, we attempt to disprove the query using this counter-example");
  cl::cl cmd("Expression Prover : Tries and proves a given expr file");
  cmd.add_arg(&expr_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&counter_example_infile);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();

  ifstream in(expr_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
    exit(1);
  }

  //ctx.read_range_submap(in);
  map<expr_id_t, expr_ref> emap;
  expr_ref e;
  string line = read_expr_and_expr_map(in, e, emap, &ctx);


  //vector<memlabel_ref> relevant_memlabels;
  //cs.solver_set_relevant_memlabels(relevant_memlabels);

  g_query_dir_init();

  //DYN_DBG_ELEVATE(ce_add, 1);
  //DYN_DBG_ELEVATE(smt_query_dump, 2);
  //DYN_DBG_ELEVATE(ce_eval, 1);
  //DBG_SET(UNALIASED_MEMSLOT, 2);
  set<memlabel_ref> relevant_memlabels = expr_get_relevant_memlabels(e);

  if (counter_example_infile.get_value() != "") {
    // need to populate original relevant_memlabels for correct evaluation of memmask()
    string counter_example_str = file_to_string(counter_example_infile.get_value());
    counter_example_t counter_example = counter_example_t::counter_example_from_string(counter_example_str, &ctx);
    expr_ref ret;
    counter_example_t rand_vals(&ctx, ctx.get_next_ce_name(RAND_VALS_CE_NAME_PREFIX));
    cout << "CE:\n" << counter_example.counter_example_to_string() << endl;
    bool eval_success = counter_example.evaluate_expr_assigning_random_consts_as_needed(e, ret, rand_vals, relevant_memlabels_t(relevant_memlabels));
    cout << __func__ << " " << __LINE__ << ": Evaluated CE on expr:\n";
    cout << "evaluated expression (ret):\n" << expr_string(ret) << endl;
    cout << "rand_vals:\n" << rand_vals.counter_example_to_string() << endl;
    map<expr_id_t, expr_ref> eval = evaluate_counter_example_on_expr_map(emap, relevant_memlabels, counter_example);
    ASSERT(eval.size());
    cout << __func__ << " " << __LINE__ << ": eval.size() = " << eval.size() << endl;
    cout << "\n===\nevaluation:\n";
    expr_print_with_ce_visitor visitor(e, emap, eval, cout);
    visitor.print_result();
    return 0;
  }


  bool result = false;
  //list<counter_example_t> returned_ces;
  proof_result_t proof_result = ctx.get_solver()->expr_is_provable(e, string(__func__), relevant_memlabels_t(relevant_memlabels)/*, returned_ces*/);
  if (proof_result.get_proof_status() == proof_status_t::proof_status_proven) {
    result = true;
  } /*else if (!counter_example.is_empty()) {
    //counter_example.add_missing_memlabel_lb_and_ub_consts(&ctx, relevant_memlabels);
  }*/

  if (!result) {
    size_t i = 0;
    for (auto& ce : proof_result.get_counterexamples()) {
      stringstream ss;
      ss << expr_file.get_value() + ".counter_example." << i++;
      ofstream counter_example_ofstream(ss.str());
      cout << "counter_example:\n" << ce.counter_example_to_string() << endl;
      counter_example_ofstream << "counter_example:\n" << ce.counter_example_to_string() ;
      for (size_t i = 0; i < cs.relevant_memlabels.size(); i++) {
        counter_example_ofstream << cs.relevant_memlabels[i]->get_ml().to_string() << ": " << i << endl;
      }
      counter_example_ofstream << "\n===\nevaluation:\n";
      map<expr_id_t, expr_ref> eval = evaluate_counter_example_on_expr_map(emap, relevant_memlabels, ce);
      expr_print_with_ce_visitor visitor(e, emap, eval, counter_example_ofstream);
      visitor.print_result();
      //for (auto ev : eval) {
      //  counter_example_ofstream << ev.first << ": " << expr_string(ev.second) << "\n";
      //}
      counter_example_ofstream.close();
      cout << "NOT-PROVABLE" << endl;
      cout << "counter_example: " << ss.str() << endl;
      expr_ref ret;
      counter_example_t rand_vals(&ctx, ctx.get_next_ce_name(RAND_VALS_CE_NAME_PREFIX));
      bool eval_success = ce.evaluate_expr_assigning_random_consts_as_needed(e, ret, rand_vals, relevant_memlabels);
      ASSERT(eval_success);
      ASSERT(ret->is_const());
      ASSERT(!ret->get_bool_value());
    }
  }
  solver_kill();
  call_on_exit_function();

  if (result) {
    cout << "PROVABLE" << endl;
  } else {
    cout << "NOT-PROVABLE" << endl;
  }

  exit(1);
}
