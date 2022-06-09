#include <map>
#include <iostream>
#include "insn/src-insn.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/mman.h>
#include "support/timers.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "support/src-defs.h"
#include "support/globals.h"

#include "cutils/imap_elem.h"
#include "i386/cpu_consts.h"
#include "insn/rdefs.h"
//#include "ppc/execute.h"
#include "rewrite/static_translate.h"
#include "valtag/memslot_map.h"
#include "valtag/memslot_set.h"
#include "valtag/nextpc.h"

#include "config-host.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "rewrite/bbl.h"
#include "insn/cfg.h"
#include "expr/memlabel.h"

#include "insn/jumptable.h"
#include "rewrite/peephole.h"
#include "superopt/rand_states.h"
#include "support/c_utils.h"
#include "valtag/regmap.h"
//#include "rewrite/memlabel_map.h"
//#include "ppc/regmap.h"
//#include "imm_map.h" 
#include "ppc/insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "valtag/transmap.h"
#include "support/debug.h"
#include "rewrite/transmap_set.h"
#include "cutils/enumerator.h"
#include "rewrite/transmap_genset.h"
#include "cutils/large_int.h"
#include "valtag/imm_vt_map.h"
//#include "gsupport/sym_exec.h"
#include "insn/live_ranges.h"

#include "i386/regs.h"
#include "x64/regs.h"

#include "valtag/elf/elf.h"
#include "cutils/rbtree.h"
#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"
#include "superopt/superopt.h"
#include "superopt/superoptdb.h"
//#include "rewrite/harvest.h"
#include "rewrite/symbol_map.h"
#include "valtag/pc_with_copy_id.h"
#include "gsupport/graph_symbol.h"
#include "insn/fun_type_info.h"
#include "cutils/pc_elem.h"

using namespace std;

static unsigned char bin1[1024];
static char as1[4096000];
static char as2[10240];
static char as3[10240];

bool ignore_fetchlen_reset = false;
char const *profile_name = NULL;

#ifdef LARGE_INT_DEBUG
large_int_t si_max_fetchlen = { .val = {8388608,}, .lval = 8388608 };
#else
large_int_t si_max_fetchlen = { .val = {8388608,} };
#endif

static void
src_iseq_fetch_len(input_exec_t const *in, struct peep_table_t const *peep_table,
    src_ulong pc, si_iseq_t *e, large_int_t const *max_fetchlen, bool first, bool get_max);
static bool
src_iseq_fetch_iterate(input_exec_t const *in,
    struct peep_table_t const *peep_table, src_ulong pc,
    large_int_t *fetchlen, large_int_t const *max_fetchlen, fetch_branch_t *paths,
    long *num_paths, bool first, bool get_max);
static long
src_iseq_fetch_next(input_exec_t const *in, src_ulong start_pc, fetch_branch_t *paths,
    long *num_paths, bool get_max, large_int_t const *max_fetchlen, large_int_t *increment);



#define CMN COMMON_SRC
#include "cmn/cmn_src_insn.h"
#undef CMN




namespace eqspace {

size_t
memlabel_t::memlabel_get_rodata_symbols(memlabel_t const &ml, symbol_id_t *rodata_symbol_ids, eqspace::graph_symbol_map_t const &symbol_map)
{
  size_t ret = 0;
  if (!memlabel_is_top(&ml)) {
    for (const auto &as_elem : ml.m_alias_set) {
      if (   as_elem.alias_region_get_alias_type() == alias_region_t::alias_type_symbol
          && symbol_map.symbol_is_rodata(as_elem.alias_region_get_var_id())) {
        rodata_symbol_ids[ret] = as_elem.alias_region_get_var_id();
        ret++;
      }
    }
  }
  return ret;
}

class expr_rename_rodata_symbols_to_their_section_names_and_offsets_visitor : public expr_visitor
{
public:
  //expr_substitution_my(expr_ref e, const expr_vector &from, const expr_vector &to);
  expr_rename_rodata_symbols_to_their_section_names_and_offsets_visitor(expr_ref const& e, input_exec_t const* in, symbol_map_t const& symbol_map) : m_expr(e), m_in(in), m_symbol_map(symbol_map), m_ctx(e->get_context()), m_cs(m_ctx->get_consts_struct())
  {
    this->visit_recursive(e);
  }
  //expr_ref get_substituted(context* ctx, const expr_vector& from, const expr_vector& to);
  expr_ref get_result(void)
  {
    return m_map.at(m_expr->get_id());
  }

private:
  virtual void visit(expr_ref const &e);
  expr_ref const &m_expr;
  map<expr_id_t, expr_ref> m_map;
  input_exec_t const* m_in;
  symbol_map_t const& m_symbol_map;
  context* m_ctx;
  consts_struct_t const& m_cs;
};

void
expr_rename_rodata_symbols_to_their_section_names_and_offsets_visitor::visit(expr_ref const& e)
{
  //cout << __func__ << " " << __LINE__ << ": visiting " << expr_string(e) << endl;
  if (e->is_const()) {
    m_map.insert(make_pair(e->get_id(), e));
    //cout << __func__ << " " << __LINE__ << ": returning\n";
    return;
  }
  if (!e->is_var()) {
    expr_vector new_args;
    for (auto const& arg : e->get_args()) {
      new_args.push_back(m_map.at(arg->get_id()));
    }
    m_map.insert(make_pair(e->get_id(), m_ctx->mk_app(e->get_operation_kind(), new_args)));
    //cout << __func__ << " " << __LINE__ << ": returning\n";
    return;
  }
  if (!m_cs.is_symbol(e)) {
    m_map.insert(make_pair(e->get_id(), e));
    //cout << __func__ << " " << __LINE__ << ": returning\n";
    return;
  }
  symbol_id_t canon_symbol_id = m_cs.get_symbol_id_from_expr(e);
  input_exec_t const* in = m_in;
  ASSERT(canon_symbol_id >= 0 && canon_symbol_id < m_symbol_map.num_symbols);
  symbol_t const& sym = m_symbol_map.table[canon_symbol_id];
  if (!in->symbol_is_rodata(sym)) {
    m_map.insert(make_pair(e->get_id(), e));
    //cout << __func__ << " " << __LINE__ << ": returning\n";
    return;
  }
  int section_num = in->get_section_index_for_orig_index(sym.shndx);
  int offset = sym.value - in->input_sections[section_num].addr;
  expr_ref section_expr = m_cs.get_expr_value(reg_type_section, section_num);
  expr_ref off = m_ctx->mk_bv_const(section_expr->get_sort()->get_size(), offset);
  expr_ref ret = m_ctx->mk_bvadd(section_expr, off);
  m_map.insert(make_pair(e->get_id(), ret));
  //cout << __func__ << " " << __LINE__ << ": returning " << expr_string(ret) << "\n";
}

expr_ref
context::expr_rename_rodata_symbols_to_their_section_names_and_offsets(expr_ref const& e, struct input_exec_t const* in, symbol_map_t const& symbol_map)
{
  expr_rename_rodata_symbols_to_their_section_names_and_offsets_visitor visitor(e, in, symbol_map);
  expr_ref ret = visitor.get_result();
  //cout << __func__ << " " << __LINE__ << ": returning " << expr_string(ret) << " for " << expr_string(e) << endl;
  return ret;
}

}

#if 0
static bool
tb_dominates_pc(input_exec_t const *in, tb_t const *starttb, src_ulong cur)
{
  tb_t *curtb;
  struct imap_elem_t needle;
  struct hash_elem *hfound;
  int tb_index;

  needle.pc = cur;
  hfound = hash_find(&in->tb_map, &needle.h_elem);
  if (!hfound) {
    return true;  /* cur is an internal pc. its bbl leader must have been checked. */
  }
  tb_index = hash_entry(hfound, imap_elem_t, h_elem)->val;
  curtb = &in->tbs[tb_index];
  ASSERT(curtb->pc == cur);

  return tb_dominates(in, starttb, curtb);
}
#endif

static void
edges_copy(edge_t* dst, edge_t const* src, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    dst[i] = src[i];
  }
}

static void
si_iseq_init(si_iseq_t *e)
{
  e->iseq = NULL;
  e->iseq_len = 0;
  e->pcs = NULL;

  e->num_out_edges = 0;
  e->out_edges = NULL;
  e->out_edge_weights = NULL;

  e->num_function_calls = 0;
  e->function_calls = NULL;

  e->paths = NULL;
  e->num_paths = 0;

  large_int_copy(&e->iseq_fetchlen, large_int_zero());
}

static void
si_entry_init(si_entry_t *e, input_exec_t const *in,
    src_ulong pc, long index, bbl_t const *bbl, long bblindex)
{
  src_insn_t insn;
  long i;

  e->pc = pc;
  e->index = index;
  e->bbl = bbl;
  e->bblindex = bblindex;

  DBG2(SRC_INSN, "e->pc %lx, fetching insn\n", (long)e->pc);

  if (src_insn_fetch(&insn, in, e->pc, &e->fetched_insn_size)) {
    DBG2(SRC_INSN, "%s", "fetch successful.\n");
    //e->fetched_insn = (src_insn_t *)malloc(sizeof(src_insn_t));
    e->fetched_insn = new src_insn_t;
    ASSERT(e->fetched_insn);
    src_insn_copy(e->fetched_insn, &insn);
  } else {
    DBG(SRC_INSN, "e->pc %lx, fetch unsuccessful.\n", (long)e->pc);
  }
  //e->is_nop = false;

  si_iseq_init(&e->max_first);
  si_iseq_init(&e->max_middle);
  for (i = 0; i < SI_NUM_CACHED; i++) {
    si_iseq_init(&e->cached[i]);
  }
  e->eliminate_branch_to_fallthrough_pc = false;
}

static void
si_alloc(input_exec_t *in)
{
  unsigned long num_insns;
  src_ulong pc;
  bbl_t *bbl;
  long i, n;

  num_insns = 0;
  for (i = 0; i < in->num_bbls; i++) {
    bbl = &in->bbls[i];
    num_insns += bbl_num_insns(bbl);
  }

  in->si = (si_entry_t *)malloc(num_insns * sizeof(si_entry_t));
  ASSERT(in->si);
  in->num_si = num_insns;

  DBG3(SRC_INSN, "num_si = %lu\n", in->num_si);
  myhash_init(&in->pc2index_map, imap_hash_func, imap_equal_func, NULL);

  n = 0;
  for (i = 0; i < in->num_bbls; i++) {
    bbl = &in->bbls[i];
    for (pc = bbl_first_insn(bbl); pc != PC_UNDEFINED; pc = bbl_next_insn(bbl, pc)) {
      struct imap_elem_t *imap_elem;

      si_entry_init(&in->si[n], in, pc, n, bbl, bbl_insn_index(bbl, pc));

      imap_elem = (struct imap_elem_t *)malloc(sizeof *imap_elem);
      ASSERT(imap_elem);
      imap_elem->pc = pc;
      imap_elem->val = n;
      //printf("adding pc 0x%lx index %ld to pc2index_map\n", (long)pc, n);
      myhash_insert(&in->pc2index_map, &imap_elem->h_elem);
      n++;
    }
  }
}

static void
paths_free(fetch_branch_t *paths, long num_paths)
{
  /* free paths. */
  long i;

  for (i = 0; i < num_paths; i++) {
    free(paths[i].edges);
    paths[i].edges = NULL;
    free(paths[i].locks);
    paths[i].locks = NULL;
  }
}

static void
si_iseq_free(si_iseq_t *entry)
{
  if (entry->iseq) {
    //free(entry->iseq);
    delete [] entry->iseq;
    entry->iseq = NULL;
  }
  if (entry->pcs) {
    free(entry->pcs);
    entry->pcs = NULL;
    entry->iseq_len = 0;
    large_int_copy(&entry->iseq_fetchlen, large_int_zero());
  }
  if (entry->out_edges) {
    free(entry->out_edges);
    entry->out_edges = NULL;
    entry->num_out_edges = 0;
  }
  if (entry->out_edge_weights) {
    free(entry->out_edge_weights);
    entry->out_edge_weights = NULL;
  }
  if (entry->function_calls) {
    //free(entry->function_calls);
    delete[] entry->function_calls;
    entry->function_calls = NULL;
    entry->num_function_calls = 0;
  }
  if (entry->paths) {
    paths_free(entry->paths, entry->num_paths);
    free(entry->paths);
    entry->paths = NULL;
    entry->num_paths = 0;
  }
}

static void
si_entry_free(si_entry_t *entry)
{
  long i;
  //free(entry->fetched_insn);
  delete entry->fetched_insn;
  si_iseq_free(&entry->max_first);
  si_iseq_free(&entry->max_middle);
  for (i = 0; i < SI_NUM_CACHED; i++) {
    si_iseq_free(&entry->cached[i]);
  }
  //si_entry_init(entry, entry->pc, entry->index, entry->bbl, entry->bblindex);
}

void
si_free(struct input_exec_t *in)
{
  int i;
  for (i = 0; i < in->num_si; i++) {
    si_entry_free(&in->si[i]);
  }
  if (in->num_si) {
    free(in->si);
    myhash_destroy(&in->pc2index_map, imap_elem_free);
  }
  DBG_EXEC(ASSERTCHECKS, assertcheck_si_init_done = false;);
}

long
pc2index(input_exec_t const *in, src_ulong pc)
{
  struct imap_elem_t needle;
  struct myhash_elem *found;

  needle.pc = pc;
  if ((found = myhash_find(&in->pc2index_map, &needle.h_elem))) {
    return chash_entry(found, imap_elem_t, h_elem)->val;
  }
  return -1;
}

static void
iseq_fcalls_make_union(nextpc_t *dst_attrs, long *num_dst_attrs,
    nextpc_t const *src_attrs, long num_src_attrs, src_ulong const *dst_pcs,
    long num_dst_pcs, nextpc_t const *nextpcs, long num_nextpcs)
{
  long j, orig_num_dst_attrs;

  orig_num_dst_attrs = *num_dst_attrs;
  for (j = 0; j < num_src_attrs; j++) {
    bool exists = false;
    long k;
    for (k = 0; k < num_dst_pcs && !exists; k++) {
      if (   src_attrs[j].get_tag() == tag_const
          && dst_pcs[k] == src_attrs[j].get_val()) {
        exists = true;
      }
    }
    for (k = 0; k < orig_num_dst_attrs && !exists; k++) {
      if (dst_attrs[k].equals(src_attrs[j])) {
        exists = true;
      }
    }
    for (k = 0; k < num_nextpcs && !exists; k++) {
      if (nextpcs[k].equals(src_attrs[j])) {
        exists = true;
      }
    }
    if (!exists) {
      dst_attrs[*num_dst_attrs] = src_attrs[j];
      (*num_dst_attrs)++;
    }
  }
}

/* static void
iseq_attrs_make_union_with_weight_sum(src_ulong *dst_attrs,
    double *dst_weights, long *num_dst_attrs, src_ulong const *src_attrs,
    double const *src_weights, long num_src_attrs, src_ulong const *dst_pcs,
    long num_dst_pcs)
{
  long orig_num_dst_attrs, i, j;

  orig_num_dst_attrs = *num_dst_attrs;
  for (i = 0; i < num_src_attrs; i++) {
    bool found = false;
    for (j = 0; j < num_dst_pcs && !found; j++) {
      if (src_attrs[i] == dst_pcs[j]) {
        found = true;
      }
    }
    for (j = 0; j < orig_num_dst_attrs && !found; j++) {
      if (src_attrs[i] == dst_attrs[j]) {
        found = true;
        dst_weights[j] += src_weights[i];
      }
    }
    if (!found) {
      dst_attrs[*num_dst_attrs] = src_attrs[i];
      dst_weights[*num_dst_attrs] = src_weights[i];
      (*num_dst_attrs)++;
    }
  }
} */

#define MAKE_NON_NULL(x) do { if (!(x)) { (x) = &l_##x; } } while(0)
#define MAKE_NON_NULL_ARR(x) do { if (!(x)) { (x) = l_##x; } } while(0)

static void
compute_nextpcs_and_lastpcs_from_out_edges(input_exec_t const *in,
    long *num_nextpcs, nextpc_t *nextpcs,
    long *num_lastpcs, src_ulong *lastpcs, double *lastpc_weights, long num_edges,
    edge_t const *edges, double const *edge_weights, long num_pcs, src_ulong const *pcs)
{
  nextpc_t *l_nextpcs; //, l_lastpcs[num_pcs];
  long i, l, l_num_nextpcs; /*, l_num_lastpcs;
  double l_lastpc_weights[num_pcs]; */

  if (!nextpcs) {
    //l_nextpcs = (src_ulong *)malloc(2 * (num_edges + 1) * sizeof(src_ulong));
    l_nextpcs = new nextpc_t[2 * (num_edges + 1)];
    ASSERT(l_nextpcs);
    nextpcs = l_nextpcs;
    if (!num_nextpcs) {
      num_nextpcs = &l_num_nextpcs;
    }
  } else {
    l_nextpcs = NULL;
  }

  CPP_DBG_EXEC(SRC_ISEQ_FETCH,
    cout << __func__ << " " << __LINE__ << ": " << hex << (long)pcs[0] << dec << ": num edges = " << num_edges << "\n";
    for (size_t i = 0; i < num_edges; i++) {
      cout << "edges[" << i << "] = " << hex << edges[i].src << " -> " << edges[i].dst.nextpc_to_string() << dec << endl;
    }
  );
  
  /* MAKE_NON_NULL(num_nextpcs);
  MAKE_NON_NULL(num_lastpcs);
  MAKE_NON_NULL_ARR(lastpcs);
  MAKE_NON_NULL_ARR(lastpc_weights); */

  ASSERT(num_nextpcs);
  *num_nextpcs = 0;

  if (num_lastpcs) {
    *num_lastpcs = 0;
  }

  for (i = 0; i < num_edges; i++) {
    if (   nextpc_t::nextpc_array_search(nextpcs, *num_nextpcs, edges[i].dst) == -1
        && (   edges[i].dst.get_tag() != tag_const
            || array_search(pcs, num_pcs, edges[i].dst.get_val()) == -1)) {
      if (nextpcs) {   
        nextpcs[*num_nextpcs] = edges[i].dst;
      }
      if (num_nextpcs) {
        (*num_nextpcs)++;
      }
      if (!lastpcs) {
        continue;
      }
      if (array_search(lastpcs, *num_lastpcs, edges[i].src) == -1) {
        lastpcs[*num_lastpcs] = edges[i].src;
        (*num_lastpcs)++;
      }
    }
  }

  if (lastpc_weights) {
    for (i = 0; i < *num_lastpcs; i++) {
      lastpc_weights[i] = 0;
    }
    for (i = 0; i < num_edges; i++) {
      l = array_search(lastpcs, *num_lastpcs, edges[i].src);
      if (l == -1) {
        continue;
      }
      lastpc_weights[l] += edge_weights[i];
    }
  }
  if (l_nextpcs) {
    //free(l_nextpcs);
    delete[] l_nextpcs;
  }
}

