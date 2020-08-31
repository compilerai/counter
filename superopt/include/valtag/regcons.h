#ifndef I386_REGCONS_H
#define I386_REGCONS_H

#include <stdint.h>
#include "ppc/insn.h"
#include "i386/insntypes.h"
//#include "temporaries.h"
#include "valtag/regset.h"
#include "support/src-defs.h"
#include "valtag/fixed_reg_mapping.h"

struct regmap_t;
struct temporaries_t;

typedef struct regcons_entry_t {
  regcons_entry_t()
  {
    //bitmap = UINT32_MAX;
    bitmap = 0;
  }
  regcons_entry_t(uint32_t b) : bitmap(b) {}
  bool operator==(regcons_entry_t const &other) const
  {
    return this->bitmap == other.bitmap;
  }

  void regcons_entry_set(exreg_group_id_t group, int num_regs, fixed_reg_mapping_t const &fixed_reg_mapping = fixed_reg_mapping_t());
  void clear() { bitmap = 0; }
  bool regcons_entry_allows_single_reg(int *regnum) const
  {
    for (size_t i = 0; i < sizeof(bitmap)*BYTE_LEN; i++) {
      if (bitmap == (1 << i)) {
        *regnum = i;
        return true;
      }
    }
    return false;
  }
  static regcons_entry_t regcons_entry_for_single_reg(int regnum)
  {
    regcons_entry_t ret;
    ASSERT(regnum >= 0 && regnum < sizeof(bitmap)*BYTE_LEN);
    ret.bitmap = 1 << regnum;
    return ret;
  }
  uint32_t bitmap;
} regcons_entry_t;

typedef struct regcons_t {
  //regcons_entry_t regcons_extable[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)];
  map<exreg_group_id_t, map<reg_identifier_t, regcons_entry_t>> regcons_extable;
} regcons_t;

uint32_t regcons_get_bitmap(regcons_t const *regcons, exreg_group_id_t group, reg_identifier_t const &regnum);
//regcons_entry_t *regcons_get_entry(regcons_t *regcons, exreg_group_id_t group, exreg_id_t regnum);
//regcons_entry_t const *regcons_get_entry(regcons_t const *regcons, exreg_group_id_t group, exreg_id_t regnum);
//void regcons_clear(regcons_t *regcons);
void regcons_set(regcons_t *regcons);

/*void regcons_set_src(regcons_t *regcons);
void regcons_set_i386(regcons_t *regcons);
void regcons_set_ppc(regcons_t *regcons);*/

void regcons_clear_vreg(regcons_t *regcons, int vreg, src_tag_t tag);
//void regcons_set_vreg(regcons_t *regcons, int vreg, src_tag_t tag);
void regcons_clear_exreg(regcons_t *regcons, int group, int vreg);
//void regcons_set_exreg(regcons_t *regcons, int group, int vreg, src_tag_t tag);

void regcons_mark(regcons_t *regcons, int src_regnum, src_tag_t tag,
    int i386_regnum);
void regcons_unmark(regcons_t *regcons, int src_regnum, src_tag_t tag,
    int i386_regnum);

void regcons_mark_ex(regcons_t *regcons, int groupnum, int src_regnum,
    src_tag_t tag, int x86_regnum);
void regcons_unmark_ex(regcons_t *regcons, int groupnum, int src_regnum,
    src_tag_t tag, int x86_regnum);

/*void regcons_reg_clear(regcons_t *regcons, int src_regnum);
void regcons_exreg_clear(regcons_t *regcons, int groupnum,
    int src_regnum);*/

bool regcons_vreg_is_set(regcons_t const *regcons, int vregnum, src_tag_t tag);
bool regcons_exreg_is_set(regcons_t const *regcons, int groupnum,
    int exregnum, src_tag_t tag);

void regcons_copy(regcons_t *dst, regcons_t const *src);
//void regcons_entry_copy(regcons_entry_t *dst, regcons_entry_t const *src);
bool regcons_equal(regcons_t const *c1, regcons_t const *c2);

char *regcons_to_string(regcons_t const *regcons, char *buf, size_t size);
void regcons_from_string(regcons_t *regcons, char const *buf);
/*int regcons_has_single_reg_constraint(regcons_t const *regcons, int i386_reg,
    src_tag_t *tag);*/
