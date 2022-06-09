#ifndef __PPC_REGSET_H
#define __PPC_REGSET_H

#define PPC_EXREG_GROUP_GPRS 0
#define PPC_EXREG_GROUP_FPREG 1
#define PPC_EXREG_GROUP_CRF 2
#define PPC_EXREG_GROUP_CRF_SO 3
#define PPC_EXREG_GROUP_LR 4
#define PPC_EXREG_GROUP_CTR 5
#define PPC_EXREG_GROUP_XER_OV 6
#define PPC_EXREG_GROUP_XER_CA 7
#define PPC_EXREG_GROUP_XER_SO 8
#define PPC_EXREG_GROUP_XER_BC_CMP 9

#define PPC_NUM_EXREG_GROUPS 10

#define PPC_NUM_GPRS 32
#define PPC_NUM_CRFREGS 8
#define PPC_NUM_FPREGS 32

#define PPC_SPR_NUM_XER_BC_CMP 1
#define PPC_SPR_NUM_CTR 9
#define PPC_SPR_NUM_LR 8

#define PPC_STATE_CRF_BIT_LT 3
#define PPC_STATE_CRF_BIT_GT 2
#define PPC_STATE_CRF_BIT_EQ 1
#define PPC_STATE_CRF_BIT_SO 0

#define PPC_INSN_CRF_BIT_SO (3 - PPC_STATE_CRF_BIT_SO)

#define PPC_NUM_EXREGS(i) ( \
    ((i) == PPC_EXREG_GROUP_GPRS) ? PPC_NUM_GPRS: \
    ((i) == PPC_EXREG_GROUP_FPREG) ? PPC_NUM_FPREGS : \
    ((i) == PPC_EXREG_GROUP_CRF) ? PPC_NUM_CRFREGS : \
    ((i) == PPC_EXREG_GROUP_CRF_SO) ? PPC_NUM_CRFREGS : \
    ((i) == PPC_EXREG_GROUP_LR) ? 1 : \
    ((i) == PPC_EXREG_GROUP_CTR) ? 1 : \
    ((i) == PPC_EXREG_GROUP_XER_OV) ? 1 : \
    ((i) == PPC_EXREG_GROUP_XER_CA) ? 1 : \
    ((i) == PPC_EXREG_GROUP_XER_SO) ? 1 : \
    ((i) == PPC_EXREG_GROUP_XER_BC_CMP) ? 1 : 0) /* should be in decreasing order */
#define PPC_EXREG_LEN(i) ( \
    ((i) == PPC_EXREG_GROUP_GPRS) ? DWORD_LEN: \
    ((i) == PPC_EXREG_GROUP_FPREG) ? QWORD_LEN: \
    ((i) == PPC_EXREG_GROUP_CRF) ? HBYTE_LEN: \
    ((i) == PPC_EXREG_GROUP_CRF_SO) ? BIT_LEN: \
    ((i) == PPC_EXREG_GROUP_LR) ? DWORD_LEN : \
    ((i) == PPC_EXREG_GROUP_CTR) ? DWORD_LEN : \
    ((i) == PPC_EXREG_GROUP_XER_OV) ? BIT_LEN : \
    ((i) == PPC_EXREG_GROUP_XER_CA) ? BIT_LEN : \
    ((i) == PPC_EXREG_GROUP_XER_SO) ? BIT_LEN : \
    ((i) == PPC_EXREG_GROUP_XER_BC_CMP) ? DWORD_LEN : 0)

#define PPC_EXREG_MASK(i) ( \
    ((i) == PPC_EXREG_GROUP_GPRS) ? MAKE_MASK(DWORD_LEN): \
    ((i) == PPC_EXREG_GROUP_FPREG) ? MAKE_MASK(QWORD_LEN): \
    ((i) == PPC_EXREG_GROUP_CRF) ? MAKE_MASK(HBYTE_LEN) & (~0x1ULL): \
    ((i) == PPC_EXREG_GROUP_CRF_SO) ? MAKE_MASK(BIT_LEN): \
    ((i) == PPC_EXREG_GROUP_LR) ? MAKE_MASK(DWORD_LEN): \
    ((i) == PPC_EXREG_GROUP_CTR) ? MAKE_MASK(DWORD_LEN): \
    ((i) == PPC_EXREG_GROUP_XER_OV) ? MAKE_MASK(BIT_LEN): \
    ((i) == PPC_EXREG_GROUP_XER_CA) ? MAKE_MASK(BIT_LEN): \
    ((i) == PPC_EXREG_GROUP_XER_SO) ? MAKE_MASK(BIT_LEN): \
    ((i) == PPC_EXREG_GROUP_XER_BC_CMP) ? MAKE_MASK(DWORD_LEN): 0)


/* offsets based on qemu-1.7.0 (also work with qemu-2.0.0) */
#define PPC_ENV_EXOFF(i, j) ( \
    ((i) == PPC_EXREG_GROUP_GPRS) ? (j) * 4 : \
    ((i) == PPC_EXREG_GROUP_FPREG) ? 0x168 + (j) * 8 : \
    ((i) == PPC_EXREG_GROUP_CRF) ? 0x108 + (j) * 4: \
    ((i) == PPC_EXREG_GROUP_CRF_SO) ? 0x138 + (j) * 4: \
    ((i) == PPC_EXREG_GROUP_LR) ? 0x100 : /* lr */\
    ((i) == PPC_EXREG_GROUP_CTR) ? 0x104 : /* ctr */\
    ((i) == PPC_EXREG_GROUP_XER_OV) ? 0x130 : /* ov */\
    ((i) == PPC_EXREG_GROUP_XER_CA) ? 0x134 : /* ca */\
    ((i) == PPC_EXREG_GROUP_XER_SO) ? 0x12c : /* so */\
    ((i) == PPC_EXREG_GROUP_XER_BC_CMP) ? 0x128 : /* xer_bc_cmp */ ({NOT_REACHED(); -1;}))

//#define PPC_NUM_REGS PPC_NUM_GPRS

#define ppc_regset_check(regset) do { \
  /*
  if (!_ppc_regset_check(regset)) { \
    dbg("ppc_regset_check() failed. bitmap=%" PRIx64 ", fine_flags=%x. regset=%s\n", \
	regset->bitmap, regset->fine_flags, \
	ppc_regset_to_string(regset,as1, sizeof as1)); \
    assert (0); \
  } */\
} while (0);

#if 0
#define ppc_regset_mark(regset, regno) do { \
  dbg ("call to ppc_regset_mark (%d)\n", regno); \
  dbg ("%s","calling ppc_regset_check\n"); \
  ppc_regset_check(regset); \
  dbg ("%s", "returned from call to ppc_regset_check\n"); \
  _ppc_regset_mark(regset, regno); \
  dbg ("%s","calling ppc_regset_check\n"); \
  ppc_regset_check(regset); \
  dbg ("%s", "returned from call to ppc_regset_check\n"); \
} while (0)
#endif

#define PPC_MEM_TO_EXREG_COST(group) (group == PPC_EXREG_GROUP_GPRS)?10:11
#define PPC_EXREG_TO_MEM_COST(group) (group == PPC_EXREG_GROUP_GPRS)?10:11
#define PPC_EXREG_TO_EXREG_COST(group1, group2) ({ASSERT(group1 != group2); 2;})

#define PPC_NUM_INVISIBLE_REGS 0
#define PPC_INVISIBLE_REGS_LEN(i) BYTE_LEN

#define PPC_NUM_SC_IMPLICIT_REGS 7

//char *ppc_regname(int regnum, char *buf, size_t buf_size);

//#define PPC_NUM_CRFS 8
#endif
