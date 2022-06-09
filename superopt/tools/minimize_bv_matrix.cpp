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
#include "support/linear_solver.h"
#include "support/bv_const.h"
#include "support/bv_solve.h"

#include "expr/consts_struct.h"
#include "expr/z3_solver.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"

#include "graph/expr_group.h"

#define DEBUG_TYPE "minimize_bv_matrix.main"

using namespace std;
using namespace eqspace;


static string
get_varname(int varnum)
{
  return string("v") + int_to_string(varnum);
}


static map<expr_id_t, pair<expr_ref, expr_ref>>
get_ce_submap(context* ctx)
{
  map<expr_id_t, pair<expr_ref, expr_ref>> ret;
  sort_ref bv32_sort = ctx->mk_bv_sort(32);
  expr_ref v0 = ctx->mk_var(get_varname(1), bv32_sort);
  expr_ref v1 = ctx->mk_var(get_varname(2), bv32_sort);
  expr_ref v2 = ctx->mk_var(get_varname(3), bv32_sort);
  expr_ref v3 = ctx->mk_var(get_varname(4), bv32_sort);
  expr_ref v4 = ctx->mk_var(get_varname(5), bv32_sort);
  expr_ref v5 = ctx->mk_var(get_varname(6), bv32_sort);
  expr_ref v6 = ctx->mk_var(get_varname(7), bv32_sort);
  expr_ref v7 = ctx->mk_var(get_varname(8), bv32_sort);
  expr_ref v8 = ctx->mk_var(get_varname(9), bv32_sort);
  expr_ref v9 = ctx->mk_var(get_varname(10), bv32_sort);
  expr_ref v10 = ctx->mk_var(get_varname(11), bv32_sort);
  expr_ref v11 = ctx->mk_var(get_varname(12), bv32_sort);
  expr_ref v12 = ctx->mk_var(get_varname(13), bv32_sort);
  expr_ref v13 = ctx->mk_var(get_varname(14), bv32_sort);
  expr_ref v14 = ctx->mk_var(get_varname(15), bv32_sort);
  expr_ref v15 = ctx->mk_var(get_varname(16), bv32_sort);
  expr_ref v16 = ctx->mk_var(get_varname(17), bv32_sort);
  expr_ref v17 = ctx->mk_var(get_varname(18), bv32_sort);
  expr_ref v18 = ctx->mk_var(get_varname(19), bv32_sort);
  expr_ref v19 = ctx->mk_var(get_varname(20), bv32_sort);
  expr_ref v20 = ctx->mk_var(get_varname(21), bv32_sort);
  expr_ref v21 = ctx->mk_var(get_varname(22), bv32_sort);
  ret.insert(make_pair(v0->get_id(), make_pair(v0, ctx->mk_bv_const(32, 4294967295))));
  ret.insert(make_pair(v1->get_id(), make_pair(v1, ctx->mk_bv_const(32, 0))));
  ret.insert(make_pair(v2->get_id(), make_pair(v2, ctx->mk_bv_const(32, 0))));
  ret.insert(make_pair(v3->get_id(), make_pair(v3, ctx->mk_bv_const(32, 0))));
  ret.insert(make_pair(v4->get_id(), make_pair(v4, ctx->mk_bv_const(32, 0))));
  ret.insert(make_pair(v5->get_id(), make_pair(v5, ctx->mk_bv_const(32, 84))));
  ret.insert(make_pair(v6->get_id(), make_pair(v6, ctx->mk_bv_const(32, 506))));
  ret.insert(make_pair(v7->get_id(), make_pair(v7, ctx->mk_bv_const(32, 1))));
  ret.insert(make_pair(v8->get_id(), make_pair(v8, ctx->mk_bv_const(32, 182))));
  ret.insert(make_pair(v9->get_id(), make_pair(v9, ctx->mk_bv_const(32, 54))));
  ret.insert(make_pair(v10->get_id(), make_pair(v10, ctx->mk_bv_const(32, 194))));
  ret.insert(make_pair(v11->get_id(), make_pair(v11, ctx->mk_bv_const(32, 108230))));
  ret.insert(make_pair(v12->get_id(), make_pair(v12, ctx->mk_bv_const(32, 55175))));
  ret.insert(make_pair(v13->get_id(), make_pair(v13, ctx->mk_bv_const(32, 86))));
  ret.insert(make_pair(v14->get_id(), make_pair(v14, ctx->mk_bv_const(32, 230))));
  ret.insert(make_pair(v15->get_id(), make_pair(v15, ctx->mk_bv_const(32, 246))));
  ret.insert(make_pair(v16->get_id(), make_pair(v16, ctx->mk_bv_const(32, 0))));
  ret.insert(make_pair(v17->get_id(), make_pair(v17, ctx->mk_bv_const(32, 1))));
  ret.insert(make_pair(v18->get_id(), make_pair(v18, ctx->mk_bv_const(32, 1))));
  ret.insert(make_pair(v19->get_id(), make_pair(v19, ctx->mk_bv_const(32, 12))));
  ret.insert(make_pair(v20->get_id(), make_pair(v20, ctx->mk_bv_const(32, 1))));
  ret.insert(make_pair(v21->get_id(), make_pair(v21, ctx->mk_bv_const(32, 1))));

  return ret;
}



