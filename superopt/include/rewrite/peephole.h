#ifndef __PEEPHOLE_H
#define __PEEPHOLE_H
#include <stdbool.h>
#include "cutils/rbtree.h"
#include "support/types.h"
#include "valtag/regcount.h"
#include "rewrite/nomatch_pairs.h"
#include "rewrite/nomatch_pairs_set.h"
#include "rewrite/regconst_map_possibilities.h"
#include "superopt/peep_type_cons.h"
#include "insn/insn_line_map.h"
#include "valtag/fixed_reg_mapping.h"
#include "valtag/regset.h"
#include "valtag/memset.h"

#define MAX_PPC_ILEN 3
//#define MAX_X86_ILEN 1024

//#define RELOC_STRINGS_START 0x200
//#define RELOC_STRINGS_MAX_LEN 0x10000
/* VCONSTANT_PLACEHOLDER & 0xff should be greater than SRC_NUM_REGS : XXX: why? */
/* VCONSTANT_PLACEHOLDER should not be a multiple of 4 (to avoid ambiguity
   with reg offsets to SRC_ENV_ADDR) */
/* VCONSTANT_PLACEHOLDER & 0xff should not conflict with a constant used
   in peep.trans.tab to avoid ambiguity while parsing peep.trans.tab. */
/* VCONSTANT_PLACEHOLDER & 0xff should be a valid shift operand */
//#define VCONSTANT_PLACEHOLDER 0x87654311

#define MAX_NUM_SI 500000
#define MAX_ISEQ_BLOWUP 100


struct translation_instance_t;
struct transmap_set_row_t;
struct regcons_t;
struct input_exec_t;
struct typestate_t;
struct bbl_t;
struct transmap_t;
struct regset_t;
struct regmap_t;
struct peep_entry_t;
struct transmap_set_t;
struct pred_transmap_set_t;
struct src_insn_t;
struct regset_t;
struct vconstants_t;
struct src_insn_t;
struct i386_insn_t;
struct si_entry_t;
struct peep_table_t;
struct regcount_t;
struct itable_t;
struct symbol_map_t;
struct nextpc_function_name_map_t;
class rand_states_t;
class nextpc_t;
struct memset_t;

typedef enum {
  REGVAR_INPUT = 1,
  REGVAR_TEMP = 2,
} regvar_t;


#define PEEP_ENTRY_FLAGS_BASE_ONLY 0x00000001
#define PEEP_ENTRY_FLAGS_FRESH     0x00000002
#define PEEP_ENTRY_FLAGS_NOT_FRESH 0x00000004


void peep_table_init(struct peep_table_t *tab);
void peep_table_load(struct translation_instance_t *ti);
void peephole_table_free(struct peep_table_t *peep_table);
void peeptab_free_insn_line_maps(struct peep_table_t *peep_table);
/*void test_peephole_table(struct peep_table_t const *peep_table,
    bool (*check_equivalence)(struct src_insn_t const *, int, struct transmap_t const *,
      struct transmap_t const *, struct i386_insn_t const *,
      int, struct regset_t const *, int, long));*/

struct transmap_set_elem_t *peep_enumerate_default_transmap(void);
/*struct transmap_set_elem_t *peep_enumerate_fallback_transmap(
    struct transmap_set_elem_t *list);*/

//struct transmap_set_elem_t *peep_search_transmaps(
//    struct translation_instance_t const *ti,
//    struct si_entry_t const *si, struct src_insn_t const *src_iseq, long src_iseq_len,
//    long fetchlen, struct peep_table_t const *peep_table,
//    struct regset_t const *live_outs, long num_live_outs,
//    struct transmap_set_elem_t *list);

struct transmap_set_elem_t *peep_enumerate_transmaps(
    struct translation_instance_t const *ti,
    struct si_entry_t const *si, struct src_insn_t const *src_iseq, long src_iseq_len,
    long fetchlen,
    struct regset_t const *live_outs, long num_live_outs,
    struct peep_table_t const *peep_table,
    struct pred_transmap_set_t **pred_transmap_sets, long num_pred_transmap_sets,
    struct transmap_set_elem_t *list);

struct translation_instance_t;
void peep_translate(struct translation_instance_t const *ti,
    struct peep_entry_t const *peeptab_entry_id,
    struct src_insn_t const *src_iseq,
    long src_iseq_len, nextpc_t const *nextpcs,/* src_ulong const *insn_pcs,*/
    struct regset_t const *live_outs,
    long num_nextpcs,
    struct transmap_t const *in_tmap, struct transmap_t *out_tmaps,
    struct transmap_t *peep_in_tmap, struct transmap_t *peep_out_tmaps,
    struct regcons_t *peep_regcons,
    struct regset_t *peep_temp_regs_used,
    struct nomatch_pairs_set_t *peep_nomatch_pairs_set,
    struct regcount_t *touch_not_live_in,
    struct dst_insn_t *dst_iseq, long *dst_iseq_len, long dst_iseq_size
    //, bool var
    );

