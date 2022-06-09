#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "support/debug.h"
#include "cutils/large_int.h"

#define BASE ((unsigned long long)UINT32_MAX + 1)

#ifdef LARGE_INT_DEBUG
static bool
large_int_check(large_int_t const *li)
{
  long long computed_val;
  int i;

  computed_val = 0;
  for (i = LARGE_INT_SIZE - 1; i >= 0; i--) {
    computed_val *= BASE;
    computed_val += li->val[i];
  }
  return (computed_val == li->lval);
}
#endif

void
large_int_copy(large_int_t *dst, large_int_t const *src)
{
  memcpy(dst, src, sizeof(large_int_t));
}

static void
large_int_add_with_carry(large_int_t *dst, large_int_t const *src1,
    large_int_t const *src2, int incarry)
{
  long long sum;
  int carry;
  int i;

#ifdef LARGE_INT_DEBUG
  ASSERT(large_int_check(src1));
  ASSERT(large_int_check(src2));
#endif
  ASSERT(incarry == 0 || incarry == 1);

  carry = incarry;

  for (i = 0; i < LARGE_INT_SIZE; i++) {
    sum = (long long)src1->val[i] + (long long)src2->val[i] + (long long)carry;
    if (sum >= BASE) {
      carry = 1;
    } else {
      carry = 0;
    }
    dst->val[i] = sum % BASE;
  }
  /*if (   (carry && large_int_compare(dst, large_int_zero()) >= 0)
      || (!carry && large_int_compare(dst, large_int_zero()) < 0)) {
    PANIC("large int overflow");
  }*/

#ifdef LARGE_INT_DEBUG
  dst->lval = src1->lval + src2->lval + incarry;;
  ASSERT(large_int_check(dst));
#endif
}

void
large_int_add(large_int_t *dst, large_int_t const *src1, large_int_t const *src2)
{
  //dst->val = src1->val + src2->val;
  large_int_add_with_carry(dst, src1, src2, 0);
}

void
large_int_not(large_int_t *dst, large_int_t const *src)
{
  int i;

#ifdef LARGE_INT_DEBUG
  ASSERT(large_int_check(src));
#endif
  for (i = 0; i < LARGE_INT_SIZE; i++) {
    dst->val[i] = ~src->val[i];
  }

#ifdef LARGE_INT_DEBUG
  dst->lval = ~src->lval;
  ASSERT(large_int_check(dst));
#endif
}

static void
large_int_mul_one_digit(large_int_t *dst, large_int_t const *src,
    uint32_t digit)
{
  unsigned long long prod;
  int i, carry;

#ifdef LARGE_INT_DEBUG
  ASSERT(large_int_check(src));
#endif
  carry = 0;
  for (i = 0; i < LARGE_INT_SIZE; i++) {
    prod = (unsigned long long)digit * (unsigned long long)src->val[i];
    prod += carry;

    //DBG(TMP, "i = %d, prod = %llx\n", i, prod);
    if (prod >= BASE) {
      carry = prod / BASE;
      dst->val[i] = prod % BASE;
    } else {
      carry = 0;
      dst->val[i] = prod;
    }
  }

  /*if (carry) {
    PANIC("overflow in multiplication");
  }*/

#ifdef LARGE_INT_DEBUG
  dst->lval = src->lval * digit;
  ASSERT(large_int_check(dst));
#endif
}

static void
large_int_shift_left_elems(large_int_t *dst, int shlen)
{
  int i;

#ifdef LARGE_INT_DEBUG
  ASSERT(large_int_check(dst));
#endif
  ASSERT(shlen >= 0 && shlen <= LARGE_INT_SIZE);
  for (i = LARGE_INT_SIZE - 1; i >= shlen; i--) {
    dst->val[i] = dst->val[i - shlen];
  }

  while (i >= 0) {
    dst->val[i] = 0;
    i--;
  }

#ifdef LARGE_INT_DEBUG
  dst->lval = dst->lval << (shlen / sizeof(uint32_t));
  ASSERT(large_int_check(dst));
#endif
}

void
large_int_sub(large_int_t *dst, large_int_t const *src1, large_int_t const *src2)
{
  //dst->val = src1->val - src2->val;
  large_int_t src2_not;

  large_int_not(&src2_not, src2);
  large_int_add_with_carry(dst, src1, &src2_not, 1);
}

void
large_int_neg(large_int_t *dst, large_int_t const *src)
{
  large_int_sub(dst, large_int_zero(), src);
}

void
large_int_mul(large_int_t *dst, large_int_t const *src1, large_int_t const *src2)
{
  bool src1_is_negative, src2_is_negative;
  large_int_t usrc1, usrc2;
  int i;

#ifdef LARGE_INT_DEBUG
  long long lmul;
  lmul = src1->lval * src2->lval;
#endif
  if (large_int_compare(src1, large_int_zero()) < 0) {
    large_int_neg(&usrc1, src1);
    src1_is_negative = true;
  } else {
    large_int_copy(&usrc1, src1);
    src1_is_negative = false;
  }

  if (large_int_compare(src2, large_int_zero()) < 0) {
    large_int_neg(&usrc2, src2);
    src2_is_negative = true;
  } else {
    large_int_copy(&usrc2, src2);
    src2_is_negative = false;
  }

  large_int_copy(dst, large_int_zero());
  for (i = 0; i < LARGE_INT_SIZE; i++) {
    large_int_t product;

    large_int_mul_one_digit(&product, &usrc1, usrc2.val[i]);
    large_int_shift_left_elems(&product, i);
    large_int_add(dst, dst, &product);
  }

  if (src1_is_negative != src2_is_negative) {
    large_int_neg(dst, dst);
  }
#ifdef LARGE_INT_DEBUG
  ASSERT(large_int_check(dst));
  ASSERT(dst->lval == lmul);
#endif
}

