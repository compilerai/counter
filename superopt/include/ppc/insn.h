#pragma once

#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <set>
#include <vector>
#include "support/types.h"
#include "support/src_tag.h"
#include "valtag/valtag.h"
#include "valtag/memvt.h"
//#include "rewrite/memvt_list.h"

//#define PPC_MAX_ILEN 2048
//#define PPC_INSN_MAX_VCONSTS 3

#define PPC_MAX_NUM_OPERANDS 1 //all the insn operands are considered as one operand, so the generic code can work at operand granularity, while the ppc insn does not have numbered operands.

#include "ppc/regs.h"
#include "valtag/imm_vt_map.h"
#include "valtag/memset.h"
#include "valtag/regmap.h"

struct translation_instance_t;
struct regmap_t;
struct regcons_t;
struct imm_vt_map_t;
struct nextpc_map_t;
struct transmap_t;
struct regset_t;
struct memset_t;
struct input_exec_t;
struct state_t;
struct typestate_t;
struct typestate_constraints_t;
struct vconstants_t;

typedef struct ppc_strict_canonicalization_state_t
{
  int dummy;
} ppc_strict_canonicalization_state_t;

typedef struct ppc_insn_valtag_t {
  union {
    memvt_t rS;
    memvt_t rD;
  } rSD;
  valtag_t rA;
  valtag_t rB;

  /* implicit regs. */
  valtag_t crf0;                     /* implicit crf0 */
  valtag_t i_crf0_so;                /* implicit crf0_so */
  valtag_t i_crfS_so;                /* implicit reg for SO bit of crfS */
  valtag_t i_crfD_so;                /* implicit reg for SO bit of crfS */
  valtag_t i_crfs[PPC_NUM_CRFREGS];  /* implicit crfs for mfcr / mtcrf */
  int num_i_crfs;
  valtag_t i_crfs_so[PPC_NUM_CRFREGS];  /* implicit crfs_so for mfcr / mtcrf */
  int num_i_crfs_so;
  valtag_t i_sc_regs[PPC_NUM_SC_IMPLICIT_REGS];
  int num_i_sc_regs;

  valtag_t lr0;
  valtag_t ctr0;
  valtag_t xer_ov0;           /* implicit xer_ov0 */
  valtag_t xer_so0;           /* implicit xer_so0 */
  valtag_t xer_ca0;           /* implicit xer_ca0 */
  valtag_t xer_bc_cmp0;

  /* A value of -1 for imm_signed indicates no immediate operand */
  int imm_signed:2;		/* 1 + 1 */

  union {
    valtag_t fpimm;
    valtag_t simm;
    valtag_t uimm;
  } imm;

  valtag_t BD;
  valtag_t LI;

  valtag_t crfS;
  valtag_t crfD;

  valtag_t crbA;
  valtag_t crbB;
  valtag_t crbD;

  valtag_t BI;

  valtag_t NB;
  valtag_t SH;
  valtag_t MB;
  valtag_t ME;

  int CRM:9;

  union {
    memvt_t frS;
    memvt_t frD;
  } frSD;
} ppc_insn_valtag_t;

typedef struct ppc_insn_t {
  int opc1:7;			/* 6 + 1 */
  int opc2:6;			/* 5 + 1 */
  int opc3:6;			/* 5 + 1 */

  int FM:9;			  /* 8 + 1 */
  int Rc:2;		    /* 1 + 1 */

  int LK:2;		/* 1 + 1 */

  int BO:6;		/* 5  + 1 */

  int SPR:11;		/* 10 + 1 */

  int SR:5;		/* 4 + 1 */

  int AA:2;		/* 1 + 1 */

  ppc_insn_valtag_t valtag;

  uint32_t reserved_mask;

  //memvt_list_t memtouch;
  //int is_fp_insn:1;
  //int invalid:1;
} ppc_insn_t;

