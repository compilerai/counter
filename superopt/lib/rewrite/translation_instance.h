#ifndef __TRANSLATION_INSTANCE_T
#define __TRANSLATION_INSTANCE_T

#include <map>
#include <string>
#include <vector>
#include "support/types.h"
#include "support/src_tag.h"
#define MAX_HELPER_SECTIONS 32
#define MAX_PSECTIONS 65536
#ifndef ELFIO_HPP
#include "valtag/elf/elf.h"
#endif
#include "valtag/symbol.h"
#include "cutils/rbtree.h"
#include "cutils/chash.h"
//#include "cache.h"
#include "cutils/large_int.h"
#include "rewrite/insn_line_map.h"
#include "rewrite/alloc_map.h"
#include "valtag/fixed_reg_mapping.h"
#include "rewrite/transmap.h"
#include "rewrite/bbl.h"
#include "rewrite/translation_unit.h"
#include "rewrite/prog_point.h"
#include "graph/graph.h"
#include "rewrite/tail_calls_set.h"

#define SRC_ISEQ_MAX_BRANCHES 2


struct fun_type_info_t;
struct interval_set;
struct bbl_t;
struct edge_t;

namespace eqspace {
class tfg;
class graph_symbol_t;
class symbol_or_section_id_t;
}

typedef struct input_section_t {
  int orig_index;
  char *name;
  Elf32_Addr addr;
  Elf32_Word type;
  Elf32_Word flags;
  Elf32_Word size;
  Elf32_Word info;
  Elf32_Word align;
  Elf32_Word entry_size;
  char *data;
} input_section_t;

typedef struct helper_section_t {
  char *name;
  char *data;
  Elf32_Addr addr;
  Elf32_Word size;
  int type;
  int flags;
} helper_section_t;


typedef struct reloc_t {
  Elf64_Addr offset;
  Elf64_Addr symbolValue;
  char symbolName[MAX_SYMNAME_LEN];
  char sectionName[MAX_SYMNAME_LEN];
  Elf_Word type;
  Elf_Sxword addend;
  Elf_Sxword calcValue;
  uint32_t orig_contents;
} reloc_t;

typedef struct dyn_entry_t {
  Elf_Xword tag;
  Elf_Xword value;
  char name[MAX_SYMNAME_LEN];
} dyn_entry_t;

#define MAX_DYN_ENTRIES 32

struct jumptable_t;
struct src_insn_t;

typedef
struct lock_t {
  src_ulong pc;
  long depth;
} lock_t;

typedef
struct fetch_branch_t {
  struct edge_t *edges;
  long depth;
  bool locked;
  struct lock_t *locks;
  long num_locks;
  double weight;
} fetch_branch_t;

typedef
struct si_iseq_t {
  fetch_branch_t *paths;
  long num_paths;

  struct src_insn_t *iseq;  // min_iseq with single exit
  long iseq_len;
  src_ulong *pcs;
  large_int_t iseq_fetchlen;

  struct edge_t *out_edges;
  double *out_edge_weights;
  long num_out_edges;
  long num_nextpcs; /* derived field for efficiency of src_iseq_fetch(). */

  long num_function_calls;
  nextpc_t *function_calls;

  nextpc_t fallthrough_pc;

  bool first;
} si_iseq_t;

#define SI_NUM_CACHED 1
typedef
struct si_entry_t {
  src_ulong pc;
  long index;

  struct bbl_t const *bbl;
  long bblindex;

  si_iseq_t max_first;
  si_iseq_t max_middle;

  si_iseq_t cached[SI_NUM_CACHED];
  bool eliminate_branch_to_fallthrough_pc;

  struct src_insn_t *fetched_insn;
  unsigned long fetched_insn_size;

  uint8_t *tc_ptr; /* for use at code generation time. */
} si_entry_t;

struct bbl_ptr_t {
  bbl_t const *m_bbl;
  bbl_ptr_t(bbl_t const *bbl) : m_bbl(bbl) {}
  string to_string() const { NOT_IMPLEMENTED(); }
  bool operator<(bbl_ptr_t const &other) const { return this->m_bbl->pc < other.m_bbl->pc; }
  bool operator==(bbl_ptr_t const &other) const { return this->m_bbl->pc == other.m_bbl->pc; }
};

