#include <map>
#include <iostream>
#include <set>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "support/c_utils.h"
#include "support/debug.h"
#include "support/types.h"
#include "support/src-defs.h"
#include "support/globals.h"
#include "support/timers.h"
#include "support/serialize.h"

#include "cutils/imap_elem.h"

#include "valtag/elf/elf.h"
#include "valtag/ldst_input.h"
#include "valtag/regmap.h"
#include "valtag/transmap.h"
#include "valtag/regcons.h"
#include "valtag/imm_vt_map.h"
#include "valtag/regcount.h"
#include "valtag/memset.h"
#include "valtag/nextpc_map.h"
#include "valtag/memslot_map.h"

#include "expr/allocsite.h"

#include "i386/insn.h"
#include "i386/cpu_consts.h"
#include "x64/insn.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "i386/insntypes.h"
#include "i386/execute.h"

#include "insn/src-insn.h"
#include "insn/jumptable.h"
#include "insn/cfg.h"
#include "insn/dst-insn.h"

#include "rewrite/bbl.h"
#include "rewrite/static_translate.h"

#include "superopt/src_env.h"

#include "rewrite/translation_instance.h"
#include "rewrite/src_iseq_match_renamings.h"

extern "C" {
#include "i386/disas.h"
#include "gas/common/as.h"
}
//#include "graph/tfg.h"

#undef INLINE
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "config-host.h"

#include "support/cartesian.h"
#include "support/permute.h"
#include "support/debug.h"

#include "valtag/nextpc.h"
#include "valtag/pc_with_copy_id.h"

#include "rewrite/peephole.h"
#include "rewrite/unroll_loops.h"

#include "exec/state.h"

#include "superopt/typestate.h"

using namespace std;

static unsigned char bin1[1024];
static char as1[40960];
static char as2[10240];
static char as3[10240];

#define DBE(...)

extern "C" {
/* gas assembly functions */
int i386_md_assemble(char *assembly, uint8_t *bincode);
void i386_md_begin(void);
void subsegs_begin(void);
void symbol_begin(void);
void ppc_md_begin(void);
}

static char pb1[1024];

static char const *regs8lo[] = {"al", "cl", "dl", "bl"};
static char const *regs8hi[] = {"ah", "ch", "dh", "bh"};
static char const *regs8_rex[] = {"al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"};
static char const *regs16[]= {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"};
static char const *regs32[]= {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};
static char const *regs64[]= {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};

static char const *segs[] = {"es", "cs", "ss", "ds", "fs", "gs"};
static size_t const num_segs = sizeof segs/sizeof segs[0];

#if ARCH_DST == ARCH_I386 || ARCH_DST == ARCH_X64
#define i386_insn_get_usedef_regs dst_insn_get_usedef_regs
#define i386_iseq_get_usedef_regs dst_iseq_get_usedef_regs
#define transmap_translation_insns transmap_translation_insns
#else
#define i386_insn_get_usedef_regs NOTREACHED_MACRO
#define i386_iseq_get_usedef_regs NOTREACHED_MACRO
#define transmap_translation_insns NOTREACHED_MACRO_RETVAL
#endif

regset_t i386_reserved_regs;
regset_t i386_callee_save_regs;

static int
reg2str(int val, src_tag_t tag, int size, bool rex_used, uint64_t regmask,
    char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;

  if (tag == tag_const) {
    if (size == 1) {
      if (rex_used) {
        ASSERT(val >= 0 && val < X64_NUM_GPRS);
        ASSERT(regmask == 0xff);
        ptr += snprintf(ptr, end - ptr, "%%%s", regs8_rex[val]);
      } else {
        ASSERT(val >= 0 && val < I386_NUM_GPRS);
        if (regmask == 0xff) {
          ptr += snprintf(ptr, end - ptr, "%%%s", regs8lo[val]);
        } else if (regmask == 0xff00) {
          ptr += snprintf(ptr, end - ptr, "%%%s", regs8hi[val]);
        } else NOT_REACHED();
      }
    } else if (size == 2) {
      ASSERT(val >= 0 && val < X64_NUM_GPRS);
      ASSERT(regmask == 0xffff);
      ptr += snprintf(ptr, end - ptr, "%%%s", regs16[val]);
    } else if (size == 4) {
      ASSERT(val >= 0 && val < X64_NUM_GPRS);
      ASSERT(regmask == 0xffffffff);
      ptr += snprintf(ptr, end - ptr, "%%%s", regs32[val]);
    } else if (size == 8) {
      ASSERT(val >= 0 && val < X64_NUM_GPRS);
      if (regmask != 0xffffffffffffffffULL) {
        cout << __func__ << " " << __LINE__ << ": regmask = " << regmask << endl;
      }
      ASSERT(regmask == 0xffffffffffffffffULL);
      ptr += snprintf(ptr, end - ptr, "%%%s", regs64[val]);
    } else NOT_REACHED();
  } else if (tag == tag_var) {
    ptr += exvreg_to_string(I386_EXREG_GROUP_GPRS, val, tag, size,
        rex_used, regmask, ptr, end - ptr);
  } else {
    cout << __func__ << " " << __LINE__ << ": tag = " << tag2str(tag) << endl;
    NOT_REACHED();
  }
  return ptr - buf;
}

//int i386_base_reg = -1;

int i386_available_regs[] = {0, 1, 2, 3, 4, 5, 6, 7};
int i386_num_available_regs = sizeof i386_available_regs/sizeof i386_available_regs[0];

//int i386_available_xmm_regs[] = {0, 1, 2, 3, 4, 5, 6, 7};
//int i386_num_available_xmm_regs = sizeof i386_available_xmm_regs/sizeof i386_available_xmm_regs[0];

static char *
i386_regname_byte(int regnum, bool high_order, char *buf, int buf_size)
{
  switch (regnum) {
    case 0: snprintf (buf, buf_size, "%%a"); break;
    case 1: snprintf (buf, buf_size, "%%c"); break;
    case 2: snprintf (buf, buf_size, "%%d"); break;
    case 3: snprintf (buf, buf_size, "%%b"); break;
    case 4: snprintf (buf, buf_size, "%%sp"); break;
    case 5: snprintf (buf, buf_size, "%%bp"); break;
    case 6: snprintf (buf, buf_size, "%%si"); break;
    case 7: snprintf (buf, buf_size, "%%di"); break;
    case 8: snprintf (buf, buf_size, "%%r8"); break;
    case 9: snprintf (buf, buf_size, "%%r9"); break;
    case 10: snprintf (buf, buf_size, "%%r10"); break;
    case 11: snprintf (buf, buf_size, "%%r11"); break;
    case 12: snprintf (buf, buf_size, "%%r12"); break;
    case 13: snprintf (buf, buf_size, "%%r13"); break;
    case 14: snprintf (buf, buf_size, "%%r14"); break;
    case 15: snprintf (buf, buf_size, "%%r15"); break;
    default: NOT_REACHED(); return NULL;
  }
  if (high_order) {
    ASSERT(regnum >= 0 && regnum < 4);
    strncat(buf, "h", buf_size);
  } else {
    strncat(buf, "l", buf_size);
  }
  return buf;
}

char *
i386_regname_word(int regnum, char *buf, int buf_size)
{
  switch (regnum) {
    case 0: snprintf (buf, buf_size, "%%ax"); break;
    case 1: snprintf (buf, buf_size, "%%cx"); break;
    case 2: snprintf (buf, buf_size, "%%dx"); break;
    case 3: snprintf (buf, buf_size, "%%bx"); break;
    case 4: snprintf (buf, buf_size, "%%sp"); break;
    case 5: snprintf (buf, buf_size, "%%bp"); break;
    case 6: snprintf (buf, buf_size, "%%si"); break;
    case 7: snprintf (buf, buf_size, "%%di"); break;
    case 8: snprintf (buf, buf_size, "%%r8w"); break;
    case 9: snprintf (buf, buf_size, "%%r9w"); break;
    case 10: snprintf (buf, buf_size, "%%r10w"); break;
    case 11: snprintf (buf, buf_size, "%%r11w"); break;
    case 12: snprintf (buf, buf_size, "%%r12w"); break;
    case 13: snprintf (buf, buf_size, "%%r13w"); break;
    case 14: snprintf (buf, buf_size, "%%r14w"); break;
    case 15: snprintf (buf, buf_size, "%%r15w"); break;
    default: NOT_REACHED(); return NULL;
  }
  return buf;
}

char *
i386_regname(int regnum, char *buf, int buf_size)
{
  switch (regnum) {
    case 0: snprintf (buf, buf_size, "%%eax"); break;
    case 1: snprintf (buf, buf_size, "%%ecx"); break;
    case 2: snprintf (buf, buf_size, "%%edx"); break;
    case 3: snprintf (buf, buf_size, "%%ebx"); break;
    case 4: snprintf (buf, buf_size, "%%esp"); break;
    case 5: snprintf (buf, buf_size, "%%ebp"); break;
    case 6: snprintf (buf, buf_size, "%%esi"); break;
    case 7: snprintf (buf, buf_size, "%%edi"); break;
    case 8: snprintf (buf, buf_size, "%%r8d"); break;
    case 9: snprintf (buf, buf_size, "%%r9d"); break;
    case 10: snprintf (buf, buf_size, "%%r10d"); break;
    case 11: snprintf (buf, buf_size, "%%r11d"); break;
    case 12: snprintf (buf, buf_size, "%%r12d"); break;
    case 13: snprintf (buf, buf_size, "%%r13d"); break;
    case 14: snprintf (buf, buf_size, "%%r14d"); break;
    case 15: snprintf (buf, buf_size, "%%r15d"); break;
    default: NOT_REACHED(); snprintf(buf, buf_size, "%d", regnum); break;
  }
  return buf;
}

char *
x64_regname(int regnum, char *buf, int buf_size)
{
  switch (regnum) {
    case 0: snprintf (buf, buf_size, "%%rax"); break;
    case 1: snprintf (buf, buf_size, "%%rcx"); break;
    case 2: snprintf (buf, buf_size, "%%rdx"); break;
    case 3: snprintf (buf, buf_size, "%%rbx"); break;
    case 4: snprintf (buf, buf_size, "%%rsp"); break;
    case 5: snprintf (buf, buf_size, "%%rbp"); break;
    case 6: snprintf (buf, buf_size, "%%rsi"); break;
    case 7: snprintf (buf, buf_size, "%%rdi"); break;
    case 8: snprintf (buf, buf_size, "%%r8"); break;
    case 9: snprintf (buf, buf_size, "%%r9"); break;
    case 10: snprintf (buf, buf_size, "%%r10"); break;
    case 11: snprintf (buf, buf_size, "%%r11"); break;
    case 12: snprintf (buf, buf_size, "%%r12"); break;
    case 13: snprintf (buf, buf_size, "%%r13"); break;
    case 14: snprintf (buf, buf_size, "%%r14"); break;
    case 15: snprintf (buf, buf_size, "%%r15"); break;
    default: NOT_REACHED(); snprintf(buf, buf_size, "%d", regnum); break;
  }
  return buf;
}

char *
i386_regname_bytes(long regnum, long nbytes, bool high_order, char *buf, size_t buf_size)
{
  if (nbytes == 1) {
    return i386_regname_byte(regnum, high_order, buf, buf_size);
  }
  if (nbytes == 2) {
    return i386_regname_word(regnum, buf, buf_size);
  }
  if (nbytes == 4) {
    return i386_regname(regnum, buf, buf_size);
  }
  if (nbytes == 8) {
    return x64_regname(regnum, buf, buf_size);
  }
  NOT_REACHED();
}

static int
seg2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (val != -1) {
    ASSERT(val >= 0 && val < I386_NUM_SEGS);
    if (tag == tag_const) {
      ptr += snprintf(ptr, end - ptr, "%%%s", segs[val]);
    } else if (tag == tag_var) {
      ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_SEGS, val);
    } else NOT_REACHED();
  }
  return (ptr - buf);
}

extern "C"
int
default_seg(int base)
{
  switch (base) {
    case R_ESP:
    case R_EBP:
      return R_SS;
    default:
      return R_DS;
  }
  NOT_REACHED();
}


static int
mmx2str(int val, int tag, /*int exvreg_group, */char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_MMX, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%mm%d", val);
  /*} else if (tag == tag_temp) {
    ptr += snprintf(ptr, end - ptr, "%%extr%d.%d", I386_EXREG_GROUP_MMX, val);
  } else if (tag == tag_exvreg) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", exvreg_group, val);*/
  } else NOT_REACHED();
  return ptr - buf;
}

static int
xmm2str(int val, int tag, /*int exvreg_group, */char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_XMM, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%xmm%d", val);
  /*} else if (tag == tag_temp) {
    ptr += snprintf(ptr, end - ptr, "%%extr%d.%d", I386_EXREG_GROUP_XMM, val);
  } else if (tag == tag_exvreg) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", exvreg_group, val);*/
  } else NOT_REACHED();

  return ptr - buf;
}

static int
ymm2str(int val, int tag, /*int exvreg_group, */char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_YMM, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%ymm%d", val);
  /*} else if (tag == tag_temp) {
    ptr += snprintf(ptr, end - ptr, "%%extr%d.%d", I386_EXREG_GROUP_XMM, val);
  } else if (tag == tag_exvreg) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", exvreg_group, val);*/
  } else NOT_REACHED();

  return ptr - buf;
}



static int
cr2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  ptr += snprintf(ptr, end - ptr, tag?"crNN%d":"%%cr%d", val);
  return ptr - buf;
}

static int
db2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  ptr += snprintf(ptr, end - ptr, tag?"dbNN%d":"db%d", val);
  return ptr - buf;
}

static int
tr2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  ptr += snprintf(ptr, end - ptr, tag?"trNN%d":"tr%d", val);
  return ptr - buf;
}

static int
op3dnow2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  ptr += snprintf(ptr, end - ptr, tag?"op3dnowNN%d":"op3dnow%d", val);
  return ptr - buf;
}

static int
prefix2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "prefixNN");
  } else {
    switch (val) {
      case I386_PREFIX_REPZ:  ptr += snprintf(ptr, end - ptr, "repz"); break;
      case I386_PREFIX_REPNZ: ptr += snprintf(ptr, end - ptr, "repnz"); break;
      case I386_PREFIX_LOCK:  ptr += snprintf(ptr, end - ptr, "lock"); break;
      default: NOT_REACHED();
    }
  }
  return ptr - buf;
}

static int
st2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  ASSERT(tag == tag_const);
  /*if (tag) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_ST, val);
  } else {*/
    ptr += snprintf(ptr, end - ptr, "%%st(%d)", val);
  //}
  return ptr - buf;
}

//static int
//flags_unsigned2str(int val, int tag, /*int group, */char *buf, int buflen)
//{
//  char *ptr = buf, *end = buf + buflen;
//  if (tag == tag_var) {
//    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_EFLAGS_UNSIGNED, val);
//    //ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_EFLAGS, val);
//  } else if (tag == tag_const) {
//    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_EFLAGS_UNSIGNED,
//        val);
//  } else NOT_REACHED();
//  return ptr - buf;
//}
//
//static int
//flags_signed2str(int val, int tag, /*int group, */char *buf, int buflen)
//{
//  char *ptr = buf, *end = buf + buflen;
//  if (tag == tag_var) {
//    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_EFLAGS_SIGNED, val);
//    //NOT_REACHED();
//    //ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_EFLAGS, val);
//  } else if (tag == tag_const) {
//    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_EFLAGS_SIGNED,
//        val);
//  } else NOT_REACHED();
//  return ptr - buf;
//}

#define DEF_FLAGS_STR_FN(flagname, groupname) \
static int \
flags_## flagname ## 2str(int val, int tag, char *buf, int buflen) { \
  char *ptr = buf, *end = buf + buflen; \
  if (tag == tag_var) { \
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", groupname, val); \
  } else if (tag == tag_const) { \
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", groupname, val); \
  } else NOT_REACHED(); \
  return ptr - buf; \
}

DEF_FLAGS_STR_FN(other, I386_EXREG_GROUP_EFLAGS_OTHER)
DEF_FLAGS_STR_FN(eq, I386_EXREG_GROUP_EFLAGS_EQ)
DEF_FLAGS_STR_FN(ne, I386_EXREG_GROUP_EFLAGS_NE)
DEF_FLAGS_STR_FN(ult, I386_EXREG_GROUP_EFLAGS_ULT)
DEF_FLAGS_STR_FN(slt, I386_EXREG_GROUP_EFLAGS_SLT)
DEF_FLAGS_STR_FN(ugt, I386_EXREG_GROUP_EFLAGS_UGT)
DEF_FLAGS_STR_FN(sgt, I386_EXREG_GROUP_EFLAGS_SGT)
DEF_FLAGS_STR_FN(ule, I386_EXREG_GROUP_EFLAGS_ULE)
DEF_FLAGS_STR_FN(sle, I386_EXREG_GROUP_EFLAGS_SLE)
DEF_FLAGS_STR_FN(uge, I386_EXREG_GROUP_EFLAGS_UGE)
DEF_FLAGS_STR_FN(sge, I386_EXREG_GROUP_EFLAGS_SGE)

static int
st_top2str(int val, int tag, /*int exvreg_group, */char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  //ASSERT(exvreg_group == I386_EXREG_GROUP_ST_TOP);
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_ST_TOP, val);
    //ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_ST_TOP, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_ST_TOP, val);
  /*} else if (tag == tag_exvreg) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", exvreg_group, val);
  } else if (tag == tag_temp) {
    ptr += snprintf(ptr, end - ptr, "%%extr%d.%d", I386_EXREG_GROUP_ST_TOP, val);*/
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_data_reg2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_DATA_REGS, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_DATA_REGS, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_control_reg_rm2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_CONTROL_REG_RM, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_CONTROL_REG_RM, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_control_reg_other2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_CONTROL_REG_OTHER, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_CONTROL_REG_OTHER, val);
  } else NOT_REACHED();
  return ptr - buf;
}


static int
fp_tag_reg2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_TAG_REG, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_TAG_REG, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_last_instr_pointer2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_LAST_INSTR_POINTER, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_LAST_INSTR_POINTER, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_last_operand_pointer2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_opcode2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_OPCODE_REG, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_OPCODE_REG, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
mxcsr_rm2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_MXCSR_RM, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_MXCSR_RM, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_status_reg_c02str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_C0, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_C0, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_status_reg_c12str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_C1, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_C1, val);
  } else NOT_REACHED();
  return ptr - buf;
}


static int
fp_status_reg_c22str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_C2, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_C2, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_status_reg_c32str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_C3, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_C3, val);
  } else NOT_REACHED();
  return ptr - buf;
}

static int
fp_status_reg_other2str(int val, int tag, char *buf, int buflen)
{
  char *ptr = buf, *end = buf + buflen;
  if (tag == tag_var) {
    ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_OTHER, val);
  } else if (tag == tag_const) {
    ptr += snprintf(ptr, end - ptr, "%%excr%d.%d", I386_EXREG_GROUP_FP_STATUS_REG_OTHER, val);
  } else NOT_REACHED();
  return ptr - buf;
}


static size_t
i386_excreg_to_string(int exreg_group, int val, src_tag_t tag, int size,
    bool rex_used, uint64_t regmask, char *buf, size_t buflen)
{
  ASSERT(tag == tag_const);
  switch (exreg_group) {
    case I386_EXREG_GROUP_GPRS:
      return reg2str(val, tag, size, rex_used, regmask, buf, buflen);
    case I386_EXREG_GROUP_SEGS:
      return seg2str(val, tag, buf, buflen);
    default:
      NOT_REACHED();
  }
}

size_t
operand2str(operand_t const *op, char const *reloc_strings, size_t reloc_strings_size,
    i386_as_mode_t mode, char *buf, size_t size)
{
  char *ptr = buf, *end = buf + size;

  switch (op->type) {
    case invalid:
      DBE(MD_ASSEMBLE, printf("(inval)"));
      break;
    case op_reg:
      ptr += reg2str(op->valtag.reg.val, op->valtag.reg.tag, op->size, op->rex_used,
          op->valtag.reg.mod.reg.mask, ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(reg) %s", buf));
      break;
    case op_seg:
      ptr += seg2str(op->valtag.seg.val, op->valtag.seg.tag, ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(seg) %s", buf));
      break;
    case op_mem:
      ptr += memvt_to_string(&op->valtag.mem, op->rex_used,
          reloc_strings, reloc_strings_size,
          mode,
          &i386_excreg_to_string,
          ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(mem) %s", buf));
      break;
    case op_imm:
      *ptr++ = '$';
      ptr += imm2str(op->valtag.imm.val, op->valtag.imm.tag,
          op->valtag.imm.mod.imm.modifier, op->valtag.imm.mod.imm.constant_val,
          op->valtag.imm.mod.imm.constant_tag,
          op->valtag.imm.mod.imm.sconstants, op->valtag.imm.mod.imm.exvreg_group,
          reloc_strings, reloc_strings_size,
          mode, ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(imm) %s", buf));
      break;
    case op_pcrel:
      ptr += pcrel2str(op->valtag.pcrel.val, op->valtag.pcrel.tag, reloc_strings,
          reloc_strings_size, ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(pcrel) %s", buf));
      break;
    case op_cr:
      ptr += cr2str(op->valtag.cr.val, op->valtag.cr.tag, ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(cr) %s", buf));
      break;
    case op_db:
      ptr += db2str(op->valtag.db.val, op->valtag.db.tag, ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(db) %s", buf));
      break;
    case op_tr:
      ptr += tr2str(op->valtag.tr.val, op->valtag.tr.tag, ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(tr) %s", buf));
      break;
    case op_mmx:
      ptr += mmx2str(op->valtag.mmx.val, op->valtag.mmx.tag,
          ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(mm) %s", buf));
      break;
    case op_xmm:
      ptr += xmm2str(op->valtag.xmm.val, op->valtag.xmm.tag,
          ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(xmm) %s", buf));
      break;
    case op_ymm:
      ptr += ymm2str(op->valtag.ymm.val, op->valtag.ymm.tag,
          ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(ymm) %s", buf));
      break;
    case op_d3now:
      ptr += op3dnow2str(op->valtag.d3now.val, op->valtag.d3now.tag, ptr, end - ptr);
      DBE(MD_ASSEMBLE, printf("(3dnow) %s", buf));
      break;
    case op_prefix:
      ptr += prefix2str(op->valtag.prefix.val, op->valtag.prefix.tag, ptr, end - ptr);
      break;
    case op_st:
      ptr += st2str(op->valtag.st.val, op->valtag.st.tag, ptr, end - ptr);
      break;
#define case_flag(flagname) \
    case op_flags_##flagname: \
      ptr += flags_##flagname##2str(op->valtag.flags_##flagname.val, op->valtag.flags_##flagname.tag, ptr, end - ptr); \
      break

    case_flag(other);
    case_flag(eq);
    case_flag(ne);
    case_flag(ult);
    case_flag(slt);
    case_flag(ugt);
    case_flag(sgt);
    case_flag(ule);
    case_flag(sle);
    case_flag(uge);
    case_flag(sge);
    //case op_flags_unsigned:
    //  ptr += flags_unsigned2str(op->valtag.flags_unsigned.val,
    //      op->valtag.flags_unsigned.tag, ptr, end - ptr);
    //  break;
    //case op_flags_signed:
    //  ptr += flags_signed2str(op->valtag.flags_signed.val, op->valtag.flags_signed.tag,
    //      ptr, end - ptr);
    //  break;
    case op_st_top:
      ptr += st_top2str(op->valtag.st_top.val, op->valtag.st_top.tag,
          ptr, end - ptr);
      break;
    case op_fp_data_reg:
      ptr += fp_data_reg2str(op->valtag.fp_data_reg.val, op->valtag.fp_data_reg.tag,
          ptr, end - ptr);
      break;
    case op_fp_control_reg_rm:
      ptr += fp_control_reg_rm2str(op->valtag.fp_control_reg_rm.val, op->valtag.fp_control_reg_rm.tag,
          ptr, end - ptr);
      break;
    case op_fp_control_reg_other:
      ptr += fp_control_reg_other2str(op->valtag.fp_control_reg_other.val, op->valtag.fp_control_reg_other.tag,
          ptr, end - ptr);
      break;
    case op_fp_tag_reg:
      ptr += fp_tag_reg2str(op->valtag.fp_tag_reg.val, op->valtag.fp_tag_reg.tag,
          ptr, end - ptr);
      break;
    case op_fp_last_instr_pointer:
      ptr += fp_last_instr_pointer2str(op->valtag.fp_last_instr_pointer.val, op->valtag.fp_last_instr_pointer.tag,
          ptr, end - ptr);
      break;
    case op_fp_last_operand_pointer:
      ptr += fp_last_operand_pointer2str(op->valtag.fp_last_operand_pointer.val, op->valtag.fp_last_operand_pointer.tag,
          ptr, end - ptr);
      break;
    case op_fp_opcode:
      ptr += fp_opcode2str(op->valtag.fp_opcode.val, op->valtag.fp_opcode.tag,
          ptr, end - ptr);
      break;
    case op_mxcsr_rm:
      ptr += mxcsr_rm2str(op->valtag.mxcsr_rm.val, op->valtag.mxcsr_rm.tag,
          ptr, end - ptr);
      break;
    case op_fp_status_reg_c0:
      ptr += fp_status_reg_c02str(op->valtag.fp_status_reg_c0.val, op->valtag.fp_status_reg_c0.tag,
          ptr, end - ptr);
      break;
    case op_fp_status_reg_c1:
      ptr += fp_status_reg_c12str(op->valtag.fp_status_reg_c1.val, op->valtag.fp_status_reg_c1.tag,
          ptr, end - ptr);
      break;
    case op_fp_status_reg_c2:
      ptr += fp_status_reg_c22str(op->valtag.fp_status_reg_c2.val, op->valtag.fp_status_reg_c2.tag,
          ptr, end - ptr);
      break;
    case op_fp_status_reg_c3:
      ptr += fp_status_reg_c32str(op->valtag.fp_status_reg_c3.val, op->valtag.fp_status_reg_c3.tag,
          ptr, end - ptr);
      break;
    case op_fp_status_reg_other:
      ptr += fp_status_reg_other2str(op->valtag.fp_status_reg_other.val, op->valtag.fp_status_reg_other.tag,
          ptr, end - ptr);
      break;
    default:
      printf("Invalid op->type[%d]\n", op->type);
      ASSERT(0);
  }

  return ptr - buf;
}

static opc_t opc_jmp, opc_je, opc_jne, opc_jg, opc_jle, opc_jl, opc_jge, opc_ja, opc_jbe,
      opc_jb, opc_jae, opc_jo, opc_jno, opc_js, opc_jns, opc_jp,
      opc_jnp, opc_calll/*, opc_jcxz*/;
static opc_t jmp_opcodes[100];
int num_jmp_opcodes = 0;

static opc_t opc_sete, opc_setne, opc_setg, opc_setle, opc_setl, opc_setge, opc_seta,
      opc_setbe, opc_setb, opc_setae, opc_seto, opc_setno, opc_sets, opc_setns,
      opc_setp, opc_setpe, opc_setpo, opc_setnp;
static opc_t setcc_opcodes[100];
int num_setcc_opcodes = 0;

static opc_t opc_cmovel, opc_cmovnel, opc_cmovgl, opc_cmovlel, opc_cmovll, opc_cmovgel, opc_cmoval,
      opc_cmovbel, opc_cmovbl, opc_cmovael, opc_cmovol, opc_cmovnol, opc_cmovsl, opc_cmovnsl,
      opc_cmovpl, opc_cmovpel, opc_cmovpol, opc_cmovnpl;
static opc_t opc_cmovew, opc_cmovnew, opc_cmovgw, opc_cmovlew, opc_cmovlw, opc_cmovgew, opc_cmovaw,
      opc_cmovbew, opc_cmovbw, opc_cmovaew, opc_cmovow, opc_cmovnow, opc_cmovsw, opc_cmovnsw,
      opc_cmovpw, opc_cmovpew, opc_cmovpow, opc_cmovnpw;
static opc_t cmovcc_opcodes[100];
int num_cmovcc_opcodes = 0;

extern "C" void
i386_insn_init_constants(void)
{
  autostop_timer func_timer(__func__);
#define INIT_OPC(name) opc_##name = opc_nametable_find(#name)
#define INIT_OPC_CC(name) do { \
  INIT_OPC(j##name); \
  INIT_OPC(set##name); \
  INIT_OPC(cmov##name##l); \
  INIT_OPC(cmov##name##w); \
  jmp_opcodes[num_jmp_opcodes++] = opc_j##name; \
  ASSERT(num_jmp_opcodes < sizeof jmp_opcodes); \
  setcc_opcodes[num_setcc_opcodes++] = opc_set##name; \
  ASSERT(num_setcc_opcodes < sizeof setcc_opcodes); \
  cmovcc_opcodes[num_cmovcc_opcodes++] = opc_cmov##name##l; \
  ASSERT(num_cmovcc_opcodes < sizeof cmovcc_opcodes); \
  cmovcc_opcodes[num_cmovcc_opcodes++] = opc_cmov##name##w; \
  ASSERT(num_cmovcc_opcodes < sizeof cmovcc_opcodes); \
} while (0)

  INIT_OPC(jmp);
  jmp_opcodes[num_jmp_opcodes++] = opc_jmp;

  INIT_OPC_CC(e);
  INIT_OPC_CC(ne);
  INIT_OPC_CC(g);
  INIT_OPC_CC(le);
  INIT_OPC_CC(l);
  INIT_OPC_CC(ge);
  INIT_OPC_CC(a);
  INIT_OPC_CC(be);
  INIT_OPC_CC(b);
  INIT_OPC_CC(ae);
  INIT_OPC_CC(o);
  INIT_OPC_CC(no);
  INIT_OPC_CC(s);
  INIT_OPC_CC(ns);
  INIT_OPC_CC(p);
  //INIT_OPC_CC(pe);
  //INIT_OPC_CC(po);
  INIT_OPC_CC(np);

  INIT_OPC(calll);
  //INIT_OPC(jcxz);
}

bool
i386_insn_is_nop(i386_insn_t const *insn)
{
  char const *opc;
  int n;

  opc = opctable_name(insn->opc);
  n = i386_insn_num_implicit_operands(insn);
  if (   (   strstart(opc, "xchg", NULL)
          || strstart(opc, "mov", NULL)
          || strstart(opc, "cmov", NULL))
      && insn->op[n].type == op_reg
      && insn->op[n + 1].type == op_reg
      && insn->op[n].valtag.reg.val == insn->op[n + 1].valtag.reg.val
      && insn->op[n].size == insn->op[n + 1].size
      && insn->op[n].valtag.reg.mod.reg.mask
             == insn->op[n + 1].valtag.reg.mod.reg.mask) {
    return true;
  }
  if (!strcmp(opc, "lea") || !strcmp(opc, "leal")) {
    if (   insn->op[0].type != op_reg
        || insn->op[1].type != op_mem) { //this can happen due to replace_tmap_loc_with_const
      return false;
    }
    ASSERT(insn->op[0].type == op_reg);
    ASSERT(insn->op[1].type == op_mem);
    /*if (insn->op[1].valtag.mem.disp.val == 0) {
      printf("%s() %d: insn = %s\n", __func__, __LINE__, i386_insn_to_string(insn, as1, sizeof as1));
      printf("insn->op[0].valtag.reg.val = %d\n", insn->op[0].valtag.reg.val);
      printf("insn->op[1].valtag.mem.base.val = %d\n", insn->op[1].valtag.mem.base.val);
      printf("insn->op[1].valtag.mem.index.val = %d\n", insn->op[1].valtag.mem.index.val);
    }*/
    if (   insn->op[1].valtag.mem.base.val == insn->op[0].valtag.reg.val
        && insn->op[1].valtag.mem.index.val == -1
        && insn->op[1].valtag.mem.disp.val == 0
        && insn->op[1].valtag.mem.disp.tag == tag_const) {
      return true;
    }
  }
  if (strstart(opc, "nop", NULL)) {
    return true;
  }
  return false;
}

static bool
i386_insn_is_cmpxchg(i386_insn_t const *insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);
  if (strstart(opc, "cmpxchg", NULL)) {
    return true;
  }
  return false;
}

bool
i386_insn_is_intmul(i386_insn_t const *insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);
  if (   strstart(opc, "imul", NULL)
      || (strstart(opc, "mul", NULL) && opc[3] != 'p' && opc[3] != 's')) {
    return true;
  }
  return false;
}

bool
i386_insn_is_intdiv(i386_insn_t const *insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);
  if (   strstart(opc, "idiv", NULL)
      || (strstart(opc, "div", NULL) && opc[3] != 'p' && opc[3] != 's')) {
    return true;
  }
  return false;
}

static bool
i386_insn_is_setcc(i386_insn_t const *insn)
{
  opc_t *setcc_opc;
  for (setcc_opc = setcc_opcodes;
      setcc_opc < setcc_opcodes + num_setcc_opcodes;
      setcc_opc++) {
    if (insn->opc == *setcc_opc) {
      return true;
    }
  }
  return false;
}

static bool
i386_insn_is_conditional_move(i386_insn_t const *insn)
{
  char const *opc;

  opc = opctable_name(insn->opc);
  if (strstart(opc, "cmov", NULL)) {
    return true;
  }
  return false;
}

static bool
i386_is_string_opcode(char const *opc)
{
  if (   strstart(opc, "lods", NULL)
      || strstart(opc, "stos", NULL)
      || strstart(opc, "scas", NULL)
      || (   strstart(opc, "cmps", NULL)
          && (   opc[4] == 'b'
              || opc[4] == 'w'
              || opc[4] == 'l'
              || opc[4] == 'q'))
      || !strcmp(opc, "movsb")
      || !strcmp(opc, "movsw")
      || !strcmp(opc, "movsl")
      || !strcmp(opc, "movsq")
      || !strcmp(opc, "insb")
      || !strcmp(opc, "insw")
      || !strcmp(opc, "insl")
      || !strcmp(opc, "outsb")
      || !strcmp(opc, "outsw")
      || !strcmp(opc, "outsl")) {
    return true;
  }
  return false;
}

static void
i386_insn_get_usedef_flag_group(struct i386_insn_t const *insn,
    struct regset_t *use, struct regset_t *def, exreg_group_id_t flag_group)
{
  bool use_found, def_found;
  //opertype_t flags_type;
  char const *remaining;
  //src_tag_t tag_flags;
  char const *opc;
  int i;

  opc = opctable_name(insn->opc);
  //flags_type = is_signed ? op_flags_signed : op_flags_unsigned;

  use_found = false;
  def_found = false;

//#define SET_USE_COND(suffix) do { \
//  if (   insn->opc == opc_j ## suffix \
//      || insn->opc == opc_set ## suffix \
//      || insn->opc == opc_cmov ## suffix ## l \
//      || insn->opc == opc_cmov ## suffix ## w) { \
//    use_found = true; \
//  } \
//} while(0)
//
//  if (flag_group == I386_EXREG_GROUP_EFLAGS_EQ) {
//    SET_USE_COND(e);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_NE) {
//    SET_USE_COND(ne);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_ULT) {
//    SET_USE_COND(b);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_SLT) {
//    SET_USE_COND(l);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_UGT) {
//    SET_USE_COND(a);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_SGT) {
//    SET_USE_COND(g);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_ULE) {
//    SET_USE_COND(be);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_SLE) {
//    SET_USE_COND(le);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_UGE) {
//    SET_USE_COND(ae);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_SGE) {
//    SET_USE_COND(ge);
//  } else if (flag_group == I386_EXREG_GROUP_EFLAGS_OTHER) {
//    // string instructions use direction flag
//    if (i386_is_string_opcode(opc)) {
//      use_found = true;
//    }
//    if (   strstart(opc, "adc", NULL)
//        || strstart(opc, "sbb", NULL)) {
//      use_found = true;
//    }
//    if (   strstart(opc, "cmc", NULL)
//        || !strcmp(opc, "aaa")
//        || !strcmp(opc, "aas")
//        || !strcmp(opc, "daa")
//        || !strcmp(opc, "das")) {
//      use_found = true;
//    }
//    if (   strstart(opc, "lahf", NULL)
//        || strstart(opc, "pushf", NULL)) {
//      use_found = true;
//    }
//    if (   strstart(opc, "clc", NULL)
//        || strstart(opc, "cmc", NULL)
//        || strstart(opc, "stc", NULL)
//        || strstart(opc, "cld", NULL)
//        || strstart(opc, "std", NULL)) {
//      def_found = true;
//    }
//  } else NOT_REACHED();


  if (   (strstart(opc, "j", NULL) && insn->opc != opc_jmp && !i386_insn_is_jecxz(insn))
      || strstart(opc, "set", NULL)
      || strstart(opc, "cmov", NULL)) {
    use_found = true;
  }

  if (   (   strstart(opc, "cmp", NULL)
          && (   opc[3] == 'b'
              || opc[3] == 'w'
              || opc[3] == 'l'
              || opc[3] == 'q'
              || opc[3] == 'x'
              || (   (   opc[3] == 's'
                      || opc[3] == 'd')
                  && (   opc[4] == 'b'
                      || opc[4] == 'w'
                      || opc[4] == 'l'
                      || opc[4] == 'q'))))
      || strstart(opc, "test", NULL)
      || strstart(opc, "addb", NULL)
      || strstart(opc, "addw", NULL)
      || strstart(opc, "addl", NULL)
      || strstart(opc, "subb", NULL)
      || strstart(opc, "subw", NULL)
      || strstart(opc, "subl", NULL)
      || strstart(opc, "adc", NULL)
      || strstart(opc, "sbb", NULL)
      || strstart(opc, "inc", NULL)
      || strstart(opc, "dec", NULL)
      || strstart(opc, "neg", NULL)
      || (strstart(opc, "or", NULL) && !strstart(opc, "orp", NULL))
      || (strstart(opc, "and", NULL) && !strstart(opc, "andp", NULL) && !strstart(opc, "andnp", NULL))
      || strstart(opc, "test", NULL)
      || (strstart(opc, "xor", NULL) && !strstart(opc, "xorp", NULL))
      || (strstart(opc, "shr", &remaining) && *remaining != 'x')
      || (strstart(opc, "shl", &remaining) && *remaining != 'x')
      || (strstart(opc, "sar", &remaining) && *remaining != 'x')
      || strstart(opc, "rol", NULL)
      || (strstart(opc, "ror", &remaining) && *remaining != 'x')
      || strstart(opc, "rcl", NULL)
      || strstart(opc, "rcr", NULL)
      || strstart(opc, "mulb", NULL)
      || strstart(opc, "mulw", NULL)
      || strstart(opc, "mull", NULL)
      || strstart(opc, "imul", NULL)
      || strstart(opc, "divb", NULL)
      || strstart(opc, "divw", NULL)
      || strstart(opc, "divl", NULL)
      || strstart(opc, "idiv", NULL)
      || strstart(opc, "aaa", NULL)
      || strstart(opc, "aas", NULL)
      || strstart(opc, "aad", NULL)
      || strstart(opc, "aam", NULL)
      || strstart(opc, "daa", NULL)
      || strstart(opc, "das", NULL)
      || strstart(opc, "bt", NULL)
      || strstart(opc, "bsr", NULL)
      || strstart(opc, "bsf", NULL)
      || strstart(opc, "xadd", NULL)
      || strstart(opc, "sahf", NULL)
      || strstart(opc, "scas", NULL)
      || (   strstart(opc, "cmps", NULL)
          && (   opc[4] == 'b'
              || opc[4] == 'w'
              || opc[4] == 'l'
              || opc[4] == 'q'))
      || strstart(opc, "popf", NULL)
      //bmi
      || (strstart(opc, "andn", NULL) && !strstart(opc, "andnp", NULL))
      || strstart(opc, "bextr", NULL)
      || strstart(opc, "blsi", NULL)
      || strstart(opc, "blsmsk", NULL)
      || strstart(opc, "blsr", NULL)
      || strstart(opc, "bzhi", NULL)
      || strstart(opc, "lzcnt", NULL)
      || strstart(opc, "tzcnt", NULL)
      //popcnt
      || strstart(opc, "popcnt", NULL)
      //ptest
      || strstart(opc, "ptest", NULL)
      //float comparisons
      || strstart(opc, "fcomi", NULL)
      || strstart(opc, "fucomi", NULL)
      || !strcmp(opc, "ucomiss")
      || !strcmp(opc, "ucomisd")
      || !strcmp(opc, "comiss")
      || !strcmp(opc, "comisd")
     ) {
    def_found = true;
  }

  DBG2(IMPLICIT_OPS, "flag_group = %d, use_found = %d, def_found = %d\n", flag_group,
      use_found, def_found);
  if (!use_found && !def_found) {
    return;
  }
  if (use_found) {
    //if (is_signed) {
    //  regset_mark_ex_mask(use, I386_EXREG_GROUP_EFLAGS_SIGNED, 0,
    //      MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_EFLAGS_SIGNED)));
    //} else {
    //  regset_mark_ex_mask(use, I386_EXREG_GROUP_EFLAGS_UNSIGNED, 0,
    //      MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_EFLAGS_UNSIGNED)));
    //}
    regset_mark_ex_mask(use, flag_group, 0,
        MAKE_MASK(I386_EXREG_LEN(flag_group)));
  }
  if (def_found) {
    //if (is_signed) {
    //  regset_mark_ex_mask(def, I386_EXREG_GROUP_EFLAGS_SIGNED, 0,
    //      MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_EFLAGS_SIGNED)));
    //} else {
    //  regset_mark_ex_mask(def, I386_EXREG_GROUP_EFLAGS_UNSIGNED, 0,
    //      MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_EFLAGS_UNSIGNED)));
    //  DBG2(IMPLICIT_OPS, "def = %s\n", regset_to_string(def, as1, sizeof as1));
    //}
    regset_mark_ex_mask(def, flag_group, 0,
        MAKE_MASK(I386_EXREG_LEN(flag_group)));
  }
  DBG2(IMPLICIT_OPS, "def = %s\n", regset_to_string(def, as1, sizeof as1));
}

/*static void
i386_insn_get_usedef_flags_unsigned(struct i386_insn_t const *insn,
    struct regset_t *use, struct regset_t *def)
{
  i386_insn_get_usedef_flags_signed_unsigned(insn, use, def, false);
}

static void
i386_insn_get_usedef_flags_signed(struct i386_insn_t const *insn,
    struct regset_t *use, struct regset_t *def)
{
  i386_insn_get_usedef_flags_signed_unsigned(insn, use, def, true);
}*/

static bool
i386_insn_accesses_fp_control_reg_rm(i386_insn_t const* insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);

  if (   false
      || strstart(opc, "fadd", NULL)
      || strstart(opc, "fiadd", NULL)

      || strstart(opc, "fsub", NULL)
      || strstart(opc, "fisub", NULL)

      || strstart(opc, "fsubr", NULL)
      || strstart(opc, "fisubr", NULL)

      || !strcmp(opc, "fist")
      || !strcmp(opc, "fists")
      || !strcmp(opc, "fistl")
      || !strcmp(opc, "fistp")
      || !strcmp(opc, "fistps")
      || !strcmp(opc, "fistpl")
      || !strcmp(opc, "fistpll")

      || !strcmp(opc, "fld1")
      || !strcmp(opc, "fldl2t")
      || !strcmp(opc, "fldl2e")
      || !strcmp(opc, "fldpi")
      || !strcmp(opc, "fldlg2")
      || !strcmp(opc, "fldln2")
      || !strcmp(opc, "fldz")

      || strstart(opc, "fmul", NULL)
      || strstart(opc, "fimul", NULL)
      || strstart(opc, "fdiv", NULL)
      || strstart(opc, "fidiv", NULL)
      || !strcmp(opc, "fpatan")
      || !strcmp(opc, "fptan")
      || !strcmp(opc, "frndint")
      || !strcmp(opc, "fsin")
      || !strcmp(opc, "fsincos")
      || !strcmp(opc, "fsqrt")

      || !strcmp(opc, "fyl2x")
      || !strcmp(opc, "fyl2xp1")

      || strstart(opc, "fdivr", NULL)
      || strstart(opc, "fidivr", NULL)

      //for fst{p} opcodes, if the dest is smaller (dword/qword_len), then we need the rounding mode
      || !strcmp(opc, "fsts")
      || !strcmp(opc, "fstl")
      || !strcmp(opc, "fstps")
      || !strcmp(opc, "fstpl")

      || !strcmp(opc, "haddpd")
      || !strcmp(opc, "haddps")
      || !strcmp(opc, "hsubpd")
      || !strcmp(opc, "hsubps")
     ) {
    return true;
  }
  return false;
}

static bool
i386_insn_accesses_fp_status_reg_c0_only(i386_insn_t const* insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);

  if (   false
      || strstart(opc, "fcomi", NULL)
      || strstart(opc, "fucomi", NULL)
     ) {
    return true;
  }
  return false;
}

static bool
i386_insn_accesses_fp_status_reg_condition_codes(i386_insn_t const* insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);

  if (   false
      || !strcmp(opc, "fcom")
      || !strcmp(opc, "fcoms")
      || !strcmp(opc, "fcoml")
      || !strcmp(opc, "fcomp")
      || !strcmp(opc, "fcomps")
      || !strcmp(opc, "fcompl")
      || !strcmp(opc, "fcompp")

      || !strcmp(opc, "fucom")
      || !strcmp(opc, "fucomp")
      || !strcmp(opc, "fucompp")

      || !strcmp(opc, "fild")
      || !strcmp(opc, "fildl")
      || !strcmp(opc, "fildll")

      || !strcmp(opc, "fld")
      || !strcmp(opc, "flds")
      || !strcmp(opc, "fldl")
      || !strcmp(opc, "fldt")

      || !strcmp(opc, "fst")
      || !strcmp(opc, "fsts")
      || !strcmp(opc, "fstl")

      || !strcmp(opc, "fstp")
      || !strcmp(opc, "fstps")
      || !strcmp(opc, "fstpl")
      || !strcmp(opc, "fstpt")

      || !strcmp(opc, "fist")
      || !strcmp(opc, "fists")
      || !strcmp(opc, "fistl")
      || !strcmp(opc, "fistp")
      || !strcmp(opc, "fistps")
      || !strcmp(opc, "fistpl")
      || !strcmp(opc, "fistpll")

      || !strcmp(opc, "fisttp")
      || !strcmp(opc, "fisttpl")
      || !strcmp(opc, "fisttpll")

      || !strcmp(opc, "fadd")
      || !strcmp(opc, "fadds")
      || !strcmp(opc, "faddl")
      || !strcmp(opc, "faddp")
      || !strcmp(opc, "fiadd")
      || !strcmp(opc, "fiadds")
      || !strcmp(opc, "fiaddl")
      || !strcmp(opc, "fsub")
      || !strcmp(opc, "fsubs")
      || !strcmp(opc, "fsubl")
      || !strcmp(opc, "fsubp")
      || !strcmp(opc, "fisub")
      || !strcmp(opc, "fisubs")
      || !strcmp(opc, "fisubl")
      || !strcmp(opc, "fisubp")
      || !strcmp(opc, "fsubr")
      || !strcmp(opc, "fsubrs")
      || !strcmp(opc, "fsubrl")
      || !strcmp(opc, "fsubrp")
      || !strcmp(opc, "fisubr")
      || !strcmp(opc, "fisubrs")
      || !strcmp(opc, "fisubrl")
      || !strcmp(opc, "fisubrp")
      || !strcmp(opc, "fmul")
      || !strcmp(opc, "fmuls")
      || !strcmp(opc, "fmull")
      || !strcmp(opc, "fmulp")
      || !strcmp(opc, "fimul")
      || !strcmp(opc, "fimuls")
      || !strcmp(opc, "fimull")
      || !strcmp(opc, "fimulp")
      || !strcmp(opc, "fdiv")
      || !strcmp(opc, "fdivs")
      || !strcmp(opc, "fdivl")
      || !strcmp(opc, "fdivp")
      || !strcmp(opc, "fidiv")
      || !strcmp(opc, "fidivs")
      || !strcmp(opc, "fidivl")
      || !strcmp(opc, "fidivp")
      || !strcmp(opc, "fdivr")
      || !strcmp(opc, "fdivrs")
      || !strcmp(opc, "fdivrl")
      || !strcmp(opc, "fdivrp")
      || !strcmp(opc, "fidivr")
      || !strcmp(opc, "fidivrs")
      || !strcmp(opc, "fidivrl")
      || !strcmp(opc, "fidivrp")

     ) {
    return true;
  }
  //NOT_IMPLEMENTED();
  return false;
}

static bool
i386_insn_accesses_mxcsr_rm(i386_insn_t const* insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);

  if (   false
      || !strcmp(opc, "cvtdq2pd")
      || !strcmp(opc, "cvtdq2ps")
      || !strcmp(opc, "cvtss2si")
      || strstart(opc, "cvtsi2ss", NULL)
      || strstart(opc, "cvtsi2sd", NULL)
      || strstart(opc, "cvtpi2pd", NULL)
      || !strcmp(opc, "cvtsd2ss")
      || !strcmp(opc, "cvtsd2si")
      || !strcmp(opc, "cvtps2pi")
      || !strcmp(opc, "cvtps2dq")
      || !strcmp(opc, "cvtpi2ps")
      || !strcmp(opc, "cvtpd2ps")
      || !strcmp(opc, "cvtpd2pi")
      || !strcmp(opc, "cvtpd2dq")

      || !strcmp(opc, "addps")
      || !strcmp(opc, "addpd")
      || !strcmp(opc, "addss")
      || !strcmp(opc, "addsd")
      || !strcmp(opc, "subps")
      || !strcmp(opc, "subpd")
      || !strcmp(opc, "subss")
      || !strcmp(opc, "subsd")
      || !strcmp(opc, "mulps")
      || !strcmp(opc, "mulpd")
      || !strcmp(opc, "mulss")
      || !strcmp(opc, "mulsd")
      || !strcmp(opc, "divps")
      || !strcmp(opc, "divpd")
      || !strcmp(opc, "divss")
      || !strcmp(opc, "divsd")
     ) {
    return true;
  }
  //printf("%s() %d: returning false for %s\n", __func__, __LINE__, opc);
  return false;
}

static bool
i386_insn_accesses_flag_group(i386_insn_t const *insn, exreg_group_id_t flag_group)
{
  regset_t use, def;

  regset_clear(&use);
  regset_clear(&def);

  i386_insn_get_usedef_flag_group(insn, &use, &def, flag_group);

  DBG(IMPLICIT_OPS, "use=%s\n", regset_to_string(&use, as1, sizeof as1));
  DBG(IMPLICIT_OPS, "def=%s\n", regset_to_string(&def, as1, sizeof as1));
  DBG(IMPLICIT_OPS, "insn->num_implicit_ops=%d\n", insn->num_implicit_ops);

  if (!regset_equal(&use, regset_empty())) {
    return true;
  }
  if (!regset_equal(&def, regset_empty())) {
    return true;
  }
  return false;
}

//static bool
//i386_insn_accesses_flags_unsigned(i386_insn_t const *insn)
//{
//  return i386_insn_accesses_flags_signed_unsigned(insn, false);
//}
//
//static bool
//i386_insn_accesses_flags_signed(i386_insn_t const *insn)
//{
//  return i386_insn_accesses_flags_signed_unsigned(insn, true);
//}

static void
i386_insn_get_usedef_st_top(i386_insn_t const *insn, regset_t *use,
    regset_t *def)
{
  bool use_found, def_found;
  //src_tag_t tag_st_top;
  char const *opc;
  int i;

  opc = opctable_name(insn->opc);

  /*tag_st_top = tag_invalid;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_st_top) {
      ASSERT(tag_st_top == tag_invalid);
      tag_st_top = insn->op[i].valtag.st_top.tag;
    }
  }*/

  use_found = false;
  def_found = false;

  if (strstart(opc, "fld", NULL)) {
    use_found = true;
    def_found = true;
  }
  if (   strstart(opc, "fst", NULL)
      || strstart(opc, "fsts", NULL)
      || strstart(opc, "fstl", NULL)
      || strstart(opc, "fstt", NULL)
      || strstart(opc, "fstp", NULL)) {
      //|| strstart(opc, "fxch", NULL)
      //|| strstart(opc, "fucom", NULL)
      //|| strstart(opc, "fch", NULL))
    use_found = true;
  }

  if (!use_found && !def_found) {
    return;
  }

  /*if (tag_st_top == tag_invalid) {
    // implicit ops must not have been added so far.
    ASSERT(insn->num_implicit_ops == -1);
    tag_st_top = tag_const;
  }*/

  if (use_found) {
    regset_mark_ex_mask(use, I386_EXREG_GROUP_ST_TOP, 0,
        MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_ST_TOP)));
  }
  if (def_found) {
    regset_mark_ex_mask(def, I386_EXREG_GROUP_ST_TOP, 0,
        MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_ST_TOP)));
  }
}

static bool
i386_insn_accesses_st_top(i386_insn_t const *insn)
{
  char const *opc, *suffix;

  opc = opctable_name(insn->opc);
  if (   false
      || strstart(opc, "fld", NULL)
      || strstart(opc, "fst", NULL)
      || strstart(opc, "fnst", NULL)
      || strstart(opc, "fild", NULL)
      || strstart(opc, "fist", NULL)
      || strstart(opc, "fincstp", NULL)
      || strstart(opc, "fdecstp", NULL)
      || strstart(opc, "ficom", NULL)
      || strstart(opc, "fadd", NULL)
      || strstart(opc, "fiadd", NULL)
      || strstart(opc, "fsub", NULL)
      || strstart(opc, "fisub", NULL)
      || strstart(opc, "fmul", NULL)
      || strstart(opc, "fimul", NULL)
      || strstart(opc, "fdiv", NULL)
      || strstart(opc, "fidiv", NULL)
      || strstart(opc, "fchs", NULL)
      || strstart(opc, "fabs", NULL)
      || strstart(opc, "fcmov", NULL)
      || strstart(opc, "fcom", NULL)
      || strstart(opc, "fucom", NULL)
      || strstart(opc, "fcos", NULL)
      || strstart(opc, "fsin", NULL)
      || strstart(opc, "fpatan", NULL)
      || strstart(opc, "fptan", NULL)
      || strstart(opc, "fsincos", NULL)
      || strstart(opc, "fsqrt", NULL)
      || strstart(opc, "ffree", NULL)
      || strstart(opc, "fprem", NULL)
      || strstart(opc, "fsave", NULL)
      || strstart(opc, "fnsave", NULL)
      || strstart(opc, "frstor", NULL)
      || strstart(opc, "frndint", NULL)
      || strstart(opc, "fscale", NULL)
      || strstart(opc, "ftst", NULL)
      || strstart(opc, "fxam", NULL)
      || strstart(opc, "fxchg", NULL)
      || strstart(opc, "fxrstor", NULL)
      || strstart(opc, "fxsave", NULL)
      || strstart(opc, "fxtract", NULL)
      || strstart(opc, "fyl2x", NULL)
      || strstart(opc, "fylxp1", NULL)
     ) {
    return true;
  }
  return false;
}

static int
i386_insn_is_popa_or_pusha(i386_insn_t const *insn)
{
  char const *opc, *suffix;

  opc = opctable_name(insn->opc);
  if (strstart(opc, "pusha", &suffix)) {
    if (*suffix == 'w') {
      return 2;
    } else if (*suffix == 'l') {
      return 4;
    } else NOT_REACHED();
  }
  if (strstart(opc, "popa", &suffix)) {
    if (*suffix == 'w') {
      return 2;
    } else if (*suffix == 'l') {
      return 4;
    } else NOT_REACHED();
  }
  return -1;
}

#define I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, field, group, len) do { \
  if (i386_insn_accesses_flag_group(dst, group)) { \
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid); \
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) { \
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t)); \
    } \
    dst->op[0].type = op_##field; \
    dst->op[0].valtag.field.tag = tag; \
    dst->op[0].valtag.field.val = 0; \
    dst->op[0].valtag.field.mod.reg.mask = MAKE_MASK(len); \
    num_implicit_ops += 1; \
  } \
} while(0)

static void
i386_insn_add_implicit_operands_helper(i386_insn_t *dst, fixed_reg_mapping_t const &fixed_reg_mapping, src_tag_t tag)
{
  int i, num_implicit_ops;
  char const *opc_orig;

  opc_orig = opctable_name(dst->opc);
  if (!opc_orig) { //invalid opcode
    return;
  }
  char opc[strlen(opc_orig) + 16];
  if (strstart(opc_orig, "ret", NULL)) {
    opc[0] = ' ';
    strncpy(&opc[1], opc_orig, sizeof opc - 1);
  } else {
    strncpy(opc, opc_orig, sizeof opc);
  }

  ASSERT(dst->num_implicit_ops == -1);
  num_implicit_ops = 0;

  if (i386_insn_accesses_stack(dst)) {
    int is_popa_or_pusha;
    is_popa_or_pusha = i386_insn_is_popa_or_pusha(dst);
    if (is_popa_or_pusha != -1) {
      ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 7].type == invalid);
      for (i = I386_MAX_NUM_OPERANDS - 8; i >= 0; i--) {
        memcpy(&dst->op[i + 7], &dst->op[i], sizeof(operand_t));
      }
      int popa_pusha_regs[7] = {R_EDI, R_ESI, R_EBP, R_EBX, R_EDX, R_ECX, R_EAX};
      for (i = 0; i < 7; i++) {
        dst->op[i].type = op_reg;
        dst->op[i].valtag.reg.val = popa_pusha_regs[i];
        dst->op[i].valtag.reg.tag = tag;
        dst->op[i].valtag.seg.mod.reg.mask = MASKBITS(0, is_popa_or_pusha * BYTE_LEN- 1);
        dst->op[i].size = is_popa_or_pusha;
      }
      num_implicit_ops += 7;
    }
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
    for (i = I386_MAX_NUM_OPERANDS - 3; i >= 0; i--) {
      memcpy(&dst->op[i + 2], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_seg;
    dst->op[0].valtag.seg.tag = tag;
    exreg_id_t r_ss = fixed_reg_mapping.get_mapping(I386_EXREG_GROUP_SEGS, R_SS);
    if (r_ss == -1) {
      r_ss = R_SS;
    }
    ASSERT(r_ss >= 0 && r_ss < I386_NUM_SEGS);
    dst->op[0].valtag.seg.val = r_ss;
    dst->op[0].valtag.seg.mod.reg.mask = MASKBITS(0, WORD_LEN - 1);
    dst->op[0].size = WORD_LEN/BYTE_LEN;

    dst->op[1].type = op_reg;
    dst->op[1].valtag.reg.tag = tag;
    //dst->op[1].valtag.reg.val = R_ESP;
    dst->op[1].valtag.reg.val = fixed_reg_mapping.get_mapping(I386_EXREG_GROUP_GPRS, R_ESP);
    if (dst->op[1].valtag.reg.val == -1) {
      dst->op[1].valtag.reg.val = R_ESP;
    }
    dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
    ASSERT(dst->op[1].valtag.reg.mod.reg.mask != 0);
    dst->op[1].size = DST_LONG_SIZE;
    num_implicit_ops += 2;

    if (strstart(opc, "leave", NULL) || strstart(opc, "enter", NULL)) {
      //add R_EBP
      ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
      for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
        memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
      }
      dst->op[0].type = op_reg;
      dst->op[0].valtag.reg.tag = tag;
      dst->op[0].valtag.reg.val = R_EBP;
      dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      ASSERT(dst->op[0].valtag.reg.mod.reg.mask != 0);
      dst->op[0].size = DST_LONG_SIZE;
      num_implicit_ops += 1;
    }
    if (strstart(opc, "call", NULL)) {
      /* add caller-save registers. */
      for (i = I386_MAX_NUM_OPERANDS - 4; i >= 0; i--) {
        memcpy(&dst->op[i + 3], &dst->op[i], sizeof(operand_t));
      }
      dst->op[0].type = op_reg;
      dst->op[0].valtag.reg.tag = tag;
      dst->op[0].valtag.reg.val = R_EAX;
      dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[0].size = DST_LONG_SIZE;

      dst->op[1].type = op_reg;
      dst->op[1].valtag.reg.tag = tag;
      dst->op[1].valtag.reg.val = R_ECX;
      dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[1].size = DST_LONG_SIZE;

      dst->op[2].type = op_reg;
      dst->op[2].valtag.reg.tag = tag;
      dst->op[2].valtag.reg.val = R_EDX;
      dst->op[2].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[2].size = DST_LONG_SIZE;

      num_implicit_ops += 3;
    } else if (strstart(opc, " ret", NULL)) {
      /* add callee-save registers. */
      for (i = I386_MAX_NUM_OPERANDS - 7; i >= 0; i--) {
        memcpy(&dst->op[i + 6], &dst->op[i], sizeof(operand_t));
      }
      //callee-save registers
      dst->op[0].type = op_reg;
      dst->op[0].valtag.reg.tag = tag;
      dst->op[0].valtag.reg.val = R_EBX;
      dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[0].size = DST_LONG_SIZE;

      dst->op[1].type = op_reg;
      dst->op[1].valtag.reg.tag = tag;
      dst->op[1].valtag.reg.val = R_EBP;
      dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[1].size = DST_LONG_SIZE;

      dst->op[2].type = op_reg;
      dst->op[2].valtag.reg.tag = tag;
      dst->op[2].valtag.reg.val = R_ESI;
      dst->op[2].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[2].size = DST_LONG_SIZE;

      dst->op[3].type = op_reg;
      dst->op[3].valtag.reg.tag = tag;
      dst->op[3].valtag.reg.val = R_EDI;
      dst->op[3].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[3].size = DST_LONG_SIZE;

      //return value registers
      dst->op[4].type = op_reg;
      dst->op[4].valtag.reg.tag = tag;
      dst->op[4].valtag.reg.val = R_EAX;
      dst->op[4].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[4].size = DST_LONG_SIZE;

      dst->op[5].type = op_reg;
      dst->op[5].valtag.reg.tag = tag;
      dst->op[5].valtag.reg.val = R_EDX;
      dst->op[5].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[5].size = DST_LONG_SIZE;

      num_implicit_ops += 6;
    }

  } else if (i386_insn_is_jecxz(dst)) {
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_ECX;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
    dst->op[0].size = DST_LONG_SIZE;
    num_implicit_ops += 1;
  } else if (strstart(opc, "mulx", NULL)) {
    int i;
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      i386_operand_copy(&dst->op[i + 1], &dst->op[i]);
    }
    ASSERT(dst->op[1].type == op_reg);
    i386_operand_copy(&dst->op[0], &dst->op[1]);
    dst->op[0].valtag.reg.val = R_EDX;
    num_implicit_ops += 1;
  } else if (   (i386_insn_is_intmul(dst) && dst->op[1].type == invalid)
             || i386_insn_is_intdiv(dst)) {
    ASSERT(dst->op[1].type == invalid);
    ASSERT(dst->op[2].type == invalid);
    i386_operand_copy(&dst->op[2], &dst->op[0]);
    dst->op[0].type = dst->op[1].type = op_reg;
    dst->op[0].valtag.reg.tag = dst->op[1].valtag.reg.tag = tag;
    dst->op[1].valtag.reg.val = R_EAX;
    if (opc[strlen(opc) - 1] == 'b') {
      dst->op[0].valtag.reg.val = R_EAX;
      dst->op[0].size = 1;
      dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(BYTE_LEN, WORD_LEN - 1);
      dst->op[1].size = 1;
      dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, BYTE_LEN - 1);
    } else {
      int size;
      if (opc[strlen(opc) - 1] == 'w') {
        size = WORD_LEN/BYTE_LEN;
        dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, WORD_LEN - 1);
        dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, WORD_LEN - 1);
      } else if (opc[strlen(opc) - 1] == 'l') {
        size = DWORD_LEN/BYTE_LEN;
        dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, DWORD_LEN - 1);
        dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, DWORD_LEN - 1);
      } else if (opc[strlen(opc) - 1] == 'q') {
        size = QWORD_LEN/BYTE_LEN;
        dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, QWORD_LEN - 1);
        dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, QWORD_LEN - 1);
      } else NOT_REACHED();
      dst->op[0].valtag.reg.val = R_EDX;
      dst->op[0].size = dst->op[1].size = size;
    }
    num_implicit_ops += 2;
  } else if (i386_insn_is_cmpxchg(dst)) {
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EAX;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, dst->op[2].size*BYTE_LEN - 1);
    dst->op[0].size = dst->op[2].size;
    num_implicit_ops += 1;
  } else if (   !strcmp(opc, "lahf")
             || !strcmp(opc, "sahf")) {
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EAX;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(BYTE_LEN, WORD_LEN - 1);
    dst->op[0].size = 1;
    num_implicit_ops += 1;
  } else if (   !strcmp(opc, "aad")
             || !strcmp(opc, "aam")) {
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EAX;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, WORD_LEN - 1);
    dst->op[0].size = 2;
    num_implicit_ops += 1;
  } else if (   !strcmp(opc, "aaa")
             || !strcmp(opc, "aas")
             || !strcmp(opc, "das")
             || !strcmp(opc, "daa")) {
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EAX;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, BYTE_LEN - 1);
    dst->op[0].size = 1;
    num_implicit_ops += 1;
  } else if (   !strcmp(opc, "rdtsc")
             || !strcmp(opc, "rdpmc")) {
    for (i = I386_MAX_NUM_OPERANDS - 3; i >= 0; i--) {
      memcpy(&dst->op[i + 2], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EDX;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, DWORD_LEN - 1);
    dst->op[0].size = 4;
    dst->op[1].type = op_reg;
    dst->op[1].valtag.reg.tag = tag;
    dst->op[1].valtag.reg.val = R_EAX;
    dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, DWORD_LEN - 1);
    dst->op[1].size = 4;
    num_implicit_ops += 2;
  } else if (!strcmp(opc, "cpuid")) {
    for (i = I386_MAX_NUM_OPERANDS - 5; i >= 0; i--) {
      memcpy(&dst->op[i + 4], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EDX;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, DWORD_LEN - 1);
    dst->op[0].size = 4;
    dst->op[1].type = op_reg;
    dst->op[1].valtag.reg.tag = tag;
    dst->op[1].valtag.reg.val = R_ECX;
    dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, DWORD_LEN - 1);
    dst->op[1].size = 4;
    dst->op[2].type = op_reg;
    dst->op[2].valtag.reg.tag = tag;
    dst->op[2].valtag.reg.val = R_EBX;
    dst->op[2].valtag.reg.mod.reg.mask = MASKBITS(0, DWORD_LEN - 1);
    dst->op[2].size = 4;
    dst->op[3].type = op_reg;
    dst->op[3].valtag.reg.tag = tag;
    dst->op[3].valtag.reg.val = R_EAX;
    dst->op[3].valtag.reg.mod.reg.mask = MASKBITS(0, DWORD_LEN - 1);
    dst->op[3].size = 4;
    num_implicit_ops += 4;
  } else if (   !strcmp(opc, "cbtw")
             || !strcmp(opc, "cwtl")
             || !strcmp(opc, "cltq")) {
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EAX;
    dst->op[0].size = (!strcmp(opc, "cbtw"))?1:(!strcmp(opc, "cwtl")?2:4);
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, dst->op[0].size*BYTE_LEN - 1);
    num_implicit_ops += 1;
  } else if (   !strcmp(opc, "cwtd")
             || !strcmp(opc, "cltd")
             || !strcmp(opc, "cqto")) {
    for (i = I386_MAX_NUM_OPERANDS - 3; i >= 0; i--) {
      memcpy(&dst->op[i + 2], &dst->op[i], sizeof(operand_t));
    }
    int size = (!strcmp(opc, "cwtd"))?2:(!strcmp(opc, "cltd")?4:8);
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EDX;
    dst->op[0].size = size;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, size * BYTE_LEN - 1);
    dst->op[1].type = op_reg;
    dst->op[1].valtag.reg.tag = tag;
    dst->op[1].valtag.reg.val = R_EAX;
    dst->op[1].size = size;
    dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, size * BYTE_LEN - 1);
    num_implicit_ops += 2;
  } else if (strstart(opc, "int", NULL)) {
    ASSERT(dst->op[1].type == invalid);
    memcpy(&dst->op[I386_NUM_GPRS], &dst->op[0], sizeof(operand_t));
    for (i = 0; i < I386_NUM_GPRS; i++) {
      dst->op[i].type = op_reg;
      dst->op[i].valtag.reg.tag = tag;
      dst->op[i].valtag.reg.val = i;
      dst->op[i].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
      dst->op[i].size = DST_LONG_SIZE;
    }
    num_implicit_ops += I386_NUM_GPRS;
  } else if (strstart(opc, "xlat", NULL)) {
    ASSERT(dst->op[0].type == op_mem);
    ASSERT(dst->op[0].valtag.mem.base.tag == tag);
    ASSERT(dst->op[0].valtag.mem.base.val == R_EBX);
    ASSERT(dst->op[0].valtag.mem.index.val == -1);
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EAX;
    dst->op[0].size = 1;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, BYTE_LEN - 1);
    num_implicit_ops += 1;
  } else if (   strstart(opc, "xgetbv", NULL)
             || strstart(opc, "xsetbv", NULL)) {
    ASSERT(dst->op[0].type == invalid);
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_EAX;
    dst->op[0].size = DST_LONG_SIZE;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);

    dst->op[1].type = op_reg;
    dst->op[1].valtag.reg.tag = tag;
    dst->op[1].valtag.reg.val = R_EDX;
    dst->op[1].size = DST_LONG_SIZE;
    dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);

    dst->op[2].type = op_reg;
    dst->op[2].valtag.reg.tag = tag;
    dst->op[2].valtag.reg.val = R_ECX;
    dst->op[2].size = DST_LONG_SIZE;
    dst->op[2].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
    num_implicit_ops += 3;
  } else if (   i386_is_string_opcode(opc)
             && i386_insn_has_rep_prefix(dst)) {
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_reg;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = R_ECX;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, DST_LONG_BITS - 1);
    dst->op[0].size = DST_LONG_SIZE;
    num_implicit_ops += 1;
  } else if (!strcmp(opc, "vzeroupper") || !strcmp(opc, "vzeroall")) {
    for (i = 0; i < I386_NUM_EXREGS(I386_EXREG_GROUP_YMM); i++) {
      dst->op[i].type = op_ymm;
      dst->op[i].valtag.reg.tag = tag;
      dst->op[i].valtag.reg.val = i;
      dst->op[i].valtag.reg.mod.reg.mask = 0xffffffffffffffffUL; //XXX: need bigger mask
      dst->op[i].size = I386_EXREG_LEN(I386_EXREG_GROUP_YMM)/BYTE_LEN;
    }
    num_implicit_ops = I386_NUM_EXREGS(I386_EXREG_GROUP_YMM);
  } else if (i386_insn_accesses_mxcsr_rm(dst)) {
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_mxcsr_rm;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = 0;
    dst->op[0].size = 1;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, WORD2_LEN - 1);
    num_implicit_ops += 1;
  } else if (i386_insn_accesses_st_top(dst)) {
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_st_top;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = 0;
    dst->op[0].size = 1;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, WORD3_LEN - 1);
    num_implicit_ops += 1;
  }

  if (i386_insn_accesses_fp_control_reg_rm(dst)) {
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_fp_control_reg_rm;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = 0;
    dst->op[0].size = 1;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, WORD2_LEN - 1);
    num_implicit_ops += 1;
  }

  if (i386_insn_accesses_fp_status_reg_condition_codes(dst)) {
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 4].type == invalid);
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 3].type == invalid);
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 2].type == invalid);
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
    for (i = I386_MAX_NUM_OPERANDS - 5; i >= 0; i--) {
      memcpy(&dst->op[i + 4], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_fp_status_reg_c0;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = 0;
    dst->op[0].size = 1;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, 0);

    dst->op[1].type = op_fp_status_reg_c1;
    dst->op[1].valtag.reg.tag = tag;
    dst->op[1].valtag.reg.val = 0;
    dst->op[1].size = 1;
    dst->op[1].valtag.reg.mod.reg.mask = MASKBITS(0, 0);

    dst->op[2].type = op_fp_status_reg_c2;
    dst->op[2].valtag.reg.tag = tag;
    dst->op[2].valtag.reg.val = 0;
    dst->op[2].size = 1;
    dst->op[2].valtag.reg.mod.reg.mask = MASKBITS(0, 0);

    dst->op[3].type = op_fp_status_reg_c3;
    dst->op[3].valtag.reg.tag = tag;
    dst->op[3].valtag.reg.val = 0;
    dst->op[3].size = 1;
    dst->op[3].valtag.reg.mod.reg.mask = MASKBITS(0, 0);

    num_implicit_ops += 4;
  } else if (i386_insn_accesses_fp_status_reg_c0_only(dst)) {
    ASSERT(dst->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
    for (i = I386_MAX_NUM_OPERANDS - 2; i >= 0; i--) {
      memcpy(&dst->op[i + 1], &dst->op[i], sizeof(operand_t));
    }
    dst->op[0].type = op_fp_status_reg_c0;
    dst->op[0].valtag.reg.tag = tag;
    dst->op[0].valtag.reg.val = 0;
    dst->op[0].size = 1;
    dst->op[0].valtag.reg.mod.reg.mask = MASKBITS(0, 0);
    num_implicit_ops += 1;
  }

  //I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_signed, DWORD_LEN);
  //I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_unsigned, DWORD_LEN);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_other, I386_EXREG_GROUP_EFLAGS_OTHER, DST_EXREG_LEN(I386_EXREG_GROUP_EFLAGS_OTHER));
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_eq, I386_EXREG_GROUP_EFLAGS_EQ, 1);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_ne, I386_EXREG_GROUP_EFLAGS_NE, 1);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_ult, I386_EXREG_GROUP_EFLAGS_ULT, 1);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_slt, I386_EXREG_GROUP_EFLAGS_SLT, 1);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_ugt, I386_EXREG_GROUP_EFLAGS_UGT, 1);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_sgt, I386_EXREG_GROUP_EFLAGS_SGT, 1);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_ule, I386_EXREG_GROUP_EFLAGS_ULE, 1);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_sle, I386_EXREG_GROUP_EFLAGS_SLE, 1);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_uge, I386_EXREG_GROUP_EFLAGS_UGE, 1);
  I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, flags_sge, I386_EXREG_GROUP_EFLAGS_SGE, 1);
  //I386_INSN_ADD_IMPLICIT_OPERANDS_FOR_FIELD(dst, st_top, I386_EXREG_GROUP_ST_TOP, DWORD_LEN);

  if (i386_is_string_opcode(opc)) {
    //mark all implicit
    int i;
    for (i = 0; i < I386_MAX_NUM_OPERANDS - 1; i++) {
      if (dst->op[i].type == invalid) {
        break;
      }
    }
    num_implicit_ops = i;
  }

  dst->num_implicit_ops = num_implicit_ops;
  DBG(IMPLICIT_OPS, "returning %d for %s\n", num_implicit_ops,
      i386_insn_to_string(dst, as1, sizeof as1));
}

void
i386_insn_add_implicit_operands(i386_insn_t *dst, fixed_reg_mapping_t fixed_reg_mapping)
{
  return i386_insn_add_implicit_operands_helper(dst, fixed_reg_mapping, tag_const);
}

void
i386_insn_add_implicit_operands_var(i386_insn_t *dst, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  return i386_insn_add_implicit_operands_helper(dst, fixed_reg_mapping, tag_var);
}



/*static src_tag_t
operand_tag(operand_t const *op)
{
  switch (op->type) {
    case op_reg: return op->tag.reg;
    case op_mem: return op->tag.mem.all;
    case op_seg: return op->tag.seg;
    case op_imm: return op->tag.imm;
    case op_pcrel: return op->tag.cr;
    case op_db: return op->tag.db;
    case op_tr: return op->tag.tr;
    case op_mmx: return op->tag.mmx;
    case op_xmm: return op->tag.xmm;
    case op_d3now: return op->tag.d3now;
    case op_prefix: return op->tag.prefix;
    case op_st: return op->tag.st;
    case op_flags_unsigned: return op->tag.flags_unsigned;
    case op_flags_signed: return op->tag.flags_signed;
    case op_st_top: return op->tag.st_top;
    default: NOT_REACHED();
  }
}

static int
i386_insn_num_const_implicit_operands(i386_insn_t const *insn)
{
  int i;
  ASSERT(insn->num_implicit_ops != -1);
  for (i = 0; i < insn->num_implicit_ops; i++) {
    if (operand_tag(&insn->op[i]) == tag_var) {
      return 0;
    }
  }
  return insn->num_implicit_ops;
}*/

int
i386_insn_num_implicit_operands(i386_insn_t const *insn)
{
  ASSERT(insn->num_implicit_ops != -1);
  return insn->num_implicit_ops;
}

/*static size_t
i386_operand_mem_type_to_string(operand_mem_valtag_t const *memop, char *buf, size_t size)
{
  char base_reg[256], index_reg[256];
  char *ptr, *end;

  ptr = buf;
  end = buf + size;

  if (memop->segtype == segtype_sel) {
    ptr += vt_type_to_string(&memop->seg.sel.type, ptr, end - ptr);
  } else {
    ptr += vt_type_to_string(&memop->seg.desc.type, ptr, end - ptr);
  }
  ptr += snprintf(ptr, end - ptr, ": ");
  ptr += vt_type_to_string(&memop->disp.type, ptr, end - ptr);

  if (memop->base.val != -1) {
    vt_type_to_string(&memop->base.type, base_reg, sizeof base_reg);
  } else {
    snprintf(base_reg, sizeof base_reg, "%s", "");
  }

  if (memop->index.val != -1) {
    vt_type_to_string(&memop->index.type, index_reg, sizeof index_reg);
  }

  if (memop->base.val != -1 || memop->index.val != -1) {
    ptr += snprintf(ptr, end - ptr, "(");
  }
  if (memop->index.val != -1) {
    ptr += snprintf(ptr, end - ptr, "%s,%s", base_reg, index_reg);
  } else if (memop->base.val != -1) {
    ptr += snprintf(ptr, end - ptr, "%s", base_reg);
  }
  if (memop->base.val != -1 || memop->index.val != -1) {
    ptr += snprintf(ptr, end - ptr, ")");
  }
  return ptr - buf;
}

static size_t
i386_operand_type_to_string(operand_t const *op, char *buf, size_t size)
{
  char *ptr, *end;

  ptr = buf;
  end = buf + size;

  switch (op->type) {
    case invalid:
      break;
    case op_reg:
      ptr += vt_type_to_string(&op->valtag.reg.type, ptr, end - ptr);
      break;
    case op_seg:
      ptr += vt_type_to_string(&op->valtag.seg.type, ptr, end - ptr);
      break;
    case op_mem:
      ptr += i386_operand_mem_type_to_string(&op->valtag.mem, ptr, end - ptr);
      break;
    case op_imm:
      ptr += vt_type_to_string(&op->valtag.imm.type, ptr, end - ptr);
      break;
    case op_pcrel:
      ptr += vt_type_to_string(&op->valtag.pcrel.type, ptr, end - ptr);
      break;
    case op_cr:
      ptr += vt_type_to_string(&op->valtag.cr.type, ptr, end - ptr);
      break;
    case op_db:
      ptr += vt_type_to_string(&op->valtag.db.type, ptr, end - ptr);
      break;
    case op_tr:
      ptr += vt_type_to_string(&op->valtag.tr.type, ptr, end - ptr);
      break;
    case op_xmm:
      ptr += vt_type_to_string(&op->valtag.xmm.type, ptr, end - ptr);
      break;
    case op_d3now:
      ptr += vt_type_to_string(&op->valtag.d3now.type, ptr, end - ptr);
      break;
    case op_prefix:
      ptr += vt_type_to_string(&op->valtag.prefix.type, ptr, end - ptr);
      break;
    case op_st:
      ptr += vt_type_to_string(&op->valtag.st.type, ptr, end - ptr);
      break;
    case op_flags_unsigned:
      ptr += vt_type_to_string(&op->valtag.flags_unsigned.type, ptr, end - ptr);
      break;
    case op_flags_signed:
      ptr += vt_type_to_string(&op->valtag.flags_signed.type, ptr, end - ptr);
      break;
    case op_st_top:
      ptr += vt_type_to_string(&op->valtag.st_top.type, ptr, end - ptr);
      break;
    default: NOT_REACHED();
  }
  return ptr - buf;
}

static char *
i386_insn_types_to_string(i386_insn_t const *insn, char *buf, size_t size)
{
  char *ptr, *end;
  int i;

  ptr = buf;
  end = buf + size;

  for (i = I386_MAX_NUM_OPERANDS - 1; i >= 0; i--) {
    ptr += i386_operand_type_to_string(&insn->op[i], ptr, end - ptr);
  }
  return buf;
}

static void
i386_insn_types_from_string(i386_insn_t *insn, char const *buf, size_t size)
{
  NOT_IMPLEMENTED();
}
*/

static int
insn2str(i386_insn_t const *insn, char const *reloc_strings, size_t reloc_strings_size,
    i386_as_mode_t mode, char *buf, size_t size)
{
  char *ptr = buf, *end = buf + size;
  int startop, endop, update;
  int i, num_implicit_ops;

  if (mode != I386_AS_MODE_64_REGULAR && i386_insn_is_nop(insn)) {
    ptr += snprintf(ptr, end - ptr, "nop  #");
    return ptr - buf;
  }

  char const *opc = opctable_name(insn->opc);

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_prefix) {
      ptr += prefix2str(insn->op[i].valtag.prefix.val, insn->op[i].valtag.prefix.tag,
          ptr, end - ptr);
      *ptr++ = ' '; *ptr = '\0';
    }
  }

  ptr += snprintf(ptr, end - ptr, "%s", opc);
  *ptr++ = ' '; *ptr = '\0';
  if (   (strstart(opc, "jmp", NULL) && insn->op[0].type == op_mem)
      || (strstart(opc, "calll", NULL) && insn->op[insn->num_implicit_ops].type
              == op_mem)) {
    /* Special case for OP_indirE */
    *ptr++ = '*'; *ptr = '\0';
  }

  num_implicit_ops = i386_insn_num_implicit_operands(insn);
  if (   strstart(opc, "bound", NULL)
      || strstart(opc, "enter", NULL)) {
    startop = insn->num_implicit_ops; endop = I386_MAX_NUM_OPERANDS; update = 1;
  } else {
    startop = I386_MAX_NUM_OPERANDS - 1; update = -1;
    endop = insn->num_implicit_ops -1;
  }
  bool explicit_operands_exist = false;
  for (i = startop; i != endop; i += update) {
    if (insn->op[i].type == op_prefix) {
      continue;
    }
    DBE(MD_ASSEMBLE, printf(" %d:", i));
    ptr += operand2str(&insn->op[i], reloc_strings, reloc_strings_size, mode,
        ptr, end - ptr);
    if (insn->op[i].type != invalid) {
      explicit_operands_exist = true;
    }
    /*if (i == num_implicit_ops && update == -1) {
      ptr += snprintf(ptr, end - ptr, " #");
    }*/
    if (   insn->op[i].type != invalid
        && i + update != endop
        && insn->op[i + update].type != invalid) {
      ptr += snprintf(ptr, end - ptr, ",");
    }
  }
  ptr += snprintf(ptr, end - ptr, " #");
  bool implicit_operands_started = false;
  for (i = insn->num_implicit_ops - 1; i >= 0; i--) {
    if (insn->op[i].type == op_prefix) {
      continue;
    }
    ASSERT(insn->op[i].type != invalid);
    if (explicit_operands_exist || implicit_operands_started) {
      ptr += snprintf(ptr, end - ptr, ",");
    }
    implicit_operands_started = true;
    ptr += operand2str(&insn->op[i], reloc_strings, reloc_strings_size, mode,
        ptr, end - ptr);
  }


  //ptr += snprintf(ptr, end - ptr, "|");
  //memvt_list_to_string(&insn->memtouch, &i386_excreg_to_string, ptr, end - ptr);
  //ptr += strlen(ptr);
  //ptr += snprintf(ptr, end - ptr, "|");

  ASSERT(ptr < end);
  return ptr - buf;
}

bool
i386_insn_terminates_bbl(i386_insn_t const *insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);
  if (opc[0] == 'j') {
    return true;
  }
  if (opc[0] == 'l' && opc[1] == 'j') {
    return true;
  }
  /*if (i386_insn_is_function_call(insn)) {
    return true;
  }*/
  if (strstart(opc, "loop", NULL)) {
    return true;
  }
  /*
  if (!strcmp(opc, "int")) {
    return true;
  }
  */
  if (i386_insn_is_function_return(insn)) {
    return true;
  }
  if (!strcmp(opc, "lretw") || !strcmp(opc, "lretl")) {
    return true;
  }
  if (!strcmp(opc, "iret")) {
    return true;
  }
  return false;
}


bool
i386_insn_is_branch(i386_insn_t const *insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);
  if (opc[0] == 'j') {
    return true;
  }
  if (opc[0] == 'l' && opc[1] == 'j') {
    return true;
  }
  if (i386_insn_is_function_call(insn)) {
    return true;
  }
  if (strstart(opc, "loop", NULL)) {
    return true;
  }
  /*
  if (!strcmp(opc, "int")) {
    return true;
  }
  */
  if (i386_insn_is_function_return(insn)) {
    return true;
  }
  if (!strcmp(opc, "lretw") || !strcmp(opc, "lretl")) {
    return true;
  }
  if (!strcmp(opc, "iret")) {
    return true;
  }
  return false;
}

bool
i386_insn_is_branch_not_fcall(i386_insn_t const *insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);
  if (opc[0] == 'j') {
    return true;
  }
  if (opc[0] == 'l' && opc[1] == 'j') {
    return true;
  }
  if (strstart(opc, "loop", NULL)) {
    return true;
  }
  if (i386_insn_is_function_return(insn)) {
    return true;
  }
  if (!strcmp(opc, "lretw") || !strcmp(opc, "lretl")) {
    return true;
  }
  if (!strcmp(opc, "iret")) {
    return true;
  }
  return false;
}

static void
operand_init(operand_t *op, size_t long_len)
{
  int i;

  memset(op, 0, sizeof *op);  /* set all unused bits to 0. */

  /* Load the operand with the default values for memory operands. */
  op->type = invalid;
  op->size = 0;
  op->valtag.reg.mod.reg.mask = 0;
  memvt_init(&op->valtag.mem, long_len);
}

extern "C"
void
i386_insn_init(i386_insn_t *insn, size_t long_len)
{
  int i;
  memset(insn, 0, sizeof *insn);
  insn->opc = opc_inval;
  //strlcpy(insn->opc, "inval", sizeof insn->opc);
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    operand_init(&insn->op[i], long_len);
  }
  insn->num_implicit_ops = -1;
  //memvt_list_init(&insn->memtouch);
}

static bool
is_pcrel_operand(i386_insn_t const *insn, int j)
{
  char const *opc = opctable_name(insn->opc);
  if (   opc[0] == 'j'
      || !strcmp(opc, "calll")
      || !strcmp(opc, "loop")) {
    if (j == 0) {
      return true;
    }
  }
  return false;
}

static bool
imms_equal(uint64_t val1, src_tag_t tag1, imm_modifier_t imm_modifier1, uint64_t const *sconstants1,
    int constant_val1, src_tag_t constant_tag1,
    int exvreg_group1, uint64_t val2, src_tag_t tag2,
    imm_modifier_t imm_modifier2, uint64_t const *sconstants2, int constant_val2,
    src_tag_t constant_tag2, int exvreg_group2)
{
  if (tag1 != tag2) {
    return false;
  }
  if (tag1 == tag_const) {
    return val1 == val2;
  }
  if (imm_modifier1 != imm_modifier2) {
    return false;
  }
  if (tag1 == tag_imm_exvregnum && exvreg_group1 != exvreg_group2) {
    return false;
  }
  switch (imm_modifier1) {
    /*case IMM_NEXTPC_TIMES_SC2_UMASK_SPECIAL_CONSTANT:
      return val1 == val2;*/
    case IMM_UNMODIFIED:
      return val1 == val2;
    case IMM_TIMES_SC2_PLUS_SC1_MASK_SC0:
    case IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0:
      return    val1 == val2 && sconstants1[0] == sconstants2[0]
             && sconstants1[1] == sconstants2[1]
             && sconstants1[2] == sconstants2[2];
    case IMM_SIGN_EXT_SC0:
    case IMM_LSHIFT_SC0:
    case IMM_ASHIFT_SC0:
      return val1 == val2 && sconstants1[0] == sconstants2[0];
    case IMM_PLUS_SC0_TIMES_C0:
    case IMM_LSHIFT_SC0_PLUS_MASKED_C0:
    case IMM_LSHIFT_SC0_PLUS_UMASKED_C0:
      return    val1 == val2
             && sconstants1[0] == sconstants2[0]
             && constant_val1 == constant_val2
             && constant_tag1 == constant_tag2;
    case IMM_TIMES_SC2_PLUS_SC1_MASKTILL_SC0:
    case IMM_TIMES_SC2_PLUS_SC1_MASKFROM_SC0:
      return    val1 == val2
             && sconstants1[0] == sconstants2[0]
             && sconstants1[1] == sconstants2[1]
             && sconstants1[2] == sconstants2[2];
    case IMM_TIMES_SC3_PLUS_SC2_MASKTILL_C0_TIMES_SC1_PLUS_SC0:
      return    val1 == val2
             && constant_val1 == constant_val2
             && constant_tag1 == constant_tag2
             && sconstants1[0] == sconstants2[0]
             && sconstants1[1] == sconstants2[1]
             && sconstants1[2] == sconstants2[2]
             && sconstants1[3] == sconstants2[3];
    default:
      printf("modifier = %d\n", imm_modifier1);
  }
  NOT_REACHED();
}

static bool
base_index_scale_equal(valtag_t const *base1, valtag_t const *index1, int scale1,
    valtag_t const *base2, valtag_t const *index2, int scale2)
{
  if (   reg_valtags_equal(base1, base2)
      && reg_valtags_equal(index1, index2)
      && (index1->val == -1 || scale1 == scale2)) {
    return true;
  }
  if (   base1->val == -1
      && index2->val == -1
      && scale1 == 1
      && reg_valtags_equal(index1, base2)) {
    return true;
  }
  if (   index1->val == -1
      && base2->val == -1
      && scale2 == 1
      && reg_valtags_equal(base1, index2)) {
    return true;
  }

  return false;
}

static bool
mem_operands_equal(operand_t const *op1, operand_t const *op2)
{
  ASSERT(op1->type == op_mem);
  ASSERT(op2->type == op_mem);

  return (   op1->valtag.mem.segtype == op2->valtag.mem.segtype
          && op1->valtag.mem.seg.sel.tag == op2->valtag.mem.seg.sel.tag
          && op1->valtag.mem.seg.desc.tag == op2->valtag.mem.seg.desc.tag
          && base_index_scale_equal(&op1->valtag.mem.base, &op1->valtag.mem.index,
                 op1->valtag.mem.scale, &op2->valtag.mem.base, &op2->valtag.mem.index,
                 op2->valtag.mem.scale)
          && imms_equal(op1->valtag.mem.disp.val,
                        op1->valtag.mem.disp.tag,
                        op1->valtag.mem.disp.mod.imm.modifier,
                        op1->valtag.mem.disp.mod.imm.sconstants,
                        op1->valtag.mem.disp.mod.imm.constant_val,
                        op1->valtag.mem.disp.mod.imm.constant_tag,
                        op1->valtag.mem.disp.mod.imm.exvreg_group,
                        op2->valtag.mem.disp.val,
                        op2->valtag.mem.disp.tag,
                        op2->valtag.mem.disp.mod.imm.modifier,
                        op2->valtag.mem.disp.mod.imm.sconstants,
                        op2->valtag.mem.disp.mod.imm.constant_val,
                        op2->valtag.mem.disp.mod.imm.constant_tag,
                        op2->valtag.mem.disp.mod.imm.exvreg_group)
          && reg_valtags_equal(&op1->valtag.mem.riprel, &op2->valtag.mem.riprel)
         );

}

bool
operands_equal(operand_t const *op1, operand_t const *op2)
{
  if (op1->type != op2->type) {
    return false;
  }
  if (op1->size != op2->size) {
    return false;
  }
  if (op1->type == invalid) {
    return true;
  }

  switch (op1->type) {
    case op_reg:   return (   op1->valtag.reg.val == op2->valtag.reg.val
                           && op1->valtag.reg.tag == op2->valtag.reg.tag
                           && op1->valtag.reg.mod.reg.mask == op2->valtag.reg.mod.reg.mask);
    case op_seg:   return (   op1->valtag.seg.val == op2->valtag.seg.val
                           && op1->valtag.seg.tag == op2->valtag.seg.tag);
    case op_mem:   return mem_operands_equal(op1, op2);
    case op_imm:   /* All tag_const imm operands should have 0 size. */
                   if (op1->size != op2->size) {
                     return false;
                   }
                   return imms_equal(op1->valtag.imm.val, op1->valtag.imm.tag,
                       op1->valtag.imm.mod.imm.modifier,
                       op1->valtag.imm.mod.imm.sconstants,
                       op1->valtag.imm.mod.imm.constant_val,
                       op1->valtag.imm.mod.imm.constant_tag,
                       op1->valtag.imm.mod.imm.exvreg_group,
                       op2->valtag.imm.val,
                       op2->valtag.imm.tag,
                       op2->valtag.imm.mod.imm.modifier,
                       op2->valtag.imm.mod.imm.sconstants,
                       op2->valtag.imm.mod.imm.constant_val,
                       op2->valtag.imm.mod.imm.constant_tag,
                       op2->valtag.imm.mod.imm.exvreg_group);
                   /* return (   op1->val.imm == op2->val.imm
                           && op1->tag.imm == op2->tag.imm); */
    case op_pcrel: /* All tag_const pcrel operands should have 0 size. */
                   ASSERT(op1->valtag.pcrel.tag != tag_const || op1->size == 0);
                   ASSERT(op2->valtag.pcrel.tag != tag_const || op2->size == 0);
                   return (   op1->valtag.pcrel.val == op2->valtag.pcrel.val
                           && op1->valtag.pcrel.tag == op2->valtag.pcrel.tag);
    case op_cr:    return (   op1->valtag.cr.val == op2->valtag.cr.val
                           && op1->valtag.cr.tag == op2->valtag.cr.tag);
    case op_db:    return (   op1->valtag.db.val == op2->valtag.db.val
                           && op1->valtag.db.tag == op2->valtag.db.tag);
    case op_tr:    return (   op1->valtag.tr.val == op2->valtag.tr.val
                           && op1->valtag.tr.tag == op2->valtag.tr.tag);
    case op_mmx:   return (   op1->valtag.mmx.val == op2->valtag.mmx.val
                           && op1->valtag.mmx.tag == op2->valtag.mmx.tag);
    case op_xmm:   return (   op1->valtag.xmm.val == op2->valtag.xmm.val
                           && op1->valtag.xmm.tag == op2->valtag.xmm.tag);
    case op_ymm:   return (   op1->valtag.ymm.val == op2->valtag.ymm.val
                           && op1->valtag.ymm.tag == op2->valtag.ymm.tag);
    case op_d3now: return (   op1->valtag.d3now.val == op2->valtag.d3now.val
                           && op1->valtag.d3now.tag == op2->valtag.d3now.tag);
    case op_prefix: return (   op1->valtag.prefix.val == op2->valtag.prefix.val
                            && op1->valtag.prefix.tag == op2->valtag.prefix.tag);
    //case op_flags_unsigned:
    //    return (   op1->valtag.flags_unsigned.val == op2->valtag.flags_unsigned.val
    //            && op1->valtag.flags_unsigned.tag == op2->valtag.flags_unsigned.tag);
    //case op_flags_signed:
    //    return (   op1->valtag.flags_signed.val == op2->valtag.flags_signed.val
    //            && op1->valtag.flags_signed.tag == op2->valtag.flags_signed.tag);
#define case_cmp_flag(flag_name) \
    case op_flags_##flag_name: \
        return (   op1->valtag.flags_##flag_name.val == op2->valtag.flags_##flag_name.val \
                && op1->valtag.flags_##flag_name.tag == op2->valtag.flags_##flag_name.tag);
    case_cmp_flag(other);
    case_cmp_flag(eq);
    case_cmp_flag(ne);
    case_cmp_flag(ult);
    case_cmp_flag(slt);
    case_cmp_flag(ugt);
    case_cmp_flag(sgt);
    case_cmp_flag(ule);
    case_cmp_flag(sle);
    case_cmp_flag(uge);
    case_cmp_flag(sge);
    case op_st_top: return (   op1->valtag.st_top.val == op2->valtag.st_top.val
                            && op1->valtag.st_top.tag == op2->valtag.st_top.tag);
    case op_fp_data_reg: return (   op1->valtag.fp_data_reg.val == op2->valtag.fp_data_reg.val
                                 && op1->valtag.fp_data_reg.tag == op2->valtag.fp_data_reg.tag);
    case op_fp_control_reg_rm: return (   op1->valtag.fp_control_reg_rm.val == op2->valtag.fp_control_reg_rm.val
                                       && op1->valtag.fp_control_reg_rm.tag == op2->valtag.fp_control_reg_rm.tag);
    case op_fp_control_reg_other: return (   op1->valtag.fp_control_reg_other.val == op2->valtag.fp_control_reg_other.val
                                          && op1->valtag.fp_control_reg_other.tag == op2->valtag.fp_control_reg_other.tag);
    //case op_fp_status_reg: return (   op1->valtag.fp_status_reg.val == op2->valtag.fp_status_reg.val
    //                                && op1->valtag.fp_status_reg.tag == op2->valtag.fp_status_reg.tag);
    case op_fp_tag_reg: return (   op1->valtag.fp_tag_reg.val == op2->valtag.fp_tag_reg.val
                                && op1->valtag.fp_tag_reg.tag == op2->valtag.fp_tag_reg.tag);
    case op_fp_last_instr_pointer: return (   op1->valtag.fp_last_instr_pointer.val == op2->valtag.fp_last_instr_pointer.val
                                           && op1->valtag.fp_last_instr_pointer.tag == op2->valtag.fp_last_instr_pointer.tag);
    case op_fp_last_operand_pointer: return (   op1->valtag.fp_last_operand_pointer.val == op2->valtag.fp_last_operand_pointer.val
                                             && op1->valtag.fp_last_operand_pointer.tag == op2->valtag.fp_last_operand_pointer.tag);
    case op_fp_opcode: return (   op1->valtag.fp_opcode.val == op2->valtag.fp_opcode.val
                               && op1->valtag.fp_opcode.tag == op2->valtag.fp_opcode.tag);
    case op_mxcsr_rm: return (   op1->valtag.mxcsr_rm.val == op2->valtag.mxcsr_rm.val
                              && op1->valtag.mxcsr_rm.tag == op2->valtag.mxcsr_rm.tag);
    case op_fp_status_reg_c0: return (   op1->valtag.fp_status_reg_c0.val == op2->valtag.fp_status_reg_c0.val
                                      && op1->valtag.fp_status_reg_c0.tag == op2->valtag.fp_status_reg_c0.tag);
    case op_fp_status_reg_c1: return (   op1->valtag.fp_status_reg_c1.val == op2->valtag.fp_status_reg_c1.val
                                      && op1->valtag.fp_status_reg_c1.tag == op2->valtag.fp_status_reg_c1.tag);
    case op_fp_status_reg_c2: return (   op1->valtag.fp_status_reg_c2.val == op2->valtag.fp_status_reg_c2.val
                                      && op1->valtag.fp_status_reg_c2.tag == op2->valtag.fp_status_reg_c2.tag);
    case op_fp_status_reg_c3: return (   op1->valtag.fp_status_reg_c3.val == op2->valtag.fp_status_reg_c3.val
                                      && op1->valtag.fp_status_reg_c3.tag == op2->valtag.fp_status_reg_c3.tag);
    case op_fp_status_reg_other: return (   op1->valtag.fp_status_reg_other.val == op2->valtag.fp_status_reg_other.val
                                      && op1->valtag.fp_status_reg_other.tag == op2->valtag.fp_status_reg_other.tag);
    case op_st: return (   op1->valtag.st.val == op2->valtag.st.val
                        && op1->valtag.st.tag == op2->valtag.st.tag);
    case invalid: NOT_REACHED();
  }
  ASSERT(0);
  return false;
}

bool
i386_insn_is_indirect_branch(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  if (   strstart(opc, "jmp", NULL)
      && (insn->op[0].type == op_reg || insn->op[0].type == op_mem)) {
    return true;
  }
  if (i386_insn_is_function_return(insn)) {
    return true;
  }
  if (   i386_insn_is_function_call(insn)
      && (   insn->op[insn->num_implicit_ops].type == op_reg
          || insn->op[insn->num_implicit_ops].type == op_mem)) {
    return true;
  }
  return false;
}

static bool
i386_insn_is_unconditional_indirect_branch(i386_insn_t const *insn)
{
  return i386_insn_is_indirect_branch(insn);
}

bool
i386_insn_is_indirect_branch_not_fcall(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  if (   strstart(opc, "jmp", NULL)
      && (insn->op[0].type == op_reg || insn->op[0].type == op_mem)) {
    return true;
  }
  if (i386_insn_is_function_return(insn)) {
    return true;
  }
  /* if (   i386_insn_is_function_call(insn)
      && (insn->op[1].type == op_reg || insn->op[1].type == op_mem)) {
    return true;
  } */
  return false;
}

bool
i386_insn_is_unconditional_indirect_branch_not_fcall(i386_insn_t const *insn)
{
  return i386_insn_is_indirect_branch_not_fcall(insn);
}

bool
i386_insn_is_lea(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  if (   !strcmp(opc, "lea")
      || !strcmp(opc, "leal")) {
    return true;
  }
  return false;
}

bool
i386_insn_is_direct_branch(i386_insn_t const *insn)
{
  /*char const *opc = opctable_name(insn->opc); */
  valtag_t *pcrel;
  opc_t *jmp_opc;
  if (pcrel = i386_insn_get_pcrel_operand(insn)) {
    for (jmp_opc = jmp_opcodes;
        jmp_opc < jmp_opcodes + num_jmp_opcodes;
        jmp_opc++) {
      if (insn->opc == *jmp_opc) {
        return true;
      }
    }
  }
  if (i386_insn_is_jecxz(insn)) {
    ASSERT(insn->num_implicit_ops == 1);
    ASSERT(insn->op[insn->num_implicit_ops].type == op_pcrel);
    return true;
  }
  if (   i386_insn_is_function_call(insn)
      && insn->op[insn->num_implicit_ops].type == op_pcrel) {
    return true;
  }
  return false;
}

bool
i386_insn_is_conditional_direct_branch(i386_insn_t const *insn)
{
  if (   insn->opc != opc_jmp
      && insn->opc != opc_calll
      && i386_insn_is_direct_branch(insn)) {
    return true;
  }
  return false;
}

bool
i386_insn_is_unconditional_direct_branch_not_fcall(i386_insn_t const *insn)
{
  if (   insn->opc == opc_jmp
      && i386_insn_is_direct_branch(insn)) {
    return true;
  }
  return false;
}

bool
i386_insn_is_unconditional_direct_branch(i386_insn_t const *insn)
{
  if (   insn->opc == opc_jmp
      && i386_insn_is_direct_branch(insn)) {
    return true;
  }
  if (   i386_insn_is_function_call(insn)
      && i386_insn_is_direct_branch(insn)) {
    return true;
  }
  return false;
}

bool
i386_insn_is_unconditional_branch_not_fcall(i386_insn_t const *insn)
{
  if (   i386_insn_is_unconditional_direct_branch_not_fcall(insn)
      || i386_insn_is_indirect_branch_not_fcall(insn)) {
    return true;
  }
  return false;
}

bool
i386_insn_is_unconditional_branch(i386_insn_t const *insn)
{
  if (   i386_insn_is_unconditional_direct_branch(insn)
      || i386_insn_is_indirect_branch(insn)) {
    return true;
  }
  return false;
}

static inline bool
operand_is_mem16(operand_t const *op)
{
  if (op->type == op_mem && op->valtag.mem.addrsize == 2) {
    return true;
  }
  return false;
}

static inline bool
operand_is_mem(operand_t const *op)
{
  return (op->type == op_mem);
}

static inline bool
operand_is_prefix(operand_t const *op)
{
  return (op->type == op_prefix);
}

/*static bool
i386_insn_accesses_op_type(i386_insn_t const *insn, struct operand_t const **op1,
    struct operand_t const **op2, bool (*type_fn)(operand_t const *op))
{
  struct operand_t const *retop1 = NULL, *retop2 = NULL;
  bool ret = false;
  if (!strcmp(opctable_name(insn->opc), "lea")) {
    return false;
  }
  if ((*type_fn)(&insn->op[0])) {
    retop1 = &insn->op[0];
    ret = true;
  }
  if ((*type_fn)(&insn->op[1])) {
    if (!retop1) {
      retop1 = &insn->op[1];
    } else {
      retop2 = &insn->op[1];
    }
    ret = true;
  }
  if ((*type_fn)(&insn->op[2])) {
    if (!retop1) {
      retop1 = &insn->op[2];
    } else {
      retop2 = &insn->op[2];
    }
    ret = true;
  }
  if (op1) {
    *op1 = retop1;
  }
  if (op2) {
    *op2 = retop2;
  }
  return ret;
}

bool
i386_insn_accesses_mem16(i386_insn_t const *insn, struct operand_t const **op1,
    struct operand_t const **op2)
{
  return i386_insn_accesses_op_type(insn, op1, op2, operand_is_mem16);
}

bool
i386_insn_accesses_mem(i386_insn_t const *insn, struct operand_t const **op1,
    struct operand_t const **op2)
{
  return i386_insn_accesses_op_type(insn, op1, op2, operand_is_mem);
}*/

static char const *stack_opcodes[] = { "push", "pop", "call", " ret", "enter", "leave" };
static int num_stack_opcodes = sizeof stack_opcodes/sizeof stack_opcodes[0];

static char const *jecxz_opcodes[] = { "jecxz", "jcxz" };
static int num_jecxz_opcodes = sizeof jecxz_opcodes/sizeof jecxz_opcodes[0];

bool
i386_insn_accesses_stack(i386_insn_t const *insn)
{
  char const *opc_orig = opctable_name(insn->opc);
  char opc[strlen(opc_orig) + 16];
  if (strstart(opc_orig, "ret", NULL)) {
    opc[0] = ' ';
    strncpy(&opc[1], opc_orig, sizeof opc - 1);
  } else {
    strncpy(opc, opc_orig, sizeof opc);
  }

  char const *found;
  if (   (found = strstr_arr(opc, stack_opcodes, num_stack_opcodes))
      && !strstart(found, "popcnt", NULL)) {
    return true;
  }
  return false;
}

/*struct operand_t const *
i386_insn_has_prefix(i386_insn_t const *insn)
{
  struct operand_t const *retop = NULL;
  if (!i386_insn_accesses_op_type(insn, &retop, NULL, operand_is_prefix)) {
    return NULL;
  }
  ASSERT(retop);
  return retop;
}*/

/*static unsigned
operand_get_size(operand_t const *op)
{
  switch (op->type) {
    case op_reg: return op->size;
    case op_mem: return op->size;
    default: return (unsigned)-1;
  }
}

unsigned
i386_insn_get_operand_size(i386_insn_t const *insn)
{
  unsigned size = 8;
  size = min(size, operand_get_size(&insn->op[0]));
  size = min(size, operand_get_size(&insn->op[1]));
  size = min(size, operand_get_size(&insn->op[2]));
  return size;
}

unsigned
i386_insn_get_addr_size(i386_insn_t const *insn)
{
  operand_t const *op;
  bool mem;
  mem = i386_insn_accesses_mem(insn, &op, NULL);
  ASSERT(mem);
  ASSERT(op->tag.all == tag_const);
  ASSERT(op->tag.mem.all == tag_const);
  ASSERT(op->val.mem.addrsize == 2 || op->val.mem.addrsize == 4);
  return op->val.mem.addrsize;
}*/

/*void
i386_iseq_get_usedef_regs(i386_insn_t const *insns, int n_insns,
    struct regset_t *use, struct regset_t *def)
{
  memset_t memuse, memdef;
  memset_init(&memuse);
  memset_init(&memdef);

  i386_iseq_get_usedef(insns, n_insns, use, def, &memuse, &memdef);

  memset_free(&memuse);
  memset_free(&memdef);
}*/

/*static void
i386_regset_mark_vreg(regset_t *regset, int regno)
{
  if (regno < VREG0_HIGH_8BIT) {
    regset_mark(regset, regno);
  } else if (regno < TEMP_REG0) {
    regset_mark(regset, regno - VREG0_HIGH_8BIT);
  }
}*/

static bool
regmap_maps_reg_to_constant(regmap_t const *regmap, exreg_group_id_t group, exreg_id_t reg_id)
{
  //cout << __func__ << " " << __LINE__ << ": regmap =\n" << regmap_to_string(regmap, as1, sizeof as1) << endl;
  if (regmap->regmap_extable.count(group) == 0) {
    return false;
  }
  if (regmap->regmap_extable.at(group).count(reg_id) == 0) {
    return false;
  }
  reg_identifier_t r = regmap->regmap_extable.at(group).at(reg_id);
  return r.reg_identifier_denotes_constant();
}

#define I386_OPERAND_RENAME_EXREG(regmap, field, exreg_group, tag) do { \
  if (op->valtag.field.tag == tag_var) { \
    int creg; \
    DBG3(I386_INSN, "before rename, op->valtag.field.val = %u\n", (unsigned)op->valtag.field.val); \
    op->valtag.field.tag = tag; \
    creg = regmap_rename_exreg(regmap, exreg_group, \
        reg_identifier_t(op->valtag.field.val).reg_identifier_get_id()).reg_identifier_get_id(); \
    if (creg != TRANSMAP_REG_ID_UNASSIGNED) { /* creg == TRANSMAP_REG_ID_UNASSIGNED can happen for reserved regs; let them be identity mapped. */\
      op->valtag.field.val = creg; \
    } \
    ASSERT(   op->valtag.field.val >= 0 \
           && op->valtag.field.val < MAX_NUM_EXREGS(exreg_group)); \
  } else NOT_REACHED();\
} while(0)

static void
i386_operand_rename_memory_operands(operand_t *op, regmap_t const *dst_regmap,
    //imm_map_t const *imm_map,
    imm_vt_map_t const *imm_vt_map, memslot_map_t const *memslot_map,
    regmap_t const *src_regmap, nextpc_t const *nextpcs,
    long num_nextpcs, src_tag_t tag, dst_ulong base_gpr_offset)
{
  ASSERT(op->type == op_mem);

  if (mem_valtag_is_src_env_const_access(&op->valtag.mem)) {
    return;
  }

  ASSERT(op->valtag.mem.segtype == segtype_sel);
  if (op->valtag.mem.segtype == segtype_sel) {
    I386_OPERAND_RENAME_EXREG(dst_regmap, mem.seg.sel, I386_EXREG_GROUP_SEGS, tag);
  }

  if (memslot_map) {
    mem_valtag_rename_using_memslot_map(&op->type, &op->valtag.mem, &op->valtag.imm, memslot_map, base_gpr_offset);
  }

  if (op->valtag.mem.disp.tag != tag_const) {
    /*op->valtag.mem.disp.val = */imm_vt_map_rename_vconstant(/*op->valtag.mem.disp.val,
        op->valtag.mem.disp.tag,
        op->valtag.mem.disp.mod.imm.modifier,
        op->valtag.mem.disp.mod.imm.constant, op->valtag.mem.disp.mod.imm.sconstants,
        op->valtag.mem.disp.mod.imm.exvreg_group, */
        &op->valtag.mem.disp, imm_vt_map, nextpcs,
        num_nextpcs, src_regmap/*, tag*/);
    //op->valtag.mem.disp.tag = tag;
  }
  if (op->valtag.mem.base.val != -1 && op->valtag.mem.base.tag != tag_const) {
    ASSERT(op->valtag.mem.base.tag == tag_var);
    ASSERT(op->valtag.mem.base.val >= 0);
    ASSERT(op->valtag.mem.base.val < I386_NUM_GPRS);
    if (regmap_maps_reg_to_constant(dst_regmap, I386_EXREG_GROUP_GPRS, op->valtag.mem.base.val)) {
      long orig_val = op->valtag.mem.base.val;
      op->valtag.mem.base.val = -1;
      op->valtag.mem.base.tag = tag_const;
      imm_valtag_add_constant_using_reg_id(&op->valtag.mem.disp, dst_regmap->regmap_extable.at(I386_EXREG_GROUP_GPRS).at(orig_val));
    } else {
      I386_OPERAND_RENAME_EXREG(dst_regmap, mem.base, I386_EXREG_GROUP_GPRS, tag);
    }
  }
  if (op->valtag.mem.index.val != -1) {
    ASSERT(op->valtag.mem.index.tag == tag_var);
    ASSERT(op->valtag.mem.index.val >= 0);
    ASSERT(op->valtag.mem.index.val < I386_NUM_GPRS);
    if (regmap_maps_reg_to_constant(dst_regmap, I386_EXREG_GROUP_GPRS, op->valtag.mem.index.val)) {
      long orig_val = op->valtag.mem.index.val;
      op->valtag.mem.index.val = -1;
      op->valtag.mem.index.tag = tag_const;
      imm_valtag_add_constant_using_reg_id(&op->valtag.mem.disp, dst_regmap->regmap_extable.at(I386_EXREG_GROUP_GPRS).at(orig_val));
    } else {
      I386_OPERAND_RENAME_EXREG(dst_regmap, mem.index, I386_EXREG_GROUP_GPRS, tag);
    }
  }
  if (tag == tag_const && op->valtag.mem.index.val == R_ESP) {
    if (op->valtag.mem.scale == 0) {
      /* x86 does not allow this. try and swap base and index */
      int tmp;
      tmp = op->valtag.mem.base.val;
      op->valtag.mem.base.val = op->valtag.mem.index.val;
      op->valtag.mem.index.val = tmp;
    }
  }
}

static int
operand_accesses_memory_outside_region(operand_t const *op,
    uint32_t region_start, uint32_t region_stop)
{
  if (op->type != op_mem) {
    return 0;
  }
  if (   op->valtag.mem.base.val == -1
      && op->valtag.mem.index.val == -1
      && op->valtag.mem.disp.val >= region_start
      && op->valtag.mem.disp.val < region_stop) {
    return 0;
  } else {
    return 1;
  }
}

int
i386_insn_accesses_memory_outside_region(i386_insn_t const *insn, uint32_t region_start,
    uint32_t region_stop)
{
  int i;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (operand_accesses_memory_outside_region(&insn->op[i], region_start,
            region_stop)) {
      return 1;
    }
  }
  return 0;
}


uint8_t i386_jump_insn[5] = {0xe9, 0x00, 0x00, 0x00, 0x00};
int i386_jump_target_offset = 1; /* offset of target in i386_jump_insn */

static int
expand_jump (uint8_t *bincode)
{
  /* jecxz should not reach here. */
  ASSERT(bincode[0] != 0xe3);
  if (bincode[0] >= 0x70 && bincode[0] < 0x80) {
    /* convert 0x7_ --> 0xf 0x8_ */
    uint8_t target8 = *(int8_t *)(&bincode[1]);
    uint32_t target = (int32_t)target8;
    DBG (MD_ASSEMBLE, "bincode=%hhx,%hhx,%hhx,%hhx. jump target = %x\n", bincode[0],
        bincode[1], bincode[2], bincode[3], target);
    bincode[1] = bincode[0] + 0x10;
    bincode[0] = 0xf;
    *((uint32_t *)&bincode[2]) = target;
    return 6;
  }

  if (bincode[0] == 0xeb) {
    bincode[0] = 0xe9;
    return 5;
  }
  return 0;
}

static int
is_conditional_jump(uint8_t const *bincode)
{
  if (bincode[0]>=0x70 && bincode[0]<0x80)
    return 1;

  if (bincode[0] == 0xe3)
    return 1;

  if (bincode[0] != 0x0f)
    return 0;

  if (bincode[1]>=0x80 && bincode[1]<0x90)
    return 1;

  return 0;
}

static int
is_jump(uint8_t const *bincode)
{
  if (bincode[0] == 0xe9 || bincode[0] == 0xeb)
    return 1;
  return 0;
}

static bool
is_call(uint8_t const *bincode)
{
  if (bincode[0] == 0xe8)
    return true;
  return false;
}

static bool
is_jecxz_or_loop(uint8_t const *bincode)
{
  return bincode[0] >= 0xe0 && bincode[0] <= 0xe3;
}

static bool
is_jcxz(uint8_t const *bincode)
{
  return bincode[0] == 0x67 && bincode[1] == 0xe3;
}

static ssize_t
i386_patch_jump_offset(uint8_t *tmpbuf, ssize_t len, sym_type_t symtype,
    unsigned long symval, char const *binptr, bool first_pass)
{
  int jmp_off = 0;
  int jmp_target_size = 0;
  int32_t val = 0;

  if (is_jecxz_or_loop(tmpbuf)) {
    DBG(MD_ASSEMBLE, "%s", "is_jecxz_or_loop\n");
    jmp_off = 1;
    jmp_target_size = 1;
  } else if (is_jcxz(tmpbuf)) {
    DBG(MD_ASSEMBLE, "%s", "is_jcxz\n");
    jmp_off = 2;
    jmp_target_size = 1;
  } else if (is_conditional_jump(tmpbuf)) {
    DBG(MD_ASSEMBLE, "%s", "is_conditional_jump\n");
    int exp = expand_jump(tmpbuf);
    len = exp ? exp : len;
    jmp_off = 2;
    jmp_target_size = 4;
  } else if (is_jump(tmpbuf)) {
    DBG(MD_ASSEMBLE, "%s", "is_jump\n");
    int exp = expand_jump(tmpbuf);
    len = exp ? exp : len;
    jmp_off = 1;
    jmp_target_size = 4;
  } else if (is_call(tmpbuf)) {
    DBG(MD_ASSEMBLE, "%s", "is_call\n");
    jmp_off = 1;
    jmp_target_size = 4;
  }

  if (symtype == SYMBOL_PCREL) {
    ASSERT(jmp_off);
    val = symval - (unsigned long)((uint8_t *)binptr + jmp_off + jmp_target_size);
    DBG(MD_ASSEMBLE, "val=%x, symval=%lx, binptr=%p, jmp_off=%x\n", val, symval,
        binptr, jmp_off);
  } else if (symtype == SYMBOL_ABS) {
    val = symval;
  }
  if (jmp_target_size == 4) {
    *(uint32_t *)((uint8_t *)tmpbuf + jmp_off) = val;
  } else if (jmp_target_size == 1) {
    ASSERT(first_pass || ((int8_t)val == val));
    *(uint8_t *)((uint8_t *)tmpbuf + jmp_off) = (uint8_t)val;
  }
  DBG(MD_ASSEMBLE, "i386_md_assemble() returned %s\n",
      hex_string(tmpbuf, len, as1, sizeof as1));
  return len;
}

static size_t
is_non_sib_memory_access(char const *buf, i386_as_mode_t *mode)
{
  char const *ptr;

  ptr = buf;
  if (*ptr != '(') {
    return 0;
  }
  *mode = I386_AS_MODE_32;
  while (*ptr != ')') {
    if (*ptr == ',') {
      return 0;
    }
    if (*ptr == '%') {
      if (*(ptr + 1) == 'r') {
        *mode = I386_AS_MODE_64;
      }
    }
    ASSERT(*ptr != '\0');
    ptr++;
  }
  ASSERT(*ptr == ')');
  //cout << __func__ << " " << __LINE__ << ": mode = " << *mode << " for " << buf << endl;
  return (ptr + 1) - buf;
}

static size_t
convert_to_sib_memory_access(char *dst, size_t dst_size, char const *src, size_t srclen, i386_as_mode_t mode)
{
  char *ptr, *end;

  ptr = dst;
  end = dst + dst_size;

  memcpy(ptr, src, srclen - 1);
  ptr += srclen - 1;
  if (mode == I386_AS_MODE_32) {
    ptr += snprintf(ptr, end - ptr, ",%%eiz,1)");
  } else {
    ptr += snprintf(ptr, end - ptr, ",%%riz,1)");
  }

  return ptr - dst;
}

static void
i386_assembly_add_fake_index_to_memory_addresses(char *dst, char const *src,
    size_t dst_size)
{
  char *dptr, *dend;
  char const *sptr;
  size_t len;

  dptr = dst;
  dend = dptr + dst_size;
  for (sptr = src, dptr = dst; *sptr;) {
    *dptr = *sptr;
    i386_as_mode_t mode;
    if (len = is_non_sib_memory_access(sptr, &mode)) {
      dptr += convert_to_sib_memory_access(dptr, dend - dptr, sptr, len, mode);
      sptr += len;
    } else {
      sptr++;
      dptr++;
    }
  }
  *dptr = '\0';
}

static ssize_t
__x86_assemble(translation_instance_t *ti, uint8_t *bincode, char const *assembly,
    sym_t *symbols, long *num_symbols)
{
  char assembly_copy[strlen(assembly) * 2 + 64];
  char const *ptr;

  if (regular_assembly_only) {
    i386_assembly_add_fake_index_to_memory_addresses(assembly_copy, assembly,
        sizeof assembly_copy);
    ptr = assembly_copy;
  } else {
    ptr = assembly;
  }
  return insn_assemble(ti, bincode, ptr, &i386_md_assemble,
      &i386_patch_jump_offset, symbols, num_symbols);
}

ssize_t
x86_assemble(translation_instance_t *ti, uint8_t *bincode, size_t bincode_size,
    char const *assembly, i386_as_mode_t mode)
{
  autostop_timer func_timer(__func__);
  char *assembly_copy;
  sym_t *symbols;
  long num_symbols;
  long ret;

  ASSERT(gas_inited);

  assembly_copy = (char *)malloc((strlen(assembly) + 16) * sizeof(char));
  ASSERT(assembly_copy);

  symbols = new sym_t[strlen(assembly) + 1];
  ASSERT(symbols);

  num_symbols = 0;
  strip_extra_whitespace_and_comments(assembly_copy, assembly);
  DBG(MD_ASSEMBLE, "assembly:\n%s\nassembly_copy:\n%s\n", assembly, assembly_copy);
  if (mode == I386_AS_MODE_32) {
    flag_code = CODE_32BIT;
    regular_assembly_only = false;
  } else if (mode == I386_AS_MODE_64) {
    flag_code = CODE_64BIT;
    regular_assembly_only = false;
  } else if (mode == I386_AS_MODE_64_REGULAR) {
    flag_code = CODE_64BIT;
    regular_assembly_only = true;
  } else NOT_REACHED();

  /* do it twice so that the symbols are correctly resolved */
  __x86_assemble(ti, bincode, assembly_copy, symbols, &num_symbols);
  ret = __x86_assemble(ti, bincode, assembly_copy, symbols, &num_symbols);
  free(assembly_copy);
  delete[] symbols;
  return ret;
}

static operand_t *
i386_insn_get_imm_operand(i386_insn_t const *insn)
{
  operand_t *ret;
  int i;

  ret = NULL;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_imm) {
      ASSERT(!ret);
      ret = (operand_t *)&insn->op[i];
    }
  }
  return ret;
}

operand_t *
i386_insn_get_pcrel_operand_full(i386_insn_t const *insn)
{
  operand_t *ret;
  int i;

  ret = NULL;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_pcrel) {
      ASSERT(!ret);
      ret = (operand_t *)&insn->op[i];
    }
  }
  return ret;
}

valtag_t *
i386_insn_get_pcrel_operand(i386_insn_t const *insn)
{
  operand_t *op;

  op = i386_insn_get_pcrel_operand_full(insn);
  if (op) {
    return &op->valtag.pcrel;
  }
  return NULL;
}



static char *
i386_insn_with_relocs_to_string(i386_insn_t const *i386insn, char const *reloc_strings,
    size_t reloc_strings_size, i386_as_mode_t mode, char *buf, int buf_size)
{
  char *ptr = buf, *end = buf + buf_size;
  *ptr = '\0';

  insn2str(i386insn, reloc_strings, reloc_strings_size, mode, ptr, end - ptr);
  ptr += strlen(ptr);
  *ptr++ = '\n';
  *ptr = '\0';

  return buf;
}

char *
i386_insn_to_string(i386_insn_t const *i386insn, char *buf, size_t buf_size)
{
  return i386_insn_with_relocs_to_string(i386insn, NULL, 0, I386_AS_MODE_32, buf, buf_size);
}

char *
i386_iseq_with_relocs_to_string(i386_insn_t const *iseq, long iseq_len,
    char const *reloc_strings, size_t reloc_strings_size, i386_as_mode_t mode,
    char *buf, size_t buf_size)
{
  char *ptr = buf, *end = buf+buf_size;
  i386_insn_t const *iptr;
  bool seen_branch_to_end;
  valtag_t *op_pcrel;
  i386_insn_t insn;
  long i;

  *ptr = '\0';
  seen_branch_to_end = false;

  for (i = 0; i < iseq_len; i++) {
    ptr += snprintf(ptr, end - ptr, ".i%ld: ", i);
    if (   (op_pcrel = i386_insn_get_pcrel_operand(&iseq[i]))
        && op_pcrel->tag == tag_const) {
      i386_insn_copy(&insn, &iseq[i]);
      op_pcrel = i386_insn_get_pcrel_operand(&insn);
      ASSERT(op_pcrel && op_pcrel->tag == tag_const);
      op_pcrel->val += (i + 1);
      op_pcrel->val = (dst_ulong)op_pcrel->val;
      if (op_pcrel->val == iseq_len) {
        seen_branch_to_end = true;
      }
      iptr = &insn;
    } else {
      iptr = &iseq[i];
    }
    i386_insn_with_relocs_to_string(iptr, reloc_strings, reloc_strings_size,
        mode, ptr, end - ptr);
    ptr += strlen(ptr);
  }
  if (seen_branch_to_end) {
    ptr += snprintf(ptr, end - ptr, ".i%ld:\n", iseq_len);
  }
  return buf;
}

char *
i386_iseq_to_string_mode(i386_insn_t const *iseq, long iseq_len, i386_as_mode_t mode,
    char *buf, size_t buf_size)
{
  return i386_iseq_with_relocs_to_string(iseq, iseq_len, NULL, 0, mode, buf, buf_size);
}

char *
i386_iseq_to_string(i386_insn_t const *iseq, long iseq_len, char *buf, size_t buf_size)
{
  return i386_iseq_to_string_mode(iseq, iseq_len, I386_AS_MODE_32, buf, buf_size);
}

static void
i386_operand_rename(operand_t *op, regmap_t const *dst_regmap,// imm_map_t const *imm_map,
    imm_vt_map_t const *imm_vt_map,
    memslot_map_t const *memslot_map,
    regmap_t const *src_regmap,
    nextpc_map_t const *nextpc_map,
    nextpc_t const *nextpcs, long num_nextpcs, src_tag_t tag,
    dst_ulong base_gpr_offset)
{
  switch (op->type) {
    case op_reg:
      if (regmap_maps_reg_to_constant(dst_regmap, I386_EXREG_GROUP_GPRS, op->valtag.reg.val)) {
        op->type = op_imm;
        long orig_val = op->valtag.reg.val;
        op->valtag.imm.tag = tag_const;
        op->valtag.imm.val = 0;
        imm_valtag_add_constant_using_reg_id(&op->valtag.imm, dst_regmap->regmap_extable.at(I386_EXREG_GROUP_GPRS).at(orig_val));
      } else {
        I386_OPERAND_RENAME_EXREG(dst_regmap, reg, I386_EXREG_GROUP_GPRS, tag);
      }
      /*ASSERT(op->valtag.reg.tag == tag_var);
      op->valtag.reg.val = regmap_rename_reg(dst_regmap, op->valtag.reg.val);
      op->valtag.reg.tag = tag_const;*/
      break;
    case op_imm:
      if (op->valtag.imm.tag != tag_const) {
        /*op->valtag.imm.val = */imm_vt_map_rename_vconstant(/*op->valtag.imm.val,
            op->valtag.imm.tag,
            op->valtag.imm.mod.imm.modifier, op->valtag.imm.mod.imm.constant,
            op->valtag.imm.mod.imm.sconstants,
            op->valtag.imm.mod.imm.exvreg_group, */
            &op->valtag.imm,// imm_map,
            imm_vt_map,
            nextpcs, num_nextpcs, src_regmap/*, tag*/);
        //op->valtag.imm.tag = tag_const;
      }
      break;
    case op_mem:
      i386_operand_rename_memory_operands(op, dst_regmap,// imm_map,
          imm_vt_map, memslot_map,
          src_regmap, nextpcs, num_nextpcs, tag, base_gpr_offset);
      break;
    case op_seg:
      I386_OPERAND_RENAME_EXREG(dst_regmap, seg, I386_EXREG_GROUP_SEGS, tag);
      break;
    case op_mmx:
      I386_OPERAND_RENAME_EXREG(dst_regmap, mmx, I386_EXREG_GROUP_MMX, tag);
      break;
    case op_xmm:
      I386_OPERAND_RENAME_EXREG(dst_regmap, xmm, I386_EXREG_GROUP_XMM, tag);
      break;
    case op_ymm:
      I386_OPERAND_RENAME_EXREG(dst_regmap, ymm, I386_EXREG_GROUP_YMM, tag);
      break;
#define case_rename_flag(flagname) \
    case op_flags_##flagname: \
      if (op->valtag.flags_##flagname.tag == tag_var) {\
        op->valtag.flags_##flagname.tag = tag; \
      } \
      break
    case_rename_flag(other);
    case_rename_flag(eq);
    case_rename_flag(ne);
    case_rename_flag(ult);
    case_rename_flag(slt);
    case_rename_flag(ugt);
    case_rename_flag(sgt);
    case_rename_flag(ule);
    case_rename_flag(sle);
    case_rename_flag(uge);
    case_rename_flag(sge);
    //case op_flags_unsigned:
    //  if (op->valtag.flags_unsigned.tag == tag_var) {
    //    op->valtag.flags_unsigned.tag = tag;
    //  }
    //  break;
    //case op_flags_signed:
    //  if (op->valtag.flags_signed.tag == tag_var) {
    //    op->valtag.flags_signed.tag = tag;
    //  }
    //  break;
    case op_st_top:
      if (op->valtag.st_top.tag == tag_var) {
        op->valtag.st_top.tag = tag;
      }
      break;
    case op_fp_data_reg:
      I386_OPERAND_RENAME_EXREG(dst_regmap, fp_data_reg, I386_EXREG_GROUP_FP_DATA_REGS, tag);
      break;
    case op_fp_control_reg_rm:
      if (op->valtag.fp_control_reg_rm.tag == tag_var) {
        op->valtag.fp_control_reg_rm.tag = tag;
      }
      break;
    case op_fp_control_reg_other:
      if (op->valtag.fp_control_reg_other.tag == tag_var) {
        op->valtag.fp_control_reg_other.tag = tag;
      }
      break;
    case op_fp_tag_reg:
      if (op->valtag.fp_tag_reg.tag == tag_var) {
        op->valtag.fp_tag_reg.tag = tag;
      }
      break;
    case op_fp_last_instr_pointer:
      if (op->valtag.fp_last_instr_pointer.tag == tag_var) {
        op->valtag.fp_last_instr_pointer.tag = tag;
      }
      break;
    case op_fp_last_operand_pointer:
      if (op->valtag.fp_last_operand_pointer.tag == tag_var) {
        op->valtag.fp_last_operand_pointer.tag = tag;
      }
      break;
    case op_fp_opcode:
      if (op->valtag.fp_opcode.tag == tag_var) {
        op->valtag.fp_opcode.tag = tag;
      }
      break;
    case op_mxcsr_rm:
      if (op->valtag.mxcsr_rm.tag == tag_var) {
        op->valtag.mxcsr_rm.tag = tag;
      }
      break;
    case op_fp_status_reg_c0:
      if (op->valtag.fp_status_reg_c0.tag == tag_var) {
        op->valtag.fp_status_reg_c0.tag = tag;
      }
      break;
    case op_fp_status_reg_c1:
      if (op->valtag.fp_status_reg_c1.tag == tag_var) {
        op->valtag.fp_status_reg_c1.tag = tag;
      }
      break;
    case op_fp_status_reg_c2:
      if (op->valtag.fp_status_reg_c2.tag == tag_var) {
        op->valtag.fp_status_reg_c2.tag = tag;
      }
      break;
    case op_fp_status_reg_c3:
      if (op->valtag.fp_status_reg_c3.tag == tag_var) {
        op->valtag.fp_status_reg_c3.tag = tag;
      }
      break;
    case op_fp_status_reg_other:
      if (op->valtag.fp_status_reg_other.tag == tag_var) {
        op->valtag.fp_status_reg_other.tag = tag;
      }
      break;
    case op_pcrel:
      if (nextpc_map/* && tag == tag_var */&& op->valtag.pcrel.tag == tag_var) {
        src_tag_t nextpc_tag;
        int nextpc_val;

        ASSERT(   op->valtag.pcrel.val >= 0
               && op->valtag.pcrel.val < MAX_NUM_NEXTPCS);

        nextpc_tag = nextpc_map->table[op->valtag.pcrel.val].tag;
        nextpc_val = nextpc_map->table[op->valtag.pcrel.val].val;

        ASSERT(nextpc_tag == tag_var || nextpc_tag == tag_const);
        ASSERT(   nextpc_tag != tag_var
               || (nextpc_val >= 0 && nextpc_val < MAX_NUM_NEXTPCS));
        ASSERT(   nextpc_tag != tag_const
               || (nextpc_val >= 0 && nextpc_val < MAX_INSN_ID_CONSTANTS));

        op->valtag.pcrel.tag = nextpc_tag;
        op->valtag.pcrel.val = nextpc_val;
      }
      break;
    default:
      break;
  }
}

static void
operand_copy(operand_t *dst, operand_t const *src)
{
  memcpy(dst, src, sizeof(operand_t));
}



static void
i386_insn_rename(i386_insn_t *insn,
    regmap_t const *dst_regmap,// struct imm_map_t const *imm_map,
    imm_vt_map_t const *imm_vt_map,
    memslot_map_t const *memslot_map,
    regmap_t const *src_regmap, nextpc_map_t const *nextpc_map,
    nextpc_t const *nextpcs, long num_nextpcs,
    src_tag_t tag, dst_ulong base_gpr_offset)
{
  if (i386_insn_is_nop(insn)) {
    return;
  }
  long i;

  DBG(I386_INSN, "%s", "Calling i386::rename() functions\n");
  DBG(I386_INSN, "instruction: %s\n",
      i386_insn_to_string(insn, as1, sizeof as1));
  DBG(I386_INSN, "dst_regmap:\n%s",
      regmap_to_string(dst_regmap, as1, sizeof as1));
  DBG(I386_INSN, "Printing imm_vt_map:\n%s",
      imm_vt_map?imm_vt_map_to_string(imm_vt_map, as1,
          sizeof as1):"(null)");
  DBG(I386_INSN, "src_regmap:\n%s",
      src_regmap?regmap_to_string(src_regmap, as1, sizeof as1):"(null)");

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    i386_operand_rename(&insn->op[i], dst_regmap,// imm_map,
        imm_vt_map, memslot_map,
        src_regmap, nextpc_map, nextpcs,
        num_nextpcs, tag, base_gpr_offset);
  }
  //handle special case of imull $constant, reg; convert it to imull $constant, reg, reg
  if (   !strcmp(opctable_name(insn->opc), "imull")
      && insn->op[insn->num_implicit_ops].type == op_reg
      && insn->op[insn->num_implicit_ops + 1].type == op_imm
      && insn->op[insn->num_implicit_ops + 2].type == invalid) {
    operand_copy(&insn->op[insn->num_implicit_ops + 2], &insn->op[insn->num_implicit_ops + 1]);
    operand_copy(&insn->op[insn->num_implicit_ops + 1], &insn->op[insn->num_implicit_ops]);
  }
  //memvt_list_rename(&insn->memtouch, dst_regmap);
  DBG(I386_INSN, "after rename, instruction: %s\n",
      i386_insn_to_string(insn, as1, sizeof as1));
}

static void
i386_insn_update_base_gpr_offset(dst_ulong *base_gpr_offset, i386_insn_t const *insn, exreg_id_t base_gpr)
{
  if (base_gpr == -1) {
    return;
  }
  ASSERT(*base_gpr_offset >= 0);
  ASSERT(base_gpr >= 0 && base_gpr < I386_NUM_GPRS);
  /*regset_t use, def;
  i386_insn_get_usedef_regs(insn, &use, &def); //not  possible because this is usually a tag_const instruction, and we support usedef regs only for tag_var insns
  if (!regset_belongs_ex(&def, I386_EXREG_GROUP_GPRS, reg_identifier_t(base_gpr))) {
    return;
  }*/
  char const *opc = opctable_name(insn->opc);
  ASSERT(base_gpr == R_ESP);
  if (strstart(opc, "pushl", NULL)) {
    *base_gpr_offset += 4;
  } else if (strstart(opc, "popl", NULL)) {
    *base_gpr_offset -= 4;
    ASSERT(*base_gpr_offset >= 0);
  } else if (strstart(opc, "ret", NULL)) {
    //do nothing
  } else {
    //cout << __func__ << " " << __LINE__ << ": unhandled instruction in translation that updates stack pointer: " << i386_insn_to_string(insn, as1, sizeof as1) << endl;
    //NOT_IMPLEMENTED();
    return;
  }
}

void
i386_iseq_rename(i386_insn_t *iseq, long iseq_len,
    regmap_t const *dst_regmap, imm_vt_map_t const *imm_vt_map,
    memslot_map_t const *memslot_map,
    regmap_t const *src_regmap, nextpc_map_t const *nextpc_map,
    nextpc_t const *nextpcs, long num_nextpcs)
{
  //cout << __func__ << " " << __LINE__ << ": entry\n";
  exreg_id_t base_gpr = memslot_map ? memslot_map->get_base_reg() : -1;
  dst_ulong base_gpr_offset = 0;
  for (long i = 0; i < iseq_len; i++) {
    i386_insn_rename(&iseq[i], dst_regmap, imm_vt_map, memslot_map, src_regmap, nextpc_map,
        nextpcs, num_nextpcs, tag_const, base_gpr_offset);
    i386_insn_update_base_gpr_offset(&base_gpr_offset, &iseq[i], base_gpr);
  }
}

void
i386_iseq_rename_var(i386_insn_t *iseq, long iseq_len, regmap_t const *dst_regmap,
    imm_vt_map_t const *imm_vt_map, memslot_map_t const *memslot_map,
    regmap_t const *src_regmap,
    nextpc_map_t const *nextpc_map, nextpc_t const *nextpcs, long num_nextpcs)
{
  long i;

  for (i = 0; i < iseq_len; i++) {
    i386_insn_rename(&iseq[i], dst_regmap, imm_vt_map, memslot_map, src_regmap, nextpc_map,
        nextpcs, num_nextpcs, tag_var, 0);
  }
}

void
i386_insn_rename_constants(i386_insn_t *insn, imm_vt_map_t const *imm_vt_map/*,
    src_ulong const *nextpcs, long num_nextpcs,
    regmap_t const *regmap*/)
{
  int i;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    switch (insn->op[i].type) {
      case op_mem:
        if (/*   insn->op[i].tag.mem.all == tag_const
            && */insn->op[i].valtag.mem.disp.tag != tag_const) {
          ASSERT(insn->op[i].valtag.mem.disp.val < imm_vt_map->num_imms_used);
          /*insn->op[i].valtag.mem.disp.val = */imm_vt_map_rename_vconstant(
              /*insn->op[i].valtag.mem.disp.val, insn->op[i].valtag.mem.disp.tag,
              insn->op[i].valtag.mem.disp.mod.imm.modifier,
              insn->op[i].valtag.mem.disp.mod.imm.constant,
              insn->op[i].valtag.mem.disp.mod.imm.sconstants,
              insn->op[i].valtag.mem.disp.mod.imm.exvreg_group,*/
              &insn->op[i].valtag.mem.disp,
              imm_vt_map, NULL, 0, NULL/*, tag_const*/);
          //insn->op[i].valtag.mem.disp.tag = tag_const;
        }
        break;
      case op_imm:
        if (insn->op[i].valtag.imm.tag != tag_const) {
          ASSERT(insn->op[i].valtag.imm.val < imm_vt_map->num_imms_used);
          /*insn->op[i].valtag.imm.val = */imm_vt_map_rename_vconstant(
              /*insn->op[i].valtag.imm.val, insn->op[i].valtag.imm.tag,
              insn->op[i].valtag.imm.mod.imm.modifier,
              insn->op[i].valtag.imm.mod.imm.constant,
              insn->op[i].valtag.imm.mod.imm.sconstants,
              insn->op[i].valtag.imm.mod.imm.exvreg_group, */
              &insn->op[i].valtag.imm, imm_vt_map,
              NULL, 0, NULL/*, tag_const*/);
          //insn->op[i].valtag.imm.tag = tag_const;
        }
        break;
      default:
        break;
    }
  }
}

int
i386_insn_assemble(i386_insn_t const *insn, unsigned char *buf, int size)
{
  unsigned char lbuf[4096];
  long ret, len, aslen;
  char assembly[512];

  i386_insn_to_string(insn, assembly, sizeof assembly);
  DBG(MD_ASSEMBLE, "%s(): calling x86_assemble(%s)\n", __func__, assembly);

  if (!buf) {
    buf = lbuf;
    size = sizeof lbuf;
  }
  len = x86_assemble(NULL, buf, size, assembly, I386_AS_MODE_32);
  DBG(MD_ASSEMBLE, "%s(): x86_assemble() returned '%s'\n", __func__,
      hex_string(buf, len, as1, sizeof as1));
  ASSERT(len <= size);
  return len;
}

static nextpc_t
i386_insn_branch_target(i386_insn_t const *insn, src_ulong nip)
{
  vector<nextpc_t> targets = i386_insn_branch_targets(insn, nip);
  if (targets.size() == 0) {
    return (src_ulong)-1;
  }
  return *targets.begin();
}

long long
i386_iseq_compute_execution_cost(i386_insn_t const *iseq, long iseq_len, i386_costfn_t costfn)
{
  if (!iseq_len) {
    return 0;
  }
  ASSERT(iseq_len > 0);
  DBG(I386_INSN, "Calculating cost for:\n%s",
      i386_iseq_to_string(iseq, iseq_len, as1, sizeof as1));

  long i;
  int offsets[iseq_len], ends[iseq_len], cur_offset = 0;
  for (i = 0; i < iseq_len; i++) {
    offsets[i] = cur_offset;
    //cur_offset += i386_insn_assemble(&iseq[i], bin1, sizeof bin1);
    ends[i] = cur_offset;
  }
  DBG(I386_INSN, "Calculating cost for:\n%s",
      i386_iseq_to_string(iseq, iseq_len, as1, sizeof as1));

  int weight[iseq_len];
  memset(weight, 0x0, sizeof weight);
#define MAX_WEIGHT 1024
  weight[0] = MAX_WEIGHT;
  for (i = 0; i < iseq_len; i++) {
    if (   i386_insn_is_conditional_direct_branch(&iseq[i])
        && !i386_insn_is_function_call(&iseq[i])) {
      int target_addr = i386_insn_branch_target(&iseq[i], ends[i]).get_val();
      long target;
      for (target = i + 1; target < iseq_len; target++) {
        if (target_addr == offsets[target])
          break;
      }
      if (target != iseq_len) {
        weight[target] += weight[i]/2;
      }
      if ((i + 1) < iseq_len) {
        weight[i+1] += weight[i]/2;
      }
    } else if (i386_insn_is_branch_not_fcall(&iseq[i])) {
      long target_addr = i386_insn_branch_target(&iseq[i], ends[i]).get_val();
      long target;

      for (target = i + 1; target < iseq_len; target++) {
        if (target_addr == offsets[target]) {
          break;
        }
      }
      if (target != iseq_len) {
        weight[target] += weight[i];
      }
    } else {
      if ((i + 1) < iseq_len) {
        weight[i + 1] += weight[i];
      }
    }
  }

  DBG(I386_INSN, "Calculating cost for:\n%s",
      i386_iseq_to_string(iseq, iseq_len, as1, sizeof as1));
  long long cost = 0;
  for (i = 0; i < iseq_len; i++) {
    long long icost = (*costfn)(&iseq[i]);
    DBG(I386_INSN, "insn: %s\n", i386_insn_to_string(&iseq[i], as1, sizeof as1));
    cost += (weight[i] * icost)/MAX_WEIGHT;
    DBG(I386_INSN, "insn: %s\n", i386_insn_to_string(&iseq[i], as1, sizeof as1));
    DBG(I386_INSN, "i %ld: weight %d, icost %lld\n", i, weight[i], icost);
  }
  return cost;
}

int
i386_insn_invert_jump_cond (uint8_t *ptr)
{
  uint8_t *cur = ptr;
  if (*cur != 0xf) {
    err("cur = 0x%hhx, 0x%hhx, 0x%hhx\n", cur[0], cur[1], cur[2]);
    NOT_REACHED();
  }

  cur++;
  if (*cur >= 0x80 && *cur < 0x90) {
    *cur++ ^= 0x1;
  }
  return (cur - ptr);
}

/*
int
i386_insn_is_jump (i386_insn_t *i386insn)
{
  if (   i386_insn_is_direct_branch(i386insn)
      || i386_insn_is_indirect_branch(i386insn)) {
    return 1;
  } else {
    return 0;
  }
}
*/

int
i386_insns_are_jumptable_access(i386_insn_t *i386insns, int n)
{
#if ARCH_SRC == ARCH_I386
  if (i386_insn_is_indirect_branch_not_fcall(&i386insns[0])) {
    return 1;
  } else {
    return -1;
  }
#else
  if (n < 2) {
    return -1;
  }
  //printf("%s(): %s\n", __func__, x86_iseq_to_string(x86insns, 2, as1, sizeof as1));
  if (!i386_insn_is_lea(&i386insns[0])) {
    //printf("%s() %d: returning -1\n", __func__, __LINE__);
    return -1;
  }
  if (!i386_insn_is_indirect_branch_not_fcall(&i386insns[1])) {
    //printf("%s() %d: returning -1\n", __func__, __LINE__);
    return -1;
  }
  if (   i386insns[0].op[1].valtag.mem.disp.val < SRC_ENV_JUMPTABLE_ADDR
      || i386insns[0].op[1].valtag.mem.disp.val > SRC_ENV_JUMPTABLE_ADDR +
             8 * SRC_ENV_JUMPTABLE_INTERVAL) {
    //printf("%s() %d: returning -1\n", __func__, __LINE__);
    return -1;
  }

  if (!(   i386insns[0].op[0].valtag.reg.val == i386insns[0].op[1].valtag.mem.base.val
        && i386insns[0].op[0].valtag.mem.index.val == -1)) {
    //printf("%s() %d: returning -1\n", __func__, __LINE__);
    return -1;
  }

  if (   i386insns[0].op[0].valtag.reg.val != i386insns[1].op[0].valtag.reg.val
      && i386insns[0].op[0].valtag.reg.val != i386insns[1].op[0].valtag.mem.base.val) {
    //printf("%s() %d: returning -1\n", __func__, __LINE__);
    return -1;
  }

  //printf("%s() %d: returning %d\n", __func__, __LINE__, x86insns[0]->insn.op[0].val.reg);
  return i386insns[0].op[0].valtag.reg.val;
#endif
}

/*unsigned long
i386_insn_jump_operand (i386_insn_t const *i386insn)
{
  if (i386insn->op[0].type == op_pcrel) {
    return (unsigned long)i386insn->op[0].val.pcrel;
  } else {
    return (unsigned long)-1;
  }
}*/

/*static bool
i386_insn_is_branch_to_target0(i386_insn_t const *insn)
{
  if (   insn->op[0].type == op_pcrel
      && insn->op[0].tag.pcrel == tag_var
      && insn->op[0].val.pcrel == EXT_JUMP_ID0) {
    return true;
  }
  if (i386_insn_is_function_call(insn) || i386_insn_is_jecxz(insn)) {
    if (   insn->op[1].type == op_pcrel
        && insn->op[1].tag.pcrel == tag_var
        && insn->op[1].val.pcrel == EXT_JUMP_ID0) {
      return true;
    }
    return false;
  }
  return false;
}*/

static void
i386_insn_convert_es_fs_segments_to_ds(i386_insn_t *insn)
{
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_mem) {
      ASSERT(insn->op[i].valtag.mem.seg.sel.tag == tag_const);
      if (   insn->op[i].valtag.mem.seg.sel.val == R_ES
          || insn->op[i].valtag.mem.seg.sel.val == R_FS) {
        insn->op[i].valtag.mem.seg.sel.val = R_DS;
      }
    }
  }
}

static void
i386_iseq_convert_es_fs_segments_to_ds(i386_insn_t *iseq, size_t iseq_len)
{
  for (size_t i = 0; i < iseq_len; i++) {
    i386_insn_convert_es_fs_segments_to_ds(&iseq[i]);
  }
}

static size_t
__i386_iseq_assemble(translation_instance_t const *ti, i386_insn_t const *insns,
    size_t n_insns, unsigned char *buf, size_t size, i386_as_mode_t mode)
{
  unsigned char *ptr;
  long i, max_alloc;
  size_t binsize;
  char *assembly;

  if (n_insns == 0) {
    return 0;
  }
  max_alloc = n_insns * 100;
  if (max_alloc < 4096) {
    max_alloc = 4096;
  }
  assembly = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(assembly);

  if (mode == I386_AS_MODE_64 || mode == I386_AS_MODE_64_REGULAR) {
    i386_insn_t *insns_copy = new i386_insn_t[n_insns];
    i386_iseq_copy(insns_copy, insns, n_insns);
    i386_iseq_convert_es_fs_segments_to_ds(insns_copy, n_insns);
    insns = insns_copy;
  }

  i386_iseq_to_string_mode(insns, n_insns, mode, assembly, max_alloc);
  DBG(MD_ASSEMBLE, "calling x86_assemble(%p): %s\n", ti, assembly);
  binsize = x86_assemble((translation_instance_t *)ti, buf, size, assembly, mode);
  DBG(MD_ASSEMBLE, "returned from x86_assemble(%p)\n", ti);
  if (!binsize) {
    if (mode == I386_AS_MODE_64 || mode == I386_AS_MODE_64_REGULAR) {
      delete [] insns;
    }
    //fprintf(stderr, "assembly:\n%s\n", assembly);
    return binsize;
  }
  ASSERT(binsize && binsize < size);
  free(assembly);

  DBG(MD_ASSEMBLE, "%s", "patching pcrels\n");
  for (ptr = buf, i = 0; ptr < buf + binsize; i++) {
    i386_insn_t const *insn;
    i386_insn_t as;
    valtag_t *op;
    valtag_t const *op_insn;
    size_t len, len2;
    long j;

    //len = disas_insn_i386(ptr, (long)ptr, &dummy, mode);
    DBG(MD_ASSEMBLE, "patching pcrels for insn %ld, ptr = %p\n", i, ptr);
    len = i386_insn_disassemble(&as, ptr, mode);
    DBG(MD_ASSEMBLE, "len for insn %ld, ptr = %p is len=%zd: insn = %s\n", i, ptr, len, i386_insn_to_string(&as, as1, sizeof as1));
    insn = &insns[i];
    if (op = i386_insn_get_pcrel_operand(&as)) {
      op_insn = i386_insn_get_pcrel_operand(insn);
      ASSERT(op_insn);
      if (op_insn->tag == tag_var) {
        DBG(MD_ASSEMBLE, "Converting pcrel at %lx+%lx=%lx val %lx to %lx\n", (long)ptr,
            (long)len, (long)(ptr + len), (long)op_insn->val,
            (long)(op_insn->val - (long)(ptr + len)));
        op->val = op_insn->val - (long)(ptr + len);
        op->val = (src_ulong)op->val;
        DBG(MD_ASSEMBLE, "op->val.pcrel=%lx\n", (long)op->val);
        len2 = i386_insn_assemble(&as, ptr, len);
        ASSERT(len == len2);
      } else if (op_insn->tag == tag_reloc_string) {
        dst_ulong val;

        val = op_insn->val;
        op->val = -4;
        len2 = i386_insn_assemble(&as, ptr, len);
        ASSERT(len == len2);
        ti_add_reloc_entry((translation_instance_t *)ti, &ti->reloc_strings[val],
            (ptr + 1) - TX_TEXT_START_PTR/*code_gen_start*/, MYTEXT_SECTION_NAME, R_386_PC32);
        DBG(RELOC_STRINGS, "Adding reloc entry at offset %lx for %s.\n", (long)(ptr + 1 - TX_TEXT_START_PTR/*code_gen_start*/), &ti->reloc_strings[val]);
      }
    }
    ptr += len;
  }
  if (mode == I386_AS_MODE_64 || mode == I386_AS_MODE_64_REGULAR) {
    delete [] insns;
  }
  return binsize;
}

size_t
i386_iseq_assemble(translation_instance_t const *ti, i386_insn_t const *insns,
    size_t n_insns, unsigned char *buf, size_t size, i386_as_mode_t mode)
{
  size_t ret;

  stopwatch_run(&i386_iseq_assemble_timer);
  ret = __i386_iseq_assemble(ti, insns, n_insns, buf, size, mode);
  stopwatch_stop(&i386_iseq_assemble_timer);

  return ret;
}

static void
i386_operand_rename_addresses_to_symbols(operand_t *op, input_exec_t const *in, src_ulong pc, size_t pc_size, src_ulong insn_end_pc)
{
  if (op->type == op_imm) {
    valtag_rename_address_to_symbol(&op->valtag.imm, in, pc, pc_size, insn_end_pc);
  } else if (op->type == op_mem) {
    valtag_rename_address_to_symbol(&op->valtag.mem.disp, in, pc, pc_size, insn_end_pc);
  }
}

static void
i386_operand_rename_symbols_to_addresses(operand_t *op, input_exec_t const *in, struct chash const *tcodes)
{
  if (op->type == op_imm) {
    valtag_rename_symbol_to_address(&op->valtag.imm, in, tcodes);
  } else if (op->type == op_mem) {
    valtag_rename_symbol_to_address(&op->valtag.mem.disp, in, tcodes);
  }
}

static map<int, src_ulong>
identify_relocatable_operand_to_pc_map(input_exec_t const *in, src_ulong pc, size_t insn_size)
{
  uint8_t bincode[insn_size];
  i386_insn_t *orig_insn = new i386_insn_t;
  ASSERT(orig_insn);
  i386_insn_t *insn = new i386_insn_t;
  ASSERT(insn);
  int disas;

  src_ulong orig_pc = pc_get_orig_pc(pc);
  for (int i = 0; i < insn_size; i++) {
    bincode[i] = ldub_input(in, orig_pc + i);
  }
  i386_insn_init(orig_insn);
#if ARCH_DST == ARCH_I386
  i386_as_mode_t i386_as_mode = I386_AS_MODE_32;
#elif ARCH_DST == ARCH_X64
  i386_as_mode_t i386_as_mode = I386_AS_MODE_64;
#else
#error unknown ARCH_DST
#endif
  disas = i386_insn_disassemble(orig_insn, bincode, i386_as_mode);
  map<src_ulong, int> pc_to_operand_map;
  for (int i = 0; i < insn_size; i++) {
    bincode[i]++;
    int disas2 = i386_insn_disassemble(insn, bincode, i386_as_mode);
    bincode[i]--;
    if (!disas2) {
      CPP_DBG_EXEC2(SYMBOLS, cout << __func__ << " " << __LINE__ << ": could not disassemble after updating pc " << hex << pc + i << dec << endl);
      continue;
    }
    CPP_DBG_EXEC2(SYMBOLS, cout << __func__ << " " << __LINE__ << ": after updating pc " << hex << pc + i << dec << ": insn = " << i386_insn_to_string(insn, as1, sizeof as1) << endl);
    if (insn->opc != orig_insn->opc) {
      continue;
    }
    set<int> differing_operands;
    bool insn_changed_drastically = false;
    for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
      if (insn->op[i].type != orig_insn->op[i].type) {
        insn_changed_drastically = true;
        break;
      }
      if (!operands_equal(&insn->op[i], &orig_insn->op[i])) {
        if (insn->op[i].type == op_imm) {
          ASSERT(insn->op[i].valtag.imm.val != orig_insn->op[i].valtag.imm.val);
          differing_operands.insert(i);
        } else if (insn->op[i].type == op_mem) {
          if (insn->op[i].size != orig_insn->op[i].size) {
            insn_changed_drastically = true;
            break;
          }
          if (insn->op[i].rex_used != orig_insn->op[i].rex_used) {
            insn_changed_drastically = true;
            break;
          }
          operand_t op_copy = insn->op[i];
          op_copy.valtag.mem.disp.val = orig_insn->op[i].valtag.mem.disp.val;
          //operand2str(&op_copy, nullptr, 0, I386_AS_MODE_32, as1, sizeof as1); cout  << "op_copy = " << as1 << endl;
          //operand2str(&orig_insn->op[i], nullptr, 0, I386_AS_MODE_32, as1, sizeof as1); cout  << "orig_insn->op[i] = " << as1 << endl;
          if (!operands_equal(&op_copy, &orig_insn->op[i])) {
            insn_changed_drastically = true;
            break;
          }
          ASSERT(insn->op[i].valtag.mem.disp.val != orig_insn->op[i].valtag.mem.disp.val);
          differing_operands.insert(i);
        } else {
          insn_changed_drastically = true;
          break;
        }
      }
    }
    if (insn_changed_drastically || differing_operands.size() != 1) {
      continue;
    }
    pc_to_operand_map.insert(make_pair(pc + i, *differing_operands.begin()));
  }
  delete insn;
  delete orig_insn;

  map<int, src_ulong> ret;
  for (int i = 0; i < insn_size; i++) {
    if (!pc_to_operand_map.count(pc + i)) {
      continue;
    }
    int opnum = pc_to_operand_map.at(pc + i);
    if (ret.count(opnum)) { //ignore if a mapping already exists because we are interested in the first mapping
      continue;
    }
    ret.insert(make_pair(opnum, pc + i));
  }
  return ret;
}

void
i386_insn_rename_addresses_to_symbols(i386_insn_t *insn, input_exec_t const *in, src_ulong pc)
{
  //identify instruction size
  i386_insn_t tmp;
  size_t insn_size;
  bool fetched;
  src_ulong orig_pc = pc_get_orig_pc(pc);
  fetched = i386_insn_fetch_raw(&tmp, in, orig_pc, &insn_size);
  ASSERT(fetched);
  src_ulong insn_end_pc = pc + insn_size;

  CPP_DBG_EXEC(SYMBOLS,
    cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << ", orig_pc = " << hex << orig_pc << dec << ", insn_size = " << insn_size << ", tmp = " << i386_insn_to_string(&tmp, as1, sizeof as1) << endl;
  );

  //create operand to pc mapping
  map<int, src_ulong> relocatable_operand_to_pc_map = identify_relocatable_operand_to_pc_map(in, pc, insn_size);

  CPP_DBG_EXEC(SYMBOLS,
    cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << ", insn = " << i386_insn_to_string(insn, as1, sizeof as1) << endl;
    cout << __func__ << " " << __LINE__ << ": relocatable_operand_to_pc_map =\n";
    for (auto const& ro : relocatable_operand_to_pc_map) {
      cout << ro.first << " -> " << hex << ro.second << dec << endl;
    }
  );

  //rename addresses to symbols in the respective operands based on these mappings
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (relocatable_operand_to_pc_map.count(i)) {
      src_ulong operand_pc = relocatable_operand_to_pc_map.at(i);
      i386_operand_rename_addresses_to_symbols(&insn->op[i], in, operand_pc, 1, insn_end_pc);
    }
  }
}

void
i386_insn_rename_symbols_to_addresses(i386_insn_t *insn, input_exec_t const *in, struct chash const *tcodes)
{
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    i386_operand_rename_symbols_to_addresses(&insn->op[i], in, tcodes);
  }
}

bool
i386_insn_fetch_raw_helper(i386_insn_t *insn, input_exec_t const *in, src_ulong eip,
    unsigned long *size, i386_as_mode_t i386_as_mode)
{
  uint8_t bincode[I386_MAX_INSN_SIZE];
  int i, disas;
  for (i = 0; i < I386_MAX_INSN_SIZE; i++) {
    bincode[i] = ldub_input(in, eip + i);
  }
  i386_insn_init(insn);
  disas = i386_insn_disassemble(insn, bincode, i386_as_mode);
  if (size) {
    if (disas == 0) {
      *size = 1;
    } else {
      *size = disas;
    }
  }

  DBG(REFRESH_DB, "insn:\n%s\n",
        i386_insn_to_string(insn, as1, sizeof as1));
  valtag_t *op;
  if (op = i386_insn_get_pcrel_operand(insn)) {
    ASSERT(op->tag == tag_const);
    if (op->val < 0 && (-op->val) < disas) {
      //cout << __func__ << " " << __LINE__ << ": reloc eip = " << hex << eip << dec << ": insn = " << i386_insn_to_string(insn, as1, sizeof as1) << endl;
      src_ulong offset = eip + disas + op->val;
      bool reloc_found = false;
      for (size_t i = 0; i < in->num_relocs; i++) {
        //cout << "reloc offset " << hex << in->relocs[i].offset << ", symbolValue " << in->relocs[i].symbolValue << ", symbolName = " << in->relocs[i].symbolName << ", addend = " << in->relocs[i].addend << ", calcValue = " << in->relocs[i].calcValue << dec << endl;
        if (in->relocs[i].offset == offset) {
          //op->val = in->relocs[i].symbolValue + in->relocs[i].addend;
          CPP_DBG_EXEC(SRC_INSN, cout << "created pcrel with reloc " << i << ": offset " << hex << in->relocs[i].offset << ", symbolValue " << in->relocs[i].symbolValue << ", symbolName = " << in->relocs[i].symbolName << ", addend = " << in->relocs[i].addend << ", calcValue = " << in->relocs[i].calcValue << dec << endl);
          op->val = i;
          op->tag = tag_input_exec_reloc_index;
          reloc_found = true;
          break;
        }
      }
      if (!reloc_found) {
        cout << __func__ << " " << __LINE__ << ": reloc eip = " << hex << eip << dec << ": insn = " << i386_insn_to_string(insn, as1, sizeof as1) << ", offset = " << hex << "0x" << offset << dec << endl;
        for (size_t i = 0; i < in->num_relocs; i++) {
          cout << "reloc " << i << ": reloc offset " << hex << in->relocs[i].offset << ", symbolValue " << in->relocs[i].symbolValue << ", symbolName = " << in->relocs[i].symbolName << ", addend = " << in->relocs[i].addend << ", calcValue = " << in->relocs[i].calcValue << dec << endl;
        }
      }
      ASSERT(reloc_found);
    }
  }
  return disas != 0;
}

bool
i386_insn_fetch_raw(i386_insn_t *insn, input_exec_t const *in, src_ulong eip,
    unsigned long *size)
{
  return i386_insn_fetch_raw_helper(insn, in, eip, size, I386_AS_MODE_32);
}

void
i386_insn_copy(i386_insn_t *dst, i386_insn_t const *src)
{
  if (dst == src) {
    return;
  }
  memcpy(dst, src, sizeof(i386_insn_t));
}

void
i386_iseq_copy(struct i386_insn_t *dst, struct i386_insn_t const *src, long n)
{
  autostop_timer func_timer(__func__);
  long i;
  for (i = 0; i < n; i++) {
    i386_insn_copy(&dst[i], &src[i]);
  }
}

bool
i386_insns_equal(i386_insn_t const *i1, i386_insn_t const *i2)
{
  int i;

  if (i386_insn_is_nop(i1) && i386_insn_is_nop(i2)) {
    return true;
  }
  if (i1->opc != i2->opc) {
    return false;
  }
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (!operands_equal(&i1->op[i], &i2->op[i])) {
      return false;
    }
  }
  /*if (!memvt_lists_equal(&i1->memtouch, &i2->memtouch)) {
    return false;
  }*/
  return true;
}

char const *shift_opcodes[] = {"shr", "shl", "ror", "rol", "sar", "sal", "rcl", "rcr"};
size_t num_shift_opcodes = sizeof shift_opcodes/sizeof shift_opcodes[0];

/* data movement string opcodes. */
char const *dm_string_opcodes[] = {"movsb", "movsw", "movsl", "movsq", "cmpsb", "cmpsw", "cmpsl", "cmpsq"};
size_t num_dm_string_opcodes = sizeof dm_string_opcodes/sizeof dm_string_opcodes[0];

char const *fstsw_string_opcodes[] = {"fstsw", "fnstsw"};
size_t num_fstsw_string_opcodes =
  sizeof fstsw_string_opcodes/sizeof fstsw_string_opcodes[0];

char const *blendv_opcodes[] = {"pblendvb", "blendvps", "blendvpd"};
size_t num_blendv_opcodes =
  sizeof blendv_opcodes/sizeof blendv_opcodes[0];

char const *out_string_opcodes[] = {"stos", "insb", "insw", "insl"};
size_t num_out_string_opcodes = sizeof out_string_opcodes/sizeof out_string_opcodes[0];

char const *in_string_opcodes[] = {"lods", "scas", "outsb", "outsw", "outsl"};
size_t num_in_string_opcodes = sizeof in_string_opcodes/sizeof in_string_opcodes[0];

static bool
is_shift_opcode(char const *opc)
{
  int i;
  for (i = 0; i < num_shift_opcodes; i++) {
    if (strstart(opc, shift_opcodes[i], NULL)) {
      return true;
    }
  }
  return false;
}

static bool
is_shiftx_opcode(char const *opc)
{
  char const *remaining;
  int i;

  for (i = 0; i < num_shift_opcodes; i++) {
    if (strstart(opc, shift_opcodes[i], &remaining) && *remaining == 'x') {
      return true;
    }
  }
  return false;
}



int
i386_max_operand_size(operand_t const *op, opc_t opct, int explicit_op_index)
{
  if (op->type != op_imm) {
    return 4;
  }
  if (explicit_op_index < 0) {
    //implicit op
    return 4;
  }
  char const *opc = opctable_name(opct);
  if (strstart(opc, "int", NULL)) {
    if (explicit_op_index == I386_NUM_GPRS) {
      return 1;
    }
  } else if (   is_shift_opcode(opc)
             && !is_shiftx_opcode(opc)
             && explicit_op_index == 1) {
    return 1;
  } else if (   is_shiftx_opcode(opc)
             && explicit_op_index == 2) {
    return 1;
  } else if (!strcmp(opc, "palignr")) {
    return 1;
  } else if (strstart(opc, "enter", NULL)) {
    if (explicit_op_index == 0) {
      return 2;
    } else if (explicit_op_index == 1) {
      return 1;
    } else NOT_REACHED();
  } else if (strstr(opc, "ret")) {
    ASSERT(explicit_op_index == 0);
    return 2;
  } else if (!strcmp(opc, "aam") || !strcmp(opc, "aad")) {
    ASSERT(explicit_op_index == 0);
    return 1;
  } else if (   !strcmp(opc, "inb")
             || !strcmp(opc, "inw")
             || !strcmp(opc, "inl")) {
    if (explicit_op_index == 1) {
      return 1;
    }
  } else if (   !strcmp(opc, "outb")
             || !strcmp(opc, "outw")
             || !strcmp(opc, "outl")) {
    if (explicit_op_index == 0) {
      return 1;
    }
  } else if (   (strstart(opc, "shld", NULL) || strstart(opc, "shrd", NULL))
             && explicit_op_index == 2) {
    return 1;
  } else if (   (strstart(opc, "pinsr", NULL) || strstart(opc, "pextr", NULL))
             && explicit_op_index == 2) {
    return 1;
  } else if (strstart(opc, "bt", NULL) && explicit_op_index == 1) {
    return 1;
  } else if (   strstart(opc, "psrl", NULL)
             || strstart(opc, "psra", NULL)
             || strstart(opc, "psll", NULL)) {
    if (explicit_op_index == 1) {
      return 1;
    }
  } else if (strstart(opc, "pshuf", NULL) && explicit_op_index == 2) {
    return 1;
  } else if (!strcmp(opc, "extrq") && (explicit_op_index == 1 || explicit_op_index == 2)) {
    return 1;
  } else if (!strcmp(opc, "insertq") && explicit_op_index == 2) {
    return 1;
  } else if ((strstart(opc, "round", NULL) || strstr(opc, "blend")) && explicit_op_index == 2) {
    return 1;
  } else if (strstart(opc, "pextr", NULL) && explicit_op_index == 2) {
    return 1;
  } else if (strstart(opc, "extractps", NULL) && explicit_op_index == 2) {
    return 1;
  } else if (strstart(opc, "insertps", NULL) && explicit_op_index == 2) {
    return 1;
  } else if (strstart(opc, "dpp", NULL) && explicit_op_index == 2) {
    return 1;
  } else if (!strcmp(opc, "mpsadbw") && explicit_op_index == 2) {
    return 1;
  } else if (strstart(opc, "pcmp", NULL) && explicit_op_index == 2) {
    return 1;
  }
  return 4;
}

long
i386_operand_strict_canonicalize_imms(i386_strict_canonicalization_state_t *scanon_state,
    long insn_index,
    long op_index,
    i386_insn_t *iseq_var[], long iseq_var_len[], regmap_t *st_regmap,
    imm_vt_map_t *st_imm_map,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    long num_iseq_var, long max_num_iseq_var)
{
  ASSERT(num_iseq_var >= 1);
  if (   iseq_var[0][insn_index].op[op_index].type != op_imm
      && iseq_var[0][insn_index].op[op_index].type != op_mem) {
    return num_iseq_var;
  }

  long max_num_out_var_ops = max_num_iseq_var - num_iseq_var + 1;
  long max_op_size, m, n, new_n, num_out_var_ops;
  imm_vt_map_t out_imm_maps[max_num_out_var_ops];
  valtag_t out_var_ops[max_num_out_var_ops];
  i386_insn_t *iseq_var_copy[num_iseq_var];
  imm_vt_map_t st_imm_map_copy[num_iseq_var];
  long iseq_var_copy_len[num_iseq_var];
  operand_t *var_op;
  imm_vt_map_t *st_map;

  ASSERT(num_iseq_var >= 1);
  ASSERT(max_num_iseq_var >= num_iseq_var);
  for (n = 0; n < num_iseq_var; n++) {
    //iseq_var_copy[n] = (i386_insn_t *)malloc(iseq_var_len[n] * sizeof(i386_insn_t));
    iseq_var_copy[n] = new i386_insn_t[iseq_var_len[n]];
    ASSERT(iseq_var_copy[n]);
    i386_iseq_copy(iseq_var_copy[n], iseq_var[n], iseq_var_len[n]);
    iseq_var_copy_len[n] = iseq_var_len[n];
    imm_vt_map_copy(&st_imm_map_copy[n], &st_imm_map[n]);
  }

  DBG(PEEP_TAB, "num_iseq_var = %ld\n", num_iseq_var);
  new_n = 0;
  for (n = 0; n < num_iseq_var; n++) {
    int explicit_op_index;
    i386_insn_t *insn;
    opc_t opc;

    insn = &iseq_var_copy[n][insn_index];
    var_op = &insn->op[op_index];
    explicit_op_index = op_index - insn->num_implicit_ops;
    opc = insn->opc;
    //i386_operand_copy(var_op, op);
    st_map = &st_imm_map_copy[n];
    DBG2(PEEP_TAB, "n = %ld, st_map:\n%s\n", n, imm_vt_map_to_string(st_map, as1, sizeof as1));
    max_op_size = i386_max_operand_size(var_op, opc, explicit_op_index) * 8;
    imm_vt_map_copy(&out_imm_maps[0], st_map);
    switch (var_op->type) {
      case op_imm:
        DBG4(PEEP_TAB, "insn = %s.\n", i386_insn_to_string(insn, as1, sizeof as1));
        DBG4(PEEP_TAB, "explicit_op_index = %d.\n", (int)explicit_op_index);
        DBG4(PEEP_TAB, "max_op_size = %d\n", (int)max_op_size);
        num_out_var_ops = ST_CANON_IMM(opc, explicit_op_index, out_var_ops,
            out_imm_maps, max_num_out_var_ops, var_op->valtag.imm, max_op_size, st_map, i386_imm_operand_needs_canonicalization);
        break;
      case op_mem:
        if (explicit_op_index >= 0) {
          num_out_var_ops = ST_CANON_IMM(opc, explicit_op_index, out_var_ops,
              out_imm_maps, max_num_out_var_ops, var_op->valtag.mem.disp, max_op_size, st_map, i386_imm_operand_needs_canonicalization);
        } else {
          DBG(PEEP_TAB, "not canonicalizing mem.disp.val %lx\n",
              (long)var_op->valtag.mem.disp.val);
          valtag_copy(&out_var_ops[0], &var_op->valtag.mem.disp);
          num_out_var_ops = 1;
        }
        break;
      default:
        num_out_var_ops = 1;
        break;
    }
    DBG(PEEP_TAB, "n = %ld, st_map:\n%s\n", n, imm_vt_map_to_string(st_map, as1, sizeof as1));
    for (m = 0; m < num_out_var_ops; m++) {
      ASSERT(new_n <= max_num_iseq_var);
      ASSERT(max_num_iseq_var - new_n >= num_iseq_var - n);
      if (m != 0 && max_num_iseq_var - new_n == num_iseq_var - n) {
        break;
      }
      ASSERT(new_n < max_num_iseq_var);
      DBG2(PEEP_TAB, "m = %ld, out_imm_map:\n%s\n", m,
          imm_vt_map_to_string(&out_imm_maps[m], as1, sizeof as1));
      ASSERT(new_n < max_num_iseq_var);
      i386_iseq_copy(iseq_var[new_n], iseq_var_copy[n], iseq_var_copy_len[n]);
      iseq_var_len[new_n] = iseq_var_copy_len[n];
      i386_operand_copy(&iseq_var[new_n][insn_index].op[op_index], var_op);
      imm_vt_map_copy(&st_imm_map[new_n], &out_imm_maps[m]);
      switch (var_op->type) {
        case op_imm:
          valtag_copy(&iseq_var[new_n][insn_index].op[op_index].valtag.imm,
              &out_var_ops[m]);
          break;
        case op_mem:
          valtag_copy(&iseq_var[new_n][insn_index].op[op_index].valtag.mem.disp,
              &out_var_ops[m]);
          break;
        default:
          break;
      }
      new_n++;
      ASSERT(new_n <= max_num_iseq_var);
    }
  }
  for (n = 0; n < num_iseq_var; n++) {
    //free(iseq_var_copy[n]);
    delete [] iseq_var_copy[n];
  }
  return new_n;
}

#define I386_ST_CANON_EXREG(var_op, field, exreg_group, num_used, st_map, fixed_reg_mapping) do { \
  /*ASSERT(var_op->field.tag == tag_const); */\
  (var_op)->field.val = ST_CANON_EXREG(reg_identifier_t(var_op->field.val), exreg_group, num_used, st_map, fixed_reg_mapping).reg_identifier_get_id(); \
  (var_op)->field.tag = tag_var; \
} while(0)


/*#define ST_CANON_REG(field, size) do { \
  if (op->valtag.field.val != -1) { \
    ASSERT(op->valtag.field.tag == tag_const); \
    ASSERT(op->valtag.field.val >= 0 && op->valtag.field.val < I386_NUM_GPRS); \
    unsigned reg = op->valtag.field.val; \
    if (size == 1 && reg >= 4) { \
      reg -= 4; \
    } \
    ASSERT(reg >= 0 && reg < I386_NUM_GPRS); \
    if (st_regmap->table[reg] == -1) { \
      st_regmap->table[reg] = num_gprs_used++; \
      ASSERT(num_gprs_used <= I386_NUM_GPRS); \
    } \
    if (reg != op->valtag.field.val) { \
      op->valtag.field.val = st_regmap->table[reg] + VREG0_HIGH_8BIT; \
      ASSERT(   op->valtag.field.val >= VREG0_HIGH_8BIT \
             && op->valtag.field.val < VREG0_HIGH_8BIT + SRC_NUM_GPRS); \
    } else { \
      op->valtag.field.val = st_regmap->table[reg]; \
      ASSERT(op->valtag.field.val >= 0 && op->valtag.field.val < SRC_NUM_GPRS); \
    } \
    op->valtag.field.tag = tag_var; \
  } \
} while(0)*/

#define MEMVT_CANON_EXREG(var_op, mem, num_exregs_used, st_map) do { \
  if (var_op->mem.segtype == segtype_sel) { \
    I386_ST_CANON_EXREG(var_op, mem.seg.sel, \
        I386_EXREG_GROUP_SEGS, num_exregs_used, st_map, fixed_reg_mapping); \
  } \
  if (var_op->mem.base.val != -1) { \
    I386_ST_CANON_EXREG(var_op, mem.base, \
        I386_EXREG_GROUP_GPRS, num_exregs_used, st_map, fixed_reg_mapping); \
  } \
  if (var_op->mem.index.val != -1) { \
    I386_ST_CANON_EXREG(var_op, mem.index, I386_EXREG_GROUP_GPRS, num_exregs_used, \
        st_map, fixed_reg_mapping); \
  } \
} while(0)

long
i386_operand_strict_canonicalize_regs(i386_strict_canonicalization_state_t *scanon_state,
    long insn_index, long op_index,
    i386_insn_t *iseq_var[], long iseq_var_len[], regmap_t *st_regmap,
    imm_vt_map_t *st_imm_map,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    long num_iseq_var, long max_num_iseq_var)
{
  long i, n, num_exregs_used[I386_NUM_EXREG_GROUPS];
  i386_insn_t *var_insn;
  operand_t *var_op;
  regmap_t *st_map;

  ASSERT(num_iseq_var <= max_num_iseq_var);
  for (n = 0; n < num_iseq_var; n++) {
    for (i = 0; i < I386_NUM_EXREG_GROUPS; i++) {
      num_exregs_used[i] = regmap_count_exgroup_regs(&st_regmap[n], i);
    }

    var_insn = &iseq_var[n][insn_index];
    var_op = &var_insn->op[op_index];
    //i386_operand_copy(var_op, op);
    st_map = &st_regmap[n];
    switch (var_op->type) {
      case op_reg:
        /*if (var_op->valtag.reg.tag == tag_const) */{
          I386_ST_CANON_EXREG(var_op, valtag.reg, I386_EXREG_GROUP_GPRS, num_exregs_used, st_map, fixed_reg_mapping);
        }
        break;
      case op_mem:
        MEMVT_CANON_EXREG(var_op, valtag.mem, num_exregs_used, st_map);
        break;
      case op_seg:
        I386_ST_CANON_EXREG(var_op, valtag.seg, I386_EXREG_GROUP_SEGS, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_mmx:
        I386_ST_CANON_EXREG(var_op, valtag.mmx, I386_EXREG_GROUP_MMX, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_xmm:
        I386_ST_CANON_EXREG(var_op, valtag.xmm, I386_EXREG_GROUP_XMM, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_ymm:
        I386_ST_CANON_EXREG(var_op, valtag.ymm, I386_EXREG_GROUP_YMM, num_exregs_used, st_map, fixed_reg_mapping);
        break;
#define case_st_canon_flag(flagname, groupname) \
      case op_flags_##flagname: \
        I386_ST_CANON_EXREG(var_op, valtag.flags_##flagname, groupname, num_exregs_used, st_map, fixed_reg_mapping); \
        break;

      case_st_canon_flag(other, I386_EXREG_GROUP_EFLAGS_OTHER);
      case_st_canon_flag(eq, I386_EXREG_GROUP_EFLAGS_EQ);
      case_st_canon_flag(ne, I386_EXREG_GROUP_EFLAGS_NE);
      case_st_canon_flag(ult, I386_EXREG_GROUP_EFLAGS_ULT);
      case_st_canon_flag(slt, I386_EXREG_GROUP_EFLAGS_SLT);
      case_st_canon_flag(ugt, I386_EXREG_GROUP_EFLAGS_UGT);
      case_st_canon_flag(sgt, I386_EXREG_GROUP_EFLAGS_SGT);
      case_st_canon_flag(ule, I386_EXREG_GROUP_EFLAGS_ULE);
      case_st_canon_flag(sle, I386_EXREG_GROUP_EFLAGS_SLE);
      case_st_canon_flag(uge, I386_EXREG_GROUP_EFLAGS_UGE);
      case_st_canon_flag(sge, I386_EXREG_GROUP_EFLAGS_SGE);
      //case op_flags_unsigned:
      //  I386_ST_CANON_EXREG(var_op, valtag.flags_unsigned, I386_EXREG_GROUP_EFLAGS_UNSIGNED,
      //      num_exregs_used, st_map, fixed_reg_mapping);
      //  break;
      //case op_flags_signed:
      //  I386_ST_CANON_EXREG(var_op, valtag.flags_signed, I386_EXREG_GROUP_EFLAGS_SIGNED,
      //      num_exregs_used, st_map, fixed_reg_mapping);
      //  break;
      case op_st_top:
        I386_ST_CANON_EXREG(var_op, valtag.st_top, I386_EXREG_GROUP_ST_TOP, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_data_reg:
        I386_ST_CANON_EXREG(var_op, valtag.fp_data_reg, I386_EXREG_GROUP_FP_DATA_REGS, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_control_reg_rm:
        I386_ST_CANON_EXREG(var_op, valtag.fp_control_reg_rm, I386_EXREG_GROUP_FP_CONTROL_REG_RM, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_control_reg_other:
        I386_ST_CANON_EXREG(var_op, valtag.fp_control_reg_other, I386_EXREG_GROUP_FP_CONTROL_REG_OTHER, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_tag_reg:
        I386_ST_CANON_EXREG(var_op, valtag.fp_tag_reg, I386_EXREG_GROUP_FP_TAG_REG, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_last_instr_pointer:
        I386_ST_CANON_EXREG(var_op, valtag.fp_last_instr_pointer, I386_EXREG_GROUP_FP_LAST_INSTR_POINTER, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_last_operand_pointer:
        I386_ST_CANON_EXREG(var_op, valtag.fp_last_operand_pointer, I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_opcode:
        I386_ST_CANON_EXREG(var_op, valtag.fp_opcode, I386_EXREG_GROUP_FP_OPCODE_REG, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_mxcsr_rm:
        I386_ST_CANON_EXREG(var_op, valtag.mxcsr_rm, I386_EXREG_GROUP_MXCSR_RM, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_status_reg_c0:
        I386_ST_CANON_EXREG(var_op, valtag.fp_status_reg_c0, I386_EXREG_GROUP_FP_STATUS_REG_C0, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_status_reg_c1:
        I386_ST_CANON_EXREG(var_op, valtag.fp_status_reg_c1, I386_EXREG_GROUP_FP_STATUS_REG_C1, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_status_reg_c2:
        I386_ST_CANON_EXREG(var_op, valtag.fp_status_reg_c2, I386_EXREG_GROUP_FP_STATUS_REG_C2, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_status_reg_c3:
        I386_ST_CANON_EXREG(var_op, valtag.fp_status_reg_c3, I386_EXREG_GROUP_FP_STATUS_REG_C3, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      case op_fp_status_reg_other:
        I386_ST_CANON_EXREG(var_op, valtag.fp_status_reg_other, I386_EXREG_GROUP_FP_STATUS_REG_OTHER, num_exregs_used, st_map, fixed_reg_mapping);
        break;
      default:
        break;
    }
    /*if (op_index == I386_MAX_NUM_OPERANDS - 1) {
      int i;
      for (i = 0; i < var_insn->memtouch.n_elem; i++) {
        MEMVT_CANON_EXREG(var_insn, memtouch.ls[i], num_exregs_used, st_map);
      }
    }*/
  }
  return num_iseq_var;
}


/*long
i386_memtouch_strict_canonicalize(long insn_index, long op_index,
    struct i386_insn_t *iseq_var[], long iseq_var_len[], struct memlabel_map_t *st_memlabel_map,
    long num_iseq_var, long max_num_iseq_var)
{
  return num_iseq_var; //do nothing
}*/


static void
i386_operand_hash_canonicalize(operand_t *op, struct regmap_t *hash_regmap,
    struct imm_vt_map_t *hash_imm_map)
{
  /* XXX : related to hash_func */
}

void
i386_insn_hash_canonicalize(i386_insn_t *insn, struct regmap_t *hash_regmap,
    struct imm_vt_map_t *hash_imm_map)
{
  int i;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    i386_operand_hash_canonicalize(&insn->op[i], hash_regmap, hash_imm_map);
  }
}

static unsigned long
tag2int(src_tag_t tag)
{
  return (unsigned long)tag;
}

static unsigned long
i386_operand_hash_func(operand_t const *op)
{
  switch (op->type) {
    case invalid:
      return 0;
    case op_reg:
      return 13 /* (op->val.reg << (op->size + op->rex_used))*/;
    case op_seg:
      return 17 /* (op->val.seg) */;
    case op_mem:
      return 23 * (memvt_hash_func(&op->valtag.mem, op->size));
    case op_imm:
      return 29 /* * op->val.imm*/;
    case op_pcrel:
      return 37 /* op->val.pcrel*/;
    case op_cr:
      return 41 * op->valtag.cr.val;
    case op_db:
      return 47 * op->valtag.db.val;
    case op_tr:
      return 53 * op->valtag.tr.val;
    case op_mmx:
      return 59;
    case op_xmm:
      return 67;
    case op_ymm:
      return 101;
    case op_d3now:
      return 71 * op->valtag.d3now.val;
    case op_prefix:
      return 79 * op->valtag.prefix.val;
    case op_st:
      return 83;
    //case op_flags_unsigned:
    //  return 89;
    //case op_flags_signed:
    //  return 91;
    case op_flags_other:
    case op_flags_eq:
    case op_flags_ne:
    case op_flags_ult:
    case op_flags_slt:
    case op_flags_ugt:
    case op_flags_sgt:
    case op_flags_ule:
    case op_flags_sle:
    case op_flags_uge:
    case op_flags_sge:
      return 89 + (op->type - op_flags_other);
    case op_st_top:
      return 97;
    case op_fp_data_reg:
      return 101;
    case op_fp_control_reg_rm:
      return 103;
    case op_fp_control_reg_other:
      return 137;
    case op_fp_tag_reg:
      return 109;
    case op_fp_last_instr_pointer:
      return 113;
    case op_fp_last_operand_pointer:
      return 127;
    case op_fp_opcode:
      return 131;
    case op_mxcsr_rm:
      return 133;
    case op_fp_status_reg_c0:
      return 137;
    case op_fp_status_reg_c1:
      return 139;
    case op_fp_status_reg_c2:
      return 143;
    case op_fp_status_reg_c3:
      return 149;
    case op_fp_status_reg_other:
      return 151;
    default:
      printf("Invalid op->type[%d]\n", op->type);
  }
  NOT_REACHED();
}

unsigned long
i386_insn_hash_func(i386_insn_t const *insn)
{
  unsigned long ret;
  int i;
  ret = insn->opc * 95143;
  DBG2(I386_INSN, "ret = %ld\n", ret);
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    ret += (i386_operand_hash_func(&insn->op[i]) << i);
    DBG2(I386_INSN, "i = %d, ret = %ld\n", i, ret);
  }
  return ret;
}


int
i386_insn_disassemble(i386_insn_t *insn, uint8_t const *bincode,
    i386_as_mode_t i386_as_mode)
{
  int size;
  DYN_DBG(i386_disas, "bincode = %hhx,%hhx,%hhx,%hhx\n", bincode[0], bincode[1], bincode[2], bincode[3]);
  size = disas_insn_i386(bincode, (unsigned long)bincode, insn, i386_as_mode);
  //ASSERT(insn->memtouch.n_elem == 0);
  //DBG(MD_ASSEMBLE, "insn.op[0].size = %d (%d), insn.op[1].size = %d (%d)\n", (int)insn->op[0].size, (int)insn->op[0].valtag.imm.val, insn->op[1].size, (int)insn->op[1].valtag.imm.val);
  if (insn->opc == -1) {
    return 0;
  }
  DYN_DBG(i386_disas, "insn->opc = %s.\n", opctable_name(insn->opc));
  fixed_reg_mapping_t const &dst_fixed_reg_mapping_reserved_identity = fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity();
  //cout << __func__ << " " << __LINE__ << ": dst_reserved_regs =\n" << regset_to_string(&dst_reserved_regs, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": fixed_reg_mapping =\n" << dst_fixed_reg_mapping_reserved_identity.fixed_reg_mapping_to_string() << endl;

  if (size) {
    i386_insn_add_implicit_operands(insn, dst_fixed_reg_mapping_reserved_identity);
    DBG(REFRESH_DB, "insn:\n%s\n",
          i386_insn_to_string(insn, as1, sizeof as1));
  }
  //i386_insn_canonicalize(insn);//not needed so far
  //DBG(TMP, "insn: %s\n", i386_insn_to_string(insn, as1, sizeof as1));
  return size;
}

void
i386_insn_canonicalize(i386_insn_t *insn)
{
  char const *opc = opctable_name(insn->opc);
  if (strstart(opc, "xchg", NULL)) {
    if (   insn->op[0].type == op_reg
        && insn->op[1].type == op_reg
        && insn->op[0].valtag.reg.tag == insn->op[1].valtag.reg.tag
        && insn->op[0].valtag.reg.val < insn->op[1].valtag.reg.val) {
      /* i386 assembler seems to reorder operands for shortest assembly.
         To avoid inconsistencies, let's keep xchg instructions in a canonical
         order. */
      int tmp;
      tmp = insn->op[0].valtag.reg.val;
      insn->op[0].valtag.reg.val = insn->op[1].valtag.reg.val;
      insn->op[1].valtag.reg.val = tmp;
    }
    //following code is okay but not needed till now
    /*if (   insn->op[0].type == op_reg
        && insn->op[1].type == op_mem) {
      // Let's keep xchg reg-mem instructions in a canonical order too
      operand_t tmp;
      operand_copy(&tmp, &insn->op[0]);
      operand_copy(&insn->op[0], &insn->op[1]);
      operand_copy(&insn->op[1], &tmp);
    }*/
  }
}

/*#define MATCH_REG_FIELD(fld, map, size) do { \
  ASSERT(op->valtag.fld.tag == tag_const); \
  if (_template->valtag.fld.tag == tag_const) { \
    if (_template->valtag.fld.val != op->valtag.fld.val) { \
      DBG2(PEEP_TAB, "Returning false because " #fld " tag_const mismatch: " \
          "_template->tag." #fld "=%ld, op->val." #fld "=%ld\n", \
          (long)_template->valtag.fld.tag, (long)op->valtag.fld.val); \
      return false; \
    } \
  } else { \
    unsigned template_reg = _template->valtag.fld.val; \
    unsigned op_reg = op->valtag.fld.val; \
    if (template_reg >= VREG0_HIGH_8BIT) { \
      template_reg -= VREG0_HIGH_8BIT; \
    } \
    if (size == 1 && op_reg >= 4) { \
      op_reg -= 4; \
    } \
    ASSERT(template_reg >= 0 && template_reg < SRC_NUM_GPRS); \
    if (map->table[template_reg] != -1) { \
      if (map->table[template_reg] != op_reg) { \
        DBG2(PEEP_TAB, "Returning false because " #fld " tag_var mismatch: " \
            "template_reg=%ld, op_reg=%ld, map->table[template_reg]=%ld\n", \
            (long)template_reg, (long)op_reg, (long)map->table[template_reg]); \
        return false; \
      } \
    } else { \
      map->table[template_reg] = op_reg; \
    } \
  } \
} while(0)*/

#define MATCH_EXREG_FIELD(map, field, exreg_group) do { \
  /* ASSERT(op->field.tag == tag_const); */ \
  DBG(I386_MATCH, "%s", "match_exreg_field: matching tag\n"); \
  if (_template->field.tag == tag_const) { \
    if (_template->field.val != op->field.val) { \
      DBG2(I386_MATCH, "Returning false because of " #field " mismatch same tags. " \
        "_template->val.field = %ld, op->val.field = %ld\n", \
        (long)_template->field.val, (long)op->field.val); \
      return false; \
    } \
  } else { \
    unsigned template_reg = _template->field.val; \
    unsigned op_reg = op->field.val; \
    ASSERT(template_reg >= 0 && template_reg < MAX_NUM_EXREGS(exreg_group)); \
    DBG2(I386_MATCH, "map->regmap_extable[%d][%d] = %d\n", exreg_group, template_reg, \
        regmap_get_elem(map, exreg_group, template_reg).reg_identifier_get_id()); \
    if (!regmap_get_elem(map, exreg_group, template_reg).reg_identifier_is_unassigned()) { \
      if (regmap_get_elem(map, exreg_group, template_reg).reg_identifier_get_id() != op_reg) { \
        DBG2(I386_MATCH, "Returning false because of " #field " mismatch tag_var. " \
          "template_reg = %ld, op_reg = %ld, " \
          "map->regmap_extable[group %ld][template_reg]=%ld\n", \
          (long)template_reg, (long)op_reg, (long)exreg_group, \
          (long)regmap_get_elem(map, exreg_group, template_reg).reg_identifier_get_id()); \
        return false; \
      } \
    } else { \
      regmap_set_elem(map, exreg_group, template_reg, op_reg); \
    } \
  } \
  DBG(I386_MATCH, "%s", "MATCH_EXREG_FIELD, checking reg.mask\n"); \
  if (_template->field.mod.reg.mask != op->field.mod.reg.mask) { \
    DBG(I386_MATCH, "Returning false because of " #field ".mod.reg.mask mismatch. " \
      "_template's mask = 0x%lx, op's mask= 0x%lx\n", \
      (long)_template->field.mod.reg.mask, (long)op->field.mod.reg.mask); \
    return false; \
  } \
  DBG(I386_MATCH, "%s", "MATCH_EXREG_FIELD, reached end\n"); \
} while(0)


static bool
i386_operand_matches_template(operand_t const *_template, operand_t const *op,
    src_iseq_match_renamings_t &src_iseq_match_renamings,
    //regmap_t *st_regmap, imm_vt_map_t *st_imm_map,
    //nextpc_map_t *nextpc_map,
    bool var)
{
  regmap_t st_regmap = src_iseq_match_renamings.get_regmap();
  imm_vt_map_t st_imm_map = src_iseq_match_renamings.get_imm_vt_map();
  nextpc_map_t nextpc_map = src_iseq_match_renamings.get_nextpc_map();

  if (_template->type != op->type) {
    DBG2(I386_MATCH, "types mismatch %d<->%d. returning false\n",
        _template->type, op->type);
    return false;
  }
  if (_template->type != op_imm && _template->size != op->size) {
    DBG2(I386_MATCH, "_template->type = %d, size mismatch %d<->%d. returning false\n",
        _template->type, _template->size, op->size);
    return false;
  }
  switch (_template->type) {
    case invalid:
      break;
    case op_reg:
      DBG2(I386_MATCH, "comparing reg field. op->val.reg=%ld [tag %d], "
          "_template->val.reg=%ld [tag %d], "
          "size=%d, st_regmap:\n%s\n", op->valtag.reg.val, op->valtag.reg.tag,
          _template->valtag.reg.val, _template->valtag.reg.tag, op->size,
          regmap_to_string(&st_regmap, as1, sizeof as1));
      MATCH_EXREG_FIELD((&st_regmap), valtag.reg, I386_EXREG_GROUP_GPRS);
      break;
    case op_seg:
      MATCH_EXREG_FIELD((&st_regmap), valtag.seg, I386_EXREG_GROUP_SEGS);
      break;
    case op_mmx:
      MATCH_EXREG_FIELD((&st_regmap), valtag.mmx, I386_EXREG_GROUP_MMX);
      break;
    case op_xmm:
      MATCH_EXREG_FIELD((&st_regmap), valtag.xmm, I386_EXREG_GROUP_XMM);
      break;
    case op_ymm:
      MATCH_EXREG_FIELD((&st_regmap), valtag.ymm, I386_EXREG_GROUP_YMM);
      break;
    case op_mem:
      MATCH_MEMVT_FIELD((&st_regmap), (&st_imm_map), mem);
      //XXX: need to also match memlabels using memlabel_map, when all memory accesses are annotated with memlabels
      break;
    case op_imm:
      //if (var) {
        MATCH_IMM_VT_FIELD(op->valtag.imm, _template->valtag.imm, (&st_imm_map));
      //} else {
        //MATCH_IMM_FIELD(op, _template, imm, st_imm_map);
      //}
      break;
    case op_pcrel:
      DBG2(I386_MATCH, "%s", "matching nextpc field.\n");
      DBG2(I386_MATCH, "ts->valtag.pcrel.tag = %d\n", _template->valtag.pcrel.tag);
      DBG2(I386_MATCH, "ts->valtag.pcrel.val = %ld\n", _template->valtag.pcrel.val);
      MATCH_NEXTPC_FIELD(op, _template, pcrel, (&nextpc_map), var);
      /*if (op->valtag.pcrel.tag == tag_const) {
        if (   op->valtag.pcrel.val != _template->valtag.pcrel.val
            && _template->valtag.pcrel.tag == tag_const) {
          return false;
        }
      } else {
        if (_template->valtag.pcrel.tag == tag_const) {
          return false;
        }
        if (var) {
          src_tag_t nextpc_tag;
          int nextpc_val;

          ASSERT(_template->valtag.pcrel.val >= 0);
          ASSERT(_template->valtag.pcrel.val < MAX_NUM_NEXTPCS);
          nextpc_tag = nextpc_map->table[_template->valtag.pcrel.val].tag;
          nextpc_val = nextpc_map->table[_template->valtag.pcrel.val].val;

          ASSERT(nextpc_tag == tag_const || nextpc_tag == tag_var);
          ASSERT(   nextpc_tag != tag_var
                 || (nextpc_val >= 0 && nextpc_val < MAX_NUM_NEXTPCS));
          ASSERT(   nextpc_tag != tag_const
                 || (nextpc_val >= 0 && nextpc_val < MAX_INSN_ID_CONSTANTS));

          if (nextpc_map->table[_template->valtag.pcrel.val].tag == tag_invalid) {
            nextpc_map->table[_template->valtag.pcrel.val].tag = op->valtag.pcrel.tag;
            nextpc_map->table[_template->valtag.pcrel.val].val = op->valtag.pcrel.val;
          }
          if (   nextpc_map->table[_template->valtag.pcrel.val].tag !=
                     op->valtag.pcrel.tag
              || nextpc_map->table[_template->valtag.pcrel.val].val !=
                     op->valtag.pcrel.val) {
            return false;
          }
        } else {
          if (_template->valtag.pcrel.val != op->valtag.pcrel.val) {
            return false;
          }
        }
      }*/
      break;
    case op_st:
      DBG2(I386_MATCH, "comparing reg field. op->val.st=%ld [tag %d], "
          "_template->val.st=%ld [tag %d], "
          "size=%d, st_regmap:\n%s\n", op->valtag.st.val, op->valtag.st.tag,
          _template->valtag.st.val, _template->valtag.st.tag, op->size,
          regmap_to_string((&st_regmap), as1, sizeof as1));
      ASSERT(_template->valtag.st.tag == tag_const);
      if (_template->valtag.st.val != op->valtag.st.val) {
        return false;
      }
      //MATCH_EXREG_FIELD(st_regmap, st, I386_EXREG_GROUP_ST);
      break;
#define case_flag_match(flagname, groupname) \
    case op_flags_##flagname: \
      regmap_set_elem(&st_regmap, groupname, 0, 0); \
      break;

    case_flag_match(other, I386_EXREG_GROUP_EFLAGS_OTHER);
    case_flag_match(eq, I386_EXREG_GROUP_EFLAGS_EQ);
    case_flag_match(ne, I386_EXREG_GROUP_EFLAGS_NE);
    case_flag_match(ult, I386_EXREG_GROUP_EFLAGS_ULT);
    case_flag_match(slt, I386_EXREG_GROUP_EFLAGS_SLT);
    case_flag_match(ugt, I386_EXREG_GROUP_EFLAGS_UGT);
    case_flag_match(sgt, I386_EXREG_GROUP_EFLAGS_SGT);
    case_flag_match(ule, I386_EXREG_GROUP_EFLAGS_ULE);
    case_flag_match(sle, I386_EXREG_GROUP_EFLAGS_SLE);
    case_flag_match(uge, I386_EXREG_GROUP_EFLAGS_UGE);
    case_flag_match(sge, I386_EXREG_GROUP_EFLAGS_SGE);
    //case op_flags_unsigned:
    //  regmap_set_elem(st_regmap, I386_EXREG_GROUP_EFLAGS_UNSIGNED, 0, 0);
    //  return true;
    //case op_flags_signed:
    //  regmap_set_elem(st_regmap, I386_EXREG_GROUP_EFLAGS_SIGNED, 0, 0);
    //  return true;
    case op_st_top:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_ST_TOP, 0, 0);
      break;
    case op_fp_data_reg:
      MATCH_EXREG_FIELD((&st_regmap), valtag.fp_data_reg, I386_EXREG_GROUP_FP_DATA_REGS);
      break;
    case op_fp_control_reg_rm:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_CONTROL_REG_RM, 0, 0);
      break;
    case op_fp_control_reg_other:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_CONTROL_REG_OTHER, 0, 0);
      break;
    case op_fp_tag_reg:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_TAG_REG, 0, 0);
      break;
    case op_fp_last_instr_pointer:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_LAST_INSTR_POINTER, 0, 0);
      break;
    case op_fp_last_operand_pointer:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER, 0, 0);
      break;
    case op_fp_opcode:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_OPCODE_REG, 0, 0);
      break;
    case op_mxcsr_rm:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_MXCSR_RM, 0, 0);
      break;
    case op_fp_status_reg_c0:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_STATUS_REG_C0, 0, 0);
      break;
    case op_fp_status_reg_c1:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_STATUS_REG_C1, 0, 0);
      break;
    case op_fp_status_reg_c2:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_STATUS_REG_C2, 0, 0);
      break;
    case op_fp_status_reg_c3:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_STATUS_REG_C3, 0, 0);
      break;
    case op_fp_status_reg_other:
      regmap_set_elem(&st_regmap, I386_EXREG_GROUP_FP_STATUS_REG_OTHER, 0, 0);
      break;
    case op_prefix:
      if (_template->valtag.st.val != op->valtag.st.val) {
        return false;
      }
      break;
    default:
      break;
  }
  src_iseq_match_renamings.update(st_regmap, st_imm_map, nextpc_map);
  return true;
}

static bool
i386_insn_matches_template(i386_insn_t const *_template, i386_insn_t const *insn,
    src_iseq_match_renamings_t &src_iseq_match_renamings,
    //regmap_t *st_regmap,
    //imm_vt_map_t *st_imm_vt_map,
    //nextpc_map_t *nextpc_map,
    bool var)
{
  char const *opct, *opc;
  int i;

  opct = opctable_name(_template->opc);
  opc = opctable_name(insn->opc);
  if (i386_insn_is_nop(_template) && i386_insn_is_nop(insn)) {
    return true;
  }
  if (strcmp(opct, opc)) {
    DBG2(I386_MATCH, "%s() returning false because opcodes mismatch (%s<->%s)\n", __func__,
        opct, opc);
    return false;
  }
  DBG(I386_MATCH, "comparing _template: %s\nwith:\n%s\n",
      i386_insn_to_string(_template, as1, sizeof as1),
      i386_insn_to_string(insn, as2, sizeof as2));
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    DBG2(I386_MATCH, "comparing operand %d\n", i);
    if (!i386_operand_matches_template(&_template->op[i], &insn->op[i],
          src_iseq_match_renamings,
          //st_regmap,
          //st_imm_vt_map, nextpc_map,
          var)) {
      DBG(I386_MATCH, "returning false because operands[%d] mismatch "
          "(types %d<->%d, sizes %d<->%d)\n", i, _template->op[i].type,
          insn->op[i].type, _template->op[i].size, insn->op[i].size);
      return false;
    }
  }
  if (i386_insn_is_indirect_branch(insn)) {
    nextpc_map_t nextpc_map = src_iseq_match_renamings.get_nextpc_map();
    nextpc_map_add_indir_nextpc_if_not_present(&nextpc_map);
    src_iseq_match_renamings.set_nextpc_map(nextpc_map);
  }
  DBG(I386_MATCH, "%s", "insn match succeeded\n");
  /*if (_template->memtouch.n_elem != insn->memtouch.n_elem) {
    DBG(insn_match, "returning false because memtouch.n_elem mismatch %d <-> %d\n",
        _template->memtouch.n_elem, insn->memtouch.n_elem);
    return false;
  }
  DBG(insn_match, "%s", "I am here\n");
  DBG(insn_match, "memtouch.n_elem %d\n", (int)_template->memtouch.n_elem);
  for (i = 0; i < _template->memtouch.n_elem; i++) {
    i386_insn_t const *op = insn; //because MATCH_MEMVT_FIELD assumes 'op'
    DBG(insn_match, "matching memtouch.ls[%d]\n", (int)i);
    MATCH_MEMVT_FIELD(st_regmap, st_imm_vt_map, memtouch.ls[i]);
    if (   memlabel_map
        && !match_memlabel_field(memlabel_map, &_template->memtouch.mls[i], &insn->memtouch.mls[i])) {
      return false;
    }
  }*/
  return true;
}

vector<src_iseq_match_renamings_t>
i386_iseq_matches_template(i386_insn_t const *_template, i386_insn_t const *seq,
    long len,
    //src_iseq_match_renamings_t &src_iseq_match_renamings,
    //regmap_t *st_regmap, imm_vt_map_t *st_imm_map, nextpc_map_t *nextpc_map,
    string prefix)
{
  long i;

  src_iseq_match_renamings_t src_iseq_match_renamings;
  src_iseq_match_renamings.clear();
  //regmap_init(st_regmap);
  //imm_vt_map_init(st_imm_map);
  //nextpc_map_init(nextpc_map);
  //memlabel_map_init(st_memlabel_map);
  DBG2(I386_MATCH, "comparing _template:\n%s\niseq:\n%s\n",
      i386_iseq_to_string(_template, len, as1, sizeof as1),
      i386_iseq_to_string(seq, len, as2, sizeof as2));

  for (i = 0; i < len; i++) {
    if (!i386_insn_matches_template(&_template[i], &seq[i], src_iseq_match_renamings,/*st_regmap, st_imm_map,
            nextpc_map, */false)) {
      DBG2(I386_MATCH, "failed on insn %ld\n", i);
      return vector<src_iseq_match_renamings_t>();
    }
  }
  return vector<src_iseq_match_renamings_t>({src_iseq_match_renamings});
}

bool
i386_iseq_matches_template_var(i386_insn_t const *_template, i386_insn_t const *iseq,
    long len,
    src_iseq_match_renamings_t &src_iseq_match_renamings,
    //regmap_t *st_regmap, imm_vt_map_t *st_imm_vt_map, nextpc_map_t *nextpc_map,
    char const *prefix)
{
  src_iseq_match_renamings.clear();
  //regmap_init(st_regmap);
  //imm_vt_map_init(st_imm_vt_map);
  //nextpc_map_init(nextpc_map);

  DYN_DBG2(insn_match, "comparing _template:\n%s\niseq:\n%s\n",
      i386_iseq_to_string(_template, len, as1, sizeof as1),
      i386_iseq_to_string(iseq, len, as2, sizeof as2));

  for (long i = 0; i < len; i++) {
    if (!i386_insn_matches_template(&_template[i], &iseq[i],
            src_iseq_match_renamings,
            //st_regmap,
            //st_imm_vt_map, nextpc_map/*, memlabel_map*/,
            true)) {
      DYN_DBG2(insn_match, "failed on insn %ld\n", i);
      return false;
    }
  }
  return true;
}

bool
i386_insn_has_rep_prefix(i386_insn_t const *insn)
{
  int i;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_prefix) {
      ASSERT(insn->op[i].valtag.prefix.tag == tag_const);
      if (   insn->op[i].valtag.prefix.val == I386_PREFIX_REPZ
          || insn->op[i].valtag.prefix.val == I386_PREFIX_REPNZ) {
        return true;
      }
    }
  }
  return false;
}

bool
i386_insn_is_jecxz(i386_insn_t const *insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);

  if (!strcmp(opc, "jcxz") || !strcmp(opc, "jecxz")) {
  //if (insn->opc == opc_jcxz)
    return true;
  }
  return false;
}

static bool
i386_insn_is_comparison(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  if (strstart(opc, "test", NULL) || strstart(opc, "cmp", NULL)) {
    return true;
  }
  return false;
}

static bool
is_xor_to_zero(i386_insn_t const *insn)
{
  char const *opc;
  int ni;

  opc = opctable_name(insn->opc);
  ni = insn->num_implicit_ops;
  if (   strstart(opc, "xor", NULL)
      && insn->op[ni].type == op_reg
      && insn->op[ni + 1].type == op_reg
      && insn->op[ni + 1].valtag.reg.tag == insn->op[ni].valtag.reg.tag
      && insn->op[ni + 1].valtag.reg.val == insn->op[ni].valtag.reg.val
      && insn->op[ni + 1].valtag.reg.mod.reg.mask
             == insn->op[ni].valtag.reg.mod.reg.mask) {
    return true;
  }
  return false;
}

//static bool
//writes_operand(i386_insn_t const *insn, int n)
//{
//  char const *opc;
//  opc = opctable_name(insn->opc);
//
//  if (strstart(opc, "int", NULL)) {
//    return false;
//  }
//  if (n < insn->num_implicit_ops) {
//    if (   insn->op[n].type == op_flags_other
//        || insn->op[n].type == op_flags_eq
//        || insn->op[n].type == op_flags_ne
//        || insn->op[n].type == op_flags_ult
//        || insn->op[n].type == op_flags_slt
//        || insn->op[n].type == op_flags_ugt
//        || insn->op[n].type == op_flags_sgt
//        || insn->op[n].type == op_flags_ule
//        || insn->op[n].type == op_flags_sle
//        || insn->op[n].type == op_flags_uge
//        || insn->op[n].type == op_flags_sge
//       ) {
//      return false; //flags are handled separately.
//    }
//    if (i386_insn_accesses_stack(insn)) {
//      return true; //XXX
//    } else if (i386_insn_is_jecxz(insn)) {
//      ASSERT(n == 0);
//      return false;
//    } else if (i386_insn_is_intmul(insn) && !strstart(opc, "mulx", NULL)) {
//      return true;
//    } else if (i386_insn_is_intdiv(insn)) {
//      return true;
//    } else if (i386_insn_is_cmpxchg(insn)) {
//      ASSERT(insn->num_implicit_ops == 3);
//      ASSERT(n == i386_insn_count_num_implicit_flag_regs(insn));
//      //ASSERT(insn->op[0].type == op_flags_unsigned);
//      //ASSERT(insn->op[1].type == op_flags_signed);
//      //ASSERT(n == 2);
//      return true;
//    } else if (!strcmp(opc, "lahf")) {
//      return true;
//    } else if (!strcmp(opc, "sahf")) {
//      return false;
//    } else if (   !strcmp(opc, "aaa")
//               || !strcmp(opc, "aas")
//               || !strcmp(opc, "aad")
//               || !strcmp(opc, "aam")
//               || !strcmp(opc, "daa")
//               || !strcmp(opc, "das")) {
//      return true;
//    } else if (   !strcmp(opc, "rdtsc")
//               || !strcmp(opc, "rdpmc")
//               || !strcmp(opc, "cpuid")) {
//      return true;
//    } else if (   !strcmp(opc, "cbtw")
//               || !strcmp(opc, "cwtl")) {
//      return true;
//    } else if (   !strcmp(opc, "cwtd")
//               || !strcmp(opc, "cltd")) {
//      if (n == 0) {
//        return true;
//      } else {
//        return false;
//      }
//    } else if (i386_insn_accesses_st_top(insn)) {
//      return true;
//    } else if (i386_is_string_opcode(opc)) {
//      return false;
//    } else NOT_REACHED();
//  }
//
//  n -= insn->num_implicit_ops;
//  if (n == 0) {
//    if (i386_insn_is_indirect_branch_not_fcall(insn)) {
//      return false;
//    }
//    if (i386_insn_is_comparison(insn)) {
//      return false;
//    }
//    if (i386_insn_is_nop(insn)) {
//      return false;
//    }
//    /* if (i386_insn_is_cmov(insn)) {
//      return false;
//    } */
//    if (strstart(opc, "push", NULL)) {
//      return false;
//    }
//    return true;
//  }
//  if (is_xor_to_zero(insn)) {
//    return true;
//  }
//  if (strstart(opc, "xchg", NULL)) {
//    return true;
//  }
//  return false;
//}

//static bool
//reads_operand(i386_insn_t const *insn, int n)
//{
//  char const *opc;
//  opc = opctable_name(insn->opc);
//
//  if (strstart(opc, "int", NULL)) {
//    return true;
//  }
//
//  if (n < insn->num_implicit_ops) {
//    if (   insn->op[n].type == op_flags_other
//        || insn->op[n].type == op_flags_eq
//        || insn->op[n].type == op_flags_ne
//        || insn->op[n].type == op_flags_ult
//        || insn->op[n].type == op_flags_slt
//        || insn->op[n].type == op_flags_ugt
//        || insn->op[n].type == op_flags_sgt
//        || insn->op[n].type == op_flags_ule
//        || insn->op[n].type == op_flags_sle
//        || insn->op[n].type == op_flags_uge
//        || insn->op[n].type == op_flags_sge
//       ) {
//      return false; //flags are handled separately.
//    }
//    if (i386_insn_accesses_stack(insn)) {
//      return true;
//    } else if (i386_insn_is_jecxz(insn)) {
//      ASSERT(n == 0);
//      return true;
//    } else if (i386_insn_is_intmul(insn)) {
//      ASSERT(insn->num_implicit_ops == 4);
//      //ASSERT(insn->op[0].type == op_flags_unsigned);
//      //ASSERT(insn->op[1].type == op_flags_signed);
//      size_t num_flags = i386_insn_count_num_implicit_flag_regs(insn);
//      if (n == num_flags) {
//        return false;
//      } else {
//        return true;
//      }
//    } else if (i386_insn_is_intdiv(insn)) {
//      return true;
//    } else if (i386_insn_is_cmpxchg(insn)) {
//      //ASSERT(insn->op[0].type == op_flags_unsigned);
//      //ASSERT(insn->op[1].type == op_flags_signed);
//      size_t num_flags = i386_insn_count_num_implicit_flag_regs(insn);
//      ASSERT(insn->num_implicit_ops == num_flags + 1);
//      ASSERT(n == num_flags);
//      return true;
//    } else if (!strcmp(opc, "lahf")) {
//      return false;
//    } else if (!strcmp(opc, "sahf")) {
//      return true;
//    } else if (   !strcmp(opc, "aaa")
//               || !strcmp(opc, "aas")
//               || !strcmp(opc, "aad")
//               || !strcmp(opc, "aam")
//               || !strcmp(opc, "daa")
//               || !strcmp(opc, "das")) {
//      return true;
//    } else if (   !strcmp(opc, "rdtsc")
//               || !strcmp(opc, "rdpmc")
//               || !strcmp(opc, "cpuid")) {
//      return false;
//    } else if (   !strcmp(opc, "cbtw")
//               || !strcmp(opc, "cwtl")) {
//      return true;
//    } else if (   !strcmp(opc, "cwtd")
//               || !strcmp(opc, "cltd")) {
//      if (n == 0) {
//        return false;
//      } else {
//        return true;
//      }
//    } else if (i386_insn_accesses_st_top(insn)) {
//      return true;
//    } else if (i386_is_string_opcode(opc)) {
//      return true;
//    } else NOT_REACHED();
//  }
//
//  n -= insn->num_implicit_ops;
//  if (n == 0) {
//    if (i386_insn_is_indirect_branch(insn)) {
//      return true;
//    }
//    if (strstr(opc, "mov")) {
//      return false;
//    }
//    if (strstart(opc, "set", NULL)) {
//      return false;
//    }
//    if (strstart(opc, "lea", NULL)) {
//      return false;
//    }
//    if (i386_insn_is_nop(insn)) {
//      return false;
//    }
//    if (is_xor_to_zero(insn)) {
//      return false;
//    }
//    if (   strstart(opc, "imul", NULL)
//        && insn->op[insn->num_implicit_ops + 2].type == op_imm) {
//      return false;
//    }
//    if (strstart(opc, "pop", NULL)) {
//      return false;
//    }
//    return true;
//  }
//  if (is_xor_to_zero(insn)) {
//    return false;
//  }
//
//  return true;
//}

/*static void
regset_mark_all_i386(struct regset_t *set)
{
  regset_mark_tag(set, tag_const, R_EAX);
  regset_mark_tag(set, tag_const, R_ECX);
  regset_mark_tag(set, tag_const, R_EDX);
  regset_mark_tag(set, tag_const, R_EBX);
  regset_mark_tag(set, tag_const, R_ESP);
  regset_mark_tag(set, tag_const, R_EBP);
  regset_mark_tag(set, tag_const, R_ESI);
  regset_mark_tag(set, tag_const, R_EDI);
}*/

//#define I386_INSN_GET_USEDEF_EX(field, group) do { \
//  if (writes_operand(insn, i)) { \
//    regset_mark_ex_mask(def, group, op->valtag.field.val, \
//        op->valtag.field.mod.reg.mask?op->valtag.field.mod.reg.mask: \
//            MAKE_MASK(I386_EXREG_LEN(group))); \
//  } \
//  if (reads_operand(insn, i)) { \
//    regset_mark_ex_mask(use, group, op->valtag.field.val, \
//        op->valtag.field.mod.reg.mask?op->valtag.field.mod.reg.mask: \
//            MAKE_MASK(I386_EXREG_LEN(group))); \
//  } \
//} while (0)

/*
void
i386_insn_get_usedef(struct i386_insn_t const *insn, struct regset_t *use,
    struct regset_t *def, struct memset_t *memuse, struct memset_t *memdef)
{
  char const *opc;
  int i;

  opc = opctable_name(insn->opc);
  regset_clear(use);
  regset_clear(def);
  memset_clear(memuse);
  memset_clear(memdef);

  if (i386_insn_is_nop(insn)) {
    return;
  }
  i386_insn_get_usedef_flags_unsigned(insn, use, def); //need to use this because
  i386_insn_get_usedef_flags_signed(insn, use, def);   //it also distinguishes use
                                                       //and def flags
  DBG2(PEEP_TAB, "use = %s, def = %s\n", regset_to_string(use, as1, sizeof as1),
      regset_to_string(def, as2, sizeof as2));

  //i386_insn_get_usedef_st_top(insn, use, def);

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    operand_t const *op = &insn->op[i];
    if (op->type == op_reg) {
      I386_INSN_GET_USEDEF_EX(reg, I386_EXREG_GROUP_GPRS);
    } else if (op->type == op_mmx) {
      I386_INSN_GET_USEDEF_EX(st, I386_EXREG_GROUP_MMX);
    } else if (op->type == op_xmm) {
      I386_INSN_GET_USEDEF_EX(st, I386_EXREG_GROUP_XMM);
    } else if (op->type == op_st_top) {
      regset_mark_ex_mask(use, I386_EXREG_GROUP_ST_TOP, 0,
          MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_ST_TOP)));
      regset_mark_ex_mask(def, I386_EXREG_GROUP_ST_TOP, 0,
          MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_ST_TOP)));
    } else if (op->type == op_seg) {
      I386_INSN_GET_USEDEF_EX(seg, I386_EXREG_GROUP_SEGS);
    } else if (op->type == op_mem) {
      memset_entry_t *m_entry;
      //ASSERT(op->tag.mem.all == tag_const);
      // fill memsets for only tag_const memory accesses
      if (i == 0) {
        m_entry = &memdef->memaccess[memdef->num_memaccesses];
        memdef->num_memaccesses++;
      } else {
        m_entry = &memuse->memaccess[memuse->num_memaccesses];
        memuse->num_memaccesses++;
      }
      m_entry->reg1 = m_entry->reg2 = -1;
      if (op->valtag.mem.base.val != -1) {
        regset_mark_ex_mask(use, I386_EXREG_GROUP_GPRS,
            op->valtag.mem.base.val, op->valtag.mem.base.mod.reg.mask);
        m_entry->reg1 = op->valtag.mem.base.val;
        m_entry->treg1 = (op->valtag.mem.base.tag == tag_var)?true:false;
      }
      if (op->valtag.mem.index.val != -1) {
        regset_mark_ex_mask(use, I386_EXREG_GROUP_GPRS,
            op->valtag.mem.index.val, op->valtag.mem.index.mod.reg.mask);
        if (m_entry->reg1 != -1) {
          m_entry->reg2 = op->valtag.mem.index.val;
          m_entry->treg2 = (op->valtag.mem.index.tag == tag_var)?true:false;
        } else {
          m_entry->reg1 = op->valtag.mem.index.val;
          m_entry->treg1 = (op->valtag.mem.index.tag == tag_var)?true:false;
        }
      }
      m_entry->scale = op->valtag.mem.scale;
      m_entry->disp = op->valtag.mem.disp.val;
      m_entry->tdisp = (op->valtag.mem.disp.tag == tag_var)?true:false;

    }
  }
  DBG3(I386_INSN, "insn: %s\n", i386_insn_to_string(insn, as1, sizeof as1));
  DBG3(I386_INSN, "use: %s\n", regset_to_string(use, as1, sizeof as1));
  DBG3(I386_INSN, "def: %s\n", regset_to_string(def, as1, sizeof as1));
}*/

/*void
i386_iseq_get_usedef(struct i386_insn_t const *insns, int n_insns,
    struct regset_t *use, struct regset_t *def, struct memset_t *memuse,
    struct memset_t *memdef)
{
  regset_clear(use);
  regset_clear(def);
  memset_clear(memuse);
  memset_clear(memdef);

  int i;
  for (i = n_insns - 1; i >= 0; i--) {
    regset_t iuse, idef;
    memset_t imemuse, imemdef;

    memset_init(&imemuse);
    memset_init(&imemdef);

    i386_insn_get_usedef(&insns[i], &iuse, &idef, &imemuse, &imemdef);

    regset_diff(use, &idef);
    regset_union(def, &idef);
    regset_union(use, &iuse);

    //memset_diff(&memuse, &imemdef);
    memset_union(memdef, &imemdef);
    memset_union(memuse, &imemuse);

    memset_free(&imemuse);
    memset_free(&imemdef);
  }
}*/

static int
i386_insn_get_highest_canon_constant(i386_insn_t const *insn)
{
  int i;
  int ret = -1;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_imm && insn->op[i].valtag.imm.tag == tag_var) {
      //ret = MAX(ret, insn->op[i].valtag.imm.val);
      if (ret < insn->op[i].valtag.imm.val) {
        ret = insn->op[i].valtag.imm.val;
      }
    } else if (   insn->op[i].type == op_mem
               //&& insn->op[i].tag.mem.all == tag_const
               && insn->op[i].valtag.mem.disp.tag == tag_var) {
      //ret = MAX(ret, insn->op[i].valtag.mem.disp.val);
      if (ret < insn->op[i].valtag.mem.disp.val) {
        ret = insn->op[i].valtag.mem.disp.val;
      }
    }
  }
  return ret;
}

/*int
i386_insn_get_highest_vreg(i386_insn_t const *insn)
{
  int i;
  int ret = -1;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_reg && insn->op[i].tag.reg == tag_var) {
      ret = MAX(ret, insn->op[i].val.reg);
    } else if (   insn->op[i].type == op_mem
               && insn->op[i].tag.mem.all == tag_const) {
      if (   insn->op[i].val.mem.base != -1
          && insn->op[i].tag.mem.base == tag_var) {
        ret = MAX(ret, insn->op[i].val.mem.base);
      }
      if (   insn->op[i].val.mem.index != -1
          && insn->op[i].tag.mem.index == tag_var) {
        ret = MAX(ret, insn->op[i].val.mem.index);
      }
    }
  }
  return ret;
}*/


int
i386_iseq_get_num_canon_constants(i386_insn_t const *iseq, int iseq_len)
{
  int i, ret = -1;

  for (i = 0; i < iseq_len; i++) {
    int max_canon_constant = i386_insn_get_highest_canon_constant(&iseq[i]);
    if (ret < max_canon_constant) {
      ret = max_canon_constant;
    }
  }
  return ret;
}

bool
i386_insn_is_function_return(i386_insn_t const *insn)
{
  char const *opc;
  opc = opctable_name(insn->opc);
  if (strstart(opc, "ret", NULL)) {
    return true;
  }
  return false;
}

bool
i386_insn_is_conditional_indirect_branch(i386_insn_t const *insn)
{
  return false;
}

bool
i386_insn_is_function_call(i386_insn_t const *insn)
{
  if (insn->opc == opc_calll) {
    if (   insn->op[insn->num_implicit_ops].type == op_pcrel
        && insn->op[insn->num_implicit_ops].valtag.pcrel.tag == tag_const
        && insn->op[insn->num_implicit_ops].valtag.pcrel.val == 0) {
      return false;
    }
    return true;
  }
  return false;
}

bool
i386_insn_is_direct_function_call(i386_insn_t const *insn)
{
  if (   insn->opc == opc_calll
      && insn->op[insn->num_implicit_ops].type == op_pcrel) {
    return true;
  }
  return false;
}

bool
i386_insn_is_indirect_function_call(i386_insn_t const *insn)
{
  return i386_insn_is_function_call(insn) && !i386_insn_is_direct_function_call(insn);
}

static nextpc_t
__i386_insn_branch_target(i386_insn_t const *insn, src_ulong nip, src_tag_t tag)
{
  int n = insn->num_implicit_ops;
  ASSERT(n >= 0 && n < I386_MAX_NUM_OPERANDS);
  if (insn->op[n].type == op_pcrel && insn->op[n].valtag.pcrel.tag == tag) {
    return nextpc_t(tag, nip + insn->op[n].valtag.pcrel.val);
  }
  if (i386_insn_is_function_call(insn) || i386_insn_is_jecxz(insn)) {
    if (insn->op[n].type == op_pcrel && insn->op[n].valtag.pcrel.tag == tag) {
      return nextpc_t(tag, nip + insn->op[n].valtag.pcrel.val);
    }
    return nextpc_t(tag_invalid, -1);
  }
  return nextpc_t(tag_invalid, -1);
}

//the order of the targets matters; because it dictates the order of the bbl's nextpcs, which in turn dictates how we compute fallthrough_pc during src_iseq_fetch
vector<nextpc_t>
i386_insn_branch_targets(i386_insn_t const *insn, src_ulong nip)
{
  vector<nextpc_t> ret;
  nextpc_t target0 = __i386_insn_branch_target(insn, nip, tag_const);
  if (target0.get_tag() != tag_invalid) {
    //cout << __func__ << " " << __LINE__ << ": target0 = " << target0.nextpc_to_string() << endl;
    ret.push_back(target0);
  } else {
    nextpc_t target0 = __i386_insn_branch_target(insn, 0, tag_input_exec_reloc_index);
    if (target0.get_tag() != tag_invalid) {
      //cout << __func__ << " " << __LINE__ << ": target0 = " << target0.nextpc_to_string() << endl;
      ret.push_back(target0);
    }
  }
  if (!i386_insn_is_unconditional_branch_not_fcall(insn)) {
    ret.push_back(nip);
  }
  return ret;
}

nextpc_t
i386_insn_fcall_target(i386_insn_t const *insn, src_ulong nip, struct input_exec_t const *in)
{
  return i386_insn_branch_target(insn, nip);
}

src_ulong
i386_insn_branch_target_src_insn_pc(i386_insn_t const *insn)
{
  nextpc_t ret = __i386_insn_branch_target(insn, 0, tag_src_insn_pc);
  ASSERT(ret.get_tag() == tag_src_insn_pc || ret.get_tag() == tag_invalid);
  if (ret.get_tag() == tag_src_insn_pc) {
    return ret.get_src_insn_pc();
  } else {
    return (src_ulong)-1;
  }
}

#define OPERAND_INV_RENAME_EXREG(regmap, field, group) do { \
  src_tag_t tag; \
  int rename_reg; \
  ASSERT(op->valtag.field.tag == tag_const); \
  ASSERT(op->valtag.field.val != -1); \
  rename_reg = regmap_inv_rename_exreg(regmap, group, \
      op->valtag.field.val).reg_identifier_get_id(); \
  ASSERT(rename_reg >= 0); \
  op->valtag.field.val = rename_reg; \
  op->valtag.field.tag = tag_var; \
} while(0)

static void
operand_inv_rename_registers(struct operand_t *op, regmap_t const *regmap,
    opc_t opc, int op_index)
{
  int i, rename_reg/*, exvreg_group*/;
  //src_tag_t rename_tag;

  if (op->type == op_reg) {
    OPERAND_INV_RENAME_EXREG(regmap, reg, I386_EXREG_GROUP_GPRS);
    /*ASSERT(op->valtag.reg.tag == tag_const);
    rename_reg = regmap_inv_rename_reg(regmap, op->valtag.reg.val,
        op->size);
    if (rename_reg >= 0) {
      DBG(I386_READ, "Renaming tag_const register %ld to register %d\n",
          op->valtag.reg.val, rename_reg);
      //ASSERT(rename_tag == tag_var || rename_tag == tag_temp);
      op->valtag.reg.val = rename_reg;
      op->valtag.reg.tag = tag_var;
      //op->exvreg_group[I386_OP_REG_EXVREG_NUM] = exvreg_group;
    }*/
  /*} else if (op->type == op_st) {
    OPERAND_INV_RENAME_EXREG(st, I386_EXREG_GROUP_ST);*/
  } else if (op->type == op_seg) {
    OPERAND_INV_RENAME_EXREG(regmap, seg, I386_EXREG_GROUP_SEGS);
  //} else if (op->type == op_flags_unsigned) {
  //  OPERAND_INV_RENAME_EXREG(regmap, flags_unsigned,
  //      I386_EXREG_GROUP_EFLAGS_UNSIGNED);
  //} else if (op->type == op_flags_signed) {
  //  OPERAND_INV_RENAME_EXREG(regmap, flags_signed,
  //      I386_EXREG_GROUP_EFLAGS_SIGNED);
  } else if (op->type == op_flags_other) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_other,
        I386_EXREG_GROUP_EFLAGS_OTHER);
  } else if (op->type == op_flags_eq) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_eq,
        I386_EXREG_GROUP_EFLAGS_EQ);
  } else if (op->type == op_flags_ne) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_ne,
        I386_EXREG_GROUP_EFLAGS_NE);

  } else if (op->type == op_flags_ult) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_ult,
        I386_EXREG_GROUP_EFLAGS_ULT);
  } else if (op->type == op_flags_slt) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_slt,
        I386_EXREG_GROUP_EFLAGS_SLT);
  } else if (op->type == op_flags_ugt) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_ugt,
        I386_EXREG_GROUP_EFLAGS_UGT);
  } else if (op->type == op_flags_sgt) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_sgt,
        I386_EXREG_GROUP_EFLAGS_SGT);
  } else if (op->type == op_flags_ule) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_ule,
        I386_EXREG_GROUP_EFLAGS_ULE);
  } else if (op->type == op_flags_sle) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_sle,
        I386_EXREG_GROUP_EFLAGS_SLE);
  } else if (op->type == op_flags_uge) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_uge,
        I386_EXREG_GROUP_EFLAGS_UGE);
  } else if (op->type == op_flags_sge) {
    OPERAND_INV_RENAME_EXREG(regmap, flags_sge,
        I386_EXREG_GROUP_EFLAGS_SGE);
  } else if (op->type == op_st_top) {
    OPERAND_INV_RENAME_EXREG(regmap, st_top, I386_EXREG_GROUP_ST_TOP);
  } else if (op->type == op_mmx) {
    OPERAND_INV_RENAME_EXREG(regmap, mmx, I386_EXREG_GROUP_MMX);
  } else if (op->type == op_xmm) {
    OPERAND_INV_RENAME_EXREG(regmap, xmm, I386_EXREG_GROUP_XMM);
  } else if (op->type == op_ymm) {
    OPERAND_INV_RENAME_EXREG(regmap, ymm, I386_EXREG_GROUP_YMM);
  } else if (op->type == op_fp_data_reg) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_data_reg, I386_EXREG_GROUP_FP_DATA_REGS);
  } else if (op->type == op_fp_control_reg_rm) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_control_reg_rm, I386_EXREG_GROUP_FP_CONTROL_REG_RM);
  } else if (op->type == op_fp_control_reg_other) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_control_reg_other, I386_EXREG_GROUP_FP_CONTROL_REG_OTHER);
  } else if (op->type == op_fp_tag_reg) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_tag_reg, I386_EXREG_GROUP_FP_TAG_REG);
  } else if (op->type == op_fp_last_instr_pointer) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_last_instr_pointer, I386_EXREG_GROUP_FP_LAST_INSTR_POINTER);
  } else if (op->type == op_fp_last_operand_pointer) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_last_operand_pointer, I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER);
  } else if (op->type == op_fp_opcode) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_opcode, I386_EXREG_GROUP_FP_OPCODE_REG);
  } else if (op->type == op_mxcsr_rm) {
    OPERAND_INV_RENAME_EXREG(regmap, mxcsr_rm, I386_EXREG_GROUP_MXCSR_RM);
  } else if (op->type == op_fp_status_reg_c0) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_status_reg_c0, I386_EXREG_GROUP_FP_STATUS_REG_C0);
  } else if (op->type == op_fp_status_reg_c1) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_status_reg_c1, I386_EXREG_GROUP_FP_STATUS_REG_C1);
  } else if (op->type == op_fp_status_reg_c2) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_status_reg_c2, I386_EXREG_GROUP_FP_STATUS_REG_C2);
  } else if (op->type == op_fp_status_reg_c3) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_status_reg_c3, I386_EXREG_GROUP_FP_STATUS_REG_C3);
  } else if (op->type == op_fp_status_reg_other) {
    OPERAND_INV_RENAME_EXREG(regmap, fp_status_reg_other, I386_EXREG_GROUP_FP_STATUS_REG_OTHER);
  }
}

void
i386_insn_inv_rename_registers(i386_insn_t *insn, struct regmap_t const *regmap)
{
  long i;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    operand_inv_rename_registers(&insn->op[i], regmap, insn->opc, i);
  }
}

static bool
operand_inv_rename_memory_operands(struct operand_t *op, regmap_t const *regmap)
{
  if (op->type == op_mem) {
    int rename_reg;
    int i, canon;

    if (op->valtag.mem.segtype == segtype_sel) {
      OPERAND_INV_RENAME_EXREG(regmap, mem.seg.sel, I386_EXREG_GROUP_SEGS);
    }
    if (op->valtag.mem.base.val != -1) {
      ASSERT(op->valtag.mem.base.val >= 0);
      ASSERT(op->valtag.mem.base.val < I386_NUM_GPRS);
      ASSERT(op->valtag.mem.base.tag == tag_const);
      OPERAND_INV_RENAME_EXREG(regmap, mem.base, I386_EXREG_GROUP_GPRS);
      /*rename_reg = regmap_inv_rename_reg(regmap,
          op->valtag.mem.base.val, op->valtag.mem.addrsize);
      if (rename_reg >= 0) {
        op->valtag.mem.base.val = rename_reg;
        op->valtag.mem.base.tag = tag_var;
      }*/
    }
    if (op->valtag.mem.index.val != -1) {
      ASSERT(op->valtag.mem.index.val >= 0);
      ASSERT(op->valtag.mem.index.val < I386_NUM_GPRS);
      ASSERT(op->valtag.mem.index.tag == tag_const);
      OPERAND_INV_RENAME_EXREG(regmap, mem.index, I386_EXREG_GROUP_GPRS);
      /*rename_reg = regmap_inv_rename_reg(regmap,
          op->valtag.mem.index.val, op->valtag.mem.addrsize);
      if (rename_reg >= 0) {
        op->valtag.mem.index.val = rename_reg;
        op->valtag.mem.index.tag = tag_var;
      }*/
    }
    //ASSERT(op->valtag.mem.disp.tag == tag_const);
  }
  return true;
}

bool
i386_insn_inv_rename_memory_operands(i386_insn_t *insn, regmap_t const *regmap,
    struct vconstants_t const *vconstants)
{
  int i;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (!operand_inv_rename_memory_operands(&insn->op[i], regmap)) {
      DBG(PEEP_TAB, "returning false for operand %d\n", i);
      NOT_REACHED();
      return false;
    }
  }
  return true;
}

static vconstants_t const *
operand_inv_rename_constants(struct operand_t *op, vconstants_t const *vc)
{
  long i;

  if (op->type == op_imm || op->type == op_mem) {
    int32_t imm;
    if (op->type == op_imm) {
      ASSERT(op->valtag.imm.tag == tag_const);
      imm = op->valtag.imm.val;
    } else {
      ASSERT(op->type == op_mem);
      ASSERT(op->valtag.mem.disp.tag == tag_const);
      imm = op->valtag.mem.disp.val;
    }
    DBG3(PEEP_TAB, "vc->valid=%d\n", vc->valid);
    if (vc->valid) {
      valtag_t *vt;
      ASSERT(imm == vc->placeholder);
      if (op->type == op_imm) {
        op->valtag.imm.tag = vc->tag;
        op->valtag.imm.val = vc->val;
        vt = &op->valtag.imm;
      } else {
        op->valtag.mem.disp.tag = vc->tag;
        op->valtag.mem.disp.val = vc->val;
        vt = &op->valtag.mem.disp;
      }
      vt->mod.imm.modifier = vc->imm_modifier;
      vt->mod.imm.sconstants[0] = vc->sconstants[0];
      for (i = 0; i < IMM_MAX_NUM_SCONSTANTS; i++) {
        vt->mod.imm.sconstants[i] = vc->sconstants[i];
      }
      vt->mod.imm.constant_val = vc->constant_val;
      vt->mod.imm.constant_tag = vc->constant_tag;
      vt->mod.imm.exvreg_group = vc->exvreg_group;
    }
  }
  if (op->type != invalid) {
    return vc + 1;;
  } else {
    return vc;
  }
}

void
i386_insn_inv_rename_constants(i386_insn_t *insn, vconstants_t const *vconstants)
{
  int startop, endop, update;
  vconstants_t const *vc;
  char const *opc;
  long i;

  opc = opctable_name(insn->opc);

  if (   strstart(opc, "bound", NULL)
      || strstart(opc, "enter", NULL)) {
    startop = insn->num_implicit_ops; endop = I386_MAX_NUM_OPERANDS; update = 1;
  } else {
    startop = I386_MAX_NUM_OPERANDS - 1; endop = insn->num_implicit_ops - 1; update = -1;
    //startop = 2; update = -1;
    //endop = i386_insn_num_implicit_operands(insn) - 1;
  }

  vc = vconstants;

  //for (i = I386_MAX_NUM_OPERANDS - 1; i >= 0; i--)
  for (i = startop; i != endop; i += update) {
    DBG3(PEEP_TAB, "renaming operand %ld, vc=%p\n", i, vc);
    vc = operand_inv_rename_constants(&insn->op[i], vc);
  }
}

static void
i386_regname_suffix(long regno, char const *suffix, char *name, size_t name_size)
{
  bool high_order;
  long nbytes;

  high_order = false;
  if (!strcmp(suffix, "b")) {
    nbytes = 1;
  } else if (!strcmp(suffix, "B")) {
    nbytes = 1;
    high_order = true;
  } else if (!strcmp(suffix, "w")) {
    nbytes = 2;
  } else if (!strcmp(suffix, "d")) {
    nbytes = 4;
  } else NOT_REACHED();
  i386_regname_bytes(regno, nbytes, high_order, name, name_size);
}

static int
i386_assign_unused_reg(long vregnum, regvar_t rvt, char const *suffix, bool *used_regs,
    long num_regs)
{
  long i, start_reg;

  DBG2(PEEP_TAB, "vregnum = %ld\n", vregnum);
  ASSERT(num_regs == 8);

  if (!strcmp(suffix, "b") || !strcmp(suffix, "B")) {
    start_reg = 3;
  } else {
    start_reg = 7;
  }

  for (i = start_reg; i >= 0; i--) {
    if (!used_regs[i]) {
      DBG2(I386_READ, "setting src_regno %ld to %ld\n", vregnum, i);
      used_regs[i] = true;
      return i;
    }
  }
  return -1;
}

static bool
is_rep(char const *ptr, char const *newline)
{
  char const *found = strstr(ptr, "rep");
  return found && found < newline;
}

/*static void
check_iregcons(int reg, regcons_t const *iregcons, bool *need_vreg_alloc)
{
  int vregnum;
  if ((vregnum = regcons_has_single_reg_constraint(iregcons, reg)) == -1) {
    need_vreg_alloc[reg] = true;
  }
}*/

#define FASSERT_RETFALSE(x) do { \
  if (!(x)) {DBG(INFER_CONS, "returning %d\n", 0); return false;} \
} while(0)
#define FASSERT(x) do { \
  if (!(x)) {DBG(INFER_CONS, "returning %d\n", 0); NOT_REACHED();} \
} while(0)

static bool
infer_regcons_for_string_insn_seg(regcons_t *regcons, char const *start, int segnum)
{
  char const *segpersign, *colon;
  int exvregnum, group;

  colon = strchr(start, ':');
  FASSERT_RETFALSE(colon);
  segpersign = strbchr(colon, '%', start);
  FASSERT_RETFALSE(segpersign && segpersign < colon && segpersign > start);

  char seg[colon - segpersign];
  memcpy(seg, segpersign + 1, colon - segpersign - 1);
  seg[colon - segpersign - 1] = '\0';
  exvregnum = regset_exvregname_to_num(seg, &group).reg_identifier_get_id();
  FASSERT_RETFALSE(group == I386_EXREG_GROUP_SEGS && exvregnum != -1);
  regcons_clear_exreg(regcons, I386_EXREG_GROUP_SEGS, exvregnum);
  regcons_exreg_mark(regcons, I386_EXREG_GROUP_SEGS, exvregnum, segnum);
  return true;
}

static bool
i386_infer_regcons_for_dm_string_operand(regcons_t *regcons, char const *opcode,
    char const *start, char const *end, bool first)
{
  char const *colon, *persign, *lbrack, *rbrack;

  colon = strchr(start, ':');
  FASSERT_RETFALSE(colon);
  persign = strchr(colon, '%');
  FASSERT_RETFALSE(persign && end && persign < end);
  if ((opcode[0] == 'm') == first) {
    if (!infer_regcons_for_string_insn_seg(regcons, start, R_DS)) {
      return false;
    }
  } else {
    if (!infer_regcons_for_string_insn_seg(regcons, start, R_ES)) {
      return false;
    }
  }
  lbrack = strchr(start, '(');
  rbrack = strchr(start, ')');
  FASSERT_RETFALSE(lbrack && rbrack && lbrack < persign && rbrack > persign);
  //FASSERT(lbrack > segpersign);
  char dm_arg1[rbrack - persign];
  int da1_vregnum, group;
  memcpy(dm_arg1, persign + 1, rbrack - persign - 1);
  dm_arg1[rbrack - persign - 1] = '\0';
  da1_vregnum = regset_exvregname_to_num(dm_arg1, &group).reg_identifier_get_id();
  ASSERT(da1_vregnum != -1);
  ASSERT(group == I386_EXREG_GROUP_GPRS);
  if (da1_vregnum != -1) {
    regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, da1_vregnum);
    if ((opcode[0] == 'm') == first) {
      regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, da1_vregnum, R_ESI);
    } else {
      regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, da1_vregnum, R_EDI);
    }
  }
  return true;
}

static void
i386_infer_regcons_for_byte_regs(regcons_t *regcons, char const *assembly,
    char const *pattern)
{
  char const *vreg;
  char const *ptr;
  size_t len;

  ptr = assembly;
  while (vreg = strstr(ptr, pattern)) {
    len = strlen(pattern);
    if (vreg[len + 1] == 'b' || vreg[len + 1] == 'B') {
      int vregnum = vreg[len] - '0';
      regcons_exreg_unmark(regcons, I386_EXREG_GROUP_GPRS, vregnum, R_ESP);
      regcons_exreg_unmark(regcons, I386_EXREG_GROUP_GPRS, vregnum, R_EBP);
      regcons_exreg_unmark(regcons, I386_EXREG_GROUP_GPRS, vregnum, R_ESI);
      regcons_exreg_unmark(regcons, I386_EXREG_GROUP_GPRS, vregnum, R_EDI);
    }
    ptr = vreg + 1;
  }
}

static bool
lastpersign_represents_flag_operand(char const *lastpersign)
{
  char const *remaining;
  if (   strstart(lastpersign, "%exvr", &remaining)
      && atoi(remaining) >= I386_EXREG_GROUP_EFLAGS_OTHER
      && atoi(remaining) <= I386_EXREG_GROUP_EFLAGS_SGE) {
    return true;
  }
  return false;
}

static char const *
lastpersign_ignore_flag_operands(char const *newline, char const *opcodestr)
{
  char const *lastpersign = strbchr(newline, '%', opcodestr);
  FASSERT(lastpersign);
  FASSERT(lastpersign > opcodestr);
  while (lastpersign_represents_flag_operand(lastpersign)) {
    lastpersign--;
    lastpersign = strbchr(lastpersign, '%', opcodestr);
    FASSERT(lastpersign);
    FASSERT(lastpersign > opcodestr);
  }
  return lastpersign;
}

/* Returns if the assembly was well-formed. */
bool
i386_infer_regcons_from_assembly(char const *assembly, regcons_t *regcons)
{
  autostop_timer func_timer(__func__);
  char const *ptr, *newline, *lastpersign, *secondlastpersign, *opcodestr;
  int i, j, num_i386_regs_used = 0;
  char const *newline_end;
  bool infer;

  //XXX: need to infer regcons for aad/aam/das/daa/aaa/aas instructions' implicit ops
  //regcons_clear(regcons);
  regcons_set_i386(regcons, fixed_reg_mapping_t());
  /*for (i = 0; i < I386_NUM_GPRS; i++) {
    regcons_entry_set(&regcons->table[i], I386_NUM_GPRS);
  }*/

  ptr = assembly;
  /* DBG(INFER_CONS, "before tmap:\n%s\n", transmap_to_string(tmap, as1, sizeof as1));
  transmap_remove_redundant_reg_mappings_using_assembly(tmap, assembly);
  DBG(INFER_CONS, "after tmap:\n%s\n", transmap_to_string(tmap, as1, sizeof as1)); */

  DBG(INFER_CONS, "inferring regcons for:\n%s\niregcons:\n%s\n", assembly,
      regcons_to_string(regcons, as1, sizeof as1));

  char const newline_char = '\n'; //'|'

  /* Look for special opcodes */
  while (newline = strchr(ptr, newline_char)) {
    DBG2(INFER_CONS, "looking at line, inferring regcons for:\n%s\niregcons:\n%s\n",
        ptr,
        regcons_to_string(regcons, as1, sizeof as1));

    /* check for shift count operand. */
    if (   (opcodestr = strstr_arr(ptr, shift_opcodes, num_shift_opcodes))
        && (opcodestr == ptr || *(opcodestr - 1) != 'u')  /* to avoid confusing with pushl */
        && (opcodestr[3] != 'x')
        && (opcodestr < newline)) {
      DBG3(INFER_CONS, "found shift_insn: %s\n", opcodestr);
      char const *comma = strchr(opcodestr, ',');
      char const *persign = strchr(opcodestr, '%');
      if (persign && comma && persign < comma) {
        char shift_count[comma - persign];
        int sc_vregnum, group;
        memcpy(shift_count, persign + 1, comma - persign - 1);
        shift_count[comma - persign - 1] = '\0';
        sc_vregnum = regset_exvregname_to_num(shift_count, &group).reg_identifier_get_id();
        ASSERT(sc_vregnum != -1);
        ASSERT(group == I386_EXREG_GROUP_GPRS);
        if (sc_vregnum != -1) {
          regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, sc_vregnum);
          regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, sc_vregnum, R_ECX);
        }
      }
    } else if (   (opcodestr = strstr_arr(ptr, stack_opcodes, num_stack_opcodes))
               && opcodestr < newline
               && !strstart(opcodestr, "popcnt", NULL)) {
      DBG2(INFER_CONS, "stack opcode %s.\n", opcodestr);
      //lastpersign = strbchr(newline, '%', opcodestr);
      //FASSERT(lastpersign);
      //FASSERT(lastpersign > opcodestr);
      /*if (   strstart(opcodestr, "pushf", NULL)
          || strstart(opcodestr, "popf", NULL))*/ {
        lastpersign = lastpersign_ignore_flag_operands(newline, opcodestr);
      }
      if (   strstart(opcodestr, "leave", NULL)
          || strstart(opcodestr, "enter", NULL)) {
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EBP);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        /*while (*lastpersign != '%') {
          lastpersign--;
        }*/
        FASSERT_RETFALSE(lastpersign > opcodestr);
      }
      if (strstart(opcodestr, "call", NULL)) {
        DBG3(INFER_CONS, "adding regconst for %s to R_EAX\n", lastpersign);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EAX);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign > opcodestr);

        DBG3(INFER_CONS, "adding regconst for %s to R_ECX\n", lastpersign);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_ECX);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign > opcodestr);

        DBG3(INFER_CONS, "adding regconst for %s to R_EDX\n", lastpersign);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EDX);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign > opcodestr);
      }
      if (strstart(opcodestr, " ret", NULL)) {
        DBG3(INFER_CONS, "adding regconst for %s to R_EBX\n", lastpersign);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EBX);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign > opcodestr);

        DBG3(INFER_CONS, "adding regconst for %s to R_EBP\n", lastpersign);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EBP);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign > opcodestr);

        DBG3(INFER_CONS, "adding regconst for %s to R_ESI\n", lastpersign);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_ESI);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign > opcodestr);

        DBG3(INFER_CONS, "adding regconst for %s to R_EDI\n", lastpersign);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EDI);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign > opcodestr);

        DBG3(INFER_CONS, "adding regconst for %s to R_EAX\n", lastpersign);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EAX);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign > opcodestr);

        DBG3(INFER_CONS, "adding regconst for %s to R_EDX\n", lastpersign);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EDX);
        FASSERT_RETFALSE(infer);
        lastpersign--;
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign > opcodestr);
      }
      DBG3(INFER_CONS, "adding regconst for %s to R_SS\n", lastpersign);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_SEGS, R_SS);
      FASSERT_RETFALSE(infer);
      lastpersign--;
      lastpersign = strbchr(lastpersign, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign > opcodestr);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_ESP);
      FASSERT_RETFALSE(infer);
    } else if (   (opcodestr = strstr_arr(ptr, jecxz_opcodes, num_jecxz_opcodes))
               && opcodestr < newline) {
      DBG2(INFER_CONS, "%s", "jecxz opcode\n");
      lastpersign = strbchr(newline, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign > opcodestr);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
          I386_EXREG_GROUP_GPRS, R_ECX);
      FASSERT_RETFALSE(infer);
    } else if (   (   (opcodestr = strstr(ptr, "divb"))
                   || (opcodestr = strstr(ptr, "divw"))
                   || (opcodestr = strstr(ptr, "divl"))
                   || (opcodestr = strstr(ptr, "divq")))
               && opcodestr < newline
               && (opcodestr == ptr || opcodestr[-1] != 'f')
               && (opcodestr <= ptr + 1 || opcodestr[-2] != 'f')) {
      DBG2(INFER_CONS, "%s", "div opcode\n");
      /*lastpersign = newline;
      for (i = 0; i < 2; i++) {  //ignore the flags operands (signed / unsigned)
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT(lastpersign);
        FASSERT(lastpersign > opcodestr);
        //FASSERT(lastpersign - assembly > 0);
        lastpersign--;
      }
      while (*lastpersign != '%') {
        lastpersign--;
      }*/
      lastpersign = lastpersign_ignore_flag_operands(newline, opcodestr);
      //cout << __func__ << " " << __LINE__ << ": lastpersign = " << lastpersign << endl;
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign - assembly > 0);
      secondlastpersign = lastpersign - 1;
      while (*secondlastpersign != '%') {
        secondlastpersign--;
      }
      //cout << __func__ << " " << __LINE__ << ": secondlastpersign = " << secondlastpersign << endl;
      FASSERT_RETFALSE(secondlastpersign);
      FASSERT_RETFALSE(secondlastpersign - assembly > 0);

      if (!strstart(opcodestr, "divb", NULL)) {
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EDX);
        FASSERT_RETFALSE(infer);
      }
      infer = vreg_add_regcons_one_reg(regcons, secondlastpersign, lastpersign,
          I386_EXREG_GROUP_GPRS, R_EAX);
      FASSERT_RETFALSE(infer);
    } else if (   (opcodestr = strstr(ptr, "mulx"))
               && opcodestr < newline) {
      DBG2(INFER_CONS, "%s", "mulx opcode\n");
      char const *comma, *lbrack, *rbrack, *poundsign;

      poundsign = strchr(opcodestr, '#');
      FASSERT_RETFALSE(poundsign);
      //printf("explicit_operands=%d\n", explicit_operands);
      lastpersign = newline;
      while (*lastpersign != '%') {
        lastpersign--;
      }
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign - assembly > 0);
      FASSERT_RETFALSE(lastpersign > poundsign);

      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
          I386_EXREG_GROUP_GPRS, R_EDX);
      FASSERT_RETFALSE(infer);
    } else if (   (opcodestr = strstr(ptr, "mul"))
               && opcodestr < newline
               && (opcodestr == ptr || opcodestr[-1] != 'f')
               && (opcodestr <= ptr + 1 || opcodestr[-2] != 'f')) {
      ASSERT(opcodestr[4] != 'x');
      DBG2(INFER_CONS, "%s", "mul opcode\n");
      char const *comma, *lbrack, *rbrack, *poundsign;
      bool explicit_operands = false;

      poundsign = strchr(opcodestr, '#');
      FASSERT_RETFALSE(poundsign);
      if (   (comma = strchr(opcodestr, ','))
          && comma < poundsign) {
        lbrack = strchr(opcodestr, '(');
        rbrack = strchr(opcodestr, ')');
        if (!(   lbrack
              && lbrack < comma
              && rbrack
              && rbrack > comma
              && (   !(comma = strchr(rbrack + 1, ','))
                  || comma > poundsign))) {
          explicit_operands = true;
        }
      }
      //printf("explicit_operands=%d\n", explicit_operands);
      if (!explicit_operands) {
        /*lastpersign = newline;
        for (i = 0; i < 2; i++) {  //ignore the flags operands (signed / unsigned)
          lastpersign = strbchr(lastpersign, '%', opcodestr);
          FASSERT(lastpersign);
          FASSERT(lastpersign > opcodestr);
          lastpersign--;
        }
        while (*lastpersign != '%') {
          lastpersign--;
        }*/
        lastpersign = lastpersign_ignore_flag_operands(newline, opcodestr);
        FASSERT_RETFALSE(lastpersign);
        FASSERT_RETFALSE(lastpersign - assembly > 0);
        secondlastpersign= lastpersign - 1;
        while (*secondlastpersign != '%') {
          secondlastpersign--;
        }
        FASSERT_RETFALSE(secondlastpersign);
        FASSERT_RETFALSE(secondlastpersign - assembly > 0);

        if (strstart(opcodestr, "mulb", NULL)) {
          infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
              I386_EXREG_GROUP_GPRS, R_EAX);
          FASSERT_RETFALSE(infer);
        } else {
          infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
              I386_EXREG_GROUP_GPRS, R_EDX);
          FASSERT_RETFALSE(infer);
        }

        infer = vreg_add_regcons_one_reg(regcons, secondlastpersign, lastpersign,
            I386_EXREG_GROUP_GPRS, R_EAX);
        FASSERT_RETFALSE(infer);
      }
    } else if (   (opcodestr = strstr(ptr, "cpuid"))
               && opcodestr < newline) {
      char const *end;

      DBG2(INFER_CONS, "%s", "cpuid opcode\n");
      end = newline;
      lastpersign = strbchr(newline, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign > opcodestr);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, end,
          I386_EXREG_GROUP_GPRS, R_EDX);
      FASSERT_RETFALSE(infer);

      end = lastpersign;
      lastpersign--;
      while (*lastpersign != '%') {
        lastpersign--;
      }
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign - assembly > 0);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, end,
          I386_EXREG_GROUP_GPRS, R_ECX);
      FASSERT_RETFALSE(infer);

      end = lastpersign;
      lastpersign--;
      while (*lastpersign != '%') {
        lastpersign--;
      }
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign - assembly > 0);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, end,
          I386_EXREG_GROUP_GPRS, R_EBX);
      FASSERT_RETFALSE(infer);

      end = lastpersign;
      lastpersign--;
      while (*lastpersign != '%') {
        lastpersign--;
      }
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign - assembly > 0);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, end,
           I386_EXREG_GROUP_GPRS, R_EAX);
      FASSERT_RETFALSE(infer);
    } else if (   (   (opcodestr = strstr(ptr, "rdtsc"))
                   || (opcodestr = strstr(ptr, "rdpmc"))
                   || (opcodestr = strstr(ptr, "cwtd"))
                   || (opcodestr = strstr(ptr, "cltd")))
               && opcodestr < newline) {
      DBG2(INFER_CONS, "%s", "rdtsc/rdpmc/cwtd/cltd opcode\n");
      lastpersign = strbchr(newline, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign > opcodestr);
      secondlastpersign = lastpersign - 1;
      while (*secondlastpersign != '%') {
        secondlastpersign--;
      }
      FASSERT_RETFALSE(secondlastpersign);
      FASSERT_RETFALSE(secondlastpersign - assembly > 0);

      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
           I386_EXREG_GROUP_GPRS, R_EDX);
      FASSERT_RETFALSE(infer);
      infer = vreg_add_regcons_one_reg(regcons, secondlastpersign, lastpersign,
          I386_EXREG_GROUP_GPRS, R_EAX);
      FASSERT_RETFALSE(infer);
    } else if (   (   (opcodestr = strstr(ptr, "cbtw"))
                   || (opcodestr = strstr(ptr, "cwtl")))
               && opcodestr < newline) {
      DBG2(INFER_CONS, "%s", "cbtw/cwtl opcode\n");
      lastpersign = strbchr(newline, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign > opcodestr);

      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
          I386_EXREG_GROUP_GPRS, R_EAX);
      FASSERT_RETFALSE(infer);
    } else if ((opcodestr = strstr(ptr, "int")) && opcodestr < newline) {
      DBG2(INFER_CONS, "%s", "int opcode\n");
      lastpersign = newline;
      for (i = 0; i < I386_NUM_GPRS; i++) {
        lastpersign = strbchr(lastpersign, '%', opcodestr);
        FASSERT_RETFALSE(lastpersign);
        FASSERT_RETFALSE(lastpersign > opcodestr);
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, i);
        FASSERT_RETFALSE(infer);
        lastpersign--;
      }
    } else if (   (   (opcodestr = strstr(ptr, "inb"))
                   || (opcodestr = strstr(ptr, "inw"))
                   || (opcodestr = strstr(ptr, "inl"))
               && opcodestr < newline)) {
      lastpersign = strbchr(newline, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign > opcodestr);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
          I386_EXREG_GROUP_GPRS, R_EAX);
      FASSERT_RETFALSE(infer);
      lastpersign--;
      lastpersign = strbchr(lastpersign, '%', opcodestr);
      if (lastpersign > opcodestr) {
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EDX);
        FASSERT_RETFALSE(infer);
      }
    } else if (   (   (opcodestr = strstr(ptr, "outb"))
                   || (opcodestr = strstr(ptr, "outw"))
                   || (opcodestr = strstr(ptr, "outl"))
               && opcodestr < newline)) {
      char const *comma;
      lastpersign = strbchr(newline, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign > opcodestr);
      comma = strbchr(newline, ',', opcodestr);
      FASSERT_RETFALSE(comma);
      FASSERT_RETFALSE(comma > opcodestr);
      if (lastpersign > comma) {
        //out %al,%dx
        infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
            I386_EXREG_GROUP_GPRS, R_EDX);
        FASSERT_RETFALSE(infer);
      }
      lastpersign = strbchr(comma, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign > opcodestr);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
          I386_EXREG_GROUP_GPRS, R_EAX);
      FASSERT_RETFALSE(infer);
    } else if ((opcodestr = strstr(ptr, "cmpxchg")) && opcodestr < newline) {
      DBG2(INFER_CONS, "%s", "cmpxchg opcode\n");
      lastpersign = lastpersign_ignore_flag_operands(newline, opcodestr);
      /*lastpersign = strbchr(newline, '%', opcodestr);
      FASSERT(lastpersign);
      FASSERT(lastpersign > opcodestr);
      lastpersign--;
      lastpersign = strbchr(lastpersign, '%', opcodestr);
      FASSERT(lastpersign);
      FASSERT(lastpersign - assembly > 0);
      lastpersign--;
      lastpersign = strbchr(lastpersign, '%', opcodestr);*/
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign - assembly > 0);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
          I386_EXREG_GROUP_GPRS, R_EAX);
      FASSERT_RETFALSE(infer);
    } else if ((opcodestr = strstr(ptr, "xlat")) && opcodestr < newline) {
      lastpersign = strbchr(newline, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign > opcodestr);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
          I386_EXREG_GROUP_GPRS, R_EAX);
      FASSERT_RETFALSE(infer);
      lastpersign--;
      lastpersign = strbchr(lastpersign, '%', opcodestr);
      FASSERT_RETFALSE(lastpersign);
      FASSERT_RETFALSE(lastpersign > opcodestr);
      infer = vreg_add_regcons_one_reg(regcons, lastpersign, newline,
          I386_EXREG_GROUP_GPRS, R_EBX);
      FASSERT_RETFALSE(infer);
    }
    newline_end = strchr(newline, '\n');
    ASSERT(newline_end);
    ptr = newline_end + 1;
  }

  /* Look for memory operands and infer base and index registers. */
  ptr = assembly;
  while (newline = strchr(ptr, newline_char)) {
    char const *lbrack, *rbrack;
    lbrack = strchr(ptr, '(');
    rbrack = strchr(ptr, ')');
    if (lbrack && rbrack && lbrack < newline && rbrack < newline) {
      char const *comma1, *persign;
      comma1 = strchr(lbrack, ',');
      persign = strchr(lbrack, '%');

      DBG3(INFER_CONS, "found memory access at %s\n", lbrack);
      /* check for base registers. */
      if (!comma1 || (comma1 && comma1 < rbrack)) {
        char const *base_end;
        if (comma1) {
          base_end = comma1;
        } else {
          base_end = rbrack;
        }
        if (persign && persign > lbrack && persign < base_end) {
          FASSERT_RETFALSE(base_end);
          FASSERT_RETFALSE(base_end > persign);
          char base_reg[base_end - persign];
          int base_vregnum, group;
          memcpy(base_reg, persign + 1, base_end - persign - 1);
          base_reg[base_end - persign - 1] = '\0';
          base_vregnum = regset_exvregname_to_num(base_reg, &group).reg_identifier_get_id();
          if (base_vregnum != -1) {
            if (base_reg[base_end - persign - 2] == 'w') {
              regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, base_vregnum);
              regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, base_vregnum, R_EBX);
              regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, base_vregnum, R_EBP);
            }
          }
        }
      }

      /* check for index registers. */
      if (comma1 && comma1 < rbrack) {
        char const *persign, *comma2;
        comma2 = strchr(comma1 + 1, ',');
        FASSERT_RETFALSE(comma2);
        FASSERT_RETFALSE(comma2 < rbrack);
        persign = strchr(comma1, '%');
        FASSERT_RETFALSE(persign);
        FASSERT_RETFALSE(persign < comma2);
        char index_reg[comma2 - persign];
        int index_vregnum, group;
        memcpy(index_reg, persign + 1, comma2 - persign - 1);
        index_reg[comma2 - persign - 1] = '\0';
        reg_identifier_t index_vreg_id = regset_exvregname_to_num(index_reg, &group);
        index_vregnum = index_vreg_id.reg_identifier_get_id();
        DBG3(INFER_CONS, "index_reg = %s, index_vreg_id = %s, index_vregnum = %d\n", index_reg, index_vreg_id.reg_identifier_to_string().c_str(), index_vregnum);
        if (index_vregnum != -1) {
          if (index_reg[comma2 - persign - 2] == 'd' || index_reg[comma2 - persign - 2] == 'q') {
            /* unmark esp for 32-bit addressing. */
            regcons_exreg_unmark(regcons, I386_EXREG_GROUP_GPRS, index_vregnum, R_ESP);
          } else if (index_reg[comma2 - persign - 2] == 'w') {
            regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, index_vregnum);
            regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, index_vregnum, R_ESI);
            regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, index_vregnum, R_EDI);
          } else NOT_REACHED();
        }
      }
    }
    newline_end = strchr(newline, '\n');
    ASSERT(newline_end);
    ptr = newline_end + 1;
  }

  /* check for 8-bit vregs. */
  i386_infer_regcons_for_byte_regs(regcons, assembly, "%exvr0.");
  //i386_infer_regcons_for_byte_regs(regcons, assembly, "%tr", tag_temp);

  /* check for in. */
  ptr = assembly;
  char const *in_insn;
  while (in_insn = strstr(ptr, "in")) {
    if (in_insn[2] == 'b' && in_insn[2] == 'w' || in_insn[2] == ' ') {
      char const *comma = strchr(in_insn, ',');
      char const *persign = strchr(in_insn, '%');
      char const *end = in_insn + strlen(in_insn);
      FASSERT_RETFALSE(comma);
      FASSERT_RETFALSE(persign);
      if (persign < comma) {
        char in_port[comma - persign];
        int ip_vregnum, group;
        memcpy(in_port, persign + 1, comma - persign - 1);
        in_port[comma - persign - 1] = '\0';
        ip_vregnum = regset_exvregname_to_num(in_port, &group).reg_identifier_get_id();
        if (ip_vregnum != -1) {
          regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, ip_vregnum);
          regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, ip_vregnum, R_EDX);
        }
        persign = strchr(comma, '%');
        FASSERT_RETFALSE(persign);
      }
      char in_data[end - persign];
      int id_vregnum, group;
      memcpy(in_data, persign + 1, end - persign - 1);
      in_data[end - persign - 1] = '\0';
      id_vregnum = regset_exvregname_to_num(in_data, &group).reg_identifier_get_id();
      if (id_vregnum != -1) {
        regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, id_vregnum);
        regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, id_vregnum, R_EAX);
      }
    }
    ptr = in_insn + 1;
  }

  /* check for out. */
  ptr = assembly;
  char const *out_insn;
  while (out_insn = strstr(ptr, "out")) {
    if (out_insn[2] == 'b' || out_insn[2] == 'w' || out_insn[2] == ' ') {
      char const *comma = strchr(out_insn, ',');
      char const *persign = strchr(out_insn, '%');
      char const *end = out_insn + strlen(out_insn);
      FASSERT_RETFALSE(comma && persign && persign < comma);
      char out_data[end - persign];
      int od_vregnum, group;
      memcpy(out_data, persign + 1, end - persign - 1);
      out_data[end - persign - 1] = '\0';
      od_vregnum = regset_exvregname_to_num(out_data, &group).reg_identifier_get_id();
      if (od_vregnum != -1) {
        regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, od_vregnum);
        regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, od_vregnum, R_EAX);
      }
      persign = strchr(comma, '%');
      if (persign) {
        char out_port[end - persign];
        int op_vregnum, group;
        memcpy(out_port, persign + 1, end - persign - 1);
        out_port[end - persign - 1] = '\0';
        op_vregnum = regset_exvregname_to_num(out_port, &group).reg_identifier_get_id();
        if (op_vregnum != -1) {
          regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, op_vregnum);
          regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, op_vregnum, R_EDX);
        }
      }
    }
    ptr = out_insn + 1;
  }

  /* check for outs, lods, scas. */
  ptr = assembly;
  char const *outs_insn;
  while (outs_insn = strstr_arr(ptr, in_string_opcodes, num_in_string_opcodes)) {
    char const *comma = strchr(outs_insn, ',');
    char const *end = outs_insn + strlen(outs_insn);
    char const *colon = strchr(outs_insn, ':');
    FASSERT_RETFALSE(colon);
    char const *persign = strchr(colon, '%');
    FASSERT_RETFALSE(persign && comma && persign < comma);
    FASSERT_RETFALSE(outs_insn[0] == 'o' || outs_insn[0] == 'l' || outs_insn[0] == 's');
    char const *lbrack, *rbrack;
    lbrack = strchr(outs_insn, '(');
    rbrack = strchr(outs_insn, ')');
    FASSERT_RETFALSE(lbrack && rbrack && lbrack < persign && rbrack > persign);
    char outs_data[rbrack - persign];
    int osd_vregnum, group;
    memcpy(outs_data, persign + 1, rbrack - persign - 1);
    outs_data[rbrack - persign - 1] = '\0';
    osd_vregnum = regset_exvregname_to_num(outs_data, &group).reg_identifier_get_id();
    if (osd_vregnum != -1) {
      regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, osd_vregnum);
      if (outs_insn[0] == 's') {
        if (!infer_regcons_for_string_insn_seg(regcons, outs_insn, R_ES)) {
          DBG(INFER_CONS, "returning %d\n", 0);
          return false;
        }
        regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, osd_vregnum, R_EDI);
      } else {
        if (!infer_regcons_for_string_insn_seg(regcons, outs_insn, R_DS)) {
          DBG(INFER_CONS, "returning %d\n", 0);
          return false;
        }
        regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, osd_vregnum, R_ESI);
      }
    }
    persign = strchr(comma, '%');
    FASSERT_RETFALSE(persign);
    if (outs_insn[0] == 'o') {
      char outs_port[end - persign];
      int osp_vregnum, group;
      memcpy(outs_port, persign + 1, end - persign - 1);
      outs_port[end - persign - 1] = '\0';
      osp_vregnum = regset_exvregname_to_num(outs_port, &group).reg_identifier_get_id();
      if (osp_vregnum != -1) {
        regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, osp_vregnum);
        regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, osp_vregnum, R_EDX);
      }
    } else {
      char lods_dst[end - persign];
      int ld_vregnum;
      memcpy(lods_dst, persign + 1, end - persign - 1);
      lods_dst[end - persign - 1] = '\0';
      ld_vregnum = regset_exvregname_to_num(lods_dst, &group).reg_identifier_get_id();
      if (ld_vregnum != -1) {
        regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, ld_vregnum);
        regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, ld_vregnum, R_EAX);
      }
    }
    if (is_rep(ptr, newline)) {
      persign = strchr(persign + 1, '%');
      FASSERT_RETFALSE(persign);
      char ecx_reg[end - persign];
      memcpy(ecx_reg, persign + 1, end - persign - 1);
      ecx_reg[end - persign - 1] = '\0';
      int ecx_vregnum = regset_exvregname_to_num(ecx_reg, &group).reg_identifier_get_id();
      FASSERT_RETFALSE(ecx_vregnum != -1);
      FASSERT_RETFALSE(group == I386_EXREG_GROUP_GPRS);
      regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, ecx_vregnum);
      regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, ecx_vregnum, R_ECX);
    }
    ptr = outs_insn + 1;
  }

  /* check for ins, stos. */
  ptr = assembly;
  char const *ins_insn;
  while (   (ins_insn = strstr_arr(ptr, out_string_opcodes, num_out_string_opcodes))
         && (ins_insn[0] != 'i' || ins_insn == ptr || ins_insn[-1] != 'm')) {
    FASSERT_RETFALSE(ins_insn[0] == 'i' || ins_insn[0] == 's');
    char const *comma = strchr(ins_insn, ',');
    char const *persign = strchr(ins_insn, '%');
    FASSERT_RETFALSE(persign && comma && persign < comma);
    if (ins_insn[0] == 'i') { //ins
      char ins_port[comma - persign];
      int isp_vregnum, group;
      memcpy(ins_port, persign + 1, comma - persign - 1);
      ins_port[comma - persign - 1] = '\0';
      isp_vregnum = regset_exvregname_to_num(ins_port, &group).reg_identifier_get_id();
      if (isp_vregnum != -1) {
        regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, isp_vregnum);
        regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, isp_vregnum, R_EDX);
      }
    } else {
      char stos_src[comma - persign];
      int ss_vregnum, group;
      memcpy(stos_src, persign + 1, comma - persign - 1);
      stos_src[comma - persign - 1] = '\0';
      ss_vregnum = regset_exvregname_to_num(stos_src, &group).reg_identifier_get_id();
      if (ss_vregnum != -1) {
        regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, ss_vregnum);
        regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, ss_vregnum, R_EAX);
      }
    }
    char const *colon = strchr(ins_insn, ':');
    FASSERT_RETFALSE(colon);
    if (!infer_regcons_for_string_insn_seg(regcons, ins_insn, R_ES)) {
      DBG(INFER_CONS, "returning %d\n", 0);
      return false;
    }
    persign = strchr(colon, '%');
    char const *lbrack = strchr(comma, '(');
    char const *rbrack = strchr(comma, ')');
    DBG2(INFER_CONS, "ins_insn=%p (%s) comma=%p, colon=%p, persign=%p, lbrack=%p, "
        "rbrack=%p\n", ins_insn, ins_insn, comma, colon, persign, lbrack, rbrack);
    FASSERT_RETFALSE(persign && lbrack && rbrack && lbrack < persign && rbrack > persign);
    char ins_data[rbrack - persign];
    int isd_vregnum, group;
    memcpy(ins_data, persign + 1, rbrack - persign - 1);
    ins_data[rbrack - persign - 1] = '\0';
    isd_vregnum = regset_exvregname_to_num(ins_data, &group).reg_identifier_get_id();
    FASSERT_RETFALSE(isd_vregnum != -1);
    //if (isd_vregnum != -1) {
      regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, isd_vregnum);
      regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, isd_vregnum, R_EDI);
    //}
    if (is_rep(ptr, newline)) {
      char const *end = ins_insn + strlen(ins_insn);
      persign = strchr(rbrack + 1, '%');
      FASSERT_RETFALSE(persign);
      char ecx_reg[end - persign];
      memcpy(ecx_reg, persign + 1, end - persign - 1);
      ecx_reg[end - persign - 1] = '\0';
      int ecx_vregnum = regset_exvregname_to_num(ecx_reg, &group).reg_identifier_get_id();
      FASSERT_RETFALSE(ecx_vregnum != -1);
      FASSERT_RETFALSE(group == I386_EXREG_GROUP_GPRS);
      regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, ecx_vregnum);
      regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, ecx_vregnum, R_ECX);
    }
    ptr = ins_insn + 1;
  }

  /* check for movs, cmps */
  ptr = assembly;
  char const *dms_insn;
  while (dms_insn = strstr_arr(ptr, dm_string_opcodes, num_dm_string_opcodes)) {
    if (dms_insn[-1] == 'c') { //this must be a conditional move, not dms insn
      ptr = dms_insn + 1;
      continue;
    }
    FASSERT_RETFALSE(dms_insn[0] == 'm' || dms_insn[0] == 'c');
    if (dms_insn[0] != 'm' || dms_insn[5] == ' ') {
      char const *comma = strchr(dms_insn, ',');
      char const *newline = strchr(dms_insn, newline_char);
      bool ret;
      FASSERT_RETFALSE(comma && newline && comma < newline);
      DBG(INFER_CONS, "calling i386_infer_regcons_for_dm_operand:\ndms_insn=%s, "
          "comma=%s.", dms_insn, comma);
      ret = i386_infer_regcons_for_dm_string_operand(regcons, dms_insn,
          dms_insn, comma, true);
      if (!ret) {
        DBG(INFER_CONS, "returning %d\n", 0);
        return false;
      }
      ret = i386_infer_regcons_for_dm_string_operand(regcons, dms_insn, comma,
          newline, false);
      if (!ret) {
        DBG(INFER_CONS, "returning %d\n", 0);
        return false;
      }

      if (is_rep(ptr, newline)) {
        char const *end = dms_insn + strlen(dms_insn);
        char const *rbrack = strchr(comma, ')');
        FASSERT_RETFALSE(rbrack);
        char const *persign = strchr(rbrack + 1, '%');
        FASSERT_RETFALSE(persign);
        char ecx_reg[end - persign];
        DBG(INFER_CONS, "dms_insn = %s.\n", dms_insn);
        memcpy(ecx_reg, persign + 1, end - persign - 1);
        ecx_reg[end - persign - 1] = '\0';
        DBG(INFER_CONS, "ecx_reg = %s.\n", ecx_reg);
        int group;
        DBG(INFER_CONS, "ecx_vregname= %s.\n", ecx_reg);
        int ecx_vregnum = regset_exvregname_to_num(ecx_reg, &group).reg_identifier_get_id();
        DBG(INFER_CONS, "ecx_vregnum = %d.\n", ecx_vregnum);
        DBG(INFER_CONS, "group = %d.\n", group);
        FASSERT_RETFALSE(ecx_vregnum != -1);
        FASSERT_RETFALSE(group == I386_EXREG_GROUP_GPRS);
        regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, ecx_vregnum);
        regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, ecx_vregnum, R_ECX);
      }
    }

    ptr = dms_insn + 1;
  }

  /* check for fstsw. */
  ptr = assembly;
  char const *fstsw_insn;
  while (fstsw_insn = strstr_arr(ptr, fstsw_string_opcodes, num_fstsw_string_opcodes)) {
    char const *persign = strchr(fstsw_insn, '%');
    char const *end = fstsw_insn + strlen(fstsw_insn);
    FASSERT_RETFALSE(persign);
    char fstsw_data[end - persign];
    int id_vregnum, group;
    memcpy(fstsw_data, persign + 1, end - persign - 1);
    fstsw_data[end - persign - 1] = '\0';
    id_vregnum = regset_exvregname_to_num(fstsw_data, &group).reg_identifier_get_id();
    if (id_vregnum != -1) {
      regcons_clear_exreg(regcons, I386_EXREG_GROUP_GPRS, id_vregnum);
      regcons_exreg_mark(regcons, I386_EXREG_GROUP_GPRS, id_vregnum, R_EAX);
    }
    ptr = fstsw_insn + 1;
  }

  /* check for blend opcodes */
  ptr = assembly;
  char const *pblendvb_insn;
  while (pblendvb_insn = strstr_arr(ptr, blendv_opcodes, num_blendv_opcodes)) {
    char const *persign = strchr(pblendvb_insn, '%');
    char const *end = pblendvb_insn + strlen(pblendvb_insn);
    FASSERT_RETFALSE(persign);
    char pblendvb_data[end - persign];
    memcpy(pblendvb_data, persign + 1, end - persign - 1);
    pblendvb_data[end - persign - 1] = '\0';
    CPP_DBG_EXEC3(INFER_CONS, cout << __func__ << " " << __LINE__ << ": pblendvb: persign = " << persign << endl);
    CPP_DBG_EXEC3(INFER_CONS, cout << __func__ << " " << __LINE__ << ": pblendvb: pblendvb_data = " << pblendvb_data << endl);
    int id_vregnum, group;
    id_vregnum = regset_exvregname_to_num(pblendvb_data, &group).reg_identifier_get_id();
    CPP_DBG_EXEC3(INFER_CONS, cout << __func__ << " " << __LINE__ << ": pblendvb: id_vregnum = " << id_vregnum << endl);
    if (id_vregnum != -1) {
      regcons_clear_exreg(regcons, group, id_vregnum);
      regcons_exreg_mark(regcons, group, id_vregnum, 0);
    }
    ptr = pblendvb_insn + 1;
  }

  DBG(INFER_CONS, "inferred regcons:\n%s\nfor\n%s\n",
    regcons_to_string(regcons, as1, sizeof as1),
    assembly);
  return true;
}

/*long
i386_iseq_from_string(translation_instance_t *ti, i386_insn_t *i386_iseq,
    char const *assembly)
{
  return __i386_iseq_from_string(ti, i386_iseq, assembly, I386_AS_MODE_32);
}

long
x64_iseq_from_string(translation_instance_t *ti, i386_insn_t *i386_iseq,
    char const *assembly)
{
  return __i386_iseq_from_string(ti, i386_iseq, assembly, I386_AS_MODE_64);
}*/

/*long
i386_iseq_from_string_using_transmap(translation_instance_t *ti,
    i386_insn_t *i386_iseq, char const *assembly, transmap_t const *in_tmap)
{
  NOT_IMPLEMENTED();
}*/

static int
gen_mem_modes32(char const *mem_modes[], char const *mem_constraints[],
    char const *mem_live_regs[], char const *in_text)
{
  NOT_IMPLEMENTED();
/*
  static char lmem_modes[16][32], lmem_constraints[16][32], lmem_live_regs[16][32];
  static char const *vregs[]={"vr0", "vr1", "vr2", "vr3", "vr4", "vr5", "vr6",
    "vr7"};
  regcons_t regcons;
  char vr0[8], vr1[8], c0[8];
  unsigned i;

  regcons_set(&regcons);
  i386_infer_regcons_from_assembly(in_text, &regcons, &regcons);
  vr0[0] = vr1[0] = c0[0] = '\0';
  for (i = 0; i < sizeof vregs/sizeof vregs[0]; i++) {
    if (   !strstr(in_text, vregs[i])
        && regcons_vreg_is_set(&regcons, i, tag_var)) {
      strlcpy(vr0, vregs[i], sizeof vr0);
      break;
    }
  }
  for (i = 0; i < sizeof vregs/sizeof vregs[0]; i++) {
    if (   strcmp(vr0, vregs[i])
        && !strstr(in_text, vregs[i])
        && regcons_vreg_is_set(&regcons, i, tag_var)) {
      strlcpy(vr1, vregs[i], sizeof vr1);
      break;
    }
  }
  for (i = 0; i < NUM_CANON_CONSTANTS; i++) {
    snprintf(c0, sizeof c0, "C%dd", i);
    if (!strstr(in_text, c0)) {
      break;
    }
  }
  ASSERT(vr0[0] != '\0');
  ASSERT(vr1[0] != '\0');
  ASSERT(c0[0] != '\0');

  snprintf(lmem_modes[0], sizeof lmem_modes[0], "%s(%%%sd,%%eiz,1)", c0, vr0);
  snprintf(lmem_modes[1], sizeof lmem_modes[1], "%s(%%%sd,%%%sd,1)", c0, vr0,
      vr1);
  snprintf(lmem_modes[2], sizeof lmem_modes[2], "%s(%%%sd,%%%sd,2)", c0, vr0,
      vr1);
  snprintf(lmem_modes[3], sizeof lmem_modes[3], "%s(%%%sd,%%%sd,4)", c0, vr0,
      vr1);
  snprintf(lmem_modes[4], sizeof lmem_modes[4], "%s(%%%sd,%%%sd,8)", c0, vr0,
      vr1);
  snprintf(lmem_modes[5], sizeof lmem_modes[5], "%s(,%%%sd, 1)", c0, vr0);
  snprintf(lmem_modes[6], sizeof lmem_modes[6], "%s(,%%%sd, 2)", c0, vr0);
  snprintf(lmem_modes[7], sizeof lmem_modes[7], "%s(,%%%sd, 4)", c0, vr0);
  snprintf(lmem_modes[8], sizeof lmem_modes[8], "%s(,%%%sd, 8)", c0, vr0);

  snprintf(lmem_modes[9], sizeof lmem_modes[9], "%s(,%%eiz, 1)", c0);
  unsigned num_mem_modes = 10;

  for (i = 0; i < num_mem_modes; i++) {
    lmem_constraints[i][0] = '\0';  // by default, no additional constraints.
    if (i == 0 || (i >= 5 && i <= 8)) {
      snprintf(lmem_constraints[i], sizeof lmem_constraints[i],
          "%s : R\n", vr0);
    }

    if (i >= 1 && i <= 4) {
      snprintf(lmem_constraints[i], sizeof lmem_constraints[i],
          "%s : R\n%s : R\n", vr0, vr1);
    }
  }

  for (i = 0; i < num_mem_modes; i++) {
    lmem_live_regs[i][0] = '\0';  // by default, no extra live regs.
    if (i == 0 || (i >= 5 && i <= 8)) {
      snprintf(lmem_live_regs[i], sizeof lmem_live_regs[i], "%s, ", vr0);
    }
    if (i >= 1 && i <= 4) {
      snprintf(lmem_live_regs[i], sizeof lmem_live_regs[i], "%s, %s, ", vr0, vr1);
    }
  }

  for (i = 0; i < num_mem_modes; i++) {
    mem_modes[i] = lmem_modes[i];
    mem_constraints[i] = lmem_constraints[i];
    mem_live_regs[i] = lmem_live_regs[i];
  }
  return num_mem_modes;
*/
}

int
i386_gen_mem_modes(char const *mem_modes[], char const *mem_constraints[],
    char const *mem_live_regs[], char const *in_text)
{
  if (!strstr(in_text, "MEM")) {
    mem_modes[0] = "MEM";
    mem_constraints[0] = "";
    mem_live_regs[0] = "";
    return 1;
  }
  if (strstr(in_text, "MEM32")) {
    return gen_mem_modes32(mem_modes, mem_constraints, mem_live_regs, in_text);
  }
  NOT_REACHED();
}

int
i386_gen_jcc_conds(char const *jcc_conds[], char const *in_text)
{
  if (!strstr(in_text, "jCC")) {
    jcc_conds[0] = "jCC";
    return 1;
  }
  jcc_conds[0] = "je";    jcc_conds[1] = "jne";
  jcc_conds[2] = "jg";    jcc_conds[3] = "jle";
  jcc_conds[4] = "jl";    jcc_conds[5] = "jge";
  jcc_conds[6] = "ja";    jcc_conds[7] = "jbe";
  jcc_conds[8] = "jb";    jcc_conds[9] = "jae";
  jcc_conds[10] = "jo";   jcc_conds[11] = "jno";
  jcc_conds[12] = "js";   jcc_conds[13] = "jns";
  jcc_conds[14] = "jp";   jcc_conds[15] = "jnp";
  //jcc_conds[16] = "jpe";  //jpe or parity-even is same as jp
  //jcc_conds[17] = "jpo";  //jpo or parity-odd is same as jnp
  return 16;
}

bool
i386_iseq_can_be_optimized(i386_insn_t const *iseq, int iseq_len)
{
  int i;

/* for (i = 0; i < iseq_len; i++) {
    if (   i386_insn_is_jecxz(&iseq[i])
        || i386_insn_is_indirect_branch(&iseq[i])
        || i386_insn_is_function_call(&iseq[i])) {
      return false;
    }
  }*/
  return true;
}

/*bool
i386_insn_is_not_harvestable(i386_insn_t const *insn, int index, int len)
{
  char const *opc = opctable_name(insn->opc);
  if (!strcmp(opc, "hlt")) {
    return true;
  }
  if (!strcmp(opc, "int")) {
    return true;
  }
  return false;
}*/

bool
i386_insn_get_constant_operand(i386_insn_t const *insn, uint32_t *constant)
{
  int i;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_imm) {
      *constant = insn->op[i].valtag.imm.val;
      return true;
    }
  }
  return false;
}

void
i386_insn_update_stack_pointers(i386_insn_t const *insn, src_ulong *esp, src_ulong *ebp)
{
  char const *opc = opctable_name(insn->opc);
  if (   insn->op[0].type == op_reg && insn->op[0].valtag.reg.val == R_EBP
      && insn->op[1].type == op_reg && insn->op[1].valtag.reg.val == R_ESP
      && strstart(opc, "mov", NULL)) {
    *ebp = *esp;
    return;
  }
  if (   insn->op[0].type == op_reg && insn->op[0].valtag.reg.val == R_ESP
      && insn->op[1].type == op_reg && insn->op[1].valtag.reg.val == R_EBP
      && strstart(opc, "mov", NULL)) {
    *esp = *ebp;
    return;
  }
  if (   insn->op[0].type == op_reg && insn->op[0].valtag.reg.val == R_EBP
      && insn->op[1].type == op_mem && insn->op[1].valtag.mem.base.val == R_ESP
      && insn->op[1].valtag.mem.index.val == -1
      && strstart(opc, "lea", NULL)) {
    ASSERT((insn->op[1].valtag.mem.disp.val % 4) == 0);
    if (*esp != -1) {
      *ebp = *esp + insn->op[1].valtag.mem.disp.val;
    }
    return;
  }
  if (   insn->op[0].type == op_reg && insn->op[0].valtag.reg.val == R_ESP
      && insn->op[1].type == op_mem && insn->op[1].valtag.mem.base.val == R_EBP
      && insn->op[1].valtag.mem.index.val == -1
      && strstart(opc, "lea", NULL)) {
    ASSERT((insn->op[1].valtag.mem.disp.val % 4) == 0);
    if (*ebp != -1) {
      *esp = *ebp + insn->op[1].valtag.mem.disp.val;
    }
    return;
  }

  if (   insn->op[0].type == op_reg
      && (   insn->op[0].valtag.reg.val == R_ESP
          || insn->op[0].valtag.reg.val == R_EBP)) {
    long offset = 0;
    bool done = false;
    if (strstart(opc, "sub", NULL) && insn->op[1].type == op_imm) {
      offset = -insn->op[1].valtag.imm.val;
      done = true;
    } else if (strstart(opc, "add", NULL) && insn->op[1].type == op_imm) {
      offset = insn->op[1].valtag.imm.val;
      done = true;
    }
    if (done) {
      if (insn->op[0].valtag.reg.val == R_ESP && *esp != -1) {
        *esp = *esp + offset;
      } else if (insn->op[0].valtag.reg.val == R_EBP && *ebp != -1) {
        *ebp = *ebp + offset;
      }
      return;
    }
  }
  if (strstart(opc, "push", NULL)) {
    if (*esp != -1) {
      *esp -= 4;
    }
    return;
  }
  if (strstart(opc, "pop", NULL)) {
    if (*esp != -1) {
      *esp += 4;
    }
    if (insn->op[1].type == op_reg) {
      if (insn->op[1].valtag.reg.val == R_ESP) {
        *esp = -1;
      } else if (insn->op[1].valtag.reg.val == R_EBP) {
        *ebp = -1;
      }
    }
    return;
  }
  if (!strcmp(opc, "leave")) {
    *esp = *ebp;
    *ebp = -1;
    return;
  }
  if (!strcmp(opc, "enter")) {
    if (*esp != -1) {
      *esp -= 4;
    }
    *ebp = *esp;
    return;
  }
  if (!i386_insn_accesses_stack(insn)) {
    if (insn->op[0].type == op_reg && insn->op[0].valtag.reg.val == R_EBP) {
      *ebp = -1;
      return;
    }
    if (insn->op[0].type == op_reg && insn->op[0].valtag.reg.val == R_ESP) {
      *esp = -1;
      return;
    }
  }
}

static bool
i386_operand_uses_stack_offset(operand_t const *op, int *stack_reg,
    int *offset)
{
  switch (op->type) {
    case op_mem:
      if (   op->valtag.mem.base.val == R_ESP
          && op->valtag.mem.index.val == -1) {
        *stack_reg = R_ESP;
        *offset = op->valtag.mem.disp.val & ~0x3;
        return true;
      } else if (op->valtag.mem.base.val == R_EBP && op->valtag.mem.index.val == -1) {
        *stack_reg = R_EBP;
        *offset = op->valtag.mem.disp.val & ~0x3;
        return true;
      } else if (   op->valtag.mem.index.val == R_EBP
                 && op->valtag.mem.base.val == -1
                 && op->valtag.mem.scale == 1) {
        *stack_reg = R_EBP;
        *offset = op->valtag.mem.disp.val & ~0x3;
        return true;
      }
    default:
      return false;
  }
}

bool
i386_insn_uses_stack_offset(i386_insn_t const *insn, int *stack_reg,
    int *offset)
{
  char const *opc = opctable_name(insn->opc);
  int i;
  if (strstart(opc, "push", NULL)) {
    *stack_reg = R_ESP;
    *offset = -4;
    return true;
  } else if (strstart(opc, "pop", NULL)) {
    *stack_reg = R_ESP;
    *offset = 0;
    return true;
  } else if (!strstart(opc, "lea", NULL)) {
    for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
      if (i386_operand_uses_stack_offset(&insn->op[i], stack_reg, offset)) {
        return true;
      }
    }
  }
  return false;
}

//bool
//i386_insn_passes_stack_pointer(i386_insn_t const *insn, int *stack_reg,
//    int *offset)
//{
//  char const *opc = opctable_name(insn->opc);
//  int i;
//  if (   strstart(opc, "push", NULL)
//      && insn->op[1].type == op_reg
//      && insn->op[1].valtag.reg.val == R_ESP) {
//    *stack_reg = R_ESP;
//    *offset = 0;
//    /* printf("%s() %d: returning true for '%s'", __func__, __LINE__,
//        i386_insn_to_string(insn, as1, sizeof as1)); */
//    return true;
//  }
//  if (   insn->op[1].type == op_reg
//      && (   insn->op[1].valtag.reg.val == R_ESP
//          || insn->op[1].valtag.reg.val == R_EBP)) {
//    if (!(   insn->op[0].type == op_reg
//          && (   insn->op[0].valtag.reg.val == R_EBP
//              || insn->op[0].valtag.reg.val == R_ESP))) {
//      *stack_reg = insn->op[0].valtag.reg.val;
//      *offset = 0;
//      /* printf("%s() %d: returning true for '%s'", __func__, __LINE__,
//          i386_insn_to_string(insn, as1, sizeof as1)); */
//      return true;
//    }
//  }
//
//  if (   insn->op[1].type == op_mem
//      && (   insn->op[1].valtag.mem.base.val == R_ESP
//          || insn->op[1].valtag.mem.base.val == R_EBP)
//      && strstart(opc, "lea", NULL)) {
//    if (!(   insn->op[0].type == op_reg
//          && (   insn->op[0].valtag.reg.val == R_EBP
//              || insn->op[0].valtag.reg.val == R_ESP))) {
//      *stack_reg = insn->op[1].valtag.mem.base.val;
//      *offset = insn->op[1].valtag.mem.disp.val & ~0x3;
//      /* printf("%s() %d: returning true for '%s'", __func__, __LINE__,
//          i386_insn_to_string(insn, as1, sizeof as1)); */
//      return true;
//    }
//  }
//  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
//    if (   insn->op[i].type == op_mem
//        && (   insn->op[i].valtag.mem.base.val == R_ESP
//            || insn->op[i].valtag.mem.base.val == R_EBP)) {
//      if (   insn->op[i].valtag.mem.index.val != -1
//          || !strcmp(opc, "fldt")
//          || !strcmp(opc, "fldl")
//          || !strcmp(opc, "fstpt")
//          || !strcmp(opc, "fstl")
//          || !strcmp(opc, "fstpl")
//          || !strcmp(opc, "faddl")
//          || !strcmp(opc, "fcoml")
//          || !strcmp(opc, "fcompl")
//          || !strcmp(opc, "fdivl")
//          || !strcmp(opc, "fdivrl")
//          || !strcmp(opc, "fmull")
//          || !strcmp(opc, "fsubl")
//          || !strcmp(opc, "fsubrl")) {
//        *stack_reg = insn->op[i].valtag.mem.base.val;
//        *offset = insn->op[i].valtag.mem.disp.val & ~0x3;
//        /* printf("%s() %d: returning true for '%s'", __func__, __LINE__,
//           i386_insn_to_string(insn, as1, sizeof as1)); */
//        return true;
//      }
//    }
//  }
//  return false;
//}

#define ADJUST_STACK_OFFSET(loc, base, adj) do { \
  ASSERT((adj) >= 0); \
  if (((int)(loc)) > 0) { \
    loc -= (adj); \
  } else if (((int)(loc)) < 0) { \
    loc += (adj); \
  } \
} while (0)

#if 0
int
i386_insn_replace_stack_instruction(i386_insn_t *iseq, int iseq_size,
    i386_insn_t const *insn, uint32_t esp, uint32_t ebp, unsigned long function_base,
    struct clist const *regalloced_stack_offsets)
{
  char const *opc;
  int i, iseq_len = 1;

  opc = opctable_name(insn->opc);
  memcpy(&iseq[0], insn, sizeof(i386_insn_t));
  DBG2(ANALYZE_STACK, "esp = %x, ebp = %x, insn = '%s'\n", esp, ebp,
      i386_insn_to_string(insn, as1, sizeof as1));
  if (esp != -1) {
    if (   strstart(opc, "push", NULL)
        && stack_offsets_belongs(regalloced_stack_offsets, esp - 4)) {
      if (insn->op[1].type == op_reg || insn->op[1].type == op_imm) {
        ASSERT(insn->op[0].type == op_reg && insn->op[0].valtag.reg.val == R_ESP);
        iseq[0].opc = opc_nametable_find("movl");
        iseq[0].op[0].type = op_mem;
        iseq[0].op[0].valtag.mem.base.val = iseq[0].op[0].valtag.mem.index.val = -1;
        iseq[0].op[0].valtag.mem.disp.val = function_base + esp - 4;
        iseq[0].op[0].valtag.mem.seg.sel.val = R_DS;

        i386_insn_init(&iseq[1]);
        iseq[1].opc = opc_nametable_find("lea");
        iseq[1].op[0].size = 4;
        iseq[1].op[0].type = op_reg;
        iseq[1].op[0].valtag.reg.val = R_ESP;
        iseq[1].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
        iseq[1].op[1].size = 4;
        iseq[1].op[1].type = op_mem;
        iseq[1].op[1].valtag.mem.addrsize = 4;
        iseq[1].op[1].valtag.mem.segtype = segtype_sel;
        iseq[1].op[1].valtag.mem.seg.sel.val = R_SS;
        iseq[1].op[1].valtag.mem.base.val = R_ESP;
        iseq[1].op[1].valtag.mem.base.mod.reg.mask = MAKE_MASK(DWORD_LEN);
        iseq[1].op[1].valtag.mem.index.val = -1;
        iseq[1].op[1].valtag.mem.disp.val = -4;
        iseq[1].op[1].valtag.mem.disp.tag = tag_const;
        iseq_len = 2;
      }
    }
    if (   strstart(opc, "pop", NULL)
        && stack_offsets_belongs(regalloced_stack_offsets, esp)) {
      if (insn->op[1].type == op_reg || insn->op[1].type == op_imm) {
        ASSERT(insn->op[0].type == op_reg && insn->op[0].valtag.reg.val == R_ESP);
        memcpy(&iseq[0].op[0], &iseq[0].op[1], sizeof(operand_t));
        iseq[0].opc = opc_nametable_find("movl");
        iseq[0].op[1].type = op_mem;
        iseq[0].op[1].valtag.mem.base.val = iseq[0].op[1].valtag.mem.index.val = -1;
        iseq[0].op[1].valtag.mem.disp.val = function_base + esp;
        iseq[0].op[1].valtag.mem.seg.sel.val = R_DS;

        i386_insn_init(&iseq[1]);
        iseq[1].opc = opc_nametable_find("lea");
        iseq[1].op[0].size = 4;
        iseq[1].op[0].type = op_reg;
        iseq[1].op[0].valtag.reg.val = R_ESP;
        iseq[1].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
        iseq[1].op[1].size = 4;
        iseq[1].op[1].type = op_mem;
        iseq[1].op[1].valtag.mem.addrsize = 4;
        iseq[1].op[1].valtag.mem.segtype = segtype_sel;
        iseq[1].op[1].valtag.mem.seg.sel.val = R_DS;
        iseq[1].op[1].valtag.mem.base.val = R_ESP;
        iseq[1].op[1].valtag.mem.base.mod.reg.mask = MAKE_MASK(DWORD_LEN);
        iseq[1].op[1].valtag.mem.index.val = -1;
        iseq[1].op[1].valtag.mem.disp.val = 4;
        iseq_len = 2;
      }
    }
    if (   !strcmp(opc, "enter")
        && stack_offsets_belongs(regalloced_stack_offsets, esp - 4)) {
      iseq[0].opc = opc_nametable_find("movl");
      iseq[0].op[0].type = op_mem;
      iseq[0].op[0].valtag.mem.base.val = iseq[0].op[0].valtag.mem.index.val = -1;
      iseq[0].op[0].valtag.mem.disp.val = function_base + esp - 4;
      iseq[0].op[0].valtag.mem.segtype = segtype_sel;
      iseq[0].op[0].valtag.mem.seg.sel.val = R_DS;
      iseq[0].op[0].size = 4;
      iseq[0].op[1].type = op_reg;
      iseq[0].op[1].valtag.reg.val = R_EBP;
      iseq[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
      iseq[0].op[1].size = 4;

      i386_insn_init(&iseq[1]);
      iseq[1].opc = opc_nametable_find("movl");
      iseq[1].op[0].type = op_reg;
      iseq[1].op[0].size = 4;
      iseq[1].op[0].valtag.reg.val = R_EBP;
      iseq[1].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
      iseq[1].op[1].type = op_reg;
      iseq[1].op[1].valtag.reg.val = R_ESP;
      iseq[1].op[1].size = 4;
      iseq[1].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);

      i386_insn_init(&iseq[2]);
      iseq[2].opc = opc_nametable_find("lea");
      iseq[2].op[0].size = 4;
      iseq[2].op[0].type = op_reg;
      iseq[2].op[0].valtag.reg.val = R_ESP;
      iseq[2].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
      iseq[2].op[1].size = 4;
      iseq[2].op[1].type = op_mem;
      iseq[2].op[1].valtag.mem.addrsize = 4;
      iseq[2].op[1].valtag.mem.segtype = segtype_sel;
      iseq[2].op[1].valtag.mem.seg.sel.val = R_DS;
      iseq[2].op[1].valtag.mem.base.val = R_ESP;
      iseq[2].op[1].valtag.mem.base.mod.reg.mask = MAKE_MASK(DWORD_LEN);
      iseq[2].op[1].valtag.mem.index.val = -1;
      iseq[2].op[1].valtag.mem.disp.val = -4;

      iseq_len = 3;
    }
  }

  if (ebp != -1) {
    if (   !strcmp(opc, "leave")
        && stack_offsets_belongs(regalloced_stack_offsets, ebp)) {
      iseq[0].opc = opc_nametable_find("movl");
      iseq[0].op[0].type = op_reg;
      iseq[0].op[0].valtag.reg.val = R_ESP;
      iseq[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
      iseq[0].op[0].size = 4;
      iseq[0].op[1].type = op_reg;
      iseq[0].op[1].valtag.reg.val = R_EBP;
      iseq[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
      iseq[0].op[1].size = 4;
      iseq[0].op[2].type = invalid;

      i386_insn_init(&iseq[1]);
      iseq[1].opc = opc_nametable_find("movl");
      iseq[1].op[0].type = op_reg;
      iseq[1].op[0].valtag.reg.val = R_EBP;
      iseq[1].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
      iseq[1].op[0].size = 4;
      iseq[1].op[1].type = op_mem;
      iseq[1].op[1].valtag.mem.base.val = iseq[1].op[1].valtag.mem.index.val = -1;
      iseq[1].op[1].valtag.mem.disp.val = function_base + ebp;
      iseq[1].op[1].valtag.mem.segtype = segtype_sel;
      iseq[1].op[1].valtag.mem.seg.sel.val = R_DS;
      iseq[1].op[1].size = 4;
      iseq[1].op[2].type = invalid;

      i386_insn_init(&iseq[2]);
      iseq[2].opc = opc_nametable_find("lea");
      iseq[2].op[0].size = 4;
      iseq[2].op[0].type = op_reg;
      iseq[2].op[0].valtag.reg.val = R_ESP;
      iseq[2].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
      iseq[2].op[1].size = 4;
      iseq[2].op[1].type = op_mem;
      iseq[2].op[1].valtag.mem.addrsize = 4;
      iseq[2].op[1].valtag.mem.segtype = segtype_sel;
      iseq[2].op[1].valtag.mem.seg.sel.val = R_DS;
      iseq[2].op[1].valtag.mem.base.val = R_ESP;
      iseq[2].op[1].valtag.mem.index.val = -1;
      iseq[2].op[1].valtag.mem.disp.val = 4;

      iseq_len = 3;
    }
  }

  int adjustment;
  if (   (strstart(opc, "sub", NULL) || strstart(opc, "add", NULL))
      && insn->op[0].type == op_reg
      && (   (insn->op[0].valtag.reg.val == R_ESP && esp != -1)
          || (insn->op[0].valtag.reg.val == R_EBP && ebp != -1))
      && insn->op[1].type == op_imm) {
    src_ulong base;
    int off;
    if (insn->op[0].valtag.reg.val == R_ESP) {
      base = esp;
    } else {
      base = ebp;
    }
    if (strstart(opc, "sub", NULL)) {
      off = -iseq[0].op[1].valtag.imm.val;
    } else {
      off = iseq[0].op[1].valtag.imm.val;
    }
    /*adjustment = adjust_stack_offset(regalloced_stack_offsets, base,
        base + off);
    ADJUST_STACK_OFFSET(iseq[0].op[1].val.imm, base, adjustment); */
  }

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (   esp != -1
        && !i386_insn_accesses_stack(insn)
        && insn->op[i].type == op_mem
        && insn->op[i].valtag.mem.base.val == R_ESP
        && insn->op[i].valtag.mem.index.val == -1) {
      if (   !strstart(opc, "lea", NULL)
          && stack_offsets_belongs(regalloced_stack_offsets,
              insn->op[i].valtag.mem.disp.val + esp)) {
        iseq[0].op[i].valtag.mem.base.val = -1;
        iseq[0].op[i].valtag.mem.disp.val = function_base + esp
          + insn->op[i].valtag.mem.disp.val;
      } else {
        /*adjustment = adjust_stack_offset(regalloced_stack_offsets,
            esp, esp + insn->op[i].val.mem.disp);
        ADJUST_STACK_OFFSET(iseq[0].op[i].val.mem.disp, esp, adjustment); */
      }
    }
    if (   ebp != -1
        && !i386_insn_accesses_stack(insn)
        && insn->op[i].type == op_mem
        && insn->op[i].valtag.mem.base.val == R_EBP
        && insn->op[i].valtag.mem.index.val == -1) {
      if (   !strstart(opc, "lea", NULL)
          && stack_offsets_belongs(regalloced_stack_offsets,
              (insn->op[i].valtag.mem.disp.val & ~3) + ebp)) {
        /*XXX : check that the instruction operates on less than 4 - (disp%4) bytes. */
        iseq[0].op[i].valtag.mem.base.val = -1;
        iseq[0].op[i].valtag.mem.disp.val = function_base + ebp
          + insn->op[i].valtag.mem.disp.val;
      } else {
        /* adjustment = adjust_stack_offset(regalloced_stack_offsets,
            ebp, ebp + insn->op[i].val.mem.disp);
        ADJUST_STACK_OFFSET(iseq[0].op[i].val.mem.disp, ebp, adjustment); */
      }
    }
  }
  return iseq_len;
}
#endif

int
i386_push_mem_insn(src_ulong disp, uint8_t *buf)
{
  uint8_t *ptr = buf;
  *ptr++ = 0xff;
  *ptr++ = 0x35;
  *(uint32_t *)ptr = disp;
  ptr += 4;
  return ptr - buf;
}

int
i386_popn_insn(int n, uint8_t *buf)
{
  uint8_t *ptr = buf;
  *ptr++ = 0x81;
  *ptr++ = 0xc4;
  *(uint32_t *)ptr = n;
  ptr += 4;
  return ptr - buf;
}

int
i386_pushn_insn(int n, uint8_t *buf)
{
  uint8_t *ptr = buf;
  *ptr++ = 0x81;
  *ptr++ = 0xec;
  *(uint32_t *)ptr = n;
  ptr += 4;
  return ptr - buf;
}

int
i386_mov_mem_insn(src_ulong disp1, src_ulong disp2, uint8_t *buf)
{
  uint8_t *ptr = buf;
  /* mov %eax, SRC_ENV_SCRATCH0. */
  *ptr++ = 0x89;   *ptr++ = 0x05;   *(uint32_t *)ptr = SRC_ENV_SCRATCH0;   ptr += 4;
  /* mov DISP1, %eax. */
  *ptr++ = 0x8b;   *ptr++ = 0x05;   *(uint32_t *)ptr = disp1;   ptr += 4;
  /* mov %eax, DISP2. */
  *ptr++ = 0x89;   *ptr++ = 0x05;   *(uint32_t *)ptr = disp2;   ptr += 4;
  /* mov SRC_ENV_SCRATCH0, %eax. */
  *ptr++ = 0x8b;   *ptr++ = 0x05;   *(uint32_t *)ptr = SRC_ENV_SCRATCH0;   ptr += 4;
  return ptr - buf;
}

int
i386_mov_stack_mem_insn(int offset, src_ulong disp, uint8_t *buf)
{
  uint8_t *ptr = buf;
  /* mov %eax, SRC_ENV_SCRATCH0. */
  *ptr++ = 0x89;   *ptr++ = 0x05;   *(uint32_t *)ptr = SRC_ENV_SCRATCH0;   ptr += 4;
  /* mov OFFSET(%esp), %eax. */
  *ptr++ = 0x8b;  *ptr++ = 0x84;  *ptr++ = 0x24;  *(uint32_t *)ptr = offset;  ptr += 4;
  /* mov %eax, DISP2. */
  *ptr++ = 0x89;   *ptr++ = 0x05;   *(uint32_t *)ptr = disp;   ptr += 4;
  /* mov SRC_ENV_SCRATCH0, %eax. */
  *ptr++ = 0x8b;   *ptr++ = 0x05;   *(uint32_t *)ptr = SRC_ENV_SCRATCH0;   ptr += 4;
  return ptr - buf;
}

int
i386_mov_mem_stack_insn(src_ulong disp, int offset, uint8_t *buf)
{
  uint8_t *ptr = buf;
  /* mov %eax, SRC_ENV_SCRATCH0. */
  *ptr++ = 0x89;   *ptr++ = 0x05;   *(uint32_t *)ptr = SRC_ENV_SCRATCH0;   ptr += 4;
  /* mov DISP2, %eax. */
  *ptr++ = 0x8b;   *ptr++ = 0x05;   *(uint32_t *)ptr = disp;   ptr += 4;
  /* mov %eax, OFFSET(%esp). */
  *ptr++ = 0x89;  *ptr++ = 0x84;  *ptr++ = 0x24;  *(uint32_t *)ptr = offset;  ptr += 4;
  /* mov SRC_ENV_SCRATCH0, %eax. */
  *ptr++ = 0x8b;   *ptr++ = 0x05;   *(uint32_t *)ptr = SRC_ENV_SCRATCH0;   ptr += 4;
  return ptr - buf;
}


int
i386_push_stack_insn(int offset, uint8_t *buf)
{
  uint8_t *ptr = buf;
  /* pushl OFFSET(%esp). */
  *ptr++ = 0xff;   *ptr++ = 0xb4;   *ptr++ = 0x24;
  *(uint32_t *)ptr = offset;  ptr += 4;
  return ptr - buf;
}

/*int
i386_insn_convert_branch_to_function_call(i386_insn_t *iseq, int iseq_size,
    i386_insn_t const *insn)
{
  memcpy(&iseq[0], insn, sizeof(i386_insn_t));
  iseq[0].opc = opc_nametable_find("calll");
  memcpy(&iseq[0].op[1], &insn->op[0], sizeof(operand_t));
  iseq[0].op[0].type = op_reg;
  iseq[0].op[0].val.reg = R_ESP;
  ASSERT(iseq[0].op[1].type == op_pcrel);
  iseq[0].op[1].val.pcrel = EXT_JUMP_ID0;
  iseq[0].op[1].tag.pcrel = tag_var;
  return 1;
}*/

int
i386_function_return_insn(uint8_t *buf)
{
  uint8_t *ptr = buf;
  *ptr++ = 0xc3;
  return ptr - buf;
}

bool
i386_insn_update_induction_reg_values(i386_insn_t const *insn,
    induction_reg_t *regs, long max_regs, long *nregs)
{
  NOT_IMPLEMENTED();
/*
  regset_t use, def, touch;
  long i;

  i386_insn_get_usedef_regs(insn, &use, &def);
  regset_copy(&touch, &def);
  regset_union(&touch, &use);
  for (i = 0; i < I386_NUM_GPRS; i++) {
    long j;
    if (!touch.cregs[i]) {
      continue;
    }
    for (j = 0; j < *nregs; j++) {
      if (regs[j].name == i) {
        break;
      }
    }
    if (j == *nregs) {
      if (*nregs >= max_regs) {
        return false;
      }
      regs[*nregs].name = i;
      regs[*nregs].value = 0;
      regs[*nregs].valid = true;
      (*nregs)++;
    }
  }
  char const *opc = opctable_name(insn->opc);
  for (i = 0; i < *nregs; i++) {
    if (!regs[i].valid) {
      continue;
    }
    long r = regs[i].name;
    if (!touch.cregs[r]) {
      continue;
    }
    if (   strstart(opc, "add", NULL)
        && insn->op[0].type == op_reg
        && insn->op[0].valtag.reg.val == r
        && insn->op[1].type == op_imm) {
      regs[i].value += insn->op[1].valtag.imm.val;
    } else if (strstart(opc, "sub", NULL)
        && insn->op[0].type == op_reg
        && insn->op[0].valtag.reg.val == r
        && insn->op[1].type == op_imm) {
      regs[i].value -= insn->op[1].valtag.imm.val;
    } else if (strstart(opc, "inc", NULL)
        && insn->op[0].type == op_reg
        && insn->op[0].valtag.reg.val == r) {
      regs[i].value++;
    } else if (strstart(opc, "dec", NULL)
        && insn->op[0].type == op_reg
        && insn->op[0].valtag.reg.val == r) {
      regs[i].value++;
    } else if (!strcmp(opc, "lea")
        && insn->op[0].type == op_reg
        && insn->op[0].valtag.reg.val == r
        && insn->op[1].type == op_mem
        && insn->op[1].valtag.mem.base.val == r
        && insn->op[1].valtag.mem.index.val == -1) {
      regs[i].value += insn->op[1].valtag.mem.disp.val;
    } else if (   writes_operand(insn, 0)
        && insn->op[0].type == op_reg
        && insn->op[0].valtag.reg.val == r) {
      DBG2(UNROLL_LOOPS, "Setting regs[%ld].valid (r %ld)to false due to instruction"
          " '%s'\n", (long)i, (long)r, i386_insn_to_string(insn, as1, sizeof as1));
      regs[i].valid = false;
    }
  }
  return true;
*/
}

static bool
obtain_induction_vals(induction_reg_t const *regs, int num_regs, int reg,
    src_ulong *induction_reg, long *induction_val)
{
  int i;
  for (i = 0; i < num_regs; i++) {
    if (regs[i].name == reg) {
      if (!regs[i].valid) {
        return false;
      }
      *induction_reg = reg;
      *induction_val = regs[i].value;
      return true;
    }
  }
  return false;
}

static char const *cmp_ops[] = {"jge", "jl", "jg", "jle", "ja", "jbe", "jae", "jb"};
void
i386_insn_invert_cmp_operator(char *out, size_t outsize, char const *in)
{
  int i;
  for (i = 0; i < NELEM(cmp_ops); i++) {
    if (!strcmp(in, cmp_ops[i])) {
      snprintf(out, outsize, "%s", cmp_ops[2*(i/2)+1-(i%2)]);
      return;
    }
  }
  NOT_REACHED();
}

bool
i386_insn_comparison_uses_induction_regs(i386_insn_t const *branch_insn,
    i386_insn_t const *cmp_insn, induction_reg_t const *regs, int num_regs,
    bool branch_target_is_loop_head, src_ulong *induction_reg1, long *induction_val1,
    src_ulong *induction_reg2, long *induction_val2, char *cmp_operator,
    size_t cmp_operator_size)
{
  ASSERT(i386_insn_is_conditional_direct_branch(branch_insn));
  char const *opc = opctable_name(branch_insn->opc);
  int i;
  for (i = 0; i < NELEM(cmp_ops); i++) {
    if (!strcmp(opc, cmp_ops[i])) {
      break;
    }
  }
  if (   i == NELEM(cmp_ops)
      && strcmp(opc, "je")
      && strcmp(opc, "jne")) {
    DBG2(UNROLL_LOOPS, "%s", "returning false\n");
    return false;
  }
  char const *cmp_opc = opctable_name(cmp_insn->opc);
  if (!strstart(cmp_opc, "cmp", NULL) && !strstart(cmp_opc, "test", NULL)) {
    DBG2(UNROLL_LOOPS, "%s", "returning false\n");
    return false;
  }
  int reg0 = -1, reg1 = -1;
  int val0 = 0, val1 = 0;
  ASSERT(cmp_insn->op[0].type == op_reg || cmp_insn->op[0].type == op_mem);
  if (cmp_insn->op[0].type == op_reg) {
    reg0 = cmp_insn->op[0].valtag.reg.val;
  }
  if (cmp_insn->op[0].type == op_mem) {
    if (   cmp_insn->op[0].valtag.mem.base.val != -1
        || cmp_insn->op[0].valtag.mem.index.val != -1) {
      DBG2(UNROLL_LOOPS, "%s", "returning false\n");
      return false;
    }
    reg0 = cmp_insn->op[0].valtag.mem.disp.val;
  }
  ASSERT(reg0 != -1);
  if (!obtain_induction_vals(regs, num_regs, reg0, induction_reg1, induction_val1)) {
    DBG2(UNROLL_LOOPS, "%s", "returning false\n");
    return false;
  }
  if (cmp_insn->op[1].type == op_reg) {
    reg1 = cmp_insn->op[1].valtag.reg.val;
  }
  if (cmp_insn->op[1].type == op_mem) {
    if (   cmp_insn->op[1].valtag.mem.base.val != -1
        || cmp_insn->op[1].valtag.mem.index.val != -1) {
      DBG2(UNROLL_LOOPS, "%s", "returning false\n");
      return false;
    }
    reg1 = cmp_insn->op[0].valtag.mem.disp.val;
  }
  if (!obtain_induction_vals(regs, num_regs, reg1, induction_reg2, induction_val2)) {
    DBG2(UNROLL_LOOPS, "%s", "returning false\n");
    return false;
  }
  int ival1 = *induction_val1, ival2 = *induction_val2;
  if (cmp_insn->op[1].type == op_imm) {
    *induction_reg2 = INDUCTION_REG_IMM;
    *induction_val2 = cmp_insn->op[1].valtag.imm.val;
    ival2 = 0;
  }
  for (i = 0; i < NELEM(cmp_ops); i++) {
    if (!strcmp(opc, cmp_ops[i])) {
      if (branch_target_is_loop_head) {
        i386_insn_invert_cmp_operator(cmp_operator, cmp_operator_size, opc);
      } else {
        snprintf(cmp_operator, cmp_operator_size, "%s", opc);
      }
      return true;
    }
  }

  if (!strcmp(opc, "je") && ABS(ival1 - ival2) == 1) {
    if (branch_target_is_loop_head) {
      return false;
    } else {
      if ((ival1 - ival2) == 1) {
        snprintf(cmp_operator, cmp_operator_size, "jae");
      } else {
        snprintf(cmp_operator, cmp_operator_size, "jbe");
      }
      return true;
    }
  }
  if (!strcmp(opc, "jne") && ABS(ival1 - ival2) == 1) {
    if (branch_target_is_loop_head) {
      if ((ival1 - ival2) == 1) {
        snprintf(cmp_operator, cmp_operator_size, "jae");
      } else {
        snprintf(cmp_operator, cmp_operator_size, "jbe");
      }
      return true;
    } else {
      DBG2(UNROLL_LOOPS, "%s", "returning false\n");
      return false;
    }
  }
  DBG2(UNROLL_LOOPS, "returning false, opc=%s, ival1=%d, ival2=%d\n", opc, ival1, ival2);
  return false;
}

int
i386_add_to_induction_reg_insn(uint8_t *buf, int reg, int val, int num_unrolls)
{
  uint8_t *ptr, *start;
  uint8_t lbuf[16];
  if (buf) {
    ptr = buf;
  } else {
    ptr = lbuf;
  }
  start = ptr;
  if (reg == INDUCTION_REG_IMM) {
    return 0;
  }
  if (val == 0) {
    return 0;
  }
  if (reg < I386_NUM_GPRS) {
    *ptr++ = 0x8d;
    *ptr++ = 0x80 | (reg << 3) | reg;
    *(uint32_t *)ptr = val * num_unrolls;
    ptr += 4;
    return ptr - start;
 } else {
   NOT_IMPLEMENTED();
 }
}

int
i386_cmp_insn_for_induction_regs(uint8_t *buf, int reg1, int reg2, int val1, int val2)
{
  uint8_t *ptr = buf;
  ASSERT(reg1 != INDUCTION_REG_IMM);
  if (reg2 == INDUCTION_REG_IMM) {
    *ptr++ = 0x81;
    *ptr++ = 0xf8 | reg1;
    *(uint32_t *)ptr = val2;
    ptr += 4;
    return ptr - buf;
  }
  if (reg1 < I386_NUM_GPRS && reg2 < I386_NUM_GPRS) {
    ASSERT(reg1 >= 0 && reg2 >= 0);
    *ptr++ = 0x39;
    *ptr++ = 0xc0 | (reg2 << 3) | reg1;
    return ptr - buf;
  }
  if (reg1 < I386_NUM_GPRS) {
    ASSERT(reg2 > I386_NUM_GPRS);
    *ptr++ = 0x39;
    *ptr++ = 0x05 | (reg1 << 3);
    *(uint32_t *)ptr = reg2;
    ptr += 4;
    return ptr - buf;
  }
  if (reg2 < I386_NUM_GPRS) {
    ASSERT(reg1 > I386_NUM_GPRS);
    *ptr++ = 0x3b;
    *ptr++ = 0x05 | (reg1 << 3);
    *(uint32_t *)ptr = reg2;
    ptr += 4;
    return ptr - buf;
  }
  NOT_IMPLEMENTED();
}

int
i386_unroll_loop_jcc_insn(uint8_t *buf, char const *cmp_operator, uint32_t offset)
{
  NOT_IMPLEMENTED();
/*
  uint8_t *ptr = buf;
  i386_insn_t insn;
  i386_insn_init(&insn);
  insn.opc = opc_nametable_find(cmp_operator);
  insn.op[0].type = op_pcrel;
  insn.op[0].val.pcrel = offset;
  insn.op[0].tag.pcrel = tag_const;
  ptr += i386_insn_assemble(&insn, ptr, I386_MAX_INSN_SIZE);
  ASSERT(ptr > buf);
  return ptr - buf;
*/
}

int
i386_unroll_loop_jmp_insn(uint8_t *buf, uint32_t offset)
{
  uint8_t *ptr, *start;
  uint8_t lbuf[16];
  if (buf) {
    ptr = buf;
  } else {
    ptr = lbuf;
  }
  start = ptr;
  *ptr++ = 0xe9;
  *(uint32_t *)ptr = offset;
  ptr += 4;
  return ptr - start;
}

/*bool
i386_insn_is_int(i386_insn_t const *insn)
{
  char const *opc = opctable_name(insn->opc);
  if (!strcmp(opc, "int")) {
    return true;
  }
  return false;
}*/

void
i386_insn_add_to_pcrels(i386_insn_t *insn, src_ulong pc, bbl_t const *bbl, long bblindex)
{
  valtag_t *op;
  if (op = i386_insn_get_pcrel_operand(insn)) {
    ASSERT(op->tag == tag_const || op->tag == tag_input_exec_reloc_index);
    if (op->tag == tag_const) {
      /*printf("%s(): pc=%lx, op->val.pcrel=%lx, sum=%lx\n", __func__, (long)pc,
          (long)op->val.pcrel,
          (long)pc + op->val.pcrel);*/
      op->val += pc;
      op->val = (src_ulong)op->val;
      if (bbl && (bblindex == bbl->num_insns - 1)) {
        int i;
        for (i = 0; i < bbl->num_nextpcs; i++) {
          if (pc_get_orig_pc(bbl->nextpcs[i].get_val()) == pc_get_orig_pc(op->val)) {
            op->val = bbl->nextpcs[i].get_val();
          }
        }
      }
    }
  }
}

long
i386_insn_put_jump_to_pcrel(nextpc_t const& target, i386_insn_t *insns, size_t size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + size;

  i386_insn_init(insn_ptr);
  insn_ptr->opc = opc_nametable_find("jmp");
  insn_ptr->op[0].type = op_pcrel;
  insn_ptr->op[0].valtag.pcrel.val = target.get_val();
  insn_ptr->op[0].valtag.pcrel.tag = target.get_tag();
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

long i386_insn_put_jump_to_abs_inum(src_ulong target, i386_insn_t *insns,
    size_t size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + size;

  i386_insn_init(insn_ptr);
  insn_ptr->opc = opc_nametable_find("jmp");
  insn_ptr->op[0].type = op_pcrel;
  insn_ptr->op[0].valtag.pcrel.val = target;
  insn_ptr->op[0].valtag.pcrel.tag = tag_abs_inum;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

long
i386_insn_put_jcc_to_nextpc(char const *jcc, int nextpc_num, i386_insn_t *insns,
    size_t size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + size;

  i386_insn_init(insn_ptr);
  insn_ptr->opc = opc_nametable_find(jcc);
  insn_ptr->op[0].type = op_pcrel;
  insn_ptr->op[0].valtag.pcrel.val = nextpc_num;
  insn_ptr->op[0].valtag.pcrel.tag = tag_var;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

long
i386_insn_put_jcc_to_pcrel(char const *jcc, src_ulong target, i386_insn_t *insns,
    size_t size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + size;

  i386_insn_init(insn_ptr);
  insn_ptr->opc = opc_nametable_find(jcc);
  /*insn_ptr->op[0].type = op_flags_unsigned;
  insn_ptr->op[0].valtag.flags_unsigned.val = 0;
  insn_ptr->op[0].valtag.flags_unsigned.tag = tag_const;
  insn_ptr->op[1].type = op_flags_signed;
  insn_ptr->op[1].valtag.flags_signed.val = 0;
  insn_ptr->op[1].valtag.flags_signed.tag = tag_const;*/
  insn_ptr->op[0].type = op_pcrel;
  insn_ptr->op[0].valtag.pcrel.val = target;
  insn_ptr->op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

long
i386_insn_put_jump_to_nextpc(i386_insn_t *insns, size_t size, long nextpc_num)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insn_ptr + size;

  i386_insn_init(insn_ptr);
  insn_ptr->opc = opc_nametable_find("jmp");
  insn_ptr->op[0].type = op_pcrel;
  insn_ptr->op[0].valtag.pcrel.val = nextpc_num;
  insn_ptr->op[0].valtag.pcrel.tag = tag_var;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

long
i386_insn_put_jump_to_src_insn_pc(i386_insn_t *insns, size_t size, src_ulong src_insn_pc)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insn_ptr + size;

  i386_insn_init(insn_ptr);
  insn_ptr->opc = opc_nametable_find("jmp");
  insn_ptr->op[0].type = op_pcrel;
  insn_ptr->op[0].valtag.pcrel.val = src_insn_pc;
  insn_ptr->op[0].valtag.pcrel.tag = tag_src_insn_pc;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

void
i386_insn_set_nextpc(i386_insn_t *insn, long nextpc_num)
{
  valtag_t *op;

  if (op = i386_insn_get_pcrel_operand(insn)) {
    op->tag = tag_var;
    op->val = nextpc_num;
  }
}

long
i386_insn_put_nop(i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(insn_ptr);
  insn_ptr->opc = opc_nametable_find("xchgl");
  insn_ptr->op[0].type = op_reg;
  insn_ptr->op[0].valtag.reg.val = R_EAX;
  insn_ptr->op[0].valtag.reg.tag = tag_const;
  insn_ptr->op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr->op[0].size = DST_LONG_SIZE;
  insn_ptr->op[1].type = op_reg;
  insn_ptr->op[1].valtag.reg.val = R_EAX;
  insn_ptr->op[1].valtag.reg.tag = tag_const;
  insn_ptr->op[1].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr->op[1].size = DST_LONG_SIZE;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

/*void
i386_insn_put_invalid(i386_insn_t *insn)
{
  i386_insn_init(insn);
  insn->opc = opc_inval;
  insn->num_implicit_ops = 0;
}*/

static size_t
i386_opcode_to_int_array(char const *opcode, int64_t arr[], size_t len)
{
  int i;
  for (i = 0; i < strlen(opcode); i++) {
    arr[i] = opcode[i];
    ASSERT(i < len);
  }
  arr[i] = '\0';
  i++;
  ASSERT(i < len);
  return i;
}

static size_t
i386_operand_to_int_array(operand_t const *op, int64_t arr[], size_t len)
{
#define I386_OP_TO_INT_ARRAY(field) \
  case op_##field: \
    cur += valtag_to_int_array(&op->valtag.field, &arr[cur], end - cur); \
    ASSERT(cur < end); \
    break;

  int cur = 0, end = len;
  int i;

  ASSERT(   op->type != op_reg
         || op->size != 1
         || op->valtag.reg.mod.reg.mask == 0xff
         || op->valtag.reg.mod.reg.mask == 0xff00);
  arr[cur++] = op->type;
  arr[cur++] = op->size;
  arr[cur++] = op->rex_used;
  switch (op->type) {
    case invalid:
      break;
    I386_OP_TO_INT_ARRAY(reg);
    I386_OP_TO_INT_ARRAY(seg);
    case op_mem:
      cur += memvt_to_int_array(&op->valtag.mem, &arr[cur], end - cur);
      break;
    I386_OP_TO_INT_ARRAY(imm);
    I386_OP_TO_INT_ARRAY(pcrel);
    I386_OP_TO_INT_ARRAY(cr);
    I386_OP_TO_INT_ARRAY(db);
    I386_OP_TO_INT_ARRAY(tr);
    I386_OP_TO_INT_ARRAY(mmx);
    I386_OP_TO_INT_ARRAY(xmm);
    I386_OP_TO_INT_ARRAY(ymm);
    I386_OP_TO_INT_ARRAY(d3now);
    I386_OP_TO_INT_ARRAY(prefix);
    I386_OP_TO_INT_ARRAY(st);
    //I386_OP_TO_INT_ARRAY(flags_unsigned);
    //I386_OP_TO_INT_ARRAY(flags_signed);
    I386_OP_TO_INT_ARRAY(flags_other);
    I386_OP_TO_INT_ARRAY(flags_eq);
    I386_OP_TO_INT_ARRAY(flags_ne);
    I386_OP_TO_INT_ARRAY(flags_ult);
    I386_OP_TO_INT_ARRAY(flags_slt);
    I386_OP_TO_INT_ARRAY(flags_ugt);
    I386_OP_TO_INT_ARRAY(flags_sgt);
    I386_OP_TO_INT_ARRAY(flags_ule);
    I386_OP_TO_INT_ARRAY(flags_sle);
    I386_OP_TO_INT_ARRAY(flags_uge);
    I386_OP_TO_INT_ARRAY(flags_sge);
    I386_OP_TO_INT_ARRAY(st_top);
    I386_OP_TO_INT_ARRAY(fp_data_reg);
    I386_OP_TO_INT_ARRAY(fp_control_reg_rm);
    I386_OP_TO_INT_ARRAY(fp_control_reg_other);
    I386_OP_TO_INT_ARRAY(fp_tag_reg);
    I386_OP_TO_INT_ARRAY(fp_last_instr_pointer);
    I386_OP_TO_INT_ARRAY(fp_last_operand_pointer);
    I386_OP_TO_INT_ARRAY(fp_opcode);
    I386_OP_TO_INT_ARRAY(mxcsr_rm);
    I386_OP_TO_INT_ARRAY(fp_status_reg_c0);
    I386_OP_TO_INT_ARRAY(fp_status_reg_c1);
    I386_OP_TO_INT_ARRAY(fp_status_reg_c2);
    I386_OP_TO_INT_ARRAY(fp_status_reg_c3);
    I386_OP_TO_INT_ARRAY(fp_status_reg_other);
    default:
      NOT_REACHED();
  }
  ASSERT(cur < end);
  return cur;
}

size_t
i386_insn_to_int_array(i386_insn_t const *insn, int64_t arr[], size_t len)
{
  int cur = 0, i, end = len;
  if (i386_insn_is_nop(insn)) {
    cur += i386_opcode_to_int_array("nop", &arr[cur], end - cur);
  } else {
    char const *opc = opctable_name(insn->opc);
    cur += i386_opcode_to_int_array(opc, &arr[cur], end - cur);
  }
  arr[cur++] = insn->num_implicit_ops;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    cur += i386_operand_to_int_array(&insn->op[i], &arr[cur], end - cur);
  }
  ASSERT(cur < len);
  //cur += memvt_list_to_int_array(&insn->memtouch, &arr[cur], end - cur);
  //ASSERT(cur < len);
  return cur;
}

bool
i386_insn_get_pcrel_value(struct i386_insn_t const *insn, uint64_t *pcrel_val,
    src_tag_t *pcrel_tag)
{
  valtag_t *op;
  if (!(op = i386_insn_get_pcrel_operand(insn))) {
    ASSERT(pcrel_tag);
    *pcrel_tag = tag_invalid;
    *pcrel_val = 0;
    return false;
  }
  //ASSERT(op->type == op_pcrel);
  *pcrel_val = op->val;
  *pcrel_tag = op->tag;
  return true;
}

void
i386_insn_set_pcrel_value(struct i386_insn_t *insn, uint64_t pcrel_val,
    src_tag_t pcrel_tag)
{
  valtag_t *op;
  op = i386_insn_get_pcrel_operand(insn);
  ASSERT(op);
  op->val = pcrel_val;
  op->tag = pcrel_tag;
}

/*
bool
i386_iseq_involves_implicit_operands(i386_insn_t const *insns, int n_insns)
{
  int i;
  for (i = 0; i < n_insns; i++) {
    if (   i386_insn_accesses_stack(&insns[i])
        || i386_insn_is_mul(&insns[i])
        || i386_insn_is_div(&insns[i])) {
      return true;
    }
  }
  return false;
}
*/

//static int (*costfn)(i386_insn_t const *insn) = &i386_core2_insn_compute_cost;
//struct list i386_costfns;

/*static void
costfn_add(char const *name, int (*costfn)(i386_insn_t const *insn))
{
  i386_costfn_t *e;
  e = (i386_costfn_t *)malloc(sizeof(*e));
  ASSERT(e);
  //printf("alloced %p\n", e);
  e->name = strdup(name);
  e->costfn = costfn;
  list_push_back(&i386_costfns, &e->l_elem);
}*/

static long long
i386_insn_len_compute_cost(i386_insn_t const *insn)
{
  return 1;
}

static long long
i386_insn_size_compute_cost(i386_insn_t const *insn)
{
  char assembly[1024];
  i386_insn_t cinsn;
  imm_vt_map_t imm_map;
  regcons_t regcons;
  regmap_t regmap;
  bool convert;
  int ret;

  i386_iseq_to_string(insn, 1, assembly, sizeof assembly);
  i386_infer_regcons_from_assembly(assembly, &regcons);
  regmap_init(&regmap);
  ret = regmap_assign_using_regcons(&regmap, &regcons, ARCH_I386, NULL, fixed_reg_mapping_t());
  ASSERT(ret);

  i386_insn_copy(&cinsn, insn);
  imm_vt_map_init(&imm_map);
  imm_vt_map_assign_random(&imm_map, NUM_CANON_CONSTANTS);
  i386_iseq_rename(&cinsn, 1, &regmap, &imm_map, NULL, i386_identity_regmap(),
      NULL, NULL, 0);
  ret = i386_insn_assemble(&cinsn, NULL, 0);
  ASSERT(ret >= 0);
  return ret;
}

i386_costfn_t i386_costfns[MAX_I386_COSTFNS];
long i386_num_costfns;

void
i386_init_costfns(void)
{
  autostop_timer func_timer(__func__);
  //i386_costfns[0] = i386_core2_insn_compute_cost;
  i386_costfns[0] = i386_haswell_insn_compute_cost;
  i386_costfns[1] = i386_insn_len_compute_cost;
  i386_costfns[2] = i386_insn_size_compute_cost;
  i386_num_costfns = 3;
  /*list_init(&i386_costfns);
  costfn_add("core2", &i386_core2_insn_compute_cost);
  costfn_add("iseq_len", &i386_insn_iseq_len_compute_cost);
  costfn_add("size", &i386_insn_size_compute_cost);
  */
}

void
i386_free_costfns(void)
{
  /*struct list_elem *p, *q;
  i386_costfn_t *e;

  for (p = list_begin(&i386_costfns);
      p != list_end(&i386_costfns);
      p = q) {
    q = list_next(p);
    e = list_entry(p, i386_costfn_t, l_elem);
    free(e->name);
    //printf("freeing %p, f = %p, head=%p, head->member\n", e, f, &i386_costfns.head);
    free(e);
    e = NULL;
  }*/
}

/*void
i386_insn_set_costfn(char const *costfn_str)
{
  struct list_elem const *le;
  for (le = list_begin(&i386_costfns);
      le != list_end(&i386_costfns);
      le = list_next(le)) {
    i386_costfn_t *e;
    e = list_entry(le, i386_costfn_t, l_elem);
    if (!strcmp(e->name, costfn_str)) {
      costfn = e->costfn;
      return;
    }
  }
  NOT_REACHED();
}

char const *
i386_insn_get_costfn(void)
{
  struct list_elem const *le;
  ASSERT(costfn);
  for (le = list_begin(&i386_costfns);
      le != list_end(&i386_costfns);
      le = list_next(le)) {
    i386_costfn_t *e;
    e = list_entry(le, i386_costfn_t, l_elem);
    if (costfn == e->costfn) {
      return e->name;
    }
  }
  NOT_REACHED();
}

int
i386_insn_compute_cost(i386_insn_t const *insn)
{
  ASSERT(costfn);
  return (*costfn)(insn);
}*/

void
i386_init(void)
{
  autostop_timer func_timer(__func__);
  static bool init = false;

  if (!init) {
    init = true;
    regset_clear(&i386_reserved_regs);
#if ARCH_SRC == ARCH_ETFG
    regset_mark_ex_mask(&i386_reserved_regs, I386_EXREG_GROUP_GPRS, reg_identifier_t(R_ESP), I386_EXREG_LEN(I386_EXREG_GROUP_GPRS));
#endif
    //cout << __func__ << " " << __LINE__ << ": i386_reserved_regs = " << regset_to_string(&i386_reserved_regs, as1, sizeof as1) << endl;
    {
      autostop_timer opc_init_timer("opc_init");
      opc_init();
    }
    i386_init_costfns();
    if (!gas_inited) {
      gas_init();
      gas_inited = true;
    }
    regset_clear(&i386_callee_save_regs);
#if ARCH_SRC == ARCH_ETFG
    regset_mark_ex_mask(&i386_callee_save_regs, I386_EXREG_GROUP_GPRS, reg_identifier_t(R_EBP), I386_EXREG_LEN(I386_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&i386_callee_save_regs, I386_EXREG_GROUP_GPRS, reg_identifier_t(R_EBX), I386_EXREG_LEN(I386_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&i386_callee_save_regs, I386_EXREG_GROUP_GPRS, reg_identifier_t(R_EDI), I386_EXREG_LEN(I386_EXREG_GROUP_GPRS));
    regset_mark_ex_mask(&i386_callee_save_regs, I386_EXREG_GROUP_GPRS, reg_identifier_t(R_ESI), I386_EXREG_LEN(I386_EXREG_GROUP_GPRS));
#endif
  }
  //cout << __func__ << " " << __LINE__ << ": dst_reserved_regs =\n" << regset_to_string(&dst_reserved_regs, as1, sizeof as1) << endl;
}

void
i386_free(void)
{
  i386_free_costfns();
}

size_t
i386_setup_on_startup(translation_instance_t *ti, uint8_t *buf, size_t size)
{
  return 0;
}

/*int
i386_num_costfns(void)
{
  static int ret = -1;
  if (ret == -1) {
    struct list_elem const *le;
    ret = 0;
    for (le = list_begin(&i386_costfns);
        le != list_end(&i386_costfns);
        le = list_next(le)) {
      ret++;
    }
  }
  return ret;
}*/

static long
i386_insn_put_opcode_reg_to_reg_different_sizes(char const *opcode, int dst_reg, int src_reg,
    int dst_size, int src_size, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find(opcode);

  ASSERT(dst_reg >= 0 && dst_reg < I386_NUM_GPRS);
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  if (dst_size == 1 && dst_reg >= 4) {
    insn_ptr[0].op[0].valtag.reg.val = dst_reg - 4;
    insn_ptr[0].op[0].valtag.reg.mod.reg.mask = 0xff00;
  } else {
    insn_ptr[0].op[0].valtag.reg.val = dst_reg;
    insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(dst_size * BYTE_LEN);
  }
  insn_ptr[0].op[0].size = dst_size;

  ASSERT(src_reg >= 0 && src_reg < I386_NUM_GPRS);
  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.tag = tag_const;
  if (src_size == 1 && src_reg >= 4) {
    insn_ptr[0].op[1].valtag.reg.val = src_reg - 4;
    insn_ptr[0].op[1].valtag.reg.mod.reg.mask = 0xff00;
  } else {
    insn_ptr[0].op[1].valtag.reg.val = src_reg;
    insn_ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(src_size * BYTE_LEN);
  }
  insn_ptr[0].op[1].size = src_size;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static long
i386_insn_put_opcode_reg_to_reg(char const *opcode, int dst_reg, int src_reg,
    int size, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg_different_sizes(opcode, dst_reg, src_reg, size, size, insns, insns_size);
}


static long
i386_insn_put_opcode_reg_to_mem(char const *opcode, int regno, uint32_t addr, int membase,
    int size, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find(opcode);

  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = membase;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].valtag.mem.riprel.tag = tag_invalid;
  insn_ptr[0].op[0].size = size;

  ASSERT(regno >= 0 && regno < I386_NUM_GPRS);
  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.tag = tag_const;
  insn_ptr[0].op[1].valtag.reg.val = regno;
  insn_ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(size * BYTE_LEN);
  insn_ptr[0].op[1].size = size;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static long
i386_insn_put_opcode_mem_to_reg(char const *opcode, uint32_t addr, int membase, int regno,
    int size, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find(opcode);

  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  insn_ptr[0].op[0].valtag.reg.val = regno;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(size * BYTE_LEN);
  insn_ptr[0].op[0].size = size;

  insn_ptr[0].op[1].type = op_mem;
  //insn_ptr[0].op[1].tag.mem.all = tag_const;
  insn_ptr[0].op[1].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[1].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[1].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[1].valtag.mem.base.val = membase;
  insn_ptr[0].op[1].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[1].valtag.mem.index.val = -1;
  insn_ptr[0].op[1].valtag.mem.index.tag = tag_invalid;
  insn_ptr[0].op[1].valtag.mem.scale = 0;
  insn_ptr[0].op[1].valtag.mem.disp.val = addr;
  insn_ptr[0].op[1].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[1].size = size;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static long
i386_insn_put_opcode_imm_to_reg(char const *opcode, dst_ulong imm, int regno,
    src_tag_t regno_tag, int size, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  ASSERT(regno >= 0 && regno < I386_NUM_GPRS);
  insn_ptr[0].opc = opc_nametable_find(opcode);
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.val = regno;
  insn_ptr[0].op[0].valtag.reg.tag = regno_tag;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(size * BYTE_LEN);
  insn_ptr[0].op[0].size = size;

  insn_ptr[0].op[1].type = op_imm;
  insn_ptr[0].op[1].valtag.imm.tag = tag_const;
  insn_ptr[0].op[1].valtag.imm.val = imm;
  insn_ptr[0].op[1].size = size;

  i386_insn_add_implicit_operands_helper(&insn_ptr[0], fixed_reg_mapping_t(), regno_tag);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static long
i386_insn_put_opcode_imm_to_mem(char const *opcode, dst_ulong imm, dst_ulong addr,
    int membase, int size, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);

  insn_ptr[0].opc = opc_nametable_find(opcode);

  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = membase;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.index.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.addrsize = (mode == I386_AS_MODE_32) ? DWORD_LEN/BYTE_LEN : QWORD_LEN/BYTE_LEN;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
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
i386_insn_put_push_pop_mem_mode(uint64_t addr, int membase, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size,
    bool push, i386_as_mode_t mode)
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
  insn_ptr[0].op[0].valtag.reg.val = R_ESP;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
    insn_ptr[0].op[0].size = 4;
  } else if (mode == I386_AS_MODE_64) {
    insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(QWORD_LEN);
    insn_ptr[0].op[0].size = 8;
  } else NOT_REACHED();*/
  insn_ptr[0].op[0].type = op_mem;
  insn_ptr[0].op[0].rex_used = 0;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = membase;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.index.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].op[0].valtag.mem.addrsize = 4;
    insn_ptr[0].op[0].size = 4;
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].op[0].valtag.mem.addrsize = 8;
    insn_ptr[0].op[0].size = 8;
  } else NOT_REACHED();
  i386_insn_add_implicit_operands(insn_ptr, fixed_reg_mapping);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_push_mem64(uint64_t addr, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_push_pop_mem_mode(addr, -1, fixed_reg_mapping, insns, insns_size, true, I386_AS_MODE_64);
}

long
i386_insn_put_push_mem(uint64_t addr, int membase, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_push_pop_mem_mode(addr, membase, fixed_reg_mapping, insns, insns_size, true, I386_AS_MODE_32);
}



long
i386_insn_put_push_imm(uint32_t imm, src_tag_t tag, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("pushl");
  insn_ptr[0].op[0].type = op_imm;
  insn_ptr[0].op[0].valtag.imm.val = imm;
  insn_ptr[0].op[0].rex_used = 0;
  insn_ptr[0].op[0].valtag.imm.tag = tag;
  insn_ptr[0].op[0].size = 4;
  i386_insn_add_implicit_operands(insn_ptr, fixed_reg_mapping);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_pushb_imm(uint8_t imm, fixed_reg_mapping_t const &fixed_reg_mapping,
     i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  insn_ptr += i386_insn_put_add_imm_with_reg(-1, R_ESP, tag_const,
      insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movb");
  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_SS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = R_ESP;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = 0;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = 1;
  insn_ptr[0].op[1].type = op_imm;
  insn_ptr[0].op[1].valtag.imm.val = imm;
  insn_ptr[0].op[1].rex_used = 0;
  insn_ptr[0].op[1].valtag.imm.tag = tag_const;
  insn_ptr[0].op[1].size = 1;
  i386_insn_add_implicit_operands(insn_ptr, fixed_reg_mapping);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_call_helper_function(translation_instance_t *ti,
    char const *helper_function, fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(insn_ptr);
  insn_ptr[0].opc = opc_nametable_find("calll");
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = add_or_find_reloc_symbol_name(&ti->reloc_strings,
      &ti->reloc_strings_size, helper_function);
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_reloc_string;
  i386_insn_add_implicit_operands(insn_ptr, fixed_reg_mapping);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;

}

long
i386_insn_put_daa(i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("daa");
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static long
i386_insn_put_push_pop_reg_mode(long regnum, fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size,
    bool push, i386_as_mode_t mode)
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
  } else if (mode == 64) {
    insn_ptr[0].op[0].size = 8;
  } else NOT_REACHED();*/
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.val = regnum;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].op[0].size = 4;
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].op[0].size = 8;
  } else NOT_REACHED();
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(insn_ptr[0].op[0].size*8);
  i386_insn_add_implicit_operands(insn_ptr, fixed_reg_mapping);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_push_reg(long regnum, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_push_pop_reg_mode(regnum, fixed_reg_mapping, insns, insns_size, true, I386_AS_MODE_32);
}

long
i386_insn_put_pop_reg(long regnum, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_push_pop_reg_mode(regnum, fixed_reg_mapping, insns, insns_size, false, I386_AS_MODE_32);
}

long
i386_insn_put_push_reg64(long regnum, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_push_pop_reg_mode(regnum, fixed_reg_mapping, insns, insns_size, true, I386_AS_MODE_64);
}

long
i386_insn_put_pop_reg64(long regnum, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_push_pop_reg_mode(regnum, fixed_reg_mapping, insns, insns_size, false, I386_AS_MODE_64);
}

long
i386_insn_put_movb_mem_to_reg(int regno, long addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("movb", addr, membase, regno, 1, insns, insns_size);
}

long
i386_insn_put_movw_mem_to_reg(int regno, long addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("movw", addr, membase, regno, 2, insns, insns_size);
}

long
i386_insn_put_movl_mem_to_reg(int regno, long addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("movl", addr, membase, regno, 4, insns, insns_size);
}

long
i386_insn_put_addb_mem_to_reg(int regno, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("addb", addr, -1, regno, 1, insns, insns_size);
}

long
i386_insn_put_addw_mem_to_reg(int regno, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("addw", addr, -1, regno, 2, insns, insns_size);
}

long
i386_insn_put_addl_mem_to_reg(int regno, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("addl", addr, -1, regno, 4, insns, insns_size);
}

long
i386_insn_put_andl_imm_to_mem(dst_ulong imm, dst_ulong addr,
    int membase, i386_as_mode_t mode, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_mem("andl", imm, addr, membase, 4, insns, insns_size, mode);
}

long
i386_insn_put_movq_mem_to_reg(int regno, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movq");

  ASSERT(regno >= 0 && regno < I386_NUM_GPRS);
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  insn_ptr[0].op[0].valtag.reg.val = regno;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(QWORD_LEN);
  insn_ptr[0].op[0].size = 8;

  insn_ptr[0].op[1].type = op_mem;
  //insn_ptr[0].op[1].tag.mem.all = tag_const;
  insn_ptr[0].op[1].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[1].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[1].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[1].valtag.mem.base.val = -1;
  insn_ptr[0].op[1].valtag.mem.index.val = -1;
  insn_ptr[0].op[1].valtag.mem.disp.val = addr;
  insn_ptr[0].op[1].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[1].size = 8;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_movq_reg_to_mem(dst_ulong addr, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movq");

  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = -1;
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = 8;

  ASSERT(regno >= 0 && regno < I386_NUM_GPRS);
  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.tag = tag_const;
  insn_ptr[0].op[1].valtag.reg.val = regno;
  insn_ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(QWORD_LEN);
  insn_ptr[0].op[1].size = 8;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_jump_to_indir_mem(long addr, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("jmp");
  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = -1;
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = 4;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_shl_reg_by_imm(int regno, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("shll", imm, regno, tag_const, 4,
      insns, insns_size);
}

long
i386_insn_put_shr_reg_by_imm(int regno, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("shrl", imm, regno, tag_const, 4,
      insns, insns_size);
}

long
i386_insn_put_sar_reg_by_imm(int regno, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("sarl", imm, regno, tag_const, 4,
      insns, insns_size);
}

long
i386_insn_put_movb_imm_to_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("movb", imm, regno, tag_const, 1,
      insns, insns_size);
}

long
i386_insn_put_movw_imm_to_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("movw", imm, regno, tag_const, 2,
      insns, insns_size);
}

long
i386_insn_put_movl_imm_to_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("movl", imm, regno, tag_const, 4,
      insns, insns_size);
}



long
i386_insn_put_movq_imm_to_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("movq", imm, regno, tag_const, 8,
      insns, insns_size);
}

long
i386_insn_put_and_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("andl", imm, regno, tag_const, 4,
      insns, insns_size);
}

long
i386_insn_put_testl_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("testl", imm, regno, tag_const, 4,
      insns, insns_size);
}

long
i386_insn_put_testw_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("testw", imm, regno, tag_const, 2,
      insns, insns_size);
}

long
i386_insn_put_testb_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("testb", imm, regno, tag_const, 1,
      insns, insns_size);
}

long
i386_insn_put_add_imm_with_reg(dst_ulong imm, int regno, src_tag_t regno_tag,
    i386_insn_t *insns, size_t insns_size)
{
  ASSERT(regno_tag == tag_const || regno_tag == tag_var);
  return i386_insn_put_opcode_imm_to_reg("addl", imm, regno, regno_tag, 4,
      insns, insns_size);
}

long
i386_insn_put_add_imm_with_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  int ret;
  ret = i386_insn_put_add_imm_with_reg(imm, regno, tag_var, insns, insns_size);
  ASSERT(ret == 1);
  //convert_reg_to_regvar(insns);
  return 1;
}


long
i386_insn_put_cmpb_imm_to_reg(long regno, long imm, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("cmpb", imm, regno, tag_const, 1,
      insns, insns_size);
}

long
i386_insn_put_cmpw_imm_to_reg(long regno, long imm, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("cmpw", imm, regno, tag_const,
      2, insns, insns_size);
}

long
i386_insn_put_cmpl_imm_to_reg(long regno, long imm, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("cmpl", imm, regno, tag_const,
      4, insns, insns_size);
}

long
i386_insn_put_cmpb_mem_to_reg(long regno, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("cmpb", addr, -1, regno, 1, insns, insns_size);
}

long
i386_insn_put_cmpw_mem_to_reg(long regno, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("cmpw", addr, -1, regno, 2, insns, insns_size);
}

long
i386_insn_put_cmpl_mem_to_reg(long regno, long addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("cmpl", addr, membase, regno, 4, insns, insns_size);
}

long
i386_insn_put_cmpb_reg_to_mem(long regno, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_mem("cmpb", regno, addr, -1, 1, insns, insns_size);
}

long
i386_insn_put_cmpw_reg_to_mem(long regno, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_mem("cmpw", regno, addr, -1, 2, insns, insns_size);
}

long
i386_insn_put_cmpl_reg_to_mem(long regno, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_mem("cmpl", regno, addr, -1, 4, insns, insns_size);
}

long
i386_insn_put_cmpb_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("cmpb", reg1, reg2, 1, insns, insns_size);
}

long
i386_insn_put_cmpw_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("cmpw", reg1, reg2, 2, insns, insns_size);
}

long
i386_insn_put_cmpl_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("cmpl", reg1, reg2, 4, insns, insns_size);
}

long
i386_insn_put_testb_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("testb", reg1, reg2, 1, insns, insns_size);
}

long
i386_insn_put_testw_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("testw", reg1, reg2, 2, insns, insns_size);
}

long
i386_insn_put_testl_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("testl", reg1, reg2, 4, insns, insns_size);
}

long
i386_insn_put_setcc_mem(char const *opc, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find(opc);

  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = -1;
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = 4;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_adcl_imm_with_reg(long regno, long imm, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_reg("adcl", imm, regno, tag_const,
      4, insns, insns_size);
}

long
i386_insn_put_or_mem_with_reg(int regno, uint32_t addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("orl", addr, -1, regno, 4, insns, insns_size);
}

long
i386_insn_put_add_mem_with_reg(int regno, uint32_t addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("addl", addr, -1, regno, 4, insns, insns_size);
}

static void
i386_insn_make_space_for_new_operand(i386_insn_t *insn, int operand_num)
{
  int i;

  ASSERT(insn->op[I386_MAX_NUM_OPERANDS - 1].type == invalid);
  ASSERT(insn->num_implicit_ops <= operand_num);
  for (i = I386_MAX_NUM_OPERANDS - 2; i >= operand_num; i--) {
    i386_operand_copy(&insn->op[i + 1], &insn->op[i]);
  }
}

long
i386_insn_put_imul_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size)
{
  int ret, op0num;
  ret = i386_insn_put_opcode_imm_to_reg("imull", imm, regno, tag_const, 4,
      insns, insns_size);
  ASSERT(ret == 1);
  op0num = insns[0].num_implicit_ops;
  //duplicate operand 0 into operand 1, shifting all other operands to the right.
  i386_insn_make_space_for_new_operand(&insns[0], op0num + 1);
  i386_operand_copy(&insns[0].op[op0num + 1], &insns[0].op[op0num]);

  return ret;
}

long
i386_insn_put_movb_imm_to_mem(dst_ulong imm, uint32_t addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_mem("movb", imm, addr, -1, 1, insns, insns_size, I386_AS_MODE_32);
}

long
i386_insn_put_movw_imm_to_mem(dst_ulong imm, uint32_t addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_mem("movw", imm, addr, -1, 2, insns, insns_size, I386_AS_MODE_32);
}

long
i386_insn_put_movl_imm_to_mem(dst_ulong imm, uint32_t addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_imm_to_mem("movl", imm, addr, membase, 4, insns, insns_size, I386_AS_MODE_32);
}

long
i386_insn_put_addl_imm_to_mem(dst_ulong imm, uint32_t addr, int membase, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  return i386_insn_put_opcode_imm_to_mem("addl", imm, addr, membase, 4, insns, insns_size, mode);
}

static long
i386_insn_put_cmpl_imm_to_mem(dst_ulong imm, uint32_t addr, int membase, i386_insn_t *insns,
    size_t insns_size, i386_as_mode_t mode)
{
  return i386_insn_put_opcode_imm_to_mem("cmpl", imm, addr, membase, 4, insns, insns_size, mode);
}

long
i386_insn_put_shl_mem_by_imm(dst_ulong addr, int membase, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size, i386_as_mode_t mode)
{
  return i386_insn_put_opcode_imm_to_mem("shll", imm, addr, membase, 4,
      insns, insns_size, mode);
}

long
i386_insn_put_shr_mem_by_imm(dst_ulong addr, int membase, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size, i386_as_mode_t mode)
{
  return i386_insn_put_opcode_imm_to_mem("shrl", imm, addr, membase, 4,
      insns, insns_size, mode);
}



long
i386_insn_put_store_reg1_into_memreg2_off(long reg1, long reg2, long off,
    i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movl");

  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = reg2;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = off;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = 4;

  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.tag = tag_const;
  insn_ptr[0].op[1].valtag.reg.val = reg1;
  insn_ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[1].size = 4;

  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_dereference_reg1_into_reg2(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movl");

  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  insn_ptr[0].op[0].valtag.reg.val = reg2;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].size = 4;

  insn_ptr[0].op[1].type = op_mem;
  //insn_ptr[0].op[1].tag.mem.all = tag_const;
  insn_ptr[0].op[1].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[1].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[1].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[1].valtag.mem.base.val = reg1;
  insn_ptr[0].op[1].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[1].valtag.mem.index.val = -1;
  insn_ptr[0].op[1].valtag.mem.disp.val = 0;
  insn_ptr[0].op[1].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[1].size = 4;

  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_movb_reg_to_mem(dst_ulong addr, long regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_mem("movb", regno, addr, -1, 1, insns, insns_size);
}

long
i386_insn_put_movw_reg_to_mem(dst_ulong addr, long regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_mem("movw", regno, addr, -1, 2, insns, insns_size);
}

long
i386_insn_put_movl_reg_to_mem(dst_ulong addr, int membase, long regno, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_mem("movl", regno, addr, membase, 4, insns, insns_size);
}

long
i386_insn_put_movb_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("movb", reg1, reg2, 1, insns, insns_size);
}

long
i386_insn_put_movw_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("movw", reg1, reg2, 2, insns, insns_size);
}

long
i386_insn_put_movl_reg_to_reg(long dst, long src, i386_insn_t *insns,
    size_t insns_size)
{
  ASSERT(src >= 0 && src < I386_NUM_GPRS);
  return i386_insn_put_opcode_reg_to_reg("movl", dst, src, 4, insns, insns_size);
}

long
i386_insn_put_movsbl_reg2b_to_reg1(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  long ret;

  ret = i386_insn_put_opcode_reg_to_reg("movsbl", reg1, reg2, 4, insns, insns_size);
  ASSERT(ret == 1);
  ASSERT(insns[0].op[1].type == op_reg);
  ASSERT(insns[0].op[1].size == 4);
  ASSERT(insns[0].op[1].valtag.reg.val == reg2);
  ASSERT(insns[0].op[1].valtag.reg.mod.reg.mask == 0xffffffff);
  insns[0].op[1].size = 1;
  insns[0].op[1].valtag.reg.mod.reg.mask = 0xff;

  return ret;
}

long
i386_insn_put_movswl_reg2w_to_reg1(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  long ret;

  ret = i386_insn_put_opcode_reg_to_reg("movswl", reg1, reg2, 4, insns, insns_size);
  ASSERT(ret == 1);
  ASSERT(insns[0].op[1].type == op_reg);
  ASSERT(insns[0].op[1].size == 4);
  ASSERT(insns[0].op[1].valtag.reg.val == reg2);
  ASSERT(insns[0].op[1].valtag.reg.mod.reg.mask == 0xffffffff);
  insns[0].op[1].size = 2;
  insns[0].op[1].valtag.reg.mod.reg.mask = 0xffff;

  return ret;
}

static long
i386_insn_put_opcode_operand_to_reg(char const *opc, long reg1, operand_t const *op,
    i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);

  insn_ptr[0].opc = opc_nametable_find(opc);

  ASSERT(reg1 >= 0 && reg1 < I386_NUM_GPRS);
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(op->size * BYTE_LEN);
  insn_ptr[0].op[0].valtag.reg.val = reg1;
  insn_ptr[0].op[0].size = op->size;

  i386_operand_copy(&insn_ptr[0].op[1], op);

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_move_operand_to_reg(long reg1, operand_t const *op,
    i386_insn_t *insns, size_t insns_size)
{
  char const *opc;

  if (op->size == 1) {
    opc = "movb";
  } else if (op->size == 2) {
    opc = "movw";
  } else if (op->size == 4) {
    opc = "movl";
  } else NOT_REACHED();
  return i386_insn_put_opcode_operand_to_reg(opc, reg1, op, insns, insns_size);
}

long
i386_insn_put_opcode_operand(char const *opc, operand_t const *op,
    i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);

  insn_ptr[0].opc = opc_nametable_find(opc);

  i386_operand_copy(&insn_ptr[0].op[0], op);

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}
long
i386_insn_put_xorl_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("xorl", reg1, reg2, 4, insns, insns_size);
}

long
i386_insn_put_xorw_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("xorw", reg1, reg2, 2, insns, insns_size);
}

long
i386_insn_put_xorb_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("xorb", reg1, reg2, 1, insns, insns_size);
}

long
i386_insn_put_xorl_reg_to_mem(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_mem("xorl", reg1, addr, -1, 4, insns, insns_size);
}

long
i386_insn_put_xorw_reg_to_mem(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_mem("xorw", reg1, addr, -1, 2, insns, insns_size);
}

long
i386_insn_put_xorb_reg_to_mem(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_mem("xorb", reg1, addr, -1, 1, insns, insns_size);
}

long
i386_insn_put_xorl_mem_to_reg(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("xorl", addr, -1, reg1, 4, insns, insns_size);
}

long
i386_insn_put_xorw_mem_to_reg(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("xorw", addr, -1, reg1, 2, insns, insns_size);
}

long
i386_insn_put_xorb_mem_to_reg(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_mem_to_reg("xorb", addr, -1, reg1, 1, insns, insns_size);
}

long
i386_insn_put_add_reg_to_reg(long dst_reg, long src_reg, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_opcode_reg_to_reg("addl", dst_reg, src_reg, 4, insns, insns_size);
}

static long
i386_insn_put_pushf_popf_mode(fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size, bool push,
    i386_as_mode_t mode)
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
  i386_insn_add_implicit_operands(insn_ptr, fixed_reg_mapping);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_pusha(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  if (mode == I386_AS_MODE_32) {
    insn_ptr += i386_insn_put_push_reg(R_EAX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg(R_ECX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg(R_EDX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg(R_EBX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg(R_EBP, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg(R_ESI, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg(R_EDI, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pushf(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr += i386_insn_put_push_reg64(R_EAX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg64(R_ECX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg64(R_EDX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg64(R_EBX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg64(R_EBP, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg64(R_ESI, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_push_reg64(R_EDI, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pushf64(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    /* also zero out all registers, so high-order bits do not interfere */
    insn_ptr += i386_insn_put_movq_imm_to_reg(0, R_EAX, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_movq_imm_to_reg(0, R_ECX, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_movq_imm_to_reg(0, R_EDX, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_movq_imm_to_reg(0, R_EBX, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_movq_imm_to_reg(0, R_EBP, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_movq_imm_to_reg(0, R_ESI, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_movq_imm_to_reg(0, R_EDI, insn_ptr, insn_end - insn_ptr);
  } else NOT_REACHED();
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

long
i386_insn_put_popa(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  if (mode == I386_AS_MODE_32) {
    insn_ptr += i386_insn_put_popf(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg(R_EDI, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg(R_ESI, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg(R_EBP, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg(R_EBX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg(R_EDX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg(R_ECX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg(R_EAX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr += i386_insn_put_popf64(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg64(R_EDI, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg64(R_ESI, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg64(R_EBP, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg64(R_EBX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg64(R_EDX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg64(R_ECX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
    insn_ptr += i386_insn_put_pop_reg64(R_EAX, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
  } else NOT_REACHED();
  ASSERT(insn_ptr < insn_end);
  return insn_ptr - insns;
}

long
i386_insn_put_pushf64(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_pushf_popf_mode(fixed_reg_mapping, insns, insns_size, true, I386_AS_MODE_64);
}

long
i386_insn_put_pushf(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_pushf_popf_mode(fixed_reg_mapping, insns, insns_size, true, I386_AS_MODE_32);
}

long
i386_insn_put_popf64(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_pushf_popf_mode(fixed_reg_mapping, insns, insns_size, false, I386_AS_MODE_64);
}

long
i386_insn_put_popf(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_pushf_popf_mode(fixed_reg_mapping, insns, insns_size, false, I386_AS_MODE_32);
}

#if ARCH_SRC == ARCH_I386
static long
i386_save_and_set_esp_to_scratch(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size,
    i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;
  int size;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  /* mov %esp, SRC_ENV_SCRATCH0 */
  i386_insn_init(&insn_ptr[0]);
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].opc = opc_nametable_find("movl");
    size = 4;
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].opc = opc_nametable_find("movq");
    size = 8;
  } else NOT_REACHED();
  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.addrsize = size;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = -1;
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.disp.val = SRC_ENV_SCRATCH(1);
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = size;
  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.val = R_ESP;
  insn_ptr[0].op[1].valtag.reg.tag = tag_const;
  insn_ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(mode);
  insn_ptr[0].op[1].size = size;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* mov $SRC_ENV_SCRATCH(2) + 4/8, %esp */
  if (mode == I386_AS_MODE_32) {
    insn_ptr += i386_insn_put_movl_imm_to_reg(SRC_ENV_SCRATCH(2) + 4, R_ESP, insn_ptr,
        insn_end - insn_ptr);
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr += i386_insn_put_movq_imm_to_reg(SRC_ENV_SCRATCH(2) + 8, R_ESP, insn_ptr,
        insn_end - insn_ptr);
  } else NOT_REACHED();
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static long
i386_restore_esp_from_scratch(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;
  int size;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  /* mov SRC_ENV_SCRATCH0, %esp */
  i386_insn_init(&insn_ptr[0]);
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].opc = opc_nametable_find("movl");
    size = 4;
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].opc = opc_nametable_find("movq");
    size = 8;
  } else NOT_REACHED();
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.val = R_ESP;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(size * 8);
  insn_ptr[0].op[0].size = size;
  insn_ptr[0].op[1].type = op_mem;
  //insn_ptr[0].op[1].tag.mem.all = tag_const;
  insn_ptr[0].op[1].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[1].valtag.mem.addrsize = 4;
  insn_ptr[0].op[1].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[1].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[1].valtag.mem.base.val = -1;
  insn_ptr[0].op[1].valtag.mem.index.val = -1;
  insn_ptr[0].op[1].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.disp.val = SRC_ENV_SCRATCH(1);
  insn_ptr[0].op[1].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[1].size = size;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}
#endif

//#define gen_mem_to_flags_code(memnum, ptr) do { \
//  /* save rsp to scratch. mov %rsp, SRC_ENV_SCRATCH0 */ \
//  *(ptr)++ = 0x48;  *(ptr)++ = 0x89;  *(ptr)++ = 0x24;  *(ptr)++ = 0x25;  \
//  *(uint32_t *)ptr = SRC_ENV_SCRATCH0;  ptr += 4; \
//  /* set rsp to scratch + 16. mov $(SRC_ENV_SCRATCH0+16), %rsp. */ \
//  *(ptr)++ = 0x48;  *(ptr)++ = 0xc7;  *(ptr)++ = 0xc4; \
//  *(uint32_t *)ptr = SRC_ENV_SCRATCH0 + 16;  ptr += 4; \
//  /* push mem */ \
//  *(ptr)++ = 0xff; *(ptr)++ = 0x34;  *(ptr)++ = 0x25; \
//  *(uint32_t *)(ptr) = SRC_ENV_ADDR + (memnum << 2);   (ptr) += 4; \
//  *(ptr)++ = 0x9d; /* popf */ \
//  /* load rsp from scratch. */ \
//  *(ptr)++ = 0x48;  *(ptr)++ = 0x8b;  *(ptr)++ = 0x24;  *(ptr)++ = 0x25;  \
//  *(uint32_t *)(ptr) = SRC_ENV_SCRATCH0;  (ptr) += 4; \
//} while (0)
//#define gen_mem_to_flags_code(memnum, ptr) do { \
//  if (i386_base_reg == -1) { \
//    /* cmpl $0x8, memnum(%ebp) */ \
//    *(ptr)++ = 0x83; *(ptr)++ = 0x3c; *(ptr)++ = 0x25; \
//    *(unsigned int *)(ptr) = SRC_ENV_ADDR + (memnum<<2); (ptr)+=4; *(ptr)++ = 0x8; \
//    /* jne .ge */ \
//    *(ptr)++=0x75; *(ptr)++=0x9; \
//    /* cmpl $0x10, memnum(%ebp) */ \
//    *(ptr)++ = 0x83; *(ptr)++ = 0x3c; *(ptr)++ = 0x25; \
//    *(unsigned int *)(ptr) = SRC_ENV_ADDR + (memnum<<2); (ptr)+=4; *(ptr)++ = 0x10;\
//    /* jmp .end */ \
//    *(ptr)++ = 0xeb; *(ptr)++ = 0x7; \
//    /* .ge */ \
//    /* cmpl $0x2, memnum(%ebp) */ \
//    *(ptr)++ = 0x83; *(ptr)++ = 0x3c; *(ptr)++ = 0x25; \
//    *(unsigned int *)(ptr) = SRC_ENV_ADDR + (memnum<<2); (ptr)+=4; *(ptr)++ = 0x2; \
//    /* .end: */ \
//  } else { \
//    /* cmpl $0x8, memnum(%ebp) */ \
//    *(ptr)++ = 0x83; *(ptr)++ = 0xb8|i386_base_reg; \
//    if (i386_base_reg==R_ESP) *(ptr)++=0x24; \
//    *(unsigned int *)(ptr) = memnum<<2; (ptr)+=4; *(ptr)++ = 0x8; \
//    /* jne .ge */ \
//    *(ptr)++=0x75; *(ptr)++=0x9+((i386_base_reg==R_ESP)?1:0); \
//    /* cmpl $0x10, memnum(%ebp) */ \
//    *(ptr)++ = 0x83; *(ptr)++ = 0xb8|i386_base_reg; \
//    if (i386_base_reg==R_ESP) *(ptr)++=0x24; \
//    *(unsigned int *)(ptr) = memnum<<2; (ptr)+=4; *(ptr)++ = 0x10; \
//    /* jmp .end */ \
//    *(ptr)++ = 0xeb; *(ptr)++ = 0x7+((i386_base_reg==R_ESP)?1:0); \
//    /* .ge */ \
//    /* cmpl $0x2, memnum(%ebp) */ \
//    *(ptr)++ = 0x83; *(ptr)++ = 0xb8|i386_base_reg; \
//    if (i386_base_reg==R_ESP) *(ptr)++=0x24; \
//    *(unsigned int *)(ptr) = memnum<<2; (ptr)+=4; *(ptr)++ = 0x2; \
//    /* .end: */ \
//  } \
//} while (0)
static long
i386_insn_put_move_mem_to_flags(dst_ulong addr, int membase,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size,
    i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;
  int size, addrsize;

  insn_ptr = insns;
  insn_end = insns + insns_size;

#if ARCH_SRC == ARCH_I386
  insn_ptr += i386_save_and_set_esp_to_scratch(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, mode);

  /* push <addr> */
  i386_insn_init(&insn_ptr[0]);
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].opc = opc_nametable_find("pushl");
    size = 4;
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].opc = opc_nametable_find("pushq");
    size = 8;
  } else NOT_REACHED();
  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.addrsize = size;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = membase;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = size;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* andl $0xfffbfeff, (%esp)  to mask trap flag AC, TF */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("andl");
  if (mode == I386_AS_MODE_32) {
    addrsize = 4;
  } else {
    addrsize = 8;
  }
  size = 4;
  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.addrsize = addrsize;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_SS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = R_ESP;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(QWORD_LEN);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = 0;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = size;
  insn_ptr[0].op[1].type = op_imm;
  insn_ptr[0].op[1].valtag.imm.val = 0xfffbfeff;
  insn_ptr[0].op[1].valtag.imm.tag = tag_const;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* popf */
  i386_insn_init(&insn_ptr[0]);
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].opc = opc_nametable_find("popfl");
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].opc = opc_nametable_find("popfq");
  } else NOT_REACHED();
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  insn_ptr += i386_restore_esp_from_scratch(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, mode);

#elif ARCH_SRC == ARCH_X64
  NOT_IMPLEMENTED();
#elif ARCH_SRC == ARCH_PPC
#define GE_INSN 4
#define END_INSN 5
  /* cmpl $0x8, addr */
  insn_ptr += i386_insn_put_cmpl_imm_to_mem(0x8, addr, -1, insn_ptr, insn_end - insn_ptr, I386_AS_MODE_32);

  /* jne .ge */
  i386_insn_init(insn_ptr);
  insn_ptr[0].opc = opc_nametable_find("jne");
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = GE_INSN - (insn_ptr + 1 - insns); //.ge
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  insn_ptr += i386_insn_put_cmpl_imm_to_mem(0x10, addr, -1, insn_ptr, insn_end - insn_ptr, I386_AS_MODE_32);

  /* jmp .end */
  i386_insn_init(insn_ptr);
  insn_ptr[0].opc = opc_nametable_find("jmp");
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = END_INSN - (insn_ptr + 1 - insns);  //.end
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* .ge */
  /* cmpl $0x2, memnum(%ebp) */
  ASSERT(insn_ptr - insns == GE_INSN);
  insn_ptr += i386_insn_put_cmpl_imm_to_mem(0x2, addr, -1, insn_ptr, insn_end - insn_ptr, I386_AS_MODE_32);

  /* .end: */
  ASSERT(insn_ptr - insns == END_INSN);
#undef GE_INSN
#undef END_INSN
#elif ARCH_SRC == ARCH_ETFG
  insn_ptr += i386_insn_put_push_pop_mem_mode(addr, membase, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, true, mode);
  ASSERT(insn_ptr <= insn_end);

  /* andl $0xfffbfeff, (%esp)  to mask trap flag AC, TF */
  insn_ptr += i386_insn_put_andl_imm_to_mem(0xfffbfeff, 0, fixed_reg_mapping.get_mapping(I386_EXREG_GROUP_GPRS, R_ESP), mode, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);

  /* popf */
  insn_ptr += i386_insn_put_pushf_popf_mode(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, false, mode);
  ASSERT(insn_ptr <= insn_end);
#else
#error "not-implemented"
#endif
  return insn_ptr - insns;
}

static long
i386_insn_put_move_mem_to_mmx(int reg, dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mmx_to_mem(dst_ulong addr, int membase, int reg, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_reg_to_mmx(int mmxreg, int reg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mmx_to_reg(int reg, int mmxreg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_xmm(int reg, dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_ymm(int reg, dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_data_reg(int reg, dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_xmm_to_mem(dst_ulong addr, int membase, int reg, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_ymm_to_mem(dst_ulong addr, int membase, int reg, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_data_reg_to_mem(dst_ulong addr, int membase, int reg, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_reg_to_xmm(int xmmreg, int reg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_reg_to_ymm(int ymmreg, int reg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_xmm_to_reg(int reg, int xmmreg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_ymm_to_reg(int reg, int ymmreg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}



static long
i386_insn_put_move_mem_to_seg(int reg, dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_seg_to_mem(dst_ulong addr, int membase, int reg, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_reg_to_seg(int seg, int reg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_seg_to_reg(int reg, int seg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}


static long
i386_insn_put_move_mem_to_st_top(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_st_top_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_reg_to_st_top(int reg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_st_top_to_reg(int reg, i386_insn_t *insns,
    size_t insns_size)
{
  NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_control_reg_rm(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_control_reg_other(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_tag_reg(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_last_instr_pointer(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_last_operand_pointer(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_opcode_reg(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_mxcsr_rm(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_control_reg_rm_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_control_reg_other_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_tag_reg_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_last_instr_pointer_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_last_operand_pointer_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_opcode_reg_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mxcsr_rm_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_status_reg_c0(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_status_reg_c0_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_status_reg_c1(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_status_reg_c1_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_status_reg_c2(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_status_reg_c2_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_status_reg_c3(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_status_reg_c3_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_mem_to_fp_status_reg_other(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

static long
i386_insn_put_move_fp_status_reg_other_to_mem(dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  return 0;
  //NOT_IMPLEMENTED();
}

//#define gen_flags_to_mem_code(signedness, overflow, memnum, ptr) do { \
//  /* save rsp to scratch. mov %rsp, SRC_ENV_SCRATCH0 */ \
//  *(ptr)++ = 0x48;  *(ptr)++ = 0x89;  *(ptr)++ = 0x24;  *(ptr)++ = 0x25; \
//  *(uint32_t *)ptr = SRC_ENV_SCRATCH0;  ptr += 4; \
//  /* set rsp to scratch + 16. mov $(SRC_ENV_SCRATCH0 + 16), %rsp. */ \
//  *(ptr)++ = 0x48;  *(ptr)++ = 0xc7;  *(ptr)++ = 0xc4; \
//  *(uint32_t *)ptr = SRC_ENV_SCRATCH0 + 16;  ptr += 4; \
//  *(ptr)++ = 0x9c; /* pushf */ \
//  /* pop mem */ \
//  *(ptr)++ = 0x8f; *(ptr)++ = 0x04;  *(ptr)++ = 0x25; \
//  *(uint32_t *)(ptr) = SRC_ENV_ADDR + (memnum << 2);   (ptr) += 4; \
//  /* load rsp from scratch. */ \
//  *(ptr)++ = 0x48;  *(ptr)++ = 0x8b;  *(ptr)++ = 0x24;  *(ptr)++ = 0x25;  \
//  *(uint32_t *)(ptr) = SRC_ENV_SCRATCH0;  (ptr) += 4; \
//} while (0)
////#define gen_flags_to_mem_code(signedness, overflow, memnum, ptr) do { \
//  if (i386_base_reg == -1) { \
//    /* j[ga]e .ge */ \
//    *(ptr)++ = signedness?0x73:0x7d; *(ptr)++ = 0xc; \
//    /* movl $0x8, memnum(SRC_ENV_ADDR) */ \
//    *(ptr)++ = 0xc7; *(ptr)++ = 0x04; *(ptr)++ = 0x25; \
//    *(unsigned int *)ptr = SRC_ENV_ADDR + memnum*4; ptr+=4; \
//    *(unsigned int *)ptr = 0x8; ptr+=4; \
//    /* jmp .end */ \
//    *(ptr)++ = 0xeb; *(ptr)++ = 0x18; \
//    /*.ge */ \
//    *(ptr)++=0x74; *(ptr)++ = 0x0c;/* je .e */ \
//    /* movl $0x4, memnum(SRC_ENV_ADDR) */ \
//    *(ptr)++ = 0xc7; *(ptr)++ = 0x04; *(ptr)++ = 0x25; \
//    *(unsigned int *)(ptr) = SRC_ENV_ADDR + memnum*4; ptr+=4; \
//    *(unsigned int *)(ptr) = 0x4; (ptr)+=4; \
//    /* jmp .end */ \
//    *(ptr)++ = 0xeb; *(ptr)++ = 0xa; \
//    /* .e: movl $0x2, memnum(SRC_ENV_ADDR) */ \
//    *(ptr)++ = 0xc7; *(ptr)++ = 0x04;  *(ptr)++ = 0x25; \
//    *(unsigned int *)(ptr)=SRC_ENV_ADDR+memnum*4; ptr+=4; \
//    *(unsigned int *)(ptr) = 0x2; (ptr)+=4; \
//    /*.end: cmpl $0x0, xer_ov; je .skip; orl $0x1, xer_ov; .skip: */ \
//    if (overflow) { \
//      /* cmpl $0x0,PPC_REG_XER_OV(SRC_ENV_ADDR) XXX \
//      *(ptr)++ = 0x83; *(ptr)++ = 0x3c; *(ptr)++ = 0x25; \
//      *(unsigned int *)(ptr)=SRC_ENV_ADDR+(PPC_REG_XER_OV<<2); (ptr)+=4; \
//      *(ptr)++ = 0x0; \
//      /* je .end  \
//      *(ptr)++ = 0x74; *(ptr)++ = 0x7; \
//      /* orl $0x1, memnum(i386_base_reg)  \
//      *(ptr)++ = 0x83; *(ptr)++ = 0x04; *(ptr)++ = 0x25; \
//      *(unsigned int *)(ptr)=SRC_ENV_ADDR+(memnum<<2); (ptr)+=4; \
//      *(ptr)++ = 0x1; */ \
//    } \
//  } else { \
//    /* j[ga]e .ge */ \
//    *(ptr)++ = signedness?0x73:0x7d; \
//    *(ptr)++ = 0xc+((i386_base_reg==R_ESP)?1:0); \
//    /* movl $0x8, memnum(%ebp) */ \
//    *(ptr)++ = 0xc7; *(ptr)++ = 0x80 | i386_base_reg; \
//    if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//    *(unsigned int *)(ptr)=memnum*4; ptr+=4; \
//    *(unsigned int *)(ptr) = 0x8; ptr+=4; \
//    /* jmp .end */ \
//    *(ptr)++ = 0xeb; *(ptr)++ = 0x18+((i386_base_reg==R_ESP)?2:0); \
//    /*.ge */ \
//    *(ptr)++=0x74; *(ptr)++ = 0x0c+((i386_base_reg==R_ESP)?1:0);/* je .e */ \
//    /* movl $0x4, memnum(%ebp) */ \
//    *(ptr)++ = 0xc7; *(ptr)++ = 0x80|i386_base_reg; \
//    if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//    *(unsigned int *)(ptr)=memnum*4; ptr+=4; \
//    *(unsigned int *)(ptr) = 0x4; (ptr)+=4; \
//    /* jmp .end */ \
//    *(ptr)++ = 0xeb; *(ptr)++ = 0xa+((i386_base_reg==R_ESP)?1:0); \
//    /* .e: movl $0x2, memnum(%ebp) */ \
//    *(ptr)++ = 0xc7; *(ptr)++ = 0x80|i386_base_reg; \
//    if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//    *(unsigned int *)(ptr)=memnum*4; ptr+=4; \
//    *(unsigned int *)(ptr) = 0x2; (ptr)+=4; \
//    /*.end: cmpl $0x0, xer_ov; je .skip; orl $0x1, xer_ov; .skip: */ \
//    if (overflow) { \
//      /* cmpl $0x0,PPC_REG_XER_OV(i386_base_reg) XXX \
//      *(ptr)++ = 0x83; *(ptr)++ = 0xb8|i386_base_reg; \
//      if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//      *(unsigned int *)(ptr)=PPC_REG_XER_OV<<2; (ptr)+=4; *(ptr)++ = 0x0; \
//      /* je .end  \
//      *(ptr)++ = 0x74; *(ptr)++ = 0x7+((i386_base_reg==R_ESP)?1:0); \
//      /* orl $0x1, memnum(i386_base_reg)  \
//      *(ptr)++ = 0x83; *(ptr)++ = 0x80|i386_base_reg; \
//      if (i386_base_reg == R_ESP) *(ptr)++ = 0x24;\
//      *(unsigned int *)(ptr)=memnum<<2; (ptr)+=4; *(ptr)++ = 0x1; */ \
//    } \
//  } \
//} while (0)
static long
i386_insn_put_move_flags_to_mem(bool signedness, dst_ulong addr, int membase,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;
  //ASSERT(addr >= SRC_ENV_ADDR && addr < SRC_ENV_ADDR + SRC_ENV_SIZE);

  insn_ptr = insns;
  insn_end = insns + insns_size;

#if ARCH_SRC == ARCH_I386
  insn_ptr += i386_save_and_set_esp_to_scratch(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);

  /* pushf */
  insn_ptr += i386_insn_put_pushf_popf_mode(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, true, mode);
  ASSERT(insn_ptr <= insn_end);

  /* pop <addr> */
  i386_insn_init(&insn_ptr[0]);
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].opc = opc_nametable_find("popl");
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].opc = opc_nametable_find("popq");
  } else NOT_REACHED();
  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.addrsize = 4;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = membase;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = 4;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  insn_ptr += i386_restore_esp_from_scratch(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);
#elif ARCH_SRC == ARCH_X64
  NOT_IMPLEMENTED();
#elif ARCH_SRC == ARCH_PPC
#define GE_INSN 3
#define E_INSN 6
#define END_INSN 7
  /* j[ga]e .ge */
  i386_insn_init(&insn_ptr[0]);
  if (signedness) {
    insn_ptr[0].opc = opc_nametable_find("jge");
  } else {
    insn_ptr[0].opc = opc_nametable_find("jae");
  }
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = GE_INSN - (insn_ptr + 1 - insns); //.ge
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* movl $0x8, addr */
  insn_ptr += i386_insn_put_movl_imm_to_mem(0x8, addr, -1, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);

  /* jmp .end */
  i386_insn_init(insn_ptr);
  insn_ptr[0].opc = opc_nametable_find("jmp");
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = END_INSN - (insn_ptr + 1 - insns);  //.end
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* .ge */
  i386_insn_init(insn_ptr);
  ASSERT(insn_ptr - insns == GE_INSN);
  insn_ptr[0].opc = opc_nametable_find("je");
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = E_INSN - (insn_ptr + 1 - insns); //.e
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* movl $0x4, addr */
  insn_ptr += i386_insn_put_movl_imm_to_mem(0x4, addr, -1, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);

  /* jmp .end */
  i386_insn_init(insn_ptr);
  insn_ptr[0].opc = opc_nametable_find("jmp");
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = END_INSN - (insn_ptr + 1 - insns);  //.end
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* .e: movl $0x2, memnum(SRC_ENV_ADDR) */ \
  ASSERT(insn_ptr - insns == E_INSN);
  insn_ptr += i386_insn_put_movl_imm_to_mem(0x2, addr, -1, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);

  /*.end: cmpl $0x0, xer_ov; je .skip; orl $0x1, xer_ov; .skip: */ \
  ASSERT(insn_ptr - insns == END_INSN);
  /*if (overflow) {
     cmpl $0x0,PPC_REG_XER_OV(SRC_ENV_ADDR) XXX \
    *(ptr)++ = 0x83; *(ptr)++ = 0x3c; *(ptr)++ = 0x25; \
    *(unsigned int *)(ptr)=SRC_ENV_ADDR+(PPC_REG_XER_OV<<2); (ptr)+=4; \
    *(ptr)++ = 0x0; \
    // je .end
    *(ptr)++ = 0x74; *(ptr)++ = 0x7;
    // orl $0x1, memnum(i386_base_reg)  \
    *(ptr)++ = 0x83; *(ptr)++ = 0x04; *(ptr)++ = 0x25; \
    *(unsigned int *)(ptr)=SRC_ENV_ADDR+(memnum<<2); (ptr)+=4; \
    *(ptr)++ = 0x1;
  }*/
#undef GE_INSN
#undef E_INSN
#undef END_INSN
#elif ARCH_SRC == ARCH_ETFG
  /* pushf */
  insn_ptr += i386_insn_put_pushf_popf_mode(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, true, mode);
  ASSERT(insn_ptr <= insn_end);

  insn_ptr += i386_insn_put_push_pop_mem_mode(addr, membase, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, false, mode);
  ASSERT(insn_ptr <= insn_end);
#else
#error "not-implemented"
#endif
  return insn_ptr - insns;
}

static long
i386_insn_put_move_flag_to_reg(int reg, i386_insn_t *insns,
    size_t insns_size, char const *flag_suffix)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  insn_ptr += i386_insn_put_movl_imm_to_reg(0, reg, insn_ptr, insn_end - insn_ptr);
  int orig_reg = reg;
  if (reg >= 4) {
    reg = R_EAX;
    insn_ptr += i386_insn_put_xchg_reg_with_reg(orig_reg, reg, insn_ptr, insn_end - insn_ptr);
    ASSERT(insn_ptr <= insn_end);
  }

  i386_insn_init(&insn_ptr[0]);
  char opcode[64];
  snprintf(opcode, sizeof opcode, "set%s", flag_suffix);
  ASSERT(strlen(opcode) < sizeof opcode);
  insn_ptr[0].opc = opc_nametable_find(opcode);
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  insn_ptr[0].op[0].valtag.reg.val = reg;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = 0xff;
  insn_ptr[0].op[0].size = 1;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  if (orig_reg != reg) {
    insn_ptr += i386_insn_put_xchg_reg_with_reg(reg, orig_reg, insn_ptr, insn_end - insn_ptr);
    ASSERT(insn_ptr <= insn_end);
  }

  return insn_ptr - insns;
}

static long
i386_insn_put_move_flag_to_mem(dst_ulong addr, int membase,
    i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode,
    char const *flag_suffix)
{
  i386_insn_t *insn_ptr = insns;
  i386_insn_t *insn_end = insns + insns_size;

  insn_ptr += i386_insn_put_movl_imm_to_mem(0, addr, membase, insn_ptr, insn_end - insn_ptr);

  // setCC <addr>
  i386_insn_init(insn_ptr);
  char opcode[64];
  snprintf(opcode, sizeof opcode, "set%s", flag_suffix);
  ASSERT(strlen(opcode) < sizeof opcode);
  insn_ptr[0].opc = opc_nametable_find(opcode);
  insn_ptr[0].op[0].type = op_mem;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.addrsize = DST_LONG_SIZE;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = membase;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = 1;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}


//#define gen_reg_to_flags_code(regnum, ptr) do { \
//  *(ptr)++ = 0x50 | regnum; /* push regnum */ \
//  *(ptr)++ = 0x9d; /* popf */ \
//} while (0)
//#define gen_reg_to_flags_code(regnum, ptr) do { \
//  /* andl $0x6, regnum */ \
//  *(ptr)++ = 0x83; *(ptr)++ = 0xe0 | regnum; *(ptr)++ = 0x6; \
//  /* cmpl $0x2, regnum */ \
//  *(ptr)++ = 0x83; *(ptr)++ = 0xf8 | regnum; *(ptr)++ = 0x2; \
//} while (0)
static long
i386_insn_put_move_reg_to_flags(int regnum, fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size,
    i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

#if ARCH_SRC == ARCH_I386
  insn_ptr += i386_save_and_set_esp_to_scratch(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, mode);

  /* push <regnum> */
  insn_ptr += i386_insn_put_push_reg(regnum, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);

  /* popf */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("popf");
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  insn_ptr += i386_restore_esp_from_scratch(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, mode);
#elif ARCH_SRC == ARCH_X64
  NOT_IMPLEMENTED();
#elif ARCH_SRC == ARCH_PPC
  /* andl $0x6, regnum */
  insn_ptr += i386_insn_put_and_imm_with_reg(0x6, regnum, insn_ptr, insn_end - insn_ptr);

  /* cmpl $0x2, regnum */
  insn_ptr += i386_insn_put_cmpl_imm_to_reg(0x2, regnum, insn_ptr, insn_end - insn_ptr);
#elif ARCH_SRC == ARCH_ETFG
  NOT_IMPLEMENTED();
#else
#error "not-implemented"
#endif
  return insn_ptr - insns;
}

//#define gen_flags_to_reg_code(signedness, overflow, regnum, ptr) do { \
//  *(ptr)++ = 0x9c; /* pushf */ \
//  /* mov (%esp), regnum */ \
//  *(ptr)++ = 0x8b; *(ptr)++ = 0x4 | (regnum << 3); *(ptr)++ = 0x24; \
//  *(ptr)++ = 0x9d; /* popf */ \
//} while (0)
//#define gen_flags_to_reg_code(signedness, overflow, regnum, ptr) do { \
//  *(ptr)++ = signedness?0x73:0x7d; *(ptr)++ = 0x7; /* j[ga]e .ge */ \
//  *(ptr)++ = 0xb8 + (regnum); *(unsigned int *)(ptr) = 0x8; ptr+=4; \
//  *(ptr)++ = 0xeb; *(ptr)++ = 0xe; /* jmp .end */ \
//  /*.ge */ \
//  *(ptr)++ = 0x74; *(ptr)++ = 0x07; /* je .e */ \
//  *(ptr)++ = 0xb8 + (regnum); *(unsigned int *)(ptr) = 0x4; ptr+=4; \
//  *(ptr)++ = 0xeb; *(ptr)++ = 0x5; /* jmp .end */ \
//  /*.e */ \
//  *(ptr)++ = 0xb8 + (regnum); *(unsigned int *)(ptr) = 0x2; ptr+=4; \
//  /*.end:  */ \
//  if (overflow) { \
//    if (i386_base_reg == -1) { \
//      /* orl PPC_REG_XER_OV(SRC_ENV_ADDR), regnum * XXX UNCOMMENT THIS. here only to make things faster \
//      *(ptr)++ = 0xb; *(ptr)++ = 0x0 | (regnum << 3) | 0x5; \
//      *(unsigned int *)(ptr) = SRC_ENV_ADDR + (PPC_REG_XER_OV << 2); (ptr)+=4;  \
//    } else { \
//      /* orl PPC_REG_XER_OV(i386_base_reg), regnum  \
//      *(ptr)++ = 0xb; *(ptr)++ = 0x80 | (regnum << 3) | i386_base_reg; \
//      if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//      *(unsigned int *)(ptr) = PPC_REG_XER_OV << 2; (ptr)+=4; */ \
//    } \
//  } \
//} while (0)
static long
i386_insn_put_move_flags_to_reg(bool signedness, int regnum,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

#if ARCH_SRC == ARCH_I386
  insn_ptr += i386_save_and_set_esp_to_scratch(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, mode);

  /* pushf */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("pushf");
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* pop <regnum> */
  insn_ptr += i386_insn_put_pop_reg(regnum, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);

  insn_ptr += i386_restore_esp_from_scratch(fixed_reg_mapping, insn_ptr, insn_end - insn_ptr, mode);
#elif ARCH_SRC == ARCH_X64
  NOT_IMPLEMENTED();
#elif ARCH_SRC == ARCH_PPC
#define GE_INSN 3
#define E_INSN 6
#define END_INSN 7
  /* j[ga]e .ge */
  i386_insn_init(&insn_ptr[0]);
  if (signedness) {
    insn_ptr[0].opc = opc_nametable_find("jge");
  } else {
    insn_ptr[0].opc = opc_nametable_find("jae");
  }
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = GE_INSN - (insn_ptr + 1 - insns); //.ge
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* movl $0x8, regnum */
  insn_ptr += i386_insn_put_movl_imm_to_reg(0x8, regnum, insn_ptr, insn_end - insn_ptr);

  /* jmp .end */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("jmp");
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = END_INSN - (insn_ptr + 1 - insns);  //.end
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /*.ge */
  ASSERT(insn_ptr - insns == GE_INSN);
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("je");
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = E_INSN - (insn_ptr + 1 - insns); //.e
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* movl $0x4, regnum */
  insn_ptr += i386_insn_put_movl_imm_to_reg(0x4, regnum, insn_ptr, insn_end - insn_ptr);

  /* jmp .end */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("jmp");
  insn_ptr[0].op[0].type = op_pcrel;
  insn_ptr[0].op[0].valtag.pcrel.val = END_INSN - (insn_ptr + 1 - insns);  //.end
  insn_ptr[0].op[0].valtag.pcrel.tag = tag_const;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /*.e */
  ASSERT(insn_ptr - insns == E_INSN);
  insn_ptr += i386_insn_put_movl_imm_to_reg(0x2, regnum, insn_ptr, insn_end - insn_ptr);

  /*.end:  */
  ASSERT(insn_ptr - insns == END_INSN);
#undef GE_INSN
#undef E_INSN
#undef END_INSN
#elif ARCH_SRC == ARCH_ETFG
  /* pushf */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("pushf");
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* pop <regnum> */
  insn_ptr += i386_insn_put_pop_reg(regnum, fixed_reg_mapping, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);
#else
#error "not-implemented"
#endif
  return insn_ptr - insns;
}

long
i386_insn_put_move_mem_to_mem(dst_ulong from_addr, dst_ulong to_addr, int from_membase, int to_membase,
    int dead_dst_reg,
    fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size,
    i386_as_mode_t mode)
{
  i386_insn_t *ptr = insns;
  i386_insn_t *end = insns + insns_size;

  if (dead_dst_reg == -1) {
    ptr += i386_insn_put_push_pop_mem_mode(from_addr, from_membase, fixed_reg_mapping, ptr, end - ptr, true, mode);
    ptr += i386_insn_put_push_pop_mem_mode(to_addr, to_membase, fixed_reg_mapping, ptr, end - ptr, false, mode);
  } else {
    ptr += i386_insn_put_movl_mem_to_reg(dead_dst_reg, from_addr, from_membase, ptr, end - ptr);
    ptr += i386_insn_put_movl_reg_to_mem(to_addr, to_membase, dead_dst_reg, ptr, end - ptr);
  }

  return ptr - insns;
}

long
i386_insn_put_move_exreg_to_mem(dst_ulong addr, int membase, int group, int reg,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  switch (group) {
    case I386_EXREG_GROUP_GPRS:
      return i386_insn_put_movl_reg_to_mem(addr, membase, reg, insns, insns_size);
    case I386_EXREG_GROUP_MMX:
      return i386_insn_put_move_mmx_to_mem(addr, membase, reg, insns, insns_size);
    case I386_EXREG_GROUP_XMM:
      return i386_insn_put_move_xmm_to_mem(addr, membase, reg, insns, insns_size);
    case I386_EXREG_GROUP_YMM:
      return i386_insn_put_move_ymm_to_mem(addr, membase, reg, insns, insns_size);
    case I386_EXREG_GROUP_FP_DATA_REGS:
      return i386_insn_put_move_fp_data_reg_to_mem(addr, membase, reg, insns, insns_size);
    case I386_EXREG_GROUP_SEGS:
      return i386_insn_put_move_seg_to_mem(addr, membase, reg, insns, insns_size);
    case I386_EXREG_GROUP_EFLAGS_OTHER:
      return i386_insn_put_move_flags_to_mem(false, addr, membase, fixed_reg_mapping, insns, insns_size, mode);
    case I386_EXREG_GROUP_EFLAGS_EQ:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "e");
    case I386_EXREG_GROUP_EFLAGS_NE:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "ne");
    case I386_EXREG_GROUP_EFLAGS_ULT:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "b");
    case I386_EXREG_GROUP_EFLAGS_SLT:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "l");
    case I386_EXREG_GROUP_EFLAGS_UGT:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "a");
    case I386_EXREG_GROUP_EFLAGS_SGT:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "g");
    case I386_EXREG_GROUP_EFLAGS_ULE:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "be");
    case I386_EXREG_GROUP_EFLAGS_SLE:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "le");
    case I386_EXREG_GROUP_EFLAGS_UGE:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "ae");
    case I386_EXREG_GROUP_EFLAGS_SGE:
      return i386_insn_put_move_flag_to_mem(addr, membase, insns, insns_size, mode, "ge");
    //case I386_EXREG_GROUP_EFLAGS_UNSIGNED:
    //  return i386_insn_put_move_flags_to_mem(false, addr, membase, fixed_reg_mapping, insns, insns_size, mode);
    //case I386_EXREG_GROUP_EFLAGS_SIGNED:
    //  return i386_insn_put_move_flags_to_mem(true, addr, membase, fixed_reg_mapping, insns, insns_size, mode);
    case I386_EXREG_GROUP_ST_TOP:
      return i386_insn_put_move_st_top_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_CONTROL_REG_RM:
      return i386_insn_put_move_fp_control_reg_rm_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_CONTROL_REG_OTHER:
      return i386_insn_put_move_fp_control_reg_other_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_TAG_REG:
      return i386_insn_put_move_fp_tag_reg_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_LAST_INSTR_POINTER:
      return i386_insn_put_move_fp_last_instr_pointer_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER:
      return i386_insn_put_move_fp_last_operand_pointer_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_OPCODE_REG:
      return i386_insn_put_move_fp_opcode_reg_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_MXCSR_RM:
      return i386_insn_put_move_mxcsr_rm_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_C0:
      return i386_insn_put_move_fp_status_reg_c0_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_C1:
      return i386_insn_put_move_fp_status_reg_c1_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_C2:
      return i386_insn_put_move_fp_status_reg_c2_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_C3:
      return i386_insn_put_move_fp_status_reg_c3_to_mem(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_OTHER:
      return i386_insn_put_move_fp_status_reg_other_to_mem(addr, membase, insns, insns_size);
    default:
      cout << __func__ << " " << __LINE__ << ": group = " << group << endl;
      NOT_REACHED();
  }
  NOT_REACHED();
}

static long
i386_insn_put_mul_reg_by_two(int reg, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_shl_reg_by_imm(reg, 1, insns, insns_size);
}

static long
i386_insn_put_div_reg_by_two(int reg, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_shr_reg_by_imm(reg, 1, insns, insns_size);
}

static long
i386_insn_put_mul_mem_by_two(dst_ulong addr, int membase, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  return i386_insn_put_shl_mem_by_imm(addr, membase, 1, insns, insns_size, mode);
}

static long
i386_insn_put_div_mem_by_two(dst_ulong addr, int membase, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  return i386_insn_put_shr_mem_by_imm(addr, membase, 1, insns, insns_size, mode);
}

static long
i386_insn_put_unary_op_reg(int regno, int size, i386_insn_t *insns, size_t insns_size, char const *opc)
{
  i386_insn_t *ptr, *end;

  ptr = insns;
  end = insns + insns_size;

  ASSERT(ptr < end);
  i386_insn_init(&ptr[0]);
  ptr[0].opc = opc_nametable_find(opc);
  ptr[0].op[0].size = size;
  ptr[0].op[0].type = op_reg;
  ptr[0].op[0].valtag.reg.val = regno;
  ptr[0].op[0].valtag.reg.tag = tag_const;
  ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(size * BYTE_LEN);
  i386_insn_add_implicit_operands(&ptr[0]);
  ptr++;

  return ptr - insns;
}

static long
i386_insn_put_negl_reg(int reg, i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_unary_op_reg(reg, 4, insns, insns_size, "negl");
}

static long
i386_insn_put_unary_op_mem(char const *opc, dst_ulong addr,
    int membase, int size, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  i386_insn_t *ptr, *end;

  ptr = insns;
  end = insns + insns_size;

  ASSERT(ptr < end);
  i386_insn_init(&ptr[0]);
  ptr[0].opc = opc_nametable_find(opc);
  ptr[0].op[0].type = op_mem;
  ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  ptr[0].op[0].valtag.mem.base.val = membase;
  ptr[0].op[0].valtag.mem.base.tag = tag_const;
  ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  ptr[0].op[0].valtag.mem.index.val = -1;
  ptr[0].op[0].valtag.mem.index.tag = tag_const;
  ptr[0].op[0].valtag.mem.disp.val = addr;
  ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  ptr[0].op[0].valtag.mem.addrsize = (mode == I386_AS_MODE_32) ? DWORD_LEN/BYTE_LEN : QWORD_LEN/BYTE_LEN;
  ptr[0].op[0].valtag.mem.riprel.val = -1;
  ptr[0].op[0].size = size;
  i386_insn_add_implicit_operands(&ptr[0]);
  ptr++;
  return ptr - insns;
}

static long
i386_insn_put_negl_mem(dst_ulong addr, int membase, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  return i386_insn_put_unary_op_mem("negl", addr, membase, 4, insns, insns_size, mode);
}

static long
i386_insn_put_move_reg_to_flag_lt(int reg, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr = insns;
  i386_insn_t *insn_end = insns + insns_size;
  insn_ptr += i386_insn_put_mul_reg_by_two(reg, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_negl_reg(reg, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_add_imm_with_reg(2, reg, tag_const, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_cmpl_imm_to_reg(reg, 1, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_div_reg_by_two(reg, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

static long
i386_insn_put_move_reg_to_flag_gt(int reg, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr = insns;
  i386_insn_t *insn_end = insns + insns_size;
  insn_ptr += i386_insn_put_mul_reg_by_two(reg, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_cmpl_imm_to_reg(reg, 1, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_div_reg_by_two(reg, insn_ptr, insn_end - insn_ptr);
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

static long
i386_insn_put_move_mem_to_flag_lt(dst_ulong addr, int membase, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr = insns;
  i386_insn_t *insn_end = insns + insns_size;
  insn_ptr += i386_insn_put_mul_mem_by_two(addr, membase, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_negl_mem(addr, membase, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_addl_imm_to_mem(2, addr, membase, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_cmpl_imm_to_mem(1, addr, membase, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_div_mem_by_two(addr, membase, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

static long
i386_insn_put_move_mem_to_flag_gt(dst_ulong addr, int membase, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr = insns;
  i386_insn_t *insn_end = insns + insns_size;
  insn_ptr += i386_insn_put_mul_mem_by_two(addr, membase, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_cmpl_imm_to_mem(1, addr, membase, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);
  insn_ptr += i386_insn_put_div_mem_by_two(addr, membase, insn_ptr, insn_end - insn_ptr, mode);
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}


long
i386_insn_put_move_mem_to_exreg(int group, int reg, dst_ulong addr, int membase,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  switch (group) {
    case I386_EXREG_GROUP_GPRS:
      return i386_insn_put_movl_mem_to_reg(reg, addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_MMX:
      return i386_insn_put_move_mem_to_mmx(reg, addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_XMM:
      return i386_insn_put_move_mem_to_xmm(reg, addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_YMM:
      return i386_insn_put_move_mem_to_ymm(reg, addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_DATA_REGS:
      return i386_insn_put_move_mem_to_fp_data_reg(reg, addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_SEGS:
      return i386_insn_put_move_mem_to_seg(reg, addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_EFLAGS_OTHER:
      return i386_insn_put_move_mem_to_flags(addr, membase, fixed_reg_mapping, insns, insns_size, mode);
    case I386_EXREG_GROUP_EFLAGS_EQ:
      return i386_insn_put_cmpl_imm_to_mem(1, addr, membase, insns, insns_size, mode);
    case I386_EXREG_GROUP_EFLAGS_NE:
      return i386_insn_put_cmpl_imm_to_mem(0, addr, membase, insns, insns_size, mode);
    case I386_EXREG_GROUP_EFLAGS_ULT:
    case I386_EXREG_GROUP_EFLAGS_SLT:
    case I386_EXREG_GROUP_EFLAGS_ULE:
    case I386_EXREG_GROUP_EFLAGS_SLE:
    case I386_EXREG_GROUP_EFLAGS_UGT:
    case I386_EXREG_GROUP_EFLAGS_SGT:
    case I386_EXREG_GROUP_EFLAGS_UGE:
    case I386_EXREG_GROUP_EFLAGS_SGE:
      NOT_REACHED();
    //case I386_EXREG_GROUP_EFLAGS_UNSIGNED:
    //  return i386_insn_put_move_mem_to_flags(addr, membase, fixed_reg_mapping, insns, insns_size, mode);
    case I386_EXREG_GROUP_ST_TOP:
      return i386_insn_put_move_mem_to_st_top(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_CONTROL_REG_RM:
      return i386_insn_put_move_mem_to_fp_control_reg_rm(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_CONTROL_REG_OTHER:
      return i386_insn_put_move_mem_to_fp_control_reg_other(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_TAG_REG:
      return i386_insn_put_move_mem_to_fp_tag_reg(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_LAST_INSTR_POINTER:
      return i386_insn_put_move_mem_to_fp_last_instr_pointer(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER:
      return i386_insn_put_move_mem_to_fp_last_operand_pointer(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_OPCODE_REG:
      return i386_insn_put_move_mem_to_fp_opcode_reg(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_MXCSR_RM:
      return i386_insn_put_move_mem_to_mxcsr_rm(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_C0:
      return i386_insn_put_move_mem_to_fp_status_reg_c0(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_C1:
      return i386_insn_put_move_mem_to_fp_status_reg_c1(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_C2:
      return i386_insn_put_move_mem_to_fp_status_reg_c2(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_C3:
      return i386_insn_put_move_mem_to_fp_status_reg_c3(addr, membase, insns, insns_size);
    case I386_EXREG_GROUP_FP_STATUS_REG_OTHER:
      return i386_insn_put_move_mem_to_fp_status_reg_other(addr, membase, insns, insns_size);
    default:
      cout << __func__ << " " << __LINE__ << ": group = " << group << endl;
      NOT_REACHED();
  }
  NOT_REACHED();
}

static long
i386_insn_put_move_reg_to_exreg(int dst_group, int dst_reg, int src_reg,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns,
    size_t insns_size, i386_as_mode_t mode)
{
  ASSERT(dst_reg >= 0 && dst_reg < I386_NUM_EXREGS(dst_group));
  switch (dst_group) {
    case I386_EXREG_GROUP_GPRS:
      return i386_insn_put_movl_reg_to_reg(dst_reg, src_reg, insns, insns_size);
    case I386_EXREG_GROUP_MMX:
      return i386_insn_put_move_reg_to_mmx(dst_reg, src_reg, insns, insns_size);
    case I386_EXREG_GROUP_XMM:
      return i386_insn_put_move_reg_to_xmm(dst_reg, src_reg, insns, insns_size);
    case I386_EXREG_GROUP_YMM:
      return i386_insn_put_move_reg_to_ymm(dst_reg, src_reg, insns, insns_size);
    case I386_EXREG_GROUP_SEGS:
      return i386_insn_put_move_reg_to_seg(dst_reg, src_reg, insns, insns_size);
    case I386_EXREG_GROUP_EFLAGS_OTHER:
      return i386_insn_put_move_reg_to_flags(src_reg, fixed_reg_mapping, insns, insns_size, mode);
    case I386_EXREG_GROUP_EFLAGS_EQ:
      return i386_insn_put_cmpl_imm_to_reg(src_reg, 1, insns, insns_size);
    case I386_EXREG_GROUP_EFLAGS_NE:
      return i386_insn_put_cmpl_imm_to_reg(src_reg, 0, insns, insns_size);
    case I386_EXREG_GROUP_EFLAGS_ULT:
    case I386_EXREG_GROUP_EFLAGS_SLT:
    case I386_EXREG_GROUP_EFLAGS_ULE:
    case I386_EXREG_GROUP_EFLAGS_SLE:
      //return i386_insn_put_move_reg_to_flag_lt(src_reg, insns, insns_size);
      //return i386_insn_put_cmpl_imm_to_reg(src_reg, 2, insns, insns_size);
    case I386_EXREG_GROUP_EFLAGS_UGT:
    case I386_EXREG_GROUP_EFLAGS_SGT:
    case I386_EXREG_GROUP_EFLAGS_UGE:
    case I386_EXREG_GROUP_EFLAGS_SGE:
      //return i386_insn_put_move_reg_to_flag_gt(src_reg, insns, insns_size);
      NOT_REACHED();
      //return i386_insn_put_cmpl_imm_to_reg(src_reg, 0, insns, insns_size);
    //case I386_EXREG_GROUP_EFLAGS_UNSIGNED:
    //  return i386_insn_put_move_reg_to_flags(src_reg, fixed_reg_mapping, insns, insns_size, mode);
    //case I386_EXREG_GROUP_EFLAGS_SIGNED:
    //  return i386_insn_put_move_reg_to_flags(src_reg, fixed_reg_mapping, insns, insns_size, mode);
    case I386_EXREG_GROUP_ST_TOP:
      return i386_insn_put_move_reg_to_st_top(src_reg, insns, insns_size);
  }
  NOT_REACHED();
}

static long
i386_insn_put_move_exreg_to_reg(int dst_reg, int src_group, int src_reg,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns,
    size_t insns_size, i386_as_mode_t mode)
{
  switch (src_group) {
    case I386_EXREG_GROUP_MMX:
      return i386_insn_put_move_mmx_to_reg(dst_reg, src_reg, insns, insns_size);
    case I386_EXREG_GROUP_XMM:
      return i386_insn_put_move_xmm_to_reg(dst_reg, src_reg, insns, insns_size);
    case I386_EXREG_GROUP_YMM:
      return i386_insn_put_move_ymm_to_reg(dst_reg, src_reg, insns, insns_size);
    case I386_EXREG_GROUP_SEGS:
      return i386_insn_put_move_seg_to_reg(dst_reg, src_reg, insns, insns_size);
    case I386_EXREG_GROUP_EFLAGS_OTHER:
      return i386_insn_put_move_flags_to_reg(false, dst_reg, fixed_reg_mapping, insns, insns_size, mode);
    case I386_EXREG_GROUP_EFLAGS_EQ:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "e");
    case I386_EXREG_GROUP_EFLAGS_NE:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "ne");
    case I386_EXREG_GROUP_EFLAGS_ULT:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "b");
    case I386_EXREG_GROUP_EFLAGS_SLT:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "l");
    case I386_EXREG_GROUP_EFLAGS_UGT:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "a");
    case I386_EXREG_GROUP_EFLAGS_SGT:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "g");
    case I386_EXREG_GROUP_EFLAGS_ULE:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "be");
    case I386_EXREG_GROUP_EFLAGS_SLE:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "le");
    case I386_EXREG_GROUP_EFLAGS_UGE:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "ae");
    case I386_EXREG_GROUP_EFLAGS_SGE:
      return i386_insn_put_move_flag_to_reg(dst_reg, insns, insns_size, "ge");
    //case I386_EXREG_GROUP_EFLAGS_UNSIGNED:
    //  return i386_insn_put_move_flags_to_reg(false, dst_reg, fixed_reg_mapping, insns, insns_size, mode);
    //case I386_EXREG_GROUP_EFLAGS_SIGNED:
    //  return i386_insn_put_move_flags_to_reg(true, dst_reg, fixed_reg_mapping, insns, insns_size, mode);
    case I386_EXREG_GROUP_ST_TOP:
      return i386_insn_put_move_st_top_to_reg(dst_reg, insns, insns_size);
  }
  NOT_REACHED();
}

long
i386_insn_put_move_exreg_to_exreg(int dst_group, int dst_reg, int src_group, int src_reg,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  if (dst_group == src_group && dst_reg == src_reg) {
    return 0;
  }
  if (src_group == I386_EXREG_GROUP_GPRS) {
    return i386_insn_put_move_reg_to_exreg(dst_group, dst_reg, src_reg, fixed_reg_mapping, insns, insns_size, mode);
  } else if (dst_group == I386_EXREG_GROUP_GPRS) {
    return i386_insn_put_move_exreg_to_reg(dst_reg, src_group, src_reg, fixed_reg_mapping, insns, insns_size, mode);
  }
  cout << __func__ << " " << __LINE__ << ": src_group = " << src_group << ", src_reg = " << src_reg << ", dst_group = " << dst_group << ", dst_reg = " << dst_reg << endl;
  NOT_IMPLEMENTED();
}

long
i386_insn_put_xchg_exreg_with_exreg(int group1, int reg1, int group2, int reg2,
    i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  if (group1 == I386_EXREG_GROUP_GPRS && group2 == I386_EXREG_GROUP_GPRS) {
    return i386_insn_put_xchg_reg_with_reg(reg1, reg2, insns, insns_size);
  }
  NOT_IMPLEMENTED();
}

void
i386_operand_copy(operand_t *dst, operand_t const *src)
{
  memcpy(dst, src, sizeof(operand_t));
}

static void
i386_swap_operands(operand_t *op1, operand_t *op2)
{
  operand_t tmp;

  i386_operand_copy(&tmp, op1);
  i386_operand_copy(op1, op2);
  i386_operand_copy(op2, &tmp);
}

bool
i386_insn_get_indir_branch_operand(operand_t *op, i386_insn_t const *insn)
{
  if (insn->opc == opc_calll || i386_insn_is_jecxz(insn)) {
    if (   insn->op[insn->num_implicit_ops].type == op_reg
        || insn->op[insn->num_implicit_ops].type == op_mem) {
      i386_operand_copy(op, &insn->op[insn->num_implicit_ops]);
      return true;
    }
  } else if (i386_insn_is_branch(insn)) {
    i386_operand_copy(op, &insn->op[0]);
    return true;
  }
  return false;
}

long
i386_indir_branch_redir(i386_insn_t *insns, size_t insns_size,
    operand_t const *indir_op, i386_insn_t const *indir_insn,
    transmap_t const *indir_out_tmap,
    transmap_t const *indir_tmap, regset_t const *indir_live_out,
    eqspace::state const *start_state, int memloc_offset_reg,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping, i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;
  operand_t const *op;
  regset_t def, touch;
  operand_t redir_op;
  bool memop_found;
  long i, treg;

#if CALLRET == CALLRET_ID
  if (i386_insn_is_function_return(indir_insn)) {
    i386_insn_copy(&insns[0], indir_insn);
    return 1;
  }
#endif

  ASSERT(indir_op);
  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_get_usedef_regs(indir_insn, &touch, &def);

  regset_union(&touch, &def);
  treg = -1;
  for (i = 0; i < I386_NUM_GPRS; i++) {
    if (!regset_belongs_ex(&touch, I386_EXREG_GROUP_GPRS, i)) {
      treg = i;
      break;
    }
  }
  ASSERT(treg != -1);

  insn_ptr += transmap_translation_insns(indir_out_tmap, indir_tmap,
      start_state, insn_ptr, insn_end - insn_ptr, memloc_offset_reg,
      dst_fixed_reg_mapping, mode);
#if SRC_INSN_ALIGN < 4
  // push %tr0d
  insn_ptr += i386_insn_put_movl_reg_to_mem(SRC_ENV_SCRATCH(1), -1, treg, insn_ptr,
      insn_end - insn_ptr);

  // movl addr, %tr0d
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movl");
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  insn_ptr[0].op[0].valtag.reg.val = treg;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].size = DST_LONG_SIZE;
  i386_insn_add_implicit_operands(insn_ptr);
  i386_operand_copy(&insn_ptr[0].op[1], indir_op);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  // andl $3, %tr0d
  insn_ptr += i386_insn_put_and_imm_with_reg(3, treg, insn_ptr, insn_end - insn_ptr);

  // imul $SRC_ENV_JUMPTABLE_INTERVAL, %tr0d
  insn_ptr += i386_insn_put_imul_imm_with_reg(SRC_ENV_JUMPTABLE_INTERVAL, treg, insn_ptr,
       insn_end - insn_ptr);

  // addl $SRC_ENV_JUMPTABLE_ADDR, %tr0d
  insn_ptr += i386_insn_put_add_imm_with_reg(SRC_ENV_JUMPTABLE_ADDR, treg, tag_const,
      insn_ptr, insn_end - insn_ptr);

  // addl addr, %tr0d
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("addl");
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  insn_ptr[0].op[0].valtag.reg.val = treg;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].size = DST_LONG_SIZE;
  i386_operand_copy(&insn_ptr[0].op[1], indir_op);
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  /* andl $0xfffffffc, %tr0d */
  insn_ptr += i386_insn_put_and_imm_with_reg(0xfffffffc, treg, insn_ptr,
      insn_end - insn_ptr);

  /* movl (%tr0d), %tr0d */
  insn_ptr += i386_insn_put_dereference_reg1_into_reg2(treg, treg, insn_ptr,
      insn_end - insn_ptr);

  /* movl %tr0d, SRC_ENV_SCRATCH0 */
  insn_ptr += i386_insn_put_movl_reg_to_mem(SRC_ENV_SCRATCH0, -1, treg, insn_ptr,
      insn_end - insn_ptr);

  /* popl %tr0d */
  insn_ptr += i386_insn_put_movl_mem_to_reg(treg, SRC_ENV_SCRATCH(1), -1, insn_ptr,
      insn_end - insn_ptr);

  redir_op.type = op_mem;
  //redir_op.tag.mem.all = tag_const;
  redir_op.valtag.mem.addrsize = 4;
  redir_op.valtag.mem.segtype = segtype_sel;
  redir_op.valtag.mem.seg.sel.val = R_DS;
  redir_op.valtag.mem.seg.sel.tag = tag_const;
  redir_op.valtag.mem.seg.sel.mod.reg.mask = 0;
  redir_op.valtag.mem.base.val = -1;
  redir_op.valtag.mem.index.val = -1;
  redir_op.valtag.mem.disp.val = SRC_ENV_SCRATCH0;
  redir_op.valtag.mem.disp.tag = tag_const;
  redir_op.valtag.mem.riprel.val = -1;
  redir_op.size = 4;
#else
  if (indir_op->type == op_mem) {
    // push %tr0d
    insn_ptr += i386_insn_put_movl_reg_to_mem(SRC_ENV_SCRATCH(1), treg, insn_ptr,
        insn_end - insn_ptr);

    // movl addr, %tr0d
    i386_insn_init(&insn_ptr[0]);
    insn_ptr[0].opc = opc_nametable_find("movl");
    insn_ptr[0].op[0].type = op_reg;
    insn_ptr[0].op[0].valtag.reg.tag = tag_const;
    insn_ptr[0].op[0].valtag.reg.val = treg;
    insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
    insn_ptr[0].op[0].size = DST_LONG_SIZE;
    i386_insn_add_implicit_operands(insn_ptr);
    i386_operand_copy(&insn_ptr[0].op[1], indir_op);
    insn_ptr++;
    ASSERT(insn_ptr <= insn_end);

    // addl $SRC_ENV_JUMPTABLE_ADDR, %tr0d
    insn_ptr += i386_insn_put_add_imm_with_reg(SRC_ENV_JUMPTABLE_ADDR, treg, tag_const,
        insn_ptr, insn_end - insn_ptr);

    /* movl (%tr0d), %tr0d */
    insn_ptr += i386_insn_put_dereference_reg1_into_reg2(treg, treg, insn_ptr,
        insn_end - insn_ptr);

    /* movl %tr0d, SRC_ENV_SCRATCH0 */
    insn_ptr += i386_insn_put_movl_reg_to_mem(SRC_ENV_SCRATCH0, treg, insn_ptr,
        insn_end - insn_ptr);

    /* popl %tr0d */
    insn_ptr += i386_insn_put_movl_mem_to_reg(treg, SRC_ENV_SCRATCH(1), insn_ptr,
        insn_end - insn_ptr);

    redir_op.type = op_mem;
    //redir_op.tag.mem.all = tag_const;
    redir_op.valtag.mem.addrsize = 4;
    redir_op.valtag.mem.segtype = segtype_sel;
    redir_op.valtag.mem.seg.sel.val = R_DS;
    redir_op.valtag.mem.seg.sel.tag = tag_const;
    //redir_op.valtag.mem.seg.sel.mod.reg.mask = 0xffff;
    redir_op.valtag.mem.base.val = -1;
    redir_op.valtag.mem.index.val = -1;
    redir_op.valtag.mem.riprel.val = -1;
    redir_op.valtag.mem.disp.val = SRC_ENV_SCRATCH0;
    redir_op.valtag.mem.disp.tag = tag_const;
    redir_op.size = 4;
  } else {
    redir_op.type = op_mem;
    //redir_op.tag.mem.all = tag_const;
    redir_op.valtag.mem.addrsize = DST_LONG_SIZE;
    redir_op.valtag.mem.segtype = segtype_sel;
    redir_op.valtag.mem.seg.sel.val = R_DS;
    redir_op.valtag.mem.seg.sel.tag = tag_const;
    //redir_op.valtag.mem.seg.sel.mod.reg.mask = 0xffff;
    redir_op.valtag.mem.base.val = indir_op->valtag.reg.val;
    redir_op.valtag.mem.base.tag = tag_const;
    redir_op.valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
    redir_op.valtag.mem.index.val = -1;
    redir_op.valtag.mem.disp.val = SRC_ENV_JUMPTABLE_ADDR;
    redir_op.valtag.mem.disp.tag = tag_const;
    redir_op.valtag.mem.riprel.val = -1;
    redir_op.size = 4;
  }
#endif

  /* final control flow transfer instruction */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr->opc = opc_nametable_find("jmp");
#if CALLRET == CALLRET_ID
  if (i386_insn_is_function_call(indir_insn)) {
    insn_ptr->opc = opc_nametable_find("calll");
  }
#endif
  memop_found = false;
  DBG(CONNECT_ENDPOINTS, "before: indir_insn %s\n", i386_insn_to_string(indir_insn, as1, sizeof as1));
  operand2str(&redir_op, NULL, 0, I386_AS_MODE_32, as1, sizeof as1);
  DBG(CONNECT_ENDPOINTS, "redir_op %s\n", as1);
  for (i = I386_MAX_NUM_OPERANDS - 1; i >= 0; i--) {
    operand_t const *op;

    op = &indir_insn->op[i];
    if (   !memop_found
        && (   op->type == op_reg
            || op->type == op_mem
            || op->type == op_pcrel)) {

      operand2str(op, NULL, 0, I386_AS_MODE_32, as1, sizeof as1);
      DBG(CONNECT_ENDPOINTS, "found memop at %d, %s\n", (int)i, as1);
      memop_found = true;
      i386_operand_copy(&insn_ptr->op[i], &redir_op);
    } else {
      i386_operand_copy(&insn_ptr->op[i], &indir_insn->op[i]);
    }
  }
  insn_ptr[0].num_implicit_ops = indir_insn->num_implicit_ops;
  DBG(CONNECT_ENDPOINTS, "after: insn_ptr %s\n", i386_insn_to_string(insn_ptr, as1, sizeof as1));
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  DBG(CONNECT_ENDPOINTS, "returning %ld:\n%s\n", (long)(insn_ptr - insns),
      i386_iseq_to_string(insns, insn_ptr - insns, as1, sizeof as1));
  return insn_ptr - insns;
}


size_t
i386_insn_put_nop_as(char *buf, size_t size)
{
  char *ptr, *end;
  ptr = buf;
  end = buf + size;
  ptr += snprintf(ptr, end - ptr, "nop\n");
  ASSERT(ptr < end);
  return ptr - buf;
}

size_t
i386_insn_put_linux_exit_syscall_as(char *buf, size_t size)
{
  char *ptr, *end;
  ptr = buf;
  end = buf + size;
  ptr += snprintf(ptr, end - ptr, "%s", "movl $1, %eax\nxorl %ebx,%ebx\nint $0x80\n");
  ASSERT(ptr < end);
  return ptr - buf;
}

bool
i386_iseq_rename_regs_and_imms(i386_insn_t *iseq, long iseq_len,
    regmap_t const *regmap, imm_vt_map_t const *imm_map, nextpc_t const *nextpcs,
    long num_nextpcs/*, src_ulong const *insn_pcs, long num_insns*/)
{
  long i, n, val, reg;
  i386_insn_t *insn;
  operand_t *op;

  for (n = 0; n < iseq_len; n++) {
    insn = &iseq[n];
    for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
      op = &insn->op[i];
      if (op->type == op_reg && op->valtag.reg.tag == tag_var) {
        /*if (op->valtag.reg.val >= VREG0_HIGH_8BIT) {
          reg = op->valtag.reg.val - VREG0_HIGH_8BIT;
        } else {
          reg = op->valtag.reg.val;
        }*/
        reg = op->valtag.reg.val;
        ASSERT(reg >= 0 && reg < I386_NUM_GPRS);
        val = regmap_get_elem(regmap, I386_EXREG_GROUP_GPRS, reg).reg_identifier_get_id();
        ASSERT(val >= 0 && val < I386_NUM_GPRS);
        op->valtag.reg.val = val;
        op->valtag.reg.tag = tag_const;
        /*if (reg == op->valtag.reg.val) {
          op->valtag.reg.val = val;
        } else {
          if (val >= 4) {
            return false;
          }
          op->valtag.reg.val = val + 4;
        }*/
      } else if (op->type == op_imm && op->valtag.imm.tag != tag_const) {
        /*op->valtag.imm.val = */imm_vt_map_rename_vconstant(/*op->valtag.imm.val,
            op->valtag.imm.tag,
            op->valtag.imm.mod.imm.modifier, op->valtag.imm.mod.imm.constant,
            op->valtag.imm.mod.imm.sconstants,
            op->valtag.imm.mod.imm.exvreg_group, */
            &op->valtag.imm, imm_map,
            nextpcs, num_nextpcs, regmap/*, tag_const*/);
        //op->valtag.imm.tag = tag_const;
      } else if (op->type == op_mem) {
        //ASSERT(op->tag.mem.all == tag_const);
        if (op->valtag.mem.base.val != -1 && op->valtag.mem.base.tag == tag_var) {
          ASSERT(op->valtag.mem.base.val >= 0);
          ASSERT(op->valtag.mem.base.val < I386_NUM_GPRS);
          val = regmap_get_elem(regmap, I386_EXREG_GROUP_GPRS, op->valtag.mem.base.val).reg_identifier_get_id();
          ASSERT(val >= 0 && val < I386_NUM_GPRS);
          op->valtag.mem.base.tag = tag_const;
          op->valtag.mem.base.val = val;
          op->valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
        }
        if (op->valtag.mem.index.val != -1 && op->valtag.mem.index.tag == tag_var) {
          ASSERT(op->valtag.mem.index.val >= 0);
          ASSERT(op->valtag.mem.index.val < I386_NUM_GPRS);
          val = regmap_get_elem(regmap, I386_EXREG_GROUP_GPRS, op->valtag.mem.index.val).reg_identifier_get_id();
          ASSERT(val >= 0 && val < I386_NUM_GPRS);
          op->valtag.mem.index.tag = tag_const;
          op->valtag.mem.index.val = val;
        }
        if (op->valtag.mem.disp.tag != tag_const) {
          /*op->valtag.mem.disp.val = */imm_vt_map_rename_vconstant(/*
              op->valtag.mem.disp.val,
              op->valtag.mem.disp.tag,
              op->valtag.mem.disp.mod.imm.modifier,
              op->valtag.mem.disp.mod.imm.constant,
              op->valtag.mem.disp.mod.imm.sconstants,
              op->valtag.mem.disp.mod.imm.exvreg_group, */
              &op->valtag.mem.disp, imm_map,
              nextpcs, num_nextpcs, /*insn_pcs, num_insns, */regmap/*, tag_const*/);
          //op->valtag.mem.disp.tag = tag_const;
        }
        op->valtag.mem.seg.sel.val = default_seg(op->valtag.mem.base.val);
      }
    }
  }
  return true;
}


//#define gen_swap_out_code(regno, x86regno, ptr) do { \
//  ASSERT (regno >= 0); \
//  if (i386_base_reg == -1) { \
//    /* mov x86_regno, regno*4(SRC_ENV_ADDR) */ \
//    if (exec) { \
//      *(ptr)++ = 0x48; \
//    } \
//    *(ptr)++ = 0x89;    /* mov */ \
//    *(ptr)++ = 0x4 | ((x86regno) << 3); \
//    *(ptr)++ = 0x25; \
//    *(unsigned int *)(ptr) = SRC_ENV_ADDR+((regno)<<2); ptr += 4;\
//  } else { \
//    /* mov x86_regno, regno*4(i386_base_reg) */ \
//    *(ptr)++ = 0x89;    /* mov */ \
//    *(ptr)++ = (0x1 << 7) | ((x86regno) << 3) | i386_base_reg; \
//    if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//    *(unsigned int *)(ptr) = ((regno)<<2); ptr += 4;\
//  } \
//} while (0)
/*long
i386_gen_swap_out_insns(dst_ulong addr, int regnum, i386_insn_t *insns,
    size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  // mov <regnum>, <addr>
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movl");
  insn_ptr[0].op[0].type = op_mem;
  insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].val.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].val.mem.addrsize = 4;
  insn_ptr[0].op[0].val.mem.seg.sel= R_DS;
  insn_ptr[0].op[0].val.mem.base = -1;
  insn_ptr[0].op[0].val.mem.index = -1;
  insn_ptr[0].op[0].tag.mem.disp = tag_const;
  insn_ptr[0].op[0].val.mem.disp = addr;
  insn_ptr[0].op[0].size = 4;

  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].val.reg = regnum;
  insn_ptr[0].op[1].tag.reg = tag_const;
  insn_ptr[0].op[1].size = 4;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}
*/

//#define gen_swap_in_code(regno, x86regno, ptr) do { \
//  ASSERT (regno >= 0); \
//  if (i386_base_reg == -1) { \
//    /* mov x86_regno, regno*4(SRC_ENV_ADDR) */ \
//    if (exec) { \
//      *(ptr)++ = 0x48; \
//    } \
//    *(ptr)++ = 0x8b;    /* mov */ \
//    *(ptr)++ = 0x4 | ((x86regno) << 3); \
//    *(ptr)++ = 0x25; \
//    *(unsigned int *)(ptr) = SRC_ENV_ADDR+((regno)<<2); ptr += 4;\
//    /**(ptr)++ = 0x90;*/ \
//  } else { \
//    /* mov regno*4(i386_base_reg),x86_regno  */ \
//    *(ptr)++ = 0x8b;    /* mov */ \
//    *(ptr)++ = (0x1 << 7) | ((x86regno) << 3) | i386_base_reg; \
//    if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//    *(unsigned int *)(ptr) = ((regno)<<2); ptr += 4;\
//    /**(ptr)++ = 0x90;*/ \
//  } \
//} while (0)
/*long
i386_gen_swap_in_insns(dst_ulong addr, int regnum, i386_insn_t *insns)
{
  i386_insn_t *insn_ptr;

  insn_ptr = insns;

  // mov <addr>, <regnum>
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movl");
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].val.reg = regnum;
  insn_ptr[0].op[0].tag.reg = tag_const;
  insn_ptr[0].op[0].size = 4;

  insn_ptr[0].op[1].type = op_mem;
  insn_ptr[0].op[1].tag.mem.all = tag_const;
  insn_ptr[0].op[1].val.mem.segtype = segtype_sel;
  insn_ptr[0].op[1].val.mem.addrsize = 4;
  insn_ptr[0].op[1].val.mem.seg.sel= R_DS;
  insn_ptr[0].op[1].val.mem.base = -1;
  insn_ptr[0].op[1].val.mem.index = -1;
  insn_ptr[0].op[1].tag.mem.disp = tag_const;
  insn_ptr[0].op[1].val.mem.disp = addr;
  insn_ptr[0].op[1].size = 4;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;

  return insn_ptr - insns;
}*/

//#define gen_xmm_to_mem_code(xmm_regno, regno, ptr) do { \
//  ASSERT (xmm_regno >= 0); \
//  ASSERT (xmm_regno < I386_NUM_XMM_REGS); \
//  /* movd xmm$regno, $regno*4(%ebp)*/ \
//  /* *(ptr)++=0x66; *(ptr)++=0x0f; *(ptr)++=0x7e; \
//  *(ptr)++=0x80|(xmm_regno<<3)|i386_base_reg; \
//  if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//  *(unsigned int *)(ptr)=regno*4; (ptr)+=4; */ \
//  *(ptr)++=0x0f; *(ptr)++=0x7e; *(ptr)++=0x80|(xmm_regno<<3)|i386_base_reg; \
//  if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//  *(unsigned int *)(ptr)=regno*4; (ptr)+=4; \
//} while (0)
/*long
i386_gen_xmm_to_mem_insns(int regnum, dst_ulong addr, i386_insn_t *insns)
{
  NOT_IMPLEMENTED();
}*/

//#define gen_register_move(x86reg_src, x86reg_dst, ptr) do { \
//  *(ptr)++ = 0x89;    \
//  *(ptr)++ = 0xc0 | ((x86reg_src) << 3) | (x86reg_dst); \
//} while (0)
/*long
i386_gen_register_move_insns(int reg1, int reg2, i386_insn_t *insns)
{
  return i386_insn_put_movl_reg_to_reg(reg1, reg2, insns);
}*/

//#define gen_register_xchg(x86reg_src, x86reg_dst, ptr) do { \
//  *(ptr)++ = 0x87;    \
//  *(ptr)++ = 0xc0 | ((x86reg_src) << 3) | (x86reg_dst); \
//  /**(ptr)++ = 0x90; */ \
//} while (0)
long
i386_insn_put_xchg_reg_with_reg_tag(int reg1, int reg2, src_tag_t tag,
    i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  /* mov <reg2>, <reg1> */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("xchgl");
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.val = reg1;
  insn_ptr[0].op[0].valtag.reg.tag = tag;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].size = DST_LONG_SIZE;
  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.val = reg2;
  insn_ptr[0].op[1].valtag.reg.tag = tag;
  insn_ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[1].size = DST_LONG_SIZE;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_xchg_reg_with_reg_var(int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_xchg_reg_with_reg_tag(reg1, reg2, tag_var, insns, insns_size);
}

long
i386_insn_put_xchg_reg_with_reg(int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size)
{
  return i386_insn_put_xchg_reg_with_reg_tag(reg1, reg2, tag_const, insns, insns_size);
}



//#define gen_reg_to_xmm_code(x86regno, xmm_regno, ptr) do { \
//  ASSERT (xmm_regno >= 0); \
//  ASSERT (xmm_regno < I386_NUM_XMM_REGS); \
//  /* movd xmm$regno, $x86_regno */ \
//  /* *(ptr)++=0x66; *(ptr)++=0x0f; *(ptr)++=0x6e; \
//  *(ptr)++=0xc0|(xmm_regno<<3)|x86regno;*/ \
//  *(ptr)++ = 0x0f; *(ptr)++ = 0x6e; *(ptr)++ = 0xc0|(xmm_regno<<3)|x86regno; \
//} while (0)
/*long
i386_gen_reg_to_xmm_insns(int regnum, int xmmreg, i386_insn_t *insns)
{
  NOT_IMPLEMENTED();
}*/

//#define gen_mem_to_xmm_code(regno, xmm_regno, ptr) do { \
//  ASSERT (xmm_regno >= 0); \
//  ASSERT (xmm_regno < I386_NUM_XMM_REGS); \
//  /* movd $regno*4(%ebp), xmm$regno */ \
//  /* *(ptr)++=0x66; *(ptr)++=0x0f; *(ptr)++=0x6e; \
//  *(ptr)++=0x80|(xmm_regno<<3)|i386_base_reg; \
//  if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//  *(unsigned int *)(ptr)=regno*4; (ptr)+=4; */ \
//  *(ptr)++=0x0f; *(ptr)++=0x6e; *(ptr)++=0x80|(xmm_regno<<3)|i386_base_reg; \
//  if (i386_base_reg == R_ESP) *(ptr)++ = 0x24; \
//  *(unsigned int *)(ptr)=regno*4; (ptr)+=4; \
//} while (0)
/*long
i386_gen_mem_to_xmm_insns(dst_ulong addr, int xmmreg, i386_insn_t *insns)
{
  NOT_IMPLEMENTED();
}*/

//#define gen_xmm_move(x86reg_src, x86reg_dst, ptr) do { \
//  /* movq xmm$x86reg_src, xmm$x86reg_dst */ \
//  /* *(ptr)++ = 0xf3; *(ptr)++ = 0xf; *(ptr)++ = 0x7e; \
//  *(ptr)++ = 0xc0 | ((x86reg_dst) << 3) | (x86reg_src); */ \
//  *(ptr)++ = 0xf; *(ptr)++=0x6f; *(ptr)++=0xc0|(x86reg_dst<<3)|x86reg_src; \
//} while (0)
/*long
i386_gen_xmm_move_insns(int xmm1, int xmm2, i386_insn_t *insns)
{
  NOT_IMPLEMENTED();
}*/

//#define gen_xmm_to_reg_code(xmm_regno, x86regno, ptr) do { \
//  ASSERT (xmm_regno >= 0); \
//  ASSERT (xmm_regno < I386_NUM_XMM_REGS); \
//  /* movd xmm$regno, $x86_regno */ \
//  /* *(ptr)++=0x66; *(ptr)++=0x0f; *(ptr)++=0x7e; \
//  *(ptr)++=0xc0|(xmm_regno<<3)|x86regno; */ \
//  *(ptr)++ = 0x0f; *(ptr)++ = 0x7e; *(ptr)++ = 0xc0|(xmm_regno<<3)|x86regno; \
//} while (0)
/*long
i386_gen_xmm_to_reg_insns(int xmmreg, int regnum, i386_insn_t *insns)
{
  NOT_IMPLEMENTED();
}*/

bool
i386_iseq_types_equal(struct i386_insn_t const *iseq1, long iseq_len1,
    struct i386_insn_t const *iseq2, long iseq_len2)
{
  //XXX : check if all opc's and optypes are equal
  return true;
}

static long
i386_insn_put_function_return_mode(fixed_reg_mapping_t const &fixed_reg_mapping, struct i386_insn_t *insns, size_t insns_size,
    i386_as_mode_t mode)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  if (mode == I386_AS_MODE_32) {
    insn_ptr[0].opc = opc_nametable_find("retl");
  } else if (mode >= I386_AS_MODE_64) {
    insn_ptr[0].opc = opc_nametable_find("retq");
  } else NOT_REACHED();
  /*insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.val = R_ESP;
  insn_ptr[0].op[0].valtag.reg.tag = tag_const;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(DWORD_LEN);
  insn_ptr[0].op[0].size = 4;*/
  i386_insn_add_implicit_operands(insn_ptr, fixed_reg_mapping);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
i386_insn_put_function_return(fixed_reg_mapping_t const &fixed_reg_mapping, struct i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_function_return_mode(fixed_reg_mapping, insns, insns_size, I386_AS_MODE_32);
}

long
i386_insn_put_function_return64(fixed_reg_mapping_t const &fixed_reg_mapping, struct i386_insn_t *insns, size_t insns_size)
{
  return i386_insn_put_function_return_mode(fixed_reg_mapping, insns, insns_size, I386_AS_MODE_64);
}


/*void
i386_iseq_rename_src_pcrels_using_map(i386_insn_t *i386_iseq, long i386_iseq_len,
    long const *map, long map_size)
{
  operand_t *pcrel;
  long i;

  for (i = 0; i < i386_iseq_len; i++) {
    if (pcrel = i386_insn_get_pcrel_operand(&i386_iseq[i])) {
      if (pcrel->tag.pcrel == tag_src_pcrel) {
        ASSERT(pcrel->val.pcrel >= 0 && pcrel->val.pcrel < map_size);
        ASSERT(   map[pcrel->val.pcrel] >= 0
               && map[pcrel->val.pcrel] < i386_iseq_len);
        pcrel->val.pcrel = map[pcrel->val.pcrel] - (i + 1);
        pcrel->tag.pcrel = tag_const;
      }
    }
  }
}*/

void
i386_iseq_get_touch_not_live_in_regs(i386_insn_t const *i386_iseq, long i386_iseq_len,
    transmap_t const *tmap, regset_t *touch_not_live_in_regs)
{
  regset_t def/*, touch*/;
  regcons_t regcons;
  regset_t mapped_to_reserved_regs;
  size_t i386_assembly_size = 4096;
  char *i386_assembly = new char[i386_assembly_size];

  i386_iseq_get_usedef_regs(i386_iseq, i386_iseq_len, touch_not_live_in_regs, &def);
  regset_union(touch_not_live_in_regs, &def);
  i386_iseq_to_string(i386_iseq, i386_iseq_len, i386_assembly, i386_assembly_size);
  i386_infer_regcons_from_assembly(i386_assembly, &regcons);
  mapped_to_reserved_regs = regcons_identify_regs_that_must_be_mapped_to_reserved_regs(&regcons, &i386_reserved_regs);
  regset_diff(touch_not_live_in_regs, &mapped_to_reserved_regs);

  regset_t live_in;
  regset_clear(&live_in);
  transmap_get_used_dst_regs(tmap, &live_in);
  regset_diff(touch_not_live_in_regs, &live_in);
  //regset_count_temporaries(&touch, tmap, temps_count);
  delete [] i386_assembly;
}

bool
i386_insn_less(i386_insn_t const *a, i386_insn_t const *b)
{
  if (a->opc < b->opc) {
    return true;
  }
  return false;
}

bool
i386_insn_supports_execution_test(i386_insn_t const *insn)
{
  char const *opc;
  long i;

  opc = opctable_name(insn->opc);
  if (strstart(opc, "fld", NULL) || strstart(opc, "fst", NULL)) {
    return false;
  }
  if (i386_insn_is_indirect_branch(insn)) {
    return false;
  }
  /*if (i386_insn_accesses_stack(insn)) {
    return false;
  }*/
  if (   !strcmp(opc, "aaa")
      || !strcmp(opc, "aas")
      || !strcmp(opc, "aad")
      || !strcmp(opc, "aam")
      || !strcmp(opc, "daa")
      || !strcmp(opc, "das")
      || (!strcmp(opc, "movw") && insn->op[0].type == op_seg)
      || (!strcmp(opc, "movw") && insn->op[1].type == op_seg)) {
    return false;
  } else if (!strcmp(opc, "rdpmc")) {
    return false;
  } else if (!strcmp(opc, "rdtsc")) {
    return false;
  }
  if (!strcmp(opc, "ldmxcsr")) {
    return false;
  } else if (!strcmp(opc, "stmxcsr")) {
    return false;
  }
  if (strstart(opc, "int", NULL)) {
    return false;
  }
  if (!strcmp(opc, "jecxz") || !strcmp(opc, "jcxz")) {
    return false;
  }
  ASSERT(insn->num_implicit_ops >= 0);
  if (   strstart(opc, "bt", NULL)
      && insn->op[insn->num_implicit_ops].type == op_mem) {
    return false;
  }
  if (   strstart(opc, "bs", NULL)
      && insn->op[insn->num_implicit_ops + 1].type == op_mem) {
    return false;
  }
  if (   !strcmp(opc, "hlt")
      || !strcmp(opc, "cpuid")
      || !strcmp(opc, "sti")
      || !strcmp(opc, "std")
      || !strcmp(opc, "cli")
      || !strcmp(opc, "cld")) {
    return false;
  }
  if (opc[0] == 'f') {
    return false; //floating point
  }
  return true;
}

bool
i386_iseq_supports_execution_test(i386_insn_t const *iseq, long iseq_len)
{
  long i;
  for (i = 0; i < iseq_len; i++) {
    if (!i386_insn_supports_execution_test(&iseq[i])) {
      return false;
    }
  }
  return true;
}

bool
i386_insn_supports_boolean_test(i386_insn_t const *insn)
{
  /*if (i386_insn_has_rep_prefix(insn)) {
    return false;
  }
  if (i386_insn_is_intdiv(insn)) {
    return false;
  }*/
  return true;
}

long
i386_iseq_compute_num_nextpcs(i386_insn_t const *iseq, long n, long *nextpc_indir)
{
  nextpc_t nextpcs[n];
  long i, num_nextpcs;
  bool found_indir;
  operand_t *op;
  valtag_t *vt;

  for (i = 0; i < n; i++) {
    nextpcs[i] = PC_UNDEFINED;
  }
  found_indir = false;
  for (i = 0; i < n; i++) {
    if (i386_insn_is_indirect_branch(&iseq[i])) {
      found_indir = true;
    }
    if (   (vt = i386_insn_get_pcrel_operand(&iseq[i]))
        && vt->tag == tag_var) {
      ASSERT(vt->val >= 0 && vt->val < n);
      nextpcs[vt->val] = vt->val;
    }
    if (   (op = i386_insn_get_imm_operand(&iseq[i]))
        && op->valtag.imm.tag == tag_imm_nextpc) {
      ASSERT(op->valtag.imm.val >= 0 && op->valtag.imm.val < n);
      nextpcs[op->valtag.imm.val] = op->valtag.imm.val;
    }
  }
  if (nextpc_indir) {
    *nextpc_indir = -1;
  }
  if (found_indir && nextpc_indir) {
    for (i = 0; i < n; i++) {
      if (nextpcs[i].get_val() == PC_UNDEFINED) {
        nextpcs[i] = PC_JUMPTABLE;
        *nextpc_indir = i;
        break;
      }
    }
  }
  num_nextpcs = 0;
  for (i = 0; i < n; i++) {
    if (nextpcs[i].get_val() != PC_UNDEFINED) {
      num_nextpcs++;
    }
  }
  return num_nextpcs;
}

bool
i386_iseq_contains_reloc_reference(i386_insn_t const *iseq, long iseq_len)
{
  valtag_t const *pcrel;
  long i;

  for (i = 0; i < iseq_len; i++) {
    if ((pcrel = i386_insn_get_pcrel_operand(&iseq[i]))) {
      if (pcrel->tag == tag_reloc_string) {
        return true;
      }
    }
  }
  return false;
}

bool
i386_insn_merits_optimization(i386_insn_t const *insn)
{
  return true;
}

static void
i386_insn_mark_used_vconstants(imm_vt_map_t *map, imm_vt_map_t *nomem_map,
    imm_vt_map_t *mem_map,
    i386_insn_t const *insn)
{
  operand_t const *op;
  valtag_t const *vt;
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    op = &insn->op[i];
    vt = NULL;
    if (   op->type == op_imm
        && op->valtag.imm.tag == tag_var) {
      vt = &op->valtag.imm;
      imm_vt_map_mark_used_vconstant(nomem_map, vt);
    } else if (   op->type == op_mem
               && op->valtag.mem.disp.tag == tag_var) {
      vt = &op->valtag.mem.disp;
      imm_vt_map_mark_used_vconstant(mem_map, vt);
    }
    if (vt) {
      imm_vt_map_mark_used_vconstant(map, vt);
    }
  }
}

void
i386_iseq_mark_used_vconstants(imm_vt_map_t *map, imm_vt_map_t *nomem_map,
    imm_vt_map_t *mem_map,
    i386_insn_t const *iseq, long iseq_len)
{
  long i;

  imm_vt_map_init(map);
  imm_vt_map_init(mem_map);
  imm_vt_map_init(nomem_map);
  for (i = 0; i < iseq_len; i++) {
    i386_insn_mark_used_vconstants(map, nomem_map, mem_map, &iseq[i]);
  }
}

long
i386_insn_put_movl_nextpc_constant_to_mem(long nextpc_num, uint32_t addr,
    i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("movl");
  insn_ptr[0].op[0].type = op_mem;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = -1;
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = 4;

  insn_ptr[0].op[1].type = op_imm;
  insn_ptr[0].op[1].valtag.imm.tag = tag_imm_nextpc;
  insn_ptr[0].op[1].valtag.imm.val = nextpc_num;
  insn_ptr[0].op[1].valtag.imm.mod.imm.modifier = IMM_UNMODIFIED;
  insn_ptr[0].op[1].size = 4;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  if (insn_ptr > insn_end) {
    print_backtrace();
  }
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

/*static long
i386_insn_put_jump_to_abs_inum(long inum, i386_insn_t *insns, long size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insn_ptr + size;

  i386_insn_init(insn_ptr);
  insn_ptr->opc = opc_nametable_find("jmp");
  insn_ptr->op[0].type = op_pcrel;
  insn_ptr->op[0].valtag.pcrel.val = inum;
  insn_ptr->op[0].valtag.pcrel.tag = tag_abs_inum;
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}*/

void
i386_iseq_rename_nextpc_consts(i386_insn_t *iseq, long iseq_len,
    uint8_t const * const next_tcodes[], long num_nextpcs)
{
  long i, j, nextpc_num;
  i386_insn_t *insn;
  operand_t *op;

  for (i = 0; i < iseq_len; i++) {
    insn = &iseq[i];
    for (j = 0; j < I386_MAX_NUM_OPERANDS; j++) {
      int max_op_size;

      op = &insn->op[j];
      if (op->type != op_imm) {
        continue;
      }
      if (op->valtag.imm.tag != tag_imm_nextpc) {
        continue;
      }
      max_op_size = i386_max_operand_size(op, insn->opc, j - insn->num_implicit_ops);
      ASSERT(op->valtag.imm.mod.imm.modifier == IMM_UNMODIFIED);
      nextpc_num = op->valtag.imm.val;
      ASSERT(   (nextpc_num >= 0 && nextpc_num < num_nextpcs)
             || nextpc_num == NEXTPC_TYPE_ERROR);
      ASSERT(   next_tcodes[nextpc_num] >= (uint8_t const *)NULL
             && next_tcodes[nextpc_num] < (uint8_t const *)0xffffffff);
      op->valtag.imm.val =
          (uint32_t)(long)next_tcodes[nextpc_num] & ((1ULL << (max_op_size * BYTE_LEN)) - 1);
      op->valtag.imm.tag = tag_const;
    }
  }
}

void
i386_insn_collect_typestate_constraints(typestate_constraints_t *tscons,
    i386_insn_t const *insn, state_t *state, typestate_t *varstate)
{
  NOT_IMPLEMENTED();
}

long
i386_insn_put_not_reg(int regno, int size, i386_insn_t *insns, size_t insns_size)
{
  if (size == 2) {
    return i386_insn_put_unary_op_reg(regno, size, insns, insns_size, "notw");
  } else if (size == 4) {
    return i386_insn_put_unary_op_reg(regno, size, insns, insns_size, "notl");
  } else NOT_REACHED();
}

static void
i386_insn_get_used_vconst_regs(i386_insn_t const *insn, regset_t *use)
{
  valtag_t const *vt;
  int i, group;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_imm) {
      vt = &insn->op[i].valtag.imm;
    } else if (insn->op[i].type == op_mem) {
      vt = &insn->op[i].valtag.mem.disp;
    } else {
      continue;
    }

    if (vt->tag != tag_imm_exvregnum) {
      continue;
    }
    group = vt->mod.imm.exvreg_group;
    ASSERT(group >= 0 && group < I386_NUM_EXREG_GROUPS);
    ASSERT(vt->val >= 0 && vt->val < I386_NUM_EXREGS(group));
    regset_mark_ex_mask(use, group, vt->val, MAKE_MASK(I386_EXREG_LEN(group)));
  }
}

void
i386_iseq_get_used_vconst_regs(i386_insn_t const *iseq, long iseq_len, regset_t *use)
{
  regset_t iuse;
  long i;

  regset_clear(use);
  for (i = 0; i < iseq_len; i++) {
    i386_insn_get_used_vconst_regs(&iseq[i], &iuse);
    regset_union(use, &iuse);
  }
}

void
i386_insn_get_min_max_mem_vconst_masks(int *min_mask, int *max_mask,
    i386_insn_t const *insn)
{
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    switch (insn->op[i].type) {
      case op_mem:
        imm_valtag_get_min_max_vconst_masks(min_mask, max_mask,
            &insn->op[i].valtag.mem.disp);
        break;
      default:
        break;
    }
  }
}

void
i386_insn_get_min_max_vconst_masks(int *min_mask, int *max_mask,
    i386_insn_t const *insn)
{
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    switch (insn->op[i].type) {
      case op_imm:
        imm_valtag_get_min_max_vconst_masks(min_mask, max_mask,
            &insn->op[i].valtag.imm);
        break;
      default:
        break;
    }
  }
}

static long
i386_insn_put_movl_nomem_op_to_regmem_offset(int r_esp, int r_ss, src_tag_t regno_tag,
    int32_t offset, struct operand_t const *op, int opsize,
    i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  i386_insn_init(&insn_ptr[0]);
  if (opsize == 8) {
    insn_ptr[0].opc = opc_nametable_find("movq");
  } else if (opsize == 4) {
    insn_ptr[0].opc = opc_nametable_find("movl");
  } else if (opsize == 2) {
    insn_ptr[0].opc = opc_nametable_find("movw");
  } else if (opsize == 1) {
    insn_ptr[0].opc = opc_nametable_find("movb");
  } else NOT_REACHED();
  insn_ptr[0].op[0].type = op_mem;
  //insn_ptr[0].op[0].tag.mem.all = tag_const;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = regno_tag;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = r_ss;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = r_esp;
  insn_ptr[0].op[0].valtag.mem.base.tag = regno_tag;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = offset;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = opsize;

  ASSERT(op->type == op_reg || op->type == op_imm);
  i386_operand_copy(&insn_ptr[0].op[1], op);
  /* insn_ptr[0].op[1].type = op_imm;
  insn_ptr[0].op[1].valtag.imm.val = imm;
  insn_ptr[0].op[1].rex_used = 0;
  insn_ptr[0].op[1].valtag.imm.tag = tag_const;
  insn_ptr[0].op[1].size = 1; */
  i386_insn_add_implicit_operands(insn_ptr);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static void
regmem_offset_to_op(operand_t *op, int r_esp, int r_ss, src_tag_t regno_tag,
    int32_t offset, int opsize)
{
  op->type = op_mem;
  op->valtag.mem.segtype = segtype_sel;
  op->valtag.mem.seg.sel.tag = regno_tag;
  op->valtag.mem.seg.sel.val = r_ss;
  //op->valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  op->valtag.mem.base.val = r_esp;
  op->valtag.mem.base.tag = regno_tag;
  op->valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  op->valtag.mem.index.val = -1;
  op->valtag.mem.disp.val = offset;
  op->valtag.mem.disp.tag = tag_const;
  op->valtag.mem.riprel.val = -1;
  op->size = opsize;
}

static long
i386_insn_put_movl_mem_op2_to_mem_op1(operand_t const *op1, operand_t const *op2,
    unsigned long scratch_addr, i386_insn_t *insns, size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;
  int tmpreg, i, opsize;
  src_tag_t regno_tag;
  opc_t mov_opc;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  //ASSERT(op1->size == op2->size); //XXX: debug why this assertion is failing
  opsize = (op1->size <= op2->size) ? op1->size : op2->size;

  tmpreg = -1;
  for (i = 0; i < I386_NUM_EXREGS(I386_EXREG_GROUP_GPRS); i++) {
    if (   op1->valtag.mem.base.val != i
        && op1->valtag.mem.index.val != i
        && op2->valtag.mem.base.val != i
        && op2->valtag.mem.base.val != i) {
      tmpreg = i;
      break;
    }
  }
  ASSERT(tmpreg != -1);

  if (opsize == 8) {
    mov_opc = opc_nametable_find("movq");
  } else if (opsize == 4) {
    mov_opc = opc_nametable_find("movl");
  } else if (opsize == 2) {
    mov_opc = opc_nametable_find("movw");
  } else if (opsize == 1) {
    mov_opc = opc_nametable_find("movb");
  } else NOT_REACHED();

  ASSERT(op1->valtag.mem.base.val != -1 || op1->valtag.mem.disp.tag != tag_const);
  ASSERT(op2->valtag.mem.base.val != -1 || op2->valtag.mem.disp.tag != tag_const);
  //ASSERT(op1->valtag.mem.base.tag == op2->valtag.mem.base.tag);

  if (op1->valtag.mem.base.val == -1) {
    regno_tag = op1->valtag.mem.base.tag;
  } else {
    regno_tag = op1->valtag.mem.disp.tag;
  }
  ASSERT(regno_tag == tag_const || regno_tag == tag_var);

  /* mov tmpreg, scratch_addr */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = mov_opc;
  insn_ptr[0].op[0].type = op_mem;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = op1->valtag.mem.seg.sel.tag;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = op1->valtag.mem.seg.sel.val;
  //insn_ptr[0].op[0].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[0].valtag.mem.base.val = -1;
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = scratch_addr;
  insn_ptr[0].op[0].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = opsize;
  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.val = tmpreg;
  insn_ptr[0].op[1].valtag.reg.tag = regno_tag;
  insn_ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(opsize * BYTE_LEN);
  insn_ptr[0].op[1].size = opsize;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;

  /* mov op2, tmpreg */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = mov_opc;
  i386_operand_copy(&insn_ptr[0].op[1], op2);
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.val = tmpreg;
  insn_ptr[0].op[0].valtag.reg.tag = regno_tag;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(opsize * BYTE_LEN);
  insn_ptr[0].op[0].size = opsize;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;

  /* mov tmpreg, op1 */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = mov_opc;
  i386_operand_copy(&insn_ptr[0].op[0], op1);
  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.val = tmpreg;
  insn_ptr[0].op[1].valtag.reg.tag = regno_tag;
  insn_ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(opsize * BYTE_LEN);
  insn_ptr[0].op[1].size = opsize;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;

  /* mov scratch_addr, tmpreg */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = mov_opc;
  insn_ptr[0].op[1].type = op_mem;
  insn_ptr[0].op[1].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[1].valtag.mem.seg.sel.tag = op1->valtag.mem.seg.sel.tag;
  insn_ptr[0].op[1].valtag.mem.seg.sel.val = op1->valtag.mem.seg.sel.val;
  //insn_ptr[0].op[1].valtag.mem.seg.sel.mod.reg.mask = 0xffff;
  insn_ptr[0].op[1].valtag.mem.base.val = -1;
  insn_ptr[0].op[1].valtag.mem.index.val = -1;
  insn_ptr[0].op[1].valtag.mem.disp.val = scratch_addr;
  insn_ptr[0].op[1].valtag.mem.disp.tag = tag_const;
  insn_ptr[0].op[1].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[1].size = opsize;
  insn_ptr[0].op[0].type = op_reg;
  insn_ptr[0].op[0].valtag.reg.val = tmpreg;
  insn_ptr[0].op[0].valtag.reg.tag = regno_tag;
  insn_ptr[0].op[0].valtag.reg.mod.reg.mask = MAKE_MASK(opsize * BYTE_LEN);
  insn_ptr[0].op[0].size = opsize;
  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;

  return insn_ptr - insns;
}

static long
i386_insn_put_movl_mem_op_to_regmem_offset(int r_esp, int r_ss, src_tag_t regno_tag,
    int32_t offset, struct operand_t const *op, int opsize, unsigned long scratch_addr,
    i386_insn_t *insns, size_t insns_size)
{
  operand_t tmp_op;

  regmem_offset_to_op(&tmp_op, r_esp, r_ss, regno_tag, offset, opsize);

  return i386_insn_put_movl_mem_op2_to_mem_op1(&tmp_op, op, scratch_addr, insns,
      insns_size);
}

static long
i386_insn_put_movl_regmem_offset_to_mem_op(struct operand_t const *op, int r_esp,
    int r_ss, src_tag_t regno_tag,
    int32_t offset, int opsize, unsigned long scratch_addr,
    i386_insn_t *insns, size_t insns_size)
{
  operand_t tmp_op;

  regmem_offset_to_op(&tmp_op, r_esp, r_ss, regno_tag, offset, opsize);

  return i386_insn_put_movl_mem_op2_to_mem_op1(op, &tmp_op, scratch_addr, insns,
      insns_size);
}

long
i386_insn_put_movl_op_to_regmem_offset(int r_esp, int r_ss, src_tag_t regno_tag,
    int32_t offset, struct operand_t const *op, int opsize,
    unsigned long scratch_addr, i386_insn_t *insns, size_t insns_size)
{
  if (op->type == op_mem) {
    return i386_insn_put_movl_mem_op_to_regmem_offset(r_esp, r_ss, regno_tag, offset, op,
        opsize, scratch_addr, insns, insns_size);
  }
  return i386_insn_put_movl_nomem_op_to_regmem_offset(r_esp, r_ss, regno_tag,
      offset, op, opsize, insns, insns_size);
}

long
i386_insn_put_movl_regmem_offset_to_op(struct operand_t const *op,
    int r_esp, int r_ss, src_tag_t regno_tag, int32_t offset, int opsize,
    unsigned long scratch_addr, i386_insn_t *insns, size_t insns_size)
{
  if (op->type == op_mem) {
    return i386_insn_put_movl_regmem_offset_to_mem_op(op, r_esp, r_ss, regno_tag,
        offset, opsize, scratch_addr, insns, insns_size);
  }

  long ret;

  ret = i386_insn_put_movl_nomem_op_to_regmem_offset(r_esp, r_ss, regno_tag, offset, op,
      opsize, insns, insns_size);
  ASSERT(ret == 1);
  i386_swap_operands(&insns->op[0], &insns->op[1]);
  return ret;
}

bool
i386_insn_type_signature_supported(i386_insn_t const *insn)
{
  return true;
}

char *
i386_exregname_suffix(int groupnum, int exregnum, char const *suffix, char *buf,
    size_t size)
{
  char *ptr, *end;
  bool high_order;
  int regsize;

  ptr = buf;
  end = buf + size;

  //char const *segs[] = {"es", "cs", "ss", "ds", "fs", "gs"};
  //size_t const num_segs = sizeof segs/sizeof segs[0];
  ASSERT(exregnum >= 0 && exregnum < X64_NUM_EXREGS(groupnum));
  if (groupnum == I386_EXREG_GROUP_GPRS) {
    high_order = (suffix[0] == 'B')?true:false;
    if (suffix[0] == 'b') {
      regsize = 1;
    } else if (suffix[0] == 'B') {
      regsize = 1;
    } else if (suffix[0] == 'w') {
      regsize = 2;
    } else if (suffix[0] == 'd') {
      regsize = 4;
    } else if (suffix[0] == 'q') {
      regsize = 8;
    } else {
      cout << __func__ << " " << __LINE__ << ": groupnum = " << groupnum << ", exregnum = " << exregnum << ", suffix = " << suffix << ", DST_EXREG_GROUP_GPRS = " << DST_EXREG_GROUP_GPRS << ", X64_EXREG_GROUP_GPRS = " << X64_EXREG_GROUP_GPRS << endl;
      NOT_REACHED();
    }
    i386_regname_bytes(exregnum, regsize, high_order, ptr, end - ptr);
  } else if (groupnum == I386_EXREG_GROUP_MMX) {
    ptr += snprintf(ptr, end - ptr, "%%mm%d", exregnum);
  } else if (groupnum == I386_EXREG_GROUP_XMM) {
    ASSERT(exregnum >= 0 && exregnum < X64_NUM_EXREGS(groupnum));
    ptr += snprintf(ptr, end - ptr, "%%xmm%d", exregnum);
  } else if (groupnum == I386_EXREG_GROUP_YMM) {
    ptr += snprintf(ptr, end - ptr, "%%ymm%d", exregnum);
  } else if (groupnum == I386_EXREG_GROUP_SEGS) {
    ptr += snprintf(ptr, end - ptr, "%%%s", segs[exregnum]);
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_OTHER) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-other");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_EQ) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-eq");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_NE) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-ne");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_ULT) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-ult");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_SLT) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-slt");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_UGT) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-ugt");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_SGT) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-sgt");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_ULE) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-ule");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_SLE) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-sle");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_UGE) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-uge");
  } else if (groupnum == I386_EXREG_GROUP_EFLAGS_SGE) {
    ptr += snprintf(ptr, end - ptr, "%%eflags-sge");
  //} else if (groupnum == I386_EXREG_GROUP_EFLAGS_UNSIGNED) {
  //  ptr += snprintf(ptr, end - ptr, "%%eflags-unsigned");
  //} else if (groupnum == I386_EXREG_GROUP_EFLAGS_SIGNED) {
  //  ptr += snprintf(ptr, end - ptr, "%%eflags-signed");
  } else if (groupnum == I386_EXREG_GROUP_ST_TOP) {
    ptr += snprintf(ptr, end - ptr, "%%st_top");
  } else if (groupnum == I386_EXREG_GROUP_FP_DATA_REGS) {
    ptr += snprintf(ptr, end - ptr, "%%fp_data_regs");
  } else if (groupnum == I386_EXREG_GROUP_FP_CONTROL_REG_RM) {
    ptr += snprintf(ptr, end - ptr, "%%fp_control_reg_rm");
  } else if (groupnum == I386_EXREG_GROUP_FP_CONTROL_REG_OTHER) {
    ptr += snprintf(ptr, end - ptr, "%%fp_control_reg_other");
  } else if (groupnum == I386_EXREG_GROUP_FP_TAG_REG) {
    ptr += snprintf(ptr, end - ptr, "%%fp_tag_reg");
  } else if (groupnum == I386_EXREG_GROUP_FP_LAST_INSTR_POINTER) {
    ptr += snprintf(ptr, end - ptr, "%%fp_last_instr_pointer");
  } else if (groupnum == I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER) {
    ptr += snprintf(ptr, end - ptr, "%%fp_last_operand_pointer");
  } else if (groupnum == I386_EXREG_GROUP_FP_OPCODE_REG) {
    ptr += snprintf(ptr, end - ptr, "%%fp_opcode_reg");
  } else if (groupnum == I386_EXREG_GROUP_MXCSR_RM) {
    ptr += snprintf(ptr, end - ptr, "%%mxcsr_rm");
  } else if (groupnum == I386_EXREG_GROUP_FP_STATUS_REG_C0) {
    ptr += snprintf(ptr, end - ptr, "%%fp_status_reg_c0");
  } else if (groupnum == I386_EXREG_GROUP_FP_STATUS_REG_C1) {
    ptr += snprintf(ptr, end - ptr, "%%fp_status_reg_c1");
  } else if (groupnum == I386_EXREG_GROUP_FP_STATUS_REG_C2) {
    ptr += snprintf(ptr, end - ptr, "%%fp_status_reg_c2");
  } else if (groupnum == I386_EXREG_GROUP_FP_STATUS_REG_C3) {
    ptr += snprintf(ptr, end - ptr, "%%fp_status_reg_c3");
  } else if (groupnum == I386_EXREG_GROUP_FP_STATUS_REG_OTHER) {
    ptr += snprintf(ptr, end - ptr, "%%fp_status_reg_other");
  } else NOT_REACHED();
  return buf;
}


static bool
mem_operands_equal_mod_imms(operand_t const *op1, operand_t const *op2)
{
  ASSERT(op1->type == op_mem);
  ASSERT(op2->type == op_mem);

  return (   op1->valtag.mem.segtype == op2->valtag.mem.segtype
          && op1->valtag.mem.seg.sel.tag == op2->valtag.mem.seg.sel.tag
          && op1->valtag.mem.seg.desc.tag == op2->valtag.mem.seg.desc.tag
          && base_index_scale_equal(&op1->valtag.mem.base, &op1->valtag.mem.index,
                 op1->valtag.mem.scale, &op2->valtag.mem.base, &op2->valtag.mem.index,
                 op2->valtag.mem.scale)
          && reg_valtags_equal(&op1->valtag.mem.riprel, &op2->valtag.mem.riprel)
         );
}

static bool
operands_equal_mod_imms(operand_t const *op1, operand_t const *op2)
{
  if (op1->type != op2->type) {
    return false;
  }
  if (op1->size != op2->size) {
    return false;
  }
  if (op1->type == invalid) {
    return true;
  }
  switch (op1->type) {
    case op_mem:
      return mem_operands_equal_mod_imms(op1, op2);
    case op_imm:
      return true;
    default:
      return operands_equal(op1, op2);
  }
}

static bool
i386_insns_equal_mod_imms(i386_insn_t const *i1, i386_insn_t const *i2)
{
  long i;

  if (   i386_insn_is_nop(i1)
      && i386_insn_is_nop(i2)) {
    return true;
  }
  if (i1->opc != i2->opc) {
    return false;
  }
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (!operands_equal_mod_imms(&i1->op[i], &i2->op[i])) {
      DBG(EQUIV, "operands %ld not equal\n", i);
      return false;
    }
  }
  return true;
}

bool
i386_iseqs_equal_mod_imms(struct i386_insn_t const *iseq1, long iseq1_len,
    struct i386_insn_t const *iseq2, long iseq2_len)
{
  long i;

  if (iseq1_len != iseq2_len) {
    return false;
  }

  for (i = 0; i < iseq1_len; i++) {
    if (!i386_insns_equal_mod_imms(&iseq1[i], &iseq2[i])) {
      DBG(EQUIV, "insns %ld not equal\n", i);
      return false;
    }
  }
  return true;
}

bool
i386_iseq_contains_div(i386_insn_t const *iseq, long iseq_len)
{
  long i;
  for (i = 0; i < iseq_len; i++) {
    if (i386_insn_is_intdiv(&iseq[i])) {
      return true;
    }
    if (strstart(opctable_name(iseq[i].opc), "div", NULL)) {
      return true;
    }
  }
  return false;
}

static void
i386_operand_canonicalize_locals_symbols(operand_t *op, imm_vt_map_t *imm_vt_map, src_tag_t tag)
{
  if (op->type == op_imm) {
    valtag_canonicalize_local_symbol(&op->valtag.imm, imm_vt_map, tag);
  } else if (op->type == op_mem) {
    valtag_canonicalize_local_symbol(&op->valtag.mem.disp, imm_vt_map, tag);
  }
}

void
i386_insn_canonicalize_locals_symbols(i386_insn_t *insn, imm_vt_map_t *imm_vt_map, src_tag_t tag)
{
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    i386_operand_canonicalize_locals_symbols(&insn->op[i], imm_vt_map, tag);
  }
  //memvt_list_canonicalize_locals_symbols(&insn->memtouch, imm_vt_map, tag); //XXX
}

static bool
i386_operand_inv_rename_locals_symbols(operand_t *op, imm_vt_map_t *imm_vt_map, src_tag_t tag)
{
  if (op->type == op_imm) {
    if (!valtag_inv_rename_local_symbol(&op->valtag.imm, imm_vt_map, tag)) {
      return false;
    }
  } else if (op->type == op_mem) {
    if (!valtag_inv_rename_local_symbol(&op->valtag.mem.disp, imm_vt_map, tag)) {
      return false;
    }
  }
  return true;
}

bool
i386_insn_inv_rename_locals_symbols(i386_insn_t *i386_insn, imm_vt_map_t *imm_vt_map, src_tag_t tag)
{
  bool ret;
  int i;

  ret = true;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (!i386_operand_inv_rename_locals_symbols(&i386_insn->op[i], imm_vt_map, tag)) {
      //DBG(PEEP_TAB, "returning false for inv_renaming %dth operand\n", i);
      ret = false;
    }
  }
  return ret;
}

static void
i386_operand_rename_nextpcs(operand_t *op, nextpc_map_t const *nextpc_map)
{
  valtag_t const *vt;
  if (op->type != op_pcrel) {
    return;
  }
  if (op->valtag.pcrel.tag != tag_var) {
    return;
  }
  ASSERT(   op->valtag.pcrel.val >= 0
         && op->valtag.pcrel.val < nextpc_map->num_nextpcs_used);
  vt = &nextpc_map->table[op->valtag.pcrel.val];
  ASSERT(vt->tag == tag_var);
  valtag_copy(&op->valtag.pcrel, vt);
}


static bool
i386_operand_symbols_are_contained_in_map(operand_t const *op,
    struct imm_vt_map_t const *imm_vt_map)
{
  if (op->type == op_imm) {
    return valtag_symbols_are_contained_in_map(&op->valtag.imm, imm_vt_map);
  } else if (op->type == op_mem) {
    return valtag_symbols_are_contained_in_map(&op->valtag.mem.disp, imm_vt_map);
  }
  return true;
}

bool
i386_insn_symbols_are_contained_in_map(struct i386_insn_t const *insn,
    struct imm_vt_map_t const *imm_vt_map)
{
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (!i386_operand_symbols_are_contained_in_map(&insn->op[i], imm_vt_map)) {
      return false;
    }
  }
  return true;
}

static bool
symbol_is_contained_in_i386_operand(operand_t const *op, long symval, src_tag_t symtag)
{
  if (op->type == op_imm) {
    return symbol_is_contained_in_valtag(&op->valtag.imm, symval, symtag);
  } else if (op->type == op_mem) {
    return symbol_is_contained_in_valtag(&op->valtag.mem.disp, symval, symtag);
  }
  return false;
}

bool
symbol_is_contained_in_i386_insn(struct i386_insn_t const *insn, long symval,
    src_tag_t symtag)
{
  int i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (symbol_is_contained_in_i386_operand(&insn->op[i], symval, symtag)) {
      return true;
    }
  }
  return false;
}

void
i386_insn_copy_symbols(i386_insn_t *dst, i386_insn_t const *src)
{
  int i;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    ASSERT(dst->op[i].type == src->op[i].type);
    if (   dst->op[i].type == op_mem
        && valtag_contains_symbol(&dst->op[i].valtag.mem.disp)) {
      valtag_copy(&dst->op[i].valtag.mem.disp, &src->op[i].valtag.mem.disp);
    } else if (   dst->op[i].type == op_imm
               && valtag_contains_symbol(&dst->op[i].valtag.imm)) {
      valtag_copy(&dst->op[i].valtag.imm, &src->op[i].valtag.imm);
    }
  }
  //memvt_list_copy(&dst->memtouch, &src->memtouch);
}

#if 0
#define ANNOTATE_LOCAL(field, insn, op_index, regtype, rt_index) do { \
      if (insn->op[i].valtag.field.tag != tag_const) { \
        return; /*locals have already been annotated; just return */ \
      } \
      ASSERT(insn->op[i].valtag.field.tag == tag_const); \
      printf("changing insn from:\n%s\n", i386_insn_to_string(insn, as1, sizeof as1)); \
      long orig_val = insn->op[i].valtag.field.val; \
      insn->op[i].valtag.field.val = rt_index; \
      insn->op[i].valtag.field.tag = regtype; \
      insn->op[i].valtag.field.mod.imm.modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0; \
      insn->op[i].valtag.field.mod.imm.sconstants[2] = 1; \
      insn->op[i].valtag.field.mod.imm.sconstants[1] = orig_val - offset; \
      insn->op[i].valtag.field.mod.imm.sconstants[0] = 32; \
      printf("\nto:\n%s\n", i386_insn_to_string(insn, as1, sizeof as1)); \
} while(0)

void
i386_insn_annotate_local(i386_insn_t *insn, int op_index, memlabel_t const *memlabel, src_tag_t regtype, int rt_index, long offset)
{
  int i;
  if (op_index < INSN_MEMTOUCH_OP_INDEX_IN_COMMENTS) {
    for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
      if (insn->op[i].type == op_mem) {
        ANNOTATE_LOCAL(mem.disp, insn, op_index, regtype, rt_index);
      }
    }
  } else {
    int memtouch_index = op_index - INSN_MEMTOUCH_OP_INDEX_IN_COMMENTS;
    ASSERT(memtouch_index >= 0 && memtouch_index < insn->memtouch.n_elem);
    //memlabel_t memlabel;
    //memlabel_populate_using_regtype_and_index(&memlabel, regtype, rt_index);
    DBG(ANNOTATE_LOCALS, "annotating mls[%d] with %s. regtype = %d, rt_index = %d.\n", i, memlabel_to_string_c(memlabel, as1, sizeof as1), regtype, rt_index);
    memlabel_copy(&insn->memtouch.mls[i], memlabel);
    /*insn->memtouch.regtype[memtouch_index] = regtype;
    insn->memtouch.rt_index[memtouch_index] = rt_index;
    memvt_t *mvt = &insn->memtouch.ls[memtouch_index];
    if (mvt->disp.tag == regtype) {
      return;
    }
    ASSERT(mvt->disp.tag == tag_const);
    long orig_val = mvt->disp.val;
    mvt->disp.val = rt_index;
    mvt->disp.tag = regtype;
    mvt->disp.mod.imm.modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0;
    mvt->disp.mod.imm.sconstants[2] = 1;
    mvt->disp.mod.imm.sconstants[1] = orig_val;
    mvt->disp.mod.imm.sconstants[0] = 32;*/
  }
}
#endif

static size_t
i386_operand_get_symbols(operand_t const *op, long const *start_symbol_ids, long *symbol_ids, size_t max_symbol_ids)
{
  if (op->type == op_mem) {
    return valtag_get_symbol_id(start_symbol_ids, symbol_ids, max_symbol_ids, &op->valtag.mem.disp);
  } else if (op->type == op_imm) {
    return valtag_get_symbol_id(start_symbol_ids, symbol_ids, max_symbol_ids, &op->valtag.imm);
  }
  return 0;
}

size_t
i386_insn_get_symbols(i386_insn_t const *insn, long const *start_symbol_ids, long *symbol_ids, size_t max_symbol_ids)
{
  long *ptr = symbol_ids, *end = symbol_ids + max_symbol_ids;
  long i;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    ptr += i386_operand_get_symbols(&insn->op[i], start_symbol_ids, ptr, end - ptr);
  }
  return ptr - symbol_ids;
}

static void
i386_operand_rename_code_address(operand_t *op, struct chash const *tcodes)
{
  uint32_t new_val;
  ASSERT(op->type == op_imm);
  ASSERT(op->valtag.imm.tag == tag_const);
  if (get_translated_addr_using_tcodes(&new_val, tcodes, op->valtag.imm.val)) {
    op->valtag.imm.val = new_val;
  }
}

void
i386_insn_rename_immediate_operands_containing_code_addresses(i386_insn_t *insn,
    struct chash const *tcodes)
{
  long i;
  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (insn->op[i].type == op_imm) {
      i386_operand_rename_code_address(&insn->op[i], tcodes);
    }
  }
}

void
i386_insn_fetch_preprocess(i386_insn_t *insn, input_exec_t const *in, src_ulong pc,
    unsigned long size)
{
  char const *function_name;
  src_ulong val, dst_pc;

  if (   i386_insn_is_function_call(insn)
      && insn->op[insn->num_implicit_ops].type == op_pcrel
      && insn->op[insn->num_implicit_ops].valtag.pcrel.tag == tag_const) {
    long val = insn->op[insn->num_implicit_ops].valtag.pcrel.val;
    dst_pc = pc + size + val;
    function_name = pc2function_name(in, dst_pc);
    if (function_name && strstr(function_name, "__x86.get_pc_thunk.bx")) {
      long iret = i386_insn_put_movl_imm_to_reg(pc + size, R_EBX, insn, 1);
      ASSERT(iret == 1);
    }
    else if (function_name && strstr(function_name, "__x86.get_pc_thunk.dx")) {
      long iret = i386_insn_put_movl_imm_to_reg(pc + size, R_EDX, insn, 1);
      ASSERT(iret == 1);
    }
  }
}

void
i386_insn_gen_sym_exec_reader_file(i386_insn_t const *insn, FILE *ofp)
{
  size_t ret;
  //printf("sizeof i386_insn = %zd\n", sizeof *insn);
  ret = fwrite(insn, 1, sizeof(i386_insn_t), ofp);
  if (ret != sizeof(i386_insn_t)) {
    printf("ret = %zd, sizeof(i386_insn_t) = %zd.\n", ret, sizeof(i386_insn_t));
  }
  ASSERT(ret == sizeof(i386_insn_t));
/*
  uint8_t const *ptr, *start, *end;

  start = (uint8_t *)insn;
  end = (uint8_t *)insn + sizeof(i386_insn_t);
  ptr = start;
  fprintf(ofp, ".insn = {");
  for (ptr = start; ptr < end; ptr++) {
    fprintf(ofp, "%hhx,", *ptr);
  }
  fprintf(ofp, "};\n");
*/
}

bool
i386_imm_operand_needs_canonicalization(opc_t opc, int explicit_op_index)
{
  char const *opc_name;

  if (explicit_op_index < 0) {
    return true;
  }

  opc_name = opctable_name(opc);
  if (   strstart(opc_name, "pcmpistri", NULL)
      && explicit_op_index == 2) {
    return false;
  }
  if (   strstart(opc_name, "pcmpistrm", NULL)
      && explicit_op_index == 2) {
    return false;
  }
  if (   strstart(opc_name, "pcmpestri", NULL)
      && explicit_op_index == 2) {
    return false;
  }
  if (   strstart(opc_name, "pcmpestrm", NULL)
      && explicit_op_index == 2) {
    return false;
  }
  if (   (   strstart(opc_name, "pshuf", NULL)
          || strstart(opc_name, "pins", NULL)
          || strstart(opc_name, "pext", NULL))
      && explicit_op_index == 2) {
    return false;
  }
  if (   (   !strcmp(opc_name, "cmpss")
          || !strcmp(opc_name, "cmpsd"))
      && explicit_op_index == 2) {
    return false;
  }
  if (   (   !strcmp(opc_name, "shufps")
          || !strcmp(opc_name, "shufpd"))
      && explicit_op_index == 2) {
    return false;
  }
  return true;
}

regset_t const *
i386_regset_live_at_jumptable()
{
  return regset_all();
}

struct i386_strict_canonicalization_state_t *
i386_strict_canonicalization_state_init()
{
  return NULL;
}

void
i386_strict_canonicalization_state_free(struct i386_strict_canonicalization_state_t *)
{
}

long
i386_insn_get_max_num_imm_operands(i386_insn_t const *insn)
{
  return I386_MAX_NUM_OPERANDS;
}

vector<valtag_t>
i386_insn_get_pcrel_values(struct i386_insn_t const *insn)
{
  uint64_t pcrel_val;
  src_tag_t pcrel_tag;
  bool ret = i386_insn_get_pcrel_value(insn, &pcrel_val, &pcrel_tag);
  if (!ret) {
    return vector<valtag_t>();
  }
  valtag_t vt;
  vt.val = pcrel_val;
  vt.tag = pcrel_tag;
  vector<valtag_t> vvt;
  vvt.push_back(vt);
  return vvt;
}

void
i386_insn_set_pcrel_values(struct i386_insn_t *insn, vector<valtag_t> const &vs)
{
  ASSERT(vs.size() <= 1);
  if (vs.size() == 0) {
    return;
  }
  ASSERT(vs.size() == 1);
  uint64_t pcrel_val = vs.at(0).val;
  src_tag_t pcrel_tag = vs.at(0).tag;
  i386_insn_set_pcrel_value(insn, pcrel_val, pcrel_tag);
}

valtag_t *
i386_insn_get_pcrel_operands(i386_insn_t const *insn)
{
  return i386_insn_get_pcrel_operand(insn);
}

void
i386_pc_get_function_name_and_insn_num(input_exec_t const *in, src_ulong pc, string &function_name, long &insn_num_in_function)
{
  //this function is only meaningful for etfg
  function_name = "ppc";
  insn_num_in_function = -1;
}

static void
i386_operand_inv_rename_imms_using_imm_vt_map(operand_t *op, imm_vt_map_t const *imm_vt_map)
{
  valtag_t *imm_vt = NULL;
  if (op->type == op_mem) {
    imm_vt = &op->valtag.mem.disp;
  } else if (op->type == op_imm) {
    imm_vt = &op->valtag.imm;
  }
  if (!imm_vt) {
    return;
  }
  for (size_t i = 0; i < imm_vt_map->num_imms_used; i++) {
    if (   imm_vt_map->table[i].tag == imm_vt->tag
        && imm_vt->tag == tag_const) {
      if (imm_vt->val == imm_vt_map->table[i].val) {
        imm_vt->tag = tag_var;
        imm_vt->val = i;
        imm_vt->mod.imm.modifier = IMM_UNMODIFIED;
      } else if (abs(imm_vt->val - imm_vt_map->table[i].val) <= 1) {
        imm_vt->mod.imm.modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0;
        imm_vt->mod.imm.sconstants[2] = 1;
        imm_vt->mod.imm.sconstants[1] = imm_vt->val - imm_vt_map->table[i].val;
        imm_vt->mod.imm.sconstants[0] = DST_LONG_BITS;
        imm_vt->tag = tag_var;
        imm_vt->val = i;
      }
    }
  }
}

static void
i386_insn_inv_rename_imms_using_imm_vt_map(i386_insn_t *insn, imm_vt_map_t const *imm_vt_map)
{
  for (size_t i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    i386_operand_inv_rename_imms_using_imm_vt_map(&insn->op[i], imm_vt_map);
  }
}

void
i386_iseq_inv_rename_imms_using_imm_vt_map(i386_insn_t *iseq, size_t iseq_len, imm_vt_map_t const *imm_vt_map)
{
  for (size_t i = 0; i < iseq_len; i++) {
    i386_insn_inv_rename_imms_using_imm_vt_map(&iseq[i], imm_vt_map);
  }
}

static void
i386_insn_rename_nextpc_vars_to_pcrel_relocs(struct translation_instance_t *ti, i386_insn_t *insn, map<nextpc_id_t, string> const &nextpc_map)
{
  valtag_t *pcrel;
  if (   (pcrel = i386_insn_get_pcrel_operand(insn))
      && pcrel->tag == tag_src_insn_pc
      && PC_IS_NEXTPC_CONST(pcrel->val)) {
    nextpc_id_t nextpc_id = PC_NEXTPC_CONST_GET_NEXTPC_ID(pcrel->val);
    if (!nextpc_map.count(nextpc_id)) {
      for (auto n : nextpc_map) {
        cout << n.first << ": " << n.second << "\n";
      }
      cout << __func__ << " " << __LINE__ << ": nextpc_id = " << nextpc_id << ": insn = " << i386_iseq_to_string(insn, 1, as1, sizeof as1) << endl;
    }
    ASSERT(nextpc_map.count(nextpc_id));
    string const &nextpc_name = nextpc_map.at(nextpc_id);
    pcrel->val = add_or_find_reloc_symbol_name(&ti->reloc_strings, &ti->reloc_strings_size, nextpc_name.c_str());
    pcrel->tag = tag_reloc_string;
  }
}

void
i386_iseq_rename_nextpc_vars_to_pcrel_relocs(struct translation_instance_t *ti, i386_insn_t *iseq, size_t iseq_len, map<nextpc_id_t, string> const &nextpc_map)
{
  //cout << __func__ << " " << __LINE__ << ": iseq = " << i386_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
  for (size_t i = 0; i < iseq_len; i++) {
    i386_insn_rename_nextpc_vars_to_pcrel_relocs(ti, &iseq[i], nextpc_map);
  }
}

long
i386_insn_put_xchg_reg_with_mem(int reg1, dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size)
{
  i386_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  /* mov <reg2>, <reg1> */
  i386_insn_init(&insn_ptr[0]);
  insn_ptr[0].opc = opc_nametable_find("xchgl");

  insn_ptr[0].op[0].type = op_mem;
  insn_ptr[0].op[0].valtag.mem.segtype = segtype_sel;
  insn_ptr[0].op[0].valtag.mem.seg.sel.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.seg.sel.val = R_DS;
  insn_ptr[0].op[0].valtag.mem.base.val = membase;
  insn_ptr[0].op[0].valtag.mem.base.tag = tag_const;
  insn_ptr[0].op[0].valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[0].valtag.mem.index.val = -1;
  insn_ptr[0].op[0].valtag.mem.disp.val = addr;
  insn_ptr[0].op[0].valtag.mem.riprel.val = -1;
  insn_ptr[0].op[0].size = DST_LONG_SIZE;

  insn_ptr[0].op[1].type = op_reg;
  insn_ptr[0].op[1].valtag.reg.tag = tag_const;
  insn_ptr[0].op[1].valtag.reg.val = reg1;
  insn_ptr[0].op[1].valtag.reg.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
  insn_ptr[0].op[1].size = DST_LONG_SIZE;

  i386_insn_add_implicit_operands(&insn_ptr[0]);
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

static size_t
get_local_offset_for_local_id(graph_locals_map_t const &locals_map, size_t stack_alloc_size, allocsite_t const& local_id)
{
  ASSERT(locals_map.count(local_id));
  ssize_t cur_off = 0;
  for (auto iter = locals_map.get_map().rbegin(); iter->first != local_id; iter--) {
    //cout << __func__ << " " << __LINE__ << ": local_id = " << local_id << ", locals_map.size() = " << locals_map.size() << ", i = " << i << endl;
    cur_off += iter->second.get_size();
  }
  cur_off += locals_map.at(local_id).get_size();
  ASSERT(cur_off <= stack_alloc_size);
  return stack_alloc_size - cur_off;
}

static void
i386_operand_rename_locals_using_locals_map(operand_t *op, graph_locals_map_t const &locals_map, size_t stack_alloc_size)
{
  if (   op->type == op_mem
      && op->valtag.mem.disp.tag == tag_imm_local) {
    size_t local_offset = get_local_offset_for_local_id(locals_map, stack_alloc_size, allocsite_t::allocsite_from_longlong(op->valtag.mem.disp.val));
    if (op->valtag.mem.index.val == -1) {
      valtag_copy(&op->valtag.mem.index, &op->valtag.mem.base);
      op->valtag.mem.disp.tag = tag_const;
      op->valtag.mem.disp.val = local_offset;
      op->valtag.mem.base.tag = tag_const;
      op->valtag.mem.base.val = R_ESP;
      op->valtag.mem.base.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
    } else if (op->valtag.mem.base.val == -1) {
      op->valtag.mem.disp.tag = tag_const;
      op->valtag.mem.disp.val = local_offset;
      op->valtag.mem.index.tag = tag_const;
      op->valtag.mem.index.val = R_ESP;
      op->valtag.mem.index.mod.reg.mask = MAKE_MASK(DST_LONG_BITS);
    } else {
      cout << __func__ << " " << __LINE__ << ": Error: tag_imm_local disp specified with valid base and index registers in translation. Leaving as-is\n";
      NOT_REACHED();
    }
  }
}

static void
i386_insn_rename_locals_using_locals_map(i386_insn_t *insn, graph_locals_map_t const &locals_map, size_t stack_alloc_size)
{
  for (size_t i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    i386_operand_rename_locals_using_locals_map(&insn->op[i], locals_map, stack_alloc_size);
  }
}

void
i386_iseq_rename_locals_using_locals_map(i386_insn_t *i386_iseq, size_t i386_iseq_len, graph_locals_map_t const &locals_map, size_t stack_alloc_size)
{
  for (size_t i = 0; i < i386_iseq_len; i++) {
    i386_insn_rename_locals_using_locals_map(&i386_iseq[i], locals_map, stack_alloc_size);
  }
}

static bool
i386_operand_replace_tmap_loc_with_const(operand_t *op, transmap_loc_t const &loc/*uint8_t tmap_loc, dst_ulong tmap_regnum*/, long constant, bool is_implicit_op)
{
  ASSERT(loc.get_loc() == TMAP_LOC_SYMBOL || loc.get_loc() == TMAP_LOC_EXREG(I386_EXREG_GROUP_GPRS));
  uint8_t tmap_loc = loc.get_loc();
  switch (op->type) {
  case op_reg:
    if (   tmap_loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_GPRS)
        && op->valtag.reg.tag == tag_var
        && loc.get_reg_id() == op->valtag.reg.val) {
      if (is_implicit_op) {
        return false;
      }
      op->type = op_imm;
      op->valtag.imm.tag = tag_const;
      op->valtag.imm.val = constant;
    }
    break;
  case op_mem:
    if (tmap_loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_GPRS)) {
      if (   op->valtag.mem.base.tag == tag_var
          && loc.get_reg_id() == op->valtag.mem.base.val) {
        if (is_implicit_op) {
          return false;
        }
        if (op->valtag.mem.disp.tag == tag_const) {
          op->valtag.mem.disp.val += constant;
          op->valtag.mem.base.val = -1;
        //} else if (op->valtag.mem.disp.tag == tag_var) { //XXX: NOT_IMPLEMENTED
        } else {
          return false;
        }
      }
      if (   op->valtag.mem.index.tag == tag_var
          && loc.get_reg_id() == op->valtag.mem.index.val) {
        if (is_implicit_op) {
          return false;
        }
        if (op->valtag.mem.disp.tag == tag_const) {
          op->valtag.mem.disp.val += op->valtag.mem.scale * constant;
          op->valtag.mem.index.val = -1;
        //} else if (op->valtag.mem.disp.tag == tag_var) { //XXX: NOT_IMPLEMENTED
        } else {
          return false;
        }
      }
    } else if (tmap_loc == TMAP_LOC_SYMBOL) {
      if (/*   op->valtag.mem.disp.tag == tag_const
          && op->valtag.mem.disp.val >= PEEPTAB_MEM_CONSTANT(0)
          && op->valtag.mem.disp.val < PEEPTAB_MEM_CONSTANT(PEEPTAB_MAX_MEM_CONSTANTS)
          && op->valtag.mem.disp.val == loc.get_regnum()*/
             op->valtag.mem.disp.tag == tag_imm_symbol
          && op->valtag.mem.disp.val == loc.get_regnum()) {
        if (is_implicit_op) {
          return false;
        }
        op->type = op_imm;
        op->valtag.imm.tag = tag_const;
        op->valtag.imm.val = constant;
      }
    }
    break;
  default:
    break;
  }
  return true;
}

static bool
i386_insn_replace_tmap_loc_with_const(i386_insn_t *insn, transmap_loc_t const &loc/*uint8_t tmap_loc, dst_ulong tmap_regnum*/, long constant)
{
  if (i386_insn_is_nop(insn)) {
    return true;
  }
  for (size_t i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (!i386_operand_replace_tmap_loc_with_const(&insn->op[i], loc/*tmap_loc, tmap_regnum*/, constant, i < insn->num_implicit_ops)) {
      return false;
    }
  }
  return true;
}

bool
i386_iseq_replace_tmap_loc_with_const(i386_insn_t *iseq, size_t iseq_len, transmap_loc_t const &loc/*, uint8_t tmap_loc, dst_ulong tmap_regnum*/, long constant)
{
  if (   loc.get_loc() != TMAP_LOC_SYMBOL
      && loc.get_loc() != TMAP_LOC_EXREG(I386_EXREG_GROUP_GPRS)) {
    return true;
  }
  for (size_t i = 0; i < iseq_len; i++) {
    if (!i386_insn_replace_tmap_loc_with_const(&iseq[i], loc, constant)) {
      return false;
    }
  }
  return true;
}

size_t
i386_iseq_rename_randomly_and_assemble(i386_insn_t const *iseq, size_t iseq_len, size_t num_nextpcs, uint8_t *buf, size_t buf_size, i386_as_mode_t mode)
{
  i386_iseq_to_string(iseq, iseq_len, as1, sizeof as1);
  regcons_t regcons;
  bool ret = i386_infer_regcons_from_assembly(as1, &regcons);
  if (!ret) {
    CPP_DBG_EXEC(REGCONST, printf("Could not infer regcons for assembly:\n%s\n", as1));
    return 0;
  }
  regmap_t regmap;
  regmap_init(&regmap);
  ret = regmap_assign_using_regcons(&regmap, &regcons, ARCH_DST, NULL, fixed_reg_mapping_t());
  if (!ret) {
    CPP_DBG_EXEC(REGCONST, printf("Could not assign regmap for regcons:\n%s\n", regcons_to_string(&regcons, as1, sizeof as1)));
    return 0;
  }
  nextpc_t nextpcs[num_nextpcs];
  for (size_t i = 0; i < num_nextpcs; i++) {
    nextpcs[i] = i;
  }
  imm_vt_map_t *imm_vt_map = new imm_vt_map_t;
  for (size_t i = 0; i < NUM_CANON_CONSTANTS; i++) {
    imm_vt_map->table[i] = { .val = i, .tag = tag_const };
  }
  imm_vt_map->num_imms_used = NUM_CANON_CONSTANTS;
  i386_insn_t *iseq_copy = new i386_insn_t[iseq_len];
  i386_iseq_copy(iseq_copy, iseq, iseq_len);
  i386_iseq_rename(iseq_copy, iseq_len, &regmap, imm_vt_map, NULL, NULL, NULL, nextpcs, num_nextpcs);
  CPP_DBG_EXEC(REGCONST, cout << __func__ << " " << __LINE__ << ": assembling iseq_copy:\n" << i386_iseq_to_string(iseq_copy, iseq_len, as1, sizeof as1));
  size_t retsz = i386_iseq_assemble(NULL, iseq_copy, iseq_len, buf, buf_size, I386_AS_MODE_32);
  CPP_DBG_EXEC(REGCONST, cout << __func__ << " " << __LINE__ << ": assembler returned retsz = " << retsz << endl);
  delete [] iseq_copy;
  delete imm_vt_map;
  return retsz;
}

size_t
i386_insn_compute_transmap_entry_loc_value(int dst_reg, transmap_entry_t const &tmap_entry_in, int esp_reg, long esp_offset, int locnum, fixed_reg_mapping_t const &dst_fixed_reg_mapping, i386_insn_t *buf, size_t buf_size)
{
  i386_insn_t *ptr = buf;
  i386_insn_t *end = buf + buf_size;
  if (tmap_entry_in.get_locs().size() == 0) {
    ptr += i386_insn_put_movl_imm_to_reg(0, dst_reg, ptr, end - ptr);
    return ptr - buf;
  }
  transmap_entry_t tmap_entry = tmap_entry_in;
  if (locnum != -1) {
    ASSERT(locnum >= 0 && locnum < tmap_entry_in.get_locs().size());
    transmap_loc_t const &loc = tmap_entry_in.get_loc(locnum);
    if (loc.get_loc() == TMAP_LOC_SYMBOL) {
      tmap_entry.set_to_symbol_loc(loc.get_loc(), loc.get_regnum());
    } else {
      tmap_entry.set_to_exreg_loc(loc.get_loc(), loc.get_reg_id().reg_identifier_get_id(), loc.get_modifier());
    }
  }
  if (tmap_entry.contains_mapping_to_loc(TMAP_LOC_EXREG(I386_EXREG_GROUP_GPRS))) {
    reg_identifier_t const &src_reg_id = tmap_entry.get_reg_id_for_loc(TMAP_LOC_EXREG(I386_EXREG_GROUP_GPRS));
    ASSERT(!src_reg_id.reg_identifier_is_unassigned());
    dst_ulong src_reg = src_reg_id.reg_identifier_get_id();
    //ptr += i386_insn_put_movl_reg_to_reg(dst_reg, src_reg, ptr, end - ptr);
    ASSERT(src_reg >= 0 && src_reg < I386_NUM_GPRS);
    ASSERT(src_reg != R_ESP);
    dst_ulong reg_off = (src_reg < R_ESP) ? esp_offset - (src_reg + 1) * 4 : esp_offset - src_reg * 4;
    ptr += i386_insn_put_movl_mem_to_reg(dst_reg, reg_off, esp_reg, ptr, end - ptr);
    return ptr - buf;
  }
  if (tmap_entry.contains_mapping_to_loc(TMAP_LOC_SYMBOL)) {
    dst_ulong reg_off = tmap_entry.get_regnum_for_loc(TMAP_LOC_SYMBOL);
    reg_off += esp_offset;
    ptr += i386_insn_put_movl_mem_to_reg(dst_reg, reg_off, esp_reg, ptr, end - ptr);
    return ptr - buf;
  }
  for (exreg_group_id_t g = I386_EXREG_GROUP_EFLAGS_OTHER; g <= I386_EXREG_GROUP_EFLAGS_SGE; g++) {
    if (tmap_entry.contains_mapping_to_loc(TMAP_LOC_EXREG(g))) {
      dst_ulong flags_off = esp_offset - 8 * 4;
      ptr += i386_insn_put_movl_mem_to_reg(dst_reg, flags_off, esp_reg, ptr, end - ptr);
      ptr += i386_insn_put_push_reg(dst_reg, dst_fixed_reg_mapping, ptr, end - ptr);
      ptr += i386_insn_put_popf(dst_fixed_reg_mapping, ptr, end - ptr);
      reg_identifier_t const &reg_id = tmap_entry.get_reg_id_for_loc(TMAP_LOC_EXREG(g));
      ptr += i386_insn_put_move_exreg_to_reg(dst_reg, g, reg_id.reg_identifier_get_id(), dst_fixed_reg_mapping, ptr, end - ptr, I386_AS_MODE_64);
      return ptr - buf;
    }
  }

  cout << "tmap_entry = " << tmap_entry.transmap_entry_to_string() << endl;
  NOT_IMPLEMENTED();
}

/*eq::tfg *
i386_iseq_get_tfg(i386_insn_t const *i386_iseq, long i386_iseq_len)
{
  NOT_IMPLEMENTED();
}*/

shared_ptr<tfg_llvm_t>
src_iseq_get_preprocessed_tfg(i386_insn_t const *i386_iseq, long i386_iseq_len, regset_t const *live_out, size_t num_live_out, map<pc, map<string_ref, expr_ref>> const &return_reg_map, context *ctx, sym_exec &se)
{
  NOT_IMPLEMENTED();
}

static size_t
i386_insn_put_movzbl_reg_to_reg(exreg_id_t dst, exreg_id_t src, i386_insn_t *buf, size_t buf_size)
{
  return i386_insn_put_opcode_reg_to_reg_different_sizes("movzbl", dst, src,
      DWORD_LEN/BYTE_LEN, 1, buf, buf_size);
}

static size_t
i386_insn_put_movzwl_reg_to_reg(exreg_id_t dst, exreg_id_t src, i386_insn_t *buf, size_t buf_size)
{
  return i386_insn_put_opcode_reg_to_reg_different_sizes("movzwl", dst, src,
      DWORD_LEN/BYTE_LEN, WORD_LEN/BYTE_LEN, buf, buf_size);
}

size_t
i386_insn_put_zextend_exreg(exreg_group_id_t group, exreg_id_t regnum, int cur_size, i386_insn_t *buf, size_t buf_size)
{
  if (cur_size >= DWORD_LEN) {
    return 0;
  }
  i386_insn_t *ptr = buf;
  i386_insn_t *end = buf + buf_size;
  if (group == I386_EXREG_GROUP_GPRS) {
    ASSERT(cur_size == BYTE_LEN || cur_size == WORD_LEN);
    if (cur_size == BYTE_LEN) {
      if (regnum < 4) {
        ptr += i386_insn_put_movzbl_reg_to_reg(regnum, regnum, ptr, end - ptr);
      } else {
        ptr += i386_insn_put_shl_reg_by_imm(regnum, DWORD_LEN - BYTE_LEN, ptr, end - ptr);
        ptr += i386_insn_put_shr_reg_by_imm(regnum, DWORD_LEN - BYTE_LEN, ptr, end - ptr);
      }
    } else if (cur_size == WORD_LEN) {
      ptr += i386_insn_put_movzwl_reg_to_reg(regnum, regnum, ptr, end - ptr);
    } else NOT_REACHED();
  } else {
    NOT_IMPLEMENTED();
  }
  return ptr - buf;
}

size_t
i386_insn_put_function_return(i386_insn_t *insn)
{
  NOT_IMPLEMENTED();
}

size_t
i386_insn_put_invalid(i386_insn_t *insn)
{
  NOT_IMPLEMENTED();
}

size_t
i386_insn_put_ret_insn(i386_insn_t *insns, size_t insns_size, int retval)
{
  NOT_IMPLEMENTED();
}

size_t
i386_emit_translation_marker_insns(i386_insn_t *buf, size_t buf_size)
{
  return i386_insn_put_nop(buf, buf_size);
}

bool
i386_iseq_has_marker_prefix(i386_insn_t const *insns, size_t n_insns)
{
  return i386_insn_is_nop(&insns[0]);
}

bool
x64_insn_get_usedef_regs(i386_insn_t const *insn, regset_t *use, regset_t *def)
{
  char const *op0_read_ops[] = {"pushq", "pushfq"};
  size_t n_read_ops = NELEM(op0_read_ops);
  char const *op0_write_ops[] = {"movq", "popq", "popfq", "andl"};
  size_t n_write_ops = NELEM(op0_write_ops);
  char const *opc = opctable_name(insn->opc);
  if (streq_arr(opc, op0_read_ops, n_read_ops) || streq_arr(opc, op0_write_ops, n_write_ops)) {
    for (size_t i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
      if (insn->op[i].type == op_reg) {
        if (i == 0 && streq_arr(opc, op0_write_ops, n_write_ops)) {
          regset_mark_ex_mask(def, I386_EXREG_GROUP_GPRS, insn->op[i].valtag.reg.val,
              MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_GPRS)));
        }
        if (i != 0 || streq_arr(opc, op0_read_ops, n_read_ops)) {
          regset_mark_ex_mask(use, I386_EXREG_GROUP_GPRS, insn->op[i].valtag.reg.val,
              MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_GPRS)));
        }
      } else if (insn->op[i].type == op_mem) {
        if (insn->op[i].valtag.mem.base.val != -1) {
          regset_mark_ex_mask(use, I386_EXREG_GROUP_GPRS, insn->op[i].valtag.mem.base.val,
              MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_GPRS)));
        }
        if (insn->op[i].valtag.mem.index.val != -1) {
          regset_mark_ex_mask(use, I386_EXREG_GROUP_GPRS,
              insn->op[i].valtag.mem.index.val,
              MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_GPRS)));
        }
      } else if (insn->op[i].type == op_mmx || insn->op[i].type == op_xmm || insn->op[i].type == op_ymm) {
        if (!strcmp(opctable_name(insn->opc), "movq")) {
          return false; //this has non-reg and non-mem operand, could be mmx/xmm, return false. Let it be handled by i386_insn_get_usedef_regs()
        }
      }
    }
    return true;
  }
  return false;
}

static void
i386_operand_rename_regs_from_tag_to_tag(operand_t *op, src_tag_t from_tag, src_tag_t to_tag)
{
  switch (op->type) {
    case op_reg:
      ASSERT(op->valtag.reg.tag == from_tag);
      op->valtag.reg.tag = to_tag;
      break;
    case op_mem:
      ASSERT(op->valtag.mem.seg.sel.tag == from_tag);
      op->valtag.mem.seg.sel.tag = to_tag;
      if (op->valtag.mem.base.val != -1) {
        ASSERT(op->valtag.mem.base.tag == from_tag);
        op->valtag.mem.base.tag = to_tag;
      }
      if (op->valtag.mem.index.val != -1) {
        ASSERT(op->valtag.mem.index.tag == from_tag);
        op->valtag.mem.index.tag = to_tag;
      }
      break;
    case op_seg:
      ASSERT(op->valtag.seg.tag == from_tag);
      op->valtag.seg.tag = to_tag;
      break;
    case op_mmx:
      ASSERT(op->valtag.mmx.tag == from_tag);
      op->valtag.mmx.tag = to_tag;
      break;
    case op_xmm:
      ASSERT(op->valtag.xmm.tag == from_tag);
      op->valtag.xmm.tag = to_tag;
      break;
    case op_ymm:
      ASSERT(op->valtag.ymm.tag == from_tag);
      op->valtag.ymm.tag = to_tag;
      break;
#define rename_flag_tag_to_tag(flagname) \
    case op_flags_##flagname: \
      ASSERT(op->valtag.flags_##flagname.tag == from_tag); \
      op->valtag.flags_##flagname.tag = to_tag; \
      break;

    rename_flag_tag_to_tag(other);
    rename_flag_tag_to_tag(eq);
    rename_flag_tag_to_tag(ne);
    rename_flag_tag_to_tag(ult);
    rename_flag_tag_to_tag(slt);
    rename_flag_tag_to_tag(ugt);
    rename_flag_tag_to_tag(sgt);
    rename_flag_tag_to_tag(ule);
    rename_flag_tag_to_tag(sle);
    rename_flag_tag_to_tag(uge);
    rename_flag_tag_to_tag(sge);
    //case op_flags_unsigned:
    //  ASSERT(op->valtag.flags_unsigned.tag == from_tag);
    //  op->valtag.flags_unsigned.tag = to_tag;
    //  break;
    //case op_flags_signed:
    //  ASSERT(op->valtag.flags_signed.tag == from_tag);
    //  op->valtag.flags_signed.tag = to_tag;
    //  break;
    case op_st_top:
      ASSERT(op->valtag.st_top.tag == from_tag);
      op->valtag.st_top.tag = to_tag;
      break;
    case op_fp_data_reg:
      ASSERT(op->valtag.fp_data_reg.tag == from_tag);
      op->valtag.fp_data_reg.tag = to_tag;
      break;
    case op_fp_control_reg_rm:
      ASSERT(op->valtag.fp_control_reg_rm.tag == from_tag);
      op->valtag.fp_control_reg_rm.tag = to_tag;
      break;
    case op_fp_control_reg_other:
      ASSERT(op->valtag.fp_control_reg_other.tag == from_tag);
      op->valtag.fp_control_reg_other.tag = to_tag;
      break;
    case op_fp_tag_reg:
      ASSERT(op->valtag.fp_tag_reg.tag == from_tag);
      op->valtag.fp_tag_reg.tag = to_tag;
      break;
    case op_fp_last_instr_pointer:
      ASSERT(op->valtag.fp_last_instr_pointer.tag == from_tag);
      op->valtag.fp_last_instr_pointer.tag = to_tag;
      break;
    case op_fp_last_operand_pointer:
      ASSERT(op->valtag.fp_last_operand_pointer.tag == from_tag);
      op->valtag.fp_last_operand_pointer.tag = to_tag;
      break;
    case op_fp_opcode:
      ASSERT(op->valtag.fp_opcode.tag == from_tag);
      op->valtag.fp_opcode.tag = to_tag;
      break;
    case op_mxcsr_rm:
      ASSERT(op->valtag.mxcsr_rm.tag == from_tag);
      op->valtag.mxcsr_rm.tag = to_tag;
      break;
    case op_fp_status_reg_c0:
      ASSERT(op->valtag.fp_status_reg_c0.tag == from_tag);
      op->valtag.fp_status_reg_c0.tag = to_tag;
      break;
    case op_fp_status_reg_c1:
      ASSERT(op->valtag.fp_status_reg_c1.tag == from_tag);
      op->valtag.fp_status_reg_c1.tag = to_tag;
      break;
    case op_fp_status_reg_c2:
      ASSERT(op->valtag.fp_status_reg_c2.tag == from_tag);
      op->valtag.fp_status_reg_c2.tag = to_tag;
      break;
    case op_fp_status_reg_c3:
      ASSERT(op->valtag.fp_status_reg_c3.tag == from_tag);
      op->valtag.fp_status_reg_c3.tag = to_tag;
      break;
    case op_fp_status_reg_other:
      ASSERT(op->valtag.fp_status_reg_other.tag == from_tag);
      op->valtag.fp_status_reg_other.tag = to_tag;
      break;
    default:
      break;
  }
}

static void
i386_operand_rename_cregs_to_vregs(operand_t *op)
{
  i386_operand_rename_regs_from_tag_to_tag(op, tag_const, tag_var);
}

static void
i386_operand_rename_vregs_to_cregs(operand_t *op)
{
  i386_operand_rename_regs_from_tag_to_tag(op, tag_var, tag_const);
}


void
i386_insn_rename_cregs_to_vregs(i386_insn_t *insn)
{
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    i386_operand_rename_cregs_to_vregs(&insn->op[i]);
  }
}

void
i386_insn_rename_vregs_to_cregs(i386_insn_t *insn)
{
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    i386_operand_rename_vregs_to_cregs(&insn->op[i]);
  }
}

long
i386_insn_t::insn_hash_func_after_canon(long hash_size) const
{
  regmap_t hash_regmap;
  imm_vt_map_t hash_imm_map;

  regmap_init(&hash_regmap);
  imm_vt_map_init(&hash_imm_map);

  i386_insn_t *hash_insn = new i386_insn_t;
  i386_insn_copy(hash_insn, this);
  i386_insn_hash_canonicalize(hash_insn, &hash_regmap, &hash_imm_map);
  //cout << __func__ << " " << __LINE__ << ": hash_insn = " << i386_insn_to_string(hash_insn, as1, sizeof as1) << endl;
  long ret = i386_insn_hash_func(hash_insn);
  //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
  ret = ret % hash_size;
  delete hash_insn;
  return ret;
}

bool
i386_insn_t::insn_matches_template(i386_insn_t const &templ, src_iseq_match_renamings_t &src_iseq_match_renamings/*regmap_t &regmap, imm_vt_map_t &imm_vt_map, nextpc_map_t &nextpc_map*/, bool var) const
{
  src_iseq_match_renamings.clear();
  //regmap_init(&regmap);
  //imm_vt_map_init(&imm_vt_map);
  //nextpc_map_init(&nextpc_map);
  return i386_insn_matches_template(&templ, this, src_iseq_match_renamings/*&regmap, &imm_vt_map, &nextpc_map*/, var);
}

static void
i386_add_transmap_entries_for_loc(list<transmap_entry_t> &ls, int tmap_loc)
{
  transmap_entry_t tmap_entry;
  //tmap_entry.set_to_exreg_loc(tmap_loc, TRANSMAP_REG_ID_UNASSIGNED, TMAP_LOC_MODIFIER_SRC_SIZED); //only enumerate dst-sized locs
  //ls.push_back(tmap_entry);
  tmap_entry.set_to_exreg_loc(tmap_loc, TRANSMAP_REG_ID_UNASSIGNED, TMAP_LOC_MODIFIER_DST_SIZED);
  ls.push_back(tmap_entry);
}

static list<transmap_entry_t>
i386_get_choices_for_bitmap(uint64_t bitmap)
{
  list<transmap_entry_t> ret;
  ASSERT(bitmap != 0);

  transmap_entry_t tmap_entry;
  tmap_entry.set_to_symbol_loc(TMAP_LOC_SYMBOL, -1);
  ret.push_back(tmap_entry);

  i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_GPRS));

  if (bitmap == 1) {
    //i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_UNSIGNED));
    //i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_SIGNED));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_EQ));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_NE));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_ULT));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_SLT));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_UGT));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_SGT));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_ULE));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_SLE));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_UGE));
    i386_add_transmap_entries_for_loc(ret, TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_SGE));
  }
  return ret;
}

static void
transmap_add_mapping(transmap_t &tmap, string const &k, transmap_entry_t const &e, regset_t const *live_regs)
{
  //size_t dot = k.find('.');
  //ASSERT(dot != string::npos);
  exreg_group_id_t g;
  string reg_id_str;
  char ch;
  istringstream iss(k);
  iss >> g;
  iss >> ch;
  ASSERT(ch == '.');
  iss >> reg_id_str;
  reg_identifier_t reg_id = reg_identifier_t(stoi(reg_id_str));
  //cout << __func__ << " " << __LINE__ << ": g = " << g << endl;
  //cout << __func__ << " " << __LINE__ << ": reg_id = " << reg_id.reg_identifier_to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": live_regs = " << regset_to_string(live_regs, as1, sizeof as1) << endl;
  if (regset_belongs_ex(live_regs, g, reg_id)) {
    //cout << __func__ << " " << __LINE__ << ": updating tmap entry\n";
    tmap.extable[g][reg_id] = e;
  }
}

//#define IN_TMAP_PREFIX "in_tmap"
//#define OUT_TMAP_PREFIX "out_tmap"

static pair<transmap_t, vector<transmap_t>>
get_transmap_from_choice_assignment(map<string, transmap_entry_t> const &choice_assignment, regset_t const *live_in, regset_t const *live_out, long num_live_out)
{
  vector<transmap_t> out_tmaps;
  transmap_t in_tmap;
  for (long i = 0; i < num_live_out; i++) {
    out_tmaps.push_back(transmap_t());
  }

  for (const auto &a : choice_assignment) {
    string const &k = a.first;
    transmap_entry_t const &v = a.second;
    //cout << __func__ << " " << __LINE__ << ": k = " << k << endl;
    //cout << __func__ << " " << __LINE__ << ": v = " << v.transmap_entry_to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": live_in = " << regset_to_string(live_in, as1, sizeof as1) << endl;
    transmap_add_mapping(in_tmap, k, v, live_in);
    for (long i = 0; i < num_live_out; i++) {
      transmap_add_mapping(out_tmaps.at(i), k, v, &live_out[i]);
    }
  }
  //cout << __func__ << " " << __LINE__ << ": in_tmap = " << transmap_to_string(&in_tmap, as1, sizeof as1) << endl;
  return make_pair(in_tmap, out_tmaps);
}

list<pair<transmap_t, vector<transmap_t>>>
i386_enumerate_in_out_transmaps_for_usedef(regset_t const &use, regset_t const &def, regset_t const *live_out, long num_live_out)
{
  //first identify all loc options for all live_in or live_out in in_tmap; then identify all loc options for all defed and live-out (but not live-in) regs in all out_tmaps
  regset_t live_in_or_live_out;
  regset_copy(&live_in_or_live_out, &use);
  for (long i = 0; i < num_live_out; i++) {
    regset_union(&live_in_or_live_out, &live_out[i]);
  }
  //regset_union(&touch, &def);
  map<string, list<transmap_entry_t>> choices;
  for (const auto &g : live_in_or_live_out.excregs) {
    for (const auto &r : g.second) {
      stringstream ss;
      ss << /*IN_TMAP_PREFIX "." << */g.first << "." << r.first.reg_identifier_to_string();
      list<transmap_entry_t> tmap_entries = i386_get_choices_for_bitmap(r.second);
      choices.insert(make_pair(ss.str(), tmap_entries));
    }
  }
  CPP_DBG_EXEC(ENUMERATE_IN_OUT_TMAPS,
      for (const auto &choice : choices) {
        cout << "choice " << choice.first << " : ";
        for (const auto &tmap_entry : choice.second) {
          cout << " " << tmap_entry.transmap_entry_to_string("");
        }
        cout << endl;
      }
  );
  //then cross-product all these loc options
  cartesian_t<string, transmap_entry_t> cartesian(choices);
  list<map<string, transmap_entry_t>> choice_assignments = cartesian.get_all_possibilities();
  list<pair<transmap_t, vector<transmap_t>>> ret;
  for (const auto &choice_assignment : choice_assignments) {
    pair<transmap_t, vector<transmap_t>> tmap_assignment = get_transmap_from_choice_assignment(choice_assignment, &use, live_out, num_live_out);
    add_transmap_renamings_of_in_out_transmaps_to_ls(ret, tmap_assignment.first, tmap_assignment.second);
  }
  return ret;
}

static void
i386_insn_convert_peeptab_mem_constants_to_symbols(i386_insn_t *insn)
{
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    valtag_t *vt = NULL;
    switch (insn->op[i].type) {
      case op_imm:
        vt = &insn->op[i].valtag.imm;
        break;
      case op_mem:
        vt = &insn->op[i].valtag.mem.disp;
        break;
      default:
        break;
    }
    if (!vt || vt->tag != tag_const) {
      continue;
    }
    if (   vt->val >= PEEPTAB_MEM_CONSTANT(0)
        && vt->val < PEEPTAB_MEM_CONSTANT(PEEPTAB_MAX_MEM_CONSTANTS)) {
      vt->val = PEEPTAB_MEM_CONSTANT_GET_ORIG(vt->val);
      vt->tag = tag_imm_symbol;
      vt->mod.imm.modifier = IMM_UNMODIFIED;
    }
  }
}

void
i386_iseq_convert_peeptab_mem_constants_to_symbols(i386_insn_t *iseq, long iseq_len)
{
  for (long i = 0; i < iseq_len; i++) {
    i386_insn_convert_peeptab_mem_constants_to_symbols(&iseq[i]);
  }
}

void
i386_insn_rename_vregs_using_regmap(i386_insn_t *insn, regmap_t const &regmap)
{
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    i386_operand_rename(&insn->op[i], &regmap, NULL, NULL, NULL, NULL, NULL, 0, tag_var, 0);
  }
}

static bool
opertype_is_flag(opertype_t opertype)
{
  if (   opertype == op_flags_other
      || opertype == op_flags_eq
      || opertype == op_flags_ne
      || opertype == op_flags_ult
      || opertype == op_flags_slt
      || opertype == op_flags_ugt
      || opertype == op_flags_sgt
      || opertype == op_flags_ule
      || opertype == op_flags_sle
      || opertype == op_flags_uge
      || opertype == op_flags_sge
     ) {
    return true;
  }
  return false;
}

size_t
i386_insn_count_num_implicit_flag_regs(i386_insn_t const *insn)
{
  size_t ret = 0;
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (opertype_is_flag(insn->op[i].type)) {
      ret++;
    } else {
      break;
    }
  }
  return ret;
}

uint64_t
i386_eflags_construct_from_cmp_bits(bool eq, bool ult, bool slt)
{
  bool flag_zf = eq;
  bool flag_cf = ult && !eq;
  bool flag_of = false;
  bool flag_ge = (!slt || eq);
  bool flag_sf = flag_ge ? flag_of : !flag_of;
  uint32_t flag_zf_mask = flag_zf ? FLAG_ZF : 0;
  uint32_t flag_cf_mask = flag_cf ? FLAG_CF : 0;
  uint32_t flag_sf_mask = flag_sf ? FLAG_SF : 0;
  uint32_t flag_of_mask = flag_of ? FLAG_OF : 0;
  return flag_zf_mask | flag_cf_mask | flag_sf_mask | flag_of_mask;
}

static bool
str_belongs(vector<string> const &eqclass, char const *str)
{
  for (const auto &estr : eqclass) {
    if (!strcmp(estr.c_str(), str)) {
      return true;
    }
  }
  return false;
}

static bool
i386_opcode_is_related_to_opcode(char const *opc_a, char const *opc_b)
{
  if (!strcmp(opc_a, opc_b)) {
    return true;
  }
  static vector<vector<string>> const eqclasses = {
    {"seta", "setbe"},
    {"setg", "setle"},
    {"setae", "setb"},
    {"setge", "setl"},
    {"leal", "addl", "subl"},
    {"je", "jne", "jg", "jle", "jge", "jl", "ja", "jae", "jb", "jbe", "jecxz", "jcxz"},
  };
  for (size_t i = 0; i < eqclasses.size(); i++) {
    if (   str_belongs(eqclasses.at(i), opc_a)
        && str_belongs(eqclasses.at(i), opc_b)) {
      return true;
    }
  }
  return false;
}

bool
i386_opcodes_related(i386_insn_t const &a, i386_insn_t const *b_iseq, long b_iseq_len)
{
  char const *opc_a = opctable_name(a.opc);
  for (long i = 0; i < b_iseq_len; i++) {
    char const *opc_b = opctable_name(b_iseq[i].opc);
    if (i386_opcode_is_related_to_opcode(opc_a, opc_b)) {
      return true;
    }
  }
  return false;
}

void
i386_insn_rename_memory_operand_to_symbol(i386_insn_t &insn, symbol_id_t symbol_id)
{
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    operand_t &op = insn.op[i];
    switch (op.type) {
      case op_mem:
        op.valtag.mem.base.val = -1;
        op.valtag.mem.base.tag = tag_invalid;
        op.valtag.mem.index.val = -1;
        op.valtag.mem.index.tag = tag_invalid;
        op.valtag.mem.scale = 0;
        op.valtag.mem.disp.tag = tag_imm_symbol;
        op.valtag.mem.disp.val = symbol_id;
        break;
      default:
        break;
    }
  }
}

long
i386_iseq_count_touched_regs_as_cost(i386_insn_t const *iseq, long iseq_len)
{
  regset_t regset;
  regset_clear(&regset);
  for (long i = 0; i < iseq_len; i++) {
    for (long j = 0; j < I386_MAX_NUM_OPERANDS; j++) {
      switch (iseq[i].op[j].type) {
        case op_reg:
          regset_mark_ex_mask(&regset, I386_EXREG_GROUP_GPRS, iseq[i].op[j].valtag.reg.val, MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_GPRS)));
          break;
        case op_mem:
          if (iseq[i].op[j].valtag.mem.base.val != -1) {
            regset_mark_ex_mask(&regset, I386_EXREG_GROUP_GPRS, iseq[i].op[j].valtag.mem.base.val, MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_GPRS)));
          }
          if (iseq[i].op[j].valtag.mem.index.val != -1) {
            regset_mark_ex_mask(&regset, I386_EXREG_GROUP_GPRS, iseq[i].op[j].valtag.mem.index.val, MAKE_MASK(I386_EXREG_LEN(I386_EXREG_GROUP_GPRS)));
          }
          break;
        default:
          break;
      }
    }
  }
  return regset_count_exregs(&regset, I386_EXREG_GROUP_GPRS);
}

void
i386_insn_rename_tag_src_insn_pcs_to_tag_vars(i386_insn_t *insn, nextpc_t const *nextpcs, uint8_t const * const *varvals, size_t num_nextpcs)
{
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    operand_t *op = &insn->op[i];
    if (op->type != op_pcrel) {
      continue;
    }
    if (op->valtag.pcrel.tag != tag_src_insn_pc) {
      continue;
    }
    if (PC_IS_NEXTPC_CONST(op->valtag.pcrel.val)) {
      continue;
    }
    long index = nextpc_t::nextpc_array_search(nextpcs, num_nextpcs, op->valtag.pcrel.val);
    ASSERT(index != -1);
    ASSERT(index >= 0 && index < num_nextpcs);
    op->valtag.pcrel.val = (long)varvals[index];
    op->valtag.pcrel.tag = tag_var;
  }
}

void
i386_insn_rename_tag_src_insn_pcs_to_tag_var_using_tcodes(i386_insn_t *insn, struct chash const *tcodes)
{
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    operand_t *op = &insn->op[i];
    if (op->type != op_pcrel) {
      continue;
    }
    if (op->valtag.pcrel.tag != tag_src_insn_pc) {
      continue;
    }
    if (PC_IS_NEXTPC_CONST(op->valtag.pcrel.val)) {
      continue;
    }
    uint8_t const *tc_ptr = tcodes_query(tcodes, op->valtag.pcrel.val);
    if (!tc_ptr) {
      continue;
    }
    op->valtag.pcrel.val = (long)tc_ptr;
    op->valtag.pcrel.tag = tag_var;
  }
}

void
i386_insn_rename_tag_src_insn_pcs_to_dst_pcrels_using_tinsns_map(i386_insn_t *insn, map<src_ulong, i386_insn_t const *> const &tinsns_map)
{
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    operand_t *op = &insn->op[i];
    if (op->type != op_pcrel) {
      continue;
    }
    if (op->valtag.pcrel.tag != tag_src_insn_pc) {
      continue;
    }
    if (PC_IS_NEXTPC_CONST(op->valtag.pcrel.val)) {
      continue;
    }
    if (!tinsns_map.count(op->valtag.pcrel.val)) {
      continue;
    }
    i386_insn_t const *dst_insn_ptr = tinsns_map.at(op->valtag.pcrel.val);
    op->valtag.pcrel.val = dst_insn_ptr - (insn + 1);
    op->valtag.pcrel.tag = tag_const;
  }
}

bool
i386_insn_is_move_insn(i386_insn_t const *insn, exreg_group_id_t &g, exreg_id_t &from, exreg_id_t &to)
{
  if (   !strcmp(opctable_name(insn->opc), "movl")
      && insn->op[0].type == op_reg
      && insn->op[1].type == op_reg) {
    //ASSERT(insn->op[0].type == op_reg);
    //ASSERT(insn->op[1].type == op_reg);
    ASSERT(insn->op[2].type == invalid); //there are only two operands op[0] and op[1]
    g = I386_EXREG_GROUP_GPRS;
    from = insn->op[1].valtag.reg.val;
    to = insn->op[0].valtag.reg.val;
    return true;
  }
  return false;
}

bool
i386_insn_rename_defed_reg(i386_insn_t *insn, exreg_group_id_t g, reg_identifier_t const &from, reg_identifier_t const &to)
{
  if (   insn->op[0].type == op_reg
      && g == I386_EXREG_GROUP_GPRS
      && insn->op[0].valtag.reg.val == from.reg_identifier_get_id()) {
    ASSERT(insn->op[0].valtag.reg.tag == tag_const);
    insn->op[0].valtag.reg.val = to.reg_identifier_get_id();
    return true;
  }
  return false;
}

static bool
i386_operand_is_well_formed(operand_t const *op)
{
  if (op->type == invalid) {
    return true;
  }
  if (op->type == op_mem) {
    if (   op->valtag.mem.base.val != -1
        && op->valtag.mem.base.mod.reg.mask != MAKE_MASK(DWORD_LEN)) {
      return false;
    }
  }
  return true;
}

static bool
i386_insn_is_well_formed(i386_insn_t const *insn)
{
  for (size_t i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    if (!i386_operand_is_well_formed(&insn->op[i])) {
      return false;
    }
  }
  return true;
}

bool
i386_iseq_is_well_formed(i386_insn_t const *iseq, size_t iseq_len)
{
  for (size_t i = 0; i < iseq_len; i++) {
    if (!i386_insn_is_well_formed(&iseq[i])) {
      return false;
    }
  }
  return true;
}

void
i386_insn_rename_regs(i386_insn_t *insn, regmap_t const *regmap)
{
  long i/*, n, val*/, reg;
  operand_t *op;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    op = &insn->op[i];
    if (op->type == op_reg) {
      reg = op->valtag.reg.val;
      ASSERT(reg >= 0 && reg < I386_NUM_GPRS);
      reg_identifier_t const &val = regmap_get_elem(regmap, I386_EXREG_GROUP_GPRS, reg);
      if (val.reg_identifier_is_unassigned()) {
        continue;
      }
      //ASSERT(val >= 0 && val < I386_NUM_GPRS);
      op->valtag.reg.val = val.reg_identifier_get_id();
    } else if (op->type == op_mem) {
      //ASSERT(op->tag.mem.all == tag_const);
      if (op->valtag.mem.base.val != -1) {
        ASSERT(op->valtag.mem.base.val >= 0);
        ASSERT(op->valtag.mem.base.val < I386_NUM_GPRS);
        reg_identifier_t const &val = regmap_get_elem(regmap, I386_EXREG_GROUP_GPRS, op->valtag.mem.base.val).reg_identifier_get_id();
        if (val.reg_identifier_is_unassigned()) {
          continue;
        }
        op->valtag.mem.base.val = val.reg_identifier_get_id();
      }
      if (op->valtag.mem.index.val != -1) {
        ASSERT(op->valtag.mem.index.val >= 0);
        ASSERT(op->valtag.mem.index.val < I386_NUM_GPRS);
        reg_identifier_t const &val = regmap_get_elem(regmap, I386_EXREG_GROUP_GPRS, op->valtag.mem.index.val).reg_identifier_get_id();
        if (val.reg_identifier_is_unassigned()) {
          continue;
        }
        op->valtag.mem.index.val = val.reg_identifier_get_id();
      }
    }
  }
}

static bool
insn_regcons_allows_renaming(i386_insn_t const *insn, int opnum)
{
  char const *opc = opctable_name(insn->opc);
  char const *p;
  //need to check if the regcons allows insn->op[opnum] to be renamed; for now we are just putting special checks; need a better solution that perhaps utilizes infer_regcons_from_assembly() TODO

  //the second non-implicit operand of shift opcodes has to be cl
  if (   opnum == insn->num_implicit_ops + 1
      && insn->op[opnum].type == op_reg
      && (p = strstr_arr(opc, shift_opcodes, num_shift_opcodes))
      && (p == opc || *(p - 1) != 'u')  /* to avoid confusing with pushl */
      && (p[3] != 'x')) {
    return false;
  }
  return true;
}

bool
i386_insn_reg_is_replaceable(i386_insn_t const *insn, exreg_group_id_t group, exreg_id_t reg)
{
  for (int i = 0; i < insn->num_implicit_ops; i++) {
    if (   insn->op[i].type == op_reg
        && group == I386_EXREG_GROUP_GPRS
        && insn->op[i].valtag.reg.val == reg) {
      return false; //shouldn't be an implicit op
    }
    //XXX: need to add more rules for other groups
  }
  for (int i = insn->num_implicit_ops; i < I386_MAX_NUM_OPERANDS; i++) {
    if (   insn->op[i].type == op_reg
        && group == I386_EXREG_GROUP_GPRS
        && insn->op[i].valtag.reg.val == reg) {
      if (!insn_regcons_allows_renaming(insn, i)) {
        return false;
      }
      return true; //should be an explicit op
    }
    //XXX: need to add more rules for other groups
  }
  return false;
}

bool
i386_insn_reg_def_is_replaceable(i386_insn_t const *insn, exreg_group_id_t group, exreg_id_t reg)
{
  //we are looking for cases where ONLY the def'ed register is replaceable; it shouldn't so happen that replacing the def'ed register also replaces the used register; the two instructions that fit this condition are lea and mov
  exreg_group_id_t g;
  exreg_id_t from, to;
  if (   (   i386_insn_is_lea(insn)
          || i386_insn_is_move_insn(insn, g, from, to))
      && insn->op[0].valtag.reg.val == reg
      && group == I386_EXREG_GROUP_GPRS) {
    return true;
  }
  return false;
}

void
i386_insn_rename_dst_reg(i386_insn_t *insn, exreg_group_id_t group, exreg_id_t from, exreg_id_t to)
{
  //we are looking for cases where ONLY the def'ed register is replaced; it shouldn't so happen that replacing the def'ed register also replaces the used register; the two instructions that fit this condition are lea and mov
  exreg_group_id_t g;
  exreg_id_t from_reg, to_reg;
  if (   (   i386_insn_is_lea(insn)
          || i386_insn_is_move_insn(insn, g, from_reg, to_reg))
      && insn->op[0].valtag.reg.val == from
      && group == I386_EXREG_GROUP_GPRS) {
    insn->op[0].valtag.reg.val = to;
    return;
  }
  NOT_REACHED();
}

void
i386_insn_invert_branch_condition(i386_insn_t *insn)
{
  ASSERT(i386_insn_is_conditional_direct_branch(insn));
  if (insn->opc == opc_je) {
    insn->opc = opc_jne;
  } else if (insn->opc == opc_jne) {
    insn->opc = opc_je;
  } else if (insn->opc == opc_jg) {
    insn->opc = opc_jle;
  } else if (insn->opc == opc_jle) {
    insn->opc = opc_jg;
  } else if (insn->opc == opc_jl) {
    insn->opc = opc_jge;
  } else if (insn->opc == opc_jge) {
    insn->opc = opc_jl;
  } else if (insn->opc == opc_ja) {
    insn->opc = opc_jbe;
  } else if (insn->opc == opc_jbe) {
    insn->opc = opc_ja;
  } else if (insn->opc == opc_jb) {
    insn->opc = opc_jae;
  } else if (insn->opc == opc_jae) {
    insn->opc = opc_jb;
  } else if (insn->opc == opc_jo) {
    insn->opc = opc_jno;
  } else if (insn->opc == opc_jno) {
    insn->opc = opc_jo;
  } else if (insn->opc == opc_js) {
    insn->opc = opc_jns;
  } else if (insn->opc == opc_jns) {
    insn->opc = opc_js;
  } else if (insn->opc == opc_jp) {
    insn->opc = opc_jnp;
  } else if (insn->opc == opc_jnp) {
    insn->opc = opc_jp;
  } else NOT_REACHED();
}

bool
i386_insn_deallocates_stack(i386_insn_t const *insn)
{
  int n = insn->num_implicit_ops;
  ASSERT(n >= 0);
  if (   !strcmp(opctable_name(insn->opc), "addl")
      && insn->op[n + 0].type == op_reg
      && insn->op[n + 0].valtag.reg.val == R_ESP
      && insn->op[n + 0].valtag.reg.tag == tag_const
      && insn->op[n + 1].type == op_imm) {
    return true;
  }
  return false;
}

void
i386_insn_convert_function_call_to_branch(i386_insn_t *insn)
{
  insn->opc = opc_jmp;
  operand_copy(&insn->op[0], &insn->op[insn->num_implicit_ops]);
  for (size_t i = 1; i < I386_MAX_NUM_OPERANDS; i++) {
    insn->op[i].type = invalid;
    insn->op[i].size = 0;
  }
  insn->num_implicit_ops = 0;
}

bool
i386_iseq_is_function_tailcall_opt_candidate(i386_insn_t const *iseq, long iseq_len)
{
  return false;
}

static void
i386_insn_convert_pcrels_to_reloc_strings(i386_insn_t *insn, map<long, string> const& labels, nextpc_t const* nextpcs, long num_nextpcs, input_exec_t const* in, src_ulong const* insn_pcs, int insn_index, char **reloc_strings, long *reloc_strings_size)
{
  valtag_t *pcrel_op = i386_insn_get_pcrel_operand(insn);
  if (!pcrel_op) {
    return;
  }
  if (pcrel_op->tag == tag_const) {
    long target_index = pcrel_op->val + 1 + insn_index;
    if (!labels.count(target_index)) {
      cout << __func__ << " " << __LINE__ << ": could not find label for index " << target_index << endl;
    }
    ASSERT(labels.count(target_index));
    string const& labelname = labels.at(target_index);
    pcrel_op->val = add_or_find_reloc_symbol_name(reloc_strings, reloc_strings_size, labelname.c_str());
    pcrel_op->tag = tag_reloc_string;
  } else if (pcrel_op->tag == tag_var) {
    long nextpc_num = pcrel_op->val;
    ASSERT(nextpc_num >= 0 && nextpc_num < num_nextpcs);
    nextpc_t const& nextpc = nextpcs[nextpc_num];
    if (nextpc.get_tag() == tag_input_exec_reloc_index) {
      src_ulong nextpc_val = nextpc.get_reloc_idx();
      ASSERT(nextpc_val >= 0 && nextpc_val < in->num_relocs);
      //string nextpc_name = string("@") + in->relocs[nextpc_val].symbolName;
      string nextpc_name = in->relocs[nextpc_val].symbolName;
      pcrel_op->val = add_or_find_reloc_symbol_name(reloc_strings, reloc_strings_size, nextpc_name.c_str());
      pcrel_op->tag = tag_reloc_string;
    } else if (nextpc.get_tag() == tag_const) {
      src_ulong nextpc_val = nextpc.get_val();
      char const* sym = lookup_symbol(in, nextpc_val);
      if (sym) {
        pcrel_op->val = add_or_find_reloc_symbol_name(reloc_strings, reloc_strings_size, sym);
        pcrel_op->tag = tag_reloc_string;
      } else {
        NOT_REACHED(); //not sure if this can be reached or not
      }
    }
  } else {
    return;
  }
}

static void
i386_insn_convert_symbols_to_reloc_strings(i386_insn_t *insn, input_exec_t const* in, set<symbol_id_t>& syms_used, char **reloc_strings, long *reloc_strings_size)
{
  //rename op_imm and op_mem.disp symbols
  for (size_t i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    valtag_t* op = nullptr;
    if (insn->op[i].type == op_imm) {
      op = &insn->op[i].valtag.imm;
    } else if (insn->op[i].type == op_mem) {
      op = &insn->op[i].valtag.mem.disp;
    } else {
      continue;
    }
    ASSERT(op);
    if (op->tag == tag_imm_symbol) {
      long symbol_id = op->val;
      ASSERT(in->symbol_map.count(symbol_id));
      syms_used.insert(symbol_id);
      //string symbol_name = string("@") + in->symbol_map.at(symbol_id).name;
      string symbol_name = in->symbol_map.at(symbol_id).name;
      long offset = add_or_find_reloc_symbol_name(reloc_strings, reloc_strings_size, symbol_name.c_str());
      op->tag = tag_reloc_string;
      op->val = offset;
    }
  }
}

void
i386_insn_convert_pcrels_and_symbols_to_reloc_strings(i386_insn_t *insn, map<long, string> const& labels, nextpc_t const* nextpcs, long num_nextpcs, input_exec_t const* in, src_ulong const* insn_pcs, int insn_index, std::set<symbol_id_t>& syms_used, char **reloc_strings, long *reloc_strings_size)
{
  i386_insn_convert_pcrels_to_reloc_strings(insn, labels, nextpcs, num_nextpcs, in, insn_pcs, insn_index, reloc_strings, reloc_strings_size);
  i386_insn_convert_symbols_to_reloc_strings(insn, in, syms_used, reloc_strings, reloc_strings_size);
}

void
i386_insn_t::serialize(ostream& os) const
{
  serialize_int(os, this->opc);
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    this->op[i].serialize(os);
  }
  serialize_int(os, this->num_implicit_ops);
}

void
i386_insn_t::deserialize(istream& is)
{
  this->opc = deserialize_opc_t(is);
  for (int i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    this->op[i].deserialize(is);
  }
  this->num_implicit_ops = deserialize_int(is);
}

void
operand_t::serialize(ostream& os) const
{
  serialize_int(os, (int)this->type);
  serialize_int(os, (int)this->size);
  serialize_int(os, (int)this->rex_used);
  if (this->type == op_mem) {
    this->valtag.mem.serialize(os);
  } else {
    this->valtag.seg.serialize(os); //all other types are valtag_t
  }
}

void
operand_t::deserialize(istream& is)
{
  this->type = (opertype_t)deserialize_int(is);
  this->size = deserialize_int(is);
  this->rex_used = deserialize_int(is);
  if (this->type == op_mem) {
    this->valtag.mem.deserialize(is);
  } else {
    this->valtag.seg.deserialize(is);
  }
}

void
serialize_opc_t(ostream& os, opc_t opc)
{
  serialize_int(os, opc);
}

opc_t
deserialize_opc_t(istream& is)
{
  return deserialize_int(is);
}

void
i386_insn_convert_malloc_to_mymalloc(i386_insn_t* insn, char** reloc_strings, long* reloc_strings_size)
{
  valtag_t *pcrel_op = i386_insn_get_pcrel_operand(insn);
  if (!pcrel_op) {
    return;
  }
  if (pcrel_op->tag != tag_reloc_string) {
    return;
  }
  char const* callee_name = *reloc_strings + pcrel_op->val;
  string new_callee_name = convert_malloc_callee_to_mymalloc(callee_name);
  if (new_callee_name == callee_name) {
    return;
  }
  pcrel_op->val = add_or_find_reloc_symbol_name(reloc_strings, reloc_strings_size, new_callee_name.c_str());
}

static void
i386_iseq_expand_constants_to_max_width(i386_insn_t *exec_iseq_const,
    i386_insn_t const *exec_iseq, size_t exec_iseq_len)
{
  operand_t const *orig_op;
  int i, j, max_op_size;
  operand_t *op;

  for (i = 0; i < exec_iseq_len; i++) {
    for (j = 0; j < I386_MAX_NUM_OPERANDS; j++) {
      op = &exec_iseq_const[i].op[j];
      orig_op = &exec_iseq[i].op[j];
      max_op_size = i386_max_operand_size(op, exec_iseq_const[i].opc, j - exec_iseq_const[i].num_implicit_ops);
      if (op->type == op_imm) {
        ASSERT(op->valtag.imm.tag == tag_const);
        ASSERT(orig_op->type == op_imm);
        if (orig_op->valtag.imm.tag == tag_var) {
          op->valtag.imm.val = 0x10101010 & ((1ULL << (max_op_size * BYTE_LEN)) - 1);
        }
      } else if (op->type == op_mem) {
        ASSERT(op->valtag.mem.disp.tag == tag_const);
        ASSERT(orig_op->type == op_mem);
        if (orig_op->valtag.mem.disp.tag == tag_var) {
          op->valtag.mem.disp.val = 0x10101010;
        }
      }
    }
  }
}

size_t
i386_assemble_exec_iseq_using_regmap(uint8_t *buf, size_t buf_size,
    i386_insn_t const *exec_iseq, long exec_iseq_len, regmap_t const *dst_regmap,
    regmap_t const *src_regmap/*, int d*/, memslot_map_t const &memslot_map)
{
  i386_insn_t *exec_iseq_const = new i386_insn_t[exec_iseq_len];
  static imm_vt_map_t const_imm_map;
  static bool init = false;
  size_t ret;

  if (!init) {
    int i;
    imm_vt_map_init(&const_imm_map);
    for (i = 0; i < NUM_CANON_CONSTANTS; i++) {
      const_imm_map.table[i].tag = tag_const;
      const_imm_map.table[i].val = 0x10101010 + i;
    }
    const_imm_map.num_imms_used = NUM_CANON_CONSTANTS;
    init = true;
  }

  stopwatch_run(&i386_assemble_exec_iseq_using_regmap_timer);

  i386_iseq_copy(exec_iseq_const, exec_iseq, exec_iseq_len);
  i386_iseq_rename(exec_iseq_const, exec_iseq_len, dst_regmap, //use_regmap?&iregmap:&regmap,
      &const_imm_map, &memslot_map, src_regmap, NULL,
      //(!use_regmap && (d == JTABLE_EXEC_ISEQ_TYPE))?&iregmap:NULL,
      NULL, 0);
  //expand constants to max width to ensure regular assembly
  i386_iseq_expand_constants_to_max_width(exec_iseq_const, exec_iseq, exec_iseq_len);

  DBG3(JTABLE, "dst_regmap:\n%s\n",
      dst_regmap?regmap_to_string(dst_regmap, as1, sizeof as1):NULL);
  DBG3(JTABLE, "src_regmap:\n%s\n",
      src_regmap?regmap_to_string(src_regmap, as1, sizeof as1):NULL);
  DBG3(JTABLE, "exec_iseq_const:\n%s\n",
      i386_iseq_to_string(exec_iseq_const, exec_iseq_len, as1, sizeof as1));

  ret = i386_iseq_assemble(NULL, exec_iseq_const, exec_iseq_len, buf, buf_size,
      I386_AS_MODE_64_REGULAR);
  if (!(ret >= 0 && ret < buf_size)) {
    DBG(TMP, "exec_iseq_const:\n%s\n",
        i386_iseq_to_string(exec_iseq_const, exec_iseq_len, as1, sizeof as1));
  }
  ASSERT(ret >= 0 && ret < buf_size);
  stopwatch_stop(&i386_assemble_exec_iseq_using_regmap_timer);

  delete [] exec_iseq_const;
  return ret;
}

bool
i386_insn_t::uses_serialization()
{
  return true;
}
