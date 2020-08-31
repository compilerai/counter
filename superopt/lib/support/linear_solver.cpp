// Original author: Praveen Kulkarni (praveenkulkarni1996@gmail.com)

#include <vector>
#include <fstream>
#include "support/bv_const.h"
#include "support/bv_solve.h"
#include "support/utils.h"
#include "support/globals_cpp.h"
#include "support/mytimer.h"
#include "support/query_comment.h"

using namespace std;

namespace eqspace {

// Finds the position of the element within the a[from ... n][from ... n]
// submatrix with the smallest rank
static pair<int, int> find_pivot(const vector<vector<bv_const>> &a, const int from, const bv_const& mod, unsigned int dim) {
  const int m = a.size();
  const int n = a[0].size();
  pair<int, int> pivot = {from, from};
  for (int row = from; row < m; row++) {
    for (int col = from; col < n; col++) {
      if (bv_rank(a[pivot.first][pivot.second],dim) > bv_rank(a[row][col],dim)) {
        pivot = {row, col};
      }
    }
  }
  return pivot;
}

// Performs one row and column operations to move the `from` element to `to` position
static void perform_pivot(vector<vector<bv_const>> &a, vector<vector<bv_const>> &b, vector<int> &var_ids, pair<int, int> const& from, pair<int, int> const& to) {
  const int m = a.size();
  const int n = a[0].size();
  swap(a[from.first], a[to.first]);
  for (int row = 0; row < m; row++) {
    swap(a[row][from.second], a[row][to.second]);
  }
  for (int row = 0; row < n; row++) {
    swap(b[row][from.second], b[row][to.second]);
  }
  swap(var_ids[from.second], var_ids[to.second]);
}

inline static void SUBMUL(bv_const& out, bv_const const& a, bv_const const& b, bv_const const& mod, bv_const& tmp)
{
  tmp = out;
  mpz_submul(tmp.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  mpz_mod(out.get_mpz_t(), tmp.get_mpz_t(), mod.get_mpz_t());
}
inline static void MUL(bv_const& a, bv_const const& b, bv_const const& mod, bv_const& out)
{
  mpz_mul(out.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  mpz_mod(out.get_mpz_t(), out.get_mpz_t(), mod.get_mpz_t());
}

static void clear_row(vector<vector<bv_const>> &a, vector<vector<bv_const>> &b, int pos, const bv_const& mod, unsigned int dim) {
  const int m = a.size();
  const int n = a[0].size();
  assert(m <= n);
  const unsigned rank = bv_rank(a[pos][pos], dim);
  bv_const odd = a[pos][pos] >> rank;
  bv_const mod_val = bv_exp2(dim - rank);
  bv_const inv = bv_mulinv(odd, mod_val);
  assert(1 == bv_mod(odd * inv, mod_val));

  vector<bv_const> mul_factor(n, 0);
  for (int col = 0; col < n; col++) {
    if (col == pos) continue;
    if (a[pos][col] == 0) continue;
    unsigned this_rank = bv_rank(a[pos][col],dim);
    //assert(this_rank >= rank);
    bv_const odd2 = a[pos][col] >> this_rank;
    mul_factor[col] = bv_mod(bv_mod(odd2 * inv, mod) << (this_rank - rank), mod);
    assert (bv_mod(mul_factor[col] * a[pos][pos], mod) == a[pos][col]);
  }

  vector<bv_const> acache(m), bcache(n);

  for (int row = 0; row < m; row++) {
    acache[row] = a[row][pos];
  }

  for (int row = 0; row < n; row++) {
    bcache[row] = b[row][pos];
  }

  bv_const tmp;
  for (int row = 0; row < m; row++) {
    for (int col = 0; col < n; col++) {
      SUBMUL(a[row][col], acache[row], mul_factor[col], mod, tmp);
    }
  }
  for (int row = 0; row < n; row++) {
    for (int col = 0; col < n; col++) {
      SUBMUL(b[row][col], bcache[row], mul_factor[col], mod, tmp);
    }
  }
}

static void clear_col(vector<vector<bv_const>> &a, int pos, const bv_const& mod, unsigned int dim) {
  const int m = a.size();
  const int n = a[0].size();
  assert (m <= n);
  const unsigned rank = bv_rank(a[pos][pos], dim);
  bv_const odd = a[pos][pos] >> rank;
  bv_const mod_val = bv_exp2(dim - rank);
  bv_const inv = bv_mulinv(odd, mod);
  ASSERT(1 == bv_mod(odd * inv, mod));
  bv_const tmp;
  for (int row = 0; row < m; row++) {
    if (row == pos)
      continue;
    if (a[row][pos] == 0)
      continue;
    unsigned this_rank = bv_rank(a[row][pos], dim);
    ASSERT(this_rank >= rank);
    bv_const odd2 = a[row][pos] >> this_rank;
    bv_const m = bv_mod(bv_mod(odd2*inv, mod) << (this_rank-rank), mod);
    for (int col = 0; col < n; col++) {
      SUBMUL(a[row][col], a[pos][col], m, mod, tmp);
    }
  }
  CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after clearing other rows for pivot column, a =\n"; print_2d_matrix<bv_const>(a));
  for (int col = 0; col < n; col++) {
    bv_const tmp;
    MUL(a[pos][col], inv, mod, tmp);
    a[pos][col] = tmp;
  }
  CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after clearing pivot row for pivot column, a =\n"; print_2d_matrix<bv_const>(a));
}

static void ldr_decomposition(vector<vector<bv_const>> &a, vector<vector<bv_const>> &b, vector<int> &var_ids, const bv_const& mod, unsigned int dim) {
  const int m = a.size();
  const int n = a[0].size();
  assert(m <= n);
  for (int pos = 0; pos < m; pos++) {
    pair<int, int> target = {pos, pos};
    pair<int, int> source = find_pivot(a, pos, mod, dim);
    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": pos = " << pos << ", target = (" << target.first << "," << target.second << "), source pivot = (" << source.first << "," << source.second << ")\n");
    perform_pivot(a, b, var_ids, source, target);
    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after perform pivot, a =\n"; print_2d_matrix<bv_const>(a); cout << "\nb =\n"; print_2d_matrix<bv_const>(b));
    if (a[pos][pos] == 0)
      break;
    clear_col(a, pos, mod, dim);
    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after clear col, a =\n"; print_2d_matrix<bv_const>(a));
    clear_row(a, b, pos, mod, dim);
    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after clear row, a =\n"; print_2d_matrix<bv_const>(a); cout << "\nb =\n"; print_2d_matrix<bv_const>(b));
  }
}

static vector<vector<bv_const>> generate_solutions(const vector<vector<bv_const>> &a, const bv_const& mod, unsigned int dim) {
  const int m = a.size();
  const int n = a[0].size();
  assert (m <= n);
  vector<vector<bv_const>> soln(n, vector<bv_const>(n, 0));
  for (int pos = 0; pos < m; pos++) {
    soln[pos][pos] = bv_exp2(dim - bv_rank(a[pos][pos],dim));
  }
  for (int pos = m; pos < n; pos++) {
    soln[pos][pos] = 1;
  }
  return soln;
}

static vector<vector<bv_const>> transpose(const vector<vector<bv_const>> &a)
{
  if (a.empty() or a[0].empty()) return vector<vector<bv_const>>();
  const int n = a.size();
  const int m = a[0].size();
  vector<vector<bv_const>> b(m, vector<bv_const>(n, 0));
  for (int row = 0; row < n; row++) {
    for (int col = 0; col < m; col++) {
      b[col][row] = a[row][col];
    }
  }
  return b;
}

static bool zeros (const vector<bv_const> &v)
{
  for (const bv_const &x: v) {
    if (x != 0) return false;
  }
  return true;
}

static void eliminate_zero_rows(map<bv_solve_var_idx_t, vector<bv_const>> &a)
{
	//for (int row = a.size() - 1; row >= 0; row--) {
  //  if (zeros(a[row])) {
  //    a.erase(a.begin() + row);
  //  }
  //}
  set<bv_solve_var_idx_t> to_erase;
  for (auto &vrow : a) {
    auto &row = vrow.second;
    bv_solve_var_idx_t var_idx = vrow.first;
    if (zeros(row)) {
      to_erase.insert(var_idx);
    }
  }
  for (auto var_idx : to_erase) {
    a.erase(var_idx);
  }
}

static void normalize_constant_term(vector<vector<bv_const>> &a, const bv_const& mod, unsigned int dim)
{
	for(auto &row : a) {
		// move constant coeff to last
		bv_const constant_coeff = row[0];
		row.erase(row.begin());
		row.push_back(constant_coeff);
  }
}

static void make_square_matrix_by_adding_zero_columns(vector<vector<bv_const>>& points)
{
  unsigned num_pts = points.size();
  unsigned num_vars = points[0].size();
  ASSERT(num_pts > num_vars);

  vector<bv_const> zero_vars(num_pts - num_vars);
  for (auto &row : points) {
    row.insert(row.end(), zero_vars.begin(), zero_vars.end());
  }
}

static void clip_extra_vars(map<bv_solve_var_idx_t, vector<bv_const>> &a, unsigned int num_added_vars)
{
	unsigned int idx_first_extra_var = a.begin()->second.size() - num_added_vars -1; //last is constant term
	for (auto &vrow : a) {
    auto &row = vrow.second;
    row.erase(row.begin()+idx_first_extra_var, row.end()-1);
  }
}

inline static void MOD(bv_const& out, bv_const const& x, bv_const const& mod)
{
  mpz_mod(out.get_mpz_t(), x.get_mpz_t(), mod.get_mpz_t());
}

void convert_red_ech_form(map<bv_solve_var_idx_t, vector<bv_const>> &amap, const bv_const& mod, unsigned int dim)
{
  autostop_timer func_timer(__func__);
  vector<bv_solve_var_idx_t> vars;
  vector<vector<bv_const>> a;
  for (auto const& amapping : amap) {
    vars.push_back(amapping.first);
    a.push_back(amapping.second);
  }

  if (a.size() == 0) return;
  const int n = a.size();
  bv_const tmp;
  for (int i = 0, j = 0; i < n; i++) {
    CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": before iteration " << i << " matrix \n"; print_2d_matrix<bv_const>(a));
    CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": before iteration " << i << " vars \n"; 
        for(auto const &v : vars) 
          cout << v << "  ";
        cout << endl;
          );
    // find the row with the element of the least rank at the `i`th position
    int pivot_rank = dim+1; // rank can never exceed dim; choose `dim+1` as MAX
    int pivot_row = -1;
    for (int row = j; row < (int)a.size(); row++) {
      if (a[row][i] != 0) {
        if (bv_rank(a[row][i], dim) < pivot_rank) {
          pivot_rank = bv_rank(a[row][i],dim);
          pivot_row = row;
        }
      }
    }

    // ... and swap it at the j'th position
    if (pivot_row == -1)
      continue;
    swap(a[j], a[pivot_row]);
    swap(vars[j], vars[pivot_row]);

    const int p1 = bv_rank(a[j][i], dim);
    const bv_const u = a[j][i] >> p1;

    // ensure that a[j][i] is a power of 2
//    if (u != 1) {
//      bv_const mod_val = bv_exp2(dim - p);
//      bv_const uinv = bv_mulinv(u, mod_val);
//      for (bv_const &x: a[j]) {
//        tmp = x * uinv;
//        MOD(x, tmp, mod);
//      }
//    }
//
//    {// can remove this assertion
//      auto x = a[j][i];
//      assert(x == 0 || (x & (x-1)) == 0); // x is a power of 2
//    }

    // clear the column from positions [j+1 ... n-1]
    for (int row = j+1; row < (int)a.size(); row++) {
      if (a[row][i] != 0) {
        const int p2 = bv_rank(a[row][i], dim);
        const bv_const v = a[j][i] >> p2;
        assert(p2 >= p1);
        bv_const mult =  a[row][i] >> p1;
        for (int idx = 0; idx < n; idx++) {
          bv_const tmp1;
          MUL(a[row][idx], u, mod, tmp1);
          SUBMUL(tmp1, a[j][idx], mult, mod, tmp);
          a[row][idx] = tmp1;
        }
        assert(a[row][i] == 0);
      }
    }

    j++;
  }
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after convert_red_ech_form : matrix \n"; print_2d_matrix<bv_const>(a));
  vector<int> zero_rows;
  ASSERT(vars.size() == a.size());
  for (int row = (int)a.size() - 1; row >= 0; row--) {
    if (zeros(a[row])) {
      a.erase(a.begin() + row);
      vars.erase(vars.begin() + row);
    }
  }
  amap.clear();
  ASSERT(vars.size() == a.size());
  for (int i = 0; i < (int)a.size(); i++) {
    amap[vars.at(i)] = a.at(i);
  }
}
void howell(map<bv_solve_var_idx_t, vector<bv_const>> &amap, const bv_const& mod, unsigned int dim)
{
  autostop_timer func_timer(__func__);
  vector<bv_solve_var_idx_t> vars;
  vector<vector<bv_const>> a;
  for (auto const& amapping : amap) {
    vars.push_back(amapping.first);
    a.push_back(amapping.second);
  }

  if (a.size() == 0) return;
  const int n = a.size();
  bv_const tmp;
  for (int i = 0, j = 0; i < n; i++) {
    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": before iteration " << i << " matrix \n"; print_2d_matrix<bv_const>(a));
    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": before iteration " << i << " vars \n"; 
        for(auto const &v : vars) 
          cout << v << "  ";
        cout << endl;
          );
    // find the row with the element of the least rank at the `i`th position
    int pivot_rank = dim+1; // rank can never exceed dim; choose `dim+1` as MAX
    int pivot_row = -1;
    for (int row = j; row < (int)a.size(); row++) {
      if (a[row][i] != 0) {
        if (bv_rank(a[row][i], dim) < pivot_rank) {
          pivot_rank = bv_rank(a[row][i],dim);
          pivot_row = row;
        }
      }
    }
    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": pivot_row = " << pivot_row << ", pivot_rank = " << pivot_rank << endl);

    // ... and swap it at the j'th position
    if (pivot_row == -1)
      continue;
    swap(a[j], a[pivot_row]);
    swap(vars[j], vars[pivot_row]);

    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": a =\n"; print_2d_matrix<bv_const>(a));

    const int p = bv_rank(a[j][i], dim);
    const bv_const u = a[j][i] >> p;

    // ensure that a[j][i] is a power of 2
    if (u != 1) {
      bv_const mod_val = bv_exp2(dim - p);
      bv_const uinv = bv_mulinv(u, mod_val);
      for (bv_const &x: a[j]) {
        tmp = x * uinv;
        MOD(x, tmp, mod);
      }
    }

    {// can remove this assertion
      auto x = a[j][i];
      assert(x == 0 || (x & (x-1)) == 0); // x is a power of 2
    }

    // clear the column from positions [j+1 ... n-1]
    for (int row = j+1; row < (int)a.size(); row++) {
      if (a[row][i] != 0) {
        assert(bv_rank(a[row][i], dim) >= p);
        bv_const mult =  a[row][i] >> p;
        for (int idx = 0; idx < n; idx++) {
          SUBMUL(a[row][idx], a[j][idx], mult, mod, tmp);
        }
        assert(a[row][i] == 0);
      }
    }

    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": a =\n"; print_2d_matrix<bv_const>(a));

    // ensure that a[j][i] is the largest element in it's column
    for (int row = 0; row < j; row++) {
      bv_const d = a[row][i] >> p;
      for (int idx = 0; idx < n; idx++) {
        SUBMUL(a[row][idx], a[j][idx], d, mod, tmp);
      }
      assert(0 <= a[row][i] && a[row][i] < a[j][i]);
    }
    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": a =\n"; print_2d_matrix<bv_const>(a));

    if (a[j][i] != 1) {
      vector<bv_const> newrow(a[j]);
      bv_const mult = bv_exp2(dim - p);
      for (bv_const &x: newrow) {
        tmp = x * mult;
        MOD(x, tmp, mod);
      }
      a.push_back(newrow);
      vars.push_back(vars.at(j));
    }
    CPP_DBG_EXEC3(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": a =\n"; print_2d_matrix<bv_const>(a));
    j++;
  }
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after howell : matrix \n"; print_2d_matrix<bv_const>(a));
  vector<int> zero_rows;
  ASSERT(vars.size() == a.size());
  for (int row = (int)a.size() - 1; row >= 0; row--) {
    if (zeros(a[row])) {
      a.erase(a.begin() + row);
      vars.erase(vars.begin() + row);
    }
  }
  amap.clear();
  ASSERT(vars.size() == a.size());
  for (int i = 0; i < (int)a.size(); i++) {
    amap[vars.at(i)] = a.at(i);
  }
}

