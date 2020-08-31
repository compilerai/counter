#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <set>
//#include "gas/gas_common.h"
//#include "imm_map.h"
#include "support/debug.h"
#include "ppc/insn.h"
#include "support/c_utils.h"
#include "rewrite/peephole.h"
#include "valtag/memset.h"
#include "valtag/elf/elf.h"
#include "rewrite/translation_instance.h"
#include "rewrite/src-insn.h"
#include "rewrite/transmap.h"
#include "valtag/regcons.h"
#include "valtag/regmap.h"
#include "rewrite/bbl.h"
#include "exec/state.h"
#include "superopt/typestate.h"
#include "valtag/nextpc_map.h"
#include "rewrite/ldst_input.h"
#include "rewrite/dst-insn.h"
#include "gas/opcode-ppc.h"

static char as1[40960];
static char as2[1024];
static char as3[1024];

static int gas_inited;

ppc_costfn_t ppc_costfns[MAX_PPC_COSTFNS];
long ppc_num_costfns;

using namespace std;


extern "C" {
/* gas assembly functions */
int ppc_md_assemble(char *assembly, uint8_t *bincode);
void ppc_md_begin(void);
//int i386_md_assemble (char *assembly, char *bincode);
void i386_md_begin(void);
void subsegs_begin(void);
void symbol_begin(void);
void ppc_md_begin(void);
}

static bool
ppc_bincode_is_unconditional_branch(uint8_t const *bincode)
{
  if ((bincode[3] >> 2) == 0x12) {
    return true;
  }
  return false;
}

static bool
ppc_bincode_is_conditional_branch(uint8_t const *bincode)
{
  if ((bincode[3] >> 2) == 0x10) {
    return true;
  }
  return false;
}

static ssize_t
ppc_patch_jump_offset(uint8_t *tmpbuf, ssize_t len, sym_type_t symtype,
    unsigned long symval, char const *binptr, bool first_pass)
{
  int jmp_off, jmp_target_size;
  int32_t val;

  jmp_off = 0;
  jmp_target_size = 0;
  if (ppc_bincode_is_unconditional_branch(tmpbuf)) {
    jmp_off = 0;
    jmp_target_size = 3;
  } else if (ppc_bincode_is_conditional_branch(tmpbuf)) {
    jmp_off = 0;
    jmp_target_size = 2;
  }
  val = 0;
  if (symtype == SYMBOL_PCREL) {
    ASSERT(jmp_target_size);
    val = symval - (unsigned long)binptr;
  } else if (symtype == SYMBOL_ABS) {
    val = symval;
  }
  if (jmp_target_size == 3) {
    *(uint8_t *)(tmpbuf + jmp_off + 0) &= ~0xfc;
    *(uint8_t *)(tmpbuf + jmp_off + 0) |= (uint8_t)(val & 0xfc);
    *(uint8_t *)(tmpbuf + jmp_off + 1) = (uint8_t)((val >> 8) & 0xff);
    *(uint8_t *)(tmpbuf + jmp_off + 2) = (uint8_t)((val >> 16) & 0xff);
  } else if (jmp_target_size == 2) {
    *(uint8_t *)(tmpbuf + jmp_off + 0) &= ~0xfc;
    *(uint8_t *)(tmpbuf + jmp_off + 0) |= (uint8_t)(val & 0xfc);
    *(uint8_t *)(tmpbuf + jmp_off + 1) = (uint8_t)((val >> 8) & 0xff);
  }
  ASSERT(len == 4);
  return len;
}

static ssize_t
__ppc_assemble(uint8_t *bincode, size_t size, char const *assembly,
    sym_t *symbols, long *num_symbols)
{
  return insn_assemble(NULL, bincode, assembly, &ppc_md_assemble,
      &ppc_patch_jump_offset, symbols, num_symbols);

  //factor out common code with __x86_assemble
  /*DBG(MD_ASSEMBLE, "Calling ppc_md_assemble: %s\n", assembly);
  as = ppc_md_assemble(assembly_copy, (char *)bincode);
  if (as == -1) {
    DBG(MD_ASSEMBLE, "%s", "ppc_md_assemble() failed.\n");
    return -1;
  } else {
    DBG(MD_ASSEMBLE, "ppc_md_assemble() returned %s\n",
        hex_string(bincode, as, as1, sizeof as1));
    ASSERT(as == 4);
    return as;
  }*/
}

ssize_t
ppc_assemble(translation_instance_t *ti, uint8_t *bincode, size_t bincode_size,
    char const *assembly, i386_as_mode_t mode)
{
  char assembly_copy[strlen(assembly) + 16];
  sym_t symbols[strlen(assembly) + 1];
  long num_symbols;
  size_t ret;

  num_symbols = 0;

  strip_extra_whitespace_and_comments(assembly_copy, assembly);
  DBG(MD_ASSEMBLE, "assembly:\n%s\nassembly_copy:\n%s\n", assembly, assembly_copy);

  __ppc_assemble(bincode, bincode_size, assembly_copy, symbols, &num_symbols);
  ret = __ppc_assemble(bincode, bincode_size, assembly_copy, symbols, &num_symbols);
  return ret;
}

/*
void print_ppc_insn(ppc_insn_t const *insn)
{
  printf("\topc1 %x, opc2 %x, opc3 %x r[SD] %x, rA %x, rB %x, FM %x simm %x(%d)"
      ", LI %x, AA %d, LK %d, crfS %d, crfD %d, crbA %d, crbB %d, crbD %d "
      "BO %x, BI %d, BD %x, Rc %d, NB %d, SH %d, MB %d, ME %d, SPR %x, CRM %x "
      "reserved_mask %x\n",
      insn->opc1, insn->opc2, insn->opc3, insn->rSD.rD,
      insn->rA, insn->rB, insn->FM, insn->imm.simm, insn->imm_signed,
      insn->LI, insn->AA,
      insn->LK, insn->crfS, insn->crfD, insn->crbA, insn->crbB, insn->crbD,
      insn->BO, insn->BI, insn->BD, insn->Rc, insn->NB, insn->SH,
      insn->MB, insn->ME, insn->SPR, insn->CRM, insn->reserved_mask);
}
*/

static bool
ppc_insn_zero_rA_operand_means_no_reg(ppc_insn_t const *insn)
{
  int opc23;

  //addi addis lbz lbzx lwz lwzx lhz lhzx lha lhax
  //stb stbx stw stwx sth sthx lhbrx lwbrx sthbrx stwbrx lwarx stwcx.


  if (insn->opc1 == 14 || insn->opc1 == 15) {
    // addi addis 
    //(addic and addic_ do not fall in this category,
    // ug011 manual has incorrect listing, see detailed instruction spec for clarity)
    return true;
  }

  if (   insn->opc1 == 34  //lbz
      || (insn->opc1 == 31 && insn->opc2 == 23 && insn->opc3 == 2) //lbzx
      || insn->opc1 == 32  //lwz
      || (insn->opc1 == 31 && insn->opc2 == 23 && insn->opc3 == 0) //lwzx
      || insn->opc1 == 40  //lhz
      || (insn->opc1 == 31 && insn->opc2 == 23 && insn->opc3 == 8) //lhzx
      || insn->opc1 == 42  //lha
      || (insn->opc1 == 31 && insn->opc2 == 23 && insn->opc3 == 10) //lhax
     ) {
    return true;
  }

  opc23 = insn->opc3 * 32 + insn->opc2;
  if (/*   (insn->opc1 == 31 && opc23 == 758)  //dcbz
      || */(insn->opc1 == 31 && opc23 == 86)   //dcbf
      || (insn->opc1 == 31 && opc23 == 470)  //dcbi
      || (insn->opc1 == 31 && opc23 == 54)   //dcbst
      || (insn->opc1 == 31 && opc23 == 278)  //dcbt
      || (insn->opc1 == 31 && opc23 == 246)  //dcbtst
      || (insn->opc1 == 31 && opc23 == 1014) //dcbz
      || (insn->opc1 == 31 && opc23 == 982)  //icbi
      || (insn->opc1 == 31 && opc23 == 262)  //icbt
     ) {
    //printf("returning true for %s\n", ppc_insn_to_string(insn, as1, sizeof as1));
    return true;
  }

  if (   insn->opc1 == 38  //stb
      || (insn->opc1 == 31 && insn->opc2 == 23 && insn->opc3 == 6) //stbx
      || insn->opc1 == 36  //stw
      || (insn->opc1 == 31 && insn->opc2 == 23 && insn->opc3 == 4) //stwx
      || insn->opc1 == 44  //sth
      || (insn->opc1 == 31 && insn->opc2 == 23 && insn->opc3 == 12) //sthx
     ) {
    return true;
  }

  if (   insn->opc1 == 31
      && insn->opc2 == 22
      && insn->opc3 == 24) {
    // lhbrx
    return true;
  }

  if (   insn->opc1 == 31
      && insn->opc2 == 22
      && insn->opc3 == 16) {
    // lwbrx
    return true;
  }

  if (   insn->opc1 == 31
      && insn->opc2 == 22
      && insn->opc3 == 20) {
    // stwbrx
    return true;
  }

  if (   insn->opc1 == 31
      && insn->opc2 == 22
      && insn->opc3 == 28) {
    // sthbrx
    return true;
  }

  /*if (insn->opc1 == 47) {
    // stmw
    return true;
  }*/

  if (   insn->opc1 == 0x1f
      && insn->opc2 == 0x14
      && insn->opc3 == -1) {
    // lwarx
    return true;
  }
  if (   insn->opc1 == 0x1f
      && insn->opc2 == 0x16
      && insn->opc3 == 0x4) {
    // stwcx
    return true;
  }
  if (   insn->opc1 == 0x1f
      && insn->opc2 == 0x16
      && (insn->opc3 == 0x1f || 0x7 || 0x8 || 0x1 || 0xe || 0x2 || 0x1e)) {
    //dcbz dcbtst dcbt dcbst dcbi dcbf icbi
    return true;
  }

  return false;
}



#define PPC_INSN_CONST_OPERAND_IS_NULL_REG(insn, field) ({ \
  bool __ret; \
  /*if (insn->valtag.field.val != 0) { \
    __ret = true;\
  } else */if (strcmp("rA", #field)) { \
    __ret = false; \
  } else if (!ppc_insn_zero_rA_operand_means_no_reg(insn)) { \
    __ret = false; \
  } else { \
    __ret = true; \
  } \
  __ret; \
})



#define EXTRACT_IMM(insn, field) do { \
  int i;\
  *imm_mod = insn->valtag.field.mod.imm.modifier; \
  *constant_val = insn->valtag.field.mod.imm.constant_val; \
  *constant_tag = insn->valtag.field.mod.imm.constant_tag; \
  for (i = 0; i < IMM_MAX_NUM_SCONSTANTS; i++) { \
    sconstants[i] = insn->valtag.field.mod.imm.sconstants[i]; \
  } \
  *exvreg_group = insn->valtag.field.mod.imm.exvreg_group; \
} while(0)

static void
operand_extract_from_insn(struct powerpc_operand const *operand, ppc_insn_t const *insn,
    uint32_t bincode, int *invalid, int32_t *value_ptr, src_tag_t *tag,
    imm_modifier_t *imm_mod, unsigned *constant_val, src_tag_t *constant_tag,
    uint64_t *sconstants, int *exvreg_group, int vconst_num)
{
  int32_t value;
  //bool imm;
  int i;

  if (operand->extract) {
    value = (*operand->extract)(bincode, invalid);
  } else {
    value = (bincode >> operand->shift) & ((1 << operand->bits) - 1);
    //DBG(TMP, "value = %x\n", value);
    if ((operand->flags & PPC_OPERAND_SIGNED) != 0
        && (value & (1 << (operand->bits - 1))) != 0) {
      value -= 1 << operand->bits;
    }
    //DBG(TMP, "value = %x\n", value);
  }
  if (!value_ptr) {
    return;
  }
  *value_ptr = value;
  if (invalid) {
    *tag = tag_invalid;
  } else {
    *tag = tag_const;
  }

  //imm = false;
  if (operand->shift == 21 && operand->bits == 5) {
    if (insn->valtag.rSD.rS.base.tag != tag_invalid) {
      ASSERT(insn->valtag.rSD.rS.base.val == value);
      *tag = insn->valtag.rSD.rS.base.tag;
      //*exvreg_group = insn->exvreg_group.rSD.rS;
    } else if (insn->valtag.frSD.frS.base.tag != tag_invalid) {
      ASSERT(insn->valtag.frSD.frS.base.val == value);
      *tag = insn->valtag.frSD.frS.base.tag;
      //*exvreg_group = insn->exvreg_group.frSD.frS;
    } else if (insn->valtag.crbD.tag != tag_invalid) {
      ASSERT((insn->valtag.crbD.val & 31) == (value & 31));
      *value_ptr = insn->valtag.crbD.val;
      *tag = insn->valtag.crbD.tag;
      //*exvreg_group = insn->exvreg_group.crbD;
    } else if (insn->BO != -1) {
      ASSERT((insn->BO & 31) == (value & 31));
      *value_ptr = insn->BO;
      *tag = tag_const;
    } else NOT_REACHED();
  } else if (operand->shift == 16 && operand->bits == 5) {
    if (insn->valtag.rA.tag != tag_invalid) {
      if (insn->valtag.rA.val != value) {
        printf("%s() %d: insn->valtag.rA.val = %x, value = %x.\n", __func__, __LINE__, (unsigned)insn->valtag.rA.val, (unsigned)value);
      }
      ASSERT(insn->valtag.rA.val == value);
      *tag = insn->valtag.rA.tag;
      //*exvreg_group = insn->exvreg_group.rA;
    } else if (value == 0 && ppc_insn_zero_rA_operand_means_no_reg(insn)) {
      ASSERT(value == 0);
    } else if (insn->valtag.crbA.tag != tag_invalid) {
      ASSERT((insn->valtag.crbA.val & 31) == (value & 31));
      *value_ptr = insn->valtag.crbA.val;
      *tag = insn->valtag.crbA.tag;
      //*exvreg_group = insn->exvreg_group.crbA;
    } else if (insn->valtag.BI.tag != tag_invalid) {
      ASSERT((insn->valtag.BI.val & 31) == (value & 31));
      *value_ptr = insn->valtag.BI.val;
      *tag = insn->valtag.BI.tag;
      //*exvreg_group = insn->exvreg_group.BI;
    } else NOT_REACHED();
  } else if (operand->shift == 11 && operand->bits == 5) {
    if (insn->valtag.rB.tag != tag_invalid) {
      ASSERT(insn->valtag.rB.val == value);
      *tag = insn->valtag.rB.tag;
      //*exvreg_group = insn->exvreg_group.rB;
    } else if (insn->valtag.SH.tag != tag_invalid) {
      ASSERT((insn->valtag.SH.val & 31) == (value & 31));
      *value_ptr = insn->valtag.SH.val;
      *tag = insn->valtag.SH.tag;
      DBG2(PPC_INSN, "insn->tag.SH=%d, *vconstnum=%d, insn->modifier[vconst_num]=%d\n",
          (int)insn->valtag.SH.tag, (int)vconst_num,
          (int)insn->valtag.SH.mod.imm.modifier);
      //imm = true;
      EXTRACT_IMM(insn, SH);
      //*exvreg_group = insn->valtag.SH.exvreg_group[0];
    } else if (insn->valtag.NB.tag != tag_invalid) {
      ASSERT((insn->valtag.NB.val & 31) == (value & 31));
      *value_ptr = insn->valtag.NB.val;
      *tag = insn->valtag.NB.tag;
      //*exvreg_group = insn->exvreg_group.NB;
      EXTRACT_IMM(insn, NB);
      //imm = true;
    } else if (insn->valtag.crbB.tag != tag_invalid) {
      ASSERT((insn->valtag.crbB.val & 31) == (value & 31));
      *value_ptr = insn->valtag.crbB.val;
      *tag = insn->valtag.crbB.tag;
      //*exvreg_group = insn->exvreg_group.crbB;
    } else NOT_REACHED();
  } else if (operand->shift == 0 && operand->bits == 16) {
    if (insn->valtag.imm.simm.tag != tag_invalid) {
      /*ASSERT(   insn->tag.imm.simm == tag_const
             || (value & 0xffff) == (VCONSTANT_PLACEHOLDER & 0xffff));*/
      if (insn->valtag.imm_signed == 1) {
        *value_ptr = value; //insn->valtag.imm.simm.val; value is sign-extended form
        *tag = insn->valtag.imm.simm.tag;
        //*exvreg_group = insn->valtag.imm.simm.exvreg_group[0];
        EXTRACT_IMM(insn, imm.simm);
      } else if (insn->valtag.imm_signed == 0) {
        *value_ptr = value; //insn->valtag.imm.uimm.val;
        *tag = insn->valtag.imm.uimm.tag;
        //*exvreg_group = insn->valtag.imm.uimm.exvreg_group[0];
        EXTRACT_IMM(insn, imm.uimm);
      } else NOT_REACHED();
      //imm = true;
    } else {
      ASSERT(insn->valtag.BD.tag != tag_invalid);
      ASSERT((insn->valtag.BD.val & ((1 << 16) - 4)) == (value & ((1 << 16) - 4)));
      *value_ptr = insn->valtag.BD.val;
      *tag = insn->valtag.BD.tag;
    }
  } else if (operand->shift == 6 && operand->bits == 5) {
    ASSERT((insn->valtag.MB.val & 31) == (value & 31));
    *value_ptr = insn->valtag.MB.val;
    *tag = insn->valtag.MB.tag;
    //*exvreg_group = insn->valtag.MB.exvreg_group[0];
    EXTRACT_IMM(insn, MB);
    DBG3(PPC_INSN, "insn->tag.MB=%d, *vconstnum=%d, insn->modifier[vconst_num]=%d\n",
        (int)insn->valtag.MB.tag, (int)vconst_num,
        (int)insn->valtag.MB.mod.imm.modifier);
    //imm = true;
  } else if (operand->shift == 1 && operand->bits == 5) {
    ASSERT((insn->valtag.ME.val & 31) == (value & 31));
    *value_ptr = insn->valtag.ME.val;
    *tag = insn->valtag.ME.tag;
    //*exvreg_group = insn->valtag.ME.exvreg_group;
    DBG3(PPC_INSN, "insn->tag.ME=%d, *vconstnum=%d, insn->modifier[vconst_num]=%d\n",
        (int)insn->valtag.ME.tag, (int)vconst_num,
        (int)insn->valtag.ME.mod.imm.modifier);
    //imm = true;
    EXTRACT_IMM(insn, ME);
  } else if (operand->shift == 0 && operand->bits == 26) {
    ASSERT((insn->valtag.LI.val & ((1 << 26) - 4)) == (value & ((1 << 26) - 4)));
    *value_ptr = insn->valtag.LI.val;
    *tag = insn->valtag.LI.tag;
  } else if (operand->shift == 23 && operand->bits == 3) {
    *value_ptr = insn->valtag.crfD.val * 4;
    *tag = insn->valtag.crfD.tag;
    //*exvreg_group = insn->exvreg_group.crfD;
  } else if (operand->shift == 18 && operand->bits == 3) {
    *value_ptr = insn->valtag.crfS.val * 4;
    *tag = insn->valtag.crfS.tag;
    //*exvreg_group = insn->exvreg_group.crfS;
  }
  /*if (imm) {
    ASSERT(vconst_num < MAX_NUM_VCONSTANTS_PER_INSN);
    *imm_mod = insn->imm_modifier[vconst_num];
    *constant = insn->constant[vconst_num];
    for (i = 0; i < IMM_MAX_NUM_SCONSTANTS; i++) {
      sconstants[i] = insn->sconstants[vconst_num][i];
    }
    //(*vconst_num_ptr)++;
  }*/
}

static size_t
crf_reg_snprintf(char *buf, size_t size, src_tag_t tag,
    int32_t value, /*int exvreg_group, */struct powerpc_operand const *operand)
{
  char regname[32];
  char *ptr, *end;

  ASSERT(value >= 0 && value < 32);
  if (tag == tag_const) {
    snprintf(regname, sizeof regname, "cr%d", value/4);
  } else if (tag == tag_var) {
    snprintf(regname, sizeof regname, "%%exvr%d.%d", PPC_EXREG_GROUP_CRF, value/4);
  /*} else if (tag == tag_exvreg) {
    snprintf(regname, sizeof regname, "%%exvr%d.%d", exvreg_group, value/4);*/
  } else NOT_REACHED();
  ptr = buf;
  end = buf + size;
  if (operand->bits == 3) {
    ptr += snprintf(ptr, end - ptr, "%s", regname);
  } else {
    static const char *cbnames[4] = { "lt", "gt", "eq", "so" };
    int cr;
    int cc;

    cr = value >> 2;
    if (tag != tag_const || cr != 0) { //must not print cr0 reg for correct assembly
      ptr += snprintf(ptr, end - ptr, "4*%s", regname);
    }
    cc = value & 3;
    /*if (cc != 0)
    {*/
      if (tag != tag_const || cr != 0) {
        ptr += snprintf(ptr, end - ptr, "+");
      }
      ptr += snprintf(ptr, end - ptr, "%s", cbnames[cc]);
    //}
  }
  ASSERT(ptr < end);
  return ptr - buf;
}

static int
operand_snprintf(struct powerpc_operand const *operand, char *buf, size_t size,
    int32_t value, src_tag_t tag, imm_modifier_t imm_mod, unsigned constant_val,
    src_tag_t constant_tag, uint64_t const *sconstants, int exvreg_group, long insn_index)
{
  char *ptr = buf, *end = buf + size;
  if (tag == tag_const) {
    /* Print the operand as directed by the flags.  */
    if ((operand->flags & PPC_OPERAND_GPR) != 0)
      ptr += snprintf(ptr, end - ptr, "%d", value);
    else if ((operand->flags & PPC_OPERAND_FPR) != 0)
      ptr += snprintf(ptr, end - ptr, "%d", value);
    else if ((operand->flags & PPC_OPERAND_RELATIVE) != 0) {
      //ptr += snprintf(ptr, end - ptr, ".i%08lX", insn_index + 1 + value);
      ptr += snprintf(ptr, end - ptr, ".i%ld", insn_index + 1 + value);
    } else if ((operand->flags & PPC_OPERAND_ABSOLUTE) != 0) {
      ptr += snprintf(ptr, end - ptr, "%08X", value & 0xffffffff);
    } else if ((operand->flags & PPC_OPERAND_CR) == 0)
      ptr += snprintf(ptr, end - ptr, "0x%x", value);
    else {
      ptr += crf_reg_snprintf(ptr, end - ptr, tag_const, value, /*-1, */operand);
    }
  } else if (tag == tag_var /*|| tag == tag_exvreg*/) {
    /*char prefix[256];
    if (tag == tag_var) {
      snprintf(prefix, sizeof prefix, "%%vr");
    } else if (tag == tag_var) {
      snprintf(prefix, sizeof prefix, "%%exvr%d.", exvreg_group);
    } else NOT_REACHED();*/
    /* Print the operand as directed by the flags.  */
    if ((operand->flags & PPC_OPERAND_GPR) != 0) {
      ptr += snprintf(ptr, end - ptr, "%%exvr%d.%dd", PPC_EXREG_GROUP_GPRS, value);
    } else if ((operand->flags & PPC_OPERAND_FPR) != 0) {
      ptr += snprintf(ptr, end - ptr, "%%exvr%d.%d", PPC_EXREG_GROUP_FPREG, value);
    } else if ((operand->flags & PPC_OPERAND_RELATIVE) != 0) {
      ptr += snprintf(ptr, end - ptr, ".NEXTPC0x%lx", (long)value);
    } else if ((operand->flags & PPC_OPERAND_ABSOLUTE) != 0) {
      ptr += imm2str(value, tag, imm_mod, constant_val, constant_tag, sconstants,
          exvreg_group, nullptr, 0, I386_AS_MODE_32, ptr, end - ptr);
    } else if ((operand->flags & PPC_OPERAND_CR) == 0) {
      ptr += imm2str(value, tag, imm_mod, constant_val, constant_tag,
          sconstants, exvreg_group, nullptr, 0, I386_AS_MODE_32, ptr, end - ptr);
    } else if (operand->flags & PPC_OPERAND_CR) {
      ptr += crf_reg_snprintf(ptr, end - ptr, tag, value, /*exvreg_group, */operand);
    } else NOT_REACHED();
  } else NOT_REACHED();
  //printf("printed operand '%s', tag=%d, operand->flags=%x.\n", buf, tag,
  //operand->flags);
  return ptr - buf;
}

