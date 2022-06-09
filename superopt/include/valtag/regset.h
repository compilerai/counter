#ifndef SRC_REGSET_H
#define SRC_REGSET_H
#include <stddef.h>
#include <stdint.h>
#include "support/src-defs.h"
//#include "i386/insntypes.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "ppc/regs.h"
#include "x64/insntypes.h"
#include "etfg/etfg_regs.h"
#include "support/src_tag.h"
#include "support/consts.h"
#include "support/types.h"
#include "valtag/reg_identifier.h"
#include <map>
using namespace std;

struct regcount_t;
struct nomatch_pair_t;
struct regmap_t;
struct regset_t;
struct memset_t;
struct transmap_t;
struct src_insn_t;
struct dst_insn_t;
struct valtag_t;
class fixed_reg_mapping_t;
class memslot_set_t;
class fingerprintdb_live_out_t;

typedef struct regset_t {
  //uint64_t excregs[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)];
  map<exreg_group_id_t, map<reg_identifier_t, uint64_t>> excregs;
  void regset_remove_zero_elems();
  void regset_round_up_bitmaps_to_byte_size();
  string to_string() const;
  bool operator!=(regset_t const &other) const;
  bool operator==(regset_t const &other) const;
} regset_t;

uint64_t regset_get_elem(regset_t const *regset, exreg_group_id_t group_id, reg_identifier_t reg_id);
void regset_clear(regset_t *regset);
void regset_clear_exreg_group(regset_t *regset, int group);
//void regset_mark_tag(regset_t *regset, src_tag_t tag, int reg);
//void regset_unmark_tag(regset_t *regset, src_tag_t tag, int reg);
//void regset_mark(regset_t *regset, int reg);
//void regset_unmark(regset_t *regset, int reg);
//void regset_mark_mask(regset_t *regset, int reg, uint64_t mask);
void regset_diff(regset_t *dst, regset_t const *src);
void regset_union(regset_t *dst, regset_t const *src);
void regset_union_arr(regset_t *dst, regset_t const *src_arr, size_t src_arr_len);

char *regset_to_string(regset_t const *regset, char *buf,
    size_t buf_size);
regset_t *regset_all(void);
regset_t *regset_empty(void);
bool regset_is_empty(regset_t const *regs);
regset_t const *src_volatile_regs(void);
void regset_copy(regset_t *dst, regset_t const *src);
void regsets_copy(regset_t *dst, regset_t const *src, size_t num_regsets);
bool regset_equal(regset_t const *regs1, regset_t const *regs2);
bool regset_arr_equal(regset_t const *regs1, regset_t const *regs2, size_t n);
//void regset_invert (regset_t *dst, regset_t const *src);
//void regset_invert_coarsely(regset_t *dst, regset_t const *src);
//bool regset_belongs(regset_t const *regset, int regno);
bool regset_belongs_ex(regset_t const *regset, exreg_group_id_t exgroupno, reg_identifier_t const &exregno);
bool regset_belongs_exvreg_bit(regset_t const *regset, exreg_group_id_t exgroupno,
    reg_identifier_t const &exregno, int bitnum);
//bool regset_belongs_vreg(regset_t const *regset, int regno);
//bool regset_belongs_exvreg(regset_t const *regset, int group, int regno);
//int regset_vregname_to_num(char *regname, src_tag_t *tag);
reg_identifier_t regset_exvregname_to_num(char *regname, int *group);
//bool regset_contains_mod_flexible_regs(regset_t const *haystack, regset_t const *needle, regset_t const &flexible_regs);
bool regset_contains(regset_t const *regs1, regset_t const *regs2);
bool regset_coarse_contains(regset_t const *regs1,
    regset_t const *regs2);
long regsets_from_string(regset_t *regsets, char const *str);
void regset_from_string(regset_t *regset, char const *str);
//void regset_init(regset_t *regset);
void regset_inv_rename(regset_t *out, regset_t const *in,
    struct regmap_t const *regmap);
void regset_rename_using_transmap(regset_t *out_regs, memslot_set_t *out_memslot_set,
    regset_t const *in, struct transmap_t const *tmap);
/*void regset_rename(regset_t *out, regset_t const *in,
    struct regmap_t const *regmap);*/
