#ifndef SRC_INSN_H
#define SRC_INSN_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "support/types.h"
#include "support/src-defs.h"
#include "support/consts.h"
#include "valtag/imm_vt_map.h"
#include "insn/insn_line_map.h"
#include "i386/insntypes.h"
#include "ppc/insn.h"
#include "tfg/tfg.h"

//#define MAX_ILEN 256
//assert MAX_ILEN >= I386_MAX_ILEN, PPC_MAX_ILEN, ...

struct src_insn_t;
struct i386_insn_t;
struct regset_t;
struct regmap_t;
struct input_exec_t;
struct si_entry_t;
struct peep_table_t;
struct translation_instance_t;
struct input_exec_t;
//struct imm_map_t;
struct transmap_t;
struct memlabel_t;
struct memset_t;
//struct memlabel_map_t;
struct src_strict_canonicalization_state_t;
class fixed_reg_mapping_t;
class nextpc_t;

void src_iseq_check_sanity(struct src_insn_t const *src_iseq, long src_iseq_len,
    struct regset_t const *touch, struct regset_t const *tmap_regs,
    struct regset_t const *live_regs, long num_live_outs,
    long linenum);
void src_iseq_compute_touch_var(struct regset_t *touch, struct regset_t *use,
    struct memset_t *memuse, struct memset_t *memdef,
    struct src_insn_t const *src_iseq, long src_iseq_len);
//void src_iseq_convert_pcs_to_inums(struct src_insn_t *src_iseq,
//    src_ulong const *pcs, long src_iseq_len, src_ulong *nextpcs, long *num_nextpcs);
bool src_iseq_fetch(struct src_insn_t *iseq, size_t iseq_size,
    long *len,
    struct input_exec_t const *in, struct peep_table_t const *peep_table,
    src_ulong pc, long fetchlen,
    long *num_nextpcs, nextpc_t *nextpcs, src_ulong *insn_pcs,
    long *num_lastpcs, src_ulong *lastpcs, double *lastpc_weights,
    bool allow_shorter);
/*bool src_iseq_fetch_with_pcs(struct src_insn_t *iseq,
    src_ulong *pcs, long *len,
    struct input_exec_t const *in, struct peep_table_t const *peep_table,
    src_ulong pc, long fetchlen,
    long *num_lastpcs, src_ulong *lastpcs, double *lastpc_weights,
    src_ulong *fallthrough_pc);*/

void si_init(struct input_exec_t *in, char const *function_name);
void si_free(struct input_exec_t *in);

long pc2index(struct input_exec_t const *in, src_ulong pc);

long si_get_num_preds(struct si_entry_t const *si);
long si_get_preds(struct input_exec_t const *in, struct si_entry_t const *si,
  src_ulong *preds, double *weights);
void si_print(FILE *file, struct input_exec_t const *in);
void src_iseq_copy(struct src_insn_t *dst, struct src_insn_t const *src, long n);

typedef enum { SYMBOL_NONE, SYMBOL_PCREL, SYMBOL_ABS } sym_type_t;
typedef struct sym_t
{
  char name[64];
  unsigned long val;
  sym_type_t type;
} sym_t;

ssize_t insn_assemble(struct translation_instance_t *ti, uint8_t *bincode,
    char const *assembly,
    int (*insn_md_assemble)(char *, uint8_t *),
    ssize_t (*patch_jump_offset)(uint8_t *, ssize_t, sym_type_t, unsigned long,
        char const *, bool),
    sym_t *symbols, long *num_symbols);
void src_iseq_rename_abs_inums_using_map(struct src_insn_t *iseq, long iseq_len,
    long const *map, long map_size);
