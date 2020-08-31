#pragma once

#include <list>
#include <map>

#include "support/debug.h"

using namespace std;

template<typename T_KEY, typename T_VAL>
class lru_t
{
public:
  lru_t(size_t max_size) : m_max_size(max_size)
  {
    ASSERT(m_max_size > 1);
  }

  void clear()
  {
    m_map.clear();
    m_list.clear();
  }

  void put(T_KEY const& key, T_VAL const& val)
  {
    if (m_list.size() >= m_max_size) {
      auto itr = prev(m_list.end());
      m_map.erase(itr->first);
      m_list.pop_back();
    }
    m_list.push_front(make_pair(key,val));
    m_map[key] = m_list.begin(); // overwrite previous value, if any
  }

  bool lookup(T_KEY const& key) const
  {
    return m_map.count(key);
  }

  T_VAL const& get(T_KEY const& key) const
  {
    auto itr = m_map.find(key);
    ASSERT(itr != m_map.end());
    m_list.splice(m_list.begin(), m_list, itr->second); // move to front
    return m_list.front().second;
  }

private:
  size_t m_max_size;
  mutable list<pair<T_KEY,T_VAL>> m_list; // list's iterators are not invalidated upon insert or erase
  map<T_KEY,typename decltype(m_list)::const_iterator> m_map;
};

