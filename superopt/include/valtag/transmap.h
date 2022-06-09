#ifndef __TRANSMAP_H
#define __TRANSMAP_H

#include <stdio.h>
#include <stdint.h>
#include "support/types.h"
#include "support/consts.h"
#include "support/src-defs.h"
#include "ppc/regs.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "x64/insn.h"
#include "etfg/etfg_regs.h"
#include "valtag/regset.h"
#include "valtag/transmap_loc.h"
//#include "tfg/state.h"
#include "rewrite/alloc_map.h"
#include <vector>
#include <set>
#include <list>

struct regset_t;
struct regmap_t;
struct regcons_t;
struct transmap_t;
struct temporaries_t;
struct live_ranges_t;
struct transmap_entry_t;
struct dst_insn_t;
struct regcount_t;
class memslot_map_t;
// forward declaration
namespace eqspace {
  class state;
}

class exreg_id_obj_t
{
public:
  exreg_id_t id;
  exreg_id_obj_t(exreg_id_t e) : id(e) {}
  static exreg_id_obj_t get_default_object(size_t off) { return exreg_id_obj_t((exreg_id_t)off); }
  static exreg_id_obj_t get_invalid_object() { return exreg_id_obj_t(-1); }
  static exreg_id_obj_t from_string(string const &s)
  {
    return exreg_id_t(stol(s));
  }
  friend ostream &operator<<(ostream &out, exreg_id_obj_t const &e)
  {
    out << e.id;
    return out;
  }
  bool operator<(exreg_id_obj_t const &other) const { return this->id < other.id; }
  bool operator==(exreg_id_obj_t const &other) const { return this->id == other.id; }
};



void transmap_init(struct transmap_t **tmap_ptr);
void transmap_free(struct transmap_t *tmap);
void transmap_set_default(struct transmap_t *tmap);

void transmap_entry_copy(struct transmap_entry_t *dst,
    struct transmap_entry_t const *src);
void transmap_copy(struct transmap_t *dst, struct transmap_t const *src);
void transmaps_copy(struct transmap_t *dst, struct transmap_t const *src,
    size_t num_transmaps);
void transmap_check(struct transmap_t *tmap);

/*
int transmap_matches_template(struct transmap_t const *templ_tmap,
    struct transmap_t const *in_tmap);
 */
int transmap_matches_template_on_regs(struct transmap_t const *templ_tmap,
    struct transmap_t const *in_tmap, struct regset_t const *regs);

bool transmaps_equal(struct transmap_t const *tmap1, struct transmap_t const *tmap2);
bool transmaps_equal_mod_regnames(struct transmap_t const *tmap1, struct transmap_t const *tmap2);
bool transmap_arr_equal(struct transmap_t const *tmap1, struct transmap_t const *tmap2,
    size_t n);
bool transmaps_locs_equal(struct transmap_t const *tmap1,
    struct transmap_t const *tmap2);
bool transmaps_identical(struct transmap_t const *tmap1, struct transmap_t const *tmap2);
//bool transmap_mappings_are_identity(struct transmap_t const *tmap);

char const *transmap_get_memory_expression(struct transmap_t *tmap, int memno,
    char *retstr, int retstr_size);

long transmap_translation_cost(struct transmap_t const *tmap1,
    struct transmap_t const *tmap2/*, struct regset_t const *live_in*/, bool is_loop_edge_with_permuted_phi_assignment);

/*long transmap_translation_cost_opt(struct transmap_t const *tmap1,
    struct transmap_t const *tmap2,
    regset_t const *live_regs);*/

long transmap_translation_insns(struct transmap_t const *tmap1,
    struct transmap_t const *tmap2, eqspace::state const *start_state,
    /*struct regset_t const *live_regs, bool exec, */struct dst_insn_t *buf,
    size_t buf_size, int memloc_offset_reg, fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    i386_as_mode_t mode);

long transmap_translation_code(struct transmap_t const *tmap1,
    struct transmap_t const *tmap2,
    int memloc_offset_reg,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    //struct regset_t const *live_regs, bool exec,
    i386_as_mode_t mode,
    uint8_t *buf, size_t size);


void transmaps_acquiesce(struct transmap_t *dst, struct transmap_t const *src);

void transmap_alloc_xmm_regs(struct transmap_t *dst_in, struct transmap_t *dst_out,
    struct transmap_t const *src, struct regset_t const *live_regs,
    struct live_ranges_t const *live_ranges, struct regset_t const *use);

