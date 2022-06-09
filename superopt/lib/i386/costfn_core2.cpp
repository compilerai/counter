#include <stdlib.h>
#include <string.h>
#include <map>
#include "i386/insn.h"
#include "x64/insn.h"
#include "support/c_utils.h"

static int
alop_compute_cost(i386_insn_t const *insn)
{
  int n;
  n = i386_insn_num_implicit_operands(insn);
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_reg) {
    return 1;
  }
  if (insn->op[n + 0].type == op_mem && insn->op[n + 1].type == op_reg) {
    return 6;
  }
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_mem) {
    return 4;
  }
  return 10; //XXX
}

static int
unop_compute_cost(i386_insn_t const *insn)
{
  int n;
  n = i386_insn_num_implicit_operands(insn);
  if (insn->op[n + 0].type == op_reg) {
    return 1;
  }
  if (insn->op[n + 0].type == op_mem) {
    return 6;
  }
  return 10; //XXX
}

static int
alcop_compute_cost(i386_insn_t const *insn)
{
  int n;
  n = i386_insn_num_implicit_operands(insn);
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_reg) {
    return 2;
  }
  if (insn->op[n + 0].type == op_mem && insn->op[n + 1].type == op_reg) {
    return 7;
  }
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_mem) {
    return 4;
  }
  return 10; //XXX
}

static int
cmpop_compute_cost(i386_insn_t const *insn)
{
  int n;
  n = i386_insn_num_implicit_operands(insn);
  /*if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_reg) {
    return 1;
  }*/
  if (insn->op[n + 0].type == op_mem || insn->op[n + 1].type == op_mem) {
    return 3;
  }
  return 1; //XXX
}

static int
movop_compute_cost(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  int n;
  n = i386_insn_num_implicit_operands(insn);
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_reg) {
    return 1;
  }
  if (insn->op[n + 0].type == op_mem && insn->op[n + 1].type == op_reg) {
    return 2;
  }
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_mem) {
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

static int
shftop_compute_cost(i386_insn_t const *insn)
{
  int n;
  n = i386_insn_num_implicit_operands(insn);
  if (insn->op[n + 0].type == op_reg) {
    if (insn->op[n + 1].type == op_imm) {
      return 1;
    } else if (insn->op[n + 1].type == op_reg) {
      return 2;
    }
  }
  if (insn->op[n + 0].type == op_mem) {
    if (insn->op[n + 1].type == op_imm) {
      return 6;
    } else if (insn->op[n + 1].type == op_reg) {
      return 7;
    }
  }
  return 10; //XXX
}

static int
btop_compute_cost(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  int n;
  n = i386_insn_num_implicit_operands(insn);
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_reg) {
    return 1;
  }
  if (insn->op[n + 0].type == op_mem && insn->op[n + 1].type == op_reg) {
    if (!strcmp(opc, "bt")) {
      return 4;
    } else {
      return 5;
    }
  }
  return 10; //XXX
}

static int
bsop_compute_cost(i386_insn_t const *insn)
{
  int n;
  n = i386_insn_num_implicit_operands(insn);
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_reg) {
    return 2;
  }
  if (insn->op[n + 0].type == op_reg && insn->op[n + 1].type == op_mem) {
    return 5;
  }
  return 10; //XXX
}

static int
setccop_compute_cost(i386_insn_t const *insn)
{
  int n;
  n = i386_insn_num_implicit_operands(insn);
  if (insn->op[n + 0].type == op_reg) {
    return 2; //XXX
  }
  if (insn->op[n + 0].type == op_mem) {
    return 8;
  }
  return 10;
}

long long
i386_core2_insn_compute_cost(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  long long i;
  int n;
  n = i386_insn_num_implicit_operands(insn);

  if (i386_insn_is_nop(insn)) {
    return 0;
  }

  char const *alops[] = {"add", "sub", "and", "or", "xor"};
  for (i = 0; i < sizeof alops/sizeof alops[0]; i++) {
    if (strstart(opc, alops[i], NULL)) {
      return alop_compute_cost(insn);
    }
  }
  char const *unops[] = {"not", "neg", "inc", "dec"};
  for (i = 0; i < sizeof unops/sizeof unops[0]; i++) {
    if (strstart(opc, unops[i], NULL)) {
      return unop_compute_cost(insn);
    }
  }
  char const *alcops[] = {"adc", "sbb"};
  for (i = 0; i < sizeof alcops/sizeof alcops[0]; i++) {
    if (strstart(opc, alcops[i], NULL)) {
      return alcop_compute_cost(insn);
    }
  }

  if (strstart(opc, "cmp", NULL)) {
    return cmpop_compute_cost(insn);
  }
  if (strstart(opc, "mov", NULL) || strstart(opc, "cmov", NULL)) {
    return movop_compute_cost(insn);
  }
  if (strstart(opc, "push", NULL)) {
    if (insn->op[n + 0].type == op_reg) {
      return 2;
    } else {
      return 6; //XXX
    }
  }
  if (strstart(opc, "pop", NULL)) {
    if (insn->op[n + 0].type == op_reg) {
      return 3;
    } else {
      return 7;
    }
  }
  if (strstart(opc, "call", NULL)) {
    return 20;
  }
  if (strstart(opc, "lea", NULL)) {
    return 1;
  }
  if (strstart(opc, "imul", NULL)) {
    return 3;
  }
  if (strstart(opc, "mul", NULL)) {
    return 5;
  }
  if (strstart(opc, "div", NULL)) {
    return 23;
  }
  char const *shftops[] = {"shl", "shr", "ror", "rol", "rcl", "rcr" /*XXX*/};
  for (i = 0; i < sizeof shftops/sizeof shftops[0]; i++) {
    if (strstart(opc, shftops[i], NULL)) {
      return shftop_compute_cost(insn);
    }
  }
  char const *btops[] = {"bt", "bts", "btr", "btc"};
  for (i = 0; i < sizeof btops/sizeof btops[0]; i++) {
    if (strstart(opc, btops[i], NULL)) {
      return btop_compute_cost(insn);
    }
  }
  if (strstart(opc, "bs", NULL)) {
    return bsop_compute_cost(insn);
  }
  if (strstart(opc, "set", NULL)) {
    return setccop_compute_cost(insn);
  }
  if (!strcmp(opc, "pmulhw")) {
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mmx) {
      return 3;
    }
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mem) {
      return 6;
    }
  }
  char const *mmxarithops[] = {"padd", "psub", "por", "pand", "pxor", "psll", "psrl",
    "packss", "punpck"};
  for (i = 0; i < sizeof mmxarithops/sizeof mmxarithops[0]; i++) {
    if (strstart(opc, mmxarithops[i], NULL)) {
      if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mmx) {
        return 1;
      }
      if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mem) {
        return 4;
      }
    }
  }
  if (strstart(opc, "pmadd", NULL)) {
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mmx) {
      return 3;
    }
    if (insn->op[n + 0].type == op_mmx && insn->op[n + 1].type == op_mem) {
      return 6;
    }
  }
  if (i386_insn_is_branch(insn)) {
    return 20;
  }
  for (i = 0; i < 3; i++) {
    if (insn->op[i].type == op_mem && strcmp(opc, "lea")) {
      return 10; //XXX
    }
  }
  return 3; //XXX
}
