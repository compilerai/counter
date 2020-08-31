#pragma once
#include "valtag/valtag.h"
#include "support/debug.h"
#include "valtag/reg_identifier.h"
#include <string>

typedef enum {
  TS_TAB_ENTRY_STATE_LOC_NONE = 0,
  TS_TAB_ENTRY_STATE_LOC_EXREG,
  TS_TAB_ENTRY_STATE_LOC_VCONST,
  TS_TAB_ENTRY_STATE_LOC_MEM,
  TS_TAB_ENTRY_STATE_LOC_TMP,
} ts_tab_entry_state_loc_t;

typedef struct state_elem_desc_t {
  ts_tab_entry_state_loc_t loc;
  reg_identifier_t base, index; //base stores group and index stores regnum for LOC_EXREG
  int scale;
  uint64_t regmask;
  valtag_t disp;
  int len;

  state_elem_desc_t() : loc(TS_TAB_ENTRY_STATE_LOC_NONE), base(reg_identifier_t::reg_identifier_invalid()), index(reg_identifier_t::reg_identifier_invalid()), scale(0), regmask(0), len(0)
  {
    disp.tag = tag_const;
    disp.val = 0;
  }
  bool operator<(state_elem_desc_t const &other) const;

  bool operator==(state_elem_desc_t const &other) const
  {
    return !(   *this < other
             || other < *this);
  }

  bool state_elem_desc_contains(state_elem_desc_t const &needle) const
  {
    if (this->loc != needle.loc) {
      return false;
    }
    if (this->loc == TS_TAB_ENTRY_STATE_LOC_MEM) {
      if (this->base != needle.base) {
        return false;
      }
      if (this->index != needle.index) {
        return false;
      }
      if (this->scale != needle.scale) {
        return false;
      }
      bool found_equal_disp = false;
      for (size_t i = 0; i < this->len; i++) {
        valtag_t disp = this->disp;
        imm_valtag_add_constant(&disp, i);
        if (imm_valtags_equal(&disp, &needle.disp)) {
          found_equal_disp = true;;
        }
      }
      if (!found_equal_disp) {
        return false;
      }
      return true;
    }
    NOT_IMPLEMENTED();
  }

} state_elem_desc_t;

struct regmap_t;
struct imm_vt_map_t;

char *state_elem_desc_to_string(struct state_elem_desc_t const *e, char *buf, size_t size);
size_t state_elem_desc_from_string(struct state_elem_desc_t *e, char const *buf,
    bool has_size);
void state_elem_desc_copy(struct state_elem_desc_t *to,
    struct state_elem_desc_t const *from);
bool state_elem_desc_equal(struct state_elem_desc_t const *a,
    struct state_elem_desc_t const *b);
bool state_elem_desc_intersect(struct state_elem_desc_t const *a,
    struct state_elem_desc_t const *b);
void state_elem_desc_rename(struct state_elem_desc_t *to,
    struct state_elem_desc_t const *from, struct regmap_t const *regmap,
    struct imm_vt_map_t const *inv_imm_vt_map);
void state_elem_desc_inv_rename(struct state_elem_desc_t *to,
    struct state_elem_desc_t const *from, struct regmap_t const *regmap);
