#ifndef INSN_DB_H
#define INSN_DB_H
#include "rewrite/src_iseq_match_renamings.h"

class usedef_tab_entry_t;
class ts_tab_entry_t;

template<typename I, typename V>
class insn_db_list_elem_t
{
public:
  I i;
  V v;
  insn_db_list_elem_t(I const &_i, V const &_v) : i(_i), v(_v) {}
};

template<typename I, typename V>
class insn_db_match_t
{
public:
  insn_db_list_elem_t<I, V> &e; //non-const reference, so its value can be changed by the client (e.g., to implement on-demand tfg construction of insns)
  //regmap_t regmap;
  //imm_vt_map_t imm_vt_map;
  //nextpc_map_t nextpc_map;
  src_iseq_match_renamings_t src_iseq_match_renamings;
  insn_db_match_t(insn_db_list_elem_t<I, V> &_e, src_iseq_match_renamings_t const &_src_iseq_match_renamings/*regmap_t const &_regmap, imm_vt_map_t const &_imm_vt_map, nextpc_map_t const &_nextpc_map*/) : e(_e), src_iseq_match_renamings(_src_iseq_match_renamings)//regmap(_regmap), imm_vt_map(_imm_vt_map), nextpc_map(_nextpc_map)
  {
  }
};



template<typename I, typename V>
class insn_db_list_t
{
private:
  std::vector<insn_db_list_elem_t<I, V>> m_vec;
public:
  insn_db_list_t() {}
  void insn_db_list_add(I const &i, V const &v)
  {
    m_vec.push_back(insn_db_list_elem_t<I, V>(i, v));
  }
  vector<insn_db_match_t<I, V>> insn_db_list_get_all_matches(I const &i, bool var)
  {
    autostop_timer func_timer(__func__);
    vector<insn_db_match_t<I, V>> ret;
    //imm_vt_map_t *imm_vt_map = new imm_vt_map_t;
    //nextpc_map_t *nextpc_map = new nextpc_map_t;
    //regmap_t *regmap = new regmap_t;
    src_iseq_match_renamings_t *src_iseq_match_renamings = new src_iseq_match_renamings_t;
    for (auto &e : m_vec) {
      if (i.insn_matches_template(e.i, *src_iseq_match_renamings/* *regmap, *imm_vt_map, *nextpc_map*/, var)) {
        ret.push_back(insn_db_match_t<I, V>(e, *src_iseq_match_renamings/* *regmap, *imm_vt_map, *nextpc_map*/));
      }
    }
    delete src_iseq_match_renamings;
    //delete imm_vt_map;
    //delete nextpc_map;
    //delete regmap;
    return ret;
  }
};

template<typename I, typename V>
class insn_db_t
{
private:
  insn_db_list_t<I, V> *m_hash;
  size_t m_hash_size;
public:
  insn_db_t(int hash_size) : m_hash_size(hash_size)
  {
    m_hash = new insn_db_list_t<I, V>[hash_size];
  }
  void insn_db_add(I const &i, V const &v)
  {
    long index = i.insn_hash_func_after_canon(m_hash_size);
    m_hash[index].insn_db_list_add(i, v);
  }
  vector<insn_db_match_t<I, V>> insn_db_get_all_matches(I const &i, bool var)
  {
    autostop_timer func_timer(__func__);
    long index = i.insn_hash_func_after_canon(m_hash_size);
    CPP_DBG_EXEC(INSN_MATCH, cout << __func__ << " " << __LINE__ << ": index = " << index << endl);
    return m_hash[index].insn_db_list_get_all_matches(i, var);
  }
  void clear()
  {
    delete [] m_hash;
  }
};

shared_ptr<insn_db_t<src_insn_t, usedef_tab_entry_t>> src_usedef_db_read(char const *filename);
shared_ptr<insn_db_t<dst_insn_t, usedef_tab_entry_t>> dst_usedef_db_read(char const *filename);
insn_db_t<src_insn_t, ts_tab_entry_t> src_types_db_read(char const *filename);
insn_db_t<dst_insn_t, ts_tab_entry_t> dst_types_db_read(char const *filename);

#endif