bool
ppc_insn_is_nop(ppc_insn_t const *insn)
{
  if (   insn->opc1 == 24
      && insn->valtag.rSD.rS.base.val == 0
      && insn->valtag.rSD.rS.base.tag == tag_const
      && insn->valtag.rA.val == 0
      && insn->valtag.rA.tag == tag_const
      && insn->valtag.imm.uimm.val == 0
      && insn->valtag.imm.uimm.tag == tag_const) {
    return true;
  }
  return false;
}

static char *
ppc_insn_contents_to_string(ppc_insn_t const *insn, char *buf, size_t size)
{
  char *ptr, *end;
  int i;

  ptr = buf;
  end = buf + size;

  ptr += snprintf(ptr, end - ptr, "\n");
  ptr += snprintf(ptr, end - ptr, "\topc1 %x, opc2 %x, opc3 %x r[SD] %lx[%d], "
      "rA %lx[%d], rB %lx[%d], crf0 %lx[%d] xer_ov0 %lx[%d] xer_so0 %lx[%d] "
      " xer_ca0 %lx[%d] FM %x simm %lx(%d)"
      ", LI %lx, AA %d, LK %d, crfS %ld[%d], crfD %ld[%d], crbA %ld[%d], "
      "crbB %ld[%d], crbD %ld[%d] "
      "BO %x, BI %ld[%d], BD %lx[%d], Rc %d, "
      //"NB %d[%d], SH %d[%d], MB %d[%d], ME %d[%d], "
      //"SPR %x, CRM %x fr[SD] %x[%d] reserved_mask %x"
      "\n",
      insn->opc1, insn->opc2, insn->opc3, insn->valtag.rSD.rD.base.val,
      insn->valtag.rSD.rD.base.tag,
      insn->valtag.rA.val, insn->valtag.rA.tag, insn->valtag.rB.val, insn->valtag.rB.tag,
      insn->valtag.crf0.val, insn->valtag.crf0.tag,
      insn->valtag.xer_ov0.val, insn->valtag.xer_ov0.tag,
      insn->valtag.xer_so0.val, insn->valtag.xer_so0.tag,
      insn->valtag.xer_ca0.val, insn->valtag.xer_ca0.tag,
      insn->FM,
      insn->valtag.imm.simm.val, insn->valtag.imm_signed,
      insn->valtag.LI.val, insn->AA,
      insn->LK, insn->valtag.crfS.val, insn->valtag.crfS.tag, insn->valtag.crfD.val,
      insn->valtag.crfD.tag,
      insn->valtag.crbA.val, insn->valtag.crbA.tag, insn->valtag.crbB.val,
      insn->valtag.crbB.tag, insn->valtag.crbD.val,
      insn->valtag.crbD.tag, insn->BO, insn->valtag.BI.val, insn->valtag.BI.tag,
      insn->valtag.BD.val,
      insn->valtag.BD.tag, insn->Rc/*, insn->val.NB, insn->tag.NB, insn->val.SH, insn->tag.SH,
      insn->val.MB, insn->tag.MB, insn->val.ME, insn->tag.ME, insn->SPR, insn->val.CRM,
      insn->val.frSD.frD, insn->tag.frSD.frD, insn->reserved_mask*/);
  /*for (i = 0; i < MAX_NUM_VCONSTANTS_PER_INSN; i++) {
    ptr += snprintf(ptr, end - ptr,
        "vconst[%d]=[mod %d sc 0x%" PRIx64 " sc2 0x%" PRIx64 " sc3 0x%" PRIx64 "\n", i,
        insn->imm_modifier[i], insn->sconstants[i][0], insn->sconstants[i][1],
        insn->sconstants[i][2]);
  }*/
  return buf;
}

/*
#define PPC_VT_TYPES_TO_STRING(pvt, field, buf, size) ({ \
  size_t __ret = 0; \
  if (pvt->field.tag != tag_invalid) { \
    __ret = vt_type_to_string(&pvt->field.type, buf, size); \
  } \
  __ret; \
})

static char *
ppc_insn_types_to_string(ppc_insn_t const *insn, char *buf, size_t size)
{
  ppc_insn_valtag_t const *pvt;
  char *ptr, *end;

  ptr = buf;
  end = buf + size;

  pvt = &insn->valtag;
  ptr += PPC_VT_TYPES_TO_STRING(pvt, rSD.rS, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, rA, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, rB, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, rB, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, imm.uimm, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, BD, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, LI, ptr, end - ptr);

  ptr += PPC_VT_TYPES_TO_STRING(pvt, crfS, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, crfD, ptr, end - ptr);

  ptr += PPC_VT_TYPES_TO_STRING(pvt, crbA, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, crbB, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, crbD, ptr, end - ptr);

  ptr += PPC_VT_TYPES_TO_STRING(pvt, BI, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, NB, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, SH, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, MB, ptr, end - ptr);
  ptr += PPC_VT_TYPES_TO_STRING(pvt, ME, ptr, end - ptr);

  ptr += PPC_VT_TYPES_TO_STRING(pvt, frSD.frS, ptr, end - ptr);

  return buf;
}

static void
ppc_insn_types_from_string(ppc_insn_t *insn, char const *buf)
{
  NOT_IMPLEMENTED();
}
*/

#define SNPRINT_IMPLICIT_OP(ptr, end, insn, field, fieldstr, group) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    if (insn->valtag.field.tag == tag_const) { \
      ptr += snprintf(ptr, end - ptr, ",%s", fieldstr); \
    } else if (insn->valtag.field.tag == tag_var) { \
      /*if (group == -1) { \
        ptr += snprintf(ptr, end - ptr, ",%%vr%ldd", insn->valtag.field.val); \
      } else */{ \
        ASSERT(group >= 0 && group < PPC_NUM_EXREG_GROUPS); \
        ptr += snprintf(ptr, end - ptr, ",%%exvr%d.%ld", group, \
            insn->valtag.field.val); \
      }\
    } else NOT_REACHED(); \
  } \
} while(0)

static char *
ppc_insn_with_index_to_string(ppc_insn_t const *insn, long insn_index,
    char *buf, size_t buf_size)
{
  /* Adapted from snprint_insn_powerpc (ppc/disas.c) */
  char *ptr = buf, *end = buf + buf_size;
  const struct powerpc_opcode *opcode_end;
  const struct powerpc_opcode *opcode;
  uint32_t bincode;
  uint32_t op;
  int i;

  if (ppc_insn_is_nop(insn)) {
    ptr += snprintf(ptr, end - ptr, "nop #\n");
    return buf;
  }
  /* Get the major opcode of the instruction.  */
  op = insn->opc1;

  ppc_insn_get_bincode(insn, (uint8_t *)&bincode);

  /* Find the first match in the opcode table.  We could speed this up
     a bit by doing a binary search on the major opcode.  */
  opcode_end = powerpc_opcodes + powerpc_num_opcodes;
  for (opcode = powerpc_opcodes; opcode < opcode_end; opcode++) {
    uint32_t table_op;
    const unsigned char *opindex;
    const struct powerpc_operand *operand;
    int invalid;
    int need_comma;
    int need_paren;
    int vconst_num;

    table_op = PPC_OP (opcode->opcode);
    if (op < table_op)
      break;
    if (op > table_op)
      continue;

    if (   (bincode & opcode->mask) != opcode->opcode
        || (opcode->flags & (PPC | B32 | M601)) == 0)
      continue;

#define RA_MASK (0x1f << 16)
    if ((opcode->mask & RA_MASK) == RA_MASK) { //sorav
      if (insn->valtag.rA.tag != tag_invalid) {
        /* because it passed the previous check
               (bincode & opcode->mask) == opcode->opcode
           the only possibility is that rA's val is zero. Also
           if opcode->mask shows that rA is not ignored, then it must
           be the special case where rA == 0, and we can reach here only
           if rA.tag is tag_var (tag_const is not set for rA == 0 case) */
        ASSERT(insn->valtag.rA.val == 0);
        ASSERT(insn->valtag.rA.tag == tag_var);
        continue;
      }
    }

    /* Make two passes over the operands.  First see if any of them
       have extraction functions, and, if they do, make sure the
       instruction is valid.  */
    invalid = 0;
    for (opindex = opcode->operands; *opindex != 0; opindex++) {
      operand = powerpc_operands + *opindex;
      if (operand->extract) {
        operand_extract_from_insn(operand, insn, bincode, &invalid, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, -1);
      }
    }
    if (invalid)
      continue;

    /* The instruction is valid.  */
    ptr += snprintf(ptr, end - ptr, "%s", opcode->name);
    if (opcode->operands[0] != 0) {
      ptr += snprintf(ptr, end - ptr, " ");
    }

    /* Now extract and print the operands.  */
    need_comma = 0;
    need_paren = 0;
    vconst_num = 0;
    for (opindex = opcode->operands; *opindex != 0; opindex++) {
      imm_modifier_t imm_modifier;
      uint64_t sconstants[IMM_MAX_NUM_SCONSTANTS];
      src_tag_t constant_tag;
      unsigned constant_val;
      int exvreg_group;
      int32_t value;
      src_tag_t tag;

      operand = powerpc_operands + *opindex;

      /* Operands that are marked FAKE are simply ignored.  We
         already made sure that the extract function considered
         the instruction to be valid.  */
      if ((operand->flags & PPC_OPERAND_FAKE) != 0)
        continue;

      DBG3(PPC_INSN, "checking operand number %d\n", (int)(opindex - opcode->operands));
      /* Extract the value from the instruction.  */
      operand_extract_from_insn(operand, insn, bincode, (int *) 0, &value, &tag,
          &imm_modifier, &constant_val, &constant_tag, sconstants, &exvreg_group,
          vconst_num);
      vconst_num++;

      /* If the operand is optional, and the value is zero, don't
         print anything.  */
      /* sorav if ((operand->flags & PPC_OPERAND_OPTIONAL) != 0
          && (operand->flags & PPC_OPERAND_NEXT) == 0
          && value == 0)
        continue; */

      if (need_comma) {
        ptr += snprintf(ptr, end - ptr, ",");
        need_comma = 0;
      }

      ptr += operand_snprintf(operand, ptr, end - ptr, value, tag, imm_modifier,
          constant_val, constant_tag, sconstants, exvreg_group, insn_index);
      if (need_paren) {
        ptr += snprintf(ptr, end - ptr, ")");
        need_paren = 0;
      }

      if ((operand->flags & PPC_OPERAND_PARENS) == 0)
        need_comma = 1;
      else
      {
        ptr += snprintf(ptr, end - ptr, "(");
        need_paren = 1;
      }
    }

    char regname[128];
    ptr += snprintf(ptr, end - ptr, " #");
    SNPRINT_IMPLICIT_OP(ptr, end, insn, crf0, "crf0", PPC_EXREG_GROUP_CRF);
    SNPRINT_IMPLICIT_OP(ptr, end, insn, i_crf0_so, "i_crf0_so", PPC_EXREG_GROUP_CRF_SO);
    snprintf(regname, sizeof regname, "i_crfS%ld_so", insn->valtag.i_crfS_so.val);
    SNPRINT_IMPLICIT_OP(ptr, end, insn, i_crfS_so, regname, PPC_EXREG_GROUP_CRF_SO);
    snprintf(regname, sizeof regname, "i_crfD%ld_so", insn->valtag.i_crfD_so.val);
    SNPRINT_IMPLICIT_OP(ptr, end, insn, i_crfD_so, regname, PPC_EXREG_GROUP_CRF_SO);
    for (i = 0; i < insn->valtag.num_i_crfs; i++) {
      snprintf(regname, sizeof regname, "i_crfs%s[%ld]",
          (insn->valtag.i_crfs[i].tag == tag_const)?"":"_var",
          insn->valtag.i_crfs[i].val);
      SNPRINT_IMPLICIT_OP(ptr, end, insn, i_crfs[i], regname, PPC_EXREG_GROUP_CRF);
    }
    for (i = 0; i < insn->valtag.num_i_crfs_so; i++) {
      snprintf(regname, sizeof regname, "i_crfs_so%s[%ld]",
          (insn->valtag.i_crfs_so[i].tag == tag_const)?"":"_var",
          insn->valtag.i_crfs_so[i].val);
      SNPRINT_IMPLICIT_OP(ptr, end, insn, i_crfs_so[i], regname, PPC_EXREG_GROUP_CRF_SO);
    }
    for (i = 0; i < insn->valtag.num_i_sc_regs; i++) {
      snprintf(regname, sizeof regname, "i_sc_regs%s[%ld]",
          (insn->valtag.i_sc_regs[i].tag == tag_const)?"":"_var",
          insn->valtag.i_sc_regs[i].val);
      SNPRINT_IMPLICIT_OP(ptr, end, insn, i_sc_regs[i], regname, PPC_EXREG_GROUP_GPRS);
    }

    SNPRINT_IMPLICIT_OP(ptr, end, insn, lr0, "lr", PPC_EXREG_GROUP_LR);
    SNPRINT_IMPLICIT_OP(ptr, end, insn, ctr0, "ctr", PPC_EXREG_GROUP_CTR);
    SNPRINT_IMPLICIT_OP(ptr, end, insn, xer_ov0, "xer_ov", PPC_EXREG_GROUP_XER_OV);
    SNPRINT_IMPLICIT_OP(ptr, end, insn, xer_so0, "xer_so", PPC_EXREG_GROUP_XER_SO);
    SNPRINT_IMPLICIT_OP(ptr, end, insn, xer_ca0, "xer_ca", PPC_EXREG_GROUP_XER_CA);
    SNPRINT_IMPLICIT_OP(ptr, end, insn, xer_bc_cmp0, "xer_bc_cmp", PPC_EXREG_GROUP_XER_BC_CMP);

    /*static char tmpbuf[40960];
    ptr += snprintf(ptr, end - ptr, " &&& %s",
        ppc_insn_types_to_string(insn, tmpbuf, sizeof tmpbuf));*/

    ptr += snprintf(ptr, end - ptr, "\n");

    /* We have found and printed an instruction; return.  */
    //DBG(TMP, "returning: %s.\n", buf);
    return buf;
  }

  /* We could not find a match.  */
  ptr += snprintf(ptr, end - ptr, ".long 0x%x\n", bincode);

  return buf;
}

char *
ppc_insn_to_string(ppc_insn_t const *insn, char *buf, size_t buf_size)
{
  return ppc_insn_with_index_to_string(insn, 0, buf, buf_size);
}

char *
ppc_iseq_contents_to_string(ppc_insn_t const *iseq, int iseq_len,
    char *buf, int buf_size)
{
  char *ptr = buf, *end = buf+buf_size;
  *ptr = '\0';

  int i;
  for (i = 0; i < iseq_len; i++) {
    ptr += snprintf(ptr, end - ptr, ".i%d: ", i);
    ppc_insn_contents_to_string(&iseq[i], ptr, end - ptr);
    ptr += strlen(ptr);
  }
  ptr += snprintf(ptr, end - ptr, ".i%d:\n", i);
  return buf;
}

char *
ppc_iseq_to_string(ppc_insn_t const *iseq, long iseq_len,
    char *buf, size_t buf_size)
{
  char *ptr = buf, *end = buf+buf_size;
  *ptr = '\0';

  long i;
  for (i = 0; i < iseq_len; i++) {
    ptr += snprintf(ptr, end - ptr, ".i%ld: ", i);
    ppc_insn_with_index_to_string(&iseq[i], i, ptr, end - ptr);
    ptr += strlen(ptr);
  }
  //ptr += snprintf(ptr, end - ptr, ".i%ld:\n", i);
  return buf;
}

void
ppc_insn_copy(ppc_insn_t *dst, ppc_insn_t const *src)
{
  memcpy(dst, src, sizeof(ppc_insn_t));
}

void
ppc_insn_init(ppc_insn_t *insn)
{
  int i;
  memset(insn, 0xff, sizeof(ppc_insn_t));  /* set everything to 0xff */
  memset(&insn->valtag, 0, sizeof(ppc_insn_valtag_t));
  insn->valtag.imm_signed = -1;
  insn->valtag.CRM = -1;
  insn->valtag.imm.uimm.val = 0;
  //insn->is_fp_insn = 0;
  //insn->invalid = 0;
  insn->reserved_mask = 0x0;
  insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = insn->valtag.frSD.frD.base.tag =
      insn->valtag.rB.tag = tag_invalid;
  /*for (i = 0; i < MAX_NUM_VCONSTANTS_PER_INSN; i++) {
    insn->imm_modifier[i] = IMM_UNMODIFIED;
  }*/
  insn->valtag.imm.uimm.tag = tag_invalid;
  insn->valtag.num_i_crfs = 0;
  insn->valtag.num_i_crfs_so = 0;
  insn->valtag.num_i_sc_regs = 0;

  //memvt_list_init(&insn->memtouch);
}

unsigned long
ppc_insn_hash_func(ppc_insn_t const *insn)
{
  return 97*insn->opc1 + 413*insn->opc2 + 95143*insn->opc3
      + 3319 * insn->LK + 19441*insn->Rc;
  /*
  unsigned long ret = 0;
  int nb = sizeof (ppc_insn_t);
  int i;
  for (i = 0; i < nb; i+=sizeof(int)) {
    if (nb - i >= sizeof(int)) {
      ret += *((int *)((char *)insn + i));
    } else {
      int j;
      for (j = 0; j < nb - i; j++) {
        ret += (((unsigned long)(*((char *)((char *)insn + i + j)))) << j);
      }
    }
  }
  return ret;
  */
}

#define PPC_INSN_RENAME_EXREG(field, group) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    if (regmap) { \
      /*ASSERT(PPC_INSN_CONST_OPERAND_IS_NON_NULL_REG(insn, field)); */\
      if (insn->valtag.field.tag == tag_var) { \
        ASSERT(   insn->valtag.field.val >= 0 \
               && insn->valtag.field .val< PPC_NUM_EXREGS(group)); \
        insn->valtag.field.tag = tag_const; \
        insn->valtag.field.val = regmap_get_elem(regmap, group, insn->valtag.field.val).reg_identifier_get_id(); \
        ASSERT(   insn->valtag.field.val >= 0 \
               && insn->valtag.field.val < PPC_NUM_EXREGS(group)); \
      /*} else if (   insn->valtag.field.tag == tag_const \
                 && !PPC_INSN_CONST_OPERAND_IS_NON_NULL_REG(insn, field)) { \
        ASSERT(insn->valtag.field.val == 0); \*/ \
      } else NOT_REACHED(); \
    } \
    /*vn++; */\
  } \
} while(0)

/*#define PPC_INSN_RENAME_REG(field) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    if (regmap) { \
      if (insn->valtag.field.tag == tag_var) { \
        ASSERT(insn->valtag.field.val >= 0 && insn->valtag.field.val < PPC_NUM_GPRS); \
        insn->valtag.field.tag = tag_const; \
        insn->valtag.field.val = regmap->table[insn->valtag.field.val]; \
      } else if (   insn->valtag.field.tag == tag_const \
                 && !PPC_INSN_CONST_OPERAND_IS_NON_NULL_REG(insn, field)) { \
        ASSERT(insn->valtag.field.val == 0); \
      } else NOT_REACHED(); \
    } \
  } \
} while(0)*/

#define PPC_INSN_RENAME_CREGBIT(field) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    if (regmap) { \
      if (insn->valtag.field.tag == tag_var) { \
        int renamed_crf, crf, bitnum; \
        crf = insn->valtag.field.val / 4; \
        ASSERT(crf >= 0 && crf < 8); \
        bitnum = insn->valtag.field.val % 4; \
        insn->valtag.field.tag = tag_const; \
        if (bitnum == PPC_INSN_CRF_BIT_SO) { \
          renamed_crf = regmap_get_elem(regmap, PPC_EXREG_GROUP_CRF_SO, crf).reg_identifier_get_id(); \
        } else { \
          renamed_crf = regmap_get_elem(regmap, PPC_EXREG_GROUP_CRF, crf).reg_identifier_get_id(); \
        } \
        ASSERT(renamed_crf >= 0 && renamed_crf < 8); \
        insn->valtag.field.val = renamed_crf * 4 + (insn->valtag.field.val % 4); \
      } else NOT_REACHED(); \
    } \
    /*vn++; */\
  } \
} while(0)

static bool
ppc_insn_bo_indicates_bi_use(ppc_insn_t const *insn)
{
  /* Table 3-5 from PowerPC Processor Reference Guide UG011 (v1.3)
     January 11, 2010
  0000y Decrement the CTR. Branch if the decremented CTR != 0 and CR[BI]=0.
  0001y Decrement the CTR. Branch if the decremented CTR = 0 and CR[BI]=0.
  001zy Branch if CR[BI]=0.
  0100y Decrement the CTR. Branch if the decremented CTR != 0 and CR[BI]=1.
  0101y Decrement the CTR. Branch if the decremented CTR=0 and CR[BI]=1.
  011zy Branch if CR[BI]=1.
  1z00y Decrement the CTR. Branch if the decremented CTR != 0.
  1z01y Decrement the CTR. Branch if the decremented CTR = 0.
  1z1zz Branch always.
  */
  if (insn->BO == -1) {
    return false;
  }
  ASSERT(insn->BO >= 0 && insn->BO <= 0x1f);
  if (insn->BO & 0x10) {
    return false;
  }
  return true;
}

