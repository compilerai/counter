#ifndef TYPESYSTEM_H
#define TYPESYSTEM_H
#include <stdio.h>
#include <string>
#include <set>
#include <stdlib.h>
#include <stdint.h>
#include "superopt/typestate.h"
#include "support/debug.h"

typedef size_t typesystem_id_t;
typedef bool convert_addr_to_intaddr_t;
typedef bool make_non_poly_copyable_t;
typedef bool set_poly_bytes_in_non_poly_regs_to_bottom_t;
typedef bool enum_vconst_modifiers_t;
typedef size_t tmp_reg_count_t;
typedef size_t tmp_stackslot_count_t;

class typesystem_t
{
private:
  //typesystem_id_t m_id;
  use_types_t m_use_types;
  convert_addr_to_intaddr_t m_convert_addr_to_intaddr;
  make_non_poly_copyable_t m_make_non_poly_copyable;
  set_poly_bytes_in_non_poly_regs_to_bottom_t m_set_poly_bytes_in_non_poly_regs_to_bottom;
  enum_vconst_modifiers_t m_enum_vconst_modifiers;
  tmp_reg_count_t m_tmp_reg_count;
  tmp_stackslot_count_t m_tmp_stackslot_count;

  static vector<typesystem_t> get_typesystem_hierarchy(tmp_reg_count_t regcount, tmp_stackslot_count_t stackslot_count, bool allow_vconst_modifiers);
public:
  static size_t const max_num_temp_regs_for_typecheck = 3;
  static size_t const max_num_stack_slots_for_typecheck = 4;
  static size_t const max_num_temp_regs_for_enum = 2;
  static size_t const max_num_stack_slots_for_enum = 0;
  static size_t const stack_slot_increment_step = 4;
  typesystem_t(use_types_t use_types,
               convert_addr_to_intaddr_t convert_addr_to_intaddr,
               make_non_poly_copyable_t make_non_poly_copyable,
               set_poly_bytes_in_non_poly_regs_to_bottom_t set_poly_bytes_in_non_poly_regs_to_bottom,
               enum_vconst_modifiers_t enum_vconst_modifiers,
               tmp_reg_count_t tmp_reg_count,
               tmp_stackslot_count_t tmp_stackslot_count)
      : m_use_types(use_types),
        m_convert_addr_to_intaddr(convert_addr_to_intaddr),
        m_make_non_poly_copyable(make_non_poly_copyable),
        m_set_poly_bytes_in_non_poly_regs_to_bottom(set_poly_bytes_in_non_poly_regs_to_bottom),
        m_enum_vconst_modifiers(enum_vconst_modifiers),
        m_tmp_reg_count(tmp_reg_count),
        m_tmp_stackslot_count(tmp_stackslot_count)
  {
  }
  typesystem_t() : m_use_types(USE_TYPES_NONE),
                   m_convert_addr_to_intaddr(true),
                   m_make_non_poly_copyable(true),
                   m_set_poly_bytes_in_non_poly_regs_to_bottom(true),
                   m_enum_vconst_modifiers(true),
                   m_tmp_reg_count(max_num_temp_regs_for_enum),
                   m_tmp_stackslot_count(max_num_stack_slots_for_enum)
  {
  }
  use_types_t get_use_types() const { return m_use_types; }
  convert_addr_to_intaddr_t get_convert_addr_to_intaddr() const { return m_convert_addr_to_intaddr; }
  make_non_poly_copyable_t get_make_non_poly_copyable() const { return m_make_non_poly_copyable; }
  set_poly_bytes_in_non_poly_regs_to_bottom_t get_set_poly_bytes_in_non_poly_regs_to_bottom() const { return m_set_poly_bytes_in_non_poly_regs_to_bottom; }
  enum_vconst_modifiers_t get_enum_vconst_modifiers() const { return m_enum_vconst_modifiers; }
  tmp_reg_count_t get_tmp_reg_count() const { return m_tmp_reg_count; }
  tmp_reg_count_t get_tmp_reg_count_for_group(exreg_group_id_t g) const
  {
#if ARCH_DST == ARCH_I386 || ARCH_DST == ARCH_X64
    size_t count = (g == DST_EXREG_GROUP_GPRS) ? m_tmp_reg_count : (g >= I386_EXREG_GROUP_EFLAGS_OTHER && g <= I386_EXREG_GROUP_EFLAGS_SGE) ? 1 : 0;
#else
    NOT_IMPLEMENTED();
#endif
    return count;
  }
  vector<tmp_reg_count_t> get_tmp_reg_count_for_all_groups() const
  {
    vector<tmp_reg_count_t> ret;
    for (exreg_group_id_t g = 0; g < DST_NUM_EXREG_GROUPS; g++) {
      ret.push_back(get_tmp_reg_count_for_group(g));
    }
    return ret;
  }
  void set_tmp_reg_count_for_group(exreg_group_id_t g, tmp_reg_count_t r) { ASSERT(g == DST_EXREG_GROUP_GPRS); m_tmp_reg_count = r; }
  void set_tmp_reg_count(tmp_reg_count_t rcount[DST_NUM_EXREG_GROUPS]) { m_tmp_reg_count = rcount[DST_EXREG_GROUP_GPRS]; }
  tmp_stackslot_count_t get_tmp_stackslot_count() const { return m_tmp_stackslot_count; }
  string typesystem_to_string() const;
  void typesystem_from_string(string const &s);
  string typesystem_get_variation_string(size_t numchars = 0) const;
  bool typesystem_dominates(typesystem_t const &other) const;
  static bool typesystem_is_dominated(vector<typesystem_t> const &typesystem_hierarchy, set<typesystem_id_t> const &dominators, typesystem_id_t typesystem_id);
  static vector<typesystem_t> get_typesystem_hierarchy_for_enum();
  static vector<typesystem_t> get_typesystem_hierarchy_for_typecheck();
  static void typesystem_hierarchy_print_matrix_using_dominators(vector<typesystem_t> const &typesystem_hierarchy, set<typesystem_id_t> const &dominators);
};

#endif
