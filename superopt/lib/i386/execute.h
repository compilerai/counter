#ifndef __I386_EXECUTE_H
#define __I386_EXECUTE_H
#include <stdint.h>
#include "equiv/equiv.h"
#include "i386/regs.h"
//#include "imm_map.h"

/*#if SRC == ARCH_I386
#define MEM_BASE_REG 7
#endif*/

struct transmap_t;
struct i386_insn_t;
struct regmap_t;
struct regset_t;
struct state_t;
struct imm_map_t;


/*size_t i386_iseq_gen_exec_code(struct i386_insn_t const *i386_iseq, long i386_iseq_len,
    imm_map_t const *imm_map, uint8_t *i386_code, size_t i386_codesize,
    src_ulong const *nextpcs, uint8_t const * const *next_tcodes,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    long num_nextpcs);

size_t i386_insn_gen_exec_code(struct i386_insn_t *buf, size_t size,
    struct i386_insn_t const *insn,
    struct imm_map_t const *imm_map, long insn_index);*/

long i386_iseq_sandbox(struct i386_insn_t *i386_iseq, long i386_iseq_len,
    long i386_iseq_size, int tmpreg/*, struct regset_t *ctemps*/);

long i386_insn_put_movl_mem_to_reg_var(long regno, long addr, int r_ds,
    struct i386_insn_t *insns, size_t insns_size);

long i386_insn_sandbox(struct i386_insn_t *dst, size_t dst_size,
    struct i386_insn_t const *src,
    struct regmap_t const *c2v/*, struct regset_t *ctemps*/, int tmpreg, long insn_index);
long i386_insn_put_movl_reg_var_to_mem(long addr, int regvar, int r_ds,
    struct i386_insn_t *buf, size_t size);
long i386_insn_put_add_reg_to_reg_var(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);

size_t save_callee_save_registers(uint8_t *buf, size_t size);
//size_t add_jump_handling_code(uint8_t *buf, uint8_t * const tc_start,
//    uint8_t *const tc_end, uint16_t jmp_offset0, struct transmap_t const *out_tmap);
#endif
