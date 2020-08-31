#ifndef MEMSLOT_MAP_H
#define MEMSLOT_MAP_H
#include <map>
#include "support/types.h"
#include "valtag/valtag.h"

class memslot_map_t
{
private:
  typedef enum { MEMSLOT_MAP_TYPE_MEM, MEMSLOT_MAP_TYPE_CONST } memslot_map_type_t;
  std::map<dst_ulong, std::pair<memslot_map_type_t, valtag_t>> m_map; //map from addr->(type, addr) or addr->(type, imm-val)
  int m_base_reg;
public:
  memslot_map_t(int base_reg) : m_base_reg(base_reg) {}
  bool memslot_map_belongs(dst_ulong addr) const;
  bool memslot_map_denotes_constant(dst_ulong addr) const;
  valtag_t const &memslot_map_get_mapping(dst_ulong addr) const;
  void memslot_map_add_memaddr_mapping(dst_ulong peep_addr, valtag_t const &vt);
  void memslot_map_add_constant_mapping(dst_ulong peep_addr, valtag_t const &vt);
  int get_base_reg() const { return m_base_reg; }
  std::string memslot_map_to_string() const;
  bool memslot_map_is_empty() const { return m_map.size() == 0; }
  dst_ulong memslot_map_get_max_valtag_val() const;
  static memslot_map_t const *memslot_map_for_src_env_state();
};

#endif