void ppc_insn_init(ppc_insn_t *insn);
int ppc_insn_assemble(ppc_insn_t const *insn, uint8_t *buf, size_t size);
int ppc_iseq_assemble(struct translation_instance_t const *ti, ppc_insn_t const *insns,
    size_t n_insns, unsigned char *buf, size_t size, i386_as_mode_t mode);
int ppc_insn_disassemble(ppc_insn_t *insn, uint8_t const *bincode, i386_as_mode_t i386_as_mode);
void print_ppc_insn(ppc_insn_t const *insn);
char *ppc_insn_to_string(ppc_insn_t const *insn, char *buf, size_t buf_size);
char *ppc_iseq_to_string(ppc_insn_t const *iseq, long iseq_len,
    char *buf, size_t buf_size);
char *ppc_iseq_contents_to_string(ppc_insn_t const *iseq, int iseq_len,
    char *buf, int buf_size);
void ppc_insn_copy(ppc_insn_t *dst, ppc_insn_t const *src);
bool ppc_insn_equals(ppc_insn_t const *i1, ppc_insn_t const *i2);

void ppc_insn_get_bincode(ppc_insn_t const *insn, uint8_t *bincode);
void ppc_insn_get_usedef(ppc_insn_t const *insn, struct regset_t *use,
    struct regset_t *def, struct memset_t *memuse,
    struct memset_t *memdef);
void ppc_insn_get_usedef_regs(ppc_insn_t const *insn, struct regset_t *use,
    struct regset_t *def);

void ppc_iseq_get_usedef(ppc_insn_t const *insns, int n_insns,
    struct regset_t *use, struct regset_t *def,
    struct memset_t *memuse, struct memset_t *memdef);
void ppc_iseq_get_usedef_regs(ppc_insn_t const *insns, int n_insns,
    struct regset_t *use, struct regset_t *def);

bool ppc_insn_is_conditional_branch(ppc_insn_t const *insn);
bool ppc_insn_is_unconditional_branch(ppc_insn_t const *insn);
bool ppc_insn_is_unconditional_branch_not_fcall(ppc_insn_t const *insn);
bool ppc_insn_is_unconditional_indirect_branch_not_fcall(ppc_insn_t const *insn);
bool ppc_insn_is_unconditional_direct_branch(ppc_insn_t const *insn);
bool ppc_insn_is_unconditional_direct_branch_not_fcall(ppc_insn_t const *insn);
bool ppc_insn_is_conditional_direct_branch(ppc_insn_t const *insn);
bool ppc_insn_is_conditional_indirect_branch(ppc_insn_t const *insn);
bool ppc_insn_is_indirect_branch_not_fcall(ppc_insn_t const *insn);
bool ppc_insn_is_function_call(ppc_insn_t const *insn);
bool ppc_insn_is_indirect_function_call(ppc_insn_t const *insn);
bool ppc_insn_is_direct_function_call(ppc_insn_t const *insn);
bool ppc_insn_is_direct_branch(ppc_insn_t const *insn);
std::vector<src_ulong> ppc_insn_branch_targets(ppc_insn_t const *insn, src_ulong nip);
src_ulong ppc_insn_fcall_target(ppc_insn_t const *insn, src_ulong nip, struct input_exec_t const *in);
src_ulong ppc_insn_branch_target_var(ppc_insn_t const *insn);
bool ppc_insn_is_nop(ppc_insn_t const *insn);

bool ppc_insn_is_function_return(ppc_insn_t const *insn);
bool ppc_insn_is_crf_logical_op(ppc_insn_t const *insn);

long ppc_iseq_canonicalize(ppc_insn_t *iseq, long iseq_len);
bool ppc_insn_get_constant_operand(ppc_insn_t const *insn, uint32_t *constant);

unsigned long ppc_iseq_hash_func(ppc_insn_t const *iseq, int iseq_len);

bool ppc_iseq_matches_template(ppc_insn_t const *_template,
    ppc_insn_t const *seq, long len, struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map);

