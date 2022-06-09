#pragma once
#include "valtag/regmap.h"

class src_iseq_match_renamings_t
{
private:
  regmap_t m_regmap;
  imm_vt_map_t m_imm_vt_map;
  nextpc_map_t m_nextpc_map;
public:
  src_iseq_match_renamings_t() {}
  //src_iseq_match_renamings_t(src_iseq_match_renamings_t const &other)
  //{
  //  *this = other;
  //}
  void clear()
  {
    regmap_init(&m_regmap);
    imm_vt_map_init(&m_imm_vt_map);
    nextpc_map_init(&m_nextpc_map);
  }
  void set_regmap(regmap_t const &regmap)
  {
    regmap_copy(&m_regmap, &regmap);
  }
  void set_imm_vt_map(imm_vt_map_t const &imm_vt_map)
  {
    imm_vt_map_copy(&m_imm_vt_map, &imm_vt_map);
  }
  void set_nextpc_map(nextpc_map_t const &nextpc_map)
  {
    nextpc_map_copy(&m_nextpc_map, &nextpc_map);
  }
  void update(regmap_t const &regmap, imm_vt_map_t const &imm_vt_map, nextpc_map_t const &nextpc_map)
  {
    this->set_regmap(regmap);
    this->set_imm_vt_map(imm_vt_map);
    this->set_nextpc_map(nextpc_map);
  }
  void operator=(src_iseq_match_renamings_t const &other)
  {
    regmap_copy(&m_regmap, &other.m_regmap);
    imm_vt_map_copy(&m_imm_vt_map, &other.m_imm_vt_map);
    nextpc_map_copy(&m_nextpc_map, &other.m_nextpc_map);
  }
  regmap_t const &get_regmap() const { return m_regmap; }
  imm_vt_map_t  const &get_imm_vt_map() const { return m_imm_vt_map; }
  nextpc_map_t const &get_nextpc_map() const { return m_nextpc_map; }
};