class binary_search_on_matrix_rows_t : public binary_search_t
{
public:
  binary_search_on_matrix_rows_t(vector<map<bv_solve_var_idx_t, bv_const>> const& matrix, std::function<bool (vector<map<bv_solve_var_idx_t, bv_const>> const&)> const& f)
    : m_matrix(matrix), m_f(f)
  { }

  static vector<map<bv_solve_var_idx_t, bv_const>> select_rows(vector<map<bv_solve_var_idx_t, bv_const>> const& m, set<size_t> const& select_indices)
  {
    vector<map<bv_solve_var_idx_t, bv_const>> ret;
    for (size_t i = 0; i < m.size(); i++) {
      if (set_belongs(select_indices, i)) {
        ret.push_back(m.at(i));
      }
    }
    return ret;
  }

private:
  bool test(set<size_t> const& indices) const override
  {
    //cout << _FNLN_ << ": indices =";
    //for (auto i : indices) { cout << " " << i; }
    //cout << "...\n";

    vector<map<bv_solve_var_idx_t, bv_const>> new_matrix = select_rows(m_matrix, indices);
    return m_f(new_matrix);
  }

private:
  vector<map<bv_solve_var_idx_t, bv_const>> const& m_matrix;
  std::function<bool (vector<map<bv_solve_var_idx_t, bv_const>> const&)> const& m_f;
};

static vector<map<bv_solve_var_idx_t, bv_const>>
minimize_rows(vector<map<bv_solve_var_idx_t, bv_const>> const& matrix, unsigned& num_rows, std::function<bool (vector<map<bv_solve_var_idx_t, bv_const>> const&)> const& f)
{
  binary_search_on_matrix_rows_t binsearch(matrix, f);
  set<size_t> selected_rows = binsearch.do_binary_search(identity_set(num_rows));
  num_rows = selected_rows.size();
  return binary_search_on_matrix_rows_t::select_rows(matrix, selected_rows);
}

class binary_search_on_matrix_columns_t : public binary_search_t
{
public:
  binary_search_on_matrix_columns_t(vector<map<bv_solve_var_idx_t, bv_const>> const& matrix, std::function<bool (vector<map<bv_solve_var_idx_t, bv_const>> const&)> const& f)
    : m_matrix(matrix), m_f(f)
  { }

  static vector<map<bv_solve_var_idx_t, bv_const>> select_columns(vector<map<bv_solve_var_idx_t, bv_const>> const& m, set<size_t> const& select_indices)
  {
    vector<map<bv_solve_var_idx_t, bv_const>> ret;
    for (size_t i = 0; i < m.size(); i++) {
      map<bv_solve_var_idx_t, bv_const> new_row;
      auto iter = m.at(i).begin();
      for (size_t j = 0; j < m.at(i).size(); j++, iter++) {
        if (set_belongs(select_indices, j)) {
          new_row.insert(make_pair(iter->first, iter->second));
        }
      }
      ret.push_back(new_row);
    }
    return ret;
  }

private:
  bool test(set<size_t> const& indices) const override
  {
    cout << _FNLN_ << ": indices =";
    for (auto i : indices) { cout << " " << i; }
    cout << "...\n";

    vector<map<bv_solve_var_idx_t, bv_const>> new_matrix = select_columns(m_matrix, indices);
    return m_f(new_matrix);
  }

private:
  vector<map<bv_solve_var_idx_t, bv_const>> const& m_matrix;
  std::function<bool (vector<map<bv_solve_var_idx_t, bv_const>> const&)> const& m_f;
};

static vector<map<bv_solve_var_idx_t, bv_const>>
minimize_columns(vector<map<bv_solve_var_idx_t, bv_const>> const& matrix, unsigned& num_columns, std::function<bool (vector<map<bv_solve_var_idx_t, bv_const>> const&)> const& f)
{
  binary_search_on_matrix_columns_t binsearch(matrix, f);
  set<size_t> selected_columns = binsearch.do_binary_search(identity_set(num_columns));
  num_columns = selected_columns.size();
  return binary_search_on_matrix_columns_t::select_columns(matrix, selected_columns);
}

