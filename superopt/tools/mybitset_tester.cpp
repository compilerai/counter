#include "support/mybitset.h"
#include "support/bv_const.h"

#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace eqspace;


//template<typename T>
//void assertcheck(T const& a, T const& b)
//{
//  if (!(a == b)) {
//    cout << a << " != " << b << endl;
//    assert(a == b);
//  }
//}

#define assertcheck(a, b) do { if (!((a) == (b))) { cout << #a << " != " << #b << endl; assert(0); } } while (0);

int main(int argc, char **argv)
{
  constexpr int bvlen = 32;

  assertcheck(((mybitset(-1,bvlen)+mybitset(1,bvlen)) & mybitset(~0,bvlen)), mybitset(0,bvlen));
  assertcheck(mybitset(-1,bvlen)+mybitset(~0,bvlen), mybitset(-2,bvlen));
  assertcheck(mybitset(-1,bvlen).bv_right_rotate(bvlen/2), mybitset(-1,bvlen));
  assertcheck(mybitset(-1,bvlen).bv_left_rotate(bvlen/2), mybitset(-1,bvlen));
  assertcheck(mybitset("0xff00ff",24).bv_left_rotate(8), mybitset("0x00ffff",24));
  assertcheck(mybitset(-1,bvlen).bvextract(bvlen/2-1,0), ~mybitset(0,bvlen/2));
  assertcheck(mybitset(204159,bvlen).bvextract(bvlen-1,0), mybitset(204159,bvlen));
  assertcheck((mybitset(2049,bvlen)*mybitset(2049,bvlen/2)).bvurem(mybitset(1<<16,bvlen)), mybitset(4097,bvlen));
  assertcheck(mybitset::mybitset_bvconcat({ mybitset("0xca",8), mybitset("0xfe",8), mybitset("0xba",8), mybitset("0xbe",8)}), mybitset(0xcafebabe,32));

  //==== Corner cases: for bv{s,u}div and bv{s,y}rem ====

  // if b is zero vector
  // (bvudiv a b)  = 0b111...1
  assertcheck(mybitset(0,32).bvudiv(mybitset(0,32)), mybitset(-1,32));
  assertcheck(mybitset(-1,32).bvudiv(mybitset(0,32)), mybitset(-1,32));
  // (bvurem a b)  = a
  assertcheck(mybitset(-1,32).bvurem(mybitset(0,32)), mybitset(-1,32));
  assertcheck(mybitset(1,32).bvurem(mybitset(0,32)), mybitset(1,32));

  // if b is zero vector
  // (bvsdiv a b)  = 0b000..01 if a is negative
  // (bvsdiv a b)  = 0b111...1 if a is non-negative
  assertcheck(mybitset(0,32).bvsdiv(mybitset(0,32)), mybitset(-1,32));
  assertcheck(mybitset(-1,32).bvsdiv(mybitset(0,32)), mybitset(1,32));
  // (bvsrem a b)  = a
  assertcheck(mybitset(0,32).bvsrem(mybitset(0,32)), mybitset(0,32));
  assertcheck(mybitset(-1,32).bvsrem(mybitset(0,32)), mybitset(-1,32));

  assertcheck(bv_round_down_to_power_of_two(-3), -4);
  assertcheck(bv_round_down_to_power_of_two(-16), -16);

  assertcheck(bv_round_up_to_power_of_two(-3), -2);
  assertcheck(bv_round_up_to_power_of_two(-16), -16);

  assertcheck(bv_round_down_to_power_of_two(3), 2);
  assertcheck(bv_round_down_to_power_of_two(16), 16);

  assertcheck(bv_round_up_to_power_of_two(3), 4);
  assertcheck(bv_round_up_to_power_of_two(16), 16);

  assertcheck(bv_round_down_to_power_of_two(0), 0);
  assertcheck(bv_round_up_to_power_of_two(0), 0);

  return 0;
}
