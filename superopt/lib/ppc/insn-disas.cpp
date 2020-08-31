#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <map>

#include "support/debug.h"
#include "support/c_utils.h"
#include "ppc/regs.h"
#include "ppc/insn.h"
#include "valtag/memset.h"

static char as1[4096];
static char as2[4096];


/***                           Instruction decoding                        ***/
#define OPCODE_SET_HELPER(name, shift, nb) \
uint32_t set_##name (uint32_t opcode, uint32_t val); \
uint32_t set_##name (uint32_t opcode, uint32_t val) \
{ \
  return ((opcode & ~(((1 << (nb)) - 1) << (shift))) \
          | ((val & ((1 << (nb)) - 1)) << shift)); \
}

#define EXTRACT_HELPER(name, shift, nb)                                       \
uint32_t name (uint32_t opcode); \
uint32_t name (uint32_t opcode)                                 \
{                                                                             \
    return (opcode >> (shift)) & ((1 << (nb)) - 1);                           \
} \
OPCODE_SET_HELPER(name, shift, nb)

#define EXTRACT_SHELPER(name, shift, nb)                                      \
int32_t name (uint32_t opcode);\
int32_t name (uint32_t opcode)                                  \
{                                                                             \
    return (int16_t)((opcode >> (shift)) & ((1 << (nb)) - 1));                \
} \
OPCODE_SET_HELPER(name, shift, nb)


/* Opcode part 1 */
EXTRACT_HELPER(opc1, 26, 6);
/* Opcode part 2 */
EXTRACT_HELPER(opc2, 1, 5);
/* Opcode part 3 */
EXTRACT_HELPER(opc3, 6, 5);
/* Update Cr0 flags */
EXTRACT_HELPER(Rc, 0, 1);
/* Destination */
EXTRACT_HELPER(rD, 21, 5);
/* Source */
EXTRACT_HELPER(rS, 21, 5);
/* First operand */
EXTRACT_HELPER(rA, 16, 5);
/* Second operand */
EXTRACT_HELPER(rB, 11, 5);
/* Third operand */
EXTRACT_HELPER(rC, 6, 5);
/***                               Get CRn                                 ***/
EXTRACT_HELPER(crfD, 23, 3);
EXTRACT_HELPER(crfS, 18, 3);
EXTRACT_HELPER(crbD, 21, 5);
EXTRACT_HELPER(crbA, 16, 5);
EXTRACT_HELPER(crbB, 11, 5);
/* SPR / TBL */
EXTRACT_HELPER(_SPR, 11, 10);
uint32_t SPR (uint32_t opcode);
uint32_t SPR (uint32_t opcode)
{
    uint32_t sprn = _SPR(opcode);

    return ((sprn >> 5) & 0x1F) | ((sprn & 0x1F) << 5);
}
uint32_t set_SPR (uint32_t opcode, uint32_t val);
uint32_t set_SPR (uint32_t opcode, uint32_t val)
{
    uint32_t sprn = ((val >> 5) & 0x1F) | ((val & 0x1F) << 5);
    return set__SPR(opcode, sprn);
}

/***                              Get constants                            ***/
EXTRACT_HELPER(IMM, 12, 8);
/* 16 bits signed immediate value */
//EXTRACT_SHELPER(SIMM, 0, 16);
EXTRACT_HELPER(SIMM, 0, 16);
/* 16 bits unsigned immediate value */
EXTRACT_HELPER(UIMM, 0, 16);
/* Bit count */
EXTRACT_HELPER(NB, 11, 5);
/* Shift count */
EXTRACT_HELPER(SH, 11, 5);
/* Mask start */
EXTRACT_HELPER(MB, 6, 5);
/* Mask end */
EXTRACT_HELPER(ME, 1, 5);
/* Trap operand */
EXTRACT_HELPER(TO, 21, 5);

EXTRACT_HELPER(CRM, 12, 8);
EXTRACT_HELPER(FM, 17, 8);
EXTRACT_HELPER(SR, 16, 4);
EXTRACT_HELPER(FPIMM, 20, 4);

/***                            Jump target decoding                       ***/
/* Displacement */
//EXTRACT_SHELPER(d, 0, 16);
EXTRACT_HELPER(d, 0, 16);
/* Immediate address */
uint32_t LI (uint32_t opcode);
uint32_t LI (uint32_t opcode)
{
    return (opcode >> 0) & 0x03FFFFFC;
    //return SIGN_EXT((opcode >> 0) & 0x03FFFFFC, 26);
}
uint32_t set_LI (uint32_t opcode, uint32_t val);
uint32_t set_LI (uint32_t opcode, uint32_t val)
{
    return ((opcode & ~0x03FFFFFC) | (val & 0x03FFFFFC));
    //return (opcode >> 0) & 0x03FFFFFC;
}

uint32_t BD(uint32_t opcode);
uint32_t BD(uint32_t opcode)
{
    //printf("opcode=%x, BD=%x\n", opcode, (opcode >> 0) &  0xFFFC);
    return (opcode >> 0) & 0xFFFC;
    //return SIGN_EXT((opcode >> 0) & 0xFFFC, 16);
}
uint32_t set_BD (uint32_t opcode, uint32_t val);
uint32_t set_BD (uint32_t opcode, uint32_t val)
{
    return ((opcode & ~0xFFFC) | (val & 0xFFFC));
}

EXTRACT_HELPER(BO, 21, 5);
EXTRACT_HELPER(BI, 16, 5);
/* Absolute/relative address */
EXTRACT_HELPER(AA, 1, 1);
/* Link */
EXTRACT_HELPER(LK, 0, 1);

/* Create a mask between <start> and <end> bits */
static uint32_t MASK32(uint32_t start, uint32_t end)
{
    uint32_t ret;

    ret = (((uint32_t)(-1)) >> (start)) ^ (((uint32_t)(-1) >> (end)) >> 1);
    if (start > end)
        return ~ret;

    return ret;
}

#define OPCODE_SET_VAL(FIELD, opcode, insn) do { \
  if (insn->valtag.FIELD.tag != tag_invalid) { \
    opcode = set_##FIELD(opcode, insn->valtag.FIELD.val); \
  } \
} while (0)

#define OPCODE_SET(FIELD, opcode, insn) do { \
  if (insn->FIELD != -1) { \
    opcode = set_##FIELD(opcode, insn->FIELD); \
  } \
} while (0)

/* This function is experimental. Not yet tested... */
void
ppc_insn_get_bincode(ppc_insn_t const *insn, uint8_t *bincode)
{
  uint32_t opcode = 0x0;

  DBG(SRC_USEDEF, "insn->Rc=%d, BI=%ld\n", insn->Rc, insn->valtag.BI.val);
  OPCODE_SET(opc1, opcode, insn);
  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);
  OPCODE_SET(SPR, opcode, insn);
  OPCODE_SET(AA, opcode, insn);
  OPCODE_SET_VAL(BD, opcode, insn);
  OPCODE_SET_VAL(BI, opcode, insn);
  OPCODE_SET(BO, opcode, insn);
  OPCODE_SET_VAL(crbA, opcode, insn);
  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);
  OPCODE_SET_VAL(crbB, opcode, insn);
  OPCODE_SET_VAL(crbD, opcode, insn);
  OPCODE_SET_VAL(crfS, opcode, insn);
  OPCODE_SET_VAL(crfD, opcode, insn);
  //OPCODE_SET_VAL(CRM, opcode, insn);
  if (insn->valtag.CRM != -1) {
    opcode = set_CRM(opcode, insn->valtag.CRM);
  }
  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);
  //XXX: OPCODE_SET_d(opcode, insn);
  OPCODE_SET(FM, opcode, insn);
  //OPCODE_SET(frA, opcode, insn);
  //OPCODE_SET(frB, opcode, insn);
  //OPCODE_SET(frC, opcode, insn);
  //OPCODE_SET(frD, opcode, insn);
  //OPCODE_SET(frS, opcode, insn);
  //OPCODE_SET(FP_IMM, opcode, insn);
  OPCODE_SET_VAL(LI, opcode, insn);
  OPCODE_SET(LK, opcode, insn);
  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);
  OPCODE_SET_VAL(MB, opcode, insn);
  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);
  OPCODE_SET_VAL(ME, opcode, insn);
  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);
  OPCODE_SET_VAL(SH, opcode, insn);
  OPCODE_SET_VAL(NB, opcode, insn);
  //OPCODE_SET(OE, opcode, insn);
  OPCODE_SET_VAL(rA, opcode, insn);
  OPCODE_SET_VAL(rB, opcode, insn);
  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);

  if (insn->valtag.rSD.rS.base.tag != tag_invalid) {
    opcode = set_rS(opcode, insn->valtag.rSD.rS.base.val);
  }
  if (insn->valtag.frSD.frS.base.tag != tag_invalid) {
    opcode = set_rS(opcode, insn->valtag.frSD.frS.base.val);
  }


  OPCODE_SET(Rc, opcode, insn);
  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);
  //OPCODE_SET(IMM, opcode, insn);
  if (insn->valtag.imm.simm.tag != tag_invalid) {
    //if (insn->tag.imm.simm == tag_const) {
    opcode = insn->valtag.imm_signed ? set_SIMM(opcode, insn->valtag.imm.simm.val)
                                     : set_UIMM(opcode, insn->valtag.imm.uimm.val);
    /*} else {
      opcode = set_UIMM(opcode, VCONSTANT_PLACEHOLDER & 0xffff);
    }*/
  }
  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);
  OPCODE_SET(SR, opcode, insn);
  //OPCODE_SET(TO, opcode, insn);
  OPCODE_SET(opc2, opcode, insn);
  OPCODE_SET(opc3, opcode, insn);

  opcode |= insn->reserved_mask;

  DBG(SRC_USEDEF, "opcode=0x%x\n", opcode);
  memcpy(bincode, &opcode, sizeof opcode);
  //return opcode;
}




typedef struct opc_handler_t {
    /* invalid bits */
    uint32_t inval;
    /* instruction type */
    uint32_t type;
    /* handler */
    //void (*handler)(DisasContext *ctx);
    /* disassembler */
    void (*disassembler)(uint32_t opcode, struct ppc_insn_t *insn);
    /* compute use-def sets */
    /*void (*usedef)(uint32_t opcode, struct regset_t *use,
        struct regset_t *def, struct memset_t *memuse,
        struct memset_t *memdef);*/
} opc_handler_t;

typedef struct opcode_t {
    unsigned char opc1, opc2, opc3;
#if DST_LONG_BITS == 64 /* Explicitely align to 64 bits */
    unsigned char pad[5];
#else
    unsigned char pad[1];
#endif
    opc_handler_t handler;
    const char *oname;
} opcode_t;

#define DISAS_HANDLER(name)                 \
static void disas_##name(uint32_t opcode, ppc_insn_t *insn)

#define DISAS_OPCODE(name, op1, op2, op3, invl, _typ)                         \
opcode_t disas_##name = {                                     \
    .opc1 = op1,                                                              \
    .opc2 = op2,                                                              \
    .opc3 = op3,                                                              \
    .pad  = { 0, },                                                           \
    .handler = {                                                              \
        .inval   = invl,                                                      \
        .type = _typ,                                                         \
        /*.handler = &gen_##name,*/                                             \
        .disassembler = &disas_##name,					      \
        /*.usedef = &usedef_##name,*/					      \
    },                                                                        \
    .oname = stringify(name),                                                 \
}

#define DISAS_OPCODE_MARK(name)                                                 \
opcode_t opc_##name = {                                       \
    .opc1 = 0xFF,                                                             \
    .opc2 = 0xFF,                                                             \
    .opc3 = 0xFF,                                                             \
    .pad  = { 0, },                                                           \
    .handler = {                                                              \
        .inval   = 0x00000000,                                                \
        .type = 0x00,                                                         \
        /* .handler = NULL,     */                                                 \
        .disassembler = NULL,						      \
        /*.usedef = NULL,*/						      \
    },                                                                        \
    .oname = stringify(name),                                                 \
}

//#include "ppc/usedef.c"

/* #if DST_LONG_BITS == 64
#define OPC_ALIGN 8
#else
#define OPC_ALIGN 4
#endif
#if defined(__APPLE__)
#define OPCODES_SECTION \
    __attribute__ ((section("__TEXT,__opcodes"), unused, aligned (OPC_ALIGN) ))
#else
#define OPCODES_SECTION \
    __attribute__ ((section(".opcodes"), unused, aligned (OPC_ALIGN) ))
#endif */

/* Invalid instruction */
DISAS_HANDLER(invalid)
{
  ASSERT(0);
}

/***                           Integer arithmetic                          ***/
#define __DISAS_INT_ARITH2(name)               				\
DISAS_HANDLER(name)            \
{                                                                       \
    insn->valtag.rA.val = rA(opcode);						\
    insn->valtag.rB.val = rB(opcode);						\
    insn->valtag.rSD.rD.base.val = rD(opcode);						\
    insn->Rc = Rc(opcode);						\
    insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
}

