#pragma once

#include <vector>
#include <map>

#include "support/types.h"
#include "support/bv_const.h"

using namespace std;

namespace eqspace {

/*
  Given a list of vectors (x_i) which are solution of equation
    a0 * x0 = a1 * x1 + a2 * x2 + ... + an * xn + c (mod m)

  where m == 2^dim
  bv_solve will try to determine values of coefficients ai's and c
*/
map<bv_solve_var_idx_t, vector<bv_const>> bv_solve(vector<vector<bv_const>> const& ipoints, const unsigned dim);

}