bool
ppc_insn_bo_indicates_ctr_use(ppc_insn_t const *insn)
{
  if (insn->BO == -1) {
    return false;
  }
  ASSERT(insn->BO >= 0 && insn->BO <= 0x1f);
  if (insn->BO & 0x4) {
    return false;
  }
  return true;
}

static bool
ppc_insn_is_memory_access(ppc_insn_t const *insn)
{
  if (   insn->opc1 == 34  /* lbz */
      || insn->opc1 == 35  /* lbzu */
      || insn->opc1 == 32  /* lwz */
      || insn->opc1 == 33  /* lwzu */
      || insn->opc1 == 42  /* lha */
      || insn->opc1 == 43  /* lhau */
      || insn->opc1 == 40  /* lhz */
      || insn->opc1 == 41  /* lhzu */
      || insn->opc1 == 46  /* lmw */
      || insn->opc1 == 38  /* stb */
      || insn->opc1 == 39  /* stbu */
      || insn->opc1 == 36  /* stw */
      || insn->opc1 == 37  /* stwu */
      || insn->opc1 == 44  /* sth */
      || insn->opc1 == 45  /* sthu */
      || insn->opc1 == 47  /* stmw */
      || insn->opc1 == 48  /* lfs */
      || insn->opc1 == 49  /* lfsu */
      || insn->opc1 == 50  /* lfd */
      || insn->opc1 == 51  /* lfdu */
      || insn->opc1 == 52  /* stfs */
      || insn->opc1 == 53  /* stfsu */
      || insn->opc1 == 54  /* stfd */
      || insn->opc1 == 55  /* stfdu */
     ) {
    return true;
  }
  return false;

//XXX: unhandled
      //|| insn->opc2 == 31  /* lbzx */
}

static void
ppc_insn_convert_vconst_to_const(/*unsigned long vconst, src_tag_t vconst_tag,
    int vconst_exvreg_group, imm_modifier_t imm_modifier,
    uint64_t constant, int64_t const *sconstants,*/
    valtag_t *vt,
    imm_vt_map_t const *imm_map, src_ulong const *nextpcs, long num_nextpcs,
    /*src_ulong const *insn_pcs, long num_insns, */regmap_t const *regmap)
{
  //unsigned long ret;
  if (!imm_map) {
    //return (VCONSTANT_PLACEHOLDER & 0xffff);
    //return 0;
    return /*vconst*/; //cannot return 0, as for some insns, equal constants may
                   //make the insn a nop
                   //(which will interfere with usedef computation)
  }
  /*ret = (unsigned long)*/imm_vt_map_rename_vconstant(/*vconst, vconst_tag, imm_modifier,
      constant,
      sconstants, vconst_exvreg_group, */
      vt, imm_map, nextpcs, num_nextpcs,
      regmap);
  /*DBG3(PPC_INSN, "returning %lx for (vconst=%lx, vconst_tag=%d, modifier=%d, sc3=%lx, "
      "sc2=%lx, sc1=%lx, sc0=%lx)\n", ret, vconst,
      vconst_tag, imm_modifier, (long)sconstants[3], (long)sconstants[2],
      (long)sconstants[1], (long)sconstants[0]);*/
  DBG3(PPC_INSN, "imm_map=%s\n", imm_vt_map_to_string(imm_map, as1, sizeof as1));
  return /*ret*/;
}

#define PPC_INSN_RENAME_CONST(field) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    if (imm_map) { \
      if (insn->valtag.field.tag != tag_const) { \
        DBG3(PPC_INSN, "insn->tag." #field "=%d, imm_modifier=%d\n", \
            (int)insn->valtag.field.tag, (int)insn->valtag.field.mod.imm.modifier); \
        /*insn->valtag.field.val = */ppc_insn_convert_vconst_to_const(/*insn->valtag.field.val, \
            insn->valtag.field.tag, \
            insn->valtag.field.mod.imm.exvreg_group, \
            insn->valtag.field.mod.imm.modifier, \
            insn->valtag.field.mod.imm.constant, \
            insn->valtag.field.mod.imm.sconstants,*/ &insn->valtag.field, \
            imm_map, nextpcs, num_nextpcs, /*insn_pcs, num_insns, */regmap); \
        /*insn->valtag.field.tag = tag_const; */\
        if (imm_map && insn->valtag.field.val < 0) { \
          return false; \
        } \
      } \
    } \
    /*vn++; */\
  } \
} while(0)

static bool
ppc_insn_rename(ppc_insn_t *insn, regmap_t const *regmap, imm_vt_map_t const *imm_map,
    src_ulong const *nextpcs, long num_nextpcs/*, src_ulong const *insn_pcs,
    long num_insns*/)
{
  int i/*, vn = 0*/;
  DBG(PPC_INSN, "before, insn:%s\n", ppc_insn_to_string(insn, as1, sizeof as1));
  DBG(PPC_INSN, "regmap:%s\n", regmap_to_string(regmap, as1, sizeof as1));

  if (ppc_insn_is_nop(insn)) {
    return true;
  }

  PPC_INSN_RENAME_EXREG(rSD.rS.base, PPC_EXREG_GROUP_GPRS);
  PPC_INSN_RENAME_EXREG(frSD.frS.base, PPC_EXREG_GROUP_FPREG);

  if (ppc_insn_is_memory_access(insn)) {
    if (insn->valtag.imm_signed != -1) {
      PPC_INSN_RENAME_CONST(imm.uimm);
    }
  }
  PPC_INSN_RENAME_EXREG(rA, PPC_EXREG_GROUP_GPRS);
  PPC_INSN_RENAME_EXREG(rB, PPC_EXREG_GROUP_GPRS);

  PPC_INSN_RENAME_EXREG(crfS, PPC_EXREG_GROUP_CRF);
  PPC_INSN_RENAME_EXREG(crfD, PPC_EXREG_GROUP_CRF);

  PPC_INSN_RENAME_CREGBIT(crbA);
  PPC_INSN_RENAME_CREGBIT(crbB);
  PPC_INSN_RENAME_CREGBIT(crbD);

  /*if (insn->BO != -1) {
    vn++;
  }*/

  if (ppc_insn_bo_indicates_bi_use(insn)) {
    PPC_INSN_RENAME_CREGBIT(BI);
  } /*else if (insn->valtag.BI.tag != tag_invalid) {
    vn++;
  }*/
  DBG(PPC_INSN, "after, insn:%s\n", ppc_insn_to_string(insn, as1, sizeof as1));

  if (!ppc_insn_is_memory_access(insn)) {
    if (insn->valtag.imm_signed != -1) {
      PPC_INSN_RENAME_CONST(imm.uimm);
    }
  }

  PPC_INSN_RENAME_CONST(NB);
  PPC_INSN_RENAME_CONST(SH);
  PPC_INSN_RENAME_CONST(MB);
  PPC_INSN_RENAME_CONST(ME);

  PPC_INSN_RENAME_EXREG(crf0, PPC_EXREG_GROUP_CRF);
  PPC_INSN_RENAME_EXREG(i_crf0_so, PPC_EXREG_GROUP_CRF_SO);
  PPC_INSN_RENAME_EXREG(i_crfS_so, PPC_EXREG_GROUP_CRF_SO);
  PPC_INSN_RENAME_EXREG(i_crfD_so, PPC_EXREG_GROUP_CRF_SO);
  for (i = 0; i < insn->valtag.num_i_crfs; i++) {
    PPC_INSN_RENAME_EXREG(i_crfs[i], PPC_EXREG_GROUP_CRF);
  }
  for (i = 0; i < insn->valtag.num_i_crfs_so; i++) {
    PPC_INSN_RENAME_EXREG(i_crfs_so[i], PPC_EXREG_GROUP_CRF_SO);
  }
  for (i = 0; i < insn->valtag.num_i_sc_regs; i++) {
    PPC_INSN_RENAME_EXREG(i_sc_regs[i], PPC_EXREG_GROUP_GPRS);
  }
  PPC_INSN_RENAME_EXREG(lr0, PPC_EXREG_GROUP_LR);
  PPC_INSN_RENAME_EXREG(ctr0, PPC_EXREG_GROUP_CTR);
  PPC_INSN_RENAME_EXREG(xer_ov0, PPC_EXREG_GROUP_XER_OV);
  PPC_INSN_RENAME_EXREG(xer_so0, PPC_EXREG_GROUP_XER_SO);
  PPC_INSN_RENAME_EXREG(xer_ca0, PPC_EXREG_GROUP_XER_CA);
  PPC_INSN_RENAME_EXREG(xer_bc_cmp0, PPC_EXREG_GROUP_XER_BC_CMP);

  /*  bool ret;
  ppc_insn_rename_regs(insn, regmap);
  ret = ppc_insn_rename_constants(insn, NULL, NULL, 0, regmap);*/
  return true;
}

bool
ppc_insn_rename_constants(ppc_insn_t *insn, imm_vt_map_t const *imm_map,
    src_ulong const *nextpcs, long num_nextpcs, /*src_ulong const *insn_pcs,
    long num_insns, */regmap_t const *regmap)
{
  return ppc_insn_rename(insn, NULL, imm_map, nextpcs, num_nextpcs
      /*, insn_pcs, num_insns*/);
}

/*void
ppc_insn_get_usedef_regs(ppc_insn_t const *insn, regset_t *use,
    regset_t *def)
{
  memset_t memuse, memdef;
  memset_init(&memuse);
  memset_init(&memdef);
  ppc_insn_get_usedef(insn, use, def, &memuse, &memdef);
  memset_free(&memuse);
  memset_free(&memdef);
}*/

static bool
ppc_insn_is_sc(ppc_insn_t const *insn)
{
  if (insn->opc1 == 0x11) {
    return true;
  }
  return false;
}


/*
void
ppc_insn_get_usedef(ppc_insn_t const *insn, regset_t *use,
    regset_t *def, memset_t *memuse, memset_t *memdef)
{
  //XXX: need to fix this function
  ppc_insn_t insn_copy;
  uint8_t opcode[4];
  int ret, i;
  bool retb;

  ppc_insn_copy(&insn_copy, insn);
  //retb = ppc_insn_rename(&insn_copy, ppc_v2c_regmap());
  //ASSERT(retb);
  DBG2(PEEP_TAB, "insn:\n%s\n", ppc_insn_to_string(insn, as1, sizeof as1));
  DBG2(PEEP_TAB, "renamed insn:\n%s\n", ppc_insn_to_string(&insn_copy, as1, sizeof as1));
  DBG2(PEEP_TAB, "insn->BI=%ld\n", insn->valtag.BI.val);
  DBG2(PEEP_TAB, "BI=%ld\n", insn_copy.valtag.BI.val);
  ppc_insn_get_bincode(&insn_copy, opcode);
  ret = get_ppc_usedef(opcode, use, def, memuse, memdef);
  DBG2(PEEP_TAB, "use=%s, def=%s\n",
      regset_to_string(use, as1, sizeof as1),
      regset_to_string(def, as2, sizeof as2));
  //if (renamed) {
  //  regset_inv_rename(use, use, ppc_v2c_regmap());
  //  regset_inv_rename(def, def, ppc_v2c_regmap());
  //  DBG2(PEEP_TAB, "after rename, use=%s, def=%s\n",
  //      regset_to_string(use, as1, sizeof as1),
  //      regset_to_string(def, as2, sizeof as2));
  //  //memset_inv_rename(memuse, memuse, ppc_v2c_regmap());//XXX
  //  //memset_inv_rename(memdef, memdef, ppc_v2c_regmap());//XXX
  //}
  ASSERT (ret == 4);
  if (ppc_insn_is_sc(insn)) {
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, insn->valtag.i_sc_regs[0].val,
        MAKE_MASK(SRC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_SO, 0,
        MAKE_MASK(SRC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    for (i = 0; i < insn->valtag.num_i_sc_regs; i++) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, insn->valtag.i_sc_regs[i].val,
          MAKE_MASK(SRC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
  }
  if (ppc_insn_is_mtcrf(insn)) { //needed to ensure that correct vregs are marked
    regset_clear_exreg_group(use, PPC_EXREG_GROUP_CRF);
    regset_clear_exreg_group(use, PPC_EXREG_GROUP_CRF_SO);
    regset_clear_exreg_group(def, PPC_EXREG_GROUP_CRF);
    regset_clear_exreg_group(def, PPC_EXREG_GROUP_CRF_SO);
    for (i = 0; i < insn->valtag.num_i_crfs; i++) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF, insn->valtag.i_crfs[i].val,
          MAKE_MASK(SRC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
    }
    for (i = 0; i < insn->valtag.num_i_crfs_so; i++) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF_SO,
          insn->valtag.i_crfs_so[i].val,
          MAKE_MASK(SRC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
    }
  }
}

void
ppc_iseq_get_usedef(ppc_insn_t const *insns, int n_insns,
    regset_t *use, regset_t *def,
    memset_t *memuse, memset_t *memdef)
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

    ppc_insn_get_usedef(&insns[i], &iuse, &idef, &imemuse, &imemdef);
    DBG(SRC_USEDEF, "insns[%d]: %siuse: %s, idef: %s\n", i,
        src_insn_to_string(&insns[i], as1, sizeof as1),
        regset_to_string(&iuse, as2, sizeof as2),
        regset_to_string(&idef, as3, sizeof as3));

    regset_diff (use, &idef);
    regset_union (def, &idef);

    regset_union(use, &iuse);

    memset_union(memdef, &imemdef);

    memset_union(memuse, &imemuse);

    memset_free(&imemuse);
    memset_free(&imemdef);
  }
}

void
ppc_iseq_get_usedef_regs(ppc_insn_t const *insns, int n_insns,
    regset_t *use, regset_t *def)
{
  memset_t memuse, memdef;

  memset_init(&memuse);
  memset_init(&memdef);

  ppc_iseq_get_usedef(insns, n_insns, use, def, &memuse, &memdef);

  memset_free(&memuse);
  memset_free(&memdef);
}*/

/*
#define HASH_CANON_REG(regname) do {  \
  if (insn->regname != -1) { \
    if (++num_gpr_operands < PPC_NUM_GPRS) { \
      st_regmap.table[num_gpr_operands] = insn->regname; \
      insn->regname= num_gpr_operands; \
    } else { \
      ASSERT (num_gpr_operands == PPC_NUM_GPRS); \
      int i; \
      for (i = 0; i < num_gpr_operands; i++) { \
        if (st_regmap.table[i] == insn->regname) \
        insn->regname = i; \
      } \
    } \
  } \
} while (0)

#define HASH_CANON_CRF_REG(regname) do {  \
  if (insn->regname != -1) { \
    if (++num_crf_operands < 8) { \
      st_regmap.table[PPC_REG_CRFN(num_crf_operands)] = PPC_REG_CRFN(insn->regname); \
      insn->regname= num_crf_operands; \
    } else { \
      ASSERT (num_crf_operands == 8); \
      int i; \
      for (i = 0; i < num_crf_operands; i++) { \
        if (st_regmap.table[PPC_REG_CRFN(i)] == PPC_REG_CRFN(insn->regname)) \
        insn->regname = PPC_REG_CRFN(i); \
      } \
    } \
  } \
} while (0)

#define HASH_CANON_CRF_BIT(regname) do {  \
  if (insn->regname != -1) \
  { \
    int bitnum = insn->regname & 0x3; \
    if (++num_crf_operands < 8) \
    { \
      st_regmap.table[PPC_REG_CRFN(num_crf_operands)] = PPC_REG_CRFN(insn->regname>>2); \
      insn->regname = (PPC_REG_CRFN(num_crf_operands) << 2) | bitnum; \
    } else { \
      ASSERT (num_crf_operands == 8); \
      int i; \
      for (i = 0; i < num_crf_operands; i++) { \
        if (st_regmap.table[PPC_REG_CRFN(i)] == PPC_REG_CRFN(insn->regname>>2)) \
          insn->regname = (PPC_REG_CRFN(i) << 2) | bitnum; \
      } \
    } \
  } \
} while (0)


#define HASH_CANON_SHIMM(shfield) do {  \
    if (insn->shfield!= -1) { \
      if (st_sh_imm_map.num_sh_imms_used < NUM_SH_CANON_CONSTANTS) { \
        st_sh_imm_map.table[st_sh_imm_map.num_sh_imms_used] = \
        insn->shfield; \
        insn->shfield = sh_canon_constants[st_sh_imm_map.num_sh_imms_used]; \
        st_sh_imm_map.num_sh_imms_used++; \
      } else { \
        int j; \
        for (j = 0; j < st_sh_imm_map.num_sh_imms_used; j++) { \
          if (insn->shfield == st_sh_imm_map.table[j]) \
          insn->shfield = sh_canon_constants[j]; \
        } \
      } \
    } \
} while (0)


#define HASH_CANON_CRM() do { \
  if (insn->CRM != -1) { \
    if (st_imm_map.num_imms_used < NUM_CANON_CONSTANTS) { \
      st_imm_map.table[st_imm_map.num_imms_used] = \
         imm_apply_invmodifier(insn->CRM, IMM_SIGN_EXT8); \
      insn->CRM=imm_apply_modifier(canon_constants[st_imm_map.num_imms_used],\
          IMM_SIGN_EXT8);\
      st_imm_map.num_imms_used++; \
    } else { \
      int j; \
      for (j = 0; j < st_imm_map.num_imms_used; j++) { \
        if (insn->CRM == imm_apply_modifier(st_imm_map.table[j], IMM_SIGN_EXT8)) \
          insn->CRM = imm_apply_modifier(canon_constants[j], IMM_SIGN_EXT8); \
      } \
    } \
  } \
} while (0)

#define HASH_CANON_Rc() do { \
  if (insn->Rc == 1) { \
    if (st_regmap.table[PPC_REG_CRFN(0)] == -1) { \
      st_regmap.table[PPC_REG_CRFN(0)] = PPC_REG_CRFN(0); \
    } else if (st_regmap.table[PPC_REG_CRFN(0)] != PPC_REG_CRFN(0)) { \
      err ("st_regmap.table[%d]=%d\n", PPC_REG_CRFN(0), \
          st_regmap.table[PPC_REG_CRFN(0)]); \
      ASSERT(0); \
    } \
  } \
} while (0)
*/

void
ppc_insn_hash_canonicalize(ppc_insn_t *insn, regmap_t *regmap, imm_vt_map_t *imm_map)
{
  /* XXX : related to hash_func */
/*
  regmap_t st_regmap;
  imm_map_t st_imm_map;
  //sh_imm_map_t st_sh_imm_map;
  int i;
  int num_gpr_operands = 0;
  int num_crf_operands = 0;

  regmap_init (&st_regmap);
  imm_map_init (&st_imm_map);
  sh_imm_map_init (&st_sh_imm_map);

  for (i = 0; i < num_insns; i++) {
    ppc_insn_t *insn = &iseq[i];

    HASH_CANON_REG(rSD.rD);
    HASH_CANON_REG(rA);
    HASH_CANON_REG(rB);

    HASH_CANON_CRF_REG(crfS);
    HASH_CANON_CRF_REG(crfD);

    HASH_CANON_CRF_BIT(crbA);
    HASH_CANON_CRF_BIT(crbB);
    HASH_CANON_CRF_BIT(crbD);

    HASH_CANON_CRF_BIT(BI);

    if (insn->imm_signed != -1) {
      int32_t imm;
      if (insn->imm_signed == 0) {
        imm = insn->imm.uimm;
      } else {
        imm = insn->imm.simm;
      }

      if (st_imm_map.num_imms_used < NUM_CANON_CONSTANTS) {
        st_imm_map.table[st_imm_map.num_imms_used] = imm;
        insn->imm.uimm = canon_constants[st_imm_map.num_imms_used];
        st_imm_map.num_imms_used++;
      } else {
        int j;
        for (j = 0; j < st_imm_map.num_imms_used; j++) {
          if (insn->imm.uimm == st_imm_map.table[j])
            insn->imm.uimm = canon_constants[j];
        }
      }
    }

    HASH_CANON_CRM ();
    HASH_CANON_Rc ();

    HASH_CANON_SHIMM (SH);
    HASH_CANON_SHIMM (MB);
    HASH_CANON_SHIMM (ME);
  }
*/
}

static bool
can_match_field(ppc_insn_t const *t_insn, char const *field,
    int orig_val, int new_val)
{
  return true;
  /* if (   t_insn->opc1 == 0x1f
      && t_insn->opc2 == 0x17
      && (t_insn->opc3 == 0x06 || t_insn->opc3 == 0x0c || t_insn->opc3 == 0x04))
  {
    // stbx sthx stwx
    if (!strcmp(field, "rA")) {
      if (orig_val != new_val && (orig_val == 0 || new_val == 0)) {
        return false;
      }
    }
  }
  return true; */
}

#define MATCH_EXREG(regname, group) do { \
  DBG2(PEEP_TAB, "matching exreg %s, t_insn tag=%d val=%ld, insn tag=%d val=%ld\n", \
      #regname, t_insn->valtag.regname.tag, t_insn->valtag.regname.val, \
      insn->valtag.regname.tag, \
      insn->valtag.regname.val); \
  if (t_insn->valtag.regname.tag != tag_invalid) { \
    if (insn->valtag.regname.tag == tag_invalid) { \
      DBG(PEEP_TAB, "Register " #regname " tag mismatch at insn %d.\n", j); \
      return false; \
    } \
    /*ASSERT(insn->valtag.regname.tag == tag_const); */\
    ASSERT(t_insn->valtag.regname.tag == tag_var); \
    ASSERT(   t_insn->valtag.regname.val >= 0 \
           && t_insn->valtag.regname.val < PPC_NUM_EXREGS(group)); \
    if (regmap_get_elem(st_regmap, group, t_insn->valtag.regname.val).reg_identifier_is_unassigned()) { \
      regmap_set_elem(st_regmap, group, t_insn->valtag.regname.val, insn->valtag.regname.val); \
    } else if (regmap_get_elem(st_regmap, group, t_insn->valtag.regname.val).reg_identifier_get_id() != insn->valtag.regname.val) { \
      DBG2(PEEP_TAB, "Register " #regname " mismatch at insn %d : %ld <-> %ld.\n", j, \
          (long)regmap_get_elem(st_regmap, group, t_insn->valtag.regname.val).reg_identifier_get_id(), \
          insn->valtag.regname.val); \
      return false; \
    } \
  } else if (insn->valtag.regname.tag != tag_invalid) { \
    return false; \
  } \
} while (0)


