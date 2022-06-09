#include <assert.h>
#include <stdio.h>
#include <bits/stdc++.h>
#include <iostream>
#include "support/debug.h"
#include "support/c_utils.h"
#include "valtag/regset.h"
#include "insn/live_ranges.h"

using namespace std;

static char as1[4096], as2[4096];

#define LIVE_RANGE_INFINITY (1UL<<25)
live_ranges_t const *
live_ranges_zero(void)
{
  static live_ranges_t _live_ranges_zero;
  static int init = 0;

  if (!init) {
    live_ranges_reset_all(&_live_ranges_zero);
    /*int i, j;
    for(i = 0; i < SRC_NUM_REGS; i++) {
      _live_ranges_zero.table[i] = 0;
    }
    for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
      for (j = 0; j < SRC_NUM_EXREGS(i); j++) {
        _live_ranges_zero.lr_extable[i][j] = 0;
      }
    }*/
    init = 1;
  }

  return &_live_ranges_zero;
}

void
live_ranges_copy(live_ranges_t *dst, live_ranges_t const *src)
{
  dst->lr_extable = src->lr_extable;
  //memcpy(dst, src, sizeof(live_ranges_t));
}

/*
void ppc_live_ranges_copy_if_less(ppc_live_ranges_t *dst,
    ppc_live_ranges_t const *src)
{
  int i;
  for (i = 0; i < SRC_NUM_REGS; i++)
  {
    if (dst->table[i] > src->table[i])
      dst->table[i] = src->table[i];
  }
}
*/

void
live_ranges_combine(live_ranges_t *dst,
    live_ranges_t const *src, double w_src)
{
  ASSERT(w_src <= 1.0);

  //first deal with all dst elems
  for (auto &gtable : dst->lr_extable) {
    for (auto &rtable : gtable.second) {
      int i;
      i = gtable.first;
      reg_identifier_t j = rtable.first;
      //cout << __func__ << " " << __LINE__ << ": i = " << i << ", j = " << j << ": elem = " << rtable.second << ", src_elem = " << live_ranges_get_elem(src, i, j) << endl;
      rtable.second = (int)(rtable.second + w_src*live_ranges_get_elem(src, i, j));
    }
  }

  //then deal with all src elems that are not available in dst elems
  for (auto &gtable : src->lr_extable) {
    for (auto &rtable : gtable.second) {
      int i;
      i = gtable.first;
      reg_identifier_t j = rtable.first;
      if (!dst->live_ranges_elem_exists(i, j)) {
        dst->lr_extable[i][j] = w_src * rtable.second;
      }
    }
  }

  /*int i, j;
  for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < SRC_NUM_EXREGS(i); j++) {
      dst->lr_extable[i][j] = (int)(live_ranges_get_elem(dst, i, j) + w_src*live_ranges_get_elem(src, i, j));
    }
  }*/
}

void
live_ranges_set(live_ranges_t *ranges, regset_t const *regs)
{
  //int i, j;
  for (const auto &g : regs->excregs) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t j = r.first;
      if (regset_belongs_ex(regs, i, j)) {
        ranges->lr_extable[i][j] = LIVE_RANGE_INFINITY;
      }
    }
  }
}

void
live_ranges_reset_all(live_ranges_t *ranges)
{
  ranges->lr_extable.clear();
}

void
live_ranges_reset(live_ranges_t *ranges, regset_t const *regs)
{
  for (const auto &g : regs->excregs) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t j = r.first;
      if (regset_belongs_ex(regs, i, j)) {
        ranges->lr_extable[i][j] = 0;
      }
    }
  }
}

void
live_ranges_decrement(live_ranges_t *ranges)
{
  //int i, j;
  for (const auto &g : ranges->lr_extable) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t j = r.first;
      if (ranges->lr_extable[i][j] - 1 < 0) {
        ranges->lr_extable[i][j] = 0;
      } else {
        ranges->lr_extable[i][j]--;
      }
      ASSERT(ranges->lr_extable[i][j] >= 0);
    }
  }
}

bool
live_ranges_equal(live_ranges_t const *r1, live_ranges_t const *r2)
{
  return r1->lr_extable == r2->lr_extable;
  /*int i, j;
  for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < SRC_NUM_EXREGS(i); j++) {
      if (live_ranges_get_elem(r1, i, j) != live_ranges_get_elem(r2, i, j)) {
        return false;
      }
    }
  }
  return true;*/
}

