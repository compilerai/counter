#include "support/bv_solve.h"
#include "support/cl.h"
#include "support/globals_cpp.h"
#include "support/cl.h"
#include "gsupport/predicate.h"
#include "gsupport/memlabel_assertions.h"
#include "graph/prove.h"

#include <vector>
#include <iostream>
#include <string>
#include <fstream>

#define DEBUG_TYPE "main"

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
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  g_query_dir_init();
  g_ctx_init();
  context::config cfg(0, sage_query_timeout.get_value());
  if (use_sage.get_value())
    cfg.use_sage = true;
  g_ctx->set_config(cfg);

  unsigned d;
  vector<map<bv_solve_var_idx_t, bv_const>> input_points, input_points_select;
  ifstream matrix_ifs(matrix_file.get_value());

  input_points = bv_matrix_map2_from_stream(matrix_ifs, d);

  cout << "Input points:\n";
  for (auto const& pt : input_points) {
    cout << "[ " << bv_const_map2str(pt) << "]\n";
  }

  if (cols.get_value() != "") {
    vector<int> v = cl::parse_int_list(cols.get_value());
    size_t i = 0;
    for (auto const& row : input_points) {
      NOT_IMPLEMENTED();
      //auto select_row = select_elems<bv_const>(row, v);
      //input_points_select.push_back(select_row);
    }
    cout << "Input points select:\n";
    for (auto const& pt : input_points_select) {
      cout << "[ " << bv_const_map2str(pt) << "]\n";
    }
  } else {
    input_points_select = input_points;
  }

  DBG_SET(BV_SOLVE, 3);
  DYN_DBG_ELEVATE(bv_solve_debug, 3);


  bvsolve_retval_t bvsolve_retval = bv_solve_returning_intermediate_structures(input_points_select, d);

  cout << "bvsolve_retval =\n"; bvsolve_retval.bvsolve_retval_to_stream(cout);

  map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> soln = bvsolve_retval.get_solution();

  cout << "Solution:\n";
  for (auto const& soln_row : soln) {
    cout << soln_row.first << '\t' << "[ " << bv_const_map2str(soln_row.second) << " ]\n";
  }

  return 0;
}
