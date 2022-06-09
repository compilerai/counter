#ifndef INSN_LIST_H
#define INSN_LIST_H

#include "support/src-defs.h"
#include <stdbool.h>
struct src_insn_list_elem_t;
struct dst_insn_list_elem_t;
struct src_strict_canonicalization_state_t;
struct dst_strict_canonicalization_state_t;

struct src_insn_list_elem_t *src_read_insn_list_from_file(char const *filename, bool (*accept_insn_check)(struct src_insn_t *), struct src_strict_canonicalization_state_t *scanon_state);
struct dst_insn_list_elem_t *dst_read_insn_list_from_file(char const *filename, bool (*accept_insn_check)(struct dst_insn_t *), struct dst_strict_canonicalization_state_t *scanon_state);

void src_insn_list_free(struct src_insn_list_elem_t *ls);
void dst_insn_list_free(struct dst_insn_list_elem_t *ls);

#define CMN COMMON_SRC
#include "cmn/cmn_insn_list_h.h"
#undef CMN

#define CMN COMMON_DST
#include "cmn/cmn_insn_list_h.h"
#undef CMN

#endif /* insn_list.h */