/*void peep_substitute(struct translation_instance_t const *ti,
    struct src_insn_t const *src_iseq,
    long src_iseq_len,
    struct regset_t const *live_outs,
    long num_nextpcs,
    struct transmap_t const *in_tmap, struct transmap_t *out_tmaps,
    struct i386_insn_t *i386_iseq, long *i386_iseq_len, long i386_iseq_size);*/


struct temporaries_t;
//extern char const *peep_trans_table;
void tb_append_unconditional_jump(struct bbl_t *tb);
void regmap_invert(struct regmap_t *inverted, struct regmap_t const *orig);
//long fscanf_src_assembly(FILE *fp, char *src_assembly, size_t src_assembly_size);
/*long fscanf_live_regs_strings(FILE *fp, char **live_regs_strings,
    size_t num_live_regs_strings, long *n);*/
/* bool peep_tab_has_base_translation_entry(struct peep_table_t const *tab,
    struct src_insn_t const *insn, struct transmap_t *in_tmap,
    struct transmap_t *out_tmap); */
void transmap_set_row_init(struct transmap_set_row_t **row, src_ulong pc, int len,
    src_ulong const *lastpcs, int num_lastpcs);
void obtain_nextpc_and_live_outs(struct input_exec_t const *in, src_ulong const *lastpcs,
    int num_lastpcs, nextpc_t *nextpcs, struct regset_t const *live_out0,
    struct regset_t const *live_out1);
struct transmap_set_elem_t *prune_transmaps(struct transmap_set_elem_t *ls,
    long max_size, transmap_set_elem_t *&pruned_out_list);
bool peep_entry_exists_for_prefix_insns(struct peep_table_t const *tab,
    struct src_insn_t const *iseq, long iseq_len);
void peeptab_try_and_add(struct peep_table_t *tab, struct src_insn_t const *src_iseq,
    long src_iseq_len,
    //regset_t const &flexible_regs,
    struct transmap_t const *transmap, struct transmap_t const *out_transmap,
    struct dst_insn_t const *dst_iseq, long dst_iseq_len,
    struct regset_t const *live_out, long num_live_out, bool mem_live_out,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    //struct symbol_map_t const *src_symbol_map, struct symbol_map_t const *dst_symbol_map,
    //struct locals_info_t *src_locals_info, struct locals_info_t *dst_locals_info,
    //struct insn_line_map_t const *src_iline_map, struct insn_line_map_t const *dst_iline_map,
    //struct nextpc_function_name_map_t const *nextpc_function_name_map,
    long linenum);
void peeptab_write(struct translation_instance_t *ti, struct peep_table_t const *tab,
    char const *file);


void peep_preprocess_all_constants(struct translation_instance_t *ti, char *line,
    struct vconstants_t *vconstants, int num_vconstants);
/*bool peep_preprocess_vregs(char *line, struct regmap_t const *regmap,
    void (*regname_suffix)(long regno, char const *suffix, char *name,
        size_t name_size));*/
/*void peep_preprocess_tregs(char *line, struct temporaries_t const *temporaries,
    bool *used_regs,
    long num_regs,
    int (*assign_unused_reg)(long vreg, regvar_t rvt, char const *suffix,
        bool *used, long num_regs),
    void (*regname_suffix)(long regno, char const *suffix, char *name,
        size_t name_size));*/
void peep_preprocess_scan_label(char *line, char const *label, long val);
void transmap_temporaries_remove_unnecessary_mappings(struct transmap_t *transmap,
    struct temporaries_t *temporaries, char const *i386_assembly);
bool peep_preprocess_exvregs(char *line, struct regmap_t const *regmap,
    char *(*exregname)(int, int, char const *, char *, size_t));
/*void peep_preprocess_extregs(char *line, struct temporaries_t const *temps,
    char *(*exregname)(int, int, char *, size_t));*/
void compute_nextpc_is_indir(struct src_insn_t const *src_iseq, long src_iseq_len,
    bool *nextpc_is_indir, long num_live_outs);
struct transmap_t const *find_out_tmap_for_nextpc(struct transmap_set_elem_t const *iter,
    src_ulong pc);
int compute_nextpc_indir(struct src_insn_t const *src_iseq, long src_iseq_len,
    long num_live_outs);
void read_peep_trans_table(struct translation_instance_t *ti,
    struct peep_table_t *peep_table, char const *file, char const *tab);
