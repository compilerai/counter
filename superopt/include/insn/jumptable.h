#ifndef __JUMPTABLE_H
#define __JUMPTABLE_H
#include "support/types.h"

#define SRC_ENV_JUMPTABLE_ADDR     0x08000000
#define SRC_ENV_JUMPTABLE_INTERVAL 0x01000000

struct input_exec_t;
struct jumptable_t;

void jumptable_init(struct input_exec_t *in);
void jumptable_add(struct jumptable_t *jtable, long target_addr);
bool jumptable_belongs(struct jumptable_t const *jtable, long target_addr);
void jumptable_free(struct jumptable_t *jtable);

int jumptable_count(struct jumptable_t const *jtable);
void *jumptable_iter_begin(struct jumptable_t const *jtable, src_ulong *addr);
void *jumptable_iter_next(struct jumptable_t const *jtable, void *iter,
    src_ulong *addr);
#endif
