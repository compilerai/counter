#ifndef IMM_VT_MAP_H
#define IMM_VT_MAP_H

//#include "imm_map.h"
#include "valtag/valtag.h"
#include "support/types.h"
#ifdef __cplusplus
#include "expr/expr.h"
#include <istream>
#endif
//#include "rewrite/reg_identifier.h"

//struct imm_map_t;
struct symbol_set_t;
struct vconstants_t;
struct translation_instance_t;
struct valtag_t;
struct symbol_set_t;
struct symbol_map_t;
struct regmap_t;
#ifdef __cplusplus
class nextpc_t;
#endif

typedef struct imm_vt_map_t {
  int num_imms_used;
  struct valtag_t table[NUM_CANON_CONSTANTS];

#ifdef __cplusplus
  void imm_vt_map_from_stream(istream &iss);
#endif
} imm_vt_map_t;

void imm_vt_map_init(imm_vt_map_t *map);
char *imm_vt_map_to_string(imm_vt_map_t const *imm_vt_map, char *buf, int size);
imm_vt_map_t const *imm_vt_map_identity(void);
char *imm_vt_map_to_string_using_symbols(imm_vt_map_t const *imm_vt_map,
    struct symbol_set_t const *symbol_set, char *buf, size_t size);

bool imm_vt_map_rename_symbols_using_symbol_sets(imm_vt_map_t *imm_vt_map,
    struct symbol_set_t const *symbol_set1, struct symbol_set_t const *symbol_set2);

#ifdef __cplusplus
std::pair<bool, size_t>
parse_vconstant(char const *buf,
    struct vconstants_t *vconstant,
    struct translation_instance_t *ti);
#endif
int imm_vt_map_count_assigned_imms(struct imm_vt_map_t *map);
void imm_valtag_get_min_max_vconst_masks(int *min_mask, int *max_mask,
    struct valtag_t const *vt);

#ifdef __cplusplus
class reg_identifier_t;
void imm_valtag_add_constant_using_reg_id(struct valtag_t *vt, reg_identifier_t const &reg_id);
#endif

void imm_vt_map_assign_random(imm_vt_map_t *map, long num_canon_imms);
void imm_vt_map_copy(imm_vt_map_t *dst, imm_vt_map_t const *src);
bool imm_vt_maps_equal(imm_vt_map_t const *a, imm_vt_map_t const *b);
bool imm_vt_map_contains(imm_vt_map_t const *a, imm_vt_map_t const *b);
size_t imm2str(uint64_t val, int tag, imm_modifier_t imm_modifier, int constant_val,
    src_tag_t constant_tag, uint64_t const *sconstants, int exvreg_group,
    char const* reloc_strings, size_t reloc_strings_size,
    i386_as_mode_t mode, char *buf, size_t buflen);
void imm_vt_map_change_val(imm_vt_map_t *dst, imm_vt_map_t const *src, long imm_num,
    src_ulong val);
void imm_vt_map_mark_used_vconstant(imm_vt_map_t *map, struct valtag_t const *vt);
void imm_vt_map_mark_used_vconstant_ignoring_gaps(imm_vt_map_t *map,
    struct valtag_t const *vt);

void imm_vt_map_to_symbol_map(struct symbol_map_t *symbol_map, imm_vt_map_t const *imm_vt_map, struct symbol_set_t const *symbol_set);

/* For every I-->i in 'in' and J-->I in 'inv', put J-->i in 'out' */
/* void imm_vt_map_inv_rename (imm_vt_map_t *out, imm_vt_map_t const *in,
    imm_vt_map_t const *inv); */

extern int64_t special_constants[];
extern int num_special_constants;
bool is_special_constant(int64_t n);

#ifdef __cplusplus
class reg_identifier_t;
int32_t imm_vt_map_rename_vconstant_to_const(struct valtag_t const *vt, int32_t curval,
    valtag_t const *table, long num_imms_used, nextpc_t const *nextpcs,
    int num_nextpcs,
    std::map<exreg_group_id_t, std::map<reg_identifier_t, reg_identifier_t>> const *regmap_extable);

void imm_vt_map_rename_vconstant(/*uint64_t val, src_tag_t tag,
    imm_modifier_t imm_modifier,
    int constant, int64_t const *sconstants, int exvreg_group,*/
    struct valtag_t *vt,
    imm_vt_map_t const *imm_vt_map,// struct imm_vt_map_t const *imm_vt_map,
    nextpc_t const *nextpcs, long num_nextpcs,
    struct regmap_t const *regmap/*, src_tag_t dst_tag*/);

expr_ref read_imm_do_mask(context *ctx, expr_ref bvconst, int mask_len, bool is_unsigned);
expr_ref read_imm_do_lshift(context *ctx, expr_ref bvconst, int shlen, bool arith);
expr_ref read_imm_times_sc2_plus_sc1_mask_x_sc0(context *ctx, bool till, expr_ref bvconst,
    uint64_t const *sconstants);
