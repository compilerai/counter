#ifndef LIVE_RANGES_H
#define LIVE_RANGES_H

#include <stdlib.h>
#include <map>
#include "valtag/reg_identifier.h"
#include "support/types.h"

typedef struct live_ranges_t
{
  //long lr_extable[SRC_NUM_EXREG_GROUPS][SRC_NUM_EXREGS(0)];
  std::map<exreg_group_id_t, std::map<reg_identifier_t, long>> lr_extable;
  size_t get_num_elems() const
  {
    size_t n = 0;
    for (const auto &gtable : lr_extable) {
      n += gtable.second.size();
    }
    return n;
  }
  bool live_ranges_elem_exists(exreg_group_id_t group, reg_identifier_t regnum) const
  {
    if (lr_extable.count(group) == 0) {
      return false;
    }
    return lr_extable.at(group).count(regnum) != 0;
  }
} live_ranges_t;

typedef struct live_range_sort_struct_t {
  int group;
  reg_identifier_t regnum;
  long live_range;

  live_range_sort_struct_t() : group(-1), regnum(reg_identifier_t::reg_identifier_invalid()), live_range(0)
  {
  }
} live_range_sort_struct_t;

long live_ranges_get_elem(live_ranges_t const *lr, exreg_group_id_t group, reg_identifier_t regnum);
live_ranges_t const *live_ranges_zero(void);
void live_ranges_copy(live_ranges_t *dst, live_ranges_t const *src);
void live_ranges_combine(live_ranges_t *dst,
    live_ranges_t const *src, double w_dst);

void live_ranges_set(live_ranges_t *ranges, struct regset_t const *regs);
void live_ranges_reset(live_ranges_t *ranges, struct regset_t const *regs);
void live_ranges_reset_all(live_ranges_t *ranges);
void live_ranges_decrement(live_ranges_t *ranges);

bool live_ranges_equal(live_ranges_t const *r1,
    live_ranges_t const *r2);

long live_range(live_ranges_t const *ranges, int regno);
long live_range_ex(live_ranges_t const *ranges, int group, int regno);

int
src_regs_sort_based_on_live_ranges(live_range_sort_struct_t *sorted_regs,
    live_ranges_t const *live_ranges);

char *live_ranges_to_string(live_ranges_t const *lranges,
    struct regset_t const *lregs, char *buf, size_t buf_size);

char *live_ranges_to_string_full(live_ranges_t const *lranges,
    struct regset_t const *lregs, char *buf, size_t buf_size);
#endif
