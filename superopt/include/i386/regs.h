#ifndef I386_REGSET_H
#define I386_REGSET_H

#include <stdbool.h>
#include "i386/cpu_consts.h"
#include "config-host.h"

#define I386_NUM_YMMREGS 16
#define I386_NUM_GPRS 8
#define I386_NUM_MMXREGS 8
#define I386_NUM_XMMREGS 8
#define I386_NUM_FP_DATA_REGS 8
#define I386_NUM_SEGS 6
#define I386_NUM_EFLAGS_OTHER 1
#define I386_NUM_EFLAGS_EQ 1
#define I386_NUM_EFLAGS_NE 1
#define I386_NUM_EFLAGS_ULT 1
#define I386_NUM_EFLAGS_SLT 1
#define I386_NUM_EFLAGS_UGT 1
#define I386_NUM_EFLAGS_SGT 1
#define I386_NUM_EFLAGS_ULE 1
#define I386_NUM_EFLAGS_SLE 1
#define I386_NUM_EFLAGS_UGE 1
#define I386_NUM_EFLAGS_SGE 1
#define I386_NUM_ST_TOP 1 //can also be called FP_STATUS_REG
#define I386_NUM_FP_CONTROL_REG_RM 1
#define I386_NUM_FP_CONTROL_REG_OTHER 1
#define I386_NUM_FP_TAG_REG 1
#define I386_NUM_FP_LAST_INSTR_POINTER 1
#define I386_NUM_FP_LAST_OPERAND_POINTER 1
#define I386_NUM_FP_OPCODE_REG 1
#define I386_NUM_MXCSR_RM 1
#define I386_NUM_FP_STATUS_REG_C0 1
#define I386_NUM_FP_STATUS_REG_C1 1
#define I386_NUM_FP_STATUS_REG_C2 1
#define I386_NUM_FP_STATUS_REG_C3 1
#define I386_NUM_FP_STATUS_REG_OTHER 1

#define I386_NUM_EXREG_GROUPS 30

#define I386_INVISIBLE_REG_POPCNT_ITER 0
#define I386_INVISIBLE_REG_POPCNT_SUM 1

#define I386_NUM_INVISIBLE_REGS 2
#define I386_INVISIBLE_REGS_LEN(i) BYTE_LEN
#define I386_INVISIBLE_REG0 I386_INVISIBLE_REG_POPCNT_ITER
#define I386_INVISIBLE_REG1 I386_INVISIBLE_REG_POPCNT_SUM

