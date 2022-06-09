#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <inttypes.h>
#include <sys/mman.h>
#include "support/timers.h"
#include "rewrite/bbl.h"
#include "support/src-defs.h"

#include "valtag/memslot_map.h"
#include "rewrite/peep_entry_list.h"
#include "rewrite/peep_match_attrs.h"

#include "i386/cpu_consts.h"
#include "insn/rdefs.h"
#include "rewrite/static_translate.h"

#include "config-host.h"

#include "insn/jumptable.h"
#include "rewrite/peephole.h"
//#include "rewrite/harvest.h"

#include "support/globals.h"
#include "support/debug.h"
#include "support/c_utils.h"
#include "valtag/regmap.h"
#include "valtag/regcons.h"
//#include "imm_map.h" 
#include "valtag/imm_vt_map.h" 
//#include "rewrite/memlabel_map.h" 
#include "ppc/insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "insn/src-insn.h"
#include "insn/dst-insn.h"
#include "valtag/transmap.h"
#include "rewrite/symbol_map.h"
#include "rewrite/src_iseq_match_renamings.h"
#include "support/debug.h"
#include "rewrite/transmap_set.h"
//#include "enumerator.h"
#include "rewrite/transmap_genset.h"

#include "insn/live_ranges.h"

#include "i386/regs.h"
#include "x64/regs.h"

#include "valtag/elf/elf.h"
#include "cutils/rbtree.h"
#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"
#include "rewrite/nomatch_pairs.h"
#include "rewrite/nomatch_pairs_set.h"

#include "valtag/regcount.h"
#include "valtag/nextpc.h"
#include "valtag/nextpc_map.h"
//#include "rewrite/locals_info.h"
#include "tfg/nextpc_function_name_map.h"
#ifdef offsetof
#undef offsetof
#endif
#define offsetof(type, field) ((size_t) &((type *)0)->field)

using namespace std;


//bool running_as_fbgen = false;
bool running_as_peepgen = false;
//char const *g_exec_name = NULL;

static char pbuf1[8192], pbuf2[2048];

#define FAST_REG_ALLOC_LOOKAHEAD 3

size_t snprint_insn_ppc(char *buf, size_t buf_size, target_ulong pc,
    uint8_t *code, target_ulong size, int flags);



//char const *peep_trans_table = ABUILD_DIR "/peep.trans.tab.preprocessed";

/* Strings used to print hex characters (for debugging only) */
static char as1[409600];
static char as2[40960];
static char as3[10240];
static char as4[40960];
static char as5[10240];
static char as6[10240];
static char as7[10240];
static char as8[10240];
static char as9[10240];

//char pred_row_buf[1L<<20];

//static uint32_t use_M_for_RM = 0;   /* 32-bit bitmap */

static void
add_peephole_entry_to_hash_index(peep_entry_list_t *tab, long tab_size, peep_entry_t *e)
{
  peep_entry_t *prev, *iter;
  long i;

  i = src_iseq_hash_func_after_canon(e->src_iseq, e->src_iseq_len, tab_size);
  DYN_DBG(peep_tab_add, "src_iseq_len = %ld, hash index: %ld\n", e->src_iseq_len, i);

  /* Add in increasing order of cost */
  /*for (prev = NULL, iter = tab[i]; iter; prev = iter, iter = iter->next) {
    if (iter->cost >= e->cost) {
      break;
    }
  }

  if (!prev) {
    ASSERT(iter == tab[i]);
    e->next = tab[i];
    tab[i] = e;
  } else {
    ASSERT(iter == prev->next);
    e->next = iter;
    prev->next = e;
  }*/
  tab[i].insert(e);
}

static void
rehash_peephole_table(peep_entry_list_t *_new, long new_size,
    peep_entry_list_t const *old, long old_size)
{
  peep_entry_t *e;//, *next;
  long i;

  //memset(_new, 0, new_size * sizeof(peep_entry_t *));
  for (i = 0; i < new_size; i++) {
    _new[i].clear();
  }
  for (i = 0; i < old_size; i++) {
    for (auto iter = old[i].begin(); iter != old[i].end(); iter++) {
      //next = iter->next;
      e = *iter;
      add_peephole_entry_to_hash_index(_new, new_size, e);
    }
  }
}

static void
update_hash_index(peep_table_t *tab, peep_entry_t *e)
{
  peep_entry_t *iter, *prev;
  peep_entry_list_t *new_hash;
  long new_hash_size;

  if (tab->hash_size < 2 * (tab->num_peep_entries + 1)) {
    //new_hash_size = MAX(200, 2 * (tab->hash_size + 1));
    new_hash_size = 2 * (tab->hash_size + 1);
    if (new_hash_size < 200) {
      new_hash_size = 200;
    }
    //new_hash = (peep_entry_t **)malloc(new_hash_size * sizeof(peep_entry_t *));
    new_hash = new peep_entry_list_t[new_hash_size];
    ASSERT(new_hash);
    rehash_peephole_table(new_hash, new_hash_size, tab->hash, tab->hash_size);
    //free(tab->hash);
    delete [] tab->hash;
    tab->hash = new_hash;
    tab->hash_size = new_hash_size;
  }
  add_peephole_entry_to_hash_index(tab->hash, tab->hash_size, e);
}

static void
update_rbtree_index(peep_table_t *tab, peep_entry_t *e)
{
  rbtree_insert(&tab->rbtree, &e->rb_elem);
}

static void
dst_regcons_make_excess_entries_zero_for_group(regcons_t *regcons, exreg_group_id_t g, size_t max_num_regs)
{
  if (regcons->regcons_extable.count(g) == 0) {
    return;
  }
  for (auto &rr : regcons->regcons_extable.at(g))  {
    if (rr.first.reg_identifier_get_id() >= max_num_regs) {
      //cout << __func__ << " " << __LINE__ << ": clearing regcons entry associated with group " << g << " reg " << rr.first.reg_identifier_to_string() << endl;
      //cout << __func__ << " " << __LINE__ << ": cur_num_regs = " << cur_num_regs << endl;
      //cout << __func__ << " " << __LINE__ << ": max_num_regs = " << max_num_regs << endl;
      rr.second.clear();
    }
  }
}

static void
dst_regcons_make_excess_entries_zero(regcons_t *regcons)
{
  for (exreg_group_id_t i = 0; i < DST_NUM_EXREG_GROUPS; i++) {
    dst_regcons_make_excess_entries_zero_for_group(regcons, i, DST_NUM_EXREGS(i) - regset_count_exregs(&dst_reserved_regs, i));
  }
}

static void
regcons_avoid_dst_reserved_reg(regcons_t *regcons, exreg_group_id_t group, exreg_id_t dst_reg)
{
  int i;

  //cout << __func__ << " " << __LINE__ << ": entry regcons =\n" << regcons_to_string(regcons, as1, sizeof as1) << endl;
  ASSERT(group >= 0 && group < MAX_NUM_EXREG_GROUPS);
  int num_non_zero_bitmaps = 0;
  int last_non_zero_regnum = -1;
  for (i = 0; i < MAX_NUM_EXREGS(group); i++) {
    //regcons_entry_unmark(&regcons->regcons_extable[group][i], dst_reg);
    //regcons_entry_unmark(regcons_get_entry(regcons, group, i), dst_reg);
    auto bitmap = regcons->regcons_extable[group][i].bitmap;
    bitmap &= ~(1 << dst_reg);
    if (bitmap != 0) {
      regcons->regcons_extable[group][i].bitmap = bitmap;
      num_non_zero_bitmaps++;
      last_non_zero_regnum = i;
    }
  }
  ASSERT(last_non_zero_regnum != -1);
  if (num_non_zero_bitmaps == DST_NUM_EXREGS(group)) {
    regcons->regcons_extable[group][last_non_zero_regnum].bitmap = (1 << dst_reg);
  }
  //cout << __func__ << " " << __LINE__ << ": returning\n" << regcons_to_string(regcons, as1, sizeof as1) << endl;
}

static void
regcons_avoid_dst_reserved_regs(regcons_t *regcons, regset_t const *dst_reserved_regs)
{
  //cout << __func__ << " " << __LINE__ << ": dst_reserved_regs = " << regset_to_string(dst_reserved_regs, as1, sizeof as1) << endl;
  for (const auto &g : dst_reserved_regs->excregs) {
    for (const auto &r : g.second) {
      DYN_DEBUG(INFER_CONS, cout << __func__ << " " << __LINE__ << ": calling regcons_avoid_dst_reserved_reg(" << g.first << ", " << r.first.reg_identifier_to_string() << ")\n");
      regcons_avoid_dst_reserved_reg(regcons, g.first, r.first.reg_identifier_get_id());
    }
  }
}

/*static void
transmaps_union(transmap_t *union_tmap, transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_out_tmaps)
{
  transmap_copy(union_tmap, in_tmap);
  for (size_t i = 0; i < num_out_tmaps; i++) {
    transmap_t const &out_tmap = out_tmaps[i];
    for (const auto &g : out_tmap.extable) {
      for (const auto &r : g.second) {
        union_tmap->extable[g.first][r.first].transmap_entries_union(&r.second);
      }
    }
  }
}*/

static void
add_peephole_entry(peep_table_t *tab,
    src_insn_t const *src_iseq, long src_iseq_len,
    transmap_t const *transmap, transmap_t const *out_transmap,
    dst_insn_t const *dst_iseq, long dst_iseq_len,
    regset_t const *live_out, int nextpc_indir,
    long num_live_out, bool mem_live_out,
    //regset_t const &flexible_regs,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    //nomatch_pairs_t const *nomatch_pairs, int num_nomatch_pairs,
    nomatch_pairs_set_t const &nomatch_pairs_set,
    regconst_map_possibilities_t const &regconst_map_possibilities,
    //struct symbol_map_t const *src_symbol_map, struct symbol_map_t const *dst_symbol_map,
    //struct locals_info_t *src_locals_info, struct locals_info_t *dst_locals_info,
    //struct insn_line_map_t const *src_iline_map, struct insn_line_map_t const *dst_iline_map,
    //nextpc_function_name_map_t const *nextpc_function_name_map,
    //uint32_t peep_flags,
    long cost, long linenum)
{
  static char dst_assembly[409600];
  peep_entry_t *peep_entry;
  long i;

  /*if (tab->num_peep_entries == tab->peep_entries_size) {
    tab->peep_entries_size *= 2;
    tab->peep_entries_size = MAX(tab->peep_entries_size, 100);
    tab->peep_entries = (peep_entry_t *)realloc(tab->peep_entries,
        tab->peep_entries_size * sizeof(peep_entry_t));
    ASSERT(tab->peep_entries);
  }
  peep_entry = &tab->peep_entries[tab->num_peep_entries];*/
  //peep_entry = (peep_entry_t *)malloc(sizeof(peep_entry_t));
  peep_entry = new peep_entry_t;
  ASSERT(peep_entry);
  tab->num_peep_entries++;

  //insn_line_map_init(&peep_entry->src_iline_map);
  //insn_line_map_init(&peep_entry->dst_iline_map);

  //peep_entry->in_tcons = (regcons_t *)malloc(sizeof(regcons_t));
  peep_entry->in_tcons = new regcons_t;
  ASSERT(peep_entry->in_tcons);
  dst_iseq_to_string(dst_iseq, dst_iseq_len, dst_assembly,
      sizeof dst_assembly);
  dst_infer_regcons_from_assembly(dst_assembly, peep_entry->in_tcons);
  //regcons_unmark_using_regset(peep_entry->in_tcons, &dst_reserved_regs);
  //dst_regcons_make_excess_entries_zero(peep_entry->in_tcons);
  regcons_avoid_dst_reserved_regs(peep_entry->in_tcons, &dst_reserved_regs);
  //regcons_minimize(peep_entry->in_tcons);
  DYN_DBG2(INFER_CONS, "dst_assembly:\n%s\nin_tcons:\n%s\n", dst_assembly,
      regcons_to_string(peep_entry->in_tcons, as1, sizeof as1));

  regmap_t regmap;
  bool ret;
  regmap_init(&regmap);
  ret = regmap_assign_using_regcons(&regmap, peep_entry->in_tcons, ARCH_DST, NULL, fixed_reg_mapping_t());
  ASSERT(ret);
  DYN_DBG2(PEEP_TAB, "regmap:\n%s\n", regmap_to_string(&regmap, as1, sizeof as1));

  //peep_entry->src_iseq = (src_insn_t *)malloc(src_iseq_len * sizeof(src_insn_t));
  peep_entry->src_iseq = new src_insn_t[src_iseq_len];
  ASSERT(peep_entry->src_iseq);
  src_iseq_copy(peep_entry->src_iseq, src_iseq, src_iseq_len);
  peep_entry->src_iseq_len = src_iseq_len;

  transmap_t *in_tmap;
  transmap_init(&in_tmap);
  transmap_copy(in_tmap, transmap);
  transmap_rename_using_dst_regmap(in_tmap, &regmap, NULL);
  peep_entry->set_in_tmap(in_tmap);

  DYN_DBG3(peep_tab_add, "in_tmap:\n%s\n", transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1));

  //peep_entry->out_tmap = (transmap_t *)malloc(num_live_out * sizeof(transmap_t));
  peep_entry->out_tmap = new transmap_t[num_live_out];
  ASSERT(peep_entry->out_tmap);
  for (i = 0; i < num_live_out; i++) {
    transmap_copy(&peep_entry->out_tmap[i], &out_transmap[i]);
    transmap_rename_using_dst_regmap(&peep_entry->out_tmap[i], &regmap, NULL);
    DYN_DBG3(peep_tab_add, "peep_entry->out_tmap[%ld]:\n%s\n", i,
        transmap_to_string(&peep_entry->out_tmap[i], as1, sizeof as1));
  }
  //peep_entry->flexible_used_regs = flexible_regs;

  //transmap_t *union_tmap;
  //transmap_init(&union_tmap);
  //transmaps_union(union_tmap, peep_entry->get_in_tmap(), peep_entry->out_tmap, num_live_out);
  //peep_entry->set_union_tmap(union_tmap);

  //peep_entry->dst_iseq = (dst_insn_t *)malloc(dst_iseq_len * sizeof(dst_insn_t));
  peep_entry->dst_iseq = new dst_insn_t[dst_iseq_len];
  ASSERT(peep_entry->dst_iseq);
  dst_iseq_copy(peep_entry->dst_iseq, dst_iseq, dst_iseq_len);
  peep_entry->dst_iseq_len = dst_iseq_len;
  dst_iseq_rename_var(peep_entry->dst_iseq, peep_entry->dst_iseq_len, &regmap, NULL, NULL, NULL, NULL, NULL, 0);
  DYN_DBG3(PEEP_TAB, "dst_iseq:\n%s\n", dst_iseq_to_string(peep_entry->dst_iseq, peep_entry->dst_iseq_len, as1, sizeof as1));
  //peep_entry->peep_flags = peep_flags;
  peep_entry->cost = cost;

  regcons_t renamed_tcons;
  regcons_rename(&renamed_tcons, peep_entry->in_tcons, &regmap);
  regcons_copy(peep_entry->in_tcons, &renamed_tcons);

  dst_iseq_get_touch_not_live_in_regs(peep_entry->dst_iseq, peep_entry->dst_iseq_len,
      peep_entry->get_in_tmap(), &peep_entry->touch_not_live_in_regs);
  regset_copy(&peep_entry->temp_regs_used, &peep_entry->touch_not_live_in_regs);

  //peep_entry->live_out = (regset_t *)malloc(num_live_out * sizeof(regset_t));
  peep_entry->live_out = new regset_t[num_live_out];
  ASSERT(peep_entry->live_out);
  for (i = 0; i < num_live_out; i++) {
    regset_copy(&peep_entry->live_out[i], &live_out[i]);

    regset_t dst_live_out;
    regset_clear(&dst_live_out);
    transmap_get_used_dst_regs(&peep_entry->out_tmap[i], &dst_live_out);
    regset_diff(&peep_entry->temp_regs_used, &dst_live_out);
  }
  peep_entry->nextpc_indir = nextpc_indir;
  peep_entry->num_live_outs = num_live_out;
  peep_entry->mem_live_out = mem_live_out;

  peep_entry->dst_fixed_reg_mapping = fixed_reg_mapping.fixed_reg_mapping_rename_using_regmap(regmap);

  //peep_entry->nomatch_pairs = NULL;
  //if (num_nomatch_pairs) {
  //  peep_entry->nomatch_pairs = new nomatch_pairs_t[num_nomatch_pairs];
  //  ASSERT(peep_entry->nomatch_pairs);
  //}
  peep_entry->nomatch_pairs_set = nomatch_pairs_set;

  //for (i = 0; i < num_nomatch_pairs; i++) {
  //  peep_entry->nomatch_pairs[i] = nomatch_pairs[i];
  //  //memcpy(&peep_entry->nomatch_pairs[i], &nomatch_pairs[i],
  //  //    sizeof (nomatch_pairs_t));
  //}
  //peep_entry->num_nomatch_pairs = num_nomatch_pairs;

  peep_entry->regconst_map_possibilities = regconst_map_possibilities;

  //peep_entry->src_symbol_map = (symbol_map_t *)malloc(sizeof(symbol_map_t));
  //peep_entry->dst_symbol_map = (symbol_map_t *)malloc(sizeof(symbol_map_t));
  //ASSERT(peep_entry->src_symbol_map);
  //ASSERT(peep_entry->dst_symbol_map);
  //symbol_map_init(peep_entry->src_symbol_map);
  //symbol_map_init(peep_entry->dst_symbol_map);
  //if (src_symbol_map) {
  //  symbol_map_copy(peep_entry->src_symbol_map, src_symbol_map);
  //}
  //if (dst_symbol_map) {
  //  symbol_map_copy(peep_entry->dst_symbol_map, dst_symbol_map);
  //}
  //peep_entry->src_locals_info = src_locals_info;
  //peep_entry->dst_locals_info = dst_locals_info;

  //peep_entry->nextpc_function_name_map = (nextpc_function_name_map_t *)malloc(sizeof(nextpc_function_name_map_t));
  //ASSERT(peep_entry->nextpc_function_name_map);
  //nextpc_function_name_map_init(peep_entry->nextpc_function_name_map);
  //if (nextpc_function_name_map) {
  //  nextpc_function_name_map_copy(peep_entry->nextpc_function_name_map, nextpc_function_name_map);
  //}

  //DYN_DBG(PEEP_TAB, "src_iline_map->num_entries = %d\n", (int)src_iline_map->num_entries);
  //DYN_DBG(PEEP_TAB, "dst_iline_map->num_entries = %d\n", (int)dst_iline_map->num_entries);

  //insn_line_map_copy(&peep_entry->src_iline_map, src_iline_map);
  //insn_line_map_copy(&peep_entry->dst_iline_map, dst_iline_map);

  //DYN_DBG(PEEP_TAB, "peep_entry->src_iline_map->num_entries = %d\n", (int)peep_entry->src_iline_map.num_entries);

  peep_entry->peep_entry_set_line_number(linenum);

  peep_entry->populate_usedef();

  DYN_DBG(peep_tab_add, "Adding peep_table entry (linenum %ld):\n"
      "\tsrc_iseq: len %ld\n%s\ttransmap:\n%s\tdst_iseq:\n%s"
      "\tdst_fixed_reg_mapping:\n%s\n"
      "\ttouch_not_live_in:\n%s\n"
      "\ttemp_regs_used:\n%s\n"
      "\tnomatch_pairs_set:\n%s"
      "\tregconst_map_possibilities:\n%s"
      "\tcost = %ld\n", linenum, (long)src_iseq_len,
      src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1),
      transmaps_to_string(peep_entry->get_in_tmap(), peep_entry->out_tmap, peep_entry->num_live_outs, as2, sizeof as2),
      dst_iseq_to_string(peep_entry->dst_iseq, peep_entry->dst_iseq_len, as4, sizeof as4),
      peep_entry->dst_fixed_reg_mapping.fixed_reg_mapping_to_string().c_str(),
      regset_to_string(&peep_entry->touch_not_live_in_regs, as7, sizeof as7),
      regset_to_string(&peep_entry->temp_regs_used, as5, sizeof as5),
      //nomatch_pairs_to_string(nomatch_pairs, num_nomatch_pairs, as8, sizeof as8),
      nomatch_pairs_set_to_string(nomatch_pairs_set/*, num_nomatch_pairs*/, as8, sizeof as8),
      regconst_map_possibilities.to_string().c_str(),
      cost);
  DYN_DBG(peep_tab_add, "in_tmap:\n%s\n", transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1));
  //DYN_DBG(peep_tab_add, "live_out[%ld]: %s\n", i,
  //    regset_to_string(&live_out[i], as1, sizeof as1));

  update_hash_index(tab, peep_entry);
  update_rbtree_index(tab, peep_entry);
}

/*static void
write_src_regcons_entry(regcons_t const *transcons, int src_regnum,
    char *buf, size_t size)
{
  char *ptr = buf, *end = buf + size;
  if (regcons_allows_all(transcons, src_regnum)) {
    ptr += snprintf(ptr, end - ptr, "%c", 'R');
  } else {
    int i;
    char const regcons_letter_name[] = {'A', 'C', 'D', 'B', 'S', 'T', 'I', 'J'};
    for (i = 0; i < I386_NUM_GPRS; i++) {
      if (regcons_allows(transcons, src_regnum, i)) {
        ptr += snprintf(ptr, end - ptr, "%c", regcons_letter_name[i]);
      }
    }
  }
}*/

bool
src_iseqs_equal(src_insn_t const *i1, src_insn_t const *i2, size_t n)
{
  char *tbuf1, *tbuf2;
  size_t max_alloc;

  max_alloc = 409600;
  tbuf1 = (char *)malloc(max_alloc * sizeof(char));
  tbuf2 = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(tbuf1);
  ASSERT(tbuf2);

  src_iseq_to_string(i1, n, tbuf1, max_alloc);
  src_iseq_to_string(i2, n, tbuf2, max_alloc);
  if (strcmp(tbuf1, tbuf2)) {
    free(tbuf1);
    free(tbuf2);
    return false;
  }
  free(tbuf1);
  free(tbuf2);
  return true;
}

bool
dst_iseqs_equal(dst_insn_t const *i1, dst_insn_t const *i2, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    if (!dst_insns_equal(&i1[i], &i2[i])) {
      return false;
    }
  }
  return true;
  /*char *tbuf1, *tbuf2;
  size_t max_alloc;

  max_alloc = 409600;
  tbuf1 = (char *)malloc(max_alloc * sizeof(char));
  tbuf2 = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(tbuf1);
  ASSERT(tbuf2);

  dst_iseq_to_string(i1, n, tbuf1, max_alloc);
  dst_iseq_to_string(i2, n, tbuf2, max_alloc);
  if (strcmp(tbuf1, tbuf2)) {
    free(tbuf1);
    free(tbuf2);
    return false;
  }
  free(tbuf1);
  free(tbuf2);
  return true;*/
}

static bool
dst_insn_equals(dst_insn_t const *i1, dst_insn_t const *i2)
{
#define MAX_DST_INSN_SIZE 40960
  char *tbuf1, *tbuf2;

  tbuf1 = (char *)malloc(MAX_DST_INSN_SIZE * sizeof(char));
  ASSERT(tbuf1);
  tbuf2 = (char *)malloc(MAX_DST_INSN_SIZE * sizeof(char));
  ASSERT(tbuf2);

  dst_insn_to_string(i1, tbuf1, MAX_DST_INSN_SIZE);
  dst_insn_to_string(i2, tbuf2, MAX_DST_INSN_SIZE);
  if (strcmp(tbuf1, tbuf2)) {
    free(tbuf1);
    free(tbuf2);
    return false;
  }
  free(tbuf1);
  free(tbuf2);
  return true;
}

static bool
transmap_matches_template_on_regset(transmap_t const *templ_tmap,
    transmap_t const *in_tmap, regset_t const *regs)
{
  //int i, j;

  /*for (i = 0; i < SRC_NUM_REGS; i++) {
    if (!regset_belongs_vreg(regs, i)) {
      continue;
    }
    if (!transmap_entry_matches_template(&templ_tmap->table[i], &in_tmap->table[i])) {
      return false;
    }
  }*/
  for (const auto &g : regs->excregs) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t j = r.first;
      if (!regset_belongs_ex(regs, i, j)) {
        continue;
      }
      if (!transmap_get_elem(in_tmap, i, j)->transmap_entry_matches_template(*transmap_get_elem(templ_tmap, i, j))) {
        return false;
      }
    }
  }
  return true;
}

/*
void
peep_substitute(struct translation_instance_t const *ti,
    struct src_insn_t const *src_iseq,
    long src_iseq_len,
    struct regset_t const *live_out,
    long num_nextpcs,
    struct transmap_t const *in_tmap, struct transmap_t *out_tmaps,
    struct i386_insn_t *i386_iseq, long *i386_iseq_len, long i386_iseq_size)
{
  peep_table_t const *tab;
  peep_entry_t const *e;
  bool mismatch_found;
  long i, n;

  tab = &ti->peep_table;
  ASSERT(tab->hash_size);

  n = peep_table_hash_func(src_iseq, src_iseq_len, tab->hash_size);
  for (e = tab->hash[n]; e; e = e->next) {
    if (e->src_iseq_len != src_iseq_len) {
      DYN_DBG2(peep_tab_add, "src_iseq_len mismatch: %ld<->%ld\n", e->src_iseq_len,
          src_iseq_len);
      continue;
    }
    if (!src_iseq_matches_template_for_substitute(e->src_iseq, src_iseq, src_iseq_len,
        &inv_src_regmap, &imm_vt_map)) {
      continue;
    }

    if (transmaps_match(

    regset_t templ_touch;
    src_iseq_compute_touch_var(&templ_touch, e->src_iseq, e->src_iseq_len);

    if (!transmap_matches_template_on_regset(e->in_tmap, in_tmap, &templ_touch)) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }

    if (e->num_live_outs != num_nextpcs) {
      continue;
    }

    mismatch_found = false;
    for (i = 0; i < num_nextpcs; i++) {
      if (!regset_contains(&live_out[i], &e->live_out[i])) {
        DYN_DBG2(peep_tab_add, "live_out[%ld] mismatch: %s <-> %s\n", i,
            regset_to_string(&e->live_out[i], as1, sizeof as1),
            regset_to_string(&live_out[i], as2, sizeof as2));
        mismatch_found = true;
        break;
      }
    }
    if (mismatch_found) {
      continue;
    }
    ASSERT(e->dst_iseq_len < i386_iseq_size);
    i386_iseq_copy(i386_iseq, e->dst_iseq, e->dst_iseq_len);
    *i386_iseq_len = e->dst_iseq_len;
    return;
  }
  NOT_REACHED();
}
*/

