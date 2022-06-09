#include <map>
#include "rewrite/bbl.h"
#include "support/src-defs.h"
#include "support/debug.h"
#include "ppc/insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
//#include <cache.h>
#include <stdlib.h>

unsigned long
bbl_num_insns(bbl_t const *bbl)
{
  return bbl->num_insns;
}

src_ulong
bbl_first_insn(bbl_t const *bbl)
{
  //ASSERT(bbl->pcs[0] == bbl->pc);
  return bbl->pcs[0];
}

src_ulong
bbl_last_insn(bbl_t const *bbl)
{
  return bbl->pcs[bbl->num_insns - 1];
}

src_ulong
bbl_next_insn(bbl_t const *bbl, src_ulong pc)
{
  unsigned long i;

  ASSERT(pc >= bbl->pcs[0] && pc < bbl->pcs[0] + bbl->size);
  for (i = 0; i < bbl->num_insns; i++) {
    if (bbl->pcs[i] == pc) {
      if (i == bbl->num_insns - 1) {
        return PC_UNDEFINED;
      } else {
        return bbl->pcs[i + 1];
      }
    }
  }
  NOT_REACHED();
}

src_ulong
bbl_prev_insn(bbl_t const *bbl, src_ulong pc)
{
  unsigned long i;

  ASSERT(pc >= bbl->pcs[0] && pc < bbl->pcs[0] + bbl->size);
  for (i = 0; i < bbl->num_insns; i++) {
    if (bbl->pcs[i] == pc) {
      if (i == 0) {
        return PC_UNDEFINED;
      } else {
        return bbl->pcs[i - 1];
      }
    }
  }
  NOT_REACHED();
}

long
bbl_insn_index(bbl_t const *bbl, src_ulong pc)
{
  unsigned long i;
  for (i = 0; i < bbl->num_insns; i++) {
    if (bbl->pcs[i] == pc) {
      return i;
    }
  }
  return -1;
}

src_ulong
bbl_insn_address(bbl_t const *bbl, unsigned long n)
{
  ASSERT(n >= 0 && n <= bbl->num_insns);
  if (n == bbl->num_insns) {
    return bbl->pcs[0] + bbl->size;
  } else {
    return bbl->pcs[n];
  }
}

/*void
bbl_fetch_insn(bbl_t const *bbl, unsigned long index, unsigned char *code)
{
  NOT_REACHED();
}
*/
