#pragma once

#include <gmpxx.h>

#include "support/utils.h"

using namespace std;

namespace eqspace {

class mybitset
{
  friend std::ostream& operator<<(std::ostream &s, const mybitset &n);
public:
  /* constructors */
  mybitset();
  mybitset(const char* c, size_t bvsize);
  mybitset(const std::string& s, size_t bvsize);
  mybitset(bool b, size_t bvsize);
  mybitset(int l, size_t bvsize);
  mybitset(long l, size_t bvsize);
  mybitset(long long l, size_t bvsize);
  mybitset(unsigned int l, size_t bvsize);
  mybitset(unsigned long l, size_t bvsize);
  mybitset(unsigned long long l, size_t bvsize);
  mybitset(mpz_class const& l, size_t bvsize);
  mybitset(const mybitset & l);
  mybitset(vector<bool> const& v);

  static mybitset mybitset_from_stream(istream& is);
  static mybitset mybitset_from_string(string const& str);
  string mybitset_to_string() const;

  static mybitset mybitset_from_binary_string(string const& str);
  string mybitset_to_binary_string() const;

  static mybitset mybitset_zero(size_t numbits);
  static mybitset mybitset_all_ones(size_t numbits);
  static mybitset mybitset_int_min(size_t numbits);

  const mybitset& operator=(const mybitset & l);

  // for map
  bool operator<(mybitset const& other) const { return this->bvult(other); }

  /* unary increment/decrement operators */
  mybitset inc() const;
  mybitset dec() const;

  /* arithmetic operations */
  mybitset operator-() const;
  mybitset operator+(const mybitset& rhs) const;
  mybitset operator+(const int rhs) const;
  mybitset operator-(const mybitset& rhs) const;
  mybitset operator-(const int rhs) const;
  mybitset operator*(const mybitset& rhs) const;

  /* relational operations */
  bool operator==(const mybitset& rhs) const;
  bool operator!=(const mybitset& rhs) const;
  bool is_zero() const;
  bool is_max_val() const;
  bool is_one() const;
  bool is_neg() const;

  /* logical operations */
  mybitset operator!() const;
  mybitset operator&&(mybitset const &rhs) const;
  mybitset operator||(mybitset const &rhs) const;
  mybitset operator|(mybitset const &rhs) const;
  mybitset operator&(mybitset const &rhs) const;
  mybitset operator^(mybitset const &rhs) const;
  mybitset operator~() const;
  bool get_bool() const;

  mybitset bvsign() const;

  mybitset bvsign_ext(int b) const;
  mybitset bvzero_ext(int b) const;

  static mybitset mybitset_bvconcat(vector<mybitset> const &a);
  static mybitset mybitset_umin(mybitset const& a, mybitset const& b) { return a.bvule(b) ? a : b; }
  static mybitset mybitset_umax(mybitset const& a, mybitset const& b) { return a.bvuge(b) ? a : b; }

  /* shift ops */
  mybitset operator<<(unsigned shiftcount) const;
  mybitset bvshl(int b) const;
  mybitset bvlshr(int b) const;
  mybitset bvashr(int b) const;
  mybitset bv_left_rotate(int b) const;
  mybitset bv_right_rotate(int b) const;
  mybitset bvextract(int ub, int lb) const;
  mybitset bvnand(mybitset const &b) const;
  mybitset bvnor(mybitset const &b) const;
  mybitset bvxnor(mybitset const &b) const;

  //shifts out right at most SHIFTCOUNT zeros; returns the number of zeros shifted out
  int mybitset_shift_arith_right_zeros(int shiftcount);

  /* comparison ops */
  bool bvult(mybitset const &b) const;
  bool bvule(mybitset const &b) const;
  bool bvule(int const rhs) const;
  bool bvslt(mybitset const &b) const;
  bool bvsle(mybitset const &b) const;
  bool bvugt(mybitset const &b) const;
  bool bvuge(mybitset const &b) const;
  bool bvsgt(mybitset const &b) const;
  bool bvsge(mybitset const &b) const;
  mybitset bvudiv(mybitset const &b) const;
  mybitset bvsdiv(mybitset const &b) const;
  mybitset bvurem(mybitset const &b) const;
  mybitset bvsrem(mybitset const &b) const;

  /* integer square root */
  //mybitset intSqrt() const; // throw

  /* bit operations */
  bool bitAt(size_t i) const { return m_bits.at(i);  }
  size_t get_numbits() const { return m_bits.size(); }

  /* string conversion */
  std::string toString(int base = 10) const;
  std::string toStringAsSignedNumber() const;
  std::string toStringAsFloat() const;

  static bool mybitset_size_represents_fplen(size_t sz);

  static mybitset get_ieee_bv_for_float(float_max_t f, size_t numbits);
  float_max_t get_float_from_ieee_bv() const;

  static mybitset get_ieee_floatx_bv_for_float(float_max_t f, size_t numbits);
  float_max_t get_float_from_ieee_floatx_bv() const;

  static std::pair<int, int> floating_point_get_ebits_and_sbits_from_size(size_t sz);
  tuple<bool,int64_t,__uint128_t> mybitset_get_floating_point_breakup(int ebits, int sbits) const;

  // returns the mask len (number of zero bits) if found to be an alignment mask
  optional<unsigned> mybitset_represents_alignment_mask() const;

  /* conversion to primitive types */
  int toInt() const; // throw
  int64_t toInt64() const; // throw
  long toLong() const; // throw
  long long toLongLong() const; // throw
  __int128_t toInt128() const;
  __uint128_t toUint128() const;
  unsigned int toUnsignedInt() const; // throw
  unsigned long toUnsignedLong() const; // throw
  uint64_t toUint64() const; // throw
  unsigned long long toUnsignedLongLong() const; // throw
  __uint128_t toUInt128() const;

  static mybitset mybitset_rand(size_t numbits);

  //void read_from_bigint(mpz_class const &bigint);
  mpz_class get_signed_mpz() const;
  mpz_class get_unsigned_mpz() const { return m_mpz; }

private:
  mybitset mybitset_add_floatx80_integer_bit() const;
  bool compute_floatx80_bit() const;

  mpz_class bits_to_unsigned_mpz() const;
  mpz_class bits_to_signed_mpz() const;

  void sync_bits_with_mpz();
  void incbits();
  void decbits();

  vector<bool>   m_bits; /* 2's complement repr for negatives; v[0] is the least significant bit */
  mpz_class      m_mpz; /* dual of m_bits: always equals unsigned value of m_bits */
};

}

namespace std
{
template<>
struct hash<eqspace::mybitset>
{
  std::size_t operator()(eqspace::mybitset const &s) const
  {
    std::size_t seed = 0;
    myhash_combine(seed, s.get_numbits());
    myhash_combine(seed, s.toString());
    return seed;
  }
};
}