static void
append_iseq_compute_attrs(input_exec_t const *in,
    long dst_iseq_len, src_ulong const *dst_iseq_pcs, long orig_dst_iseq_len,
    long *dst_iseq_num_nextpcs, nextpc_t *dst_iseq_nextpcs,
    long *dst_iseq_num_lastpcs, src_ulong *dst_iseq_lastpcs,
    double *dst_iseq_lastpc_weights,
    long *dst_iseq_num_function_calls, nextpc_t *dst_iseq_function_calls,
    long src_iseq_num_out_edges,
    edge_t const *src_iseq_out_edges, double const *src_iseq_out_edge_weights,
    long src_iseq_num_function_calls, nextpc_t const *src_iseq_function_calls)
{
  compute_nextpcs_and_lastpcs_from_out_edges(in, dst_iseq_num_nextpcs, dst_iseq_nextpcs,
      dst_iseq_num_lastpcs, dst_iseq_lastpcs, dst_iseq_lastpc_weights,
      src_iseq_num_out_edges, src_iseq_out_edges, src_iseq_out_edge_weights,
      dst_iseq_len, dst_iseq_pcs);

  iseq_fcalls_make_union(dst_iseq_function_calls, dst_iseq_num_function_calls,
      src_iseq_function_calls, src_iseq_num_function_calls, dst_iseq_pcs,
      orig_dst_iseq_len, dst_iseq_nextpcs, *dst_iseq_num_nextpcs);
}

static void
append_iseq(input_exec_t const *in, src_insn_t *dst_iseq, size_t dst_iseq_size,
    long *dst_iseq_len, large_int_t *dst_iseq_fetchlen, src_ulong *dst_iseq_pcs,
    long *dst_iseq_num_nextpcs, nextpc_t *dst_iseq_nextpcs,
    long *dst_iseq_num_lastpcs, src_ulong *dst_iseq_lastpcs,
    double *dst_iseq_lastpc_weights,
    long *dst_iseq_num_function_calls, nextpc_t *dst_iseq_function_calls,
    si_iseq_t const *entry)
{
  long src_iseq_len, src_iseq_num_out_edges;
  nextpc_t const *src_iseq_function_calls;
  double const *src_iseq_out_edge_weights;
  edge_t const *src_iseq_out_edges;
  long src_iseq_num_function_calls;
  src_ulong const *src_iseq_pcs;
  large_int_t src_iseq_fetchlen;
  src_insn_t const *src_iseq;
  long j, orig_dst_iseq_len;

  src_iseq = entry->iseq;
  src_iseq_len = entry->iseq_len;
  large_int_copy(&src_iseq_fetchlen, &entry->iseq_fetchlen);
  src_iseq_pcs = entry->pcs;
  src_iseq_num_out_edges = entry->num_out_edges;
  src_iseq_num_function_calls = entry->num_function_calls;
  src_iseq_out_edges = entry->out_edges;
  src_iseq_out_edge_weights = entry->out_edge_weights;
  src_iseq_function_calls = entry->function_calls;

  orig_dst_iseq_len = *dst_iseq_len;
  //(*dst_iseq_fetchlen) += src_iseq_fetchlen;
  large_int_add(dst_iseq_fetchlen, dst_iseq_fetchlen, &src_iseq_fetchlen);

  /* concatenate iseqs and pcs */
  for (j = 0; j < src_iseq_len; j++) {
    ASSERT(*dst_iseq_len < dst_iseq_size);
    if (dst_iseq) {
      src_insn_copy(&dst_iseq[*dst_iseq_len], &src_iseq[j]);
    }
    dst_iseq_pcs[*dst_iseq_len] = src_iseq_pcs[j];
    (*dst_iseq_len)++;
  }

  append_iseq_compute_attrs(in, *dst_iseq_len,
      dst_iseq_pcs, orig_dst_iseq_len,
      dst_iseq_num_nextpcs, dst_iseq_nextpcs, dst_iseq_num_lastpcs, dst_iseq_lastpcs,
      dst_iseq_lastpc_weights, dst_iseq_num_function_calls, dst_iseq_function_calls,
      src_iseq_num_out_edges, src_iseq_out_edges,
      src_iseq_out_edge_weights, src_iseq_num_function_calls, src_iseq_function_calls);
}


static inline edge_t const *
get_last_edge_const(fetch_branch_t const *a)
{
  return &a->edges[a->depth - 1];
}

static inline edge_t *
get_last_edge(fetch_branch_t *a)
{
  return &a->edges[a->depth - 1];
}

static int
path_edges_cmp(fetch_branch_t const *a, fetch_branch_t const *b)
{
  if (a->depth != b->depth) {
    return a->depth - b->depth;
  }
  return memcmp(a->edges, b->edges, a->depth * sizeof(edge_t));
}

static void
path_copy(fetch_branch_t *a, fetch_branch_t const *b)
{
  memcpy(a, b, sizeof(fetch_branch_t));
}

static void
path_copy_contents(fetch_branch_t *a, fetch_branch_t const *b)
{
  a->depth = b->depth;
  a->locked = b->locked;
  a->num_locks = b->num_locks;
  a->weight = b->weight;
  memcpy(a->edges, b->edges, b->depth * sizeof(edge_t));
  a->locks = (lock_t *)realloc(a->locks, b->num_locks * sizeof(lock_t));
  ASSERT(!b->num_locks || a->locks);
  memcpy(a->locks, b->locks, b->num_locks * sizeof(lock_t));
}

static void
paths_deep_copy_with_alloc(fetch_branch_t *a, fetch_branch_t const *b, long n,
    long alloc)
{
  long i;

  for (i = 0; i < n; i++) {
    //a[i].edges = (edge_t *)malloc(((alloc == 0)?b[i].depth:alloc) * sizeof(edge_t));
    a[i].edges = new edge_t[(alloc == 0)?b[i].depth:alloc];
    ASSERT(!alloc || a[i].edges);

    //printf("alloc = %ld\n", alloc);
    a[i].locks = NULL;
    //ASSERT(!alloc || a[i].locks);
    //printf("alloc = %ld\n", alloc);
    path_copy_contents(&a[i], &b[i]);
    //printf("alloc = %ld\n", alloc);
    //ASSERT(!alloc || a[i].locks);
  }
}

static void
path_alloc(fetch_branch_t *a, long max)
{
  //a->edges = (edge_t *)malloc(max * sizeof(edge_t));
  a->edges = new edge_t[max];
  ASSERT(a->edges);
  a->locks = (lock_t *)malloc(max * sizeof(lock_t));
  ASSERT(a->locks);
}

static void
resize_paths(fetch_branch_t *paths, long num_paths)
{
  /* resize paths. */
  long i;

  for (i = 0; i < num_paths; i++) {
    ASSERT(paths[i].depth);
    //paths[i].edges = (edge_t *)realloc(paths[i].edges, paths[i].depth * sizeof(edge_t));
    edge_t *old_edges = paths[i].edges;
    paths[i].edges = new edge_t[paths[i].depth];
    ASSERT(paths[i].edges);
    edges_copy(paths[i].edges, old_edges, paths[i].depth);
    delete[] old_edges;
    paths[i].locks = (lock_t *)realloc(paths[i].locks,
        paths[i].num_locks * sizeof(lock_t));
  }
}

static bool
edge_belongs_to_locked_paths(edge_t const *edge, fetch_branch_t const *a,
    long const *locked_paths, long num_locked_paths)
{
  long i, j, k;

  for (i = 0; i < num_locked_paths; i++) {
    j = locked_paths[i];
    for (k = 0; k < a[j].depth; k++) {
      if (a[j].edges[k].src == edge->src && a[j].edges[k].dst.equals(edge->dst)) {
        //printf("belongs to a[%ld]: (%lx->%lx)\n", j, (long)a[j].edges[k].src, (long)edge->dst);
        return true;
      }
    }
  }
  return false;
}

static long
reset_to_locked_ancestors(edge_t *e, long elen, fetch_branch_t const *a,
    long const *locked_paths, long num_locked_paths)
{
  long i;

  for (i = 0; i < elen; i++) {
    if (!edge_belongs_to_locked_paths(&e[i], a, locked_paths, num_locked_paths)) {
      //this assert fails if "src_insn_is_function_call" does not handle branch to
      //fallthrough pc correctly
      ASSERT(i + 1 < elen);
      e[i + 1].src = e[i].src;
      e[i + 1].dst = e[i].dst;
      return i + 2;
    }
  }
  return elen;
}

static void
unlock_paths(fetch_branch_t *a, long alen, src_ulong pc)
{
  long j;
  for (j = 0; j < alen; j++) {
    edge_t const* last_edge = get_last_edge_const(&a[j]);
    if (a[j].locked && last_edge->dst.get_tag() == tag_const && last_edge->dst.get_val() == pc) {
      a[j].locked = false;
    }
  }
}

static void
add_lock(fetch_branch_t *a, src_ulong pc)
{
  long i;

  for (i = 0; i < a->num_locks; i++) {
    if (a->locks[i].pc == pc && a->locks[i].depth == a->depth - 1) {
      return;
    }
  }

  a->locks = (lock_t *)realloc(a->locks, (a->num_locks + 1) * sizeof(lock_t));
  ASSERT(a->locks);
  a->locks[a->num_locks].pc = pc;
  a->locks[a->num_locks].depth = a->depth - 1;
  a->num_locks++;
}

static long
src_iseq_fetch_reset(fetch_branch_t *a, long alen, long const *locked_paths,
    long num_locked_paths)
{
  long i, j, k, new_depth, lock_depth;
  edge_t *edge1, *edge2;
  bool redundant[alen];

  for (i = alen - 1; i >= 0; i--) {
    bool remove_lock[a[i].num_locks];
    
    if (a[i].locked) {
      continue;
    }
    new_depth = reset_to_locked_ancestors(a[i].edges, a[i].depth, a, locked_paths,
        num_locked_paths);
    memset(remove_lock, 0, a[i].num_locks * sizeof(bool));
    for (j = 0; j < a[i].num_locks; j++) {
      lock_depth = a[i].locks[j].depth;
      ASSERT(lock_depth < a[i].depth);
      if (lock_depth >= new_depth) {
        unlock_paths(a, i, a[i].locks[j].pc);
        remove_lock[j] = true;
      }
    }
    k = 0;
    for (j = 0; j < a[i].num_locks; j++) {
      if (remove_lock[j]) {
        continue;
      }
      if (j != k) {
        memcpy(&a[i].locks[k], &a[i].locks[j], sizeof(lock_t));
      }
      k++;
    }
    a[i].num_locks = k;
    a[i].depth = new_depth;
  }

  memset(redundant, 0, alen * sizeof(bool));
  for (i = 0; i < alen; i++) {
    if (redundant[i]) {
      continue;
    }
    for (j = i + 1; j < alen; j++) {
      if (path_edges_cmp(&a[i], &a[j]) == 0) {
        ASSERT(!redundant[j]);
        redundant[j] = true;
        a[i].weight += a[j].weight;
      }
    }
  }
  j = 0;
  for (i = 0; i < alen; i++) {
    if (redundant[i]) {
      paths_free(&a[i], 1);
      continue;
    }
    if (i != j) {
      path_copy(&a[j], &a[i]);
    }
    j++;
  }
  return j;
}

static long
locks_search(lock_t const *locks, long num_locks, src_ulong pc)
{
  long i;

  for (i = 0; i < num_locks; i++) {
    if (locks[i].pc == pc) {
      return i;
    }
  }
  return -1;
}

static long
edges_search(edge_t const *edges, long num_edges, edge_t const *edge)
{
  long i;

  for (i = 0; i < num_edges; i++) {
    if (edges[i].src == edge->src && edges[i].dst.equals(edge->dst)) {
      return i;
    }
  }
  return -1;
}

static bool 
si_insn_fetch(src_insn_t *insn, si_entry_t const *si)
{
  if (!si->fetched_insn) {
    return false;
  }
  src_insn_copy(insn, si->fetched_insn);
#if SRC_PCREL_TYPE == SRC_PCREL_START
  src_insn_add_to_pcrels(insn, si->pc, si->bbl, si->bblindex);
#elif SRC_PCREL_TYPE == SRC_PCREL_END
  src_insn_add_to_pcrels(insn, si->pc + si->fetched_insn_size, si->bbl, si->bblindex);
#endif
  return true; 
/*
  bool fetch;
  long size;

  if (si->max_first.iseq) {  // if si is already setup, use it.
    ASSERT(si->max_first.iseq_len >= 0);
    if (si->max_first.iseq_len == 0) {
      DBG(SRC_INSN, "si->max_first.iseq=%p, si->max_first.iseq_len=%ld\n",
          si->max_first.iseq, si->max_first.iseq_len);
      return false;
    }
    src_insn_copy(insn, &si->max_first.iseq[0]);
  } else {  // else fetch the instruction.
#endif
    if (!src_insn_fetch(insn, si->pc, &size)) {
      return false;
    }
#if SRC_PCREL_TYPE == SRC_PCREL_START
    src_insn_add_to_pcrel(insn, si->pc);
#elif SRC_PCREL_TYPE == SRC_PCREL_END
    src_insn_add_to_pcrel(insn, si->pc + size);
#endif
  //}
  return true;
*/
}

static void
si_get_insns_including_loop(input_exec_t const *in, src_ulong *pcs, src_insn_t *iseq,
    long *iseq_len, si_entry_t const *si, nextpc_t *nextpcs, long *num_nextpcs)
{
  bool fetch;
  long i, n;

  if (num_nextpcs) {
    *num_nextpcs = 0;
  }
  n = *iseq_len;
  if (si->bblindex > 0 || si->bbl->loop == NULL) {
    pcs[n] = si->pc;
    fetch = si_insn_fetch(&iseq[n], si);
    ASSERT(fetch);
    n++;
    if (num_nextpcs) {
      if (si->bblindex != si->bbl->num_insns - 1) {
        *num_nextpcs = 1;
        nextpcs[0] = bbl_insn_address(si->bbl, si->bblindex + 1);
      } else {
        //memcpy(nextpcs, si->bbl->nextpcs, si->bbl->num_nextpcs * sizeof(src_ulong));
        nextpc_t::nextpc_array_copy(nextpcs, si->bbl->nextpcs, si->bbl->num_nextpcs);
        *num_nextpcs = si->bbl->num_nextpcs;
      }
    }
  } else {
    for (i = 0; i < si->bbl->loop->num_pcs; i++) {
      nextpc_t pc_fallthrough;
      nextpc_t inextpcs[16];
      long inum_nextpcs;
      long index;

      pcs[n] = si->bbl->loop->pcs[i];
      index = pc2index(in, si->bbl->loop->pcs[i]);
      ASSERT(index >= 0 && index < in->num_si);
      fetch = si_insn_fetch(&iseq[n], &in->si[index]);
      ASSERT(fetch);
      n++;
    
      inum_nextpcs = pc_get_nextpcs(in, si->bbl->loop->pcs[i], inextpcs, NULL);
      ASSERT(inum_nextpcs > 0 && inum_nextpcs < sizeof inextpcs/sizeof inextpcs[0]);
      if (i < si->bbl->loop->num_pcs - 1) {
        pc_fallthrough = si->bbl->loop->pcs[i + 1];
      } else {
        if (si->bbl->loop->num_out_edges > 0) {
          pc_fallthrough =
              si->bbl->loop->out_edges[si->bbl->loop->num_out_edges - 1].dst;
        } else {
          pc_fallthrough = (src_ulong)-1;
        }
      }
      /*printf("%s() %d: si->bbl->loop->pcs[%ld] = %lx\n", __func__, __LINE__, (long)i, (unsigned long)si->bbl->loop->pcs[i]);
      printf("%s() %d: pc_fallthrough = %lx\n", __func__, __LINE__, pc_fallthrough);
      int j;
      for (j = 0; j < inum_nextpcs; j++) {
        printf("%s() %d: inextpcs[%ld] = %lx\n", __func__, __LINE__, (long)j, (unsigned long)inextpcs[j]);
      }*/
      if (   !pc_fallthrough.equals(inextpcs[inum_nextpcs - 1])
          && !src_insn_is_unconditional_branch_not_fcall(&iseq[n - 1])) {
        //src_insn_put_jump_to_pcrel(inextpcs[inum_nextpcs - 1], &iseq[n], 1);
        //pcs[n] = INSN_PC_INVALID;
        //n++;
        n += add_jump_to_pcrel_src_insns(inextpcs[inum_nextpcs - 1], &iseq[n], &pcs[n], 1);
      }
    }
    if (num_nextpcs) {
      for (i = 0; i < si->bbl->loop->num_out_edges; i++) {
        if (nextpc_t::nextpc_array_search(nextpcs, *num_nextpcs, si->bbl->loop->out_edges[i].dst) == -1) {
          nextpcs[*num_nextpcs] = si->bbl->loop->out_edges[i].dst;
          (*num_nextpcs)++;
        }
      }
    }
  }
  *iseq_len = n;
}