namespace std
{
using namespace eqspace;
template<>
struct hash<bbl_ptr_t>
{
  std::size_t operator()(bbl_ptr_t const &b) const
  {
    return (size_t)b.m_bbl->pc;
  }
};
};


typedef struct input_exec_t {
private:
#if ARCH_SRC == ARCH_ETFG
  set<src_ulong> m_phi_edge_incident_pcs;
  set<src_ulong> m_executable_pcs;
#endif
public:
  char const *filename;
  int num_input_sections;
  int num_plt_sections;
  input_section_t input_sections[MAX_PSECTIONS];
  input_section_t plt_sections[MAX_PSECTIONS];
  bool has_dynamic_section;
  input_section_t dynamic_section;

  bool fresh;

  struct fun_type_info_t *fun_type_info;
  int num_fun_type_info;

  std::map<symbol_id_t, symbol_t> symbol_map;
  //symbol_t *symbols;
  //int num_symbols;

  std::map<symbol_id_t, symbol_t> dynsym_map;
  //symbol_t *dynsyms;
  //int num_dynsyms;

  reloc_t *relocs;
  int num_relocs;

  dyn_entry_t *dyn_entries;
  int num_dyn_entries;

  unsigned char type;
  Elf32_Addr entry;

  /* derived structures. */
  struct rbtree bbl_leaders;
  //struct rbtree function_entries;
  struct chash bbl_map;

  struct bbl_t *bbls;
  long num_bbls;
  //struct cache tb_cache;
  //struct cache insn_cache;


  long *dominators;
  long *post_dominators;
  struct jumptable_t *jumptable;
  long *innermost_natural_loop; //stores a map from each bbl-index to the (bbl index of the) head of its innermost natural loop. For the head node itself, this stores the (bbl index) of the head of the outer loop (instead of itself); thus one can determine the loop hierarchy by walking this structure. For a bbl-index that belongs to no natural loop, this maps to -1

  struct chash insn_boundaries;

  /* fields for src_iseq_fetch. */
  struct si_entry_t *si;
  unsigned long num_si;
  //struct locals_info_t **locals_info;

  struct insn_line_map_t insn_line_map;

  struct chash pc2index_map;

  /* fields for rodata ptrs; used by harvest for eqchecker. */
  src_ulong *rodata_ptrs;
  unsigned long num_rodata_ptrs;
  size_t rodata_ptrs_size;

  vector<translation_unit_t> get_translation_units() const;
  size_t get_translation_unit_start_si(translation_unit_t const &tunit) const;
  size_t get_translation_unit_stop_si(translation_unit_t const &tunit) const;
  bool pc_is_etfg_function_entry(src_ulong pc, string &function_name) const;
  string pc_get_function_name(src_ulong pc) const;
  eqspace::graph<bbl_ptr_t, eqspace::node<bbl_ptr_t>, eqspace::edge<bbl_ptr_t>> get_cfg_as_graph() const;
  eqspace::graph<bbl_ptr_t, eqspace::node<bbl_ptr_t>, eqspace::edge<bbl_ptr_t>> get_fcalls_as_graph() const;
  map<long, set<long>> identify_natural_loops() const;
  //size_t get_symbol_size(char const* symbol_name) const;
  align_t get_symbol_alignment(symbol_id_t symbol_id) const;
  bool symbol_is_rodata(symbol_t const& s) const;
  graph_symbol_t get_graph_symbol_for_symbol_or_section_id(symbol_or_section_id_t const& ssid) const;
  int get_section_index_for_orig_index(int orig_index) const;

#if ARCH_SRC == ARCH_ETFG
  void populate_phi_edge_incident_pcs();
  void populate_executable_pcs();
  set<src_ulong> const &get_phi_edge_incident_pcs() const { return m_phi_edge_incident_pcs; }
  set<src_ulong> const &get_executable_pcs() const { return m_executable_pcs; }
#endif

private:
  set<pair<long, long>> identify_backedges() const;
  set<long> identify_bbls_that_reach_tail_without_going_through_head(long tail, long head) const;
  set<long> identify_bbls_that_reach_tail_without_going_through_stop_bbls(long tail, set<long> const &stop_bbls) const;
} input_exec_t;

