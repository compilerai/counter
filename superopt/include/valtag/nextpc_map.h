#ifndef __NEXTPC_MAP_H
#define __NEXTPC_MAP_H

#include <stdint.h>
#include "valtag/valtag.h"
#include "support/consts.h"

struct symbol_set_t;

typedef struct nextpc_map_t {
  int num_nextpcs_used;
  //int table[MAX_NUM_NEXTPCS];
  valtag_t table[MAX_NUM_NEXTPCS];
  bool maps_last_nextpc_identically() const
  {
    ASSERT(num_nextpcs_used >= 0);
    if (num_nextpcs_used == 0) {
      return true;
    }
    return    table[num_nextpcs_used - 1].val == num_nextpcs_used - 1
           && table[num_nextpcs_used - 1].tag == tag_var;
  }
} nextpc_map_t;

void nextpc_map_init(nextpc_map_t *nextpc_map);
void nextpc_map_from_short_string_using_symbols(nextpc_map_t *nextpc_map1, char const *nextpcs1_string,
    struct symbol_set_t const *symbol_set);
bool nextpc_maps_join(nextpc_map_t *out, nextpc_map_t const *in1,
    struct symbol_set_t const *set1, nextpc_map_t const *in2,
    struct symbol_set_t const *set2);
void nextpc_map_copy(nextpc_map_t *dst, nextpc_map_t const *src);
char *nextpc_map_to_string(nextpc_map_t const *nextpc_map, char *buf, size_t size);
void nextpc_map_set_identity_var(nextpc_map_t *nextpc_map, size_t num_nextpcs);
void nextpc_map_rename_pcrel(valtag_t &pcrel, nextpc_map_t const *nextpc_map);

#define MATCH_NEXTPC_VALTAG(vt, tvt, nextpc_map, var) do { \
  if (tvt.tag == tag_const) { \
    ASSERT(vt.tag == tag_const || vt.tag == tag_var); \
    if (vt.tag == tag_const) { \
      if (tvt.val != vt.val) { \
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": tvt.tag == vt.tag == tag_const\n"); \
        return false; \
      } \
    } else { \
      DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": tvt.tag == tag_const, vt.tag != tag_const\n"); \
      return false; \
    } \
  } else if (tvt.tag == tag_var) { \
    if (1 || var) { \
      src_tag_t nextpc_tag; \
      int nextpc_val; \
      ASSERT(tvt.val >= 0); \
      ASSERT(tvt.val < MAX_NUM_NEXTPCS); \
      DBG2(PEEP_TAB, "nextpc_map->num_nextpcs_used=%d\n", (int)nextpc_map->num_nextpcs_used);\
      DBG2(PEEP_TAB, "tvt.val=%d\n", (int)tvt.val);\
      DBG2(PEEP_TAB, "nextpc_map[tvt.val].tag=%d\n", (int)nextpc_map->table[tvt.val].tag);\
      DBG2(PEEP_TAB, "nextpc_map[tvt.val].val=%d\n", (int)nextpc_map->table[tvt.val].val);\
      DBG2(PEEP_TAB, "vt.val=%d\n", (int)vt.val);\
      DBG2(PEEP_TAB, "vt.tag=%d\n", (int)vt.tag);\
      if (!(nextpc_map->num_nextpcs_used >= tvt.val)) { \
        printf("tvt.val=%d\n", (int)tvt.val);\
        printf("tvt.tag=%d\n", (int)tvt.tag);\
        printf("nextpc_map->num_nextpcs_used=%d\n", (int)nextpc_map->num_nextpcs_used);\
      } \
      ASSERT(nextpc_map->num_nextpcs_used >= tvt.val); \
      if (nextpc_map->num_nextpcs_used <= tvt.val) { \
        ASSERT(nextpc_map->num_nextpcs_used == tvt.val); \
        nextpc_map->table[tvt.val].tag = vt.tag; \
        nextpc_map->table[tvt.val].val = vt.val; \
        nextpc_map->num_nextpcs_used++; \
      } \
      nextpc_tag = nextpc_map->table[tvt.val].tag; \
      nextpc_val = nextpc_map->table[tvt.val].val; \
      ASSERT(   nextpc_tag == tag_const \
             || nextpc_tag == tag_reloc_string \
             || nextpc_tag == tag_src_insn_pc \
             || nextpc_tag == tag_abs_inum \
             || nextpc_tag == tag_sboxed_abs_inum \
             || nextpc_tag == tag_binpcrel_inum \
             || nextpc_tag == tag_input_exec_reloc_index \
             || nextpc_tag == tag_var); \
      if (   nextpc_map->table[tvt.val].tag != vt.tag \
          || nextpc_map->table[tvt.val].val != vt.val) { \
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": nextpc mappping of tvt does not match vt\n"); \
        return false; \
      } \
    } else { \
      if (vt.tag != tvt.tag) { \
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": tvt.tag == tag_var and vt.tag != tvt.tag\n"); \
        return false; \
      } \
      if (vt.val != tvt.val) { \
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": tvt.tag == tag_var and vt.val(" << vt.val << ") != tvt.val(" << tvt.val << ")\n"); \
        return false; \
      } \
    } \
  } else ASSERT(tvt.tag == tag_invalid); \
} while (0)

#define MATCH_NEXTPC_FIELD(s, ts, fld, nextpc_map, var) do { \
  MATCH_NEXTPC_VALTAG(s->valtag.fld, ts->valtag.fld, nextpc_map, var); \
} while (0)

#endif
