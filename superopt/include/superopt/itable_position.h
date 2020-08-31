#ifndef ITABLE_POSITION_H
#define ITABLE_POSITION_H

#include "support/consts.h"

struct itable_entry_t;
typedef struct itable_position_t {
  long sequence[ENUM_MAX_LEN];
  long sequence_len;

  long exreg_sequence[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN][MAX_REGS_PER_ITABLE_ENTRY];
  long exreg_sequence_len[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN];
  int32_t vconst_sequence[ENUM_MAX_LEN][MAX_VCONSTS_PER_ITABLE_ENTRY];
  long vconst_sequence_len[ENUM_MAX_LEN];
  long memloc_sequence[ENUM_MAX_LEN][MAX_MEMLOCS_PER_ITABLE_ENTRY];
  long memloc_sequence_len[ENUM_MAX_LEN];
  long nextpc_sequence[ENUM_MAX_LEN][MAX_NEXTPCS_PER_ITABLE_ENTRY];
  long nextpc_sequence_len[ENUM_MAX_LEN];
} itable_position_t;

itable_position_t const *itable_position_zero(void);
void itable_position_min(itable_position_t *a, itable_position_t const *b);
bool itable_positions_equal(itable_position_t const *a, itable_position_t const *b);
void itable_position_copy(itable_position_t *dst, itable_position_t const *src);
void itable_position_from_string(itable_position_t *ipos, char const *string);
char *itable_position_to_string(itable_position_t const *ipos, char *buf, size_t size);
int itable_positions_compare(itable_position_t const *a, itable_position_t const *b);
//void itable_prune_insns_using_loc(itable_t *itab, int itab_prune_loc, int max_used);

char *itable_position_to_string(itable_position_t const *ipos, char *buf, size_t size);
void itable_position_from_string(itable_position_t *ipos, char const *string);
long itable_position_next(itable_position_t *ipos, itable_t const *itable,
    long long const *max_costs, long cur);
itable_position_t itable_position_from_stream(FILE *fp);

#define itable_position_lim(ipos, arr, lenarr, cur, _i, _lim) ({ \
  long __i, __j; \
  long ret = -1; \
  for (__i = 0; __i < cur; __i++) { \
    for (__j = 0; __j < ipos->lenarr[__i]; __j++) { \
      ret = max(ret, (long)ipos->arr[__i][__j]); \
    } \
  } \
  for (__j = 0; __j < _i; __j++) { \
    ret = max(ret, (long)ipos->arr[cur][__j]); \
  } \
  min(ret + 2, (long)_lim); \
})

#define IPOS_RESET(ipos, itable, arr, lenarr, cur) do { \
  long _i; \
  ASSERT(itable->get_num_entries() > 0); \
  ASSERT(ipos->sequence[cur] >= 0 && ipos->sequence[cur] < itable->get_num_entries()); \
  ipos->lenarr[cur] = itable->get_entry(ipos->sequence[cur]).lenarr; \
  for (_i = 0; _i < ipos->lenarr[cur]; _i++) { \
    ipos->arr[cur][_i] = 0; \
  } \
} while(0)

#define IPOS_NEXT(ipos, arr, lenarr, cur, _lim, use_types) do { \
  long _i, _size; \
  _size = ipos->lenarr[cur]; \
  for (_i = _size - 1; _i >= 0; _i--) { \
    /*long eff_lim = use_types?_lim:itable_position_lim(ipos, arr, lenarr, cur, _i, _lim);*/\
    long eff_lim = _lim; \
    if (ipos->arr[cur][_i] + 1 < eff_lim) { \
      ipos->arr[cur][_i]++; \
      return cur; \
    } \
    ipos->arr[cur][_i] = 0; \
  } \
} while(0)



#endif
