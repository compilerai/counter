#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
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

  const mybitset& operator=(const mybitset & l);

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
  mybitset intSqrt() const; // throw

  /* bit operations */
  bool bitAt(size_t i) const; // throw
  size_t get_numbits() const;

  /* size in bytes */
  size_t size() const;

  /* string conversion */
  std::string toString() const;
  std::string toStringAsSignedNumber() const;

  /* conversion to primitive types */
  int toInt() const; // throw
  long toLong() const; // throw
  long long toLongLong() const; // throw
  unsigned int toUnsignedInt() const; // throw
  unsigned long toUnsignedLong() const; // throw
  unsigned long long toUnsignedLongLong() const; // throw

  static mybitset mybitset_rand(size_t numbits);

  //void read_from_bigint(mpz_class const &bigint);
  mpz_class get_signed_mpz() const;
  mpz_class get_unsigned_mpz() const;

private:

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
    myhash_combine(seed, s.toString());
    return seed;
  }
};
}

