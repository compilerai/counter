#include "support/bv_solve.h"
#include "support/cl.h"
#include "support/globals_cpp.h"
#include "support/cl.h"
#include "tfg/predicate.h"
#include "graph/memlabel_assertions.h"
#include "graph/prove.h"

#include <vector>
#include <iostream>
#include <string>
#include <fstream>

#define DEBUG_TYPE "main"
static char as1[40960];

using namespace std;
using namespace eqspace;

template<typename T>
static vector<T>
select_elems(vector<T> const& a, vector<int> const& indices)
{
  vector<T> ret;
  for (auto const& i : indices) {
    ret.push_back(a.at(i));
  }
  return ret;
}
// input format:
// M: # of rows 
// N: # of columns,
// D: bitwidth
//
// M N D
// ...........................
// ...  M rows, N columns ...
// ...........................
int main(int argc, char **argv)
{
	cl::arg<string> matrix_file(cl::implicit_prefix, "", "path to .matrix file");
  cl::arg<string> cols(cl::explicit_prefix, "cols", "", "Columns to select from matrix; default: all");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::arg<bool> use_sage(cl::explicit_flag, "use_sage", false, "Select whether to use sage for solving BV equations or C++ implementation.  Default: false (Use C++ implementation)");
  cl::cl cmd("bv_solver : computes the null-space of the input matrix");
  cmd.add_arg(&matrix_file);
  cmd.add_arg(&cols);
  cmd.add_arg(&use_sage);
  cmd.parse(argc, argv);

  g_query_dir_init();
  g_ctx_init();
  context::config cfg(0, sage_query_timeout.get_value());
  if (use_sage.get_value())
    cfg.use_sage = true;
  g_ctx->set_config(cfg);

  unsigned m, n, d;
  vector<vector<bv_const>> input_points, input_points_select;
  ifstream matrix_ifs(matrix_file.get_value());

  matrix_ifs >> m >> n >> d;
  //cout << __func__ << " " << __LINE__ << ": m = " << m << endl;
  //cout << __func__ << " " << __LINE__ << ": n = " << n << endl;
  //cout << __func__ << " " << __LINE__ << ": d = " << d << endl;
  input_points.reserve(m);
  for (size_t i = 0; i < m; ++i) {
    vector<bv_const> ip;
    ip.reserve(n);
    for (size_t j = 0; j < n; ++j) {
      bv_const c;
      matrix_ifs >> c;
      //cout << __func__ << " " << __LINE__ << ": input_points[" << i << "][" << j << "] = " << c << endl;
      ip.push_back(c);
    }
    input_points.push_back(ip);
  }

  cout << "Input points:\n";
  for (auto const& pt : input_points) {
    cout << "[ " << bv_const_vector2str(pt) << "]\n";
  }

  if (cols.get_value() != "") {
    vector<int> v = cl::parse_int_list(cols.get_value());
    size_t i = 0;
    for (auto const& row : input_points) {
      auto select_row = select_elems<bv_const>(row, v);
      input_points_select.push_back(select_row);
    }
    cout << "Input points select:\n";
    for (auto const& pt : input_points_select) {
      cout << "[ " << bv_const_vector2str(pt) << "]\n";
    }
  } else {
    input_points_select = input_points;
  }

  DBG_SET(BV_SOLVE, 3);
  map<bv_solve_var_idx_t, vector<bv_const>> soln = bv_solve(input_points_select, d);

  cout << "Solution:\n";
  for (auto const& soln_row : soln) {
    cout << soln_row.first << '\t' << "[ " << bv_const_vector2str(soln_row.second) << " ]\n";
  }

  return 0;
}
