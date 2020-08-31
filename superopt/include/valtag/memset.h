#ifndef __PPC_MEMSET_H
#define __PPC_MEMSET_H
#include "superopt/typestate.h"
#include <set>

/*
typedef struct memset_entry_t {
  int reg1, reg2;
  uint64_t disp;
  int scale;

  // tags. true=tag_var, false=tag_const.
  bool treg1, treg2;
  bool tdisp;
} memset_entry_t;
*/

typedef struct memset_t
{
  //int num_memaccesses;
  //state_elem_desc_t *memaccess;
  //int memaccess_size;
  std::set<state_elem_desc_t> memaccess;
  bool memset_is_empty() const { return memaccess.size() == 0; }
} memset_t;

struct regset_t;
struct regmap_t;
struct imm_vt_map_t;

void memset_init(memset_t *memset);
void memset_free(memset_t *memset);

void memset_clear(memset_t *memset);
void memset_copy(memset_t *dst, memset_t const *src);
void memset_intersect(memset_t *dst, memset_t const *src);
void memset_union(memset_t *dst, memset_t const *src);
bool memset_equals(memset_t const *ms1, memset_t const *ms2);
//void memset_get_used_regs(struct regset_t *regs, memset_t const *memset);
void memset_add(memset_t *memset, state_elem_desc_t const *new_elem);

memset_t const *memset_empty(void);
/*char *memset_entry_to_x86_assembly(memset_entry_t const *memset_entry, char *buf,
    int size);*/
char *memset_to_string(memset_t const *memset, char *buf, size_t size);
void memset_from_string(memset_t *memset, char const *buf);
void memset_rename(memset_t *dst, memset_t const *src, struct regmap_t const *inv_regmap,
    struct imm_vt_map_t const *inv_imm_vt_map);
void memset_inv_rename(memset_t *dst, memset_t const *src, struct regmap_t const *regmap);
bool memset_contains(memset_t const *haystack, memset_t const *needle);
void memset_subtract_accesses_relative_to_fixed_regs_and_symbols(memset_t *memset, fixed_reg_mapping_t const &fixed_reg_mapping, exreg_group_id_t gpr_group);

#endif