void print_transmap(struct transmap_t const *tmap);
long transmap_from_stream(struct transmap_t *tmap, FILE *fp);
char *transmap_to_string(struct transmap_t const *tmap, char *buf, size_t size);
char *transmaps_to_string(struct transmap_t const *in_tmap,
    struct transmap_t const *out_tmaps, size_t num_out_tmaps, char *buf, size_t size);
/*char *transmaps_with_flexible_regs_to_string(struct transmap_t const *in_tmap,
    struct transmap_t const *out_tmaps, size_t num_out_tmaps, regset_t const &flexible_regs,
    char *buf, size_t size);*/
/*char *transmap_locs_to_string(struct transmap_t const *tmap,
    char *buf, size_t buf_size); */
void fprintf_transmap(FILE *logfile, struct transmap_t const *tmap);
struct transmap_t const *transmap_none(void);
struct transmap_t const &transmap_env(void);
struct transmap_t const &transmap_env_dst(void);
struct transmap_t const *transmap_no_regs(void);
void transmap_init_env(struct transmap_t *tmap, long env_addr);
void transmap_init_env_dst(struct transmap_t *tmap, long env_addr);
void transmap_fill_no_regs_using_env_addr(struct transmap_t *tmap, long env_addr);
struct transmap_t i386_transmap_default(fixed_reg_mapping_t const &fixed_reg_mapping);
struct transmap_t x64_transmap_default(fixed_reg_mapping_t const &fixed_reg_mapping);
struct transmap_t ppc_transmap_default(fixed_reg_mapping_t const &fixed_reg_mapping);
struct transmap_t const *transmap_default(void);
//struct transmap_t const *transmap_exec(void);
struct transmap_t const *transmap_fallback(void);
unsigned long transmap_hash_func(struct transmap_t const *tmap);

/* For every mapping R-->r in in_regmap and r-->x in in_tmap, puts
   R-->x in out_tmap. Note that r may appear multiple times in in_regmap */
void transmap_inv_rename(struct transmap_t *out_tmap, struct transmap_t const *in_tmap,
    struct regmap_t const *in_regmap);

uint64_t transmap_compute_bitmap(struct transmap_t const *tmap);
uint64_t transmap_compute_dirty_bitmap(struct transmap_t const *tmap);
int transmap_bitmap_diff(uint64_t bmap1, uint64_t bmap2);
//int transmap_bitmap_agrees(struct transmap_t const *tmap, uint64_t bmap);

void transmap_get_temporaries(struct transmap_t const *tmap,
    struct temporaries_t *temps);

int transmap_count_regallocs(struct transmap_t const *tmap);

void transmap_apply_diff(struct transmap_t *tmap, struct transmap_t const *in_tmap,
    struct transmap_t const *out_tmap, struct regmap_t const *inv_regmap,
    struct regset_t const *def);

void transmap_copy_entry(struct transmap_t *dst, struct transmap_t const *src,
    int group, reg_identifier_t const &regnum);

void transmap_get_dirty_bits(struct transmap_t const *tmap, int *dirty);

int transmap_reg_mem_transfer_cost(struct transmap_t const *tmap1,
    struct transmap_t const *tmap2, struct regset_t const *live_in);
int transmap_flag_mem_transfer_cost(struct transmap_t const *tmap1,
    struct transmap_t const *tmap2, struct regset_t const *live_in);
int transmap_flag_reg_transfer_cost(struct transmap_t const *tmap1,
    struct transmap_t const *tmap2, struct regset_t const *live_in);
//size_t transmap_to_int_array(struct transmap_t const *tmap, int64_t arr[], size_t len);
void transmap_assign_regs_using_i386_regmap(struct transmap_t *out,
    struct transmap_t const *in, struct regmap_t const *i386_regmap);

bool transmap_create_mapping_with_unused_reg(struct transmap_t *tmap, long src_regno,
    char const *suffix, bool *used_regs, long num_regs);

bool
transmap_temporaries_assign_using_src_regcons(struct transmap_t *transmap,
    struct temporaries_t *temporaries, struct regcons_t const *tcons,
    char const *assembly);
void temporaries_assign_using_transmap_and_src_regcons(struct temporaries_t *temporaries,
    struct transmap_t const *transmap, struct regcons_t const *regcons);
long transmap_hash(struct transmap_t const *tmap);
void transmap_intersect_with_regset(struct transmap_t *tmap,
    struct regset_t const *regset);
void transmap_rename(struct transmap_t *out_tmap, struct transmap_t const *in_tmap,
    struct regmap_t const *in_regmap);
/*void transmaps_rename(struct transmap_t *in_tmap, struct transmap_t *out_tmaps,
    long num_out_tmaps, struct regmap_t const *regmap);*/