/*long
live_range(live_ranges_t const *ranges, int regno)
{
  NOT_REACHED();
  //return ranges->table[regno];
}*/

long
live_range_ex(live_ranges_t const *ranges, int group, int regno)
{
  ASSERT(group >= 0 && group < SRC_NUM_EXREG_GROUPS);
  ASSERT(regno >= 0 && regno < SRC_NUM_EXREGS(group));
  return live_ranges_get_elem(ranges, group, regno);
}
/*
typedef struct {
  int group;
  int regno;
  long live_range;
} live_range_sort_struct_t;

static int
live_range_sort_struct_compare(const void *_a, const void *_b)
{
  live_range_sort_struct_t const *a = (live_range_sort_struct_t *)_a;
  live_range_sort_struct_t const *b = (live_range_sort_struct_t *)_b;

  // sort in increasing order
  return a->live_range - b->live_range;
}

void
src_regs_sort_based_on_live_ranges(int *sorted_regs, long const *live_ranges,
    int num_regs)
{
  live_range_sort_struct_t live_range_sort_structs[num_regs];
  int i;
  
  for (i = 0; i < num_regs; i++) {
    live_range_sort_structs[i].group = -1;
    live_range_sort_structs[i].regno = i;
    live_range_sort_structs[i].live_range = live_ranges[i];
  }

  qsort(live_range_sort_structs, num_regs, sizeof(live_range_sort_struct_t),
      live_range_sort_struct_compare);
  for (i = 0; i < num_regs; i++) {
    sorted_regs[i] = live_range_sort_structs[i].regno;
  }
}

static int
live_range_sort_struct_compare(const void *_a, const void *_b)
{
  live_range_sort_struct_t const *a, *b;

  a = (live_range_sort_struct_t *)_a;
  b = (live_range_sort_struct_t *)_b;

  //sort in increasing order
  return a->live_range - b->live_range;
}*/

static bool
live_range_sort_struct_compare(live_range_sort_struct_t const &a, live_range_sort_struct_t const &b)
{
  //sort in increasing order
  return a.live_range < b.live_range;
}



int
src_regs_sort_based_on_live_ranges(live_range_sort_struct_t *sorted_regs,
    live_ranges_t const *live_ranges)
{
  int /*i, j, */n;

  n = 0;
  for (const auto &g : live_ranges->lr_extable) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t const &j = r.first;
      sorted_regs[n].group = i;
      sorted_regs[n].regnum = j;;
      sorted_regs[n].live_range = live_ranges_get_elem(live_ranges, i, j);
      n++;
    }
  }
  /*qsort(sorted_regs, n, sizeof(live_range_sort_struct_t),
      &live_range_sort_struct_compare);*/
  sort(sorted_regs, sorted_regs + n, live_range_sort_struct_compare);
  return n;
}

char *
live_ranges_to_string(live_ranges_t const *lranges,
    regset_t const *lregs, char *buf, size_t buf_size)
{
  int /*i, j, */n;
  buf[0] = '\0';

  /*if (!lregs) {
    lregs = regset_all();
  }*/

  live_range_sort_struct_t live_range_sort_structs[lranges->get_num_elems()];

  n = 0;

  for (const auto &g : lranges->lr_extable) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t j = r.first;
      live_range_sort_structs[n].group = i;
      live_range_sort_structs[n].regnum = j;
      if (lregs && !regset_belongs_ex(lregs, i, j)) {
        ASSERT(live_ranges_get_elem(lranges, i, j) == 0);
        live_range_sort_structs[n].live_range = -1;
      } else {
        live_range_sort_structs[n].live_range = live_ranges_get_elem(lranges, i, j);
      }
      n++;
    }
  }

  /*qsort(live_range_sort_structs, n, sizeof(live_range_sort_struct_t),
      live_range_sort_struct_compare);*/
  sort(live_range_sort_structs, live_range_sort_structs + n, live_range_sort_struct_compare);


  char *ptr = buf, *end = buf + buf_size;
  char chr=' ';

  for (int i = 0; i < n; i++) {
    if (live_range_sort_structs[i].live_range < 0) {
      continue;
    }
    int g = live_range_sort_structs[i].group;
    reg_identifier_t r = live_range_sort_structs[i].regnum;

    /*if (g == -1) {
      ptr += snprintf(ptr, end-ptr, "%cr%d(%ld)", chr, r, lranges->table[r]);
    } else */{
      ptr += snprintf(ptr, end-ptr, "%cexr%d.%s(%ld)", chr, g, r.reg_identifier_to_string().c_str(), live_ranges_get_elem(lranges, g, r));
    }
    chr=',';
  }
  return buf;
}

