#ifndef PERMUTE_H
#define PERMUTE_H
#include <list>

using namespace std;
/* input:  {1,2}
   output: {{1, 2}, {2, 1}}
*/
template<typename VALUE>
class permute_t
{
public:
  permute_t(list<VALUE> const &values) : m_values(values)
  {
  }
  list<list<VALUE>> get_all_permutations()
  {
    return get_all_rec(m_values);
  }
  bool get_next_permutation(list<VALUE> &perm)
  {
    //cout << "before perm: "; print_list(perm);
    list<size_t> indices = get_indices_for_perm(perm);
    //cout << "before indices: "; print_list(indices);
    if (!increment_indices(indices)) {
      return false;
    }
    //cout << "after indices: "; print_list(indices);
    perm = get_perm_for_indices(indices);
    //cout << "after perm: "; print_list(perm);
    return true;
  }
private:
  template<typename T>
  static void print_list(list<T> const &ls)
  {
    for (const auto &p : ls) {
      cout << " " << p;
    }
    cout << endl;
  }
  list<size_t> get_indices_for_perm(list<VALUE> const &perm)
  {
    list<size_t> ret;
    for (auto iter = m_values.cbegin(); iter != m_values.cend(); iter++) {
      size_t index = find_index_for_value_in_perm(*iter, perm, iter, m_values.cend());
      ret.push_back(index);
    }
    return ret;
  }
  list<VALUE> get_perm_for_indices(list<size_t> const &indices)
  {
    size_t i = 0;
    list<VALUE> ret;
    auto viter = m_values.crbegin();
    for (auto iter = indices.crbegin(); iter != indices.crend(); iter++, viter++, i++) {
      ASSERT(*iter >= 0 && *iter <= i);
      insert_at_position(ret, *iter, *viter);
    }
    return ret;
  }
  static bool increment_indices(list<size_t> &indices)
  {
    size_t i = 0;
    for (auto iter = indices.rbegin(); iter != indices.rend(); iter++, i++) {
      ASSERT(*iter >= 0 && *iter <= i);
      if (*iter < i) {
        (*iter)++;
        return true;
      }
      *iter = 0;
    }
    return false;
  }

  static bool value_belongs_to_list_of_values(VALUE const &v, typename list<VALUE>::const_iterator values_begin, typename list<VALUE>::const_iterator values_end)
  {
    for (auto iter = values_begin; iter != values_end; iter++) {
      if (*iter == v) {
        return true;
      }
    }
    return false;
  }

  static size_t find_index_for_value_in_perm(VALUE const &v, list<VALUE> const &perm, typename list<VALUE>::const_iterator values_begin, typename list<VALUE>::const_iterator values_end)
  {
    size_t cur_index = 0;
    for (const auto &p : perm) {
      if (!value_belongs_to_list_of_values(p, values_begin, values_end)) {
        continue;
      }
      if (p == v) {
        return cur_index;
      }
      cur_index++;
    }
    NOT_REACHED();
  }

  static void insert_at_position(list<VALUE> &out, long pos, VALUE const &v)
  {
    long i = 0;
    auto iter = out.begin();
    for (; i < pos; iter++, i++);
    out.insert(iter, v);
  }
  static list<list<VALUE>> get_all_rec(list<VALUE> const &choices)
  {
    if (choices.size() == 0) {
      return list<list<VALUE>>();
    }
    VALUE v = *choices.begin();
    if (choices.size() == 1) {
      list<VALUE> ls = {v};
      list<list<VALUE>> ret = {ls};
      return ret;
    }
    list<VALUE> choices_sub = choices;
    choices_sub.pop_front();
    list<list<VALUE>> perms_sub = get_all_rec(choices_sub);
    list<list<VALUE>> ret;
    for (list<VALUE> const &perm_sub : perms_sub) {
      for (long i = 0; i < perm_sub.size() + 1; i++) {
        list<VALUE> perm = perm_sub;
        insert_at_position(perm, i, v);
        ret.push_back(perm);
      }
    }
    return ret;
  }
  list<VALUE> const &m_values;
};

#endif
