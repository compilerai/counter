#ifndef DST_INSN_H
#define DST_INSN_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "support/types.h"
#include "support/src-defs.h"
#include "support/consts.h"
#include "valtag/imm_vt_map.h"
#include "valtag/fixed_reg_mapping.h"
#include "tfg/tfg.h"
#include "i386/insn.h"

struct dst_insn_t;
struct translation_instance_t;
struct input_exec_t;
struct regmap_t;
struct regset_t;
struct transmap_t;
struct imm_vt_map_t;
struct nextpc_map_t;
struct memset_t;
struct ts_tab_entry_cons_t;
//struct memlabel_map_t;
struct dst_strict_canonicalization_state_t;
class rand_states_t;
class fixed_reg_mapping_t;

bool dst_iseq_contains_nop(struct dst_insn_t const *iseq, size_t iseq_len);
bool dst_iseq_contains_div(struct dst_insn_t const *iseq, long iseq_len);

long dst_iseq_optimize_branching_code(struct dst_insn_t *iseq, long iseq_len, long iseq_size);
void dst_iseq_rename_abs_inums_using_map(struct dst_insn_t *iseq, long iseq_len,
    long const *map, long map_size);
void dst_iseq_rename_sboxed_abs_inums_using_map(struct dst_insn_t *iseq, long iseq_len,
    long const *map, long map_size);
void dst_iseq_convert_abs_inums_to_pcrel_inums(struct dst_insn_t *iseq, long iseq_len);
void dst_iseq_convert_sboxed_abs_inums_to_pcrel_inums(struct dst_insn_t *iseq, long iseq_len);
void dst_iseq_convert_sboxed_abs_inums_to_binpcrel_inums(struct dst_insn_t *iseq, long iseq_len, long const *pc_map);
void dst_iseq_convert_pcrel_inums_to_abs_inums(struct dst_insn_t *iseq, long iseq_len);
long dst_iseq_eliminate_nops(struct dst_insn_t *iseq, long iseq_len);
void dst_iseq_convert_pcs_to_inums(struct dst_insn_t *dst_iseq, src_ulong const *pcs,
    long dst_iseq_len, nextpc_t *nextpcs, long *num_nextpcs);
void dst_iseq_pick_reg_assignment(struct regmap_t *regmap, struct dst_insn_t const *dst_iseq,
    long dst_iseq_len);
void dst_iseq_add_insns_to_exit_paths(struct dst_insn_t *dst_iseq, long *dst_iseq_len,
    size_t dst_iseq_size, struct dst_insn_t const *exit_insns, long num_exit_insns);
long dst_iseq_connect_end_points(struct dst_insn_t *iseq, long len, size_t iseq_size,
    nextpc_t const *nextpcs,
    //uint8_t const * const *next_tcodes,
    //dst_insn_t const * const *next_tinsns,
    //txinsn_t const *next_tinsns,
    struct transmap_t const *out_tmaps,
    struct transmap_t const * const *next_tmaps,
    struct regset_t const *live_outs, long num_nextpcs, bool fresh,
    eqspace::state const *start_state, int memloc_offset_reg,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    std::map<symbol_id_t, symbol_t> const& symbol_map,
    i386_as_mode_t mode);
long dst_iseq_canonicalize(struct dst_insn_t *iseq, long iseq_len);
void dst_iseq_rename_vconsts(struct dst_insn_t *iseq, long iseq_len,
    struct imm_vt_map_t const *imm_map/*,
    src_ulong const *nextpcs, long num_nextpcs, struct regmap_t const *regmap*/);
long dst_iseq_make_position_independent(struct dst_insn_t *iseq, long iseq_len,
    long iseq_size, uint32_t scratch_addr);
long dst_iseq_canonicalize(struct dst_insn_t *iseq, long iseq_len);
long dst_iseq_strict_canonicalize(struct dst_strict_canonicalization_state_t *scanon_state,
    struct dst_insn_t const *iseq, long const iseq_len,
    struct dst_insn_t *iseq_var[], long *iseq_var_len, struct regmap_t *regmap,
    struct imm_vt_map_t *imm_map, fixed_reg_mapping_t const &fixed_reg_mapping,
    long max_num_insn_var,
    bool canonicalize_imms);
long dst_iseq_strict_canonicalize_regs(struct dst_strict_canonicalization_state_t *scanon_state,
    struct dst_insn_t *iseq, long iseq_len,
    struct regmap_t *st_regmap);
