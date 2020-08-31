#pragma once

#include "gsupport/pc.h"
#include "graph/graph.h"
#include "support/utils.h"
#include "support/log.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "graph/binary_search_on_preds.h"
#include "graph/lr_map.h"
#include "gsupport/sprel_map.h"
/* #include "graph/graph_cp.h" */
#include "expr/local_sprel_expr_guesses.h"
#include "expr/local_sprel_expr_guesses.h"
#include "graph/memlabel_assertions.h"

#include "gsupport/edge_with_cond.h"
//#include "graph/edge_guard.h"
//#include "graph/concrete_values_set.h"
#include "support/timers.h"
#include "graph/delta.h"
#include "graph_with_paths.h"

#include <map>
#include <list>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_execution : public graph_with_paths<T_PC, T_N, T_E, T_PRED>
{
public:
  graph_with_execution(string const &name, context *ctx)
    : graph_with_paths<T_PC, T_N, T_E, T_PRED>(name, ctx)
  { }

  graph_with_execution(graph_with_execution const &other)
    : graph_with_paths<T_PC, T_N, T_E, T_PRED>(other)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_execution(istream& is, string const& name, context* ctx) : graph_with_paths<T_PC, T_N, T_E, T_PRED>(is, name, ctx)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }
  virtual ~graph_with_execution() = default;

// returns propagation_success, evaluation success
// It is required to distinguish whether the propagation_failure is due to evaluation failure or edge_composition condition failure
// Invariant mantained is, for a parallel path in edge compositon, if evaluation status of one the parallel path is false then eval status of parallel ec is also false
//                         For a series path, if evaluation status of one the series path is false then eval status of series ec is also false
  pair<bool, bool>
  counter_example_translate_on_edge_composition(counter_example_t const &ce, graph_edge_composition_ref<T_PC,T_E> const &pth, counter_example_t &out_ce, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels) const
  {
    autostop_timer func_timer(__func__);
    PRINT_PROGRESS("%s %s() %d: pth = %s\n", __FILE__, __func__, __LINE__, pth->graph_edge_composition_to_string().c_str());
    ASSERT(pth);
    if (pth->is_epsilon()) {
      out_ce = ce;
      return make_pair(true, true);
    }
    context *ctx = this->get_context();
    std::function<tuple<bool, bool, bool, counter_example_t, counter_example_t> (shared_ptr<T_E const> const&, tuple<bool, bool, bool, counter_example_t, counter_example_t> const &)> atomf = [ctx, this, relevant_memlabels](shared_ptr<T_E const> const& e, tuple<bool, bool, bool, counter_example_t, counter_example_t> const &bc)
    {
      bool execution_successful = get<0>(bc);
      bool evaluation_successful = get<1>(bc);
      bool is_epsilon_ec = get<2>(bc);
      counter_example_t const &c = get<3>(bc);
      counter_example_t rand_vals = get<4>(bc);
      if (!execution_successful) {
        return bc;
      }
      ASSERT(execution_successful);
      if ( e->is_empty()) {
        return make_tuple(execution_successful, evaluation_successful, true, c, rand_vals);
      }
      counter_example_t ret(ctx);
      //return exec_succ, eval_succ
      pair<bool,bool> status = counter_example_translate_on_edge(c, e, ret, rand_vals, relevant_memlabels);
      return make_tuple(status.first, (status.second && evaluation_successful), false, ret, rand_vals);
    };
    std::function<tuple<bool, bool, bool, counter_example_t, counter_example_t> (tuple<bool, bool, bool, counter_example_t, counter_example_t> const &, tuple<bool, bool, bool, counter_example_t, counter_example_t> const &)> parf = [](tuple<bool, bool, bool, counter_example_t, counter_example_t> const &a, tuple<bool, bool, bool, counter_example_t, counter_example_t> const &b)
    {
      bool eval_status = get<1>(a) && get<1>(b);
      tuple<bool, bool, bool, counter_example_t, counter_example_t> ret_a = make_tuple(get<0>(a), eval_status, get<2>(a), get<3>(a), get<4>(a));
      tuple<bool, bool, bool, counter_example_t, counter_example_t> ret_b = make_tuple(get<0>(b), eval_status, get<2>(b), get<3>(b), get<4>(b));
      if (get<0>(a) && !get<0>(b)) {
        return ret_a;
      } else if (get<0>(b) && !get<0>(a)) {
        return ret_b;
      } else if(get<0>(a) && get<0>(b)){
        ASSERT(get<2>(a) || get<2>(b));
        if(get<2>(a))
          return ret_b; //b;
        else
          return ret_a; //a
      } else {
        ASSERT(!get<0>(a) && !get<0>(b));
        return ret_a; 
      }
    };
    //ASSERT(rand_vals.is_empty());
    CPP_DBG_EXEC4(CE_ADD, cout << __func__ << " " << __LINE__ << ": ce =\n" << ce.counter_example_to_string() << endl);
    tuple<bool, bool, bool, counter_example_t, counter_example_t> pr = pth->visit_nodes_preorder_series(atomf, parf, make_tuple(true, true, false, ce, rand_vals));
    ASSERT(!get<2>(pr));
    if (get<0>(pr)) {
      out_ce = get<3>(pr);
      rand_vals = get<4>(pr);
    }
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return make_pair(get<0>(pr), get<1>(pr));
  }

   virtual tfg const &get_src_tfg() const = 0; //src_tfg is the tfg against which the edge-guards are computed
   //virtual tfg const &get_dst_tfg() const = 0;

