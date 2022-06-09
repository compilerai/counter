#ifndef SEARCHABLE_QUEUE_H
#define SEARCHABLE_QUEUE_H

#include <queue>
#include <list>
#include "support/debug.h"
#include "support/dyn_debug.h"

template<typename T>
class searchable_queue_t
{
private:
  queue<T> m_q;
  list<T> m_list;
public:
  void searchable_queue_push_back(list<T> const& l) {
    for (auto const& t : l) {
      this->searchable_queue_push_back(t);
    }
  }
  size_t size() const { return m_list.size(); }
  void searchable_queue_push_back(T const& t) {
    for (auto iter = m_list.cbegin(); iter != m_list.cend(); iter++) {
      if (*iter == t) {
        return;
      }
    }
    m_q.push(t);
    m_list.push_back(t);
  }
  T searchable_queue_pop_front() {
    ASSERT(m_q.size() == m_list.size());
    //cout << _FNLN_ << ": m_q.size = " << m_q.size() << endl;
    T ret = m_q.front();
    m_q.pop();
    bool found = false;
    for (auto iter = m_list.begin(); iter != m_list.end();) {
      if (*iter == ret) {
        iter = m_list.erase(iter);
        found = true;
      } else {
        iter++;
      }
    }
    ASSERT(found);
    //m_set.erase(ret);
    return ret;
  }
  bool empty() const { return m_q.empty(); }
};

#endif
