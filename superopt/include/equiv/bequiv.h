#ifndef BEQUIV_H
#define BEQUIV_H
#include <stdbool.h>
#include <stdlib.h>
#include "support/src-defs.h"
#define MAX_SRC_NUM_REGS 48

struct src_insn_t;
struct dst_insn_t;
struct regset_t;
struct transmap_t;
struct ts_tab_entry_cons_t;
struct ts_tab_entry_transfer_t;
struct locals_info_t;
struct insn_line_map_t;
//struct symbol_map_t;
struct nextpc_function_name_map_t;
struct imm_vt_map_t;
namespace eqspace {
class sym_exec;
}
class quota;
class fixed_reg_mapping_t;

bool
boolean_check_equivalence(src_insn_t const src_iseq[], long src_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmap,
    dst_insn_t const dst_iseq[], long dst_iseq_len,
    regset_t const *live_out, int nextpc_indir,
    long num_live_out, bool mem_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    long max_unroll, quota const &qt);

char *src_get_transfer_state_str(struct src_insn_t const* src_insn,
    int num_nextpcs, int nextpc_indir);
char *dst_get_transfer_state_str(struct dst_insn_t const *dst_insn,
    int num_nextpcs, int nextpc_indir);

/*
void gen(struct src_insn_t const *src_iseq, long src_iseq_len,
    struct transmap_t const *transmap, struct transmap_t const *out_transmap,
    struct dst_insn_t const *dst_iseq, long dst_iseq_len,
    struct regset_t const *live_out, int nextpc_indir, long num_live_out,
    bool mem_live_out,
    struct symbol_map_t const *src_symbol_map,
    struct symbol_map_t const *dst_symbol_map,
    struct locals_info_t *src_locals_info,
    struct locals_info_t *dst_locals_info,
    struct insn_line_map_t const *src_iline_map,
    struct insn_line_map_t const *dst_iline_map,
    struct nextpc_function_name_map_t const *nextpc_function_name_map,
    long max_unroll, char const *tfg_llvm, char const *function_name, struct context *ctx);
*/

void src_get_usedef_regs_using_transfer_function(struct src_insn_t const *insn,
    struct regset_t *use, struct regset_t *def);
void dst_get_usedef_regs_using_transfer_function(struct dst_insn_t const *insn,
    struct regset_t *use, struct regset_t *def);

struct ts_tab_entry_cons_t *
src_get_type_constraints_using_transfer_function(struct src_insn_t const *insn,
    char const *progress_str);
//struct ts_tab_entry_cons_t *
//dst_get_type_constraints_using_transfer_function(struct dst_insn_t const *insn,
//    char const *progress_str);

struct ts_tab_entry_transfer_t *
src_get_type_transfers_using_transfer_function(struct src_insn_t const *insn,
    char const *progress_str);
struct ts_tab_entry_transfer_t *
dst_get_type_transfers_using_transfer_function(struct dst_insn_t const *insn,
    char const *progress_str);



#endif /* bequiv.h */