/*#define MATCH_REG(regname) do { \
  DBG2(PEEP_TAB, "matching register %s, t_insn tag=%d val=%ld, insn tag=%d val=%ld\n", \
      #regname, t_insn->valtag.regname.tag, t_insn->valtag.regname.val, \
      insn->valtag.regname.tag, insn->valtag.regname.val); \
  if (t_insn->valtag.regname.tag != tag_invalid) { \
    if (insn->valtag.regname.tag == tag_invalid) { \
      DBG(PEEP_TAB, "Register " #regname " tag mismatch at insn %d.\n", j); \
      return false; \
    } \
    ASSERT(insn->valtag.regname.tag == tag_const); \
    if (t_insn->valtag.regname.tag == tag_var) { \
      if (!PPC_INSN_CONST_OPERAND_IS_NON_NULL_REG(insn, regname)) { \
        return false; \
      } \
      if (st_regmap->table[t_insn->valtag.regname.val] == -1) { \
        st_regmap->table[t_insn->valtag.regname.val] = insn->valtag.regname.val; \
      } else if (st_regmap->table[t_insn->valtag.regname.val] != \
            insn->valtag.regname.val) { \
        DBG2(PEEP_TAB, "Register " #regname " mismatch at insn %d : %ld <-> %ld.\n", j, \
            (long)st_regmap->table[t_insn->valtag.regname.val], \
            insn->valtag.regname.val); \
        return false; \
      } \
    } else if (t_insn->valtag.regname.tag == tag_const) { \
      ASSERT(!PPC_INSN_CONST_OPERAND_IS_NON_NULL_REG(t_insn, regname)); \
      if (insn->valtag.regname.val != t_insn->valtag.regname.val) { \
        return false; \
      } \
    } else NOT_REACHED(); \
  } else if (insn->valtag.regname.tag != tag_invalid) { \
    return false; \
  } \
} while (0)*/

/*#define MATCH_CRF_REG(regname, group) do { \
  if (t_insn->tag.regname != tag_invalid) { \
    ASSERT(insn->tag.regname == tag_const); \
    ASSERT(t_insn->tag.regname == tag_exvreg); \
    if (st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF][t_insn->val.regname] == -1) { \
      DBG2(PEEP_TAB, "Mapping crf%d-->crf%d\n", t_insn->val.regname, insn->val.regname);\
      st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF][t_insn->val.regname] = \
          insn->val.regname; \
    } else if (st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF][t_insn->val.regname] != \
                    insn->val.regname) { \
      DBG2(PEEP_TAB, "Register" #regname "mismatch at insn %d.\n", j); \
      return false; \
    } \
  } else { \
    ASSERT(insn->tag.regname == tag_invalid); \
  } \
} while (0)*/


/*#define MATCH_FPREG(regname) do { \
  if (t_insn->tag.regname != tag_invalid) { \
    ASSERT(insn->tag.regname != tag_invalid); \
    ASSERT(insn->tag.regname != tag_var); \
    ASSERT(t_insn->tag.regname == tag_var); \
    if (st_regmap->regmap_extable[PPC_EXREG_GROUP_FPREG][t_insn->val.regname] == -1) { \
      DBG2(PEEP_TAB, "Mapping fr%d-->fr%d\n", t_insn->val.regname, insn->val.regname);\
      st_regmap->regmap_extable[PPC_EXREG_GROUP_FPREG][t_insn->val.regname] = \
          insn->val.regname; \
    } else if (st_regmap->regmap_extable[PPC_EXREG_GROUP_FPREG][t_insn->val.regname] != \
                    insn->val.regname) { \
      DBG2(PEEP_TAB, "Register" #regname "mismatch at insn %d.\n", j); \
      return false; \
    } \
  } else { \
    ASSERT(insn->tag.regname == tag_invalid); \
  } \
} while (0)*/

#define MATCH_CRF_BIT(bitname) do { \
  DBG2(PEEP_TAB, "matching crf_bit %s, t_insn tag=%d val=%ld, insn tag=%d val=%ld\n", \
      #bitname, t_insn->valtag.bitname.tag, (long)t_insn->valtag.bitname.val, \
      insn->valtag.bitname.tag, \
      (long)insn->valtag.bitname.val); \
  if (t_insn->valtag.bitname.tag != tag_invalid) { \
    ASSERT(insn->valtag.bitname.tag != tag_invalid); \
    ASSERT(t_insn->valtag.bitname.tag == tag_var); \
    /*ASSERT(t_insn->tag.bitname == tag_exvreg); */\
    int t_bitpos = t_insn->valtag.bitname.val % 4; \
    int i_bitpos = insn->valtag.bitname.val % 4; \
    int _groupnum; \
    if (t_bitpos != i_bitpos) { \
      DBG2(PEEP_TAB, "%s", "Bit " #bitname " mismatch on last two bits.\n"); \
      return false; \
    } \
    if (t_bitpos == PPC_INSN_CRF_BIT_SO) { \
      _groupnum = PPC_EXREG_GROUP_CRF_SO; \
    } else { \
      _groupnum = PPC_EXREG_GROUP_CRF; \
    } \
    /*ASSERT(t_insn->exvreg_group.bitname == _groupnum); */\
    if (regmap_get_elem(st_regmap, _groupnum, t_insn->valtag.bitname.val >> 2).reg_identifier_is_unassigned()) { \
      DBG2(PEEP_TAB, "Mapping crf%ld-->crf%ld for bit " #bitname "\n", \
          t_insn->valtag.bitname.val >> 2, insn->valtag.bitname.val >> 2);\
      regmap_set_elem(st_regmap, _groupnum, t_insn->valtag.bitname.val >> 2, insn->valtag.bitname.val >> 2); \
    } else if (regmap_get_elem(st_regmap, _groupnum, t_insn->valtag.bitname.val >> 2).reg_identifier_get_id() != \
                   insn->valtag.bitname.val >> 2) { \
      DBG2(PEEP_TAB, "Bit " #bitname " mismatch at insn %d. previous map "\
          "dictates crf%ld-->crf%ld. This insn expects crf%ld-->crf%ld.\n", j, \
          t_insn->valtag.bitname.val >> 2, \
          (long)regmap_get_elem(st_regmap, _groupnum, t_insn->valtag.bitname.val >> 2).reg_identifier_get_id(), \
          t_insn->valtag.bitname.val >> 2, insn->valtag.bitname.val >> 2); \
      return false; \
    } \
  } else { \
    ASSERT(insn->valtag.bitname.tag == tag_invalid); \
  } \
} while (0)

#define MATCH_OPCODES(templ, insn)  \
    (   (templ->opc1 == insn->opc1) \
     && (templ->opc2 == insn->opc2) \
     && (templ->opc3 == insn->opc3))

#if 0
#define MATCH_CRF() do { \
  if (t_insn->opc1 == 0x1f && t_insn->opc2 == 0x13 && t_insn->opc3 == 0x0) { \
    /* mfcr */ \
    int i; \
    for (i = 0; i < PPC_NUM_CRFREGS; i++) { \
      ASSERT(   st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF][i] == i \
             || st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF][i] == -1); \
      ASSERT(   st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF_SO][i] == i \
             || st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF_SO][i] == -1); \
      st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF][i] = i; \
      st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF_SO][i] = i; \
    } \
  } else if (t_insn->opc1 == 0x1f && t_insn->opc2 == 0x10 && t_insn->opc3 == 0x4) { \
    /* mtcrf */ \
    int i; \
    for (i = 0; i < PPC_NUM_CRFREGS; i++) { \
      ASSERT(   st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF][i] == i \
             || st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF][i] == -1); \
      ASSERT(   st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF_SO][i] == i \
             || st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF_SO][i] == -1); \
      st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF][i] = i; \
      st_regmap->regmap_extable[PPC_EXREG_GROUP_CRF_SO][i] = i; \
    } \
  } \
} while (0)
#endif

#define MATCH_CRM() do { \
  if (t_insn->valtag.CRM != -1) { \
    ASSERT(insn->valtag.CRM != -1); \
    if (t_insn->valtag.CRM != insn->valtag.CRM) { \
      DBG2(PEEP_TAB, "CRM mismatch: %ld <-> %ld\n", (long)t_insn->valtag.CRM, \
          (long)insn->valtag.CRM); \
      return false; \
    } \
  } else { \
    ASSERT(insn->valtag.CRM == -1); \
  } \
} while (0)

#define MATCH_Rc() do { \
  DBG2(PEEP_TAB, "Match_Rc called. t_insn->Rc=%d\n", t_insn->Rc); \
  if (t_insn->Rc == 1) { \
    ASSERT(insn->Rc == 1); \
    if (regmap_get_elem(st_regmap, PPC_EXREG_GROUP_CRF, 0).reg_identifier_is_unassigned()) { \
      regmap_set_elem(st_regmap, PPC_EXREG_GROUP_CRF, 0, 0); \
    } else if (regmap_get_elem(st_regmap, PPC_EXREG_GROUP_CRF, 0).reg_identifier_get_id() != 0) { \
      DBG2(PEEP_TAB,"Rc bit set,but CRF0 already mapped to crf%d.returning 0\n",\
          regmap_get_elem(st_regmap, PPC_EXREG_GROUP_CRF, 0).reg_identifier_get_id()); \
      return false; \
    } \
    if (regmap_get_elem(st_regmap, PPC_EXREG_GROUP_CRF_SO, 0).reg_identifier_is_unassigned()) { \
      regmap_set_elem(st_regmap, PPC_EXREG_GROUP_CRF_SO, 0, 0); \
    } else if (regmap_get_elem(st_regmap, PPC_EXREG_GROUP_CRF_SO, 0).reg_identifier_get_id() != 0) { \
      DBG2(PEEP_TAB,"Rc bit set,but CRF0 already mapped to crf%d.returning 0\n",\
          regmap_get_elem(st_regmap, PPC_EXREG_GROUP_CRF_SO, 0).reg_identifier_get_id()); \
      return false; \
    } \
  } \
} while (0)

#define PPC_MATCH_IMM_FIELD(s, ts, fld, map) do { \
  if (ts->valtag.fld.tag != tag_invalid) { \
    ASSERT(s->valtag.fld.tag != tag_invalid); \
    ASSERT(s->valtag.fld.tag == tag_const); \
    MATCH_IMM_FIELD(s->valtag.fld, ts->valtag.fld, map); \
  } \
} while (0)

#define PPC_MATCH_IMM_VT_FIELD(s, ts, fld, vt_map) do { \
  if (ts->valtag.fld.tag != tag_invalid) { \
    ASSERT(s->valtag.fld.tag != tag_invalid); \
    MATCH_IMM_VT_FIELD(s->valtag.fld, ts->valtag.fld, vt_map); \
  } \
} while (0)

#define PPC_MATCH_NEXTPC_FIELD(s, ts, fld, nextpc_map, var) do { \
  MATCH_NEXTPC_FIELD(s, ts, fld, nextpc_map, var); \
} while(0)

#define PPC_MATCH_IMM(insn, t_insn, imm_map) do { \
  if (insn->valtag.imm_signed) { \
    DBG2(PEEP_TAB, "Matching simm %ld\n", (long)t_insn->valtag.imm.simm.tag); \
    PPC_MATCH_IMM_FIELD(insn, t_insn, imm.simm, imm_map); \
  } else { \
    DBG2(PEEP_TAB, "Matching uimm %lu\n", (long)t_insn->valtag.imm.uimm.tag); \
    PPC_MATCH_IMM_FIELD(insn, t_insn, imm.uimm, imm_map); \
  } \
} while(0)

#define PPC_MATCH_IMM_VT(insn, t_insn, imm_vt_map) do { \
  if (insn->valtag.imm_signed) { \
    DBG2(PEEP_TAB, "Matching simm %ld\n", (long)t_insn->valtag.imm.simm.tag); \
    PPC_MATCH_IMM_VT_FIELD(insn, t_insn, imm.simm, imm_vt_map); \
  } else { \
    DBG2(PEEP_TAB, "Matching uimm %lu\n", (long)t_insn->valtag.imm.uimm.tag); \
    PPC_MATCH_IMM_VT_FIELD(insn, t_insn, imm.uimm, imm_vt_map); \
  } \
} while(0)

/*static bool
ppc_insn_is_dcbi(ppc_insn_t const *insn)
{
  return (insn->opc1 == 31) && (insn->opc3 == 14) && (insn->opc2 == 22);
}*/

static bool
ppc_iseq_matches_template_on_regs(ppc_insn_t const *_template, ppc_insn_t const *iseq,
    long iseq_len, regmap_t *st_regmap)
{
  regmap_init(st_regmap);
  DBG(PEEP_TAB, "Comparing:\n%s\nwith\n%s\n",
      ppc_iseq_to_string(iseq, iseq_len, as1, sizeof as1),
      ppc_iseq_to_string(_template, iseq_len, as2, sizeof as2));

  int i, j;
  for (j = 0; j < iseq_len; j++) {
    ppc_insn_t const *t_insn = &_template[j];
    ppc_insn_t const *insn = &iseq[j];
    //int vconst_num = 0;

    DBG2(PEEP_TAB, "Comparing opcodes: 0x%x<->0x%x, 0x%x<->0x%x, 0x%x<->0x%x\n",
        t_insn->opc1, insn->opc1, t_insn->opc2, insn->opc2, t_insn->opc3, insn->opc3);
    if (ppc_insn_is_nop(t_insn)) {
      if (ppc_insn_is_nop(insn)) {
        continue;
      } else {
        DBG2(PEEP_TAB, "%s", "t_insn is nop, insn is not nop, returning false.\n");
        return false;
      }
    }
    if (!MATCH_OPCODES(t_insn, insn)) {
      DBG2(PEEP_TAB, "Opcode-match on instruction %d failed.\n", j);
      return false;
    }

    if (   t_insn->FM != insn->FM
        || t_insn->BO != insn->BO
        || t_insn->Rc != insn->Rc
        || t_insn->LK != insn->LK) {
      DBG2(PEEP_TAB, "Instruction Fields mismatch, FM %d<->%d, BO %d<->%d, "
          "Rc %d<->%d, LK %d<->%d.\n", t_insn->FM, insn->FM, t_insn->BO, insn->BO,
          t_insn->Rc, insn->Rc, t_insn->LK, insn->LK);
      return false;
    }

    MATCH_EXREG(rSD.rD.base, PPC_EXREG_GROUP_GPRS);
    MATCH_EXREG(frSD.frD.base, PPC_EXREG_GROUP_FPREG);

    MATCH_EXREG(rA, PPC_EXREG_GROUP_GPRS);
    MATCH_EXREG(rB, PPC_EXREG_GROUP_GPRS);

    MATCH_EXREG(crfS, PPC_EXREG_GROUP_CRF);
    MATCH_EXREG(crfD, PPC_EXREG_GROUP_CRF);

    MATCH_CRF_BIT(crbA);
    MATCH_CRF_BIT(crbB);
    MATCH_CRF_BIT(crbD);

    if (ppc_insn_bo_indicates_bi_use(insn)) {
      MATCH_CRF_BIT(BI);
    }

    //MATCH_CRF();
    //MATCH_Rc();

    MATCH_CRM();

    MATCH_EXREG(crf0, PPC_EXREG_GROUP_CRF);
    MATCH_EXREG(i_crf0_so, PPC_EXREG_GROUP_CRF_SO);
    MATCH_EXREG(i_crfS_so, PPC_EXREG_GROUP_CRF_SO);
    MATCH_EXREG(i_crfD_so, PPC_EXREG_GROUP_CRF_SO);

    if (t_insn->valtag.num_i_crfs != insn->valtag.num_i_crfs) {
      return false;
    }
    for (i = 0; i < insn->valtag.num_i_crfs; i++) {
      MATCH_EXREG(i_crfs[i], PPC_EXREG_GROUP_CRF);
    }
    if (t_insn->valtag.num_i_crfs_so != insn->valtag.num_i_crfs_so) {
      return false;
    }
    for (i = 0; i < insn->valtag.num_i_crfs_so; i++) {
      MATCH_EXREG(i_crfs_so[i], PPC_EXREG_GROUP_CRF_SO);
    }
    if (t_insn->valtag.num_i_sc_regs != insn->valtag.num_i_sc_regs) {
      return false;
    }
    for (i = 0; i < insn->valtag.num_i_sc_regs; i++) {
      MATCH_EXREG(i_sc_regs[i], PPC_EXREG_GROUP_GPRS);
    }

    MATCH_EXREG(lr0, PPC_EXREG_GROUP_LR);
    MATCH_EXREG(ctr0, PPC_EXREG_GROUP_CTR);
    MATCH_EXREG(xer_ov0, PPC_EXREG_GROUP_XER_OV);
    MATCH_EXREG(xer_so0, PPC_EXREG_GROUP_XER_SO);
    MATCH_EXREG(xer_ca0, PPC_EXREG_GROUP_XER_CA);
    MATCH_EXREG(xer_bc_cmp0, PPC_EXREG_GROUP_XER_BC_CMP);

    /*regset_t use, def;
    ppc_insn_get_usedef_regs(insn, &use, &def);
    regset_union(&use, &def);
    if (regset_belongs_ex(&use, PPC_EXREG_GROUP_LR, 0)) {
      regmap_mark_ex_id(st_regmap, PPC_EXREG_GROUP_LR, 0);
    }
    if (regset_belongs_ex(&use, PPC_EXREG_GROUP_CTR, 0)) {
      regmap_mark_ex_id(st_regmap, PPC_EXREG_GROUP_CTR, 0);
    }
    if (regset_belongs_ex(&use, PPC_EXREG_GROUP_XER_OV, 0)) {
      regmap_mark_ex_id(st_regmap, PPC_EXREG_GROUP_XER_OV, 0);
    }
    if (regset_belongs_ex(&use, PPC_EXREG_GROUP_XER_CA, 0)) {
      regmap_mark_ex_id(st_regmap, PPC_EXREG_GROUP_XER_CA, 0);
    }
    if (regset_belongs_ex(&use, PPC_EXREG_GROUP_XER_SO, 0)) {
      regmap_mark_ex_id(st_regmap, PPC_EXREG_GROUP_XER_SO, 0);
    }
    if (regset_belongs_ex(&use, PPC_EXREG_GROUP_XER_BC, 0)) {
      regmap_mark_ex_id(st_regmap, PPC_EXREG_GROUP_XER_BC, 0);
    }
    if (regset_belongs_ex(&use, PPC_EXREG_GROUP_XER_CMP, 0)) {
      regmap_mark_ex_id(st_regmap, PPC_EXREG_GROUP_XER_CMP, 0);
    }*/
  }

  return true;
}

bool
ppc_iseq_matches_template(ppc_insn_t const *_template, ppc_insn_t const *iseq,
    long iseq_len, regmap_t *st_regmap, imm_vt_map_t *imm_map)
{
  DBG(PEEP_TAB, "Comparing:\n%s\nwith\n%s\n",
      ppc_iseq_to_string(iseq, iseq_len, as1, sizeof as1),
      ppc_iseq_to_string(_template, iseq_len, as2, sizeof as2));

  if (!ppc_iseq_matches_template_on_regs(_template, iseq, iseq_len, st_regmap)) {
    return false;
  }
  imm_vt_map_init(imm_map);

  int j;
  for (j = 0; j < iseq_len; j++) {
    ppc_insn_t const *t_insn = &_template[j];
    ppc_insn_t const *insn = &iseq[j];
    //int vconst_num = 0;

    DBG2(PEEP_TAB, "Comparing opcodes: 0x%x<->0x%x, 0x%x<->0x%x, 0x%x<->0x%x\n",
        t_insn->opc1, insn->opc1, t_insn->opc2, insn->opc2, t_insn->opc3, insn->opc3);
    if (ppc_insn_is_nop(t_insn)) {
      ASSERT(ppc_insn_is_nop(insn));
      continue;
    }
    ASSERT(MATCH_OPCODES(t_insn, insn));

    PPC_MATCH_IMM(insn, t_insn, imm_map);

    PPC_MATCH_IMM_FIELD(insn, t_insn, SH, imm_map);
    PPC_MATCH_IMM_FIELD(insn, t_insn, MB, imm_map);
    PPC_MATCH_IMM_FIELD(insn, t_insn, ME, imm_map);
  }
  return true;
}

bool
ppc_iseq_matches_template_var(ppc_insn_t const *_template,
    ppc_insn_t const *iseq, long iseq_len, struct regmap_t *st_regmap,
    struct imm_vt_map_t *imm_vt_map, nextpc_map_t *nextpc_map)
{
  DBG(PEEP_TAB, "Comparing:\n%s\nwith\n%s\n",
      ppc_iseq_to_string(iseq, iseq_len, as1, sizeof as1),
      ppc_iseq_to_string(_template, iseq_len, as2, sizeof as2));

  if (!ppc_iseq_matches_template_on_regs(_template, iseq, iseq_len, st_regmap)) {
    return false;
  }
  imm_vt_map_init(imm_vt_map);
  nextpc_map_init(nextpc_map);

  int j;
  for (j = 0; j < iseq_len; j++) {
    ppc_insn_t const *t_insn = &_template[j];
    ppc_insn_t const *insn = &iseq[j];
    //int vconst_num = 0;

    DBG2(PEEP_TAB, "Comparing opcodes: 0x%x<->0x%x, 0x%x<->0x%x, 0x%x<->0x%x\n",
        t_insn->opc1, insn->opc1, t_insn->opc2, insn->opc2, t_insn->opc3, insn->opc3);
    if (ppc_insn_is_nop(t_insn)) {
      ASSERT(ppc_insn_is_nop(insn));
      continue;
    }
    ASSERT(MATCH_OPCODES(t_insn, insn));

    PPC_MATCH_IMM_VT(insn, t_insn, imm_vt_map);

    PPC_MATCH_IMM_VT_FIELD(insn, t_insn, SH, imm_vt_map);
    PPC_MATCH_IMM_VT_FIELD(insn, t_insn, MB, imm_vt_map);
    PPC_MATCH_IMM_VT_FIELD(insn, t_insn, ME, imm_vt_map);

    PPC_MATCH_NEXTPC_FIELD(insn, t_insn, BD, nextpc_map, true);
    PPC_MATCH_NEXTPC_FIELD(insn, t_insn, LI, nextpc_map, true);
  }
  return true;
}

long 
ppc_iseq_canonicalize(ppc_insn_t *iseq, long iseq_len)
{
#if ARCH_SRC == ARCH_PPC
  return src_iseq_eliminate_nops(iseq, iseq_len);
#else
  NOT_IMPLEMENTED();
#endif
}

bool
ppc_insn_is_unconditional_indirect_branch(ppc_insn_t const *insn)
{
  if (ppc_insn_is_conditional_branch(insn)) {
    return false;
  }
  if (   insn->opc1 == 19
      && insn->opc2 == 0x10
      && insn->opc3 == 0x10) {
    /* blr */
    return true;
  }
  if (   insn->opc1 == 19
      && insn->opc2 == 0x10
      && insn->opc3 == 0) {
    /* bctr */
    return true;
  }
  return false;
}

bool
ppc_insn_is_unconditional_branch(ppc_insn_t const *insn)
{
  return    ppc_insn_is_unconditional_direct_branch(insn)
         || ppc_insn_is_unconditional_indirect_branch(insn);
}

bool
ppc_insn_is_unconditional_branch_not_fcall(ppc_insn_t const *insn)
{
  return    ppc_insn_is_unconditional_branch(insn)
         && !ppc_insn_is_function_call(insn);
}

bool
ppc_insn_terminates_bbl(ppc_insn_t const *insn)
{
  return ppc_insn_is_branch_not_fcall(insn);
}