void
large_int_div(large_int_t *dst, large_int_t const *src1, large_int_t const *src2)
{
  bool src1_is_negative, src2_is_negative;
  large_int_t usrc1, usrc2;
  //int i;

#ifdef LARGE_INT_DEBUG
  long long ldiv;
  ldiv = src1->lval / src2->lval;
#endif
  if (large_int_compare(src1, large_int_zero()) < 0) {
    src1_is_negative = true;
    large_int_neg(&usrc1, src1);
  } else {
    src1_is_negative = false;
    large_int_copy(&usrc1, src1);
  }

  if (large_int_compare(src2, large_int_zero()) < 0) {
    src2_is_negative = true;
    large_int_neg(&usrc2, src2);
  } else {
    src2_is_negative = false;
    large_int_copy(&usrc2, src2);
  }

  large_int_copy(dst, large_int_zero());

  while (large_int_compare(&usrc1, &usrc2) >= 0) {
    large_int_sub(&usrc1, &usrc1, &usrc2);
    large_int_add(dst, dst, large_int_one());
  }
  if (src1_is_negative != src2_is_negative) {
    large_int_neg(dst, dst);
  }
#ifdef LARGE_INT_DEBUG
  ASSERT(large_int_check(dst));
  ASSERT(ldiv == dst->lval);
#endif
}

void
large_int_from_unsigned_long_long(large_int_t *dst, unsigned long long src)
{
  unsigned long long cur;
  int i;

  cur = src;
  for (i = 0; i < LARGE_INT_SIZE; i++) {
    dst->val[i] = cur % BASE;
    cur -= dst->val[i];
    cur /= BASE;
  }
#ifdef LARGE_INT_DEBUG
  dst->lval = src;
  ASSERT(large_int_check(dst));
#endif
}

void
large_int_from_long_long(large_int_t *dst, long long src)
{
  int i;
  ASSERT(src >= INT32_MIN && src <= INT32_MAX);

  dst->val[0] = src;
  if (src < 0) {
    for (i = 1; i < LARGE_INT_SIZE; i++) {
      dst->val[i] = (uint32_t)-1;
    }
  } else {
    for (i = 1; i < LARGE_INT_SIZE; i++) {
      dst->val[i] = 0;
    }
  }
#ifdef LARGE_INT_DEBUG
  dst->lval = src;
  ASSERT(large_int_check(dst));
#endif
}

int
large_int_compare(large_int_t const *a, large_int_t const *b)
{
  bool a_is_negative, b_is_negative;
  int i;

  if ((int32_t)a->val[LARGE_INT_SIZE - 1] < 0) {
    a_is_negative = true;
  } else {
    a_is_negative = false;
  }

  if ((int32_t)b->val[LARGE_INT_SIZE - 1] < 0) {
    b_is_negative = true;
  } else {
    b_is_negative = false;
  }

  if (a_is_negative != b_is_negative) {
    if (a_is_negative) {
      return -1;
    } else {
      return 1;
    }
  }

  for (i = LARGE_INT_SIZE - 1; i >= 0; i--) {
    if (a->val[i] < b->val[i]) {
      return -1;
    } else if (a->val[i] > b->val[i]) {
      return 1;
    }
  }
  return 0;
  /*if (a->val > b->val) {
    return 1;
  } else if (a->val < b->val) {
    return -1;
  }
  return 0;*/
}

char *
large_int_to_string(large_int_t const *li, char *buf, size_t size)
{
  char *ptr, *end;
  int i;

  ptr = buf;
  end = buf + size;
  ptr += snprintf(ptr, end - ptr, "0x");
  for (i = LARGE_INT_SIZE - 1; i >=0; i--) {
    ptr += snprintf(ptr, end - ptr, "%08x", li->val[i]);
    ASSERT(ptr < end);
  }
  //snprintf(buf, size, "0x%llx", li->val);
  return buf;
}

large_int_t const *
large_int_zero()
{
  static large_int_t __large_int_zero;
  static bool init = false;

  if (!init) {
    int i;
    for (i = 0; i < LARGE_INT_SIZE; i++) {
      __large_int_zero.val[i] = 0;
    }
#ifdef LARGE_INT_DEBUG
    __large_int_zero.lval = 0;
#endif
    init = true;
  }

  return &__large_int_zero;
}

large_int_t const *
large_int_one()
{
  static large_int_t __large_int_one;
  static bool init = false;

  if (!init) {
    int i;
    __large_int_one.val[0] = 1;
    for (i = 1; i < LARGE_INT_SIZE; i++) {
      __large_int_one.val[i] = 0;
    }
#ifdef LARGE_INT_DEBUG
    __large_int_one.lval = 1;
#endif
    init = true;
  }

  return &__large_int_one;
}

large_int_t const *
large_int_minusone(void)
{
  static large_int_t __large_int_minusone;
  static bool init = false;

  if (!init) {
    int i;
    for (i = 0; i < LARGE_INT_SIZE; i++) {
      __large_int_minusone.val[i] = 0xffffffff;
    }
#ifdef LARGE_INT_DEBUG
    __large_int_minusone.lval = -1;
#endif
    init = true;
  }

  return &__large_int_minusone;
}

large_int_t const *
large_int_max()
{
  static large_int_t __large_int_max;
  static bool init = false;

  if (!init) {
    int i;
    for (i = 0; i < LARGE_INT_SIZE; i++) {
      __large_int_max.val[i] = 0xffffffff;
    }
    __large_int_max.val[LARGE_INT_SIZE - 1] = 0x7fffffff;
#ifdef LARGE_INT_DEBUG
    __large_int_max.lval = 0x7fffffffffffffffULL;
#endif
    init = true;
  }

  return &__large_int_max;
}


