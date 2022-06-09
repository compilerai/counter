#include "expr/context.h"
#include "expr/range.h"

#include <vector>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;
using namespace eqspace;

int main(int argc, char **argv)
{
  mybitset mb_0(0, 32);
  mybitset mb_10(10, 32);
  mybitset mb_11(11, 32);
  mybitset mb_20(20, 32);
  mybitset mb_21(20, 32);
  mybitset mb_30(30, 32);
  mybitset mb_31(31, 32);
  mybitset mb_60(60, 32);

  range_t a = range_t::range_from_mybitset(mb_0, mb_10);
  range_t b = range_t::range_from_mybitset(mb_11, mb_20);
  range_t c = range_t::range_from_mybitset(mb_21, mb_30);
  range_t d = range_t::range_from_mybitset(mb_0, mb_30);
  range_t e = range_t::range_from_mybitset(mb_31, mb_60);

  list<range_t> rl = { a, b, c };
  range_t u = e;
  cout << "Union of [";
  for (auto const& r : rl) cout << ' ' << r.to_string();
  cout << " ] with " << u.to_string() << " = ";
  range_t::union_with_sorted_range(rl, u);
  cout << "[";
  for (auto const& r : rl) cout << ' ' << r.to_string();
  cout << " ]";



  return 0;
}