bool
ppc_insn_is_unconditional_direct_branch(ppc_insn_t const *insn)
{
  if (insn->opc1 == 0x12) {
    return true;
  }
  if (insn->opc1 == 0x10 && !ppc_insn_is_conditional_branch(insn)) {
    return true;
  }
  return false;
}

bool
ppc_insn_is_unconditional_direct_branch_not_fcall(ppc_insn_t const *insn)
{
  return    ppc_insn_is_unconditional_direct_branch(insn)
         && !ppc_insn_is_function_call(insn);
}

bool
ppc_insn_is_unconditional_indirect_branch_not_fcall(ppc_insn_t const *insn)
{
  return ppc_insn_is_indirect_branch_not_fcall(insn);
}

bool
ppc_insn_is_conditional_branch(ppc_insn_t const *insn)
{
  if ((insn->BO & 0x14) == 0x14) {
    return false;
  }
  return true;
}

bool
ppc_insn_is_conditional_direct_branch(ppc_insn_t const *insn)
{
  if (insn->opc1 == 0x10 && ppc_insn_is_conditional_branch(insn)) {
    return true;
  }
  return false;
}

bool
ppc_insn_is_conditional_indirect_branch(ppc_insn_t const *insn)
{
  if (ppc_insn_is_indirect_branch(insn) && ppc_insn_is_conditional_branch(insn)) {
    return true;
  }
  return false;
}

bool
ppc_insn_is_function_call(ppc_insn_t const *insn)
{
  if (insn->LK != 1) {
    return false;
  }
  if (insn->opc1 == 0x12) {  //b
    if (insn->valtag.LI.tag == tag_const && insn->valtag.LI.val == 4) {
      return false;
    }
    return true;
  }
  if (insn->opc1 == 0x10) { //bc
    if (insn->valtag.BD.tag == tag_const && insn->valtag.BD.val == 4) {
      return false;
    }
    return true;
  }
  if (ppc_insn_is_indirect_branch(insn)) {
    return true;
  }
  NOT_REACHED();
  return false;
}

bool
ppc_insn_is_direct_function_call(ppc_insn_t const *insn)
{
  if (insn->LK != 1) {
    return false;
  }
  if (insn->valtag.LI.tag == tag_invalid && insn->valtag.BD.tag == tag_invalid) {
    return false;
  }
  return true;
}

static void
ppc_insn_set_branch_target(ppc_insn_t *insn, src_ulong target_val, src_tag_t target_tag)
{
  src_ulong target;
  int32_t li = -1;

  if (insn->opc1 == 0x12) {  /* b */
    ASSERT(insn->valtag.LI.tag != tag_invalid);
    insn->valtag.LI.val = target_val;
    insn->valtag.LI.tag = target_tag;
  } else if (insn->opc1 == 0x10) { /* bc */
    ASSERT(insn->valtag.BD.tag != tag_invalid);
    insn->valtag.BD.val = target_val;
    insn->valtag.BD.tag = target_tag;
  } else NOT_REACHED();
}

static src_ulong
__ppc_insn_branch_target(ppc_insn_t const *insn, src_ulong nip, src_tag_t tag)
{
  src_ulong target;

  target = (src_ulong)-1;
  if (insn->opc1 == 0x12) {  /* b */
    ASSERT(insn->AA == 0 || insn->AA == 1);
    ASSERT(insn->valtag.LI.tag != tag_invalid);
    if (insn->valtag.LI.tag == tag) {
      target = SIGN_EXT(insn->valtag.LI.val, 24);
      if (insn->AA == 0) {
        target += nip - 4;
      }
    }
  } else if (insn->opc1 == 0x10) { /* bc */
    ASSERT(insn->AA == 0 || insn->AA == 1);
    ASSERT(insn->valtag.BD.tag != tag_invalid);
    if (insn->valtag.BD.tag == tag) {
      target = SIGN_EXT(insn->valtag.BD.val, 16);
      if (insn->AA == 0) {
        target += nip - 4;
      }
    }
  } else NOT_REACHED();

  DBG(PPC_INSN, "returning %lx for %lx\n", (long)target, (long)nip);
  return target;
}

vector<src_ulong>
ppc_insn_branch_targets(ppc_insn_t const *insn, src_ulong nip)
{
  vector<src_ulong> ret;
  src_ulong target0 = __ppc_insn_branch_target(insn, nip, tag_const);
  if (target0 != (src_ulong)-1) {
    ret.push_back(target0);
  }
  if (!ppc_insn_is_unconditional_branch_not_fcall(insn)) {
    ret.push_back(nip);
  }
  return ret;
}

static src_ulong
ppc_insn_branch_target(ppc_insn_t const *insn, src_ulong nip)
{
  vector<src_ulong> targets = ppc_insn_branch_targets(insn, nip);
  if (targets.size() == 0) {
    return (src_ulong)-1;
  }
  ASSERT(targets.size() >= 1);
  return *targets.begin();
}

src_ulong
ppc_insn_fcall_target(ppc_insn_t const *insn, src_ulong nip, struct input_exec_t const *in)
{
  return ppc_insn_branch_target(insn, nip);
}

src_ulong
ppc_insn_branch_target_var(ppc_insn_t const *insn)
{
  return __ppc_insn_branch_target(insn, 4, tag_var);
}

bool
ppc_insn_is_indirect_function_call(ppc_insn_t const *insn)
{
  return    ppc_insn_is_indirect_branch(insn)
         && ppc_insn_is_function_call(insn);
}

bool
ppc_insn_is_indirect_branch(ppc_insn_t const *insn)
{
  /*
  printf("insn->opc1=%x, insn->opc2=%x, insn->opc3=%x, insn->rB=%x\n", insn->opc1,
      insn->opc2, insn->opc3, insn->rB);
      */
  if (insn->opc1 == 0x13 && insn->opc2 == 0x10 /*&& insn->opc3 == 0x19*/) /* bcctr */
    return true;
  if (insn->opc1 == 0x13 && insn->opc2 == 0x10 && insn->opc3 == 0x0) /* bclr */
    return true;
  return false;
}

bool
ppc_insn_is_indirect_branch_not_fcall(ppc_insn_t const *insn)
{
  return    ppc_insn_is_indirect_branch(insn)
         && !ppc_insn_is_function_call(insn);
}

bool
ppc_insn_is_branch(ppc_insn_t const *insn)
{
  if (insn->opc1 == 0x10) { /* bc */
    return true;
  }
  if (insn->opc1 == 0x12) { /* b */
    return true;
  }
  if (ppc_insn_is_indirect_branch(insn)) {
    return true;
  }
  return false;
}

bool
ppc_insn_is_branch_not_fcall(ppc_insn_t const *insn)
{
  if (ppc_insn_is_function_call(insn)) {
    return false;
  }
  if (insn->opc1 == 0x10) { /* bc */
    return true;
  }
  if (insn->opc1 == 0x12) { /* b */
    return true;
  }
  if (ppc_insn_is_indirect_branch(insn)) {
    return true;
  }
  return false;
}

#if 0
int ppc_insn_is_indirect_branch (ppc_insn_t const *insn)
{
  if (insn->opc1 == 0x13 && insn->opc2 == 0x10 && insn->opc3 == 0x10)/* bcctr */
    return 1;
  if (insn->opc1 == 0x13 && insn->opc2 == 0x10 && insn->opc3 == 0x0)/* bclr */
    return 1;
  if (insn->opc1 == 19)  /* bcctr */
    return 1;

  return 0;
}
#endif

bool
ppc_insn_is_function_return(ppc_insn_t const *insn)
{
  if (insn->opc1 == 0x13 && insn->opc2 == 0x10 && insn->opc3 == 0x0) { /* bclr */
    return true;
  }
  return false;
}

bool
ppc_insn_is_crf_logical_op(ppc_insn_t const *insn)
{
  if (insn->opc1 == 0x13 && insn->opc2 == 0x1) {
    /*crand*/
    if (insn->opc3 == 0x8) {
      return true;
    }
    /*crandc*/
    if (insn->opc3 == 0x4) {
      return true;
    }
    /*creqv*/
    if (insn->opc3 == 0x9) {
      return true;
    }
    /*crnand*/
    if (insn->opc3 == 0x7) {
      return true;
    }
    /*crnor*/
    if (insn->opc3 == 0x1) {
      return true;
    }
    /*cror*/
    if (insn->opc3 == 0xE) {
      return true;
    }
    /*crorc*/
    if (insn->opc3 == 0xD) {
      return true;
    }
    /*crxor*/
    if (insn->opc3 == 0x6) {
      return true;
    }
  }

  /* mcrf */
  if (insn->opc1 == 0x13 && insn->opc2 == 0x0 && insn->opc3 == -1) {
    return true;
  }
  /* mcrfs */
  if (insn->opc1 == 0x3f && insn->opc2 == 0x0 && insn->opc3 == 0x02) {
    return true;
  }
  /* mtcrf */
  if (insn->opc1 == 0x1f && insn->opc2 == 0x10 && insn->opc3 == 0x04) {
    return true;
  }
  return false;
}

bool
ppc_insn_is_mfcr(ppc_insn_t const *insn)
{
  /*mcrf*/
  if (insn->opc1 == 0x1f && insn->opc2 == 0x13 && insn->opc3 == 0) {
    return true;
  }
  return false;
}

bool
ppc_insn_is_mtcrf(ppc_insn_t const *insn)
{
  /*mtcrf*/
  if (insn->opc1 == 0x1f && insn->opc2 == 0x10 && insn->opc3 == 0x04) {
    return true;
  }
  return false;
}

bool
ppc_insn_fetch_raw(ppc_insn_t *insn, input_exec_t const *in, src_ulong pc, unsigned long *size)
{
  uint32_t opcode = ldl_input(in, pc);
  int disas;
  ppc_insn_init(insn);
  disas = ppc_insn_disassemble(insn, (uint8_t *)&opcode, I386_AS_MODE_32);
  //printf("%s() %d: pc = %lx, opcode = %x, disas = %d\n", __func__, __LINE__, (long)pc, opcode, disas);
  if (size) {
    *size = 4;
  }
  return disas != 0;
}

int
ppc_insn_assemble(ppc_insn_t const *insn, uint8_t *buf, size_t size)
{
  uint8_t bincode[4];
  ppc_insn_get_bincode(insn, bincode);
  uint32_t opcode = *(uint32_t *)bincode;

  stl(buf, opcode);
  return 4;
}

/*static bool
ppc_insn_from_string(ppc_insn_t *insn, char *line)
{
  uint8_t bincode[4];
  ssize_t as;

  as = ppc_assemble(bincode, line);
  ASSERT(as > 0);

  DBG2(PEEP_TAB, "bincode for %s = %s\n",
      line, hex_string(bincode, 4, as1, sizeof as1));

  int disas;
  disas = ppc_insn_disassemble(insn, bincode);
  if (disas) {
    DBG2(SRC_INSN, "Printing disassembled ppc_insn_t (opcode %s):\n%s",
        hex_string(bincode, 4, as1, sizeof as1),
        ppc_insn_to_string(insn, as2, sizeof as2));
    return true;
  }
  return false;
}*/

static int
ppc_assign_unused_reg(long vregnum, regvar_t rvt, char const *suffix,
    bool *used, long num_regs)
{
  ASSERT(rvt == REGVAR_INPUT);
  if (vregnum >= 0 && vregnum < PPC_NUM_GPRS) {
    ASSERT(vregnum != PPC_NUM_GPRS - 1);
    used[vregnum + 1] = true;
    return vregnum + 1;
  }
  NOT_REACHED();
}

/*
static void
ppc_regname_suffix(long reg, char const *suffix, char *name, size_t name_size)
{
  ASSERT(!strcmp(suffix, "d"));
  if (reg >= 0 && reg < PPC_NUM_GPRS) {
    snprintf(name, name_size, "%ld", reg);
    return;
  }
  NOT_REACHED();
}
*/

char * 
ppc_exregname_suffix(int groupnum, int exregnum, char const *suffix, char *buf, size_t size)
{
  char *ptr, *end;

  ptr = buf;
  end = buf + size;
  switch (groupnum) {
    case PPC_EXREG_GROUP_GPRS:
      ptr += snprintf(ptr, end - ptr, "%d", exregnum);
      return buf;
    case PPC_EXREG_GROUP_FPREG:
      ptr += snprintf(ptr, end - ptr, "%d", exregnum);
      return buf;
    case PPC_EXREG_GROUP_CRF:
      ptr += snprintf(ptr, end - ptr, "cr%d", exregnum);
      return buf;
    case PPC_EXREG_GROUP_CRF_SO:
      ptr += snprintf(ptr, end - ptr, "cr_so%d", exregnum);
      return buf;
    case PPC_EXREG_GROUP_LR:
      ASSERT(exregnum == 0);
      ptr += snprintf(ptr, end - ptr, "lr");
      return buf;
    case PPC_EXREG_GROUP_CTR:
      ASSERT(exregnum == 0);
      ptr += snprintf(ptr, end - ptr, "ctr");
      return buf;
    case PPC_EXREG_GROUP_XER_OV:
      ASSERT(exregnum == 0);
      ptr += snprintf(ptr, end - ptr, "xer_ov");
      return buf;
    case PPC_EXREG_GROUP_XER_SO:
      ASSERT(exregnum == 0);
      ptr += snprintf(ptr, end - ptr, "xer_so");
      return buf;
    case PPC_EXREG_GROUP_XER_CA:
      ASSERT(exregnum == 0);
      ptr += snprintf(ptr, end - ptr, "xer_ca");
      return buf;
    case PPC_EXREG_GROUP_XER_BC_CMP:
      ASSERT(exregnum == 0);
      ptr += snprintf(ptr, end - ptr, "xer_bc_cmp");
      return buf;
    /*case PPC_EXREG_GROUP_XER_CMP:
      ASSERT(exregnum == 0);
      ptr += snprintf(ptr, end - ptr, "xer_cmp");
      return;*/
  }
  NOT_REACHED();
}

/*
static void
ppc_peep_preprocess(char *line, regmap_t const *regmap, vconstants_t *vconstants,
    long num_vconstants)
{
  bool ret;
  DBG2(PEEP_TAB, "before preprocessing:\n%s\n", line);
  //peep_preprocess_vregs(line, regmap, &ppc_regname_suffix);

  ret = peep_preprocess_exvregs(line, regmap, &ppc_exregname);
  ASSERT(ret);

  peep_preprocess_all_constants(NULL, line, vconstants, num_vconstants);
  DBG(PEEP_TAB, "after preprocessing, vc=%p:\n%s\n", vconstants, line);
}
*/

/*#define PPC_INSN_INV_RENAME_REG(field) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    ASSERT(insn->valtag.field.tag == tag_const); \
    ASSERT(insn->valtag.field.val >= 0 && insn->valtag.field.val < PPC_NUM_GPRS); \
    ASSERT(insn->valtag.field.val >= 0); \
    if (PPC_INSN_CONST_OPERAND_IS_NON_NULL_REG(insn, field)) { \
      long ret; \
      ret = regmap_inv_rename_reg(regmap, insn->valtag.field.val, \
          4); \
      DBG2(PEEP_TAB, "Inv-renaming reg %s %ld->%ld\n", #field, insn->valtag.field.val, \
          ret); \
      insn->valtag.field.val = ret; \
      insn->valtag.field.tag = tag_var; \
    } \
    vc++; \
  } \
} while(0)*/

#define PPC_INSN_INV_RENAME_EXREG(field, group) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    ASSERT(insn->valtag.field.tag == tag_const); \
    /*ASSERT(PPC_INSN_CONST_OPERAND_IS_NON_NULL_REG(insn, field)); */{ \
      long ret; \
      ret = regmap_inv_rename_exreg(regmap, \
          group, insn->valtag.field.val).reg_identifier_get_id(); \
      ASSERT(ret >= 0); \
      DBG2(PEEP_TAB, "Inv-renaming reg %s %ld->%ld\n", #field, insn->valtag.field.val, \
          ret); \
      insn->valtag.field.val = ret; \
      insn->valtag.field.tag = tag_var; \
    } \
    vc++; \
  } else if (PPC_INSN_CONST_OPERAND_IS_NULL_REG(insn, field)) { \
    vc++; \
  } \
} while(0)


/*
#define PPC_INSN_INV_RENAME_CRF(field) do { \
  if (insn->tag.field != tag_invalid) { \
    ASSERT(insn->tag.field == tag_const); \
    ASSERT(insn->val.field >= 0 && insn->val.field < PPC_NUM_CRFREGS); \
    insn->tag.field = tag_var; \
    DBG2(PEEP_TAB, "Inv-renaming crf %s %d(const)->%d(var)\n", #field, insn->val.field, \
        insn->val.field); \
    vc++; \
  } \
} while(0)

#define PPC_INSN_INV_RENAME_FPREG(field) do { \
  if (insn->tag.field != tag_invalid) { \
    ASSERT(insn->tag.field == tag_const); \
    ASSERT(insn->val.field >= 0 && insn->val.field < PPC_NUM_FPREGS); \
    insn->tag.field = tag_var; \
    DBG2(PEEP_TAB, "Inv-renaming fpreg %s %d(const)->%d(var)\n", #field, insn->val.field, \
        insn->val.field); \
    vc++; \
  } \
} while(0)
*/

#define PPC_INSN_INV_RENAME_CRF_BIT(field) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    ASSERT(insn->valtag.field.tag == tag_const); \
    ASSERT(insn->valtag.field.val >= 0 && insn->valtag.field.val < 32); \
    long ret, crf, bit, _groupnum; \
    crf = insn->valtag.field.val / 4; \
    bit = insn->valtag.field.val % 4; \
    if (bit == PPC_INSN_CRF_BIT_SO) { \
      _groupnum = PPC_EXREG_GROUP_CRF_SO; \
    } else { \
      _groupnum = PPC_EXREG_GROUP_CRF; \
    } \
    ret = regmap_inv_rename_exreg(regmap, \
        _groupnum, crf).reg_identifier_get_id(); \
    ASSERT(ret >= 0); \
    DBG2(PEEP_TAB, "Inv-renaming crf bit %s %ld(const)->%ld(var)\n", #field, \
        insn->valtag.field.val, ret * 4 + bit); \
    insn->valtag.field.val = ret * 4 + bit; \
    insn->valtag.field.tag = tag_var; \
    vc++; \
  } \
} while(0)

/*#define PPC_INSN_INV_RENAME_CRF_BIT_NO_VC_INC(field) do { \
  if (insn->tag.field != tag_invalid) { \
    ASSERT(insn->tag.field == tag_const); \
    ASSERT(insn->val.field >= 0 && insn->val.field < 32); \
    insn->tag.field = tag_var; \
    DBG2(PEEP_TAB, "Inv-renaming crf bit %s %d(const)->%d(var)\n", #field, \
        insn->val.field, insn->val.field); \
  } \
} while(0) */

#define PPC_INSN_INV_RENAME_VCONST(field) do { \
  /*if ((insn->val.field & 0xff) == (VCONSTANT_PLACEHOLDER & 0xff))*/ { \
    if (vc->valid) {\
      int vconst_num = vc - vconstants; \
      int i; \
      DBG2(PEEP_TAB, "inv-renaming vconst field %s\n", #field); \
      ASSERT(insn->valtag.field.val == vc->placeholder); \
      insn->valtag.field.val = vc->val; \
      insn->valtag.field.tag = vc->tag; \
      insn->valtag.field.mod.imm.exvreg_group = vc->exvreg_group; \
      insn->valtag.field.mod.imm.modifier = vc->imm_modifier; \
      insn->valtag.field.mod.imm.constant_val = vc->constant_val; \
      insn->valtag.field.mod.imm.constant_tag = vc->constant_tag; \
      for (i = 0; i < IMM_MAX_NUM_SCONSTANTS; i++) { \
        insn->valtag.field.mod.imm.sconstants[i] = vc->sconstants[i]; \
      } \
    } \
    vc++; \
  } \
} while(0)

#define PPC_INSN_INV_RENAME_LI_BD(field) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    int vconst_num = vc - vconstants; \
    if (   insn->valtag.field.tag == tag_const \
        && vc->valid) {\
      NOT_TESTED(true); \
      ASSERT(vc->imm_modifier == IMM_UNMODIFIED); \
      insn->valtag.field.val = vc->val; \
      insn->valtag.field.tag = vc->tag; \
      insn->valtag.field.mod.imm.modifier = vc->imm_modifier; \
      vc++; \
    } \
  } \
} while(0)


void
ppc_insn_inv_rename_constants(ppc_insn_t *insn, vconstants_t const *vconstants)
{
  //do nothing. all work done in ppc_insn_inv_rename_memory_operands
}

