#pragma once

#include <gmpxx.h>
#include <set>

#include "support/types.h"
#include "support/dyn_debug.h"
#include "support/debug.h"

using namespace std;

namespace eqspace {

using bv_const = mpz_class;

inline bv_const bv_one()  { return 1_mpz; }
inline bv_const bv_zero() { return 0_mpz; }
bv_const bv_mod(bv_const const& a, bv_const const& b);
bv_const bv_mulinv(bv_const const& a, bv_const const& m);
bv_const bv_gcd(bv_const const& a, bv_const const& b);
bv_const bv_ceil(bv_const const& a, bv_const const& b);
bv_const bv_exp2(unsigned n);
bv_const bv_neg(bv_const const& a);
bv_const bv_dot_product_mod(vector<bv_const>::const_iterator aitr, vector<bv_const>::const_iterator aend, vector<bv_const>::const_iterator bit, vector<bv_const>::const_iterator bend, bv_const const& mod);
bv_const bv_avg_roundup(bv_const const& a, bv_const const& b);
bv_const bv_avg_rounddown(bv_const const& a, bv_const const& b);
bv_const bv_round_up_to_power_of_two(bv_const const& a);
bv_const bv_round_down_to_power_of_two(bv_const const& a);
//bv_const bv_avg_signed_roundup(bv_const const& a, bv_const const& b, size_t bvlen);
//bv_const bv_avg_signed_rounddown(bv_const const& a, bv_const const& b, size_t bvlen);
bv_const bv_convert_to_signed(bv_const const& a, size_t bvlen);
inline int bv_cmp(bv_const const& a, bv_const const& b) { return mpz_cmp(a.get_mpz_t(), b.get_mpz_t());  }
//int bv_cmp_signed(bv_const const& a, bv_const const& b, size_t bvlen);

bv_const bv_add(bv_const const& a, bv_const const& b);
bv_const bv_sub(bv_const const& a, bv_const const& b);

inline bv_const bv_inc(bv_const const& a) { return bv_add(a, bv_one()); }
inline bv_const bv_dec(bv_const const& a) { return bv_sub(a, bv_one()); }

inline bool bv_is_power_of_2(bv_const const& a, unsigned n) { return a == bv_exp2(n); }

string bv_const_to_string(bv_const const& bv);

string bv_const_vector2str(vector<bv_const> const& bv, string const& sep = ", ");
string bv_const_map2str(map<bv_solve_var_idx_t, bv_const> const& bv, string const& sep = ", ");

void parse_bv_const_list_list(string const& in, vector<vector<bv_const>>& out);
unsigned int bv_rank(bv_const const& bv_in, unsigned int dim);
bv_const bv_ashr(bv_const const& bv_in, unsigned int shift);
inline bool bv_isodd(bv_const const& in) { return mpz_odd_p(in.get_mpz_t()); }
vector<vector<bv_const>> bv_mat_mul(const vector<vector<bv_const>> &a, const vector<vector<bv_const>> &b, const bv_const& mod);
bool bv_const_belongs_to_range(bv_const const& needle, bv_const const& haystack_start, size_t haystack_size);

void bv_matrix_to_stream(ostream& out, vector<vector<bv_const>> const& matrix, unsigned m, unsigned n, unsigned d);
vector<vector<bv_const>> bv_matrix_from_stream(istream& in, unsigned& m, unsigned& n, unsigned& d);

void bv_matrix_map_to_stream(ostream& ss, map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> const& matrix);
map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> bv_matrix_map_from_stream(istream& is);

void bv_matrix_map2_to_stream(ostream& ss, vector<map<bv_solve_var_idx_t, bv_const>> const& matrix, unsigned d);
vector<map<bv_solve_var_idx_t, bv_const>> bv_matrix_map2_from_stream(istream& ss, unsigned& d);

void bv_matrix_map3_to_stream(ostream& ss, map<bv_solve_var_idx_t, vector<bv_const>> const& matrix);
map<bv_solve_var_idx_t, vector<bv_const>> bv_matrix_map3_from_stream(istream& ss);

map<bv_solve_var_idx_t, bv_const> bv_const_row_from_string(string const& line);

bv_const
get_least_bv_const_elem(std::set<bv_const> const& input_pts, std::function<bool (bv_const const&, bv_const const&)> compare_f, std::function<bv_const ()> init_val, bool is_signed, size_t bvlen);
bv_const get_smallest_value_signed(std::set<bv_const> const& input_pts, size_t bvlen, bv_const const& threshold);
bv_const get_largest_value_signed(std::set<bv_const> const& input_pts, size_t bvlen, bv_const const& threshold);
bv_const get_smallest_value_unsigned(std::set<bv_const> const& input_pts, size_t bvlen, bv_const const& threshold);
bv_const get_largest_value_unsigned(std::set<bv_const> const& input_pts, size_t bvlen, bv_const const& threshold);

inline static void ADDMUL(bv_const& out, bv_const const& a, bv_const const& b, bv_const const& mod, bv_const& tmp)
{
  tmp = out;
  mpz_addmul(tmp.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  mpz_mod(out.get_mpz_t(), tmp.get_mpz_t(), mod.get_mpz_t());
}
inline static void SUB(bv_const& out, bv_const const& a, bv_const const& b, bv_const const& mod, bv_const& tmp)
{
  tmp = out;
  mpz_sub(tmp.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  mpz_mod(out.get_mpz_t(), tmp.get_mpz_t(), mod.get_mpz_t());
}


inline static void SUBMUL(bv_const& out, bv_const const& a, bv_const const& b, bv_const const& mod, bv_const& tmp)
{
  tmp = out;
  mpz_submul(tmp.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  mpz_mod(out.get_mpz_t(), tmp.get_mpz_t(), mod.get_mpz_t());
}
inline static bv_const MUL(bv_const const& a, bv_const const& b, bv_const const& mod)
{
  bv_const out;
  mpz_mul(out.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  mpz_mod(out.get_mpz_t(), out.get_mpz_t(), mod.get_mpz_t());
  return out;
}
inline static bv_const DIV(bv_const const& dividend, bv_const const& divisor, bv_const const& mod, int dim/*, bv_const& quotient*/)
{
  const unsigned dividend_rank = bv_rank(dividend, dim);
  const unsigned divisor_rank = bv_rank(divisor, dim);

  bv_const dividend_odd = dividend >> dividend_rank;
  bv_const dividend_mod = bv_exp2(dim - dividend_rank);

  bv_const divisor_odd = divisor >> divisor_rank;
  bv_const divisor_mod = bv_exp2(dim - divisor_rank);
  bv_const divisor_inv = bv_mulinv(divisor_odd, divisor_mod);

  int shift;
  if (dividend_rank >= divisor_rank) {
    shift = dividend_rank - divisor_rank;
  } else {
    shift = (dim - divisor_rank) + dividend_rank;
  }
  ASSERT(shift >= 0);
  return bv_mod(bv_mod(dividend_odd * divisor_inv, mod) << shift, mod);
}


map<bv_solve_var_idx_t, bv_const> read_point(string const& line);

}