void regset_intersect(regset_t *dst, regset_t const *src);
void regset_intersect_exreg_group(regset_t *dst, regset_t const *src, exreg_group_id_t groupnum);
//int regset_count_regs(regset_t const *regset);
int regset_count_exregs(regset_t const *regset, exreg_group_id_t group);
void regset_count(regcount_t *regcount, regset_t const *regset);
long regset_count_bits(regset_t const *regset);
//int regset_count_vregs(regset_t const *regset);
//void regset_union_cregs_with_vregs(regset_t *regset);
size_t regset_to_int_array(regset_t const *regset, int64_t arr[], size_t len);
//void regset_mark_ex(regset_t *regset, int groupno, int regno);
//void regset_mark_exc(regset_t *regset, int groupno, int regno);
void regset_mark_ex_mask(regset_t *regset, exreg_group_id_t groupno, reg_identifier_t const &reg,
    uint64_t mask);
uint64_t regset_compute_bitmap(regset_t const *regset);
void regset_count_temporaries(regset_t const *regset, struct transmap_t const *tmap,
    struct regcount_t *touch_not_live_in);
void regset_unmark_ex(regset_t *regset, exreg_group_id_t groupno, reg_identifier_t const &regno);
void regset_mark_all(regset_t *regset);
//void regset_mark_vreg(regset_t *regset, int reg);
bool regset_equals_coarsely(regset_t const *regs1, regset_t const *regs2);
//int regset_count_vregs(regset_t const *regset);
char *ppc_regname(int regnum, char *buf, size_t buf_size);
size_t regsets_to_string(struct regset_t const *regset, long n, char *buf, size_t size);
void regset_subset_enumerate_first(regset_t *iter, regset_t const *set, fingerprintdb_live_out_t const *fingerprintdb_live_out = NULL);
bool regset_subset_enumerate_next(regset_t *iter, regset_t const *set, fingerprintdb_live_out_t const *fingerprintdb_live_out = NULL);
//void regsets_rename(struct regset_t *out, long num_out, struct regmap_t const *map);

void usedef_init(void);
void src_insn_get_usedef(struct src_insn_t const *src_insn, regset_t *use, regset_t *def,
    struct memset_t *memuse, struct memset_t *memdef);
void dst_insn_get_usedef(struct dst_insn_t const *dst_insn, regset_t *use, regset_t *def,
    struct memset_t *memuse, struct memset_t *memdef);
void src_insn_get_usedef_regs(struct src_insn_t const *src_insn, regset_t *use,
    regset_t *def);
void dst_insn_get_usedef_regs(struct dst_insn_t const *dst_insn, regset_t *use,
    regset_t *def);
void src_iseq_get_usedef(struct src_insn_t const *src_insn, int iseq_len,
    regset_t *use, regset_t *def, struct memset_t *memuse, struct memset_t *memdef);
void dst_iseq_get_usedef(struct dst_insn_t const *dst_insn, int iseq_len, regset_t *use,
    regset_t *def, struct memset_t *memuse, struct memset_t *memdef);
void src_iseq_get_usedef_regs(struct src_insn_t const *src_insn, int iseq_len,
    regset_t *use, regset_t *def);
void dst_iseq_get_usedef_regs(struct dst_insn_t const *dst_insn, int iseq_len,
    regset_t *use, regset_t *def);
void regset_live_for_retsize(struct regset_t *regs, int retsize);
int exvreg_to_string(int exvreg_group, int val, src_tag_t tag, int size,
    bool rex_used, uint64_t regmask,
    char *buf, int buflen);
size_t exvreg_from_string(struct valtag_t *vt, char const *buf);

//struct regset_t const *regset_exregs0_zero_to_five(void);
//struct regset_t const *regset_exregs0_zero_three_and_five(void);
struct regset_t const *src_regset_live_at_jumptable(void);
void regset_rename(regset_t *out, regset_t const *in, regmap_t const *regmap);
void regset_subtract_fixed_regs(regset_t *regset, fixed_reg_mapping_t const &fixed_reg_mapping);
exreg_id_t regset_pick_one(regset_t const *regset, exreg_group_id_t group);
void regset_subtract_mapped_regs_in_regmap(regset_t *regset, regmap_t const *regmap);
void regset_set_elem(regset_t *regset, exreg_group_id_t group, reg_identifier_t regnum, uint64_t bitmap);

#endif /* regset.h */
