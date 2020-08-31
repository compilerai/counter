#include <vector>
#include <algorithm>

#include "support/mybitset.h"
#include "support/debug.h"

#define BYTE_LEN 8

namespace eqspace {

/*** helpers ***/

mpz_class ll2mpz(long long ll)
{
  mpz_t z;
  //mpz_import (mpz_t rop, size_t count, int order, size_t size, int endian, size_t nails, const void *op)
  mpz_import(z, 1, /*lsw*/-1, sizeof ll, 0, 0, &ll);
  return mpz_class(z);
}

static mpz_class eb32("4294967296");

mpz_class mpz_exp2(unsigned n)
{
  if (n == 32) {
    return eb32;
  } else {
    mpz_class ret(0);
    mpz_setbit(ret.get_mpz_t(), n);
    return ret;
  }
}

mpz_class mpz_mod_exp2(mpz_class const& a, unsigned n)
{
  mpz_class ret;
  mpz_class b = mpz_exp2(n);
  mpz_mod(ret.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  return ret;
}

mpz_class mpz_mod(mpz_class const& a, mpz_class const& b)
{
  mpz_class ret;
  mpz_mod(ret.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  return ret;
}

mpz_class mpz_fdiv_exp2(mpz_class const& d, unsigned n)
{
  mpz_class ret;
  mpz_fdiv_q_2exp(ret.get_mpz_t(), d.get_mpz_t(), n);
  return ret;
}

bool mpz_isodd(mpz_class const& c)
{
  return mpz_tstbit(c.get_mpz_t(),0);
}

/*** mybitset begins ***/

mybitset::mybitset()
{ }

mybitset::mybitset(const char* c, size_t bvsize)
  : mybitset(string(c), bvsize)
{ }

mybitset::mybitset(const std::string& s, size_t bvsize)
	: m_bits(bvsize, false)
{
  ASSERT(bvsize);
  m_mpz = mpz_mod_exp2(mpz_class(s),bvsize);
  this->sync_bits_with_mpz();
}

mybitset::mybitset(bool b, size_t bvsize)
  : m_bits({ b }),
    m_mpz((int)b)
{
  ASSERT(bvsize == 1);
}

mybitset::mybitset(int l, size_t bvsize)
  : m_mpz(mpz_mod_exp2(l,bvsize))
{
  ASSERT(bvsize);
  size_t significant_bits = min(bvsize, sizeof(int)*BYTE_LEN);
  m_bits.resize(bvsize, false);
  for (size_t i = 0; i < significant_bits; i++) {
    if (l & (1 << i)) {
      m_bits[i] = true;
    }
  }
}

mybitset::mybitset(long l, size_t bvsize)
  : m_mpz(mpz_mod_exp2(l,bvsize))
{
  ASSERT(bvsize);
  ASSERT(bvsize <= sizeof(long) * BYTE_LEN);
  m_bits.resize(bvsize, false);
  for (size_t i = 0; i < bvsize; i++) {
    if (l & (1L << i)) {
      m_bits[i] = true;
    }
  }
}

mybitset::mybitset(long long l, size_t bvsize)
  : m_mpz(mpz_mod_exp2(ll2mpz(l),bvsize))
{
  ASSERT(bvsize);
  ASSERT(bvsize <= sizeof(long long) * BYTE_LEN);
  m_bits.resize(bvsize, false);
  for (size_t i = 0; i < bvsize; i++) {
    if (l & (1LL << i)) {
      m_bits[i] = true;
    }
  }
}

mybitset::mybitset(unsigned int l, size_t bvsize)
  : mybitset((int)l, bvsize)
{ }

mybitset::mybitset(unsigned long l, size_t bvsize)
  : mybitset((long)l, bvsize)
{ }

mybitset::mybitset(unsigned long long l, size_t bvsize)
  : mybitset((long long)l, bvsize)
{ }

mybitset::mybitset(mpz_class const& l, size_t bvsize)
  : m_bits(bvsize, false),
    m_mpz(mpz_mod_exp2(l,bvsize))
{
  ASSERT(bvsize);
  this->sync_bits_with_mpz();
}

mybitset::mybitset(mybitset const &other)
  : m_bits(other.m_bits),
    m_mpz(other.m_mpz)
{ }

const mybitset& mybitset::operator=(const mybitset& other)
{
  m_bits = other.m_bits;
  m_mpz = other.m_mpz;
  return *this;
}

void mybitset::incbits()
{
  size_t i = 0;
  while (i < m_bits.size() && m_bits[i]) {
    m_bits[i] = false;
    ++i;
  }
  if (i < m_bits.size()) {
    m_bits[i] = true;
  }
}

mybitset mybitset::inc() const
{
  mybitset ret(*this);
  ret.incbits();
  ret.m_mpz = (m_mpz+1);
  mpz_class mod = mpz_exp2(m_bits.size());
  if (ret.m_mpz >= mod)
    ret.m_mpz -= mod;
  return ret;
}

void mybitset::decbits()
{
  size_t i = 0;
  while (i < m_bits.size() && !m_bits[i]) {
    m_bits[i] = true;
    ++i;
  }
  if (i < m_bits.size()) {
    m_bits[i] = false;
  }
}

mybitset mybitset::dec() const
{
  mybitset ret(*this);
  ret.decbits();
  ret.m_mpz = (m_mpz == 0) ? mpz_exp2(m_bits.size())-1 : ret.m_mpz - 1;
  return ret;
}

mybitset mybitset::operator-() const
{
  return mybitset(~*this).inc();
}

mybitset mybitset::operator+(const mybitset& rhs) const
{
  mybitset ret(*this);
  ret.m_mpz = mpz_mod_exp2(ret.m_mpz + rhs.m_mpz, ret.m_bits.size());
  ret.sync_bits_with_mpz();
  return ret;
}

mybitset mybitset::operator+(const int rhs) const
{
  mybitset ret(*this);
  ret.m_mpz = mpz_mod_exp2(ret.m_mpz + rhs, ret.m_bits.size());
  ret.sync_bits_with_mpz();
  return ret;
}

mybitset mybitset::operator-(const mybitset& rhs) const
{
  mybitset ret(*this);
  ret.m_mpz = mpz_mod_exp2(ret.m_mpz - rhs.m_mpz, ret.m_bits.size());
  ret.sync_bits_with_mpz();
  return ret;
}

mybitset mybitset::operator-(const int rhs) const
{
  mybitset ret(*this);
  ret.m_mpz = mpz_mod_exp2(ret.m_mpz - rhs, ret.m_bits.size());
  ret.sync_bits_with_mpz();
  return ret;
}

mybitset mybitset::operator*(const mybitset& rhs) const
{
  mybitset ret(*this);
  ret.m_mpz = mpz_mod_exp2(ret.m_mpz * rhs.m_mpz, ret.m_bits.size());
  ret.sync_bits_with_mpz();
  return ret;
}

mybitset mybitset::bvudiv(mybitset const &rhs) const
{
  if (rhs.m_mpz == 0) {
    // See Section 2.3.2 of Yices manual (http://yices.csl.sri.com/papers/manual.pdf)
    return ~mybitset(0, m_bits.size());
  }
  mybitset ret(*this);
  ret.m_mpz = (ret.m_mpz / rhs.m_mpz); // mpz_mod_exp2(, ret.m_bits.size());
  ret.sync_bits_with_mpz();
  return ret;
}

mybitset mybitset::bvsdiv(mybitset const &rhs) const
{
  if (rhs.m_mpz == 0) {
    // See Section 2.3.2 of Yices manual (http://yices.csl.sri.com/papers/manual.pdf)
    return this->is_neg() ? mybitset(1, m_bits.size()) : ~mybitset(0, m_bits.size());
  }
  mybitset ret(*this);
  ret.m_mpz = mpz_mod_exp2(ret.get_signed_mpz() / rhs.get_signed_mpz(), ret.m_bits.size());
  ret.sync_bits_with_mpz();
  return ret;
}

mybitset mybitset::bvurem(mybitset const &rhs) const
{
  if (rhs.m_mpz == 0) {
    // See Section 2.3.2 of Yices manual (http://yices.csl.sri.com/papers/manual.pdf)
    return *this;
  }
  mybitset ret(*this);
  ret.m_mpz = mpz_mod(ret.m_mpz, rhs.m_mpz);
  ret.sync_bits_with_mpz();
  return ret;
}

mybitset mybitset::bvsrem(mybitset const &rhs) const
{
  if (rhs.m_mpz == 0) {
    // See Section 2.3.2 of Yices manual (http://yices.csl.sri.com/papers/manual.pdf)
    return *this;
  }
  mybitset ret(*this);
  ret.m_mpz = mpz_mod_exp2(ret.get_signed_mpz() % rhs.get_signed_mpz(), ret.m_bits.size());
  ret.sync_bits_with_mpz();
  return ret;
}

mybitset mybitset::bvlshr(int shiftcount) const
{
  mybitset ret(*this);
  for (size_t i = 0; i < ret.m_bits.size(); i++) {
    if (i + shiftcount < m_bits.size() && i + shiftcount >= 0) {
      ret.m_bits[i] = m_bits[i + shiftcount];
    } else {
      ret.m_bits[i] = false;
    }
  }
  ret.m_mpz = ret.m_mpz >> shiftcount;
  return ret;
}

mybitset mybitset::bvashr(int shiftcount) const
{
  mybitset ret(*this);
  for (size_t i = 0; i < ret.m_bits.size(); i++) {
    if (i + shiftcount < m_bits.size() && i + shiftcount >= 0) {
      ret.m_bits[i] = m_bits[i + shiftcount];
    } else {
      ret.m_bits[i] = m_bits[m_bits.size()-1];
    }
  }
  ret.m_mpz = mpz_mod_exp2(mpz_fdiv_exp2(ret.get_signed_mpz(), shiftcount), m_bits.size());
  return ret;
}

mybitset
mybitset::bv_left_rotate(int shiftcount) const
{
  mybitset ret(*this);
  for (size_t i = 0; i < ret.m_bits.size(); i++) {
    ret.m_bits[i] = m_bits[(i - shiftcount + m_bits.size()) % m_bits.size()];
  }
  ret.m_mpz = ret.bits_to_unsigned_mpz();
  return ret;
}

mybitset
mybitset::bv_right_rotate(int shiftcount) const
{
  mybitset ret(*this);
  for (size_t i = 0; i < ret.m_bits.size(); i++) {
    ret.m_bits[i] = m_bits[(i + shiftcount) % m_bits.size()];
  }
  ret.m_mpz = ret.bits_to_unsigned_mpz();
  return ret;

}

bool mybitset::operator==(const mybitset& rhs) const
{
  if (m_bits.size() != rhs.m_bits.size()) {
    return false;
  }
  return this->m_mpz == rhs.m_mpz;
}

bool mybitset::is_zero() const
{
  return this->m_mpz == 0;
}

bool mybitset::is_one() const
{
  return this->m_mpz == 1;
}

bool mybitset::is_neg() const
{
  return this->get_signed_mpz() < 0;
}

bool mybitset::operator!=(const mybitset& rhs) const
{
  ASSERT(m_bits.size() == rhs.m_bits.size());
  return this->m_mpz != rhs.m_mpz;
}

mybitset mybitset::operator!() const
{
  ASSERT(m_bits.size() == 1);
  return m_mpz == 0 ? mybitset(1,1) : mybitset(0,1);
}

mybitset mybitset::operator&&(mybitset const &rhs) const
{
  ASSERT(m_bits.size() == 1);
  ASSERT(m_bits.size() == rhs.m_bits.size());
  return this->m_mpz && rhs.m_mpz ? mybitset(1,1) : mybitset(0,1);
}

mybitset mybitset::operator||(mybitset const &rhs) const
{
  ASSERT(m_bits.size() == 1);
  ASSERT(m_bits.size() == rhs.m_bits.size());
  return this->m_mpz || rhs.m_mpz ? mybitset(1,1) : mybitset(0,1);
}

mybitset mybitset::operator&(mybitset const &rhs) const
{
  ASSERT(m_bits.size() == rhs.m_bits.size());
  mybitset ret(*this);
  for (size_t i = 0; i < m_bits.size(); i++) {
    ret.m_bits[i] = m_bits[i] && rhs.m_bits[i];
  }
  ret.m_mpz = ret.m_mpz & rhs.m_mpz;
  return ret;
}

mybitset mybitset::operator|(mybitset const &rhs) const
{
  ASSERT(m_bits.size() == rhs.m_bits.size());
  mybitset ret(*this);
  for (size_t i = 0; i < m_bits.size(); i++) {
    ret.m_bits[i] = m_bits[i] || rhs.m_bits[i];
  }
  ret.m_mpz = ret.m_mpz | rhs.m_mpz;
  return ret;
}

mybitset mybitset::operator^(mybitset const &rhs) const
{
  ASSERT(m_bits.size() == rhs.m_bits.size());
  mybitset ret(*this);
  for (size_t i = 0; i < m_bits.size(); i++) {
    ret.m_bits[i] = m_bits[i] ^ rhs.m_bits[i];
  }
  ret.m_mpz = ret.m_mpz ^ rhs.m_mpz;
  return ret;
}

mybitset mybitset::operator~() const
{
  mybitset ret(*this);
  ret.m_bits.flip();
  CPP_DBG_EXEC2(ASSERTCHECKS, assert(ret.m_mpz < mpz_exp2(ret.m_bits.size())));
  ret.m_mpz = mpz_exp2(m_bits.size()) - ret.m_mpz - 1;
  return ret;
}

bool mybitset::get_bool() const
{
  ASSERT(m_bits.size() == 1);
  return m_bits.front();
}

mybitset mybitset::operator<<(unsigned shiftcount) const
{
  mybitset ret(*this);
  for (size_t i = 0; i < ret.m_bits.size(); i++) {
    if (i - shiftcount >= 0 && i - shiftcount < m_bits.size()) {
      ret.m_bits[i] = m_bits[i - shiftcount];
    } else {
      ret.m_bits[i] = false;
    }
  }
  ret.m_mpz = mpz_mod_exp2(ret.m_mpz << shiftcount, m_bits.size());
  CPP_DBG_EXEC2(ASSERTCHECKS, assert(ret.m_mpz == ret.bits_to_unsigned_mpz()));
  return ret;
}

mybitset mybitset::bvshl(int b) const
{
  return *this << b;
}

bool mybitset::bvult(mybitset const &b) const
{
  return this->m_mpz < b.m_mpz;
}

bool mybitset::bvule(mybitset const &b) const
{
  return this->m_mpz <= b.m_mpz;
}

bool mybitset::bvslt(mybitset const &b) const
{
  return this->get_signed_mpz() < b.get_signed_mpz();
}

bool mybitset::bvsle(mybitset const &b) const
{
  return this->get_signed_mpz() <= b.get_signed_mpz();
}

bool mybitset::bvugt(mybitset const &b) const
{
  return this->m_mpz > b.m_mpz;
}

bool mybitset::bvuge(mybitset const &b) const
{
  return this->m_mpz >= b.m_mpz;
}

bool mybitset::bvsgt(mybitset const &b) const
{
  return this->get_signed_mpz() > b.get_signed_mpz();
}

bool mybitset::bvsge(mybitset const &b) const
{
  return this->get_signed_mpz() >= b.get_signed_mpz();
}

mybitset mybitset::bvsign_ext(int b) const
{
  mybitset ret(*this);
  mpz_class signed_mpz = ret.get_signed_mpz();
  ret.m_mpz = mpz_mod_exp2(signed_mpz, m_bits.size() + b);
  for (int i = 0; i < b; i++) {
    ret.m_bits.push_back(m_bits[m_bits.size()-1]);
  }
  return ret;
}

mybitset mybitset::bvzero_ext(int b) const
{
  mybitset ret(*this);
  for (int i = 0; i < b; i++) {
    ret.m_bits.push_back(false);
  }
  return ret;
}

mybitset mybitset::bvsign() const
{
  ASSERT(m_bits.size());
  return mybitset(m_bits[m_bits.size()-1], 1);
}

mybitset mybitset::bvextract(int ub, int lb) const
{
  mybitset ret;
  ASSERT(lb >= 0);
  ASSERT(ub < (int)m_bits.size());
  for (int i = lb; i <= ub; i++) {
    ret.m_bits.push_back(m_bits[i]);
  }
  ret.m_mpz = ret.bits_to_unsigned_mpz();
  return ret;
}

mybitset mybitset::bvnand(mybitset const &b) const
{
  ASSERT(m_bits.size() == b.m_bits.size());
  mybitset ret(*this);
  for (size_t i = 0; i < m_bits.size(); i++) {
    ret.m_bits[i] = !(m_bits[i] && b.m_bits[i]);
  }
  ret.m_mpz = ret.bits_to_unsigned_mpz();
  return ret;
}

mybitset mybitset::bvnor(mybitset const &b) const
{
  ASSERT(m_bits.size() == b.m_bits.size());
  mybitset ret(*this);
  for (size_t i = 0; i < m_bits.size(); i++) {
    ret.m_bits[i] = !(m_bits[i] || b.m_bits[i]);
  }
  ret.m_mpz = ret.bits_to_unsigned_mpz();
  return ret;
}

mybitset mybitset::bvxnor(mybitset const &b) const
{
  ASSERT(m_bits.size() == b.m_bits.size());
  mybitset ret(*this);
  for (size_t i = 0; i < m_bits.size(); i++) {
    ret.m_bits[i] = !(m_bits[i] ^ b.m_bits[i]);
  }
  ret.m_mpz = ret.bits_to_unsigned_mpz();
  return ret;
}

bool mybitset::bitAt(size_t i) const
{
  return m_bits.at(i);
}

size_t mybitset::get_numbits() const
{
  return m_bits.size();
}

std::string
mybitset::toString() const
{
  if (m_bits.size() == 0) {
    return "";
  }
  return m_mpz.get_str(10);
}

std::string
mybitset::toStringAsSignedNumber() const
{
  if (m_bits.size() == 0) {
    return "";
  }
  mpz_class n = this->bits_to_signed_mpz();
  return n.get_str(10);
}

int mybitset::toInt() const
{
  //ASSERT(m_bits.size() <= sizeof(int) * BYTE_LEN);
  int ret = 0;
  for (size_t i = 0; i < m_bits.size(); i++) {
    if (m_bits.at(i)) {
      if (i > sizeof(int) * BYTE_LEN) {
        cout << __func__ << " " << __LINE__ << ": this->toString() = " << this->toString() << endl;
      }
      ASSERT(i <= sizeof(int) * BYTE_LEN);
      ret |= (1 << i);
    }
  }
  return ret;
}

long mybitset::toLong() const
{
  ASSERT(m_bits.size() <= sizeof(long) * BYTE_LEN);
  long ret = 0;
  for (size_t i = 0; i < m_bits.size(); i++) {
    if (m_bits.at(i)) {
      ret |= (1L << i);
    }
  }
  return ret;
}

long long mybitset::toLongLong() const
{
  ASSERT(m_bits.size() <= sizeof(long long) * BYTE_LEN);
  long long ret = 0;
  for (size_t i = 0; i < m_bits.size(); i++) {
    if (m_bits.at(i)) {
      ret |= (1LL << i);
    }
  }
  return ret;
}

unsigned int mybitset::toUnsignedInt() const
{
  return (unsigned int)toInt();
}

unsigned long mybitset::toUnsignedLong() const
{
  return (unsigned long)toLong();
}

unsigned long long mybitset::toUnsignedLongLong() const
{
  return (unsigned long long)toLongLong();
}

mpz_class mybitset::bits_to_unsigned_mpz() const
{
  if (m_bits.size() == 0) {
    return 0;
  }
  mpz_class ret = 0;
  for (ssize_t i = m_bits.size() - 1; i >= 0; i--) {
    ret <<= 1;
    if (m_bits[i]) {
      ++ret;
    }
  }
  return ret;
}

mpz_class mybitset::bits_to_signed_mpz() const
{
  if (m_bits.size() == 0) {
    return 0;
  }
  mpz_class ret;
  if (m_bits.at(m_bits.size() - 1)) {
    ret = -1;
  }
  for (ssize_t i = m_bits.size() - 2; i >= 0; i--) {
    ret = ret * 2;
    if (m_bits[i]) {
      ret += 1;
    }
  }
  return ret;
}

void
mybitset::sync_bits_with_mpz()
{
  mpz_class cur = m_mpz;
  ASSERT(cur >= 0);
  size_t i = 0;
  while (cur != 0 && i < m_bits.size()) {
    m_bits[i++] = mpz_isodd(cur);
    cur >>= 1;
  }

	if (i < m_bits.size())
		fill(m_bits.begin()+i, m_bits.end(), false);

  CPP_DBG_EXEC(ASSERTCHECKS,
    bool overflow = i >= m_bits.size();
    if (!overflow && this->bits_to_signed_mpz() != this->get_signed_mpz()) {
      cout << __func__ << " " << __LINE__ << ": m_mpz = " << m_mpz << endl;
      cout << __func__ << " " << __LINE__ << ": signed m_mpz = " << this->get_signed_mpz() << endl;
      cout << __func__ << " " << __LINE__ << ": signed bits = " << this->bits_to_signed_mpz() << endl;
      cout << __func__ << " " << __LINE__ << ": m_bits.size() = " << m_bits.size() << endl;
      NOT_REACHED();
    });
}

/* return signed value of m_mpz */
mpz_class
mybitset::get_signed_mpz() const
{
  if (m_bits.size() == 0) {
    return 0;
  }
  if (m_bits.back()) {
    mpz_class mod = mpz_exp2(m_bits.size());
    ASSERT(m_mpz < mod);
    return -mpz_mod_exp2(mod - m_mpz, m_bits.size());
  } else {
    return m_mpz;
  }
}

mpz_class
mybitset::get_unsigned_mpz() const
{
  return m_mpz;
}

/* the mpz wrapper rises */

/******************** NON-MEMBER OPERATORS ********************/
/**************************************************************/

std::ostream& operator<<(std::ostream &s, const mybitset &n)
{
  s << n.toString();
  return s;
}

mybitset
mybitset::mybitset_bvconcat(vector<mybitset> const &vec)
{
  mybitset ret;
  for (auto iter = vec.rbegin(); iter != vec.rend(); iter++) {
    mybitset const &elem = *iter;
    ret.m_mpz = ret.m_mpz | (elem.m_mpz << (ret.m_bits.size()));
    for (auto const& elembit : elem.m_bits) {
      ret.m_bits.push_back(elembit);
    }
  }
  return ret;
}

mybitset
mybitset::mybitset_rand(size_t numbits)
{
  mybitset ret;
  for (size_t i = 0; i < numbits; i++) {
    ret.m_bits.push_back((rand() % 2) == 0);
  }
  return ret;
}

int
mybitset::mybitset_shift_arith_right_zeros(int shiftcount)
{
  int num_zeros = 0;
  for (int i = 0; i < shiftcount && !m_bits.at(i); i++) {
    num_zeros++;
  }
  *this = this->bvashr(num_zeros);
  return num_zeros;
}

}