bool ppc_iseq_matches_template_var(ppc_insn_t const *_template,
    ppc_insn_t const *seq, long len, struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_vt_map, struct nextpc_map_t *nextpc_map);

int ppc_iseq_get_num_canon_constants(ppc_insn_t const *iseq, int iseq_len);
long ppc_insn_put_nop(ppc_insn_t *insns, size_t insns_size);
long ppc_insn_put_xor(int regA, int regS, int regB, ppc_insn_t *insns, size_t insns_size);
long ppc_insn_put_dcbi(int reg1, int reg2, ppc_insn_t *insns, size_t insns_size);
//long ppc_insn_put_add_regs(int rd, int ra, int rb, ppc_insn_t *insns, size_t insns_size);
//long ppc_insn_put_subf_regs(int rd, int ra, int rb, ppc_insn_t *insns, size_t insns_size);
long ppc_insn_put_jump_to_pcrel(src_ulong target, ppc_insn_t *insns, size_t insns_size);
long ppc_insn_put_jump_to_nextpc(ppc_insn_t *insns, size_t insns_size, long nextpc_num);
long ppc_insn_put_jump_to_indir_mem(long addr, ppc_insn_t *insns, size_t insns_size);
void ppc_insn_set_pcrel_value(ppc_insn_t *insn, uint64_t pcrel_val,
    src_tag_t pcrel_tag);
bool ppc_insn_get_pcrel_value(ppc_insn_t const *insn, uint64_t *pcrel_val,
    src_tag_t *pcrel_tag);

void ppc_insn_set_pcrel_values(ppc_insn_t *insn, std::vector<valtag_t> const &vs);
std::vector<valtag_t> ppc_insn_get_pcrel_values(ppc_insn_t const *insn);

valtag_t *ppc_insn_get_pcrel_operands(ppc_insn_t const *insn);
bool ppc_insn_is_not_harvestable(ppc_insn_t const *insn, int index, int len);

bool ppc_insn_is_branch(ppc_insn_t const *insn);
bool ppc_insn_is_indirect_branch(ppc_insn_t const *insn);
bool ppc_insn_is_mfcr(ppc_insn_t const *insn);
bool ppc_insn_is_mtcrf(ppc_insn_t const *insn);
bool ppc_insn_fetch_raw(ppc_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long *size);
long ppc_iseq_from_string(struct translation_instance_t *ti, ppc_insn_t *iseq,
    char const *assembly);

long ppc_iseq_strict_canonicalize(ppc_insn_t const *iseq, long iseq_len, 
    ppc_insn_t *iseq_var[], long *iseq_var_len, struct regmap_t *st_regmap,
    struct imm_vt_map_t *imm_map, long max_num_iseq_var);
size_t ppc_insn_to_int_array(ppc_insn_t const *insn, int64_t arr[], size_t len);
void ppc_insn_hash_canonicalize(ppc_insn_t *insn, struct regmap_t *regmap,
    struct imm_vt_map_t *imm_map);
bool ppc_insn_rename_constants(ppc_insn_t *insn, struct imm_vt_map_t const *imm_map,
    nextpc_t const *nextpcs, long num_nextpcs, /*src_ulong const *insn_pcs,
    long num_insns, */struct regmap_t const *regmap);
bool ppc_iseq_can_be_optimized(ppc_insn_t const *iseq, int iseq_len);
size_t ppc_insn_put_nop_as(char *buf, size_t size);
size_t ppc_insn_put_linux_exit_syscall_as(char *buf, size_t size);
bool ppc_iseq_rename_regs_and_imms(struct ppc_insn_t *iseq, long iseq_len,
    struct regmap_t const *regmap, struct imm_vt_map_t const *imm_map,
    nextpc_t const *nextpcs, long num_nextpcs/*, src_ulong const *insn_pcs,
    long num_insns*/);