bool
ppc_insn_inv_rename_memory_operands(ppc_insn_t *insn, regmap_t const *regmap,
    vconstants_t const *vconstants)
{
  vconstants_t const *vc;
  int i;

  vc = vconstants;

  //temporaries = &temporaries_store;
  //temporaries_init(temporaries);

  /* Rename all registers to tag_var, rename all constants using vconstants */
  PPC_INSN_INV_RENAME_EXREG(rSD.rS.base, PPC_EXREG_GROUP_GPRS);

  PPC_INSN_INV_RENAME_EXREG(frSD.frS.base, PPC_EXREG_GROUP_FPREG);

  if (ppc_insn_is_memory_access(insn)) {
    /* for memory accesses, uimm needs to be inv-renamed *before* rA, to conform
       to the order of vconstants. */
    if (insn->valtag.imm_signed != -1) {
      PPC_INSN_INV_RENAME_VCONST(imm.uimm);
    }
  }
  PPC_INSN_INV_RENAME_EXREG(rA, PPC_EXREG_GROUP_GPRS);
  PPC_INSN_INV_RENAME_EXREG(rB, PPC_EXREG_GROUP_GPRS);

  PPC_INSN_INV_RENAME_EXREG(crfS, PPC_EXREG_GROUP_CRF);
  PPC_INSN_INV_RENAME_EXREG(crfD, PPC_EXREG_GROUP_CRF);

  PPC_INSN_INV_RENAME_CRF_BIT(crbA);
  PPC_INSN_INV_RENAME_CRF_BIT(crbB);
  PPC_INSN_INV_RENAME_CRF_BIT(crbD);

  /*if (   insn->BO == 20
      || insn->BO == 16) */
  if (insn->BO != -1) {
    vc++;
  }
  if (ppc_insn_bo_indicates_bi_use(insn)) {
    PPC_INSN_INV_RENAME_CRF_BIT(BI);
  } else if (insn->valtag.BI.tag != tag_invalid) {
    vc++;
  }

  DBG2(PEEP_TAB, "insn->val.imm_signed=%d, insn->imm.uimm=%ld, vc-vconstants=%zd, "
      "vc->valid=%d\n",
      insn->valtag.imm_signed, (long)insn->valtag.imm.uimm.val, vc - vconstants, vc->valid);
  if (!ppc_insn_is_memory_access(insn)) {
    if (insn->valtag.imm_signed != -1) {
      PPC_INSN_INV_RENAME_VCONST(imm.uimm);
    }
  }
  if (insn->valtag.NB.tag != tag_invalid) {
    PPC_INSN_INV_RENAME_VCONST(NB);
  }
  if (insn->valtag.SH.tag != tag_invalid) {
    PPC_INSN_INV_RENAME_VCONST(SH);
  }
  if (insn->valtag.MB.tag != tag_invalid) {
    PPC_INSN_INV_RENAME_VCONST(MB);
  }
  if (insn->valtag.ME.tag != tag_invalid) {
    PPC_INSN_INV_RENAME_VCONST(ME);
  }

  PPC_INSN_INV_RENAME_LI_BD(LI);
  PPC_INSN_INV_RENAME_LI_BD(BD);

  PPC_INSN_INV_RENAME_EXREG(crf0, PPC_EXREG_GROUP_CRF);
  PPC_INSN_INV_RENAME_EXREG(i_crf0_so, PPC_EXREG_GROUP_CRF_SO);
  PPC_INSN_INV_RENAME_EXREG(i_crfS_so, PPC_EXREG_GROUP_CRF_SO);
  PPC_INSN_INV_RENAME_EXREG(i_crfD_so, PPC_EXREG_GROUP_CRF_SO);
  for (i = 0; i < insn->valtag.num_i_crfs; i++) {
    PPC_INSN_INV_RENAME_EXREG(i_crfs[i], PPC_EXREG_GROUP_CRF);
  }
  for (i = 0; i < insn->valtag.num_i_crfs_so; i++) {
    PPC_INSN_INV_RENAME_EXREG(i_crfs_so[i], PPC_EXREG_GROUP_CRF_SO);
  }
  for (i = 0; i < insn->valtag.num_i_sc_regs; i++) {
    PPC_INSN_INV_RENAME_EXREG(i_sc_regs[i], PPC_EXREG_GROUP_GPRS);
  }
  PPC_INSN_INV_RENAME_EXREG(lr0, PPC_EXREG_GROUP_LR);
  PPC_INSN_INV_RENAME_EXREG(ctr0, PPC_EXREG_GROUP_CTR);
  PPC_INSN_INV_RENAME_EXREG(xer_ov0, PPC_EXREG_GROUP_XER_OV);
  PPC_INSN_INV_RENAME_EXREG(xer_so0, PPC_EXREG_GROUP_XER_SO);
  PPC_INSN_INV_RENAME_EXREG(xer_ca0, PPC_EXREG_GROUP_XER_CA);
  PPC_INSN_INV_RENAME_EXREG(xer_bc_cmp0, PPC_EXREG_GROUP_XER_BC_CMP);
  return true;
}

void
ppc_insn_inv_rename_registers(ppc_insn_t *insn, regmap_t const *regmap)
{
  //do nothing. all work done in ppc_insn_inv_rename_memory_operands
}

/*
static void
ppc_iseq_inv_rename(ppc_insn_t *insns, int n_insns, regmap_t const *regmap,
    vconstants_t vconstants[][MAX_NUM_VCONSTANTS_PER_INSN])
{
  int i;
  for (i = 0; i < n_insns; i++) {
    DBG2(PEEP_TAB, "Inv-renaming insn %d, vc=%p\n", i, vconstants[i]);
    ppc_insn_inv_rename(&insns[i], regmap, vconstants[i]);
  }
}
*/

static char const *
ppc_mtcrf_infer_regcons(regcons_t *regcons,
    int const *crfs_used, int num_crfs_used, char const *start, char const *end,
    int group)
{
  char const *persign;
  bool ret;
  int i;

  i = 0;
  persign = start;
  while (i < num_crfs_used) {
    persign = strchr(persign, '%');
    ASSERT(persign);
    ret = vreg_add_regcons_one_reg(regcons, persign, end, group,
        crfs_used[i]);
    ASSERT(ret);
    persign++;
    i++;
  }
  ASSERT(persign < end);
  return persign;
}

char const *firstop_rA_zero_is_null_opcodes[] = {
  "dcbi", "dcbst", "dcbt", "dcbtst", "dcbz", "dcbf", "icbi", "icbt", 
};
size_t num_firstop_rA_zero_is_null_opcodes = NELEM(firstop_rA_zero_is_null_opcodes);
char const *secondop_rA_zero_is_null_opcodes[] = {
  "addi", "addis", "lbz", "lbzx", "lwz", "lwzx", "lhz", "lhzx", "lha", "lhax",
  "stb", "stbx", "stw", "stwx", "sth", "sthx", "lhbrx", "lwbrx", "stwbrx", "sthbrx",
  "lwarx", "stwcx",
};
size_t num_secondop_rA_zero_is_null_opcodes = NELEM(secondop_rA_zero_is_null_opcodes);

bool
ppc_infer_regcons_from_assembly(char const *assembly, regcons_t *regcons)
{
  char const *ptr, *newline, *opcodestr, *hash, *persign;
  int crfs_used[PPC_NUM_CRFREGS];
  int i, j;

  //regcons_clear(regcons);
  /*for (i = 0; i < PPC_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < PPC_NUM_EXREGS(i); j++) {
      //regcons_entry_set(regcons_get_entry(regcons, i, j), PPC_NUM_EXREGS(i));
      regcons_entry_set(regcons, i, j, PPC_NUM_EXREGS(i));
    }
  }*/
  regcons_set(regcons);

  int sc_regs[PPC_NUM_SC_IMPLICIT_REGS] = {0, 3, 4, 5, 6, 7, 8};
  bool ret;
  ptr = assembly;
  while (newline = strchr(ptr, '\n')) {
    if (   (opcodestr = strstr(ptr, "mtcrf"))
        && opcodestr < newline) {
      int crm, num_crfs_used;

      num_crfs_used = 0;
      crm = strtol(opcodestr + strlen("mtcrf") + 1, NULL, 0);
      for (i = 0; i < PPC_NUM_CRFREGS; i++) {
        if (crm & (1 << i)) {
          crfs_used[num_crfs_used++] = PPC_NUM_CRFREGS - i - 1;
        }
      }
      hash = strchr(opcodestr, '#');
      ASSERT(hash);
      persign = hash;
      persign = ppc_mtcrf_infer_regcons(regcons, crfs_used, num_crfs_used,
          persign, newline, PPC_EXREG_GROUP_CRF);
      persign = ppc_mtcrf_infer_regcons(regcons, crfs_used, num_crfs_used,
          persign, newline, PPC_EXREG_GROUP_CRF_SO);
      ASSERT(persign < newline);
    } else if (   (opcodestr = strstr(ptr, "sc"))
               && opcodestr < newline) {
      /*for (i = 0; i < PPC_NUM_GPRS - 1; i++) {
        regcons->regcons_extable[PPC_EXREG_GROUP_GPRS][i].bitmap |= 0x1; //re-allow register 0
      }*/

      hash = strchr(opcodestr, '#');
      ASSERT(hash);
      persign = hash;
      i = 0;
      while (i < PPC_NUM_SC_IMPLICIT_REGS) {
        persign = strchr(persign, '%');
        ASSERT(persign);
        DBG2(PEEP_TAB, "adding one-reg constraint %d for %s.\n", sc_regs[i], persign);
        ret = vreg_add_regcons_one_reg(regcons, persign, newline, PPC_EXREG_GROUP_GPRS,
            sc_regs[i]);
        ASSERT(ret);
        DBG2(PEEP_TAB, "regcons:\n%s\n", regcons_to_string(regcons, as1, sizeof as1));
        persign++;
        i++;
      }
      persign = strchr(persign, '%');
      ASSERT(persign);
      ret = vreg_add_regcons_one_reg(regcons, persign, newline,
          PPC_EXREG_GROUP_XER_SO, 0);
      ASSERT(ret);
    } else if (   (opcodestr = strstr_arr(ptr, firstop_rA_zero_is_null_opcodes,
                      num_firstop_rA_zero_is_null_opcodes))
               && opcodestr < newline) {
      char const *comma;
      persign = strchr(opcodestr, '%');
      ASSERT(persign);
      comma = strchr(opcodestr, ',');
      ASSERT(comma);
      if (persign < comma) {
        ret = vreg_remove_regcons_one_reg(regcons, persign, newline,
            PPC_EXREG_GROUP_GPRS, 0);
        ASSERT(ret);
      }
    } else if (   (opcodestr = strstr_arr(ptr, secondop_rA_zero_is_null_opcodes,
                      num_secondop_rA_zero_is_null_opcodes))
               && opcodestr < newline) {
      char const *comma;
      persign = strchr(opcodestr, '%');
      ASSERT(persign);
      comma = strchr(opcodestr, ',');
      ASSERT(comma);
      ASSERT(persign < comma);
      persign = comma + 1;
      comma = strchr(persign, ',');
      persign = strchr(persign, '%');
      if (persign && (!comma || persign < comma)) {
        ret = vreg_remove_regcons_one_reg(regcons, persign, newline,
            PPC_EXREG_GROUP_GPRS, 0);
        ASSERT(ret);
      }
    }
    DBG3(PPC_INSN, "ptr=%s\nregcons:\n%s\n", ptr, regcons_to_string(regcons, as1, sizeof as1));
    ptr = newline + 1;
  }
  DBG2(PPC_INSN, "regcons:\n%s\n", regcons_to_string(regcons, as1, sizeof as1));
  return true;
}

#if 0
long
ppc_iseq_from_string(translation_instance_t *ti, ppc_insn_t *iseq, char const *assembly)
{
  long max_num_insns = strlen(assembly)/3;
  vconstants_t vconstants[max_num_insns][MAX_NUM_VCONSTANTS_PER_INSN];
  char ppc_assembly[2 * strlen(assembly)], line[1024];
  uint8_t const *binptr, *binend;
  bool used_regs[PPC_NUM_GPRS];
  char const *newline, *ptr;
  char *ppc_ptr, *ppc_end;
  uint8_t bincode[1024];
  regcons_t regcons;
  regmap_t regmap;
  long n, i, j;
  ssize_t ret;
  bool bret;

  ppc_infer_regcons_from_assembly(assembly, &regcons);
  DBG(PEEP_TAB, "regcons:\n%s\n", regcons_to_string(&regcons, as1, sizeof as1));
  regmap_init(&regmap);
  bret = regmap_assign_using_regcons(&regmap, &regcons, ARCH_PPC, NULL);
  ASSERT(bret);
  DBG(PEEP_TAB, "regmap:\n%s\n", regmap_to_string(&regmap, as1, sizeof as1));

  for (i = 1; i < PPC_NUM_GPRS; i++) {
    used_regs[i] = false;
  }
  used_regs[0] = true; // do not use reg 0, as it could mean 'no register'

  ppc_ptr = ppc_assembly;
  ppc_end = ppc_ptr + sizeof ppc_assembly;

  ptr = assembly;
  n = 0;
  *ppc_ptr = '\0';
  while (newline = strchr(ptr, '\n')) {
    bool disas;
    ASSERT(newline - ptr < sizeof line);
    strncpy(line, ptr, newline - ptr);
    line[newline - ptr] = '\0';
    DBG(PEEP_TAB, "Calling ppc_peep_preprocess() on '%s' [line %ld], vc=%p.\n", line, n,
        vconstants[n]);
    ppc_peep_preprocess(line, &regmap, vconstants[n], MAX_NUM_VCONSTANTS_PER_INSN);
    DBG(PEEP_TAB, "Calling ppc_insn_from_string() on '%s'\n", line);
    n++;
    ASSERT(n < max_num_insns);
    ppc_ptr += snprintf(ppc_ptr, ppc_end - ppc_ptr, "%s\n", line);
    ptr = newline + 1;
  }
  ret = ppc_assemble(bincode, sizeof bincode, ppc_assembly);
  ASSERT(ret > 0);

  binptr = bincode;
  binend = bincode + ret;
  i = 0;
  while (binptr < binend) {
    int disas;
    disas = ppc_insn_disassemble(&iseq[i], binptr, I386_AS_MODE_32);
    ASSERT(disas == 4);
    binptr += disas;
    i++;
  }
  //ASSERT(i == n);//XXX

  DBG(PEEP_TAB, "before inv_rename, ppc_iseq: '%s'\n",
      ppc_iseq_to_string(iseq, i, as1, sizeof as1));
  ppc_iseq_inv_rename(iseq, i, &regmap, vconstants);
  DBG(PEEP_TAB, "after inv_rename, ppc_iseq: '%s'\n",
      ppc_iseq_to_string(iseq, i, as1, sizeof as1));
  return n;
}
#endif

bool
ppc_insn_get_constant_operand(ppc_insn_t const *insn, uint32_t *constant)
{
  return false;
}

/*static void
ppc_insn_assign_vregs_to_cregs(ppc_insn_t *insn, regmap_t const *regmap)
{
  ppc_insn_rename(insn, regmap, NULL, NULL, 0);
}

static void
ppc_assign_id_regmap(regmap_t *regmap)
{
  int i, j;

  memset(regmap, 0xff, sizeof(regmap_t));

  for (i = 0; i < SRC_NUM_REGS - 1; i++) {
    regmap->table[i] = i + 1;
  }
  for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < SRC_NUM_EXREGS(i); j++) {
      regmap->extable[i][j] = j;
    }
  }
}*/

void
ppc_iseq_copy(ppc_insn_t *dst, ppc_insn_t const *src, long len)
{
  long i;
  for (i = 0; i < len; i++) {
    ppc_insn_copy(&dst[i], &src[i]);
  } 
}

void
ppc_iseq_assign_vregs_to_cregs(ppc_insn_t *dst, ppc_insn_t const *src, long src_len,
    regmap_t *regmap, bool exec)
{
  regcons_t regcons;
  bool ret;
  long i;

  ppc_iseq_to_string(src, src_len, as1, sizeof as1);
  ppc_infer_regcons_from_assembly(as1, &regcons);
  regmap_init(regmap);
  ret = regmap_assign_using_regcons(regmap, &regcons, ARCH_PPC, NULL, fixed_reg_mapping_t());
  ASSERT(ret);
  ppc_iseq_copy(dst, src, src_len);
  for (i = 0; i < src_len; i++) {
    ppc_insn_rename(&dst[i], regmap, NULL, NULL, 0);
  }
/*
  int i;
  ASSERT(dst != src);
  ppc_assign_id_regmap(regmap);
  for (i = 0; i < src_len; i++) {
    ppc_insn_copy(&dst[i], &src[i]);
    ppc_insn_assign_vregs_to_cregs(&dst[i], regmap);
  }
  return true;
*/
}

//#define PPC_INSN_STRICT_CANONICALIZE_REG(field) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    ASSERT(insn->valtag.field.tag == tag_const); \
    ASSERT(insn->valtag.field.val >= 0 && insn->valtag.field.val < PPC_NUM_GPRS); \
    if (PPC_INSN_CONST_OPERAND_IS_NON_NULL_REG(insn, field)) { \
      unsigned reg = insn->valtag.field.val; \
      if (st_regmap->table[reg] == -1) { \
        st_regmap->table[reg] = num_gprs_used++; \
        ASSERT(num_gprs_used <= PPC_NUM_GPRS); \
      } \
      insn->valtag.field.val = st_regmap->table[reg]; \
      ASSERT(insn->valtag.field.val >= 0 && insn->valtag.field.val < PPC_NUM_GPRS); \
      insn->valtag.field.tag = tag_var; \
    } \
    /*vconst_num++; */\
  } \
} while(0)

#define PPC_INSN_STRICT_CANONICALIZE_EXREG(field, group) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    ASSERT(insn->valtag.field.tag == tag_const); \
    ASSERT(   insn->valtag.field.val >= 0 \
           && insn->valtag.field.val < PPC_NUM_EXREGS(group)); \
    /*ASSERT(PPC_INSN_CONST_OPERAND_IS_NON_NULL_REG(insn, field));*/{ \
      unsigned reg = insn->valtag.field.val; \
      if (regmap_get_elem(st_regmap, group, reg).reg_identifier_is_unassigned()) { \
        regmap_set_elem(st_regmap, group, reg, (num_exregs_used[group]++)); \
        ASSERT(num_exregs_used[group] <= PPC_NUM_EXREGS(group)); \
      } \
      insn->valtag.field.val = regmap_get_elem(st_regmap, group, reg).reg_identifier_get_id(); \
      insn->valtag.field.tag = tag_var; \
    } \
  } \
} while(0)

/*#define PPC_INSN_STRICT_CANONICALIZE_FPREG(field) do { \
  if (insn->tag.field != tag_invalid) { \
    ASSERT(insn->tag.field == tag_const); \
    ASSERT(insn->val.field >= 0 && insn->val.field < PPC_NUM_FPREGS); \
    unsigned fpn = insn->val.field; \
    unsigned reg = fpn; \
    if (st_regmap->regmap_extable[PPC_EXREG_GROUP_FPREG][reg] == -1) { \
      st_regmap->regmap_extable[PPC_EXREG_GROUP_FPREG][reg] = (num_fpreg_used++); \
      ASSERT(num_fpreg_used <= PPC_NUM_FPREGS); \
    } \
    insn->val.field = st_regmap->regmap_extable[PPC_EXREG_GROUP_FPREG][reg]; \
    insn->tag.field = tag_exvreg; \
    insn->exvreg_group.field = PPC_EXREG_GROUP_FPREG; \
  } \
} while(0)
*/

#define PPC_INSN_STRICT_CANONICALIZE_CRB(field) do { \
  if (insn->valtag.field.tag != tag_invalid) { \
    ASSERT(insn->valtag.field.tag == tag_const); \
    ASSERT(insn->valtag.field.val >= 0 && insn->valtag.field.val < 32); \
    unsigned crbn = insn->valtag.field.val; \
    unsigned reg = crbn/4; \
    unsigned _bit = crbn % 4; \
    unsigned _groupnum; \
    if (_bit == PPC_INSN_CRF_BIT_SO) { \
      _groupnum = PPC_EXREG_GROUP_CRF_SO; \
    } else { \
      _groupnum = PPC_EXREG_GROUP_CRF; \
    } \
    if (regmap_get_elem(st_regmap, _groupnum, reg).reg_identifier_is_unassigned()) { \
      regmap_set_elem(st_regmap, _groupnum, reg, (num_exregs_used[_groupnum]++)); \
      ASSERT(num_exregs_used[_groupnum] <= \
          PPC_NUM_EXREGS(_groupnum)); \
    } \
    insn->valtag.field.val = regmap_get_elem(st_regmap, _groupnum, reg).reg_identifier_get_id() * 4 + (crbn%4); \
    insn->valtag.field.tag = tag_var; \
  } \
} while(0)

/*
static long
ppc_iseq_strict_canonicalize_regs(ppc_insn_t *iseq_var[], long *iseq_var_len,
    regmap_t *st_regmaps, long num_iseq_var, long max_num_iseq_var)
{
  int num_gprs_used, num_exregs_used[PPC_NUM_EXREG_GROUPS];
  regmap_t *st_regmap;
  ppc_insn_t *insn;
  long i, c;

  num_gprs_used = 0;
  for (i = 0; i < PPC_NUM_EXREG_GROUPS; i++) {
    num_exregs_used[i] = 0;
  }

  ASSERT(num_iseq_var >= 1);
  DBG(PPC_INSN, "before:\n%s\n",
      ppc_iseq_to_string(iseq_var[0], iseq_var_len[0], as1, sizeof as1));
  for (i = 0; i < iseq_var_len[0]; i++) {
  }
  return 1;
}
*/

void
ppc_insn_add_implicit_operands(ppc_insn_t *dst)
{
  //do nothing
}

long
ppc_operand_strict_canonicalize_imms(ppc_strict_canonicalization_state_t *scanon_state,
    long insn_index, long op_index,
    ppc_insn_t *iseq_var[],
    long iseq_var_len[], struct regmap_t *st_regmap,
    imm_vt_map_t *st_imm_map, long num_iseq_var, long max_num_iseq_var)
{
  long max_num_out_var_ops = max_num_iseq_var - num_iseq_var + 1;
  imm_vt_map_t out_imm_maps[max_num_out_var_ops];
  valtag_t out_var_ops[max_num_out_var_ops];
  long n, num_out_var_ops;
  imm_vt_map_t *st_map;
  ppc_insn_t *insn;

  ASSERT(st_imm_map);
  ASSERT(op_index == 0); //there is only one "operand" in ppc insn

  ASSERT(num_iseq_var >= 1);
  ASSERT(max_num_iseq_var >= num_iseq_var);

  for (n = 0; n < num_iseq_var; n++) {
    st_map = &st_imm_map[n];
    insn = &iseq_var[n][insn_index];
    if (insn->valtag.imm_signed == 1) {
      num_out_var_ops = ST_CANON_IMM(-1, -1, out_var_ops, out_imm_maps,
          max_num_out_var_ops, insn->valtag.imm.simm, 16, st_map, ppc_imm_operand_needs_canonicalization);
      ASSERT(num_out_var_ops >= 1);
      valtag_copy(&insn->valtag.imm.simm, &out_var_ops[0]);
      imm_vt_map_copy(st_map, &out_imm_maps[0]);
    } else if (insn->valtag.imm_signed == 0) {
      num_out_var_ops = ST_CANON_IMM(-1, -1, out_var_ops, out_imm_maps,
          max_num_out_var_ops, insn->valtag.imm.uimm, 16, st_map, ppc_imm_operand_needs_canonicalization);
      ASSERT(num_out_var_ops >= 1);
      valtag_copy(&insn->valtag.imm.uimm, &out_var_ops[0]);
      imm_vt_map_copy(st_map, &out_imm_maps[0]);
    }

    num_out_var_ops = ST_CANON_IMM(-1, -1, out_var_ops, out_imm_maps,
        max_num_out_var_ops, insn->valtag.NB, 5, st_map, ppc_imm_operand_needs_canonicalization);
    ASSERT(num_out_var_ops >= 1);
    valtag_copy(&insn->valtag.NB, &out_var_ops[0]);
    imm_vt_map_copy(st_map, &out_imm_maps[0]);

    num_out_var_ops = ST_CANON_IMM_ONLY_EQ_CHECK(out_var_ops, out_imm_maps,
        max_num_out_var_ops, insn->valtag.SH, 5, st_map);
    ASSERT(num_out_var_ops >= 1);
    valtag_copy(&insn->valtag.SH, &out_var_ops[0]);
    imm_vt_map_copy(st_map, &out_imm_maps[0]);

    if (insn->valtag.SH.tag == tag_var) {
      ASSERT(insn->valtag.SH.mod.imm.modifier == IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0);
      ASSERT(insn->valtag.SH.mod.imm.sconstants[2] == 1);
      ASSERT(insn->valtag.SH.mod.imm.sconstants[1] == 0);
      ASSERT(insn->valtag.SH.mod.imm.sconstants[0] == 5);
      if (insn->valtag.MB.tag == tag_const) {
        if (insn->valtag.MB.val != 0) {
          if (   st_map->table[insn->valtag.SH.val].tag == tag_const
              && insn->valtag.MB.val == 32 - st_map->table[insn->valtag.SH.val].val) {
            insn->valtag.MB.tag = tag_var;
            insn->valtag.MB.val = insn->valtag.SH.val;
            insn->valtag.MB.mod.imm.modifier =  IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0;
            insn->valtag.MB.mod.imm.sconstants[2] = -1;
            insn->valtag.MB.mod.imm.sconstants[1] = 32;
            insn->valtag.MB.mod.imm.sconstants[0] = 5;
            //vconst_num++;
          } else {
            num_out_var_ops = ST_CANON_IMM_ONLY_EQ_CHECK(out_var_ops, out_imm_maps,
                max_num_out_var_ops, insn->valtag.MB, 5, st_map);
            ASSERT(num_out_var_ops >= 1);
            valtag_copy(&insn->valtag.MB, &out_var_ops[0]);
            imm_vt_map_copy(st_map, &out_imm_maps[0]);
          }
        }
      }
    }
    if (insn->valtag.MB.tag == tag_const && insn->valtag.MB.val != 0) {
      num_out_var_ops = ST_CANON_IMM_ONLY_EQ_CHECK(out_var_ops, out_imm_maps,
          max_num_out_var_ops, insn->valtag.MB, 5, st_map);
      ASSERT(num_out_var_ops >= 1);
      valtag_copy(&insn->valtag.MB, &out_var_ops[0]);
      imm_vt_map_copy(st_map, &out_imm_maps[0]);
    }
    if (insn->valtag.MB.tag != tag_invalid) {
      ASSERT(insn->valtag.ME.tag == tag_const);
      if (insn->valtag.ME.val != 31) {
        if (   insn->valtag.SH.tag == tag_var
            && st_map->table[insn->valtag.SH.val].tag == tag_const
            && insn->valtag.ME.val == 31 - st_map->table[insn->valtag.SH.val].val) {
          insn->valtag.MB.tag = tag_var;
          insn->valtag.MB.val = insn->valtag.SH.val;
          insn->valtag.MB.mod.imm.modifier =  IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0;
          insn->valtag.MB.mod.imm.sconstants[2] = -1;
          insn->valtag.MB.mod.imm.sconstants[1] = 31;
          insn->valtag.MB.mod.imm.sconstants[0] = 5;
          //vconst_num++;
        } else {
          num_out_var_ops = ST_CANON_IMM_ONLY_EQ_CHECK(out_var_ops, out_imm_maps,
              max_num_out_var_ops, insn->valtag.ME, 5, st_map);
          ASSERT(num_out_var_ops >= 1);
          valtag_copy(&insn->valtag.ME, &out_var_ops[0]);
          imm_vt_map_copy(st_map, &out_imm_maps[0]);
        }
      }
    }
  }

  DBG(PPC_INSN, "after %ld:\n%s\n", n,
      ppc_iseq_to_string(iseq_var[n], iseq_var_len[n], as1, sizeof as1));
  return num_iseq_var;
  //ASSERT(vconst_num <= MAX_NUM_VCONSTANTS_PER_INSN);
}

