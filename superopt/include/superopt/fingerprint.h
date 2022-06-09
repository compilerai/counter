#pragma once
#include "insn/src-insn.h"

void src_iseq_compute_fingerprint(uint64_t *fingerprint,
    src_insn_t const *src_iseq, long src_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    regset_t const *live_out, long num_live_out, bool mem_live_out,
    rand_states_t const &rand_states);
