#ifndef __REGCOUNT_H
#define __REGCOUNT_H

#include "support/src-defs.h"
#include "i386/regs.h"
#include "ppc/regs.h"

struct regset_t;

typedef struct regcount_t {
  //int reg_count;
  int exreg_count[DST_NUM_EXREG_GROUPS];
} regcount_t;


regcount_t const *regcount_zero(void);
void regcount_copy(regcount_t *dst, regcount_t const *src);
long regcount_hash(regcount_t const *temps_count);
char *regcount_to_string(regcount_t const *temps_count, char *buf,
    size_t size);
bool regcounts_equal(regcount_t const *a,
    regcount_t const *b);
void regcount_union(regcount_t *a, regcount_t const *b);
size_t get_temporaries_for_group(long *buf, size_t size, int group,
    struct regset_t const *touch);
void regset_get_temporaries(struct regset_t *temps_regset,
    struct regset_t const *regset);
//void temporaries_count_copy_minus_dst_reserved_regs(temporaries_count_t *dst, temporaries_count_t const *src);

#endif /* regcount.h */