expr_ref imm_valtag_to_expr_ref(context *ctx, valtag_t const *vt,
    consts_struct_t const *cs);
#endif
//void imm_vt_map_rename_symbols_using_symbol_sets(struct imm_vt_map_t *imm_vt_map,
//    struct symbol_set_t const *symbol_set1, struct symbol_set_t const *symbol_set2);

typedef struct vconstants_t {
  int valid:1;
  uint32_t placeholder;
  imm_modifier_t imm_modifier;
  src_tag_t tag;
  int val;
  uint64_t sconstants[IMM_MAX_NUM_SCONSTANTS];
  int constant_val;
  src_tag_t constant_tag;
  int exvreg_group;
} vconstants_t;

uint64_t TIMES_SC2_PLUS_SC1_MASK_SC0(uint64_t val, int64_t sc2,
    int64_t sc1, int64_t sc0);
uint64_t TIMES_SC2_PLUS_SC1_UMASK_SC0(uint64_t val, int64_t sc2,
    int64_t sc1, int64_t sc0);
uint64_t TIMES_SC2_PLUS_SC1_MASKTILL_SC0(uint64_t val, int64_t sc2,
    int64_t sc1, int64_t sc0);
uint64_t TIMES_SC2_PLUS_SC1_MASKFROM_SC0(uint64_t val, int64_t sc2,
    int64_t sc1, int64_t sc0);

#define MATCH_IMM_FIELD_OP_SC0(s, ts, MODIFIER, OP, map/*, vn*/) \
      case MODIFIER: \
        ASSERT(ts.val >= 0 && ts.val < NUM_CANON_CONSTANTS); \
        ASSERT(map->num_imms_used > ts.val); \
        if (   map->table[ts.val].tag == tag_const \
            && OP(map->table[ts.val].val, \
                  ts.mod.imm.sconstants[0]) \
               != s.val) {\
          DBG2(PEEP_TAB, "MATCH_IMM_FIELD_OP_SC0: var %d assignment " \
              "mismatch " #OP "(0x%x, 0x%x) <-> 0x%x\n", \
              (int)ts.val, (int)map->table[ts.val].val, \
              (int)ts.mod.imm.sconstants[0], (int)s.val); \
          return false; \
        } \
        break; \

#define MATCH_IMM_FIELD_OP_SC2_SC1_SC0(s, ts, MODIFIER, OP, map/*, vn*/) \
      case MODIFIER: \
        ASSERT(ts.val >= 0 && ts.val < NUM_CANON_CONSTANTS); \
        ASSERT(map->num_imms_used > ts.val); \
        if (   map->table[ts.val].tag == tag_const \
            && OP(map->table[ts.val].val, \
               ts.mod.imm.sconstants[2], \
               ts.mod.imm.sconstants[1], \
               ts.mod.imm.sconstants[0]) != s.val) {\
          DBG2(PEEP_TAB, "MATCH_IMM_FIELD_OP_SC0: var %d assignment " \
              "mismatch " #OP "(0x%x, 0x%x, 0x%x, 0x%x) <-> 0x%x\n", \
              (int)ts.val, (int)map->table[ts.val].val, \
              (int)ts.mod.imm.sconstants[2], \
              (int)ts.mod.imm.sconstants[1], \
              (int)ts.mod.imm.sconstants[0], \
              (int)s.val); \
          return false; \
        } \
        break; \

#define MATCH_IMM_VT_FIELD(s, ts, vt_map) do { \
  /*if (s.tag != ts.tag) { \
    return false; \
  } */\
  if (s.tag == tag_const) { \
    /*imm_map_t imm_map_orig, imm_map_final; \
    imm_map_from_imm_vt_map(&imm_map_orig, vt_map); \
    imm_map_copy(&imm_map_final, &imm_map_orig); \
    DBG2(PEEP_TAB, "before matching field " #fld " imm_map:\n%s\n", imm_map_to_string(&imm_map_final, as1, sizeof as1)); \
    MATCH_IMM_FIELD(s, ts, fld, (&imm_map_final)); \
    DBG2(PEEP_TAB, "after matching field " #fld " imm_map:\n%s\n", imm_map_to_string(&imm_map_final, as1, sizeof as1)); \
    imm_vt_map_add_using_imm_map(vt_map, &imm_map_orig, &imm_map_final); */\
    MATCH_IMM_FIELD(s, ts, vt_map); \
  } else { \
    if (ts.tag == tag_const) { \
      ASSERT(s.tag != tag_const); \
      return false; \
    } else if (ts.tag == tag_var) { \
      ASSERT(s.tag != tag_const); \
      int val, mask; \
      switch (ts.mod.imm.modifier) { \
        case IMM_UNMODIFIED: \
          val = ts.val; \
          mask = 32; \
          break; \
        case IMM_TIMES_SC2_PLUS_SC1_MASK_SC0: \
        case IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0: \
          val = ts.val; \
          mask = ts.mod.imm.sconstants[0]; \
          ASSERT(ts.mod.imm.sconstants[1] == 0); \
          ASSERT(ts.mod.imm.sconstants[2] == 1); \
          break; \
        default: return false; \
      } \
      ASSERT(val >= 0 && val < NUM_CANON_CONSTANTS); \
      if (vt_map->num_imms_used > val) { \
        if (!imm_valtags_equal(&vt_map->table[val], &s)) { \
          printf("%s() %d: returning false because existing imm_valtag not equal to new one\n", __func__, __LINE__); \
          return false; \
        } \
      } else { \
        ASSERT(vt_map->num_imms_used == val); \
        valtag_copy(&vt_map->table[val], &s); \
        vt_map->num_imms_used++; \
      } \
    } else NOT_REACHED(); \
  } \
} while(0)

