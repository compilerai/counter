#ifndef ANALYZE_STACK_H
#define ANALYZE_STACK_H
#include <stdbool.h>
#include <stdlib.h>
//#include "cpu-defs.h"
#include "support/types.h"

struct clist;
struct translation_instance_t;

void analyze_stack(struct translation_instance_t *ti);
bool stack_offsets_belongs(struct clist const *clist, int offset);
int adjust_stack_offset(struct clist const *regalloced_stack_offsets, int offset1,
    int offset2);

extern bool print_stacklocs;

typedef struct stackloc_t {
  src_ulong function;
  int offset;
} stackloc_t;

extern stackloc_t stacklocs[];
extern size_t stacklocs_size, num_stacklocs;
#endif
