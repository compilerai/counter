#pragma once
#include <unordered_map>
//#include "debug.h"

namespace eqspace {

using namespace std;
typedef size_t pool_id_t;

template <typename T> class pool
{
public:
  pool() : m_cur_pool_id(1) {}
  pool_id_t reg_elem(T const &given)
  {
    if (m_reverse_lookup_map.count(given) == 0) {
      //ret = make_shared<T>(given);
      pool_id_t cur_pool_id = m_cur_pool_id;
      m_lookup_map.insert(make_pair(cur_pool_id, given));
      m_reverse_lookup_map.insert(make_pair(given, cur_pool_id));
      m_cur_pool_id++;
      return cur_pool_id;
    } else {
      return m_reverse_lookup_map.at(given);
    }
  }

  T const &get(pool_id_t id) {  return m_lookup_map.at(id);  }

  void free_elems()
  {
    m_lookup_map.clear();
    m_reverse_lookup_map.clear();
  }

private:
  std::unordered_map<pool_id_t, T> m_lookup_map;
  std::unordered_map<T, pool_id_t> m_reverse_lookup_map;
  pool_id_t m_cur_pool_id;
};

}