long dst_iseq_sandbox_without_converting_sboxed_abs_inums_to_pcrel_inums(dst_insn_t *dst_iseq, long dst_iseq_len,
    long dst_iseq_size,
    int tmpreg, fixed_reg_mapping_t const &dst_fixed_reg_mapping, long cur_insn_num, long *map);
long dst_iseq_sandbox(struct dst_insn_t *dst_iseq, long dst_iseq_len, long dst_iseq_size,
    int tmpreg/*, struct regset_t *ctemps*/, fixed_reg_mapping_t const &dst_fixed_reg_mapping);
long dst_iseq_from_string(struct translation_instance_t *ti, struct dst_insn_t *src_iseq,
    size_t src_iseq_size, char const *assembly, i386_as_mode_t i386_as_mode);
int dst_iseq_disassemble(struct dst_insn_t *dst_iseq, uint8_t *bincode, int binsize,
    struct vconstants_t *vconstants[], nextpc_t *nextpcs,
    long *num_nextpcs, i386_as_mode_t i386_as_mode);

int dst_iseq_disassemble_with_bin_offsets(struct dst_insn_t *dst_iseq, uint8_t *bincode, int binsize,
    struct vconstants_t *vconstants[], nextpc_t *nextpcs,
    long *num_nextpcs, src_ulong* bin_offsets, i386_as_mode_t i386_as_mode);
void dst_iseq_compute_fingerprint(uint64_t *fingerprint,
    struct dst_insn_t const *dst_iseq, long dst_iseq_len,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    struct regset_t const *live_out, long num_live_out, bool mem_live_out,
    rand_states_t const &rand_states);
long fscanf_harvest_output_dst(FILE *fp, struct dst_insn_t *dst_iseq, size_t dst_iseq_size,
    long *dst_iseq_len, struct regset_t *live_out, size_t live_out_size,
    size_t *num_live_out, bool *mem_live_out, long linenum);
bool dst_iseq_inv_rename_symbols(struct dst_insn_t *dst_iseq, long dst_iseq_len,
    struct imm_vt_map_t *imm_vt_map);
bool dst_iseq_inv_rename_locals(struct dst_insn_t *dst_iseq, long dst_iseq_len,
    struct imm_vt_map_t const *imm_vt_map);
void dst_iseq_rename_nextpcs(struct dst_insn_t *dst_iseq, long dst_iseq_len,
    struct nextpc_map_t const *nextpc_map);
void dst_iseq_rename_symbols_to_addresses(struct dst_insn_t *dst_iseq,
    long dst_iseq_len, struct input_exec_t const *in, struct chash const *tcodes);
unsigned long dst_iseq_hash_func_after_canon(struct dst_insn_t const *dst_iseq, long dst_iseq_len, long table_size);
unsigned long dst_iseq_hash_func(struct dst_insn_t const *insns, int n_insns);
void dst_iseq_hash_canonicalize(struct dst_insn_t *insn, int num_insns);
//void dst_insn_add_memtouch_elem(struct dst_insn_t *insn, int seg, int base, int index, int64_t disp,
//    int size, bool var);
void gas_init(void);
void dst_iseq_canonicalize_symbols(struct dst_insn_t *dst_iseq, long dst_iseq_len,
    struct imm_vt_map_t *imm_vt_map);
void dst_iseq_canonicalize_locals(struct dst_insn_t *dst_iseq, long dst_iseq_len,
    struct imm_vt_map_t *imm_vt_map);
void dst_get_usedef_using_transfer_function(dst_insn_t const *insn,
    regset_t *use, regset_t *def, memset_t *memuse,
    memset_t *memdef);
void dst64_iseq_get_usedef_regs(struct dst_insn_t const *dst_iseq, long dst_iseq_len,
    regset_t *use, regset_t *def);
//ts_tab_entry_cons_t * dst_get_type_constraints_using_transfer_function(dst_insn_t const *insn,
//    char const *progress_str);
bool dst_insn_get_pcrel_value(struct dst_insn_t const *insn, unsigned long *pcrel_val,
    src_tag_t *pcrel_tag);
void dst_insn_set_pcrel_value(struct dst_insn_t *insn, unsigned long pcrel_val,
    src_tag_t pcrel_tag);
