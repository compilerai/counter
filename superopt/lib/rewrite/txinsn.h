#ifndef TXINSN_H
#define TXINSN_H
#include "support/debug.h"
#include "support/types.h"
#include "support/src_tag.h"

class txinsn_t
{
private:
  src_tag_t m_tag;
  long m_val;
public:
  txinsn_t() : m_tag(tag_invalid), m_val(0) {}
  txinsn_t(src_tag_t tag, long val) : m_tag(tag), m_val(val) {}
  src_tag_t txinsn_get_tag() const { ASSERT(m_tag != tag_invalid); return m_tag; }
  long txinsn_get_val() const { ASSERT(m_tag != tag_invalid); return m_val; }
  string txinsn_to_string() const
  {
    stringstream ss;
    ss << "(" << (m_tag == tag_invalid ? "invalid" : ((m_tag == tag_var) ? "var" : "const")) << ", " << m_val << ")";
    return ss.str();
  }
};

#endif
