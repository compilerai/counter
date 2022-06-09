#ifndef STATIC_TRANSLATE_H
#define STATIC_TRANSLATE_H
#include "support/types.h"
#include "support/dshared_ptr.h"
//#include "src_env.h"


struct CPUPPCState;
struct translation_instance_t;
struct output_exec_t;
struct transmap_t;
struct regset_t;
struct chash;

namespace eqspace {
class tfg;
struct context;
}

int restore_stack_pointer_code(uint8_t *buf, int buf_size);

void static_translate(struct translation_instance_t *ti, char const *filename,
    char const *output_file);
int is_frequent_nip(unsigned long nip);
bool get_translated_addr_using_tcodes(uint32_t *new_addr, struct chash const *tcodes, src_ulong addr);

extern char const *profile_name;
extern int preserve_instruction_order;
extern int aggressive_assumptions_about_flags;
//extern bool use_fallback_translation_only;
extern int static_link_flag;

void ti_add_reloc_entry(struct translation_instance_t *ti, char const *name, long loc,
    char const *section_name, int type);

void static_translate_for_ti(translation_instance_t *ti);
void translation_instance_free_tx_structures(translation_instance_t *ti);
void get_live_outs(input_exec_t const *in, regset_t const **live_outs,
    src_ulong const *pcs, long num_pcs);

void codegen(map<string, dshared_ptr<eqspace::tfg>> const &function_tfg_map, string const &output_filename, vector<string> const& disable_dst_insn_folding_for_functions, eqspace::context *ctx);



struct input_exec_t;

/*int tb_dominates(struct input_exec_t const *in,
    struct bbl_t const *tb1, struct bbl_t const *tb2);

struct tb_t *get_translation_block(struct input_exec_t const *in,
    src_ulong cur);
struct transmap_set_row_t *
get_long_pred_transmap_set(struct input_exec_t const *in,
    struct tb_t const *tb, int len, int index);*/
//void process_cfg(struct input_exec_t *in);

/*size_t debug_dump_regs(uint8_t *buf, size_t buf_size, struct transmap_t const *tmap,
    struct regset_t const *live_in);
size_t debug_restore_regs(uint8_t *buf, size_t buf_size, struct transmap_t const *tmap,
    struct regset_t const *live_in);*/
/*size_t debug_call_print_machine_state(struct translation_instance_t *ti, uint8_t *buf,
    size_t buf_size, src_ulong pc, struct regset_t const *live_in);*/

//extern uint8_t *code_gen_start;

#endif