#define I386_NUM_EXREGS(i) ( \
    (i == I386_EXREG_GROUP_GPRS) ? I386_NUM_GPRS: \
    (i == I386_EXREG_GROUP_MMX) ? I386_NUM_MMXREGS : \
    (i == I386_EXREG_GROUP_XMM) ? I386_NUM_XMMREGS : \
    (i == I386_EXREG_GROUP_YMM) ? I386_NUM_YMMREGS : \
    (i == I386_EXREG_GROUP_FP_DATA_REGS) ? I386_NUM_FP_DATA_REGS : \
    (i == I386_EXREG_GROUP_SEGS) ? I386_NUM_SEGS : \
    (i == I386_EXREG_GROUP_EFLAGS_OTHER)? I386_NUM_EFLAGS_OTHER : \
    (i == I386_EXREG_GROUP_EFLAGS_EQ)? I386_NUM_EFLAGS_EQ : \
    (i == I386_EXREG_GROUP_EFLAGS_NE)? I386_NUM_EFLAGS_NE : \
    (i == I386_EXREG_GROUP_EFLAGS_ULT)? I386_NUM_EFLAGS_ULT: \
    (i == I386_EXREG_GROUP_EFLAGS_SLT)? I386_NUM_EFLAGS_SLT: \
    (i == I386_EXREG_GROUP_EFLAGS_UGT)? I386_NUM_EFLAGS_UGT: \
    (i == I386_EXREG_GROUP_EFLAGS_SGT)? I386_NUM_EFLAGS_SGT: \
    (i == I386_EXREG_GROUP_EFLAGS_ULE)? I386_NUM_EFLAGS_ULE: \
    (i == I386_EXREG_GROUP_EFLAGS_SLE)? I386_NUM_EFLAGS_SLE: \
    (i == I386_EXREG_GROUP_EFLAGS_UGE)? I386_NUM_EFLAGS_UGE: \
    (i == I386_EXREG_GROUP_EFLAGS_SGE)? I386_NUM_EFLAGS_SGE: \
    (i == I386_EXREG_GROUP_ST_TOP) ? I386_NUM_ST_TOP: \
    (i == I386_EXREG_GROUP_FP_CONTROL_REG_RM)? I386_NUM_FP_CONTROL_REG_RM: \
    (i == I386_EXREG_GROUP_FP_CONTROL_REG_OTHER)? I386_NUM_FP_CONTROL_REG_OTHER: \
    (i == I386_EXREG_GROUP_FP_TAG_REG)? I386_NUM_FP_TAG_REG: \
    (i == I386_EXREG_GROUP_FP_LAST_INSTR_POINTER)? I386_NUM_FP_LAST_INSTR_POINTER: \
    (i == I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER)? I386_NUM_FP_LAST_OPERAND_POINTER: \
    (i == I386_EXREG_GROUP_FP_OPCODE_REG)? I386_NUM_FP_OPCODE_REG: \
    (i == I386_EXREG_GROUP_MXCSR_RM)? I386_NUM_MXCSR_RM: \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_C0)? I386_NUM_FP_STATUS_REG_C0: \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_C1)? I386_NUM_FP_STATUS_REG_C1: \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_C2)? I386_NUM_FP_STATUS_REG_C2: \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_C3)? I386_NUM_FP_STATUS_REG_C3: \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_OTHER)? I386_NUM_FP_STATUS_REG_OTHER: \
    0) /* should be in decreasing order */
#define I386_EXREG_LEN(i) ( \
    (i == I386_EXREG_GROUP_GPRS) ? DWORD_LEN: \
    (i == I386_EXREG_GROUP_MMX) ? QWORD_LEN : \
    (i == I386_EXREG_GROUP_XMM) ? DQWORD_LEN : \
    (i == I386_EXREG_GROUP_YMM) ? QQWORD_LEN : \
    (i == I386_EXREG_GROUP_FP_DATA_REGS) ? FPWORD_LEN : \
    (i == I386_EXREG_GROUP_SEGS) ? WORD_LEN : \
    (i == I386_EXREG_GROUP_EFLAGS_OTHER) ? DWORD_LEN : \
    (i == I386_EXREG_GROUP_EFLAGS_EQ)? 1: \
    (i == I386_EXREG_GROUP_EFLAGS_NE)? 1: \
    (i == I386_EXREG_GROUP_EFLAGS_ULT)? 1: \
    (i == I386_EXREG_GROUP_EFLAGS_SLT)? 1: \
    (i == I386_EXREG_GROUP_EFLAGS_UGT)? 1: \
    (i == I386_EXREG_GROUP_EFLAGS_SGT)? 1: \
    (i == I386_EXREG_GROUP_EFLAGS_ULE)? 1: \
    (i == I386_EXREG_GROUP_EFLAGS_SLE)? 1: \
    (i == I386_EXREG_GROUP_EFLAGS_UGE)? 1: \
    (i == I386_EXREG_GROUP_EFLAGS_SGE)? 1: \
    (i == I386_EXREG_GROUP_ST_TOP) ? WORD3_LEN: \
    (i == I386_EXREG_GROUP_FP_CONTROL_REG_RM) ? WORD2_LEN : \
    (i == I386_EXREG_GROUP_FP_CONTROL_REG_OTHER) ? WORD_LEN : \
    (i == I386_EXREG_GROUP_FP_TAG_REG) ? WORD_LEN : \
    (i == I386_EXREG_GROUP_FP_LAST_INSTR_POINTER) ? WORD48_LEN : \
    (i == I386_EXREG_GROUP_FP_LAST_OPERAND_POINTER) ? WORD48_LEN : \
    (i == I386_EXREG_GROUP_FP_OPCODE_REG) ? WORD11_LEN : \
    (i == I386_EXREG_GROUP_MXCSR_RM) ? WORD2_LEN : \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_C0) ? 1 : \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_C1) ? 1 : \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_C2) ? 1 : \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_C3) ? 1 : \
    (i == I386_EXREG_GROUP_FP_STATUS_REG_OTHER) ? WORD_LEN : \
    0)

