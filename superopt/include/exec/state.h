#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "support/src-defs.h"
#include "i386/cpu_consts.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "ppc/regs.h"
#include "x64/insntypes.h"
#include "valtag/imm_vt_map.h"
#include "rewrite/txinsn.h"

#define GPR_GROUP 0

#define MEMORY_SIZE_BITS 8
#define MEMORY_SIZE (1 << MEMORY_SIZE_BITS)
#define MEMORY_PADSIZE 8

struct transmap_t;
struct dst_insn_t;
struct src_insn_t;
struct regset_t;
struct regmap_t;
struct regcons_t;
struct state_t;
struct typestate_t;
class rand_states_t;
class fixed_reg_mapping_t;
class memslot_set_t;
class nextpc_t;

typedef struct state_reg_t
{
private:
  uint64_t q0, q1, q2, q3;
public:
  uint64_t state_reg_to_int64() const { return q0; }
  void state_reg_from_int64(uint64_t i) { q0 = i; q1 = 0; q2 = 0; q3 = 0; }
  void state_reg_from_bool(bool b) { q0 = b ? 1 : 0; q1 = 0; q2 = 0; q3 = 0; }
  bool state_reg_to_bool() const
  {
    ASSERT(q0 == 0 || q0 == 1);
    return q0 == 1;
  }
  uint64_t state_reg_compute_fingerprint(long n, uint64_t mask, state_reg_t const &instate_reg) const
  {
    uint64_t regret;
    regret = instate_reg.state_reg_to_int64() ^ this->state_reg_to_int64();
    regret = regret & mask;
    regret *= (n + 1) * 12343ULL;
    return regret;
  }
} state_reg_t;

typedef struct state_t
{
  state_reg_t exregs[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)];
  state_reg_t memlocs[MAX_NUM_MEMLOCS];
  uint32_t stack[STATE_MAX_STACK_SIZE >> 2]; //exregs[DST_EXREG_GROUP_GPRS][DST_STACK_REGNUM] maintains the stack pointer for dst_iseq execution; for src_iseq execution, stack is not used.
  int nextpc_num;

  uint32_t *memory; //[(MEMORY_SIZE>>2)+2];

  imm_vt_map_t imm_vt_map;
  int last_executed_insn;

  static state_t read_concrete_state(istream &iss);
  void set_exreg_to_value(exreg_group_id_t g, exreg_id_t r, uint32_t value);
  void dst_state_make_exregs_consistent();
} state_t;

void state_init(struct state_t *state);
void states_init(struct state_t *states, int num_states);
void state_free(struct state_t *state);
void states_free(struct state_t *states, int num_states);
void state_copy(struct state_t *dst, struct state_t const *src);

bool states_equivalent(state_t const *state1, state_t const *state2,
    struct regset_t const *live_regs,
    memslot_set_t const *live_memslots,
    long num_live_out, bool mem_live_out);

char *state_to_string(state_t const *state, char *buf, size_t size);
void state_rename_using_regmap(struct state_t *state,
    struct regmap_t const *regmap);
void state_inv_rename_using_regmap(struct state_t *state,
    struct regmap_t const *regmap);

//void i386_execute_code_on_state(uint8_t const *code, struct state_t *state);
void dst_execute_code_on_state_and_typestate(uint8_t const *code,
    struct state_t *state, struct typestate_t *typestate_initial,
    struct typestate_t const *typestate_final, fixed_reg_mapping_t const &dst_fixed_reg_mapping);

bool
dst_iseq_rename_regs_and_transmaps_after_inferring_regcons(struct dst_insn_t *dst_iseq,
    long dst_iseq_len,
    struct transmap_t *in_tmap_const, struct transmap_t *out_tmaps_const,
    long num_live_out,
    struct regcons_t *regcons, struct regmap_t *regmap, fixed_reg_mapping_t const &fixed_reg_mapping);

bool dst_gen_out_states/*_using_transmap*/(struct dst_insn_t const *dst_iseq, int dst_iseq_len,
    rand_states_t const &in_state, struct state_t *out_state,
    int num_states, nextpc_t const *nextpcs, uint8_t const *const *next_tcodes,
    //struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    long num_live_out, fixed_reg_mapping_t const &dst_fixed_reg_mapping);

bool
src_gen_out_states(struct src_insn_t const *src_iseq, long src_iseq_len,
    rand_states_t const &in_state, struct state_t *out_state,
    int num_states,
    nextpc_t const *nextpcs,
    uint8_t const * const *next_tcodes,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    long num_live_out);
void state_rename_using_transmap(struct state_t *state, struct transmap_t const *tmap);
void state_rename_using_transmap_inverse(struct state_t *state, struct transmap_t const *tmap);


void ti_fallback_init(void);
void ti_fallback_free(void);

long peep_substitute_using_ti_fallback(src_insn_t const *src_iseq, long src_iseq_len,
    long num_live_out, dst_insn_t *dst_iseq, long dst_iseq_size);
long src_iseq_gen_sandboxed_iseq(dst_insn_t *dst_iseq, long dst_iseq_size,
    src_insn_t const *src_iseq, long src_iseq_len/*, transmap_t const *in_tmap,
    transmap_t const *out_tmaps*/, long num_live_out);
long src_iseq_gen_prefix_insns(dst_insn_t *buf, long size,
    transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping);
void state_copy_to_env(state_t *state);
void state_copy_from_env(state_t *state);



#endif /* state.h */
