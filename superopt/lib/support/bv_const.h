#pragma once

#include <vector>
#include <string>
#include <gmpxx.h>

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

string bv_const_vector2str(vector<bv_const> const& bv, string const& sep = ", ");

void parse_bv_const_list_list(string const& in, vector<vector<bv_const>>& out);
unsigned int bv_rank(bv_const const& bv_in, unsigned int dim);
bv_const bv_ashr(bv_const const& bv_in, unsigned int shift);
inline bool bv_isodd(bv_const const& in) { return mpz_odd_p(in.get_mpz_t()); }
vector<vector<bv_const>> bv_mat_mul(const vector<vector<bv_const>> &a, const vector<vector<bv_const>> &b, const bv_const& mod);
bool bv_const_belongs_to_range(bv_const const& needle, bv_const const& haystack_start, size_t haystack_size);

}