#define __DISAS_INT_ARITH2_O(name)               				\
DISAS_HANDLER(name)            \
{                                                                       \
    insn->valtag.rA.val = rA(opcode);						\
    insn->valtag.rB.val = rB(opcode);						\
    insn->valtag.rSD.rD.base.val = rD(opcode);						\
    insn->Rc = Rc(opcode);						\
    insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
    insn->valtag.xer_ov0.val = 0; \
    insn->valtag.xer_ov0.tag = tag_const; \
    insn->valtag.xer_so0.val = 0; \
    insn->valtag.xer_so0.tag = tag_const; \
}

#define __DISAS_INT_ARITH2c(name)               				\
DISAS_HANDLER(name)            \
{                                                                       \
    insn->valtag.rA.val = rA(opcode);						\
    insn->valtag.rB.val = rB(opcode);						\
    insn->valtag.rSD.rD.base.val = rD(opcode);						\
    insn->Rc = Rc(opcode);						\
    insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
    insn->valtag.xer_ca0.val = 0; \
    insn->valtag.xer_ca0.tag = tag_const; \
}

#define __DISAS_INT_ARITH2c_O(name)               				\
DISAS_HANDLER(name)            \
{                                                                       \
    insn->valtag.rA.val = rA(opcode);						\
    insn->valtag.rB.val = rB(opcode);						\
    insn->valtag.rSD.rD.base.val = rD(opcode);						\
    insn->Rc = Rc(opcode);						\
    insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
    insn->valtag.xer_ca0.val = 0; \
    insn->valtag.xer_ca0.tag = tag_const; \
    insn->valtag.xer_ov0.val = 0; \
    insn->valtag.xer_ov0.tag = tag_const; \
    insn->valtag.xer_so0.val = 0; \
    insn->valtag.xer_so0.tag = tag_const; \
}

#define __DISAS_INT_ARITH1(name)                            \
DISAS_HANDLER(name)            \
{                                                                             \
    insn->valtag.rA.val = rA(opcode);						\
    insn->valtag.rSD.rD.base.val = rD(opcode);						\
    insn->Rc = Rc(opcode);						\
    insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
}

#define __DISAS_INT_ARITH1_O(name)                            \
DISAS_HANDLER(name)            \
{                                                                             \
    insn->valtag.rA.val = rA(opcode);						\
    insn->valtag.rSD.rD.base.val = rD(opcode);						\
    insn->Rc = Rc(opcode);						\
    insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
    insn->valtag.xer_ov0.val = 0; \
    insn->valtag.xer_ov0.tag = tag_const; \
    insn->valtag.xer_so0.val = 0; \
    insn->valtag.xer_so0.tag = tag_const; \
}

#define __DISAS_INT_ARITH1e(name)                            \
DISAS_HANDLER(name)            \
{                                                                             \
    insn->valtag.rA.val = rA(opcode);						\
    insn->valtag.rSD.rD.base.val = rD(opcode);						\
    insn->Rc = Rc(opcode);						\
    insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
    insn->valtag.xer_ca0.val = 0; \
    insn->valtag.xer_ca0.tag = tag_const; \
}

#define __DISAS_INT_ARITH1e_O(name)                            \
DISAS_HANDLER(name)            \
{                                                                             \
    insn->valtag.rA.val = rA(opcode);						\
    insn->valtag.rSD.rD.base.val = rD(opcode);						\
    insn->Rc = Rc(opcode);						\
    insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
    insn->valtag.xer_ca0.val = 0; \
    insn->valtag.xer_ca0.tag = tag_const; \
    insn->valtag.xer_ov0.val = 0; \
    insn->valtag.xer_ov0.tag = tag_const; \
    insn->valtag.xer_so0.val = 0; \
    insn->valtag.xer_so0.tag = tag_const; \
}