#define REL_SECTION_MYTEXT 1
#define REL_SECTION_JT_STUBCODE 2
typedef struct generated_reloc_t {
  uint32_t symbol_index;
  uint32_t symbol_loc;
  //int32_t symbol_value;
  uint32_t section_name_index;
  int type;
} generated_reloc_t;

typedef struct generated_symbol_t {
  uint32_t symbol_index;
  int32_t symbol_value;
  long size;
  unsigned char bind;
  unsigned char type;
  uint32_t section_name_index;
  unsigned char other;
} generated_symbol_t;

typedef struct output_exec_t {
  char const *name;
  int num_helper_sections;
  helper_section_t helper_sections[MAX_HELPER_SECTIONS];

  helper_section_t env_section;

  generated_symbol_t *generated_symbols;
  long generated_symbols_size;
  long num_generated_symbols;

  generated_reloc_t *generated_relocs;
  long generated_relocs_size;
  long num_generated_relocs;
  //dyn_entry_t dyn_entries[MAX_DYN_ENTRIES];
  //int num_dyn_entries;
} output_exec_t;

typedef enum translation_mode_t {
  TRANSLATION_MODE0 = -1,
  FBGEN,
  //SUBSETTING,
  UNROLL_LOOPS,
  SUPEROPTIMIZE,
  TRANSLATION_MODE_END,
  END,
  ANALYZE_STACK,
} translation_mode_t;

struct peep_entry_t;
struct transmap_set_t;
struct pred_t;

typedef struct peep_entry_t peep_entry_t;

typedef
struct peep_table_t {
  //struct peep_entry_t *peep_entries;
  //long peep_entries_size;
  long num_peep_entries;

  struct peep_entry_list_t *hash;
  long hash_size;

  struct rbtree rbtree;
} peep_table_t;

typedef
struct translation_instance_t {
private:
  int m_base_reg;
  //size_t m_memslot_size;
  fixed_reg_mapping_t m_dst_fixed_reg_mapping;
  //bool m_perform_dst_insn_folding;
  vector<string> m_disable_dst_insn_folding_functions;
  tail_calls_set_t m_tail_calls_set;
#if ARCH_SRC == ARCH_ETFG
  map<unsigned, shared_ptr<state const>> m_function_id_to_input_state_map;
#endif
public:

  translation_instance_t(fixed_reg_mapping_t const &dst_fixed_reg_mapping, vector<string> const& disable_dst_insn_folding_functions = {}) : m_dst_fixed_reg_mapping(dst_fixed_reg_mapping), m_disable_dst_insn_folding_functions(disable_dst_insn_folding_functions)
  { }

  fixed_reg_mapping_t const &ti_get_dst_fixed_reg_mapping() const { return m_dst_fixed_reg_mapping; }
  size_t dst_iseq_disassemble_considering_nextpc_symbols(dst_insn_t *dst_iseq, size_t dst_iseq_size) const;

  input_exec_t *in;
  output_exec_t *out;

#if ARCH_SRC == ARCH_ETFG
  state const* ti_get_input_state_at_etfg_pc(src_ulong pc);
#endif

  std::map<std::string, std::vector<std::string>> function_arg_names; //function_name->arg names
  std::map<std::string, std::map<prog_point_t, alloc_map_t<stack_offset_t>>> stack_slot_map_for_function; //function name->(pc->stack slot map)
  //std::map<std::string, size_t> stack_space_for_function; //function name->stack space size (in bytes)

  char *reloc_strings;
  long reloc_strings_size;

  peep_table_t peep_table;

  char *superoptdb_server;

  struct pred_t **pred_set;

  struct transmap_set_t *transmap_sets;
  struct transmap_set_elem_t **transmap_decision;
  struct transmap_set_elem_t *transmap_set_elem_pruned_out_list; //maintain this list to avoid dangling pointers through "decision" field in transmap_set_elem_t

