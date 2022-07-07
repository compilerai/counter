#pragma once

#include "support/debug.h"
#include "support/mybitset.h"

#include "expr/expr.h"
#include "gsupport/predicate.h"

using namespace std;

namespace eqspace {

enum class sp_relation_status_t {
         SP_RELATION_STATUS_TOP,
         SP_RELATION_STATUS_AND_MASK_OFFSET,
         SP_RELATION_STATUS_OR_MASK_OFFSET,
         SP_RELATION_STATUS_LT,
         SP_RELATION_STATUS_LE,
         SP_RELATION_STATUS_GT,
         SP_RELATION_STATUS_GE,
         SP_RELATION_STATUS_BOTTOM
};

class sp_relation_t
{
  sp_relation_status_t m_status;

  // only for SP_RELATION_STATUS_AND_MASK_OFFSET and SP_RELATION_STATUS_OR_MASK_OFFSET
  unsigned m_masklen = 0; // `mask'-lower bits are masked away (by 0 with AND, by 1 with OR)
  mybitset m_offset;  // offset added after masking
  // ((x >> masklen) << masklen) + offset

private:
  sp_relation_t(sp_relation_status_t status)
    : m_status(status)
  {}
  sp_relation_t(sp_relation_status_t status, unsigned masklen, mybitset offset)
    : m_status(status),
      m_masklen(masklen),
      m_offset(offset)
  {
    ASSERT(status == sp_relation_status_t::SP_RELATION_STATUS_AND_MASK_OFFSET || status == sp_relation_status_t::SP_RELATION_STATUS_OR_MASK_OFFSET);
  }

  static mybitset or_mask_for_masklen(unsigned masklen, size_t bvsize)
  {
    return mybitset((1<<masklen)-1, bvsize);
  }
  static mybitset and_mask_for_masklen(unsigned masklen, size_t bvsize)
  {
    return ~or_mask_for_masklen(masklen, bvsize);
  }

  static sp_relation_t meet_offsets(sp_relation_t const& a, sp_relation_t const& b);

  static string sp_relation_status_to_string(sp_relation_status_t sps);
  static sp_relation_status_t sp_relation_status_from_string(string const& s);

  bool implies_equality() const
  {
    if (!(   m_status == sp_relation_status_t::SP_RELATION_STATUS_AND_MASK_OFFSET
          || m_status == sp_relation_status_t::SP_RELATION_STATUS_OR_MASK_OFFSET)) {
      return false;
    }
    return m_masklen == 0 && m_offset.is_zero();
  }

public:
  sp_relation_t(istream& in);
  ostream& sp_relation_to_stream(ostream& os) const;

  string to_string() const
  {
    ostringstream ss;
    this->sp_relation_to_stream(ss);
    return ss.str();
  }

  bool is_top() const    { return m_status == sp_relation_status_t::SP_RELATION_STATUS_TOP; }
  bool is_bottom() const { return m_status == sp_relation_status_t::SP_RELATION_STATUS_BOTTOM; }

  sp_relation_t inverse() const;
  // if    (x,y) -> this,
  //       (y,z) -> other
  // then, (x,z) -> this.compse(other)
  sp_relation_t compose(sp_relation_t const& other) const;
  bool implies(sp_relation_t const& other) const;

  bool meet(sp_relation_t const& other);

  bool operator==(sp_relation_t const& other) const
  {
    // NOTE: too strong? do not check mask and offset for cases where they may not be set?
    return    m_status == other.m_status
           && m_masklen == other.m_masklen
           && m_offset == other.m_offset
           ;
  }
  bool operator!=(sp_relation_t const& other) const { return !(this->operator==(other)); }
  bool operator<(sp_relation_t const& other) const
  {
    return    m_status < other.m_status
           && m_masklen < other.m_masklen
           && m_offset < other.m_offset;
  }

  bool represents_inequality() const
  {
    return    this->m_status == sp_relation_status_t::SP_RELATION_STATUS_LT
           || this->m_status == sp_relation_status_t::SP_RELATION_STATUS_LE
           || this->m_status == sp_relation_status_t::SP_RELATION_STATUS_GT
           || this->m_status == sp_relation_status_t::SP_RELATION_STATUS_GE
      ;
  }

  shared_ptr<predicate const> pred_from_sp_relation(expr_ref const& a, expr_ref const& b) const;
  set<shared_ptr<predicate const>> to_non_trivial_preds(expr_ref const& a, expr_ref const& b) const;

  static sp_relation_t top()    { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_TOP); }
  static sp_relation_t bottom() { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_BOTTOM); }
  static sp_relation_t and_mask_offset(unsigned mask, mybitset offset) { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_AND_MASK_OFFSET, mask, offset); }
  static sp_relation_t or_mask_offset(unsigned mask, mybitset offset)  { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_OR_MASK_OFFSET, mask, offset); }
  static sp_relation_t less_than()    { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_LT); }
  static sp_relation_t greater_than() { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_GT); }
  static sp_relation_t less_than_or_equal_to()    { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_LE); }
  static sp_relation_t greater_than_or_equal_to() { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_GE); }
  static sp_relation_t equal_to(unsigned bvsize) { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_AND_MASK_OFFSET, 0, mybitset(0,bvsize)); }

  friend struct std::hash<sp_relation_t>;
};

}

namespace std {
template<>
struct hash<sp_relation_t>
{
  size_t operator()(sp_relation_t const &spr) const
  {
    size_t seed = 0;
    myhash_combine<size_t>(seed, hash<size_t>()(static_cast<std::underlying_type_t<sp_relation_status_t>>(spr.m_status)));
    myhash_combine<size_t>(seed, hash<size_t>()(spr.m_masklen));
    myhash_combine<size_t>(seed, hash<mybitset>()(spr.m_offset));
    return seed;
  }
};

}
