#ifndef PEEP_ENTRY_LIST_H
#define PEEP_ENTRY_LIST_H
#include <list>
#include "rewrite/peephole.h"
#include "valtag/imm_vt_map.h"
#include "valtag/regmap.h"
#include "valtag/nextpc_map.h"
#include "codegen/etfg_insn.h"

class peep_entry_list_t
{
private:
  static bool peep_entry_less(peep_entry_t const *a, peep_entry_t const *b)
  {
    /*static regmap_t st_regmap;
    static imm_vt_map_t st_imm_vt_map;
    static nextpc_map_t nextpc_map;
    if (   a->src_iseq_len == b->src_iseq_len
        && src_iseq_matches_template_var(b->src_iseq, a->src_iseq, a->src_iseq_len, &st_regmap, &st_imm_vt_map, &nextpc_map)) {
      return true;
    }*/
    return a->cost < b->cost;
  }
  std::list<peep_entry_t *> m_ls;
public:
  typedef std::list<peep_entry_t *>::iterator iterator;
  typedef std::list<peep_entry_t *>::const_iterator const_iterator;
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
  size_t size()
  {
    return m_ls.size();
  }
  void insert(peep_entry_t *e)
  {
    iterator iter;
    for (iter = m_ls.begin(); iter != m_ls.end(); iter++) {
      if (peep_entry_less(e, *iter)) {
        break;
      }
    }
    m_ls.insert(iter, e);
  }
  void clear()
  {
    m_ls.clear();
  }
};

#endif
