#ifndef __PPC_USEDEF_C
#define __PPC_USEDEF_C

#include <assert.h>
#include "support/debug.h"
#include "ppc/regs.h"
#include "valtag/regset.h"
#include "rewrite/peephole.h"
#include "valtag/memset.h"
#include "valtag/state_elem_desc.h"

static char as1[4096];

#define USEDEF_HANDLER(name)                 \
static void usedef_##name(uint32_t opcode,regset_t *use,regset_t *def,\
    memset_t *memuse, memset_t *memdef)

/* Invalid instruction */
USEDEF_HANDLER(invalid)
{
  ASSERT(0);
}

/***                           Integer arithmetic                          ***/
#define __USEDEF_INT_ARITH2(name)                    \
USEDEF_HANDLER(name)                       \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    if (Rc(opcode) != 0) { \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    } \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}


#define __USEDEF_INT_ARITH2_O(name)                    \
USEDEF_HANDLER(name)                       \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    if (Rc(opcode) != 0) { \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    } \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_OV, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_OV))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
}

#define __USEDEF_INT_ARITH2c(name)                    \
USEDEF_HANDLER(name)                       \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
    if (Rc(opcode) != 0) { \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

#define __USEDEF_INT_ARITH2c_O(name)                    \
USEDEF_HANDLER(name)                       \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
    if (Rc(opcode) != 0) {\
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_OV, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_OV))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
}

#define __USEDEF_INT_ARITH2e(name)                    \
USEDEF_HANDLER(name)                       \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
    if (Rc(opcode) != 0) {\
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
}

#define __USEDEF_INT_ARITH2e_O(name)                    \
USEDEF_HANDLER(name)                       \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
    if (Rc(opcode) != 0) {\
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_OV, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_OV))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
}



#define __USEDEF_INT_ARITH1(name) \
USEDEF_HANDLER(name) \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    if (Rc(opcode) != 0) {\
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}
#define __USEDEF_INT_ARITH1_O(name) \
USEDEF_HANDLER(name) \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    if (Rc(opcode) != 0) {\
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_OV, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_OV))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
}

#define __USEDEF_INT_ARITH1e(name) \
USEDEF_HANDLER(name) \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
    if (Rc(opcode) != 0) {\
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
}
#define __USEDEF_INT_ARITH1e_O(name) \
USEDEF_HANDLER(name) \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
    if (Rc(opcode) != 0) {\
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_OV, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_OV))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
}