#define MATCH_IMM_FIELD(s, ts, map) do { \
  uint64_t (*maskfn)(uint64_t a, int64_t b) = nullptr; \
  ASSERT(s.tag == tag_const); \
  DYN_DBG2(insn_match, "ts.tag=%d\n", ts.tag); \
  if (ts.tag == tag_const) { \
    if (ts.val != s.val) { \
      DYN_DBG2(insn_match, "MATCH_IMM_FIELD: constants mismatch %" PRIx64 \
          " <-> %" PRIx64 "\n", (uint64_t)ts.val, \
          (uint64_t)s.val); \
      return false; \
    } \
  } else { \
    switch (ts.mod.imm.modifier) { \
      case IMM_UNMODIFIED: \
        ASSERT(ts.val >= 0 && ts.val < NUM_CANON_CONSTANTS); \
        if (map->num_imms_used > ts.val) { \
          if (   map->table[ts.val].tag != tag_const \
              || map->table[ts.val].val != s.val) { \
            DYN_DBG2(insn_match, "MATCH_IMM_FIELD: var %d assignment mismatch 0x%x (tag %d) <-> 0x%x\n", \
                (int)ts.val, (int)map->table[ts.val].val, \
                (int)map->table[ts.val].tag, \
                (int)s.val);\
            return false; \
          } \
        } else { \
          ASSERT(map->num_imms_used == ts.val); /*this may not be true if MATCH_IMM_FIELD called from within MATCH_IMM_VT_FIELD*/\
          valtag_copy(&map->table[map->num_imms_used], &s); \
          map->num_imms_used++; \
        } \
        break; \
      case IMM_TIMES_SC2_PLUS_SC1_MASK_SC0: \
      case IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0: \
        ASSERT(ts.val >= 0 && ts.val < NUM_CANON_CONSTANTS); \
        /*ASSERT(map->num_imms_used >= ts.val); */\
        if (ts.val >= map->num_imms_used) { \
          int kk;\
          for (kk = map->num_imms_used; kk < ts.val; kk++) { \
            map->table[kk].tag = tag_invalid; \
            map->table[kk].val = -1; \
          } \
          /* The introduction of a new vconst can only be using either IMM_MODIFIED
             or with IMM_TIMES_SC2_PLUS_SC1_MASK_SC0 with sc3=1,sc2=0. */ \
          ASSERT(ts.mod.imm.sconstants[1] == 0); \
          ASSERT(ts.mod.imm.sconstants[2] == 1); \
          if (   ts.mod.imm.modifier == IMM_TIMES_SC2_PLUS_SC1_MASK_SC0 \
              && MASK(s.val * ts.mod.imm.sconstants[2] \
                      + ts.mod.imm.sconstants[1], \
                     ts.mod.imm.sconstants[0]) !=  \
                     SIGN_EXT(s.val, ts.mod.imm.sconstants[0])) { \
            DYN_DBG2(insn_match, "MATCH_IMM_FIELD: IMM_TIMES_SC3_PLUS_SC2_MASK_SC: " \
                "MASK(0x%x*%d+%" PRId64 ",0x%x)!=SIGN_EXT(0x%x,0x%x)\n", \
                (int)s.val, (int)ts.mod.imm.sconstants[2], \
                ts.mod.imm.sconstants[1], \
                (int)ts.mod.imm.sconstants[0], \
                (int)s.val, (int)ts.mod.imm.sconstants[0]);\
            return false; \
          } else if ( \
                 ts.mod.imm.modifier == IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0 \
              && UMASK(s.val * ts.mod.imm.sconstants[2] \
                     + ts.mod.imm.sconstants[1], \
                     ts.mod.imm.sconstants[0]) !=  \
                     s.val) { \
            DYN_DBG2(insn_match, "MATCH_IMM_FIELD: IMM_TIMES_SC3_PLUS_SC2_UMASK_SC: " \
                "UMASK(0x%x*%d+%" PRId64 ",0x%x)!=0x%x\n", (int)s.val, \
                (int)ts.mod.imm.sconstants[2], \
                ts.mod.imm.sconstants[1], \
                (int)ts.mod.imm.sconstants[0], \
                (int)s.val);\
            return false; \
          } \
          valtag_copy(&map->table[ts.val], &s); \
          /*map->table[ts.val] = s.val; */\
          map->num_imms_used++; \
        } else { \
          ASSERT(map->num_imms_used > ts.val); \
          if (   ts.mod.imm.modifier == IMM_TIMES_SC2_PLUS_SC1_MASK_SC0 \
              && map->table[ts.val].tag == tag_const \
              && MASK(map->table[ts.val].val \
                      * ts.mod.imm.sconstants[2] \
                      + ts.mod.imm.sconstants[1], \
                     ts.mod.imm.sconstants[0]) != \
                         SIGN_EXT(s.val, \
                             ts.mod.imm.sconstants[0])) {\
            DYN_DBG2(insn_match, "MATCH_IMM_FIELD: IMM_TIMES_SC3_PLUS_SC2_MASK_SC: " \
                "var %d assignment mismatch MASK(0x%x*%d+%d, 0x%x) <-> " \
                "SIGN_EXT(0x%x, 0x%x)\n", (int)ts.val, \
                (int)map->table[ts.val].val, \
                (int)ts.mod.imm.sconstants[2], \
                (int)ts.mod.imm.sconstants[1], \
                (int)ts.mod.imm.sconstants[0], (int)s.val, \
                (int)ts.mod.imm.sconstants[0]); \
            return false; \
          } else if ( \
                 ts.mod.imm.modifier == IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0 \
              && map->table[ts.val].tag == tag_const \
              && UMASK(map->table[ts.val].val \
                       * ts.mod.imm.sconstants[2] \
                       + ts.mod.imm.sconstants[1], \
                     ts.mod.imm.sconstants[0]) != s.val) { \
            DYN_DBG2(insn_match, "MATCH_IMM_FIELD: IMM_TIMES_SC3_PLUS_SC2_UMASK_SC: " \
                "var %d assignment mismatch MASK(0x%x*%d+%d, 0x%x) <-> 0x%x\n", \
                (int)ts.val, (int)map->table[ts.val].val, \
                (int)ts.mod.imm.sconstants[2], \
                (int)ts.mod.imm.sconstants[1], \
                (int)ts.mod.imm.sconstants[0], \
                (int)s.val); \
            return false; \
          } \
        } \
        break; \
      MATCH_IMM_FIELD_OP_SC0(s, ts, IMM_SIGN_EXT_SC0, SIGN_EXT, \
          map/*, vn*/); \
      MATCH_IMM_FIELD_OP_SC0(s, ts, IMM_LSHIFT_SC0, LSHIFT, \
          map/*, vn*/); \
      MATCH_IMM_FIELD_OP_SC0(s, ts, IMM_ASHIFT_SC0, ASHIFT, \
          map/*, vn*/); \
      MATCH_IMM_FIELD_OP_SC2_SC1_SC0(s, ts, \
          IMM_TIMES_SC2_PLUS_SC1_MASKTILL_SC0, \
          TIMES_SC2_PLUS_SC1_MASKTILL_SC0, map/*, vn*/); \
      MATCH_IMM_FIELD_OP_SC2_SC1_SC0(s, ts, \
          IMM_TIMES_SC2_PLUS_SC1_MASKFROM_SC0, \
          TIMES_SC2_PLUS_SC1_MASKFROM_SC0, map/*, vn*/); \
      case IMM_PLUS_SC0_TIMES_C0: \
        ASSERT(ts.val >= 0 && ts.val < NUM_CANON_CONSTANTS); \
        ASSERT(   ts.mod.imm.constant_val >= 0 \
               && ts.mod.imm.constant_val < NUM_CANON_CONSTANTS); \
        ASSERT(ts.mod.imm.constant_tag == tag_var); \
        ASSERT(map->num_imms_used > ts.val); \
        ASSERT(map->num_imms_used > ts.mod.imm.constant_val); \
        if (   map->table[ts.val].tag == tag_const \
            && map->table[ts.mod.imm.constant_val].tag == tag_const \
            && (  map->table[ts.val].val \
                + map->table[ts.mod.imm.constant_val].val \
                  * ts.mod.imm.sconstants[0]) \
                     != s.val) { \
          DYN_DBG2(insn_match, "MATCH_IMM_FIELD: var %d assignment " \
              "mismatch %x + %d * %x) <-> %x\n", \
              (int)ts.val, (int)map->table[ts.val].val, \
              (int)ts.mod.imm.sconstants[0], \
              (int)map->table[ts.mod.imm.constant_val].val, \
              (int)s.val); \
          return false; \
        } \
      break; \
      case IMM_LSHIFT_SC0_PLUS_MASKED_C0: \
      case IMM_LSHIFT_SC0_PLUS_UMASKED_C0: \
        ASSERT(ts.val >= 0 && ts.val < NUM_CANON_CONSTANTS); \
        ASSERT(   ts.mod.imm.constant_val >= 0 \
               && ts.mod.imm.constant_val < NUM_CANON_CONSTANTS); \
        ASSERT(ts.mod.imm.constant_tag == tag_var); \
        ASSERT(map->num_imms_used > ts.val); \
        ASSERT(map->num_imms_used > ts.mod.imm.constant_val); \
        if (ts.mod.imm.modifier \
            == IMM_LSHIFT_SC0_PLUS_MASKED_C0) { \
          maskfn = MASK; \
        } else if (ts.mod.imm.modifier \
            == IMM_LSHIFT_SC0_PLUS_UMASKED_C0) { \
          maskfn = UMASK; \
        } \
        ASSERT(maskfn); \
        if (   map->table[ts.val].tag == tag_const \
            && map->table[ts.mod.imm.constant_val].tag == tag_const \
            && (  LSHIFT(map->table[ts.val].val, ts.mod.imm.sconstants[0]) \
                + (*maskfn)(map->table[ts.mod.imm.constant_val].val, \
                      ts.mod.imm.sconstants[0])) \
                     != s.val) { \
          DBG2(PEEP_TAB, "MATCH_IMM_FIELD: var %d assignment " \
              "mismatch %x + %d * %x) <-> %x\n", \
              (int)ts.val, (int)map->table[ts.val].val, \
              (int)ts.mod.imm.sconstants[0], \
              (int)map->table[ts.mod.imm.constant_val].val, \
              (int)s.val); \
          return false; \
        } \
        break; \
      case IMM_TIMES_SC3_PLUS_SC2_MASKTILL_C0_TIMES_SC1_PLUS_SC0: \
        ASSERT(ts.val >= 0 && ts.val < NUM_CANON_CONSTANTS); \
        ASSERT(   ts.mod.imm.constant_val >= 0 \
               && ts.mod.imm.constant_val < NUM_CANON_CONSTANTS); \
        ASSERT(ts.mod.imm.constant_tag == tag_var); \
        ASSERT(map->num_imms_used > ts.val); \
        ASSERT(map->num_imms_used > ts.mod.imm.constant_val); \
        if (   map->table[ts.val].tag == tag_const \
            && map->table[ts.mod.imm.constant_val].tag == tag_const \
            && MASKBITS(map->table[ts.val].val \
                   * ts.mod.imm.sconstants[3] \
                   + ts.mod.imm.sconstants[2], \
                   map->table[ts.mod.imm.constant_val].val \
                   * ts.mod.imm.sconstants[1] \
                   + ts.mod.imm.sconstants[0]) \
                     != s.val) { \
          DBG2(PEEP_TAB, "MATCH_IMM_FIELD: var %d assignment " \
              "mismatch %x + %d * %x) <-> %x\n", \
              (int)ts.val, (int)map->table[ts.val].val, \
              (int)ts.mod.imm.sconstants[0], \
              (int)map->table[ts.mod.imm.constant_val].val, \
              (int)s.val); \
          return false; \
        } \
        break; \
      default: NOT_REACHED(); \
    } \
  } \
} while(0)

