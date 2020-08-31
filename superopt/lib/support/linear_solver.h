#pragma once

#include <vector>
#include <map>

#include "support/debug.h"
#include "support/bv_const.h"
#include "support/globals_cpp.h"

using namespace std;

namespace eqspace {

// Returns nullspace basis of matrix `points' in `soln',
// where each point in `points` is `mod(m)' and `m == 2^dim'
void get_nullspace_basis(const vector<vector<bv_const>>& points, const unsigned dim, map<bv_solve_var_idx_t, vector<bv_const>>& soln);

}
