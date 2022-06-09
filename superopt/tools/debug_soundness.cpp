#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"

#include "support/timers.h"
#include "expr/z3_solver.h"

#include "support/dyn_debug.h"
#include "eq/corr_graph.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

template<typename T>
list<list<T>>
eliminate_non_relevant_elements(list<T> const& ls, list<T> const& pre, function<bool(list<T> const&)> has_relevant_elements, function<list<T>(list<T> const&,list<T> const&)> ls_combine)
{
  cout << __func__ << ": input list size = " << ls.size() << endl;
  if (ls.empty()) {
    return {};
  }
  if (!has_relevant_elements(ls_combine(ls, pre))) {
    return {};
  }
  if (ls.size() == 1) {
    return { ls };
  }
  auto [lh,rh] = list_split_into_two(ls, ls.size()/2);

  bool l_rel = has_relevant_elements(ls_combine(lh, pre));
  bool r_rel = has_relevant_elements(ls_combine(rh, pre));

  if (l_rel && r_rel) {
    auto ret_l = eliminate_non_relevant_elements(lh, pre, has_relevant_elements, ls_combine);
    auto ret_r = eliminate_non_relevant_elements(rh, pre, has_relevant_elements, ls_combine);
    ret_l.insert(ret_l.end(), ret_r.begin(), ret_r.end());
    return ret_l;
  }
  else if (l_rel) {
    return eliminate_non_relevant_elements(lh, pre, has_relevant_elements, ls_combine);
  }
  else if (r_rel) {
    return eliminate_non_relevant_elements(rh, pre, has_relevant_elements, ls_combine);
  }
  else {
    auto ret_l = eliminate_non_relevant_elements(lh, ls_combine(rh, pre), has_relevant_elements, ls_combine);
    auto ret_r = eliminate_non_relevant_elements(rh, ls_combine(lh, pre), has_relevant_elements, ls_combine);

    list<list<T>> ret;
    for (auto const& l : ret_l) {
      for (auto const& r : ret_r) {
        auto t = l;
        t.insert(t.end(), r.begin(), r.end());
        ret.push_back(t);
      }
    }
    return ret;
  }
}

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to input file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cl::arg<string> eval_expr(cl::explicit_prefix, "eval-expr", "", "Evaluate the returned counter example (if any) on expr in this file");
  cl::arg<bool> elim_alias_cons(cl::explicit_flag, "elim-alias-cons", false, "Eliminate non-offending aliasing constraints");
  cl::arg<bool> validate(cl::explicit_flag, "validate", false, "Validate the resulting offending preds set.");
  cl::cl cmd("Soundness debugger -- uses binary search for identifying offending predicate(s)");
  cmd.add_arg(&expr_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&eval_expr);
  cmd.add_arg(&debug);
  cmd.add_arg(&elim_alias_cons);
  cmd.add_arg(&validate);
  cmd.parse(argc, argv);

  //DYN_DBG_SET(ce_add, 4);
  //DYN_DBG_SET(smt_query, 2);
  //DYN_DBG_SET(smt_file, 1);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  context::config cfg(smt_query_timeout.get_value());
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();
  g_query_dir_init();

  ifstream in(expr_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
    exit(1);
  }

  bool eliminate_preds = !elim_alias_cons.get_value();
  bool eliminate_aliasing_constraints = elim_alias_cons.get_value();

  list<guarded_pred_t> lhs_set;
  precond_t precond;
  sprel_map_pair_t sprel_map_pair;
  tfg_suffixpath_t src_suffixpath;
  avail_exprs_t src_avail_exprs;
  aliasing_constraints_t alias_cons;
  graph_memlabel_map_t memlabel_map;
  expr_ref src, dst;
  map<expr_id_t, expr_ref> src_map, dst_map;
  map<expr_id_t, pair<expr_ref, expr_ref>> concrete_address_submap;
  //dshared_ptr<memlabel_assertions_t> mlasserts;
  dshared_ptr<tfg_llvm_t> src_tfg;
  graph_edge_composition_ref<pc, tfg_edge> ec;

  relevant_memlabels_t relevant_memlabels({});

  read_lhs_set_guard_etc_and_src_dst_from_file(in, &ctx, lhs_set, precond, ec, sprel_map_pair, src_suffixpath, src_avail_exprs, alias_cons, memlabel_map, src, dst, src_map, dst_map, concrete_address_submap, src_tfg, relevant_memlabels);
  src_tfg->populate_auxilliary_structures_dependent_on_locs();

  if (eliminate_preds) {
    function<bool(list<guarded_pred_t> const&)> has_offender =
      [&](list<guarded_pred_t> lset) -> bool
      {
        std::atomic<bool> should_return_sharedvar;
        query_comment qc("has_offender." + to_string(lset.size()));
        auto [ret, returned_ces] = prove(&ctx, lset, precond, ec, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, expr_true(&ctx), expr_false(&ctx), qc, concrete_address_submap, relevant_memlabels, alias_cons, *src_tfg, should_return_sharedvar);
        return (ret == proof_status_proven);
      };
    function<list<guarded_pred_t>(list<guarded_pred_t> const&, list<guarded_pred_t> const&)> combine_ls =
      [](list<guarded_pred_t> const& xs, list<guarded_pred_t> const& ys)
      {
        auto ret = xs;
        guarded_predicate_set_union(ret, ys);
        return ret;
      };
    cout << "Eliminating non-offending preds..." << endl;
    list<list<guarded_pred_t>> offending_predls = eliminate_non_relevant_elements(lhs_set, {}, has_offender, combine_ls);
    cout << "Predicate sets which prove false:\n\n";
    int i = 1;
    for (auto const& ps : offending_predls) {
      cout << "++++ Group #" << i << " ++++ \n";
      int j = 1;
      for (auto const& p : ps) {
        ostringstream os;
        os << "=lhs-guarded-pred" << i << '.' << j;
        guarded_pred_to_stream(cout, p, os.str());
        ++j;
      }
      cout << endl;
      if (validate.get_value()) {
        auto ret = has_offender(ps);
        if (!ret) {
          cout << "Validation failed!!";
          break;
        }
      }
      ++i;
    }
  } else if (eliminate_aliasing_constraints) {
    function<bool(list<aliasing_constraint_t> const&)> has_offender =
      [&](list<aliasing_constraint_t> alias_cons_ls) -> bool
      {
        std::atomic<bool> should_return_sharedvar;
        query_comment qc("has_offender." + to_string(alias_cons_ls.size()));
        aliasing_constraints_t alias_cons_set(set<aliasing_constraint_t>(alias_cons_ls.begin(), alias_cons_ls.end()));
        auto [ret, returned_ces] = prove(&ctx, lhs_set, precond, ec, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, expr_true(&ctx), expr_false(&ctx), qc, concrete_address_submap, relevant_memlabels, alias_cons_set, *src_tfg, should_return_sharedvar);
        return (ret == proof_status_proven);
      };
    function<list<aliasing_constraint_t>(list<aliasing_constraint_t> const&, list<aliasing_constraint_t> const&)> combine_ls =
      [](list<aliasing_constraint_t> const& xs, list<aliasing_constraint_t> const& ys)
      {
        auto ret = xs;
        list_append(ret, ys);
        return ret;
      };
    cout << "Eliminating non-offending aliasing constraints..." << endl;
    auto alias_cons_set = alias_cons.get_ls();
    list<aliasing_constraint_t> alias_cons_ls(alias_cons_set.begin(), alias_cons_set.end());
    list<list<aliasing_constraint_t>> offending_aliascons_ls = eliminate_non_relevant_elements(alias_cons_ls, {}, has_offender, combine_ls);
    cout << "Aliasing contraints which prove false:\n\n";
    int i = 1;
    for (auto const& als : offending_aliascons_ls) {
      cout << "++++ Group #" << i << " ++++ \n";
      aliasing_constraints_t acons(set<aliasing_constraint_t>(als.begin(),als.end()));
      cout << acons.to_string_for_eq() << endl;
      if (validate.get_value()) {
        auto ret = has_offender(als);
        if (!ret) {
          cout << "Validation failed!!";
          break;
        }
      }
      ++i;
    }
  }

  solver_kill();
  call_on_exit_function();

  exit(0);
}