/* Two operands arithmetic functions */
#define USEDEF_INT_ARITH2(name)                             \
__USEDEF_INT_ARITH2(name) \
__USEDEF_INT_ARITH2_O(name##o)

/* Two operands arithmetic functions */
#define USEDEF_INT_ARITH2c(name)                             \
__USEDEF_INT_ARITH2c(name) \
__USEDEF_INT_ARITH2c_O(name##o)

/* Two operands arithmetic functions */
#define USEDEF_INT_ARITH2e(name)                             \
__USEDEF_INT_ARITH2e(name) \
__USEDEF_INT_ARITH2e_O(name##o)



/* Two operands arithmetic functions with no overflow allowed */
#define USEDEF_INT_ARITHN(name) \
__USEDEF_INT_ARITH2(name)

/* One operand arithmetic functions */
#define USEDEF_INT_ARITH1(name) \
__USEDEF_INT_ARITH1(name) \
__USEDEF_INT_ARITH1_O(name##o)

#define USEDEF_INT_ARITH1e(name) \
__USEDEF_INT_ARITH1e(name) \
__USEDEF_INT_ARITH1e_O(name##o)


/* add    add.    addo    addo.    */
USEDEF_INT_ARITH2 (add);
/* addc   addc.   addco   addco.   */
USEDEF_INT_ARITH2c (addc);
/* adde   adde.   addeo   addeo.   */
USEDEF_INT_ARITH2e (adde);
/* addme  addme.  addmeo  addmeo.  */
USEDEF_INT_ARITH1e (addme);
/* addze  addze.  addzeo  addzeo.  */
USEDEF_INT_ARITH1e (addze);
/* divw   divw.   divwo   divwo.   */
USEDEF_INT_ARITH2 (divw);
/* divwu  divwu.  divwuo  divwuo.  */
USEDEF_INT_ARITH2 (divwu);
/* mulhw  mulhw.                   */
USEDEF_INT_ARITHN (mulhw);
/* mulhwu mulhwu.                  */
USEDEF_INT_ARITHN (mulhwu);
/* mullw  mullw.  mullwo  mullwo.  */
USEDEF_INT_ARITH2 (mullw);
/* neg    neg.    nego    nego.    */
USEDEF_INT_ARITH1 (neg);
/* subf   subf.   subfo   subfo.   */
USEDEF_INT_ARITH2 (subf);
/* subfc  subfc.  subfco  subfco.  */
USEDEF_INT_ARITH2c (subfc);
/* subfe  subfe.  subfeo  subfeo.  */
USEDEF_INT_ARITH2e (subfe);
/* subfme subfme. subfmeo subfmeo. */
USEDEF_INT_ARITH1e (subfme);
/* subfze subfze. subfzeo subfzeo. */
USEDEF_INT_ARITH1e (subfze);
/* addi */
USEDEF_HANDLER(addi)
{
  if (rA(opcode) != 0) {
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  }
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* addic */
USEDEF_HANDLER(addic)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* addic. */
USEDEF_HANDLER(addic_)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* addis */
USEDEF_HANDLER(addis)
{
    if (rA(opcode) != 0) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* mulli */
USEDEF_HANDLER(mulli)
{
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode),
      MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode),
      MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* subfic */
USEDEF_HANDLER(subfic)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/***                           Integer comparison                          ***/
#define USEDEF_CMP(name)                                                 \
USEDEF_HANDLER(name) \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
}

/* cmp */
USEDEF_CMP(cmp);
/* cmpi */
USEDEF_HANDLER(cmpi)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
}
/* cmpl */
USEDEF_CMP(cmpl);
/* cmpli */
USEDEF_HANDLER(cmpli)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
}

/***                            Integer logical                            ***/
#define __USEDEF_LOGICAL2(name)                                   \
USEDEF_HANDLER(name) \
{                                                                             \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    if (Rc(opcode) != 0) { \
        regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
        regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}
#define USEDEF_LOGICAL2(name)                                               \
__USEDEF_LOGICAL2(name)

#define USEDEF_LOGICAL1(name)                                               \
USEDEF_HANDLER(name) \
{                                                                             \
    DBG (PPC_DISAS, "computing usedef_logical1, opcode %x, rS=%d,rA=%d\n", opcode, rS(opcode), rA(opcode));		\
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    if (Rc(opcode) != 0) {\
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); \
    }\
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

/* and & and. */
USEDEF_LOGICAL2(and);
/* andc & andc. */
USEDEF_LOGICAL2(andc);
/* andi. */
USEDEF_HANDLER(andi_)
{
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* andis. */
USEDEF_HANDLER(andis_)
{
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* cntlzw */
USEDEF_LOGICAL1(cntlzw);
/* eqv & eqv. */
USEDEF_LOGICAL2(eqv);
/* extsb & extsb. */
USEDEF_LOGICAL1(extsb);
/* extsh & extsh. */
USEDEF_LOGICAL1(extsh);
/* nand & nand. */
USEDEF_LOGICAL2(nand);
/* nor & nor. */
USEDEF_LOGICAL2(nor);

/* or & or. */
USEDEF_HANDLER(or)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    /* Optimisation for mr case */
    if (rS(opcode) != rB(opcode)) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    if (Rc(opcode) != 0) {
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    }
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* orc & orc. */
USEDEF_LOGICAL2(orc);
/* xor & xor. */
USEDEF_HANDLER(xor)
{
    /* Optimisation for "set to zero" case */
    if (rS(opcode) != rB(opcode)) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    if (Rc(opcode) != 0) {
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    }
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* ori */
USEDEF_HANDLER(ori)
{
  if (rS(opcode) == rA(opcode) && UIMM(opcode) == 0) {
    /* NOP */
    return;
  }
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* oris */
USEDEF_HANDLER(oris)
{
    if (rS(opcode) == rA(opcode) && UIMM(opcode) == 0) {
        /* NOP */
        return;
        }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* xori */
USEDEF_HANDLER(xori)
{
    if (rS(opcode) == rA(opcode) && UIMM(opcode) == 0) {
        /* NOP */
        return;
    }

    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* xoris */
USEDEF_HANDLER(xoris)
{
    if (rS(opcode) == rA(opcode) && UIMM(opcode) == 0) {
        /* NOP */
        return;
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/***                             Integer rotate                            ***/
/* rlwimi & rlwimi. */
USEDEF_HANDLER(rlwimi)
{
    uint32_t mb, me;

    mb = MB(opcode);
    me = ME(opcode);

    /* if (mb == me)
      return; */
    
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    if (Rc(opcode) != 0) {
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    }
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* rlwinm & rlwinm. */
USEDEF_HANDLER(rlwinm)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    if (Rc(opcode) != 0) {
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    }
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}
/* rlwnm & rlwnm. */
USEDEF_HANDLER(rlwnm)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    if (Rc(opcode) != 0) {
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    }
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/***                             Integer shift                             ***/
/* slw & slw. */
__USEDEF_LOGICAL2(slw);
/* sraw & sraw. */
USEDEF_HANDLER(sraw)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    if (Rc(opcode) != 0) {
        regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
        regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    }
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA)));
}
/* srawi & srawi. */
USEDEF_HANDLER(srawi)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    DBG2(SRC_USEDEF, "opcode=0x%x\n", opcode);
    if (Rc(opcode) != 0) {
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    }
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA)));
}
/* srw & srw. */
__USEDEF_LOGICAL2(srw);

/***                       Floating-Point arithmetic                       ***/
#define _USEDEF_FLOAT_ACB(name)                        \
USEDEF_HANDLER(f##name)                   \
{                                                                             \
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

#define USEDEF_FLOAT_ACB(name)                                            \
_USEDEF_FLOAT_ACB(name);                                   \
_USEDEF_FLOAT_ACB(name##s);

#define _USEDEF_FLOAT_AB(name)                     \
USEDEF_HANDLER(f##name)                     \
{                                                                             \
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}
#define USEDEF_FLOAT_AB(name)                                     \
_USEDEF_FLOAT_AB(name);                               \
_USEDEF_FLOAT_AB(name##s);

#define _USEDEF_FLOAT_AC(name)                     \
USEDEF_HANDLER(f##name)                        \
{                                                                             \
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}
#define USEDEF_FLOAT_AC(name) \
_USEDEF_FLOAT_AC(name); \
_USEDEF_FLOAT_AC(name##s);

#define USEDEF_FLOAT_B(name)                                           \
USEDEF_HANDLER(f##name)                   \
{                                                                             \
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

#define USEDEF_FLOAT_BS(name)                                          \
USEDEF_HANDLER(f##name)                   \
{                                                                             \
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/* fadd - fadds */
USEDEF_FLOAT_AB(add)
/* fdiv - fdivs */
USEDEF_FLOAT_AB(div)
/* fmul - fmuls */
USEDEF_FLOAT_AC(mul)

/* fres */
USEDEF_FLOAT_BS(res)

/* frsqrte */
USEDEF_FLOAT_BS(rsqrte);

/* fsel */
_USEDEF_FLOAT_ACB(sel);
/* fsub - fsubs */
USEDEF_FLOAT_AB(sub);
/* Optional: */
/* fsqrt */
USEDEF_HANDLER(fsqrt)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

USEDEF_HANDLER(fsqrts)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/***                     Floating-Point multiply-and-add                   ***/
/* fmadd - fmadds */
USEDEF_FLOAT_ACB(madd);
/* fmsub - fmsubs */
USEDEF_FLOAT_ACB(msub);
/* fnmadd - fnmadds */
USEDEF_FLOAT_ACB(nmadd);
/* fnmsub - fnmsubs */
USEDEF_FLOAT_ACB(nmsub);

/***                     Floating-Point round & convert                    ***/
/* fctiw */
USEDEF_FLOAT_B(ctiw);
/* fctiwz */
USEDEF_FLOAT_B(ctiwz);
/* frsp */
USEDEF_FLOAT_B(rsp);

/***                         Floating-Point compare                        ***/
/* fcmpo */
USEDEF_HANDLER(fcmpo)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/* fcmpu */
USEDEF_HANDLER(fcmpu)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/***                         Floating-point move                           ***/
/* fabs */
USEDEF_FLOAT_B(abs);

/* fmr  - fmr. */
USEDEF_HANDLER(fmr)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/* fnabs */
USEDEF_FLOAT_B(nabs);
/* fneg */
USEDEF_FLOAT_B(neg);

/***                  Floating-Point status & ctrl register                ***/
/* mcrfs */
USEDEF_HANDLER(mcrfs)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/* mffs */
USEDEF_HANDLER(mffs)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/* mtfsb0 */
USEDEF_HANDLER(mtfsb0)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/* mtfsb1 */
USEDEF_HANDLER(mtfsb1)
{
  ASSERT(0);
}

/* mtfsf */
USEDEF_HANDLER(mtfsf)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/* mtfsfi */
USEDEF_HANDLER(mtfsfi)
{
  /*regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, PPC_REG_UNKNOWN, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); */\
}

/***                             Integer load                              ***/
#define USEDEF_LD(width)                                                    \
USEDEF_HANDLER(l##width) \
{                                                                             \
  uint32_t simm = SIMM(opcode); \
  if (rA(opcode) != 0) {                                               \
      memuse->memaccess[memuse->num_memaccesses].base = rA(opcode); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  }                                                                         \
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  memuse->memaccess[memuse->num_memaccesses].disp.val = simm; \
  memuse->memaccess[memuse->num_memaccesses].disp.tag = tag_const; \
  memuse->num_memaccesses++;  \
}

#define USEDEF_LDU(width)                                                   \
USEDEF_HANDLER(l##width##u)            \
{                                                                             \
  uint32_t simm = SIMM(opcode); \
  if (rA(opcode) == 0 ||                                               \
      rA(opcode) == rD(opcode)) {                                 \
      return;                                                               \
  }                                                                         \
  memuse->memaccess[memuse->num_memaccesses].base = rA(opcode); \
  memuse->memaccess[memuse->num_memaccesses].disp.val = simm; \
  memuse->memaccess[memuse->num_memaccesses].disp.tag = tag_const; \
  memuse->num_memaccesses++; \
 \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

#define USEDEF_LDUX(width)                                                  \
USEDEF_HANDLER(l##width##ux)           \
{                                                                             \
    if (rA(opcode) == 0 ||                                               \
        rA(opcode) == rD(opcode)) {                                 \
        return;                                                               \
    }                                                                         \
    memuse->memaccess[memuse->num_memaccesses].base = rA(opcode); \
    memuse->memaccess[memuse->num_memaccesses].index = rB(opcode); \
    memuse->num_memaccesses++; \
  \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

#define USEDEF_LDX(width)                                            \
USEDEF_HANDLER(l##width##x)           \
{                                                                             \
    memuse->memaccess[memuse->num_memaccesses].base = -1; \
    memuse->memaccess[memuse->num_memaccesses].index = -1; \
    memuse->memaccess[memuse->num_memaccesses].disp.val = 0; \
    memuse->memaccess[memuse->num_memaccesses].disp.tag = tag_const; \
    if (rA(opcode) != 0) {                                               \
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
        memuse->memaccess[memuse->num_memaccesses].base = rA(opcode); \
    }                                                                         \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    memuse->memaccess[memuse->num_memaccesses].index = rB(opcode); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    memuse->num_memaccesses++; \
}

#define USEDEF_LDS(width)                                                    \
USEDEF_LD(width);                                                     \
USEDEF_LDU(width);                                                    \
USEDEF_LDUX(width);                                                   \
USEDEF_LDX(width)

/* lbz lbzu lbzux lbzx */
USEDEF_LDS(bz);
/* lha lhau lhaux lhax */
USEDEF_LDS(ha);
/* lhz lhzu lhzux lhzx */
USEDEF_LDS(hz);
/* lwz lwzu lwzux lwzx */
USEDEF_LDS(wz);

/***                              Integer store                            ***/
#define USEDEF_ST(width)                                                    \
USEDEF_HANDLER(st##width)              \
{                                                                             \
    uint32_t simm = SIMM(opcode); \
    if (rA(opcode) != 0) {                                               \
      memdef->memaccess[memdef->num_memaccesses].base = rA(opcode); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    }                                                                         \
    memdef->memaccess[memdef->num_memaccesses].disp.val = simm; \
    memdef->memaccess[memdef->num_memaccesses].disp.tag = tag_const; \
    memdef->num_memaccesses++;  \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

#define USEDEF_STU(width)                                                   \
USEDEF_HANDLER(st##width##u)           \
{                                                                             \
    uint32_t simm = SIMM(opcode); \
    if (rA(opcode) == 0) {                                               \
        return;                                                               \
    }                                                                         \
    memdef->memaccess[memdef->num_memaccesses].base = rA(opcode); \
    memdef->memaccess[memdef->num_memaccesses].disp.val = simm; \
    memdef->memaccess[memdef->num_memaccesses].disp.tag = tag_const; \
    memdef->num_memaccesses++; \
  \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

#define USEDEF_STUX(width)                                                  \
USEDEF_HANDLER(st##width##ux)          \
{                                                                             \
    /*if (rA(opcode) == 0) {                                               \
      NOT_REACHED(); \
      return;                                                               \
    }  */                                                                     \
    memdef->memaccess[memdef->num_memaccesses].base = rA(opcode); \
    memdef->memaccess[memdef->num_memaccesses].index = rB(opcode); \
    memdef->num_memaccesses++; \
  \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

#define USEDEF_STX(width)                                            \
USEDEF_HANDLER(st##width##x)          \
{                                                                             \
    if (rA(opcode) == 0) {                                               \
        memdef->memaccess[memdef->num_memaccesses].index = rB(opcode); \
      \
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    } else {                                                                  \
        memdef->memaccess[memdef->num_memaccesses].base = rA(opcode); \
        memdef->memaccess[memdef->num_memaccesses].index = rB(opcode); \
      \
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
    }                                                                         \
    memdef->num_memaccesses++; \
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

#define USEDEF_STS(width)                                                    \
USEDEF_ST(width);                                                     \
USEDEF_STU(width);                                                    \
USEDEF_STUX(width);                                                   \
USEDEF_STX(width);

/* stb stbu stbux stbx */
USEDEF_STS(b);
/* sth sthu sthux sthx */
USEDEF_STS(h);
/* stw stwu stwux stwx */
USEDEF_STS(w);

/***                Integer load and store with byte reverse               ***/
/* lhbrx */
USEDEF_LDX(hbr);
/* lwbrx */
USEDEF_LDX(wbr);
/* sthbrx */
USEDEF_STX(hbr);
/* stwbrx */
USEDEF_STX(wbr);

/***                    Integer load and store multiple                    ***/
/* lmw */
USEDEF_HANDLER(lmw)
{
    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }

    int i;
    for (i = rD(opcode); i < PPC_NUM_GPRS; i++)
    {
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
}

/* stmw */
USEDEF_HANDLER(stmw)
{
    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }

    int i;
    for (i = rD(opcode); i < PPC_NUM_GPRS; i++)
    {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }

}

/***                    Integer load and store strings                     ***/

/* lswi */
/* PowerPC32 specification says we must generate an exception if
 * rA is in the range of registers to be loaded.
 * In an other hand, IBM says this is valid, but rA won't be loaded.
 * For now, I'll follow the spec...
 */
USEDEF_HANDLER(lswi)
{
    int nb = NB(opcode);
    int start = rD(opcode);
    int ra = rA(opcode);
    int nr;

    if (nb == 0)
        nb = 32;
    nr = nb / 4;
    if (((start + nr) > 32  && start <= ra && (start + nr - 32) > ra) ||
        ((start + nr) <= 32 && start <= ra && (start + nr) > ra)) {
        return;
    }
    if (ra != 0) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, ra, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    //regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC_CMP, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC_CMP)));//actually only bc
    //op_ldsts(lswi, start);//XXX: conservatively assume nothing is def'd
}

/* lswx */
USEDEF_HANDLER(lswx)
{
    int ra = rA(opcode);
    int rb = rB(opcode);

    if (ra == 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rb, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    } else {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, ra, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rb, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    //regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC_CMP, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC_CMP))); //actually only bc
    //op_ldstsx(lswx, rD(opcode), ra, rb);//XXX: conservatively assume nothing is def'd
}

/* stswi */
USEDEF_HANDLER(stswi)
{
    int nb = NB(opcode);

    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    if (nb == 0)
        nb = 32;

    //op_ldsts(stsw, rS(opcode));
    //XXX: conservatively assume, everything is used
    int i;
    for (i = 0; i < PPC_NUM_GPRS; i++)
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* stswx */
USEDEF_HANDLER(stswx)
{
    int ra = rA(opcode);

    if (ra != 0) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, ra, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    //regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC_CMP, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC_CMP)));//actually only bc
    //op_ldsts(stsw, rS(opcode));
    //XXX: conservatively assume, everything is used
    int i;
    for (i = 0; i < PPC_NUM_GPRS; i++)
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/***                        Memory synchronisation                         ***/
/* eieio */
USEDEF_HANDLER(eieio)
{
}

/* isync */
USEDEF_HANDLER(isync)
{
}

/* lwarx */
USEDEF_HANDLER(lwarx)
{
    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* stwcx. */
USEDEF_HANDLER(stwcx_)
{
        if (rA(opcode) != 0) {
            regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
	}
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
}

/* sync */
USEDEF_HANDLER(sync)
{
}

/***                         Floating-point load                           ***/
#define USEDEF_LDF(width)                                                   \
USEDEF_HANDLER(l##width)  \
{                                                                             \
  /* This fp instruction is sometimes used to read machine state */ \
  uint32_t simm = SIMM(opcode); \
  if (rA(opcode) != 0) {                                               \
      memuse->memaccess[memuse->num_memaccesses].base = rA(opcode); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  }                                                                         \
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_FPREG, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_FPREG))); \
  memuse->memaccess[memuse->num_memaccesses].disp.val = simm; \
  memuse->memaccess[memuse->num_memaccesses].disp.tag = tag_const; \
  memuse->num_memaccesses++;  \
}

#define USEDEF_LDUF(width)                                                  \
USEDEF_HANDLER(l##width##u) \
{                                                                             \
  /* This fp instruction is sometimes used to read machine state */ \
  uint32_t simm = SIMM(opcode); \
  if (rA(opcode) == 0) {                                 \
      return;                                                               \
  }                                                                         \
  memuse->memaccess[memuse->num_memaccesses].base = rA(opcode); \
  memuse->memaccess[memuse->num_memaccesses].disp.val = simm; \
  memuse->memaccess[memuse->num_memaccesses].disp.tag = tag_const; \
  memuse->num_memaccesses++; \
 \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_FPREG, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_FPREG))); \
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

#define USEDEF_LDUXF(width)                                                 \
USEDEF_HANDLER(l##width##ux) \
{                                                                             \
  /* This fp instruction is sometimes used to read machine state */ \
}

#define USEDEF_LDXF(width)                                           \
USEDEF_HANDLER(l##width##x) \
{                                                                             \
  /* This fp instruction is sometimes used to read machine state */ \
}

#define USEDEF_LDFS(width)                                                   \
USEDEF_LDF(width);                                                    \
USEDEF_LDUF(width);                                                   \
USEDEF_LDUXF(width);                                                  \
USEDEF_LDXF(width)

/* lfd lfdu lfdux lfdx */
USEDEF_LDFS(fd);
/* lfs lfsu lfsux lfsx */
USEDEF_LDFS(fs);

/***                         Floating-point store                          ***/
#define USEDEF_STF(width)                                                   \
USEDEF_HANDLER(st##width) \
{                                                                             \
  /* This fp instruction is sometimes used to read machine state */ \
  uint32_t simm = SIMM(opcode); \
  if (rA(opcode) != 0) {                                               \
      memuse->memaccess[memuse->num_memaccesses].base = rA(opcode); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  }                                                                         \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_FPREG, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_FPREG))); \
  memuse->memaccess[memuse->num_memaccesses].disp.val = simm; \
  memuse->memaccess[memuse->num_memaccesses].disp.tag = tag_const; \
  memuse->num_memaccesses++;  \
}

#define USEDEF_STUF(width)                                                  \
USEDEF_HANDLER(st##width##u) \
{                                                                             \
  /* This function is sometimes used to dump machine state */ \
  uint32_t simm = SIMM(opcode); \
  if (rA(opcode) == 0) {                                 \
      return;                                                               \
  }                                                                         \
  memuse->memaccess[memuse->num_memaccesses].base = rA(opcode); \
  memuse->memaccess[memuse->num_memaccesses].disp.val = simm; \
  memuse->memaccess[memuse->num_memaccesses].disp.tag = tag_const; \
  memuse->num_memaccesses++; \
 \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_FPREG, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_FPREG))); \
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS))); \
}

#define USEDEF_STUXF(width)                                                 \
USEDEF_HANDLER(st##width##ux) \
{                                                                             \
  /* This function is sometimes used to dump machine state */ \
}

#define USEDEF_STXF(width)                                           \
USEDEF_HANDLER(st##width##x) \
{                                                                             \
  /* This function is sometimes used to dump machine state */ \
    /*ASSERT(0);*/ \
}

#define USEDEF_STFS(width)                                                   \
USEDEF_STF(width);                                                    \
USEDEF_STUF(width);                                                   \
USEDEF_STUXF(width);                                                  \
USEDEF_STXF(width)

/* stfd stfdu stfdux stfdx */
USEDEF_STFS(fd);
/* stfs stfsu stfsux stfsx */
USEDEF_STFS(fs);

/* Optional: */
/* stfiwx */
USEDEF_HANDLER(stfiwx)
{
    ASSERT(0);
}

/***                                Branch                                 ***/

/* b ba bl bla */
USEDEF_HANDLER(b)
{
    if (LK(opcode)) {
        regset_mark_ex_mask(def, PPC_EXREG_GROUP_LR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_LR)));
    }
}

#define BCOND_IM  0
#define BCOND_LR  1
#define BCOND_CTR 2

static inline void
usedef_bcond(uint32_t opcode, int type, regset_t *use, regset_t *def) 
{                                                                             
  uint32_t bo = BO(opcode);                                            
  uint32_t bi = BI(opcode);                                            
  DBG2(SRC_USEDEF, "bi=%d\n", bi);
  if ((bo & 0x4) == 0) {
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
  }
  switch(type) {
  case BCOND_IM:
    break;
  case BCOND_CTR:
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
    break;
  default:
  case BCOND_LR:
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_LR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_LR)));
    break;
  }
  if (LK(opcode)) {                                        
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_LR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_LR)));
  }
  if (bo & 0x10) {
    /* No CR condition */                                                 
    switch (bo & 0x6) {                                                   
    case 0:                                                               
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
      break;
    case 2:                                                               
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
      break;                                                            
    default:
    case 4:                                                               
    case 6:                                                               
      return;
    }
  } else {                                                                  
    int bitnum = (3 - (bi & 0x03));

    if (bitnum == 3 - PPC_INSN_CRF_BIT_SO) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF_SO, (bi >> 2), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
    } else {
      uint32_t mask = 1 << bitnum;                                        
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF, (bi >> 2), mask);
    }
    DBG(SRC_USEDEF, "marking fine flags for bit %d [crf %d]: use: %s\n", bi,
        bi >> 2, regset_to_string(use, as1, sizeof as1));
    if (bo & 0x8) {                                                       
      switch (bo & 0x6) {                                               
        case 0:                                                           
          regset_mark_ex_mask(use, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
          break;                                                        
        case 2:                                                           
          regset_mark_ex_mask(use, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
          break;                                                        
        default:                                                          
        case 4:                                                           
        case 6:                                                           
          break;                                                        
      }                                                                 
    } else {                                                              
      switch (bo & 0x6) {                                               
        case 0:                                                           
          regset_mark_ex_mask(use, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
          break;                                                        
        case 2:                                                           
          regset_mark_ex_mask(use, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
          break;                                                        
        default:
        case 4:                                                           
        case 6:                                                           
          break;                                                        
      }                                                                 
    }                                                                     
  }                                                                         
}

USEDEF_HANDLER(bc)
{                                                                             
  usedef_bcond(opcode, BCOND_IM, use, def);
}

USEDEF_HANDLER(bcctr)
{                                                                             
  usedef_bcond(opcode, BCOND_CTR, use, def);
}

USEDEF_HANDLER(bclr)
{                                                                             
  usedef_bcond(opcode, BCOND_LR, use, def);
}

/***                      Condition register logical                       ***/
#define USEDEF_CRLOGIC(op)                                                  \
USEDEF_HANDLER(cr##op) \
{                                                                             \
    if ((crbA(opcode) % 4) == PPC_INSN_CRF_BIT_SO) { \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF_SO, (crbA(opcode) >> 2), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
    } else { \
      uint32_t mask = (1 << (3 - (crbA(opcode) & 3))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF, \
          (crbA(opcode) >> 2), mask); \
    } \
    if ((crbB(opcode) % 4) == PPC_INSN_CRF_BIT_SO) { \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF_SO, (crbB(opcode) >> 2), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
    } else { \
      uint32_t mask = (1 << (3 - (crbB(opcode) & 3))); \
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF, \
          (crbB(opcode) >> 2), mask); \
    } \
    if ((crbD(opcode) % 4) == PPC_INSN_CRF_BIT_SO) { \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, (crbD(opcode) >> 2), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO))); \
    } else { \
      uint32_t mask = (1 << (3 - (crbD(opcode) & 3))); \
      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, \
          (crbD(opcode) >> 2), mask); \
    } \
}

/* crand */
USEDEF_CRLOGIC(and);
/* crandc */
USEDEF_CRLOGIC(andc);
/* creqv */
USEDEF_CRLOGIC(eqv);
/* crnand */
USEDEF_CRLOGIC(nand);
/* crnor */
USEDEF_CRLOGIC(nor);
/* cror */
USEDEF_CRLOGIC(or);
/* crorc */
USEDEF_CRLOGIC(orc);
/* crxor */
USEDEF_CRLOGIC(xor);
/* mcrf */
USEDEF_HANDLER(mcrf)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF, crfS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF_SO, crfS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
}

/***                           System linkage                              ***/
/* rfi (supervisor only) */
USEDEF_HANDLER(rfi)
{
  ASSERT(0);
}


/* sc */
USEDEF_HANDLER(sc)
{
#if 0
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO))); /* mark xer_so */

  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, 3, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, 4, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, 5, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, 6, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, 7, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, 8, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));

  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, 3, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, 4, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, 5, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, 6, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, 7, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
  regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, 8, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
#endif
}

/***                                Trap                                   ***/
/* tw */
USEDEF_HANDLER(tw)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* twi */
USEDEF_HANDLER(twi)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}


/* mcrxr */
USEDEF_HANDLER(mcrxr)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_OV, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_OV)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, crfD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_OV, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_OV)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
}

/* mfcr */
USEDEF_HANDLER(mfcr)
{
    int i;
    for (i = 0; i < PPC_NUM_CRFREGS; i++) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF, i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF_SO, i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
    }
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}


USEDEF_HANDLER(mfmsr)
{
  ASSERT(0);
}

/* mfspr */
static inline void usedef_op_mfspr (uint32_t opcode, regset_t *use,
    regset_t *def)
{
    uint32_t sprn = SPR(opcode);

    switch (sprn)
    {
      case 1:
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_OV, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_OV)));
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA)));
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
	//regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC)));
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC_CMP, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC_CMP)));//actually only bc
	break;
      case 8:
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_LR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_LR)));
	break;
      case 9:
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
	break;
      default:
	//err ("Invalid sprn: %d\n", sprn);
	//ASSERT(0);
	break;
    }
    
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