void ppc_insn_disas_init(void);
bool ppc_insn_terminates_bbl(ppc_insn_t const *insn);
bool ppc_insn_is_branch_not_fcall(ppc_insn_t const *insn);
bool ppc_insn_bo_indicates_ctr_use(ppc_insn_t const *insn);
bool ppc_insn_less(ppc_insn_t const *a, ppc_insn_t const *b);
void ppc_iseq_copy(ppc_insn_t *dst, ppc_insn_t const *src, long len);
void ppc_iseq_assign_vregs_to_cregs(ppc_insn_t *dst, ppc_insn_t const *src, long src_len,
    regmap_t *regmap, bool exec);
unsigned long ppc_insn_hash_func(ppc_insn_t const *insn);
bool ppc_insn_is_unconditional_indirect_branch(ppc_insn_t const *insn);
void ppc_insn_add_to_pcrels(ppc_insn_t *insn, src_ulong pc, struct bbl_t const *bbl, long bblindex);
void ppc_init(void);
void ppc_free(void);
long ppc_iseq_window_convert_pcrels_to_nextpcs(ppc_insn_t *ppc_iseq,
    long ppc_iseq_len, long start, long *nextpc2src_pcrel_vals,
    src_tag_t *nextpc2src_pcrel_tags);
int get_ppc_usedef(uint8_t *bincode, struct regset_t *use,
    struct regset_t *def, struct memset_t *memuse, struct memset_t *memdef);
void
ppc_iseq_rename(struct ppc_insn_t *iseq, long iseq_len,
    struct regmap_t const *dst_regmap,
    struct imm_vt_map_t const *imm_map, struct regmap_t const *src_regmap,
    nextpc_t const *nextpcs, long num_nextpcs);
bool ppc_insn_supports_execution_test(ppc_insn_t const *insn);
bool ppc_insn_supports_boolean_test(ppc_insn_t const *insn);
bool ppc_iseq_supports_execution_test(ppc_insn_t const *insns, long n);
bool ppc_infer_regcons_from_assembly(char const *assembly, struct regcons_t *regcons);
bool ppc_insn_merits_optimization(ppc_insn_t const *insn);
void ppc_iseq_mark_used_vconstants(struct imm_vt_map_t *map, struct imm_vt_map_t *nomem_map,
    struct imm_vt_map_t *mem_map, ppc_insn_t const *iseq, long iseq_len);
long ppc_iseq_compute_num_nextpcs(ppc_insn_t const *iseq, long iseq_len,
    long *nextpc_indir);
void ppc_insn_collect_typestate_constraints(struct typestate_constraints_t *tscons,
    ppc_insn_t const *insn, struct state_t *state, struct typestate_t *varstate);
void ppc_insn_get_min_max_vconst_masks(int *min_mask, int *max_mask,
    ppc_insn_t const *src_insn);
void ppc_insn_get_min_max_mem_vconst_masks(int *min_mask, int *max_mask,
    ppc_insn_t const *src_insn);
bool ppc_insn_type_signature_supported(ppc_insn_t const *insn);

long ppc_iseq_connect_end_points(ppc_insn_t *iseq, long len, size_t iseq_size,
    nextpc_t const *nextpcs,
    uint8_t const * const *next_tcodes, struct transmap_t const *out_tmaps,
    struct transmap_t const * const *next_tmaps,
    struct regset_t const *live_outs, long num_nextpcs, bool fresh, i386_as_mode_t mode);

long ppc_insn_put_movl_nextpc_constant_to_mem(long nextpc_num, uint32_t addr,
    struct ppc_insn_t *insns, size_t insns_size);
