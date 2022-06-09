#ifndef I386_INSN_H
#define I386_INSN_H
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <cutils/clist.h>
#include <set>
#include <vector>
#include <string>
#include "i386/insntypes.h"
#include "support/types.h"
#include "support/src-defs.h"
#include "valtag/imm_vt_map.h"
#include "valtag/fixed_reg_mapping.h"
#include "valtag/transmap_loc.h"
#include "support/consts.h"

//#define I386_MAX_ILEN 2048
//#define I386_ISEQ_MAX_EXITS I386_MAX_ILEN

struct translation_instance_t;
struct input_exec_t;
struct vconstants_t;
struct induction_reg_t;
struct state_t;
struct typestate_t;
struct typestate_constraints_t;
struct i386_insn_t;
struct regset_t;
struct regcons_t;
struct assignments_t;
struct intr_frame;
struct regmap_t;
struct ppc_regset_t;
struct src_insn_t;
struct temporaries_t;
struct regcount_t;
struct transmap_t;
struct imm_vt_map_t;
struct memset_t;
struct nextpc_map_t;
struct chash;
//struct memlabel_map_t;
struct bbl_t;
class memslot_map_t;
class transmap_entry_t;
class operand_t;
class fixed_reg_mapping_t;
class src_iseq_match_renamings_t;

typedef struct i386_strict_canonicalization_state_t
{
  int dummy;
} i386_strict_canonicalization_state_t;


struct i386_strict_canonicalization_state_t *i386_strict_canonicalization_state_init();
void i386_strict_canonicalization_state_free(struct i386_strict_canonicalization_state_t *);

#ifdef __cplusplus
extern "C"
#endif
void i386_insn_init(struct i386_insn_t *insn, size_t long_len = DWORD_LEN);
void i386_insn_get_usedef(struct i386_insn_t const *insn, struct regset_t *use,
    struct regset_t *def, struct memset_t *memuse, struct memset_t *memdef);
void i386_insn_get_usedef_regs(struct i386_insn_t const *insn, struct regset_t *use,
    struct regset_t *def);
/*void i386_insn_get_usedef_vregs(struct i386_insn_t const *insn,
    struct regset_t *use, struct regset_t *def);*/
void i386_iseq_get_usedef(struct i386_insn_t const *insns, int n_insns,
    struct regset_t *use, struct regset_t *def, struct memset_t *memuse,
    struct memset_t *memdef);
void i386_iseq_get_usedef_regs(i386_insn_t const *insns, int n_insns,
    struct regset_t *use, struct regset_t *def);
void i386_iseq_get_used_vconst_regs(i386_insn_t const *iseq, long iseq_len,
    struct regset_t *use);

bool i386_insn_is_branch(struct i386_insn_t const *insn);
bool i386_insn_is_branch_not_fcall(i386_insn_t const *insn);
bool i386_insn_is_unconditional_indirect_branch_not_fcall(i386_insn_t const *insn);
bool i386_insn_accesses_mem16(struct i386_insn_t const *insn, struct operand_t const **op1,
		struct operand_t const **op2);
bool i386_insn_accesses_mem(struct i386_insn_t const *insn, struct operand_t const **op1,
		struct operand_t const **op2);
bool i386_insn_accesses_stack(i386_insn_t const *insn);
struct operand_t const *i386_insn_has_prefix(i386_insn_t const *insn);
//size_t i386_insn_get_operand_size(i386_insn_t const *insn);
//size_t i386_insn_get_addr_size(i386_insn_t const *insn);
bool i386_insn_is_direct_branch(i386_insn_t const *insn);
bool i386_insn_is_conditional_direct_branch(i386_insn_t const *insn);
bool i386_insn_is_unconditional_direct_branch(i386_insn_t const *insn);
bool i386_insn_is_unconditional_direct_branch_not_fcall(i386_insn_t const *insn);
bool i386_insn_is_unconditional_branch(i386_insn_t const *insn);
bool i386_insn_is_unconditional_branch_not_fcall(i386_insn_t const *insn);
bool i386_insn_is_lea(i386_insn_t const *insn);
//int i386_insn_compute_cost(i386_insn_t const *insn);
long i386_iseq_compute_num_nextpcs(i386_insn_t const *iseq, long n, long *nextpc_indir);

/* sim is a null terminated array with even number of elements. */
//void i386_insn_rename_immediates(i386_insn_t *insn, uint64_t const *sim);
//void i386_seq_inv_rename(i386_insn_t *iseq, int iseq_len, struct transmap_t const *tmap);
void i386_insn_rename_constants(struct i386_insn_t *insn,
    struct imm_vt_map_t const *imm_map/*,
    src_ulong const *nextpcs, long num_nextpcs,
    struct regmap_t const *regmap*/);

int i386_insn_accesses_memory_outside_region(i386_insn_t const *insn,
    uint32_t region_start, uint32_t region_stop);

int i386_insns_are_jumptable_access(i386_insn_t *i386insn, int n_insns);

int i386_insn_invert_jump_cond (uint8_t *ptr);
void i386_iseq_rename(i386_insn_t *iseq, long iseq_len,
    struct regmap_t const *dst_regmap,
    struct imm_vt_map_t const *imm_map,
    memslot_map_t const *memslot_map,
    struct regmap_t const *src_regmap,
    struct nextpc_map_t const *nextpc_map,
    nextpc_t const *nextpcs, long num_nextpcs);
void i386_iseq_rename_var(i386_insn_t *iseq, long iseq_len,
    struct regmap_t const *dst_regmap,
    struct imm_vt_map_t const *imm_vt_map,
    memslot_map_t const *memslot_map,
    struct regmap_t const *src_regmap,
    struct nextpc_map_t const *nextpc_map, nextpc_t const *nextpcs, long num_nextpcs);