/*char *regcons_entrynum_to_string(regcons_t const *tcons, int src_reg, char *buf,
    int buf_size);*/
char *regcons_entry_to_string(regcons_entry_t const *tcons, char *buf,
    int buf_size);
void fprintf_src_regcons(FILE *logfile, regcons_t const *tcons);

void transmaps_acquiesce_with_dst_regcons(struct transmap_t *dst_in,
    struct transmap_t *dst_outs, regcons_t const *dst_in_cons,
    regcons_t const *dst_out_cons, long num_outs);
/*bool transmap_temporaries_assign_using_src_regcons(struct transmap_t *transmap,
    struct temporaries_t *temporaries, struct regcons_t const *transcons,
    struct regcons_entry_t const *available);*/
/*void temporaries_acquiesce_with_src_regcons(temporaries_t *temps,
    struct transmap_t const *dst_in, struct regcons_t const *tcons);*/
int regcons_allows_all(regcons_t const *tcons, int src_reg);
//bool regcons_all_is_set(regcons_t const *tcons);
bool regcons_agree(struct transmap_t const *in_tmap, regcons_t const *in_tcons,
    struct transmap_t const *out_tmap, regcons_t const *out_tcons);
/*void regcons_rename(regcons_t *out_tcons, regcons_t const *in_tcons,
    struct regmap_t const *in_regmap);*/
//int regcons_entry_pick_reg(regcons_entry_t const *tcons_entry);

//regcons_t const *regcons_all_set(void);
//regcons_entry_t const *regcons_available_regs(void);
//regcons_entry_t const *regcons_all_regs(void);

bool regcons_agree(struct transmap_t const *in_tmap,
    struct regcons_t const *in_tcons,
    struct transmap_t const *out_tmap, struct regcons_t const *out_tcons);
bool regcons_allows_extemporary(struct regcons_t const *tcons, int group,
    int reg, int x86_reg);
bool vreg_add_regcons_one_reg(regcons_t *regcons, char const *start,
    char const *end, int igroup, int iregnum);
bool vreg_remove_regcons_one_reg(regcons_t *regcons, char const *start,
    char const *end, int igroup, int iregnum);
void regcons_unmark_using_regset(regcons_t *regcons, regset_t const *regset);
void regcons_exreg_mark(regcons_t *regcons, int group, int vreg,
    int dstreg);
void regcons_exreg_unmark(regcons_t *regcons, int group, int vreg,
    int dstreg);
void regcons_rename(regcons_t *out_tcons, regcons_t const *peep_tcons,
    regmap_t const *regmap);
void regcons_copy_using_transmaps_and_src_regmap(regcons_t *dst, regcons_t const *src,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps, long num_out_tmaps,
    struct regmap_t const *src_regmap);
void regcons_intersect(regcons_t *dst, regcons_t const *src);
//void regcons_entry_intersect(regcons_entry_t *entry1, regcons_entry_t const *entry2);
bool regcons_entry_intersect(regcons_t *dst, regcons_t const *src, exreg_group_id_t group, reg_identifier_t const &reg_dst, reg_identifier_t const &reg_src);

bool regmap_satisfies_regcons(regmap_t const *regmap, struct regcons_t const *regcons);
void regcons_set_i386(struct regcons_t  *regcons, fixed_reg_mapping_t const &fixed_reg_mapping);
void regcons_set_ppc(struct regcons_t  *regcons, fixed_reg_mapping_t const &fixed_reg_mapping);
void regcons_unmark_dst_reg(regcons_t *regcons, int group, int dst_reg);
//void regcons_minimize(struct regcons_t  *regcons);
//void regcons_avoid_dst_reg(regcons_t *regcons, int group, int dst_reg);
regset_t regcons_identify_regs_that_must_be_mapped_to_reserved_regs(regcons_t const *regcons, regset_t const *reserved_regs);
void regcons_update_using_fixed_reg_mapping(regcons_t *regcons, fixed_reg_mapping_t const &fixed_reg_mapping);
exreg_id_t regcons_requires_single_reg_mapping_to_exreg(regcons_t const *regcons, exreg_group_id_t group, reg_identifier_t const &r);
void regcons_unmark_dst_reg_for_regset(regcons_t *regcons, regset_t const &regset, exreg_group_id_t group, exreg_id_t dst_reg);

#endif /* regcons.h */
