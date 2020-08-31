#ifndef EQUIV_H
#define EQUIV_H
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "support/src-defs.h"
#include "support/types.h"
#include "support/consts.h"
#include "superopt/typestate.h"

//struct symbol_map_t;

//extern void *membase;
extern struct imm_vt_map_t *src_env_imm_map;
//extern void *memtmp;

extern bool /*verify_boolean, */verify_execution, verify_fingerprint;
extern bool terminate_on_equiv_failure;
extern bool verify_syntactic_check;
extern bool fp_imm_map_is_constant;
extern use_types_t default_use_types;

struct transmap_t;
struct src_insn_t;
struct dst_insn_t;
struct regset_t;
struct regmap_t;
struct state_t;
struct typestate_t;
struct translation_instance_t;
struct src_fingerprintdb_elem_t;
struct itable_t;
struct jtable_t;
struct itable_position_t;
class memslot_set_t;
class txinsn_t;
namespace eqspace {
class sym_exec;
}
class quota;
class rand_states_t;
//struct insn_line_map_t;
class fingerprintdb_live_out_t;

//extern struct translation_instance_t ti_fallback;
class fixed_reg_mapping_t;

void equiv_init(void);
bool check_equivalence(struct src_insn_t const *src_iseq, int src_iseq_len,
    struct transmap_t const *transmap, struct transmap_t const *out_transmap,
    struct dst_insn_t const *dst_iseq,
    int dst_iseq_len, struct regset_t const *live_out,
    int nextpc_indir,
    long num_live_outs, bool mem_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    long max_unroll, quota const &qt);
int init_execution_environment(void);

bool check_fingerprints(struct src_insn_t const *src_iseq, long src_iseq_len,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    struct dst_insn_t const *dst_iseq, long dst_iseq_len,
    struct regset_t const *live_out,
    long num_live_out, bool mem_live_out, rand_states_t const &rand_states);

/*void
src_fingerprintdb_remove_variants(struct src_fingerprintdb_elem_t **ret,
    struct src_insn_t const *src_iseq,
    long src_iseq_len, regset_t const *live_out,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    long num_live_out,
    use_types_t use_types, bool mem_live_out, struct regmap_t const *in_regmap);*/

/*void
fingerprintdb_add_variants(struct fingerprintdb_elem_t **ret,
    struct src_insn_t const *src_iseq,
    long src_iseq_len, struct regset_t const *live_out,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    long num_live_out, use_types_t use_types,
    struct typestate_t *typestate_initial,
    struct typestate_t *typestate_final, bool mem_live_out,
    struct regmap_t const *in_regmap, char const *comment);
*/

bool src_fingerprintdb_query(struct src_fingerprintdb_elem_t **fingerprintdb,
    struct dst_insn_t const *dst_iseq, long dst_iseq_len,
    char const *equiv_output_filename);

long
src_fingerprintdb_query_using_jtable_and_ipos(struct src_fingerprintdb_elem_t **fingerprintdb,
    struct jtable_t *jtable, struct itable_t const *itable,
    struct itable_position_t *ipos,
    uint8_t *bincode, size_t binsize,
    long long const *max_costs,
    struct typestate_t const *typestate_initial,
    struct typestate_t const *typestate_final, regset_t const *def,
    memslot_set_t const *memslot_def, bool *found,
    char const *equiv_output_filename, rand_states_t const &rand_states,
    uint64_t **fps, long *num_fps,
    fingerprintdb_live_out_t const &fingerprintdb_live_out);


bool check_type_using_bincode(uint8_t const *bincode, size_t binsize);
uint64_t state_compute_fingerprint(struct state_t const *instate,
    struct state_t const *state,
    struct regset_t const *live_out,
    memslot_set_t const *live_out_memslots,
    long num_live_out, bool mem_live_out);


/*uint64_t src_iseq_compute_fingerprint(struct src_insn_t const *src_iseq,
    long src_iseq_len,
    struct transmap_t const *tmap, struct transmap_t const *out_tmaps,
    struct regset_t const *live_out,
    long num_live_out);*/

/*void src_gen_out_states_using_regmap(struct src_insn_t const *src_iseq,
    long src_iseq_len,
    struct regmap_t const *regmap, struct state_t const *in_state,
    struct state_t *out_state, int num_states, src_ulong const *nextpcs,
    //src_ulong const *insn_pcs,
    uint8_t const * const *next_tcodes, long num_live_out);*/
/*void i386_gen_out_states(struct i386_insn_t const *i386_iseq, int i386_iseq_len,
    struct state_t const *in_state,
    struct state_t *out_state, int num_states,
    uint8_t const **next_tcodes, long num_live_out);
void i386_gen_out_states_using_bincode(uint8_t code[][MAX_CODE_LEN], size_t codesize,
    uint16_t jmp_offset0, struct regmap_t const *regmap,
    struct state_t const *in_state, struct state_t *out_state,
    int num_states, struct transmap_t const *transmap,
    struct transmap_t const *out_transmaps, long num_live_out);*/

struct state_t;
//extern int imm_map_index[];
//extern struct state_t *rand_state;

/*
#define copy_to_env(state, outstate) do { \
  state_t *cpustate = (state_t *)SRC_ENV_ADDR; \
  memcpy(cpustate->regs, (state)->regs, sizeof (state)->regs); \
  if ((state) != ((outstate))) { \
    memcpy((outstate)->memory, (state)->memory, MEMORY_SIZE + 8); \
  } \
  ((state_t *)cpustate)->regs[MEM_BASE_REG] = (uint32_t)(unsigned long)((outstate)->memory); \
} while (0)

#define copy_from_env(state) do { \
  state_t *cpustate = (state_t *)SRC_ENV_ADDR; \
  memcpy((state)->regs, cpustate->regs, sizeof (state)->regs); \
} while (0)

#define execute_code_on_state(code, state, outstate) do { \
    int jump_not_taken; \
    membase = (uint32_t *)((outstate)->memory); \
    copy_to_env((state), (outstate)); \
    jump_not_taken = ((int(*)())(code))(); \
    if (jump_not_taken) { \
      (outstate)->branch_taken = 0; \
    } else { \
      (outstate)->branch_taken = 1; \
    } \
    copy_from_env((outstate)); \
    (outstate)->memory[0] = (outstate)->memory[0] ^ (outstate)->memory[MEMORY_SIZE>>2]; \
    (outstate)->memory[1] = (outstate)->memory[1] ^ (outstate)->memory[(MEMORY_SIZE>>2) + 1]; \
} while (0)
*/

class nextpc_t;
//extern txinsn_t *exec_next_tcodes;
extern uint8_t **exec_next_tcodes;
extern nextpc_t *exec_nextpcs;
extern long exec_nextpcs_size;
extern long num_exec_nextpcs;
class rand_states_t;

void alloc_exec_nextpcs(long num_live_out);
bool iseqs_match_syntactically(src_insn_t const *src_iseq, long src_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    dst_insn_t const *dst_iseq, long dst_iseq_len, regset_t const *live_out,
    long nextpc_indir, long num_live_out);
bool execution_check_equivalence(src_insn_t const src_iseq[], int src_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    dst_insn_t const dst_iseq[], int dst_iseq_len,
    regset_t const *live_out, long num_live_out, bool mem_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping);

long itable_construct_dst_iseq(struct dst_insn_t *iseq, struct itable_t const *itable,
    struct itable_position_t const *ipos);


//extern struct rand_states_t fp_state;
extern bool peepgen_assume_fcall_conventions;

#endif /* equiv.h */
