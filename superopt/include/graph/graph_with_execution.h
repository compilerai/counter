#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "expr/local_sprel_expr_guesses.h"
#include "expr/counterexample_eval_retval.h"
#include "expr/counterexample_translate_retval.h"
#include "expr/pc.h"
#include "expr/relevant_memlabels.h"

#include "gsupport/sprel_map.h"
#include "gsupport/memlabel_assertions.h"
#include "gsupport/edge_with_cond.h"
#include "gsupport/failcond.h"
#include "gsupport/src_dst_cg_path_tuple.h"

#include "graph/graph.h"
//#include "graph/binary_search_on_preds.h"
#include "graph/lr_map.h"
#include "graph/graph_with_paths.h"


namespace eqspace {

using namespace std;
class tfg_llvm_t;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_execution : public graph_with_paths<T_PC, T_N, T_E, T_PRED>
{
public:
  graph_with_execution(string const &name, string const& fname, context *ctx)
    : graph_with_paths<T_PC, T_N, T_E, T_PRED>(name, fname, ctx)
  { }

  graph_with_execution(graph_with_execution const &other)
    : graph_with_paths<T_PC, T_N, T_E, T_PRED>(other)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_execution(istream& is, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx) : graph_with_paths<T_PC, T_N, T_E, T_PRED>(is, name, read_edge_fn, ctx)
  {
    autostop_timer ft(string("graph_with_execution_constructor.") + name);
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }
  virtual ~graph_with_execution() = default;

  counterexample_translate_retval_t
  counter_example_translate_on_edge_composition(counter_example_t &ce, graph_edge_composition_ref<T_PC,T_E> const &pth, counter_example_t &rand_vals, relevant_memlabels_t const& relevant_memlabels, bool ignore_assumes = false, bool ignore_wfconds = false) const;

  virtual list<tuple<graph_edge_composition_ref<pc,tfg_edge>, expr_ref, shared_ptr<T_PRED const>>> edge_composition_apply_trans_funs_on_pred(shared_ptr<T_PRED const> const &p, shared_ptr<graph_edge_composition_t<T_PC,T_E> const> const &ec) const = 0;

  virtual tfg_llvm_t const &get_src_tfg() const = 0; //src_tfg is the tfg against which the edge-guards are computed
  //virtual tfg const &get_dst_tfg() const = 0;

protected:
  virtual bool edge_assumes_satisfied_by_counter_example(dshared_ptr<T_E const> const& edge, counter_example_t const &ce, counter_example_t &rand_vals, relevant_memlabels_t const& relevant_memlabels) const = 0;

  virtual failcond_t edge_well_formedness_conditions_falsified_by_counter_example(dshared_ptr<T_E const> const& e, counter_example_t const& ce, relevant_memlabels_t const& relevant_memlabels) const = 0;

  bool counter_example_falsifies_preds( counter_example_t const &ce, list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<T_PRED const>>> const& preds, counter_example_t &rand_vals, relevant_memlabels_t const& relevant_memlabels, shared_ptr<T_PRED const>& failed_pred) const;

  bool counter_example_satisfies_preds(counter_example_t const &ce, list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<T_PRED const>>> const& preds, counter_example_t &rand_vals, relevant_memlabels_t const& relevant_memlabels, shared_ptr<T_PRED const>& failed_pred) const;

// returns exec_success, eval_success
  virtual counterexample_translate_retval_t counter_example_translate_on_edge(counter_example_t &counter_example, dshared_ptr<T_E const> const &edge, counter_example_t &rand_vals, failcond_t& failcond, relevant_memlabels_t const& relevant_memlabels, bool ignore_assumes = false, bool ignore_wfconds = false) const = 0;

private:
  counterexample_eval_retval_t counter_example_evaluate_preds(counter_example_t const &ce, list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> const& preds, counter_example_t &rand_vals, relevant_memlabels_t const& relevant_memlabels, shared_ptr<T_PRED const>& failed_pred) const;

};

}