USEDEF_HANDLER(mfspr)
{
    usedef_op_mfspr(opcode, use, def);
}

/* mftb */
USEDEF_HANDLER(mftb)
{
    usedef_op_mfspr(opcode, use, def);
}

/* mtcrf */
/* The mask should be 0x00100801, but Mac OS X 10.4 use an alternate form */
USEDEF_HANDLER(mtcrf)
{
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    uint8_t crm = CRM(opcode);

    int i;
    for (i = 0; i < PPC_NUM_CRFREGS; i++) {
      //we do not need to mark CRF to use, do we?
      //regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF, i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
      //regset_mark_ex_mask(use, PPC_EXREG_GROUP_CRF_SO, i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
      if (crm & (1 << i)) {
	      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF, 7 - i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF)));
	      regset_mark_ex_mask(def, PPC_EXREG_GROUP_CRF_SO, 7 - i, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CRF_SO)));
      }
    }
}

/* mtmsr */
USEDEF_HANDLER(mtmsr)
{
  ASSERT(0);
}

/* mtspr */
USEDEF_HANDLER(mtspr)
{
    uint32_t sprn = SPR(opcode);

    switch (sprn)
    {
      case 1:
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_OV, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_OV)));
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_CA, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_CA)));
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_SO, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_SO)));
	//regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC)));
	regset_mark_ex_mask(use, PPC_EXREG_GROUP_XER_BC_CMP, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_XER_BC_CMP)));//actually only bc
	break;
      case 8:
	regset_mark_ex_mask(def, PPC_EXREG_GROUP_LR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_LR)));
	break;
      case 9:
	regset_mark_ex_mask(def, PPC_EXREG_GROUP_CTR, 0, MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_CTR)));
	break;
      default:
	err ("Invalid sprn: %d\n", sprn);
	ASSERT(0);
	break;
    }
 
  regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/***                         Cache management                              ***/
