#include <sstream>
#include <iostream>

#include "support/bv_const.h"
#include <gmpxx.h>
#include "support/debug.h"

namespace eqspace {

bv_const bv_mod(bv_const const& a, bv_const const& b) {
  bv_const t;
  mpz_mod(t.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  return t;
}

bv_const bv_mulinv(bv_const const& a, bv_const const&m)
{
  bv_const t;
  ASSERT(mpz_invert(t.get_mpz_t(), a.get_mpz_t(), m.get_mpz_t()));
  return t;
}

bv_const bv_gcd(bv_const const& a, bv_const const& b)
{
  bv_const t;
  mpz_gcd(t.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  return t;
}

bv_const bv_ceil(bv_const const& a, bv_const const& b)
{
  return (a+b-1) / b;
}

bv_const bv_exp2(unsigned n)
{
  bv_const t;
  bv_const m = 1_mpz;
  mpz_mul_2exp(t.get_mpz_t(), m.get_mpz_t(), n);
  //std::cout << __func__ << " " << __LINE__ << ": n = " << n << ", t = " << t << endl;
  return t;
}

bv_const bv_neg(bv_const const& a)
{
  bv_const ret;
  mpz_neg(ret.get_mpz_t(), a.get_mpz_t());
  return ret;
}

bv_const bv_dot_product_mod(vector<bv_const>::const_iterator aitr, vector<bv_const>::const_iterator aend, vector<bv_const>::const_iterator bitr, vector<bv_const>::const_iterator bend, bv_const const& mod)
{
  bv_const dot = 0_mpz;
  while (aitr != aend && bitr != bend) {
    dot = bv_mod( dot + (*aitr)*(*bitr), mod);
    ++aitr; ++bitr;
  }
  return dot;
}

string bv_const_vector2str(vector<bv_const> const& bv, string const& sep)
{
  stringstream ss;
  for (auto const& p : bv)
    ss << p << sep;
  return ss.str();
}

void
parse_bv_const_list_list(string const &in, vector<vector<bv_const>>& out) {
  stringstream ss(in);
  string s;
  while (ss >> s && s == "[") {
    vector<bv_const> t;
    while (ss >> s && s != "]") {
      t.push_back(bv_const(s));
    }
    ASSERT(t.size());
    out.push_back(t);
  }
}

unsigned int bv_rank(const bv_const& bv_in, unsigned int dim)
{
  unsigned int rank;
  rank = (bv_in == 0) ? dim : mpz_scan1(bv_in.get_mpz_t(), 0);
  ASSERT(dim >= rank);
  return rank;
}

bv_const bv_ashr(const bv_const& bv_in, unsigned int shift)
{
	bv_const res;
  mpz_fdiv_q_2exp(res.get_mpz_t(), bv_in.get_mpz_t(), shift);
  return res;
}

vector<vector<bv_const>>
bv_mat_mul(const vector<vector<bv_const>> &a, const vector<vector<bv_const>> &b, const bv_const& mod) {
  ASSERT(a.size() == a[0].size());
  ASSERT(b.size() == b[0].size());
  ASSERT(a.size() == b.size());

  size_t const n = a.size();
  vector<vector<bv_const>> p(n, vector<bv_const>(n, 0));
  bv_const pv;
  size_t const B = 32;
  for (int rr = 0; rr < n; rr += B) {
    for (int cc = 0; cc < n; cc += B) {
      for (int ss = 0; ss < n; ss += B) {
        // block multiplication
        for (int row = rr; row < min(n, rr+B); row++) {
          for (int col = cc; col < min(n, cc+B); col++) {
            pv = p[row][col];
            for (int step = ss; step < min(n, ss+B); step++) {
              mpz_addmul(pv.get_mpz_t(), a[row][step].get_mpz_t(), b[step][col].get_mpz_t());
            }
            mpz_mod(p[row][col].get_mpz_t(), pv.get_mpz_t(), mod.get_mpz_t()); // mod is more expensive than addmul
          }
        }
      }
    }
  }
  return p;
}

bv_const
bv_convert_to_signed(bv_const const& a, size_t bvlen)
{
  ASSERT(bvlen > 0);
  if (bv_cmp(a, bv_exp2(bvlen - 1)) >= 0) {
    bv_const ret = bv_add(a, bv_neg(bv_exp2(bvlen)));
    //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for " << a << ", len " << bvlen << endl;
    return ret;
  } else {
    //cout << __func__ << " " << __LINE__ << ": returning " << a << " for " << a << ", len " << bvlen << endl;
    return a;
  }
}

bv_const
bv_avg_roundup(bv_const const& a, bv_const const& b)
{
  bv_const sum = bv_add(a, b);
  bv_const ret;
  mpz_cdiv_q_ui(ret.get_mpz_t(), sum.get_mpz_t(), 2);
  return ret;
}

bv_const
bv_avg_rounddown(bv_const const& a, bv_const const& b)
{
  bv_const sum = bv_add(a, b);
  bv_const ret;
  mpz_fdiv_q_ui(ret.get_mpz_t(), sum.get_mpz_t(), 2);
  return ret;
}

//bv_const
//bv_avg_signed_roundup(bv_const const& a, bv_const const& b, size_t bvlen)
//{
//  return bv_avg_roundup(bv_convert_to_signed(a, bvlen), bv_convert_to_signed(b, bvlen));
//}
//
//bv_const
//bv_avg_signed_rounddown(bv_const const& a, bv_const const& b, size_t bvlen)
//{
//  return bv_avg_rounddown(bv_convert_to_signed(a, bvlen), bv_convert_to_signed(b, bvlen));
//}

//int
//bv_cmp_signed(bv_const const& a, bv_const const& b, size_t bvlen)
//{
//  return bv_cmp(bv_convert_to_signed(a, bvlen), bv_convert_to_signed(b, bvlen));
//}

bv_const
bv_add(bv_const const& a, bv_const const& b)
{
  bv_const ret;
  mpz_add(ret.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  return ret;
}

bv_const
bv_sub(bv_const const& a, bv_const const& b)
{
  bv_const ret;
  mpz_sub(ret.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  return ret;
}

bv_const
bv_round_up_to_power_of_two(bv_const const& a)
{
  if (a == 0)
    return a;

  size_t bitlen = mpz_sizeinbase(a.get_mpz_t(), 2);
  if (a > 0) {
    if (a == bv_exp2(bitlen-1))
      return a;
    else
      return bv_exp2(bitlen);
  }
  else {
    return -bv_exp2(bitlen-1);
  }
}

bv_const
bv_round_down_to_power_of_two(bv_const const& a)
{
  if (a == 0)
    return a;

  size_t bitlen = mpz_sizeinbase(a.get_mpz_t(), 2);
  if (a > 0) {
    return bv_exp2(bitlen-1);
  }
  else {
    if (a == -bv_exp2(bitlen-1))
      return a;
    else
      return -bv_exp2(bitlen);
  }
}

bool
bv_const_belongs_to_range(bv_const const& needle, bv_const const& haystack_start, size_t haystack_size)
{
  return (needle >= haystack_start && needle < (haystack_start + haystack_size));
}


}