//static bool
//is_only_entry_in_column(const vector<vector<bv_const>>& soln, size_t col, size_t row, unsigned rank, unsigned dim)
//{
//  for (size_t r = 0; r < soln.size(); r++) {
//    if (r == row) {
//      continue;
//    }
//    if (soln.at(r).at(col) == 0) {
//      continue;
//    }
//    if (bv_rank(soln.at(r).at(col), dim) >= rank) {
//      cout << __func__ << " " << __LINE__ << ": returning false due to row " << r << ", elem " << soln.at(r).at(col) << endl;
//      return false;
//    }
//  }
//  return true;
//}

// points is a matrix with #row >= #columns
// each row represents a possible state of vars (columns)
static void bv_linear_solve(const vector<vector<bv_const>>& points, const unsigned dim, map<bv_solve_var_idx_t, vector<bv_const>>& soln_map)
{
  autostop_timer func_timer(__func__);
  ASSERT(dim > 0);

  auto ipoints = points;
  unsigned num_pts = ipoints.size();
  unsigned num_vars = ipoints.at(0).size();

  if(num_pts > num_vars) {
    // solver cannot handle more points than vars
    // add zero columns to compensate
    make_square_matrix_by_adding_zero_columns(ipoints);
  }
  ASSERT(ipoints.size() <= ipoints[0].size());

  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": ipoints:\n"; print_2d_matrix<bv_const>(ipoints));
  unsigned n = ipoints[0].size();
  vector<vector<bv_const>> id(n, vector<bv_const>(n, 0));
  for (int row = 0; row < n; row++) {
    id[row][row] = 1;
  }
  vector<int> var_ids;
  var_ids.push_back(-1);
  for (int row = 0; row < n; row++) {
    var_ids.push_back(row);
  }
  const bv_const mod = bv_exp2(dim);
  ldr_decomposition(ipoints, id, var_ids, mod, dim);
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": ipoints:\n"; print_2d_matrix<bv_const>(ipoints));
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": id:\n"; print_2d_matrix<bv_const>(id));
  //ASSERT(is_diagonal(points));
  auto gens = generate_solutions(ipoints, mod, dim);
  // mul_verify(points, soln);
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after generate_solutions, gens:\n"; print_2d_matrix<bv_const>(gens));
  auto result = bv_mat_mul(id, gens, mod);
  // mul_verify(b, result);
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after matmul, result:\n"; print_2d_matrix<bv_const>(result));
  vector<vector<bv_const>> soln = transpose(result);
  //ASSERT(zeros(result.at(0))); //the first row (representing pivot on constant term) must be all zero
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after transpose, soln:\n"; print_2d_matrix<bv_const>(soln));
  normalize_constant_term(soln, mod, dim);
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after normalize constant terms, soln:\n"; print_2d_matrix<bv_const>(soln));
  //ASSERT(zeros(soln.at(0))); //the first row (representing pivot on constant term) must be all zero
  for (size_t si = 0; si < soln.size(); si++) {
    soln_map.insert(make_pair(var_ids.at(si), soln.at(si)));
  }
  //for (size_t si = 0; si < soln.size(); si++) {
  //  int only_entry_in_column = -1;
  //  bool found_nonzero = false;
  //  for (size_t i = 0; i < n; i++) {
  //    if (soln.at(si).at(i) == 0) {
  //      continue;
  //      //cout << __func__ << " " << __LINE__ << ": si = " << si << ", i = " << i << endl;
  //    }
  //    unsigned rank = bv_rank(soln.at(si).at(i), dim);
  //    if (soln.at(si).at(i) != bv_exp2(rank)) {
  //      continue;
  //    }
  //    found_nonzero = true;
  //    if (is_only_entry_in_column(soln, i, si, rank, dim)) {
  //      only_entry_in_column = i;
  //      break;
  //    }
  //    //ASSERT(soln.at(i).at(si) == 0);
  //  }
  //  if (found_nonzero && only_entry_in_column == -1) {
  //    cout << __func__ << " " << __LINE__ << ": failed for row " << si << endl;
  //  }
  //  ASSERT(!found_nonzero || only_entry_in_column != -1); //there must be at least one element where it is the only non-zero entry in its column
  //  soln_map.insert(make_pair(si - 1, soln.at(si)));
  //}
  //cout << __func__ << " " << __LINE__ << ": after normalize constant term, soln:\n"; print_2d_matrix<bv_const>(soln);
  //howell(soln_map, mod, dim);
  //convert_red_ech_form(soln_map, mod, dim);
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": after convert_red_ech_form, soln:\n";
    for (auto const &s : soln_map) {
      for (const auto &e : s.second) {
        cout << " " << e;
      }
      cout << endl;
    }
  );
  if (soln_map.empty()) {
    return;
  }
  if (num_pts > num_vars) {
    clip_extra_vars(soln_map, num_pts-num_vars);
  }
  //cout << __func__ << " " << __LINE__ << ": after clip_extra_vars, soln:\n"; print_2d_matrix<bv_const>(soln);
  ASSERT(soln_map.begin()->second.size() == num_vars);
  eliminate_zero_rows(soln_map);
  //cout << __func__ << " " << __LINE__ << ": after eliminate zero rows, soln:\n"; print_2d_matrix<bv_const>(soln);
  //ASSERT(is_howell(soln));
}

