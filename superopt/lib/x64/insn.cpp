#include "support/utils.h"
#include "support/globals.h"

#include "valtag/regset.h"

#include "x64/cpu_consts.h"
#include "x64/insn.h"
#include "i386/disas.h"
#include "insn/dst-insn.h"

regset_t x64_callee_save_regs;

//size_t
//x64_insn_to_int_array(x64_insn_t const *insn, int64_t arr[], size_t len)
//{
//  NOT_IMPLEMENTED();
//}

//char *x64_iseq_to_string(x64_insn_t const *iseq, long iseq_len,
//    char *buf, size_t buf_size)
//{
//  NOT_IMPLEMENTED();
//}

void x64_init(void)
{
  static bool init = false;

  if (!init) {
    init = true;
    regset_clear(&x64_reserved_regs);
#if ARCH_SRC == ARCH_ETFG
    regset_mark_ex_mask(&x64_reserved_regs, X64_EXREG_GROUP_GPRS, reg_identifier_t(R_RSP), X64_EXREG_LEN(X64_EXREG_GROUP_GPRS));
#endif
    {
      autostop_timer opc_init_timer("opc_init");
      opc_init();
    }
    i386_init_costfns();
    if (!gas_inited) {
      gas_init();
      gas_inited = true;
    }
    regset_clear(&x64_callee_save_regs);
#if ARCH_SRC == ARCH_ETFG
    regset_mark_ex_mask(&x64_callee_save_regs, X64_EXREG_GROUP_GPRS, reg_identifier_t(R_RBX), X64_EXREG_LEN(X64_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&x64_callee_save_regs, X64_EXREG_GROUP_GPRS, reg_identifier_t(R_RBP), X64_EXREG_LEN(X64_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&x64_callee_save_regs, X64_EXREG_GROUP_GPRS, reg_identifier_t(R_RDI), X64_EXREG_LEN(X64_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&x64_callee_save_regs, X64_EXREG_GROUP_GPRS, reg_identifier_t(R_RSI), X64_EXREG_LEN(X64_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&x64_callee_save_regs, X64_EXREG_GROUP_GPRS, reg_identifier_t(R_R12), X64_EXREG_LEN(X64_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&x64_callee_save_regs, X64_EXREG_GROUP_GPRS, reg_identifier_t(R_R13), X64_EXREG_LEN(X64_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&x64_callee_save_regs, X64_EXREG_GROUP_GPRS, reg_identifier_t(R_R14), X64_EXREG_LEN(X64_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&x64_callee_save_regs, X64_EXREG_GROUP_GPRS, reg_identifier_t(R_R15), X64_EXREG_LEN(X64_EXREG_GROUP_GPRS));
#endif
  }
  //cout << __func__ << " " << __LINE__ << ": dst_reserved_regs =\n" << regset_to_string(&dst_reserved_regs, as1, sizeof as1) << endl;

}

void
x64_free(void)
{
  i386_free_costfns();
}

long x64_insn_sandbox(struct x64_insn_t *dst, size_t dst_size,
    struct x64_insn_t const *src,
    struct regmap_t const *c2v, int tmpreg, long insn_index)
{
  NOT_IMPLEMENTED();
}

long x64_insn_put_move_imm_to_mem(dst_ulong imm, uint64_t addr, int membase, x64_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}

size_t
x64_insn_put_ret_insn(x64_insn_t *insns, size_t insns_size, int retval)
{
  NOT_IMPLEMENTED();
}

long x64_insn_put_function_return(fixed_reg_mapping_t const &fixed_reg_mapping, struct x64_insn_t *insns, size_t insns_size)
{
  NOT_IMPLEMENTED();
}

bool
x64_insn_fetch_raw(i386_insn_t *insn, input_exec_t const *in, src_ulong eip,
    unsigned long *size)
{
  return i386_insn_fetch_raw_helper(insn, in, eip, size, I386_AS_MODE_64);
}

regset_t const *
x64_regset_live_at_jumptable()
{
  return regset_all();
}

char *
x64_iseq_to_string(x64_insn_t const *iseq, long iseq_len, char *buf, size_t buf_size)
{
  return i386_iseq_to_string_mode(iseq, iseq_len, I386_AS_MODE_64, buf, buf_size);
}