void ti_add_peep_table_to_superoptdb(struct translation_instance_t *ti,
    bool add_dst_iseq);
void ti_remove_peep_table_from_superoptdb(struct translation_instance_t *ti);
void peeptab_entry_write(FILE *fp, struct peep_entry_t const *peep_entry,
    struct translation_instance_t const *ti);
void peephole_table_type_categorize(struct peep_table_t *peep_table,
    char const *outname, rand_states_t const &rand_states);
bool src_iseqs_equal(struct src_insn_t const *i1, struct src_insn_t const *i2,
    size_t n);
bool dst_iseqs_equal(struct dst_insn_t const *i1, struct dst_insn_t const *i2,
    size_t n);
void itables_print(FILE *fp, vector<itable_t> const &itables, bool include_details);
//void peep_entry_compute_typestate(struct typestate_t *typestate_initial,
//    struct typestate_t *typestate_final, struct peep_entry_t const *cur);
void emit_types_to_file(FILE *fp, struct typestate_t const *ts_init,
    struct typestate_t const *ts_fin, size_t num_states);
bool peep_preprocess_using_regmap(struct translation_instance_t *ti, char *line,
    struct regmap_t const *regmap, struct vconstants_t *vconstants,
    char * (*exregname_suffix_fn)(int, int, char const *, char *, size_t));
void read_default_peeptabs(translation_instance_t *ti);

typedef struct peep_entry_t {
private:
  regset_t m_use, m_touch;
  memset_t m_memuse, m_memdef;
  struct transmap_t *in_tmap;
  long peep_entry_line_number; //serves as an ID for the peep_entry, also handy for debugging
public:
  struct src_insn_t *src_iseq;
  long src_iseq_len;

  struct dst_insn_t *dst_iseq;
  long dst_iseq_len;

  struct regset_t *live_out;
  long num_live_outs;
  int nextpc_indir;

  bool mem_live_out;

  peep_type_cons_t peep_type_cons;

  //regcount_t touch_not_live_in;
  regset_t touch_not_live_in_regs;
  regset_t temp_regs_used;

  regconst_map_possibilities_t regconst_map_possibilities;
  //regset_t flexible_used_regs;

  //int num_nomatch_pairs;
  //nomatch_pairs_t *nomatch_pairs;
  nomatch_pairs_set_t nomatch_pairs_set;

  struct transmap_t *out_tmap;
  struct regcons_t *in_tcons;

  fixed_reg_mapping_t dst_fixed_reg_mapping; //maps machine registers to their (potentially canonicalized) names occurring in the dst_iseq. e.g., DST_EXREG_GROUP_GPRS, DST_STACK_REGNUM -> 1

  //struct symbol_map_t *src_symbol_map;
  //struct symbol_map_t *dst_symbol_map;

  //struct locals_info_t *src_locals_info;
  //struct locals_info_t *dst_locals_info;

  //struct insn_line_map_t src_iline_map;
  //struct insn_line_map_t dst_iline_map;

  //struct nextpc_function_name_map_t *nextpc_function_name_map;

  //uint32_t peep_flags;
  long cost;

  /* for hash-chaining */
  //struct peep_entry_t *next;

  /* for rbtree index */
  struct rbtree_elem rb_elem;


  void populate_usedef();
  transmap_t const *get_in_tmap() const { return in_tmap; }
  void set_in_tmap(transmap_t *tmap) { in_tmap = tmap; }
  long peep_entry_get_line_number() const { return peep_entry_line_number; }
  void peep_entry_set_line_number(long l) { peep_entry_line_number = l; }
  void free_in_tmap();
  regset_t const& get_use() const { return m_use; }
  regset_t const& get_touch() const { return m_touch; }
  memset_t const& get_memuse() const { return m_memuse; }
  memset_t const& get_memdef() const { return m_memdef; }
  //transmap_t const *get_union_tmap() const { return union_tmap; }
  //void set_union_tmap(transmap_t *tmap) { union_tmap = tmap; }
  //void free_union_tmap();
} peep_entry_t;

void print_peephole_table(struct peep_table_t const *tab);
long fscanf_entry_section(FILE *fp, char *src_assembly, size_t src_assembly_size);
bool mem_live_out_from_string(char const *buf);
long fscanf_live_regs_strings(FILE *fp, char **live_regs_string,
    size_t num_live_regs_strings, size_t *n);
int fscanf_mem_live_string(FILE *fp, char *buf, size_t buf_size);
size_t src_harvested_iseq_to_string(char *buf, size_t size, src_insn_t const *insns,
    long num_insns,
    regset_t const *live_out, long num_live_out, bool mem_live_out);


#endif