bool i386_iseq_rename_regs_and_imms(i386_insn_t *iseq, long iseq_len,
    struct regmap_t const *i386_regmap, struct imm_vt_map_t const *imm_map,
    nextpc_t const *nextpcs, long num_nextpcs/*, src_ulong const *insn_pcs,
    long num_insns*/);
void i386_insn_rename_regs(i386_insn_t *insn, struct regmap_t const *regmap);
size_t i386_iseq_assemble(struct translation_instance_t const *ti,
    i386_insn_t const *insns,
    size_t n_insns, unsigned char *buf, size_t size, i386_as_mode_t mode);

typedef long long (*i386_costfn_t)(i386_insn_t const *);
long long i386_iseq_compute_execution_cost(i386_insn_t const *iseq, long iseq_len,
    i386_costfn_t costfn);

int i386_insn_touches_reg(i386_insn_t const *insn, int regno);

ssize_t x86_assemble(struct translation_instance_t *ti, uint8_t *bincode,
    size_t bincode_size, char const *assembly, i386_as_mode_t mode);
int i386_insn_assemble(i386_insn_t const *insn, unsigned char *buf, int len);
int i386_insn_assemble_sandboxed(i386_insn_t const *insn, unsigned char *buf,
    int len, int tmpreg, int memsize);


void print_i386_insn(i386_insn_t const *insn);

extern uint8_t i386_jump_insn[5];
extern int i386_jump_target_offset;

bool
i386_insn_fetch_raw_helper(i386_insn_t *insn, input_exec_t const *in, src_ulong eip,
    unsigned long *size, i386_as_mode_t i386_as_mode);
bool i386_insn_fetch_raw(struct i386_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long *size);
void i386_iseq_copy(struct i386_insn_t *dst, struct i386_insn_t const *src, long n);
void i386_insn_copy(struct i386_insn_t *dst, struct i386_insn_t const *src);
bool i386_insns_equal(struct i386_insn_t const *i1, struct i386_insn_t const *i2);
bool i386_iseqs_equal_mod_imms(struct i386_insn_t const *iseq1, long iseq1_len,
    struct i386_insn_t const *iseq2, long iseq2_len);
bool i386_iseq_types_equal(struct i386_insn_t const *iseq1, long iseq_len1,
    struct i386_insn_t const *iseq2, long iseq_len2);
void i386_insn_hash_canonicalize(struct i386_insn_t *insn, struct regmap_t *regmap,
    struct imm_vt_map_t *imm_map);
unsigned long i386_insn_hash_func(i386_insn_t const *insn);
int i386_disassemble(i386_insn_t *insn, uint8_t const *bincode);
void i386_insn_canonicalize(i386_insn_t *insn);
int i386_iseq_contains_indir_branch (i386_insn_t const *insns, int n_insns);
/*size_t snprint_insn_i386(char *buf, size_t buf_size, src_ulong nip,
    uint8_t *code, src_ulong size, int flags);*/
int i386_iseq_get_num_canon_constants(i386_insn_t const *iseq, int iseq_len);
int i386_iseq_get_num_sh_canon_constants(i386_insn_t const *iseq, int iseq_len);

bool i386_insn_is_function_return(i386_insn_t const *insn);
bool i386_insn_is_indirect_branch(i386_insn_t const *insn);
bool i386_insn_is_indirect_branch_not_fcall(i386_insn_t const *insn);
bool i386_insn_is_conditional_indirect_branch(i386_insn_t const *insn);
bool i386_insn_is_function_call(i386_insn_t const *insn);
bool i386_insn_is_direct_function_call(i386_insn_t const *insn);
bool i386_insn_is_indirect_function_call(i386_insn_t const *insn);
std::vector<nextpc_t> i386_insn_branch_targets(i386_insn_t const *insn, src_ulong nip);
nextpc_t i386_insn_fcall_target(i386_insn_t const *insn, src_ulong nip, struct input_exec_t const *in);
src_ulong i386_insn_branch_target_src_insn_pc(i386_insn_t const *insn);

long i386_insn_put_movl_nextpc_constant_to_mem(long nextpc_num, uint32_t addr,
    struct i386_insn_t *insns, size_t insns_size);
long i386_insn_put_movl_regmem_offset_to_op(struct operand_t const *op,
    int r_esp, int r_ss, src_tag_t regno_tag, int32_t offset, int opsize,
    unsigned long scratch_addr, i386_insn_t *insns,
    size_t insn_size);
long i386_insn_put_movl_op_to_regmem_offset(int r_esp, int r_ss, src_tag_t regno_tag,
    int32_t offset, struct operand_t const *op, int opsize,
    unsigned long scratch_addr, i386_insn_t *insns, size_t insn_size);

char *i386_insn_to_string(i386_insn_t const *insn, char *buf, size_t buf_size);
char *i386_iseq_to_string(i386_insn_t const *iseq, long iseq_len,
    char *buf, size_t buf_size);
char *i386_iseq_to_string_mode(i386_insn_t const *iseq, long iseq_len,
    i386_as_mode_t mode, char *buf, size_t buf_size);
char *i386_iseq_with_relocs_to_string(i386_insn_t const *iseq, long iseq_len,
    char const *reloc_strings, size_t reloc_strings_size,
    i386_as_mode_t mode, char *buf, size_t buf_size);

/*long i386_iseq_from_string_using_transmap(
    struct translation_instance_t *ti, i386_insn_t *i386_iseq,
    char const *assembly, struct transmap_t const *in_tmap);*/