#define ST_CANON_IMM_OP_SC0(OP, MODIFIER) \
  if (   special_constants[__j] != 0 \
      && st_imm_map->table[__i].tag == tag_const \
      && (OP(st_imm_map->table[__i].val, special_constants[__j]) == imm)) { \
    index = __i; \
    imm_modifier = MODIFIER; \
    sconstants[0] = special_constants[__j]; \
    break; \
  }

#define ST_CANON_IMM_OP_SC2_SC1_SC0(OP, MODIFIER) \
        if (   (special_constants[__j] != 0) \
            && (special_constants[__k] != 0) \
            && (special_constants[__l] != 0) \
            && st_imm_map->table[__i].tag == tag_const \
            && (OP(st_imm_map->table[__i].val, special_constants[__l], special_constants[__k], \
                  special_constants[__j]) == imm)) { \
        DBG2(IMM_MAP, "st_canon_imm_op_sc2..0: checked imm %lx against %lx (%lx,%lx,%lx,%lx)\n", \
            (long)imm, (long)OP(st_imm_map->table[__i].val, (long)special_constants[__l], (long)special_constants[__k], \
              (long)special_constants[__j]), (long)st_imm_map->table[__i].val, (long)special_constants[__l], (long)special_constants[__k], (long)special_constants[__j]); \
          index = __i; \
          tag = tag_var; \
          imm_modifier = MODIFIER; \
          sconstants[0] = special_constants[__j]; \
          sconstants[1] = special_constants[__k]; \
          sconstants[2] = special_constants[__l]; \
          break; \
        }