static bool
peep_search_for_identical_entry(peep_table_t const *tab,
    src_insn_t const *src_iseq, long src_iseq_len,
    transmap_t const *transmap, transmap_t const *out_transmap,
    dst_insn_t const dst_iseq[], long dst_iseq_len,
    regset_t const *live_out, int nextpc_indir,
    long num_live_outs, bool mem_live_out,
    //nomatch_pairs_t const *nomatch_pairs, int num_nomatch_pairs,
    nomatch_pairs_set_t const &nomatch_pairs_set,
    regconst_map_possibilities_t const &regconst_map_possibilities,
    //struct symbol_map_t const *src_symbol_map, struct symbol_map_t const *dst_symbol_map,
    //struct locals_info_t *src_locals_info, struct locals_info_t *dst_locals_info,
    //struct insn_line_map_t const *src_iline_map, struct insn_line_map_t const *dst_iline_map,
    int cost, int linenum) 
{
  bool mismatch_found;
  long i, n;

  if (!tab->hash_size) {
    return false;
  }
  n = src_iseq_hash_func_after_canon(src_iseq, src_iseq_len, tab->hash_size);
  ASSERT(n >= 0 && n < tab->hash_size);

  //printf("%s() %d: tab->hash_size = %zd\n", __func__, __LINE__, tab->hash_size);
  peep_entry_t const *peep_entry;
  for (auto iter = tab->hash[n].begin(); iter != tab->hash[n].end(); iter++) {
    peep_entry = *iter;
    //printf("%s() %d: peep_entry = %p\n", __func__, __LINE__, peep_entry);
    if (peep_entry->src_iseq_len != src_iseq_len) {
      DYN_DBG2(peep_tab_add, "src_iseq_len mismatch: %ld<->%ld\n", peep_entry->src_iseq_len,
          src_iseq_len);
      continue;
    }
    mismatch_found = false;
    for (i = 0; i < src_iseq_len; i++) {
      if (!src_iseqs_equal(&peep_entry->src_iseq[i], &src_iseq[i], 1)) {
        mismatch_found = true;
        DYN_DBG2(peep_tab_add, "mismatch at insn %ld: '%s' <-> '%s'\n", i,
            src_insn_to_string(&peep_entry->src_iseq[i], as1, sizeof as1),
            src_insn_to_string(&src_iseq[i], as2, sizeof as2));
      }
    }
    if (mismatch_found) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }

    if (!transmaps_equal(peep_entry->get_in_tmap(), transmap)) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }

    /*if (!temporaries_count_equal(&peep_entry->temporaries_count, temporaries_count)) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }*/

    /*ASSERT(peep_entry->in_tcons);
    ASSERT(transcons);
    if (!regcons_equal(peep_entry->in_tcons, transcons)) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }*/

    if (peep_entry->dst_iseq_len != dst_iseq_len) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }

    mismatch_found = false;
    for (i = 0; i <dst_iseq_len; i++) {
      if (!dst_insn_equals(&peep_entry->dst_iseq[i], &dst_iseq[i])) {
        DYN_DBG2(peep_tab_add, "%s() %d: mismatch found in instruction %ld : (%s) vs (%s)\n",
            __func__, __LINE__, i, dst_insn_to_string(&dst_iseq[i], as1, sizeof as1),
            dst_insn_to_string(&peep_entry->dst_iseq[i], as2, sizeof as2));
        mismatch_found = true;
        break;
      }
    }
    if (mismatch_found) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }

    if (peep_entry->num_live_outs != num_live_outs) {
      continue;
    }

    bool live_out_mismatch_found = false;
    long l;
    for (l = 0; l < num_live_outs; l++) {
      if (!regset_equal(&peep_entry->live_out[l], &live_out[l])) {
        DYN_DBG2(peep_tab_add, "live_out[%ld] mismatch: %s <-> %s\n", l,
            regset_to_string(&peep_entry->live_out[l], as1, sizeof as1),
            regset_to_string(&live_out[l], as2, sizeof as2));
        live_out_mismatch_found = true;
        break;
      }
    }
    if (live_out_mismatch_found) {
      continue;
    }
    if (peep_entry->mem_live_out != mem_live_out) {
      continue;
    }
    if (peep_entry->nextpc_indir != nextpc_indir) {
      continue;
    }
    /*if (memcmp(peep_entry->nextpc_is_indir, nextpc_is_indir,
        num_live_outs * sizeof(bool))) {
      continue;
    }*/

    if (!peep_entry->nomatch_pairs_set.equals(nomatch_pairs_set)) {
      continue;
    }
    //if (peep_entry->num_nomatch_pairs != num_nomatch_pairs) {
    //  DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
    //  continue;
    //}

    //mismatch_found = false;
    //for (i = 0; i < num_nomatch_pairs; i++) {
    //  if (memcmp(&peep_entry->nomatch_pairs[i], &nomatch_pairs[i],
    //        sizeof (nomatch_pairs_t))) {
    //    mismatch_found = true;
    //    break;
    //  }
    //}
    if (mismatch_found) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }

    if (!peep_entry->regconst_map_possibilities.equals(regconst_map_possibilities)) {
      continue;
    }
    /*if (!locals_info_equal(peep_entry->src_locals_info, src_locals_info)) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }
    if (!locals_info_equal(peep_entry->dst_locals_info, dst_locals_info)) {
      DYN_DBG2(peep_tab_add, "%s() %d:\n", __func__, __LINE__);
      continue;
    }*/
    DYN_DBG2(peep_tab_add, "returning true: src_iseq=%s, dst_seq=%s, "
        "peep_entry->src_iseq=%s, peep_entry->dst_iseq=%s\ntransmap:\n%s\n"
        "peep_entry->in_tmap:\n%s\n",
        src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1),
        dst_iseq_to_string(dst_iseq, dst_iseq_len, as2, sizeof as2),
        src_iseq_to_string(peep_entry->src_iseq, peep_entry->src_iseq_len,
          as3, sizeof as3),
        dst_iseq_to_string(peep_entry->dst_iseq, peep_entry->dst_iseq_len,
          as4, sizeof as4),
        transmap_to_string(transmap, as5, sizeof as5),
        transmap_to_string(peep_entry->get_in_tmap(), as6, sizeof as6));
    return true;
  }
  DYN_DBG2(peep_tab_add, "%s", "returning false.\n");
  return false;
}

/*static int
generic_gen_mem_modes(char const *mem_modes[], char const *mem_constraints[],
    char const *mem_live_regs[], char const *in_text)
{
  mem_modes[0] = "MEM";
  mem_constraints[0] = "";
  mem_live_regs[0] = "";
  return 1;
}

static int
generic_gen_jcc_conds(char const *jcc_conds[], char const *in_text)
{
  jcc_conds[0] = "jCC";
  return 1;
}*/

static void
make_string_replacement(char *out, char const *pattern, char const *replacement)
{
  char *ptr;
  int i;

  int pattern_len, replacement_len;
  pattern_len = strlen(pattern);
  replacement_len = strlen(replacement);

  ASSERT(pattern_len > 0);
  ASSERT(replacement_len >= 0);

  //printf("%s <-- %s\n", patterns[i], replacements[i]);

  ptr = out;
  while (ptr = strstr(ptr, pattern)) {
    int j;

    memmove(ptr + replacement_len, ptr + pattern_len,
        strlen(ptr + pattern_len) + 1);

    for (j = 0; j < replacement_len; j++) {
      *ptr++ = *(replacement + j);
    }
  }
}

/*void
transmap_remove_unnecessary_mappings(transmap_t *transmap,
    char const *dst_assembly)
{
  char pattern[256];
  long i, j;

  for (i = 0; i < SRC_NUM_REGS; i++) {
    snprintf(pattern, sizeof pattern, "%%vr%ld", i);
    if (!strstr(dst_assembly, pattern)) {
      transmap->table[i].loc = TMAP_LOC_NONE;
    }
  }
  for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < SRC_NUM_EXREGS(i); j++) {
      snprintf(pattern, sizeof pattern, "%%exvr%ld.%ld", i, j);
      if (!strstr(dst_assembly, pattern)) {
        transmap->extable[i][j].loc = TMAP_LOC_NONE;
      }
    }
  }
}*/

int
compute_nextpc_indir(src_insn_t const *src_iseq, long src_iseq_len,
    long num_live_outs)
{
  bool nextpc_is_indir[num_live_outs];
  src_tag_t pcrel_tag;
  uint64_t pcrel_val;
  long i;

  for (i = 0; i < num_live_outs; i++) {
    nextpc_is_indir[i] = true;
  }
  for (i = 0; i < src_iseq_len; i++) {
    vector<valtag_t> vs = src_insn_get_pcrel_values(&src_iseq[i]);
    for (auto v : vs) {
      pcrel_val = v.val;
      if (!(pcrel_val >= 0 && pcrel_val < num_live_outs)) {
        printf("src_iseq:\n%s\ni = %ld\nnum_live_outs=%ld", src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1), i, num_live_outs);
      }
      ASSERT(pcrel_val >= 0 && pcrel_val < num_live_outs);
      nextpc_is_indir[pcrel_val] = false;
    }
  }

  /* check that there should be at most one indir nextpc */
  bool found = false;
  int ret = -1;
  for (i = 0; i < num_live_outs; i++) {
    if (nextpc_is_indir[i] == true) {
      if (found) {
        cout << __func__ << " " << __LINE__ << ": num_live_outs = " << num_live_outs << endl;
        for (size_t nl = 0; nl < num_live_outs; nl++) {
          cout << "nextpc_is_indir[" << nl << "] = " << nextpc_is_indir[nl] << endl;
        }
      }
      ASSERT(!found);
      found = true;
      ret = i;
    }
  }
  return ret;
}

