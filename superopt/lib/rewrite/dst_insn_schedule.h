#pragma once
#include "support/types.h"
struct dst_insn_t;

long dst_insns_schedule(struct dst_insn_t *dst_iseq, long dst_iseq_len, long dst_iseq_size, flow_t const *estimated_flow);