static vector<map<bv_solve_var_idx_t, bv_const>>
minimize_bitwidth(vector<map<bv_solve_var_idx_t, bv_const>> const& matrix, unsigned& d, std::function<bool (vector<map<bv_solve_var_idx_t, bv_const>> const&)> const& f)
{
  vector<map<bv_solve_var_idx_t, bv_const>> ret = matrix;
  for (size_t i = 1; i < d; i++) {
    vector<map<bv_solve_var_idx_t, bv_const>> new_matrix = matrix;
    for (auto& row : new_matrix) {
      for (auto& elem : row) {
        elem.second >>= i;
      }
    }
    if (!f(new_matrix)) {
      d -= i;
      break;
    }
    ret = new_matrix;
  }
  return ret;
}

static expr_ref
get_expr_for_vector(context* ctx, map<bv_solve_var_idx_t, bv_const> const& vec, unsigned d)
{
  expr_ref zerobv = ctx->mk_zerobv(d);
  expr_ref ret = zerobv;
  //cout << _FNLN_ << ": vec.size() = " << vec.size() << endl;
  for (auto const& [i, vi] : vec) {
    expr_ref v = ctx->mk_var(get_varname(i), ctx->mk_bv_sort(d));
    //expr_ref c = ctx->mk_bv_const(d, mybitset(vec.at(i), d));
    expr_ref c = ctx->mk_bv_const(d, mybitset(vi, d));
    ret = expr_bvadd(ret, expr_bvmul(c, v));
  }
    {
      map<expr_id_t, pair<expr_ref, expr_ref>> submap = get_ce_submap(ctx);

      expr_ref esub = ctx->expr_substitute(ret, submap);
      esub = ctx->expr_do_simplify(esub);
      cout << _FNLN_ << ": ret-sub =\n" << ctx->expr_to_string_table(esub) << endl;
    }


  ret = expr_eq(ret, zerobv);
  return ret;
}

static expr_ref
get_expr_for_nullspace_basis(context* ctx, map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> const& nullspace_basis, unsigned d)
{
  expr_ref ret = expr_true(ctx);
  for (auto const& nb : nullspace_basis) {

    expr_ref e = get_expr_for_vector(ctx, nb.second, d);
    {
      map<expr_id_t, pair<expr_ref, expr_ref>> submap = get_ce_submap(ctx);

      expr_ref esub = ctx->expr_substitute(e, submap);
      esub = ctx->expr_do_simplify(esub);
      cout << _FNLN_ << ": solve_var_idx " << nb.first << ", esub =\n" << ctx->expr_to_string_table(esub) << endl;
    }
    ret = expr_and(ret, e);
  }
  return ret;
}

