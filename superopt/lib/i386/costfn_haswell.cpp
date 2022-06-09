#include <map>
#include "i386/insn.h"
#include "x64/insn.h"
#include "support/debug.h"
#include "support/c_utils.h"

static char as1[4096];

static int
haswell_movop_compute_cost(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  int swap_bytes = 0;
  int cmov = 0;
  int n;
  n = i386_insn_num_implicit_operands(insn);

  if (strstart(opc, "movbe", NULL)) {
    swap_bytes = 1;
  }
  if (strstart(opc, "cmov", NULL)) {
    cmov = 1;
  }

  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_reg) {
    return cmov + swap_bytes + 1;
  }
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_imm) {
    return cmov + swap_bytes + 1;
  }
  if (insn->op[n + 0].type == op_mem && insn->op[n + 1].type == op_reg) {
    //return cmov + swap_bytes + 3;
    return cmov + swap_bytes + 2; //XXX: was + 3 earlier as above; changed to +2 to be symmetric for reg-mem and mem-reg transfers
  }
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_mem) {
    return cmov + swap_bytes + 2;
  }
  if (insn->op[n + 0].type == op_mem && insn->op[n + 1].type == op_imm) {
    return cmov + swap_bytes + 2;
  }

  if (   strstart(opc, "movs", NULL)
      && !strstart(opc, "movsx", NULL)) {
    return 3;
  }
  if (!strcmp(opc, "movq")) {
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mmx) {
      return 1;
    }
    if (insn->op[n + 0].type == op_mem && insn->op[n + 1].type == op_mmx) {
      return 2;
    }
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mem) {
      return 3;
    }
  }
  if (!strcmp(opc, "movd")) {
    if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_mmx) {
      return 2;
    }
    if (insn->op[n + 0].type == op_mem && insn->op[n + 1].type == op_mmx) {
      return 5; //XXX
    }
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mem) {
      return 6; //XXX
    }
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_reg) {
      return 2;
    }
  }
  return 10; //XXX
}

static long long
haswell_xchg_compute_cost(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  long long i;
  int n;
  n = i386_insn_num_implicit_operands(insn);

  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_reg) {
    return 2;
  } else {
    return 21;
  }
  NOT_REACHED();
}

static long long
haswell_xlat_compute_cost(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  long long i;
  int n;
  n = i386_insn_num_implicit_operands(insn);

  return 7;
}
//#define COST_INFINITY 10000