valtag_t *dst_insn_get_pcrel_operand(dst_insn_t const *insn);
int dst_get_callee_save_regnum(int index);
shared_ptr<eqspace::tfg> dst_iseq_get_preprocessed_tfg(dst_insn_t const *iseq, long iseq_len, transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_live_out, graph_arg_regs_t const &argument_regs, map<pc, map<string_ref, expr_ref>> const &return_reg_map, fixed_reg_mapping_t const &fixed_reg_mapping, context *ctx, eqspace::sym_exec &se);
void dst_regset_truncate_masks_based_on_reg_sizes_in_insn(regset_t *regs, dst_insn_t const *dst_insn);
size_t dst_insn_get_size_for_exreg(dst_insn_t const *dst_insn, exreg_group_id_t group, reg_identifier_t const &r);
void dst_insn_rename_nextpcs(dst_insn_t *insn, nextpc_map_t const *nextpc_map);
void dst_iseq_patch_jump_to_nextpcs_using_markers(dst_insn_t *dst_iseq, size_t dst_iseq_len, vector<size_t> const &nextpc_code_indices, size_t nm_nextpcs);
size_t dst_iseq_get_idx_for_marker(dst_insn_t const *dst_iseq, size_t dst_iseq_len, size_t marker);
void dst_iseq_rename_cregs_to_vregs(dst_insn_t *iseq, long iseq_len);
void dst_iseq_rename_vregs_to_cregs(dst_insn_t *iseq, long iseq_len);
size_t read_all_dst_insns(vector<dst_insn_t> *buf, size_t size, function<bool (vector<dst_insn_t>)> f, dst_strict_canonicalization_state_t *scanon_state, fixed_reg_mapping_t const &fixed_reg_mapping);
size_t read_related_dst_insns(std::vector<dst_insn_t> *buf, size_t size, dst_insn_t const *dst_iseq, long dst_iseq_len, function<bool (vector<dst_insn_t>)> f, dst_strict_canonicalization_state_t *scanon_state, fixed_reg_mapping_t const &fixed_reg_mapping);

char *dst_insn_vec_to_string(std::vector<dst_insn_t> const &iseq, char *buf, size_t size);
void dst_insn_vec_get_usedef_regs(std::vector<dst_insn_t> const &iseq, regset_t *use, regset_t *def);
void dst_insn_vec_get_usedef(std::vector<dst_insn_t> const &iseq, regset_t *use, regset_t *def, memset_t *memuse, memset_t *memdef);
void dst_insn_vec_copy_to_arr(dst_insn_t *arr, std::vector<dst_insn_t> const &vec);
void dst_insn_vec_copy(std::vector<dst_insn_t> &dst, std::vector<dst_insn_t> const &src);
bool dst_insn_vec_is_nop(vector<dst_insn_t> const &iseq);
void dst_insn_vec_convert_last_nextpc_to_fallthrough(vector<dst_insn_t> &iseq);
bool dst_insn_vecs_equal(vector<dst_insn_t> const &a, vector<dst_insn_t> const &b);
bool dst_iseq_supports_boolean_test(dst_insn_t const *dst_iseq, long dst_iseq_len);
bool dst_iseq_is_nop(dst_insn_t const *dst_iseq, long dst_iseq_len);
bool dst_iseqs_equal(dst_insn_t const *a, dst_insn_t const *b, long iseq_len);
void dst_iseq_canonicalize_vregs(dst_insn_t *dst_iseq, long dst_iseq_len, regmap_t *dst_regmap, fixed_reg_mapping_t const &fixed_reg_mapping);
vector<dst_insn_t> dst_insn_vec_from_iseq(dst_insn_t const *dst_iseq, long dst_iseq_len);
long dst_iseq_get_num_nextpcs(dst_insn_t const *iseq, long iseq_len);
void dst_iseq_rename_fixed_regs(dst_insn_t *dst_iseq, long dst_iseq_len, fixed_reg_mapping_t const &from, fixed_reg_mapping_t const &to);
void dst_iseq_rename_vregs_using_regmap(dst_insn_t *dst_iseq, long dst_iseq_len, regmap_t const &regmap);
long long dst_iseq_compute_cost(dst_insn_t const *iseq, long iseq_len, dst_costfn_t costfn);
void dst_iseq_rename_tag_src_insn_pcs_to_tag_vars(dst_insn_t *iseq, long iseq_len, nextpc_t const *nextpcs, uint8_t const * const *varvals, size_t num_varvals);
void dst_insn_pcs_to_stream(ostream& os, std::vector<dst_ulong> const& dst_insn_pcs);
vector<dst_ulong> dst_insn_pcs_from_stream(istream& is);
vector<dst_insn_t> dst_iseq_from_stream(istream& is);
void dst_iseq_serialize(ostream& os, dst_insn_t const* iseq, size_t iseq_len);
size_t dst_iseq_deserialize(istream& is, dst_insn_t *buf, size_t bufsize);


#endif