void
peeptab_try_and_add(peep_table_t *peeptab, src_insn_t const *src_iseq, long src_iseq_len,
    //regset_t const &flexible_regs,
    transmap_t const *transmap, transmap_t const *out_transmap,
    dst_insn_t const *dst_iseq, long dst_iseq_len,
    regset_t const *live_out,
    long num_live_out, bool mem_live_out,/* regcons_t const *transcons,*/
    fixed_reg_mapping_t const &fixed_reg_mapping,
    //struct symbol_map_t const *src_symbol_map, struct symbol_map_t const *dst_symbol_map,
    //struct locals_info_t *src_locals_info, struct locals_info_t *dst_locals_info,
    //struct insn_line_map_t const *src_iline_map, struct insn_line_map_t const *dst_iline_map,
    //nextpc_function_name_map_t const *nextpc_function_name_map,
    //uint32_t peep_flags,
    long linenum)
{
  long cost, num_nomatch_pairs;
  //regset_t touch;
  int nextpc_indir;

  //src_iseq_compute_touch_var(&touch, src_iseq, src_iseq_len);
  /*dst_insn_t dst_iseq_canon[dst_iseq_len];
  long dst_iseq_canon_len;
  regmap_t dst_regmap;

  dst_iseq_copy(dst_iseq_canon, dst_iseq, dst_iseq_len);
  dst_iseq_canon_len = dst_iseq_len;
  dst_iseq_canonicalize_vregs(dst_iseq_canon, dst_iseq_canon_len, &dst_regmap);

  transmap_t in_tmap_renamed;
  transmap_t out_tmaps_renamed[num_live_out];

  transmap_copy(&in_tmap_renamed, transmap);
  transmaps_copy(out_tmaps_renamed, out_transmap, num_live_out);
  transmaps_rename_using_dst_regmap(&in_tmap_renamed, out_tmaps_renamed, num_live_out, &dst_regmap);*/

//#define MAX_NUM_NOMATCH_PAIRS 256
  //nomatch_pairs_t nomatch_pairs[MAX_NUM_NOMATCH_PAIRS];
  //num_nomatch_pairs = compute_nomatch_pairs(nomatch_pairs,
  //    MAX_NUM_NOMATCH_PAIRS,
  //    transmap, out_transmap, num_live_out, dst_iseq, dst_iseq_len);
  nomatch_pairs_set_t nomatch_pairs_set = compute_nomatch_pairs(transmap, out_transmap, num_live_out, dst_iseq, dst_iseq_len);

  DYN_DEBUG(REGCONST, cout << "computing regconst_map_possibilities for src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl);
  DYN_DEBUG(REGCONST, cout << "dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);
  DYN_DEBUG(REGCONST, cout << "transmap =\n" << transmap_to_string(transmap, as1, sizeof as1) << endl);
  regconst_map_possibilities_t regconst_map_possibilities = compute_regconst_map_possibilities(transmap, out_transmap, dst_iseq, dst_iseq_len, num_live_out);

  cost = dst_iseq_compute_cost(dst_iseq, dst_iseq_len, dst_costfns[0]);

  nextpc_indir = compute_nextpc_indir(src_iseq, src_iseq_len, num_live_out);

  if (!peep_search_for_identical_entry(peeptab, src_iseq, src_iseq_len,
        transmap, out_transmap, dst_iseq, dst_iseq_len, live_out, nextpc_indir,
        num_live_out, mem_live_out,
        /*transcons, temporaries_count,*/
        //nomatch_pairs, num_nomatch_pairs,
        nomatch_pairs_set,
        regconst_map_possibilities,
        //src_symbol_map, dst_symbol_map,
        //src_locals_info, dst_locals_info,
        //src_iline_map, dst_iline_map,
        cost, linenum)) {
    DYN_DBG(PEEP_TAB, "dst_iseq:\n%s\n",
        dst_iseq_to_string(dst_iseq, dst_iseq_len, as2, sizeof as2));
    add_peephole_entry(peeptab, src_iseq, src_iseq_len, transmap, out_transmap,
        dst_iseq, dst_iseq_len, live_out, nextpc_indir, num_live_out, mem_live_out,
        /*flexible_regs, */fixed_reg_mapping,
        //nomatch_pairs, num_nomatch_pairs,
        nomatch_pairs_set,
        regconst_map_possibilities,
        // src_symbol_map, dst_symbol_map,
        //src_locals_info, dst_locals_info,
        //src_iline_map, dst_iline_map,
        //nextpc_function_name_map,
        //peep_flags,
        cost, linenum);
  } else {
    DYN_DBG(peep_tab_add, "Duplicate add peeptab entry (linenum %ld):\n",
        linenum);
  }
  DYN_DBG2(PEEP_TAB, "\tCOST %ld\n", cost);
}

static void
process_peep_tab_entry(translation_instance_t *ti,
    peep_table_t *peep_table, src_insn_t const *src_iseq,
    long src_iseq_len, regset_t const *live_out, long num_live_out,
    bool mem_live_out,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    //regset_t const &flexible_regs,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    //struct symbol_map_t const *src_symbol_map, struct symbol_map_t const *dst_symbol_map,
    //struct locals_info_t *src_locals_info, struct locals_info_t *dst_locals_info,
    //struct insn_line_map_t const *src_iline_map,
    //struct insn_line_map_t const *dst_iline_map,
    char const *dst_assembly,// uint32_t peep_flags,
    //nextpc_function_name_map_t const *nextpc_function_name_map,
    int linenum)
{
  //char src_assembly_preprocessed[strlen(src_assembly) + 1024];
  long /*max_src_iseq_len, */max_dst_iseq_len;
  //transmap_t in_tmap, out_tmaps[num_live_out];
  //regset_t live_regs[num_live_outs];
  //temporaries_count_t temporaries_count;
  dst_insn_t *dst_iseq;
  regset_t tmap_regs;
  //src_insn_t *src_iseq;
  long dst_iseq_len = 0;
  long i/*, src_iseq_len*/;
  char const *ptr;
  char line[2048];

  //max_src_iseq_len = strlen(src_assembly) + 1024;
  max_dst_iseq_len = strlen(dst_assembly) + 1024;
  /*src_iseq = (src_insn_t *)malloc(max_src_iseq_len * sizeof(src_insn_t));
  ASSERT(src_iseq);*/
  //dst_iseq = (dst_insn_t *)malloc(max_dst_iseq_len * sizeof(dst_insn_t));
  dst_iseq = new dst_insn_t[max_dst_iseq_len];
  ASSERT(dst_iseq);

  /*for (i = 0; i < max_src_iseq_len; i++) {
    src_insn_init(&src_iseq[i]);
  }
  for (i = 0; i < max_i386_iseq_len; i++) {
    dst_insn_init(&dst_iseq[i]);
  }

  src_iseq_len = src_iseq_from_string(NULL, src_iseq, src_assembly);
  ASSERT(src_iseq_len > 0);*/

  //src_iseq_len = src_iseq_canonicalize(src_iseq, src_iseq_len);

  /*transmaps_from_string(&in_tmap, out_tmaps,
      num_live_out, &tmap_regs, tmap_string);*/

  /*for (i = 0; i < num_live_outs; i++) {
    if (!strcmp(live_regs_string[i], "")) {
      regset_copy(&live_regs[i], regset_empty());
    } else {
      DYN_DBG2(PEEP_TAB, "live_regs_strings[%ld] = %s\n", i, live_regs_string[i]);
      regset_from_string(&live_regs[i], live_regs_string[i]);
    }
  }*/

  DYN_DBG2(PEEP_TAB, "calling dst_iseq_from_string.\n"
      "dst_assembly:\n%s\ntransmap:\n%s\n",
      dst_assembly, transmap_to_string(in_tmap, as2, sizeof as2));
  dst_iseq_len = dst_iseq_from_string(ti, dst_iseq, max_dst_iseq_len, dst_assembly, I386_AS_MODE_32);
  if (dst_iseq_len == -1) {
    printf("linenum %d: dst_iseq_from_string "
        "returned -1.\ndst_assembly:\n%s\n", linenum,
        dst_assembly);
  }
  ASSERT(dst_iseq_len >= 0);
  DYN_DBG2(PEEP_TAB, "After dst_iseq_from_string, "
    "string:\n%s\ndst_iseq:\n%s\n", dst_assembly,
    dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
  DYN_DEBUG2(PEEP_TAB,
    int i;
    for (i = 0; i < dst_num_costfns; i++) {
      printf("cost[%d] = %lld\n", i, dst_iseq_compute_cost(dst_iseq, dst_iseq_len,
          dst_costfns[i]));
    }
  );

  peeptab_try_and_add(peep_table, src_iseq, src_iseq_len, /*flexible_regs, */in_tmap, out_tmaps,
      dst_iseq, dst_iseq_len, live_out, num_live_out, mem_live_out,//peep_flags,
      fixed_reg_mapping,
      //src_symbol_map, dst_symbol_map, src_locals_info, dst_locals_info, src_iline_map, dst_iline_map,
      //nextpc_function_name_map,
      linenum);
  //free(dst_iseq);
  delete [] dst_iseq;
}

/*static uint32_t
parse_peep_entry_flags(char const *remaining)
{
  char const *ptr = remaining, *end = ptr + strlen(ptr);
  uint32_t ret = (uint32_t)-1;
  bool base_flag_seen = false;
  while (ptr < end) {
    char const *next;
    while (*ptr == ' ') {
      ptr++;
    }
    next = ptr + 1;
    if (strstart(ptr, "base", &next)) {
      base_flag_seen = true;
    } else if (strstart(ptr, "fresh", &next)) {
      ret &= ~PEEP_ENTRY_FLAGS_NOT_FRESH;
    } else if (strstart(ptr, "!fresh", &next)) {
      ret &= ~PEEP_ENTRY_FLAGS_FRESH;
    }
    ptr = next;
  }
  if (!base_flag_seen) {
    ret &= ~PEEP_ENTRY_FLAGS_BASE_ONLY;
  } 
  return ret;
}*/

/*static char const *
peep_flags_to_string(uint32_t flags, char *buf, size_t size)
{
  char *ptr = buf, *end = ptr + size;
  uint32_t ret = (uint32_t)-1;

  if (flags & PEEP_ENTRY_FLAGS_BASE_ONLY) {
    ptr += snprintf(ptr, end - ptr, " base");
  }
  if (flags & PEEP_ENTRY_FLAGS_FRESH) {
    ptr += snprintf(ptr, end - ptr, " fresh");
  }
  if (flags & PEEP_ENTRY_FLAGS_NOT_FRESH) {
    ptr += snprintf(ptr, end - ptr, " !fresh");
  }
  return buf;
}*/

static int
fscanf_dst_assembly(FILE *fp, char *buf, size_t size, char *entry_tag, size_t entry_tag_size)
{
  char const *remaining;
  static char line[409600];
  int linenum = 0;
  buf[0] = '\0';
  for (;;) {
    int num_read;
    num_read = fscanf(fp, "  %[^\n]\n", line);
    linenum++;
    ASSERT (num_read == 1);
 
    if (strstart(line, "==", &remaining)) {
      if (entry_tag) {
        strncpy(entry_tag, remaining, entry_tag_size);
      }
      break;
    }
    strcat(buf, line);
    strcat(buf, "\n");
  }
  return linenum;
}

/*static uint32_t
compute_peep_flags(translation_instance_t const *ti)
{
  uint32_t ret = (uint32_t)-1;
  ASSERT(ti);
  if (ti->in && ti->in->fresh) {
    ret &= ~PEEP_ENTRY_FLAGS_NOT_FRESH;
  } else {
    ret &= ~PEEP_ENTRY_FLAGS_FRESH;
  }
  if (ti->mode == SUPEROPTIMIZE) {
    // filter out rules other than base rules. 
    ret &= ~PEEP_ENTRY_FLAGS_BASE_ONLY;
  }
  return ret;
}*/

void
print_peephole_table(struct peep_table_t const *tab)
{
  peep_entry_t *e/*, *prev*/;
  long i, j;

  printf("Printing peeptab\n");
  for (i = 0; i < tab->hash_size; i++) {
    j = 0;
    for (/*prev = NULL, */auto iter = tab->hash[i].begin();
        iter != tab->hash[i].end();
        iter++) {
      e = *iter;
      printf("%ld.%ld: line %ld (iter %p)\n%scost=%ld\n", i, j, e->peep_entry_get_line_number(), e,
           src_iseq_to_string(e->src_iseq, e->src_iseq_len, as1, sizeof as1),
           e->cost);
      //printf("nomatch_pairs =\n%s\n", nomatch_pairs_to_string(e->nomatch_pairs,
      //      e->num_nomatch_pairs, as1, sizeof as1));
      printf("nomatch_pairs_set =\n%s\n", nomatch_pairs_set_to_string(e->nomatch_pairs_set,
            /*e->num_nomatch_pairs, */as1, sizeof as1));
      j++;
    }
  }
}

/*void
peeptab_free_insn_line_maps(peep_table_t *tab)
{
  long n;
  for (n = 0; n < tab->hash_size; n++) {
    peep_entry_t *peep_entry;
    for (peep_entry = tab->hash[n]; peep_entry; peep_entry = peep_entry->next) {
      insn_line_map_free(&peep_entry->src_iline_map);
      insn_line_map_free(&peep_entry->dst_iline_map);
    }
  }
}*/

void
read_default_peeptabs(translation_instance_t *ti)
{
  read_peep_trans_table(ti, &ti->peep_table, SUPEROPTDBS_DIR "fb.trans.tab", NULL);
  read_peep_trans_table(ti, &ti->peep_table, BUILD_DIR "/etfg-i386.ptab", NULL);
}

void
read_peep_trans_table(translation_instance_t *ti, peep_table_t *peep_table,
    char const *file, char const *peeptab_tag)
{
  if (!file) {
    return;
  }
  int linenum = 0;
  static char line[40960];
  size_t fs;
  FILE *fp;

  /*size_t max_alloc = 40960;
  char *src_assembly;
  src_assembly = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(src_assembly);*/
  //printf("%s() %d: peep_table->hash_size = %zd\n", __func__, __LINE__, peep_table->hash_size);

  fp = fopen(file, "r");
  if (!fp) {
    printf("fopen(%s) failed: %s.\n", file, strerror(errno));
    NOT_REACHED();
  }

  while ((fs = fscanf(fp, "%[^\n]\n", line)) != EOF) {
    char const *remaining = NULL;
    int rc;
    linenum++;
    if (fs == 0) {
      rc = fread(line, 1, 1, fp);
      ASSERT(rc >= 0);
      ASSERT(line[0] == '\n');
      continue;
    }
    if (line[0] == '#') {
      continue;
    } else if (   !strstart(line, "ENTRY:", &remaining)
               && !strstart(line, "NEW_ENTRY:", &remaining)) {
      err ("Parse error in linenum %d: %s\n", linenum, line);
      ASSERT (0);
    }
    ASSERT(remaining);
    /*uint32_t flags, peep_flags;
    peep_flags = parse_peep_entry_flags(remaining);
    flags = compute_peep_flags(ti);*/

    long src_iseq_len, src_iseq_size, live_out_size, out_tmaps_size;
    transmap_t in_tmap, *out_tmaps;
    long sa, num_out_tmaps;
    src_insn_t *src_iseq;
    regset_t *live_out;
    bool mem_live_out;
    size_t num_live_out;
    //regset_t flexible_regs;
    int num_read;

    src_iseq_size = 4096;
    live_out_size = 16;
    out_tmaps_size = live_out_size;

    //src_iseq = (src_insn_t *)malloc(src_iseq_size * sizeof(src_insn_t));
    src_iseq = new src_insn_t[src_iseq_size];
    ASSERT(src_iseq);
    //live_out = (regset_t *)malloc(live_out_size * sizeof(regset_t));
    live_out = new regset_t[live_out_size];
    ASSERT(live_out);
    //out_tmaps = (transmap_t *)malloc(out_tmaps_size * sizeof(transmap_t));
    out_tmaps = new transmap_t[out_tmaps_size];
    ASSERT(out_tmaps);

    num_read = fscanf_harvest_output_src(fp, src_iseq, src_iseq_size, &src_iseq_len,
        live_out, live_out_size, &num_live_out, &mem_live_out, 0);
    ASSERT(num_read != EOF);

    num_read = fscanf(fp, "--\n");
    ASSERT(num_read != EOF);

    //num_read = fscanf_transmaps_with_flexible_regs(fp, &in_tmap, out_tmaps, out_tmaps_size, &num_out_tmaps, flexible_regs);
    num_read = fscanf_transmaps(fp, &in_tmap, out_tmaps, out_tmaps_size, &num_out_tmaps/*, flexible_regs*/);
    ASSERT(num_read != EOF);
    linenum += num_read;
    //ASSERT(num_out_tmaps == num_live_out);
    ASSERT(num_out_tmaps == num_live_out);

    fixed_reg_mapping_t fixed_reg_mapping;
    num_read = fixed_reg_mapping_t::fscanf_fixed_reg_mappings(fp, fixed_reg_mapping);
    ASSERT(num_read != EOF);
    linenum += num_read;

    static symbol_map_t src_symbol_map, dst_symbol_map;
    //num_read = fscanf(fp, "--\n");
    //ASSERT(num_read != EOF);

    //printf("%s() %d: peep_table->hash_size = %zd\n", __func__, __LINE__, peep_table->hash_size);

    //fscanf_symbol_map(fp, &src_symbol_map);

    //num_read = fscanf(fp, "--\n");
    //ASSERT(num_read != EOF);

    //fscanf_symbol_map(fp, &dst_symbol_map);

    //num_read = fscanf(fp, "--\n");
    //ASSERT(num_read != EOF);

    //printf("%s() %d: peep_table->hash_size = %zd\n", __func__, __LINE__, peep_table->hash_size);

    //ASSERT(g_ctx);
    //struct locals_info_t *src_locals_info = locals_info_new(g_ctx);
    //struct locals_info_t *src_locals_info = locals_info_new_using_g_ctx();
    //fscanf_locals_info(fp, src_locals_info);

    //num_read = fscanf(fp, "--\n");
    //ASSERT(num_read != EOF);

    //struct locals_info_t *dst_locals_info = locals_info_new_using_g_ctx();
    //fscanf_locals_info(fp, dst_locals_info);

    //printf("%s() %d: peep_table->hash_size = %zd\n", __func__, __LINE__, peep_table->hash_size);

    //num_read = fscanf(fp, "--\n");
    //ASSERT(num_read != EOF);

    //struct insn_line_map_t src_iline_map, dst_iline_map;
    //insn_line_map_init(&src_iline_map);
    //insn_line_map_init(&dst_iline_map);

    //printf("%s() %d: peep_table->hash_size = %zd\n", __func__, __LINE__, peep_table->hash_size);
    //fscanf_insn_line_map(fp, &src_iline_map);

    //printf("%s() %d: peep_table->hash_size = %zd\n", __func__, __LINE__, peep_table->hash_size);

    //num_read = fscanf(fp, "--\n");
    //ASSERT(num_read != EOF);

    //fscanf_insn_line_map(fp, &dst_iline_map);

    //printf("%s() %d: peep_table->hash_size = %zd\n", __func__, __LINE__, peep_table->hash_size);

    //num_read = fscanf(fp, "--\n");
    //ASSERT(num_read != EOF);

    long entry_tag_max_alloc, dst_assembly_max_alloc;
    char *dst_assembly;
    char *entry_tag;

    dst_assembly_max_alloc = 409600;
    dst_assembly = (char *)malloc(dst_assembly_max_alloc * sizeof(char));
    ASSERT(dst_assembly);

    entry_tag_max_alloc = 409600;
    entry_tag = (char *)malloc(entry_tag_max_alloc * sizeof(char));
    ASSERT(entry_tag);

    //printf("%s() %d: peep_table->hash_size = %zd\n", __func__, __LINE__, peep_table->hash_size);

    linenum += fscanf_dst_assembly(fp, dst_assembly, dst_assembly_max_alloc, entry_tag, entry_tag_max_alloc);
    ASSERT(strlen(dst_assembly) < dst_assembly_max_alloc);
    ASSERT(strlen(entry_tag) < entry_tag_max_alloc);
    linenum++;

    //nextpc_function_name_map_t nextpc_function_name_map;
    //printf("%s() %d: peep_table->hash_size = %zd\n", __func__, __LINE__, peep_table->hash_size);
    //printf("%s() %d: entry_tag = %s.\n", __func__, __LINE__, entry_tag);
    //nextpc_function_name_map_from_peep_comment(&nextpc_function_name_map, entry_tag);
    //printf("%s() %d: entry_tag = %s. nextpc_function_name_map.num_nextpcs = %d.\n", __func__, __LINE__, entry_tag, nextpc_function_name_map.num_nextpcs);

    if (   !peeptab_tag
        || strstr(entry_tag, peeptab_tag)) {
      DYN_DBG(PEEP_TAB, "processing entry with tag %s.\n", entry_tag);
      process_peep_tab_entry(ti, peep_table, src_iseq, src_iseq_len,
          live_out, num_live_out, mem_live_out, &in_tmap, out_tmaps,
          //flexible_regs,
          fixed_reg_mapping,
          //&src_symbol_map, &dst_symbol_map,
          //src_locals_info, dst_locals_info, &src_iline_map,
          //&dst_iline_map,
          dst_assembly,// &nextpc_function_name_map,
          linenum);
    } else {
      DYN_DBG(PEEP_TAB, "ignoring entry with tag %s.\n", entry_tag);
    }
    free(entry_tag);
    free(dst_assembly);
    //free(src_iseq);
    delete [] src_iseq;
    //free(live_out);
    delete [] live_out;
    //free(out_tmaps);
    //cout << __func__ << " " << __LINE__ << ": out_tmaps_size = " << out_tmaps_size << endl;
    //cout << __func__ << " " << __LINE__ << ": num_live_out = " << num_live_out << endl;
    //cout << __func__ << " " << __LINE__ << ": in_tmap = " << transmap_to_string(&in_tmap, as1, sizeof as1) << endl;
    //for (int i  = 0; i < num_live_out; i++) {
    //  cout << __func__ << " " << __LINE__ << ": out_tmaps[" << i << "] = " << transmap_to_string(&out_tmaps[i], as1, sizeof as1) << endl;
    //}
    delete [] out_tmaps;
    //insn_line_map_free(&src_iline_map);
    //insn_line_map_free(&dst_iline_map);
  }
  fclose(fp);
  DYN_DEBUG(peep_tab_add, print_peephole_table(&ti->peep_table););
}

static void
make_replacements(char *buf, size_t size, char *patterns[], char *replacements[],
    int np)
{
  int i;
  for (i = 0; i < np; i++) {
    char *ptr;
    ptr = buf;
    while (ptr = strstr(ptr, patterns[i])) {
      size_t pattern_len = strlen(patterns[i]);
      size_t replacement_len = strlen(replacements[i]);
      memmove(ptr + replacement_len, ptr + pattern_len, strlen(ptr + pattern_len) + 1);
      memcpy (ptr, replacements[i], replacement_len);
      ptr += replacement_len;
    }
  }
}

#define MAX_PATTERN_SIZE 64
/*static int
generate_reg_replacements(int x86_regnum, int repl, char *prefix, char *patterns[],
    char *replacements[], int np)
{
  patterns[np] = malloc(MAX_PATTERN_SIZE);
  replacements[np] = malloc(MAX_PATTERN_SIZE);
  dst_regl2str(x86_regnum, patterns[np], MAX_PATTERN_SIZE);
  snprintf(replacements[np], MAX_PATTERN_SIZE, "%s%d", prefix, repl);
  np++;

  patterns[np] = malloc(MAX_PATTERN_SIZE);
  replacements[np] = malloc(MAX_PATTERN_SIZE);
  dst_regw2str(x86_regnum, patterns[np], MAX_PATTERN_SIZE);
  snprintf(replacements[np], MAX_PATTERN_SIZE, "%s%dw", prefix, repl);
  np++;

  if (x86_regnum < R_ESP) {
    patterns[np] = malloc(MAX_PATTERN_SIZE);
    replacements[np] = malloc(MAX_PATTERN_SIZE);
    dst_regb2str(x86_regnum, patterns[np], MAX_PATTERN_SIZE);
    snprintf(replacements[np], MAX_PATTERN_SIZE, "%s%db", prefix, repl);
    np++;
  }

  return np;
}*/

void
peeptab_entry_write(FILE *fp, peep_entry_t const *peep_entry,
    translation_instance_t const *ti)
{
  long i, max_alloc;
  char *buf;

  fprintf(fp, "ENTRY:\n");
  max_alloc = 409600;
  buf = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(buf);
  src_harvested_iseq_to_string(buf, max_alloc, peep_entry->src_iseq,
      peep_entry->src_iseq_len, peep_entry->live_out, peep_entry->num_live_outs,
      peep_entry->mem_live_out);
  fprintf(fp, "%s", buf);
  free(buf);
  
  fprintf(fp, "--\n");
  fprintf(fp, "%s", transmaps_to_string(peep_entry->get_in_tmap(), peep_entry->out_tmap,
      peep_entry->num_live_outs/*, peep_entry->flexible_used_regs*/, as1, sizeof as1));
  fprintf(fp, "--\n");
  fprintf(fp, "%s", peep_entry->dst_fixed_reg_mapping.fixed_reg_mapping_to_string().c_str());
  /*fprintf(fp, "--\n");
  locals_info_to_string(peep_entry->src_locals_info, as1, sizeof as1);
  fprintf(fp, "%s", as1);
  fprintf(fp, "--\n");
  locals_info_to_string(peep_entry->dst_locals_info, as1, sizeof as1);
  fprintf(fp, "%s", as1);
  fprintf(fp, "--\n");
  insn_line_map_to_string(&peep_entry->src_iline_map, as1, sizeof as1);
  fprintf(fp, "%s", as1);
  fprintf(fp, "--\n");
  insn_line_map_to_string(&peep_entry->dst_iline_map, as1, sizeof as1);
  fprintf(fp, "%s", as1);*/
  fprintf(fp, "--\n");
  dst_iseq_with_relocs_to_string(peep_entry->dst_iseq, peep_entry->dst_iseq_len,
      ti?ti->reloc_strings:NULL, ti?ti->reloc_strings_size:0, I386_AS_MODE_32, as1, sizeof as1);
  fprintf(fp, "%s", as1);
  fprintf(fp, "==\n\n");
}

void
peeptab_write(translation_instance_t *ti, peep_table_t const *peeptab, char const *file)
{
  //uint32_t flag_groups[] = {PEEP_ENTRY_FLAGS_BASE_ONLY, ~PEEP_ENTRY_FLAGS_BASE_ONLY};
  //char const *flag_group_suffixes[] = {".base", ""};
  char filebase[strlen(file) + 32];
  peep_entry_t *peep_entry;
  //peep_table_t *peeptab;
  long i, f;
  FILE *fp;

  //for (f = 0; f < sizeof flag_groups / sizeof flag_groups[0]; f++) {
    snprintf(filebase, sizeof filebase, "%s", file/*, flag_group_suffixes[f]*/);
    fp = fopen(filebase, "w");
    ASSERT(fp);
    //peeptab = &ti->peep_table;
    for (i = 0; i < peeptab->hash_size; i++) {
      for (auto iter = peeptab->hash[i].begin(); iter != peeptab->hash[i].end(); iter++) {
        peep_entry = *iter;
        //if ((peep_entry->peep_flags & flag_groups[f]) == peep_entry->peep_flags) {
          peeptab_entry_write(fp, peep_entry, ti);
        //}
      }
    }
    fclose(fp);
  //}
}

static void
peep_table_build_index(translation_instance_t *ti, peep_entry_t **peep_table)
{
}

/*static void
read_peep_trans_table(translation_instance_t *ti, peep_table_t *peep_table,
    char const *file)
{
  char outfile[strlen(file) + 16];
#if ARCH_SRC == ARCH_I386
#define MAX_SRC_REGS_USED 0
#else
#define MAX_SRC_REGS_USED 4
#endif
  //for (use_M_for_RM = 0; use_M_for_RM < (1 << MAX_SRC_REGS_USED); use_M_for_RM++) {
    __read_peep_trans_table(ti, peep_table, file);
  //}
  //snprintf(outfile, sizeof outfile, "%s.out", file);
}
*/

/*static void
build_temporaries(temporaries_t *temps, temporaries_t const *peep_entry_temps,
    transmap_t const *tmap, regcons_t const *tcons)
{
  bool extemporaries_available[I386_NUM_EXREG_GROUPS][I386_NUM_EXREGS(0)];
  bool temporaries_available[I386_NUM_REGS];
  long i;

  memset(temporaries_available, 0x0, sizeof temporaries_available);
  for (i = 0; i < I386_NUM_REGS; i++) {
    temporaries_available[i] = true;
  }
  for (i = 0; i < I386_NUM_EXREG_GROUPS; i++) {
    int j;
    for (j = 0; j < I386_NUM_EXREGS(i); j++) {
      extemporaries_available[i][j] = true;
    }
  }

  for (i = 0; i < SRC_NUM_REGS; i++) {
    if (tmap->table[i].loc == TMAP_LOC_REG || tmap->table[i].loc == TMAP_LOC_RF) {
      ASSERT(tmap->table[i].reg.rf.regnum >= 0 );
      ASSERT(tmap->table[i].reg.rf.regnum < I386_NUM_GPRS);
      temporaries_available[tmap->table[i].reg.rf.regnum] = false;
    }
    if (   tmap->table[i].loc >= TMAP_LOC_EXREG(0)
        && tmap->table[i].loc < TMAP_LOC_EXREG(I386_NUM_EXREG_GROUPS)) {
      int group = tmap->table[i].loc - TMAP_LOC_EXREG(0);
      ASSERT(tmap->table[i].reg.rf.regnum >= 0 );
      ASSERT(tmap->table[i].reg.rf.regnum < I386_NUM_EXREGS(group));
      extemporaries_available[group][tmap->table[i].reg.rf.regnum] = false;
    }
  }
  for (i = 0; i < I386_NUM_REGS; i++) {
    if (peep_entry_temps->table[i] == -1) {
      continue;
    }
    ASSERT(peep_entry_temps->table[i] >= 0);
    ASSERT(peep_entry_temps->table[i] < I386_NUM_REGS);
    int j;
    for (j = 0; j < I386_NUM_REGS; j++) {
      if (   temporaries_available[j]
          && regcons_allows_temporary(tcons, i, j)) {
        // DYN_DBG(PEEP_SEARCH,"peep_entry_temps->table[%d]=%d, temps->table[%d]=%d\n",
        //    i, peep_entry_temps->table[i], j, temps->table[j]);
        //dst_regmap->table[(int)peep_entry_temps->table[i]] = temps->table[j];
        temps->table[i] = j;
        temporaries_available[j] = false;
        break;
      }
    }
    ASSERT (j < I386_NUM_REGS); // we must have found a match
  }
  for (i = 0; i < I386_NUM_EXREG_GROUPS; i++) {
    int j;
    for (j = 0; j < I386_NUM_EXREGS(i); j++) {
      if (peep_entry_temps->extable[i][j] == -1) {
        continue;
      }
      ASSERT(peep_entry_temps->extable[i][j] >= 0);
      ASSERT(peep_entry_temps->extable[i][j] < I386_NUM_EXREGS(i));
      int k;
      for (k = 0; k < I386_NUM_EXREGS(i); k++) {
        if (   extemporaries_available[i][k]
            && regcons_allows_extemporary(tcons, i, j, k)) {
          // DYN_DBG(PEEP_SEARCH,"peep_entry_temps->table[%d]=%d, temps->table[%d]=%d\n",
          //  i, peep_entry_temps->table[i], j, temps->table[j]);
          //dst_regmap->table[(int)peep_entry_temps->table[i]] = temps->table[j];
          temps->extable[i][j] = k;
          extemporaries_available[i][k] = false;
          break;
        }
      }
    }
  }
}*/

/*static void
transmap_intersect_with_regs(transmap_t *tmap, regset_t const *regs)
{
  //int i, j;

  //for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
  //  for (j = 0; j < SRC_NUM_EXREGS(i); j++) {
  for (const auto &g : tmap->extable) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t j = r.first;
      if (!regset_belongs_ex(regs, i, j)) {
        //tmap->extable[i][j].num_locs = 0;
        tmap->extable[i][j].num_locs = 0;
      }
    }
  }
}*/

static bool
transmap_agrees_with_temporaries_count(transmap_t const *tmap,
    regcount_t const *temps_count)
{
  regcount_t tmap_count;
  int i;

  tmap->transmap_count_dst_regs(&tmap_count);
  DYN_DBG2(enum_tmaps, "tmap_count:\n%s\n",
      regcount_to_string(&tmap_count, as1, sizeof as1));

  /*if (tmap_count.reg_count + temps_count->reg_count > DST_NUM_REGS) {
    return false;
  }*/
  for (i = 0; i < DST_NUM_EXREG_GROUPS; i++) {
    if (tmap_count.exreg_count[i] + temps_count->exreg_count[i] > DST_NUM_EXREGS(i) - regset_count_exregs(&dst_reserved_regs, i)) {
      return false;
    }
  }
  return true;
}

static void
regmap_add_constant_mappings_using_transmap(regmap_t *dst, transmap_t const *in_tmap, regmap_t const *inv_src_regmap)
{
  for (const auto &g : inv_src_regmap->regmap_extable) {
    for (const auto &r : g.second) {
      if (   r.second.reg_identifier_denotes_constant()
          && in_tmap->extable.count(g.first)
          && in_tmap->extable.at(g.first).count(r.first)) {
        in_tmap->extable.at(g.first).at(r.first).regmap_add_constant_mappings_using_transmap_entry(dst, r.second);
      }
    }
  }
}

static void
regset_erase_regs_mapped_to_constants(regset_t *regset, regmap_t const *regmap)
{
  for (const auto &g : regmap->regmap_extable) {
    for (const auto &r : g.second) {
      if (r.second.reg_identifier_denotes_constant()) {
        if (   regset->excregs.count(g.first)
            && regset->excregs.at(g.first).count(r.first)) {
          regset->excregs.at(g.first).erase(r.first);
        }
      }
    }
  }
}

static void
populate_memslot_map_from_peep_tmap_and_actual_tmap(memslot_map_t &ret, transmap_t const *peep_tmap, transmap_t const *actual_tmap, regmap_t const *inv_src_regmap, int base_reg)
{
  //memslot_map_t ret(base_reg);
  for (const auto &g : peep_tmap->extable) {
    for (const auto &r : g.second) {
      for (const auto &peep_loc : r.second.get_locs()) {
        if (peep_loc.get_loc() == TMAP_LOC_SYMBOL) {
          bool tmap_mapping_exists =    actual_tmap->extable.count(g.first)
                                     && actual_tmap->extable.at(g.first).count(r.first)
                                     && actual_tmap->extable.at(g.first).at(r.first).contains_mapping_to_loc(TMAP_LOC_SYMBOL);
          bool regconst_mapping_exists =    inv_src_regmap->regmap_extable.count(g.first)
                                         && inv_src_regmap->regmap_extable.at(g.first).count(r.first)
                                         && inv_src_regmap->regmap_extable.at(g.first).at(r.first).reg_identifier_denotes_constant();
          dst_ulong peep_regnum = peep_loc.get_regnum();
          if (!tmap_mapping_exists && !regconst_mapping_exists) {
            if (!ret.memslot_map_belongs(peep_regnum)) {
              valtag_t vt = { .val = -1, .tag = tag_const };
              ret.memslot_map_add_memaddr_mapping(peep_regnum, vt);
            }
            continue;
          }
          ASSERT(tmap_mapping_exists != regconst_mapping_exists);
          if (!ret.memslot_map_belongs(peep_regnum)) {
            if (tmap_mapping_exists) {
              dst_ulong actual_regnum = actual_tmap->extable.at(g.first).at(r.first).get_regnum_for_loc(TMAP_LOC_SYMBOL);
              valtag_t vt = { .val = actual_regnum, .tag = tag_const };
              ret.memslot_map_add_memaddr_mapping(peep_regnum, vt);
            } else {
              valtag_t imm_vt = inv_src_regmap->regmap_extable.at(g.first).at(r.first).reg_identifier_get_imm_valtag();
              ret.memslot_map_add_constant_mapping(peep_regnum, imm_vt);
            }
          }
        }
      }
    }
  }
  //return ret;
}

static transmap_entry_t const *
transmap_get_entry_for_reg(transmap_t const *tmap, exreg_group_id_t ig, reg_identifier_t const &ir)
{
  for (const auto &g : tmap->extable) {
    if (g.first != ig) {
      continue;
    }
    for (const auto &r : g.second) {
      if (r.first == ir) {
        return &r.second;
      }
    }
  }
  return NULL;
}

static transmap_entry_t const &
transmaps_get_entry_for_reg(transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_out_tmaps, exreg_group_id_t g, reg_identifier_t const &r)
{
  transmap_entry_t const *ret;
  if ((ret = transmap_get_entry_for_reg(in_tmap, g, r))) {
    return *ret;
  }
  for (size_t i = 0; i < num_out_tmaps; i++) {
    if ((ret = transmap_get_entry_for_reg(&out_tmaps[i], g, r))) {
      return *ret;
    }
  }
  NOT_REACHED();
}

static void
populate_inv_dst_regmap_using_loc_pair(transmap_loc_t const &first_loc, transmap_loc_t const &loc, regmap_t *inv_dst_regmap, memslot_map_t *inv_memslot_map)
{
  transmap_loc_id_t l = loc.get_loc();
  ASSERT(first_loc.get_loc() == l);
  if (l == TMAP_LOC_SYMBOL) {
    dst_ulong lr = loc.get_regnum();
    dst_ulong fr = first_loc.get_regnum();
    valtag_t vt;
    vt.tag = tag_var;
    vt.val = fr;
    inv_memslot_map->memslot_map_add_memaddr_mapping(lr, vt);
  } else {
    exreg_group_id_t g = l - TMAP_LOC_EXREG(0);
    reg_identifier_t const &lr = loc.get_reg_id();
    reg_identifier_t const &fr = first_loc.get_reg_id();
    inv_dst_regmap->add_mapping(g, lr, fr);
  }
}

static void
populate_inv_dst_regmap_using_equal_regset(exreg_group_id_t g, set<reg_identifier_t> const &equal_regset, transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_out_tmaps, regmap_t *inv_dst_regmap, memslot_map_t *inv_memslot_map)
{
  ASSERT(equal_regset.size() > 1);
  reg_identifier_t const &first_among_equals = *equal_regset.begin();
  transmap_entry_t const &first_entry = transmaps_get_entry_for_reg(in_tmap, out_tmaps, num_out_tmaps, g, first_among_equals);
  ASSERT(first_entry.get_num_locs() == 1); //peep tmaps can have only one loc per entry
  transmap_loc_t const &first_loc = first_entry.get_loc(0);
  for (const auto &r : equal_regset) { //XXX: why is r not getting used in loop body?
    transmap_entry_t const &entry = transmaps_get_entry_for_reg(in_tmap, out_tmaps, num_out_tmaps, g, first_among_equals);
    ASSERT(entry.get_num_locs() == 1); //peep tmaps can have only one loc per entry
    transmap_loc_t const &loc = entry.get_loc(0);
    populate_inv_dst_regmap_using_loc_pair(first_loc, loc, inv_dst_regmap, inv_memslot_map);
  }
}

static void
transmaps_get_inv_dst_regmap_from_inv_src_regmap(transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_out_tmaps, regmap_t const *inv_src_regmap, regmap_t *inv_dst_regmap, memslot_map_t *inv_memslot_map)
{
  autostop_timer func_timer(__func__);
  map<exreg_group_id_t, map<reg_identifier_t, set<reg_identifier_t>>> many_to_one_mappings; //for every R1->r and R2->r in inv_src_regmap, add r->[R1,R2] in many_to_one_mappings
  for (const auto &g : inv_src_regmap->regmap_extable) {
    for (const auto &r : g.second) {
      many_to_one_mappings[g.first][r.second].insert(r.first);
    }
  }
  for (const auto &g : many_to_one_mappings) {
    for (const auto &r : g.second) {
      if (r.second.size() > 1) {
        populate_inv_dst_regmap_using_equal_regset(g.first, r.second, in_tmap, out_tmaps, num_out_tmaps, inv_dst_regmap, inv_memslot_map);
      }
    }
  }
}

static vector<src_iseq_match_renamings_t>
peep_entry_match_src_side(
    peep_entry_t const *peep_entry,
    src_insn_t const *src_iseq,
    long src_iseq_len,
    regset_t const *live_outs, long num_live_outs,
    regset_t const& src_iseq_use, regset_t const& src_iseq_def,
    memset_t const& src_iseq_memuse, memset_t const& src_iseq_memdef,
    string prefix)
{
  autostop_timer func_timer(__func__);

  //cout << __func__ << " " << __LINE__ << endl;
  if (peep_entry->src_iseq_len != src_iseq_len) {
    DYN_DBG2(enum_tmaps, "src_iseq_len mismatch: %ld <-> %ld ", peep_entry->src_iseq_len,
        src_iseq_len);
    return vector<src_iseq_match_renamings_t>();
  }
  vector<src_iseq_match_renamings_t> renamings;
  //autostop_timer func2_timer(string(__func__) + ".2");
  renamings = src_iseq_matches_template(peep_entry->src_iseq, src_iseq, src_iseq_len/*,
      src_iseq_match_renamings*/, prefix + "." + string(__func__));
  //  DYN_DBG2(enum_tmaps, "src_iseq mismatch:\npeep->src_iseq:\n%s\nsrc_iseq:\n%s\n",
  //      src_iseq_to_string(peep_entry->src_iseq, src_iseq_len, as1, sizeof as1),
  //      src_iseq_to_string(src_iseq, src_iseq_len, as2, sizeof as2));

  //autostop_timer func3_timer(string(__func__) + ".3");
  vector<src_iseq_match_renamings_t> ret;
  for (const auto &src_iseq_match_renamings : renamings) {
    DYN_DBGS2(prefix.c_str(), "%s", "Enum-tmaps instructions match\n");
    DYN_DBGS2(prefix.c_str(), "inv_src_regmap:\n%s\n",
        regmap_to_string(&src_iseq_match_renamings.get_regmap(), as1, sizeof as1));

    //autostop_timer func4_timer(string(__func__) + ".4");
    if (!src_inv_regmap_agrees_with_regconst_map_possibilities(&src_iseq_match_renamings.get_regmap(),
          peep_entry->get_in_tmap(), peep_entry->out_tmap, peep_entry->num_live_outs,
          peep_entry->regconst_map_possibilities)) {
      DYN_DEBUGS2(prefix.c_str(), cout << "Enum-tmaps Rule at line " << peep_entry->peep_entry_get_line_number() << ": This rule requires a reg to be "
          "mapped to a const which is not permitted. Skipping this rule..\n"
          "regconst_map_possibilities:\n" << peep_entry->regconst_map_possibilities.to_string() << "\n"
          "inv_src_regmap:\n" << regmap_to_string(&src_iseq_match_renamings.get_regmap(), as2, sizeof as2) << "\n");
      continue;
    }

    //autostop_timer func5_timer(string(__func__) + ".5");
    if (!src_inv_regmap_agrees_with_nomatch_pairs(&src_iseq_match_renamings.get_regmap(),
          peep_entry->nomatch_pairs_set
          /*peep_entry->nomatch_pairs, peep_entry->num_nomatch_pairs*/)) {
      DYN_DBGS2(prefix.c_str(), "Enum-tmaps Rule at line %ld: This rule requires aliasing of two "
          "un-aliasable registers. Skipping this rule..\nnomatch_pairs:\n%s\n"
          "inv_src_regmap:\n%s\n", peep_entry->peep_entry_get_line_number(),
          //nomatch_pairs_to_string(peep_entry->nomatch_pairs,
          //  peep_entry->num_nomatch_pairs, as1, sizeof as1),
          nomatch_pairs_set_to_string(peep_entry->nomatch_pairs_set,
            /*peep_entry->num_nomatch_pairs, */as1, sizeof as1),
          regmap_to_string(&src_iseq_match_renamings.get_regmap(), as2, sizeof as2));
      continue;
    }

    //autostop_timer func6_timer(string(__func__) + ".6");
    if (num_live_outs != peep_entry->num_live_outs) {
      DYN_DBGS2(prefix.c_str(), "Enum-tmaps Rule at line number %ld: Number of exits don't match. "
          "num_live_outs=%ld, peep_entry->num_live_outs=%ld\n", peep_entry->peep_entry_get_line_number(),
          num_live_outs, peep_entry->num_live_outs);
      continue;
    }

    //autostop_timer func7_timer(string(__func__) + ".7");
    regset_t use, def, touch;
    memset_t memuse, memdef;
    regset_copy(&use, &src_iseq_use);
    regset_copy(&def, &src_iseq_def);
    memset_copy(&memuse, &src_iseq_memuse);
    memset_copy(&memdef, &src_iseq_memdef);
    //src_iseq_get_usedef(src_iseq, src_iseq_len, &use, &def, &memuse, &memdef);
    regset_copy(&touch, &use);
    regset_union(&touch, &def);

    DYN_DBGS2(prefix.c_str(), "Enum-tmaps use: %s\n",
        regset_to_string(&use, as1, sizeof as1));
    DYN_DBGS2(prefix.c_str(), "Enum-tmaps def: %s\n",
        regset_to_string(&def, as1, sizeof as1));

    //autostop_timer func8_timer(string(__func__) + ".8");
    //regset_clear(&st_use);
    //regset_clear(&st_touch);
    regset_t st_use, st_touch;
    //memset_t st_memuse, st_memdef;
    regset_inv_rename(&st_use, &use, &src_iseq_match_renamings.get_regmap());
    regset_inv_rename(&st_touch, &touch, &src_iseq_match_renamings.get_regmap());
    //memset_inv_rename(&st_memuse, &memuse, &inv_src_regmap);
    //memset_inv_rename(&st_memdef, &memdef, &inv_src_regmap);
    //cout << __func__ << " " << __LINE__ << ": memuse = " << memset_to_string(&memuse, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": st_memuse = " << memset_to_string(&st_memuse, as1, sizeof as1) << endl;

    DYN_DBGS2(prefix.c_str(), "Enum-tmaps touch: %s\n",
        regset_to_string(&touch, as1, sizeof as1));
    DYN_DBGS2(prefix.c_str(), "Enum-tmaps st_touch: %s\n",
        regset_to_string(&st_touch, as1, sizeof as1));

    //autostop_timer func9_timer(string(__func__) + ".9");
    regset_t templ_touch, templ_use;
    memset_t templ_memuse, templ_memuse_renamed, templ_memdef, templ_memdef_renamed;
    //src_iseq_compute_touch_var(&templ_touch, &templ_use, &templ_memuse,
    //    &templ_memdef,
    //    peep_entry->src_iseq,
    //    peep_entry->src_iseq_len);
    regset_copy(&templ_use, &peep_entry->get_use());
    //regset_copy(&templ_def, &peep_entry->get_def());
    regset_copy(&templ_touch, &peep_entry->get_touch());
    memset_copy(&templ_memuse, &peep_entry->get_memuse());
    memset_copy(&templ_memdef, &peep_entry->get_memdef());
    //autostop_timer func91_timer(string(__func__) + ".91");
    memset_rename(&templ_memuse_renamed, &templ_memuse, &src_iseq_match_renamings.get_regmap(), &src_iseq_match_renamings.get_imm_vt_map());
    memset_rename(&templ_memdef_renamed, &templ_memdef, &src_iseq_match_renamings.get_regmap(), &src_iseq_match_renamings.get_imm_vt_map());
    //autostop_timer func92_timer(string(__func__) + ".92");
    regset_erase_regs_mapped_to_constants(&templ_touch, &src_iseq_match_renamings.get_regmap());
    regset_erase_regs_mapped_to_constants(&templ_use, &src_iseq_match_renamings.get_regmap());
    //regset_union_cregs_with_vregs(&templ_touch);

    DYN_DBGS2(prefix.c_str(), "Enum-tmaps templ_touch: %s\n",
        regset_to_string(&templ_touch, as1, sizeof as1));

    //autostop_timer func10_timer(string(__func__) + ".10");
    if (!regset_equals_coarsely(&templ_touch, &st_touch)) {
      DYN_DBGS2(prefix.c_str(), "Enum-tmaps Rule at line %ld: the touched register-set is not equal. "
          "templ_touch=%s\nst_touch=%s\n", peep_entry->peep_entry_get_line_number(),
          regset_to_string(&templ_touch, as1, sizeof as1),
          regset_to_string(&st_touch, as2, sizeof as2));
      continue;
    }

    //autostop_timer func11_timer(string(__func__) + ".11");
    if (   !memset_contains(&memuse, &templ_memuse_renamed)
        || !memset_contains(&templ_memuse_renamed, &memuse)
        || !memset_contains(&memdef, &templ_memdef_renamed)
        || !memset_contains(&templ_memdef_renamed, &memdef)) {
      DYN_DBGS(prefix.c_str(), "Enum-tmaps Rule at line %ld: the st-used mem-set does not contain "
          "templ_memuse. "
          "templ_memuse_renamed=%s\nmemuse=%s\n", peep_entry->peep_entry_get_line_number(),
          memset_to_string(&templ_memuse_renamed, as1, sizeof as1),
          memset_to_string(&memuse, as2, sizeof as2));
      DYN_DBGS(prefix.c_str(), "templ_memdef_renamed=%s\n", memset_to_string(&templ_memdef_renamed, as1, sizeof as1));
      DYN_DBGS(prefix.c_str(), "memdef=%s\n", memset_to_string(&memdef, as1, sizeof as1));
      continue;
    }

    //autostop_timer func12_timer(string(__func__) + ".12");
    bool live_regs_mismatch = false;
    long l;
    for (l = 0; l < num_live_outs; l++) {
      regset_t plive_out;
      regset_t st_live_out;
      //autostop_timer func13_timer(string(__func__) + ".13");

      regset_copy(&plive_out, &live_outs[l]);
      regset_intersect(&plive_out, &touch);
      regset_inv_rename(&st_live_out, &plive_out, &src_iseq_match_renamings.get_regmap());

      /* intersect with templ_touch, as it is possible that due to aliasing,
         the bits in st_live_out (and touch and st_live_out) are union of
         the bits present in the aliased vregs. */
      regset_intersect(&st_live_out, &templ_touch);

      DYN_DBGS2(prefix.c_str(), "Enum-tmaps Checking Live regs %ld. peep_entry->live_out[%ld] "
        "= %s ; st_live_out = %s\n", l, l,
        regset_to_string(&peep_entry->live_out[l], as1, sizeof as1),
        regset_to_string(&st_live_out, as2, sizeof as2));

      if (!regset_contains(&peep_entry->live_out[l], &st_live_out)) {
        DYN_DBGS2(prefix.c_str(), "Rule at line number %ld: Live registers %ld dont match.\n",
            peep_entry->peep_entry_get_line_number(), l);
        live_regs_mismatch = true;
        break;
      }
    }
    //autostop_timer func14_timer(string(__func__) + ".14");
    if (live_regs_mismatch) {
      DYN_DBGS2(prefix.c_str(), "%s", "Enum-tmaps live_regs_mismatch\n");
      continue;
    }
    ret.push_back(src_iseq_match_renamings);
  }
  return ret;
}

static cost_t
compute_peep_match_cost(cost_t peepcost, src_iseq_match_renamings_t const &src_iseq_match_renamings)
{
  nextpc_map_t const &nextpc_map = src_iseq_match_renamings.get_nextpc_map();
  if (!nextpc_map.maps_last_nextpc_identically()) {
    return peepcost + PEEP_MATCH_COST_FALLTHROUGH_NEXTPC_NOT_MAPPED_IDENTICALLY;
  } else {
    return peepcost;
  }
}

static cost_t
peep_entry_match(
    peep_entry_t const *peep_entry,
    src_insn_t const *iseq,
    long iseq_len, transmap_t *in_tmap, dst_insn_t *dst_iseq,
    long *dst_iseq_len, long dst_iseq_size,
    transmap_t *out_tmaps, nextpc_t const *nextpcs,
    /*src_ulong const *insn_pcs, */regset_t const *live_outs, long num_live_outs,
    transmap_t *peep_in_tmap, transmap_t *peep_out_tmaps,
    regcons_t *peep_regcons,
    regset_t *peep_temp_regs_used,
    nomatch_pairs_set_t *peep_nomatch_pairs_set,
    regcount_t *touch_not_live_in, int base_reg,
    string prefix
    )
{
  autostop_timer func_timer(__func__);
  src_iseq_match_renamings_t src_iseq_match_renamings;
  DYN_DBG2(PEEP_TAB, "Trying entry %ld\n", peep_entry->peep_entry_get_line_number());
  /* if ((peep_entry->peep_flags & peep_flags) != peep_flags) {
    DYN_DBG2(PEEP_TAB, "flag constraints not met. peep_entry->flags=0x%x, flags=0x%x\n",
        (unsigned)peep_entry->peep_flags, (unsigned)peep_flags);
    continue;
  } */

  prefix += string(".") + string(__func__);
  regset_t iseq_use, iseq_def;
  memset_t iseq_memuse, iseq_memdef;
  src_iseq_get_usedef(iseq, iseq_len, &iseq_use, &iseq_def, &iseq_memuse, &iseq_memdef);

  vector<src_iseq_match_renamings_t> renamings = peep_entry_match_src_side(peep_entry, iseq, iseq_len, live_outs, num_live_outs, iseq_use, iseq_def, iseq_memuse, iseq_memdef, prefix);

  for (const auto &src_iseq_match_renamings : renamings) {
    regset_t use, def, touch;
    src_iseq_get_usedef_regs(iseq, iseq_len, &use, &def);
    regset_copy(&touch, &use);
    regset_union(&touch, &def);

    regset_t st_use, st_touch;
    regset_inv_rename(&st_use, &use, &src_iseq_match_renamings.get_regmap());
    regset_inv_rename(&st_touch, &touch, &src_iseq_match_renamings.get_regmap());

    //if (iseq_len != peep_entry->src_iseq_len) {
    //  DYN_DBG2(PEEP_TAB, "iseq-len mismatch: actual %ld <-> peeptab %ld\n", iseq_len,
    //      peep_entry->src_iseq_len);
    //  return NULL;
    //}

    //imm_vt_map_t inv_imm_vt_map;
    //regmap_t inv_src_regmap;
    //nextpc_map_t nextpc_map;
    //imm_vt_map_t inv_imm_map;
    //memlabel_map_t inv_memlabel_map;

    //DYN_DEBUG(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": var = " << var << "\n");
    //if (var) {
    //  if (!src_iseq_matches_template_var(peep_entry->src_iseq, iseq, iseq_len,
    //          src_iseq_match_renamings/*&inv_src_regmap, &inv_imm_vt_map, &nextpc_map*/, __func__)) {
    //    DYN_DBG(PEEP_TAB, "%s", "src_iseq_matches_template_var() returned false\n");
    //    return NULL;
    //  }
    //} else {
      //if (!src_iseq_matches_template(peep_entry->src_iseq, iseq, iseq_len,
      //        src_iseq_match_renamings/*&inv_src_regmap, &inv_imm_map, &nextpc_map*/, __func__)) {
      //  DYN_DBG(PEEP_TAB, "%s", "src_iseq_matches_template() returned false\n");
      //  return NULL;
      //}
    //}

    //{
    //  /* Print debugging info */
    //  DYN_DBG(PEEP_TAB, "peep_entry at line %ld: All instructions match.\n",
    //      peep_entry->peep_entry_get_line_number());

    //  DYN_DBG(PEEP_TAB, "inv_src_regmap:\n%s\n",
    //      regmap_to_string(&src_iseq_match_renamings.get_regmap(), as1, sizeof as1));

    //  /*DYN_DBG2(PEEP_TAB, "inv_imm_map:\n%s\n",
    //      imm_map_to_string(&inv_imm_map, as1, sizeof as1));*/
    //}

    //if (!src_inv_regmap_agrees_with_regconst_map_possibilities(&src_iseq_match_renamings.get_regmap(),
    //      peep_entry->regconst_map_possibilities)) {
    //  DYN_DEBUG(PEEP_TAB, cout << "Rule at line " << peep_entry->peep_entry_get_line_number() << ": This rule requires a reg to be "
    //      "mapped to a const which is not permitted (checked against regconst_map_possibilities). Skipping this rule..\n"
    //      "regconst_map_possibilities:\n" << peep_entry->regconst_map_possibilities.to_string() << "\n"
    //      "inv_src_regmap:\n" << regmap_to_string(&src_iseq_match_renamings.get_regmap(), as2, sizeof as2) << "\n");
    //  return NULL;
    //}

    //DYN_DBG2(PEEP_TAB, "%s", "Calling src_inv_regmap_agrees_with_nomatch_pairs()\n");

    //if (!src_inv_regmap_agrees_with_nomatch_pairs(&src_iseq_match_renamings.get_regmap(),
    //      //peep_entry->nomatch_pairs, peep_entry->num_nomatch_pairs
    //      peep_entry->nomatch_pairs_set
    //   )) {
    //  DYN_DBG(PEEP_TAB, "Rule at line %ld: This rule requires aliasing of two "
    //      "un-aliasable registers. Skipping this rule..\nnomatch_pairs_set:\n%s\n"
    //      "inv_src_regmap:\n%s\n", peep_entry->peep_entry_get_line_number(),
    //      //nomatch_pairs_to_string(peep_entry->nomatch_pairs,
    //      //  peep_entry->num_nomatch_pairs, as1, sizeof as1),
    //      nomatch_pairs_set_to_string(peep_entry->nomatch_pairs_set,
    //        /*peep_entry->num_nomatch_pairs, */as1, sizeof as1),
    //      regmap_to_string(&src_iseq_match_renamings.get_regmap(), as2, sizeof as2));
    //  return NULL;
    //}

    //if (num_live_outs != peep_entry->num_live_outs) {
    //  DYN_DBG(PEEP_TAB, "Rule at line number %ld: Number of exits don't match. "
    //      "num_live_outs=%ld, peep_entry->num_live_outs=%ld\n", peep_entry->peep_entry_get_line_number(),
    //      num_live_outs, peep_entry->num_live_outs);
    //  return NULL;
    //}

    //regset_t templ_touch, templ_use;
    //memset_t templ_memuse, templ_memuse_renamed, templ_memdef, templ_memdef_renamed;
    //src_iseq_compute_touch_var(&templ_touch, &templ_use, &templ_memuse, &templ_memdef,
    //    peep_entry->src_iseq, peep_entry->src_iseq_len);
    //regset_erase_regs_mapped_to_constants(&templ_touch, &src_iseq_match_renamings.get_regmap());
    //regset_erase_regs_mapped_to_constants(&templ_use, &src_iseq_match_renamings.get_regmap());
    //memset_rename(&templ_memuse_renamed, &templ_memuse, &src_iseq_match_renamings.get_regmap(), &src_iseq_match_renamings.get_imm_vt_map());
    //memset_rename(&templ_memdef_renamed, &templ_memdef, &src_iseq_match_renamings.get_regmap(), &src_iseq_match_renamings.get_imm_vt_map());

    //DYN_DBG2(PEEP_TAB, "templ_touch: %s\n",
    //    regset_to_string(&templ_touch, as1, sizeof as1));

    //regset_t st_use, st_touch;
    ////memset_t st_memuse, st_memdef;
    //memset_t memuse, memdef;
    ////if (var) {
    ////  regset_t use, touch;
    ////  //memset_t memuse, memdef;
    ////  src_iseq_compute_touch_var(&touch, &use, &memuse, &memdef, iseq, iseq_len);
    ////  regset_inv_rename(&st_use, &use, &src_iseq_match_renamings.get_regmap());
    ////  regset_inv_rename(&st_touch, &touch, &src_iseq_match_renamings.get_regmap());
    ////  //memset_inv_rename(&st_memuse, &memuse, &inv_src_regmap);
    ////  //memset_inv_rename(&st_memdef, &memdef, &inv_src_regmap);
    ////} else {
    //  regset_t use, def, touch;
    //  //memset_t memuse, memdef;
    //  src_iseq_get_usedef(iseq, iseq_len, &use, &def, &memuse, &memdef);
    //  regset_copy(&touch, &use);
    //  regset_union(&touch, &def);
    //  DYN_DBG2(PEEP_TAB, "use: %s, def: %s, touch: %s\n",
    //      regset_to_string(&use, as1, sizeof as1),
    //      regset_to_string(&def, as2, sizeof as2),
    //      regset_to_string(&touch, as3, sizeof as3));

    //  regset_inv_rename(&st_use, &use, &src_iseq_match_renamings.get_regmap());
    //  regset_inv_rename(&st_touch, &touch, &src_iseq_match_renamings.get_regmap());
    //  //memset_inv_rename(&st_memuse, &memuse, &inv_src_regmap);
    //  //memset_inv_rename(&st_memdef, &memdef, &inv_src_regmap);
    ////}

    //DYN_DBG2(PEEP_TAB, "memuse = %s\n", memset_to_string(&memuse, as1, sizeof as1));
    //DYN_DBG2(PEEP_TAB, "memdef = %s\n", memset_to_string(&memdef, as1, sizeof as1));

    //DYN_DBG2(PEEP_TAB, "st_touch: %s\n",
    //    regset_to_string(&st_touch, as1, sizeof as1));

    //if (!regset_equals_coarsely(&templ_touch, &st_touch)) {
    //  DYN_DBG(PEEP_TAB, "Rule at line %ld: the touched register-set is not equal. "
    //      "templ_touch=%s\nst_touch=%s\n", peep_entry->peep_entry_get_line_number(),
    //      regset_to_string(&templ_touch, as1, sizeof as1),
    //      regset_to_string(&st_touch, as2, sizeof as2));
    //  return NULL;
    //}

    //if (   !memset_contains(&memuse, &templ_memuse_renamed)
    //    || !memset_contains(&templ_memuse_renamed, &memuse)
    //    || !memset_contains(&memdef, &templ_memdef_renamed)
    //    || !memset_contains(&templ_memdef_renamed, &memdef)) {
    //  DYN_DBG(PEEP_TAB, "Rule at line %ld: the st-used mem-set does not contain "
    //      "templ_memuse. "
    //      "templ_memuse_renamed=%s\nmemuse=%s\n", peep_entry->peep_entry_get_line_number(),
    //      memset_to_string(&templ_memuse_renamed, as1, sizeof as1),
    //      memset_to_string(&memuse, as2, sizeof as2));
    //  DYN_DBG(PEEP_TAB, "templ_memdef_renamed = %s.\n", memset_to_string(&templ_memuse_renamed, as1, sizeof as1));
    //  DYN_DBG(PEEP_TAB, "memdef = %s.\n", memset_to_string(&memdef, as1, sizeof as1));
    //  return NULL;
    //}

    //bool live_regs_mismatch = false;
    //long l;
    //for (l = 0; l < num_live_outs; l++) {
    //  regset_t plive_out;
    //  regset_t st_live_out;

    //  regset_copy(&plive_out, &live_outs[l]);
    //  DYN_DBG2(PEEP_TAB, "before intersect: live_out[%ld] = %s.\n", l, regset_to_string(&live_outs[l], as1, sizeof as1));
    //  //DYN_DBG2(PEEP_TAB, "st_touch = %s.\n", regset_to_string(&st_touch, as1, sizeof as1));
    //  //regset_intersect(&plive_out, &st_touch);
    //  //DYN_DBG2(PEEP_TAB, "after intersect: live_out[%ld] = %s.\n", l, regset_to_string(&live_outs[l], as1, sizeof as1));
    //  DYN_DBG2(PEEP_TAB, "inv_src_regmap = %s.\n", regmap_to_string(&src_iseq_match_renamings.get_regmap(), as1, sizeof as1));
    //  regset_inv_rename(&st_live_out, &plive_out, &src_iseq_match_renamings.get_regmap());
    //  DYN_DBG2(PEEP_TAB, "after inv_rename: st_live_out[%ld] = %s.\n", l, regset_to_string(&st_live_out, as1, sizeof as1));

    //  /* intersect with templ_touch, as it is possible that due to aliasing,
    //     the bits in st_live_out (and touch and st_live_out) are union of
    //     the bits present in the aliased vregs. */
    //  regset_intersect(&st_live_out, &templ_touch);

    //  DYN_DBG(PEEP_TAB, "Checking Live regs %ld. peep_entry->live_out[%ld] "
    //    "= %s ; st_live_out = %s\n", l, l,
    //    regset_to_string(&peep_entry->live_out[l], as1, sizeof as1),
    //    regset_to_string(&st_live_out, as2, sizeof as2));

    //  if (!regset_contains(&peep_entry->live_out[l], &st_live_out)) {
    //    DYN_DBG2(PEEP_TAB, "Rule at line number %ld: Live registers %ld dont match.\n",
    //        peep_entry->peep_entry_get_line_number(), l);
    //    live_regs_mismatch = true;
    //    break;
    //  }
    //}
    //if (live_regs_mismatch) {
    //  return NULL;
    //}

    transmap_t st_tmap;
    transmap_copy(&st_tmap, transmap_none());
    transmap_inv_rename(&st_tmap, in_tmap, &src_iseq_match_renamings.get_regmap());
    transmap_t ost_tmap[num_live_outs];
    for (size_t i = 0; i < num_live_outs; i++) {
      transmap_copy(&ost_tmap[i], transmap_none());
      transmap_inv_rename(&ost_tmap[i], &out_tmaps[i], &src_iseq_match_renamings.get_regmap());
      DYN_DEBUG2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": out_tmaps[" << i << "] =\n" << transmap_to_string(&out_tmaps[i], as1, sizeof as1) << endl);
      DYN_DEBUG2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": ost_tmap[" << i << "] =\n" << transmap_to_string(&ost_tmap[i], as1, sizeof as1) << endl);
    }
    //cout << __func__ << " " << __LINE__ << ": in_tmap =\n" << transmap_to_string(in_tmap, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": inv_src_regmap =\n" << regmap_to_string(&inv_src_regmap, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": st_tmap =\n" << transmap_to_string(&st_tmap, as1, sizeof as1) << endl;


    if (!transmap_matches_template_on_regset(peep_entry->get_in_tmap(), &st_tmap,
          &st_use)) {
      DYN_DBG(PEEP_TAB, "Rule at line number %ld: Transmaps don't match on use "
          "regs\n", peep_entry->peep_entry_get_line_number());
      DYN_DBG(PEEP_TAB, "Printing peep_entry->in_tmap:\n%s",
          transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "Printing st_tmap:\n%s",
          transmap_to_string(&st_tmap, as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "use:\n%s", regset_to_string(&st_use, as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "in_tmap:\n%s", transmap_to_string(in_tmap, as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "inv_src_regmap:\n%s",
          regmap_to_string(&src_iseq_match_renamings.get_regmap(), as1, sizeof as1));
      continue;
    }

    DYN_DBG2(PEEP_TAB, "Transmaps match on use regs:\nst_use: %s\n"
        "peep_entry->in_tmap:\n%s\nst_tmap:\n%s\n",
        regset_to_string(&st_use, as1, sizeof as1),
        transmap_to_string(peep_entry->get_in_tmap(), as2, sizeof as2),
        transmap_to_string(&st_tmap, as3, sizeof as3));

    regcount_t l_touch_not_live_in;
    regset_t touch_not_live_in_regs_renamed;
    regmap_t inv_dst_regmap;
    memslot_map_t inv_memslot_map(-1);
    transmaps_get_inv_dst_regmap_from_inv_src_regmap(peep_entry->get_in_tmap(), peep_entry->out_tmap, num_live_outs, &src_iseq_match_renamings.get_regmap(), &inv_dst_regmap, &inv_memslot_map);
    regset_rename(&touch_not_live_in_regs_renamed, &peep_entry->touch_not_live_in_regs, &inv_dst_regmap);
    regset_count(&l_touch_not_live_in, &touch_not_live_in_regs_renamed);

    if (!transmap_agrees_with_temporaries_count(in_tmap, &l_touch_not_live_in)) {
      DYN_DBG(PEEP_TAB, "Rule at line number %ld: transmap does not agree with "
          "touch_not_live_in. transmap:\n%s\ntouch_not_live_in_regs_renamed:\n%s\n",
          peep_entry->peep_entry_get_line_number(), transmap_to_string(in_tmap, as1, sizeof as1),
          regset_to_string(&touch_not_live_in_regs_renamed, as2, sizeof as2));
      continue;
    }

    /*if (dst_num_available_regs - transmap_count_regallocs(in_tmap)
         < temporaries_num_gprs(&peep_entry->temporaries)) {
      DYN_DBG2(PEEP_TAB, "Rule at line number %ld: Error: dst_num_available_regs=%d, "
          "regallocs=%d, temporaries_num_gprs=%d\n", peep_entry->peep_entry_get_line_number(),
          dst_num_available_regs, transmap_count_regallocs(in_tmap),
          temporaries_num_gprs(&peep_entry->temporaries));
      NOT_REACHED();
      return NULL;
    }*/


    /* Everything is a match. */
    if (   /*!var
        && */!transmap_matches_template_on_regset(peep_entry->get_in_tmap(), &st_tmap,
               &st_touch)) {
      /* While we found a match, it is certainly a duplicate of a match we
         will find else-where. Hence, ignore this match. */

      /* more detail: the min-cost match reg-allocates some def'ed registers.
         This translation will be duplicated while trying reg-allocation of
         def'd register in in_tmap. Hence, return COST_INFINITY for this
         particular st_tmap (to avoid duplicates and hence improve performance)
         */
      DYN_DBG(PEEP_TAB, "Rule at line number %ld: transmap_matches_template() "
          "failed.\npeep_entry->in_tmap:\n%s\nst_tmap:\n%s\nst_touch:\n%s\n",
          peep_entry->peep_entry_get_line_number(),
          transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1),
          transmap_to_string(&st_tmap, as2, sizeof as2),
          regset_to_string(&st_touch, as3, sizeof as3));
      continue;
    }

    DYN_DBG2(PEEP_TAB, "Transmaps match on touched regs. peep_entry->"
        "union transmap:\n%s\nst_tmap:\n%s\nst_touch=%s\n",
        transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1),
        transmap_to_string(&st_tmap, as2, sizeof as2),
        //regset_to_string(&touch, as3, sizeof as3),
        regset_to_string(&st_touch, as4, sizeof as4));

    DYN_DBG(PEEP_TAB, "%s", "Everything match!\n");
    DYN_DBG(PEEP_TAB, "peep_entry->src_iseq =\n%s\n", src_iseq_to_string(peep_entry->src_iseq, peep_entry->src_iseq_len, as1, sizeof as1));

    ASSERT(out_tmaps);

    regmap_t dst_regmap;
    { //compute dst_regmap
      DYN_DBG2(PEEP_TAB, "found a candidate entry:\nin_tmap:\n%s",
          //dst_iseq ? dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1) : "null",
          transmap_to_string(peep_entry->get_in_tmap(), as2, sizeof as2));
      regmap_init(&dst_regmap);
      transmaps_diff(&dst_regmap, peep_entry->get_in_tmap(), &st_tmap, regset_all());
      for (size_t i = 0; i < num_live_outs; i++) {
        transmaps_diff(&dst_regmap, &peep_entry->out_tmap[i], &ost_tmap[i], regset_all());
      }
      DYN_DBG2(PEEP_TAB, "peep_entry->in_tmap:\n%s\n",
          transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1));
      DYN_DBG2(PEEP_TAB, "st_tmap:\n%s\n",
          transmap_to_string(&st_tmap, as1, sizeof as1));
      DYN_DBG2(PEEP_TAB, "before adding constant mappings: dst_regmap:\n%s\n", regmap_to_string(&dst_regmap, as1, sizeof as1));
      regmap_add_constant_mappings_using_transmap(&dst_regmap, peep_entry->get_in_tmap(), &src_iseq_match_renamings.get_regmap());
      DYN_DBG2(PEEP_TAB, "after adding constant mappings: dst_regmap:\n%s\n", regmap_to_string(&dst_regmap, as1, sizeof as1));

      regset_t tmap_used;
      regcons_t regcons;
      regset_clear(&tmap_used);
      transmap_get_used_dst_regs(in_tmap, &tmap_used);
      DYN_DBG(PEEP_TAB, "in_tmap:\n%s\n", transmap_to_string(in_tmap, as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "peep_in_tmap:\n%s\n", peep_in_tmap ? transmap_to_string(peep_in_tmap, as1, sizeof as1) : "(null)");
      //DYN_DBG(PEEP_TAB, "tmap_used:\n%s\n", regset_to_string(&tmap_used, as1, sizeof as1));
      //if (var) {
      //  regcons_set(&regcons);
      //} else {
        regcons_copy(&regcons, peep_entry->in_tcons);
      //}
      DYN_DBG(PEEP_TAB, "before unmark, regcons: %s\n", regcons_to_string(&regcons, as1, sizeof as1));
      regcons_unmark_using_regset(&regcons, &tmap_used); //this is necessary to ensure that the temporary registers used in the dst_iseq are appropriately renamed to avoid conflict with tmap_used regs
      DYN_DBG(PEEP_TAB, "after unmark, regcons: %s\n", regcons_to_string(&regcons, as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "dst_regmap:\n%s\n", regmap_to_string(&dst_regmap, as1, sizeof as1));
      bool ret = regmap_assign_using_regcons(&dst_regmap, &regcons, ARCH_DST, NULL, fixed_reg_mapping_t());
      //ASSERT(ret); //it may not have assigned all registers, but it would have assigned
                     //the first few (used registers in dst_iseq) and that's what matters
                     //for dst_iseq_rename(). so allow ret to be false.
      DYN_DBG2(PEEP_TAB, "after assigning temps, dst_regmap:\n%s\n",
          regmap_to_string(&dst_regmap, as1, sizeof as1));
    }

    for (size_t l = 0; l < num_live_outs; l++) {
      transmap_t tmp_tmap;

      transmap_copy(&out_tmaps[l], in_tmap);

      /* first subtract peep_entry's in_tmap */
      transmap_copy(&tmp_tmap, transmap_none());
      transmap_rename(&tmp_tmap, peep_entry->get_in_tmap(), &src_iseq_match_renamings.get_regmap());
      //cout << __func__ << __LINE__ << ": tmp_tmap =\n" << transmap_to_string(&tmp_tmap, as1, sizeof as1) << endl;
      //cout << __func__ << __LINE__ << ": before: out_tmaps[l] = \n" << transmap_to_string(&out_tmaps[l], as1, sizeof as1) << endl;
      transmaps_subtract(&out_tmaps[l], &out_tmaps[l], &tmp_tmap);
      //cout << __func__ << __LINE__ << ": after: out_tmaps[l] = \n" << transmap_to_string(&out_tmaps[l], as1, sizeof as1) << endl;

      /* then add peep_entry's out_tmap */
      transmap_copy(&tmp_tmap, transmap_none());
      transmap_rename(&tmp_tmap, &peep_entry->out_tmap[l], &src_iseq_match_renamings.get_regmap());
      transmap_rename_using_dst_regmap(&tmp_tmap, &dst_regmap, NULL);
      //cout << __func__ << __LINE__ << ": peep_entry->out_tmap =\n" << transmap_to_string(&peep_entry->out_tmap[l], as1, sizeof as1) << endl;
      //cout << __func__ << __LINE__ << ": inv_src_regmap =\n" << regmap_to_string(&inv_src_regmap, as1, sizeof as1) << endl;
      //cout << __func__ << __LINE__ << ": tmp_tmap =\n" << transmap_to_string(&tmp_tmap, as1, sizeof as1) << endl;
      //cout << __func__ << __LINE__ << ": before: out_tmaps[l] = \n" << transmap_to_string(&out_tmaps[l], as1, sizeof as1) << endl;
      transmaps_union(&out_tmaps[l], &out_tmaps[l], &tmp_tmap);
      transmap_intersect_with_regset(&out_tmaps[l], &live_outs[l]);
      //cout << __func__ << __LINE__ << ": after: out_tmaps[l] = \n" << transmap_to_string(&out_tmaps[l], as1, sizeof as1) << endl;
      /*transmap_apply_diff(&out_tmaps[l], peep_entry->in_tmap,
          &peep_entry->out_tmap[l], &inv_src_regmap, &def);*/
    }

    in_tmap->transmap_update_modifiers_using_peep_tmap_and_regmap(peep_entry->get_in_tmap(), src_iseq_match_renamings.get_regmap());

    if (!dst_iseq) {
      /* we are only interested in the cost */
      return compute_peep_match_cost(peep_entry->cost, src_iseq_match_renamings);
    }

    /*if (!var) */{
      ASSERT(peep_entry->in_tcons);
      ASSERT(peep_in_tmap);
      ASSERT(peep_out_tmaps);
      ASSERT(peep_regcons);
      ASSERT(peep_temp_regs_used);
      ASSERT(peep_nomatch_pairs_set);
      ASSERT(touch_not_live_in);
      regcons_copy_using_transmaps_and_src_regmap(peep_regcons, peep_entry->in_tcons,
          peep_entry->get_in_tmap(), peep_entry->out_tmap, num_live_outs, &src_iseq_match_renamings.get_regmap());
      nomatch_pairs_set_copy_using_regmap(peep_nomatch_pairs_set, &peep_entry->nomatch_pairs_set,
          &src_iseq_match_renamings.get_regmap());
      DYN_DBG(PEEP_TAB, "peep_entry->in_tcons:\n%s\n",
          regcons_to_string(peep_entry->in_tcons, as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "peep_regcons:\n%s\n", regcons_to_string(peep_regcons, as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "peep_entry->in_tmap:\n%s\n",
          transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "inv_src_regmap:\n%s\n",
          regmap_to_string(&src_iseq_match_renamings.get_regmap(), as1, sizeof as1));
      transmap_copy(peep_in_tmap, in_tmap);
      transmap_unassign_regs(peep_in_tmap);
      DYN_DBG(PEEP_TAB, "peep_entry->in_tmap:\n%s\n", transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1));
      DYN_DBG(PEEP_TAB, "inv_src_regmap:\n%s\n", regmap_to_string(&src_iseq_match_renamings.get_regmap(), as1, sizeof as1));
      transmap_rename(peep_in_tmap, peep_entry->get_in_tmap(), &src_iseq_match_renamings.get_regmap());
      DYN_DBG(PEEP_TAB, "peep_in_tmap:\n%s\n", transmap_to_string(peep_in_tmap, as1, sizeof as1));

      //regset_t live_in_or_out_all, out_tmaps_used_all, use, def;
      //regset_clear(&out_tmaps_used_all);
      //regset_clear(&live_in_or_out_all);
      //src_iseq_get_usedef_regs(iseq, iseq_len, &use, &def);
      //regset_union(&live_in_or_out_all, &use);
      regset_t peep_dst_regs_used;
      regset_clear(&peep_dst_regs_used);
      transmap_get_used_dst_regs(peep_in_tmap, &peep_dst_regs_used);
      for (size_t l = 0; l < num_live_outs; l++) {
        transmap_copy(&peep_out_tmaps[l], &out_tmaps[l]);
        transmap_unassign_regs(&peep_out_tmaps[l]);
        transmap_rename(&peep_out_tmaps[l], &peep_entry->out_tmap[l], &src_iseq_match_renamings.get_regmap());
        //peep_out_tmaps[l].transmap_get_assigned_src_regs(out_tmaps_used_all);
        transmap_get_used_dst_regs(&peep_out_tmaps[l], &peep_dst_regs_used);
               transmap_intersect_with_regset(&peep_out_tmaps[l], &live_outs[l]);
        //regset_union(&live_in_or_out_all, &live_outs[l]);
      }
      regset_copy(peep_temp_regs_used, &peep_entry->temp_regs_used);
      regset_t dst_regs_used = transmaps_get_used_dst_regs(peep_in_tmap, peep_out_tmaps, num_live_outs);
      regset_diff(&peep_dst_regs_used, &dst_regs_used);
      regset_union(peep_temp_regs_used, &peep_dst_regs_used); //identify regs that are not used in peep_* transmaps (perhaps because they are not live) but used in peep_entry->*_tmaps, as temp_regs
      //regset_t out_tmaps_dead_regs = out_tmaps_used_all;
      //regset_diff(&out_tmaps_dead_regs, &live_in_or_out_all);
      //regset_inv_rename(&out_tmaps_dead_regs, &out_tmaps_dead_regs, &inv_src_regmap);
      //for (const auto &g : out_tmaps_dead_regs.excregs) {
      //  for (const auto &r : g.second) {
      //    ASSERT(num_live_outs > 0);
      //    if (   !peep_entry->out_tmap[0].extable.count(g.first)
      //        || !peep_entry->out_tmap[0].extable.at(g.first).count(r.first)) {
      //      continue;
      //    }
      //    transmap_entry_t tmap_entry_intersected = peep_entry->out_tmap[0].extable.at(g.first).at(r.first);
      //    for (l = 0; l < num_live_outs; l++) {
      //      if (   !peep_entry->out_tmap[l].extable.count(g.first)
      //          || !peep_entry->out_tmap[l].extable.at(g.first).count(r.first)) {
      //        tmap_entry_intersected.clear();
      //      } else {
      //        transmap_entry_t const &tmap_entry = peep_entry->out_tmap[l].extable.at(g.first).at(r.first);
      //        tmap_entry_intersected.transmap_entry_intersect(tmap_entry);
      //      }  
      //    }  
      //    for (const auto &loc : tmap_entry_intersected.get_locs()) {
      //      if (   loc.get_loc() >= TMAP_LOC_EXREG(0)
      //          && loc.get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
      //        exreg_group_id_t dst_group = loc.get_loc() - TMAP_LOC_EXREG(0);
      //        reg_identifier_t const &dst_reg = loc.get_reg_id();
      //        regset_mark_ex_mask(peep_temp_regs_used, dst_group, dst_reg, MAKE_MASK(DST_EXREG_LEN(dst_group)));
      //      }
      //    }
      //  }
      //}
      //regcount_copy(touch_not_live_in, &peep_entry->touch_not_live_in);
      regcount_copy(touch_not_live_in, &l_touch_not_live_in);
    }

    ASSERT(peep_entry->dst_iseq_len < dst_iseq_size);
    int j;
    for (j = 0; j < peep_entry->dst_iseq_len; j++) {
      memcpy(&dst_iseq[j], &peep_entry->dst_iseq[j], sizeof dst_iseq[j]);
    }
    *dst_iseq_len = peep_entry->dst_iseq_len;

    DYN_DBG2(PEEP_TAB, "peep_entry->in_tmap:\n%s\npeep_entry->in_tcons:\n%s\n",
        transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1),
        regcons_to_string(peep_entry->in_tcons, as2, sizeof as2));

    transmap_rename_using_dst_regmap(peep_in_tmap, &dst_regmap, NULL);
    for (size_t i = 0; i < num_live_outs; i++) {
      transmap_rename_using_dst_regmap(&peep_out_tmaps[i], &dst_regmap, NULL);
    }

    memslot_map_t memslot_map(base_reg);
    /*if (!var) */{
      DYN_DEBUG2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": st_tmap =\n" << transmap_to_string(&st_tmap, as1, sizeof as1) << endl);
      DYN_DEBUG2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": peep_entry->get_in_tmap =\n" << transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1) << endl);
      populate_memslot_map_from_peep_tmap_and_actual_tmap(memslot_map, peep_entry->get_in_tmap(), &st_tmap, &src_iseq_match_renamings.get_regmap(), base_reg);
      for (size_t i = 0; i < num_live_outs; i++) {
        DYN_DEBUG2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": ost_tmap[" << i << "] =\n" << transmap_to_string(&ost_tmap[i], as1, sizeof as1) << endl);
        DYN_DEBUG2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": peep_entry->get_out_tmap[" << i << "] =\n" << transmap_to_string(&peep_entry->out_tmap[i], as1, sizeof as1) << endl);
        populate_memslot_map_from_peep_tmap_and_actual_tmap(memslot_map, &peep_entry->out_tmap[i], &ost_tmap[i], &src_iseq_match_renamings.get_regmap(), base_reg);
      }
      DYN_DEBUG2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": calling dst_iseq_rename_var(): peep_in_tmap =\n" << transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1) << endl << "dst_iseq =\n" << dst_iseq_to_string(dst_iseq, *dst_iseq_len, as2, sizeof as2) << endl);
      DYN_DEBUG2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": memslot_map =\n" << memslot_map.memslot_map_to_string() << endl);
    }

    //if (var) {
    //  dst_iseq_rename_var(dst_iseq, *dst_iseq_len, &dst_regmap, &src_iseq_match_renamings.get_imm_vt_map(),
    //      &memslot_map, &src_iseq_match_renamings.get_regmap(), &src_iseq_match_renamings.get_nextpc_map(), nextpcs, num_live_outs);
    //} else {
      dst_iseq_rename(dst_iseq, *dst_iseq_len, &dst_regmap, &src_iseq_match_renamings.get_imm_vt_map(),
          &memslot_map, &src_iseq_match_renamings.get_regmap(), &src_iseq_match_renamings.get_nextpc_map(), nextpcs, num_live_outs);
    //}
    DYN_DBG2(PEEP_TAB, "after rename, dst_iseq:\n%s\n",
        dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1));

    return compute_peep_match_cost(peep_entry->cost, src_iseq_match_renamings);
  }
  return COST_INFINITY;
}


