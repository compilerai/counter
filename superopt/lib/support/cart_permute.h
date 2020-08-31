#ifndef CART_PERMUTE_H
#define CART_PERMUTE_H
#include "support/permute.h"
#include "support/cartesian.h"

using namespace std;

/* this is similar to cartesian_t; except that the choices are given by all the permutations of the given list of values */
template<typename KEY, typename VALUE>
class cart_permute_t
{
public:
  cart_permute_t(map<KEY, list<VALUE>> const &perm_choices) : m_perm_choices(perm_choices)
  {
  }
  list<map<KEY, list<VALUE>>> get_all_possibilities()
  {
    return get_all_rec(m_perm_choices);
  }
  bool get_next_cartperm(map<KEY, list<VALUE>> &cartperm)
  {
    for (pair<KEY const, list<VALUE>> &kl : cartperm) {
      KEY const &k = kl.first;
      list<VALUE> &l = kl.second;
      list<VALUE> const &orig_l = m_perm_choices.at(k);
      permute_t<VALUE> permute(orig_l);
      if (permute.get_next_permutation(l)) {
        return true;
      }
      l = orig_l;
    }
    return false;
  }
private:
  list<map<KEY, list<VALUE>>> get_all_rec(map<KEY, list<VALUE>> const &perm_choices)
  {
    map<KEY, list<list<VALUE>>> choices;
    for (const auto &kl : perm_choices) {
      KEY const &k = kl.first;
      permute_t<VALUE> permute(kl.second);
      list<list<VALUE>> choices_for_k = permute.get_all_permutations();
      choices.insert(make_pair(k, choices_for_k));
    }
    cartesian_t<KEY, list<VALUE>> cartesian(choices);
    return cartesian.get_all_possibilities();
  }
  map<KEY, list<VALUE>> const &m_perm_choices;
};

#endif
