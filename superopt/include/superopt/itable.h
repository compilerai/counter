#ifndef ITABLE_H
#define ITABLE_H

#include "support/src-defs.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "insn/dst-insn.h"
#include "superopt/typestate.h"
#include "superopt/typesystem.h"
#include "support/consts.h"

#define MAX_ITABLE_SIZE 30000

#define ITAB_PRUNE_MEM 1
#define ITAB_PRUNE_VCONST 2
#define ITAB_PRUNE_EXREG(i) (20 + i)

#define ITABLE_ENTRY_ISEQ_MAX_LEN 512
struct itable_position_t;

typedef struct itable_entry_t {
private:
  string assembly;
  vector<dst_insn_t> iseq;
  vector<dst_insn_t> iseq_ft;
  long long cost[MAX_I386_COSTFNS];
public:
  long exreg_sequence_len[I386_NUM_EXREG_GROUPS];
  long vconst_sequence_len;
  long memloc_sequence_len;
  long nextpc_sequence_len;

  itable_entry_t(string const &in_assembly, dst_insn_t *template_insn, long iseq_len)
  {
    this->assembly = in_assembly;
    for (long i = 0; i < iseq_len; i++) {
      static char as1[4096];
      this->iseq.push_back(template_insn[i]);
    }
    this->iseq_ft = this->iseq;
    dst_insn_vec_convert_last_nextpc_to_fallthrough(this->iseq_ft);
    for (long i = 0; i < dst_num_costfns; i++) {
      this->cost[i] = dst_iseq_compute_cost(template_insn, iseq_len, dst_costfns[i]);
    }
  }
  vector<dst_insn_t> const &get_iseq_ft() const { return iseq_ft; } //fallthrough iseq
  vector<dst_insn_t> const &get_iseq() const { return iseq; }
  string const &get_assembly() const { return assembly; }
  long long get_cost0() const { return cost[0]; }
  long long get_cost(int c) const { return cost[c]; }
  static bool itable_entry_cost_cmp(itable_entry_t const &a, itable_entry_t const &b);
  static int itable_eliminate_dups_cmp(const void *_a, const void *_b);
} itable_entry_t;

class itable_characteristics_t {
private:
  size_t num_nextpcs;
  size_t num_memlocs;
  size_t num_regs_used[I386_NUM_EXREG_GROUPS], num_vconsts_used;
  //size_t num_temps[I386_NUM_EXREG_GROUPS];
  //size_t num_temp_stack_slots;
  size_t num_memaccesses;
  size_t num_regs_used_in_memaccesses/*, num_vconsts_used_in_memaccesses*/;
  long nextpc_indir;

  //use_types_t use_types;
  typesystem_t m_typesystem;
public:
  itable_characteristics_t() {}
  typesystem_t const &get_typesystem() const { return m_typesystem; }
  typesystem_t &get_typesystem() { return m_typesystem; }
  size_t get_num_memlocs() const { return num_memlocs; }
  void set_num_memlocs(size_t n) { num_memlocs = n; }
  size_t get_num_nextpcs() const { return num_nextpcs; }
  void set_num_nextpcs(size_t n) { num_nextpcs = n; }
  void set_nextpc_indir(long in_nextpc_indir) { nextpc_indir = in_nextpc_indir; }
  long get_nextpc_indir() const { return nextpc_indir; }
  size_t const *get_num_regs_used() const { return num_regs_used; }
  size_t *get_num_regs_used() { return num_regs_used; }
  //use_types_t get_use_types() const { return use_types; }
  void set_typesystem(typesystem_t const &typesystem) { m_typesystem = typesystem; }
  //void set_use_types(use_types_t in_use_types) { use_types = in_use_types; }
  //void set_num_temp_stack_slots(size_t n) { num_temp_stack_slots = n; }
  //size_t get_num_temp_stack_slots() const { return num_temp_stack_slots; }
  void set_num_memaccesses(size_t n) { num_memaccesses = n; }
  size_t get_num_memaccesses() const { return num_memaccesses; }
  void set_num_regs_used(size_t in_num_regs_used[DST_NUM_EXREG_GROUPS]) { memcpy(num_regs_used, in_num_regs_used, sizeof num_regs_used); }
  //void set_num_temps(size_t in_num_temps[DST_NUM_EXREG_GROUPS]) { memcpy(num_temps, in_num_temps, sizeof num_temps); }
  //size_t const *get_num_temps() const { return num_temps; }
  //size_t *get_num_temps() { return num_temps; }
  void set_num_regs_used_in_memaccesses(size_t in_num_regs_used_in_memaccesses) { num_regs_used_in_memaccesses = in_num_regs_used_in_memaccesses; }
  size_t get_num_regs_used_in_memaccesses() const { return num_regs_used_in_memaccesses; }
  void set_num_vconsts_used(size_t in_num_vconsts_used) { num_vconsts_used = in_num_vconsts_used; }
  size_t get_num_vconsts_used() const { return num_vconsts_used; }
  regset_t get_dst_relevant_regs() const;
  string itable_characteristics_to_string() const;
  static itable_characteristics_t itable_characteristics_from_stream(FILE *fp);
};