static struct peep_entry_t const *
__peep_search(peep_table_t const *tab,
    peep_entry_t const *peeptab_entry_id,
    src_insn_t const *iseq,
    long iseq_len, transmap_t *in_tmap, dst_insn_t *dst_iseq,
    long *dst_iseq_len, long dst_iseq_size,
    transmap_t *out_tmaps, nextpc_t const *nextpcs,
    /*src_ulong const *insn_pcs, */regset_t const *live_outs, long num_live_outs,
    transmap_t *peep_in_tmap, transmap_t *peep_out_tmaps,
    regcons_t *peep_regcons,
    regset_t *peep_temp_regs_used,
    nomatch_pairs_set_t *peep_nomatch_pairs_set,
    regcount_t *touch_not_live_in, int base_reg,
    string prefix
    )
{
  autostop_timer func_timer(__func__);
  if (!tab->hash_size) {
    *dst_iseq_len = 0;
    ASSERT(!peeptab_entry_id);
    return NULL;
  }
  if (peeptab_entry_id) {
    cost_t peep_match_cost;
    peep_match_cost = peep_entry_match(peeptab_entry_id, iseq, iseq_len, in_tmap, dst_iseq, dst_iseq_len, dst_iseq_size, out_tmaps, nextpcs, live_outs, num_live_outs, peep_in_tmap, peep_out_tmaps, peep_regcons, peep_temp_regs_used, peep_nomatch_pairs_set, touch_not_live_in, base_reg, prefix);
    ASSERT(peep_match_cost != COST_INFINITY);
    return peeptab_entry_id;
  }
  long i = src_iseq_hash_func_after_canon(iseq, iseq_len, tab->hash_size);

  DYN_DBG(peep_tab_add, "Searching index %ld\n", i);
  DYN_DBG2(PEEP_TAB, "Searching index %ld for \n\tsrc_iseq:\n%s\tin_tmap:\n%s",
      i, src_iseq_to_string(iseq, iseq_len, as1, sizeof as1),
      transmap_to_string(in_tmap, as2, sizeof as2));

  /* This list is sorted in order of increasing cost. Simply look for the first match */
  peep_entry_t const *ret_peep_entry = NULL;
  cost_t min_peep_match_cost = COST_INFINITY;
  for (auto iter = tab->hash[i].begin(); iter != tab->hash[i].end(); iter++) {
    peep_entry_t const *peep_entry = *iter;
    cost_t peep_match_cost = peep_entry_match(peep_entry, iseq, iseq_len, in_tmap, dst_iseq, dst_iseq_len, dst_iseq_size, out_tmaps, nextpcs, live_outs, num_live_outs, peep_in_tmap, peep_out_tmaps, peep_regcons, peep_temp_regs_used, peep_nomatch_pairs_set, touch_not_live_in, base_reg, prefix);
    DYN_DEBUG2(PEEP_TAB,
      if (peep_match_cost != COST_INFINITY) {
        printf("%s() %d: found match with peep_entry linenum %ld\n", __func__, __LINE__, peep_entry->peep_entry_get_line_number());
      }
    );

    if (peep_match_cost < min_peep_match_cost) {
      DYN_DBG2(PEEP_TAB, "setting peep_entry linenum %ld as the mincost return value\n", peep_entry->peep_entry_get_line_number());
      min_peep_match_cost = peep_match_cost;
      ret_peep_entry = peep_entry;
    }
  }
  return ret_peep_entry;
}