bool transmap_rename_using_dst_regmap(struct transmap_t *tmap,
    struct regmap_t const *regmap, memslot_map_t const *memslot_map);
bool transmaps_rename_using_dst_regmap(struct transmap_t *in_tmap,
    struct transmap_t *out_tmaps,
    long num_out_tmaps, struct regmap_t const *regmap,
    memslot_map_t const *memslot_map);
void transmap_unassign_regs(struct transmap_t *tmap);
//void transmap_assign_canonically(struct transmap_t *tmap);
void transmaps_canonicalize(transmap_t &in_tmap, transmap_t *out_tmaps, size_t num_out_tmaps);
void transmaps_diff(struct regmap_t *dst_regmap, struct transmap_t const *tmap1,
    struct transmap_t const *tmap2, regset_t const *in_live);
//void transmap_acquiesce_with_regcons(struct transmap_t *tmap,
//    struct transmap_t const *orig_tmap, struct regcons_t const *regcons, regset_t const *in_live);
void out_transmap_rename(transmap_t *out_tmap, transmap_t const *in_tmap,
    transmap_t const *orig_out_tmap, transmap_t const *orig_in_tmap,
    regcons_t const *regcons);
//void out_transmaps_rename(struct transmap_t *out_tmaps,
//    struct transmap_t const *in_tmap, struct transmap_t const *orig_out_tmaps,
//    struct transmap_t const *orig_in_tmap, long num_out_tmaps,
//    struct regcons_t const *regcons);
//void transmaps_assign_regs_where_unassigned(struct transmap_t *in_tmap,
//    struct transmap_t *out_tmaps, long num_out_tmaps,
//    struct regcount_t const *touch_not_live_in);
void transmaps_get_dst_regmap_using_src_regmap(struct regmap_t *dst,
    struct transmap_t const *in_tmap,
    struct transmap_t const *out_tmaps, long num_out_tmaps, struct regmap_t const *src);
//bool read_transmaps(FILE *fp, struct transmap_t *in_tmap, struct transmap_t *out_tmaps,
//    size_t num_out_tmaps);
size_t transmaps_from_string(struct transmap_t *transmap,
    struct transmap_t *out_transmap, long max_out_transmaps,
    char const *tmap_string, long *num_out_transmaps);
long fscanf_transmaps(FILE *fp, struct transmap_t *in_tmap,
    struct transmap_t *out_tmaps, long out_tmaps_size, long *num_out_tmaps/*,
    char *endline, size_t endline_size*/);
/*long fscanf_transmaps_with_flexible_regs(FILE *fp, struct transmap_t *in_tmap,
    struct transmap_t *out_tmaps, long out_tmaps_size, long *num_out_tmaps, struct regset_t &flexible_regs);*/

void transmap_get_used_dst_regs(transmap_t const *tmap, struct regset_t *used);
std::set<stack_offset_t> transmap_get_used_stack_slots(transmap_t const *tmap, alloc_map_t<stack_offset_t> const &ssm = alloc_map_t<stack_offset_t>());
regset_t transmaps_get_used_dst_regs(transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_out_tmaps);
bool transmaps_contain_mapping_to_loc(struct transmap_t const *in_tmap,
    struct transmap_t const *out_tmaps, long num_out_tmaps, int loc);
bool transmap_invert(struct transmap_t *out, struct transmap_t const *in);
//void transmap_identity_for_regset(struct transmap_t *out, struct regset_t const *regs);
transmap_entry_t const *transmap_get_elem(struct transmap_t const *tmap, exreg_group_id_t group, reg_identifier_t regnum);
alloc_map_t<stack_offset_t> transmap_assign_stack_slots_where_unassigned(transmap_t const *tmap);
void transmap_entry_copy(struct transmap_entry_t *dst, struct transmap_entry_t const *src);

/* debug functions */
void transmap_check_consistency(struct transmap_t const *tmap);

class memaddr_t;
class stack_offset_t;