/*static long
ppc_iseq_strict_canonicalize_imms(ppc_insn_t *iseq_var[], long *iseq_var_len,
    imm_map_t *st_imm_map, long num_iseq_var, long max_num_iseq_var)
{
  long i;

  ASSERT(num_iseq_var >= 1);
  for (i = 0; i < iseq_var_len[0]; i++) {
    num_iseq_var = ppc_insn_strict_canonicalize_imms(i, iseq_var, iseq_var_len,
        st_imm_map, num_iseq_var, max_num_iseq_var);
  }
  return num_iseq_var;
}

long
ppc_iseq_strict_canonicalize(ppc_insn_t const *iseq, long iseq_len,
    ppc_insn_t *iseq_var[], long *iseq_var_len,
    regmap_t *st_regmap, imm_map_t *st_imm_map, long max_num_iseq_var)
{
  imm_map_t l_st_imm_map[max_num_iseq_var];
  long num_iseq_var, i;

  ASSERT(max_num_iseq_var >= 1);

  ppc_iseq_copy(iseq_var[0], iseq, iseq_len);
  iseq_var_len[0] = ppc_iseq_canonicalize(iseq_var[0], iseq_len);

  ASSERT(st_regmap);
  regmap_init(&st_regmap[0]);
  if (!st_imm_map) {
    st_imm_map = l_st_imm_map;
  }
  ASSERT(st_imm_map);
  imm_map_init(&st_imm_map[0]);
  num_iseq_var = 1;

  num_iseq_var = ppc_iseq_strict_canonicalize_regs(iseq_var, iseq_var_len, st_regmap,
      num_iseq_var, max_num_iseq_var);
  ASSERT(iseq_var[0]);
  ASSERT(num_iseq_var == 1);

  num_iseq_var = ppc_iseq_strict_canonicalize_imms(iseq_var, iseq_var_len, st_imm_map,
      num_iseq_var, max_num_iseq_var);
  ASSERT(num_iseq_var <= max_num_iseq_var);
  ASSERT(iseq_var[0]);
  for (i = 1; i < num_iseq_var; i++) {
    regmap_copy(&st_regmap[i], &st_regmap[0]);
  }
  return num_iseq_var;
}
*/

static size_t
ppc_insn_valtag_to_int_array(ppc_insn_valtag_t const *valtag, int64_t arr[], size_t len)
{
#define VT2INT(field) do { \
  DBG(CAMLARGS, "writing " #field ". cur = %d\n", cur); \
  cur += valtag_to_int_array(&valtag->field, &arr[cur], end - cur);\
} while(0)

  int cur = 0, end = len;
  int i;

  VT2INT(rSD.rS.base);
  VT2INT(rSD.rD.base);

  VT2INT(rA);
  VT2INT(rB);

  arr[cur++] = valtag->imm_signed;
  VT2INT(imm.fpimm);
  VT2INT(imm.simm);
  VT2INT(imm.uimm);

  VT2INT(BD);
  VT2INT(LI);

  VT2INT(crfS);
  VT2INT(crfD);

  VT2INT(crbA);
  VT2INT(crbB);
  VT2INT(crbD);

  VT2INT(BI);

  VT2INT(NB);
  VT2INT(SH);
  VT2INT(MB);
  VT2INT(ME);

  //VT2INT(CRM);
  arr[cur++] = valtag->CRM;

  VT2INT(frSD.frS.base);
  VT2INT(frSD.frD.base);

  VT2INT(crf0);
  VT2INT(i_crf0_so);
  VT2INT(i_crfS_so);
  VT2INT(i_crfD_so);
  arr[cur++] = valtag->num_i_crfs;
  for (i = 0; i < valtag->num_i_crfs; i++) {
    VT2INT(i_crfs[i]);
  }
  arr[cur++] = valtag->num_i_crfs_so;
  for (i = 0; i < valtag->num_i_crfs_so; i++) {
    VT2INT(i_crfs_so[i]);
  }
  arr[cur++] = valtag->num_i_sc_regs;
  for (i = 0; i < valtag->num_i_sc_regs; i++) {
    VT2INT(i_sc_regs[i]);
  }
  VT2INT(lr0);
  VT2INT(ctr0);
  VT2INT(xer_ov0);
  VT2INT(xer_so0);
  VT2INT(xer_ca0);
  VT2INT(xer_bc_cmp0);

  return cur;
}

size_t
ppc_insn_to_int_array(ppc_insn_t const *insn, int64_t arr[], size_t len)
{
  int cur = 0, end = len, i, j;
  DBG(CAMLARGS, "writing opc1, cur=%d\n", cur);
  arr[cur++] = insn->opc1;
  arr[cur++] = insn->opc2;
  arr[cur++] = insn->opc3;
  arr[cur++] = insn->FM;
  arr[cur++] = insn->Rc;
  arr[cur++] = insn->LK;
  arr[cur++] = insn->BO;
  DBG(CAMLARGS, "writing SPR, cur=%d\n", cur);
  arr[cur++] = insn->SPR;
  arr[cur++] = insn->SR;
  arr[cur++] = insn->AA;
  DBG(CAMLARGS, "writing valtag, cur=%d\n", cur);
  cur += ppc_insn_valtag_to_int_array(&insn->valtag, &arr[cur], end - cur);
  DBG(CAMLARGS, "cur=%d\n", cur);
  /*for (i = 0; i < MAX_NUM_VCONSTANTS_PER_INSN; i++) {
    arr[cur++] = insn->imm_modifier[i];
    arr[cur++] = insn->constant[i];
    for (j = 0; j < IMM_MAX_NUM_SCONSTANTS; j++) {
      arr[cur++] = insn->sconstants[i][j];
      ASSERT(cur < len);
    }
    ASSERT(cur < len);
  }*/
  arr[cur++] = insn->reserved_mask;
  //arr[cur++] = insn->is_fp_insn;
  ASSERT(cur < len);
  //cur += memvt_list_to_int_array(&insn->memtouch, &arr[cur], end - cur);
  //ASSERT(cur < len);
  return cur;
}

bool
ppc_insn_get_pcrel_value(ppc_insn_t const *insn, uint64_t *pcrel_val,
    src_tag_t *pcrel_tag)
{
  ASSERT(insn->valtag.LI.tag == tag_invalid || insn->valtag.BD.tag == tag_invalid);
  if (insn->valtag.LI.tag != tag_invalid) {
    *pcrel_val = insn->valtag.LI.val;
    *pcrel_tag = insn->valtag.LI.tag;
    return true;
  }
  if (insn->valtag.BD.tag != tag_invalid) {
    *pcrel_val = insn->valtag.BD.val;
    *pcrel_tag = insn->valtag.BD.tag;
    return true;
  }
  *pcrel_val = -1;
  *pcrel_tag = tag_invalid;
  return false;
}

void
ppc_insn_set_pcrel_value(ppc_insn_t *insn, uint64_t pcrel_val,
    src_tag_t pcrel_tag)
{
  ASSERT(insn->valtag.LI.tag == tag_invalid || insn->valtag.BD.tag == tag_invalid);
  ASSERT(insn->valtag.LI.tag != tag_invalid || insn->valtag.BD.tag != tag_invalid);
  if (insn->valtag.LI.tag != tag_invalid) {
    insn->valtag.LI.val = pcrel_val;
    insn->valtag.LI.tag = pcrel_tag;
    return;
  }
  if (insn->valtag.BD.tag != tag_invalid) {
    insn->valtag.BD.val = pcrel_val;
    insn->valtag.BD.tag = pcrel_tag;
    return;
  }
  NOT_REACHED();
}

long
ppc_insn_put_jump_to_pcrel(src_ulong target, ppc_insn_t *insns, size_t insns_size)
{
  ppc_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  ppc_insn_init(insn_ptr);
  insn_ptr->opc1 = 18; /* b */
  insn_ptr->valtag.LI.val = target;
  insn_ptr->valtag.LI.tag = tag_const;
  insn_ptr->AA = 0;
  insn_ptr->LK = 0;
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
ppc_insn_put_jump_to_nextpc(ppc_insn_t *insns, size_t insns_size, long nextpc_num)
{
  ppc_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  ppc_insn_init(insn_ptr);
  insn_ptr->opc1 = 18; /* b */
  insn_ptr->valtag.LI.val = nextpc_num;
  insn_ptr->valtag.LI.tag = tag_var;
  insn_ptr->AA = 0;
  insn_ptr->LK = 0;
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);
  return insn_ptr - insns;
}

void
ppc_insn_add_to_pcrels(ppc_insn_t *insn, src_ulong pc, struct bbl_t const *bbl, long bblindex)
{
  printf("%s() %d: Warning: insn add to pcrel failed\n", __func__, __LINE__);
  ASSERT(insn->valtag.LI.tag == tag_invalid || insn->valtag.BD.tag == tag_invalid);
  if (insn->valtag.LI.tag != tag_invalid) {
    ASSERT(insn->valtag.LI.tag == tag_const);
    insn->valtag.LI.val = SIGN_EXT(insn->valtag.LI.val, 24) + pc;
    return;
  }
  if (insn->valtag.BD.tag != tag_invalid) {
    ASSERT(insn->valtag.BD.tag == tag_const);
    /*printf("pc %lx, BD=%lx, sum %lx\n", (long)pc, (long)insn->val.BD,
        (long)(insn->val.BD + pc));*/
    insn->valtag.BD.val = SIGN_EXT(insn->valtag.BD.val, 16) + pc;
    return;
  }
}

long
ppc_insn_put_nop(ppc_insn_t *insns, size_t insns_size)
{
  ppc_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  ppc_insn_init(insn_ptr);
  insn_ptr->opc1 = 24;  /* ori */
  insn_ptr->valtag.rSD.rS.base.val = 0;
  insn_ptr->valtag.rSD.rS.base.tag = tag_const;
  insn_ptr->valtag.rA.val = 0;
  insn_ptr->valtag.rA.tag = tag_const;
  insn_ptr->valtag.imm.uimm.val = 0;
  insn_ptr->valtag.imm.uimm.tag = tag_const;
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

/*
long
ppc_insn_put_add_regs(int rd, int ra, int rb, ppc_insn_t *insns, size_t insns_size)
{
  NOT_IMPLEMENTED();
}

long
ppc_insn_put_subf_regs(int rd, int ra, int rb, ppc_insn_t *insns, size_t insns_size)
{
  NOT_IMPLEMENTED();
}
*/

long
ppc_insn_put_xor(int regA, int regS, int regB, ppc_insn_t *insns, size_t insns_size)
{
  ppc_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  ppc_insn_init(insn_ptr);
  insn_ptr->opc1 = 31;
  insn_ptr->opc2 = 316 % 32;
  insn_ptr->opc3 = 316 / 32;
  insn_ptr->valtag.rA.val = regA;
  insn_ptr->valtag.rA.tag = tag_const;
  insn_ptr->valtag.rSD.rS.base.val = regS;
  insn_ptr->valtag.rSD.rS.base.tag = tag_const;
  insn_ptr->valtag.rB.val = regB;
  insn_ptr->valtag.rB.tag = tag_const;
  insn_ptr->Rc = 0;
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

long
ppc_insn_put_dcbi(int reg1, int reg2, ppc_insn_t *insns, size_t insns_size)
{
  ppc_insn_t *insn_ptr, *insn_end;

  insn_ptr = insns;
  insn_end = insns + insns_size;

  ppc_insn_init(insn_ptr);
  insn_ptr->opc1 = 31;
  insn_ptr->opc2 = 470 % 32;
  insn_ptr->opc3 = 470 / 32;
  insn_ptr->valtag.rA.val = reg1;
  insn_ptr->valtag.rA.tag = tag_const;
  insn_ptr->valtag.rB.val = reg2;
  insn_ptr->valtag.rB.tag = tag_const;
  //insn_ptr->Rc = -1;
  insn_ptr++;
  ASSERT(insn_ptr <= insn_end);

  return insn_ptr - insns;
}

int
ppc_iseq_get_num_canon_constants(ppc_insn_t const *iseq, int iseq_len)
{
  NOT_IMPLEMENTED();
}

bool
ppc_iseq_can_be_optimized(ppc_insn_t const *iseq, int iseq_len)
{
  NOT_IMPLEMENTED();
}

bool
ppc_iseq_rename_regs_and_imms(ppc_insn_t *iseq, long iseq_len,
    regmap_t const *regmap, imm_vt_map_t const *imm_map, src_ulong const *nextpcs,
    long num_nextpcs/*, src_ulong const *insn_pcs, long num_insns*/)
{
  ppc_insn_t *insn;
  long n;

  for (n = 0; n < iseq_len; n++) {
    insn = &iseq[n];
    if (!ppc_insn_rename(insn, regmap, imm_map, nextpcs, num_nextpcs/*, insn_pcs,
        num_insns*/)) {
      return false;
    }
  }
  return true;
}

size_t
ppc_insn_put_nop_as(char *buf, size_t size)
{
  char *ptr, *end;
  ptr = buf;
  end = buf + size;
  ptr += snprintf(ptr, end - ptr, "nop #\n");
  ASSERT(ptr < end);
  return ptr - buf;
}

size_t
ppc_insn_put_linux_exit_syscall_as(char *buf, size_t size)
{
  char *ptr, *end;
  ptr = buf;
  end = buf + size;
  ptr += snprintf(ptr, end - ptr, "%s", "li 0,1\nli 3,1\nsc\n");
  ASSERT(ptr < end);
  return ptr - buf;
}

long
ppc_iseq_window_convert_pcrels_to_nextpcs(ppc_insn_t *ppc_iseq,
    long ppc_iseq_len, long start, long *nextpc2src_pcrel_vals,
    src_tag_t *nextpc2src_pcrel_tags)
{
  long i, j, cur, target_val;
  src_tag_t target_tag;
  ppc_insn_t *insn;
  bool done;

  cur = 0;
  for (i = 0; i < ppc_iseq_len; i++) {
    insn = &ppc_iseq[i];
    if (insn->valtag.LI.tag != tag_invalid) {
      target_val = insn->valtag.LI.val;
      target_tag = insn->valtag.LI.tag;
    } else if (insn->valtag.BD.tag != tag_invalid) {
      target_val = insn->valtag.BD.val;
      target_tag = insn->valtag.BD.tag;
    } else if (ppc_insn_is_indirect_branch(insn)) {
      target_val = PC_JUMPTABLE;
      target_tag = tag_const;
      nextpc2src_pcrel_vals[cur] = PC_JUMPTABLE;
      nextpc2src_pcrel_tags[cur] = tag_const;
      cur++;
    } else {
      continue;
    }
    done = false;
    for (j = 0; j < cur; j++) {
      if (   nextpc2src_pcrel_vals[j] == start + 1 + target_val
          && nextpc2src_pcrel_tags[j] == target_tag) {
        ppc_insn_set_branch_target(insn, j, tag_var);
        done = true;
        break;
      }
    }
    if (done) {
      continue;
    }

    if (target_tag == tag_const) {
      if (   target_val != -1   //not a self loop
          && target_val != PC_JUMPTABLE) {
        nextpc2src_pcrel_vals[cur] = start + 1 + target_val;
        nextpc2src_pcrel_tags[cur] = target_tag;
        ppc_insn_set_branch_target(insn, cur, tag_var);
        cur++;
      }
    } else {
      ASSERT(target_tag == tag_var);
      nextpc2src_pcrel_vals[cur] = target_val;
      nextpc2src_pcrel_tags[cur] = target_tag;
      ppc_insn_set_branch_target(insn, cur, tag_var);
      cur++;
    }
  }
  return cur;
}

bool
ppc_insn_less(ppc_insn_t const *a, ppc_insn_t const *b)
{
  if (a->opc1 > b->opc1) {
    return false;
  } else if (a->opc1 < b->opc1) {
    return true;
  }
  ASSERT(a->opc1 == b->opc1);

  if (a->opc2 > b->opc2) {
    return false;
  } else if (a->opc2 < b->opc2) {
    return true;
  }
  ASSERT(a->opc2 == b->opc2);

  if (a->opc3 > b->opc3) {
    return false;
  } else if (a->opc3 < b->opc3) {
    return true;
  }
  ASSERT(a->opc3 == b->opc3);
  return false;
}

void
ppc_iseq_rename(ppc_insn_t *iseq, long iseq_len, regmap_t const *dst_regmap,
    imm_vt_map_t const *imm_map, regmap_t const *src_regmap,
    src_ulong const *nextpcs, long num_nextpcs)
{
  long i;

  for (i = 0; i < iseq_len; i++) {
    ppc_insn_rename(&iseq[i], dst_regmap, imm_map, nextpcs, num_nextpcs);
  }
}

static bool
ppc_insn_is_stwcx_(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 22 && insn->opc3 == 4) {
    return true;
  }
  return false;
}

static bool
ppc_insn_is_lwarx(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 20 && insn->opc3 == -1) {
    return true;
  }
  return false;
}

static bool
ppc_insn_is_cntlzw(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 26 && insn->opc3 == 0) {
    return true;
  }
  return false;
}

static bool
ppc_insn_is_sraw(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 24 && insn->opc3 == 24) {
    return true;
  }
  return false;
}

static bool
ppc_insn_is_dcbz(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 22 && insn->opc3 == 31) {
    return true;
  }
  return false;
}

static bool
ppc_insn_fallback_translation_has_callback(ppc_insn_t const *insn)
{
  if (   ppc_insn_is_stwcx_(insn)
      || ppc_insn_is_lwarx(insn)
      || ppc_insn_is_cntlzw(insn)
      || ppc_insn_is_sraw(insn)
      || ppc_insn_is_dcbz(insn)) {
    return true;
  }
  return false;
}

bool
ppc_insn_supports_execution_test(ppc_insn_t const *insn)
{
  if (ppc_insn_is_indirect_branch(insn)) {
    return false;
  }
  if (ppc_insn_is_function_call(insn)) {
    return false;
  }
  if (ppc_insn_is_mtcrf(insn)) {
    return false;
  }
  if (!ppc_insn_supports_boolean_test(insn)) {
    return false;
  }
  if (ppc_insn_fallback_translation_has_callback(insn)) {
    return false;
  }
  return true;
}

static bool
ppc_insn_is_mfmsr(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 19 && insn->opc3 == 2) {
    return true;
  }
  return false;
}

static bool
ppc_insn_is_mtmsr(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 18 && insn->opc3 == 4) {
    return true;
  }
  return false;
}

static bool
ppc_insn_is_mftb(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 19 && insn->opc3 == 11) {
    return true;
  }
  return false;
}

static bool
ppc_insn_is_mfspr(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 19 && insn->opc3 == 10) {
    return true;
  }
  return false;
}

static bool
ppc_insn_is_mtspr(ppc_insn_t const *insn)
{
  if (insn->opc1 == 31 && insn->opc2 == 19 && insn->opc3 == 14) {
    return true;
  }
  return false;
}

bool
ppc_insn_is_intdiv(ppc_insn_t const *insn)
{
  int opc23_ = (insn->opc3 & 15) * 32 + insn->opc2;
  if (insn->opc1 == 31 && (opc23_ == 459 || opc23_ == 491)) {
    return true;
  }
  return false;
}

bool
ppc_insn_supports_boolean_test(ppc_insn_t const *insn)
{
  if (ppc_insn_is_sc(insn)) { /* sc */
    return false;
  }
  if (   ppc_insn_is_mfmsr(insn)
      || ppc_insn_is_mtmsr(insn)
      || ppc_insn_is_mftb(insn)
      || (   (ppc_insn_is_mfspr(insn) || ppc_insn_is_mtspr(insn))
          && insn->SPR != PPC_SPR_NUM_XER_BC_CMP
          && insn->SPR != PPC_SPR_NUM_CTR
          && insn->SPR != PPC_SPR_NUM_LR)
      || ppc_insn_is_intdiv(insn)) {
    return false;
  }
  return true;
}

bool
ppc_iseq_supports_execution_test(ppc_insn_t const *iseq, long iseq_len)
{
  long i;
  for (i = 0; i < iseq_len; i++) {
    if (!ppc_insn_supports_execution_test(&iseq[i])) {
      return false;
    }
  }
  DBG2(EQUIV, "returning true for:\n%s\n",
      ppc_iseq_to_string(iseq, iseq_len, as1, sizeof as1));
  return true;
}

