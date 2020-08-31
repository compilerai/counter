#include <string.h>
#include <map>
#include "support/debug.h"
#include "support/src-defs.h"
#include "support/types.h"
//#include "cpu-all.h"
//#include "exec-all.h"
#include "ppc/insn.h"
#include "rewrite/bbl.h"

unsigned long
bbl_num_insns(bbl_t const *bbl)
{
  return bbl->size >> 2;
}

src_ulong
bbl_first_insn(bbl_t const *bbl)
{
  return bbl->pc;
}

src_ulong
bbl_last_insn(bbl_t const *bbl)
{
  return bbl->pc + bbl->size - 4;
}

src_ulong
bbl_next_insn(bbl_t const *bbl, src_ulong pc)
{
  ASSERT(pc >= bbl->pc && pc < bbl->pc + bbl->size);
  pc += 4;
  ASSERT(pc >= bbl->pc && pc <= bbl->pc + bbl->size);
  if (pc - bbl->pc < bbl->size) {
    return pc;
  } else {
    return PC_UNDEFINED;
  }
}

src_ulong
bbl_prev_insn(bbl_t const *bbl, src_ulong pc)
{
  ASSERT(pc >= bbl->pc && pc <= bbl->pc + bbl->size);
  if (pc > bbl->pc) {
    pc -= 4;
  } else {
    return PC_UNDEFINED;
  }
  ASSERT(pc >= bbl->pc && pc <= bbl->pc + bbl->size);
  return pc;
}

long
bbl_insn_index(bbl_t const *bbl, src_ulong pc)
{
  return (pc - bbl->pc)>>2;
}

src_ulong
bbl_insn_address(bbl_t const *bbl, unsigned long n)
{
  return bbl->pc + (n << 2);
}

/*void
bbl_fetch_insn(bbl_t const *bbl, unsigned long index, unsigned char *code)
{
  uint32_t opcode = ldl_input(bbl->pc + index*4);
  memcpy(code, &opcode, sizeof opcode);
}*/
