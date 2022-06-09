#ifndef LARGE_INT_H
#define LARGE_INT_H
#include <stdlib.h>
//#include "config-host.h"
#include "support/consts.h"

//#define LARGE_INT_DEBUG

#define LARGE_INT_MAX_LEN 128
//#define LARGE_INT_MAX_LEN 64
#define LARGE_INT_SIZE (LARGE_INT_MAX_LEN / (BYTE_LEN * sizeof(uint32_t)))

typedef struct large_int_t {
  uint32_t val[LARGE_INT_SIZE];

#ifdef LARGE_INT_DEBUG
  long long lval; //for debugging only
#endif
} large_int_t;

void large_int_copy(large_int_t *dst, large_int_t const *src);
void large_int_add(large_int_t *dst, large_int_t const *src1, large_int_t const *src2);
void large_int_sub(large_int_t *dst, large_int_t const *src1, large_int_t const *src2);
void large_int_mul(large_int_t *dst, large_int_t const *src1, large_int_t const *src2);
void large_int_div(large_int_t *dst, large_int_t const *src1, large_int_t const *src2);
void large_int_from_unsigned_long_long(large_int_t *dst, unsigned long long src);
void large_int_from_long_long(large_int_t *dst, long long src);
long long large_int_to_long_long(large_int_t const *src);

void large_int_not(large_int_t *dst, large_int_t const *src);
void large_int_neg(large_int_t *dst, large_int_t const *src);

int large_int_compare(large_int_t const *a, large_int_t const *b);

char *large_int_to_string(large_int_t const *li, char *buf, size_t size);

large_int_t const *large_int_zero(void);
large_int_t const *large_int_one(void);
large_int_t const *large_int_minusone(void);
large_int_t const *large_int_max(void);

#endif