#if 0
#define SUMMARY(x) (((x)==0)?"(Z)":"")
char *live_ranges_to_string(live_ranges_t const *lranges,
    regset_t const *lregs, char *buf, int buf_size)
{
  int i;

  if (!lranges) {
    return "(nil)";
  }

  if (!lregs)
    lregs = regset_all();

  live_range_sort_struct_t live_range_sort_structs[SRC_NUM_REGS];

  for (i = 0; i < SRC_NUM_REGS; i++) {
    if (!regset_belongs(lregs, i)) {
      live_range_sort_structs[i].regno = i;
      live_range_sort_structs[i].live_range = -1;
      if (lranges->table[i] != 0) {
        err ("mismatching lregs and lranges:\nlregs: %s\nlranges[%d]=%d\n",
            regset_to_string(lregs,as1,sizeof as1),
            i, lranges->table[i]);
        //assert (0);
        live_range_sort_structs[i].live_range = lranges->table[i];
      }
    } else {
      live_range_sort_structs[i].regno = i;
      live_range_sort_structs[i].live_range = lranges->table[i];
    }
  }

  qsort(live_range_sort_structs, SRC_NUM_REGS, sizeof(live_range_sort_struct_t),
      live_range_sort_struct_compare);

  char *ptr = buf, *end = buf + buf_size;
  char chr=' ';

  for (i = 0; i < SRC_NUM_REGS; i++) {
    if (live_range_sort_structs[i].live_range < 0)
      break;
    int r = live_range_sort_structs[i].regno;

    if (r < SRC_NUM_GPRS) {
      ptr += snprintf(ptr, end-ptr, "%c%d%s", chr, r,
          SUMMARY(lranges->table[r]));
      chr=',';
    }

#if ARCH_SRC == ARCH_PPC
    switch (r) {
      case PPC_REG_LR:
        ptr += snprintf(ptr, end - ptr, ",lr%s", SUMMARY(lranges->table[r]));
        break;
      case PPC_REG_CTR:
        snprintf(ptr, end - ptr, ",ctr%s", SUMMARY(lranges->table[r]));
        break;
      case PPC_REG_XER_SO:
        ptr+=snprintf(ptr, end - ptr, ",xer_so%s", SUMMARY(lranges->table[r]));
        break;
      case PPC_REG_XER_OV:
        ptr += snprintf(ptr, end - ptr, ",xer_ov%s",SUMMARY(lranges->table[r]));
        break;
      case PPC_REG_XER_CA:
        ptr += snprintf(ptr, end - ptr, ",xer_ca%s",SUMMARY(lranges->table[r]));
        break;
      case PPC_REG_XER_BC:
        ptr += snprintf(ptr,end - ptr,",xer_bc%s",SUMMARY(lranges->table[r]));
        break;
      case PPC_REG_XER_CMP:
        ptr+=snprintf(ptr,end - ptr,",xer_cmp%s",SUMMARY(lranges->table[r]));
        break;
      default:
        break;
    }

    if (r >= PPC_REG_CRFN(0) && r < PPC_REG_CRFN(8))
      ptr += snprintf (ptr, end - ptr, ",crf%d%s", r - PPC_REG_CRFN(0),
          SUMMARY(lranges->table[r]));
#endif
  }
  return buf;
}
#endif

long
live_ranges_get_elem(live_ranges_t const *lr, exreg_group_id_t group, reg_identifier_t regnum)
{
  if (lr->lr_extable.count(group) == 0) {
    return 0;
  }
  if (lr->lr_extable.at(group).count(regnum) == 0) {
    return 0;
  }
  ASSERT(lr->lr_extable.count(group));
  ASSERT(lr->lr_extable.at(group).count(regnum));
  return lr->lr_extable.at(group).at(regnum);
}