static void
append_pcs(input_exec_t const *in, src_ulong *pcs, src_insn_t *iseq, long *iseq_len,
    nextpc_t const& start_pc, nextpc_t const& stop_pc)
{
  long index, num_nextpcs;
  nextpc_t *nextpcs;

  if (   start_pc.get_tag() != tag_const
      || start_pc.get_val() == PC_UNDEFINED
      || start_pc.get_val() == PC_JUMPTABLE
      || start_pc.equals(stop_pc)) {
    return;
  }
  index = pc2index(in, start_pc.get_val());
  ASSERT(index >= 0 && index < in->num_si);

  //nextpcs = (src_ulong *)malloc(in->num_si * sizeof(src_ulong));
  nextpcs = new nextpc_t[in->num_si];
  ASSERT(nextpcs);
  while (stop_pc.get_tag() != tag_const || in->si[index].pc != stop_pc.get_val()) {
    DBG2(SRC_INSN, "After fetching %lx, iseq_len=%ld\n", (long)in->si[index].pc, (long)*iseq_len);
    si_get_insns_including_loop(in, pcs, iseq, iseq_len, &in->si[index], nextpcs,
        &num_nextpcs);
    ASSERT(num_nextpcs >= 1);
    if (   num_nextpcs != 1
        || nextpcs[0].get_tag() != tag_const
        || nextpcs[0].get_val() == PC_JUMPTABLE
        || nextpcs[0].get_val() == PC_UNDEFINED
        || (index = pc2index(in, nextpcs[0].get_val())) == (unsigned long)-1) {
      //ASSERT(array_search(nextpcs, num_nextpcs, stop_pc) != -1);
      break;
    }
    ASSERT(index >= 0 && index < in->num_si);
  }
  //free(nextpcs);
  delete[] nextpcs;
}

static bool
paths_empty(fetch_branch_t const *paths, long num_paths)
{
  long i;

  for (i = 0; i < num_paths; i++) {
    ASSERT(paths[i].depth >= 2);
    if (paths[i].edges[0].src != paths[i].edges[1].src) {
      return false;
    }
  }
  return true;
}

size_t
add_jump_to_pcrel_src_insns(nextpc_t const& target_pc, src_insn_t *src_iseq, src_ulong *pcs, size_t max_insns)
{
  size_t i, num_insns_for_jump;
  num_insns_for_jump = src_insn_put_jump_to_pcrel(target_pc, src_iseq, max_insns);
  for (i = 0; i < num_insns_for_jump; i++) {
    pcs[i] = INSN_PC_INVALID;
  }
  return num_insns_for_jump;
}

static void
src_iseq_construct_pcs(input_exec_t const *in, src_ulong start_pc,
    fetch_branch_t const *a, long alen, long *iseq_len, src_insn_t *iseq,
    src_ulong *pcs, bool first)
{
  long i, j, max_depth, num_seen, start_index;
  si_entry_t *start_si;
  bool fetch;

  max_depth = 0;
  for (i = 0; i < alen; i++) {
    if (max_depth < a[i].depth) {
      max_depth = a[i].depth;
    }
    //max_depth = MAX(max_depth, a[i].depth);
  }

  edge_t seen[alen * max_depth];
  num_seen = 0;

  *iseq_len = 0;

  start_index = pc2index(in, start_pc);
  ASSERT(start_index >= 0 && start_index < in->num_si);
  start_si = &in->si[start_index];
  if (!first) {
    si_get_insns_including_loop(in, pcs, iseq, iseq_len, start_si, NULL, NULL);
  } else {
    fetch = si_insn_fetch(&iseq[*iseq_len], start_si);
    ASSERT(fetch);
    pcs[*iseq_len] = start_pc;
    (*iseq_len)++;
  }

  /*if (!paths_empty(a, i)) {
    src_ulong pc_fallthrough = a[0].edges[0].dst;
    if (!src_insn_is_unconditional_branch_not_fcall(&iseq[*iseq_len - 1])) {
      src_insn_put_jump_to_pcrel(pc_fallthrough, &iseq[*iseq_len], 1);
      pcs[*iseq_len] = INSN_PC_INVALID;
      (*iseq_len)++;
    }
  }*/

  for (i = alen - 1; i >= 0; i--) {
    for (j = 0; j < a[i].depth - 1; j++) {
      if (edges_search(seen, num_seen, &a[i].edges[j]) == -1) {
        DBG2(SRC_INSN, "%lx: calling append_pcs(%s->%s)\n", (long)start_pc,
            a[i].edges[j].dst.nextpc_to_string().c_str(), a[i].edges[j + 1].dst.nextpc_to_string().c_str());
        CPP_DBG_EXEC2(SRC_INSN, cout << __func__ << " " << __LINE__ << ": iseq =\n" << src_iseq_to_string(iseq, *iseq_len, as1, sizeof as1) << endl);
        append_pcs(in, pcs, iseq, iseq_len, a[i].edges[j].dst, a[i].edges[j + 1].dst);
        seen[num_seen].src = a[i].edges[j].src;
        seen[num_seen].dst = a[i].edges[j].dst;
        num_seen++;
      }
    }
    nextpc_t const& pc_fallthrough = a[i].edges[a[i].depth - 1].dst;
    ASSERT(a[i].depth >= 2);
    if (   (i == alen - 1 && !paths_empty(a, i))
        || !paths_empty(&a[i], 1)) {
      if (!src_insn_is_unconditional_branch_not_fcall(&iseq[*iseq_len - 1])) {
        *iseq_len += add_jump_to_pcrel_src_insns(pc_fallthrough, &iseq[*iseq_len], &pcs[*iseq_len], 1);
      }
    }
  }
}

static void
src_iseq_fetch_max(input_exec_t const *in, src_ulong pc, si_iseq_t *si_iseq,
    /*, src_insn_t *iseq,
    long *iseq_len, src_ulong *pcs, long *num_out_edges, edge_t *out_edges,
    double *out_edge_weights, long *num_function_calls, src_ulong *function_calls,
    long *fetchlen, fetch_branch_t *paths, long *num_paths,
    src_ulong *fallthrough_pc*/bool first)
{
  long i;

  i = pc2index(in, pc);
  ASSERT(i >= 0 && i < in->num_si);
  /*if (first) {
    si_iseq = &in->si[i].max_first;
  } else {
    si_iseq = &in->si[i].max_middle;
  }*/

  ASSERT(si_iseq->iseq_len == 0);

  DBG2(SRC_INSN, "called on %lx. calling src_iseq_fetch_len\n", (long)pc);
  src_iseq_fetch_len(in, NULL, pc, si_iseq, large_int_minusone(), first, true);
/*iseq, iseq_len, pcs, num_out_edges, out_edges,
      out_edge_weights, num_function_calls, function_calls, fetchlen, LONG_MAX,
      paths, num_paths, fallthrough_pc, first);*/

  /*fill_si_iseq(in, si_iseq, iseq_len, iseq, pcs, num_out_edges,
      out_edges,
      out_edge_weights, num_function_calls, function_calls, fetchlen, paths,
      num_paths, fallthrough_pc, false);

  paths_free(paths, num_paths);

  free(pcs);
  free(function_calls);
  free(out_edge_weights);
  free(paths);
  free(out_edges);
  free(iseq);
  return si_iseq;*/
}

static void
compute_stepsize(large_int_t *ret, input_exec_t const *in, src_ulong start_pc,
    fetch_branch_t const *paths, long num_paths)
{
  long max_alloc = (in->num_si + 100) * 2;
  fetch_branch_t *paths_copy;
  large_int_t increment;
  //long ret;

  paths_copy = (fetch_branch_t *)malloc(max_alloc * sizeof(fetch_branch_t));
  ASSERT(paths_copy);

  paths_deep_copy_with_alloc(paths_copy, paths, num_paths, in->num_bbls + 1);

  //ret = 1;
  large_int_copy(ret, large_int_one());
  while (src_iseq_fetch_next(in, start_pc, paths_copy, &num_paths, true, large_int_minusone(),
      &increment) != -1) {
    //ret += increment;
    large_int_add(ret, ret, &increment);
  }

  paths_free(paths_copy, num_paths);
  free(paths_copy);
  //return ret;
}

static char *
paths_to_string(fetch_branch_t const *paths, long num_paths, char *buf, size_t size)
{
  char *ptr, *end;
  long i, j;

  ptr = buf;
  end = buf + size;

  *ptr = '\0';
  for (i = 0; i < num_paths; i++) {
    ptr += snprintf(ptr, end - ptr, "\t%ld:", i);
    for (j = 0; j < paths[i].depth; j++) {
      ptr += snprintf(ptr, end - ptr, " (%lx->%s)", (long)paths[i].edges[j].src,
          paths[i].edges[j].dst.nextpc_to_string().c_str());
      ASSERT(ptr < end);
    }
    ptr += snprintf(ptr, end - ptr, "\n");
    ASSERT(ptr < end);
  }
  /*ptr += snprintf(ptr, end - ptr, "locks:\n");
  for (i = 0; i < num_paths; i++) {
    ptr += snprintf(ptr, end - ptr, "\t%ld:", i);
    for (j = 0; j < paths[i].num_locks; j++) {
      ptr += snprintf(ptr, end - ptr, " %lx[depth %ld]", (long)paths[i].locks[j].pc,
          paths[i].locks[j].depth);
    }
    ptr += snprintf(ptr, end - ptr, "\n");
  }*/
  return buf;
}

static void
edge_copy(struct edge_t *dst, struct edge_t const *src)
{
  if (dst == src) {
    return;
  }
  memcpy(dst, src, sizeof(struct edge_t));
}

static void
lock_copy(struct lock_t *dst, struct lock_t const *src)
{
  if (dst == src) {
    return;
  }
  memcpy(dst, src, sizeof(struct lock_t));
}

static void
prepend_path(fetch_branch_t *dst, fetch_branch_t const *src, bool multiple)
{
  long i, prepend_src_depth_start, prepend_src_depth_stop, prepend_len;
  long dst_depth_start, dst_depth_stop;

  DBG2(SRC_INSN, "before dst:\n%s\n", paths_to_string(dst, 1, as1, sizeof as1));
  DBG2(SRC_INSN, "src:\n%s\n", paths_to_string(src, 1, as1, sizeof as1));

  ASSERT(src->depth >= 2);
  ASSERT(dst->depth >= 2);
  //ASSERT(dst->edges[0].src == src->edges[src->depth - 1].dst);

  prepend_src_depth_start = 0;
  prepend_src_depth_stop = src->depth - 1;
  if (multiple) {
    dst_depth_start = 0;
    dst_depth_stop = dst->depth;
  } else {
    dst_depth_start = 1;
    dst_depth_stop = dst->depth;
  }

  prepend_len = prepend_src_depth_stop - prepend_src_depth_start;

  //ASSERT(dst->edges alloc size == in->num_bbls + 1 >= dst->depth + src->depth)
  for (i = dst_depth_stop - 1; i >= dst_depth_start; i--) {
    edge_copy(&dst->edges[i + prepend_len - dst_depth_start], &dst->edges[i]);
  }
  for (i = prepend_src_depth_start; i < prepend_src_depth_stop; i++) {
    edge_copy(&dst->edges[i], &src->edges[i]);
  }
  dst->depth = dst_depth_stop - dst_depth_start + prepend_len;
  //edge_copy(&dst->edges[dst->depth - 1], &src->edges[prepend_src_depth_stop]);
  //ASSERT(dst->depth < in->num_bbls + 1);

  //ASSERT(dst->locks alloc size == in->num_bbls + 1 >= dst->depth + src->depth)
  dst->locks = (lock_t *)realloc(dst->locks, (dst->num_locks + src->num_locks) *
      sizeof(lock_t));
  for (i = dst->num_locks - 1; i >= 0; i--) {
    ASSERT(dst->locks[i].depth >= dst_depth_start);
    lock_copy(&dst->locks[i + src->num_locks], &dst->locks[i]);
    dst->locks[i + src->num_locks].depth += prepend_len;
  }
  for (i = 0; i < src->num_locks; i++) {
    lock_copy(&dst->locks[i], &src->locks[i]);
  }
  dst->num_locks += src->num_locks;
  dst->weight *= src->weight;
  //ASSERT(dst->num_locks < in->num_bbls + 1);
  DBG2(SRC_INSN, "after dst:\n%s\n", paths_to_string(dst, 1, as1, sizeof as1));
}

/* This function modifies numpaths and curp only if it returns true. */
static bool
src_iseq_fetch_next_elem(input_exec_t const *in,
    src_ulong start_pc, fetch_branch_t *paths, long *num_paths, long *curp,
    bool get_max, large_int_t const *max_fetchlen, large_int_t *increment)
{
  long undominated_preds, reached_preds;
  fetch_branch_t cur_fetch_entry, *b;
  long i, blen, num_preds, cur;
  bool needs_lock, bret;
  edge_t const *edge;
  src_ulong *preds;
  src_insn_t insn;
  si_iseq_t *e, f;
  edge_t *edge2;

  cur = *curp;
  path_copy(&cur_fetch_entry, &paths[cur]);
  if (cur_fetch_entry.locked) {
    DBG2(SRC_ISEQ_FETCH, "%s", "returning false because cur_fetch_entry is locked.\n");
    return false;
  }

  edge = get_last_edge(&cur_fetch_entry);

  if (   edge->dst.get_tag() != tag_const
      || edge->dst.get_val() == PC_JUMPTABLE
      || edge->dst.get_val() == PC_UNDEFINED
      || !pc_is_executable/*is_possible_nip*/(in, edge->dst.get_val())
      || edge->dst.get_val() == start_pc) {
    DBG2(SRC_ISEQ_FETCH, "returning false because edge->dst is %s.\n", edge->dst.nextpc_to_string().c_str());
    return false;
  }

  if (pc_postdominates(in, edge->dst.get_val(), start_pc)) {
    DBG2(SRC_ISEQ_FETCH, "returning false because edge->dst %lx post-dominates "
            "start_pc %lx.\n", (long)edge->dst.get_val(), (long)start_pc);
    return false;
  }
  //cout << __func__ << " " << __LINE__ << hex << ": start_pc = " << start_pc << ", edge->dst = " << edge->dst << dec << endl;
  if (!pc_dominates(in, start_pc, edge->dst.get_val())) {
    DBG2(SRC_ISEQ_FETCH, "returning false because start_pc %lx does not dominate "
            "edge->dst %lx.\n", (long)start_pc, (long)edge->dst.get_val());
    return false;
  }
  if (edge->src != PC_UNDEFINED && edge->dst.get_val() == start_pc) {
    DBG2(SRC_ISEQ_FETCH, "returning false because edge->dst is %lx is startpc\n",
        (long)edge->dst.get_val());
    return false;
  }

  preds = (src_ulong *)malloc(in->num_bbls * sizeof(src_ulong));
  ASSERT(preds);
  num_preds = pc_get_preds(in, edge->dst.get_val(), preds);

  undominated_preds = 0;
  for (i = 0; i < num_preds; i++) {
    ASSERT(preds[i] != PC_JUMPTABLE);
    if (   pc_is_reachable(in, preds[i])
        && !pc_dominates(in, edge->dst.get_val(), preds[i])) {
      undominated_preds++;
    }
  }
  free(preds);

  reached_preds = 0;
  for (i = 0; i < cur; i++) {
    if (get_last_edge(&paths[i])->dst.equals(edge->dst)) {
      reached_preds++;
    }
  }

  if (reached_preds != undominated_preds - 1) {
    DBG2(SRC_ISEQ_FETCH, "returning false because we have not yet reached all preds of "
        "the current frontier. reached_preds = %ld, undominated_preds = %ld.\n",
        reached_preds, undominated_preds);
    return false;  /* we have not yet reached all preds of the current frontier */
  }

  i = pc2index(in, edge->dst.get_val());
  ASSERT(i >= 0 && i < in->num_si);
  e = &in->si[i].max_middle;
  if (e->iseq_len == 0) {
    ASSERT(!e->iseq);
    //cout << __func__ << " " << __LINE__ << ": edge->dst = " << hex << edge->dst << dec << endl;
    src_iseq_fetch_max(in, edge->dst.get_val(), e, false);
  }
  ASSERT(e->iseq_len != 0);

  if (e->iseq_len == -1) {
    return false;
  }

  ASSERT(e->iseq_len > 0);

  /*nextpc_weights = (double *)malloc(in->num_si * sizeof(double));
  ASSERT(nextpc_weights);
  next_edges = (edge_t *)malloc(in->num_si * sizeof(edge_t));
  ASSERT(next_edges);

  num_nextpcs = pc_get_next_edges_reducing_loops(in, edge->dst, next_edges,
      nextpc_weights);
  if (num_nextpcs == 0) { // infinite loop, ignore.
    free(nextpc_weights);
    free(next_edges);
    return false;
  }*/

  /* returning true. */
  b = (fetch_branch_t *)malloc((in->num_si + 1) * sizeof(fetch_branch_t));
  ASSERT(b);
  blen = *num_paths - cur - 1;
  ASSERT(blen < in->num_si + 1);
  for (i = 0; i < blen; i++) {
    path_copy(&b[i], &paths[cur + 1 + i]);
  }

  /* lock reached preds */
  needs_lock = false;
  for (i = 0; i < cur; i++) {
    if (get_last_edge(&paths[i])->dst.get_val() == edge->dst.get_val()) {
      paths[i].locked = true;
      needs_lock = true;
    }
  }

  long locked_paths[cur + 1];
  long num_locked_paths;

  num_locked_paths = 0;
  for (i = 0; i < cur; i++) {
    if (paths[i].locked) {
      locked_paths[num_locked_paths] = i;
      num_locked_paths++;
    }
  }
  locked_paths[num_locked_paths] = cur;
  num_locked_paths++;
  //cur = src_iseq_fetch_reset(paths, cur, locked_paths, num_locked_paths);
  extern bool ignore_fetchlen_reset; //hack for now
  if (!ignore_fetchlen_reset) {
    cur = src_iseq_fetch_reset(paths, cur, locked_paths, num_locked_paths);
  }

  DBG2(SRC_ISEQ_FETCH, "%lx: after reset, paths:\n%s\n", (long)start_pc,
      paths_to_string(paths, cur, as1, sizeof as1));

  ASSERT(get_max || large_int_compare(max_fetchlen, large_int_one()) >= 0);
  DBG2(SRC_ISEQ_FETCH, "%lx: pc %lx, e->iseq_fetchlen = %s\n", (long)start_pc,
      (long)edge->dst.get_val(), large_int_to_string(&e->iseq_fetchlen, as1, sizeof as1));

  if (large_int_compare(&e->iseq_fetchlen, large_int_one()) > 0) {
    large_int_t stepsize, quotient_max_fetchlen;

    compute_stepsize(&stepsize, in, start_pc, paths, cur);
    large_int_sub(&quotient_max_fetchlen, max_fetchlen, large_int_one());
    large_int_div(&quotient_max_fetchlen, &quotient_max_fetchlen, &stepsize);
    large_int_add(&quotient_max_fetchlen, &quotient_max_fetchlen, large_int_one());
    //quotient_max_fetchlen = ((max_fetchlen - 1)/stepsize) + 1;

    if (!get_max && large_int_compare(&e->iseq_fetchlen, &quotient_max_fetchlen) > 0) {
      DBG2(SRC_ISEQ_FETCH, "calling src_iseq_fetch_len on %lx.\n", (long)edge->dst.get_val());
      src_iseq_fetch_len(in, NULL, edge->dst.get_val(), &f, &quotient_max_fetchlen, false, get_max);
      e = &f;
      ASSERT(e->iseq_len > 0);
      ASSERT(large_int_compare(&e->iseq_fetchlen, &quotient_max_fetchlen) == 0);
    }
    //*increment = ((e->iseq_fetchlen - 1) * stepsize) + 1;
    large_int_sub(increment, &e->iseq_fetchlen, large_int_one());
    large_int_mul(increment, increment, &stepsize);
    large_int_add(increment, increment, large_int_one());
  } else {
    large_int_copy(increment, large_int_one());
    //*increment = 1;
  }

  for (i = 0; i < e->num_paths; i++) {
    paths_deep_copy_with_alloc(&paths[cur + i], &e->paths[i], 1, in->num_bbls + 1);
    prepend_path(&paths[cur + i], &cur_fetch_entry, e->num_paths > 1);
    ASSERT(paths[cur + i].depth <= in->num_bbls + 1);
    ASSERT(paths[cur + i].num_locks <= in->num_bbls + 1);
    //edge2 = get_last_edge(&paths[cur + i]);
    //edge2->src = e->out_edges[i].src;
    //edge2->dst = e->out_edges[i].dst;
    //DBG3(SRC_INSN, "Replacing with (%lx->%lx)\n", (long)edge2->src, (long)edge2->dst);

    /*if (e->num_out_edges > 1) {
      paths[cur + i].depth++;
      edge2 = get_last_edge(&paths[cur + i]);
      edge2->src = e->out_edges[i].src;
      edge2->dst = e->out_edges[i].dst;
      paths[cur + i].weight = cur_fetch_entry.weight * e->out_edge_weights[i];
    }*/
    DBG2(SRC_ISEQ_FETCH, "after prepend_path, paths[%ld]:\n%s\n", cur + i,
        paths_to_string(&paths[cur + i], 1, as1, sizeof as1));
    if (needs_lock) {
      add_lock(&paths[cur + i], edge->dst.get_val());
    }
    DBG2(SRC_ISEQ_FETCH, "after adding lock, paths[%ld]:\n%s\n", cur + i,
        paths_to_string(&paths[cur + i], 1, as1, sizeof as1));
  }
  paths_free(&cur_fetch_entry, 1);
  for (i = 0; i < blen; i++) {
    path_copy(&paths[cur + e->num_paths + i], &b[i]);
  }
  *num_paths = cur + e->num_paths + blen;
  *curp = cur;
  ASSERT(*curp < *num_paths);

  //free(nextpc_weights);
  //free(next_edges);
  free(b);
  if (e == &f) {
    si_iseq_free(e);
  }
  return true;
}

