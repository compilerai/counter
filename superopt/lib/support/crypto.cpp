#include "support/crypto.h"
#include <sstream>

using namespace std;

string
md5_checksum(string const& str)
{
  unsigned char result[MD5_DIGEST_LENGTH];
  MD5((unsigned char*)str.c_str(), str.size(), result);
  stringstream ss;
  ss << hex;
  for (size_t i = 0; i < MD5_DIGEST_LENGTH; i++) {
    ss << int(result[i]);
  }
  ss << dec;
  return ss.str();
}
