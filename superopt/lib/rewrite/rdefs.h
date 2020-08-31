#ifndef RDEFS_H
#define RDEFS_H

#include <stdlib.h>
#include "ppc/regs.h"
#include "i386/regs.h"
#include "support/src-defs.h"

struct regset_t;
typedef struct list_node_t
{
  int32_t pc;
  struct list_node_t *next;
} list_node_t;

typedef struct rdefs_t
{
  //list_node_t *table[SRC_NUM_REGS];
} rdefs_t;

void rdefs_init(rdefs_t *rdefs);
void rdefs_free(rdefs_t *rdefs);
void rdefs_copy(rdefs_t *dst, rdefs_t const *src);
void rdefs_union(rdefs_t *dst,rdefs_t const *src);
void rdefs_diff(rdefs_t *dst, rdefs_t const *src);
void rdefs_register(rdefs_t *rdefs, struct regset_t const *regs,
    int32_t pc);
void rdefs_clear(rdefs_t *rdefs, struct regset_t const *regs);
int rdefs_equal(rdefs_t const *rdefs1, rdefs_t const *rdefs2);
//void rdefs_unregister(rdefs_t *rdefs, int reg, int32_t pc);

char *rdefs_to_string(rdefs_t *rdefs,
    struct regset_t *use_regs, char *buf, int buf_size);

bool rdefs_contains_crf_logical_op(rdefs_t const *rdefs, int regno);
void rdefs_unregister(rdefs_t *rdefs, int reg, int32_t pc);

#endif