static long
src_iseq_fetch_next(input_exec_t const *in, src_ulong start_pc, fetch_branch_t *paths,
    long *num_paths, bool get_max, large_int_t const *max_fetchlen, large_int_t *increment)
{
  bool found;
  long i;

  /*if (!get_max && max_fetchlen == 0) {
    return -1;
  }*/

  found = false;
  for (i = 0; i < *num_paths; i++) {
    DBG2(SRC_ISEQ_FETCH, "%lx: calling src_iseq_fetch_next_elem() on %ldth path.\n",
        (long)start_pc, i);
    if (src_iseq_fetch_next_elem(in, start_pc, paths, num_paths, &i, get_max, max_fetchlen,
        increment)) {
      DBG2(SRC_ISEQ_FETCH, "%lx: succeeded on %ldth path.\n", (long)start_pc, i);
      found = true;
      break;
    }
    DBG2(SRC_ISEQ_FETCH, "%lx: failed on %ldth path.\n", (long)start_pc, i);
  }

  ASSERT(found || (i == *num_paths));
  ASSERT(!found || (i < *num_paths));
  if (i == *num_paths) {
    ASSERT(!found);
    /* this can only happen if all calls to src_iseq_fetch_next_elem failed. */
    return -1;
  }
  return i;
}

static void
src_iseq_construct_out_edges(fetch_branch_t const *paths, long num_paths,
    long *num_out_edges, edge_t *out_edges, double *out_edge_weights)
{
  double weights[num_paths];
  edge_t const *edge;
  long i, j;

  for (i = 0; i < num_paths; i++) {
    weights[i] = 0;
  }
  *num_out_edges = 0;

  /* fill lastpcs, lastpc_weights, nextpcs. */
  for (i = 0; i < num_paths; i++) {
    weights[i] += paths[i].weight;
    edge = get_last_edge_const(&paths[i]);
    if (paths[i].locked) {
      for (j = i + 1; j < num_paths; j++) {
        if (locks_search(paths[j].locks, paths[j].num_locks, edge->dst.get_val()) != -1) {
          weights[j] += weights[i];
          break;
        }
      }
    } else {
      out_edges[*num_out_edges].src = edge->src;
      out_edges[*num_out_edges].dst = edge->dst;
      out_edge_weights[*num_out_edges] = weights[i];
      (*num_out_edges)++;
    }
  }
}

static void
src_iseq_identify_function_calls(input_exec_t const *in,
    src_insn_t const *iseq, src_ulong const *pcs,
    long iseq_len, long num_out_edges, edge_t const *out_edges,
    long *num_function_calls, nextpc_t *function_calls)
{
  nextpc_t func;
  bool found;
  long i, j;

  *num_function_calls = 0;
  for (i = 0; i < iseq_len; i++) {
    if (!src_insn_is_function_call(&iseq[i])) {
      continue;
    }
    if (src_insn_is_indirect_function_call(&iseq[i])) {
      func = PC_JUMPTABLE;
    } else {
      func = src_insn_fcall_target(&iseq[i], 0, in);
      //cout << __func__ << " " << __LINE__ << ": func = " << func.nextpc_to_string() << endl;
    }
    if (func.get_tag() == tag_const && array_search(pcs, iseq_len, func.get_val()) != -1) {
      continue;
    }
    found = false;
    for (j = 0; j < num_out_edges && !found; j++) {
      if (out_edges[j].dst.equals(func)) {
        found = true;
      }
    }
    if (found) {
      continue;
    }
    if (nextpc_t::nextpc_array_search(function_calls, *num_function_calls, func) == -1) {
      continue;
    }
    function_calls[*num_function_calls] = func;
    (*num_function_calls)++;
  }
}

static void
src_iseq_construct(input_exec_t const *in, src_ulong start_pc, fetch_branch_t const *a,
    long alen, src_insn_t *iseq, long *iseq_len, src_ulong *pcs, long *num_out_edges,
    edge_t *out_edges, double *out_edge_weights, long *num_function_calls,
    nextpc_t *function_calls, bool first)
{
  src_iseq_construct_pcs(in, start_pc, a, alen, iseq_len, iseq, pcs, first);
  src_iseq_construct_out_edges(a, alen, num_out_edges, out_edges, out_edge_weights);
  src_iseq_identify_function_calls(in, iseq, pcs, *iseq_len, *num_out_edges, out_edges,
      num_function_calls, function_calls);
  DBG(SRC_ISEQ_FETCH, "%s() %d: start_pc = %lx, iseq =\n%s\n", __func__, __LINE__, (unsigned long)start_pc, src_iseq_to_string(iseq, *iseq_len, as1, sizeof as1));
  DBG_EXEC(SRC_ISEQ_FETCH,
      int i;
       for (i = 0; i < *iseq_len; i++) {
         printf("pcs[%d] = 0x%lx\n", i, (unsigned long)pcs[i]);
       }
  );
}

static bool
src_iseq_fetch_iterate(input_exec_t const *in,
    struct peep_table_t const *peep_table, src_ulong pc,
    large_int_t *fetchlen, large_int_t const *max_fetchlen, fetch_branch_t *paths,
    long *num_paths, bool first, bool get_max)
{
  long path_index, iseq_len, i, bnum_paths, orig_bnum_paths;
  fetch_branch_t *bpaths;
  large_int_t increment;
  src_insn_t *iseq;
  src_ulong *pcs;

  bpaths = (fetch_branch_t *)malloc(in->num_si * sizeof(fetch_branch_t));
  ASSERT(bpaths);
  //iseq = (src_insn_t *)malloc(in->num_si * sizeof(src_insn_t));
  iseq = new src_insn_t[in->num_si];
  ASSERT(iseq);
  pcs = (src_ulong *)malloc(in->num_si * sizeof(src_ulong));
  ASSERT(pcs);
  while (large_int_compare(fetchlen, (get_max?SI_MAX_FETCHLEN:max_fetchlen)) < 0) {
    large_int_t remainder_fetchlen;
    large_int_sub(&remainder_fetchlen, max_fetchlen, fetchlen);
    DBG2(SRC_ISEQ_FETCH, "*fetchlen %s, paths:\n%s\n",
        large_int_to_string(fetchlen, as2, sizeof as2),
        paths_to_string(paths, *num_paths, as1, sizeof as1));
    path_index = src_iseq_fetch_next(in, pc, paths, num_paths,
        get_max, get_max?SI_MAX_FETCHLEN:&remainder_fetchlen, &increment);
    DBG2(SRC_ISEQ_FETCH, "%lx: path_index = %ld\n", (long)pc, path_index);
    if (path_index == -1) {
      break;
    }
    large_int_add(fetchlen, fetchlen, &increment);
    //(*fetchlen) += increment;

    DBG2(SRC_ISEQ_FETCH, "%lx: fetchlen %s, index %ld:\n%s\n", (long)pc,
        large_int_to_string(fetchlen, as2, sizeof as2),
        path_index, paths_to_string(paths, *num_paths, as1, sizeof as1));
    ASSERT(path_index < *num_paths);

    if (peep_table) {
      src_iseq_construct_pcs(in, pc, paths, *num_paths, &iseq_len, iseq, pcs,
          first);
      if (!peep_entry_exists_for_prefix_insns(peep_table, iseq, iseq_len)) {
        /* ignore all fetchlens exploring 0..path_index. */
        large_int_t residue_fetchlen;

        bnum_paths = path_index + 1;
        orig_bnum_paths = bnum_paths;
        memcpy(bpaths, paths, bnum_paths * sizeof(fetch_branch_t));
        ASSERT(!get_max);
        //ASSERT(*fetchlen <= max_fetchlen);
        ASSERT(large_int_compare(fetchlen, max_fetchlen) <= 0);
        large_int_sub(&residue_fetchlen, max_fetchlen, fetchlen);
        while (   (large_int_compare(fetchlen, max_fetchlen) == 0)  //(*fetchlen == max_fetchlen)
               || (src_iseq_fetch_next(in, pc, bpaths, &bnum_paths,
                      get_max, &residue_fetchlen, &increment) != -1)) {
          if (large_int_compare(fetchlen, max_fetchlen) == 0) { // *fetchlen == max_fetchlen
            paths_free(bpaths, bnum_paths);
            paths_free(&paths[path_index + 1], *num_paths - path_index - 1);
            free(bpaths);
            //free(iseq);
            delete [] iseq;
            free(pcs);
            return false;
          }
          //(*fetchlen) += increment;
          large_int_add(fetchlen, fetchlen, &increment);
        }
        memmove(&paths[bnum_paths], &paths[orig_bnum_paths],
            (*num_paths - bnum_paths) * sizeof(fetch_branch_t));
        memcpy(paths, bpaths, bnum_paths * sizeof(fetch_branch_t));
        *num_paths += bnum_paths - orig_bnum_paths;
      }
    }
  }
  free(bpaths);
  //free(iseq);
  delete [] iseq;
  free(pcs);
  return true;
}

static bool
src_iseq_fetch_init_paths(input_exec_t const *in, src_ulong pc, fetch_branch_t *paths,
    long *num_paths, bool first)
{
  long i, num_nextpcs, insn_size, pc_index;
  double *nextpc_weights;
  nextpc_t *nextpcs;
  edge_t *next_edges;
  src_insn_t insn;
  bool fetch;

  *num_paths = 0;
  if (!pc_is_executable/*is_possible_nip*/(in, pc)) {
    DBG(SRC_INSN, "%lx: returning false because !is_possible_nip.\n", (long)pc);
    return false;
  }
  pc_index = pc2index(in, pc);
  ASSERT(pc_index >= 0 && pc_index < in->num_si);
  fetch = si_insn_fetch(&insn, &in->si[pc_index]);
  if (!fetch) {
    DBG(SRC_INSN, "%lx: returning false because fetch failed.\n", (long)pc);
    return false;
  }
#if ARCH_SRC != ARCH_ETFG
  if (!harvest_unsupported_opcodes && !superoptimizer_supports_src(&insn, 1)) {
    DBG(SRC_INSN, "%lx: returning false because unsupported opcode.\n", (long)pc);
    return false;
  }
#endif

  //nextpcs = (src_ulong *)malloc((in->num_bbls + 2) * sizeof(src_ulong));
  //ASSERT(nextpcs);
  nextpcs = new nextpc_t[(in->num_bbls + 2)];
  ASSERT(nextpcs);
  nextpc_weights = (double *)malloc((in->num_bbls + 2) * sizeof(src_ulong));
  ASSERT(nextpc_weights);
  //next_edges = (edge_t *)malloc((in->num_bbls + 2) * sizeof(edge_t));
  next_edges = new edge_t[in->num_bbls + 2];
  ASSERT(next_edges);

  if (first) {
    num_nextpcs = pc_get_nextpcs(in, pc, nextpcs, nextpc_weights);
    for (i = 0; i < num_nextpcs; i++) {
      next_edges[i].src = pc;
      next_edges[i].dst = nextpcs[i];
    }
  } else {
    num_nextpcs = pc_get_next_edges_reducing_loops(in, pc, next_edges, nextpc_weights);
  }

  if (num_nextpcs == 0) {
    //free(nextpcs);
    delete[] nextpcs;
    free(nextpc_weights);
    //free(next_edges);
    delete[] next_edges;
    DBG(SRC_INSN, "%lx: returning false because num_nextpcs=0, first = %d.\n", (long)pc, (int)first);
    return false;
  }
#if HARVEST_LOOPS == 0
  for (i = 0; i < num_nextpcs; i++) {
    if (next_edges[i].dst.get_tag() == tag_const && next_edges[i].dst.get_val() == start_pc) {
      //free(nextpcs);
      delete[] nextpcs;
      free(nextpc_weights);
      //free(next_edges);
      delete[] next_edges;
      DBG(SRC_INSN, "%lx: returning false because found loop.\n", (long)pc);
      return false;
    }
  }
#endif

  for (i = 0; i < num_nextpcs; i++) {
    path_alloc(&paths[i], (in->num_bbls + 1) * 2);
    paths[i].edges[0].src = next_edges[i].src;
    paths[i].edges[0].dst = next_edges[i].dst;
    paths[i].edges[1].src = next_edges[i].src;
    paths[i].edges[1].dst = next_edges[i].dst;
    paths[i].num_locks = 0;
    paths[i].locked = false;
    paths[i].depth = 2;
    paths[i].weight = nextpc_weights[i];
  }
  *num_paths = num_nextpcs;
  DBG2(SRC_INSN, "%lx: %s\n", (long)pc,
      paths_to_string(paths, *num_paths, as1, sizeof as1));
  //free(nextpcs);
  delete[] nextpcs;
  free(nextpc_weights);
  //free(next_edges);
  delete[] next_edges;
  return true;
}

static long
si_find_cached_iseq_replacement(si_iseq_t const *cached)
{
  large_int_t max_fetchlen;
  long i, m;

  //max_fetchlen = 0;
  large_int_copy(&max_fetchlen, large_int_zero());
  m = -1;
  for (i = 0; i < SI_NUM_CACHED; i++) {
    if (large_int_compare(&cached[i].iseq_fetchlen, large_int_zero()) == 0) {
      return i;
    } else if (large_int_compare(&cached[i].iseq_fetchlen, &max_fetchlen) > 0) {
      //max_fetchlen = cached[i].iseq_fetchlen;
      large_int_copy(&max_fetchlen, &cached[i].iseq_fetchlen);
      m = i;
    }
  }
  return m;
}