/*long i386_iseq_from_string(struct translation_instance_t *ti, i386_insn_t *i386_iseq,
    char const *assembly);
long x64_iseq_from_string(struct translation_instance_t *ti, i386_insn_t *i386_iseq,
    char const *assembly);*/
vector<src_iseq_match_renamings_t> i386_iseq_matches_template(
    i386_insn_t const *_template,
    i386_insn_t const *seq, long len,
    //src_iseq_match_renamings_t &src_iseq_match_renamings,
    //struct regmap_t *st_regmap,
    //struct imm_vt_map_t *st_imm_map, struct nextpc_map_t *nextpc_map,
    string prefix);
bool i386_iseq_matches_template_var(i386_insn_t const *_template, i386_insn_t const *iseq,
    long len,
    src_iseq_match_renamings_t &src_iseq_match_renamings,
    //struct regmap_t *st_regmap, struct imm_vt_map_t *st_imm_vt_map,
    //struct nextpc_map_t *nextpc_map,
    char const *prefix);

int i386_gen_mem_modes(char const *mem_modes[], char const *mem_constraints[],
    char const *mem_live_regs[], char const *in_text);
int i386_gen_jcc_conds(char const *jcc_conds[], char const *in_text);
//bool i386_insn_is_not_harvestable(i386_insn_t const *insn, int index, int len);
bool i386_insn_get_constant_operand(i386_insn_t const *insn, uint32_t *constant);
bool i386_insn_is_nop(i386_insn_t const *insn);
//bool i386_iseq_contains_nop(i386_insn_t const *iseq, size_t iseq_len);
bool i386_insn_is_jecxz(i386_insn_t const *insn);
//bool i386_insn_is_int(i386_insn_t const *insn);
bool i386_insn_passes_stack_pointer(i386_insn_t const *insn, int *stack_reg,
    int *offset);
bool i386_insn_uses_stack_offset(i386_insn_t const *insn, int *stack_reg,
    int *offset);
int i386_insn_replace_stack_instruction(i386_insn_t *iseq, int iseq_size,
    i386_insn_t const *insn, uint32_t esp, uint32_t ebp, unsigned long function_base,
    struct clist const *regalloced_stack_offsets);
int i386_push_mem_insn(src_ulong disp, uint8_t *buf);
int i386_popn_insn(int n, uint8_t *buf);
int i386_pushn_insn(int n, uint8_t *buf);
int i386_mov_mem_insn(src_ulong disp1, src_ulong disp2, uint8_t *buf);
int i386_mov_stack_mem_insn(int offset, src_ulong disp, uint8_t *buf);
int i386_mov_mem_stack_insn(src_ulong disp, int offset, uint8_t *buf);
int i386_push_stack_insn(int offset, uint8_t *buf);
bool i386_insn_comparison_uses_induction_regs(struct i386_insn_t const *branch_insn,
    struct i386_insn_t const *cmp_insn, struct induction_reg_t const *regs, int num_regs,
    bool branch_target_is_loop_head, src_ulong *induction_reg1,
    long *induction_val1, src_ulong *induction_reg2, long *induction_val2,
    char *cmp_operator, size_t cmp_operator_size);
int i386_unroll_loop_jcc_insn(uint8_t *ptr, char const *op, uint32_t offset);
int i386_unroll_loop_jmp_insn(uint8_t *ptr, uint32_t offset);

int i386_insn_disassemble(i386_insn_t *insn, uint8_t const *bincode, i386_as_mode_t i386_as_mode);
//int i386_iseq_disassemble(i386_insn_t *i386_iseq, uint8_t *bincode, int binsize,
//    struct vconstants_t *vconstants[], src_ulong *nextpcs,
//    long *num_nextpcs, i386_as_mode_t i386_as_mode);
#ifdef __cplusplus
extern "C"
#endif
void i386_insn_init_constants(void);
void i386_insn_invert_cmp_operator(char *out, size_t outsize, char const *in);
valtag_t *i386_insn_get_pcrel_operand(i386_insn_t const *insn);
valtag_t *i386_insn_get_pcrel_operands(i386_insn_t const *insn);
operand_t *i386_insn_get_pcrel_operand_full(i386_insn_t const *insn);
size_t i386_insn_to_int_array(i386_insn_t const *insn, int64_t arr[], size_t len);
bool i386_insn_has_rep_prefix(i386_insn_t const *insn);
bool i386_infer_regcons_from_assembly(char const *assembly,
    struct regcons_t *cons);
/*bool i386_iseq_assign_vregs_to_cregs_using_transmap_and_temporaries(
    struct i386_insn_t *dst, struct i386_insn_t const *src, long len,
    struct transmap_t *tmap, struct temporaries_t *temps, bool exec);*/
bool i386_iseq_assign_vregs_to_cregs(struct i386_insn_t *dst,
    struct i386_insn_t const *src, long len, struct regmap_t *regmap,
    bool exec);
bool i386_insn_get_pcrel_value(struct i386_insn_t const *insn, uint64_t *pcrel_val,
    src_tag_t *pcrel_tag);
void i386_insn_set_pcrel_value(struct i386_insn_t *insn, uint64_t pcrel_val,
    src_tag_t pcrel_tag);

std::vector<valtag_t> i386_insn_get_pcrel_values(struct i386_insn_t const *insn);
void i386_insn_set_pcrel_values(struct i386_insn_t *insn, std::vector<valtag_t> const &vs);
void i386_insn_add_to_pcrels(i386_insn_t *insn, src_ulong pc, struct bbl_t const *bbl, long bblindex);

