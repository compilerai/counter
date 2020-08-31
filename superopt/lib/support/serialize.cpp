#include "support/serialize.h"
#include "support/debug.h"

using namespace std;

void
serialize_int(ostream& os, int i)
{
  os << ", 0x" << hex << i << dec;
}

int
deserialize_int(istream& is)
{
  char comma;
  is >> comma;
  //cout << __func__ << " " << __LINE__ << ": comma = " << comma << ".\n";
  ASSERT(comma == ',');
  unsigned int ret;
  is >> hex >> ret >> dec;
  //cout << __func__ << " " << __LINE__ << ": ret = 0x" << hex << ret << dec << ".\n";
  return ret;
}

void
serialize_long(std::ostream& os, long i)
{
  os << ", 0x" << hex << i << dec;
}

long
deserialize_long(std::istream& is)
{
  char comma;
  is >> comma;
  ASSERT(comma == ',');
  unsigned long ret;
  is >> hex >> ret >> dec;
  return ret;
}

void
serialize_int64(std::ostream& os, int64_t i)
{
  os << ", 0x" << hex << i << dec;
}

int64_t
deserialize_int64(std::istream& is)
{
  char comma;
  is >> comma;
  ASSERT(comma == ',');
  uint64_t ret;
  is >> hex >> ret >> dec;
  return ret;
}

#ifdef __SIZEOF_INT128__
void
serialize_int128(std::ostream& os, __int128_t i)
{
  uint64_t hi = ((__uint128_t)i) >> 64;
  uint64_t lo = (__uint128_t)i;
  serialize_int64(os, hi);
  serialize_int64(os, lo);
}

__int128_t
deserialize_int128(std::istream& is)
{
  uint64_t hi = deserialize_int64(is);
  uint64_t lo = deserialize_int64(is);
  __uint128_t ret = ((__uint128_t)hi << 64) | (__uint128_t)lo;
  return ret;
}
#endif
