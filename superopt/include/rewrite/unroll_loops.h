#ifndef UNROLL_LOOPS_H
#define UNROLL_LOOPS_H
#include <stdbool.h>
#include <stdlib.h>
//#include "cpu-defs.h"
#include "support/types.h"

//struct list;
struct translation_instance_t;

void unroll_loops(struct translation_instance_t *ti);

typedef struct induction_reg_t {
  src_ulong name;
  src_ulong value;
  bool valid;
} induction_reg_t;

#define INDUCTION_REG_IMM 64

#endif