/* Two operands arithmetic functions */
#define DISAS_INT_ARITH2(name)                            	\
__DISAS_INT_ARITH2(name)                       			\
__DISAS_INT_ARITH2_O(name##o)

#define DISAS_INT_ARITH2c(name)                            	\
__DISAS_INT_ARITH2c(name)                       			\
__DISAS_INT_ARITH2c_O(name##o)

#define DISAS_INT_ARITH2e DISAS_INT_ARITH2c

/* Two operands arithmetic functions (immediate operand) */
#define DISAS_INT_ARITH2i(name)                            	\
DISAS_HANDLER(name)						\
{								\
  insn->valtag.imm_signed = 1;						\
  insn->valtag.imm.simm.val = SIMM(opcode);				\
  insn->valtag.imm.simm.tag = tag_const; \
								\
  if (rA(opcode) != 0) {								\
    insn->valtag.rA.val = rA(opcode);					\
    insn->valtag.rA.tag = tag_const; \
  }								\
  insn->valtag.rSD.rD.base.val = rD(opcode);					\
  insn->valtag.rSD.rD.base.tag = tag_const; \
}
/* Two operands arithmetic functions (immediate operand) */
#define DISAS_INT_ARITH2i_c(name)                            	\
DISAS_HANDLER(name)						\
{								\
  insn->valtag.imm_signed = 1;						\
  insn->valtag.imm.simm.val = SIMM(opcode);				\
  insn->valtag.rA.val = rA(opcode);					\
  insn->valtag.rSD.rD.base.val = rD(opcode);					\
  insn->valtag.imm.simm.tag = tag_const; \
  insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
}

#define DISAS_INT_ARITH2i_c_(name)                            	\
DISAS_HANDLER(name)						\
{								\
  insn->valtag.imm_signed = 1;						\
  insn->valtag.imm.simm.val = SIMM(opcode);				\
  insn->valtag.rA.val = rA(opcode);					\
  insn->valtag.rSD.rD.base.val = rD(opcode);					\
  insn->Rc = 1;							\
  insn->valtag.imm.simm.tag = insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = tag_const; \
}

/* Two operands arithmetic functions with no overflow allowed */
#define DISAS_INT_ARITHN(name)                          \
__DISAS_INT_ARITH2(name)

/* One operand arithmetic functions */
#define DISAS_INT_ARITH1(name)                            \
__DISAS_INT_ARITH1(name)                                    \
__DISAS_INT_ARITH1_O(name##o)

#define DISAS_INT_ARITH1e(name)                            \
__DISAS_INT_ARITH1e(name)                                    \
__DISAS_INT_ARITH1e_O(name##o)


/* add    add.    addo    addo.    */
DISAS_INT_ARITH2 (add);
/* addc   addc.   addco   addco.   */
DISAS_INT_ARITH2c (addc);
/* adde   adde.   addeo   addeo.   */
DISAS_INT_ARITH2e (adde);
/* addme  addme.  addmeo  addmeo.  */
DISAS_INT_ARITH1e (addme);
/* addze  addze.  addzeo  addzeo.  */
DISAS_INT_ARITH1e (addze);
/* divw   divw.   divwo   divwo.   */
DISAS_INT_ARITH2 (divw);
/* divwu  divwu.  divwuo  divwuo.  */
DISAS_INT_ARITH2 (divwu);
/* mulhw  mulhw.                   */
DISAS_INT_ARITHN (mulhw);
/* mulhwu mulhwu.                  */
DISAS_INT_ARITHN (mulhwu);
/* mullw  mullw.  mullwo  mullwo.  */
DISAS_INT_ARITH2 (mullw);
/* neg    neg.    nego    nego.    */
DISAS_INT_ARITH1 (neg);
/* subf   subf.   subfo   subfo.   */
DISAS_INT_ARITH2 (subf);
/* subfc  subfc.  subfco  subfco.  */
DISAS_INT_ARITH2c (subfc);
/* subfe  subfe.  subfeo  subfeo.  */
DISAS_INT_ARITH2e (subfe);
/* subfme subfme. subfmeo subfmeo. */
DISAS_INT_ARITH1e (subfme);
/* subfze subfze. subfzeo subfzeo. */
DISAS_INT_ARITH1e (subfze);

/* addi */
DISAS_INT_ARITH2i(addi);

/* addic */
DISAS_HANDLER(addic)
{
  insn->valtag.imm_signed = 1;
  insn->valtag.imm.simm.val = SIMM(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.imm.simm.tag = tag_const;
  insn->valtag.rSD.rD.base.tag = tag_const;
  insn->valtag.xer_ca0.val = 0;
  insn->valtag.xer_ca0.tag = tag_const;
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rA.tag = tag_const;
}

/* addic. */
DISAS_HANDLER(addic_)
{
  insn->valtag.imm_signed = 1;
  insn->valtag.imm.simm.val = SIMM(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->Rc = 1;
  insn->valtag.imm.simm.tag = insn->valtag.rSD.rD.base.tag = tag_const;
  insn->valtag.xer_ca0.val = 0;
  insn->valtag.xer_ca0.tag = tag_const;
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rA.tag = tag_const;
}

/* addis */
DISAS_INT_ARITH2i(addis);
/* mulli */
DISAS_INT_ARITH2i_c(mulli);
/* subfic */
DISAS_HANDLER(subfic)
{
  insn->valtag.imm_signed = 1;
  insn->valtag.imm.simm.val = SIMM(opcode);
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.imm.simm.tag = tag_const;
  insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = tag_const;
  insn->valtag.xer_ca0.val = 0;
  insn->valtag.xer_ca0.tag = tag_const;
}


/***                           Integer comparison                          ***/
#define DISAS_CMP(name)                                                   \
DISAS_HANDLER(name)      					        \
{                                                                             \
  insn->valtag.rA.val = rA(opcode);						      \
  insn->valtag.rB.val = rB(opcode);						      \
  insn->valtag.crfD.val = crfD(opcode);					      \
  insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.crfD.tag = tag_const; \
}

/* cmp */
DISAS_CMP(cmp);
/* cmpi */
DISAS_HANDLER(cmpi)
{
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.imm_signed = 1;						
  insn->valtag.imm.simm.val = SIMM(opcode);
  insn->valtag.crfD.val = crfD(opcode);					      
  insn->valtag.rA.tag = insn->valtag.imm.simm.tag = insn->valtag.crfD.tag = tag_const;
}
/* cmpl */
DISAS_CMP(cmpl);
/* cmpli */
DISAS_HANDLER(cmpli)
{
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.imm_signed = 0;
  insn->valtag.imm.uimm.val = UIMM(opcode);
  insn->valtag.crfD.val = crfD(opcode);
  insn->valtag.rA.tag = insn->valtag.imm.uimm.tag = insn->valtag.crfD.tag = tag_const;
}

/***                            Integer logical                            ***/
#define __DISAS_LOGICAL2(name)                                      \
DISAS_HANDLER(name)							     \
{                                                                             \
  insn->valtag.rSD.rS.base.val = rS(opcode);						      \
  insn->valtag.rB.val = rB(opcode); 						\
  insn->Rc = Rc(opcode); 						\
  insn->valtag.rA.val = rA(opcode); 						\
  insn->valtag.rSD.rS.base.tag = insn->valtag.rB.tag = insn->valtag.rA.tag = tag_const; \
}
#define DISAS_LOGICAL2(name)                                             \
__DISAS_LOGICAL2(name)

#define DISAS_LOGICAL2i(name)						\
DISAS_HANDLER(name)						\
{									\
  insn->valtag.rSD.rS.base.val = rS(opcode);						\
  insn->valtag.imm_signed = 0;							\
  insn->valtag.imm.uimm.val = UIMM(opcode);					\
  insn->valtag.rA.val = rA(opcode);						\
  insn->valtag.rSD.rS.base.tag = insn->valtag.imm.uimm.tag = insn->valtag.rA.tag = tag_const; \
}

#define DISAS_LOGICAL2i_(name)						\
DISAS_HANDLER(name)						\
{									\
  insn->valtag.rSD.rS.base.val = rS(opcode);						\
  insn->valtag.imm_signed = 0;							\
  insn->valtag.imm.uimm.val = UIMM(opcode);					\
  insn->valtag.rA.val = rA(opcode);						\
  insn->Rc = 1;								\
  insn->valtag.rSD.rS.base.tag = insn->valtag.imm.uimm.tag = insn->valtag.rA.tag = tag_const; \
}


#define DISAS_LOGICAL1(name)                                            \
DISAS_HANDLER(name)                   \
{                                                                             \
  insn->valtag.rSD.rS.base.val = rS(opcode);						      \
  insn->Rc = Rc(opcode); \
  insn->valtag.rA.val = rA(opcode); \
  insn->valtag.rSD.rS.base.tag = insn->valtag.rA.tag = tag_const; \
}

/* and & and. */
DISAS_LOGICAL2(and);
/* andc & andc. */
DISAS_LOGICAL2(andc);
/* andi. */
DISAS_LOGICAL2i_(andi_);
/* andis. */
DISAS_LOGICAL2i_(andis_);

/* cntlzw */
DISAS_LOGICAL1(cntlzw);
/* eqv & eqv. */
DISAS_LOGICAL2(eqv);
/* extsb & extsb. */
DISAS_LOGICAL1(extsb);
/* extsh & extsh. */
DISAS_LOGICAL1(extsh);
/* nand & nand. */
DISAS_LOGICAL2(nand);
/* nor & nor. */
DISAS_LOGICAL2(nor);

/* or & or. */
DISAS_LOGICAL2(or);

/* orc & orc. */
DISAS_LOGICAL2(orc);
/* xor & xor. */
DISAS_LOGICAL2(xor);

/* ori */
DISAS_LOGICAL2i(ori);
/* oris */
DISAS_LOGICAL2i(oris);
/* xori */
DISAS_LOGICAL2i(xori);
/* xoris */
DISAS_LOGICAL2i(xoris);

/***                             Integer rotate                            ***/
/* rlwimi & rlwimi. */
DISAS_HANDLER(rlwimi)
{
  insn->valtag.MB.val = MB(opcode);
  insn->valtag.ME.val = ME(opcode);
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.SH.val = SH(opcode);
  insn->Rc = Rc(opcode);

  insn->valtag.MB.tag = tag_const;
  insn->valtag.ME.tag = tag_const;
  insn->valtag.rSD.rS.base.tag = tag_const;
  insn->valtag.rA.tag = tag_const;
  insn->valtag.SH.tag = tag_const;
}
/* rlwinm & rlwinm. */
DISAS_HANDLER(rlwinm)
{
  insn->valtag.MB.val = MB(opcode);
  insn->valtag.ME.val = ME(opcode);
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.SH.val = SH(opcode);
  insn->Rc = Rc(opcode);

  insn->valtag.MB.tag = tag_const;
  insn->valtag.ME.tag = tag_const;
  insn->valtag.rSD.rS.base.tag = tag_const;
  insn->valtag.rA.tag = tag_const;
  insn->valtag.SH.tag = tag_const;
}

/* rlwnm & rlwnm. */
DISAS_HANDLER(rlwnm)
{
  insn->valtag.MB.val = MB(opcode);
  insn->valtag.ME.val = ME(opcode);
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->Rc = Rc(opcode);

  insn->valtag.MB.tag = tag_const;
  insn->valtag.ME.tag = tag_const;
  insn->valtag.rSD.rS.base.tag = tag_const;
  insn->valtag.rA.tag = tag_const;
  insn->valtag.rB.tag = tag_const;
}

/***                             Integer shift                             ***/
/* slw & slw. */
__DISAS_LOGICAL2(slw);
/* sraw & sraw. */
DISAS_HANDLER(sraw)
{
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->Rc = Rc(opcode);
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rSD.rS.base.tag = insn->valtag.rB.tag = insn->valtag.rA.tag = tag_const;
  insn->valtag.xer_ca0.val = 0;
  insn->valtag.xer_ca0.tag = tag_const;
}

/* srawi & srawi. */
DISAS_HANDLER(srawi)
{
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.SH.val = SH(opcode);
  insn->Rc = Rc(opcode);
  insn->valtag.rA.val = rA(opcode);

  insn->valtag.rSD.rS.base.tag = tag_const;
  insn->valtag.SH.tag = tag_const;
  insn->valtag.rA.tag = tag_const;

  insn->valtag.xer_ca0.val = 0;
  insn->valtag.xer_ca0.tag = tag_const;
}
/* srw & srw. */
__DISAS_LOGICAL2(srw);

/***                       Floating-Point arithmetic                       ***/
#define _DISAS_FLOAT_ACB(name) 			                             \
DISAS_HANDLER(f##name)                   				     \
{                                                                             \
  /*insn->is_fp_insn = 1;*/							     \
}

#define DISAS_FLOAT_ACB(name)                                              \
_DISAS_FLOAT_ACB(name);                                     		   \
_DISAS_FLOAT_ACB(name##s);

#define _DISAS_FLOAT_AB(name)                     			      \
DISAS_HANDLER(f##name)                        				      \
{                                                                             \
  /*insn->is_fp_insn = 1;	*/						     \
}
#define DISAS_FLOAT_AB(name)                                     \
_DISAS_FLOAT_AB(name);                            \
_DISAS_FLOAT_AB(name##s);

#define _DISAS_FLOAT_AC(name)     				             \
DISAS_HANDLER(f##name)                        				     \
{                                                                             \
  /*insn->is_fp_insn = 1;	*/						     \
}
#define DISAS_FLOAT_AC(name)               		                      \
_DISAS_FLOAT_AC(name);                    			         \
_DISAS_FLOAT_AC(name##s);

#define DISAS_FLOAT_B(name)                                         \
DISAS_HANDLER(f##name)                 \
{                                                                             \
  /*insn->is_fp_insn = 1;	*/		\
}

#define DISAS_FLOAT_BS(name)                                          \
DISAS_HANDLER(f##name)                   			      \
{                                                                             \
  /*insn->is_fp_insn = 1;*/					\
}

/* fadd - fadds */
DISAS_FLOAT_AB(add);
/* fdiv - fdivs */
DISAS_FLOAT_AB(div);
/* fmul - fmuls */
DISAS_FLOAT_AC(mul);

/* fres */
DISAS_FLOAT_BS(res);

/* frsqrte */
DISAS_FLOAT_BS(rsqrte);

/* fsel */
_DISAS_FLOAT_ACB(sel);
/* fsub - fsubs */
DISAS_FLOAT_AB(sub);
/* Optional: */
/* fsqrt */
DISAS_HANDLER(fsqrt)
{
  //insn->is_fp_insn = 1;
}

DISAS_HANDLER(fsqrts)
{
  //insn->is_fp_insn = 1;
}

/***                     Floating-Point multiply-and-add                   ***/
/* fmadd - fmadds */
DISAS_FLOAT_ACB(madd);
/* fmsub - fmsubs */
DISAS_FLOAT_ACB(msub);
/* fnmadd - fnmadds */
DISAS_FLOAT_ACB(nmadd);
/* fnmsub - fnmsubs */
DISAS_FLOAT_ACB(nmsub);

/***                     Floating-Point round & convert                    ***/
/* fctiw */
DISAS_FLOAT_B(ctiw);
/* fctiwz */
DISAS_FLOAT_B(ctiwz);
/* frsp */
DISAS_FLOAT_B(rsp);

/***                         Floating-Point compare                        ***/
/* fcmpo */
DISAS_HANDLER(fcmpo)
{
  //insn->is_fp_insn = 1;
}

/* fcmpu */
DISAS_HANDLER(fcmpu)
{
  //insn->is_fp_insn = 1;
}

/***                         Floating-point move                           ***/
/* fabs */
DISAS_FLOAT_B(abs);

/* fmr  - fmr. */
DISAS_HANDLER(fmr)
{
  //insn->is_fp_insn = 1;
  insn->Rc = Rc(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.rB.tag = tag_const;
  insn->valtag.rSD.rD.base.tag = tag_const;
}

/* fnabs */
DISAS_FLOAT_B(nabs);
/* fneg */
DISAS_FLOAT_B(neg);

/***                  Floating-Point status & ctrl register                ***/
/* mcrfs */
DISAS_HANDLER(mcrfs)
{
  //insn->is_fp_insn = 1;
  insn->valtag.crfS.val = crfS(opcode);
  insn->valtag.crfD.val = crfD(opcode);
  insn->valtag.crfS.tag = insn->valtag.crfD.tag = tag_const;
}

/* mffs */
DISAS_HANDLER(mffs)
{
  //insn->is_fp_insn = 1;
  insn->Rc = Rc(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.rSD.rD.base.tag = tag_const;
}

/* mtfsb0 */
DISAS_HANDLER(mtfsb0)
{
  //insn->is_fp_insn = 1;
  insn->Rc = Rc(opcode);
  insn->valtag.crbD.val = crbD(opcode);
  insn->valtag.crbD.tag = tag_const;
}

/* mtfsb1 */
DISAS_HANDLER(mtfsb1)
{
  //insn->is_fp_insn = 1;
  insn->Rc = Rc(opcode);
  insn->valtag.crbD.val = crbD(opcode);
  insn->valtag.crbD.tag = tag_const;
}

/* mtfsf */
DISAS_HANDLER(mtfsf)
{
  //insn->is_fp_insn = 1;
  insn->Rc = Rc(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->FM = FM(opcode);
  insn->valtag.rB.tag = tag_const;
}

/* mtfsfi */
DISAS_HANDLER(mtfsfi)
{
  //insn->is_fp_insn = 1;
  insn->valtag.crbD.val = crbD(opcode);
  insn->valtag.imm.fpimm.val = FPIMM(opcode);
  insn->Rc = Rc(opcode);
  insn->valtag.crbD.tag = tag_const;
  insn->valtag.imm.fpimm.tag = tag_const;
}

/***                             Integer load                              ***/
#define DISAS_LD(width)                                                  \
DISAS_HANDLER(l##width)             					 \
{                                                                        \
  insn->valtag.imm_signed = 1;						         \
  insn->valtag.imm.simm.val = SIMM(opcode);					 \
  insn->valtag.rSD.rD.base.val = rD(opcode);						 \
  insn->valtag.imm.simm.tag = tag_const; \
  insn->valtag.rSD.rD.base.tag = tag_const; \
  if (rA(opcode) != 0) { \
    insn->valtag.rA.val = rA(opcode);						   \
    insn->valtag.rA.tag = tag_const; \
  } \
}

#define DISAS_LDU(width)                                                 \
DISAS_HANDLER(l##width##u)						 \
{                                                                             \
  insn->valtag.imm_signed = 1;						         \
  insn->valtag.imm.simm.val = SIMM(opcode);					 \
  insn->valtag.rA.val = rA(opcode);						 \
  insn->valtag.rSD.rD.base.val = rD(opcode);						 \
  insn->valtag.imm.simm.tag = tag_const; \
  insn->valtag.rA.tag = tag_const; \
  insn->valtag.rSD.rD.base.tag = tag_const; \
}

#define DISAS_LDUX(width)                                                  \
DISAS_HANDLER(l##width##ux)           					   \
{                                                                          \
  insn->valtag.rA.val = rA(opcode);						   \
  insn->valtag.rSD.rD.base.val = rD(opcode);						   \
  insn->valtag.rB.val = rB(opcode);						   \
  insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = insn->valtag.rB.tag = tag_const; \
}

#define DISAS_LDX(width)                                            	   \
DISAS_HANDLER(l##width##x)           					   \
{                                                                             \
  insn->valtag.rSD.rD.base.val = rD(opcode);						   \
  insn->valtag.rB.val = rB(opcode);						   \
  insn->valtag.rSD.rD.base.tag = insn->valtag.rB.tag = tag_const; \
  if (rA(opcode) != 0) { \
    insn->valtag.rA.val = rA(opcode);						   \
    insn->valtag.rA.tag = tag_const; \
  } \
}

#define DISAS_LDS(width)                                                  \
DISAS_LD(width);                                                     \
DISAS_LDU(width);                                                    \
DISAS_LDUX(width);                                                   \
DISAS_LDX(width)

/* lbz lbzu lbzux lbzx */
DISAS_LDS(bz);
/* lha lhau lhaux lhax */
DISAS_LDS(ha);
/* lhz lhzu lhzux lhzx */
DISAS_LDS(hz);
/* lwz lwzu lwzux lwzx */
DISAS_LDS(wz);

/***                              Integer store                            ***/
#define DISAS_ST(width)                                                    \
DISAS_HANDLER(st##width)              \
{                                                                             \
  insn->valtag.imm_signed = 1;						         \
  insn->valtag.imm.simm.val = SIMM(opcode);				\
  insn->valtag.rSD.rS.base.val = rS(opcode);					\
  insn->valtag.imm.simm.tag = insn->valtag.rSD.rS.base.tag = tag_const; \
  if (rA(opcode) != 0) { \
    insn->valtag.rA.val = rA(opcode);					\
    insn->valtag.rA.tag = tag_const; \
  } \
}

#define DISAS_STU(width)                                                 \
DISAS_HANDLER(st##width##u)           					 \
{                                                               \
  insn->valtag.imm_signed = 1;						         \
  insn->valtag.imm.simm.val = SIMM(opcode);				\
  insn->valtag.rA.val = rA(opcode);					\
  insn->valtag.rSD.rS.base.val = rS(opcode);					\
  insn->valtag.imm.simm.tag = insn->valtag.rA.tag = insn->valtag.rSD.rS.base.tag = tag_const; \
}

#define DISAS_STUX(width)                                                  \
DISAS_HANDLER(st##width##ux)    					      \
{                                                                             \
  insn->valtag.rA.val = rA(opcode);                                               \
  insn->valtag.rB.val = rB(opcode);                                      	    \
  insn->valtag.rSD.rS.base.val = rS(opcode);                                      \
  insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rS.base.tag = tag_const; \
}

#define DISAS_STX(width)                                            \
DISAS_HANDLER(st##width##x)          				  \
{                                                                             \
  insn->valtag.rSD.rS.base.val = rS(opcode);						   \
  insn->valtag.rB.val = rB(opcode);						   \
  insn->valtag.rB.tag = insn->valtag.rSD.rS.base.tag = tag_const; \
  if (rA(opcode) != 0) { \
    insn->valtag.rA.val = rA(opcode);					\
    insn->valtag.rA.tag = tag_const; \
  } \
}

#define DISAS_STS(width)                                                    \
DISAS_ST(width);                                                     \
DISAS_STU(width);                                                    \
DISAS_STUX(width);                                                   \
DISAS_STX(width)

/* stb stbu stbux stbx */
DISAS_STS(b);
/* sth sthu sthux sthx */
DISAS_STS(h);
/* stw stwu stwux stwx */
DISAS_STS(w);

/***                Integer load and store with byte reverse               ***/
/* lhbrx */
DISAS_LDX(hbr);
/* lwbrx */
DISAS_LDX(wbr);
/* sthbrx */
DISAS_STX(hbr);
/* stwbrx */
DISAS_STX(wbr);

/***                    Integer load and store multiple                    ***/
/* lmw */
DISAS_HANDLER(lmw)
{
  insn->valtag.imm_signed = 1;
  insn->valtag.imm.simm.val = SIMM(opcode);
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.imm.simm.tag = insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = tag_const;
}

/* stmw */
DISAS_HANDLER(stmw)
{
  insn->valtag.imm_signed = 1;
  insn->valtag.imm.simm.val = SIMM(opcode);
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.imm.simm.tag = insn->valtag.rA.tag = insn->valtag.rSD.rS.base.tag = tag_const;
}

/***                    Integer load and store strings                     ***/
/* lswi */
/* PowerPC32 specification says we must generate an exception if
 * rA is in the range of registers to be loaded.
 * In an other hand, IBM says this is valid, but rA won't be loaded.
 * For now, I'll follow the spec...
 */
DISAS_HANDLER(lswi)
{
  insn->valtag.NB.val = NB(opcode);
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.NB.tag = insn->valtag.rA.tag = insn->valtag.rSD.rD.base.tag = tag_const;
  insn->valtag.xer_bc_cmp0.val = 0;
  insn->valtag.xer_bc_cmp0.tag = tag_const;
}

/* lswx */
DISAS_HANDLER(lswx)
{
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rD.base.tag = tag_const;
  insn->valtag.xer_bc_cmp0.val = 0;
  insn->valtag.xer_bc_cmp0.tag = tag_const;
}

/* stswi */
DISAS_HANDLER(stswi)
{
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rD.base.tag = tag_const;
}

/* stswx */
DISAS_HANDLER(stswx)
{
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rS.base.tag = tag_const;
  insn->valtag.xer_bc_cmp0.val = 0;
  insn->valtag.xer_bc_cmp0.tag = tag_const;
}

/***                        Memory synchronisation                         ***/
/* eieio */
DISAS_HANDLER(eieio)
{
}

/* isync */
DISAS_HANDLER(isync)
{
  insn->reserved_mask = 0x0000012c;
}

/* lwarx */
DISAS_HANDLER(lwarx)
{
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.rB.tag = insn->valtag.rSD.rD.base.tag = tag_const;
  if (rA(opcode) != 0) {
    insn->valtag.rA.val = rA(opcode);
    insn->valtag.rA.tag = tag_const;
  }
}

/* stwcx. */
DISAS_HANDLER(stwcx_)
{
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->Rc = 1;
  insn->valtag.rB.tag = insn->valtag.rSD.rS.base.tag = tag_const;
  if (rA(opcode) != 0) {
    insn->valtag.rA.val = rA(opcode);
    insn->valtag.rA.tag = tag_const;
  }
}

/* sync */
DISAS_HANDLER(sync)
{
}

/***                         Floating-point load                           ***/
#define DISAS_LDF(width)                                                \
DISAS_HANDLER(l##width)                 				\
{                                                                       \
  /*insn->is_fp_insn = 1;*/					\
  insn->valtag.imm_signed = 1;						         \
  insn->valtag.imm.simm.val = SIMM(opcode);					\
  insn->valtag.rA.val = rA(opcode);						\
  insn->valtag.frSD.frD.base.val = rD(opcode);						\
  insn->valtag.imm.simm.tag = insn->valtag.rA.tag = \
      insn->valtag.frSD.frD.base.tag = tag_const; \
}

#define DISAS_LDUF(width)                                                  \
DISAS_HANDLER(l##width##u)              \
{                                                                             \
  /*insn->is_fp_insn = 1;*/					\
  insn->valtag.imm_signed = 1;						         \
  insn->valtag.imm.simm.val = SIMM(opcode);					\
  insn->valtag.rA.val = rA(opcode);						\
  insn->valtag.frSD.frD.base.val = rD(opcode);						\
  insn->valtag.imm.simm.tag = insn->valtag.rA.tag = \
      insn->valtag.frSD.frD.base.tag = tag_const; \
}

#define DISAS_LDUXF(width)                                                 \
DISAS_HANDLER(l##width##ux)             \
{                                                                             \
  /*insn->is_fp_insn = 1;*/					\
  insn->valtag.rA.val = rA(opcode);						\
  insn->valtag.frSD.frD.base.val = rD(opcode);						\
  insn->valtag.rB.val = rB(opcode);						\
  insn->valtag.rA.tag = insn->valtag.frSD.frD.base.tag = \
      insn->valtag.rB.tag = tag_const; \
}

#define DISAS_LDXF(width)                                           \
DISAS_HANDLER(l##width##x)             \
{                                                                             \
  /*insn->is_fp_insn = 1;*/					\
  insn->valtag.rA.val = rA(opcode);						\
  insn->valtag.frSD.frD.base.val = rD(opcode);						\
  insn->valtag.rB.val = rB(opcode);						\
  insn->valtag.rA.tag = insn->valtag.frSD.frD.base.tag = \
      insn->valtag.rB.tag = tag_const; \
}

#define DISAS_LDFS(width)                                               \
DISAS_LDF(width);                                                    \
DISAS_LDUF(width);                                                   \
DISAS_LDUXF(width);                                                  \
DISAS_LDXF(width)

/* lfd lfdu lfdux lfdx */
DISAS_LDFS(fd);
/* lfs lfsu lfsux lfsx */
DISAS_LDFS(fs);

/***                         Floating-point store                          ***/
#define DISAS_STF(width)                                                 \
DISAS_HANDLER(st##width)                				 \
{                                                                             \
  /*insn->is_fp_insn = 1;*/					\
  insn->valtag.imm_signed = 1;						         \
  insn->valtag.imm.simm.val = SIMM(opcode);					\
  insn->valtag.rA.val = rA(opcode);						\
  insn->valtag.frSD.frS.base.val = rS(opcode);						\
  insn->valtag.imm.simm.tag = insn->valtag.rA.tag = \
      insn->valtag.frSD.frS.base.tag = tag_const; \
}

#define DISAS_STUF(width)                                                \
DISAS_HANDLER(st##width##u)             				 \
{                                                                        \
  /*insn->is_fp_insn = 1;*/					\
  insn->valtag.imm_signed = 1;						         \
  insn->valtag.imm.simm.val = SIMM(opcode);					 \
  insn->valtag.rA.val = rA(opcode);						 \
  insn->valtag.frSD.frS.base.val = rS(opcode);						 \
  insn->valtag.imm.simm.tag = insn->valtag.rA.tag = \
      insn->valtag.frSD.frS.base.tag = tag_const; \
}

#define DISAS_STUXF(width)                                               \
DISAS_HANDLER(st##width##ux)            				 \
{                                                                             \
  /*insn->is_fp_insn = 1;*/					\
  insn->valtag.rA.val = rA(opcode);						 \
  insn->valtag.rB.val = rB(opcode);						 \
  insn->valtag.frSD.frS.base.val = rS(opcode);						 \
  insn->valtag.rA.tag = insn->valtag.rB.tag = \
      insn->valtag.frSD.frS.base.tag = tag_const; \
}

#define DISAS_STXF(width)                                           \
DISAS_HANDLER(st##width##x)            				    \
{                                                                   \
  /*insn->is_fp_insn = 1;*/				    \
  insn->valtag.rA.val = rA(opcode);					    \
  insn->valtag.rB.val = rB(opcode);					    \
  insn->valtag.frSD.frS.base.val = rS(opcode);					    \
  insn->valtag.rA.tag = insn->valtag.rB.tag = \
      insn->valtag.frSD.frS.base.tag = tag_const; \
}

#define DISAS_STFS(width)                                          \
DISAS_STF(width);                                                    \
DISAS_STUF(width);                                                   \
DISAS_STUXF(width);                                                  \
DISAS_STXF(width)

/* stfd stfdu stfdux stfdx */
DISAS_STFS(fd);
/* stfs stfsu stfsux stfsx */
DISAS_STFS(fs);

/* Optional: */
/* stfiwx */
DISAS_HANDLER(stfiwx)
{
  //insn->is_fp_insn = 1;
}

/* b ba bl bla */
DISAS_HANDLER(b)
{
  /* sign extend LI */
  //insn->val.LI = SIGN_EXT(LI(opcode), 26);
  insn->valtag.LI.val = LI(opcode);
  insn->AA = AA(opcode);
  insn->LK = LK(opcode);
  insn->valtag.LI.tag = tag_const;
}

#define BCOND_IM  0
#define BCOND_LR  1
#define BCOND_CTR 2

#define BO_cleanup(x) (x & ~1 /*clear lsb (y in [ppc 8-23])*/)

#define DISAS_BCOND_INDIR(opcode) do {					\
  insn->LK = LK(opcode);					\
  insn->BO = BO_cleanup(BO(opcode));					\
  insn->valtag.BI.val = BI(opcode);					\
  insn->valtag.BI.tag = tag_const; \
} while (0)

DISAS_HANDLER(bc)
{                                                                             
  insn->AA = AA(opcode);
  insn->LK = LK(opcode);
  insn->BO = BO_cleanup(BO(opcode));
  insn->valtag.BI.val = BI(opcode);
  insn->valtag.BD.val = BD(opcode);
  insn->valtag.BI.tag = insn->valtag.BD.tag = tag_const;
}

DISAS_HANDLER(bcctr)
{                                                                             
  DISAS_BCOND_INDIR(opcode);
  insn->valtag.ctr0.val = 0;
  insn->valtag.ctr0.tag = tag_const;
}

DISAS_HANDLER(bclr)
{                                                                             
  DISAS_BCOND_INDIR(opcode);
  insn->valtag.lr0.val = 0;
  insn->valtag.lr0.tag = tag_const;
}

/***                      Condition register logical                       ***/
#define DISAS_CRLOGIC(op)                                                     \
DISAS_HANDLER(cr##op)                 \
{                                                                             \
  insn->valtag.crbA.val = crbA(opcode);					\
  insn->valtag.crbB.val = crbB(opcode);					\
  insn->valtag.crbD.val = crbD(opcode);					\
  insn->valtag.crbA.tag = insn->valtag.crbB.tag = insn->valtag.crbD.tag = tag_const; \
}

/* crand */
DISAS_CRLOGIC(and)
/* crandc */
DISAS_CRLOGIC(andc)
/* creqv */
DISAS_CRLOGIC(eqv)
/* crnand */
DISAS_CRLOGIC(nand)
/* crnor */
DISAS_CRLOGIC(nor)
/* cror */
DISAS_CRLOGIC(or)
/* crorc */
DISAS_CRLOGIC(orc)
/* crxor */
DISAS_CRLOGIC(xor)
/* mcrf */
DISAS_HANDLER(mcrf)
{
  insn->valtag.crfS.val = crfS(opcode);
  insn->valtag.crfD.val = crfD(opcode);
  insn->valtag.crfS.tag = insn->valtag.crfD.tag = tag_const;
}

/***                           System linkage                              ***/
/* rfi (supervisor only) */
DISAS_HANDLER(rfi)
{
  //insn->invalid = 1;
}

/* sc */
DISAS_HANDLER(sc)
{
  int i;
  insn->AA = 1;
  i = 0;
  insn->valtag.i_sc_regs[i].val = 0;
  insn->valtag.i_sc_regs[i].tag = tag_const;
  i++;
  insn->valtag.i_sc_regs[i].val = 3;
  insn->valtag.i_sc_regs[i].tag = tag_const;
  i++;
  insn->valtag.i_sc_regs[i].val = 4;
  insn->valtag.i_sc_regs[i].tag = tag_const;
  i++;
  insn->valtag.i_sc_regs[i].val = 5;
  insn->valtag.i_sc_regs[i].tag = tag_const;
  i++;
  insn->valtag.i_sc_regs[i].val = 6;
  insn->valtag.i_sc_regs[i].tag = tag_const;
  i++;
  insn->valtag.i_sc_regs[i].val = 7;
  insn->valtag.i_sc_regs[i].tag = tag_const;
  i++;
  insn->valtag.i_sc_regs[i].val = 8;
  insn->valtag.i_sc_regs[i].tag = tag_const;
  i++;
  ASSERT(i == PPC_NUM_SC_IMPLICIT_REGS);
  insn->valtag.num_i_sc_regs = i;
  insn->valtag.xer_so0.val = 0;
  insn->valtag.xer_so0.tag = tag_const;
}

/***                                Trap                                   ***/
/* tw */
DISAS_HANDLER(tw)
{
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->reserved_mask = 0x00800000;
  insn->valtag.rA.tag = insn->valtag.rB.tag = tag_const;
}

/* twi */
DISAS_HANDLER(twi)
{
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.imm_signed = 1;
  insn->valtag.imm.simm.val = SIMM(opcode);
  insn->valtag.rA.tag = insn->valtag.imm.simm.tag = tag_const;
}

/* mcrxr */
DISAS_HANDLER(mcrxr)
{
  insn->valtag.crfD.val = crfD(opcode);
  insn->valtag.crfD.tag = tag_const;
  insn->valtag.xer_ca0.val = 0;
  insn->valtag.xer_ca0.tag = tag_const;
  insn->valtag.xer_ov0.val = 0;
  insn->valtag.xer_ov0.tag = tag_const;
  insn->valtag.xer_so0.val = 0;
  insn->valtag.xer_so0.tag = tag_const;
}

/* mfcr */
DISAS_HANDLER(mfcr)
{
  int i;

  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.rSD.rD.base.tag = tag_const;

  for (i = 0; i < PPC_NUM_CRFREGS; i++) {
    insn->valtag.i_crfs[i].val = i;
    insn->valtag.i_crfs[i].tag = tag_const;
    insn->valtag.i_crfs_so[i].val = i;
    insn->valtag.i_crfs_so[i].tag = tag_const;
  }
  insn->valtag.num_i_crfs = PPC_NUM_CRFREGS;
  insn->valtag.num_i_crfs_so = PPC_NUM_CRFREGS;
}

/* mfmsr */
DISAS_HANDLER(mfmsr)
{
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.rSD.rD.base.tag = tag_const;
}

/* mfspr */
#define disas_op_mfspr(opcode) do {				\
  insn->SPR = SPR(opcode);					\
  insn->valtag.rSD.rD.base.val = rD(opcode);					\
  insn->valtag.rSD.rD.base.tag = tag_const; \
} while (0)

DISAS_HANDLER(mfspr)
{
  disas_op_mfspr(opcode);
}

/* mftb */
DISAS_HANDLER(mftb)
{
  disas_op_mfspr(opcode);
}

/* mtcrf */
/* The mask should be 0x00100801, but Mac OS X 10.4 use an alternate form */
DISAS_HANDLER(mtcrf)
{
  int i, num_crfs;

  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.CRM = CRM(opcode);
  insn->valtag.rSD.rS.base.tag = tag_const;

  num_crfs = 0;
  for (i = 0; i < PPC_NUM_CRFREGS; i++) {
    if (insn->valtag.CRM & (1UL << (PPC_NUM_CRFREGS - 1 - i))) {
      insn->valtag.i_crfs[num_crfs].val = i;
      insn->valtag.i_crfs[num_crfs].tag = tag_const;
      insn->valtag.i_crfs_so[num_crfs].val = i;
      insn->valtag.i_crfs_so[num_crfs].tag = tag_const;
      //printf("CRM=%x, adding crf[%d]\n", insn->valtag.CRM, i);
      num_crfs++;
    }
  }
  //printf("CRM=%x, num_crfs = %d\n", insn->valtag.CRM, num_crfs);
  insn->valtag.num_i_crfs = num_crfs;
  insn->valtag.num_i_crfs_so = num_crfs;
}

/* mtmsr */
DISAS_HANDLER(mtmsr)
{
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.rSD.rS.base.tag = tag_const;
}

/* mtspr */
DISAS_HANDLER(mtspr)
{
  insn->SPR = SPR(opcode);
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.rSD.rS.base.tag = tag_const;
}

/***                         Cache management                              ***/
/* For now, all those will be implemented as nop:
 * this is valid, regarding the PowerPC specs...
 * We just have to flush tb while invalidating instruction cache lines...
 */

#define DISAS_DCB(opc) \
DISAS_HANDLER(opc) \
{ \
  insn->valtag.rB.val = rB(opcode); \
  insn->valtag.rB.tag = tag_const; \
  if (rA(opcode) != 0) { \
    insn->valtag.rA.val = rA(opcode); \
    insn->valtag.rA.tag = tag_const; \
  } \
}

/* dcbf */
DISAS_DCB(dcbf)
/* dcbi (Supervisor only) */
DISAS_DCB(dcbi)
/* dcdst */
DISAS_DCB(dcbst)
/* dcbt */
DISAS_DCB(dcbt)
/* dcbtst */
DISAS_DCB(dcbtst)
/* dcbz */
DISAS_DCB(dcbz)
/* icbi */
DISAS_DCB(icbi)

/* Optional: */
/* dcba */
DISAS_HANDLER(dcba)
{
}

/***                    Segment register manipulation                      ***/
/* Supervisor only: */
/* mfsr */
DISAS_HANDLER(mfsr)
{
}

/* mfsrin */
DISAS_HANDLER(mfsrin)
{
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.rB.tag = insn->valtag.rSD.rD.base.tag = tag_const;
}

/* mtsr */
DISAS_HANDLER(mtsr)
{
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->SR = SR(opcode);
  insn->valtag.rSD.rS.base.tag = tag_const;
}

/* mtsrin */
DISAS_HANDLER(mtsrin)
{
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rS.base.tag = insn->valtag.rB.tag = tag_const;
}

/***                      Lookaside buffer management                      ***/
/* Optional & supervisor only: */
/* tlbia */
DISAS_HANDLER(tlbia)
{
}

/* tlbie */
DISAS_HANDLER(tlbie)
{
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rB.tag = tag_const;
}

/* tlbsync */
DISAS_HANDLER(tlbsync)
{
}

/* eciwx */
DISAS_HANDLER(eciwx)
{
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rD.base.val = rD(opcode);
  insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rD.base.tag = tag_const;
}

/* ecowx */
DISAS_HANDLER(ecowx)
{
  insn->valtag.rA.val = rA(opcode);
  insn->valtag.rB.val = rB(opcode);
  insn->valtag.rSD.rS.base.val = rS(opcode);
  insn->valtag.rA.tag = insn->valtag.rB.tag = insn->valtag.rSD.rS.base.tag = tag_const;
}

/* Start opcode list */
DISAS_OPCODE_MARK(end);

/*****************************************************************************/
/* Instruction types */
enum {
    PPC_NONE        = 0x00000000,
    /* integer operations instructions             */
    /* flow control instructions                   */
    /* virtual memory instructions                 */
    /* ld/st with reservation instructions         */
    /* cache control instructions                  */
    /* spr/msr access instructions                 */
    PPC_INSNS_BASE  = 0x00000001,
#define PPC_INTEGER PPC_INSNS_BASE
#define PPC_FLOW    PPC_INSNS_BASE
#define PPC_MEM     PPC_INSNS_BASE
#define PPC_RES     PPC_INSNS_BASE
#define PPC_CACHE   PPC_INSNS_BASE
#define PPC_MISC    PPC_INSNS_BASE
    /* floating point operations instructions      */
    PPC_FLOAT       = 0x00000002,
    /* more floating point operations instructions */
    PPC_FLOAT_EXT   = 0x00000004,
    /* external control instructions               */
    PPC_EXTERN      = 0x00000008,
    /* segment register access instructions        */
    PPC_SEGMENT     = 0x00000010,
    /* Optional cache control instructions         */
    PPC_CACHE_OPT   = 0x00000020,
    /* Optional floating point op instructions     */
    PPC_FLOAT_OPT   = 0x00000040,
    /* Optional memory control instructions        */
    PPC_MEM_TLBIA   = 0x00000080,
    PPC_MEM_TLBIE   = 0x00000100,
    PPC_MEM_TLBSYNC = 0x00000200,
    /* eieio & sync                                */
    PPC_MEM_SYNC    = 0x00000400,
    /* PowerPC 6xx TLB management instructions     */
    PPC_6xx_TLB     = 0x00000800,
    /* Altivec support                             */
    PPC_ALTIVEC     = 0x00001000,
    /* Time base support                           */
    PPC_TB          = 0x00002000,
    /* Embedded PowerPC dedicated instructions     */
    PPC_4xx_COMMON  = 0x00004000,
    /* PowerPC 40x exception model                 */
    PPC_40x_EXCP    = 0x00008000,
    /* PowerPC 40x specific instructions           */
    PPC_40x_SPEC    = 0x00010000,
    /* PowerPC 405 Mac instructions                */
    PPC_405_MAC     = 0x00020000,
    /* PowerPC 440 specific instructions           */
    PPC_440_SPEC    = 0x00040000,
    /* Specific extensions */
    /* Power-to-PowerPC bridge (601)               */
    PPC_POWER_BR    = 0x00080000,
    /* PowerPC 602 specific */
    PPC_602_SPEC    = 0x00100000,
    /* Deprecated instructions                     */
    /* Original POWER instruction set              */
    PPC_POWER       = 0x00200000,
    /* POWER2 instruction set extension            */
    PPC_POWER2      = 0x00400000,
    /* Power RTC support */
    PPC_POWER_RTC   = 0x00800000,
    /* 64 bits PowerPC instructions                */
    /* 64 bits PowerPC instruction set             */
    PPC_64B         = 0x01000000,
    /* 64 bits hypervisor extensions               */
    PPC_64H         = 0x02000000,
    /* 64 bits PowerPC "bridge" features           */
    PPC_64_BRIDGE   = 0x04000000,
};

/* CPU run-time flags (MMU and exception model) */
enum {
    /* MMU model */
#define PPC_FLAGS_MMU_MASK (0x0000000F)
    /* Standard 32 bits PowerPC MMU */
    PPC_FLAGS_MMU_32B      = 0x00000000,
    /* Standard 64 bits PowerPC MMU */
    PPC_FLAGS_MMU_64B      = 0x00000001,
    /* PowerPC 601 MMU */
    PPC_FLAGS_MMU_601      = 0x00000002,
    /* PowerPC 6xx MMU with software TLB */
    PPC_FLAGS_MMU_SOFT_6xx = 0x00000003,
    /* PowerPC 4xx MMU with software TLB */
    PPC_FLAGS_MMU_SOFT_4xx = 0x00000004,
    /* PowerPC 403 MMU */
    PPC_FLAGS_MMU_403      = 0x00000005,
    /* Exception model */
#define PPC_FLAGS_EXCP_MASK (0x000000F0)
    /* Standard PowerPC exception model */
    PPC_FLAGS_EXCP_STD     = 0x00000000,
    /* PowerPC 40x exception model */
    PPC_FLAGS_EXCP_40x     = 0x00000010,
    /* PowerPC 601 exception model */
    PPC_FLAGS_EXCP_601     = 0x00000020,
    /* PowerPC 602 exception model */
    PPC_FLAGS_EXCP_602     = 0x00000030,
    /* PowerPC 603 exception model */
    PPC_FLAGS_EXCP_603     = 0x00000040,
    /* PowerPC 604 exception model */
    PPC_FLAGS_EXCP_604     = 0x00000050,
    /* PowerPC 7x0 exception model */
    PPC_FLAGS_EXCP_7x0     = 0x00000060,
    /* PowerPC 7x5 exception model */
    PPC_FLAGS_EXCP_7x5     = 0x00000070,
    /* PowerPC 74xx exception model */
    PPC_FLAGS_EXCP_74xx    = 0x00000080,
    /* PowerPC 970 exception model */
    PPC_FLAGS_EXCP_970     = 0x00000090,
};


/* PowerPC 40x instruction set */
#define PPC_INSNS_4xx (PPC_INSNS_BASE | PPC_MEM_TLBSYNC | PPC_4xx_COMMON)
/* PowerPC 401 */
#define PPC_INSNS_401 (PPC_INSNS_TODO)
#define PPC_FLAGS_401 (PPC_FLAGS_TODO)
/* PowerPC 403 */
#define PPC_INSNS_403 (PPC_INSNS_4xx | PPC_MEM_SYNC | PPC_MEM_TLBIA |         \
                       PPC_40x_EXCP | PPC_40x_SPEC)
#define PPC_FLAGS_403 (PPC_FLAGS_MMU_403 | PPC_FLAGS_EXCP_40x)
/* PowerPC 405 */
#define PPC_INSNS_405 (PPC_INSNS_4xx | PPC_MEM_SYNC | PPC_CACHE_OPT |         \
                       PPC_MEM_TLBIA | PPC_TB | PPC_40x_SPEC | PPC_40x_EXCP | \
                       PPC_405_MAC)
#define PPC_FLAGS_405 (PPC_FLAGS_MMU_SOFT_4xx | PPC_FLAGS_EXCP_40x)
/* PowerPC 440 */
#define PPC_INSNS_440 (PPC_INSNS_4xx | PPC_CACHE_OPT | PPC_405_MAC |          \
                       PPC_440_SPEC)
#define PPC_FLAGS_440 (PPC_FLAGS_TODO)
/* Non-embedded PowerPC */
#define PPC_INSNS_COMMON  (PPC_INSNS_BASE | PPC_FLOAT | PPC_MEM_SYNC |        \
                           PPC_SEGMENT | PPC_MEM_TLBIE)
/* PowerPC 601 */
#define PPC_INSNS_601 (PPC_INSNS_COMMON | PPC_EXTERN | PPC_POWER_BR)
#define PPC_FLAGS_601 (PPC_FLAGS_MMU_601 | PPC_FLAGS_EXCP_601)
/* PowerPC 602 */
#define PPC_INSNS_602 (PPC_INSNS_COMMON | PPC_FLOAT_EXT | PPC_6xx_TLB |       \
                       PPC_MEM_TLBSYNC | PPC_TB)
#define PPC_FLAGS_602 (PPC_FLAGS_MMU_SOFT_6xx | PPC_FLAGS_EXCP_602)
/* PowerPC 603 */
#define PPC_INSNS_603 (PPC_INSNS_COMMON | PPC_FLOAT_EXT | PPC_6xx_TLB |       \
                       PPC_MEM_TLBSYNC | PPC_EXTERN | PPC_TB)
#define PPC_FLAGS_603 (PPC_FLAGS_MMU_SOFT_6xx | PPC_FLAGS_EXCP_603)
/* PowerPC G2 */
#define PPC_INSNS_G2 (PPC_INSNS_COMMON | PPC_FLOAT_EXT | PPC_6xx_TLB |        \
                      PPC_MEM_TLBSYNC | PPC_EXTERN | PPC_TB)
#define PPC_FLAGS_G2 (PPC_FLAGS_MMU_SOFT_6xx | PPC_FLAGS_EXCP_603)
/* PowerPC 604 */
#define PPC_INSNS_604 (PPC_INSNS_COMMON | PPC_FLOAT_EXT | PPC_EXTERN |        \
                       PPC_MEM_TLBSYNC | PPC_TB)
#define PPC_FLAGS_604 (PPC_FLAGS_MMU_32B | PPC_FLAGS_EXCP_604)
/* PowerPC 740/750 (aka G3) */
#define PPC_INSNS_7x0 (PPC_INSNS_COMMON | PPC_FLOAT_EXT | PPC_EXTERN |        \
                       PPC_MEM_TLBSYNC | PPC_TB)
#define PPC_FLAGS_7x0 (PPC_FLAGS_MMU_32B | PPC_FLAGS_EXCP_7x0)
/* PowerPC 745/755 */
#define PPC_INSNS_7x5 (PPC_INSNS_COMMON | PPC_FLOAT_EXT | PPC_EXTERN |        \
                       PPC_MEM_TLBSYNC | PPC_TB | PPC_6xx_TLB)
#define PPC_FLAGS_7x5 (PPC_FLAGS_MMU_SOFT_6xx | PPC_FLAGS_EXCP_7x5)
/* PowerPC 74xx (aka G4) */
#define PPC_INSNS_74xx (PPC_INSNS_COMMON | PPC_FLOAT_EXT | PPC_ALTIVEC |      \
                        PPC_MEM_TLBSYNC | PPC_TB)
#define PPC_FLAGS_74xx (PPC_FLAGS_MMU_32B | PPC_FLAGS_EXCP_74xx)

/* Default PowerPC will be 604/970 */
#define PPC_INSNS_PPC32 PPC_INSNS_604
#define PPC_FLAGS_PPC32 PPC_FLAGS_604
#if 0
#define PPC_INSNS_PPC64 PPC_INSNS_970
#define PPC_FLAGS_PPC64 PPC_FLAGS_970
#endif
#define PPC_INSNS_DEFAULT PPC_INSNS_604
#define PPC_FLAGS_DEFAULT PPC_FLAGS_604




static opc_handler_t invalid_handler = {
    .inval   = 0xFFFFFFFF,
    .type    = PPC_NONE,
    /*.handler = gen_invalid,*/
    .disassembler = disas_invalid,
    //.usedef = usedef_invalid,
};



/* Opcode types */
enum {
    PPC_DIRECT   = 0, /* Opcode routine        */
    PPC_INDIRECT = 1, /* Indirect opcode table */
};

static inline int is_indirect_opcode (void *handler)
{
    return ((unsigned long)handler & 0x03) == PPC_INDIRECT;
}

static inline opc_handler_t **ind_table(void *handler)
{
    return (opc_handler_t **)((unsigned long)handler & ~3);
}

static int insert_in_table (opc_handler_t **table, unsigned char idx,
                            opc_handler_t *handler)
{
    if (table[idx] != &invalid_handler)
        return -1;
    table[idx] = handler;

    return 0;
}



static int register_direct_insn (opc_handler_t **ppc_opcodes,
                                 unsigned char idx, opc_handler_t *handler)
{
    if (insert_in_table(ppc_opcodes, idx, handler) < 0) {
        printf("*** ERROR: opcode %02x already assigned in main "
                "opcode table\n", idx);
        return -1;
    }

    return 0;
}

/* Opcodes tables creation */
static void fill_new_table (opc_handler_t **table, int len)
{
    int i;

    for (i = 0; i < len; i++)
        table[i] = &invalid_handler;
}

static int create_new_table (opc_handler_t **table, unsigned char idx)
{
    opc_handler_t **tmp;

    tmp = (opc_handler_t **)malloc(0x20 * sizeof(opc_handler_t));
    if (tmp == NULL)
        return -1;
    fill_new_table(tmp, 0x20);
    table[idx] = (opc_handler_t *)((unsigned long)tmp | PPC_INDIRECT);

    return 0;
}

static int
register_ind_in_table(opc_handler_t **table,
                      unsigned char idx1, unsigned char idx2,
                      opc_handler_t *handler)
{
    if (table[idx1] == &invalid_handler) {
        if (create_new_table(table, idx1) < 0) {
            printf("*** ERROR: unable to create indirect table "
                    "idx=%02x\n", idx1);
            return -1;
        }
    } else {
        if (!is_indirect_opcode(table[idx1])) {
            printf("*** ERROR: idx %02x already assigned to a direct "
                    "opcode\n", idx1);
            return -1;
        }
    }
    if (handler != NULL &&
        insert_in_table(ind_table(table[idx1]), idx2, handler) < 0) {
        printf("*** ERROR: opcode %02x already assigned in "
                "opcode table %02x\n", idx2, idx1);
        return -1;
    }

    return 0;
}

static int
register_ind_insn(opc_handler_t **ppc_opcodes,
                  unsigned char idx1, unsigned char idx2,
                  opc_handler_t *handler)
{
    int ret;

    ret = register_ind_in_table(ppc_opcodes, idx1, idx2, handler);

    return ret;
}

static int register_dblind_insn (opc_handler_t **ppc_opcodes, 
                                 unsigned char idx1, unsigned char idx2,
                                  unsigned char idx3, opc_handler_t *handler)
{
    if (register_ind_in_table(ppc_opcodes, idx1, idx2, NULL) < 0) {
        printf("*** ERROR: unable to join indirect table idx "
                "[%02x-%02x]\n", idx1, idx2);
        return -1;
    }
    if (register_ind_in_table(ind_table(ppc_opcodes[idx1]), idx2, idx3,
                              handler) < 0) {
        printf("*** ERROR: unable to insert opcode "
                "[%02x-%02x-%02x]\n", idx1, idx2, idx3);
        return -1;
    }

    return 0;
}

static int register_insn (opc_handler_t **ppc_opcodes, opcode_t *insn)
{
    if (insn->opc2 != 0xFF) {
        if (insn->opc3 != 0xFF) {
            if (register_dblind_insn(ppc_opcodes, insn->opc1, insn->opc2,
                                     insn->opc3, &insn->handler) < 0)
                return -1;
        } else {
            if (register_ind_insn(ppc_opcodes, insn->opc1,
                                  insn->opc2, &insn->handler) < 0)
                return -1;
        }
    } else {
        if (register_direct_insn(ppc_opcodes, insn->opc1, &insn->handler) < 0)
            return -1;
    }

    return 0;
}

#define __GEN_INT_ARITH2(name, opc1, opc2, opc3, inval)                       \
GEN_HANDLER(name, opc1, opc2, opc3, inval, PPC_INTEGER)

#define __GEN_INT_ARITH2_O(name, opc1, opc2, opc3, inval)                     \
GEN_HANDLER(name, opc1, opc2, opc3, inval, PPC_INTEGER)

#define __GEN_INT_ARITH1(name, opc1, opc2, opc3)                              \
GEN_HANDLER(name, opc1, opc2, opc3, 0x0000F800, PPC_INTEGER)

#define __GEN_INT_ARITH1_O(name, opc1, opc2, opc3)                            \
GEN_HANDLER(name, opc1, opc2, opc3, 0x0000F800, PPC_INTEGER)

#define GEN_INT_ARITH2(name, opc1, opc2, opc3)                                \
__GEN_INT_ARITH2(name, opc1, opc2, opc3, 0x00000000),                         \
__GEN_INT_ARITH2_O(name##o, opc1, opc2, opc3 | 0x10, 0x00000000)

/* Two operands arithmetic functions with no overflow allowed */
#define GEN_INT_ARITHN(name, opc1, opc2, opc3)                                \
__GEN_INT_ARITH2(name, opc1, opc2, opc3, 0x00000400)

/* One operand arithmetic functions */
#define GEN_INT_ARITH1(name, opc1, opc2, opc3)                                \
__GEN_INT_ARITH1(name, opc1, opc2, opc3),                                     \
__GEN_INT_ARITH1_O(name##o, opc1, opc2, opc3 | 0x10)

#define GEN_CMP(name, opc)                                                    \
GEN_HANDLER(name, 0x1F, 0x00, opc, 0x00400000, PPC_INTEGER)

/***                            Integer logical                            ***/
#define __GEN_LOGICAL2(name, opc2, opc3)                                      \
GEN_HANDLER(name, 0x1F, opc2, opc3, 0x00000000, PPC_INTEGER)

#define GEN_LOGICAL2(name, opc)                                               \
__GEN_LOGICAL2(name, 0x1C, opc)

#define GEN_LOGICAL1(name, opc)                                               \
GEN_HANDLER(name, 0x1F, 0x1A, opc, 0x00000000, PPC_INTEGER)

/***                       Floating-Point arithmetic                       ***/
#define _GEN_FLOAT_ACB(name, op, op1, op2, isfloat)                           \
GEN_HANDLER(f##name, op1, op2, 0xFF, 0x00000000, PPC_FLOAT)

#define GEN_FLOAT_ACB(name, op2)                                              \
_GEN_FLOAT_ACB(name, name, 0x3F, op2, 0),                                     \
_GEN_FLOAT_ACB(name##s, name, 0x3B, op2, 1)

#define _GEN_FLOAT_AB(name, op, op1, op2, inval, isfloat)                     \
GEN_HANDLER(f##name, op1, op2, 0xFF, inval, PPC_FLOAT)

#define GEN_FLOAT_AB(name, op2, inval)                                        \
_GEN_FLOAT_AB(name, name, 0x3F, op2, inval, 0),                               \
_GEN_FLOAT_AB(name##s, name, 0x3B, op2, inval, 1)

#define _GEN_FLOAT_AC(name, op, op1, op2, inval, isfloat)                     \
GEN_HANDLER(f##name, op1, op2, 0xFF, inval, PPC_FLOAT)

#define GEN_FLOAT_AC(name, op2, inval)                                        \
_GEN_FLOAT_AC(name, name, 0x3F, op2, inval, 0),                               \
_GEN_FLOAT_AC(name##s, name, 0x3B, op2, inval, 1)

#define GEN_FLOAT_B(name, op2, op3)                                           \
GEN_HANDLER(f##name, 0x3F, op2, op3, 0x001F0000, PPC_FLOAT)

#define GEN_FLOAT_BS(name, op1, op2)                                          \
GEN_HANDLER(f##name, op1, op2, 0xFF, 0x001F07C0, PPC_FLOAT)

#define GEN_LD(width, opc)                                                    \
GEN_HANDLER(l##width, opc, 0xFF, 0xFF, 0x00000000, PPC_INTEGER)

#define GEN_LDU(width, opc)                                                   \
GEN_HANDLER(l##width##u, opc, 0xFF, 0xFF, 0x00000000, PPC_INTEGER)

#define GEN_LDUX(width, opc)                                                  \
GEN_HANDLER(l##width##ux, 0x1F, 0x17, opc, 0x00000001, PPC_INTEGER)

#define GEN_LDX(width, opc2, opc3)                                            \
GEN_HANDLER(l##width##x, 0x1F, opc2, opc3, 0x00000001, PPC_INTEGER)

#define GEN_LDS(width, op)                                                    \
GEN_LD(width, op | 0x20),                                                     \
GEN_LDU(width, op | 0x21),                                                    \
GEN_LDUX(width, op | 0x01),                                                   \
GEN_LDX(width, 0x17, op | 0x00)

/***                              Integer store                            ***/
#define GEN_ST(width, opc)                                                    \
GEN_HANDLER(st##width, opc, 0xFF, 0xFF, 0x00000000, PPC_INTEGER)

#define GEN_STU(width, opc)                                                   \
GEN_HANDLER(st##width##u, opc, 0xFF, 0xFF, 0x00000000, PPC_INTEGER)

#define GEN_STUX(width, opc)                                                  \
GEN_HANDLER(st##width##ux, 0x1F, 0x17, opc, 0x00000001, PPC_INTEGER)

#define GEN_STX(width, opc2, opc3)                                            \
GEN_HANDLER(st##width##x, 0x1F, opc2, opc3, 0x00000001, PPC_INTEGER)

#define GEN_STS(width, op)                                                    \
GEN_ST(width, op | 0x20),                                                     \
GEN_STU(width, op | 0x21),                                                    \
GEN_STUX(width, op | 0x01),                                                   \
GEN_STX(width, 0x17, op | 0x00)

/***                         Floating-point load                           ***/
#define GEN_LDF(width, opc)                                                   \
GEN_HANDLER(l##width, opc, 0xFF, 0xFF, 0x00000000, PPC_FLOAT)

#define GEN_LDUF(width, opc)                                                  \
GEN_HANDLER(l##width##u, opc, 0xFF, 0xFF, 0x00000000, PPC_FLOAT)

#define GEN_LDUXF(width, opc)                                                 \
GEN_HANDLER(l##width##ux, 0x1F, 0x17, opc, 0x00000001, PPC_FLOAT)

#define GEN_LDXF(width, opc2, opc3)                                           \
GEN_HANDLER(l##width##x, 0x1F, opc2, opc3, 0x00000001, PPC_FLOAT)

#define GEN_LDFS(width, op)                                                   \
GEN_LDF(width, op | 0x20),                                                    \
GEN_LDUF(width, op | 0x21),                                                   \
GEN_LDUXF(width, op | 0x01),                                                  \
GEN_LDXF(width, 0x17, op | 0x00)

#define GEN_STF(width, opc)                                                   \
GEN_HANDLER(st##width, opc, 0xFF, 0xFF, 0x00000000, PPC_FLOAT)

#define GEN_STUF(width, opc)                                                  \
GEN_HANDLER(st##width##u, opc, 0xFF, 0xFF, 0x00000000, PPC_FLOAT)

#define GEN_STUXF(width, opc)                                                 \
GEN_HANDLER(st##width##ux, 0x1F, 0x17, opc, 0x00000001, PPC_FLOAT)

#define GEN_STXF(width, opc2, opc3)                                           \
GEN_HANDLER(st##width##x, 0x1F, opc2, opc3, 0x00000001, PPC_FLOAT)

#define GEN_STFS(width, op)                                                   \
GEN_STF(width, op | 0x20),                                                    \
GEN_STUF(width, op | 0x21),                                                   \
GEN_STUXF(width, op | 0x01),                                                  \
GEN_STXF(width, 0x17, op | 0x00)

#define GEN_CRLOGIC(op, opc)                                                  \
GEN_HANDLER(cr##op, 0x13, 0x01, opc, 0x00000001, PPC_INTEGER)

#define GEN_HANDLER(name, opc1, opc2, opc3, inval, type)                      \
GEN_OPCODE(name, opc1, opc2, opc3, inval, type)

#define GEN_OPCODE(name, op1, op2, op3, invl, _typ)                           \
{                                       \
    .opc1 = op1,                                                              \
    .opc2 = op2,                                                              \
    .opc3 = op3,                                                              \
    .pad  = { 0, },                                                           \
    .handler = {                                                              \
        .inval   = invl,                                                      \
        .type = _typ,                                                         \
        /*.handler = &gen_##name,*/                                             \
        .disassembler = &disas_##name,					      \
        /*.usedef = &usedef_##name,*/					      \
    },                                                                        \
    .oname = stringify(name),                                                 \
}

static opcode_t ppc_opcodes[] = {
GEN_HANDLER(invalid, 0x00, 0x00, 0x00, 0xFFFFFFFF, PPC_NONE),
/* add    add.    addo    addo.    */
GEN_INT_ARITH2 (add,    0x1F, 0x0A, 0x08),
/* addc   addc.   addco   addco.   */
GEN_INT_ARITH2 (addc,   0x1F, 0x0A, 0x00),
/* adde   adde.   addeo   addeo.   */
GEN_INT_ARITH2 (adde,   0x1F, 0x0A, 0x04),
/* addme  addme.  addmeo  addmeo.  */
GEN_INT_ARITH1 (addme,  0x1F, 0x0A, 0x07),
/* addze  addze.  addzeo  addzeo.  */
GEN_INT_ARITH1 (addze,  0x1F, 0x0A, 0x06),
/* divw   divw.   divwo   divwo.   */
GEN_INT_ARITH2 (divw,   0x1F, 0x0B, 0x0F),
/* divwu  divwu.  divwuo  divwuo.  */
GEN_INT_ARITH2 (divwu,  0x1F, 0x0B, 0x0E),
/* mulhw  mulhw.                   */
GEN_INT_ARITHN (mulhw,  0x1F, 0x0B, 0x02),
/* mulhwu mulhwu.                  */
GEN_INT_ARITHN (mulhwu, 0x1F, 0x0B, 0x00),
/* mullw  mullw.  mullwo  mullwo.  */
GEN_INT_ARITH2 (mullw,  0x1F, 0x0B, 0x07),
/* neg    neg.    nego    nego.    */
GEN_INT_ARITH1 (neg,    0x1F, 0x08, 0x03),
/* subf   subf.   subfo   subfo.   */
GEN_INT_ARITH2 (subf,   0x1F, 0x08, 0x01),
/* subfc  subfc.  subfco  subfco.  */
GEN_INT_ARITH2 (subfc,  0x1F, 0x08, 0x00),
/* subfe  subfe.  subfeo  subfeo.  */
GEN_INT_ARITH2 (subfe,  0x1F, 0x08, 0x04),
/* subfme subfme. subfmeo subfmeo. */
GEN_INT_ARITH1 (subfme, 0x1F, 0x08, 0x07),
/* subfze subfze. subfzeo subfzeo. */
GEN_INT_ARITH1 (subfze, 0x1F, 0x08, 0x06),
GEN_HANDLER(addi, 0x0E, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
GEN_HANDLER(addic, 0x0C, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
GEN_HANDLER(addic_, 0x0D, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
GEN_HANDLER(addis, 0x0F, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
GEN_HANDLER(mulli, 0x07, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
GEN_HANDLER(subfic, 0x08, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* cmp */
GEN_CMP(cmp, 0x00),
/* cmpi */
GEN_HANDLER(cmpi, 0x0B, 0xFF, 0xFF, 0x00400000, PPC_INTEGER),
/* cmpl */
GEN_CMP(cmpl, 0x01),
/* cmpli */
GEN_HANDLER(cmpli, 0x0A, 0xFF, 0xFF, 0x00400000, PPC_INTEGER),
/* and & and. */
GEN_LOGICAL2(and, 0x00),
/* andc & andc. */
GEN_LOGICAL2(andc, 0x01),
/* andi. */
GEN_HANDLER(andi_, 0x1C, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* andis. */
GEN_HANDLER(andis_, 0x1D, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* cntlzw */
GEN_LOGICAL1(cntlzw, 0x00),
/* eqv & eqv. */
GEN_LOGICAL2(eqv, 0x08),
/* extsb & extsb. */
GEN_LOGICAL1(extsb, 0x1D),
/* extsh & extsh. */
GEN_LOGICAL1(extsh, 0x1C),
/* nand & nand. */
GEN_LOGICAL2(nand, 0x0E),
/* nor & nor. */
GEN_LOGICAL2(nor, 0x03),
/* or & or. */
GEN_HANDLER(or, 0x1F, 0x1C, 0x0D, 0x00000000, PPC_INTEGER),
/* orc & orc. */
GEN_LOGICAL2(orc, 0x0C),
/* xor & xor. */
GEN_HANDLER(xor, 0x1F, 0x1C, 0x09, 0x00000000, PPC_INTEGER),
/* ori */
GEN_HANDLER(ori, 0x18, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* oris */
GEN_HANDLER(oris, 0x19, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* xori */
GEN_HANDLER(xori, 0x1A, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* xoris */
GEN_HANDLER(xoris, 0x1B, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* rlwimi & rlwimi. */
GEN_HANDLER(rlwimi, 0x14, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* rlwinm & rlwinm. */
GEN_HANDLER(rlwinm, 0x15, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* rlwnm & rlwnm. */
GEN_HANDLER(rlwnm, 0x17, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/***                             Integer shift                             ***/
/* slw & slw. */
__GEN_LOGICAL2(slw, 0x18, 0x00),
/* sraw & sraw. */
__GEN_LOGICAL2(sraw, 0x18, 0x18),
/* srawi & srawi. */
GEN_HANDLER(srawi, 0x1F, 0x18, 0x19, 0x00000000, PPC_INTEGER),
/* srw & srw. */
__GEN_LOGICAL2(srw, 0x18, 0x10),

/* fadd - fadds */
GEN_FLOAT_AB(add, 0x15, 0x000007C0),
/* fdiv - fdivs */
GEN_FLOAT_AB(div, 0x12, 0x000007C0),
/* fmul - fmuls */
GEN_FLOAT_AC(mul, 0x19, 0x0000F800),

/* fres */
GEN_FLOAT_BS(res, 0x3B, 0x18),

/* frsqrte */
GEN_FLOAT_BS(rsqrte, 0x3F, 0x1A),

/* fsel */
_GEN_FLOAT_ACB(sel, sel, 0x3F, 0x17, 0),
/* fsub - fsubs */
GEN_FLOAT_AB(sub, 0x14, 0x000007C0),

GEN_HANDLER(fsqrt, 0x3F, 0x16, 0xFF, 0x001F07C0, PPC_FLOAT_OPT),

GEN_HANDLER(fsqrts, 0x3B, 0x16, 0xFF, 0x001F07C0, PPC_FLOAT_OPT),

/***                     Floating-Point multiply-and-add                   ***/
/* fmadd - fmadds */
GEN_FLOAT_ACB(madd, 0x1D),
/* fmsub - fmsubs */
GEN_FLOAT_ACB(msub, 0x1C),
/* fnmadd - fnmadds */
GEN_FLOAT_ACB(nmadd, 0x1F),
/* fnmsub - fnmsubs */
GEN_FLOAT_ACB(nmsub, 0x1E),

/***                     Floating-Point round & convert                    ***/
/* fctiw */
GEN_FLOAT_B(ctiw, 0x0E, 0x00),
/* fctiwz */
GEN_FLOAT_B(ctiwz, 0x0F, 0x00),
/* frsp */
GEN_FLOAT_B(rsp, 0x0C, 0x00),

/***                         Floating-Point compare                        ***/
/* fcmpo */
GEN_HANDLER(fcmpo, 0x3F, 0x00, 0x00, 0x00600001, PPC_FLOAT),
/* fcmpu */
GEN_HANDLER(fcmpu, 0x3F, 0x00, 0x01, 0x00600001, PPC_FLOAT),

/***                         Floating-point move                           ***/
/* fabs */
GEN_FLOAT_B(abs, 0x08, 0x08),

/* fmr  - fmr. */
GEN_HANDLER(fmr, 0x3F, 0x08, 0x02, 0x001F0000, PPC_FLOAT),

/* fnabs */
GEN_FLOAT_B(nabs, 0x08, 0x04),
/* fneg */
GEN_FLOAT_B(neg, 0x08, 0x01),

/***                  Floating-Point status & ctrl register                ***/
/* mcrfs */
GEN_HANDLER(mcrfs, 0x3F, 0x00, 0x02, 0x0063F801, PPC_FLOAT),
/* mffs */
GEN_HANDLER(mffs, 0x3F, 0x07, 0x12, 0x001FF800, PPC_FLOAT),

/* mtfsb0 */
GEN_HANDLER(mtfsb0, 0x3F, 0x06, 0x02, 0x001FF800, PPC_FLOAT),

/* mtfsb1 */
GEN_HANDLER(mtfsb1, 0x3F, 0x06, 0x01, 0x001FF800, PPC_FLOAT),

/* mtfsf */
GEN_HANDLER(mtfsf, 0x3F, 0x07, 0x16, 0x02010000, PPC_FLOAT),

/* mtfsfi */
GEN_HANDLER(mtfsfi, 0x3F, 0x06, 0x04, 0x006f0800, PPC_FLOAT),

/* lbz lbzu lbzux lbzx */
GEN_LDS(bz, 0x02),
/* lha lhau lhaux lhax */
GEN_LDS(ha, 0x0A),
/* lhz lhzu lhzux lhzx */
GEN_LDS(hz, 0x08),
/* lwz lwzu lwzux lwzx */
GEN_LDS(wz, 0x00),

/* stb stbu stbux stbx */
GEN_STS(b, 0x06),
/* sth sthu sthux sthx */
GEN_STS(h, 0x0C),
/* stw stwu stwux stwx */
GEN_STS(w, 0x04),

/***                Integer load and store with byte reverse               ***/
/* lhbrx */
GEN_LDX(hbr, 0x16, 0x18),
/* lwbrx */
GEN_LDX(wbr, 0x16, 0x10),
/* sthbrx */
GEN_STX(hbr, 0x16, 0x1C),
/* stwbrx */
GEN_STX(wbr, 0x16, 0x14),

/* lmw */
GEN_HANDLER(lmw, 0x2E, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),
/* stmw */
GEN_HANDLER(stmw, 0x2F, 0xFF, 0xFF, 0x00000000, PPC_INTEGER),

/* lswi */
GEN_HANDLER(lswi, 0x1F, 0x15, 0x12, 0x00000001, PPC_INTEGER),

/* lswx */
GEN_HANDLER(lswx, 0x1F, 0x15, 0x10, 0x00000001, PPC_INTEGER),

/* stswi */
GEN_HANDLER(stswi, 0x1F, 0x15, 0x16, 0x00000001, PPC_INTEGER),
/* stswx */
GEN_HANDLER(stswx, 0x1F, 0x15, 0x14, 0x00000001, PPC_INTEGER),
/* eieio */
GEN_HANDLER(eieio, 0x1F, 0x16, 0x1A, 0x03FF0801, PPC_MEM),
/* isync */
GEN_HANDLER(isync, 0x13, 0x16, 0xFF, 0x03FF0801, PPC_MEM),
/* lwarx */
GEN_HANDLER(lwarx, 0x1F, 0x14, 0xFF, 0x00000001, PPC_RES),
/* stwcx. */
GEN_HANDLER(stwcx_, 0x1F, 0x16, 0x04, 0x00000000, PPC_RES),
/* sync */
GEN_HANDLER(sync, 0x1F, 0x16, 0x12, 0x03FF0801, PPC_MEM),
/* lfd lfdu lfdux lfdx */
GEN_LDFS(fd, 0x12),
/* lfs lfsu lfsux lfsx */
GEN_LDFS(fs, 0x10),

/* stfd stfdu stfdux stfdx */
GEN_STFS(fd, 0x16),
/* stfs stfsu stfsux stfsx */
GEN_STFS(fs, 0x14),

/* stfiwx */
GEN_HANDLER(stfiwx, 0x1F, 0x17, 0x1E, 0x00000001, PPC_FLOAT),
/* b ba bl bla */
GEN_HANDLER(b, 0x12, 0xFF, 0xFF, 0x00000000, PPC_FLOW),
GEN_HANDLER(bc, 0x10, 0xFF, 0xFF, 0x00000000, PPC_FLOW),
GEN_HANDLER(bcctr, 0x13, 0x10, 0x10, 0x00000000, PPC_FLOW),
GEN_HANDLER(bclr, 0x13, 0x10, 0x00, 0x00000000, PPC_FLOW),

/* crand */
GEN_CRLOGIC(and, 0x08),
/* crandc */
GEN_CRLOGIC(andc, 0x04),
/* creqv */
GEN_CRLOGIC(eqv, 0x09),
/* crnand */
GEN_CRLOGIC(nand, 0x07),
/* crnor */
GEN_CRLOGIC(nor, 0x01),
/* cror */
GEN_CRLOGIC(or, 0x0E),
/* crorc */
GEN_CRLOGIC(orc, 0x0D),
/* crxor */
GEN_CRLOGIC(xor, 0x06),

/* mcrf */
GEN_HANDLER(mcrf, 0x13, 0x00, 0xFF, 0x00000001, PPC_INTEGER),
/* rfi (supervisor only) */
GEN_HANDLER(rfi, 0x13, 0x12, 0xFF, 0x03FF8001, PPC_FLOW),
/* sc */
GEN_HANDLER(sc, 0x11, 0xFF, 0xFF, 0x03FFFFFD, PPC_FLOW),
/* mcrxr */
GEN_HANDLER(mcrxr, 0x1F, 0x00, 0x10, 0x007FF801, PPC_MISC),
/* mfcr */
GEN_HANDLER(mfcr, 0x1F, 0x13, 0x00, 0x001FF801, PPC_MISC),
/* mfmsr */
GEN_HANDLER(mfmsr, 0x1F, 0x13, 0x02, 0x001FF801, PPC_MISC),
GEN_HANDLER(mfspr, 0x1F, 0x13, 0x0A, 0x00000001, PPC_MISC),
/* mftb */
GEN_HANDLER(mftb, 0x1F, 0x13, 0x0B, 0x00000001, PPC_TB),
/* mtcrf */
/* The mask should be 0x00100801, but Mac OS X 10.4 use an alternate form */
GEN_HANDLER(mtcrf, 0x1F, 0x10, 0x04, 0x00000801, PPC_MISC),
/* mtmsr */
GEN_HANDLER(mtmsr, 0x1F, 0x12, 0x04, 0x001FF801, PPC_MISC),
/* mtspr */
GEN_HANDLER(mtspr, 0x1F, 0x13, 0x0E, 0x00000001, PPC_MISC),
/* dcbf */
GEN_HANDLER(dcbf, 0x1F, 0x16, 0x02, 0x03E00001, PPC_CACHE),
/* dcbi (Supervisor only) */
GEN_HANDLER(dcbi, 0x1F, 0x16, 0x0E, 0x03E00001, PPC_CACHE),
/* dcdst */
GEN_HANDLER(dcbst, 0x1F, 0x16, 0x01, 0x03E00001, PPC_CACHE),
/* dcbt */
GEN_HANDLER(dcbt, 0x1F, 0x16, 0x08, 0x03E00001, PPC_CACHE),
/* dcbtst */
GEN_HANDLER(dcbtst, 0x1F, 0x16, 0x07, 0x03E00001, PPC_CACHE),
GEN_HANDLER(dcbz, 0x1F, 0x16, 0x1F, 0x03E00001, PPC_CACHE),
/* dcba */
GEN_HANDLER(dcba, 0x1F, 0x16, 0x17, 0x03E00001, PPC_CACHE_OPT),
/* mfsr */
GEN_HANDLER(mfsr, 0x1F, 0x13, 0x12, 0x0010F801, PPC_SEGMENT),
/* mfsrin */
GEN_HANDLER(mfsrin, 0x1F, 0x13, 0x14, 0x001F0001, PPC_SEGMENT),
/* mtsr */
GEN_HANDLER(mtsr, 0x1F, 0x12, 0x06, 0x0010F801, PPC_SEGMENT),
/* mtsrin */
GEN_HANDLER(mtsrin, 0x1F, 0x12, 0x07, 0x001F0001, PPC_SEGMENT),

/* tlbia */
GEN_HANDLER(tlbia, 0x1F, 0x12, 0x0B, 0x03FFFC01, PPC_MEM_TLBIA),
/* tlbie */
GEN_HANDLER(tlbie, 0x1F, 0x12, 0x09, 0x03FF0001, PPC_MEM),
/* tlbsync */
GEN_HANDLER(tlbsync, 0x1F, 0x16, 0x11, 0x03FFF801, PPC_MEM),
/* eciwx */
GEN_HANDLER(eciwx, 0x1F, 0x16, 0x0D, 0x00000001, PPC_EXTERN),
/* ecowx */
GEN_HANDLER(ecowx, 0x1F, 0x16, 0x09, 0x00000001, PPC_EXTERN),
};
static long const num_ppc_opcodes = sizeof ppc_opcodes/sizeof ppc_opcodes[0];

/* opcode handlers */
static opc_handler_t *opcodes[0x40];

struct ppc_def_t {
  const char *name;
  uint32_t pvr;
  uint32_t pvr_mask;
  uint32_t insns_flags;
  uint32_t flags;
  uint64_t msr_mask;
};


void
ppc_insn_disas_init(void)
{
  opcode_t *opc;
  long i, ret;

  struct ppc_def_t def = {
    .name        = "750",
    .pvr         = 0, //CPU_PPC_74x,
    .pvr_mask    = 0xFFFFF000,
    .insns_flags = PPC_INSNS_7x0,
    .flags       = PPC_FLAGS_7x0,
    .msr_mask    = 0x000000000007FF77,
  };

  fill_new_table(opcodes, 0x40);

  for (i = 0; i < num_ppc_opcodes; i++) {
    opc = &ppc_opcodes[i];
    if ((opc->handler.type & def.insns_flags) != 0) {
      ret = register_insn(opcodes, opc);
      ASSERT(ret >= 0);
    }
  }
}

int
ppc_insn_disassemble(ppc_insn_t *insn, uint8_t const *bincode, i386_as_mode_t i386_as_mode)
{
  opc_handler_t **table, *handler;
  uint32_t opcode;

  ppc_insn_init(insn);

  opcode = *(uint32_t *)bincode;
  opcode = bswap32(opcode);
  opcode = ((opcode & 0xFF000000) >> 24) |
    ((opcode & 0x00FF0000) >> 8) |
    ((opcode & 0x0000FF00) << 8) |
    ((opcode & 0x000000FF) << 24);

  table = opcodes;
  handler = table[opc1(opcode)];

  DBG(PPC_DISAS, "opcode=%x. opc1=%x, handler=%p\n", opcode, opc1(opcode), handler);
  DBG(PPC_DISAS, "handler->disassembler=%p.\ninvalid_disassembler=%p.\n",
      handler->disassembler, &disas_invalid);

  insn->opc1 = opc1(opcode);
  if (is_indirect_opcode(handler)) {
    insn->opc2 = opc2(opcode);
    table = ind_table(handler);
    handler = table[opc2(opcode)];
    if (is_indirect_opcode(handler)) {
      insn->opc3 = opc3(opcode);
      table = ind_table(handler);
      handler = table[opc3(opcode)];
    }
  }

  /* Is opcode *REALLY* valid ? */
  if (handler->disassembler == &disas_invalid) {
    /* printf("%s(): invalid/unsupported opcode: "
       "%02x - %02x - %02x (%08x)\n", __func__, 
       opc1(opcode), opc2(opcode),
       opc3(opcode), opcode);
       */
    return 0;
    //assert(0);
  } else {
    if ((opcode & handler->inval) != 0)
    {
      fprintf(stderr, "invalid bits %08x: "
          "%02x -%02x - %02x\n",
          opcode,opc1(opcode),
          opc2(opcode), opc3(opcode));
      return 0;
      //assert (0);
    }
  }

  (*(handler->disassembler))(opcode, insn);

  if (insn->Rc == 1) {
    insn->valtag.crf0.val = 0;
    insn->valtag.crf0.tag = tag_const;
    insn->valtag.i_crf0_so.val = 0;
    insn->valtag.i_crf0_so.tag = tag_const;
    insn->valtag.xer_so0.val = 0;
    insn->valtag.xer_so0.tag = tag_const;
  }

  if (insn->valtag.crfS.tag != tag_invalid) {
    insn->valtag.i_crfS_so.val = insn->valtag.crfS.val;
    insn->valtag.i_crfS_so.tag = tag_const;
    insn->valtag.xer_so0.val = 0;
    insn->valtag.xer_so0.tag = tag_const;
  }

  if (insn->valtag.crfD.tag != tag_invalid) {
    insn->valtag.i_crfD_so.val = insn->valtag.crfD.val;
    insn->valtag.i_crfD_so.tag = tag_const;
    insn->valtag.xer_so0.val = 0;
    insn->valtag.xer_so0.tag = tag_const;
  }

  if (ppc_insn_bo_indicates_ctr_use(insn)) {
    insn->valtag.ctr0.val = 0;
    insn->valtag.ctr0.tag = tag_const;
  }

  if (insn->LK == 1) {
    insn->valtag.lr0.val = 0;
    insn->valtag.lr0.tag = tag_const;
  }

  if (insn->SPR == 8) {
    insn->valtag.lr0.val = 0;
    insn->valtag.lr0.tag = tag_const;
  } else if (insn->SPR == 9) {
    insn->valtag.ctr0.val = 0;
    insn->valtag.ctr0.tag = tag_const;
  } else if (insn->SPR == 1) {
    insn->valtag.xer_ov0.val = 0;
    insn->valtag.xer_ov0.tag = tag_const;
    insn->valtag.xer_so0.val = 0;
    insn->valtag.xer_so0.tag = tag_const;
    insn->valtag.xer_ca0.val = 0;
    insn->valtag.xer_ca0.tag = tag_const;
    insn->valtag.xer_bc_cmp0.val = 0;
    insn->valtag.xer_bc_cmp0.tag = tag_const;
  } else if (insn->SPR != -1) {
    //NOT_IMPLEMENTED();
  }

  /*
     uint32_t dbg_opcode = ppc_insn_get_opcode (insn);
     print_ppc_insn (insn);
     dbg ("opcode = %x, dbg_opcode = %x\n", opcode, dbg_opcode);
     assert (opcode == dbg_opcode);
     */
  return 4;
}

/*
int
get_ppc_usedef(uint8_t *bincode, regset_t *use,
    regset_t *def, memset_t *memuse, memset_t *memdef)
{
  regset_clear(use);
  regset_clear(def);

  memset_clear(memuse);
  memset_clear(memdef);

  opc_handler_t **table, *handler;
  uint32_t opcode;
  memcpy(&opcode, bincode, sizeof opcode);

  table = opcodes;
  handler = table[opc1(opcode)];

  DBG(PPC_DISAS, "opcode=%x. opc1=%x, handler=%p, "
      "handler->usedef=%p.\n"
      "invalid_usedef=%p.\n",
      opcode, opc1(opcode), handler, handler->usedef,
      &usedef_invalid);

  if (is_indirect_opcode(handler)) {
    table = ind_table(handler);
    handler = table[opc2(opcode)];
    if (is_indirect_opcode(handler)) {
      table = ind_table(handler);
      handler = table[opc3(opcode)];
    }
  }

  // Is opcode *REALLY* valid ?
  if (handler->usedef == &usedef_invalid) {
    return 0;
    //assert(0);
  } else {
    if ((opcode & handler->inval) != 0) {
      fprintf(stderr, "invalid bits %08x: "
          "%02x -%02x - %02x\n",
          opcode, opc1(opcode),
          opc2(opcode), opc3(opcode));
      return 0;
      //assert (0);
    }
  }

  (*(handler->usedef))(opcode, use, def, memuse, memdef);

  DBG(PPC_DISAS, "use = %s, def = %s\n",
      regset_to_string(use, as1, sizeof as1),
      regset_to_string(def, as2, sizeof as2));
  //dbg ("%s", "calling regset_check(use)\n");
  //regset_check(use);
  //dbg ("%s", "calling regset_check(def)\n");
  //regset_check(def);
  //dbg ("%s", "returned from regset_check(def)\n");
  return 4;
}*/