int main(int argc, char **argv)
{
  // command line args processing
  cl::cl cmd("minimize_bv_matrix : keep reducing a matrix till it satisfies a condition.  The condition is represented as a lambda function (need to change the code of tools/minimize_bv_matrix to encode the condition)");
  cl::arg<string> matrix_file(cl::implicit_prefix, "", "path to input file");
  cmd.add_arg(&matrix_file);
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 60, "Timeout per query (s)");
  cmd.add_arg(&smt_query_timeout);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  //DYN_DBG_ELEVATE(prove_dump, 1);

  context::config cfg(smt_query_timeout.get_value(), 100/*sage_query_timeout*/);
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();

  g_query_dir_init();
  //src_init();
  //dst_init();

  ifstream matrix_ifs(matrix_file.get_value());
  unsigned m, n, d;
  vector<vector<bv_const>> matrix_vec = bv_matrix_from_stream(matrix_ifs, m, n, d);
  matrix_ifs.close();
  vector<map<expr_group_t::expr_idx_t, bv_const>> matrix;
  for (auto const& row : matrix_vec) {
    map<expr_group_t::expr_idx_t, bv_const> new_row;
    expr_group_t::expr_idx_t i = expr_group_t::expr_idx_begin();
    for (auto const& elem : row) {
      new_row.insert(make_pair(i++, elem));
    }
    matrix.push_back(new_row);
  }

  std::function<bool (vector<map<bv_solve_var_idx_t, bv_const>> const&)> f =
    [&ctx,d](vector<map<bv_solve_var_idx_t, bv_const>> const& matrix)
  {
    vector<map<bv_solve_var_idx_t, bv_const>> truncated = matrix;
    truncated.pop_back();
    map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> nullspace_basis, trunc_nullspace_basis;
    nullspace_basis = bv_solve(matrix, d);
    cout << "Nullspace basis:\n";
    nullspace_basis_to_stream(cout, nullspace_basis);
    trunc_nullspace_basis = bv_solve(truncated, d);
    cout << "\n\nNullspace basis for the truncated matrix:\n";
    nullspace_basis_to_stream(cout, trunc_nullspace_basis);
    expr_ref trunc_expr = get_expr_for_nullspace_basis(&ctx, trunc_nullspace_basis, d);

    map<expr_id_t, pair<expr_ref, expr_ref>> submap = get_ce_submap(&ctx);

    expr_ref trunc_expr_sub = ctx.expr_substitute(trunc_expr, submap);
    trunc_expr_sub = ctx.expr_do_simplify(trunc_expr_sub);
    cout << _FNLN_ << ": trunc_expr_sub =\n" << ctx.expr_to_string_table(trunc_expr_sub) << endl;

    for (auto const& nb : nullspace_basis) {
      expr_ref row_expr = get_expr_for_vector(&ctx, nb.second, d);
      expr_ref e = expr_implies(trunc_expr, row_expr);
      query_comment qc(string("minimize_bv_matrix.f"));
      cout << _FNLN_ << ": looking at bv_solve_var_idx " << nb.first << ":\n";// << ctx.expr_to_string_table(e) << endl;
      list<counter_example_t> ces;

      cout << "row_expr =\n" << ctx.expr_to_string_table(row_expr) << endl << endl;
      //{
      //  expr_ref row_expr_sub = ctx.expr_substitute(row_expr, submap);
      //  map<expr_id_t, expr_ref> row_expr_sub_map = ctx.expr_to_expr_map(row_expr_sub);
      //  counter_example_t dummy_ce(&ctx);
      //  map<expr_id_t, expr_ref> row_expr_sub_map_eval = evaluate_counter_example_on_expr_map(row_expr_sub_map, set<memlabel_ref>(), dummy_ce);

      //  cout << "row_expr_sub =\n";
      //  expr_print_with_ce_visitor visitor(row_expr_sub, row_expr_sub_map, row_expr_sub_map_eval, cout);
      //  visitor.print_result();
      //  cout << _FNLN_ << ": bv_solve_idx " << nb.first << ": row_expr_sub =\n" << ctx.expr_to_string_table(row_expr_sub) << endl;
      //  row_expr_sub = ctx.expr_do_simplify(row_expr_sub);
      //  cout << _FNLN_ << ": bv_solve_idx " << nb.first << ": row_expr_sub = " << ctx.expr_to_string_table(row_expr_sub) << endl;
      //}

      expr_ref esub = ctx.expr_substitute(e, submap);
      {
        map<expr_id_t, expr_ref> esub_map = ctx.expr_to_expr_map(esub);
        counter_example_t dummy_ce(&ctx, "dummy-ce");
        map<expr_id_t, expr_ref> esub_map_eval = evaluate_counter_example_on_expr_map(esub_map, set<memlabel_ref>(), dummy_ce);

        cout << "esub =\n";
        expr_print_with_ce_visitor visitor(esub, esub_map, esub_map_eval, cout);
        visitor.print_result();
      }
      esub = ctx.expr_do_simplify(esub);

      cout << _FNLN_ << ": esub simplified =\n" << ctx.expr_to_string_table(esub) << endl;
      ASSERT(esub->is_const());
      solver_res res;
      if (esub == expr_false(&ctx)) {
        res = solver_res_false;
      } else {
        NOT_IMPLEMENTED();
        //res = ctx.expr_is_provable(e, qc, relevant_memlabels_t(set<memlabel_ref>()), ces);
      }
      cout << _FNLN_ << ": returned " << solver::solver_res_to_string(res) << endl;
      if (res == solver_res_false) {
        return true;
      }
    }
    return false;
  };

  if (!f(matrix)) {
    cout << "Fatal, f() evaluates to false on the input matrix. Exiting...\n";
    exit(1);
  }
  cout << "f() evaluates to true on the input matrix.\n";
  cout << "Minimizing the rows...\n";
  matrix = minimize_rows(matrix, m, f);
  cout << "Minimizing the columns...\n";
  matrix = minimize_columns(matrix, n, f);
  cout << "Minimizing the bitwidth...\n";
  matrix = minimize_bitwidth(matrix, d, f);

  bv_matrix_map2_to_stream(cout, matrix);

  solver_kill();
  call_on_exit_function();

  exit(0);
}