long long
i386_haswell_insn_compute_cost(i386_insn_t const *insn)
{
  char const *remaining;
  char const *opc;
  long long i;
  int n;

  opc = opctable_name(insn->opc);
  n = i386_insn_num_implicit_operands(insn);

  if (i386_insn_is_nop(insn)) {
    return 0;
  }

  if (i386_insn_is_jecxz(insn)) {
    return COST_INFINITY; //we do not want to emit jecxz instructions as they
                          //pose limitations on the jump distance (single byte)
  }

  if (   strstart(opc, "movups", NULL)
      || strstart(opc, "vmovups", NULL)) {
    return 10; //XXX
  }

  if (   strstart(opc, "mov", NULL)
      || strstart(opc, "cmov", NULL)) {
    return haswell_movop_compute_cost(insn);
  }

  if (strstart(opc, "xchg", NULL)) {
    return haswell_xchg_compute_cost(insn);
  }

  if (strstart(opc, "xlat", NULL)) {
    return haswell_xlat_compute_cost(insn);
  }


  if (strstart(opc, "pushf", NULL)) {
    return 4;
  }
  if (strstart(opc, "pusha", NULL)) {
    return 20;
  }
  if (strstart(opc, "push", NULL)) {
    if (   insn->op[n + 0].type == op_reg
        || insn->op[n + 0].type == op_imm) {
      return 3;
    } else {
      return 5; //XXX
    }
  }
  if (strstart(opc, "popf", NULL)) {
    return 10;
  }
  if (strstart(opc, "popa", NULL)) {
    return 20;
  }

  if (   strstart(opc, "pop", NULL)
      && !strstart(opc, "popcnt", NULL)) {
    if (insn->op[n + 0].type == op_reg) {
      return 2;
    } else {
      return 4;
    }
  }
  if (   strstart(opc, "lahf", NULL)
      || strstart(opc, "sahf", NULL)) {
    return 1;
  }
  if (strstart(opc, "call", NULL)) {
    return 20;
  }
  if (strstart(opc, "lea", &remaining) && *remaining != 'v') {
    ASSERT(insn->op[0].type == op_reg);
    ASSERT(insn->op[1].type == op_mem);
    if (   (   insn->op[1].valtag.mem.disp.tag == tag_const
            && insn->op[1].valtag.mem.disp.val == 0)
        || insn->op[1].valtag.mem.base.val == -1
        || insn->op[1].valtag.mem.index.val == -1) {
      //there are at most 2 components to the address.
      return 2;
    } else {
      return 3;
    }
  }
  if (strstart(opc, "bswap", NULL)) {
    if (insn->op[n + 0].size == 4) {
      return 1;
    } else if (insn->op[n + 0].size == 8) {
      return 2;
    }
  }

  //Arithmetic instructions
  if (   strstart(opc, "add", NULL)
      || strstart(opc, "sub", NULL)) {
    if (   insn->op[n + 0].type == op_reg
        && (   insn->op[n + 1].type == op_reg
            || insn->op[n + 1].type == op_imm)) {
      return 1;
    }
    if (   insn->op[n + 0].type == op_reg
        && insn->op[n + 1].type == op_mem) {
      return 2;
    }
    if (   insn->op[n + 0].type == op_mem
        && (   insn->op[n + 1].type == op_reg
            || insn->op[n + 1].type == op_imm)) {
      return 6;
    }
    if (   strstart(opc, "addp", NULL)
        || strstart(opc, "adds", NULL)
        || strstart(opc, "subp", NULL)
        || strstart(opc, "subs", NULL)) {
      return 6;
    }
    NOT_REACHED();
  }
  if (   strstart(opc, "adc", NULL)
      || strstart(opc, "sbb", NULL)) {
    if (   insn->op[n + 0].type == op_reg
        && (   insn->op[n + 1].type == op_reg
            || insn->op[n + 1].type == op_imm)) {
      return 2;
    }
    if (   insn->op[n + 0].type == op_reg
        && insn->op[n + 1].type == op_mem) {
      return 2;
    }
    if (   insn->op[n + 0].type == op_mem
        && (   insn->op[n + 1].type == op_reg
            || insn->op[n + 1].type == op_imm)) {
      return 7;
    }
    NOT_REACHED();
  }

  if (strstart(opc, "cmp", NULL)) {
    return 1;
  }

  char const *unops[] = {"not", "neg", "inc", "dec"};
  for (i = 0; i < sizeof unops/sizeof unops[0]; i++) {
    if (strstart(opc, unops[i], NULL)) {
      if (insn->op[n + 0].type == op_reg) {
        return 1;
      } else if (insn->op[n + 0].type == op_mem) {
        return 6;
      }
      NOT_REACHED();
    }
  }

  if (   strstart(opc, "aaa", NULL)
      || strstart(opc, "daa", NULL)
      || strstart(opc, "das", NULL)
      || strstart(opc, "aad", NULL)) {
    return 4;
  }

  if (strstart(opc, "aas", NULL)) {
    return 6;
  }

  if (strstart(opc, "aam", NULL)) {
    return 21;
  }

  if (  (   strstart(opc, "mul", &remaining)
         || strstart(opc, "imul", &remaining))
      && *remaining != 'x') {
    if (   insn->op[n + 0].type == op_reg
        && insn->op[n + 1].type == invalid) {
      if (   insn->op[n + 0].size == 1
          || insn->op[n + 0].size == 8) {
        return 3;
      } else {
        return 4;
      }
      NOT_REACHED();
    } else if (   insn->op[n + 0].type == op_mem
               && insn->op[n + 1].type == invalid) {
      if (   insn->op[n + 0].size == 1
          || insn->op[n + 0].size == 8) {
        return 4; //XXX
      } else {
        return 5; //XXX
      }
      NOT_REACHED();
    }
    if (   strstart(opc, "muls", NULL)
        || strstart(opc, "mulp", NULL)) {
      return 8;
    }
    ASSERT(strstart(opc, "imul", NULL));
    if (   insn->op[n + 0].type == op_reg
        && insn->op[n + 1].type == op_reg
        && insn->op[n + 2].type == invalid) {
      return 3;
    } else if (   insn->op[n + 0].type == op_reg
               && insn->op[n + 1].type == op_mem
               && insn->op[n + 2].type == invalid) {
      return 4; //XXX
    } else if (   insn->op[n + 0].type == op_reg
               && insn->op[n + 1].type == op_reg
               && insn->op[n + 2].type == op_imm) {
      if (insn->op[n + 0].size == 2) {
        return 4;
      } else {
        return 3;
      }
    } else if (   insn->op[n + 0].type == op_reg
               && insn->op[n + 1].type == op_mem
               && insn->op[n + 2].type == op_imm) {
      if (insn->op[n + 0].size == 2) {
        return 5; //XXX
      } else {
        return 4; //XXX
      }
    }
    NOT_REACHED();
  }
  if (strstart(opc, "mulx", NULL)) {
    if (insn->op[n + 2].type == op_reg) {
      return 4;
    } else if (insn->op[n + 2].type == op_mem) {
      return 5;
    }
    NOT_REACHED();
  }

  if (strstart(opc, "div", NULL)) {
    if (insn->op[n + 0].size == 1) {
      return 23;
    } else if (insn->op[n + 0].size == 2) {
      return 24;
    } else if (insn->op[n + 0].size == 4) {
      return 26;
    } else if (insn->op[n + 0].size == 8) {
      return 60;
    } else if (insn->op[n + 0].size == 16) {
      return 120; //XXX
    } else NOT_REACHED();
  }

  if (strstart(opc, "idiv", NULL)) {
    if (insn->op[n + 0].size == 1) {
      return 24;
    } else if (insn->op[n + 0].size == 2) {
      return 24;
    } else if (insn->op[n + 0].size == 4) {
      return 26;
    } else if (insn->op[n + 0].size == 8) {
      return 70;
    } else NOT_REACHED();
  }

  if (   strstart(opc, "cbtw", NULL)
      || strstart(opc, "cwtl", NULL)
      || strstart(opc, "cdqe", NULL)
      || strstart(opc, "cwtd", NULL)
      || strstart(opc, "cltd", NULL)
      || strstart(opc, "cqo", NULL)) {
    return 1;
  }

  if (strstart(opc, "popcnt", NULL)) {
    if (insn->op[n + 1].type == op_reg) {
      return 3;
    } else if (insn->op[n + 1].type == op_mem) {
      return 4; //XXX
    }
    NOT_REACHED();
  }

  if (strstart(opc, "crc32", NULL)) {
    if (insn->op[n + 1].type == op_reg) {
      return 3;
    } else if (insn->op[n + 1].type == op_mem) {
      return 4;
    }
    NOT_REACHED();
  }

  if (   (strstart(opc, "and", &remaining) && *remaining != 'n')
      || strstart(opc, "or", NULL)
      || strstart(opc, "xor", NULL)) {
    if (   (   insn->op[n + 0].type == op_reg
            || insn->op[n + 0].type == op_xmm
            || insn->op[n + 0].type == op_ymm)
        && (   insn->op[n + 1].type == op_reg
            || insn->op[n + 1].type == op_imm
            || insn->op[n + 1].type == op_xmm
            || insn->op[n + 1].type == op_ymm)) { //orps, andps
      return 1;
    }
    if (   (   insn->op[n + 0].type == op_reg
            || insn->op[n + 0].type == op_xmm
            || insn->op[n + 0].type == op_ymm)
        && insn->op[n + 1].type == op_mem) {
      return 2;
    }
    if (   insn->op[n + 0].type == op_mem
        && (   insn->op[n + 1].type == op_reg
            || insn->op[n + 1].type == op_imm)) {
      return 6;
    }
    NOT_REACHED();
  }
  if (strstart(opc, "test", NULL)) {
    return 1;
  }
  if (   (   strstart(opc, "shr", &remaining)
          || strstart(opc, "shl", &remaining)
          || strstart(opc, "sar", &remaining))
      && (*remaining != 'd' && *remaining != 'x')) {
    if (insn->op[n + 1].type == op_imm) {
      return 1;
    }
    if (insn->op[n + 1].type == op_reg) {
      if (insn->op[n + 0].type == op_reg) {
        return 2;
      }
      if (insn->op[n + 0].type == op_mem) {
        return 4;
      }
      NOT_REACHED();
    }
    NOT_REACHED();
  }

  if (   (   strstart(opc, "ror", &remaining)
          || strstart(opc, "rol", &remaining))
      && *remaining != 'x') {
    if (insn->op[n + 1].type == op_imm) {
      if (insn->op[n + 0].type == op_reg) {
        return 1;
      } else if (insn->op[n + 0].type == op_mem) {
        return 2;
      }
      NOT_REACHED();
    }
    if (insn->op[n + 1].type == op_reg) {
      if (insn->op[n + 0].type == op_reg) {
        return 2;
      } else if (insn->op[n + 0].type == op_mem) {
        return 3;
      }
      NOT_REACHED();
    }
    NOT_REACHED();
  }

  if (   strstart(opc, "rcr", NULL)
      || strstart(opc, "rcl", NULL)) {
    if (   insn->op[n + 1].type == op_imm
        && insn->op[n + 1].valtag.imm.tag == tag_const
        && insn->op[n + 1].valtag.imm.val == 1) {
      if (insn->op[n + 0].type == op_reg) {
        return 2;
      } else if (insn->op[n + 0].type == op_mem) {
        return 3;
      }
      NOT_REACHED();
    }
    if (insn->op[n + 1].type == op_imm) {
      if (insn->op[n + 0].type == op_reg) {
        return 6;
      } else {
        return 6;
      }
      NOT_REACHED();
    }
    if (insn->op[n + 1].type == op_reg) {
      if (insn->op[n + 0].type == op_reg) {
        return 6;
      } else {
        return 6;
      }
      NOT_REACHED();
    }
    NOT_REACHED();
  }

  if (   strstart(opc, "shrd", NULL)
      || strstart(opc, "shld", NULL)) {
    if (insn->op[n + 2].type == op_imm) {
      return 3;
    }
    if (   insn->op[n + 2].type == op_reg
        && insn->op[n + 0].type == op_reg) {
      if (strstart(opc, "shld", NULL)) {
        return 3;
      } else if (strstart(opc, "shrd", NULL)) {
        return 4;
      }
      NOT_REACHED();
    }
    if (   insn->op[n + 2].type == op_reg
        && insn->op[n + 0].type == op_mem) {
      return 4;
    }
    NOT_REACHED();
  }
  if (   strstart(opc, "shrx", NULL)
      || strstart(opc, "sarx", NULL)
      || strstart(opc, "shlx", NULL)
      || strstart(opc, "rorx", NULL)) {
    return 1;
  }

  if (   strstart(opc, "bt", &remaining)
      && *remaining != 'r'
      && *remaining != 's'
      && *remaining != 'c') {
    if (   insn->op[n + 0].type == op_reg
        && (   insn->op[n + 1].type == op_reg
            || insn->op[n + 1].type == op_imm)) {
      return 1;
    }
    if (   insn->op[n + 0].type == op_mem
        && insn->op[n + 1].type == op_reg) {
      return 10; //XXX
    }
    if (   insn->op[n + 0].type == op_mem
        && insn->op[n + 1].type == op_imm) {
      return 1; //XXX
    }
    NOT_REACHED();
  }
  if (   strstart(opc, "btr", NULL)
      || strstart(opc, "bts", NULL)
      || strstart(opc, "btc", NULL)) {
    if (   insn->op[n + 0].type == op_reg
        && (   insn->op[n + 1].type == op_reg
            || insn->op[n + 1].type == op_imm)) {
      return 1;
    }
    if (   insn->op[n + 0].type == op_mem
        && insn->op[n + 1].type == op_reg) {
      return 10; //XXX
    }
    if (   insn->op[n + 0].type == op_mem
        && insn->op[n + 1].type == op_imm) {
      return 3; //XXX
    }
    NOT_REACHED();
  }
  if (strstart(opc, "bs", NULL)) {
    return 3;
  }
  if (strstart(opc, "set", NULL)) {
    if (insn->op[n + 0].type == op_reg) {
      return 1;
    }
    if (insn->op[n + 0].type == op_mem) {
      return 2;
    }
    NOT_REACHED();
  }
  if (   strstart(opc, "clc", NULL)
      || strstart(opc, "stc", NULL)
      || strstart(opc, "cmc", NULL)) {
    return 1;
  }
  if (   strstart(opc, "cld", NULL)
      || strstart(opc, "std", NULL)) {
    return 3;
  }
  if (   strstart(opc, "cli", NULL)
      || strstart(opc, "sti", NULL)
      || strstart(opc, "hlt", NULL)
      || strstart(opc, "int", NULL)
      || strstart(opc, "rdpmc", NULL)
      || strstart(opc, "rdtsc", NULL)
      || strstart(opc, "cpuid", NULL)) {
    return 199;
  }

  if (   strstart(opc, "lzcnt", NULL)
      || strstart(opc, "tzcnt", NULL)) {
    if (insn->op[n + 1].type == op_reg) {
      return 3;
    }
    if (insn->op[n + 1].type == op_mem) {
      return 3; //XXX
    }
    NOT_REACHED();
  }

  if (strstart(opc, "andn", NULL)) {
    return 1;
  }

  if (   strstart(opc, "blsi", NULL)
      || strstart(opc, "blsmsk", NULL)
      || strstart(opc, "blsr", NULL)) {
    return 1;
  }
  if (strstart(opc, "bextr", NULL)) {
    if (insn->op[n + 1].type == op_reg) {
      return 2;
    }
    if (insn->op[n + 1].type == op_mem) {
      return 3;
    }
    NOT_REACHED();
  }
  if (strstart(opc, "bzhi", NULL)) {
    return 1;
  }
  if (   strstart(opc, "pdep", NULL)
      || strstart(opc, "pext", NULL)) {
    return 3;
  }

  if (strstart(opc, "j", NULL)) {
    return 2;
  }
  if (strstart(opc, "ret", NULL)) {
    return 3; //XXX
  }
  if (   strstart(opc, "lods", NULL)
      || strstart(opc, "stos", NULL)) {
    return 3; //XXX
  }
  if (strstart(opc, "movs", NULL)) {
    return 5; //XXX
  }
  if (strstart(opc, "scas", NULL)) {
    return 3; //XXX
  }
  if (strstart(opc, "cmps", NULL)) {
    return 5; //XXX
  }

  if (strstart(opc, "xadd", NULL)) {
    return 7; //XXX
  }
  if (strstart(opc, "cmpxchg8b", NULL)) {
    return 9; //XXX
  }
  if (strstart(opc, "cmpxchg16b", NULL)) {
    return 15; //XXX
  }
  if (strstart(opc, "cmpxchg", NULL)) {
    return 8; //XXX
  }
  if (strstart(opc, "enter", NULL)) {
    return 8;
  }
  if (strstart(opc, "leave", NULL)) {
    return 6; //XXX
  }

  if (!strcmp(opc, "pmulhw")) {
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mmx) {
      return 3;
    }
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mem) {
      return 6;
    }
    return 100; //must be xmm instructions
  }
  char const *mmxarithops[] = {"padd", "psub", "por", "pand", "pxor", "psll", "psrl",
    "psraw", "psrad",
    "packss", "punpck", "pavg"};
  for (i = 0; i < sizeof mmxarithops/sizeof mmxarithops[0]; i++) {
    if (strstart(opc, mmxarithops[i], NULL)) {
      if (   insn->op[n + 1].type == op_mmx
          || insn->op[n + 1].type == op_xmm
          || insn->op[n + 1].type == op_ymm
          || insn->op[n + 1].type == op_imm) {
        return 1;
      } else if (insn->op[n + 1].type == op_mem) {
        return 4;
      } else NOT_REACHED();
    }
  }
  if (strstart(opc, "ptest", NULL)) {
    if (insn->op[n + 1].type == op_mem) {
      return 5;
    } else if (   insn->op[n + 1].type == op_mmx
               || insn->op[n + 1].type == op_xmm
               || insn->op[n + 1].type == op_ymm) {
      return 1;
    } else NOT_REACHED();
  }
  char const *mmxarith2ops[] = {"pcmpeq", "pcmpgt"};
  for (i = 0; i < sizeof mmxarith2ops/sizeof mmxarith2ops[0]; i++) {
    if (strstart(opc, mmxarith2ops[i], NULL)) {
      if (insn->op[n+1].type == op_mem) {
        return 5;
      } else if (   insn->op[n+1].type == op_mmx
                 || insn->op[n+1].type == op_xmm
                 || insn->op[n+1].type == op_ymm) {
        return 1;
      } else NOT_REACHED();
    }
  }
  if (strstart(opc, "pmadd", NULL)) {
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mmx) {
      return 3;
    }
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mem) {
      return 6;
    }
    NOT_REACHED();
  }
  if (strstart(opc, "pmovmskb", NULL)) {
    return 3;
  }
  if (strstart(opc, "pshufd", NULL)) {
    return 3;
  }
  if (strstart(opc, "palignr", NULL)) {
    return 1;
  }
  if (   strstart(opc, "pshufw", NULL)
      || strstart(opc, "pshufhw", NULL)
      || strstart(opc, "pshuflw", NULL)) {
    return 2;
  }
  if (strstart(opc, "psadbw", NULL)) {
    return 3; //XXX
  }
  if (strstart(opc, "pcmpistri", NULL)) {
    return 4;
  }
  /*if (i386_insn_is_branch(insn)) {
    return 20;
  }
  for (i = 0; i < 3; i++) {
    if (insn->op[i].type == op_mem && strcmp(opc, "lea")) {
      return 10; //XXX
    }
  }*/
  if (strstart(opc, "f", NULL)) {
    return 100;
  }
  if (   strstart(opc, "lds", NULL)
      || strstart(opc, "lss", NULL)
      || strstart(opc, "les", NULL)
      || strstart(opc, "lfs", NULL)
      || strstart(opc, "lgs", NULL)) {
    return 100;
  }
  if (strstart(opc, "unpck", NULL)) {
    return 100;
  }
  if (strstart(opc, "shufp", NULL)) {
    return 100;
  }
  if (   strstart(opc, "mins", NULL)
      || strstart(opc, "minp", NULL)
      || strstart(opc, "maxs", NULL)
      || strstart(opc, "maxp", NULL)) {
    return 100;
  }
  if (strstart(opc, "stmxcsr", NULL) || strstart(opc, "ldmxcsr", NULL)) {
    return 100;
  }
  if (opc[0] == 'p') {
    return 3;
  }
  if (strstart(opc, "xgetbv", NULL) || strstart(opc, "xsetbv", NULL)) {
    return 100;
  }
  if (   strstart(opc, "sfence", NULL)
      || strstart(opc, "lfence", NULL)) {
    return 4; //XXX
  }
  if (strstart(opc, "prefetch", NULL)) {
    return 1; //XXX
  }
  if (strstart(opc, "cvt", NULL)) {
    return 4; //XXX
  }
  if (   strstart(opc, "inb", NULL)
      || strstart(opc, "inw", NULL)
      || strstart(opc, "inl", NULL)
      || strstart(opc, "outb", NULL)
      || strstart(opc, "outw", NULL)
      || strstart(opc, "outl", NULL)) {
    return 100;
  }
  if (   strstart(opc, "iret", NULL)
      || strstart(opc, "lret", NULL)) {
    return 100;
  }
  if (strstart(opc, "ud2", NULL)) {
    return 100;
  }
  if (strstart(opc, "arp", NULL)) {
    return 100;
  }
  if (   strstart(opc, "comis", NULL)
      || strstart(opc, "ucomis", NULL)) {
    return 100;
  }
  if (strstart(opc, "ucomis", NULL)) {
    return 100;
  }
  if (strstart(opc, "bound", NULL)) {
    return 100;
  }
  if (strstart(opc, "loop", NULL)) {
    return 100;
  }
  printf("WARNING: don't have cost function for insn: %s.\n", i386_insn_to_string(insn, as1, sizeof as1));
  //NOT_REACHED();
  return 3; //XXX
}
