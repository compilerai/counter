#ifndef __SUPEROPT_H
#define __SUPEROPT_H

#include <vector>
#include "support/src-defs.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "ppc/insn.h"
#include "typestate.h"
#include "support/debug.h"
#include "superopt/itable_position.h"
#include "superopt/itable.h"

struct state_t;
struct typestate_t;
struct src_insn_t;
struct i386_insn_t;
struct regset_t;
struct transmap_t;
struct jtable_t;
struct src_fingerprintdb_elem_t;
class itable_stopping_cond_t;
class rand_states_t;
class typesystem_t;

std::vector<itable_t>
build_itables(struct src_insn_t const *src_iseq, long src_iseq_len,
    regset_t const *live_out,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    long num_out_tmaps/*,
    struct vector<dst_insn_t> const *all_iseqs, size_t num_all_iseqs*/);

int superoptimize(struct src_insn_t const *src_iseq, int src_iseq_len,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmap,
    struct regset_t const *live_out0, struct regset_t const *live_out1,
    char *x86_template, int x86_template_size);

size_t itable_sequence_get_dst_iseq(dst_insn_t *dst_iseq, itable_t const *itable,
    long const *sequence, int sequence_len);
//uint64_t state_compute_fingerprint(struct state_t const *state,
//    struct regset_t const *live_regs, long num_live_outs);

bool itable_enumerate(struct itable_t const *itable, struct jtable_t *jtable,
    struct src_fingerprintdb_elem_t **fingerprintdb, itable_position_t *ipos,
    long long const *max_costs,
    uint64_t yield_secs,
    itable_stopping_cond_t itable_stopping_cond, bool slow_fingerprint,
    //bool use_types,
    struct typestate_t const *typestate_initial,
    struct typestate_t const *typestate_final,
    regset_t const &dst_touch,
    char const *equiv_output_filename,
    rand_states_t const &rand_states);

void itable_truncate_using_typestate(itable_t *itab, typestate_t const *ts_initial,
    typestate_t const *ts_final);
void itable_typestates_add_integer_temp_if_needed(itable_t *itab,
    typestate_t *typestate_initial, typestate_t *typestate_final, size_t num_typestates);
void src_fingerprintdb_compute_max_costs(src_fingerprintdb_elem_t * const *fingerprintdb,
    long long *max_costs);
itable_t itable_all(src_insn_t const *src_iseq, long src_iseq_len,
    regset_t const *live_out,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    long num_live_out,
    vector<dst_insn_t> const *all_iseqs,
    size_t num_all_iseqs,
    /*bool (*filter_function)(src_insn_t const *, long, vector<dst_insn_t> const &),
    vector<size_t> const &max_num_temps, size_t max_num_temp_stack_slots,
    bool enum_vconst_modifiers*/
    typesystem_t const &typesystem);

vector<size_t> const &standard_max_num_temps();


/*int itable_sequence_array_from_vector(int *sequence, uint64_t start, size_t itable_size);
uint64_t itable_sequence_array_to_vector(int const *sequence, size_t sequence_len,
    size_t itable_size);*/
#endif