static struct peep_entry_t const *
peep_search(peep_table_t const *tab,
    struct peep_entry_t const *peeptab_entry_id,
    src_insn_t const *iseq,
    long iseq_len, transmap_t *in_tmap, dst_insn_t *dst_iseq,
    long *dst_iseq_len, long dst_iseq_size, transmap_t *out_tmaps,
    nextpc_t const *nextpcs,
    regset_t const *live_outs,
    long num_live_outs,
    transmap_t *peep_in_tmap, transmap_t *peep_out_tmaps,
    regcons_t *peep_regcons,
    regset_t *peep_temp_regs_used,
    nomatch_pairs_set_t *peep_nomatch_pairs_set,
    regcount_t *touch_not_live_in, int base_reg,
    string prefix
    )
{
  autostop_timer func_timer(__func__);
  src_insn_t canon_iseq[iseq_len];
  long i, peepcost;

  for (i = 0; i < iseq_len; i++) {
    src_insn_copy(&canon_iseq[i], &iseq[i]);
  }
  iseq_len = src_iseq_canonicalize(canon_iseq, nullptr, iseq_len);

  struct peep_entry_t const *peep_entry;
  peep_entry = __peep_search(tab, peeptab_entry_id, canon_iseq, iseq_len, in_tmap,
      dst_iseq, dst_iseq_len, dst_iseq_size, out_tmaps, nextpcs,
      live_outs, num_live_outs,
      peep_in_tmap, peep_out_tmaps, peep_regcons, peep_temp_regs_used,
      peep_nomatch_pairs_set, touch_not_live_in, base_reg, prefix);

  DYN_DEBUG2(PEEP_TAB,
      if (dst_iseq) {
        /*if (peepcost != COST_INFINITY) */{
          printf("%s() %d: src_iseq: %s", __func__, __LINE__,
              src_iseq_to_string(iseq, iseq_len, as1, sizeof as1));
          printf("dst_iseq: %s",
              dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1));
        }
      });

  return peep_entry;
}

/*static bool
find_latest_peeptab(char *buf, size_t size)
{
  char **peeptabs, **cur, *latest = NULL;
  char prefix[256];
  bool ret;

  snprintf(prefix, sizeof prefix, "peep.trans.tab.superoptdb.");
  list_all_dirents(&peeptabs, ABUILD_DIR, prefix);
  for (cur = peeptabs; *cur; cur++) {
    char path[strlen(ABUILD_DIR) + strlen(*cur) + 512];
    char const *timestamp_str;
    int s, fd;

    snprintf(path, sizeof path, ABUILD_DIR "/%s", *cur);
    if ((fd = open(path, O_RDONLY)) < 0) {
      continue;
    }
    if (file_length(fd) == 0) {
      continue;
    }
    s = strstart(*cur, "peep.trans.tab.superoptdb.", &timestamp_str);
    ASSERT(s);
    if (!timestamp_str) {
      continue;
    }
    if (latest && time_since_beginning(timestamp_str) > time_since_beginning(latest)) {
      latest = *cur;
    }
  }
  if (!latest) {
    ret = false;
  } else {
    ret = true;
    ASSERT(strlen(latest) < size);
    snprintf(buf, size, "%s", latest);
  }
  free_listing(peeptabs);
  return ret;
}*/

static bool
peep_entry_src_iseq_less(struct rbtree_elem const *_a, struct rbtree_elem const *_b,
    void *aux)
{
  peep_entry_t const *a, *b;
  long i;

  a = rbtree_entry(_a, peep_entry_t, rb_elem);
  b = rbtree_entry(_b, peep_entry_t, rb_elem);

  for (i = 0; i < a->src_iseq_len; i++) {
    if (i == b->src_iseq_len) {
      return false;
    }
    if (src_insn_less(&b->src_iseq[i], &a->src_iseq[i])) {
      return false;
    } else if (src_insn_less(&a->src_iseq[i], &b->src_iseq[i])) {
      return true;
    }
  }
  if (a->src_iseq_len == b->src_iseq_len) {
    return false; //equal
  }
  ASSERT(a->src_iseq_len < b->src_iseq_len);
  return true;
}