static void
fill_si_iseq(input_exec_t const *in, si_iseq_t *e, long iseq_len, src_insn_t const *iseq,
    src_ulong const *pcs, long num_out_edges, edge_t const *out_edges,
    double const *out_edge_weights, long num_function_calls,
    nextpc_t const *function_calls, large_int_t const *fetchlen, fetch_branch_t const *paths,
    long num_paths, nextpc_t const& fallthrough_pc, bool first)
{
  long l;

  si_iseq_free(e);

  if (iseq_len == -1) {
    e->iseq_len = -1;
    ASSERT(large_int_compare(fetchlen, large_int_zero()) == 0);
    large_int_copy(&e->iseq_fetchlen, large_int_zero());
    //e->iseq_fetchlen = 0;
    return;
  }

  /* for (l = 0; l < MAX(iseq_len, e->iseq_len); l++) {
    printf("  %lx <-> %lx\n", (l < iseq_len)?(long)pcs[l]:-1,
        (l < e->iseq_len)?(long)e->pcs[l]:-1);
  } */
  ASSERT(iseq_len > 0);

  //e->iseq = (src_insn_t *)malloc(iseq_len * sizeof(src_insn_t));
  e->iseq = new src_insn_t[iseq_len];
  ASSERT(e->iseq);
  e->pcs = (src_ulong *)malloc(iseq_len * sizeof(src_ulong));
  ASSERT(e->pcs);

  e->iseq_len = iseq_len;
  //e->iseq_fetchlen = fetchlen;
  large_int_copy(&e->iseq_fetchlen, fetchlen);
  DBG2(SRC_INSN, "paths: %s\n", paths_to_string(paths, num_paths, as1, sizeof as1));

  for (l = 0; l < iseq_len; l++) {
    src_insn_copy(&e->iseq[l], &iseq[l]);
    e->pcs[l] = pcs[l];
  }

  //e->out_edges = (edge_t *)malloc(num_out_edges * sizeof(edge_t));
  e->out_edges = new edge_t[num_out_edges];
  ASSERT(e->out_edges);
  e->num_out_edges = num_out_edges;
  //memcpy(e->out_edges, out_edges, num_out_edges * sizeof(edge_t));
  edges_copy(e->out_edges, out_edges, num_out_edges);

  e->out_edge_weights = (double *)malloc(num_out_edges * sizeof(double));
  ASSERT(e->out_edge_weights);
  memcpy(e->out_edge_weights, out_edge_weights, num_out_edges * sizeof(double));

  //e->function_calls = (src_ulong *)malloc(num_function_calls * sizeof(src_ulong));
  e->function_calls = new nextpc_t[num_function_calls];
  ASSERT(e->function_calls);
  e->num_function_calls = num_function_calls;
  nextpc_t::nextpc_array_copy(e->function_calls, function_calls, num_function_calls);
  //memcpy(e->function_calls, function_calls, num_function_calls * sizeof(src_ulong));

  compute_nextpcs_and_lastpcs_from_out_edges(in, &e->num_nextpcs, NULL, NULL, NULL, NULL,
    e->num_out_edges, e->out_edges, e->out_edge_weights, e->iseq_len, e->pcs);

  e->paths = (fetch_branch_t *)malloc(num_paths * sizeof(fetch_branch_t));
  ASSERT(e->paths);
  e->num_paths = num_paths;
  paths_deep_copy_with_alloc(e->paths, paths, num_paths, 0);

  e->fallthrough_pc = fallthrough_pc;
  e->first = first;
}

static void
si_iseq_resize(si_iseq_t *e)
{
  src_ulong *pcs;
  nextpc_t *function_calls;
  double *out_edge_weights;
  fetch_branch_t *paths;
  edge_t *out_edges;
  src_insn_t *iseq;

  ASSERT(e->num_paths > 0);
  paths = (fetch_branch_t *)malloc(e->num_paths * sizeof(fetch_branch_t));
  ASSERT(paths);
  memcpy(paths, e->paths, e->num_paths * sizeof(fetch_branch_t));
  free(e->paths);
  e->paths = paths;
  //e->paths = (fetch_branch_t *)realloc(e->paths,
  //    e->num_paths * sizeof(fetch_branch_t));
  //ASSERT(e->paths);

  ASSERT(e->iseq_len > 0);
  //iseq = (src_insn_t *)malloc(e->iseq_len * sizeof(src_insn_t));
  iseq = new src_insn_t[e->iseq_len];
  ASSERT(iseq);
  //memcpy(iseq, e->iseq, e->iseq_len * sizeof(src_insn_t));
  src_iseq_copy(iseq, e->iseq, e->iseq_len);
  //free(e->iseq);
  delete [] e->iseq;
  e->iseq = iseq;

  pcs = (src_ulong *)malloc(e->iseq_len * sizeof(src_ulong));
  ASSERT(pcs);
  memcpy(pcs, e->pcs, e->iseq_len * sizeof(src_ulong));
  free(e->pcs);
  e->pcs = pcs;

  ASSERT(e->num_out_edges > 0);
  //out_edges = (edge_t *)malloc(e->num_out_edges * sizeof(edge_t));
  out_edges = new edge_t[e->num_out_edges];
  ASSERT(out_edges);
  //memcpy(out_edges, e->out_edges, e->num_out_edges * sizeof(edge_t));
  edges_copy(out_edges, e->out_edges, e->num_out_edges);
  //free(e->out_edges);
  delete[] e->out_edges;
  e->out_edges = out_edges;

  out_edge_weights = (double *)malloc(e->num_out_edges * sizeof(double));
  ASSERT(out_edge_weights);
  memcpy(out_edge_weights, e->out_edge_weights, e->num_out_edges * sizeof(double));
  free(e->out_edge_weights);
  e->out_edge_weights = out_edge_weights;

  //function_calls = (src_ulong *)malloc(e->num_function_calls * sizeof(src_ulong));
  function_calls = new nextpc_t[e->num_function_calls];
  ASSERT(function_calls);
  //memcpy(function_calls, e->function_calls, e->num_function_calls * sizeof(src_ulong));
  nextpc_t::nextpc_array_copy(function_calls, e->function_calls, e->num_function_calls);
  //free(e->function_calls);
  delete[] e->function_calls;
  e->function_calls = function_calls;

  ASSERT(!e->num_function_calls || e->function_calls);

  resize_paths(e->paths, e->num_paths);
}

static void
src_iseq_fetch_len(input_exec_t const *in, struct peep_table_t const *peep_table,
    src_ulong pc, si_iseq_t *e, large_int_t const *max_fetchlen, bool first, bool get_max)
{
  large_int_t max_fetchlen_seen_so_far;
  long i, m, pc_index, max_alloc;
  si_entry_t *si;

  DBG2(SRC_ISEQ_FETCH, "called on %lx.\n", (long)pc);
  max_alloc = (in->num_si + 4) * 2;
  if (max_alloc < 200) {
    max_alloc = 200;
  }
  e->pcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  ASSERT(e->pcs);
  //e->function_calls = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  e->function_calls = new nextpc_t[max_alloc];
  ASSERT(e->function_calls);
  e->out_edge_weights = (double *)malloc(max_alloc * sizeof(double));
  ASSERT(e->out_edge_weights);
  e->paths = (fetch_branch_t *)malloc(max_alloc * sizeof(fetch_branch_t));
  ASSERT(e->paths);
  //e->out_edges = (edge_t *)malloc(max_alloc * sizeof(edge_t));
  e->out_edges = new edge_t[max_alloc];
  ASSERT(e->out_edges);
  //e->iseq = (src_insn_t *)malloc(max_alloc * sizeof(src_insn_t));
  e->iseq = new src_insn_t[max_alloc];
  ASSERT(e->iseq);

  pc_index = pc2index(in, pc);
  ASSERT(pc_index >= 0 && pc_index < in->num_si);
  si = &in->si[pc_index];
  //max_fetchlen_seen_so_far = 0;
  large_int_copy(&max_fetchlen_seen_so_far, large_int_zero());
  m = -1;
  DBG2(SRC_ISEQ_FETCH, "max_fetchlen = %s\n",
      large_int_to_string(max_fetchlen, as1, sizeof as1));
  for (i = 0; i < SI_NUM_CACHED; i++) {
    if (   large_int_compare(&si->cached[i].iseq_fetchlen, large_int_zero()) != 0
        && si->cached[i].first == first
        && (get_max || large_int_compare(&si->cached[i].iseq_fetchlen, max_fetchlen) <= 0)
        && large_int_compare(&si->cached[i].iseq_fetchlen, &max_fetchlen_seen_so_far) > 0) {
      //max_fetchlen_seen_so_far = si->cached[i].iseq_fetchlen;
      large_int_copy(&max_fetchlen_seen_so_far, &si->cached[i].iseq_fetchlen);
      m = i;
    }
  }
  DBG2(SRC_ISEQ_FETCH, "m = %ld\n", m);
  if (m == -1) {
    m = si_find_cached_iseq_replacement(si->cached);
    if (!src_iseq_fetch_init_paths(in, pc, e->paths, &e->num_paths, first)) {
      DBG(SRC_ISEQ_FETCH, "could not init paths for %lx.\n", (long)pc);
      ASSERT(e->num_paths == 0);
      e->iseq_len = -1;
      //e->iseq_fetchlen = 0;
      large_int_copy(&e->iseq_fetchlen, large_int_zero());
      free(e->pcs);
      e->pcs = NULL;
      //free(e->function_calls);
      delete[] e->function_calls;
      e->function_calls = NULL;
      free(e->out_edge_weights);
      e->out_edge_weights = NULL;
      free(e->paths);
      e->paths = NULL;
      //free(e->out_edges);
      delete[] e->out_edges;
      e->out_edges = NULL;
      //free(e->iseq);
      delete [] e->iseq;
      e->iseq = NULL;
      return;
    }
    DBG2(SRC_ISEQ_FETCH, "%lx: paths inited:\n%s\n", (long)pc,
        paths_to_string(e->paths, e->num_paths, as1, sizeof as1));
    //e->iseq_fetchlen = 1;
    large_int_copy(&e->iseq_fetchlen, large_int_one());
  } else {
    e->num_paths = si->cached[m].num_paths;
    paths_deep_copy_with_alloc(e->paths, si->cached[m].paths, e->num_paths,
        in->num_bbls + 1);
    //e->iseq_fetchlen = si->cached[m].iseq_fetchlen;
    large_int_copy(&e->iseq_fetchlen, &si->cached[m].iseq_fetchlen);
    DBG2(SRC_ISEQ_FETCH, "%lx: max_fetchlen %s, cached fetchlen %s\n", (long)pc,
        large_int_to_string(max_fetchlen, as1, sizeof as1),
        large_int_to_string(&e->iseq_fetchlen, as2, sizeof as2));
  }

  DBG2(SRC_ISEQ_FETCH, "e->paths:\n%s\n",
      paths_to_string(e->paths, e->num_paths, as1, sizeof as1));
  ASSERT(large_int_compare(max_fetchlen, large_int_zero()) != 0);
  if (!src_iseq_fetch_iterate(in, peep_table, pc, &e->iseq_fetchlen, max_fetchlen,
      e->paths, &e->num_paths, first, get_max)) {
    e->num_paths = 0;
    e->iseq_len = -1;
    //e->iseq_fetchlen = 0;
    large_int_copy(&e->iseq_fetchlen, large_int_zero());
    free(e->pcs);
    e->pcs = NULL;
    //free(e->function_calls);
    delete[] e->function_calls;
    e->function_calls = NULL;
    free(e->out_edge_weights);
    e->out_edge_weights = NULL;
    free(e->paths);
    e->paths = NULL;
    //free(e->out_edges);
    delete[] e->out_edges;
    e->out_edges = NULL;
    //free(e->iseq);
    delete [] e->iseq;
    e->iseq = NULL;
    return;
  }
  src_iseq_construct(in, pc, e->paths, e->num_paths, e->iseq, &e->iseq_len, e->pcs,
      &e->num_out_edges, e->out_edges, e->out_edge_weights, &e->num_function_calls,
      e->function_calls, first);
  ASSERT(e->iseq_len <= max_alloc);

  if (e->num_paths > 0) {
    e->fallthrough_pc =
        e->paths[e->num_paths - 1].edges[e->paths[e->num_paths - 1].depth - 1].dst;
  } else {
    e->fallthrough_pc = PC_UNDEFINED;
  }
  if (m != -1) {
    fill_si_iseq(in, &si->cached[m], e->iseq_len, e->iseq, e->pcs, e->num_out_edges,
        e->out_edges, e->out_edge_weights, e->num_function_calls, e->function_calls,
        &e->iseq_fetchlen, e->paths, e->num_paths, e->fallthrough_pc, first);
  }

  si_iseq_resize(e);
}

static bool
si_is_nop(si_entry_t const *si)
{
  src_insn_t insn;
  bool fetch;

  fetch = si_insn_fetch(&insn, si);
  if (!fetch) {
    return true;
  }
  if (src_insn_is_nop(&insn)) {
    return true;
  }
  return false;
}

/*static void
si_mark_zero_iseq_len(input_exec_t const *in)
//XXX: wonder why the compiler is not warning about these violations of const modifier
{
  long i;

  for (i = 0; i < in->num_si; i++) {
    in->si[i].max_first.iseq_len = 0;
    in->si[i].max_middle.iseq_len = 0;
  }
}

static bool
si_some_iseq_len_is_zero(input_exec_t const *in)
{
  long i;

  for (i = 0; i < in->num_si; i++) {
    if (in->si[i].max_first.iseq_len == 0) {
      return true;
    }
    if (in->si[i].max_middle.iseq_len == 0) {
      return true;
    }
  }
  return false;
}*/

static void
fill_si(input_exec_t const *in, char const *function_name)
{
  src_ulong function_pc;
  long i;

  if (function_name && strcmp(function_name, "ALL")) {
    function_pc = function_name2pc(in, function_name);
  } else {
    function_pc = INSN_PC_INVALID;
  }
  /*for (i = 0; i < in->num_si; i++) {
    in->si[i].max_first.iseq_len = 0;
    in->si[i].max_first.iseq = NULL;
    in->si[i].max_middle.iseq_len = 0;
    in->si[i].max_middle.iseq = NULL;
  }*/

  for (i = 0; i < in->num_si; i++) {
    if (   function_pc != INSN_PC_INVALID
        && get_function_containing_pc(in, in->si[i].pc) != function_pc) {
      continue;
    }
    DBG(SRC_ISEQ_FETCH, "filling si for pc %lx\n", (long)in->si[i].pc);
    if (i % 1000 == 0) {
      PROGRESS("fill_si insn %ld/%ld [%.0f%%] %lx", i,
          in->num_si - 1, (double)i*100/in->num_si, (long)in->si[i].pc);
    }

    if (in->si[i].max_middle.iseq_len != 0) {
      continue;
    }
    DBG2(SRC_ISEQ_FETCH, "calling src_iseq_fetch_max(%lx) for max_middle.\n",
        (long)in->si[i].pc);
    src_iseq_fetch_max(in, in->si[i].pc, &in->si[i].max_middle,/*, iseq, &iseq_len, pcs,
        &num_out_edges, out_edges, out_edge_weights, &num_function_calls,
        function_calls, &fetchlen, paths, &num_paths, &fallthrough_pc*/ false);
    /*if (iseq_len != 0) {
    }*/
  }

  for (i = 0; i < in->num_si; i++) {
    if (   function_pc != INSN_PC_INVALID
        && get_function_containing_pc(in, in->si[i].pc) != function_pc) {
      continue;
    }
    DBG(SRC_ISEQ_FETCH, "filling si for pc %lx\n", (long)in->si[i].pc);
    ASSERT(in->si[i].max_first.iseq_len == 0);
    ASSERT(!in->si[i].max_first.iseq);
    DBG2(SRC_ISEQ_FETCH, "calling src_iseq_fetch_max(%lx) for max_first.\n",
        (long)in->si[i].pc);
    DBG2(SRC_ISEQ_FETCH, "in->si[%ld].max_first.iseq=%p.\n", (long)i, in->si[i].max_first.iseq);
    src_iseq_fetch_max(in, in->si[i].pc, &in->si[i].max_first,/*, iseq, &iseq_len, pcs,
        &num_out_edges, out_edges, out_edge_weights, &num_function_calls,
        function_calls, &fetchlen, paths, &num_paths, &fallthrough_pc*/ true);
    /*if (iseq_len != 0) {
      fill_si_iseq(in, &in->si[i].max_first, iseq_len, iseq, pcs, num_out_edges, out_edges,
          out_edge_weights, num_function_calls, function_calls, fetchlen, paths,
          num_paths, fallthrough_pc, true);
      paths_free(paths, num_paths);
    }*/
  }
  PROGRESS("%s", "fill_si done.");
}

static void
si_iseq_print(FILE *file, si_iseq_t const *e)
{
  long i;

  fprintf(file, "\n\tpaths: %s", paths_to_string(e->paths, e->num_paths, as1, sizeof as1));
  fprintf(file, "%s", src_iseq_to_string(e->iseq, e->iseq_len, as1, sizeof as1));
  fprintf(file, "\n\tpcs:");
  for (i = 0; i < e->iseq_len; i++) {
    fprintf(file, " %lx", (unsigned long)e->pcs[i]);
  }
  fprintf(file, "\n\tout_edges:");
  for (i = 0; i < e->num_out_edges; i++) {
    fprintf(file, "\n\t  0x%lx -> %s [%.2f]", (long)e->out_edges[i].src,
        e->out_edges[i].dst.nextpc_to_string().c_str(), e->out_edge_weights[i]);
  }
  fprintf(file, "\n\tfunction_calls:");
  for (i = 0; i < e->num_function_calls; i++) {
    fprintf(file, " %s", e->function_calls[i].nextpc_to_string().c_str());
  }
  fprintf(file, "\nfetchlen = %s\n", large_int_to_string(&e->iseq_fetchlen, as1, sizeof as1));
}

