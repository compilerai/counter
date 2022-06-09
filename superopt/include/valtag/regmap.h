#ifndef __REGMAP_H
#define __REGMAP_H

#include <stdint.h>
#include "support/src-defs.h"
//#include "temporaries.h"
#include "valtag/regset.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "ppc/regs.h"
#include "etfg/etfg_regs.h"
#include "valtag/reg_identifier.h"

struct regset_t;
struct regcons_t;
typedef struct regmap_t {
  //int regmap_extable[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)];
  map<exreg_group_id_t, map<reg_identifier_t, reg_identifier_t>> regmap_extable;

  bool regmap_agrees_with_fixed_reg_mapping(fixed_reg_mapping_t const &fixed_reg_mapping) const;
  void regmap_add_fixed_reg_mapping(fixed_reg_mapping_t const &fixed_reg_mapping);
  void regmap_subtract_fixed_reg_mapping(fixed_reg_mapping_t const &fixed_reg_mapping);
  vector<exreg_id_t> regmap_canonicalize_dst_regnames_for_group(exreg_group_id_t g);
  void regmap_uncanonicalize_dst_regnames_for_group(exreg_group_id_t g, vector<exreg_id_t> const &v);
  exreg_id_t regmap_get_max_mapped_value(exreg_group_id_t g) const;
  bool mapping_exists(exreg_group_id_t g, reg_identifier_t const &r) const
  {
    return (regmap_extable.count(g) != 0) && (regmap_extable.at(g).count(r) != 0);
  }
  void add_mapping(exreg_group_id_t g, reg_identifier_t const &from, reg_identifier_t const &to)
  {
    if (regmap_extable[g].count(from)) {
      regmap_extable[g].erase(from);
    }
    regmap_extable[g].insert(make_pair(from, to));
  }
  void remove_mapping(exreg_group_id_t g, reg_identifier_t const &r)
  {
    if (!regmap_extable.count(g)) {
      return;
    }
    regmap_extable.at(g).erase(r);
  }
  void regmap_from_string(string const& regmap_str);
} regmap_t;

bool regmap_contains_mapping(regmap_t const *regmap, exreg_group_id_t group, reg_identifier_t const &regnum);
reg_identifier_t const &regmap_get_elem(regmap_t const *regmap, exreg_group_id_t group, reg_identifier_t const &regnum);
void regmap_set_elem(regmap_t *regmap, exreg_group_id_t group, reg_identifier_t const &regnum, reg_identifier_t const &mapped_regnum);
char *regmap_to_string(regmap_t const *regmap, char *buf,
    size_t size);
size_t dst_regmap_from_string(regmap_t *regmap, char const *buf);
void dst_regmap_from_file(regmap_t *regmap, char const *filename);
void regmap_init(regmap_t *regmap);
regmap_t const *i386_identity_regmap();
regmap_t const *x64_identity_regmap();
void regmap_identity_for_i386(regmap_t *regmap, fixed_reg_mapping_t const &fixed_reg_mapping);
void regmap_identity_for_x64(regmap_t *regmap, fixed_reg_mapping_t const &fixed_reg_mapping);
void regmap_identity_for_ppc(regmap_t *regmap, fixed_reg_mapping_t const &fixed_reg_mapping);
regmap_t const *src_exec_regmap(void);
//long regmap_count_gprs(regmap_t const *regmap);
long regmap_count_exgroup_regs(regmap_t const *regmap, int groupnum);
bool regmap_change_val(regmap_t *dst, regmap_t const  *src,
    long regnum, long val);
bool i386_regmap_change_exval(regmap_t *dst, regmap_t const  *src,
    long group, long regnum, long val);
void regmap_copy(regmap_t *dst, regmap_t const *src);
void regmap_mark_ex_id(regmap_t *regmap, int group, int regnum);

pair<bool, reg_identifier_t> regmap_inv_rename_exreg_try(struct regmap_t const *regmap, int group, reg_identifier_t const &regnum);
reg_identifier_t regmap_inv_rename_exreg(struct regmap_t const *regmap, int group, reg_identifier_t const &regnum);
//int regmap_inv_rename_reg(struct regmap_t const *regmap, int ireg, int size);

//int regmap_rename_reg(struct regmap_t const *regmap, int regnum);
reg_identifier_t const &regmap_rename_exreg(struct regmap_t const *regmap, int group, reg_identifier_t const &regnum);

