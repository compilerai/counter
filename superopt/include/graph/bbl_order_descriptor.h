#pragma once
#include "support/string_ref.h"
#include "support/debug.h"
namespace eqspace {

class bbl_order_descriptor_t
{
private:
  int m_bbl_num;
  string_ref m_bbl_name;
public:
  bbl_order_descriptor_t(int bbl_num, string const& bbl_name) : m_bbl_num(bbl_num), m_bbl_name(mk_string_ref(bbl_name))
  { }
  int get_bbl_num() const { return m_bbl_num; }
  string_ref const& get_bbl_name() const { return m_bbl_name; }
  bool operator==(bbl_order_descriptor_t const& other) const
  {
    if (m_bbl_num == other.m_bbl_num) {
      //ASSERT(m_bbl_name == other.m_bbl_name);
      //return true;
      return m_bbl_name == other.m_bbl_name;
    } else {
      return false;
    }
  }
  string to_string() const
  {
    return int_to_string(m_bbl_num) + ":" + m_bbl_name->get_str();
  }

  static bbl_order_descriptor_t const& invalid()
  {
    static bbl_order_descriptor_t ret(-1, "INVALID");
    return ret;
  }
};

}
