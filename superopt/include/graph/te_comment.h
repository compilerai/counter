#pragma once
#include <string>
#include "support/string_ref.h"
#include "graph/bbl_order_descriptor.h"

namespace eqspace {

class te_comment_t
{
private:
  bbl_order_descriptor_t m_bbl_order;
  bool m_is_phi;
  int m_insn_idx;
  string_ref m_code;
public:
  te_comment_t(bbl_order_descriptor_t const& bbl_order, bool is_phi, int insn_idx, string const& code) : m_bbl_order(bbl_order), m_is_phi(is_phi), m_insn_idx(insn_idx), m_code(mk_string_ref(code)) {}
  string get_string() const
  {
    std::stringstream ss;
    ss << m_bbl_order.to_string() << ":" << m_is_phi << ":" << m_insn_idx << ":" << m_code->get_str();
    return ss.str();
  }
  bbl_order_descriptor_t const& get_bbl_order_desc() const { return m_bbl_order; }
  bool is_phi() const { return m_is_phi; }
  int get_insn_idx() const { return m_insn_idx; }
  string_ref const& get_code() const { return m_code; }
  bool operator==(te_comment_t const& other) const
  {
    return    true
           && m_bbl_order == other.m_bbl_order
           && m_is_phi == other.m_is_phi
           && m_insn_idx == other.m_insn_idx
           && m_code == other.m_code
    ;
  }
  bool operator<(te_comment_t const& other) const //this ordering is useful to determine the layout in llptfg_t::output_llvm_code_for_tfg()
  {
    if (m_bbl_order.get_bbl_num() != other.m_bbl_order.get_bbl_num()) {
      return m_bbl_order.get_bbl_num() < other.m_bbl_order.get_bbl_num();
    }
    if (m_is_phi != other.m_is_phi) {
      return m_is_phi;
    }
    return m_insn_idx < other.m_insn_idx;
  }
  /*bool te_comment_is_special() const
  {
    return     false
            || *this == te_comment_bb_entry()
            || *this == te_comment_fcall_edge_start()
            || *this == te_comment_fcall_edge_arg()
            || *this == te_comment_fcall_edge_end()
            || *this == te_comment_start_pc_edge()
            || *this == te_comment_intrinsic_fcall_begin()
            || *this == te_comment_intrinsic_fcall_end()
            || *this == te_comment_intrinsic_usub_result()
            || *this == te_comment_intrinsic_usub_carry()
            || *this == te_comment_epsilon()
    ;
  }*/

  static te_comment_t const& te_comment_epsilon()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "epsilon");
    return ret;
  }

  static te_comment_t const& te_comment_bb_entry()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "bb_entry");
    return ret;
  }
  static te_comment_t const& te_comment_fcall_edge_start()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "fcall_edge_start");
    return ret;
  }
  static te_comment_t const& te_comment_fcall_edge_arg()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "fcall_edge_arg");
    return ret;
  }
  static te_comment_t const& te_comment_badref()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "badref");
    return ret;
  }
  static te_comment_t const& te_comment_fcall_edge_end()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "fcall_edge_end");
    return ret;
  }
  static te_comment_t const& te_comment_start_pc_edge()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "start_pc_edge");
    return ret;
  }
  static te_comment_t const& te_comment_intrinsic_fcall_begin()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "intrinsic_fcall_begin");
    return ret;
  }
  static te_comment_t const& te_comment_intrinsic_fcall_end()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "intrinsic_fcall_end");
    return ret;
  }
  static te_comment_t const& te_comment_intrinsic_usub_result()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "intrinsic_usub_result");
    return ret;
  }
  static te_comment_t const& te_comment_intrinsic_usub_carry()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "intrinsic_usub_carry");
    return ret;
  }
  static te_comment_t const& te_comment_not_implemented()
  {
    static te_comment_t ret(bbl_order_descriptor_t::invalid(), false, -1, "te_comment_not_implemented");
    return ret;
  }
};

}