void src_iseq_convert_abs_inums_to_pcrel_inums(struct src_insn_t *iseq, long iseq_len);
void src_iseq_convert_pcrel_inums_to_abs_inums(struct src_insn_t *iseq, long iseq_len);
long src_iseq_eliminate_nops(struct src_insn_t *iseq, src_ulong* insn_pcs, long iseq_len);
bool superoptimizer_supports_src(struct src_insn_t const *iseq, long len);
//bool superoptimizer_supports_i386(struct i386_insn_t const *iseq, long len);
bool src_iseq_merits_optimization(struct src_insn_t const *iseq, long iseq_len);
void src_iseq_convert_pcs_to_inums(struct src_insn_t *src_iseq, src_ulong const *pcs,
    long src_iseq_len, nextpc_t *nextpcs, long *num_nextpcs);
long src_iseq_canonicalize(struct src_insn_t *iseq, src_ulong* canon_insn_pcs, long iseq_len);
long src_iseq_strict_canonicalize(struct src_strict_canonicalization_state_t *scanon_state,
    struct src_insn_t const *iseq, long const iseq_len,
    struct src_insn_t *iseq_var[], long *iseq_var_len, struct regmap_t *regmap,
    struct imm_vt_map_t *imm_map, fixed_reg_mapping_t const &fixed_reg_mapping,
    src_ulong* canon_insn_pcs,
    long max_num_insn_var,
    bool canonicalize_imms);
long src_iseq_strict_canonicalize_regs(struct src_strict_canonicalization_state_t *scanon_state,
    struct src_insn_t *iseq, long iseq_len,
    struct regmap_t *st_regmap);
long src_iseq_from_string(struct translation_instance_t *ti, struct src_insn_t *src_iseq,
    size_t src_iseq_size, char const *assembly, i386_as_mode_t i386_as_mode);
int src_iseq_disassemble(struct src_insn_t *src_iseq, uint8_t *bincode, int binsize,
    struct vconstants_t *vconstants[], nextpc_t *nextpcs,
    long *num_nextpcs, i386_as_mode_t i386_as_mode);
//void do_canonicalize_imms_using_syms(struct valtag_t *vt, struct input_exec_t *in);
void src_iseq_compute_fingerprint(uint64_t *fingerprint,
    struct src_insn_t const *src_iseq, long src_iseq_len,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    struct regset_t const *live_out, long num_live_out, bool mem_live_out,
    rand_states_t const &rand_states);
long fscanf_harvest_output_src(FILE *fp, struct src_insn_t *src_iseq, size_t src_iseq_size,
    long *src_iseq_len, struct regset_t *live_out, size_t live_out_size,
    size_t *num_live_out, bool *mem_live_out, long linenum);
void src_iseq_canonicalize_symbols(struct src_insn_t *src_iseq, long src_iseq_len,
    struct imm_vt_map_t *imm_vt_map);
void src_iseq_canonicalize_locals(struct src_insn_t *src_iseq, long src_iseq_len,
    struct imm_vt_map_t *imm_vt_map);
bool src_insn_fetch(struct src_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long *size);
unsigned long src_iseq_hash_func_after_canon(struct src_insn_t const *src_iseq, long src_iseq_len, long table_size);
void src_iseq_hash_canonicalize(struct src_insn_t *insn, int num_insns);
unsigned long src_iseq_hash_func(struct src_insn_t const *insns, int n_insns);
//void src_insn_add_memtouch_elem(struct src_insn_t *insn, int seg, int base, int index, int64_t disp,
//    int size, bool var);
size_t add_jump_to_pcrel_src_insns(nextpc_t const& target_pc, struct src_insn_t *src_iseq, src_ulong *pcs, size_t max_insns);
void src_get_usedef_using_transfer_function(src_insn_t const *insn,
    regset_t *use, regset_t *def, memset_t *memuse,
    memset_t *memdef);
ts_tab_entry_cons_t * src_get_type_constraints_using_transfer_function(src_insn_t const *insn,
    char const *progress_str);
void src_regset_truncate_masks_based_on_reg_sizes_in_insn(regset_t *regs, src_insn_t const *src_insn);
size_t src_insn_get_size_for_exreg(src_insn_t const *src_insn, exreg_group_id_t group, reg_identifier_t const &r);
long src_iseq_window_convert_pcrels_to_nextpcs(src_insn_t *src_iseq, long src_iseq_len,
    long start, long *nextpc2src_pcrel_vals, src_tag_t *nextpc2src_pcrel_tags);