static void
si_entry_print(FILE *file, si_entry_t const *e)
{
  fprintf(file, "%lx:\n\t", (unsigned long)e->pc);
  fprintf(file, "max_first:\n");
  si_iseq_print(file, &e->max_first);
  fprintf(file, "max_middle:\n");
  si_iseq_print(file, &e->max_middle);
}

void
si_print(FILE *file, input_exec_t const *in)
{
  si_entry_t const *data, *si;
  size_t n;
  int i;

  data = in->si;
  n = in->num_si;

  fprintf(file, "\n\nsi for %s (size %zd):\n", in->filename, n);
  for (i = 0; i < n; i++) {
    si_entry_print(file, &data[i]);
  }

  fprintf(logfile, "live_in:\n");
  for (i = 0; i < in->num_si; i++) {
    si = &in->si[i];
    if (si->bbl->live_regs) {
      fprintf(logfile, "%lx: %s\n", (long)si->pc,
          regset_to_string(&si->bbl->live_regs[si->bblindex], as1, sizeof as1));
    }
  }
  fprintf(logfile, "live_ranges_in:\n");
  for (i = 0; i < in->num_si; i++) {
    si = &in->si[i];
    if (si->bbl->live_ranges) {
      fprintf(logfile, "%lx: %s\n", (long)si->pc,
          live_ranges_to_string(&si->bbl->live_ranges[si->bblindex],
              &si->bbl->live_regs[si->bblindex], as1, sizeof as1));
    }
  }
}

void
si_init(input_exec_t *in, char const *function_name)
{
  si_alloc(in);
#if ARCH_SRC == ARCH_ETFG
  in->populate_phi_edge_incident_pcs();
  in->populate_executable_pcs();
#endif
  fill_si(in, function_name);
  DBG_EXEC(SRC_INSN, si_print(stdout, in););
  DBG_EXEC(ASSERTCHECKS, assertcheck_si_init_done = true;);
}

/* static bool
pc_dominates_pcs(input_exec_t const *in, src_ulong pc, src_ulong const *pcs,
    long num_pcs)
{
  long i;

  for (i = 0; i < num_pcs; i++) {
    if (pcs[i] && pcs[i] != PC_UNDEFINED && !pc_dominates(in, pc, pcs[i])) {
      return false;
    }
  }
  return true;
} */

/* static bool
pcs_have_single_entry(input_exec_t const *in, src_ulong const *pcs, long num_pcs)
{
  si_entry_t const *si;
  long i, j, index;
  bool ret = true;

  for (i = 0; i < num_pcs && ret; i++) {
    if (!pcs[i] || pcs[i] == PC_UNDEFINED) {
      continue;
    }
    index = pc2index(in, pcs[i]);
    ASSERT(index >= 0 && index < in->num_si);
    si = &in->si[index];
    if (si->bblindex > 0) {
      continue;
    }
    for (j = 0; j < si->bbl->num_preds; j++) {
      if (si->bbl->preds[j] == PC_JUMPTABLE) {
        ret = false;
        break;
      }
      if (array_search(pcs, num_pcs, si->bbl->preds[j]) == -1) {
        ret = false;
        break;
      }
    }
  }
  DBG2(SRC_ISEQ_FETCH, "returning %s for:", ret?"true":"false");
  DBG_EXEC2(SRC_ISEQ_FETCH,
      long i;
      for (i = 0; i < num_pcs; i++) { printf(" %lx", (long)pcs[i]); }
      printf("\n");
  );
  return true;
} */

static bool
src_iseq_fetch_with_pcs(struct src_insn_t *iseq, size_t iseq_size, src_ulong *pcs, long *len,
    struct input_exec_t const *in, struct peep_table_t const *peep_table,
    src_ulong pc, large_int_t const *fetchlen,
    long *num_lastpcs, src_ulong *lastpcs, double *lastpc_weights,
    nextpc_t *fallthrough_pc, bool allow_shorter)
{
  long num_nextpcs, num_function_calls, max_alloc;
  nextpc_t l_fallthrough_pc;
  nextpc_t *function_calls;
  large_int_t cur_fetchlen, out_fetchlen;
  long i, index, old_len;
  si_entry_t *si_entry;
  src_insn_t *l_iseq;
  nextpc_t *nextpcs;
  src_ulong curpc;
  si_iseq_t *si;
  bool first;

  l_iseq = NULL;
  max_alloc = 2 * (in->num_si + 100);
  if (!iseq) {
    //l_iseq = (src_insn_t *)malloc(max_alloc * sizeof(src_insn_t));
    l_iseq = new src_insn_t[max_alloc];
    ASSERT(l_iseq);
    iseq = l_iseq;
    iseq_size = max_alloc;
  }
  if (!fallthrough_pc) {
    fallthrough_pc = &l_fallthrough_pc;
  }

  //function_calls = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  function_calls = new nextpc_t[max_alloc];
  ASSERT(function_calls);
  //nextpcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  nextpcs = new nextpc_t[max_alloc];
  ASSERT(nextpcs);

  ASSERT(large_int_compare(fetchlen, large_int_zero()) > 0);
  curpc =  pc;
  //cur_fetchlen = fetchlen;
  large_int_copy(&cur_fetchlen, fetchlen);
  //out_fetchlen = 0;
  large_int_copy(&out_fetchlen, large_int_zero());
  *len = 0;
  num_function_calls = 0;

  do {
    if (allow_shorter && curpc == PC_JUMPTABLE) {
      DBG(SRC_ISEQ_FETCH, "pc %lx curpc %lx fetchlen %s: breaking because allow_shorter && curpc == PC_JUMPTABLE.\n", (long)pc, (long)curpc, large_int_to_string(fetchlen, as1, sizeof as1));
      break;
    }
    DBG3(SRC_ISEQ_FETCH, "pc %lx curpc %lx fetchlen %s: pc_dominates(in, pc, curpc) %d\n", (long)pc, (long)curpc, large_int_to_string(fetchlen, as1, sizeof as1), pc_dominates(in, pc, curpc));
    if (   curpc == -1
        || curpc == PC_JUMPTABLE
        || !pc_is_executable/*is_possible_nip*/(in, curpc)
        || !pc_dominates(in, pc, curpc)) {
      if (allow_shorter) {
        DBG(SRC_ISEQ_FETCH, "pc %lx curpc %lx fetchlen %s: breaking because allow_shorter && curpc == -1 || !pc_is_executable || !pc_dominates. pc_is_executable = %d, pc_dominates = %d.\n", (long)pc, (long)curpc, large_int_to_string(fetchlen, as1, sizeof as1), pc_is_executable(in, curpc), pc_dominates(in, pc, curpc));
        break;
      }
      DBG(SRC_ISEQ_FETCH, "pc %lx curpc %lx fetchlen %s: returning false. "
          "is_possible_nip %d\n", (long)pc,
          (long)curpc, large_int_to_string(fetchlen, as1, sizeof as1),
          pc_is_executable/*is_possible_nip*/(in, curpc));
      if (l_iseq) {
        //free(l_iseq);
        delete [] l_iseq;
      }
      //free(function_calls);
      delete[] function_calls;
      //free(nextpcs);
      delete[] nextpcs;
      return false;
    }
    if (!peep_entry_exists_for_prefix_insns(peep_table, iseq, *len)) {
      if (l_iseq) {
        //free(l_iseq);
        delete [] l_iseq;
      }
      //free(function_calls);
      delete[] function_calls;
      //free(nextpcs);
      delete[] nextpcs;
      DBG(SRC_ISEQ_FETCH, "pc %lx fetchlen %s: returning false.\n", (long)pc,
          large_int_to_string(fetchlen, as1, sizeof as1));
      return false;
    }

    old_len = *len;
    num_nextpcs = 0;
    if (num_lastpcs) {
      *num_lastpcs = 0;
    }
    index = pc2index(in, curpc);
    ASSERT(index >= 0 && index < in->num_si);
    si_entry = &in->si[index];
    if (large_int_compare(&out_fetchlen, large_int_zero()) == 0) {
      si = &si_entry->max_first;
      first = true;
    } else {
      si = &si_entry->max_middle;
      first = false;
    }

    DBG2(SRC_ISEQ_FETCH, "Fetching instruction at 0x%lx. cur_fetchlen=%s\n", (long)curpc,
        large_int_to_string(&cur_fetchlen, as1, sizeof as1));

    if (large_int_compare(&si->iseq_fetchlen, large_int_zero()) == 0) {
      if (l_iseq) {
        //free(l_iseq);
        delete [] l_iseq;
      }
      //free(function_calls);
      delete[] function_calls;
      //free(nextpcs);
      delete[] nextpcs;
      DBG(SRC_ISEQ_FETCH, "pc 0x%lx fetchlen %s: curpc = 0x%lx, returning false.\n", (long)pc,
          large_int_to_string(fetchlen, as1, sizeof as1), (unsigned long)curpc);
      return false;
    } else if (large_int_compare(&cur_fetchlen, &si->iseq_fetchlen) >= 0) {
      DBG2(SRC_ISEQ_FETCH, "%lx: cur_fetchlen=%s >= si->iseq_fetchlen=%s\n",
          (long)curpc, large_int_to_string(&cur_fetchlen, as1, sizeof as1),
          large_int_to_string(&si->iseq_fetchlen, as2, sizeof as2));
      append_iseq(in, iseq, iseq_size, len, &out_fetchlen, pcs, &num_nextpcs, nextpcs,
          num_lastpcs, lastpcs, lastpc_weights, &num_function_calls, function_calls,
          si);
      //cur_fetchlen -= si->iseq_fetchlen;
      large_int_sub(&cur_fetchlen, &cur_fetchlen, &si->iseq_fetchlen);
      curpc = -1;
      if (num_nextpcs >= 1) {
        size_t min_index;
        min_index = nextpc_t::nextpc_array_min(nextpcs, num_nextpcs);
        if (min_index != -1) {
          curpc = nextpcs[min_index].get_val();  //XXX hack : really should take the min in top-sort order.
        }
        /*if (curpc != nextpcs[0]) {
          printf("curpc=%lx,nextpcs[0]=%lx\n", (long)curpc, (long)nextpcs[0]);
        }*/
        //curpc = nextpcs[0];
      } 
      if (curpc == -1) {
        int i;
        for (i = 0; i < *len; i++) {
          DBG(SRC_ISEQ_FETCH, "pcs[%d]=%lx\n", i, (long)pcs[i]);
        }
        DBG(SRC_ISEQ_FETCH, "curpc %lx, si->iseq_fetchlen = %s, num_nextpcs=%ld, "
            "setting curpc = -1\n", (long)curpc,
            large_int_to_string(&si->iseq_fetchlen, as1, sizeof as1),
            (long)num_nextpcs);
        //curpc = -1;
      }
      DBG2(SRC_ISEQ_FETCH, "num_nextpcs=%ld\n", num_nextpcs);
      DBG_EXEC2(SRC_ISEQ_FETCH,
          for (int i = 0; i < num_nextpcs; i++) {
            printf("%s() %d: nextpcs[%d] = %s\n", __func__, __LINE__, i, nextpcs[i].nextpc_to_string().c_str());
          }
      );
      if (fallthrough_pc) {
        *fallthrough_pc = si->fallthrough_pc;
      }
      if (   large_int_compare(&cur_fetchlen, &si->iseq_fetchlen) > 0
          && num_nextpcs != 1
          && !allow_shorter) {
        /*if (allow_shorter) {
          break;
        }*/
        DBG(SRC_ISEQ_FETCH, "pc %lx si_entry->pc %lx fetchlen %s, cur_fetchlen %s, si->iseq_fetchlen %s: "
            "returning false at %lx, num_nextpcs = %ld, nextpcs=%s,%s,...\n", (long)pc, (long)si_entry->pc,
            large_int_to_string(fetchlen, as1, sizeof as1),
            large_int_to_string(&cur_fetchlen, as2, sizeof as2),
            large_int_to_string(&si->iseq_fetchlen, as3, sizeof as3),
            (long)curpc, (long)num_nextpcs, nextpcs[0].nextpc_to_string().c_str(), nextpcs[1].nextpc_to_string().c_str());
        if (l_iseq) {
          //free(l_iseq);
          delete [] l_iseq;
        }
        //free(function_calls);
        delete[] function_calls;
        //free(nextpcs);
        delete[] nextpcs;
        return false;
      }
    } else {
      //long cur_num_out_edges, cur_num_function_calls;
      //long fetched_len, num_paths, orig_len;
      //src_ulong *cur_function_calls;
      //double *cur_out_edge_weights;
      //edge_t *cur_out_edges;
      //fetch_branch_t *paths;
      si_iseq_t f;

      /*cur_function_calls = (src_ulong *)malloc(in->num_si * sizeof(src_ulong));
      ASSERT(cur_function_calls);
      cur_out_edges = (edge_t *)malloc(in->num_si * sizeof(src_ulong));
      ASSERT(cur_out_edges);
      cur_out_edge_weights = (double *)malloc(in->num_si * sizeof(double));
      ASSERT(cur_out_edge_weights);
      paths = (fetch_branch_t *)malloc(in->num_si * sizeof(fetch_branch_t));
      ASSERT(paths);*/

      //orig_len = *len;
      si_iseq_init(&f);
      DBG2(SRC_ISEQ_FETCH, "calling src_iseq_fetch_len on %lx.\n", (long)curpc);
      src_iseq_fetch_len(in, peep_table, curpc, &f,/* iseq, len, pcs,
          &cur_num_out_edges, cur_out_edges, cur_out_edge_weights,
          &cur_num_function_calls,
          cur_function_calls, &fetched_len,*/ &cur_fetchlen,/* paths, &num_paths,
          fallthrough_pc,*/ first, false);
      //paths_free(paths, num_paths);
      ASSERT(f.iseq_len != 0);
      if (large_int_compare(&f.iseq_fetchlen, large_int_zero()) == 0) {
        DBG(SRC_ISEQ_FETCH, "pc %lx fetchlen %s: returning false.\n", (long)pc,
            large_int_to_string(fetchlen, as1, sizeof as1));
        si_iseq_free(&f);
        /*free(cur_function_calls);
        free(cur_out_edges);
        free(cur_out_edge_weights);
        free(paths);
        if (l_iseq) {
          free(l_iseq);
        }
        free(function_calls);
        free(nextpcs);*/
        return false;
      }
      if (large_int_compare(&f.iseq_fetchlen, &cur_fetchlen) < 0) {
        //cout << __func__ << " " << __LINE__ << ": f.iseq_fetchlen = " << large_int_to_string(&f.iseq_fetchlen, as1, sizeof as1) << endl;
        //cout << __func__ << " " << __LINE__ << ": cur_fetchlen = " << large_int_to_string(&cur_fetchlen, as1, sizeof as1) << endl;
        DBG(SRC_ISEQ_FETCH, "pc %lx fetchlen %s: returning false.\n", (long)pc,
            large_int_to_string(fetchlen, as1, sizeof as1));
        si_iseq_free(&f);
        return false;
      }
      ASSERT(large_int_compare(&f.iseq_fetchlen, &cur_fetchlen) == 0);
      large_int_add(&out_fetchlen, &out_fetchlen, &cur_fetchlen);
      append_iseq_compute_attrs(in, f.iseq_len, f.pcs, *len, &num_nextpcs,
          nextpcs, num_lastpcs, lastpcs, lastpc_weights, &num_function_calls,
          function_calls, f.num_out_edges, f.out_edges,
          f.out_edge_weights, f.num_function_calls, f.function_calls);
      //cur_fetchlen = 0;
      large_int_copy(&cur_fetchlen, large_int_zero());

      ASSERT(*len + f.iseq_len <= iseq_size);
      src_iseq_copy(&iseq[*len], f.iseq, f.iseq_len);
      memcpy(&pcs[*len], f.pcs, f.iseq_len * sizeof(src_ulong));
      *len += f.iseq_len;
      *fallthrough_pc = f.fallthrough_pc;

      si_iseq_free(&f);
      /*free(cur_function_calls);
      free(cur_out_edges);
      free(cur_out_edge_weights);
      free(paths);*/
    }
    //ASSERT(pc_dominates_pcs(in, pc, &pcs[old_len], *len - old_len));
  } while (large_int_compare(&cur_fetchlen, large_int_zero()) != 0);

  //ASSERT(pcs_have_single_entry(in, pcs, *len));

  DBG_EXEC2(SRC_ISEQ_FETCH,
      long i;
      printf("\n\n%s() %d: pc %lx fetchlen %s: returning len %ld iseq:\n%spcs:",
          __func__, __LINE__, (long)pc, large_int_to_string(fetchlen, as2, sizeof as2), *len,
          src_iseq_to_string(iseq, *len, as1, sizeof as1));
      for (i = 0; i < *len; i++) {
        printf(" %lx", (long)pcs[i]);
      }
      printf("\nnextpcs:");
      for (i = 0; i < num_nextpcs; i++) {
        printf(" %s", nextpcs[i].nextpc_to_string().c_str());
      }
      if (num_lastpcs) {
        printf("\nlastpcs:");
        for (i = 0; i < *num_lastpcs; i++) {
          printf(" %lx [%f]", (long)lastpcs[i], lastpc_weights[i]);
        }
      }
      if (num_function_calls) {
        printf("\nfunction_calls:");
        for (i = 0; i < num_function_calls; i++) {
          printf(" %s", function_calls[i].nextpc_to_string().c_str());
        }
      }
      printf("\n\n"););
  if (l_iseq) {
    //free(l_iseq);
    delete [] l_iseq;
  }
  //free(function_calls);
  delete[] function_calls;
  //free(nextpcs);
  delete[] nextpcs;
  return true;
}