long ppc_insn_put_move_exreg_to_mem(dst_ulong addr, int group, int reg,
    ppc_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
long ppc_insn_put_move_mem_to_exreg(int group, int reg, dst_ulong addr,
    ppc_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
long ppc_insn_put_move_exreg_to_exreg(int group1, int reg1, int group2, int reg2,
    ppc_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
long ppc_insn_put_xchg_exreg_with_exreg(int group1, int reg1, int group2, int reg2,
    ppc_insn_t *insns, size_t insns_size, i386_as_mode_t mode);
long ppcx_assemble(struct translation_instance_t *ti, uint8_t *bincode, char const *assembly,
    i386_as_mode_t mode);
void ppc_iseq_count_temporaries(ppc_insn_t const *ppc_iseq, long ppc_iseq_len,
    struct transmap_t const *tmap, struct regset_t *touch_not_live_in_regs);
char *
ppc_iseq_with_relocs_to_string(ppc_insn_t const *iseq, long iseq_len,
    char const *reloc_strings, size_t reloc_strings_size, i386_as_mode_t mode,
    char *buf, size_t buf_size);
void
ppc_iseq_rename_var(ppc_insn_t *iseq, long iseq_len, regmap_t const *dst_regmap,
    imm_vt_map_t const *imm_vt_map, regmap_t const *src_regmap,
    struct nextpc_map_t const *nextpc_map, nextpc_t const *nextpcs, long num_nextpcs);



typedef long long (*ppc_costfn_t)(ppc_insn_t const *);

#define MAX_PPC_COSTFNS 32
extern ppc_costfn_t ppc_costfns[MAX_PPC_COSTFNS];
extern long ppc_num_costfns;

long long ppc_iseq_compute_cost(ppc_insn_t const *iseq, long iseq_len, ppc_costfn_t costfn);
bool ppc_iseq_contains_reloc_reference(ppc_insn_t const *iseq, long iseq_len);
bool ppc_insn_is_lea(ppc_insn_t const *insn);
long ppc_iseq_sandbox(ppc_insn_t *ppc_iseq, long ppc_iseq_len, long ppc_iseq_size,
    int tmpreg, struct regset_t *ctemps);
/*long ppc_iseq_make_position_independent(ppc_insn_t *iseq, long iseq_len,
    long iseq_size, uint32_t scratch_addr);*/
void ppc_iseq_rename_nextpc_consts(ppc_insn_t *iseq, long iseq_len,
    uint8_t const * const next_tcodes[], long num_nextpcs);
void ppc_iseq_get_used_vconst_regs(ppc_insn_t const *iseq, long iseq_len, regset_t *use);
long ppc_iseq_connect_end_points(ppc_insn_t *iseq, long len, size_t iseq_size,
    nextpc_t const *nextpcs,
    uint8_t const * const *next_tcodes, struct transmap_t const *out_tmaps,
    struct transmap_t const * const *next_tmaps, struct regset_t const *live_outs,
    long num_nextpcs, bool fresh, i386_as_mode_t mode);
void ppc_insn_canonicalize(ppc_insn_t *insn);
bool ppc_insns_equal(ppc_insn_t const *i1, ppc_insn_t const *i2);
void ppc_iseq_rename_vconsts(ppc_insn_t *iseq, long iseq_len,
    struct imm_vt_map_t const *imm_vt_map,
    nextpc_t const *nextpcs, long num_nextpcs, regmap_t const *regmap);
bool ppc_insn_is_intdiv(ppc_insn_t const *insn);
int ppc_iseq_disassemble(struct ppc_insn_t *ppc_iseq, uint8_t *bincode, int binsize,
    struct vconstants_t *vconstants[], nextpc_t *nextpcs,
    long *num_nextpcs, i386_as_mode_t i386_as_mode);
bool ppc_iseq_types_equal(struct ppc_insn_t const *iseq1, long iseq_len1,
    struct ppc_insn_t const *iseq2, long iseq_len2);
long ppc_operand_strict_canonicalize_regs(ppc_strict_canonicalization_state_t *scanon_state,
    long insn_index, long op_index,
    struct ppc_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map, long num_iseq_var, long max_num_iseq_var);
long ppc_operand_strict_canonicalize_imms(ppc_strict_canonicalization_state_t *scanon_state,
    long insn_index,
    long op_index,
    struct ppc_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map, long num_iseq_var, long max_num_iseq_var);

long ppc_insn_put_movl_reg_var_to_mem(long addr, int regvar, int r_ds,
    struct ppc_insn_t *buf, size_t size);
long ppc_insn_put_movl_mem_to_reg_var(long regno, long addr, int r_ds,
    struct ppc_insn_t *insns, size_t insns_size);
long ppc_insn_sandbox(struct ppc_insn_t *dst, size_t dst_size,
    struct ppc_insn_t const *src,
    struct regmap_t const *c2v, struct regset_t *ctemps, int tmpreg, long insn_index);
void ppc_insn_add_implicit_operands(struct ppc_insn_t *dst);
void ppc_insn_canonicalize_locals_symbols(struct ppc_insn_t *ppc_insn,
    struct imm_vt_map_t *imm_vt_map, src_tag_t tag);
void ppc_insn_inv_rename_constants(struct ppc_insn_t *insn,
    struct vconstants_t const *vconstants);
bool ppc_insn_inv_rename_memory_operands(struct ppc_insn_t *insn,
    struct regmap_t const *regmap, struct vconstants_t const *vconstants);
void ppc_insn_inv_rename_registers(struct ppc_insn_t *insn,
    struct regmap_t const *regmap);
ssize_t ppc_assemble(struct translation_instance_t *ti, uint8_t *bincode,
    size_t bincode_size, char const *assembly, i386_as_mode_t mode);
char *ppc_exregname_suffix(int groupnum, int exregnum, char const *suffix, char *buf,
    size_t size);
bool ppc_iseqs_equal_mod_imms(struct ppc_insn_t const *iseq1, long iseq1_len,
    struct ppc_insn_t const *iseq2, long iseq2_len);
void ppc_insn_canonicalize_symbols(struct ppc_insn_t *insn,
    struct imm_vt_map_t *imm_vt_map);
bool symbol_is_contained_in_ppc_insn(struct ppc_insn_t const *insn, long symval,
    src_tag_t symtag);
bool ppc_insn_symbols_are_contained_in_map(struct ppc_insn_t const *insn,
    struct imm_vt_map_t const *imm_vt_map);
void ppc_insn_rename_addresses_to_symbols(struct ppc_insn_t *insn,
    struct input_exec_t const *in, src_ulong pc);
void ppc_insn_copy_symbols(struct ppc_insn_t *dst, struct ppc_insn_t const *src);
size_t ppc_insn_get_symbols(struct ppc_insn_t const *insn, long const *start_symbol_ids, long *symbol_ids, size_t max_symbol_ids);
void ppc_insn_fetch_preprocess(ppc_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long size);
bool ppc_imm_operand_needs_canonicalization(int opc, int explicit_op_index);

regset_t const *ppc_regset_live_at_jumptable();
struct ppc_strict_canonicalization_state_t *ppc_strict_canonicalization_state_init();
void ppc_strict_canonicalization_state_free(struct ppc_strict_canonicalization_state_t *);
long ppc_insn_get_max_num_imm_operands(ppc_insn_t const *insn);
void ppc_pc_get_function_name_and_insn_num(input_exec_t const *in, src_ulong pc, std::string &function_name, long &insn_num_in_function);
size_t ppc_insn_put_function_return(ppc_insn_t *insn);
size_t ppc_insn_put_ret_insn(ppc_insn_t *insn, int retval);
size_t ppc_emit_translation_marker_insns(ppc_insn_t *buf, size_t buf_size);
size_t
ppc_assemble_exec_iseq_using_regmap(uint8_t *buf, size_t buf_size,
    ppc_insn_t const *exec_iseq, long exec_iseq_len, regmap_t const *dst_regmap,
    regmap_t const *src_regmap, int d);