void src_iseq_rename_nextpcs(src_insn_t *src_iseq, long src_iseq_len, nextpc_map_t const *nextpc_map);
void src_insn_rename_nextpcs(src_insn_t *insn, nextpc_map_t const *nextpc_map);
void src_iseq_rename_constants(src_insn_t *iseq, long iseq_len, imm_vt_map_t const &imm_vt_map);
list<in_out_tmaps_t> src_iseq_enumerate_in_out_transmaps(src_insn_t const *src_iseq, long src_iseq_len, regset_t const *live_out, long num_live_out, bool mem_live_out, int prioritize);


typedef struct src_fingerprintdb_elem_t {
  uint64_t *fdb_fp/*[NUM_FP_STATES]*/;
  char comment[MAX_COMMENT_SIZE];
  struct src_insn_t *src_iseq;
  long src_iseq_len;
  struct transmap_t in_tmap, *out_tmaps;
  struct regset_t *live_out;
  long num_live_out;
  bool mem_live_out;

  //dst_regmap is used to inv_rename in_tmap and out_tmaps
  //and enumerated dst_iseq before adding to superoptdb.
  //struct regmap_t dst_regmap;
  //bool optimal_found;
  //struct locals_info_t *locals_info;
  struct insn_line_map_t insn_line_map;

  struct src_fingerprintdb_elem_t *next;
} src_fingerprintdb_elem_t;
#define FINGERPRINTDB_SIZE (1024*1024)


void src_fingerprintdb_add_variants(struct src_fingerprintdb_elem_t **ret,
    struct src_insn_t const *src_iseq,
    long src_iseq_len, regset_t const *live_out,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    long num_live_out,// struct locals_info_t *locals_info_t,
    //struct insn_line_map_t const *insn_line_map,
    //use_types_t use_types,
    struct typestate_t const *typestate_initial,
    struct typestate_t const *typestate_final,
    bool mem_live_out, struct regmap_t const *in_regmap, char const *comment,
    rand_states_t const &rand_states);
void src_fingerprintdb_add_from_fp(src_fingerprintdb_elem_t **ret, FILE *fp,
    //use_types_t use_types,
    struct typestate_t *typestate_initial,
    struct typestate_t *typestate_final, char const *function_name);
void src_fingerprintdb_add_from_string(src_fingerprintdb_elem_t **ret, char const *harvested_src_iseqs,
    use_types_t use_types, typestate_t *typestate_initial, typestate_t *typestate_final, rand_states_t const &rand_states);
struct src_fingerprintdb_elem_t **src_fingerprintdb_init(void);
void src_fingerprintdb_elem_free(struct src_fingerprintdb_elem_t *elem);
void src_fingerprintdb_free(struct src_fingerprintdb_elem_t **fingerprintdb);
size_t src_fingerprintdb_to_string(char *buf, size_t size,
    struct src_fingerprintdb_elem_t * const *fingerprintdb/*, bool allow_only_identity*/);

void src_fingerprintdb_add_from_string(src_fingerprintdb_elem_t **ret,
    char const *harvested_src_iseqs,
    use_types_t use_types, struct typestate_t *typestate_initial,
    struct typestate_t *typestate_final, rand_states_t const &rand_states);
bool src_fingerprintdb_is_empty(src_fingerprintdb_elem_t * const *fingerprintdb);
size_t src_fingerprintdb_to_string(char *buf, size_t size,
    src_fingerprintdb_elem_t * const *fingerprintdb/*, bool allow_only_identity*/);
struct src_fingerprintdb_elem_t const *
src_fingerprintdb_search_for_matching_comment(char const *comment,
    //long num_nextpcs, struct imm_vt_map_t const *imm_vt_map,
    struct src_fingerprintdb_elem_t * const *haystack);
void src_iseq_serialize(ostream& os, src_insn_t const* iseq, size_t iseq_len);
size_t src_iseq_deserialize(istream& is, src_insn_t *buf, size_t bufsize);


#endif /* src-insn.h */
