#pragma once
#include <stdlib.h>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <fenv.h>

#include "support/types.h"
#include "support/src_tag.h"
#include "support/mybitset.h"

#include "expr/defs.h"

using namespace std;


namespace eqspace {

class rounding_mode_t {
public:
  enum modes { rounding_mode_towards_zero_aka_truncate, rounding_mode_to_nearest_ties_to_even, rounding_mode_to_nearest_ties_away_from_zero, rounding_mode_towards_positive_infinity, rounding_mode_towards_negative_infinity };
private:
  modes m_mode;
public:
  rounding_mode_t() = delete;
  static rounding_mode_t round_towards_zero_aka_truncate()
  {
    return rounding_mode_t(rounding_mode_towards_zero_aka_truncate);
  }
  static rounding_mode_t round_to_nearest_ties_to_even()
  {
    return rounding_mode_t(rounding_mode_to_nearest_ties_to_even);
  }
  static rounding_mode_t round_to_nearest_ties_away_from_zero()
  {
    return rounding_mode_t(rounding_mode_to_nearest_ties_away_from_zero);
  }
  static rounding_mode_t round_towards_positive_infinity()
  {
    return rounding_mode_t(rounding_mode_towards_positive_infinity);
  }
  static rounding_mode_t round_towards_negative_infinity()
  {
    return rounding_mode_t(rounding_mode_towards_negative_infinity);
  }

  //XXX: round_mode_arbitrary is used when the result should hold for any rounding mode.
  //ideally, we should be using an OP_VAR in such cases, but because we currently cannot
  //model non-const rounding-modes, we have to make do with this hack. We should eventually
  //get rid of this hack.
  static rounding_mode_t round_mode_arbitrary();
  static rounding_mode_t round_mode_random();

  string rounding_mode_to_string() const;
  static rounding_mode_t rounding_mode_from_string(string const& s);

  static rounding_mode_t set_rounding_mode(rounding_mode_t new_rm)
  {
    int cpp_round = fegetround();
    int rc = fesetround(rounding_mode_to_cpp_round(new_rm));
    ASSERT(rc == 0);
    return cpp_round_to_rounding_mode(cpp_round);
  }

  bool operator==(rounding_mode_t const& other) const
  {
    return this->m_mode == other.m_mode;
  }

  mybitset rounding_mode_to_mybitset() const
  {
    switch (this->m_mode) {
      case rounding_mode_towards_zero_aka_truncate:
        return mybitset(0, SOLVER_ROUNDING_MODE_SORT_BVSIZE);
      case rounding_mode_to_nearest_ties_to_even:
        return mybitset(1, SOLVER_ROUNDING_MODE_SORT_BVSIZE);
      case rounding_mode_towards_positive_infinity:
        return mybitset(2, SOLVER_ROUNDING_MODE_SORT_BVSIZE);
      case rounding_mode_towards_negative_infinity:
        return mybitset(3, SOLVER_ROUNDING_MODE_SORT_BVSIZE);
      case rounding_mode_to_nearest_ties_away_from_zero:
        return mybitset(4, SOLVER_ROUNDING_MODE_SORT_BVSIZE);
      default: NOT_REACHED();
    }
  }

  static rounding_mode_t rounding_mode_from_mybitset(mybitset const& mbs)
  {
    ASSERT(mbs.get_numbits() == SOLVER_ROUNDING_MODE_SORT_BVSIZE);
    switch (mbs.toInt()) {
      case 0: return rounding_mode_towards_zero_aka_truncate;
      case 1: return rounding_mode_to_nearest_ties_to_even;
      case 2: return rounding_mode_towards_positive_infinity;
      case 3: return rounding_mode_towards_negative_infinity;
      case 4: return rounding_mode_to_nearest_ties_away_from_zero;
      default: NOT_REACHED();
    }
  }

private:
  rounding_mode_t(modes m) : m_mode(m)
  { }

  static int rounding_mode_to_cpp_round(rounding_mode_t rm)
  {
    switch (rm.m_mode) {
      case rounding_mode_towards_zero_aka_truncate:
        return FE_TOWARDZERO;
      case rounding_mode_to_nearest_ties_to_even:
        return FE_TONEAREST;
      case rounding_mode_towards_positive_infinity:
        return FE_UPWARD;
      case rounding_mode_towards_negative_infinity:
        return FE_DOWNWARD;
      case rounding_mode_to_nearest_ties_away_from_zero:
        NOT_IMPLEMENTED();
      default: NOT_REACHED();
    }
  }

  static rounding_mode_t cpp_round_to_rounding_mode(int cpp_round)
  {
    switch (cpp_round) {
      case FE_TOWARDZERO:
        return round_towards_zero_aka_truncate();
      case FE_TONEAREST:
        return round_to_nearest_ties_to_even();
      case FE_UPWARD:
        return round_towards_positive_infinity();
      case FE_DOWNWARD:
        return round_towards_negative_infinity();
      default: NOT_REACHED();
    }
  }
};

}
