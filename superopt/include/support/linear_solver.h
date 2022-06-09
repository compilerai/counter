#pragma once

#include <vector>
#include <map>

#include "support/debug.h"
#include "support/bv_const.h"
#include "support/globals_cpp.h"
#include "support/bvsolve_retval.h"

using namespace std;

namespace eqspace {

// Returns nullspace basis of matrix `points' in `soln',
// where each point in `points` is `mod(m)' and `m == 2^dim'
bvsolve_retval_t get_nullspace_basis(const vector<vector<bv_const>>& points, map<size_t, bv_solve_var_idx_t> const& rev_index_map, unsigned dim);

//void input_ldr_decompose_and_linear_solve_from_file(istream& ldr_ifs, vector<vector<bv_const>>& a_matrix, vector<vector<bv_const>>& b_matrix, vector<size_t>& var_ids, map<size_t, bv_solve_var_idx_t>& rev_index_map, unsigned& dim, unsigned& num_orig_vars);

bvsolve_retval_t
ldr_decompose_and_linear_solve(vector<vector<bv_const>>& ipoints, vector<vector<bv_const>>& id, vector<size_t> const& var_ids_in, map<size_t, bv_solve_var_idx_t> const& rev_index_map, unsigned const dim, unsigned num_orig_vars, vector<vector<bv_const>> const& orig_point_vecs = {});

void nullspace_basis_to_stream(ostream& out, map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> const& soln);

//void clear_col_for_row(vector<vector<bv_const>> const&a, vector<bv_const>& row, int pos, const bv_const& mod, unsigned int dim);
bool id_matrix_is_consistent_with_var_ids(vector<vector<bv_const>> const& id_matrix, vector<size_t> const& var_ids);
vector<vector<bv_const>> transpose(const vector<vector<bv_const>> &a);

void check_nullspace_algo_invariants(vector<vector<bv_const>> const& orig_point_vecs, vector<vector<bv_const>> const& ipoints, vector<vector<bv_const>> const& id, unsigned const dim);


}