void
peep_table_init(peep_table_t *tab)
{
#define DEFAULT_PEEPTAB_HASH_SIZE 65537
  /*tab->peep_entries = NULL;
  tab->peep_entries_size = 0;*/
  tab->num_peep_entries = 0;
  //tab->hash = (peep_entry_t **)malloc(DEFAULT_PEEPTAB_HASH_SIZE * sizeof(peep_entry_t *));
  tab->hash = new peep_entry_list_t[DEFAULT_PEEPTAB_HASH_SIZE];
  ASSERT(tab->hash);
  tab->hash_size = DEFAULT_PEEPTAB_HASH_SIZE;
  //memset(tab->hash, 0, DEFAULT_PEEPTAB_HASH_SIZE * sizeof(peep_entry_t *));
  rbtree_init(&tab->rbtree, peep_entry_src_iseq_less, NULL);
}

static int
int_compare(const void *_a, const void  *_b)
{
  int a = *(int *)_a;
  int b = *(int *)_b;

  return (a - b);
}

static long long
compute_cutoff_cost(transmap_set_elem_t *head, double frac_retained)
{
  int count;
  transmap_set_elem_t const *iter;
  for (count = 0, iter = head; iter; iter = iter->next, count++);

  ASSERT(frac_retained <= 1);

  if (frac_retained == 1 || count == 0)
    return COST_INFINITY;

  int *cost_vector = (int *)malloc(count * sizeof (int));
  ASSERT (cost_vector);
  for (count = 0, iter = head; iter; iter = iter->next, count++) {
    cost_vector[count] = iter->cost;
  }
  qsort (cost_vector, count, sizeof(int), &int_compare);

  int index = count*frac_retained;
  int ret = cost_vector[index];

  /*dbg ("count = %d, frac_retained = %f, index = %d, ret = 0x%x\n", count,
      frac_retained, index, ret);*/
  free(cost_vector);
  return ret;
}

typedef struct transmap_elem_sort_struct_t
{
  transmap_set_elem_t *elem;
  long cost;
} transmap_elem_sort_struct_t;

static int
transmap_elem_sort_struct_compare(const void *_a, const void *_b)
{
  transmap_elem_sort_struct_t const *a = (transmap_elem_sort_struct_t *)_a;
  transmap_elem_sort_struct_t const *b = (transmap_elem_sort_struct_t *)_b;
  return (a->cost < b->cost) ? -1 : (a->cost == b->cost) ? 0 : 1;
}

transmap_set_elem_t *
prune_transmaps(transmap_set_elem_t *ls, long max_size, transmap_set_elem_t *&pruned_out_list)
{
  transmap_set_elem_t *cur, *prev, *ret;
  long num_elems = 0;

  for (cur = ls; cur; cur = cur->next) {
    DYN_DBG(PRUNE, "cur->cost = %lx\n", cur->cost);
    num_elems++;
  }
  DYN_DBG(PRUNE, "before pruning: num_elems=%ld\n", num_elems);
  transmap_elem_sort_struct_t *transmap_elems =
      (transmap_elem_sort_struct_t *)malloc(num_elems *
          sizeof(transmap_elem_sort_struct_t));
  ASSERT(transmap_elems);

  long i = 0, num_new_elems;
  for (cur = ls; cur; cur = cur->next) {
    transmap_elems[i].elem = cur;
    transmap_elems[i].cost = cur->cost;
    i++;
    ASSERT(i <= num_elems);
  }
  ASSERT(i == num_elems);

  qsort(transmap_elems, num_elems, sizeof(transmap_elem_sort_struct_t),
      transmap_elem_sort_struct_compare);

  cur = NULL;
  ret = NULL;
  num_new_elems = num_elems;
  /* sort the row */
  for (i = 0; i < num_elems; i++) {
    if (!cur) {
      ret = transmap_elems[i].elem;
      cur = ret;
    } else {
      ASSERT(!cur->next);
      cur->next = transmap_elems[i].elem;
      cur = cur->next;
    }
    ASSERT(cur);
    ASSERT(cur->cost == transmap_elems[i].cost);
    cur->next = NULL;
  }

  free(transmap_elems);

  /*if (max_size < 0) {
    DYN_DBG(PRUNE, "after pruning: num_new_elems=%ld\n", num_new_elems);
    return ret;
  }*/

  for (i = 0, cur = ret, prev = NULL;
      cur && (max_size >= 0 && i < max_size) && cur->cost < COST_INFINITY;
      prev = cur, cur = cur->next, i++);

  ASSERT(!cur || cur->cost == COST_INFINITY || (max_size < num_elems && i == max_size));
  if (prev) {
    prev->next = NULL;
  } else {
    ret = NULL;
  }
  while (cur) {
    DYN_DBG(PRUNE, "removing cur with cost = %ld\n", cur->cost);
    transmap_set_elem_t *next = cur->next;
    //transmap_set_elem_free(cur);
    cur->next = pruned_out_list;
    pruned_out_list = cur;
    cur = next;
    num_new_elems--;
  }
  DYN_DBG(PRUNE, "after pruning with %ld: num_new_elems=%ld\n", max_size,
      num_new_elems);
  return ret;
}

/*void
regmap_invert(regmap_t *inverted, regmap_t const *orig)
{
  int i;
  regmap_init(inverted);
  for (i = 0; i < SRC_NUM_REGS; i++) {
    if (orig->table[i] != -1 ) {
      inverted->table[orig->table[i]] = i;
    }
  }
}*/

static void
init_pred_row_elem(transmap_set_elem_t *elem)
{
  elem->out_tmaps = NULL;
  elem->next = NULL;
  elem->cost = 0/*COST_INFINITY*/;
  elem->num_decisions = 0;
  elem->decisions = NULL;
  //elem->in_tcons = NULL;
  elem->num_temporaries = 0;
}

static void
pred_row_print(long pc, long fetchlen, transmap_set_elem_t const *pred_row)
{
  transmap_set_elem_t const *iter;
  long j;

  j = 0;
  printf("%lx fetchlen %ld: PRINTING row:\n", pc, fetchlen);
  for (iter = pred_row; iter; iter = iter->next) {
    //ASSERT(!iter->out_tmaps);
    printf("Transmap %ld:\n%s\n", j++,
        transmap_to_string(iter->in_tmap, as1, sizeof as1));
  }
}

transmap_t const *
find_out_tmap_for_nextpc(transmap_set_elem_t const *iter, src_ulong pc)
{
  long i;

  if (!iter->row) {
    /* this must be the jumptable transmap. */
    ASSERT(!iter->in_tmap);
    return &iter->out_tmaps[0];
  }
  ASSERT(iter->row);
  for (i = 0; i < iter->row->num_nextpcs; i++) {
    if (iter->row->nextpcs[i].get_val() == pc) {
      return &iter->out_tmaps[i];
    }
  }
  NOT_REACHED();
}

static bool
transmap_contains_mapping(transmap_t const *tmap, int src_group, reg_identifier_t const &src_regnum,
    transmap_loc_id_t dst_loc)
{
  transmap_entry_t const *tmap_entry;
  //int /*l, */dst_loc;

  //ASSERT(dst_group == -1 || (dst_group >= 0 && dst_group < DST_NUM_EXREG_GROUPS));
  //ASSERT(dst_group >= 0 && dst_group < DST_NUM_EXREG_GROUPS);
  tmap_entry = transmap_get_elem(tmap, src_group, src_regnum);
  //dst_loc = TMAP_LOC_EXREG(dst_group);
  /*for (l = 0; l < tmap_entry->num_locs; l++) {
    if (tmap_entry->loc[l] == dst_loc) {
      return true;
    }
  }*/
  for (const auto &l : tmap_entry->get_locs()) {
    if (l.get_loc() == dst_loc) {
      return true;
    }
  }
  return false;
}

static void
transmap_spill_regs_for_matching_dst_groups(transmap_t *tmap,
    live_range_sort_struct_t const *sorted_regs,
    int num_sorted_regs, int cur_count, int desired_count,
    //live_ranges_t const *live_ranges,
    transmap_t const *touch_tmap,
    std::function<bool (transmap_entry_t const &, transmap_loc_id_t &)> match_fn
    /*int group*/)
{
  int i, j, l, src_group;
  transmap_entry_t *tmap_entry;
  //bool found;

  if (desired_count >= cur_count) {
    return;
  }
  //cout << __func__ << " " << __LINE__ << ": entry: tmap =\n" << transmap_to_string(tmap, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": entry: touch_tmap =\n" << transmap_to_string(touch_tmap, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": entry: desired_count = " << desired_count << ", cur_count = " << cur_count << endl;
  for (i = 0; i < num_sorted_regs && desired_count < cur_count; i++) {
    src_group = sorted_regs[i].group;
    reg_identifier_t const &src_regnum = sorted_regs[i].regnum;
    //cout << __func__ << " " << __LINE__ << ": looking at " << src_group << ", " << src_regnum.reg_identifier_to_string() << endl;

    //found = false;
    tmap_entry = &tmap->extable[src_group][src_regnum];
    //ASSERT(group >= 0 && group < DST_NUM_EXREG_GROUPS);
    transmap_loc_id_t tmap_loc;
    if (match_fn(*tmap_entry, tmap_loc)) {
      if (   touch_tmap
          && transmap_contains_mapping(touch_tmap, src_group, src_regnum, tmap_loc)) {
        //cout << __func__ << " " << __LINE__ << ": ignoring " << src_group << ", " << src_regnum.reg_identifier_to_string() << " because touch_tmap contains mapping" << endl;
        continue;
      }

      bool found = tmap_entry->tmap_entry_delete_loc(tmap_loc);
      ASSERT(found);
      cur_count--;
    }
    //cout << __func__ << " " << __LINE__ << ": before delete: tmap_entry =\n" << tmap_entry->transmap_entry_to_string("  ") << endl;
    //cout << __func__ << " " << __LINE__ << ": after delete: tmap_entry =\n" << tmap_entry->transmap_entry_to_string("  ") << endl;
    //if (found) {
    //  cur_count--;
    //  //cout << __func__ << " " << __LINE__ << ": successfully deleted, cur_count = " << cur_count << "\n";
    //} else {
    //  //cout << __func__ << " " << __LINE__ << ": ignoring " << src_group << ", " << src_regnum.reg_identifier_to_string() << " because tmap_entry_delete_loc returned false" << endl;
    //}
  }
  ASSERT(cur_count <= desired_count);
}

static void
transmap_spill_regs_across_conflicting_flag_groups(transmap_t *tmap,
    live_range_sort_struct_t const *sorted_regs,
    int num_sorted_regs,
    transmap_t const *touch_tmap)
{
  size_t num_flag_locs = 0;
  for (const auto &g : tmap->extable) {
    for (const auto &r : g.second) {
      transmap_loc_id_t l;
      if (r.second.transmap_entry_maps_to_flag_loc(l)) {
        num_flag_locs++;
      }
    }
  }
  std::function<bool (transmap_entry_t const &, transmap_loc_id_t &)> match_fn = [](transmap_entry_t const &e, transmap_loc_id_t &l)
  {
    return e.transmap_entry_maps_to_flag_loc(l);
  };
  transmap_spill_regs_for_matching_dst_groups(tmap, sorted_regs, num_sorted_regs, num_flag_locs, 1, touch_tmap, match_fn);
}

static void
transmap_spill_regs(transmap_t *tmap, regcount_t const *touch_not_live_in,
    live_ranges_t const *live_ranges, transmap_t const *touch_tmap)
{
  ASSERT(live_ranges);
  size_t num_src_regs = live_ranges->get_num_elems();
  live_range_sort_struct_t sorted_regs[num_src_regs];
  regcount_t tmap_count;
  int i, num_sorted_regs;

  DYN_DEBUG2(enum_tmaps, cout << __func__ << " " << __LINE__ << ": entry: tmap = \n" << transmap_to_string(tmap, as1, sizeof as1) << endl);
  tmap->transmap_count_dst_regs(&tmap_count);

	//cout << __func__ << " " << __LINE__ << ": live_ranges =\n" << live_ranges_to_string(live_ranges, NULL, as1, sizeof as1) << endl;
  num_sorted_regs = src_regs_sort_based_on_live_ranges(sorted_regs, live_ranges);

  /*transmap_spill_regs_for_dst_group(tmap, sorted_regs, num_sorted_regs,
      tmap_count.reg_count, DST_NUM_REGS - temps_count->reg_count, live_ranges,
      touch_tmap, -1);*/
  for (i = 0; i < DST_NUM_EXREG_GROUPS; i++) {
    int num_desired_regs = DST_NUM_EXREGS(i) - regset_count_exregs(&dst_reserved_regs, i) - touch_not_live_in->exreg_count[i];
    DYN_DEBUG2(enum_tmaps, cout << __func__ << " " << __LINE__ << ": group " << i << ": num_desired_regs = " << num_desired_regs << endl);
    std::function<bool (transmap_entry_t const &, transmap_loc_id_t &)> match_fn = [i](transmap_entry_t const &e, transmap_loc_id_t &l)
    {
      if (e.transmap_entry_maps_to_loc(TMAP_LOC_EXREG(i))) {
        l = TMAP_LOC_EXREG(i);
        return true;
      }
      return false;
    };

    transmap_spill_regs_for_matching_dst_groups(tmap, sorted_regs, num_sorted_regs,
        tmap_count.exreg_count[i],
        num_desired_regs,
        /*live_ranges, */touch_tmap, match_fn);
  }
  DYN_DEBUG2(enum_tmaps, cout << __func__ << " " << __LINE__ << ": exit: tmap = \n" << transmap_to_string(tmap, as1, sizeof as1) << endl);
  transmap_spill_regs_across_conflicting_flag_groups(tmap, sorted_regs, num_sorted_regs, touch_tmap);
}

/*static void
transmap_entries_union(transmap_entry_t *dst, transmap_entry_t const *src1,
    transmap_entry_t const *src2)
{
  int i, j;

  memcpy(dst, src1, sizeof(transmap_entry_t));
  for (i = 0; i < src2->num_locs; i++) {
    if (array_search(dst->loc, dst->num_locs, src2->loc[i]) == -1) {
      dst->loc[dst->num_locs] = src2->loc[i];
      dst->regnum[dst->num_locs] = src2->regnum[i];
      dst->num_locs++;
    }
  }
}*/

static void
pred_row_cartesian_product_union_choose_tmap_loc(transmap_t *tmap, int group,
    reg_identifier_t reg, transmap_entry_t const *tmap_entry1, transmap_entry_t const *tmap_entry2)
{
  transmap_entry_t *tmap_entry;

  ASSERT(group >= 0 && group < SRC_NUM_EXREG_GROUPS);
  //ASSERT(reg >= 0 && reg < SRC_NUM_EXREGS(group));
  tmap_entry = &tmap->extable[group][reg];

  /*ASSERT((tmap_entry1->num_locs == 0) == (tmap_entry2->num_locs == 0));
  if (tmap_entry1->num_locs == 0) {
    transmap_entry_assign_using_tmap_entry(tmap_entry, tmap_entry2, tmap,
        group, reg);
  } else if (tmap_entry2->num_locs == 0) {
    transmap_entry_assign_using_tmap_entry(tmap_entry, tmap_entry1, tmap,
        group, reg);
  } else {
    // use the higher-priority tmap, tmap_entry1
    transmap_entry_assign_using_tmap_entry(tmap_entry, tmap_entry1, tmap,
        group, reg);
  }*/
  tmap_entry->transmap_entries_union(tmap_entry1, tmap_entry2);
}

static transmap_set_elem_t *
transmap_set_elem_alloc(transmap_t const *tmap, transmap_t const *out_tmaps, size_t num_out_tmaps, peep_entry_t const *peep_entry_id)
{
  transmap_set_elem_t *ret;

  ret = (transmap_set_elem_t *)malloc(sizeof(transmap_set_elem_t));
  ASSERT(ret);
  transmap_init(&ret->in_tmap);
  transmap_copy(ret->in_tmap, tmap);
  ret->out_tmaps = new transmap_t[num_out_tmaps];
  transmaps_copy(ret->out_tmaps, out_tmaps, num_out_tmaps);
  ret->next = NULL;
  ret->decisions = NULL;
  ret->set_peeptab_entry_id(peep_entry_id);
  ret->cost = 0/*COST_INFINITY*/;

  return ret;
}

static transmap_set_elem_t *
pred_row_add_transmap(transmap_set_elem_t *pred_row, transmap_genset_t *pred_row_genset,
    transmap_t const *tmap)
{
  transmap_set_elem_t *new_elem;

  if (transmap_genset_belongs(pred_row_genset, tmap, NULL, 0, regcount_zero())) {
    return pred_row;
  }
  transmap_genset_add(pred_row_genset, tmap, NULL, 0, regcount_zero());
  new_elem = transmap_set_elem_alloc(tmap, NULL, 0, NULL);
  new_elem->next = pred_row;
  return new_elem;
}

static transmap_set_elem_t *
pred_row_cartesian_product_union(transmap_set_elem_t *pred_row1,
    transmap_set_elem_t *pred_row2, transmap_genset_t *pred_row_genset,
    live_ranges_t const *live_ranges)
{
  transmap_set_elem_t const *iter1, *iter2;
  transmap_set_elem_t *pred_row;
  transmap_t tmap;
  //int i, j;

  if (!pred_row1) {
    return pred_row2;
  }
  if (!pred_row2) {
    return pred_row1;
  }

  pred_row = NULL;
  for (iter1 = pred_row1; iter1; iter1 = iter1->next) {
    for (iter2 = pred_row2; iter2; iter2 = iter2->next) {

      DYN_DBG(enum_tmaps, "Cartesian product: iter1:\n%s\nand iter2:\n%s\n",
          transmap_to_string(iter1->in_tmap, as1, sizeof as1),
          transmap_to_string(iter2->in_tmap, as2, sizeof as2));
      transmap_copy(&tmap, transmap_none());

      for (const auto &g : iter1->in_tmap->extable) {
        for (const auto &r : g.second) {
          exreg_group_id_t i = g.first;
          reg_identifier_t j = r.first;
          pred_row_cartesian_product_union_choose_tmap_loc(&tmap, i, j,
              transmap_get_elem(iter1->in_tmap, i, j),
              transmap_get_elem(iter2->in_tmap, i, j));
        }
      }
      for (const auto &g : iter2->in_tmap->extable) {
        for (const auto &r : g.second) {
          exreg_group_id_t i = g.first;
          reg_identifier_t j = r.first;
          pred_row_cartesian_product_union_choose_tmap_loc(&tmap, i, j,
              transmap_get_elem(iter1->in_tmap, i, j),
              transmap_get_elem(iter2->in_tmap, i, j));
        }
      }

      transmap_spill_regs(&tmap, regcount_zero(), live_ranges, NULL);
      DYN_DBG(enum_tmaps, "Adding:\n%s\n",
          transmap_to_string(&tmap, as1, sizeof as1));
      pred_row = pred_row_add_transmap(pred_row, pred_row_genset, &tmap);
    }
  }
  return pred_row;
}

static transmap_set_elem_t *
pred_rows_cartesian_product_union(transmap_set_elem_t *pred_rows[], int num_pred_rows,
    transmap_genset_t *pred_row_genset, live_ranges_t const *live_ranges)
{
  autostop_timer func_timer(__func__);
  transmap_set_elem_t *pred_row_cartesian_product;
  long n;

  if (num_pred_rows == 0) {
    return NULL;
  }
  pred_row_cartesian_product = pred_rows[0];
  for (n = 1; n < num_pred_rows; n++) {
    pred_row_cartesian_product = pred_row_cartesian_product_union(
        pred_row_cartesian_product, pred_rows[n], pred_row_genset, live_ranges);
  }
  //ASSERT(pred_row_cartesian_product);
  return pred_row_cartesian_product;
}

static void
transmap_minus_regs(transmap_t *tmap, regset_t const *regs)
{
  /*int i, j;
  for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < SRC_NUM_EXREGS(i); j++) {*/
  for (const auto &g : regs->excregs) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t j = r.first;
      if (regset_belongs_ex(regs, i, j)) {
        //tmap->extable[i][j].num_locs = 0;
        tmap->extable[i][j].clear();
      }
    }
  }
}



static transmap_set_elem_t *
build_pred_row_from_pred_transmap_set(pred_transmap_set_t const  *pred_transmap_set,
    regset_t const *touch, regset_t const *use,
    regset_t const *live_in,
    transmap_genset_t *pred_row_genset, src_ulong pc, transmap_t *use_tmap_union)
{
  autostop_timer func_timer(__func__);
  transmap_set_elem_t *pred_row, *cur;
  pred_transmap_set_t const *pred;
  transmap_t tmap, use_tmap;
  long j;
 
  pred_row = NULL;

  cur = pred_row;

  transmap_copy(use_tmap_union, transmap_none());

  for (pred = pred_transmap_set; pred; pred = pred->next) {
    transmap_set_elem_t *iter;

    for (iter = pred->list; iter; iter = iter->next) {
      transmap_t const *out_tmap;
      long k;

      out_tmap = find_out_tmap_for_nextpc(iter, pc);
      ASSERT(out_tmap);
      DYN_DBG(enum_tmaps, "Considering out_tmap:\n%s\n",
          transmap_to_string(out_tmap, as1, sizeof as1));

      transmap_copy(&tmap, out_tmap);
      transmap_copy(&use_tmap, out_tmap);
      transmap_intersect_with_regset(&use_tmap, use);
      transmaps_union(use_tmap_union, use_tmap_union, &use_tmap);

      transmap_minus_regs(&tmap, touch);
      transmap_intersect_with_regset(&tmap, live_in);

      DYN_DBG(enum_tmaps, "touch = %s\n", regset_to_string(touch, as1, sizeof as1));
      DYN_DBG(enum_tmaps, "live_in = %s\n", regset_to_string(live_in, as1, sizeof as1));
      DYN_DBG(enum_tmaps, "Considering tmap:\n%s\n",
          transmap_to_string(&tmap, as1, sizeof as1));

      if (transmap_genset_belongs(pred_row_genset, &tmap, NULL, 0, regcount_zero())) {
        continue;
      }
      transmap_genset_add(pred_row_genset, &tmap, NULL, 0, regcount_zero());

      DYN_DBG(enum_tmaps, "Adding transmap to pred_row:\n%s\n",
          transmap_to_string(&tmap, as1, sizeof as1));
      if (!pred_row) {
        pred_row = (transmap_set_elem_t *)malloc
            (sizeof(transmap_set_elem_t));
        ASSERT(pred_row);
        cur = pred_row;
      } else {
        ASSERT(!cur->next);
        cur->next = (transmap_set_elem_t *)malloc
            (sizeof(transmap_set_elem_t));
        ASSERT(cur->next);
        cur = cur->next;
      }
      init_pred_row_elem(cur);
      transmap_init(&cur->in_tmap);
      transmap_copy(cur->in_tmap, &tmap);
      transmap_unassign_regs(cur->in_tmap);
    }
  }
  return pred_row;
}

static transmap_set_elem_t *
transmap_set_append(transmap_set_elem_t *dst, transmap_set_elem_t *src)
{
  autostop_timer func_timer(__func__);
  transmap_set_elem_t *cur, *prev;

  if (!dst) {
    return src;
  }

  for (cur = src; cur; cur = cur->next) {
    if (cur == dst) {
      return src;
    }
  }

  for (cur = dst->next, prev = dst; cur; prev = cur, cur = cur->next) {
    if (cur == src) {
      return dst;
    }
  }
  
  ASSERT(prev);
  ASSERT(prev->next == NULL);
  prev->next = src;
  return dst;
}

static transmap_set_elem_t *
build_pred_row(unsigned long pc,
    pred_transmap_set_t * const *pred_transmap_sets,
    long num_pred_transmap_sets,
    regset_t const *touch, regset_t const *use,
    regset_t const *live_in,
    live_ranges_t const *live_ranges,
    transmap_t *use_tmap_union)
{
  autostop_timer func_timer(__func__);
#define MAX_NUM_PREV_ROWS_FOR_CARTESIAN_PRODUCT 3
  transmap_set_elem_t *pred_row, *pred_row_cartesian_product;
  transmap_t pred_use_tmap_union[num_pred_transmap_sets];
  transmap_set_elem_t *pred_rows[num_pred_transmap_sets];
  transmap_genset_t pred_row_genset;
  long n;

  transmap_genset_init(&pred_row_genset);
  /*if (pc == 0x100010) {
    DYN_DBG_SET(enum_tmaps, 2);
  }*/

  for (n = 0; n < num_pred_transmap_sets; n++) {
    transmap_set_elem_t *iter;
    DYN_DBG(enum_tmaps, "%lx: calling build_pred_row_from_pred_transmap_set(%ld)\n", pc, n);
    pred_rows[n] = build_pred_row_from_pred_transmap_set(pred_transmap_sets[n], touch,
        use, live_in, &pred_row_genset, pc, &pred_use_tmap_union[n]);
    DYN_DEBUG2(enum_tmaps, printf("Printing pred_row[%ld]:\n", n);
        pred_row_print((long)pc, -1, pred_rows[n]););
  }

  transmap_copy(use_tmap_union, transmap_none());
  pred_row = NULL;
  for (n = 0; n < num_pred_transmap_sets; n++) {
    pred_row = transmap_set_append(pred_row, pred_rows[n]);
    DYN_DEBUG2(enum_tmaps, printf("Printing pred_row_cartesian after union with %ldth pred_transmap_set:\n", n);
        pred_row_print((long)pc, -1, pred_row););
    transmaps_union(use_tmap_union, use_tmap_union, &pred_use_tmap_union[n]);
  }

  size_t num_prev_rows_for_cartesian_product = num_pred_transmap_sets;
  if (num_prev_rows_for_cartesian_product > MAX_NUM_PREV_ROWS_FOR_CARTESIAN_PRODUCT) {
    num_prev_rows_for_cartesian_product = MAX_NUM_PREV_ROWS_FOR_CARTESIAN_PRODUCT;
  }
  pred_row_cartesian_product = pred_rows_cartesian_product_union(pred_rows,
      num_prev_rows_for_cartesian_product,
      &pred_row_genset, live_ranges);
  transmap_genset_free(&pred_row_genset);
  DYN_DEBUG2(enum_tmaps, printf("Printing pred_row_cartesian_product:\n");
      pred_row_print((long)pc, -1, pred_row_cartesian_product););

  pred_row = transmap_set_append(pred_row, pred_row_cartesian_product);

  /*if (pc == 0x100010) {
    DYN_DBG_SET(enum_tmaps, 0);
  }*/
  return pred_row;
}