  struct transmap_t *peep_in_tmap;
  struct transmap_t **peep_out_tmaps;
  struct regcons_t *peep_regcons;
  struct regset_t *peep_temp_regs_used;
  struct nomatch_pairs_set_t *peep_nomatch_pairs_set;
  struct regcount_t *touch_not_live_in;
  std::map<src_ulong, std::vector<transmap_t>> fixed_transmaps;

  uint8_t **bbl_headers;
  long *bbl_header_sizes;

  bool *insn_eliminated;

  translation_mode_t mode;

  uint8_t *jumptable_stub_code_buf;
  uint8_t *jumptable_stub_code_ptr;
  int *jumptable_stub_code_offsets;
  int num_jumptable_stub_code_offsets;

  uint8_t *code_gen_buffer;
  struct chash tcodes;
  size_t codesize;

  //struct chash eip_map;
  struct chash indir_eip_map;

  int get_base_reg() const { return m_base_reg; }
  void set_base_reg(int base_reg) { m_base_reg = base_reg; }
  //bool ti_get_perform_dst_insn_folding() const { return m_perform_dst_insn_folding; }
  //vector<string> const& ti_get_disable_dst_insn_folding_functions() const { return m_dst_insn_folding_functions; }
  bool ti_need_to_perform_dst_insn_folding_optimization_on_function(string const& fname) const;
  //void ti_set_perform_dst_insn_folding(bool b) { m_perform_dst_insn_folding = b; }
  void ti_set_disable_dst_insn_folding_functions(vector<string> const& func_list) { m_disable_dst_insn_folding_functions = func_list; }
  void ti_identify_tail_calls();
  bool pc_is_tail_call(src_ulong pc, string &caller_name) const;
  void dst_iseq_convert_malloc_to_mymalloc(dst_insn_t *iseq, size_t iseq_len);
  //size_t get_memslot_size() const { return m_memslot_size; }
  //void set_memslot_size(size_t s) { m_memslot_size = s; }
} translation_instance_t;


void usedef_sort(struct input_exec_t *in);

void translation_instance_init(translation_instance_t *ti, translation_mode_t mode);
void translation_instance_free(translation_instance_t *ti);

void etfg_read_input_file(input_exec_t *in, std::map<std::string, shared_ptr<eqspace::tfg>> const &function_tfg_map);
void etfg_input_file_add_symbols_for_nextpcs(input_exec_t *in, size_t const* nextpc_idx, size_t num_live_out);

void read_input_file(input_exec_t *in, char const *filename);
void init_input_file(input_exec_t *in, char const *function_name);
void input_exec_free(input_exec_t *in);
bool input_exec_addr_belongs_to_input_section(input_exec_t const *in, uint64_t addr);
bool input_exec_addr_belongs_to_rodata_section(input_exec_t const *in, src_ulong val);
void input_exec_add_rodata_ptr(input_exec_t *in, src_ulong val);
struct input_section_t const *input_exec_get_rodata_section(input_exec_t const *in);
char const *input_section_get_ptr_from_addr(input_section_t const *sec, src_ulong addr);
void input_file_annotate_using_ddsum(input_exec_t *in, char const *ddsum);
int input_exec_get_retsize_for_function(input_exec_t const *in, src_ulong pc);
int input_exec_function_get_num_arg_words(input_exec_t const *in, src_ulong pcrel_val);

int input_exec_get_section_index(input_exec_t const *in, char const *name);

bool is_possible_nip(input_exec_t const *in, src_ulong nip);
bool pc_is_executable(input_exec_t const *in, src_ulong pc);
char const *translation_mode_to_string(translation_mode_t mode, char *buf, int size);
//void tb_cache_init(struct cache *);
//void tb_cache_free(struct cache *);

#if 0
void fix_tb_pcrels(translation_instance_t *ti, struct bbl_t *bbl);
void tb_alloc_offsets(struct tb_t *tb, int n);
void tb_free_offsets(struct tb_t *tb);
#endif
translation_instance_t &get_ti_fallback();
bool pc_is_a_nextpc_symbol(std::map<symbol_id_t, symbol_t> const& symbol_map, src_ulong pc);

#endif
