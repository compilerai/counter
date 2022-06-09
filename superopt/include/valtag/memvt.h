#ifndef MEMVT_H
#define MEMVT_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "support/types.h"
#include "support/src-defs.h"
#include "support/src_tag.h"
#include "support/consts.h"
#include "valtag/imm_vt_map.h"
#include "valtag/valtag.h"

typedef enum {
  segtype_sel = 0,
  segtype_desc
} segtype_t;

typedef struct memvt_t {
  /* following fields for mem */
  unsigned addrsize:4;        //in bytes
  unsigned segtype:2;
  union {
    valtag_t sel;
    valtag_t desc;
  } seg;
  valtag_t base;
  valtag_t index;
  valtag_t riprel;
  unsigned scale:4;
  valtag_t disp;
#ifdef __cplusplus
  void serialize(std::ostream& os) const;
  void deserialize(std::istream& is);
#endif
} memvt_t;

struct regmap_t;
struct imm_vt_map_t;

void memvt_init(memvt_t *mvt, size_t addrsize);
bool mem_valtag_is_src_env_const_access(memvt_t const *mvt);
#ifdef __cplusplus
class memslot_map_t;
void mem_valtag_rename_using_memslot_map(opertype_t *opertype, memvt_t *mvt, valtag_t *imm_vt, memslot_map_t const *memslot_map, dst_ulong base_gpr_offset);
#endif
unsigned long memvt_hash_func(memvt_t const *val, int size);
size_t memvt_to_int_array(memvt_t const *vt, int64_t arr[], size_t len);
//size_t memvt_to_string(memvt_t const *vt, char *buf, size_t size);
void memvt_copy(memvt_t *dst, memvt_t const *src);
bool memvt_equal(memvt_t const *dst, memvt_t const *src);
//size_t memvt_from_string(memvt_t *mvt, char const *buf, size_t size);
//void memvt_rename(memvt_t *mvt, struct regmap_t const *regmap);
size_t memvt_to_string(memvt_t const *valtag,
    bool rex_used,
    char const* reloc_strings, size_t reloc_strings_size,
    i386_as_mode_t mode,
    size_t (*excreg2str)(int, int, src_tag_t, int, bool, uint64_t, char *, size_t),
    char *buf, size_t buflen);

#define MATCH_MEMVT_FIELD(st_regmap, st_imm_map, mem) do { \
  DYN_DBG2(insn_match, "%s", "matching mem.segtype\n"); \
  if (_template->valtag.mem.segtype == segtype_sel) { \
    DYN_DBG2(insn_match, "%s", "matching mem.seg.sel\n"); \
    MATCH_EXREG_FIELD(st_regmap, valtag.mem.seg.sel, I386_EXREG_GROUP_SEGS); \
    DYN_DBG2(insn_match, "%s", "done matching mem.seg.sel\n"); \
  } \
  DYN_DBG2(insn_match, "%s", "matching mem.seg.sel\n"); \
  if (_template->valtag.mem.base.val != -1) { \
    if (op->valtag.mem.base.val == -1) { \
      DYN_DBG2(insn_match, "returning false, _template->valtag." #mem ".base=%ld\n", \
          _template->valtag.mem.base.val); \
      return false; \
    } \
    DYN_DBG2(insn_match, "%s", "matching mem.base\n"); \
    MATCH_EXREG_FIELD(st_regmap, valtag.mem.base, I386_EXREG_GROUP_GPRS); \
  } else { \
    if (op->valtag.mem.base.val != -1) { \
      DYN_DBG2(insn_match, "returning false, op->" #mem ".base=%ld\n", \
          op->valtag.mem.base.val); \
      return false; \
    } \
  } \
  DYN_DBG2(insn_match, "%s", "matching mem.index\n"); \
  if (_template->valtag.mem.index.val != -1) { \
    if (op->valtag.mem.index.val == -1) { \
      DYN_DBG2(insn_match, "returning false, _template->valtag." #mem ".index=%ld\n", \
          _template->valtag.mem.base.val); \
      return false; \
    } \
    if (_template->valtag.mem.scale != op->valtag.mem.scale) { \
      DYN_DBG2(insn_match, "returning false, _template->" #mem ".scale=%d\n", \
          _template->valtag.mem.scale); \
      return false; \
    } \
    DYN_DBG2(insn_match, "matching index field %ld\n", op->valtag.mem.index.val); \
    MATCH_EXREG_FIELD(st_regmap, valtag.mem.index, I386_EXREG_GROUP_GPRS); \
  } else { \
    if (op->valtag.mem.index.val != -1) { \
      DYN_DBG2(insn_match, "returning false, op->valtag." #mem ".index=%ld\n", \
          op->valtag.mem.index.val); \
      return false; \
    } \
  } \
  DYN_DBG2(insn_match, "matching disp field %ld\n", op->valtag.mem.disp.val); \
  MATCH_IMM_VT_FIELD(op->valtag.mem.disp, _template->valtag.mem.disp, st_imm_map); \
} while(0)

#endif
