#ifndef EQCHECKBINARY_SEARCH_H
#define EQCHECKBINARY_SEARCH_H

#include <set>

#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/utils.h"

class binary_search_t {
public:
  std::set<size_t> do_binary_search(set<size_t> const& indices) const
  {
    /*ASSERT(size);
    std::set<size_t> indices;
    for (size_t i = 0; i < size; i++) {
      indices.insert(i);
    }*/
    return binsearch(indices, std::set<size_t>());
  }
private:
  virtual bool test(std::set<size_t> const& indices) const = 0;
  std::set<size_t> binsearch(std::set<size_t> const& indices, std::set<size_t> const& const_indices) const
  {
    ASSERT(indices.size());
    ASSERT(set_intersection_is_empty(indices, const_indices));
    if (indices.size() == 1) {
      set<size_t> indices_plus_const = indices;
      set_union(indices_plus_const, const_indices);
      //cout << _FNLN_ << ": returning\n";
      return indices_plus_const;
    }

    auto mid = indices.begin();
    std::advance(mid, indices.size() / 2);

    std::set<size_t> left, right;
    for (auto it = indices.begin(); it != mid; it++) {
      left.insert(*it);
    }
    for (auto it = mid; it != indices.end(); it++) {
      right.insert(*it);
    }

    set<size_t> left_plus_const = left;
    set_union(left_plus_const, const_indices);
    bool left_result = test(left_plus_const);

    if (left_result) {
      //cout << _FNLN_ << ": calling binsearch(left)\n";
      return binsearch(left, const_indices);
    }

    set<size_t> right_plus_const = right;
    set_union(right_plus_const, const_indices);
    bool right_result = test(right_plus_const);

    if (right_result) {
      //cout << _FNLN_ << ": calling binsearch(right)\n";
      return binsearch(right, const_indices);
    }

    ASSERT(!left_result && !right_result);
    //first, put right in const indices and binsearch on left
    std::set<size_t> new_const_indices = const_indices;
    set_union(new_const_indices, right);
    //cout << _FNLN_ << ": calling binsearch(left, new_const_indices)\n";
    std::set<size_t> new_left = binsearch(left, new_const_indices);
    set_difference(new_left, new_const_indices);

    //then, put new_left in const indices and binsearch on right
    new_const_indices = const_indices;
    set_union(new_const_indices, new_left);
    //cout << _FNLN_ << ": calling binsearch(right, new_const_indices)\n";
    return binsearch(right, new_const_indices);
  }
};

#endif
