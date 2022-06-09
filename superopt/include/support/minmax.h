#ifndef MINMAX_H
#define MINMAX_H
#include <stdlib.h>
#include <assert.h>
#include <sstream>
#include "support/utils.h"

class minmax_t
{
private:
  size_t m_min, m_max;
  bool m_is_undef;
public:
  minmax_t() : m_is_undef(true) {}
  bool minmax_is_undef() const { return m_is_undef; }
  void minmax_set_min_max(size_t min, size_t max) { m_min = min; m_max = max; m_is_undef = false;}
  void minmax_update(size_t c)
  {
    if (m_is_undef) {
      m_min = c;
      m_max = c;
      m_is_undef = false;
    } else {
      m_min = min(m_min, c);
      m_max = max(m_max, c);
    }
  }
  size_t minmax_get_min() const { assert(!m_is_undef); return m_min; }
  size_t minmax_get_max() const { assert(!m_is_undef); return m_max; }
  bool minmax_bitmap_downgrade(uint64_t &bitmap) const
  {
    if (m_is_undef) {
      return false;
    }
    int bitmap_numbits = bitmap_get_max_bit_set(bitmap);
    if (m_min == bitmap_numbits) {
      return false;
    }
    if (!bitmap_downgrade(bitmap)) {
      return false;
    }
    bitmap_numbits = bitmap_get_max_bit_set(bitmap);
    if (m_min > bitmap_numbits) {
      return false;
    }
    return true;
  }
  bool minmax_agrees_with_count(size_t c) const
  {
    if (m_is_undef) {
      return false;
    }
    return c >= m_min && c <= ROUND_UP(m_max + 1, BYTE_LEN);
  }
  string minmax_to_string() const
  {
    std::stringstream ss;
    if (m_is_undef) {
      ss << "(minmax_undef)";
    } else {
      ss << "(min " << m_min << ", max " << m_max << ")";
    }
    return ss.str();
  }
};

#endif
