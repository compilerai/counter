#ifndef FALLBACK_H
#define FALLBACK_H

#include <string>
#include "support/src-defs.h"
#include "support/types.h"

struct src_insn_t;
struct transmap_t;
struct dst_insn_t;
struct regset_t;
struct transcons_t;
struct translation_instance_t;
struct regmap_t;
struct imm_vt_map_t;
class nextpc_t;
void src_fallback_translate(struct translation_instance_t *ti,
    struct src_insn_t const *src_iseq, long src_iseq_len,
    //struct regset_t const *live_outs,
    nextpc_t const *nextpcs,
    long num_nextpcs, /*src_ulong const *insn_pcs,*/
    struct regmap_t const *st_src_regmap, struct imm_vt_map_t const *st_imm_map,
    struct dst_insn_t *dst_iseq, long *dst_iseq_len, long dst_iseq_size,
    char const *src_filename/*, std::string const &function_name*/);
void src_fallback_init(void);

#endif