static char *
pcs_to_string(src_ulong const *pcs, long n, char *buf, size_t size)
{
  char *ptr, *end;
  long i;

  ptr = buf;
  end = buf + size;

  ptr += snprintf(ptr, end - ptr, "%s", "pcs:\n");
  for (i = 0; i < n; i++) {
    ptr += snprintf(ptr, end - ptr, "%ld. %lx\n", i, (long)pcs[i]);
    ASSERT(ptr < end);
  }
  return buf;
}

/*static void
src_iseq_convert_invalid_insn_pcs_to_nextpcs(src_insn_t const *iseq, src_ulong *pcs,
    long iseq_len, long num_nextpcs)
{
  src_ulong nextpc_num;
  src_tag_t pcrel_tag;
  uint64_t pcrel_val;
  bool ret;
  long i;

  for (i = 0; i < iseq_len; i++) {
    if (pcs[i] == INSN_PC_INVALID) {
      ASSERT(src_insn_is_unconditional_branch(&iseq[i]));
      ret = src_insn_get_pcrel_value(&iseq[i], &pcrel_val, &pcrel_tag);
      ASSERT(ret);
      if (pcrel_tag == tag_var) {
        ASSERT(pcrel_val >= 0 && pcrel_val < num_nextpcs);
        pcs[i] = NEXTPC_TO_INVALID_INSN_PC(pcrel_val);
      } else {
        ASSERT(pcrel_tag == tag_const);
        pcs[i] = INSN_PC_TO_INVALID_INSN_PC(pcrel_val);
      }
    }
  }
}*/

bool
src_iseq_fetch(src_insn_t *iseq, size_t iseq_size,
    long *len, input_exec_t const *in, struct peep_table_t const *peep_table,
    src_ulong pc, long fetchlen_in, long *num_nextpcs, nextpc_t *nextpcs,
    src_ulong *insn_pcs, long *num_lastpcs, src_ulong *lastpcs, double *lastpc_weights,
    bool allow_shorter)
{
  autostop_timer func_timer(__func__);
  nextpc_t fallthrough_pc = 0;
  large_int_t fetchlen;
  long max_alloc;
  src_ulong *pcs;
  bool ret;

  if (allow_shorter) {
    large_int_copy(&fetchlen, large_int_max());
  } else {
    large_int_from_unsigned_long_long(&fetchlen, fetchlen_in);
  }

  max_alloc = (in->num_si + 100) * 4;
  pcs = NULL;
  if (!insn_pcs) {
    pcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
    ASSERT(pcs);
    insn_pcs = pcs;
  }

  ret = src_iseq_fetch_with_pcs(iseq, iseq_size, insn_pcs, len, in, peep_table, pc, &fetchlen,
      num_lastpcs, lastpcs, lastpc_weights, &fallthrough_pc, allow_shorter);
  if (!ret) {
    if (pcs) {
      free(pcs);
    }
    DBG(SRC_ISEQ_FETCH, "%lx fetchlen %s: returning false\n", (long)pc,
        large_int_to_string(&fetchlen, as1, sizeof as1));
    return false;
  }
  ASSERT(*len > 0);
  DBG2(SRC_ISEQ_FETCH, "fallthrough_pc=%s\n", fallthrough_pc.nextpc_to_string().c_str());
  if (iseq && !src_insn_is_unconditional_branch_not_fcall(&iseq[*len - 1])) {
    //src_insn_put_jump_to_pcrel(fallthrough_pc, &iseq[*len], 1);
    //insn_pcs[*len] = INSN_PC_INVALID;
    //(*len)++;

    (*len) += add_jump_to_pcrel_src_insns(fallthrough_pc, &iseq[*len], &insn_pcs[*len], iseq_size - *len);
  }
  DBG_EXEC2(SRC_ISEQ_FETCH,
    long i;
    for (i = 0; i < *len; i++) {
      printf("insn_pcs[%ld]=%lx\n", (long)i, (long)insn_pcs[i]);
    }
  );
  if (iseq) {
    DBG2(SRC_ISEQ_FETCH, "before conversion to inums:\n%s\n%s\n",
        src_iseq_to_string(iseq, *len, as1, sizeof as1),
        pcs_to_string(insn_pcs, *len, as2, sizeof as2));
    src_iseq_convert_pcs_to_inums(iseq, insn_pcs, *len, nextpcs, num_nextpcs);
    //src_iseq_convert_invalid_insn_pcs_to_nextpcs(iseq, insn_pcs, *len, *num_nextpcs);
    DBG2(SRC_ISEQ_FETCH, "after conversion to inums:\n%s\n%s\n",
        src_iseq_to_string(iseq, *len, as1, sizeof as1),
        pcs_to_string(insn_pcs, *len, as2, sizeof as2));
    CPP_DBG_EXEC2(SRC_ISEQ_FETCH,
        cout << __func__ << " " << __LINE__ << ": after conversion to inums: num_nextpcs = " << num_nextpcs << endl;
        for (int i = 0; i < *num_nextpcs; i++) {
          cout << "nextpcs[" << i << "] = " << nextpcs[i].nextpc_to_string() << endl;
        }
    );
  }
  DBG_EXEC2(SRC_ISEQ_FETCH,
    if (nextpcs) {
      long i;
      for (i = 0; i < *num_nextpcs; i++) {
        printf("nextpcs[%ld]=%s\n", (long)i, nextpcs[i].nextpc_to_string().c_str());
      }
    }
  );
  if (pcs) {
    free(pcs);
  }
  DBG(SRC_ISEQ_FETCH, "%lx fetchlen %s: returning true\n", (long)pc,
      large_int_to_string(&fetchlen, as1, sizeof as1));
  return true;
}

void
src_iseq_compute_touch_var(regset_t *touch, regset_t *use, memset_t *memuse,
    memset_t *memdef,
    src_insn_t const *src_iseq, long src_iseq_len)
{
  autostop_timer func_timer(__func__);
  src_insn_t src_iseq_const[src_iseq_len];
  regmap_t regmap;
  bool ret;
  long i;

#if ARCH_SRC == ARCH_PPC
  src_iseq_assign_vregs_to_cregs(src_iseq_const, src_iseq, src_iseq_len, &regmap,
      false);
#else
  src_iseq_copy(src_iseq_const, src_iseq, src_iseq_len);
#endif

  regset_clear(touch);
  if (use) {
    regset_clear(use);
  }
  if (memuse) {
    memset_clear(memuse);
  }
  if (memdef) {
    memset_clear(memdef);
  }
  //autostop_timer func2_timer(string(__func__) + ".2");
  for (i = src_iseq_len - 1; i >= 0; i--) {
    regset_t iuse, idef;
    memset_t imemuse, imemdef;
    //autostop_timer func3_timer(string(__func__) + ".3");
    src_insn_get_usedef(&src_iseq_const[i], &iuse, &idef, &imemuse, &imemdef);
    DBG2(PEEP_TAB, "instruction#%ld: %s\nuse=%s, def=%s\n", i,
        src_insn_to_string(&src_iseq[i], as3, sizeof as3),
        regset_to_string(&iuse, as1, sizeof as1),
        regset_to_string(&idef, as2, sizeof as2));
    //autostop_timer func4_timer(string(__func__) + ".4");
    regset_union(touch, &iuse);
    regset_union(touch, &idef);
    if (use) {
      regset_diff(use, &idef);
      regset_union(use, &iuse);
    }
    //autostop_timer func5_timer(string(__func__) + ".5");
    if (memuse) {
      memset_union(memuse, &imemuse);
    }
    if (memdef) {
      memset_union(memdef, &imemdef);
    }
  }
#if ARCH_SRC == ARCH_PPC
  regset_inv_rename(touch, touch, &regmap);
  if (use) {
    regset_inv_rename(use, use, &regmap);
  }
  if (memuse) {
    memset_inv_rename(memuse, memuse, &regmap);
  }
  if (memdef) {
    memset_inv_rename(memdef, memdef, &regmap);
  }
#endif
  DBG2(PEEP_TAB, "touch=%s\n", regset_to_string(touch, as1, sizeof as1));
}

