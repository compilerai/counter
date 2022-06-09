#ifndef DST_INSN_FOLDING_OPTS_H
#define DST_INSN_FOLDING_OPTS_H
#include "support/src-defs.h"
#include "support/types.h"
#include <stdlib.h>
#include <stdio.h>
#include <map>
struct dst_insn_t;

size_t dst_insns_folding_optimizations(struct dst_insn_t *dst_iseq, size_t dst_iseq_len, size_t dst_iseq_size, std::map<long, flow_t> const &dst_insn_computed_flow/*, size_t stack_size*/);

#endif
