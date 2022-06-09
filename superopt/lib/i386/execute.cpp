#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

#include "support/debug.h"
#include "support/c_utils.h"

#include "valtag/regset.h"
#include "valtag/nextpc.h"
#include "valtag/transmap.h"
#include "valtag/regcons.h"
#include "valtag/regmap.h"

#include "i386/execute.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "ppc/insn.h"
#include "ppc/execute.h"

#include "rewrite/static_translate.h"
#include "rewrite/peephole.h"

#include "superopt/src_env.h"

static char as1[40960];

size_t
save_callee_save_registers(uint8_t *buf, size_t size)
{
  size_t sul = sizeof(unsigned long);
  uint8_t *tc_ptr, *tc_end;

  tc_ptr = buf;
  tc_end = buf + size;
  /* mov %rbp, TMP_STORAGE*/
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x89; *tc_ptr++ = 0x2c; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE; tc_ptr += 4;
  /* mov %rbx, TMP_STORAGE+4*/
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x89; *tc_ptr++ = 0x1c; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE + sul; tc_ptr += 4;
  /* mov %rsi, TMP_STORAGE+8*/
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x89; *tc_ptr++ = 0x34; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE + 2*sul; tc_ptr += 4;
  /* mov %rdi, TMP_STORAGE+12*/
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x89; *tc_ptr++ = 0x3c; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE + 3*sul; tc_ptr += 4;
  /* mov %rsp, TMP_STORAGE+16*/
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x89; *tc_ptr++ = 0x24; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE + 4*sul; tc_ptr += 4;
  ASSERT(tc_ptr < tc_end);
  return tc_ptr - buf;
}

static size_t
restore_callee_save_registers(uint8_t *buf, size_t size)
{
  size_t sul = sizeof(unsigned long);
  uint8_t *tc_ptr, *tc_end;

  tc_ptr = buf;
  tc_end = buf + size;
  /* mov TMP_STORAGE, %rbp*/
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x8b; *tc_ptr++ = 0x2c; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE; tc_ptr += 4;
  /* mov TMP_STORAGE, %rbx*/
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x8b; *tc_ptr++ = 0x1c; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE + sul; tc_ptr += 4;
  /* mov TMP_STORAGE, %rsi*/
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x8b; *tc_ptr++ = 0x34; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE + 2*sul; tc_ptr += 4;
  /* mov TMP_STORAGE, %rdi*/
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x8b; *tc_ptr++ = 0x3c; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE + 3*sul; tc_ptr += 4;
  /* mov TMP_STORAGE + 16, %rsp */
  *tc_ptr++ = 0x48; *tc_ptr++ = 0x8b; *tc_ptr++ = 0x24; *tc_ptr++ = 0x25;
  *(unsigned long *)tc_ptr = TMP_STORAGE + 4*sul; tc_ptr += 4;
  ASSERT(tc_ptr < tc_end);
  return tc_ptr - buf;
}

static bool
memop_is_src_env_access(operand_t const *op)
{
  if (op->valtag.mem.base.val != -1) {
    return false;
  }
  if (op->valtag.mem.index.val != -1) {
    return false;
  }
  if (   op->valtag.mem.disp.tag == tag_const
      && op->valtag.mem.disp.val >= SRC_ENV_ADDR
      && op->valtag.mem.disp.val < SRC_ENV_ADDR + SRC_ENV_SIZE) {
    return true;
  }
  if (   op->valtag.mem.disp.tag == tag_imm_exvregnum
      && (   op->valtag.mem.disp.mod.imm.modifier == IMM_TIMES_SC2_PLUS_SC1_MASK_SC0
          || op->valtag.mem.disp.mod.imm.modifier == IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0)
      && op->valtag.mem.disp.mod.imm.sconstants[0] == 32
      && op->valtag.mem.disp.mod.imm.sconstants[1] >= SRC_ENV_ADDR
      && op->valtag.mem.disp.mod.imm.sconstants[1] < SRC_ENV_ADDR + SRC_ENV_SIZE) {
    return true;
  }
  if (op->valtag.mem.disp.tag == tag_imm_symbol) { //memloc
    return true;
  }
  return false;
}

static long
i386_insn_put_compare_with_imm(operand_t const *op, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;
  char const *cmp_opc;

  if (op->size == 1) {
    cmp_opc = "cmpb";
  } else if (op->size == 2) {
    cmp_opc = "cmpw";
  } else if (op->size == 4) {
    cmp_opc = "cmpl";
  } else NOT_REACHED();

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(insn_ptr);
  insn_ptr->opc = opc_nametable_find(cmp_opc);
  /*insn_ptr->op[0].type = op_flags_unsigned;
  insn_ptr->op[0].valtag.flags_unsigned.val = 0;
  insn_ptr->op[0].valtag.flags_unsigned.tag = tag_const;
  insn_ptr->op[1].type = op_flags_signed;
  insn_ptr->op[1].valtag.flags_signed.val = 0;
  insn_ptr->op[1].valtag.flags_signed.tag = tag_const;*/
  i386_operand_copy(&insn_ptr->op[0], op);
  insn_ptr->op[1].type = op_imm;
  insn_ptr->op[1].valtag.imm.tag = tag_const;
  insn_ptr->op[1].valtag.imm.val = imm;
  insn_ptr->op[1].size = op->size;
  i386_insn_add_implicit_operands_var(insn_ptr, fixed_reg_mapping_t());
  insn_ptr++;
  ASSERT(insn_ptr < insn_end);

  return insn_ptr - insns;
}

static void
i386_operand_convert_reg_to_reg_var(operand_t *op)
{
  switch (op->type) {
    case op_reg:
      op->valtag.reg.tag = tag_var;
      break;
    case op_seg:
      op->valtag.seg.tag = tag_var;
      break;
    default:
      break;
  }
}


static void
i386_iseq_convert_reg_to_reg_var(i386_insn_t *iseq, long iseq_len)
{
  int i, n;

  for (n = 0; n < iseq_len; n++) {
    for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
      i386_operand_convert_reg_to_reg_var(&iseq[n].op[i]);
    }
  }
}


static void
convert_reg_to_regvar(i386_insn_t *insn)
{
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type != op_reg) {
      continue;
    }
    if (insn->op[i].valtag.reg.tag == tag_const) {
      insn->op[i].valtag.reg.tag = tag_var;
    }
  }
}

static void
convert_mem_ds_to_memvar_ds(i386_insn_t *insn, int r_ds)
{
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type != op_mem) {
      continue;
    }
    ASSERT(   insn->op[i].valtag.mem.seg.sel.tag == tag_const
           && insn->op[i].valtag.mem.seg.sel.val == R_DS);
    insn->op[i].valtag.mem.seg.sel.val = r_ds;
    insn->op[i].valtag.mem.seg.sel.tag = tag_var;
    return;
  }
}

static long
i386_insn_put_setcc_mem_var(char const *opc, long addr, int r_ds,
    i386_insn_t *insns, size_t insns_size)
{
  int ret;
  ret = i386_insn_put_setcc_mem(opc, addr, insns, insns_size); 
  ASSERT(ret == 1);
  convert_mem_ds_to_memvar_ds(insns, r_ds);
  return 1;
}

long
i386_insn_put_movl_reg_var_to_mem(long addr, int regvar, int r_ds, i386_insn_t *buf,
    size_t size)
{
  int ret;
  ret = i386_insn_put_movl_reg_to_mem(addr, -1, regvar, buf, size); 
  ASSERT(ret == 1);
  convert_reg_to_regvar(buf);
  convert_mem_ds_to_memvar_ds(buf, r_ds);
  return 1;
}