bool i386_insn_is_intmul(i386_insn_t const *insn);
bool i386_insn_is_intdiv(i386_insn_t const *insn);
bool i386_iseq_contains_div(i386_insn_t const *iseq, long iseq_len);
bool i386_iseq_involves_implicit_operands(i386_insn_t const *insns, int n_insns);
bool i386_iseq_can_be_optimized(i386_insn_t const *iseq, int iseq_len);

void i386_init(void);
size_t i386_setup_on_startup(struct translation_instance_t *ti, uint8_t *buf, size_t size);

/* cost functions */
/*typedef struct i386_costfn_t {
  char *name;
  int (*costfn)(i386_insn_t const *insn);
  struct list_elem l_elem;
} i386_costfn_t;

void i386_insn_set_costfn(char const *costfn_str);
char const *i386_insn_get_costfn(void);
extern struct list i386_costfns;
int i386_num_costfns(void);*/
long long i386_core2_insn_compute_cost(i386_insn_t const *insn);
long long i386_haswell_insn_compute_cost(i386_insn_t const *insn);

bool i386_insn_update_induction_reg_values(struct i386_insn_t const *insn,
    struct induction_reg_t *regs, long max_regs, long *nregs);
size_t i386_insn_put_linux_exit_syscall_as(char *buf, size_t size);
size_t i386_insn_put_nop_as(char *buf, size_t size);

/*long i386_gen_flags_to_mem_insns(int signedness, int overflow, dst_ulong addr,
    struct i386_insn_t *insns, size_t insns_size, int mode);
long i386_gen_reg_to_flags_insns(int regnum, struct i386_insn_t *insns,
    size_t insns_size, int mode);
long i386_gen_mem_to_flags_insns(dst_ulong addr, struct i386_insn_t *insns,
    size_t insns_size, int mode);
long i386_gen_flags_to_reg_insns(int signedness, int overflow, int regnum,
    struct i386_insn_t *insns, size_t insns_size, int mode);
long i386_gen_flags_to_rf_insns(int signedness, int overflow, int regnum,
    struct i386_insn_t *insns, size_t insns_size, int mode);
long i386_gen_swap_out_insns(dst_ulong addr, int regnum, struct i386_insn_t *insns);
long i386_gen_swap_in_insns(dst_ulong addr, int regnum, struct i386_insn_t *insns);
long i386_gen_xmm_to_mem_insns(int regnum, dst_ulong addr, struct i386_insn_t *insns,
    size_t insns_size);
long i386_gen_register_move_insns(int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size);*/