/* max_op_size is required to ensure correct renaming of constants for execution test.
   see i386_insn_rename_constants(). */
#define ST_CANON_IMM_CONST(out_vt, out_imm_map, max_num_out_vt, max_op_size) ({ \
  uint64_t imm = out_vt[0].val; \
  imm_vt_map_t input_imm_map; \
  imm_vt_map_copy(&input_imm_map, &out_imm_map[0]); \
  long n = 0; \
  ASSERT(max_num_out_vt >= 1); \
  if (is_special_constant(imm)) { \
    if (n < max_num_out_vt) { \
      out_vt[n].val = imm; \
      out_vt[n].tag = tag_const; \
      imm_vt_map_copy(&out_imm_map[n], &input_imm_map); \
      n++; \
    } \
  } \
  long __i, index = -1, constant_val = -1; \
  src_tag_t constant_tag = tag_invalid; \
  imm_modifier_t imm_modifier; \
  uint64_t sconstants[IMM_MAX_NUM_SCONSTANTS]; \
  src_tag_t tag = tag_const; \
  for (__i = 0; __i < input_imm_map.num_imms_used; __i++) { \
    long __j; \
    if (   input_imm_map.table[__i].tag == tag_const \
        && input_imm_map.table[__i].val == imm) { \
      index = __i; \
      tag = tag_var; \
      ASSERT(max_op_size <= 32); \
      if (max_op_size == 32) { \
        imm_modifier = IMM_UNMODIFIED; \
      } else { \
        imm_modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0; \
        sconstants[0] = max_op_size; \
        sconstants[2] = 1; \
        sconstants[1] = 0; \
      }  \
      break; \
    } \
    if (max_op_size != 32) { \
      continue; \
    } \
    for (__j = 0; __j < num_special_constants; __j++) { \
      long __k, __l, __m, ck; \
      if (special_constants[__j] == 0) { \
        continue; \
      } \
      for (__k = 0; __k < num_special_constants; __k++) { \
        if (special_constants[__k] == 0) { \
          continue; \
        } \
        for (__l = 0; __l < num_special_constants; __l++) { \
          if (special_constants[__l] == 0) { \
            continue; \
          } \
          if (special_constants[__j] < 8) { \
            continue; /* ignore short masks */ \
          } \
          if (special_constants[__j] > 32 || special_constants[__j] <= -32) { \
            continue; /* ignore invalid masks */ \
          } \
          ST_CANON_IMM_OP_SC2_SC1_SC0(TIMES_SC2_PLUS_SC1_MASK_SC0, \
              IMM_TIMES_SC2_PLUS_SC1_MASK_SC0); \
          ST_CANON_IMM_OP_SC2_SC1_SC0(TIMES_SC2_PLUS_SC1_UMASK_SC0, \
              IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0); \
        } \
        if (index != -1) { \
          break; \
        } \
      } \
      if (index != -1) { \
        break; \
      } \
      for (ck = 0; ck < input_imm_map.num_imms_used; ck++) {  \
        if (   input_imm_map.table[__i].tag == tag_const \
            && input_imm_map.table[ck].tag == tag_const \
            && (  input_imm_map.table[__i].val \
                + special_constants[__j] * input_imm_map.table[ck].val == imm)) { \
          imm_modifier = IMM_PLUS_SC0_TIMES_C0; \
          index = __i; \
          tag = tag_var; \
          sconstants[0] = special_constants[__j]; \
          constant_val = ck; \
          constant_tag = tag_var; \
          break; \
        } \
      } \
      if (index != -1) { \
        break; \
      } \
      if (special_constants[__j] >= 32 || special_constants[__j] <= -32) { \
        break; \
      } \
      ST_CANON_IMM_OP_SC0(SIGN_EXT, IMM_SIGN_EXT_SC0); \
      ST_CANON_IMM_OP_SC0(LSHIFT, IMM_LSHIFT_SC0); \
      ST_CANON_IMM_OP_SC0(ASHIFT, IMM_ASHIFT_SC0); \
      if (special_constants[__j] >= 0 && special_constants[__j] < 32) { \
        for (__k = 0; __k < num_special_constants; __k++) { \
          if (special_constants[__k] < 0 || special_constants[__k] >= 32) { \
            continue; \
          } \
          for (__l = 0; __l < num_special_constants; __l++) { \
            long __prod, __sum; \
            if (special_constants[__l] !=1 && special_constants[__l] != -1) { \
              continue; \
            } \
            if (input_imm_map.table[__i].tag != tag_const) { \
              continue; \
            } \
            __prod = input_imm_map.table[__i].val * special_constants[__l]; \
            __sum = __prod + special_constants[__k]; \
            if (__sum < 0 || __sum >= 32) { \
              continue; \
            } \
            ST_CANON_IMM_OP_SC2_SC1_SC0( \
                TIMES_SC2_PLUS_SC1_MASKTILL_SC0, \
                IMM_TIMES_SC2_PLUS_SC1_MASKTILL_SC0); \
            ST_CANON_IMM_OP_SC2_SC1_SC0( \
                TIMES_SC2_PLUS_SC1_MASKFROM_SC0, \
                IMM_TIMES_SC2_PLUS_SC1_MASKFROM_SC0); \
          } \
          if (index != -1) { \
            break; \
          } \
        } \
      } \
      if (index != -1) { \
        break; \
      } \
      for (ck = 0; ck < input_imm_map.num_imms_used; ck++) {  \
        if (ck == __i) { \
          continue; \
        } \
        if (   input_imm_map.table[__i].tag == tag_const \
            && input_imm_map.table[ck].tag == tag_const \
            && (  LSHIFT(input_imm_map.table[__i].val, special_constants[__j]) \
                + MASK(input_imm_map.table[ck].val, special_constants[__j]) == imm)) { \
          imm_modifier = IMM_LSHIFT_SC0_PLUS_MASKED_C0; \
          index = __i; \
          tag = tag_var; \
          sconstants[0] = special_constants[__j]; \
          constant_val = ck; \
          constant_tag = tag_var; \
          break; \
        } \
        if (   input_imm_map.table[__i].tag == tag_const \
            && input_imm_map.table[ck].tag == tag_const \
            && (  LSHIFT(input_imm_map.table[__i].val, special_constants[__j]) \
                + UMASK(input_imm_map.table[ck].val, special_constants[__j]) == imm)) { \
          imm_modifier = IMM_LSHIFT_SC0_PLUS_UMASKED_C0; \
          index = __i; \
          tag = tag_var; \
          sconstants[0] = special_constants[__j]; \
          constant_val = ck; \
          constant_tag = tag_var; \
          break; \
        } \
        /* This is not needed. */ \
        /* for (__k = 0; __k < num_special_constants; __k++) { \
          for (__l = 0; __l < num_special_constants; __l++) { \
            for (__m = 0; __m < num_special_constants; __m++) { \
              if (MASKBITS(special_constants[__m] * st_imm_map->table[__i] \
                  + special_constants[__l], \
                  special_constants[__k] * st_imm_map->table[ck] \
                  + special_constants[__j])) { \
                 index = __i; \
                 tag = tag_var; \
                 imm_modifier = \
                     IMM_TIMES_SC3_PLUS_SC2_MASKTILL_C0_TIMES_SC1_PLUS_SC0; \
                 constant_var = ck; \
                 constant_tag = tag_var; \
                 sconstants[0] = special_constants[__j]; \
                 sconstants[1] = special_constants[__k]; \
                 sconstants[2] = special_constants[__l]; \
                 sconstants[3] = special_constants[__m]; \
               } \
            } \
            if (index != -1) { \
              break; \
            } \
          } \
          if (index != -1) { \
            break; \
          } \
        } */ \
        if (index != -1) { \
          break; \
        } \
      } \
      if (index != -1) { \
        break; \
      } \
    } \
    if (index != -1) { \
      break; \
    } \
  } \
  if (index != -1 && n < max_num_out_vt) { \
    imm_vt_map_copy(&out_imm_map[n], &input_imm_map); \
    out_vt[n].val = index; \
    out_vt[n].tag = tag; \
    out_vt[n].mod.imm.modifier = imm_modifier; \
    out_vt[n].mod.imm.constant_val = constant_val; \
    out_vt[n].mod.imm.constant_tag = constant_tag; \
    for (__i = 0; __i < IMM_MAX_NUM_SCONSTANTS; __i++) { \
      out_vt[n].mod.imm.sconstants[__i] = sconstants[__i]; \
    } \
    n++; \
  } \
  if (input_imm_map.num_imms_used < NUM_CANON_CONSTANTS && n < max_num_out_vt) { \
    imm_vt_map_t new_imm_map; \
    uint32_t masks[] = { 16, 8 }; \
    uint64_t (*extfn)(uint64_t, int64_t); \
    int ext[] = { 0, 1 }; \
    int m, e; \
    imm_vt_map_copy(&new_imm_map, &input_imm_map); \
    index = new_imm_map.num_imms_used++; \
    tag = tag_var; \
    new_imm_map.table[index].val = imm; \
    new_imm_map.table[index].tag = tag_const; \
    ASSERT(max_op_size <= 32); \
    if (max_op_size == 32) { \
      imm_vt_map_copy(&out_imm_map[n], &new_imm_map); \
      out_vt[n].val = index; \
      out_vt[n].tag = tag; \
      out_vt[n].mod.imm.modifier = IMM_UNMODIFIED; \
      n++; \
    } \
    for (m = 0; m < sizeof masks/sizeof masks[0] && n < max_num_out_vt; m++) { \
      if (masks[m] > max_op_size) { \
        continue; \
      } \
      for (e = 0; e < sizeof ext/sizeof ext[0] && n < max_num_out_vt; e++) { \
        if (ext[e] == 0) { \
          imm_modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0; \
          extfn = &ZERO_EXT; \
        } else { \
          imm_modifier = IMM_TIMES_SC2_PLUS_SC1_MASK_SC0; \
          extfn = &SIGN_EXT; \
        } \
        if ((*extfn)(imm, masks[m]) == imm) { \
          imm_vt_map_copy(&out_imm_map[n], &new_imm_map); \
          out_vt[n].val = index; \
          out_vt[n].tag = tag; \
          out_vt[n].mod.imm.modifier = imm_modifier; \
          out_vt[n].mod.imm.sconstants[0] = masks[m]; \
          out_vt[n].mod.imm.sconstants[1] = 0; \
          out_vt[n].mod.imm.sconstants[2] = 1; \
          n++; \
        } \
      } \
    } \
  } \
  /*if (index == -1) { \
    n = 0; \
  } */\
  n;\
})

