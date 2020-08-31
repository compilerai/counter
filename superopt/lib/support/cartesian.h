#ifndef CARTESIAN_H
#define CARTESIAN_H
#include <list>

using namespace std;
/* input:  a->{1,2}, b->{3, 4}
   output: {(a->1, b->3), (a->1, b->4), (a->2, b->3), (a->2, b->4)}
*/
template<typename KEY, typename VALUE>
class cartesian_t
{
public:
  cartesian_t(map<KEY, list<VALUE>> const &choices) : m_choices(choices)
  {
  }
  list<map<KEY, VALUE>> get_all_possibilities()
  {
    return get_all_rec(m_choices);
  }
private:
  static list<map<KEY, VALUE>> get_all_rec(map<KEY, list<VALUE>> const &choices)
  {
    KEY const *key_to_be_expanded = NULL;
    for (const auto &choice : choices) {
      KEY const &k = choice.first;
      //if (reg_identifier_t const * r = reinterpret_cast<reg_identifier_t const *>(&k)) {
      //  cout << __func__ << " " << __LINE__ << ": k = " << r->reg_identifier_to_string() << endl;
      //}
      list<VALUE> const &values = choice.second;
      if (values.size() == 0) {
        return list<map<KEY, VALUE>>();
      }
      if (values.size() > 1) {
        key_to_be_expanded = &k;
        break;
      }
    }
    if (!key_to_be_expanded) {
      map<KEY, VALUE> ret;
      for (const auto &choice : choices) {
        ASSERT(choice.second.size() == 1);
        ret.insert(make_pair(choice.first, *choice.second.begin()));
      }
      return list<map<KEY, VALUE>>({ret});
    }
    //if (reg_identifier_t const * r = reinterpret_cast<reg_identifier_t const *>(key_to_be_expanded)) {
    //  cout << __func__ << " " << __LINE__ << ": key_to_be_expanded = " << r->reg_identifier_to_string() << endl;
    //}
    map<KEY, list<VALUE>> choices1 = choices;
    map<KEY, list<VALUE>> choices2 = choices;
    list<VALUE> const &values = choices.at(*key_to_be_expanded);
    ASSERT(values.size() > 1);
    choices1.erase(*key_to_be_expanded);
    choices2.erase(*key_to_be_expanded);
    choices1.insert(make_pair(*key_to_be_expanded, list<VALUE>({*values.begin()})));
    list<VALUE> cdr = values;
    cdr.pop_front();
    choices2.insert(make_pair(*key_to_be_expanded, cdr));
    list<map<KEY, VALUE>> result1 = get_all_rec(choices1);
    list<map<KEY, VALUE>> result2 = get_all_rec(choices2);
    result2.insert(result2.begin(), result1.begin(), result1.end());
    return result2;
  }
  map<KEY, list<VALUE>> const &m_choices;
};

#endif