void
get_nullspace_basis(const vector<vector<bv_const>>& points, const unsigned dim, map<bv_solve_var_idx_t, vector<bv_const>>& soln)
{
  long long elapsed = stats::get().get_timer("query:bv_solve")->get_elapsed();
  autostop_timer query_bv_solve_timer("query:bv_solve");
  stats::get().inc_counter("# of queries to linear solver");

  bv_linear_solve(points, dim, soln);
  elapsed = stats::get().get_timer("query:bv_solve")->get_elapsed_so_far() - elapsed;

  CPP_DBG_EXEC2(BV_SOLVE_QUERY,
      string filename = g_query_dir + "/" + query_comment("bvsolve").to_string();
      cout << __func__ << " " << __LINE__ << ": filename = " << filename << endl;
      auto fout = ofstream(filename);
      ASSERT(fout);
      fout << "=input:\n";
      fout << points.size() << ' ' << points[0].size() << ' ' << dim << '\n';
      for (size_t i = 0; i < points.size(); ++i) { fout << bv_const_vector2str(points[i], " ") << '\n'; }
      fout << "=output:\n";
      ASSERT(soln.size());
      fout << soln.size() << ' ' << soln.begin()->second.size() << ' ' << '\n';
      for (auto const& p : soln) { fout << bv_const_vector2str(p.second, " ") << '\n'; }
      fout.close();
  );
  CPP_DBG_EXEC(BV_SOLVE_QUERY, cout << __func__ << ':' << __LINE__ << " elapsed = " << elapsed << " (" << elapsed/1.0e6 << " s)\n");
//  DYN_DEBUG(counters_enable, stats::get().add_counter(string("bv_solve-queries-time-result-OK"), elapsed));
}

}