static long
i386_insn_put_movl_reg_to_reg_var(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_movl_reg_to_reg(reg1, reg2, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

long
i386_insn_put_testw_reg_to_reg_var(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_testw_reg_to_reg(reg1, reg2, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

static long
i386_insn_put_move_operand_to_reg_var(long reg1, operand_t const *op,
    i386_insn_t *insns, size_t insns_size)
{
  int ret;
  ret = i386_insn_put_move_operand_to_reg(reg1, op, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

static long
i386_insn_put_xorl_reg_to_reg_var(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_xorl_reg_to_reg(reg1, reg2, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

static long
i386_insn_put_opcode_mem_to_reg_var(char const *opcode, int regno, long addr,
    int r_ds, int size, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find(opcode);

  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_var;
  insn_ptr[0].op[0].valtag.reg.val = regno;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(size * BYTE_LEN);
  insn_ptr[0].op[0].size = size;

  insn_ptr[0].op[1].type = op_mem;
  //insn_ptr[0].op[1].tag.mem.all = tag_const;
  insn_ptr[0].op[1].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[1].valtag.mem.seg.sel.tag = tag_var;
  insn_ptr[0].op[1].valtag.mem.seg.sel.val = r_ds;
  //insn_ptr[0].op[1].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[1].valtag.mem.base.val = -1;
  insn_ptr[0].op[1].valtag.mem.index.val = -1;
  insn_ptr[0].op[1].valtag.mem.disp.val = addr;
  insn_ptr[0].op[1].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[1].valtag.mem.riprel.tag = tag_invalid;
  insn_ptr[0].op[1].size = size;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_movl_mem_to_reg_var(long regno, long addr, int r_ds, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg_var("movl", regno, addr, r_ds, 4,
      insns, insns_size);
}

static long
i386_insn_put_opcode_imm_to_reg_var(char const *opcode, long regno, long imm,
    int size, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  ASSERT(regno >= 0 && regno < I386_NUM_GPRS);
  insn_ptr[0].opc = opc_nametable_find(opcode);
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.val = regno;
  insn_ptr[0].op[0].valtag.reg.tag = tag_var;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(size * BYTE_LEN);
  insn_ptr[0].op[0].size = size;

  insn_ptr[0].op[1].type = op_imm;
  insn_ptr[0].op[1].valtag.imm.tag = tag_const;
  insn_ptr[0].op[1].valtag.imm.val = imm;
  insn_ptr[0].op[1].size = size;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static long
i386_insn_put_movb_mem_to_reg_var(int regno, long addr, int r_ds,
    i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg_var("movb", regno, addr, r_ds, 1,
      insns, insns_size);
}

static long
i386_insn_put_cmpb_mem_to_reg_var(int regno, long addr, int r_ds, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg_var("cmpb", regno, addr, r_ds, 1,
      insns, insns_size);
}

static long
i386_insn_put_cmpb_imm_to_reg_var(long regno, long imm, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg_var("cmpb", regno, imm, 1, insns, insns_size);
}

long
i386_insn_put_movl_imm_to_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_movl_imm_to_reg(imm, regno, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

static long
i386_insn_put_addl_mem_to_reg_var(int regno, long addr, int r_ds,
    i386_insn_t *insns, size_t insns_size)
{
  int ret;
  ret = i386_insn_put_addl_mem_to_reg(regno, addr, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  convert_mem_ds_to_memvar_ds(insns, r_ds);
  return 1;
}

static long
i386_insn_put_xorw_mem_to_reg_var(long reg1, long addr, int r_ds,
    i386_insn_t *insns, size_t insns_size)
{
  int ret;
  ret = i386_insn_put_xorw_mem_to_reg(reg1, addr, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  convert_mem_ds_to_memvar_ds(insns, r_ds);
  return 1;
}

long
i386_insn_put_cmpw_imm_to_reg_var(long regno, long imm, i386_insn_t *insns,
    size_t insns_size)
{
  // XXX missing base case for recursion
  int ret;
  ret =  i386_insn_put_cmpw_imm_to_reg_var(regno, imm, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

static long
i386_insn_put_testl_reg_to_reg_var(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_testl_reg_to_reg(reg1, reg2, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

static long
i386_insn_put_xorl_mem_to_reg_var(long reg1, long addr, int r_ds,
    i386_insn_t *insns, size_t insns_size)
{
  int ret;
  ret = i386_insn_put_xorl_mem_to_reg(reg1, addr, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  convert_mem_ds_to_memvar_ds(insns, r_ds);
  return 1;
}

long
i386_insn_put_cmpl_imm_to_reg_var(long regno, long imm, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_cmpl_imm_to_reg(regno, imm, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

size_t
i386_insn_put_not_reg_var(int regno, int size, i386_insn_t *insns, size_t insns_size)
{
  long ret;

  ret = i386_insn_put_not_reg(regno, size, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}



static long
i386_insn_put_movw_imm_to_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_movw_imm_to_reg(imm, regno, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

static long
i386_insn_put_cmpw_reg_var_to_mem(long regno, long addr, int r_ds,
    i386_insn_t *insns, size_t insns_size)
{
  int ret;
  ret = i386_insn_put_cmpw_reg_to_mem(regno, addr, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  convert_mem_ds_to_memvar_ds(insns, r_ds);
  return 1;
}

static long
i386_insn_put_cmpl_reg_var_to_mem(long regno, long addr, int r_ds, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_cmpl_reg_to_mem(regno, addr, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  convert_mem_ds_to_memvar_ds(insns, r_ds);
  return 1;
}

long
i386_insn_put_add_reg_to_reg_var(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_add_reg_to_reg(reg1, reg2, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

static long
i386_insn_put_shl_reg_var_by_imm(int regno, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_shl_reg_by_imm(regno, imm, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  return 1;
}

static long
i386_insn_put_add_mem_with_reg_var(int regno, uint32_t addr, int r_ds,
    i386_insn_t *insns, size_t insns_size)
{
  int ret;
  ret = i386_insn_put_add_mem_with_reg(regno, addr, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  convert_mem_ds_to_memvar_ds(insns, r_ds);
  return 1;
}

static long
i386_insn_put_movq_reg_var_to_mem(dst_ulong addr, int regno, int r_ds,
    i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_movq_reg_to_mem(addr, regno, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  convert_mem_ds_to_memvar_ds(insns, r_ds);
  return 1;
}

static long
i386_insn_put_movq_mem_to_reg_var(int regno, long addr, int r_ds, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_movq_mem_to_reg(regno, addr, insns, insns_size);
  ASSERT(ret == 1);
  convert_reg_to_regvar(insns);
  convert_mem_ds_to_memvar_ds(insns, r_ds);
  return 1;
}

long
i386_insn_put_and_imm_with_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg_var("andl", regno, imm, 4, insns, insns_size);
}

static void
i386_insn_replace_imm_operand_with_valtag(i386_insn_t *insn, valtag_t const *vt)
{
  long i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type != op_imm) {
      continue;
    }
    valtag_copy(&insn->op[i].valtag.imm, vt);
    return;
  }
  NOT_REACHED();
}

static long
i386_insn_put_add_imm_valtag_with_reg_var(valtag_t const *vt, int reg, i386_insn_t *buf,
    size_t size)
{
  int ret;
  ret = i386_insn_put_add_imm_with_reg_var(0, reg, buf, size);
  ASSERT(ret == 1);
  i386_insn_replace_imm_operand_with_valtag(buf, vt);
  return 1;
}



static long
i386_insn_put_xorb_reg_high_var_to_reg_low_var(long reg1, long reg2,
    i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("xorb");

  ASSERT(reg1 >= 0 && reg1 < I386_NUM_GPRS);
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_var;
  insn_ptr[0].op[0].valtag.reg.val = reg1;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = 0xff;
  insn_ptr[0].op[0].size = 1;

  ASSERT(reg2 >= 0 && reg2 < I386_NUM_GPRS);
  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.tag = tag_var;
  insn_ptr[0].op[1].valtag.reg.val = reg2;
  insn_ptr[0].op[1].valtag.reg.mod.reg.mask = 0xff00;
  insn_ptr[0].op[1].size = 1;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

/*static void
i386_insn_convert_ss_esp_op0_to_var(i386_insn_t *insn_ptr, int r_esp, int r_ss)
{
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (   insn_ptr[0].op[i].type == op_reg
        && insn_ptr[0].op[i].valtag.reg.tag == tag_const
        && insn_ptr[0].op[i].valtag.reg.val == R_ESP) {
      insn_ptr[0].op[i].valtag.reg.val = r_esp;
      insn_ptr[0].op[i].valtag.reg.tag = tag_var;
    } else if (   insn_ptr[0].op[i].type == op_seg
               && insn_ptr[0].op[i].valtag.seg.tag == tag_const
               && insn_ptr[0].op[i].valtag.seg.val == R_SS) {
      insn_ptr[0].op[i].valtag.seg.val = r_ss;
      insn_ptr[0].op[i].valtag.seg.tag = tag_var;
    }
  }
}*/

static long
i386_insn_put_push_pop_mem_mode_esp_var(uint64_t addr, int r_esp, int r_ss, int r_ds,
    i386_insn_t *insns, size_t insns_size, bool push, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  if (mode == I386_AS_MODE_32) {
    if (push) {
      insn_ptr[0].opc = opc_nametable_find("pushl");
    } else {
      insn_ptr[0].opc = opc_nametable_find("popl");
    }
  } else if (mode >= I386_AS_MODE_64) {
    if (push) {
      insn_ptr[0].opc = opc_nametable_find("pushq");
    } else {
      insn_ptr[0].opc = opc_nametable_find("popq");
    }
  } else NOT_REACHED();

  insn_ptr[0].op[0].type = op_mem;
  insn_ptr[0].op[0].valtag.mem.addrsize = 4;
  insn_ptr[0].op[0].rex_used = 0;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = r_ds;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_var;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = -1;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.index.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].valtag.mem.riprel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].op[0].size = 4;
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].op[0].size = 8;
  } else NOT_REACHED();
  fixed_reg_mapping_t dst_fixed_reg_mapping = fixed_reg_mapping_t::fixed_reg_mapping_for_regs2(I386_EXREG_GROUP_GPRS, R_ESP, r_esp, I386_EXREG_GROUP_SEGS, R_SS, r_ss);
  i386_insn_add_implicit_operands_var(insn_ptr, dst_fixed_reg_mapping);
  //i386_insn_convert_ss_esp_op0_to_var(insn_ptr, r_esp, r_ss);

  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static long
i386_insn_put_push_pop_reg_var_mode_esp_var(long regnum, long r_esp, int r_ss,
    i386_insn_t *insns, size_t insns_size, bool push, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  if (mode == I386_AS_MODE_32) {
    if (push) {
      insn_ptr[0].opc = opc_nametable_find("pushl");
    } else {
      insn_ptr[0].opc = opc_nametable_find("popl");
    }
  } else if (mode >= I386_AS_MODE_64) {
    if (push) {
      insn_ptr[0].opc = opc_nametable_find("pushq");
    } else {
      insn_ptr[0].opc = opc_nametable_find("popq");
    }
  } else NOT_REACHED();
  /*insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].val.reg = R_ESP;
  insn_ptr[0].op[0].tag.reg = tag_const;
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].op[0].size = 4;
  } else if (mode == I386_AS_MODE_64) {
    insn_ptr[0].op[0].size = 8;
  } else NOT_REACHED();*/
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.val = regnum;
  insn_ptr[0].op[0].valtag.reg.tag = tag_var;
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].op[0].size = 4;
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].op[0].size = 8;
  } else NOT_REACHED();
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(insn_ptr[0].op[0].size*8);
  fixed_reg_mapping_t dst_fixed_reg_mapping = fixed_reg_mapping_t::fixed_reg_mapping_for_regs2(I386_EXREG_GROUP_GPRS, R_ESP, r_esp, I386_EXREG_GROUP_SEGS, R_SS, r_ss);
  i386_insn_add_implicit_operands_var(insn_ptr, dst_fixed_reg_mapping);
  //i386_insn_convert_ss_esp_op0_to_var(insn_ptr, r_esp, r_ss);

  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static long
i386_insn_put_pushf_popf_mode_esp_var(int r_esp, int r_ss, i386_insn_t *insns,
    size_t insns_size,
    bool push, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(insn_ptr);
  if (push) {
    if (mode == I386_AS_MODE_32) {
      insn_ptr->opc = opc_nametable_find("pushfl");
    } else if (mode >= I386_AS_MODE_64) {
      insn_ptr->opc = opc_nametable_find("pushfq");
    } else NOT_REACHED();
  } else {
    if (mode == I386_AS_MODE_32) {
      insn_ptr->opc = opc_nametable_find("popfl");
    } else if (mode >= I386_AS_MODE_64) {
      insn_ptr->opc = opc_nametable_find("popfq");
    } else NOT_REACHED();
  }
  /*insn_ptr->op[0].type = op_reg;
  insn_ptr->op[0].val.reg = R_ESP;
  insn_ptr->op[0].tag.reg = tag_const;
  insn_ptr->op[0].size = 4;*/
  fixed_reg_mapping_t dst_fixed_reg_mapping = fixed_reg_mapping_t::fixed_reg_mapping_for_regs2(I386_EXREG_GROUP_GPRS, R_ESP, r_esp, I386_EXREG_GROUP_SEGS, R_SS, r_ss);
  i386_insn_add_implicit_operands_var(insn_ptr, dst_fixed_reg_mapping);
  //i386_insn_convert_ss_esp_op0_to_var(insn_ptr, r_esp, r_ss);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);


  return insn_ptr - insns;
}

static long
i386_insn_put_push_mem64_esp_var(uint64_t addr, int r_esp, int r_ss, int r_ds, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_push_pop_mem_mode_esp_var(addr, r_esp, r_ss, r_ds, insns,
      insns_size,
      true, I386_AS_MODE_64);
}

static long
i386_insn_put_push_reg64_var_esp_var(long regnum, int r_esp, int r_ss,
    i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_push_pop_reg_var_mode_esp_var(regnum, r_esp, r_ss,
      insns, insns_size, true, I386_AS_MODE_64);
}

static long
i386_insn_put_pop_reg64_var_esp_var(long regnum, int r_esp, int r_ss,
    i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_push_pop_reg_var_mode_esp_var(regnum, r_esp, r_ss,
      insns, insns_size,
      false, I386_AS_MODE_64);
}

static long
i386_insn_put_pushf64_esp_var(int r_esp, int r_ss,
    i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_pushf_popf_mode_esp_var(r_esp, r_ss, insns, insns_size, true,
      I386_AS_MODE_64);
}

static long
i386_insn_put_popf64_esp_var(int r_esp, int r_ss, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_pushf_popf_mode_esp_var(r_esp, r_ss, insns, insns_size, false,
      I386_AS_MODE_64);
}

static long
i386_insn_put_push_addr_to_stack(uint64_t addr, regmap_t const *c2v,
    /*regset_t *ctemps, */i386_insn_t *buf, size_t size)
{
  i386_insn_t *ptr, *end;
  int r_esp, r_ss, r_ds;

  r_esp = regmap_get_elem(c2v, I386_EXREG_GROUP_GPRS, R_ESP).reg_identifier_get_id();
  r_ss = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_SS).reg_identifier_get_id();
  r_ds = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id();

  //ctemps->excregs[I386_EXREG_GROUP_SEGS][r_ss] = 0xffff;
  //ctemps->excregs[I386_EXREG_GROUP_SEGS][r_ds] = 0xffff;
  //ctemps->excregs[I386_EXREG_GROUP_GPRS][r_esp] = 0xffffffff;

  ptr = buf;
  end = buf + size;

  //ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(1), r_esp, r_ds,
  //    ptr, end - ptr);
  //ptr += i386_insn_put_movq_mem_to_reg_var(r_esp, SRC_ENV_SCRATCH(0), r_ds,
  //    ptr, end - ptr);
  ptr += i386_insn_put_push_mem64_esp_var(addr, r_esp, r_ss, r_ds, ptr, end - ptr);
  //ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(0), r_esp, r_ds,
  //    ptr, end - ptr);
  //ptr += i386_insn_put_movq_mem_to_reg_var(r_esp, SRC_ENV_SCRATCH(1), r_ds,
  //    ptr, end - ptr);

  return ptr - buf;
}

static long
i386_insn_put_push_pop_tmpreg_to_stack(int tmpreg, regmap_t const *c2v,
    /*regset_t *ctemps, */i386_insn_t *buf, size_t size, bool push)
{
  i386_insn_t *ptr, *end;
  int r_esp, r_ss, r_ds;

  r_esp = regmap_get_elem(c2v, I386_EXREG_GROUP_GPRS, R_ESP).reg_identifier_get_id();
  r_ss = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_SS).reg_identifier_get_id();
  r_ds = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id();

  //ctemps->excregs[I386_EXREG_GROUP_SEGS][r_ss] = 0xffff;
  //ctemps->excregs[I386_EXREG_GROUP_SEGS][r_ds] = 0xffff;
  //ctemps->excregs[I386_EXREG_GROUP_GPRS][r_esp] = 0xffffffff;

  ptr = buf;
  end = buf + size;

  //ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(1), r_esp, r_ds,
  //    ptr, end - ptr);
  //ptr += i386_insn_put_movq_mem_to_reg_var(r_esp, SRC_ENV_SCRATCH(0), r_ds,
  //    ptr, end - ptr);
  if (push) {
    ptr += i386_insn_put_push_reg64_var_esp_var(tmpreg, r_esp, r_ss, ptr, end - ptr);
  } else {
    ptr += i386_insn_put_pop_reg64_var_esp_var(tmpreg, r_esp, r_ss, ptr, end - ptr);
  }
  //ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(0), r_esp, r_ds,
  //    ptr, end - ptr);
  //ptr += i386_insn_put_movq_mem_to_reg_var(r_esp, SRC_ENV_SCRATCH(1), r_ds,
  //    ptr, end - ptr);

  return ptr - buf;
}

static long
i386_insn_put_push_pop_flags_to_stack(regmap_t const *c2v,
    /*regset_t *ctemps, */i386_insn_t *buf, size_t size, bool push)
{
  i386_insn_t *ptr, *end;
  int r_esp, r_ss, r_ds;

  r_esp = regmap_get_elem(c2v, I386_EXREG_GROUP_GPRS, R_ESP).reg_identifier_get_id();
  r_ss = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_SS).reg_identifier_get_id();
  r_ds = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id();

  //ctemps->excregs[I386_EXREG_GROUP_SEGS][r_ss] = 0xffff;
  //ctemps->excregs[I386_EXREG_GROUP_SEGS][r_ds] = 0xffff;
  //ctemps->excregs[I386_EXREG_GROUP_GPRS][r_esp] = 0xffffffff;

  ptr = buf;
  end = buf + size;

  //ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(1), r_esp, r_ds,
  //    ptr, end - ptr);
  //ptr += i386_insn_put_movq_mem_to_reg_var(r_esp, SRC_ENV_SCRATCH(0), r_ds,
  //    ptr, end - ptr);
  if (push) {
    ptr += i386_insn_put_pushf64_esp_var(r_esp, r_ss, ptr, end - ptr);
  } else {
    ptr += i386_insn_put_popf64_esp_var(r_esp, r_ss, ptr, end - ptr);
  }
  //ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(0), r_esp, r_ds,
  //    ptr, end - ptr);
  //ptr += i386_insn_put_movq_mem_to_reg_var(r_esp, SRC_ENV_SCRATCH(1), r_ds,
  //    ptr, end - ptr);

  return ptr - buf;
}

static long
i386_insn_put_push_flags_to_stack(regmap_t const *c2v/*, regset_t *ctemps*/,
    i386_insn_t *buf, size_t size)
{
  return i386_insn_put_push_pop_flags_to_stack(c2v/*, ctemps*/, buf, size, true);
}

static long
i386_insn_put_pop_flags_to_stack(regmap_t const *c2v/*, regset_t *ctemps*/,
    i386_insn_t *buf, size_t size)
{
  return i386_insn_put_push_pop_flags_to_stack(c2v/*, ctemps*/, buf, size, false);
}

static long
i386_insn_put_push_tmpreg_to_stack(int tmpreg, regmap_t const *c2v,
    /*regset_t *ctemps, */i386_insn_t *buf, size_t size)
{
  return i386_insn_put_push_pop_tmpreg_to_stack(tmpreg, c2v/*, ctemps*/,
      buf, size, true);
}

static long
i386_insn_put_pop_tmpreg_to_stack(int tmpreg, regmap_t const *c2v,
    /*regset_t *ctemps, */i386_insn_t *buf, size_t size)
{
  return i386_insn_put_push_pop_tmpreg_to_stack(tmpreg, c2v/*, ctemps*/,
      buf, size, false);
}

static long
i386_insn_put_push_tmpreg_and_flags_to_stack(int tmpreg, regmap_t const *c2v,
    /*regset_t *ctemps, */i386_insn_t *buf, size_t size)
{
  i386_insn_t *ptr, *end;

  ptr = buf;
  end = buf + size;
  ptr += i386_insn_put_push_pop_tmpreg_to_stack(tmpreg, c2v/*, ctemps*/,
      ptr, end - ptr, true);
  ptr += i386_insn_put_push_pop_flags_to_stack(c2v/*, ctemps*/, ptr, end - ptr, true);
  return ptr - buf;
}

static long
i386_insn_put_pop_tmpreg_and_flags_to_stack(int tmpreg, regmap_t const *c2v,
    /*regset_t *ctemps, */i386_insn_t *buf, size_t size)
{
  i386_insn_t *ptr, *end;

  ptr = buf;
  end = buf + size;

  ptr += i386_insn_put_push_pop_flags_to_stack(c2v/*, ctemps*/, ptr, end - ptr, false);
  ptr += i386_insn_put_push_pop_tmpreg_to_stack(tmpreg, c2v/*, ctemps*/, ptr, end - ptr,
      false);
  return ptr - buf;
}

class excp_jcc_insn_t {
public:
  i386_insn_t *ptr;
  char const *opc;
  int num_tmpreg_pops;
};

static long
i386_insn_put_jcc_to_set_env_exception(char const *opcname, int num_pop, excp_jcc_insn_t *jcc_insns, size_t &num_jcc_insns, long max_jcc_insns, i386_insn_t *buf, size_t buf_size)
{
  i386_insn_t *ptr = buf, *end = buf + buf_size;

  jcc_insns[num_jcc_insns].ptr = ptr;
  jcc_insns[num_jcc_insns].opc = opcname;
  jcc_insns[num_jcc_insns].num_tmpreg_pops = num_pop;
  num_jcc_insns++;
  ptr += i386_insn_put_jcc_to_pcrel(jcc_insns[num_jcc_insns - 1].opc, 0, ptr, end - ptr);
  return ptr - buf;
}

#define MAX_JCC_INSNS 16

static long
i386_insn_avoid_fp_exceptions_and_handle_control_flow(i386_insn_t *dst, size_t size,
    long insn_index, int tmpreg, regmap_t const *c2v, excp_jcc_insn_t *jcc_insns, size_t &num_jcc_insns)
{
  DBG3(EQUIV, "input:\n%s\n", i386_iseq_to_string(dst, 1, as1, sizeof as1));
  DBG3(EQUIV, "insn_index = %ld\n", insn_index);
  int r_eax, r_edx, r_esp, r_al, r_dl, r_ax, r_dx, r_ah, r_dh, r_ds, r_ss;
  i386_insn_t src, *ptr, *end;
  operand_t div_op, spill_op;
  char const *remaining;
  //int tmpreg1, tmpreg2;
  //size_t num_jcc_insns;
  dst_ulong spill_slot;
  bool use_spill_slot;
  uint64_t minmax;
  valtag_t *pcrel;
  char const *opc;
  bool is_signed;
  long i;

  //tmpreg1 = R_ECX;
  //tmpreg2 = R_EBX;
  i386_insn_copy(&src, dst);
  opc = opctable_name(src.opc);

  r_eax = regmap_get_elem(c2v, I386_EXREG_GROUP_GPRS, R_EAX).reg_identifier_get_id();
  r_ax = r_al = r_ah = r_eax;
  r_edx = regmap_get_elem(c2v, I386_EXREG_GROUP_GPRS, R_EDX).reg_identifier_get_id();
  r_dx = r_dl = r_dh = r_edx;
  r_esp = regmap_get_elem(c2v, I386_EXREG_GROUP_GPRS, R_ESP).reg_identifier_get_id();
  r_ds = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id();
  r_ss = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_SS).reg_identifier_get_id();

  DBG3(EQUIV, "src:\n%s\n", i386_insn_to_string(&src, as1, sizeof as1));

  ptr = dst;
  end = dst + size;
  //num_jcc_insns = 0;

  if (   strstart(opc, "div", NULL)
      || strstart(opc, "idiv", NULL)) {
    ptr += i386_insn_put_push_flags_to_stack(c2v/*, ctemps*/, ptr, end - ptr);
    //ptr += i386_insn_put_push_tmpreg_to_scratch0_esp(tmpreg2, ptr);
    //ASSERT(src.op[0].type == op_flags_unsigned);
    //ASSERT(src.op[1].type == op_flags_signed);
    size_t num_flags = i386_insn_count_num_implicit_flag_regs(&src);

    //i386_operand_copy(&div_op, &src.op[4]);
    i386_operand_copy(&div_op, &src.op[num_flags + 2]);

    /* check div-by-zero */
    ptr += i386_insn_put_compare_with_imm(&div_op, 0, ptr, end - ptr);

    ptr += i386_insn_put_jcc_to_set_env_exception("je", 0, jcc_insns, num_jcc_insns, MAX_JCC_INSNS, ptr, end - ptr);

    /* check INT_MIN divided by -1 */
    ptr += i386_insn_put_compare_with_imm(&div_op, -1, ptr, end - ptr);
    i386_insn_t *divisor_not_minus_one_jcc_ptr_start = ptr;
    ptr += i386_insn_put_jcc_to_pcrel("jne", 0, ptr, end - ptr);
    i386_insn_t *divisor_not_minus_one_jcc_ptr_stop = ptr;

    size_t num_flag_regs = i386_insn_count_num_implicit_flag_regs(&src);
    ptr += i386_insn_put_compare_with_imm(&src.op[num_flag_regs + 0], -1, ptr, end - ptr);
    i386_insn_t *dividend_msb_not_minus_one_jcc_ptr_start = ptr;
    ptr += i386_insn_put_jcc_to_pcrel("jne", 0, ptr, end - ptr);
    i386_insn_t *dividend_msb_not_minus_one_jcc_ptr_stop = ptr;

    ptr += i386_insn_put_compare_with_imm(&src.op[num_flag_regs + 1], INT_MIN, ptr, end - ptr);
    i386_insn_t *dividend_lsb_not_int_min_jcc_ptr_start = ptr;
    ptr += i386_insn_put_jcc_to_pcrel("jne", 0, ptr, end - ptr);
    i386_insn_t *dividend_lsb_not_int_min_jcc_ptr_stop = ptr;

    ptr += i386_insn_put_jcc_to_set_env_exception("je", 0, jcc_insns, num_jcc_insns, MAX_JCC_INSNS, ptr, end - ptr);

    i386_insn_put_jcc_to_pcrel("jne", ptr - divisor_not_minus_one_jcc_ptr_stop,
        divisor_not_minus_one_jcc_ptr_start, divisor_not_minus_one_jcc_ptr_stop - divisor_not_minus_one_jcc_ptr_start);
    i386_insn_put_jcc_to_pcrel("jne", ptr - dividend_msb_not_minus_one_jcc_ptr_stop,
        dividend_msb_not_minus_one_jcc_ptr_start, dividend_msb_not_minus_one_jcc_ptr_stop - dividend_msb_not_minus_one_jcc_ptr_start);
    i386_insn_put_jcc_to_pcrel("jne", ptr - dividend_lsb_not_int_min_jcc_ptr_stop,
        dividend_lsb_not_int_min_jcc_ptr_start, dividend_lsb_not_int_min_jcc_ptr_stop - dividend_lsb_not_int_min_jcc_ptr_start);

    //ctemps->excregs[I386_EXREG_GROUP_GPRS][r_eax] = 0xffffffff;
    //ctemps->excregs[I386_EXREG_GROUP_GPRS][r_edx] = 0xffffffff;
    //ctemps->excregs[I386_EXREG_GROUP_SEGS][r_ds] = 0xffff;

/* commenting this out because this is incorrect logic; in any case we are currently supporting only 32-bit dividends and divisors, and so underflow/overflow issues do not even arise; for 64-bit dividends, should use hand-written calls to __divdi3 just like what gcc does. also warning: the logic to check overflow/underflow below is buggy --- have seen incorrect declaration of exceptions
    // check overflow / underflow
    ptr += i386_insn_put_movl_reg_var_to_mem(SRC_ENV_SCRATCH(2), r_eax, r_ds,
        ptr, end - ptr);
    ptr += i386_insn_put_movl_reg_var_to_mem(SRC_ENV_SCRATCH(3), r_edx, r_ds,
        ptr, end - ptr);
    //ptr += i386_insn_put_movl_reg_to_reg(tmpreg1, r_eax, ptr);
    //ptr += i386_insn_put_movl_reg_to_reg(tmpreg2, r_edx, ptr);
    //store max allowable quotient (max absolute value) in eax/ax/al
    if (strstart(opc, "idivb", NULL)) {
      div_op.size = 1;
      ptr += i386_insn_put_testw_reg_to_reg_var(r_ax, r_ax, ptr, end - ptr);
      ptr += i386_insn_put_setcc_mem("setl", SRC_ENV_SCRATCH(4), ptr, end - ptr);
      ptr += i386_insn_put_move_operand_to_reg_var(r_dl, &div_op, ptr, end - ptr);
      ptr += i386_insn_put_xorb_reg_high_var_to_reg_low_var(r_dl, r_ah, ptr, end - ptr);
      ptr += i386_insn_put_cmpb_imm_to_reg_var(r_dl, 0, ptr, end - ptr);
      ptr += i386_insn_put_movl_imm_to_reg_var(0x7f, r_eax, ptr, end - ptr);
      ptr += i386_insn_put_setcc_mem("seto", SRC_ENV_SCRATCH(5), ptr, end - ptr);
      ptr += i386_insn_put_addl_mem_to_reg_var(r_eax, SRC_ENV_SCRATCH(5), r_ds,
          ptr, end - ptr);
      is_signed = true;
    } else if (strstart(opc, "idivw", NULL)) {
      div_op.size = 2;
      ptr += i386_insn_put_testw_reg_to_reg_var(r_dx, r_dx, ptr, end - ptr);
      ptr += i386_insn_put_setcc_mem("setl", SRC_ENV_SCRATCH(4), ptr, end - ptr);
      ptr += i386_insn_put_move_operand_to_reg_var(r_dx, &div_op, ptr, end - ptr);
      ptr += i386_insn_put_xorw_mem_to_reg_var(r_dx, SRC_ENV_SCRATCH(3), r_ds,
          ptr, end - ptr);
      ptr += i386_insn_put_cmpw_imm_to_reg_var(r_dx, 0, ptr, end - ptr);
      ptr += i386_insn_put_movl_imm_to_reg_var(0x7fff, r_eax, ptr, end - ptr);
      ptr += i386_insn_put_setcc_mem("seto", SRC_ENV_SCRATCH(5), ptr, end - ptr);
      ptr += i386_insn_put_addl_mem_to_reg_var(r_eax, SRC_ENV_SCRATCH(5), r_ds,
          ptr, end - ptr);
      is_signed = true;
    } else if (strstart(opc, "idivl", NULL)) {
      div_op.size = 4;
      ptr += i386_insn_put_testl_reg_to_reg_var(r_edx, r_edx, ptr, end - ptr);
      ptr += i386_insn_put_setcc_mem("setl", SRC_ENV_SCRATCH(4), ptr, end - ptr);
      ptr += i386_insn_put_move_operand_to_reg_var(r_edx, &div_op, ptr, end - ptr);
      ptr += i386_insn_put_xorl_mem_to_reg_var(r_edx, SRC_ENV_SCRATCH(3), r_ds,
          ptr, end - ptr);
      ptr += i386_insn_put_cmpl_imm_to_reg_var(r_edx, 0, ptr, end - ptr);
      ptr += i386_insn_put_movl_imm_to_reg_var(0x7fffffff, r_eax, ptr, end - ptr);
      ptr += i386_insn_put_setcc_mem("seto", SRC_ENV_SCRATCH(5), ptr, end - ptr);
      ptr += i386_insn_put_addl_mem_to_reg_var(r_eax, SRC_ENV_SCRATCH(5), r_ds,
          ptr, end - ptr);
      is_signed = true;
    } else if (strstart(opc, "divb", NULL)) {
      div_op.size = 1;
      ptr += i386_insn_put_movb_imm_to_mem(0, SRC_ENV_SCRATCH(4), ptr, end - ptr);
      ptr += i386_insn_put_movl_imm_to_reg_var(0xff, r_eax, ptr, end - ptr);
      is_signed = false;
    } else if (strstart(opc, "divw", NULL)) {
      div_op.size = 2;
      ptr += i386_insn_put_movw_imm_to_mem(0, SRC_ENV_SCRATCH(4), ptr, end - ptr);
      ptr += i386_insn_put_movw_imm_to_reg_var(0xffff, r_eax, ptr, end - ptr);
      is_signed = false;
    } else if (strstart(opc, "divl", NULL)) {
      div_op.size = 4;
      ptr += i386_insn_put_movl_imm_to_mem(0, SRC_ENV_SCRATCH(4), ptr, end - ptr);
      ptr += i386_insn_put_movl_imm_to_reg_var(0xffffffff, r_eax, ptr, end - ptr);
      is_signed = false;
    } else NOT_REACHED();

    use_spill_slot = false;
    if (div_op.type == op_reg && div_op.valtag.reg.val == r_eax) {
      use_spill_slot = true;
      spill_slot = SRC_ENV_SCRATCH(2);
    } else if (div_op.type == op_reg && div_op.valtag.reg.val == r_edx) {
      use_spill_slot = true;
      spill_slot = SRC_ENV_SCRATCH(3);
    } else if (   div_op.type == op_reg
               && div_op.size == 1
               && div_op.valtag.reg.val == r_eax
               && div_op.valtag.reg.mod.reg.mask == 0xff00) {
      use_spill_slot = true;
      spill_slot = SRC_ENV_SCRATCH(2) + 1;
    } else if (   div_op.type == op_reg
               && div_op.size == 1
               && div_op.valtag.reg.val == r_eax
               && div_op.valtag.reg.mod.reg.mask == 0xff00) {
      use_spill_slot = true;
      spill_slot = SRC_ENV_SCRATCH(3) + 1;
    }
    if (!use_spill_slot) {
      ptr += i386_insn_put_mul_var_with_operand(&div_op, is_signed, r_eax, r_edx, ptr, end - ptr);
    } else {
      spill_op.type = op_mem;
      //spill_op.tag.mem.all = tag_const;
      spill_op.valtag.mem.segtype = segtype_sel;
      spill_op.valtag.mem.seg.sel.tag = tag_const;
      spill_op.valtag.mem.seg.sel.val = r_ds;
      //spill_op.valtag.mem.seg.sel.mod.reg.mask = 0xffff;
      spill_op.valtag.mem.base.val = -1;
      spill_op.valtag.mem.index.val = -1;
      spill_op.valtag.mem.disp.val = spill_slot;
      spill_op.size = div_op.size;

      ptr += i386_insn_put_mul_var_with_operand(&spill_op, is_signed, r_eax, r_edx, ptr, end - ptr);
    }
    if (div_op.size == 1) {
      ptr += i386_insn_put_cmpw_reg_var_to_mem(r_ax, SRC_ENV_SCRATCH(2), r_ds,
          ptr, end - ptr);
    } else if (div_op.size == 2) {
      ptr += i386_insn_put_cmpw_reg_var_to_mem(r_dx, SRC_ENV_SCRATCH(3), r_ds,
          ptr, end - ptr);
    } else if (div_op.size == 4) {                        
      ptr += i386_insn_put_cmpl_reg_var_to_mem(r_edx, SRC_ENV_SCRATCH(3), r_ds,
          ptr, end - ptr);
    } else NOT_REACHED();
    // sign-bit of difference = sign-bit of tmpreg2, that
    // indicates underflow/overflow.
    if (is_signed) {
      ptr += i386_insn_put_movl_reg_var_to_mem(SRC_ENV_SCRATCH(7), r_edx, r_ds,
          ptr, end - ptr);
      ptr += i386_insn_put_setcc_mem_var("setl", SRC_ENV_SCRATCH(5), r_ds,
          ptr, end - ptr);
    } else {
      ptr += i386_insn_put_setcc_mem_var("setb", SRC_ENV_SCRATCH(5), r_ds,
          ptr, end - ptr);
    }
    ptr += i386_insn_put_movb_mem_to_reg_var(r_al, SRC_ENV_SCRATCH(4), r_ds,
        ptr, end - ptr);
    ptr += i386_insn_put_cmpb_mem_to_reg_var(r_al, SRC_ENV_SCRATCH(5), r_ds,
        ptr, end - ptr);

    //ptr += i386_insn_put_movl_reg_to_reg(r_eax, tmpreg1, ptr);
    //ptr += i386_insn_put_movl_reg_to_reg(r_edx, tmpreg2, ptr);
    ptr += i386_insn_put_movl_mem_to_reg_var(r_eax, SRC_ENV_SCRATCH(2), r_ds,
        ptr, end - ptr);
    ptr += i386_insn_put_movl_mem_to_reg_var(r_edx, SRC_ENV_SCRATCH(3), r_ds,
        ptr, end - ptr);

    ptr += i386_insn_put_jcc_to_set_env_exception("je", ptr_jcc_insns, num_jcc_insns, jcc_opcs, MAX_JCC_INSNS, ptr, end - ptr);

    // restore tmpreg2, tmpreg1, flags
    //ptr += i386_insn_put_pop_tmpreg_to_scratch0_esp(tmpreg2, ptr);
*/
    ptr += i386_insn_put_pop_flags_to_stack(c2v/*, ctemps*/, ptr, end - ptr);
    i386_insn_copy(ptr, &src);
    ptr++;
  } else if (i386_insn_is_indirect_branch(&src)) {
    //XXX: move the first non-implicit operand to indir_nextpc in state.
    NOT_IMPLEMENTED();
  } else if (i386_insn_accesses_stack(&src)) {
    //XXX: need to convert 32-bit stack accesses to 64-bit mode
    //NOT_IMPLEMENTED();
    if (strstart(opc, "push", &remaining)) {
      int opsize;
      if (!strcmp(remaining, "l")) {
        opsize = 4;
      } else if (!strcmp(remaining, "w")) {
        opsize = 2;
      } else NOT_IMPLEMENTED();
      //tmpreg must have been already loaded with the decremented (and sandboxed) value; but sandboxing in case of stack is just a bounds-check
      //ptr += i386_insn_put_push_reg64_var_esp_var(r_eax, r_esp, r_ss, ptr, end - ptr);
      //ptr += i386_insn_put_movl_mem_to_reg_var(r_eax, src_env_esp_addr, r_ds, ptr, end - ptr);
      //ptr += i386_insn_put_movl_op_to_regmem_offset(r_eax, r_ds, tag_var, 0,
      //    &src.op[src.num_implicit_ops], opsize, SRC_ENV_SCRATCH(2), ptr, end - ptr);
      ptr += i386_insn_put_movl_op_to_regmem_offset(tmpreg, r_ds, tag_var, 0,
          &src.op[src.num_implicit_ops], opsize, SRC_ENV_SCRATCH(2), ptr, end - ptr);
      //ptr += i386_insn_put_pop_reg64_var_esp_var(r_eax, r_esp, r_ss, ptr, end - ptr);
      //ASSERT(ptr <= end);
      //ptr += i386_insn_put_add_imm_with_reg_var(-opsize, r_esp, ptr, end - ptr);
      //ASSERT(ptr <= end);
    } else if (   strstart(opc, "pop", &remaining)
               && !strstart(opc, "popcnt", NULL)) {
      int opsize;
      if (!strcmp(remaining, "l")) {
        opsize = 4;
      } else if (!strcmp(remaining, "w")) {
        opsize = 2;
      } else NOT_IMPLEMENTED();
      //ptr += i386_insn_put_push_reg64_var_esp_var(r_eax, r_esp, r_ss, ptr, end - ptr);
      //ptr += i386_insn_put_movl_mem_to_reg_var(r_eax, src_env_esp_addr, r_ds, ptr, end - ptr);
      //ptr += i386_insn_put_movl_regmem_offset_to_op(&src.op[src.num_implicit_ops],
      //    r_eax, r_ds, tag_var, 0, opsize, SRC_ENV_SCRATCH(2), ptr, end - ptr);
      //ptr += i386_insn_put_pop_reg64_var_esp_var(r_eax, r_esp, r_ss, ptr, end - ptr);
      //ASSERT(ptr <= end);
      //ptr += i386_insn_put_add_imm_with_reg_var(opsize, r_esp, ptr, end - ptr);
      //ASSERT(ptr <= end);

      //ptr += i386_insn_put_movl_regmem_offset_to_op(&src.op[src.num_implicit_ops],
      //    r_esp, r_ss, tag_var, 0, opsize, SRC_ENV_SCRATCH(2), ptr, end - ptr);
      ptr += i386_insn_put_movl_regmem_offset_to_op(&src.op[src.num_implicit_ops],
          tmpreg, r_ds, tag_var, 0, opsize, SRC_ENV_SCRATCH(2), ptr, end - ptr);
      ASSERT(ptr <= end);
      //ptr += i386_insn_put_add_imm_with_reg_var(opsize, r_esp, ptr, end - ptr);
      ptr += i386_insn_put_add_imm_with_reg_var(opsize, tmpreg, ptr, end - ptr);
      ASSERT(ptr <= end);
    } else {
      NOT_IMPLEMENTED();
    }
  } else if (strstart(opc, "lea", NULL)) {
    /* some offsets in leal insns are interpreted differently (due to signedness issues)
       in 64-bit mode. so convert to addl */
    long disp, reg;
    ASSERT(src.op[1].type == op_mem);
    disp = ptr->op[1].valtag.mem.disp.val;
    ASSERT(src.op[0].type == op_reg);
    reg = ptr->op[0].valtag.reg.val;
    i386_insn_copy(ptr, &src);
    ptr->op[1].valtag.mem.disp.val = 0;
    ptr++;
    ASSERT(ptr < end);

    ptr += i386_insn_put_push_flags_to_stack(c2v/*, ctemps*/, ptr, end - ptr);
    ptr += i386_insn_put_add_imm_with_reg_var(disp, reg, ptr, end - ptr);
    ptr += i386_insn_put_pop_flags_to_stack(c2v/*, ctemps*/, ptr, end - ptr);
  } else if (   !strcmp(opc, "daa")
             || !strcmp(opc, "das")
             || (!strcmp(opc, "movw") && src.op[0].type == op_seg)
             || (!strcmp(opc, "movw") && src.op[1].type == op_seg)) {
    /* XXX: these instructions are not supported in 64-bit mode */
    NOT_IMPLEMENTED();
  } else if (!strcmp(opc, "rdpmc")) {
    /* XXX : not supported in user mode */
    NOT_IMPLEMENTED();
  } else if (!strcmp(opc, "rdtsc")) {
    /* XXX : gives non-deterministic output */
    NOT_IMPLEMENTED();
  } else if (   !strcmp(opc, "hlt")
             || !strcmp(opc, "cli")
             || !strcmp(opc, "sti")
             || !strcmp(opc, "int")) {
    //ignore
  } else if (pcrel = i386_insn_get_pcrel_operand(&src)) {
    valtag_t *dst_pcrel;
    if (pcrel->tag == tag_reloc_string) {
      NOT_REACHED();
    } else if (pcrel->tag == tag_var) {
      i386_insn_copy(ptr, &src);
      ptr++;
    } else if (pcrel->tag == tag_const) {
      i386_insn_copy(ptr, &src);
      dst_pcrel = i386_insn_get_pcrel_operand(ptr);
      ASSERT(dst_pcrel);
      dst_pcrel->tag = tag_sboxed_abs_inum;
      dst_pcrel->val += insn_index + 1;
      ptr++;
    } else NOT_REACHED();
  } else {
    i386_insn_copy(ptr, &src);
    ptr++;
  }

  DBG3(EQUIV, "returning:\n%s\n", i386_iseq_to_string(dst, ptr - dst, as1, sizeof as1));
  return ptr - dst;
}

typedef enum { NO_ACTION, INCREMENT, DECREMENT } post_sandbox_action_t;

static bool
i386_operand_accesses_memory(i386_insn_t const *insn, int opnum,
    bool *fixed, operand_t *op, post_sandbox_action_t *psa)
{
  char const *opc, *suffix, *isuffix;

  opc = opctable_name(insn->opc);
  if (   strstart(opc, "push", NULL)
      /*|| strstart(opc, "call", NULL)
      || strstart(opc, "enter", NULL)*/) {
    if (   opnum < insn->num_implicit_ops
        && insn->op[opnum].type == op_reg
        /*&& insn->op[opnum].valtag.reg.val == R_ESP
        && insn->op[opnum].valtag.reg.tag == tag_const*/) {
      op->type = op_mem;
      //op->valtag.mem.segtype = segtype_sel;
      //op->valtag.mem.seg.sel = R_SS;
      op->valtag.mem.base.val = insn->op[opnum].valtag.reg.val; //R_ESP;
      op->valtag.mem.base.tag = insn->op[opnum].valtag.reg.tag; //tag_const;
      op->valtag.mem.index.val = -1;
      op->valtag.mem.index.tag = tag_const;
      op->valtag.mem.disp.val = -4;
      op->valtag.mem.disp.tag = tag_const;
      op->size = 4;
      //*fixed = true;
      *fixed = false; //because we will replace it with move instruction
      *psa = DECREMENT;
      return true;
    }
  } else if (   (strstart(opc, "pop", NULL) && !strstart(opc, "popcnt", NULL))
             /*|| strstart(opc, "ret", NULL)
             || strstart(opc, "leave", NULL)*/) {
    if (   opnum < insn->num_implicit_ops
        && insn->op[opnum].type == op_reg
        /*&& insn->op[opnum].valtag.reg.val == R_ESP
        && insn->op[opnum].valtag.reg.tag == tag_const*/) {
      op->type = op_mem;
      //op->valtag.mem.segtype = segtype_sel;
      //op->valtag.mem.seg.sel = R_SS;
      op->valtag.mem.base.val = insn->op[opnum].valtag.reg.val; //R_ESP;
      op->valtag.mem.base.tag = insn->op[opnum].valtag.reg.tag; //tag_const;
      op->valtag.mem.index.val = -1;
      op->valtag.mem.index.tag = tag_const;
      op->valtag.mem.disp.val = 0;
      op->valtag.mem.disp.tag = tag_const;
      op->size = 4;
      //*fixed = true;
      *fixed = false;
      *psa = INCREMENT;
      return true;
    }
  } else if (   strstart(opc, "cmps", &suffix)
             || strstart(opc, "stos", &suffix)
             || strstart(opc, "lods", &suffix)
             || strstart(opc, "scas", &suffix)
             || !strcmp(opc, "movsb")
             || !strcmp(opc, "movsw")
             || !strcmp(opc, "movsl")
             || !strcmp(opc, "insb")
             || !strcmp(opc, "insw")
             || !strcmp(opc, "insl")
             || !strcmp(opc, "outsb")
             || !strcmp(opc, "outsw")
             || !strcmp(opc, "outsl")) {
    if (   strstart(opc, "movs", &isuffix)
        || strstart(opc, "ins", &isuffix)
        || strstart(opc, "outs", &isuffix)) {
      suffix = isuffix;
    }
    if (insn->op[opnum].type == op_mem) {
      i386_operand_copy(op, &insn->op[opnum]);
      if (suffix[0] == 'b') {
        op->size = 1;
      } else if (suffix[0] == 'w') {
        op->size = 2;
      } else if (suffix[0] == 'l') {
        op->size = 4;
      } else NOT_REACHED();
      *fixed = true;
      *psa = INCREMENT;
      return true;
    }
  } else if (   opnum >= insn->num_implicit_ops  /* explicit operand */
      && insn->op[opnum].type == op_mem
      && !strstart(opc, "lea", NULL)
      && !memop_is_src_env_access(&insn->op[opnum])) {
    *fixed = false;
    i386_operand_copy(op, &insn->op[opnum]);
    *psa = NO_ACTION;
    DBG_EXEC3(EQUIV,
        operand2str(op, NULL, 0, I386_AS_MODE_32, as1, sizeof as1);
        printf("%s() %d: returning true for %s\n", __func__, __LINE__, as1);
    );
    return true;
  }

  return false;
}

static long
i386_insn_sandbox_memop(operand_t &memop, bool membase_is_fixed, int tmpreg, bool &tmpreg_used, regmap_t const *c2v, bool &is_stack_access,
    excp_jcc_insn_t *jcc_insns, size_t &num_jcc_insns, i386_insn_t *buf, size_t buf_size)
{
  //at the end of this, the top of the stack (pushed value) will contain the original contents of the register that contains the sandboxed address (tmpreg if membase_is_fixed==false)
  i386_insn_t *ptr = buf, *end = buf + buf_size;
  is_stack_access = false;
  int shift, i;

  int r_ds = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id();
  int r_esp = regmap_get_elem(c2v, I386_EXREG_GROUP_GPRS, R_ESP).reg_identifier_get_id();
  dst_ulong src_env_esp_addr = SRC_ENV_ADDR + I386_ENV_EXOFF(I386_EXREG_GROUP_GPRS, r_esp);

  ptr += i386_insn_put_push_tmpreg_and_flags_to_stack(tmpreg, c2v/*, ctemps*/,
      ptr, end - ptr);

  /* XXX: shouldn't we simply use lea to load the address into tmpreg? */
  ptr += i386_insn_put_xorl_reg_to_reg_var(tmpreg, tmpreg, ptr, end - ptr);
  if (memop.valtag.mem.index.val != -1) {
    ASSERT(memop.valtag.mem.index.tag == tag_var);
    ASSERT(   memop.valtag.mem.index.val >= 0
           && memop.valtag.mem.index.val < I386_NUM_GPRS);
    if (r_esp == memop.valtag.mem.index.val) {
      is_stack_access = true;
      ASSERT(memop.valtag.mem.scale == 1);
      ptr += i386_insn_put_addl_mem_to_reg_var(tmpreg, src_env_esp_addr, r_ds, ptr, end - ptr);
    } else {
      ptr += i386_insn_put_add_reg_to_reg_var(tmpreg, memop.valtag.mem.index.val, ptr,
          end - ptr);
    }
    switch (memop.valtag.mem.scale) {
      case 1: shift = 0; break;
      case 2: shift = 1; break;
      case 4: shift = 2; break;
      case 8: shift = 3; break;
      default: NOT_REACHED();
    }
    if (shift != 0) {
      ptr += i386_insn_put_shl_reg_var_by_imm(tmpreg, shift, ptr, end - ptr);
    }
  }
  if (memop.valtag.mem.base.val != -1) {
    ASSERT(memop.valtag.mem.base.tag == tag_var);
    ASSERT(   memop.valtag.mem.base.val >= 0
           && memop.valtag.mem.base.val < I386_NUM_GPRS);
    if (r_esp == memop.valtag.mem.base.val) {
      is_stack_access = true;
      ptr += i386_insn_put_addl_mem_to_reg_var(tmpreg, src_env_esp_addr, r_ds, ptr, end - ptr);
    } else {
      ptr += i386_insn_put_add_reg_to_reg_var(tmpreg, memop.valtag.mem.base.val, ptr,
          end - ptr);
    }
  }
  if (   memop.valtag.mem.disp.tag != tag_const
      || memop.valtag.mem.disp.val != 0) {
    long len;
    len = i386_insn_put_add_imm_valtag_with_reg_var(&memop.valtag.mem.disp,
        tmpreg, ptr, end - ptr);
    //i386_insn_replace_imm_operand_with_valtag(&ptr[0], &memop.valtag.mem.disp);
    ptr += len;
  }
  if (is_stack_access) {
    //should check if the address in tmpreg belongs to the stack in src_env; if not, exit after setting exception flag
    ASSERT(memop.size >= 1 && memop.size <= 4);
    ptr += i386_insn_put_cmpl_imm_to_reg_var(tmpreg, SRC_ENV_STATE_STACK_TOP - memop.size, ptr, end - ptr);
    ptr += i386_insn_put_jcc_to_set_env_exception("ja", 1, jcc_insns, num_jcc_insns, MAX_JCC_INSNS, ptr, end - ptr);
    ptr += i386_insn_put_cmpl_imm_to_reg_var(tmpreg, SRC_ENV_STATE_STACK_BOTTOM, ptr, end - ptr);
    ptr += i386_insn_put_jcc_to_set_env_exception("jb", 1, jcc_insns, num_jcc_insns, MAX_JCC_INSNS, ptr, end - ptr);
  } else {
    ptr += i386_insn_put_and_imm_with_reg_var(MEMORY_SIZE - 1, tmpreg, ptr,
        end - ptr);
    ptr += i386_insn_put_add_mem_with_reg_var(tmpreg, SRC_ENV_MEMBASE,
        // (uint32_t)(unsigned long)&membase,
        r_ds, ptr, end - ptr);
  }
  //ctemps->excregs[I386_EXREG_GROUP_GPRS][r_ds] = 0xffff;
  if (membase_is_fixed) {
    //net effect of following code should be: (0) move tmp_reg to memop.base.val (1) pop the original contents of tmp_reg from scratch0_esp and (2) push the original contents of memop.base.val to scratch0_esp;
    /*if (is_stack_access) {
      //net effect: (1) move tmp_reg to src_env_esp_addr (2) pop the original contents of tmp_reg from stack and (3) push the original contents of src_env_esp_addr to stack
      //save tmpreg to scratch[2]
      ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(2), tmpreg,
          r_ds,
          ptr, end - ptr);

      //save src_env_esp_addr to scratch[3]
      ptr += i386_insn_put_movq_mem_to_reg_var(tmpreg, src_env_esp_addr, r_ds,
          ptr, end - ptr);
      ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(3),
          tmpreg, r_ds,
          ptr, end - ptr);

      //move original contents of tmpreg (now in scratch[2]) to src_env_esp_addr
      ptr += i386_insn_put_movq_mem_to_reg_var(tmpreg, SRC_ENV_SCRATCH(2),
          r_ds,
          ptr, end - ptr);
      ptr += i386_insn_put_movq_reg_var_to_mem(src_env_esp_addr,
          tmpreg, r_ds,
          ptr, end - ptr);

      //pop original contents of tmp_reg from stack
      ptr += i386_insn_put_pop_tmpreg_and_flags_to_stack(tmpreg, c2v, ptr, end - ptr);
      //push original contents of src_env_esp_addr (now in scratch[3] to stack
      ptr += i386_insn_put_push_addr_to_stack(SRC_ENV_SCRATCH(3), c2v, ptr, end - ptr);
    } else */{
      //save original value of memop.mem.base.val to tmp-scratch
      //move tmpreg to memop.mem.base.val
      //restore tmpreg to its original value by popping from scratch0_esp
      //push original contents of memop.mem.base.val (which are currently in tmp-stack) to scratch0 esp
      ASSERT(   memop.valtag.mem.base.val >= 0
             && memop.valtag.mem.base.val < I386_NUM_GPRS);
      ASSERT(r_ds == regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id());
      ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(2),
          memop.valtag.mem.base.val, regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id(),
          ptr, end - ptr);
      ptr += i386_insn_put_movl_reg_to_reg_var(memop.valtag.mem.base.val, tmpreg, ptr,
          end - ptr);
      ptr += i386_insn_put_pop_tmpreg_and_flags_to_stack(tmpreg, c2v/*, ctemps*/, ptr,
          end - ptr);
      ptr += i386_insn_put_push_addr_to_stack(SRC_ENV_SCRATCH(2), c2v/*, ctemps*/,
          ptr, end - ptr);
    }
  } else {
    ASSERT(!tmpreg_used);
    tmpreg_used = true;
    ptr += i386_insn_put_pop_flags_to_stack(c2v/*, ctemps*/, ptr, end - ptr);
  }
  /*
  ptr += i386_insn_put_dereference_reg1_into_reg2(tmpreg, tmpreg, ptr);
  ptr += i386_insn_put_movl_reg_to_mem((uint32_t)(unsigned long)&memtmp, tmpreg, ptr);
  */
  return ptr - buf;
}

static long
i386_insn_replace_memops(i386_insn_t const *src, operand_t const *memop, bool const *fixed, long num_memop, int tmpreg, regmap_t const *c2v, i386_insn_t *buf, size_t buf_size)
{
  i386_insn_t *ptr = buf, *end = buf + buf_size;
  int r_ds = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id();

  ptr->opc = src->opc;
  ptr->num_implicit_ops = src->num_implicit_ops;
  ASSERT(ptr->num_implicit_ops != -1);
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    bool found = false;
    for (int j = 0; j < num_memop; j++) {
      if (operands_equal(&memop[j], &src->op[i])) {
        if (fixed[j]) {
          break;
        }
        found = true;
        ptr->op[i].type = op_mem;
        ptr->op[i].size = memop[j].size;
        ptr->op[i].valtag.mem.addrsize = memop[j].valtag.mem.addrsize;
        ptr->op[i].rex_used = 0;
        ptr->op[i].valtag.mem.segtype = segtype_sel;
        ptr->op[i].valtag.mem.seg.sel.val = r_ds;
        ptr->op[i].valtag.mem.seg.sel.tag = tag_var;
        ptr->op[i].valtag.mem.seg.sel.mod.reg.mask = 0;
        ptr->op[i].valtag.mem.base.val = tmpreg;
        ptr->op[i].valtag.mem.index.val = -1;
        ptr->op[i].valtag.mem.riprel.val = -1;
        ptr->op[i].valtag.mem.riprel.tag = tag_const;
        ptr->op[i].valtag.mem.disp.val = 0;
        //ptr->op[i].valtag.mem.all = tag_const;
        ptr->op[i].valtag.mem.base.tag = tag_var;
        ptr->op[i].valtag.mem.index.tag = tag_const;
        ptr->op[i].valtag.mem.base.mod.reg.mask = MAKE_MASK(DWORD_LEN);
        ptr->op[i].valtag.mem.index.mod.reg.mask = MAKE_MASK(DWORD_LEN);
        ptr->op[i].valtag.mem.disp.tag = tag_const;
        break;
      }
    }
    if (!found) {
      i386_operand_copy(&ptr->op[i], &src->op[i]);
    }
  }
  return 1;
}

long
i386_insn_sandbox(i386_insn_t *dst, size_t dst_size, i386_insn_t const *src,
    regmap_t const *c2v/*, regset_t *ctemps*/, int tmpreg, long insn_index)
{
  post_sandbox_action_t psa[2];
  bool fixed[2], tmpreg_used, is_stackaccess[2];
  i386_insn_t *ptr, *end;
  operand_t memop[2];
  int i, r_ds;
  char const *opc;
  long num_memop;

  excp_jcc_insn_t jcc_insns[MAX_JCC_INSNS];
  size_t num_jcc_insns;

  r_ds = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id();
  int r_esp = regmap_get_elem(c2v, I386_EXREG_GROUP_GPRS, R_ESP).reg_identifier_get_id();
  DBG3(EQUIV, "sandboxing:\n%s\n", i386_insn_to_string(src, as1, sizeof as1));
  DBG3(EQUIV, "insn_index = %ld\n", insn_index);
  DBG3(EQUIV, "r_esp = %d\n", r_esp);
  ASSERT(tmpreg >= 0 && tmpreg < I386_NUM_GPRS);
  ASSERT(tmpreg != r_esp);
  num_memop = 0;
  opc = opctable_name(src->opc);

  //First identify all memory operands and store them with their characteristics in memop[]/psa[]/fixed[]
  for (i = I386_MAX_NUM_OPERANDS - 1; i >= 0; i--) {
    // iterate in reverse order, so implicit ops come last
    if (i386_operand_accesses_memory(src, i, &fixed[num_memop],
               &memop[num_memop], &psa[num_memop])) {
      num_memop++;
    }
    ASSERT(num_memop <= 2);
  }
  DBG3(EQUIV, "num_memop = %ld\n", num_memop);

  //save original registers in memop and replace it with sandboxed version; gen prefix insns to sandbox the memop
  tmpreg_used = false;
  ptr = dst;
  end = dst + dst_size;

  num_jcc_insns = 0;

  for (i = 0; i < num_memop; i++) {
    ptr += i386_insn_sandbox_memop(memop[i], fixed[i], tmpreg, tmpreg_used, c2v, is_stackaccess[i], jcc_insns, num_jcc_insns, ptr, end - ptr); //pushes the original contents of the register containing the sandboxed address to stack (tmpreg if fixed[i]==false, orig_reg otherwise)
  }

  //Generate the original instruction (with the updated memop) at *ptr (but do not increment ptr)
  long n_insns = i386_insn_replace_memops(src, memop, fixed, num_memop, tmpreg, c2v, ptr, end - ptr);
  ASSERT(n_insns == 1);

  //increment ptr by additionally generating instructions to avoid fp exceptions, etc. for the current instruction at *ptr; replace the instruction at ptr with the new instructions
  DBG2(EQUIV, "sandboxed instruction: %s\n", i386_insn_to_string(ptr, as1, sizeof as1));
  ptr += i386_insn_avoid_fp_exceptions_and_handle_control_flow(ptr, end - ptr,
      insn_index, tmpreg, c2v, jcc_insns, num_jcc_insns/*, ctemps*/);

  //gen code to restore the original values of memop registers and performing post-sandbox-actions (psa)
  for (i = 0; i < num_memop; i++) {
    if (fixed[i]) {
      int r_ds;
      r_ds = regmap_get_elem(c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id();
      //ctemps->excregs[I386_EXREG_GROUP_GPRS][r_ds] = 0xffff;
      //pop memop[i].val.mem.base. use tmpreg as memop[i].val.mem.base may be R_ESP
      ptr += i386_insn_put_movq_reg_var_to_mem(SRC_ENV_SCRATCH(2), tmpreg, r_ds,
          ptr, end - ptr);
      ptr += i386_insn_put_pop_tmpreg_to_stack(tmpreg, c2v/*, ctemps*/, ptr,
          end - ptr);
      ptr += i386_insn_put_movl_reg_to_reg_var(memop[i].valtag.mem.base.val, tmpreg, ptr,
          end - ptr);
      ptr += i386_insn_put_movq_mem_to_reg_var(tmpreg, SRC_ENV_SCRATCH(2), r_ds, ptr,
          end - ptr);
    } else {
      //pop tmpreg
      ptr += i386_insn_put_pop_tmpreg_to_stack(tmpreg, c2v/*, ctemps*/, ptr,
          end - ptr);
    }
#define ADD_TO_SRC_ENV_ESP_REG(d) do { \
  ptr += i386_insn_put_addl_imm_to_mem(d, SRC_ENV_ADDR + I386_ENV_EXOFF(I386_EXREG_GROUP_GPRS, r_esp), -1, ptr, end - ptr, I386_AS_MODE_64); \
} while(0)
    if (psa[i] == INCREMENT) {
      ASSERT(   memop[i].valtag.mem.base.val >= 0
             && memop[i].valtag.mem.base.val < I386_NUM_GPRS);
      if (is_stackaccess[i]) {
        ADD_TO_SRC_ENV_ESP_REG(memop[i].size);
      } else {
        ptr += i386_insn_put_add_imm_with_reg_var(memop[i].size,
            memop[i].valtag.mem.base.val, ptr, end - ptr);
      }
    } else if (psa[i] == DECREMENT) {
      if (is_stackaccess[i]) {
        ADD_TO_SRC_ENV_ESP_REG((-memop[i].size));
      } else {
        ptr += i386_insn_put_add_imm_with_reg_var(-memop[i].size,
            memop[i].valtag.mem.base.val, ptr, end - ptr);
      }
    } else ASSERT(psa[i] == NO_ACTION);
  }

  if (num_jcc_insns) {
    i386_insn_t *ptr_jmp_insn;
    ptr_jmp_insn = ptr;
    ptr += i386_insn_put_jump_to_pcrel(nextpc_t(0), ptr, end - ptr);
    for (i = 0; i < num_jcc_insns; i++) {
      i386_insn_put_jcc_to_pcrel(jcc_insns[i].opc, ptr - jcc_insns[i].ptr - 1,
          jcc_insns[i].ptr, 1);
    }
    //ptr += i386_insn_put_pop_tmpreg_to_stack(tmpreg2, ptr);
    ptr += i386_insn_put_pop_flags_to_stack(c2v/*, ctemps*/, ptr, end - ptr);
    for (int i = 0; i < jcc_insns[i].num_tmpreg_pops; i++) {
      ptr += i386_insn_put_pop_tmpreg_to_stack(tmpreg, c2v/*, ctemps*/, ptr, end - ptr);
    }
    ptr += i386_insn_put_movl_imm_to_mem(EXCEPTION_FPE, SRC_ENV_EXCEPTION, -1, ptr,
        end - ptr);
    //ptr += i386_insn_put_jump_to_nextpc(ptr, end - ptr, 0); //just jump out to an arbitrary exit (chose 0 because it will always be a valid exit)
    i386_insn_put_jump_to_pcrel(ptr - ptr_jmp_insn - 1, ptr_jmp_insn, 1);
  }

  DBG3(EQUIV, "returning:\n%s\n", i386_iseq_to_string(dst, ptr - dst, as1, sizeof as1));
  return ptr - dst;
}

/* static void
i386_iseq_sandbox_memory_accesses(i386_insn_t *i386_iseq, long *i386_iseq_len,
    bool fallback)
{
  i386_insn_t *sboxed, *ptr, *end;
  long i, sboxed_len, max_alloc;
  int tmpreg;


  sboxed_len = *i386_iseq_len;
  max_alloc = (sboxed_len + 10) * 10;
  sboxed = (i386_insn_t *)malloc(max_alloc * sizeof(i386_insn_t));
  ASSERT(sboxed);
  ptr = sboxed;
  end = sboxed + max_alloc;
  for (i = 0; i < sboxed_len; i++) {
    ptr += i386_insn_sandbox_memory_access(ptr, &i386_iseq[i], tmpreg, fallback);
    ASSERT(ptr < end);
  }
  *i386_iseq_len = ptr - sboxed;
  ASSERT(*i386_iseq_len > 0);
  for (i = 0; i < *i386_iseq_len; i++) {
    i386_insn_copy(&i386_iseq[i], &sboxed[i]);
  }
  free(sboxed);
} */

/*size_t
i386_insn_gen_exec_code(i386_insn_t *buf, size_t size, i386_insn_t const *insn,
    imm_map_t const *imm_map, long insn_index)
{
  i386_insn_t *ptr, *end, itmp;
  int tmpreg;

  ptr = buf;
  end = buf + size;

  i386_insn_copy(&itmp, insn);
  if (imm_map) {
    i386_insn_rename_constants(&itmp, imm_map, NULL, 0, NULL);
  }
  tmpreg = i386_iseq_find_unused_gpr(&itmp, 1);
  ASSERT(tmpreg >= 0 && tmpreg < I386_NUM_GPRS);

  ptr += i386_insn_sandbox(ptr, end - ptr, &itmp, tmpreg, insn_index);

  return ptr - buf;
}

size_t
i386_iseq_gen_exec_code(i386_insn_t const *i386_iseq, long i386_iseq_len,
    imm_map_t const *imm_map, uint8_t *i386_code, size_t i386_codesize,
    src_ulong const *nextpcs, uint8_t const * const *next_tcodes,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    long num_nextpcs)
{
  i386_insn_t *i386_iseq_sandboxed, *iptr, *iend, itmp;
  regset_t live_outs[num_nextpcs];
  transmap_t const **next_tmaps;
  long i, n, max_alloc;
  uint8_t *ptr, *end;
  int tmpreg;

  max_alloc = (i386_iseq_len + 20) * 20;
  i386_iseq_sandboxed = (i386_insn_t *)malloc(max_alloc * sizeof(i386_insn_t));
  ASSERT(i386_iseq_sandboxed);
  next_tmaps = (transmap_t const**)malloc(num_nextpcs * sizeof(transmap_t const *));
  ASSERT(next_tmaps);

  iptr = i386_iseq_sandboxed;
  iend = i386_iseq_sandboxed + max_alloc;

  iptr += i386_insn_put_pusha(iptr, iend - iptr, 64);
  iptr += i386_insn_put_movq_reg_to_mem(SRC_ENV_SCRATCH(0), R_ESP, iptr, iend - iptr);
  DBG3(EQUIV, "%s", "Calling transmap_translation_insns\n");
  iptr += transmap_translation_insns(transmap_no_regs(), in_tmap, iptr, iend - iptr, 64);
  DBG3(EQUIV, "%s", "returned from transmap_translation_insns\n");

  DBG3(EQUIV, "Generating exec code:\n%s\n",
      i386_iseq_to_string(i386_iseq, i386_iseq_len, as1, sizeof as1));
  long map[i386_iseq_len];
  for (i = 0; i < i386_iseq_len; i++) {
    map[i] = iptr - i386_iseq_sandboxed;
    DBG3(EQUIV, "map[%ld]=%ld\n", i, map[i]);

    iptr += i386_insn_gen_exec_code(iptr, iend - iptr, &i386_iseq[i], imm_map, i);
    ASSERT(iptr < iend);
  }

  DBG3(EQUIV, "before rename_abs_inums_using_map, i386_iseq_sandboxed:\n%s\n",
      i386_iseq_to_string(i386_iseq_sandboxed, iptr - i386_iseq_sandboxed,
          as1, sizeof as1));
  i386_iseq_rename_abs_inums_using_map(i386_iseq_sandboxed, iptr - i386_iseq_sandboxed,
      map, i386_iseq_len);
  i386_iseq_convert_abs_inums_to_pcrel_inums(i386_iseq_sandboxed,
      iptr - i386_iseq_sandboxed);

  for (i = 0; i < num_nextpcs; i++) {
    next_tmaps[i] = transmap_no_regs();
    regset_copy(&live_outs[i], regset_all());
  }

  //static uint32_t membase = (uint32_t)&((state_t *)SRC_ENV_ADDR)->memory[0];
  //fill transmap_no_regs in next_tmaps.
  //DBG3(EQUIV, "n = %ld\n", n);
  //i386_iseq_sandbox_memory_accesses(i386_iseq_sandboxed, &n, in_tmap->tmap_fallback);
  //ASSERT(n < max_alloc);
  //DBG3(EQUIV, "n = %ld\n", n);
  n = iptr - i386_iseq_sandboxed;
  DBG3(EQUIV, "before connect_end_points, i386_iseq_sandboxed:\n%s\nnum_nextpcs=%ld\n",
      i386_iseq_to_string(i386_iseq_sandboxed, n, as1, sizeof as1), num_nextpcs);
  n = i386_iseq_connect_end_points(i386_iseq_sandboxed, n, iend - i386_iseq_sandboxed,
      nextpcs, next_tcodes, out_tmaps, next_tmaps, live_outs, num_nextpcs,
      false, 64);
  ASSERT(n < max_alloc);
  DBG3(EQUIV, "after connect_end_points, i386_iseq_sandboxed:\n%s\n",
      i386_iseq_to_string(i386_iseq_sandboxed, n, as1, sizeof as1));

  ptr = i386_code;
  end = ptr + i386_codesize;

  ptr += i386_iseq_assemble(NULL, i386_iseq_sandboxed, n, ptr, end - ptr, 64);

  free(i386_iseq_sandboxed);
  free(next_tmaps);
  return ptr - i386_code;
}*/


/*long
i386_setup_executable_code(uint8_t *exec_code, size_t exec_codesize,
    uint8_t const *code, size_t codesize, uint16_t jmp_offset0,
    transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_live_out)
{
  NOT_IMPLEMENTED();
  uint8_t *tc_start, *tc_ptr, *tc_end;

  tc_start = exec_code;
  tc_ptr = exec_code;
  tc_end = exec_code + exec_codesize;
  DBG(I386_EXEC, "codesize=%zd, jmp_offset0 = %hd\n", codesize, jmp_offset0);

  tc_ptr += save_callee_save_registers(tc_ptr, tc_end - tc_ptr);
  tc_ptr += transmap_translation_code(transmap_no_regs(), transmap_exec(), NULL,
      true, tc_ptr, tc_end - tc_ptr);
  if (in_tmap) {
    tc_ptr += transmap_translation_code(transmap_exec(), in_tmap, NULL,
        true, tc_ptr, tc_end - tc_ptr);
  }
  if (jmp_offset0 != (uint16_t)-1) {
    jmp_offset0 += tc_ptr - tc_start;
  }
  memcpy(tc_ptr, code, codesize);
  tc_ptr += codesize;
  if (!out_tmap) {
    out_tmap = in_tmap;
  }
  tc_ptr += add_jump_handling_code(tc_ptr, tc_start, tc_end, jmp_offset0, out_tmap);
  ASSERT(tc_ptr <= tc_end);
  codesize = tc_ptr - tc_start;
  ASSERT(codesize < exec_codesize);

  DBG(I386_EXEC, "returning %zd at %p:\n%s\n", codesize, exec_code,
      ({
       char buf[4096], *ptr = buf, *end = ptr + sizeof buf;
       for (tc_ptr = tc_start; tc_ptr < tc_start + codesize; tc_ptr++) {
         ptr += snprintf(ptr, end - ptr, " %hhx", *tc_ptr);
       }
       ptr += snprintf(ptr, end - ptr, "\n");
       buf;
       }));

  return 0;
}
*/

/*
static void
__i386_state_rename_using_regmap(i386_state_t *state, regmap_t const *regmap, bool inv)
{
  uint32_t new_values[I386_NUM_GPRS];
  bool assigned[I386_NUM_GPRS];
  int i;

  for (i = 0; i < I386_NUM_GPRS; i++) {
    assigned[i] = false;
  }
  for (i = 0; i < I386_NUM_GPRS; i++) {
    if (!inv) {
      if (i >= I386_NUM_GPRS - 2) {
        new_values[i - 2] = state->regs[i];
        assigned[i - 2] = true;
      } else {
        if (regmap->table[i] != -1) {
          //printf("regmap->table[%d]=%d\n", i, regmap->table[i]);
          ASSERT(regmap->table[i] >= 0 && regmap->table[i] < I386_NUM_GPRS);
          new_values[regmap->table[i]] = state->regs[i];
          assigned[regmap->table[i]] = true;
        }
      }
    } else {
      if (i >= I386_NUM_GPRS - 2) {
        new_values[i] = state->regs[i - 2];
        assigned[i] = true;
      } else {
        if (regmap->table[i] != -1) {
          ASSERT(regmap->table[i] >= 0 && regmap->table[i] < I386_NUM_GPRS);
          new_values[i] = state->regs[regmap->table[i]];
          assigned[i] = true;
        }
      }
    }
  }
  for (i = 0; i < I386_NUM_GPRS; i++) {
    if (assigned[i]) {
      state->regs[i] = new_values[i];
    }
  }
}

void
i386_state_rename_using_regmap(i386_state_t *state, regmap_t const *regmap)
{
  __i386_state_rename_using_regmap(state, regmap, false);
}

void
i386_state_inv_rename_using_regmap(i386_state_t *state, regmap_t const *regmap)
{
  __i386_state_rename_using_regmap(state, regmap, true);
}*/

long
i386_insn_put_mul_var_with_operand(operand_t const *op, bool is_signed,
    int r_eax, int r_edx, i386_insn_t *insns, size_t insns_size)
{
  char opc[32], opc_prefix[32];
  char suffix;

  if (is_signed) {
    snprintf(opc_prefix, sizeof opc, "imul");
  } else {
    snprintf(opc_prefix, sizeof opc, "mul");
  }

  if (op->size == 1) {
    suffix = 'b';
  } else if (op->size == 2) {
    suffix = 'w';
  } else if (op->size == 4) {
    suffix = 'l';
  } else NOT_REACHED();

  snprintf(opc, sizeof opc, "%s%c", opc_prefix, suffix);
  long n = i386_insn_put_opcode_operand(opc, op, insns, insns_size);
  ASSERT(n == 1);
  ASSERT(insns[0].num_implicit_ops == 4);
  ASSERT(insns[0].op[2].type == op_reg);
  ASSERT(insns[0].op[2].valtag.reg.tag == tag_const);
  ASSERT(insns[0].op[2].valtag.reg.val == R_EDX);
  insns[0].op[2].valtag.reg.tag = tag_var;
  insns[0].op[2].valtag.reg.val = r_edx;
  ASSERT(insns[0].op[3].type == op_reg);
  ASSERT(insns[0].op[3].valtag.reg.tag == tag_const);
  ASSERT(insns[0].op[3].valtag.reg.val == R_EAX);
  insns[0].op[3].valtag.reg.tag = tag_var;
  insns[0].op[3].valtag.reg.val = r_eax;
  convert_reg_to_regvar(insns);
  return n;
}

long
i386_insn_put_movl_reg_var_to_reg_var(int dst_reg, int src_reg, i386_insn_t *insns,
    size_t insns_size)
{
  long ret;

  ret = i386_insn_put_movl_reg_to_reg(dst_reg, src_reg, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}

size_t
i386_insn_put_add_regvar_to_regvar(long dst_reg, long src_reg, i386_insn_t *insns,
    size_t insns_size)
{
  long ret;

  ret = i386_insn_put_add_reg_to_reg(dst_reg, src_reg, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}

size_t
i386_insn_put_movsbl_reg2b_var_to_reg1_var(int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size)
{
  long ret;

  ret = i386_insn_put_movsbl_reg2b_to_reg1(reg1, reg2, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}

size_t
i386_insn_put_movswl_reg2w_var_to_reg1_var(int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size)
{
  long ret;

  ret = i386_insn_put_movswl_reg2w_to_reg1(reg1, reg2, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}

size_t
i386_insn_put_shl_reg_var_by_imm(int reg, int imm, i386_insn_t *insns, size_t insns_size)
{
  long ret;

  ret = i386_insn_put_shl_reg_by_imm(reg, imm, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}

size_t
i386_insn_put_shr_reg_var_by_imm(int reg, int imm, i386_insn_t *insns, size_t insns_size)
{
  long ret;

  ret = i386_insn_put_shr_reg_by_imm(reg, imm, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}

size_t
i386_insn_put_sar_reg_var_by_imm(int reg, int imm, i386_insn_t *insns, size_t insns_size)
{
  long ret;

  ret = i386_insn_put_sar_reg_by_imm(reg, imm, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}

long
i386_insn_put_dereference_reg2_var_offset_into_reg1w_var(int reg1, int reg2, int offset,
    i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movw");

  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_var;
  insn_ptr[0].op[0].valtag.reg.val = reg1;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(WORD_LEN);
  insn_ptr[0].op[0].size = 2;

  insn_ptr[0].op[1].type = op_mem;
  //insn_ptr[0].op[1].tag.mem.all = tag_const;
  insn_ptr[0].op[1].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[1].valtag.mem.seg.sel.tag = tag_var;
  insn_ptr[0].op[1].valtag.mem.seg.sel.val = 0;
  insn_ptr[0].op[1].valtag.mem.base.val = reg2;
  insn_ptr[0].op[1].valtag.mem.base.tag = tag_var;
  insn_ptr[0].op[1].valtag.mem.base.mod.reg.mask = MAKE_MASK(DWORD_LEN);
  insn_ptr[0].op[1].valtag.mem.index.val = -1;
  insn_ptr[0].op[1].valtag.mem.disp.val = offset;
  insn_ptr[0].op[1].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[1].valtag.mem.riprel.tag = tag_const;
  insn_ptr[0].op[1].size = 2;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;

}

size_t
i386_insn_put_opcode_regmem_offset_to_reg_var(char const *opcode, int reg_var,
    int regmem,
    long offset, int size, i386_insn_t *buf, size_t buf_size)
{
  i386_insn_t *ptr, *end;

  ptr = buf;
  end = buf + buf_size;

  i386_insn_init(&ptr[0]);
  ptr[0].opc = opc_nametable_find(opcode);
  
  ptr[0].op[0].type = op_reg;
  ptr[0].op[0].valtag.reg.tag = tag_var;
  ptr[0].op[0].valtag.reg.val = reg_var;
  ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(size * BYTE_LEN);
  ptr[0].op[0].size = size;

  ptr[0].op[1].type = op_mem;
  ptr[0].op[1].valtag.mem.segtype = segtype_sel;
  ptr[0].op[1].valtag.mem.seg.sel.tag = tag_var;
  ptr[0].op[1].valtag.mem.seg.sel.val = 0;
  ptr[0].op[1].valtag.mem.base.val = regmem;
  ptr[0].op[1].valtag.mem.base.tag = tag_var;
  ptr[0].op[1].valtag.mem.base.mod.reg.mask = MAKE_MASK(DWORD_LEN);
  ptr[0].op[1].valtag.mem.index.val = -1;
  ptr[0].op[1].valtag.mem.disp.val = offset;
  ptr[0].op[1].valtag.mem.disp.tag = tag_const;
  ptr[0].op[1].valtag.mem.riprel.val = -1;
  ptr[0].op[1].valtag.mem.riprel.tag = tag_const;
  ptr[0].op[1].size = size;

  i386_insn_add_implicit_operands(&ptr[0]);
  ptr++;
  ASSERT(ptr <= end);

  return ptr - buf;
}

size_t
i386_insn_put_opcode_reg_var_to_regmem_offset(char const *opcode,
    int regmem,
    long offset, int reg_var, int size, i386_insn_t *buf, size_t buf_size)
{
  i386_insn_t *ptr, *end;

  ptr = buf;
  end = buf + buf_size;

  i386_insn_init(&ptr[0]);
  ptr[0].opc = opc_nametable_find(opcode);
  
  ptr[0].op[0].type = op_mem;
  ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  ptr[0].op[0].valtag.mem.seg.sel.tag = tag_var;
  ptr[0].op[0].valtag.mem.seg.sel.val = 0;
  ptr[0].op[0].valtag.mem.base.val = regmem;
  ptr[0].op[0].valtag.mem.base.tag = tag_var;
  ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DWORD_LEN);
  ptr[0].op[0].valtag.mem.index.val = -1;
  ptr[0].op[0].valtag.mem.disp.val = offset;
  ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  ptr[0].op[0].valtag.mem.riprel.val = -1;
  ptr[0].op[0].valtag.mem.riprel.tag = tag_const;
  ptr[0].op[0].size = size;

  ptr[0].op[1].type = op_reg;
  ptr[0].op[1].valtag.reg.tag = tag_var;
  ptr[0].op[1].valtag.reg.val = reg_var;
  ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(size * BYTE_LEN);
  ptr[0].op[1].size = size;

  i386_insn_add_implicit_operands(&ptr[0]);
  ptr++;
  ASSERT(ptr <= end);

  return ptr - buf;
}

size_t
i386_insn_put_opcode_imm_to_regmem_offset(char const *opcode,
    int regmem,
    long offset, int imm, int size, i386_insn_t *buf, size_t buf_size)
{
  i386_insn_t *ptr, *end;

  ptr = buf;
  end = buf + buf_size;

  i386_insn_init(&ptr[0]);
  ptr[0].opc = opc_nametable_find(opcode);
  
  ptr[0].op[0].type = op_mem;
  ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  ptr[0].op[0].valtag.mem.seg.sel.tag = tag_var;
  ptr[0].op[0].valtag.mem.seg.sel.val = 0;
  ptr[0].op[0].valtag.mem.base.val = regmem;
  ptr[0].op[0].valtag.mem.base.tag = tag_var;
  ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DWORD_LEN);
  ptr[0].op[0].valtag.mem.index.val = -1;
  ptr[0].op[0].valtag.mem.disp.val = offset;
  ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  ptr[0].op[0].valtag.mem.riprel.val = -1;
  ptr[0].op[0].valtag.mem.riprel.tag = tag_const;
  ptr[0].op[0].size = size;

  ptr[0].op[1].type = op_imm;
  ptr[0].op[1].valtag.imm.tag = tag_const;
  ptr[0].op[1].valtag.imm.val = imm;
  ptr[0].op[1].size = size;

  i386_insn_add_implicit_operands(&ptr[0]);
  ptr++;
  ASSERT(ptr <= end);

  return ptr - buf;
}

size_t
i386_insn_put_movl_regmem_offset_to_reg_var(int reg_var, int regmem, long offset,
    i386_insn_t *buf, size_t buf_size)
{
  return i386_insn_put_opcode_regmem_offset_to_reg_var("movl", reg_var, regmem, offset,
      4, buf, buf_size);
}

size_t
i386_insn_put_addl_tag_to_reg_var(long entrysize, int tag_num, src_tag_t tag,
    int reg, int const *temps, int num_temps, i386_insn_t *buf, size_t buf_size)
{
  i386_insn_t *ptr, *end;

  ptr = buf;
  end = buf + buf_size;

  i386_insn_init(&ptr[0]);
  ptr[0].opc = opc_nametable_find("movl");
  
  ptr[0].op[0].type = op_reg;
  ptr[0].op[0].valtag.reg.val = temps[0];
  ptr[0].op[0].valtag.reg.tag = tag_var;
  ptr[0].op[0].valtag.reg.mod.reg.mask = 0xffffffff;
  ptr[0].op[0].size = 4;

  ptr[0].op[1].type = op_imm;
  ptr[0].op[1].valtag.imm.tag = tag;
  ptr[0].op[1].valtag.imm.val = tag_num;
  ptr[0].op[1].valtag.imm.mod.imm.modifier = IMM_UNMODIFIED;
  //ptr[0].op[1].valtag.imm.mod.imm.modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0;
  //ptr[0].op[1].valtag.imm.mod.imm.sconstants[0] = 32;
  //ptr[0].op[1].valtag.imm.mod.imm.sconstants[1] = 0;
  //ptr[0].op[1].valtag.imm.mod.imm.sconstants[2] = entrysize;
  ptr[0].op[1].size = 4;

  i386_insn_add_implicit_operands(&ptr[0]);
  ptr++;
  ASSERT(ptr <= end);

  ptr += i386_insn_put_imul_imm_with_reg_var(entrysize, temps[0],
      ptr, end - ptr);
  ASSERT(ptr <= end);

  ptr += i386_insn_put_add_reg_to_reg_var(reg, temps[0], ptr, end - ptr);
  ASSERT(ptr <= end);

  return ptr - buf;

}

size_t
i386_insn_put_addl_vconst_to_reg_var(long entrysize, int vconst_num,
    int reg, int const *temps, int num_temps, i386_insn_t *buf, size_t buf_size)
{
  return i386_insn_put_addl_tag_to_reg_var(entrysize, vconst_num, tag_imm_vconst_num, reg, temps, num_temps, buf, buf_size);
}

size_t
i386_insn_put_addl_symbol_to_reg_var(long entrysize, int symbol_num,
    int reg, int const *temps, int num_temps, i386_insn_t *buf, size_t buf_size)
{
  return i386_insn_put_addl_tag_to_reg_var(entrysize, symbol_num, tag_imm_symbol_num, reg, temps, num_temps, buf, buf_size);
}

size_t
i386_insn_put_opcode_scaled_vconst_gpr_mem_to_reg_var(char const *opcode,
    int dst_reg, uint32_t start,
    int scale, int gpr, i386_insn_t *buf, size_t size)
{
  i386_insn_t *ptr, *end;

  ptr = buf;
  end = buf + size;

  i386_insn_init(&ptr[0]);
  ptr[0].opc = opc_nametable_find(opcode);
  
  ptr[0].op[0].type = op_reg;
  ptr[0].op[0].valtag.reg.tag = tag_var;
  ptr[0].op[0].valtag.reg.val = dst_reg;
  ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
  ptr[0].op[0].size = DWORD_LEN / BYTE_LEN;

  ptr[0].op[1].type = op_mem;
  ptr[0].op[1].valtag.mem.segtype = segtype_sel;
  ptr[0].op[1].valtag.mem.seg.sel.tag = tag_var;
  ptr[0].op[1].valtag.mem.seg.sel.val = 0;
  ptr[0].op[1].valtag.mem.base.val = -1;
  ptr[0].op[1].valtag.mem.base.tag = tag_const;
  ptr[0].op[1].valtag.mem.index.val = -1;
  ptr[0].op[1].valtag.mem.index.tag = tag_const;
  ptr[0].op[1].valtag.mem.disp.tag = tag_imm_exvregnum;
  ptr[0].op[1].valtag.mem.disp.val = gpr;
  ptr[0].op[1].valtag.mem.disp.mod.imm.modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0;
  ptr[0].op[1].valtag.mem.disp.mod.imm.exvreg_group = I386_EXREG_GROUP_GPRS;
  ptr[0].op[1].valtag.mem.disp.mod.imm.sconstants[0] = 32;
  ptr[0].op[1].valtag.mem.disp.mod.imm.sconstants[1] = start;
  ptr[0].op[1].valtag.mem.disp.mod.imm.sconstants[2] = scale;
  ptr[0].op[1].valtag.mem.riprel.val = -1;
  ptr[0].op[1].valtag.mem.riprel.tag = tag_const;
  ptr[0].op[1].size = DWORD_LEN / BYTE_LEN;

  i386_insn_add_implicit_operands(&ptr[0]);
  ptr++;
  ASSERT(ptr <= end);

  return ptr - buf;
}

size_t
i386_insn_put_addl_scaled_vconst_gpr_mem_to_reg_var(int dst_reg, uint32_t start,
    int scale, int gpr, i386_insn_t *buf, size_t size)
{
  return i386_insn_put_opcode_scaled_vconst_gpr_mem_to_reg_var("addl", dst_reg, start,
      scale, gpr, buf, size);
}

size_t
i386_insn_put_movl_scaled_vconst_gpr_mem_to_reg_var(int dst_reg, uint32_t start,
    int scale, int gpr, i386_insn_t *buf, size_t size)
{
  return i386_insn_put_opcode_scaled_vconst_gpr_mem_to_reg_var("movl", dst_reg, start,
      scale, gpr, buf, size);
}

size_t
i386_insn_put_opcode_scaled_vconst_exvreg_to_reg_var(char const *opcode,
    int scale, int group, int reg,
    int dst_reg, i386_insn_t *buf, size_t size)
{
  i386_insn_t *ptr, *end;

  ptr = buf;
  end = buf + size;

  i386_insn_init(&ptr[0]);
  ptr[0].opc = opc_nametable_find(opcode);
  
  ptr[0].op[0].type = op_reg;
  ptr[0].op[0].valtag.reg.tag = tag_var;
  ptr[0].op[0].valtag.reg.val = dst_reg;
  ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
  ptr[0].op[0].size = DWORD_LEN / BYTE_LEN;

  ptr[0].op[1].type = op_imm;
  ptr[0].op[1].valtag.imm.tag = tag_imm_exvregnum;
  ptr[0].op[1].valtag.imm.val = reg;
  ptr[0].op[1].valtag.imm.mod.imm.modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0;
  ptr[0].op[1].valtag.imm.mod.imm.exvreg_group = group;
  ptr[0].op[1].valtag.imm.mod.imm.sconstants[0] = 32;
  ptr[0].op[1].valtag.imm.mod.imm.sconstants[1] = 0;
  ptr[0].op[1].valtag.imm.mod.imm.sconstants[2] = scale;
  ptr[0].op[1].size = DWORD_LEN / BYTE_LEN;

  i386_insn_add_implicit_operands(&ptr[0]);
  ptr++;
  ASSERT(ptr <= end);

  return ptr - buf;
}

size_t
i386_insn_put_addl_scaled_vconst_exvreg_to_reg_var(int scale, int group, int reg,
    int dst_reg, i386_insn_t *buf, size_t size)
{
  return i386_insn_put_opcode_scaled_vconst_exvreg_to_reg_var("addl", scale, group,
      reg, dst_reg, buf, size);
}

size_t
i386_insn_put_movl_scaled_vconst_exvreg_to_reg_var(int scale, int group, int reg,
    int dst_reg, i386_insn_t *buf, size_t size)
{
  return i386_insn_put_opcode_scaled_vconst_exvreg_to_reg_var("movl", scale, group,
      reg, dst_reg, buf, size);
}

long
i386_insn_put_imul_imm_with_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  long ret;

  ret = i386_insn_put_imul_imm_with_reg(imm, regno, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}

long
i386_insn_put_testw_imm_with_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  long ret;

  ret = i386_insn_put_testw_imm_with_reg(imm, regno, insns, insns_size);
  ASSERT(ret <= insns_size);
  i386_iseq_convert_reg_to_reg_var(insns, ret);
  return ret;
}
