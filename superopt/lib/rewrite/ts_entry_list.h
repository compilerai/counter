#ifndef USEDEF_ENTRY_LIST_H
#define USEDEF_ENTRY_LIST_H
#include <list>
#include "rewrite/ts_tab_entry.h"

/*class ts_entry_list_t
{
private:
  std::list<ts_tab_entry_t *> m_ls;
public:
  typedef list<ts_tab_entry_t *>::iterator iterator;
  typedef list<ts_tab_entry_t *>::const_iterator const_iterator;

  iterator begin()
  {
    return m_ls.begin();
  }

  const_iterator begin() const
  {
    return m_ls.begin();
  }

  iterator end()
  {
    return m_ls.end();
  }

  const_iterator end() const
  {
    return m_ls.end();
  }

  void insert(ts_tab_entry_t *e)
  {
    //iterator iter;
    //for (iter = m_ls.begin(); iter != m_ls.end(); iter++) {
    //  if (usedef_entry_less(e, *iter)) {
    //    break;
    //  }
    //}
    //m_ls.insert(iter, e);
    m_ls.push_back(e);
  }
};*/

#endif
