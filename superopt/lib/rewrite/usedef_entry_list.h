#ifndef USEDEF_ENTRY_LIST_H
#define USEDEF_ENTRY_LIST_H
#include <list>
#include "rewrite/usedef_tab_entry.h"
#include "valtag/imm_vt_map.h"

class usedef_entry_list_t
{
private:
  std::list<usedef_tab_entry_t *> m_ls;
  static bool usedef_entry_less(usedef_tab_entry_t const *a, usedef_tab_entry_t const *b)
  {
    ASSERT(a->src == b->src);
    //sorting makes no sense because incomparable entries will be arbitrarily ordered and we may have an incomparable entry between two comparable entries in the wrong order: typical problem with partial orders
    /*static regmap_t regmap;
    static imm_vt_map_t st_imm_vt_map;
    static nextpc_map_t nextpc_map;
    if (a->src) {
      if (src_iseq_matches_template_var(&a->i.src_insn, &b->i.src_insn, 1, &regmap, &st_imm_vt_map, &nextpc_map, "")) {
        //cout << __func__ << " " << __LINE__ << ": comparing " << a->i.src_insn.m_comment  << " vs. " << b->i.src_insn.m_comment << " returned true\n";
        return true;
      } else if (src_iseq_matches_template_var(&b->i.src_insn, &a->i.src_insn, 1, &regmap, &st_imm_vt_map, &nextpc_map, "")) {
        //cout << __func__ << " " << __LINE__ << ": comparing " << a->i.src_insn.m_comment  << " vs. " << b->i.src_insn.m_comment << " returned false\n";
        return false;
      }
    } else {
      if (dst_iseq_matches_template_var(&a->i.dst_insn, &b->i.dst_insn, 1, &regmap, &st_imm_vt_map, &nextpc_map, "")) {
        return true;
      } else if (dst_iseq_matches_template_var(&b->i.dst_insn, &a->i.dst_insn, 1, &regmap, &st_imm_vt_map, &nextpc_map, "")) {
        return false;
      }
    }
    //cout << __func__ << " " << __LINE__ << ": comparing " << a->i.src_insn.m_comment  << " vs. " << b->i.src_insn.m_comment << " returning false\n";*/
    return false;
    /*if (a->src) {
#if SRC == ARCH_ETFG
      return a->i.src_insn.m_comment < b->i.src_insn.m_comment;
#endif
    }*/
    return false;
  }
public:
  typedef list<usedef_tab_entry_t *>::iterator iterator;
  typedef list<usedef_tab_entry_t *>::const_iterator const_iterator;

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

  void insert(usedef_tab_entry_t *e)
  {
    iterator iter;
    for (iter = m_ls.begin(); iter != m_ls.end(); iter++) {
      if (usedef_entry_less(e, *iter)) {
        break;
      }
    }
    m_ls.insert(iter, e);
  }
};

#endif
