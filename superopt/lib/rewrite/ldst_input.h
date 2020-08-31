#ifndef LDST_INPUT_H
#define LDST_INPUT_H

#include <stdint.h>
#include "support/types.h"

struct input_exec_t;
//void ldst_register_input_file(struct input_exec_t *input);
uint32_t ldl_input(struct input_exec_t const *in, src_ulong pc);
//void stl_input(src_ulong pc, uint32_t val);

uint8_t ldub_input(struct input_exec_t const *in, src_ulong pc);
//void stub_input(src_ulong pc, uint8_t val);

#endif /* ldst_input.h */