static void
peep_match_attrs_add(vector<peep_match_attrs_t> &peep_match_attrs, transmap_t const &tmap, vector<transmap_t> const &out_tmaps, regcount_t const &touch_not_live_in, struct peep_entry_t const *peep_entry, cost_t peep_match_cost)
{
  for (auto &peep_match_attr : peep_match_attrs) {
    if (peep_match_attr.match_and_update_transmaps_and_touch(tmap, out_tmaps, touch_not_live_in, peep_entry, peep_match_cost)) {
      DYN_DEBUG2(enum_tmaps, cout << __func__ << " " << __LINE__ << ": found existing match; returning without adding\n");
      return;
    }
  }
  DYN_DEBUG2(enum_tmaps, cout << __func__ << " " << __LINE__ << ": did not find match; adding new peep_match_attrs\n");
  peep_match_attrs.push_back(peep_match_attrs_t(tmap, out_tmaps, touch_not_live_in, peep_entry, peep_match_cost));
}

static vector<peep_match_attrs_t>
peep_get_all_transmaps_temporaries_for_src_iseq(struct peep_table_t const *tab,
    src_insn_t const *src_iseq, long src_iseq_len,
    regset_t const *live_outs, long num_live_outs,
    regset_t const& src_iseq_use, regset_t const& src_iseq_def,
    memset_t const& src_iseq_memuse, memset_t const& src_iseq_memdef,
    string enum_tmaps_prefix
    )
{
  autostop_timer func_timer(__func__);
  //cout << __func__ << " " << __LINE__ << endl;
  //memlabel_map_t inv_memlabel_map;
  peep_entry_t const *peep_entry;
  unsigned long i/*, ret, cur_size*/;
  //transmap_genset_t tmap_genset;
  //regmap_t inv_src_regmap;
  //imm_vt_map_t inv_imm_map;
  //nextpc_map_t inv_nextpc_map;
  transmap_t tmap;

  enum_tmaps_prefix += string(".") + string(__func__);
  //*transmaps = vector<transmap_t>();
  //*touch_not_live_in = NULL;
  DYN_DBGS2(enum_tmaps_prefix.c_str(), "Enumerating transmaps for:\n%s\n iseq_len %ld\n",
      src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1), (long)src_iseq_len);
  DYN_DEBUGS2(enum_tmaps_prefix.c_str(),
      regsets_to_string(live_outs, num_live_outs, as1, sizeof as1);
      cout << "live_outs:\n" << as1 << endl
  );
  if (!tab->hash_size) {
    DYN_DBGS2(enum_tmaps_prefix.c_str(), "%s", "tab->hash_size = 0\n");
    return vector<peep_match_attrs_t>();
  }

  i = src_iseq_hash_func_after_canon(src_iseq, src_iseq_len, tab->hash_size);
  DYN_DBGS(enum_tmaps_prefix.c_str(), "Searching index %ld\n", i);

  //ret = 0;
  //cur_size = 0;
  vector<peep_match_attrs_t> peep_match_attrs;
  //transmap_genset_init(&tmap_genset);
  DYN_DBGS2(enum_tmaps_prefix.c_str(), "i = %ld, tab->hash[i].size() = %zd\n", (long)i, (long)tab->hash[i].size());
  //autostop_timer func2_timer(string(__func__) + ".2");
  for (auto peep_entry_iter = tab->hash[i].begin(); peep_entry_iter != tab->hash[i].end(); peep_entry_iter++) {
    peep_entry = *peep_entry_iter;

    stringstream peep_prefix;
    peep_prefix << enum_tmaps_prefix << ".line" << peep_entry->peep_entry_get_line_number();
    vector<src_iseq_match_renamings_t> renamings;
    renamings = peep_entry_match_src_side(peep_entry, src_iseq, src_iseq_len, live_outs, num_live_outs, src_iseq_use, src_iseq_def, src_iseq_memuse, src_iseq_memdef, peep_prefix.str());

    //autostop_timer func3_timer(string(__func__) + ".3");
    for (const auto &src_iseq_match_renamings : renamings) {
      //autostop_timer func4_timer(string(__func__) + ".4");
      regcount_t l_touch_not_live_in;
      regset_t touch_not_live_in_regs_renamed;
      regmap_t inv_dst_regmap;
      memslot_map_t inv_memslot_map(-1);
      transmaps_get_inv_dst_regmap_from_inv_src_regmap(peep_entry->get_in_tmap(), peep_entry->out_tmap, num_live_outs, &src_iseq_match_renamings.get_regmap(), &inv_dst_regmap, &inv_memslot_map);
      DYN_DEBUGS2(enum_tmaps_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": peep_entry->touch_not_live_in_regs = " << regset_to_string(&peep_entry->touch_not_live_in_regs, as1, sizeof as1) << endl);
      DYN_DEBUGS2(enum_tmaps_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": inv_dst_regmap =\n" << regmap_to_string(&inv_dst_regmap, as1, sizeof as1) << endl);
      regset_rename(&touch_not_live_in_regs_renamed, &peep_entry->touch_not_live_in_regs, &inv_dst_regmap);
      regset_count(&l_touch_not_live_in, &touch_not_live_in_regs_renamed);
      //now count temporary regs that are used neither in in_tmap nor in out_tmap; i.e., the regs for which no mapping exists in inv_dst_regmap
      regset_t temps = peep_entry->touch_not_live_in_regs;
      regcount_t temps_count;
      regset_subtract_mapped_regs_in_regmap(&temps, &inv_dst_regmap);
      regset_count(&temps_count, &temps);
      regcount_union(&l_touch_not_live_in, &temps_count);

      //if (transmap_genset_belongs(&tmap_genset, peep_entry->get_in_tmap(), peep_entry->out_tmap, peep_entry->num_live_outs, &l_touch_not_live_in)) {
      //  DYN_DBG2(enum_tmaps, "Enum-tmaps transmap_genset_belongs() returned true for:\n%s\n",
      //      transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1));
      //  continue;
      //}
      DYN_DBGS2(enum_tmaps_prefix.c_str(), "Enum-tmaps everything match. peep_entry->in_tmap\n%s\n",
          transmap_to_string(peep_entry->get_in_tmap(), as1, sizeof as1));
      DYN_DBGS2(enum_tmaps_prefix.c_str(), "Enum-tmaps peep_entry->src_iseq:\n%s\n",
          src_iseq_to_string(peep_entry->src_iseq, peep_entry->src_iseq_len,
              as1, sizeof as1));
      DYN_DBGS2(enum_tmaps_prefix.c_str(), "Enum-tmaps inv_src_regmap\n%s\n",
          regmap_to_string(&src_iseq_match_renamings.get_regmap(), as1, sizeof as1));
      transmap_copy(&tmap, transmap_none());
      //cout << __func__ << " " << __LINE__ << ": before rename, tmap = " << transmap_to_string(&tmap, as1, sizeof as1) << endl;
      transmap_rename(&tmap, peep_entry->get_in_tmap(), &src_iseq_match_renamings.get_regmap());
      vector<transmap_t> out_tmaps;
      ASSERT(num_live_outs == peep_entry->num_live_outs);
      //autostop_timer func5_timer(string(__func__) + ".5");
      for (size_t n = 0; n < num_live_outs; n++) {
        transmap_t otmap;
        transmap_copy(&otmap, transmap_none());
        DYN_DBGS2(enum_tmaps_prefix.c_str(), "Enum-tmaps Adding considering peep_entry->out_tmap[%zd]\n%s\n", n,
            transmap_to_string(&peep_entry->out_tmap[n], as1, sizeof as1));
        DYN_DBGS2(enum_tmaps_prefix.c_str(), "Enum-tmaps Adding considering renamings.regmap\n%s\n",
            regmap_to_string(&src_iseq_match_renamings.get_regmap(), as1, sizeof as1));
        transmap_rename(&otmap, &peep_entry->out_tmap[n], &src_iseq_match_renamings.get_regmap());
        DYN_DBGS2(enum_tmaps_prefix.c_str(), "Enum-tmaps After rename, otmap\n%s\n",
            transmap_to_string(&otmap, as1, sizeof as1));
        DYN_DBGS2(enum_tmaps_prefix.c_str(), "live_outs[n]: %s\n", regset_to_string(&live_outs[n], as1, sizeof as1));
        transmap_intersect_with_regset(&otmap, &live_outs[n]);
        DYN_DBGS2(enum_tmaps_prefix.c_str(), "Enum-tmaps After intersect, otmap\n%s\n",
            transmap_to_string(&otmap, as1, sizeof as1));
        out_tmaps.push_back(otmap);
      }
      //autostop_timer func6_timer(string(__func__) + ".6");
      //cout << __func__ << " " << __LINE__ << ": after rename, tmap = " << transmap_to_string(&tmap, as1, sizeof as1) << endl;
      DYN_DBGS2(enum_tmaps_prefix.c_str(), "Enum-tmaps Adding transmap\n%s\n",
          transmap_to_string(&tmap, as1, sizeof as1));
      //transmap_genset_add(&tmap_genset, &tmap, out_tmaps, &l_touch_not_live_in);
      //ASSERT(ret <= cur_size);
      //if (ret == cur_size) {
      //  cur_size = (cur_size + 1) * 2;
      //  //*transmaps = (transmap_t *)realloc(*transmaps, cur_size * sizeof(transmap_t));
      //  //ASSERT(*transmaps);
      //  (*transmaps).resize(cur_size);
      //  *touch_not_live_in = (regcount_t *)realloc(*touch_not_live_in,
      //      cur_size * sizeof(regcount_t));
      //  ASSERT(*touch_not_live_in);
      //}
      //ASSERT(ret < cur_size);
      /*printf("*transmaps = %p, ret = %d, cur_size = %d, &(*transmaps)[ret]=%p\n",
          *transmaps, (int)ret, (int)cur_size, &(*transmaps)[ret]);*/
      cost_t peep_match_cost = compute_peep_match_cost(peep_entry->cost, src_iseq_match_renamings);
      DYN_DBGS(enum_tmaps_prefix.c_str(), "Enum-tmaps peep_entry line_number %ld, match_cost %ld, Adding transmap:\n%s\ntouch_not_live_in:\n%s\n",
          peep_entry->peep_entry_get_line_number(),
          peep_match_cost,
          transmap_to_string(&tmap, as1, sizeof as1),
          regcount_to_string(&l_touch_not_live_in, as2, sizeof as2));
      //transmap_copy(&(*transmaps)[ret], &tmap);
      //regcount_copy(&(*touch_not_live_in)[ret], &l_touch_not_live_in);
      peep_match_attrs_add(peep_match_attrs, tmap, out_tmaps, l_touch_not_live_in, peep_entry, peep_match_cost);
      //ret++;
    }
  }
  //transmap_genset_free(&tmap_genset);
  return peep_match_attrs;
}

static void
transmap_subset_iter_start(transmap_t const *set, transmap_t *iter)
{
  transmap_copy(iter, transmap_none());
}

static bool
transmap_entry_subset_iter_next(transmap_entry_t const *set, transmap_entry_t *iter)
{
  bool present[set->get_locs().size()];
  int i, j;

  /*for (i = 0; i < set->num_locs; i++) {
    if (array_search(iter->loc, iter->num_locs, set->loc[i]) == -1) {
      present[i] = false;
    } else {
      present[i] = true;
    }
  }*/
  i = 0;
  for (const auto &l : set->get_locs()) {
    if (iter->contains_mapping_to_loc(l.get_loc())) {
      present[i] = true;
    } else {
      present[i] = false;
    }
    i++;
  }

  for (i = 0; i < set->get_locs().size(); i++) {
    if (!present[i]) {
      present[i] = true;
      //iter->num_locs = 0;
      iter->clear();
      for (j = 0; j < set->get_locs().size(); j++) {
        if (present[j]) {
          //iter->loc[iter->num_locs] = set->loc[j];
          //iter->regnum[iter->num_locs] = -1;
          //iter->num_locs++;
          uint8_t loc = set->get_locs().at(j).get_loc();
          if (loc == TMAP_LOC_SYMBOL) {
            iter->transmap_entry_add_symbol_loc(loc, -1);
          } else {
            iter->transmap_entry_add_exreg_loc(loc, TRANSMAP_REG_ID_UNASSIGNED, TMAP_LOC_MODIFIER_DST_SIZED);
          }
        }
      }
      return true;
    }
    present[i] = false;
  }
  return false;
}

static bool
transmap_subset_iter_next(transmap_t const *set, transmap_t *iter)
{
  for (const auto &g : set->extable) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t j = r.first;
      if (transmap_entry_subset_iter_next(transmap_get_elem(set, i, j),
              &iter->extable[i][j])) {
        return true;
      }
      //iter->extable[i][j].num_locs = 0;
      iter->extable[i][j].clear();
    }
  }
  return false;
}

static void
transmap_add_memlocs_for_missing_live_regs(transmap_t *tmap,
    live_ranges_t const *live_ranges)
{
  for (const auto &g : live_ranges->lr_extable) {
    for (const auto &r : g.second) {
      if (r.second) {
        if (tmap->extable[g.first][r.first].get_locs().size() == 0) {
          //tmap->extable[g.first][r.first].loc[0] = TMAP_LOC_SYMBOL;
          //tmap->extable[g.first][r.first].regnum[0] = -1;
          //tmap->extable[g.first][r.first].num_locs = 1;
          tmap->extable[g.first][r.first].set_to_symbol_loc(TMAP_LOC_SYMBOL, -1);
        }
      }
    }
  }
}

static void
transmaps_construct_by_combining(src_ulong pc, transmap_t &ctmap, transmap_t *out_ctmaps, size_t num_out_tmaps, transmap_t const *pred_tmap, transmap_t const &use_tmapc_subset, transmap_t const *touch_tmap, vector<transmap_t> const &touch_out_tmaps, regcount_t const *touch_not_live_in, live_ranges_t const *live_ranges)
{
  transmaps_union(&ctmap, pred_tmap, &use_tmapc_subset);
  transmaps_union(&ctmap, &ctmap, touch_tmap);
  DYN_DBG2(enum_tmaps, "%lx: before transmap_spill_regs, ctmap:\n%s\ntouch_not_live_in:\n%s\n", (long)pc,
      transmap_to_string(&ctmap, as1, sizeof as1),
      regcount_to_string(touch_not_live_in, as2, sizeof as2));
  if (live_ranges) {
    transmap_spill_regs(&ctmap, touch_not_live_in, live_ranges, touch_tmap);
  }
  transmap_unassign_regs(&ctmap);
  DYN_DBG2(enum_tmaps, "%lx: after transmap_spill_regs, ctmap:\n%s\n", (long)pc,
      transmap_to_string(&ctmap, as1, sizeof as1));
  if (live_ranges) {
    transmap_add_memlocs_for_missing_live_regs(&ctmap, live_ranges);
  }

  ASSERT(num_out_tmaps == touch_out_tmaps.size());
  for (size_t i = 0; i < num_out_tmaps; i++) {
    transmap_copy(&out_ctmaps[i], &ctmap);
    transmaps_subtract(&out_ctmaps[i], &ctmap, touch_tmap);
    transmaps_union(&out_ctmaps[i], &out_ctmaps[i], &touch_out_tmaps.at(i));
  }
}

static transmap_set_elem_t *
transmap_gen_list_using_pred_and_touch_transmaps(src_ulong pc,
    transmap_t const *pred_tmap,
    //transmap_t const *touch_tmap, regcount_t const *touch_not_live_in,
    peep_match_attrs_t const &peep_match_attr,
    transmap_t const *use_tmap_union, live_ranges_t const *live_ranges,
    transmap_genset_t *tmap_genset)
{
  autostop_timer func_timer(__func__);
  transmap_t use_tmapc, use_tmapc_subset;
  transmap_set_elem_t *ret, *new_elem;
  transmap_t const *touch_tmap = &peep_match_attr.get_in_tmap();
  vector<transmap_t> const &touch_out_tmaps = peep_match_attr.get_out_tmaps();
  regcount_t const *touch_not_live_in = &peep_match_attr.get_touch_not_live_in();
  peep_entry_t const *peep_entry_id = peep_match_attr.get_peep_entry_id();
  size_t num_out_tmaps = touch_out_tmaps.size();

  ret = NULL;
  transmaps_subtract(&use_tmapc, use_tmap_union, touch_tmap); //enumerating use_tmapc helps identify src-regs who need to have multiple mappings in their transmap entry
  //cout << __func__ << " " << __LINE__ << ": " << hex << pc << dec << ": use_tmapc = " << transmap_to_string(&use_tmapc, as1, sizeof as1) << endl;
  transmap_subset_iter_start(&use_tmapc, &use_tmapc_subset);
  do {
    transmap_t ctmap, out_ctmaps[num_out_tmaps];

    transmaps_construct_by_combining(pc, ctmap, out_ctmaps, num_out_tmaps, pred_tmap, use_tmapc_subset, touch_tmap, touch_out_tmaps, touch_not_live_in, live_ranges);
    if (transmap_genset_belongs(tmap_genset, &ctmap, out_ctmaps, num_out_tmaps, regcount_zero())) {
      DYN_DBG2(enum_tmaps, "%s", "transmap_genset_belongs returned true\n");
      continue;
    }
    transmap_genset_add(tmap_genset, &ctmap, out_ctmaps, num_out_tmaps, regcount_zero());
    DYN_DBG2(enum_tmaps, "adding ctmap:\n%s\n",
        transmap_to_string(&ctmap, as1, sizeof as1));
    new_elem = transmap_set_elem_alloc(&ctmap, out_ctmaps, num_out_tmaps, peep_entry_id);
    new_elem->next = ret;
    ret = new_elem;
  } while (transmap_subset_iter_next(&use_tmapc, &use_tmapc_subset));

  return ret;
}

static transmap_set_elem_t *
transmaps_gen_list_using_pred_row_and_peeptab(src_ulong pc, transmap_set_elem_t const *pred_row,
    //vector<transmap_t> const &transmaps, regcount_t const *touch_not_live_in,
    //long num_transmaps,
    vector<peep_match_attrs_t> const &peep_match_attrs,
    transmap_t const *use_tmap_union,
    size_t num_out_tmaps,
    live_ranges_t const *live_ranges, transmap_set_elem_t *cur_list)
{
  autostop_timer func_timer(__func__);
  transmap_set_elem_t const *cur, *next;
  transmap_set_elem_t *new_elems;
  transmap_genset_t tmap_genset;
  //long i;

  transmap_genset_init(&tmap_genset);
  for (cur = cur_list; cur; cur = cur->next) {
    transmap_genset_add(&tmap_genset, cur->in_tmap, cur->out_tmaps, num_out_tmaps, regcount_zero());
  }

  ASSERT(pred_row);
  for (cur = pred_row; cur; cur = next) {
    for (const auto &peep_match_attr : peep_match_attrs) {
      new_elems = transmap_gen_list_using_pred_and_touch_transmaps(pc, cur->in_tmap,
          peep_match_attr, use_tmap_union, live_ranges, &tmap_genset);
      cur_list = transmap_set_append(new_elems, cur_list);
    }
    next = cur->next;
  }
  transmap_genset_free(&tmap_genset);
  return cur_list;
}

static bool
si_has_fixed_transmaps(translation_instance_t const *ti, si_entry_t const *si)
{
  return ti->fixed_transmaps.count(si->pc) > 0;
}

static pair<peep_entry_t const *, vector<transmap_t>>
si_get_out_tmaps_for_in_tmap_at_fetchlen(translation_instance_t const *ti, si_entry_t const *si, long fetchlen, transmap_t &in_tmap)
{
  autostop_timer func_timer(__func__);
  struct peep_table_t const *tab = &ti->peep_table;
  long src_iseq_len, num_nextpcs;
  input_exec_t const *in = ti->in;
  src_insn_t *src_iseq = new src_insn_t[in->num_si];
  ASSERT(src_iseq);
  nextpc_t *nextpcs = new nextpc_t[in->num_si];
  ASSERT(nextpcs);
  src_ulong *npcs = new src_ulong[in->num_si];
  bool rc = src_iseq_fetch(src_iseq, in->num_si, &src_iseq_len, in, NULL, si->pc, fetchlen,
      &num_nextpcs, nextpcs, NULL, NULL, NULL, NULL, false);
  ASSERT(rc);
  nextpc_t::nextpcs_get_pcs(npcs, nextpcs, num_nextpcs);
  regset_t const *live_out_ptrs[num_nextpcs];
  get_live_outs(in, live_out_ptrs, npcs, num_nextpcs);
  regset_t live_outs[num_nextpcs];
  for (size_t i = 0; i < num_nextpcs; i++) {
    regset_copy(&live_outs[i], live_out_ptrs[i]);
  }
  transmap_t out_tmaps[num_nextpcs];

  struct peep_entry_t const *peeptab_entry_id;
  peeptab_entry_id = peep_search(tab, NULL, src_iseq, src_iseq_len, &in_tmap,
        NULL, NULL, 0, out_tmaps, NULL, live_outs, num_nextpcs, NULL, NULL,
        NULL, NULL, NULL, NULL, ti->get_base_reg(), __func__);
  vector<transmap_t> ret;
  for (size_t i = 0; i < num_nextpcs; i++) {
    ret.push_back(out_tmaps[i]);
  }

  delete [] src_iseq;
  delete [] nextpcs;
  return make_pair(peeptab_entry_id, ret);
}

static transmap_set_elem_t *
transmaps_gen_list_for_fixed_transmaps(translation_instance_t const *ti, si_entry_t const *si,
    long fetchlen, regset_t const *live_regs)
{
  if (fetchlen != 1) {
    return NULL; //XXX: this can be removed; allow arbitrary fetchlen translations as long as the in_tmap is fixed
  }
  vector<transmap_t> const &fixed_transmaps = ti->fixed_transmaps.at(si->pc);
  transmap_set_elem_t *ret = NULL;
  transmap_entry_t tmap_entry_mem;
  tmap_entry_mem.set_to_symbol_loc(TMAP_LOC_SYMBOL, (dst_ulong)-1);
  //tmap_entry_mem.num_locs = 1;
  //tmap_entry_mem.loc[0] = TMAP_LOC_SYMBOL;
  //tmap_entry_mem.regnum[0] = (dst_ulong)-1;
  for (const auto &tmap : fixed_transmaps) {
    transmap_t ctmap = tmap;
    for (const auto &g : live_regs->excregs) {
      for (const auto &r : g.second) {
        if (   ctmap.extable.count(g.first) == 0
            || ctmap.extable.at(g.first).count(r.first) == 0) {
          ctmap.extable[g.first][r.first] = tmap_entry_mem;
        }
      }
    }
    pair<peep_entry_t const *, vector<transmap_t>> pp = si_get_out_tmaps_for_in_tmap_at_fetchlen(ti, si, fetchlen, ctmap);
    peep_entry_t const *peeptab_entry_id = pp.first;
    vector<transmap_t> const &out_ctmaps_vec = pp.second;
    ASSERT(fetchlen != 1 || peeptab_entry_id); //we expect peeptab_entry_id to be non-NULL for fetchlen == 1
    if (!peeptab_entry_id) {
      return NULL;
    }
    size_t num_out_tmaps = out_ctmaps_vec.size();
    transmap_t out_ctmaps[num_out_tmaps];
    for (size_t i = 0; i < num_out_tmaps; i++) {
      transmap_copy(&out_ctmaps[i], &out_ctmaps_vec.at(i));
    }
    transmap_set_elem_t *new_elem = transmap_set_elem_alloc(&ctmap, out_ctmaps, num_out_tmaps, peeptab_entry_id);
    //new_elem->out_tmaps = new transmap_t[1];
    //transmap_copy(&new_elem->out_tmaps[0], new_elem->in_tmap);
    new_elem->next = ret;
    ret = new_elem;
  }
  return ret;
}

