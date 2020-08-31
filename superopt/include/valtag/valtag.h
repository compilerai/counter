#ifndef VALTAG_H
#define VALTAG_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "support/types.h"
#include "support/src-defs.h"
#include "support/src_tag.h"
#include "i386/regs.h"
#include "ppc/regs.h"
#include "codegen/etfg_regs.h"
#include "support/debug.h"
#include "support/consts.h"
#ifdef __cplusplus
#include <istream>
class memslot_map_t;
#endif

struct input_exec_t;
struct imm_vt_map_t;
struct regmap_t;
#ifdef __cplusplus
class vconstants_t;
#endif

//typedef enum { VT_TYPE_BOOL, VT_TYPE_INT, VT_TYPE_CODE, VT_TYPE_BOTTOM } vt_primtype_t;

typedef struct vt_type_t {
  //char pointer_depth; //0 means the value is of type prim. 1 means it is of type &prim.
  //vt_primtype_t prim;
  //char start, stop;
  int64_t min, max;
} vt_type_t;

typedef struct valtag_t {
  long val;
  src_tag_t tag;

  /* The following fields (in next three lines) are needed for variable constants. */
  union {
    struct {
      uint64_t constant_val;
      src_tag_t constant_tag;
      uint64_t sconstants[IMM_MAX_NUM_SCONSTANTS];  //special constant
      imm_modifier_t modifier;
      int exvreg_group;
    } imm;
    struct {
#ifdef __SIZEOF_INT128__
      __uint128_t mask;
#else
      uint64_t mask[2];
#endif
    } reg;
  } mod;

#ifdef __cplusplus
  void imm_valtag_from_stream(std::istream &iss);
  void serialize(std::ostream& os) const;
  void deserialize(std::istream& is);
  /*bool operator<(valtag_t const &other) const
  {
    if (this->tag != other.tag) {
      return this->tag < other.tag;
    }
    if (this->val != other.val) {
      return this->val < other.val;
    }
    NOT_IMPLEMENTED();
  }
  bool operator==(valtag_t const &other) const
  {
    return !(*this < other || other < *this);
  }
  bool operator!=(valtag_t const &other) const
  {
    return !(*this == other);
  }*/
#endif
  //vt_type_t type;
} valtag_t;

size_t valtag_to_int_array(valtag_t const *valtag, int64_t arr[], size_t len);
size_t vt_type_to_string(vt_type_t const *vt_type, char *buf, size_t size);
size_t vt_type_from_string(vt_type_t *vt_type, char const *buf);
char *imm_valtag_to_string(valtag_t const *vt, char const *prefix, char *buf, size_t size);
size_t imm_valtag_from_string(valtag_t *vt, char const *buf);
void valtag_copy(valtag_t *dst, valtag_t const *src);
void imm_valtag_add_constant(valtag_t *vt, long constant);
bool imm_valtag_less(valtag_t const *a, valtag_t const *b);
bool imm_valtags_equal(valtag_t const *a, valtag_t const *b);
bool reg_valtags_equal(valtag_t const *a, valtag_t const *b);
void valtag_canonicalize_local_symbol(valtag_t *vt, struct imm_vt_map_t *imm_vt_map, src_tag_t tag);
void valtag_rename_address_to_symbol(valtag_t *vt, struct input_exec_t const *in, src_ulong pc, size_t pc_size);
void valtag_rename_symbol_to_address(valtag_t *vt, struct input_exec_t const *in, struct chash const *tcodes);
bool valtag_inv_rename_local_symbol(valtag_t *vt, struct imm_vt_map_t *imm_vt_map, src_tag_t tag);
bool valtag_symbols_are_contained_in_map(valtag_t const *vt,
    struct imm_vt_map_t const *imm_vt_map);
bool symbol_is_contained_in_valtag(valtag_t const *vt, long symval, src_tag_t symtag);
//void reg_valtag_rename(valtag_t *vt, int group, struct regmap_t const *regmap);
bool valtag_contains_symbol(valtag_t const *vt);
size_t valtag_get_symbol_id(long const *start_symbol_ids, long *symbol_ids, size_t symbol_ids_max_size, valtag_t const *vt);
#ifdef __cplusplus
valtag_t vconstant_to_valtag(vconstants_t const &vconstant);
void imm_valtag_rename_using_memslot_map(valtag_t *vt, memslot_map_t const &memslot_map);
std::string src_tag_to_string(src_tag_t t);
src_tag_t src_tag_from_string(std::string const &s);


#endif

#endif /* valtag.h */