#define I386_EXREG_MASK(i) (MAKE_MASK(I386_EXREG_LEN(i)))

#define I386_ENV_EXOFF(i, j) (my_offsetof(state_t, exregs[i][j]))

//#define I386_MEM_TO_REG_COST 10
//#define I386_REG_TO_MEM_COST 10
//#define I386_REG_TO_EXREG_COST(group) 1
//#define I386_EXREG_TO_REG_COST(group) 1
//#define I386_MEM_TO_EXREG_COST(group) ((group == I386_EXREG_GROUP_GPRS) ? 110 : (group >= I386_EXREG_GROUP_EFLAGS_OTHER && group <= I386_EXREG_GROUP_EFLAGS_SGE) ? COST_INFINITY : 120)
#define I386_MEM_TO_EXREG_COST(group) ((group == I386_EXREG_GROUP_GPRS) ? 110 : (group >= I386_EXREG_GROUP_EFLAGS_OTHER && group <= I386_EXREG_GROUP_EFLAGS_SGE) ? 300 : 120)
//#define I386_MEM_TO_EXREG_COST(group) ((group == I386_EXREG_GROUP_GPRS) ? 110 :  120)
#define I386_EXREG_TO_MEM_COST(group) (group == I386_EXREG_GROUP_GPRS)?110:120 //needs to be tallied with movop_compute_cost(), e.g., haswell_movop_compute_cost; this is slightly higher than the difference between move-to-reg and move-to-mem to encourage move-to-mem whenever possible
#define I386_EXREG_TO_EXREG_COST(group1, group2) ({ASSERT(group1 != group2); (group2 >= I386_EXREG_GROUP_EFLAGS_OTHER && group2 <= I386_EXREG_GROUP_EFLAGS_SGE) ? COST_INFINITY : 2;})
#define I386_EXREG_MODIFIER_TRANSLATION_COST(group) 2
#define I386_AVG_REG_MOVE_COST_FOR_LOOP_EDGE_AND_ZERO_DEAD_REGS (I386_EXREG_TO_MEM_COST(I386_EXREG_GROUP_GPRS)) //assumes that probability of the spill is 1/2=0.5

//char *src_regname_byte(int regno, int high_order, char *buf, int buf_size);
//char *src_regname_word(int regno, char *buf, int buf_size);
/*char *i386_regname_bytes(long regno, long nbytes, bool high_order, char *buf,
    size_t buf_size);*/
//char *i386_regname(int regno, char *buf, int buf_size);
//char *i386_exregname(int groupnum, int exregnum, char *buf, size_t size);

//extern char const *regs8[], *regs16[], *regs32[];
//extern char const *vregs8[], *vregs16[], *vregs32[], *vregs[];
//extern char const *tregs8[], *tregs16[], *tregs32[];
//extern size_t const num_regs, num_vregs, num_tregs;
////extern char const *segs[];
//extern char const *vsegs[];
//extern size_t const num_segs, num_vsegs;

#define I386_STATE_EFLAGS_CARRY 0
#define I386_STATE_EFLAGS_ADJUST 4
#define I386_STATE_EFLAGS_DIRECTION 10
#define I386_STATE_EFLAGS_OVERFLOW 11
#define I386_STATE_EFLAGS_SIGN 7
#define I386_STATE_EFLAGS_ZERO 6
#define I386_STATE_EFLAGS_PARITY 2

#endif