typedef struct transmap_entry_t {
private:
  //unsigned num_locs;
  std::vector<transmap_loc_t> m_locs;
  //uint8_t loc[TMAP_MAX_NUM_LOCS];
  //dst_ulong regnum[TMAP_MAX_NUM_LOCS];
public:
  transmap_entry_t() /*: num_locs(0)*/ { clear(); }
  void clear()
  {
    m_locs.clear();
  }
  bool contains_mapping_to_loc(uint8_t l) const
  {
    /*for (unsigned i = 0; i < num_locs; i++) {
      if (loc[i] == l) {
        return true;
      }
    }
    return false;*/
    for (const auto &loc : m_locs) {
      if (loc.get_loc() == l) {
        return true;
      }
    }
    return false;
  }
  dst_ulong get_regnum_for_loc(uint8_t l) const
  {
    /*for (unsigned i = 0; i < num_locs; i++) {
      if (loc[i] == l) {
        return regnum[i];
      }
    }*/
    for (const auto &loc : m_locs) {
      if (loc.get_loc() == l) {
        if (l == TMAP_LOC_SYMBOL) {
          return loc.get_regnum();
        } else if (l >= TMAP_LOC_EXREG(0) && l < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
          NOT_REACHED();
          //return loc.get_reg_id().reg_identifier_get_id();
        } else NOT_REACHED();
      }
    }
    NOT_REACHED();
  }
  reg_identifier_t const &get_reg_id_for_loc(uint8_t l) const
  {
    /*for (unsigned i = 0; i < num_locs; i++) {
      if (loc[i] == l) {
        return regnum[i];
      }
    }*/
    for (const auto &loc : m_locs) {
      if (loc.get_loc() == l) {
        if (l == TMAP_LOC_SYMBOL) {
          NOT_REACHED();
        } else if (l >= TMAP_LOC_EXREG(0) && l < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
          return loc.get_reg_id();
          //return loc.get_reg_id().reg_identifier_get_id();
        } else NOT_REACHED();
      }
    }
    NOT_REACHED();
  }

  bool operator==(transmap_entry_t const &other) const;
  string transmap_entry_to_string(char const *prefix = "") const;
  int get_num_locs() const { return m_locs.size(); }
  std::vector<transmap_loc_t> const &get_locs() const { return m_locs; }
  transmap_loc_t const &get_loc(int l) const { return m_locs.at(l); }
  void set_to_exreg_loc(uint8_t loc, dst_ulong regnum, transmap_loc_modifier_t mod)
  {
    ASSERT(loc >= TMAP_LOC_EXREG(0) && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
    m_locs.clear();
    //transmap_loc_t l = { .tmap_loc = loc, .tmap_regnum = regnum };
    //m_locs.push_back(l);
    transmap_entry_add_exreg_loc(loc, regnum, mod);
  }
  void set_to_symbol_loc(uint8_t loc, dst_ulong regnum)
  {
    ASSERT(loc == TMAP_LOC_SYMBOL);
    m_locs.clear();
    //transmap_loc_t l = { .tmap_loc = loc, .tmap_regnum = regnum };
    //m_locs.push_back(l);
    transmap_entry_add_symbol_loc(loc, regnum);
  }
  void transmap_entry_add_symbol_loc(uint8_t loc, dst_ulong regnum)
  {
    ASSERT(loc == TMAP_LOC_SYMBOL);
    transmap_loc_symbol_t l(loc, regnum); // = { .tmap_loc = loc, .tmap_regnum = regnum };
    m_locs.push_back(l);
  }
  void transmap_entry_add_exreg_loc(uint8_t loc, dst_ulong regnum, transmap_loc_modifier_t mod)
  {
    ASSERT(loc >= TMAP_LOC_EXREG(0) && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
    transmap_loc_exreg_t l(loc, regnum, mod); // = { .tmap_loc = loc, .tmap_regnum = regnum };
    m_locs.push_back(l);
  }

  void transmap_entry_assign_stack_slots(alloc_map_t<stack_offset_t> const &ssm, size_t stack_frame_size, exreg_group_id_t group, reg_identifier_t const &reg_id);
  void transmap_entry_assign_regnames_for_group(alloc_map_t<exreg_id_obj_t> const &rm, exreg_group_id_t group, reg_identifier_t const &reg_id, exreg_group_id_t dst_group);
  void transmap_entry_unmark_dst_reg(int loc, int reg);
  void transmap_entries_acquiesce(transmap_entry_t const &other, transmap_t *dst_tmap);
  void transmap_entries_rename_union(transmap_entry_t const &other, memaddr_t const &memaddr);
  bool transmap_entries_cannot_match(transmap_entry_t const &other, dst_insn_t const *iseq, long iseq_len) const;
  void transmap_entry_get_used_dst_regs(struct regset_t *used) const;
  void transmap_entry_assign_reg_where_unassigned(regset_t const *used);
  void transmap_entry_get_used_stack_slots(std::set<stack_offset_t> &ret) const;
  bool transmap_entry_matches_template(transmap_entry_t const &templ) const;
  void transmap_entry_copy_symbol_entries(transmap_entry_t const &other);
  void transmap_entry_copy_exreg_entries_for_dst_regs(transmap_entry_t const &other, regset_t const *dst_regs);
  void transmap_entries_union(transmap_entry_t const *src1, transmap_entry_t const *src2);
  void transmap_entries_union(transmap_entry_t const *src2);
  bool transmap_entry_rename_using_dst_regmap(regmap_t const *regmap, memslot_map_t const *memslot_map);
  void transmap_entries_subtract(transmap_entry_t const *src1, transmap_entry_t const *src2);
  void transmap_entry_count_dst_regs(regcount_t *regcount) const;
  size_t transmap_entry_count_dst_regs_for_group(exreg_group_id_t group) const;
  bool transmap_entries_locs_equal(transmap_entry_t const &other) const;
  void regmap_add_constant_mappings_using_transmap_entry(regmap_t *dst, reg_identifier_t const &constant_reg_id) const;
  bool transmap_entries_equal_mod_regnames(transmap_entry_t const &tmap2e) const;
  size_t transmap_entry_to_int_array(int64_t arr[], size_t len) const;
  void transmap_entry_copy(transmap_entry_t const &other);
  void transmap_entry_unassign_regs();
  bool tmap_entry_delete_loc(int loc);
  bool tmap_entry_contains_loc(transmap_loc_t const &loc) const;
  void transmap_entry_assign_using_tmap_entry(
    transmap_entry_t const *src, transmap_t const *tmap,
    int src_reg_group, int src_regnum);
  void transmap_entry_set_locs(vector<transmap_loc_t> const &locs);
  void transmap_entry_update_modifiers_using_peep_entry(transmap_entry_t const &peep_entry);
  transmap_entry_t transmap_entry_make_exreg_locs_src_sized() const;
  bool transmap_entry_maps_to_dst_reg(exreg_group_id_t g, exreg_id_t r) const;
  bool transmap_entry_maps_to_flag_loc(transmap_loc_id_t &loc_id) const;
  bool transmap_entry_maps_to_loc(transmap_loc_id_t loc_id) const;
  void transmap_entry_canonicalize_and_output_mappings(regmap_t &regmap, memslot_map_t &memslot_map);
  void transmap_entry_update_regnum_for_loc(int loc, dst_ulong regnum);
  void transmap_entry_intersect(transmap_entry_t const &other);
  bool transmap_entry_is_superset_of(transmap_entry_t const &other) const;
} transmap_entry_t;

typedef struct transmap_t {
  map<exreg_group_id_t, map<reg_identifier_t, transmap_entry_t>> extable;
  transmap_entry_t zero_locs_entry;

  void transmap_update_modifiers_using_peep_tmap_and_regmap(transmap_t const *peep_tmap, regmap_t const &inv_src_regmap);
  void transmap_scale_and_add_src_env_addr_to_memlocs();
  bool transmap_maps_to_src_env_esp_reg() const;
  transmap_t transmap_make_exreg_locs_src_sized() const;
  void transmap_get_memlocs(set<dst_ulong> &memlocs) const;
  bool transmap_maps_reg_to_flag_loc(exreg_group_id_t g, reg_identifier_t const &r, transmap_loc_id_t &loc_id) const;
  bool transmap_maps_multiple_regs_to_flag_loc() const;
  bool transmap_maps_used_reg_to_flag_loc(regset_t const &use) const;
  bool transmap_maps_to_symbol_loc(int regnum, regset_t const *live_in, exreg_group_id_t &src_group, reg_identifier_t &src_reg_id) const;
  bool transmap_maps_to_exreg_loc(exreg_group_id_t group, int regnum, exreg_group_id_t &src_group, reg_identifier_t &src_reg_id) const;
  void transmap_count_dst_regs(regcount_t *regcount) const;
  size_t transmap_count_dst_regs_for_group(exreg_group_id_t group) const;
  void transmap_get_assigned_src_regs(regset_t &out) const;
  bool transmap_is_superset_of(transmap_t const &other) const;
  bool transmap_has_mapping_for_src_reg(exreg_group_id_t g, reg_identifier_t const &r) const;
} transmap_t;

void add_transmap_renamings_of_in_out_transmaps_to_ls(std::list<std::pair<transmap_t, std::vector<transmap_t>>> &ls, transmap_t const &in_tmap, std::vector<transmap_t> const &out_tmaps);
void transmaps_subtract(transmap_t *dst, transmap_t const *src1,
    transmap_t const *src2);
void transmaps_union(transmap_t *dst, transmap_t const *src1, transmap_t const *src2);

//extern int use_xmm_regs;

#endif