bool regmap_assign_using_regcons(struct regmap_t *regmap,
    struct regcons_t const *regcons, int arch,
    regmap_t const *preferred_regmap,
    fixed_reg_mapping_t const &fixed_reg_mapping);
void regmaps_diff(regmap_t *dst, regmap_t const *src1, regmap_t const *src2);
void regmap_invert(struct regmap_t *inverted, struct regmap_t const *orig);

bool regmap_is_identity(regmap_t const *regmap);
void regmap_canonical_for_regset(struct regmap_t *regmap, struct regset_t const *regset);

bool regmap_permutation_enumerate_first(struct regmap_t *iter,
    struct regmap_t const *regmap);
bool regmap_permutation_enumerate_next(struct regmap_t *iter,
    struct regmap_t const *regmap);

bool regmap_group_permutation_enumerate_first(struct regmap_t *iter,
    struct regmap_t const *regmap, int groupnum, fixed_reg_mapping_t const &fixed_reg_mapping);
bool regmap_group_permutation_enumerate_next(struct regmap_t *iter,
    struct regmap_t const *regmap, int groupnum, fixed_reg_mapping_t const &fixed_reg_mapping);


void regmap_get_used_dst_regs(struct regset_t *available, struct regmap_t const *regmap);
bool regmaps_equal(regmap_t const *a, regmap_t const *b);
bool regmaps_equal_mod_group(regmap_t const *a, regmap_t const *b, exreg_group_id_t group);
void regmap_intersect_with_regset(regmap_t *regmap, regset_t const *regset);
void dst_regmap_add_identity_mappings_for_unmapped(regmap_t *regmap);
unsigned regmap_hash(regmap_t const *regmap);
unsigned regmap_hash_mod_group(regmap_t const *regmap, exreg_group_id_t group);
void regmaps_combine(regmap_t *dst, regmap_t const *src);

void dst_regmap_add_mappings_for_regset(regmap_t *regmap, regset_t const *temps);
void dst_regmap_remove_mappings_for_regset(regmap_t *regmap, regset_t const *temps);
//bool regmap_maps_dst_reserved_register_to_this_reg(regmap_t const *regmap, exreg_group_id_t group, exreg_id_t this_reg);

#define ST_CANON_EXREG(exreg, exreg_group, num_used, st_map, fixed_reg_mapping) ({ \
  /*cout << __func__ << " " << __LINE__ << ": 0\n"; std::cout.flush(); */\
  ASSERT(exreg.reg_identifier_is_valid()); \
  /*cout << __func__ << " " << __LINE__ << ": 1\n"; std::cout.flush(); */\
  reg_identifier_t ret = exreg; \
  exreg_id_t freg = -1; \
  if (ret.reg_identifier_has_id()) { \
    freg = fixed_reg_mapping.get_mapping(exreg_group, exreg.reg_identifier_get_id()); \
  } \
  /*cout << __func__ << " " << __LINE__ << ": ret = " << ret.reg_identifier_to_string() << endl; \
  cout << __func__ << " " << __LINE__ << ": freg = " << freg << endl; */\
  if (freg == -1) { \
    if (regmap_get_elem((st_map), exreg_group, exreg).reg_identifier_is_unassigned()) { \
      /*cout << __func__ << " " << __LINE__ << ": 2\n"; std::cout.flush(); */\
      int new_reg = num_used[exreg_group]; \
      /*cout << __func__ << " " << __LINE__ << ": 2.1\n"; std::cout.flush(); */\
      num_used[exreg_group]++; \
      /*cout << __func__ << " " << __LINE__ << ": 2.2\n"; std::cout.flush(); */\
      reg_identifier_t new_reg_id(new_reg); \
      /*cout << __func__ << " " << __LINE__ << ": 2.2\n"; std::cout.flush(); */\
      regmap_set_elem((st_map), exreg_group, exreg, new_reg_id); \
      /*cout << __func__ << " " << __LINE__ << ": 3\n"; std::cout.flush(); */\
    } \
    /*cout << __func__ << " " << __LINE__ << ": 4\n"; std::cout.flush(); */\
    ret = regmap_get_elem((st_map), exreg_group, exreg); \
    /*cout << __func__ << " " << __LINE__ << ": 5\n"; std::cout.flush(); */\
  } else { \
    ret = freg; \
  } \
  ret; \
})

/*void regmap_from_transmap_and_i386_iseq(regmap_t *regmap, i386_insn_t const *i386_iseq,
    long i386_iseq_len, transmap_t const *tmap);*/
#endif
