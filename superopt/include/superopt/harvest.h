#ifndef HARVEST_H
#define HARVEST_H
#include "support/src-defs.h"
#include "support/types.h"
#include "x64/insntypes.h"
#include <stdbool.h>
#include <string>
#include <vector>

#define MAX_DEBUG_INFO 409600
#define MAX_LOCALS_INFO 409600
#define MAX_INSN_LINE_MAP 409600

struct input_exec_t;
struct input_section_t;
struct src_insn_t;
struct dst_insn_t;
struct transmap_t;
struct regset_t;
struct regmap_t;
struct temporaries_t;
class nextpc_function_name_map_t;

void harvest_input_exec(struct input_exec_t *in,
    size_t len, bool functions_only, char const *function_name,
    //bool touch_callee_saves,
    bool fall_convention64, bool mem_live_out, bool canonicalize_imms,
    bool live_all, bool live_callee_save, bool trunc_ret,
    //bool sort_increasing, bool sort_by_occurrences,
    bool eliminate_duplicates,
    bool use_orig_regnames,
    std::string const& harvest_output_filename
    //void (*callback)(char const *, char const *, void *),
    //void *opaque
    );
size_t src_harvested_iseq_to_string(char *buf, size_t size, struct src_insn_t const *insns,
    long num_insns,
    struct regset_t const *live_out, long num_live_outs, bool mem_live_out);
//size_t dst_harvested_iseq_to_string(char *buf, size_t size, struct dst_insn_t const *insns,
//    long num_insns,
//    struct regset_t const *live_out, long num_live_outs, bool mem_live_out);
//bool read_harvested_iseq(FILE *fp, struct src_insn_t *src_iseq, long *src_iseq_len,
//    struct regset_t *live_regs,
//    long *num_live_outs);
size_t harvested_iseq_from_string(char const *harvested_iseq, struct src_insn_t *src_iseq,
    long src_iseq_size, long *src_iseq_len,
    struct regset_t *live_out, long live_out_size, size_t *num_live_out,
    bool *mem_live_out);

/*long fscanf_harvest_output_i386(FILE *fp, struct i386_insn_t *i386_iseq,
    size_t i386_iseq_size,
    size_t *i386_iseq_len, struct regset_t *live_out, size_t live_out_size,
    size_t *num_live_out, long linenum);*/
long read_harvest_output_dst(struct dst_insn_t **harvested_dst_iseqs,
    long *harvested_dst_iseq_lens, long harvested_dst_iseqs_size, char *tag,
    size_t tag_size, char const *filename);
long harvest_file_count_entries(char const *file);

char const *get_function_name_from_fdb_comment(char const *comment);
//void print_harvested_sequences(char const *ofile);
void gen_unified_harvest_log(std::vector<harvest_log_t> const& harvest_logs, std::string const& outname);
void gen_assembly_from_harvest_output(string const& asmresult, harvest_file_t const& harvest_result, harvest_log_t const& harvest_log);
bool harvest_get_next_dst_iseq(FILE *harvest_fp, dst_insn_t *dst_iseq, long *dst_iseq_len, regset_t *live_out, size_t *num_live_out, bool *mem_live_out, struct regmap_t& regmap, std::string& function_name, nextpc_function_name_map_t& nextpc_function_name_map, std::string& assembly, std::vector<dst_ulong>& insn_pcs, char *endline, long max_alloc);

#endif /* harvest.h */