transmap_set_elem_t *
peep_enumerate_transmaps(translation_instance_t const *ti, si_entry_t const *si,
    src_insn_t const *src_iseq, long src_iseq_len, long fetchlen,
    regset_t const *live_outs, long num_live_outs,
    struct peep_table_t const *tab,
    pred_transmap_set_t **pred_transmap_sets, long num_pred_transmap_sets,
    transmap_set_elem_t *ls)
{
  autostop_timer func_timer(__func__);
  //cout << __func__ << " " << __LINE__ << ": pc = 0x" << hex << si->pc << dec << endl;
  ASSERT(si);
  //regcount_t *touch_not_live_in;
  //vector<transmap_t> transmaps;
  vector<peep_match_attrs_t> peep_match_attrs;
  transmap_t use_tmap_union;
  regset_t use, def, touch;
  //regset_t src_iseq_use, src_iseq_def;
  memset_t memuse, memdef;
  src_iseq_get_usedef(src_iseq, src_iseq_len, &use, &def, &memuse, &memdef);

  //long num_transmaps;
  src_ulong pc = /*si ? */si->pc/* : 0*/;
  //ASSERT(num_live_outs);
  DYN_DBG(enum_tmaps, "getting transmaps for src_iseq at %lx fetchlen %ld\n", (long)pc,
      fetchlen);
  //if (pc == 0x1700001 && fetchlen == 1) {
  //  DYN_DBG_SET(enum_tmaps, 2);
  //}
  stringstream ss;
  ss << "enum_tmaps.pc" << hex << si->pc << dec << ".fetchlen" << fetchlen;
  string enum_tmaps_prefix = ss.str();
  peep_match_attrs = peep_get_all_transmaps_temporaries_for_src_iseq(tab, src_iseq,
      src_iseq_len, live_outs, num_live_outs, use, def, memuse, memdef, enum_tmaps_prefix);
  //if (pc == 0x100001 && fetchlen == 1) {
  //  DYN_DBG_SET(enum_tmaps, 2);
  //}
  if (peep_match_attrs.size() == 0) {
    if (fetchlen == 1) {
      printf("%s() %d: Warning: num_transmaps = 0 at %lx\n", __func__, __LINE__,
          (long)pc);
      printf("src_iseq =\n%s--\n", src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
      regsets_to_string(live_outs, num_live_outs, as1, sizeof as1);
      printf("live : %s\n", as1);
      printf("memlive : true\n");
    }
    DYN_DBGS2(enum_tmaps_prefix.c_str(), "peep_get_all_transmaps_for_src_iseq returned 0. "
        "pc %lx, fetchlen %ld, src_iseq:\n%s\n", (long)pc, fetchlen,
        src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
    /*if (si->pc == 0x10000c98) {
      enum_tmaps_flag = 0;
      PEEP_TAB_flag = 0;
    }*/
    //if (pc == 0x1700001 && fetchlen == 1) {
    //  DYN_DBG_SET(enum_tmaps, 0);
    //}
    return ls;
  }

  DYN_DBGS(enum_tmaps_prefix.c_str(), "peep_get_all_transmaps_for_src_iseq returned %ld. "
      "pc %lx, len %ld, src_iseq:\n%s\n", peep_match_attrs.size(), (long)pc, fetchlen,
      src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));

  transmap_set_elem_t *pred_row;
  regset_t use_only;

  //src_iseq_get_usedef_regs(src_iseq, src_iseq_len, &use, &def);
  regset_copy(&touch, &use);
  regset_union(&touch, &def);
  regset_copy(&use_only, &use);
  regset_diff(&use_only, &def);

  DYN_DBGS2(enum_tmaps_prefix.c_str(), "calling build_pred_row. pc %lx, len %ld, "
      "num_pred_transmap_sets %ld, use %s, def %s, use_only %s.\n", (long)pc,
      fetchlen,
      num_pred_transmap_sets, regset_to_string(&use, as1, sizeof as1),
      regset_to_string(&def, as2, sizeof as2),
      regset_to_string(&use_only, as3, sizeof as3));

  if (si->bbl) {
    stopwatch_run(&build_pred_row_timer);
    pred_row = build_pred_row(pc, pred_transmap_sets, num_pred_transmap_sets, &touch,
        &use_only, &si->bbl->live_regs[si->bblindex], &si->bbl->live_ranges[si->bblindex],
        &use_tmap_union);
    stopwatch_stop(&build_pred_row_timer);
  } else { //this can happen for tools/peep_enumerate_transmaps
    pred_row = NULL;
  }

  //if (pc == 0x100001 && fetchlen == 1) {
  //  DYN_DBG_SET(enum_tmaps, 0);
  //}

  DYN_DBGS2(enum_tmaps_prefix.c_str(), "use_tmap_union:\n%s\n",
      transmap_to_string(&use_tmap_union, as1, sizeof as1));
  DYN_DEBUGS2(enum_tmaps_prefix.c_str(), pred_row_print((long)pc, fetchlen, pred_row););

  DYN_DEBUGS2(enum_tmaps_prefix.c_str(),
    long i;
    printf("%d: %lx: Printing peeptab_transmaps. num_transmaps = %ld\n", __LINE__, (long)pc,
        peep_match_attrs.size());
    for (i = 0; i < peep_match_attrs.size(); i++) {
      printf("in_transmap[%ld]:\n%s\n", i,
          transmap_to_string(&peep_match_attrs.at(i).get_in_tmap(), as1, sizeof as1));
      for (size_t j = 0; j < peep_match_attrs.at(i).get_out_tmaps().size(); j++) {
        printf("out_transmap[%ld][%zd]:\n%s\n", i, j,
            transmap_to_string(&peep_match_attrs.at(i).get_out_tmaps().at(j), as1, sizeof as1));
      }
      printf("touch_not_live_in[%ld]:\n%s\n", i,
          regcount_to_string(&peep_match_attrs.at(i).get_touch_not_live_in(), as1, sizeof as1));
      printf("out_transmap[%ld]:\n%s\n", i,
          transmap_to_string(&peep_match_attrs.at(i).get_out_tmaps().at(0), as1, sizeof as1));
    }
  );

  transmap_set_elem_t *pred_row_plus_fb;
  if (pred_row == NULL) {
    pred_row_plus_fb = (transmap_set_elem_t *)malloc(sizeof(transmap_set_elem_t));
    ASSERT(pred_row_plus_fb);
    init_pred_row_elem(pred_row_plus_fb);
    pred_row_plus_fb->next = pred_row;
    transmap_init(&pred_row_plus_fb->in_tmap);
    transmap_copy(pred_row_plus_fb->in_tmap, transmap_default());
    transmap_minus_regs(pred_row_plus_fb->in_tmap, &touch);
    transmap_unassign_regs(pred_row_plus_fb->in_tmap);
    if (si->bbl) {
      transmap_intersect_with_regset(pred_row_plus_fb->in_tmap,
          &si->bbl->live_regs[si->bblindex]);
    }
  } else {
    pred_row_plus_fb = (transmap_set_elem_t *)pred_row;
  }
  ASSERT(pred_row_plus_fb);

  if (si->bbl && si_has_fixed_transmaps(ti, si)) {
    if (!ls) {
      ls = transmaps_gen_list_for_fixed_transmaps(ti, si, fetchlen, &si->bbl->live_regs[si->bblindex]);
      //if (!ls) {
      //  DYN_DBG(ERR, "transmap_gen_list_for_fixed_transmaps() returned null!."
      //      "pc %lx, len %ld, src_iseq:\n%s\n", (long)pc, fetchlen,
      //      src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
      //}
      //ASSERT(ls);
    }
  } else {
    ls = transmaps_gen_list_using_pred_row_and_peeptab(si->pc, pred_row_plus_fb,
        //transmaps,
        //touch_not_live_in, num_transmaps,
        peep_match_attrs,
        &use_tmap_union, num_live_outs,
        si->bbl ? &si->bbl->live_ranges[si->bblindex] : NULL, ls);
    //if (!ls) {
    //  DYN_DBG(ERR, "transmap_gen_list_using_pred_row_and_peeptab returned null!."
    //      "pc %lx, len %ld, pred_row_plus_fb = %p, src_iseq:\n%s\n", (long)pc, fetchlen,
    //      pred_row_plus_fb, src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
    //}
    //ASSERT(ls);
  }
  //ASSERT(ls);
  //ls = transmaps_list_append_spill_one_versions_for_full_transmaps(ls);
  //ASSERT(ls);

  if (pred_row_plus_fb != pred_row) {
    transmap_free(pred_row_plus_fb->in_tmap);
    free(pred_row_plus_fb);
  }

  {
    /*if (si->pc == 0x100010) {
      DYN_DBG_SET(enum_tmaps, 2);
    }*/
    DYN_DBGS2(enum_tmaps_prefix.c_str(), "%lx len %ld: Printing all transmaps after union, ls = %p:\n",
        (long)pc, fetchlen, ls);
    transmap_set_elem_t *cur;
    long count = 0;
    for (cur = ls; cur; cur = cur->next) {
      DYN_DBGS2(enum_tmaps_prefix.c_str(), "%lx len %ld: Printing in-transmap after union #%ld:\n%s",
          (long)pc,
          fetchlen, count, transmap_to_string(cur->in_tmap, as1, sizeof as1));
      for (size_t j = 0; j < num_live_outs; j++) {
        DYN_DBGS2(enum_tmaps_prefix.c_str(), "%lx len %ld: Printing out-transmap[%zd] after union #%ld:\n%s",
            (long)pc,
            fetchlen, j, count, transmap_to_string(&cur->out_tmaps[j], as1, sizeof as1));
      }
      count++;
    }
    /*if (si->pc == 0x100010) {
      DYN_DBG_SET(enum_tmaps, 0);
    }*/
  }

  //if (si->pc == 0x4ce) {
  //  DYN_DBG_SET(enum_tmaps, 0);
  //}
  //if (pc == 0x1700001 && fetchlen == 1) {
  //  DYN_DBG_SET(enum_tmaps, 0);
  //}


  free_transmap_set_row(pred_row);
  //free(transmaps);
  //free(touch_not_live_in);
  return ls;
}

//transmap_set_elem_t *
//peep_search_transmaps(translation_instance_t const *ti, si_entry_t const *si,
//    src_insn_t const *src_iseq, long src_iseq_len, long fetchlen,
//    struct peep_table_t const *tab,
//    regset_t const *live_outs,
//    long num_live_outs, transmap_set_elem_t *ls)
//{
//  long peepcost, num_transmaps;
//  transmap_set_elem_t *iter;
//  src_ulong pc;
//
//  num_transmaps = 0;
//  pc = si->pc;
//
//  TIME("%lx : LEN %ld : num_tmaps = %ld\n", (long)pc, (long)fetchlen,
//      transmap_set_count(ls));
//  for (iter = ls; iter; iter = iter->next) {
//    num_transmaps++;
//    DYN_DBG2(PEEP_TAB, "%lx len %ld: Trying Transmap #%ld:\n%s", (long)pc,
//        (long)fetchlen, (long)num_transmaps,
//        transmap_to_string(iter->in_tmap, as1, sizeof as1));
//
//    DYN_DBG(PEEP_TAB, "Calling peep_search() for len %ld [fetchlen %ld] insn seq at %lx\n"
//        "%s\n", src_iseq_len, fetchlen, (long)pc,
//        src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
//
//    if (!iter->out_tmaps) {
//      //iter->out_tmaps = (transmap_t *)malloc(num_live_outs * sizeof(transmap_t));
//      iter->out_tmaps = new transmap_t[num_live_outs];
//      ASSERT(iter->out_tmaps);
//    }
//
//    /*extern char PEEP_TAB_flag;
//    if (si->pc == 0x10000c98) {
//      PEEP_TAB_flag = 2;
//    }*/
//
//    peepcost = peep_search(tab, src_iseq, src_iseq_len, iter->in_tmap,
//        NULL, NULL, 0, iter->out_tmaps, NULL, live_outs, num_live_outs, NULL, NULL,
//        NULL, NULL, NULL, NULL, ti->get_base_reg(), false);
//
//    if (peepcost >= COST_INFINITY) {
//      cout << __func__ << " " << __LINE__ << ": src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
//      cout << __func__ << " " << __LINE__ << ": iter->in_tmap =\n" << transmap_to_string(iter->in_tmap, as1, sizeof as1) << endl;
//    }
//    ASSERT(peepcost < COST_INFINITY);
//    DYN_DEBUG2(PEEP_TAB,
//      int i;
//      printf("%s() %d:\n", __func__, __LINE__);
//      printf("%lx: peepcost = %ld, iter->in_tmap:\n", (long)si->pc, peepcost);
//      printf("%s\n", transmap_to_string(iter->in_tmap, as1, sizeof as1));
//      printf("%lx: peepcost = %ld, iter->out_tmaps:\n", (long)si->pc, peepcost);
//      if (peepcost != COST_INFINITY) {
//        for (i = 0; i < num_live_outs; i++) {
//          printf("%s\n", transmap_to_string(&iter->out_tmaps[i], as1, sizeof as1));
//        }
//      }
//    );
//    /*if (si->pc == 0x10000c98) {
//      PEEP_TAB_flag = 0;
//    }*/
//
//    DYN_DBG(PEEP_TAB, "%lx fetchlen %ld: peepcost = %ld\n",
//        (long)pc, fetchlen, peepcost);
//
//    iter->cost = peepcost;
//  }
//  if (fetchlen == 1 && !ls) {
//    printf("No translation found for pc %lx fetchlen 1\n", (long)si->pc);
//    NOT_REACHED();
//  }
//  return ls;
//}

void
peep_translate(translation_instance_t const *ti,
    struct peep_entry_t const *peeptab_entry_id,
    src_insn_t const *src_iseq,
    long src_iseq_len, nextpc_t const *nextpcs,
    regset_t const *live_outs,
    long num_nextpcs,
    struct transmap_t const *in_tmap, struct transmap_t *out_tmaps,
    transmap_t *peep_in_tmap, transmap_t *peep_out_tmaps,
    regcons_t *peep_regcons,
    regset_t *peep_temp_regs_used,
    nomatch_pairs_set_t *peep_nomatch_pairs_set,
    regcount_t *touch_not_live_in,
    dst_insn_t *dst_iseq, long *dst_iseq_len, long dst_iseq_size/*, bool var*/)
{
  long i;
  //src_ulong pc;

  ASSERT(peeptab_entry_id);
  ASSERT(dst_iseq);
  //cout << __func__ << " " << __LINE__ << ": before peep_search, out_tmaps[0] =\n" << transmap_to_string(&out_tmaps[0], as1, sizeof as1) << endl;
  peep_entry_t const *peep_entry_id;
  peep_entry_id = peep_search(&ti->peep_table, peeptab_entry_id, src_iseq, src_iseq_len,
      (transmap_t *)in_tmap, dst_iseq, dst_iseq_len, dst_iseq_size, out_tmaps, nextpcs, live_outs,
      num_nextpcs, peep_in_tmap, peep_out_tmaps, peep_regcons, peep_temp_regs_used, peep_nomatch_pairs_set, touch_not_live_in, ti->get_base_reg(), __func__);
  //cout << __func__ << " " << __LINE__ << ": after peep_search, out_tmaps[0] =\n" << transmap_to_string(&out_tmaps[0], as1, sizeof as1) << endl;

  ASSERT(peep_entry_id == peeptab_entry_id);
  long peepcost = peep_entry_id->cost;
  DYN_DEBUG2(PEEP_TAB,
    printf("%s() %d:\n", __func__, __LINE__);
    printf("peepcost = %ld, out_tmaps:\n", peepcost);
    if (peepcost != COST_INFINITY) {
      for (int i = 0; i < num_nextpcs; i++) {
        printf("%s\n", transmap_to_string(&out_tmaps[i], as1, sizeof as1));
      }
    }
  );

  if (peepcost == COST_INFINITY) {
    cout << __func__ << " " << __LINE__ << ": src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
    cout << __func__ << " " << __LINE__ << ": num_nextpcs = " << num_nextpcs << endl;
    for (int i = 0; i < num_nextpcs; i++) {
      cout << __func__ << " " << __LINE__ << ": live_outs[" << i << "] = " << regset_to_string(&live_outs[i], as1, sizeof as1) << endl;
    }
    cout << __func__ << " " << __LINE__ << ": in_tmap=\n" << transmap_to_string(in_tmap, as1, sizeof as1) << endl;
  }

  ASSERT(peepcost < COST_INFINITY);

  DYN_DBG(PEEP_TAB, "peep_search returned dst_iseq:\n%s\n",
     dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1));
}

static void
peep_entry_free(peep_entry_t *peep_entry)
{
  if (peep_entry->src_iseq) {
    //free(peep_entry->src_iseq);
    delete [] peep_entry->src_iseq;
  }
  if (peep_entry->dst_iseq) {
    //free(peep_entry->dst_iseq);
    delete [] peep_entry->dst_iseq;
  }
  if (peep_entry->live_out) {
    //free(peep_entry->live_out);
    delete [] peep_entry->live_out;
  }
  //if (peep_entry->nomatch_pairs) {
  //  delete [] peep_entry->nomatch_pairs;
  //}
  peep_entry->nomatch_pairs_set.free();
  if (peep_entry->get_in_tmap()) {
    peep_entry->free_in_tmap();
  }
  if (peep_entry->out_tmap) {
    //transmap_free(peep_entry->out_tmap);
    delete [] peep_entry->out_tmap;
  }
  if (peep_entry->in_tcons) {
    //free(peep_entry->in_tcons);
    delete peep_entry->in_tcons;
  }
  /*if (peep_entry->get_in_tmap()) {
    peep_entry->free_union_tmap();
  }*/
  //if (peep_entry->src_symbol_map) {
  //  free(peep_entry->src_symbol_map);
  //}
  //if (peep_entry->dst_symbol_map) {
  //  free(peep_entry->dst_symbol_map);
  //}
  //if (peep_entry->nextpc_function_name_map) {
  //  free(peep_entry->nextpc_function_name_map);
  //}
}

void
peephole_table_free(struct peep_table_t *tab)
{
  long i;
  for (i = 0; i < tab->hash_size; i++) {
    peep_entry_t *iter, *next;
    for (auto iter = tab->hash[i].begin(); iter != tab->hash[i].end(); iter++) {
      //next = iter->next;
      peep_entry_free(*iter);
      //free(iter);
      delete *iter;
    }
  }
  //free(tab->hash);
  delete [] tab->hash;
  //free(tab->peep_entries);
}

/*bool
peep_preprocess_vregs(char *line, regmap_t const *regmap,
    void (*regname_suffix)(long regno, char const *suffix, char *name,
        size_t name_size))
{
  char const *suffixes[] = { "b", "B", "w", "d" };
  long nbytes[] = {1, 1, 2, 4};
  long i;

  for (i = 0; i < sizeof suffixes/sizeof suffixes[0]; i++) {
    if (!peep_preprocess_vregs_suffix(line, suffixes[i], nbytes[i], regmap,
        regname_suffix)) {
      return false;
    }
  }
  return true;
}*/

/*typedef struct type_separated_peeptab_t {
  peep_table_t peeptab;
  typestate_t ts_initial[NUM_FP_STATES], ts_final[NUM_FP_STATES];
} type_separated_peeptab_t;

static void
peep_entry_compute_typestate(typestate_t *typestate_initial,
    typestate_t *typestate_final, peep_entry_t const *cur, rand_states_t const &rand_states)
{
  //typestate_init_to_top(typestate_initial);
  //typestate_init_to_top(typestate_final);
  src_iseq_compute_typestate_using_transmaps(typestate_initial, typestate_final, cur->src_iseq,
      cur->src_iseq_len, cur->live_out, cur->in_tmap, cur->out_tmap,
      cur->num_live_outs, cur->mem_live_out, rand_states);
}

static int
typestate_categorize(peep_entry_t const *e, type_separated_peeptab_t *peeptabs,
    int num_peeptabs, int max_num_peeptabs, rand_states_t const &rand_states)
{
  typestate_t *ts_init, *ts_fin;
  int i, f;

  ts_init = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
  ASSERT(ts_init);
  ts_fin = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
  ASSERT(ts_fin);

  peep_entry_compute_typestate(ts_init, ts_fin, e, rand_states);
  DYN_DBG(PEEP_TAB, "ts_init[0]:\n%s\n", typestate_to_string(&ts_init[0], as1, sizeof as1));
  DYN_DBG(PEEP_TAB, "ts_fin[0]:\n%s\n", typestate_to_string(&ts_fin[0], as1, sizeof as1));

  for (i = 0; i < num_peeptabs; i++) {
    if (   !typestates_equal_n(ts_init, peeptabs[i].ts_initial, NUM_FP_STATES)
        || !typestates_equal_n(ts_fin, peeptabs[i].ts_final, NUM_FP_STATES)) {
      continue;
    }
    add_peephole_entry(&peeptabs[i].peeptab, e->src_iseq, e->src_iseq_len,
        e->in_tmap, e->out_tmap, e->dst_iseq, e->dst_iseq_len, e->live_out,
        e->nextpc_indir, e->num_live_outs, e->mem_live_out,// e->flexible_used_regs,
        e->dst_fixed_reg_mapping, e->nomatch_pairs,
        e->num_nomatch_pairs, e->regconst_map_possibilities,
        //e->src_symbol_map, e->dst_symbol_map,
        //e->src_locals_info, e->dst_locals_info,
        //&e->src_iline_map, &e->dst_iline_map,
        //e->nextpc_function_name_map,
        e->cost, 0);
    DYN_DBG(PEEP_TAB, "num_peeptabs = %d.\n", num_peeptabs);
    free(ts_init);
    free(ts_fin);
    return num_peeptabs;
  }

  ASSERT(num_peeptabs < max_num_peeptabs);
  peep_table_init(&peeptabs[num_peeptabs].peeptab);
  for (f = 0; f < NUM_FP_STATES; f++) {
    typestate_copy(&peeptabs[num_peeptabs].ts_initial[f], &ts_init[f]);
    typestate_copy(&peeptabs[num_peeptabs].ts_final[f], &ts_fin[f]);
  }

  add_peephole_entry(&peeptabs[num_peeptabs].peeptab, e->src_iseq, e->src_iseq_len,
      e->in_tmap, e->out_tmap, e->dst_iseq, e->dst_iseq_len, e->live_out,
      e->nextpc_indir, e->num_live_outs, e->mem_live_out,// e->flexible_used_regs,
      e->dst_fixed_reg_mapping,
      e->nomatch_pairs,
      e->num_nomatch_pairs, e->regconst_map_possibilities,
      //e->src_symbol_map, e->dst_symbol_map,
      //e->src_locals_info, e->dst_locals_info,
      //&e->src_iline_map, &e->dst_iline_map,
      //e->nextpc_function_name_map,
      e->cost, 0);

  num_peeptabs++;

  DYN_DBG(PEEP_TAB, "num_peeptabs = %d.\n", num_peeptabs);
  free(ts_init);
  free(ts_fin);
  return num_peeptabs;
}*/

/*void
peephole_table_type_categorize(struct peep_table_t *tab,
    char const *outname, rand_states_t const &rand_states)
{
#define MAX_TS_PEEPTABS 64
  //static type_separated_peeptab_t peeptabs[MAX_TS_PEEPTABS];
  type_separated_peeptab_t *peeptabs = new type_separated_peeptab_t[MAX_TS_PEEPTABS];
  ASSERT(peeptabs);
  peep_entry_t const *peep_entry;
  long i, num_peeptabs;

  num_peeptabs = 0;
  for (i = 0; i < tab->hash_size; i++) {
    for (auto iter = tab->hash[i].begin(); iter != tab->hash[i].end(); iter++) {
      peep_entry = *iter;
      num_peeptabs = typestate_categorize(peep_entry, peeptabs, num_peeptabs,
          MAX_TS_PEEPTABS, rand_states);
    }
  }
  size_t num_states = rand_states.get_num_states();

  for (i = 0; i < num_peeptabs; i++) {
    char outname_i[strlen(outname) + 64];
    snprintf(outname_i, sizeof outname_i, "%s.%ld", outname, i);
    peeptab_write(NULL, &peeptabs[i].peeptab, outname_i);
    snprintf(outname_i, sizeof outname_i, "%s.%ld.types", outname, i);
    FILE *fp = fopen(outname_i, "w");
    emit_types_to_file(fp, peeptabs[i].ts_initial, peeptabs[i].ts_final, num_states);
    fclose(fp);
  }
  delete [] peeptabs;
}*/

/*
void
memlabel_canonicalize_locals_symbols(memlabel_t *ml, imm_vt_map_t *imm_vt_map, src_tag_t tag)
{
  int i;
  ASSERT(tag == tag_imm_local || tag == tag_imm_symbol);
  if (tag == tag_imm_local) {
    if (ml->m_type != MEMLABEL_LOCAL) {
      return;
    }
  } else if (tag == tag_imm_symbol) {
    if (ml->m_type != MEMLABEL_GLOBAL) {
      return;
    }
  } else NOT_REACHED();

  for (i = 0; i < imm_vt_map->num_imms_used; i++) {
    if (ml->m_id == imm_vt_map->table[i].val) {
      ml->m_id = i;
      return;
    }
  }
  ASSERT(i == imm_vt_map->num_imms_used);
  ASSERT(i < NUM_CANON_CONSTANTS);
  imm_vt_map->table[i].val = ml->m_id;
  imm_vt_map->table[i].tag = tag;
  ml->m_id = i;
  imm_vt_map->num_imms_used++;
}*/

void
peep_entry_t::free_in_tmap()
{
  delete in_tmap;
  in_tmap = NULL;
}

/*void
peep_entry_t::free_union_tmap()
{
  delete union_tmap;
  union_tmap = NULL;
}*/

void
peep_entry_t::populate_usedef()
{
  src_iseq_compute_touch_var(&this->m_touch, &this->m_use, &this->m_memuse,
      &this->m_memdef,
      this->src_iseq,
      this->src_iseq_len);
}

size_t
src_harvested_iseq_to_string(char *buf, size_t size, src_insn_t const *insns,
    long num_insns,
    regset_t const *live_out, long num_live_out, bool mem_live_out)
{
  char *ptr = buf, *end = buf + size;
  char tmpbuf[128];
  long i;

  src_iseq_to_string(insns, num_insns, ptr, end - ptr);
  ptr += strlen(ptr);
  ASSERT(ptr < end);
  /*ptr += snprintf(ptr, end - ptr, "--\n%s--\nlive : ",
      transmap_to_string(tmap, tmpbuf, sizeof tmpbuf));*/
  ptr += snprintf(ptr, end - ptr, "--\nlive :");
  ASSERT(ptr < end);

  ptr += regsets_to_string(live_out, num_live_out, ptr, end - ptr);
  ptr += snprintf(ptr, end - ptr, "\n");
  ASSERT(ptr < end);

  bool_to_string(mem_live_out, tmpbuf, sizeof tmpbuf);
  ptr += snprintf(ptr, end - ptr, "memlive : %s\n", tmpbuf);
  ASSERT(ptr < end);
  return ptr - buf;
}