protected:
  bool counter_example_satisfies_preds_at_pc( counter_example_t const &ce, unordered_set<shared_ptr<T_PRED const>> const &preds, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels) const
  {
    autostop_timer func_timer(__func__);
    CPP_DBG_EXEC3(CE_ADD, cout << __func__ << " " << __LINE__ << ": ce =\n" << ce.counter_example_to_string() << endl);

    counter_example_t combo_ce = ce;
    combo_ce.counter_example_union(rand_vals);
    for (auto const& p : preds) {
      if (p->is_precond())
        continue; // skip asserting preconds

      expr_ref const &lhs_expr = p->get_lhs_expr();
      expr_ref const &rhs_expr = p->get_rhs_expr();
      expr_ref const &precond_expr = p->get_precond().precond_get_expr(this->get_src_tfg(), true);

      bool success;
      expr_ref lhs_const, rhs_const, precond_const;
      success = combo_ce.evaluate_expr_assigning_random_consts_as_needed(precond_expr, precond_const, rand_vals, relevant_memlabels);
      if (!success) {
        CPP_DBG_EXEC2(CE_ADD, cout << __func__ << " " << __LINE__ << ": evaluation over precond_expr failed: precond_expr = " << expr_string(precond_expr) << ".\n");
        CPP_DBG_EXEC2(CE_ADD, cout << __func__ << " " << __LINE__ << ": predicate: " << p->to_string(true) << "\n");
        return false;
      }
      ASSERT(precond_const->is_const());
      if (!precond_const->get_bool_value()) {
        continue; //if the precondition is not satisfied, this predicate is satisfied
      }
      success = combo_ce.evaluate_expr_assigning_random_consts_as_needed(lhs_expr, lhs_const, rand_vals, relevant_memlabels);
      if (!success) {
        CPP_DBG_EXEC2(CE_ADD, cout << __func__ << " " << __LINE__ << ": evaluation over lhs_expr failed: lhs_expr = " << expr_string(lhs_expr) << ".\n");
        CPP_DBG_EXEC2(CE_ADD, cout << __func__ << " " << __LINE__ << ": predicate: " << p->to_string(true) << "\n");
        CPP_DBG_EXEC3(CE_ADD, cout << __func__ << " " << __LINE__ << ": combo_ce =\n" << combo_ce.counter_example_to_string() << endl);
        return false;
      }
      ASSERT(lhs_const->is_const());
      success = combo_ce.evaluate_expr_assigning_random_consts_as_needed(rhs_expr, rhs_const, rand_vals, relevant_memlabels);
      if (!success) {
        CPP_DBG_EXEC2(CE_ADD, cout << __func__ << " " << __LINE__ << ": evaluation over rhs_expr failed: rhs_expr = " << expr_string(rhs_expr) << ".\n");
        CPP_DBG_EXEC2(CE_ADD, cout << __func__ << " " << __LINE__ << ": predicate: " << p->to_string(true) << "\n");
        return false;
      }
      ASSERT(rhs_const->is_const());
      if (lhs_const != rhs_const) {
        CPP_DBG_EXEC2(CE_ADD, cout << __func__ << " " << __LINE__ << ": lhs_expr != rhs_expr. CE propagation failed.\n");
        CPP_DBG_EXEC2(CE_ADD, cout << __func__ << " " << __LINE__ << ": predicate: " << p->to_string(true) << "\n");
        CPP_DBG_EXEC2(CE_ADD, cout << __func__ << " " << __LINE__ << ": combo_ce =\n" << combo_ce.counter_example_to_string() << endl);
        return false;
      }
    }
    return true;
  }

protected:
// returns exec_success, eval_success
  virtual pair<bool,bool> counter_example_translate_on_edge(counter_example_t const &counter_example, shared_ptr<T_E const> const &edge, counter_example_t &ret, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels) const = 0;

};

}