typedef struct itable_t {
private:
  itable_characteristics_t m_characteristics;
  vector<itable_entry_t> entries;
  fixed_reg_mapping_t const &m_fixed_reg_mapping;
public:
  itable_t() : m_fixed_reg_mapping(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_at_end())
  {
  }
  //size_t num_entries, entries_size;

  itable_t(itable_t const &other) : m_characteristics(other.m_characteristics),
                                    entries(other.entries),
                                    m_fixed_reg_mapping(other.m_fixed_reg_mapping)
  {
  }
  void operator=(itable_t const &other)
  {
    ASSERT(&m_fixed_reg_mapping == &other.m_fixed_reg_mapping);
    m_characteristics = other.m_characteristics;
    entries = other.entries;
  }
  fixed_reg_mapping_t const &get_fixed_reg_mapping() const { return m_fixed_reg_mapping; }
  void itable_remove_entries_using_remove_array(bool const *remove);
  void itable_save_insn(char const *iassembly);
  itable_entry_t const &get_entry(long n) const { return entries.at(n); }
  vector<itable_entry_t> const &get_entries() const { return entries; }
  itable_characteristics_t const &get_itable_characteristics() const { return m_characteristics; }
  void set_itable_characteristics(itable_characteristics_t const &itable_characteristics) { m_characteristics = itable_characteristics; }
  size_t get_num_entries() const { return entries.size(); }
  typesystem_t const &get_typesystem() const { return m_characteristics.get_typesystem(); }
  typesystem_t &get_typesystem() { return m_characteristics.get_typesystem(); }
  size_t get_num_memlocs() const { return m_characteristics.get_num_memlocs(); }
  void set_num_memlocs(size_t num_memlocs) { return m_characteristics.set_num_memlocs(num_memlocs); }
  size_t get_num_nextpcs() const { return m_characteristics.get_num_nextpcs(); }
  void set_num_nextpcs(size_t num_nextpcs) { return m_characteristics.set_num_nextpcs(num_nextpcs); }
  size_t const *get_num_regs_used() const { return m_characteristics.get_num_regs_used(); }
  size_t *get_num_regs_used() { return m_characteristics.get_num_regs_used(); }
  //use_types_t get_use_types() const { return m_characteristics.get_use_types(); }
  void set_typesystem(typesystem_t const &typesystem) { m_characteristics.set_typesystem(typesystem); }
  //void set_use_types(use_types_t use_types) { m_characteristics.set_use_types(use_types); }
  //void set_num_temp_stack_slots(size_t n) { m_characteristics.set_num_temp_stack_slots(n); }
  //size_t get_num_temp_stack_slots() const { return m_characteristics.get_num_temp_stack_slots(); }
  void set_num_memaccesses(size_t n) { m_characteristics.set_num_memaccesses(n); }
  size_t get_num_memaccesses() const { return m_characteristics.get_num_memaccesses(); }
  void set_num_regs_used(size_t num_regs_used[DST_NUM_EXREG_GROUPS]) { m_characteristics.set_num_regs_used(num_regs_used); }
  void set_num_vconsts_used(size_t num_vconsts_used) { m_characteristics.set_num_vconsts_used(num_vconsts_used); }
  size_t get_num_vconsts_used() const { return m_characteristics.get_num_vconsts_used(); }
  void set_num_regs_used_in_memaccesses(size_t num_regs_used_in_memaccesses) { m_characteristics.set_num_regs_used_in_memaccesses(num_regs_used_in_memaccesses); }
  size_t get_num_regs_used_in_memaccesses() const { return m_characteristics.get_num_regs_used_in_memaccesses(); }
  void set_nextpc_indir(long in_nextpc_indir) { m_characteristics.set_nextpc_indir(in_nextpc_indir); }
  long get_nextpc_indir() const { return m_characteristics.get_nextpc_indir(); }
  //void set_num_temps(size_t num_temps[I386_NUM_EXREG_GROUPS]) { m_characteristics.set_num_temps(num_temps); }
  //size_t const *get_num_temps() const { return m_characteristics.get_num_temps(); }
  //size_t *get_num_temps() { return m_characteristics.get_num_temps(); }
  void itable_sort();
  void itable_eliminate_equivalent_instructions();
  regset_t itable_get_dst_relevant_regs() const;
} itable_t;

bool itables_equal(itable_t const *a, itable_t const *b);

char *itable_to_string(itable_t const *itable, char *buf, size_t size);
itable_t itable_from_stream(FILE *fp);
itable_t itable_from_string(char const *string);
//void itable_print(itable_t const *itable);

int itable_compute_cost_for_sequence(struct itable_t const *itable,
    long const *sequence, size_t sequence_len);
char *
itable_sequence_array_to_string(char *buf, size_t size, long const *sequence,
    size_t sequence_len);
size_t itable_sequence_array_from_string(long *sequence, size_t size, char const *buf);
int itable_sequence_array_cmp(long const *a, size_t alen, long const *b, size_t blen);
//void itable_init(itable_t *itable);
//void itable_free(itable_t *itable);

/*long itable_position_next(struct itable_position_t *ipos, struct itable_t const *itable,
    long long const *max_costs, long cur);*/

//void itable_remove_entries_using_remove_array(itable_t *itab, bool const *remove);
void itable_add_insn(itable_t *itable, struct dst_insn_t const *insn);
void itable_add_insn_vec(itable_t *itable, vector<dst_insn_t> const &insn_vec);
void read_itables_from_file(string const &filename, vector<itable_position_t> &ipositions, vector<itable_t> &itables);

#endif
