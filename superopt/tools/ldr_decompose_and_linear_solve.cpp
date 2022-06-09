#include <vector>
#include <iostream>
#include <string>
#include <fstream>

#include "support/bv_solve.h"
#include "support/cl.h"
#include "support/globals_cpp.h"
#include "support/cl.h"
#include "support/linear_solver.h"

#include "gsupport/predicate.h"
#include "gsupport/memlabel_assertions.h"

#include "graph/prove.h"

#define DEBUG_TYPE "main"

using namespace std;
using namespace eqspace;


int
main(int argc, char **argv)
{
  cl::cl cmd("ldr_decompose_and_linear_solve: computes the null-space of the LDR structure formed by matrix A [nxn], matrix B [nxn], and a vector VAR-IDs [n]");
	cl::arg<string> ldr_file(cl::implicit_prefix, "", "path to .ldr file");
  cmd.add_arg(&ldr_file);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  g_query_dir_init();
  g_ctx_init();
  //context::config cfg(0, 600);
  //g_ctx->set_config(cfg);

  unsigned d;
  ifstream ldr_ifs(ldr_file.get_value());

  map<size_t, bv_solve_var_idx_t> rev_index_map;
  vector<vector<bv_const>> a_matrix, b_matrix;
  unsigned int dim, num_orig_vars;
  vector<size_t> var_ids;
  string line;
  bool end;

/* Example file:

 1 0 0 0 0 -1 1
 1 0 0 0 2 1 0
 1 0 0 3 1 0 0
 1 4 0 1 1 0 0

 1 0 0 0 0 0 0
 0 1 0 0 0 0 0
 0 0 1 0 0 0 0
 0 0 0 1 0 0 0
 0 0 0 0 1 0 0
 0 0 0 0 0 1 0
 0 0 0 0 0 0 1

32

7

 0 1 2 3 4 5 6

 1 -> 0; 2 -> 1; 3 -> 2; 4 -> 3; 5 -> 4; 6 -> 5; 7 -> -2147483648;

Original matrix file for the example shown above:
matrix with bv-length 32
 0 -> 0; 1 -> 0; 2 -> 0; 3 -> 0; 4 -> -1; 5 -> 1
 0 -> 0; 1 -> 0; 2 -> 0; 3 -> 2; 4 -> 1; 5 -> 0
 0 -> 0; 1 -> 0; 2 -> 3; 3 -> 1; 4 -> 0; 5 -> 0
 0 -> 4; 1 -> 0; 2 -> 1; 3 -> 1; 4 -> 0; 5 -> 0
*/


  DBG_SET(BV_SOLVE, 3);
  DYN_DBG_ELEVATE(bv_solve_debug, 3);

  bvsolve_retval_t input = bvsolve_retval_t::bvsolve_retval_from_stream(ldr_ifs);
  end = !getline(ldr_ifs, line);
  ASSERT(!end);
  string const prefix = "dim: ";
  ASSERT(string_has_prefix(line, prefix));
  dim = string_to_int(line.substr(prefix.size()));
  //input_ldr_decompose_and_linear_solve_from_file(ldr_ifs, a_matrix, b_matrix, var_ids, rev_index_map, dim, num_orig_vars);

  auto points = input.get_points();
  auto id_matrix = input.get_id_matrix();
  //map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> soln;
  bvsolve_retval_t bvsolve_retval = ldr_decompose_and_linear_solve(points, id_matrix, input.get_var_ids(), input.get_rev_index_map(), dim, input.get_num_orig_vars());

  cout << "Solution:\n";
  for (auto const& soln_row : bvsolve_retval.get_solution()) {
    cout << soln_row.first << '\t' << "[ " << bv_const_map2str(soln_row.second) << " ]\n";
  }

  return 0;
}