void
src_iseq_check_sanity(src_insn_t const *src_iseq, long src_iseq_len,
    regset_t const *touch, regset_t const *tmap_regs,
    regset_t const *live_regs, long num_live_outs,
    long linenum)
{
  regset_t live_touch;
  long i;

  regset_copy(&live_touch, regset_empty());
  for (i = 0; i < num_live_outs; i++) {
    if (!regset_contains(touch, &live_regs[i])) {
      err("live_regs[%ld] specifies a register not used by instruction sequence\n"
          "touch = %s\nlive_regs[%ld] = %s\n", i,
          regset_to_string(touch, as1,sizeof as1), i,
          regset_to_string(&live_regs[i], as2,sizeof as2));
      err("src_assembly:\n%s\n",
          src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
      ASSERT (false);
    }
    regset_union(&live_touch, &live_regs[i]);
  }

  if (!regset_contains(tmap_regs, &live_touch)) {
    err("Transmap under-specified. at linenum %ld.\n"
        "touch=%s\ntmap_regs=%s\n", linenum,
        regset_to_string(&live_touch, as1, sizeof as1),
        regset_to_string(tmap_regs, as2, sizeof as2));
    ASSERT(false);
  }
  if (!regset_coarse_contains(touch, tmap_regs)) {
    err ("Transmap over-specified. at linenum %ld.\n"
        "touch=%s\ntmap_regs=%s\n", linenum,
        regset_to_string(touch, as1, sizeof as1),
        regset_to_string(tmap_regs, as2, sizeof as2));
    ASSERT(false);
    //goto done_with_entry;
  }
}

long
si_get_num_preds(si_entry_t const *si)
{
  if (si->bblindex > 0) {
    return 1;
  }
  return si->bbl->num_preds;
}

long
si_get_preds(input_exec_t const *in, si_entry_t const *si, src_ulong *preds,
  double *weights)
{
  bbl_t const *bbl;

  bbl = si->bbl;
  if (si->bblindex > 0) {
    preds[0] = (si - 1)->pc;
    if (weights) {
      weights[0] = 1;
    }
    return 1;
  }
  memcpy(preds, bbl->preds, bbl->num_preds * sizeof(src_ulong));
  if (weights) {
    memcpy(weights, bbl->pred_weights, bbl->num_preds * sizeof(double));
  }
  return bbl->num_preds;
}

void
src_iseq_copy(struct src_insn_t *dst, struct src_insn_t const *src, long n)
{
  autostop_timer func_timer(__func__);
  long i;
  for (i = 0; i < n; i++) {
    src_insn_copy(&dst[i], &src[i]);
  }
}

static char *
sym_type_to_string(sym_type_t type, char *buf, int size)
{
  switch(type) {
    case SYMBOL_NONE:
      snprintf (buf, size, "SYM_NONE");
      break;
    case SYMBOL_PCREL:
      snprintf (buf, size, "SYM_PCREL");
      break;
    case SYMBOL_ABS:
      snprintf (buf, size, "SYM_ABS");
      break;
    default:
      ASSERT(0);
      break;
  }
  return buf;
}
  
static void
put_symbol(sym_t *symbols, long *num_symbols, char const *name, unsigned long val,
    sym_type_t type)
{
  sym_t *sym = &symbols[*num_symbols];
  snprintf(sym->name, sizeof sym->name, "%s", name);
  sym->val = val;
  sym->type = type;
  (*num_symbols)++;
}

static unsigned long
get_symbol(translation_instance_t *ti, sym_t const *symbols, long num_symbols,
    char const *name, sym_type_t *type)
{
  char const *reloc_offset_ptr, *target;
  int i, ival;
  *type = SYMBOL_NONE;
  //printf("name='%s', ti=%p\n", name, ti);
  for (i = 0; i < num_symbols; i++) {
    if (!strncmp (symbols[i].name, name, sizeof symbols[i].name)) {
      *type = symbols[i].type;
      return symbols[i].val;
    }
  }
  DBG2(MD_ASSEMBLE, "name = '%s', ti=%p\n", name, ti);
  if (strstart(name, ".NEXTPC", &target)) {
    *type = SYMBOL_ABS;
    return strtoul(target, NULL, 0);
  }

  if (ti && strstart(name + 1, "__reloc_", &target)) {
    *type = SYMBOL_ABS;
    return add_or_find_reloc_symbol_name(&ti->reloc_strings,
        &ti->reloc_strings_size, target);
  }
  if (strstart(name, ".RELOC_STR", &reloc_offset_ptr)) {
    *type = SYMBOL_ABS;
    long val = atol(reloc_offset_ptr);
    DBG(MD_ASSEMBLE, "reloc_offset_ptr='%s', val=%ld.\n", reloc_offset_ptr, val);
    return val;
  }
  /* the following are not really needed, right? */
  if (   sscanf(name, ".i0x%x", &ival) == 1
      || sscanf(name, ".i%u", &ival) == 1
      || sscanf(name, ".sboxed_abs_inum%u", &ival) == 1
      || sscanf(name, ".binpcrel_inum%u", &ival) == 1
      || sscanf(name, ".src_insn_pc%u", &ival) == 1
     ) {
    *type = SYMBOL_ABS;
    return ival;
  }
  if (sscanf(name, ".pcrel0x%x", &ival) == 1 || sscanf(name, ".pcrel%u", &ival) == 1) {
    *type = SYMBOL_PCREL;
    return ival;
  }
  return 0;
}

ssize_t
insn_assemble(translation_instance_t *ti, uint8_t *bincode, char const *assembly,
    int (*insn_md_assemble)(char *, uint8_t *),
    ssize_t (*patch_jump_offset)(uint8_t *, ssize_t, sym_type_t, unsigned long,
        char const *, bool),
    sym_t *symbols, long *num_symbols)
{
  char const *ptr = assembly;
  uint8_t *binptr = bincode;
  uint8_t tmpbuf[512]; /* i386_md_assemble seems to be writing one extra byte sometimes.
                      so we call it using a separate buffer from bincode. */
  bool first_pass;
  int inum = 0;

  if (*num_symbols == 0) {
    first_pass = true;
  } else {
    first_pass = false;
  }

  if (!strcmp(assembly, "")) {
    DBG(I386_READ, "returning %d\n", 0);
    return 0;
  }
  DBG(MD_ASSEMBLE, "ti=%p\n", ti);
  do {
    char insn[2048], *insn_ptr=insn;
    char const *end;
    char *label_def, *space, *label_use;
    unsigned long symval = 0;
    sym_type_t symtype = SYMBOL_NONE;

    /* if (first_pass) {
      insn_binptr[inum] = binptr;
    } */
    inum++;
    end = strchr(ptr, '\n');
    ASSERT(end);
    ASSERT((end - ptr) < (int)(sizeof insn));
    strncpy(insn, ptr, end - ptr);
    insn[end - ptr] = '\0';

    DBG(MD_ASSEMBLE, "insn : %s.\n", insn);
    label_def = strchr(insn_ptr, ':');
    space = strchr(insn_ptr, ' ');
    if (space && space < label_def) {
      label_def = NULL;
    }
    if (label_def) {
      while (*insn_ptr++== ' ');
      insn_ptr--;
      char sym[64];
      ASSERT(label_def - insn_ptr < (int)(sizeof sym));
      strncpy(sym, insn_ptr, label_def - insn_ptr);
      sym[label_def - insn_ptr] = '\0';
      DBG(MD_ASSEMBLE, "Found symbol def: %s-->%zd\n", sym, binptr - bincode);
      put_symbol(symbols, num_symbols, sym, (unsigned long)binptr, SYMBOL_PCREL);
      insn_ptr = label_def;
      while(*++insn_ptr== ' ');
    }

    label_use = strchr(insn_ptr, '.');
    if (   label_use
        && (label_use == insn_ptr || label_use[-1] == ' ')
        && label_use[1] != ' ') {
      char sym[64];
      sscanf (label_use, "%s", sym);
      ASSERT(strlen(sym) < (int)(sizeof sym));
      symval = get_symbol(ti, symbols, *num_symbols, sym, &symtype);
      if (!first_pass && !symval && symtype == SYMBOL_NONE) {
        DBG(MD_ASSEMBLE, "Unresolved symbol: %s\n", sym);
        return 0;
        ASSERT (0);
      }
      if (symval) {
        DBG(MD_ASSEMBLE, "Found symbol use: %s-->%lx(%s)\n", sym,
            symval, sym_type_to_string(symtype, as1, sizeof as1));
      }
    }

    ssize_t as;
    DBG(MD_ASSEMBLE, "calling i386_md_assemble: %s\n", insn_ptr);
    stopwatch_run(&insn_md_assemble_timer);
    as = (*insn_md_assemble)(insn_ptr, tmpbuf);
    stopwatch_stop(&insn_md_assemble_timer);
    DBG(MD_ASSEMBLE, "i386_md_assemble returned %zx\n", as);
    /* DBG_EXEC(MD_ASSEMBLE,
        int i;
        printf("tmpbuf: ");
        for (i = 0; i < as; i++) {
          printf(" %hhx", tmpbuf[i]);
        }
        printf("\n");
    ); */

    if (as == -1) {
      if (strcmp(insn_ptr, "")) {
        /*DBG(I386_READ, "i386_md_assemble('%s') returned -1\n", insn_ptr);
        DBG_SET(TCDBG_I386, 2);
        i386_md_assemble(insn_ptr, (char *)binptr);
        DBG_SET(TCDBG_I386, 0);*/
        return 0;
      }
    } else {
      as = (*patch_jump_offset)(tmpbuf, as, symtype, symval, (char *)binptr,
          first_pass);
      DBG_EXEC(MD_ASSEMBLE,
          int i;
          printf("after patch_jump_offset, tmpbuf: ");
          for (i = 0; i < as; i++) {
            printf(" %hhx", tmpbuf[i]);
          }
          printf("\n");
      );

      memcpy(binptr, tmpbuf, as);
      binptr += as;
    }
    ptr = end + 1;
  } while (*ptr);

  DBG(MD_ASSEMBLE, "returning %zd for '%s': bincode(%p) = %s\n",
      binptr - bincode, assembly, bincode,
      hex_string(bincode, binptr - bincode, as1, sizeof as1));
  return binptr - bincode;
}


bool
superoptimizer_supports_src(src_insn_t const *iseq, long len)
{
  long i;
  for (i = 0; i < len; i++) {
    if (!src_insn_supports_boolean_test(&iseq[i])) {
      return false;
    }
    if (!src_insn_supports_execution_test(&iseq[i])) {
      return false;
    }
  }
  //printf("returning true for:\n%s\n", src_iseq_to_string(iseq, len, as1, sizeof as1));
  return true;
}

/*
bool
superoptimizer_supports_i386(i386_insn_t const *iseq, long len)
{
  long i;
  for (i = 0; i < len; i++) {
    if (!i386_insn_supports_boolean_test(&iseq[i])) {
      return false;
    }
    if (!i386_insn_supports_execution_test(&iseq[i])) {
      return false;
    }

  }
  return true;
}
*/

bool
src_iseq_merits_optimization(src_insn_t const *iseq, long iseq_len)
{
  long i;

  for (i = 0; i < iseq_len; i++) {
    if (!src_insn_merits_optimization(&iseq[i])) {
      return false;
    }
  }
  return true;
}

/*void
do_canonicalize_imms_using_syms(valtag_t *vt, input_exec_t *in)
{
  long i;

  ASSERT(vt->tag == tag_const);
  //DBG(TMP, "canonicalizing val %lx\n", (long)vt->val);
  for (i = 0; i < in->num_symbols; i++) {
    if (   vt->val >= in->symbols[i].value
        && vt->val < in->symbols[i].value + in->symbols[i].size
        && input_exec_addr_belongs_to_input_section(in, in->symbols[i].value)) {
      if (vt->val == in->symbols[i].value) {
        vt->mod.imm.modifier = IMM_UNMODIFIED;
      } else {
        vt->mod.imm.modifier = IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0;
        vt->mod.imm.sconstants[2] = 1;
        vt->mod.imm.sconstants[1] = vt->val - in->symbols[i].value;
        vt->mod.imm.sconstants[0] = sizeof(src_ulong) * BYTE_LEN;
      }
      vt->val = i;
      vt->tag = tag_imm_symbol;
      return;
    }
  }

  if (input_exec_addr_belongs_to_rodata_section(in, vt->val)) {
    input_exec_add_rodata_ptr(in, vt->val);
  }
}
*/

/*static void
src_insn_add_function_call_arguments(src_insn_t *insn, input_exec_t const *in,
    src_ulong pc)
{
  int i, num_arg_words;
  src_ulong target;

  if (!src_insn_is_function_call(insn)) {
    return;
  }
  ASSERT(insn->memtouch.n_elem == 0);
  target = src_insn_branch_target(insn, pc);
  //DBG(TMP, "pc %lx, target %lx\n", (long)pc, (long)target);
  if (target == -1) {
    return;
  }
  num_arg_words = input_exec_function_get_num_arg_words(in, target);
  for (i = 0; i < num_arg_words; i++) {
    src_insn_add_memtouch_elem(insn, R_SS, R_ESP, -1, i * (DWORD_LEN/BYTE_LEN),
        DWORD_LEN/BYTE_LEN, false);
    DBG(ANNOTATE_LOCALS, "after adding memtouch elem: %s\n", src_insn_to_string(insn, as1, sizeof as1));
  }
}*/

bool
src_insn_fetch(struct src_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long *size)
{
  unsigned long lsize;
  bool fetch;

  DBG_EXEC(ASSERTCHECKS, ASSERT(!assertcheck_si_init_done););

  if (!size) {
    size = &lsize;
  }
  src_ulong orig_pc = pc_get_orig_pc(pc);
  //printf("%s() %d: pc = %lx, orig_pc = %lx\n", __func__, __LINE__, (long)pc, (long)orig_pc);
  fetch = src_insn_fetch_raw(insn, in, orig_pc, size);
  //cout << __func__ << " " << __LINE__ << ": fetched instruction at pc " << hex << pc << dec << ": insn = " << src_insn_to_string(insn, as1, sizeof as1) << endl;

  if (fetch) {
    //src_insn_rename_addresses_to_symbols(insn, in);
    //src_insn_add_function_call_arguments(insn, in, pc + *size);
    src_insn_fetch_preprocess(insn, in, orig_pc, *size);
  }
  return fetch;
}

unsigned long
src_iseq_hash_func(src_insn_t const *insns, int n_insns)
{
  unsigned long ret;
  int i;

  ret = 104729 * n_insns;
  for (i = 0; i < n_insns; i++) {
    ret += (i + 1) * 3989 * src_insn_hash_func(&insns[i]);
  }
  return ret;
}

string
src_tag_to_string(src_tag_t t)
{
  switch (t) {
    case tag_invalid: return "tag_invalid";
    case tag_const: return "tag_const";
    case tag_var: return "tag_var";
    case tag_abs_inum: return "tag_abs_inum";
    case tag_sboxed_abs_inum: return "tag_sboxed_abs_inum";
    case tag_binpcrel_inum: return "tag_binpcrel_inum";
    case tag_src_insn_pc: return "tag_src_insn_pc";
    case tag_input_exec_reloc_index: return "tag_input_exec_reloc_index";
    default: cout << __func__ << " " << __LINE__ << ": t = " << t << endl; NOT_REACHED();
  }
}

src_tag_t
src_tag_from_string(string const &s)
{
  if (s == "tag_invalid") {
    return tag_invalid;
  } else if (s == "tag_const") {
    return tag_const;
  } else if (s == "tag_var") {
    return tag_var;
  } else if (s == "tag_abs_inum") {
    return tag_abs_inum;
  } else if (s == "tag_sboxed_abs_inum") {
    return tag_sboxed_abs_inum;
  } else if (s == "tag_binpcrel_inum") {
    return tag_binpcrel_inum;
  } else if (s == "tag_src_insn_pc") {
    return tag_src_insn_pc;
  } else if (s == "tag_input_exec_reloc_index") {
    return tag_input_exec_reloc_index;
  } else {
    cout << __func__ << " " << __LINE__ << ": s = " << s << endl;
    NOT_IMPLEMENTED();
  }
}

long
src_iseq_window_convert_pcrels_to_nextpcs(src_insn_t *src_iseq,
    long src_iseq_len, long start, long *nextpc2src_pcrel_vals,
    src_tag_t *nextpc2src_pcrel_tags)
{
  bool nextpc_used[2];
  //valtag_t *pcrel;
  long i, j, cur;
  bool done;

  cur = 0;
  for (i = 0; i < src_iseq_len; i++) {
    vector<valtag_t> pcrels;
    pcrels = src_insn_get_pcrel_values(&src_iseq[i]);
    //if (pcrel = src_insn_get_pcrel_operand(&i386_iseq[i]))
    for (size_t p = 0; p < pcrels.size(); p++) {
      valtag_t &pcrel = pcrels.at(p);
      done = false;
      for (j = 0; j < cur; j++) {
        if (   nextpc2src_pcrel_vals[j] == start + 1 + pcrel.val
            && nextpc2src_pcrel_tags[j] == pcrel.tag) {
          pcrel.tag = tag_var;
          pcrel.val = j;
          done = true;
          break;
        }
      }
      if (done) {
        continue;
      }
      if (pcrel.tag == tag_const) {
        if (pcrel.val != -1) { //not a self loop
          nextpc2src_pcrel_vals[cur] = start /*+ i */+ 1 + pcrel.val;
          nextpc2src_pcrel_tags[cur] = pcrel.tag;
          pcrel.tag = tag_var;
          pcrel.val = cur;
          cur++;
        }
      } else {
        ASSERT(pcrel.tag == tag_var);
        nextpc2src_pcrel_vals[cur] = pcrel.val;
        nextpc2src_pcrel_tags[cur] = pcrel.tag;
        pcrel.tag = tag_var;
        pcrel.val = cur;
        cur++;
      }
    }
    if (src_insn_is_indirect_branch(&src_iseq[i])) {
      nextpc2src_pcrel_vals[cur] = PC_JUMPTABLE;
      nextpc2src_pcrel_tags[cur] = tag_const;
      cur++;
    }
    src_insn_set_pcrel_values(&src_iseq[i], pcrels);
  }
  return cur;
}

void
src_iseq_rename_constants(src_insn_t *iseq, long iseq_len, imm_vt_map_t const &imm_vt_map)
{
  for (long i = 0; i < iseq_len; i++) {
    src_insn_rename_constants(&iseq[i], &imm_vt_map);
  }
}

static bool
transmap_assignment_agrees_syntactically_with_src_iseq(exreg_group_id_t g, reg_identifier_t const &r, transmap_loc_id_t loc, src_insn_t const *src_iseq, long src_iseq_len)
{
#if ARCH_SRC == ARCH_ETFG && ARCH_DST == ARCH_I386
  exreg_group_id_t lg = loc - TMAP_LOC_EXREG(0);
  if (lg == I386_EXREG_GROUP_EFLAGS_EQ || lg == I386_EXREG_GROUP_EFLAGS_NE) {
    return src_iseq_stores_eq_comparison_in_reg(src_iseq, src_iseq_len, G_SRC_KEYWORD, g, r);
  } else if (lg == I386_EXREG_GROUP_EFLAGS_ULT || lg == I386_EXREG_GROUP_EFLAGS_UGE) {
    return src_iseq_stores_ult_comparison_in_reg(src_iseq, src_iseq_len, G_SRC_KEYWORD, g, r);
  } else if (lg == I386_EXREG_GROUP_EFLAGS_SLT || lg == I386_EXREG_GROUP_EFLAGS_SGE) {
    return src_iseq_stores_slt_comparison_in_reg(src_iseq, src_iseq_len, G_SRC_KEYWORD, g, r);
  } else if (lg == I386_EXREG_GROUP_EFLAGS_ULE || lg == I386_EXREG_GROUP_EFLAGS_UGT) {
    return src_iseq_stores_ugt_comparison_in_reg(src_iseq, src_iseq_len, G_SRC_KEYWORD, g, r);
  } else if (lg == I386_EXREG_GROUP_EFLAGS_SLE || lg == I386_EXREG_GROUP_EFLAGS_SGT) {
    return src_iseq_stores_sgt_comparison_in_reg(src_iseq, src_iseq_len, G_SRC_KEYWORD, g, r);
  } else {
    return true;
  }
#else
  NOT_IMPLEMENTED();
#endif
}

static bool
in_out_tmap_agrees_syntactically_with_src_iseq(in_out_tmaps_t const &in_out_tmap, src_insn_t const *src_iseq, long src_iseq_len)
{
  regset_t use, def;
  src_iseq_get_usedef_regs(src_iseq, src_iseq_len, &use, &def);
  transmap_t const &in_tmap = in_out_tmap.first;
  vector<transmap_t> const &out_tmaps = in_out_tmap.second;
  if (in_tmap.transmap_maps_multiple_regs_to_flag_loc()) {
    return false;
  }
  for (const auto &out_tmap : out_tmaps) {
    if (out_tmap.transmap_maps_multiple_regs_to_flag_loc()) {
      return false;
    }
    transmap_loc_id_t flag_loc;
    for (const auto &g : def.excregs) {
      for (const auto &r : g.second) {
        if (   out_tmap.transmap_maps_reg_to_flag_loc(g.first, r.first, flag_loc)
            && !transmap_assignment_agrees_syntactically_with_src_iseq(g.first, r.first, flag_loc, src_iseq, src_iseq_len)) {
          return false;
        }
      }
    }
  }
  return true;
}

static void
prune_in_out_tmaps_syntactically(list<in_out_tmaps_t> &in_out_tmaps, src_insn_t const *src_iseq, long src_iseq_len)
{
  list<in_out_tmaps_t> prev_in_out_tmaps = in_out_tmaps;
  in_out_tmaps.clear();
  for (const auto &in_out_tmap: prev_in_out_tmaps) {
    if (in_out_tmap_agrees_syntactically_with_src_iseq(in_out_tmap, src_iseq, src_iseq_len)) {
      in_out_tmaps.push_back(in_out_tmap);
    }
  }
}

static bool
in_out_tmap_maps_used_reg_to_flag(in_out_tmaps_t const &in_out_tmap, regset_t const &use)
{
  transmap_t const &in_tmap = in_out_tmap.first;
  vector<transmap_t> const &out_tmaps = in_out_tmap.second;
  if (in_tmap.transmap_maps_used_reg_to_flag_loc(use)) {
    return true;
  }
  for (const auto &out_tmap : out_tmaps) {
    if (out_tmap.transmap_maps_used_reg_to_flag_loc(use)) {
      return true;
    }
  }
  return false;
}

static void
prune_in_out_tmaps_read_flags_except_for_cbranch(list<in_out_tmaps_t> &in_out_tmaps, src_insn_t const *src_iseq, long src_iseq_len)
{
#if ARCH_SRC == ARCH_ETFG
  list<in_out_tmaps_t> prev_in_out_tmaps = in_out_tmaps;
  in_out_tmaps.clear();
  regset_t use, def;
  src_iseq_get_usedef_regs(src_iseq, src_iseq_len, &use, &def);
  for (const auto &in_out_tmap: prev_in_out_tmaps) {
    if (   !src_iseq_involves_cbranch(src_iseq, src_iseq_len)
        && in_out_tmap_maps_used_reg_to_flag(in_out_tmap, use)) {
      continue;
    }
    in_out_tmaps.push_back(in_out_tmap);
  }
#endif
}

list<in_out_tmaps_t>
src_iseq_enumerate_in_out_transmaps(src_insn_t const *src_iseq, long src_iseq_len, regset_t const *live_out, long num_live_out, bool mem_live_out, int prioritize)
{
  regset_t use, def, touch;
  src_iseq_get_usedef_regs(src_iseq, src_iseq_len, &use, &def);
  touch = use;
  regset_union(&touch, &def);

  if (prioritize >= PRIORITIZE_ENUM_CANDIDATES_LIVE_EQ_TOUCH) {
    for (long i = 0; i < num_live_out; i++) {
      if (!regset_equal(&touch, &live_out[i])) {
        return list<pair<transmap_t, vector<transmap_t>>>();
      }
    }
  }

  list<in_out_tmaps_t> in_out_tmaps = dst_enumerate_in_out_transmaps_for_usedef(use, def, live_out, num_live_out);

  if (prioritize >= PRIORITIZE_ENUM_CANDIDATES_PRUNE_TMAP_FLAG_MAPPINGS_SYNTACTICALLY) {
    prune_in_out_tmaps_syntactically(in_out_tmaps, src_iseq, src_iseq_len);
  }
  if (prioritize >= PRIORITIZE_ENUM_CANDIDATES_PRUNE_TMAP_FLAG_MAPPINGS_READ_EXCEPT_CBRANCH) {
    prune_in_out_tmaps_read_flags_except_for_cbranch(in_out_tmaps, src_iseq, src_iseq_len);
  }
  return in_out_tmaps;
}

static bool
src_iseq_symbols_are_contained_in_map(src_insn_t const *iseq, long iseq_len,
    imm_vt_map_t const *imm_vt_map)
{
  long i;
  for (i = 0; i < iseq_len; i++) {
    if (!src_insn_symbols_are_contained_in_map(&iseq[i], imm_vt_map)) {
      printf("%s() %d: src_insn_symbols_are_contained_in_map() returned false at i=%ld\n", __func__, __LINE__, i);
      return false;
    }
  }
  return true;
}

static bool
symbol_is_contained_in_src_iseq(src_insn_t const *iseq, long iseq_len, long symbol_num,
    src_tag_t symbol_tag)
{
  long i;

  for (i = 0; i < iseq_len; i++) {
    if (symbol_is_contained_in_src_insn(&iseq[i], symbol_num, symbol_tag)) {
      return true;
    }
  }
  return false;
}

static bool
map_symbols_are_contained_in_src_iseq(src_insn_t const *iseq, long iseq_len,
    imm_vt_map_t const *imm_vt_map)
{
  long i;
  for (i = 0; i < imm_vt_map->num_imms_used; i++) {
    if (imm_vt_map->table[i].tag == tag_invalid) {
      continue;
    }
    ASSERT(   imm_vt_map->table[i].tag == tag_imm_symbol
           || imm_vt_map->table[i].tag == tag_imm_dynsym);
    if (!symbol_is_contained_in_src_iseq(iseq, iseq_len, imm_vt_map->table[i].val,
            imm_vt_map->table[i].tag)) {
      return false;
    }
  }
  return true;
}

bool
src_iseq_matches_with_symbols(src_insn_t const *iseq, long iseq_len,
    imm_vt_map_t const *imm_vt_map)
{
  bool ret1 = src_iseq_symbols_are_contained_in_map(iseq, iseq_len, imm_vt_map);
  if (!ret1) {
    printf("%s() %d: src_iseq_symbols_are_contained_in_map() returned false\n", __func__, __LINE__);
    return false;
  }
  bool ret2 = map_symbols_are_contained_in_src_iseq(iseq, iseq_len, imm_vt_map);
  if (!ret2) {
    printf("%s() %d: map_symbols_are_contained_in_src_iseq() returned false\n", __func__, __LINE__);
    return false;
  }
  return true;
}


