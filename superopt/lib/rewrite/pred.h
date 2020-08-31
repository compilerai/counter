#ifndef __PRED_H
#define __PRED_H

struct si_entry_t;

typedef
struct pred_t
{
  struct si_entry_t const *si;
  long fetchlen;

  src_ulong *lastpcs;
  long num_lastpcs;

  pred_t *next;
} pred_t;

#endif /* pred.h */