#define ST_CANON_IMM(opc, explicit_op_index, out_vt, out_imm_map, max_num_out_vt, in_vt, max_op_size, st_map, imm_operand_needs_canonicalization) ({ \
  int ret; \
  ASSERT(max_num_out_vt >= 1); \
  valtag_copy(&out_vt[0], &in_vt); \
  imm_vt_map_t *st_imm_map = &out_imm_map[0]; \
  imm_vt_map_copy(st_imm_map, st_map); \
  ret = 1; \
  if (in_vt.tag == tag_const) { \
    if (imm_operand_needs_canonicalization(opc, explicit_op_index)) { \
      ret = ST_CANON_IMM_CONST(out_vt, out_imm_map, max_num_out_vt, max_op_size); \
      ASSERT(ret >= 1); \
    } \
  } \
  ret; \
})

#define ST_CANON_IMM_ONLY_EQ_CHECK(out_vt, out_imm_map, max_num_out_vt, in_vt, max_op_size, st_imm_map) ({ \
  ASSERT(max_num_out_vt >= 1); \
  imm_vt_map_t *st_imm_map = &out_imm_map[0]; \
  imm_vt_map_copy(st_imm_map, st_map); \
  valtag_copy(&out_vt[0], &in_vt); \
  bool __done; \
  long __i; \
  __done = false; \
  if (out_vt[0].tag == tag_const) { \
    for (__i = 0; __i < st_imm_map->num_imms_used; __i++) { \
      if (   st_imm_map->table[__i].tag == tag_const \
          && st_imm_map->table[__i].val == out_vt[0].val) { \
        out_vt[0].val = __i; \
        out_vt[0].tag = tag_var; \
        ASSERT(max_op_size <= 32); \
        if (max_op_size == 32) { \
          out_vt[0].mod.imm.modifier = IMM_UNMODIFIED; \
        } else { \
          out_vt[0].mod.imm.modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0; \
          out_vt[0].mod.imm.sconstants[0] = max_op_size; \
          out_vt[0].mod.imm.sconstants[2] = 1; \
          out_vt[0].mod.imm.sconstants[1] = 0; \
        } \
        __done = true; \
        break; \
      } \
    } \
    if (!__done) { \
      st_imm_map->table[st_imm_map->num_imms_used].val = out_vt[0].val; \
      st_imm_map->table[st_imm_map->num_imms_used].tag = tag_const; \
      out_vt[0].val = st_imm_map->num_imms_used++; \
      out_vt[0].tag = tag_var; \
      ASSERT(max_op_size <= 32); \
      if (max_op_size == 32) { \
        out_vt[0].mod.imm.modifier = IMM_UNMODIFIED; \
      } else { \
        out_vt[0].mod.imm.modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0; \
        out_vt[0].mod.imm.sconstants[0] = max_op_size; \
        out_vt[0].mod.imm.sconstants[1] = 0; \
        out_vt[0].mod.imm.sconstants[2] = 1; \
      } \
      __done = true; \
    } \
  } \
  1; \
})



#endif /* imm_vt_map.h */