bool
ppc_insn_merits_optimization(ppc_insn_t const *insn)
{
  if (ppc_insn_is_mtcrf(insn)) {
    return false;
  }
  if (ppc_insn_is_mfcr(insn)) {
    return false;
  }
  return true;
}

static void
ppc_insn_mark_used_vconstants(imm_vt_map_t *map, imm_vt_map_t *nomem_map,
    imm_vt_map_t *mem_map,
    ppc_insn_t const *insn)
{
  if (insn->valtag.imm_signed == 1) {
    if (insn->valtag.imm.simm.tag == tag_var) {
      imm_vt_map_mark_used_vconstant(nomem_map, &insn->valtag.imm.simm);
      imm_vt_map_mark_used_vconstant(map, &insn->valtag.imm.simm);
    }
  } else if (insn->valtag.imm_signed == 0) {
    if (insn->valtag.imm.uimm.tag == tag_var) {
      imm_vt_map_mark_used_vconstant(nomem_map, &insn->valtag.imm.uimm);
      imm_vt_map_mark_used_vconstant(map, &insn->valtag.imm.uimm);
    }
  }
  if (insn->valtag.NB.tag == tag_var) {
    imm_vt_map_mark_used_vconstant(nomem_map, &insn->valtag.imm.uimm);
    imm_vt_map_mark_used_vconstant(map, &insn->valtag.NB);
  }
  if (insn->valtag.SH.tag == tag_var) {
    imm_vt_map_mark_used_vconstant(nomem_map, &insn->valtag.imm.uimm);
    imm_vt_map_mark_used_vconstant(map, &insn->valtag.SH);
  }
  if (insn->valtag.MB.tag == tag_var) {
    imm_vt_map_mark_used_vconstant(nomem_map, &insn->valtag.imm.uimm);
    imm_vt_map_mark_used_vconstant(map, &insn->valtag.MB);
  }
  if (insn->valtag.ME.tag == tag_var) {
    imm_vt_map_mark_used_vconstant(nomem_map, &insn->valtag.imm.uimm);
    imm_vt_map_mark_used_vconstant(map, &insn->valtag.ME);
  }
}

void
ppc_iseq_mark_used_vconstants(imm_vt_map_t *map, imm_vt_map_t *nomem_map,
    imm_vt_map_t *mem_map, ppc_insn_t const *iseq, long iseq_len)
{
  long i;

  imm_vt_map_init(map);
  imm_vt_map_init(nomem_map);
  imm_vt_map_init(mem_map);
  for (i = 0; i < iseq_len; i++) {
    ppc_insn_mark_used_vconstants(map, nomem_map, mem_map, &iseq[i]);
  }
}

long
ppc_iseq_compute_num_nextpcs(ppc_insn_t const *iseq, long iseq_len, long *nextpc_indir)
{
  long i, nextpcs[iseq_len], num_nextpcs;
  bool found_indir;

  for (i = 0; i < iseq_len; i++) {
    nextpcs[i] = PC_UNDEFINED;
  }
  found_indir = false;
  for (i = 0; i < iseq_len; i++) {
    if (ppc_insn_is_indirect_branch(&iseq[i])) {
      found_indir = true;
    }
    if (iseq[i].valtag.LI.tag == tag_var) {
      ASSERT(iseq[i].valtag.LI.val >= 0 && iseq[i].valtag.LI.val < iseq_len);
      nextpcs[iseq[i].valtag.LI.val] = iseq[i].valtag.LI.val;
    }
    if (iseq[i].valtag.BD.tag == tag_var) {
      ASSERT(iseq[i].valtag.BD.val >= 0 && iseq[i].valtag.BD.val < iseq_len);
      nextpcs[iseq[i].valtag.BD.val] = iseq[i].valtag.BD.val;
    }
  }
  if (nextpc_indir) {
    *nextpc_indir = -1;
  }
  if (found_indir && nextpc_indir) {
    for (i = 0; i < iseq_len; i++) {
      if (nextpcs[i] == PC_UNDEFINED) {
        nextpcs[i] = PC_JUMPTABLE;
        *nextpc_indir = i;
        break;
      }
    }
  }
  num_nextpcs = 0;
  for (i = 0; i < iseq_len; i++) {
    if (nextpcs[i] != PC_UNDEFINED) {
      num_nextpcs++;
    }
  }
  return num_nextpcs;
}

typedef void (*collect_typestate_constraints_fn_t)(typestate_constraints_t *,
    ppc_insn_t const *, state_t *, typestate_t *);

static void
ppc_logic_collect_tscons(typestate_constraints_t *tscons, ppc_insn_t const *insn,
    state_t *state, typestate_t *typestate)
{
  NOT_IMPLEMENTED();
}

static void
ppc_b_collect_tscons(typestate_constraints_t *tscons, ppc_insn_t const *insn,
    state_t *state, typestate_t *typestate)
{
  NOT_IMPLEMENTED();
}

typedef struct ppc_insn_desc_t {
  int opc1, opc23, extra;
  collect_typestate_constraints_fn_t collect_tscons_fn;
} ppc_insn_desc_t;

static ppc_insn_desc_t *ppc_descs = NULL;
static long ppc_num_descs = 0;
static long ppc_descs_size = 0;

static void
ppc_insn_desc_add(ppc_insn_desc_t **pd, long *pdnum, long *pdsize, int opc1, int opc23,
    int extra, collect_typestate_constraints_fn_t collect_tscons_fn)
{
  if (*pdsize < *pdnum + 1) {
    *pdsize = (*pdsize + 1) * 2;
    *pd = (ppc_insn_desc_t *)realloc(*pd, *pdsize * sizeof(ppc_insn_desc_t));
    ASSERT(*pd);
  }
  (*pd)[*pdnum].opc1 = opc1;
  (*pd)[*pdnum].opc23 = opc23;
  (*pd)[*pdnum].extra = extra;
  (*pd)[*pdnum].collect_tscons_fn = collect_tscons_fn;
  (*pdnum)++;
  ASSERT(*pdnum <= *pdsize);
}

static void
ppc_insn_init_descs(void)
{
  ppc_insn_desc_t *pd;
  long pdnum, pdsize;

  pd = ppc_descs;
  pdnum = ppc_num_descs;
  pdsize = ppc_descs_size;

  /*ppc_insn_desc_add(pd, &pdnum, &pdsize, 7, -1, -1, mulli_collect_tcons);
  ppc_insn_desc_add(pd, &pdnum, &pdsize, 8, -1, -1, subfic_collect_tcons);
  ppc_insn_desc_add(pd, &pdnum, &pdsize, 10, -1, -1, cmpli_collect_tcons);
  ppc_insn_desc_add(pd, &pdnum, &pdsize, 11, -1, -1, cmpi_collect_tcons);
  ppc_insn_desc_add(pd, &pdnum, &pdsize, 12, -1, -1, addic_collect_tcons);
  ppc_insn_desc_add(pd, &pdnum, &pdsize, 13, -1, -1, addicdot_collect_tcons);
  ppc_insn_desc_add(pd, &pdnum, &pdsize, 14, -1, -1, addi_collect_tcons);
  ppc_insn_desc_add(pd, &pdnum, &pdsize, 15, -1, -1, addis_collect_tcons);*/
  ppc_insn_desc_add(&pd, &pdnum, &pdsize, 31, 444, -1, ppc_logic_collect_tscons);
  ppc_insn_desc_add(&pd, &pdnum, &pdsize, 18, -1, -1, ppc_b_collect_tscons);

  ppc_descs = pd;
  ppc_num_descs = pdnum;
  ppc_descs_size = pdsize;
}

void
ppc_init(void)
{
  static bool ppc_inited = false;

  if (ppc_inited) {
    return;
  }
  ppc_inited = true;

  ppc_insn_disas_init();
  ppc_insn_init_descs();

  if (gas_inited) {
    return;
  }
  gas_inited = true;
  gas_init();
}

void ppc_free(void)
{
}

static void
__ppc_insn_get_min_max_vconst_masks(int *min_mask, int *max_mask,
    ppc_insn_t const *src_insn)
{
  if (src_insn->valtag.imm_signed == 1) {
    ASSERT(src_insn->valtag.imm.simm.tag != tag_invalid);
    imm_valtag_get_min_max_vconst_masks(min_mask, max_mask, &src_insn->valtag.imm.simm);
  } else if (src_insn->valtag.imm_signed == 0) {
    ASSERT(src_insn->valtag.imm.uimm.tag != tag_invalid);
    imm_valtag_get_min_max_vconst_masks(min_mask, max_mask, &src_insn->valtag.imm.uimm);
  } else ASSERT(src_insn->valtag.imm_signed == -1);

  imm_valtag_get_min_max_vconst_masks(min_mask, max_mask, &src_insn->valtag.BI);
  imm_valtag_get_min_max_vconst_masks(min_mask, max_mask, &src_insn->valtag.NB);
  imm_valtag_get_min_max_vconst_masks(min_mask, max_mask, &src_insn->valtag.SH);
  imm_valtag_get_min_max_vconst_masks(min_mask, max_mask, &src_insn->valtag.MB);
  imm_valtag_get_min_max_vconst_masks(min_mask, max_mask, &src_insn->valtag.ME);
}

void
ppc_insn_get_min_max_vconst_masks(int *min_mask, int *max_mask,
    ppc_insn_t const *src_insn)
{
  if (ppc_insn_is_memory_access(src_insn)) {
    return;
  }
  __ppc_insn_get_min_max_vconst_masks(min_mask, max_mask, src_insn);
}

void
ppc_insn_get_min_max_mem_vconst_masks(int *min_mask, int *max_mask,
    ppc_insn_t const *src_insn)
{
   if (!ppc_insn_is_memory_access(src_insn)) {
    return;
  }
  __ppc_insn_get_min_max_vconst_masks(min_mask, max_mask, src_insn);
}

bool
ppc_insn_type_signature_supported(ppc_insn_t const *insn)
{
  return true;
}

long long
ppc_iseq_compute_cost(ppc_insn_t const *iseq, long iseq_len, ppc_costfn_t costfn)
{
  NOT_IMPLEMENTED();
}

int
ppc_iseq_assemble(struct translation_instance_t const *ti, ppc_insn_t const *insns,
    size_t n_insns, unsigned char *buf, size_t size, i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
}

long
ppc_insn_put_move_exreg_to_mem(dst_ulong addr, int group, int reg,
    ppc_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
}

long
ppc_insn_put_move_mem_to_exreg(int group, int reg, dst_ulong addr,
    ppc_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
}

long
ppc_insn_put_move_exreg_to_exreg(int group1, int reg1, int group2, int reg2,
    ppc_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
}

long ppc_insn_put_xchg_exreg_with_exreg(int group1, int reg1, int group2, int reg2,
    ppc_insn_t *insns, size_t insns_size, i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
}

long
ppcx_assemble(translation_instance_t *ti, uint8_t *bincode, char const *assembly,
    i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
}

void
ppc_iseq_count_temporaries(ppc_insn_t const *ppc_iseq, long ppc_iseq_len,
    struct transmap_t const *tmap, struct temporaries_count_t *temps_count)
{
  NOT_IMPLEMENTED();
}

char *
ppc_iseq_with_relocs_to_string(ppc_insn_t const *iseq, long iseq_len,
    char const *reloc_strings, size_t reloc_strings_size, i386_as_mode_t mode,
    char *buf, size_t buf_size)
{
  NOT_IMPLEMENTED();
}

void
ppc_iseq_rename_var(ppc_insn_t *iseq, long iseq_len, regmap_t const *dst_regmap,
    imm_vt_map_t const *imm_vt_map, regmap_t const *src_regmap,
    nextpc_map_t const *nextpc_map, src_ulong const *nextpcs, long num_nextpcs)
{
  NOT_IMPLEMENTED();
}

bool
ppc_iseq_contains_reloc_reference(ppc_insn_t const *iseq, long iseq_len)
{
  NOT_IMPLEMENTED();
}

valtag_t *
ppc_insn_get_pcrel_operand(ppc_insn_t const *insn)
{
  ASSERT(insn->valtag.LI.tag == tag_invalid || insn->valtag.BD.tag == tag_invalid);
  if (insn->valtag.LI.tag != tag_invalid) {
    return (valtag_t *)&insn->valtag.LI;
  }
  if (insn->valtag.BD.tag != tag_invalid) {
    return (valtag_t *)&insn->valtag.BD;
  }
  return NULL;
}

void
ppc_insn_canonicalize(ppc_insn_t *insn)
{
  return;
}

bool
ppc_insns_equal(ppc_insn_t const *i1, ppc_insn_t const *i2)
{
  NOT_IMPLEMENTED();
}

bool
ppc_insn_is_lea(ppc_insn_t const *insn)
{
  return false;
}

/*long
ppc_iseq_sandbox(ppc_insn_t *ppc_iseq, long ppc_iseq_len, long ppc_iseq_size,
    int tmpreg, regset_t *ctemps)
{
  NOT_IMPLEMENTED();
}*/

bool
ppc_insn_is_direct_branch(ppc_insn_t const *insn)
{
  NOT_IMPLEMENTED();
}

/*long
ppc_iseq_make_position_independent(ppc_insn_t *iseq, long iseq_len,
    long iseq_size, uint32_t scratch_addr)
{
  NOT_IMPLEMENTED();
}*/

void
ppc_iseq_rename_nextpc_consts(ppc_insn_t *iseq, long iseq_len,
    uint8_t const * const next_tcodes[], long num_nextpcs)
{
  NOT_IMPLEMENTED();
}

void
ppc_iseq_get_used_vconst_regs(ppc_insn_t const *iseq, long iseq_len, regset_t *use)
{
  NOT_IMPLEMENTED();
}

/*
int
ppc_iseq_disassemble(ppc_insn_t *ppc_iseq, uint8_t *bincode, int binsize,
    vconstants_t vconstants[][MAX_NUM_VCONSTANTS_PER_INSN], src_ulong *nextpcs,
    long *num_nextpcs, i386_as_mode_t i386_as_mode)
{
  NOT_IMPLEMENTED();
}
*/

bool
ppc_iseq_types_equal(struct ppc_insn_t const *iseq1, long iseq_len1,
    struct ppc_insn_t const *iseq2, long iseq_len2)
{
  NOT_IMPLEMENTED();
}

long
ppc_insn_put_movl_nextpc_constant_to_mem(long nextpc_num, uint32_t addr,
    struct ppc_insn_t *insns, size_t insns_size)
{
  NOT_IMPLEMENTED();
}

long
ppc_operand_strict_canonicalize_regs(ppc_strict_canonicalization_state_t *scanon_state,
    long insn_index, long op_index,
    struct ppc_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map, long num_iseq_var, long max_num_iseq_var/*,
    bool canonicalize_imms_using_syms, struct input_exec_t *in*/)
{
  long c, i, n, num_exregs_used[PPC_NUM_EXREG_GROUPS];
  ppc_insn_t *insn;
  regmap_t *st_map;

  ASSERT(op_index == 0); //there is only one "operand" in ppc insn

  ASSERT(num_iseq_var <= max_num_iseq_var);
  for (n = 0; n < num_iseq_var; n++) {
    for (i = 0; i < PPC_NUM_EXREG_GROUPS; i++) {
      num_exregs_used[i] = regmap_count_exgroup_regs(&st_regmap[n], i);
    }

    insn = &iseq_var[n][insn_index];
    //i386_operand_copy(var_op, op);
    st_regmap = &st_regmap[n];

    if (ppc_insn_is_nop(insn)) {
      continue;
    }
    PPC_INSN_STRICT_CANONICALIZE_EXREG(rSD.rS.base, PPC_EXREG_GROUP_GPRS);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(frSD.frS.base, PPC_EXREG_GROUP_FPREG);
  
    PPC_INSN_STRICT_CANONICALIZE_EXREG(rA, PPC_EXREG_GROUP_GPRS);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(rB, PPC_EXREG_GROUP_GPRS);
  
    PPC_INSN_STRICT_CANONICALIZE_EXREG(crfS, PPC_EXREG_GROUP_CRF);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(crfD, PPC_EXREG_GROUP_CRF);
  
    PPC_INSN_STRICT_CANONICALIZE_CRB(crbA);
    PPC_INSN_STRICT_CANONICALIZE_CRB(crbB);
    PPC_INSN_STRICT_CANONICALIZE_CRB(crbD);
    if (ppc_insn_bo_indicates_bi_use(insn)) {
      PPC_INSN_STRICT_CANONICALIZE_CRB(BI);
    }
  
    PPC_INSN_STRICT_CANONICALIZE_EXREG(crf0, PPC_EXREG_GROUP_CRF);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(i_crf0_so, PPC_EXREG_GROUP_CRF_SO);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(i_crfS_so, PPC_EXREG_GROUP_CRF_SO);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(i_crfD_so, PPC_EXREG_GROUP_CRF_SO);
    for (c = 0; c < insn->valtag.num_i_crfs; c++) {
      PPC_INSN_STRICT_CANONICALIZE_EXREG(i_crfs[c], PPC_EXREG_GROUP_CRF);
    }
    for (c = 0; c < insn->valtag.num_i_crfs_so; c++) {
      PPC_INSN_STRICT_CANONICALIZE_EXREG(i_crfs_so[c], PPC_EXREG_GROUP_CRF_SO);
    }
    for (c = 0; c < insn->valtag.num_i_sc_regs; c++) {
      PPC_INSN_STRICT_CANONICALIZE_EXREG(i_sc_regs[c], PPC_EXREG_GROUP_GPRS);
    }
    PPC_INSN_STRICT_CANONICALIZE_EXREG(lr0, PPC_EXREG_GROUP_LR);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(ctr0, PPC_EXREG_GROUP_CTR);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(xer_ov0, PPC_EXREG_GROUP_XER_OV);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(xer_so0, PPC_EXREG_GROUP_XER_SO);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(xer_ca0, PPC_EXREG_GROUP_XER_CA);
    PPC_INSN_STRICT_CANONICALIZE_EXREG(xer_bc_cmp0, PPC_EXREG_GROUP_XER_BC_CMP);
  }
  return num_iseq_var;
}

long
ppc_insn_put_movl_reg_var_to_mem(long addr, int regvar, int r_ds,
    struct ppc_insn_t *buf, size_t size)
{
  NOT_IMPLEMENTED();
}

long
ppc_insn_put_jump_to_indir_mem(long addr, ppc_insn_t *insns, size_t insns_size)
{
  NOT_IMPLEMENTED();
}

long
ppc_insn_sandbox(struct ppc_insn_t *dst, size_t dst_size,
    struct ppc_insn_t const *src,
    struct regmap_t const *c2v, struct regset_t *ctemps, int tmpreg, long insn_index)
{
  NOT_IMPLEMENTED();
}

long
ppc_insn_put_movl_mem_to_reg_var(long regno, long addr, int r_ds,
    struct ppc_insn_t *insns, size_t insns_size)
{
  NOT_IMPLEMENTED();
}

bool
ppc_iseqs_equal_mod_imms(struct ppc_insn_t const *iseq1, long iseq1_len,
    struct ppc_insn_t const *iseq2, long iseq2_len)
{
  NOT_IMPLEMENTED();
}

void
ppc_insn_canonicalize_locals_symbols(ppc_insn_t *insn, imm_vt_map_t *imm_vt_map, src_tag_t tag)
{
  if (insn->valtag.imm_signed == 1) {
    valtag_canonicalize_local_symbol(&insn->valtag.imm.simm, imm_vt_map, tag);
  } else if (insn->valtag.imm_signed == 0) {
    valtag_canonicalize_local_symbol(&insn->valtag.imm.uimm, imm_vt_map, tag);
  }
}

bool
symbol_is_contained_in_ppc_insn(struct ppc_insn_t const *insn, long symval, src_tag_t symtag)
{
  NOT_IMPLEMENTED();
}

bool
ppc_insn_symbols_are_contained_in_map(struct ppc_insn_t const *insn,
    struct imm_vt_map_t const *imm_vt_map)
{
  NOT_IMPLEMENTED();
}

void
ppc_insn_rename_addresses_to_symbols(struct ppc_insn_t *insn,
    struct input_exec_t const *in, src_ulong pc)
{
  NOT_IMPLEMENTED();
}

void
ppc_insn_copy_symbols(struct ppc_insn_t *dst, struct ppc_insn_t const *src)
{
  NOT_IMPLEMENTED();
}

size_t
ppc_insn_get_symbols(struct ppc_insn_t const *insn, long const *start_symbol_ids, long *symbol_ids, size_t max_symbol_ids)
{
  NOT_IMPLEMENTED();
}

void
ppc_insn_fetch_preprocess(ppc_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long size)
{
}

bool
ppc_imm_operand_needs_canonicalization(opc_t opc, int explicit_op_index)
{
  return true;
}

regset_t const *
ppc_regset_live_at_jumptable()
{
  return regset_all();
}

struct ppc_strict_canonicalization_state_t *
ppc_strict_canonicalization_state_init()
{
  return NULL;
}

void
ppc_strict_canonicalization_state_free(struct ppc_strict_canonicalization_state_t *)
{
}

long
ppc_insn_get_max_num_imm_operands(ppc_insn_t const *insn)
{
  return PPC_MAX_NUM_OPERANDS;
}

vector<valtag_t>
ppc_insn_get_pcrel_values(ppc_insn_t const *insn)
{
  uint64_t pcrel_val;
  src_tag_t pcrel_tag;
  bool ret;
  ret = ppc_insn_get_pcrel_value(insn, &pcrel_val, &pcrel_tag);
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
ppc_insn_set_pcrel_values(ppc_insn_t *insn, vector<valtag_t> const &vs)
{
  ASSERT(vs.size() <= 1);
  if (vs.size() == 0) {
    return;
  }
  ASSERT(vs.size() == 1);
  uint64_t pcrel_val = vs.at(0).val;
  src_tag_t pcrel_tag = vs.at(0).tag;
  ppc_insn_set_pcrel_value(insn, pcrel_val, pcrel_tag);
}

valtag_t *
ppc_insn_get_pcrel_operands(ppc_insn_t const *insn)
{
  return ppc_insn_get_pcrel_operand(insn);
}

void
ppc_pc_get_function_name_and_insn_num(input_exec_t const *in, src_ulong pc, string &function_name, long &insn_num_in_function)
{
  //this function is only meaningful for etfg
  function_name = "ppc";
  insn_num_in_function = -1;
}

size_t
ppc_insn_put_function_return(ppc_insn_t *insn)
{
  NOT_IMPLEMENTED();
}

size_t
ppc_insn_put_invalid(ppc_insn_t *insn)
{
  NOT_IMPLEMENTED();
}

size_t
ppc_insn_put_ret_insn(ppc_insn_t *insn, int retval)
{
  NOT_IMPLEMENTED();
}

size_t
ppc_emit_translation_marker_insns(ppc_insn_t *buf, size_t buf_size)
{
  NOT_IMPLEMENTED();
}
