#pragma once
#include <iostream>

void serialize_int(std::ostream& os, int i);
int deserialize_int(std::istream& is);
void serialize_long(std::ostream& os, long i);
long deserialize_long(std::istream& is);
void serialize_int64(std::ostream& os, int64_t i);
int64_t deserialize_int64(std::istream& is);
#ifdef __SIZEOF_INT128__
void serialize_int128(std::ostream& os, __int128_t i);
__int128_t deserialize_int128(std::istream& is);
#endif
