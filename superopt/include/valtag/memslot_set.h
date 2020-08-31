#ifndef MEMSLOT_SET_H
#define MEMSLOT_SET_H
#include <set>
#include <map>
#include "support/types.h"
#include "valtag/valtag.h"

class memslot_set_t
{
private:
  std::map<dst_ulong, uint64_t> m_map; //map from addr to bitmap
public:
  bool memslot_set_belongs(dst_ulong addr) const;
  //void memslot_set_add(dst_ulong addr);
  void memslot_set_add_mapping(dst_ulong addr, uint64_t bitmap);
  std::string memslot_set_to_string() const;
  void memslot_set_round_up_bitmaps_to_byte_size();
  void memslot_set_clear();
  size_t memslot_set_size() const;
  uint64_t memslot_set_get_elem(dst_ulong addr) const
  {
    if (!m_map.count(addr)) {
      return 0;
    }
    ASSERT(m_map.count(addr));
    return m_map.at(addr);
  }
  void memslot_set_from_memset(memset_t const *memset);
  void memslot_set_copy(memslot_set_t const &other);
  void memslot_set_union(memslot_set_t const &other);
  void memslot_set_diff(memslot_set_t const &other);
  std::map<dst_ulong, uint64_t> const &get_map() const { return m_map; }
  static memslot_set_t const &memslot_set_empty();
  static void memslot_set_subset_enumerate_first(memslot_set_t *sub, memslot_set_t const *orig);
  static bool memslot_set_subset_enumerate_next(memslot_set_t *sub, memslot_set_t const *orig);
};

#endif
