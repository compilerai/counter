#ifndef FIXED_REG_MAPPING_H
#define FIXED_REG_MAPPING_H
#include "support/types.h"
#include <string>
#include <map>
class regcons_t;
class regset_t;
class regmap_t;

class fixed_reg_mapping_t
{
private:
  std::map<exreg_group_id_t, std::map<exreg_id_t, exreg_id_t>> m_map; //map from group -> (machine_regname -> var_regname)
public:
  void fixed_reg_mapping_from_string(std::string const &s);
  std::map<exreg_group_id_t, std::map<exreg_id_t, exreg_id_t>> const &get_map() const { return m_map; }
  std::string fixed_reg_mapping_to_string() const;
  void fixed_reg_mapping_to_stream(std::ostream& os) const;
  void fixed_reg_mapping_from_stream(std::istream& is);
  void add(exreg_group_id_t g, exreg_id_t machine_regname, exreg_id_t cur_regname);
  exreg_id_t get_mapping(exreg_group_id_t g, exreg_id_t r) const;
  bool var_reg_maps_to_fixed_reg(exreg_group_id_t g, exreg_id_t var_regnum) const;
  fixed_reg_mapping_t fixed_reg_mapping_rename_using_regmap(regmap_t const &regmap) const;
  void fixed_reg_mapping_intersect_machine_regs_with_regset(regset_t const &regset);
  bool fixed_reg_mapping_agrees_with(fixed_reg_mapping_t const &other) const;

  static fixed_reg_mapping_t fixed_reg_mapping_for_regs2(exreg_group_id_t group1, exreg_id_t machine_reg1, exreg_id_t reg1, exreg_group_id_t group2, exreg_id_t machine_reg2, exreg_id_t reg2);
  static fixed_reg_mapping_t default_fixed_reg_mapping_for_function_granular_eq();
  static fixed_reg_mapping_t default_dst_fixed_reg_mapping_for_fallback_translations();
  static long fscanf_fixed_reg_mappings(FILE *fp, fixed_reg_mapping_t &fixed_reg_mapping);
  static fixed_reg_mapping_t const &dst_fixed_reg_mapping_reserved_at_end();
  static fixed_reg_mapping_t const &dst_fixed_reg_mapping_reserved_identity();
  static fixed_reg_mapping_t const &dst_fixed_reg_mapping_reserved_at_end_var_to_var();
  static fixed_reg_mapping_t fixed_reg_mapping_from_regcons_touch_and_reserved_regs(regcons_t regcons, regset_t available_regs, regset_t const &touch, regset_t const &reserved_regs);
};

#endif