long i386_insn_put_function_return(fixed_reg_mapping_t const &fixed_reg_mapping, struct i386_insn_t *insns, size_t insns_size);
long i386_insn_put_function_return64(fixed_reg_mapping_t const &fixed_reg_mapping, struct i386_insn_t *insns, size_t insns_size);
long i386_insn_put_jump_to_pcrel(nextpc_t const& target, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_jump_to_abs_inum(src_ulong target, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_jcc_to_nextpc(char const *jcc, int nextpc_num, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_jcc_to_pcrel(char const *jcc, src_ulong target, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_jump_to_nextpc(i386_insn_t *insns, size_t insns_size, long nextpc_num);
long i386_insn_put_jump_to_src_insn_pc(i386_insn_t *insns, size_t insns_size, src_ulong src_insn_pc);
//void i386_insn_put_invalid(i386_insn_t *insns, size_t insns_size);
long i386_insn_put_nop(i386_insn_t *insns, size_t insns_size);
long i386_insn_put_move_operand_to_reg(long reg1, operand_t const *op,
    i386_insn_t *insns, size_t insns_size);
long i386_insn_put_xchg_reg_with_mem(int reg1, dst_ulong addr, int membase, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xchg_reg_with_reg(int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xchg_reg_with_reg_var(int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_gen_reg_to_xmm_insns(int regnum, int xmmreg, i386_insn_t *insns,
    size_t insns_size);
long i386_gen_mem_to_xmm_insns(dst_ulong addr, int xmmreg, i386_insn_t *insns,
    size_t insns_size);
long i386_gen_xmm_move_insns(int xmm1, int xmm2, i386_insn_t *insns,
    size_t insns_size);
long i386_gen_xmm_to_reg_insns(int xmmreg, int regnum, i386_insn_t *insns,
    size_t insns_size);

long i386_insn_put_not_reg(int regno, int size, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_movb_imm_to_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movw_imm_to_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movl_imm_to_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movq_imm_to_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movb_imm_to_mem(dst_ulong imm, uint32_t addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movw_imm_to_mem(dst_ulong imm, uint32_t addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movl_imm_to_mem(dst_ulong imm, uint32_t addr, int membase, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_addl_imm_to_mem(dst_ulong imm, uint32_t addr, int membase, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
long i386_insn_put_shl_reg_by_imm(int regno, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_shr_reg_by_imm(int regno, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_sar_reg_by_imm(int regno, dst_ulong imm, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movb_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movw_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movl_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movsbl_reg2b_to_reg1(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movswl_reg2w_to_reg1(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_add_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xorl_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xorw_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xorb_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xorl_reg_to_mem(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xorw_reg_to_mem(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xorb_reg_to_mem(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xorl_mem_to_reg(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xorw_mem_to_reg(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_xorb_mem_to_reg(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpl_imm_to_reg(long reg1, long imm, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpw_imm_to_reg(long reg1, long imm, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpb_imm_to_reg(long reg1, long imm, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpl_mem_to_reg(long reg1, long addr, int membase, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpw_mem_to_reg(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpb_mem_to_reg(long reg1, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpl_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpw_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpb_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_testl_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_testw_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_testb_reg_to_reg(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_setcc_mem(char const *opc, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_daa(i386_insn_t *insns, size_t insns_size);
long i386_insn_put_call_helper_function(struct translation_instance_t *ti,
    char const *helper_function, fixed_reg_mapping_t const &fixed_reg_mapping,
    i386_insn_t *insns, size_t insns_size);
long i386_insn_put_push_imm(uint32_t imm, src_tag_t tag, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_pushb_imm(uint8_t imm, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_push_reg(long regno, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_pop_reg(long regno, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_push_reg64(long regno, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_pop_reg64(long regno, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_push_mem64(uint64_t addr, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_push_mem(uint64_t addr, int membase, fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_addb_mem_to_reg(int regno, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_addw_mem_to_reg(int regno, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_addl_mem_to_reg(int regno, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movb_mem_to_reg(int regno, long addr, int membase, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movw_mem_to_reg(int regno, long addr, int membase, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movl_mem_to_reg(int regno, long addr, int membase, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movq_mem_to_reg(int regno, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_and_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_testl_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_testw_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long
i386_insn_put_testw_imm_with_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_testb_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_imul_imm_with_reg(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_add_imm_with_reg(dst_ulong imm, int regno, src_tag_t regno_tag,
    i386_insn_t *insns, size_t insns_size);
long
i386_insn_put_add_imm_with_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_or_mem_with_reg(int regno, uint32_t addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_add_mem_with_reg(int regno, uint32_t addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_store_reg1_into_memreg2_off(long reg1, long reg2, long off,
    i386_insn_t *insns, size_t insns_size);
long i386_insn_put_dereference_reg1_into_reg2(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movb_reg_to_mem(dst_ulong mem, long regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movw_reg_to_mem(dst_ulong mem, long regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movl_reg_to_mem(dst_ulong mem, int membase, long regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movq_reg_to_mem(dst_ulong mem, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_jump_to_indir_mem(long addr, i386_insn_t *insns, size_t insns_size);
bool i386_insn_terminates_bbl(i386_insn_t const *insn);
void i386_operand_copy(operand_t *dst, operand_t const *src);
long i386_insn_put_popf(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_popf64(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_pushf(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_pushf64(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size);
long i386_insn_put_pusha(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
long i386_insn_put_popa(fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
/*void i386_iseq_rename_src_pcrels_using_map(i386_insn_t *i386_iseq, long i386_iseq_len,
    long const *map, long map_size);*/
void i386_iseq_get_touch_not_live_in_regs(i386_insn_t const *i386_iseq, long i386_iseq_len,
    struct transmap_t const *tmap, struct regset_t *touch_not_live_in_regs);
/*void i386_iseq_gen_regmap_from_transmap(regmap_t *regmap, i386_insn_t const *i386_iseq,
    long i386_iseq_len, transmap_t const *tmap);*/
void i386_iseq_pick_reg_assignment(struct regmap_t *regmap,
    i386_insn_t const *i386_iseq, long i386_iseq_len);


long i386_insn_put_move_mem_to_mem(dst_ulong from_addr, dst_ulong to_addr, int from_membase, int to_membase,
    int dead_dst_reg,
    fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
long i386_insn_put_move_exreg_to_mem(dst_ulong addr, int membase, int group, int reg,
    fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
long i386_insn_put_move_mem_to_exreg(int group, int reg, dst_ulong addr, int membase,
    fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
long i386_insn_put_move_exreg_to_exreg(int group1, int reg1, int group2, int reg2,
    fixed_reg_mapping_t const &fixed_reg_mapping, i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
/*long i386_insn_put_move_reg_to_exreg(int reg1, int group2, int reg2, i386_insn_t *insns,
    size_t insns_size, int mode);
long i386_insn_put_move_exreg_to_reg(int group1, int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size, int mode);*/
long i386_insn_put_xchg_exreg_with_exreg(int group1, int reg1, int group2, int reg2,
    i386_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
bool i386_insn_less(i386_insn_t const *a, i386_insn_t const *b);
void i386_insn_update_stack_pointers(i386_insn_t const *insn, src_ulong *esp,
    src_ulong *ebp);
int i386_function_return_insn(uint8_t *buf);
int i386_add_to_induction_reg_insn(uint8_t *buf, int reg, int val, int num_unrolls);
void i386_insn_set_nextpc(i386_insn_t *insn, long nextpc_num);
void i386_free(void);
int i386_cmp_insn_for_induction_regs(uint8_t *buf, int reg1, int reg2, int val1,
    int val2);
long i386_insn_put_cmpb_reg_to_mem(long regno, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpw_reg_to_mem(long regno, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_cmpl_reg_to_mem(long regno, long addr, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_adcl_imm_with_reg(long regno, long imm, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_mul_var_with_operand(operand_t const *op, bool is_signed,
    int r_eax, int r_edx, i386_insn_t *insns, size_t insns_size);
long i386_iseq_window_convert_pcrels_to_nextpcs(i386_insn_t *i386_iseq,
    long i386_iseq_len, long start, long *nextpc2src_pcrel_vals,
    src_tag_t *nextpc2src_pcrel_tags);
bool i386_iseq_execution_test_implemented(i386_insn_t const *i386_iseq,
    long i386_iseq_len);
bool i386_insn_supports_execution_test(i386_insn_t const *insn);
bool i386_iseq_supports_execution_test(i386_insn_t const *insns, long n);
bool i386_insn_supports_boolean_test(i386_insn_t const *insn);
bool i386_iseq_contains_reloc_reference(i386_insn_t const *iseq, long iseq_len);
bool i386_insn_merits_optimization(i386_insn_t const *insn);
void i386_iseq_mark_used_vconstants(imm_vt_map_t *map, imm_vt_map_t *nomem_map,
    imm_vt_map_t *mem_map, i386_insn_t const *iseq, long iseq_len);
/*long i386_iseq_make_position_independent(i386_insn_t *iseq, long iseq_len,
    long iseq_size, uint32_t scratch_addr);*/
void i386_iseq_rename_nextpc_consts(i386_insn_t *iseq, long iseq_len,
    uint8_t const * const next_tcodes[], long num_nextpcs);
size_t
operand2str(operand_t const *op, char const *reloc_strings, size_t reloc_strings_size,
    i386_as_mode_t mode, char *buf, size_t size);
void i386_insn_collect_typestate_constraints(struct typestate_constraints_t *tscons,
    i386_insn_t const *insn, struct state_t *state, struct typestate_t *varstate);
int i386_max_operand_size(struct operand_t const *op, opc_t opct, int op_index);
void i386_insn_get_min_max_vconst_masks(int *min_mask, int *max_mask,
    i386_insn_t const *src_insn);
void i386_insn_get_min_max_mem_vconst_masks(int *min_mask, int *max_mask,
    i386_insn_t const *src_insn);
bool i386_insn_type_signature_supported(i386_insn_t const *insn);
int 
i386_insn_num_implicit_operands(i386_insn_t const *insn);
//void gas_init(void);

bool i386_insn_get_indir_branch_operand(struct operand_t *op,
    struct i386_insn_t const *insn);
long i386_indir_branch_redir(struct i386_insn_t *insns, size_t insns_size,
    struct operand_t const *indir_op, struct i386_insn_t const *indir_insn,
    struct transmap_t const *indir_out_tmap,
    struct transmap_t const *indir_tmap, struct regset_t const *indir_live_out,
    state const *start_state, int memloc_offset_reg,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping, i386_as_mode_t mode);
long i386_operand_strict_canonicalize_regs(i386_strict_canonicalization_state_t *scanon_state,
    long insn_index, long op_index,
    struct i386_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map, fixed_reg_mapping_t const &fixed_reg_mappng,
    long num_iseq_var, long max_num_iseq_var/*,
    bool canonicalize_imms_using_syms, struct input_exec_t *in*/);
long i386_memtouch_strict_canonicalize(long insn_index, long op_index,
    struct i386_insn_t *iseq_var[], long iseq_var_len[]/*, struct memlabel_map_t *st_memlabel_map*/,
    long num_iseq_var, long max_num_iseq_var);
long i386_operand_strict_canonicalize_imms(i386_strict_canonicalization_state_t *scanon_state,
    long insn_index,
    long op_index,
    struct i386_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    long num_iseq_var, long max_num_iseq_var/*,
    bool canonicalize_imms_using_syms, struct input_exec_t *in*/);
void i386_insn_inv_rename_registers(struct i386_insn_t *insn,
    struct regmap_t const *regmap);
bool i386_insn_inv_rename_memory_operands(struct i386_insn_t *insn,
    struct regmap_t const *regmap, struct vconstants_t const *vconstants);
void i386_insn_inv_rename_constants(struct i386_insn_t *insn,
    struct vconstants_t const *vconstants);

char *i386_exregname_suffix(int groupnum, int exregnum, char const *suffix, char *buf,
    size_t size);
void i386_insn_canonicalize_locals_symbols(struct i386_insn_t *i386_insn,
    struct imm_vt_map_t *imm_vt_map, src_tag_t tag);
bool i386_insn_inv_rename_locals_symbols(struct i386_insn_t *i386_insn,
    struct imm_vt_map_t *imm_vt_map, src_tag_t tag);
void i386_insn_rename_nextpcs(struct i386_insn_t *insn,
    struct nextpc_map_t const *nextpc_map);
bool i386_insn_symbols_are_contained_in_map(struct i386_insn_t const *insn, struct imm_vt_map_t const *imm_vt_map);
bool symbol_is_contained_in_i386_insn(struct i386_insn_t const *insn, long symval,
    src_tag_t symtag);
void i386_insn_rename_addresses_to_symbols(struct i386_insn_t *insn,
    struct input_exec_t const *in, src_ulong pc/*long const *allowed_symbol_ids, ssize_t num_allowed_symbol_ids*/);
void i386_insn_rename_symbols_to_addresses(struct i386_insn_t *insn,
    struct input_exec_t const *in, struct chash const *tcodes);
void i386_insn_copy_symbols(struct i386_insn_t *dst, struct i386_insn_t const *src);
//void i386_insn_annotate_local(struct i386_insn_t *insn, int op_index, memlabel_t const *memlabel, src_tag_t regtype, int rt_index, long offset);
size_t i386_insn_get_symbols(struct i386_insn_t const *insn, long const *start_symbol_ids, long *symbol_ids, size_t max_symbol_ids);
void i386_insn_rename_immediate_operands_containing_code_addresses(i386_insn_t *i386_insn,
    struct chash const *tcodes);
void i386_insn_fetch_preprocess(i386_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long size);
void i386_insn_gen_sym_exec_reader_file(i386_insn_t const *insn, FILE *ofp);
bool i386_imm_operand_needs_canonicalization(int opc, int explicit_op_index);
long i386_insn_get_max_num_imm_operands(i386_insn_t const *insn);

regset_t const *i386_regset_live_at_jumptable();
regset_t const *x64_regset_live_at_jumptable();

extern regset_t i386_reserved_regs;
extern regset_t i386_callee_save_regs;

#define MAX_I386_COSTFNS 32
extern i386_costfn_t i386_costfns[MAX_I386_COSTFNS];
extern long i386_num_costfns;

void i386_pc_get_function_name_and_insn_num(input_exec_t const *in, src_ulong pc, std::string &function_name, long &insn_num_in_function);
void i386_iseq_inv_rename_imms_using_imm_vt_map(i386_insn_t *iseq, size_t iseq_len, imm_vt_map_t const *imm_vt_map);

void i386_iseq_rename_nextpc_vars_to_pcrel_relocs(struct translation_instance_t *ti, i386_insn_t *iseq, size_t iseq_len, std::map<nextpc_id_t, std::string> const &nextpc_map);

void i386_iseq_rename_locals_using_locals_map(struct i386_insn_t *i386_iseq, size_t i386_iseq_len, graph_locals_map_t const &locals_map, size_t stack_alloc_size);

bool i386_iseq_replace_tmap_loc_with_const(i386_insn_t *iseq, size_t iseq_len, transmap_loc_t const &loc/*uint8_t tmap_loc, dst_ulong tmap_regnum*/, long constant);
size_t i386_iseq_rename_randomly_and_assemble(i386_insn_t const *iseq, size_t iseq_len, size_t num_nextpcs, uint8_t *buf, size_t buf_size, i386_as_mode_t mode);

size_t i386_insn_compute_transmap_entry_loc_value(int dst_reg, transmap_entry_t const &tmap_entry, int esp_reg, long esp_offset, int locnum, fixed_reg_mapping_t const &dst_fixed_reg_mapping, i386_insn_t *buf, size_t buf_size);
//eq::tfg *i386_iseq_get_tfg(i386_insn_t const *iseq, long iseq_len);
size_t i386_insn_put_zextend_exreg(exreg_group_id_t group, exreg_id_t regnum, int cur_size, i386_insn_t *buf, size_t buf_size);
size_t i386_insn_put_function_return(i386_insn_t *insn);
size_t i386_insn_put_ret_insn(i386_insn_t *insns, size_t insns_size, int retval);
size_t i386_emit_translation_marker_insns(i386_insn_t *buf, size_t buf_size);
bool i386_iseq_has_marker_prefix(i386_insn_t const *insns, size_t n_insns);
bool x64_insn_get_usedef_regs(i386_insn_t const *insn, regset_t *use, regset_t *def);
void i386_insn_rename_cregs_to_vregs(i386_insn_t *insn);
void i386_insn_rename_vregs_to_cregs(i386_insn_t *insn);
long i386_insn_put_opcode_operand(char const *opc, operand_t const *op,
    i386_insn_t *insns, size_t insns_size);
list<pair<transmap_t, vector<transmap_t>>> i386_enumerate_in_out_transmaps_for_usedef(regset_t const &use, regset_t const &def, regset_t const *live_out, long num_live_out);
void i386_iseq_convert_peeptab_mem_constants_to_symbols(i386_insn_t *iseq, long iseq_len);
void i386_insn_rename_vregs_using_regmap(i386_insn_t *iseq, regmap_t const &regmap);
void i386_insn_add_implicit_operands(i386_insn_t *dst, fixed_reg_mapping_t fixed_reg_mapping = fixed_reg_mapping_t());
void i386_insn_add_implicit_operands_var(i386_insn_t *dst, fixed_reg_mapping_t const &fixed_reg_mapping);
size_t i386_insn_count_num_implicit_flag_regs(i386_insn_t const *insn);
uint64_t i386_eflags_construct_from_cmp_bits(bool eq, bool ult, bool slt);
bool i386_opcodes_related(i386_insn_t const &a, i386_insn_t const *b_iseq, long b_iseq_len);
void i386_insn_rename_memory_operand_to_symbol(i386_insn_t &insn, symbol_id_t symbol_id);
long i386_iseq_count_touched_regs_as_cost(i386_insn_t const *iseq, long iseq_len);
void i386_insn_rename_tag_src_insn_pcs_to_tag_vars(i386_insn_t *insn, nextpc_t const *nextpcs, uint8_t const * const *varvals, size_t num_varvals);
void i386_insn_rename_tag_src_insn_pcs_to_tag_var_using_tcodes(i386_insn_t *insn, struct chash const *tcodes);
void i386_insn_rename_tag_src_insn_pcs_to_dst_pcrels_using_tinsns_map(i386_insn_t *insn, map<src_ulong, i386_insn_t const *> const &tinsns_map);
bool i386_insn_is_move_insn(i386_insn_t const *insn, exreg_group_id_t &g, exreg_id_t &from, exreg_id_t &to);
bool i386_insn_rename_defed_reg(i386_insn_t *insn, exreg_group_id_t g, reg_identifier_t const &from, reg_identifier_t const &to);
bool i386_iseq_is_well_formed(i386_insn_t const *iseq, size_t iseq_len);
bool i386_insn_reg_is_replaceable(i386_insn_t const *insn, exreg_group_id_t group, exreg_id_t reg);
bool i386_insn_reg_def_is_replaceable(i386_insn_t const *insn, exreg_group_id_t group, exreg_id_t reg);
void i386_insn_rename_dst_reg(i386_insn_t *insn, exreg_group_id_t g, exreg_id_t from, exreg_id_t to);
void i386_insn_invert_branch_condition(i386_insn_t *insn);
bool i386_insn_deallocates_stack(i386_insn_t const *insn);
void i386_insn_convert_function_call_to_branch(i386_insn_t *insn);
bool i386_iseq_is_function_tailcall_opt_candidate(i386_insn_t const *iseq, long iseq_len);
void i386_insn_convert_pcrels_and_symbols_to_reloc_strings(i386_insn_t *src_insn, std::map<long, std::string> const& insn_labels, nextpc_t const* nextpcs, long num_nextpcs, input_exec_t const* in, src_ulong const* insn_pcs, int insn_index, std::set<symbol_id_t>& syms_used, char **reloc_strings, long *reloc_strings_size);
void i386_insn_convert_malloc_to_mymalloc(i386_insn_t* insn, char** reloc_strings, long* reloc_strings_size);

long
i386_insn_put_imul_imm_with_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_and_imm_with_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long i386_insn_put_movl_imm_to_reg_var(dst_ulong imm, int regno, i386_insn_t *insns,
    size_t insns_size);
long
i386_insn_put_testw_reg_to_reg_var(long reg1, long reg2, i386_insn_t *insns,
    size_t insns_size);
long
i386_insn_put_cmpw_imm_to_reg_var(long regno, long imm, i386_insn_t *insns,
    size_t insns_size);
long
i386_insn_put_cmpl_imm_to_reg_var(long regno, long imm, i386_insn_t *insns,
    size_t insns_size);
size_t
i386_insn_put_not_reg_var(int regno, int size, i386_insn_t *insns, size_t insns_size);
long
i386_insn_put_movl_reg_var_to_reg_var(int dst_reg, int src_reg, i386_insn_t *insns,
    size_t insns_size);
size_t
i386_insn_put_add_regvar_to_regvar(long dst_reg, long src_reg, i386_insn_t *insns,
    size_t insns_size);
size_t
i386_insn_put_movsbl_reg2b_var_to_reg1_var(int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size);
size_t
i386_insn_put_movswl_reg2w_var_to_reg1_var(int reg1, int reg2, i386_insn_t *insns,
    size_t insns_size);
size_t
i386_insn_put_shl_reg_var_by_imm(int reg, int imm, i386_insn_t *insns, size_t insns_size);
size_t
i386_insn_put_shr_reg_var_by_imm(int reg, int imm, i386_insn_t *insns, size_t insns_size);
size_t
i386_insn_put_sar_reg_var_by_imm(int reg, int imm, i386_insn_t *insns, size_t insns_size);
long
i386_insn_put_dereference_reg2_var_offset_into_reg1w_var(int reg1, int reg2, int offset,
    i386_insn_t *insns, size_t insns_size);
size_t
i386_insn_put_opcode_regmem_offset_to_reg_var(char const *opcode, int reg_var,
    int regmem,
    long offset, int size, i386_insn_t *buf, size_t buf_size);
size_t
i386_insn_put_opcode_reg_var_to_regmem_offset(char const *opcode,
    int regmem,
    long offset, int reg_var, int size, i386_insn_t *buf, size_t buf_size);
size_t
i386_insn_put_opcode_imm_to_regmem_offset(char const *opcode,
    int regmem,
    long offset, int imm, int size, i386_insn_t *buf, size_t buf_size);
size_t
i386_insn_put_movl_regmem_offset_to_reg_var(int reg_var, int regmem, long offset,
    i386_insn_t *buf, size_t buf_size);
size_t
i386_insn_put_addl_tag_to_reg_var(long entrysize, int tag_num, src_tag_t tag,
    int reg, int const *temps, int num_temps, i386_insn_t *buf, size_t buf_size);
size_t
i386_insn_put_addl_vconst_to_reg_var(long entrysize, int vconst_num,
    int reg, int const *temps, int num_temps, i386_insn_t *buf, size_t buf_size);
size_t
i386_insn_put_addl_symbol_to_reg_var(long entrysize, int symbol_num,
    int reg, int const *temps, int num_temps, i386_insn_t *buf, size_t buf_size);
size_t
i386_insn_put_opcode_scaled_vconst_gpr_mem_to_reg_var(char const *opcode,
    int dst_reg, uint32_t start,
    int scale, int gpr, i386_insn_t *buf, size_t size);
size_t
i386_insn_put_addl_scaled_vconst_gpr_mem_to_reg_var(int dst_reg, uint32_t start,
    int scale, int gpr, i386_insn_t *buf, size_t size);
size_t
i386_insn_put_movl_scaled_vconst_gpr_mem_to_reg_var(int dst_reg, uint32_t start,
    int scale, int gpr, i386_insn_t *buf, size_t size);
size_t
i386_insn_put_opcode_scaled_vconst_exvreg_to_reg_var(char const *opcode,
    int scale, int group, int reg,
    int dst_reg, i386_insn_t *buf, size_t size);
size_t
i386_insn_put_addl_scaled_vconst_exvreg_to_reg_var(int scale, int group, int reg,
    int dst_reg, i386_insn_t *buf, size_t size);
size_t
i386_insn_put_movl_scaled_vconst_exvreg_to_reg_var(int scale, int group, int reg,
    int dst_reg, i386_insn_t *buf, size_t size);

size_t
i386_assemble_exec_iseq_using_regmap(uint8_t *buf, size_t buf_size,
    i386_insn_t const *exec_iseq, long exec_iseq_len, regmap_t const *dst_regmap,
    regmap_t const *src_regmap/*, int d*/, memslot_map_t const &memslot_map);
char *i386_regname_bytes(long regnum, long nbytes, bool high_order, char *buf, size_t buf_size);
char *i386_regname_word(int regnum, char *buf, int buf_size);
char *i386_regname(int regnum, char *buf, int buf_size);
char *x64_regname(int regnum, char *buf, int buf_size);
void i386_init_costfns(void);
void i386_free_costfns(void);

#endif /* i386/insn.h */
