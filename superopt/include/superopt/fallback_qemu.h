#ifndef FALLBACK_QEMU_H
#define FALLBACK_QEMU_H

#include "support/src-defs.h"
#include "support/types.h"

struct src_insn_t;
struct transmap_t;
struct dst_insn_t;
//struct regset_t;
struct transcons_t;
struct imm_vt_map_t;
struct translation_instance_t;
struct regmap_t;

void fallback_qemu_translate(struct translation_instance_t *ti,
    struct src_insn_t const *src_iseq, long src_iseq_len, long num_nextpcs,
    struct regmap_t const *regmap, struct imm_vt_map_t const *imm_map,
    struct dst_insn_t *dst_iseq, long *dst_iseq_len, long dst_iseq_size);
void fallback_qemu_init(void);

#endif