/* For now, all those will be implemented as nop:
 * this is valid, regarding the PowerPC specs...
 * We just have to flush tb while invalidating instruction cache lines...
 */
/* dcbf */
USEDEF_HANDLER(dcbf)
{
    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* dcbi (Supervisor only) */
USEDEF_HANDLER(dcbi)
{
  ASSERT(0);
}

/* dcdst */
USEDEF_HANDLER(dcbst)
{
    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* dcbt */
USEDEF_HANDLER(dcbt)
{
    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* dcbtst */
USEDEF_HANDLER(dcbtst)
{
    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* dcbz */

USEDEF_HANDLER(dcbz)
{
    if (rA(opcode) != 0) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* icbi */
USEDEF_HANDLER(icbi)
{
    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* Optional: */
/* dcba */
USEDEF_HANDLER(dcba)
{
    if (rA(opcode) != 0) {
        regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/***                    Segment register manipulation                      ***/
/* Supervisor only: */
/* mfsr */
USEDEF_HANDLER(mfsr)
{
  ASSERT(0);
}

/* mfsrin */
USEDEF_HANDLER(mfsrin)
{
  ASSERT(0);
}

/* mtsr */
USEDEF_HANDLER(mtsr)
{
  ASSERT(0);
}

/* mtsrin */
USEDEF_HANDLER(mtsrin)
{
  ASSERT(0);
}

/***                      Lookaside buffer management                      ***/
/* Optional & supervisor only: */
/* tlbia */
USEDEF_HANDLER(tlbia)
{
  ASSERT(0);
}

/* tlbie */
USEDEF_HANDLER(tlbie)
{
  ASSERT(0);
}

/* tlbsync */
USEDEF_HANDLER(tlbsync)
{
  ASSERT(0);
}

/***                              External control                         ***/
/* Optional: */
/* eciwx */
USEDEF_HANDLER(eciwx)
{
    if (rA(opcode) != 0) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(def, PPC_EXREG_GROUP_GPRS, rD(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

/* ecowx */
USEDEF_HANDLER(ecowx)
{
    if (rA(opcode) != 0) {
      regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rA(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    }
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rB(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
    regset_mark_ex_mask(use, PPC_EXREG_GROUP_GPRS, rS(opcode), MAKE_MASK(PPC_EXREG_LEN(PPC_EXREG_GROUP_GPRS)));
}

#endif  /* ppc/usedef.c */
