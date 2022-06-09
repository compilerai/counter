#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <algorithm>
#include <limits.h>
//#include "support/c_utils.h"
#include "support/src-defs.h"
#include "support/debug.h"
//#include "rewrite/harvest.h"
#include "etfg/etfg_insn.h"
#include "i386/insntypes.h"
#include "insn/dst-insn.h"
#include "insn/insn_list.h"
//#include "config.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "insn/jumptable.h"
//#include "strtab.h"
//#include "disas.h"
#include "expr/z3_solver.h"
#include "valtag/elf/elf.h"

#include "rewrite/static_translate.h"
#include "superopt/typestate.h"
//#include "dbg_functions.h"

#include "i386/opctable.h"
#include "valtag/regset.h"
//#include "temporaries.h"
#include "support/utils.h"
#include "valtag/transmap.h"
#include "rewrite/peephole.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "ppc/insn.h"
#include "insn/src-insn.h"

//#include "live_ranges.h"
#include "ppc/regs.h"
#include "insn/rdefs.h"
#include "valtag/memset.h"
#include "ppc/insn.h"

#include "insn/edge_table.h"
#include "rewrite/peep_entry_list.h"

#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"

#include "equiv/equiv.h"
#include "superopt/superopt.h"
#include "support/timers.h"
#include "ppc/execute.h"
#include "i386/execute.h"
#include "superopt/superoptdb.h"
#include "equiv/bequiv.h"
#include "rewrite/function_pointers.h"
#include "support/globals.h"
#include "equiv/jtable.h"
#include "support/cmd_helper.h"

//#include <caml/mlvalues.h>
//#include <caml/alloc.h>
//#include <caml/memory.h>
//#include <caml/fail.h>
//#include <caml/callback.h>
//#include <caml/custom.h>
//#include <caml/intext.h>

static char as1[40960000], as2[40960], as3[40960];

#define SRC_ISEQ_MAX_ALLOC 2048
#define ITABLES_MAX_ALLOC (4096 * 1024)
#define LIVE_OUT_MAX_ALLOC SRC_ISEQ_MAX_ALLOC
#define MAX_NUM_WINDFALLS 65536
#define TAGLINE_MAX_ALLOC 409600
#define MAX_NUM_ITABLES 32

#if 0
void test_small(void) {
  //mr
  //uint8_t ppc_code[4] = {0x7c, 0x22, 0x0b, 0x78};
  //uint8_t x86_code[2] = {0x89, 0xc1};
  //bne
  //uint8_t ppc_code[4] = {0x40, 0x82, 0xff, 0xec};
  //uint8_t x86_code[6] = {0xf, 0x85, 0x0, 0x0, 0x0, 0x0};
  //b
  uint8_t ppc_code[4] = {0x4b, 0xff, 0xff, 0xec};
  uint8_t i386_code[5] = {0xe9, 0x0, 0x0, 0x0, 0x0};
  ppc_insn_t ppc_iseq[MAX_PPC_ILEN];
  i386_insn_t i386_iseq[MAX_I386_ILEN];
  size_t ppc_iseq_len, i386_iseq_len;
  transmap_t *transmap = NULL;


  ptr = ppc_code;
  end = ptr + sizeof ppc_code;
  ppc_iseq_len = 0;
  while (ptr < end) {
    disas = ppc_insn_disassemble(&ppc_iseq[ppc_iseq_len++], bincode_be);
    ASSERT(disas);
    ptr += 4;
  }

  i386_iseq_len = i386_iseq_disassemble(i386_iseq, i386_code, sizeof i386_code);
  transmap_init(&in_tmap);
  transmap_clear(in_tmap);
  in_tmap->table[1].loc = REG;
  in_tmap->table[1].reg.rf.regnum = 0;
  in_tmap->table[1].reg.dirty = -1;

  in_tmap->table[2].loc = REG;
  in_tmap->table[2].reg.rf.regnum = 1;
  in_tmap->table[2].reg.dirty = -1;

  ppc_execute(env, &rand_state[0], ppc_code, ppc_codesize, &ppc_outstate[0]);
  x86_execute(env, &rand_state[0], x86_code,  x86_codesize, 1, -1, in_tmap,
      NULL, &x86_outstate[0]);

  if (ppc_states_equivalent(&ppc_outstate[0], &x86_outstate[0],
	                    ppc_regset_all(), ppc_regset_all())) {
    printf("test passed.\n");
  } else {
    printf("test failed.\n");
  }
}
#endif

static peep_entry_t const *
peep_entry_first(peep_table_t const *tab)
{
  //peep_entry_t const *e;
  long i;

  for (i = 0; i < tab->hash_size; i++) {
    if (tab->hash[i].size()) {
      return *tab->hash[i].begin();
    }
  }
  return NULL;
}

static void
usage(void)
{
  printf("peepgen:\n"
     "usage: peepgen [-v] [-pv] [-b] [-e] [-f] [-h <dst-harvest_filename>] "
     "[-p <peeptab_filename>]"
     "[-noterm] [-superoptdb <server-name>]\n"
     "\n"
     "<no-arg>    get work from superoptdb and do it\n"
     "-h <fn>     get work from superoptdb and enumerate using harvest_filename\n"
     "-p <fn>     get work from superoptdb and enumerate using peeptab_filename\n"
     "-v          verify current peephole table\n"
     "-b          boolean test only\n"
     "-e          execution test only\n"
     "-f          run forever\n"
     "-pv <fn>    specify the peephole table filename to verify\n"
     "-noterm     do not terminate on equiv failure\n"
     "-dump <fn>  get all superoptdb peeptab entries (including empty ones)\n"
     "-superoptdb specify superoptdb server (default " SUPEROPTDB_SERVER_DEFAULT ")\n"
     "\n"
     );
}


static src_fingerprintdb_elem_t **
src_fingerprintdb_from_peep_entry(peep_entry_t const *e, use_types_t use_types,
    typestate_t *typestate_initial, typestate_t *typestate_final)
{
  src_fingerprintdb_elem_t **fdb;
  char linestr[64];

  fdb = src_fingerprintdb_init();
  ASSERT(fdb);

  snprintf(linestr, sizeof linestr, "line%ld", e->peep_entry_get_line_number());
  src_fingerprintdb_add_variants(fdb, e->src_iseq, e->src_iseq_len, e->live_out,
      e->in_tmap, e->out_tmap, e->num_live_outs/*, e->src_locals_info,
      &e->src_iline_map*/,
      use_types, typestate_initial,
      typestate_final, e->mem_live_out, NULL, linestr);
  return fdb;
}

static src_fingerprintdb_elem_t **
src_fingerprintdb_from_peeptab(peep_table_t const *tab, use_types_t use_types,
    regmap_t const *regmap)
{
  typestate_t *typestate_initial, *typestate_final;
  src_fingerprintdb_elem_t **fdb;
  peep_entry_t const *e;
  long i;

  fdb = src_fingerprintdb_init();
  ASSERT(fdb);

  //typestate_initial = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
  typestate_initial = new typestate_t[NUM_FP_STATES];
  //typestate_final = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
  typestate_final = new typestate_t[NUM_FP_STATES];
  ASSERT(typestate_initial);
  ASSERT(typestate_final);

  typestate_init_to_top(typestate_initial);
  typestate_init_to_top(typestate_final);

  for (i = 0; i < tab->hash_size; i++) {
    for (auto iter = tab->hash[i].begin(); iter != tab->hash[i].end(); iter++) {
      e = *iter;
      char linestr[64];

      snprintf(linestr, sizeof linestr, "line%ld", e->peep_entry_get_line_number());
      CPP_DBG_EXEC(TMP, cout << __func__ << " " << __LINE__ << ": e->in_tmap =\n" << transmap_to_string(e->in_tmap, as1, sizeof as1) << endl);
      src_fingerprintdb_add_variants(fdb, e->src_iseq, e->src_iseq_len, e->live_out,
          e->in_tmap, e->out_tmap, e->num_live_outs/*, e->src_locals_info,
          &e->src_iline_map*/, use_types,
          typestate_initial, typestate_final, //the typestates are not used (dummy)
          e->mem_live_out, regmap, linestr);
    }
  }
  //free(typestate_initial);
  delete [] typestate_initial;
  //free(typestate_final);
  delete [] typestate_final;
  return fdb;
}

static void
src_fingerprintdb_remove_using_peeptab(src_fingerprintdb_elem_t **fdb, peep_table_t const *tab,
    use_types_t use_types, regmap_t const *in_regmap)
{
  peep_entry_t const *e;
  long i;

  for (i = 0; i < tab->hash_size; i++) {
    for (auto iter = tab->hash[i].begin(); iter != tab->hash[i].end(); iter++) {
      e = *iter;
      src_fingerprintdb_remove_variants(fdb, e->src_iseq, e->src_iseq_len, e->live_out,
          e->in_tmap, e->out_tmap, e->num_live_outs, use_types,
          e->mem_live_out, in_regmap);
    }
  }
}

static void
choose_random_itable_ipos_from_file(itable_t *itable, itable_position_t *ipos,
    char const *itab_filename, use_types_t use_types)
{
  NOT_IMPLEMENTED();
#if 0
  //XXX: currently, this function simply returns the first itable/ipos in the file.
  //char line[2048], last_line[2048];
  size_t itab_filename_len;
  FILE *fp;
  int ret;

  itab_filename_len = strlen(itab_filename);
  if (itab_filename[itab_filename_len - 7] == '.') {
    strncpy(itable->id_string, &itab_filename[itab_filename_len - 6],
        sizeof itable->id_string);
  } else {
    strncpy(itable->id_string, "test", sizeof itable->id_string);
  }
  fp = fopen(itab_filename, "r");
  ASSERT(fp);
  *ipos = itable_position_from_stream(fp); 
  *itable = itable_from_stream(fp);
  fclose(fp);
#endif
}

static void
harvest_from_peeptab(char const *harvest_filename, char const *peeptab_filename)
{
  NOT_IMPLEMENTED();
}

static void
src_fingerprintdb_check_with_harvest_output(src_fingerprintdb_elem_t **fingerprintdb,
    dst_insn_t * const *harvested_dst_iseqs,
    long const *harvested_dst_iseq_lens,
    long num_harvested_dst_iseqs, char const *equiv_output_filename)
{
  src_fingerprintdb_elem_t *cur, *row/*, **rows*/;
  long i, r, nextpc_indir, num_rows;

  //rows = (fingerprintdb_elem_t **)malloc(FINGERPRINTDB_SIZE *
  //    sizeof(fingerprintdb_elem_t *));
  //ASSERT(rows);

  for (i = 0; i < num_harvested_dst_iseqs; i++) {
    src_fingerprintdb_query(fingerprintdb, harvested_dst_iseqs[i],
        harvested_dst_iseq_lens[i], equiv_output_filename);
  }
  //free(rows);
}

/*
static void
compute_min_costs(long long *cur_costs, i386_insn_t **iseqs, long *iseq_lens,
    long num_iseqs)
{
  long long cost;
  long i, c;

  for (i = 0; i < i386_num_costfns; i++) {
    cur_costs[i] = COST_INFINITY;
  }

  for (i = 0; i < num_iseqs; i++) {
    for (c = 0; c < i386_num_costfns; c++) {
      cost = i386_iseq_compute_cost(iseqs[i], iseq_lens[i], i386_costfns[c]);
      if (cost < cur_costs[c]) {
        cur_costs[c] = cost;
      }
    }
  }
}
*/

static void
peepgen_get_src_iseq_work_multiple(translation_instance_t *ti,
    src_insn_t *src_iseq[], long src_iseq_max_alloc, long src_iseq_len[],
    regset_t *live_out[], transmap_t in_tmap[], transmap_t *out_tmaps[],
    long out_tmaps_max_alloc, size_t num_out_tmaps[], bool mem_live_out[],
    char *tagline[],
    long tagline_max_alloc,
    dst_insn_t **windfalls[],
    long *windfall_lens[], long windfalls_max_alloc, long num_windfalls[],
    struct itable_t *itables[], long itables_max_alloc,
    size_t num_itables[], long num_choices)
{
  long i, num_tries;
  bool ret;

  i = 0;
  num_tries = 0;
  while (i < num_choices) {
    num_tries++;
    ret = superoptdb_get_src_iseq_work(ti, src_iseq[i], src_iseq_max_alloc,
        &src_iseq_len[i], live_out[i], &in_tmap[i],
        out_tmaps[i],
        out_tmaps_max_alloc, &num_out_tmaps[i], &mem_live_out[i],
        tagline[i], tagline_max_alloc,
        windfalls[i], windfall_lens[i],
        windfalls_max_alloc,
        &num_windfalls[i], itables[i],
        itables_max_alloc, &num_itables[i]);
    ASSERT(ret);

    DBG_EXEC2(SUPEROPTDB,
        long j;
        regsets_to_string(live_out[i], num_out_tmaps[i], as2, sizeof as2);
        printf("Got from superoptdb_server i %ld:\n%s--\n%s--\n%s--\n", i,
            src_iseq_to_string(src_iseq[i], src_iseq_len[i], as1, sizeof as1),
            as2,
            transmap_to_string(&in_tmap[i], as3, sizeof as3));
        for (j = 0; j < num_windfalls[i]; j++) {
          printf("%s== windfall %ld : tagline %s ==\n",
              dst_iseq_to_string(windfalls[i][j], windfall_lens[i][j], as1,
                  sizeof as1), j, tagline[i]);
        }
    );

    if (   !superoptimizer_supports_src(src_iseq[i], src_iseq_len[i])
        || !src_iseq_merits_optimization(src_iseq[i], src_iseq_len[i])
        || transmaps_contain_mapping_to_loc(&in_tmap[i], out_tmaps[i], num_out_tmaps[i],
               TMAP_LOC_SYMBOL)) {
      free_get_src_iseq_retvals(&itables[i], &num_itables[i], &windfalls[i],
          &num_windfalls[i], 1);
      continue;
    }
    i++;
    ASSERT(num_tries < 1000 * num_choices);
  }
}

static long
src_iseq_num_live_def_bits(src_insn_t const *src_iseq, long src_iseq_len,
    regset_t const *live_out, long num_live_out)
{
  autostop_timer func_timer(__func__);
  memset_t memuse, memdef;
  regset_t use, def;
  long i, ret;

  memset_init(&memuse);
  memset_init(&memdef);
  src_iseq_get_usedef(src_iseq, src_iseq_len, &use, &def, &memuse, &memdef);

  for (i = 0; i < num_live_out; i++) {
    regset_intersect(&def, &live_out[i]);
  }
  ret = regset_count_bits(&def);
  //ret += src_iseq_compute_num_nextpcs(src_iseq, src_iseq_len, NULL) * DWORD_LEN;
  ret += num_live_out * DWORD_LEN;
  ret += memdef.memaccess.size() * DWORD_LEN;
  memset_free(&memuse);
  memset_free(&memdef);
  return ret;
}

static long
mkitab_choose_best_match(transmap_t const *in_tmap, transmap_t * const *out_tmaps,
    size_t const *num_out_tmaps, itable_t * const itables[],
    size_t const num_itables[], long N)
{
  long i, c, min_num_itables;

  c = 0;
  min_num_itables = MAX_NUM_ITABLES;

  for (i = 0; i < N; i++) {
    if (transmaps_contain_mapping_to_loc(&in_tmap[i], out_tmaps[i], num_out_tmaps[i],
        TMAP_LOC_SYMBOL)) {
      continue;
    }
    if (transmaps_equal(&in_tmap[i], transmap_none())) {
      continue;
    }
    if (num_itables[i] <= min_num_itables) {
      min_num_itables = num_itables[i];
      c = i;
    }
  }
  return c;
}

static long
run_harvest_choose_best_match(char const *tag, char *taglines[],
    src_insn_t const * const src_iseqs[],
    long const src_iseq_lens[], regset_t const * const live_out[],
    size_t const num_live_out[], long n)
{
  ssize_t prefix_len, max_prefix_len, num_live_def_bits, max_num_live_def_bits;
  long i, ret;

  max_prefix_len = -1;
  max_num_live_def_bits = -1;
  ret = 0;
  for (i = 0; i < n; i++) {
    prefix_len = longest_prefix_len(tag, taglines[i]);
    if (prefix_len < max_prefix_len) {
      continue;
    }
    if (prefix_len > max_prefix_len) {
      ret = i;
      max_prefix_len = prefix_len;
      max_num_live_def_bits = src_iseq_num_live_def_bits(src_iseqs[i], src_iseq_lens[i],
          live_out[i], num_live_out[i]);
      continue;
    }
    ASSERT(prefix_len == max_prefix_len);
    num_live_def_bits = src_iseq_num_live_def_bits(src_iseqs[i], src_iseq_lens[i],
        live_out[i], num_live_out[i]);
    if (num_live_def_bits < max_num_live_def_bits) {
      continue;
    }
    if (num_live_def_bits > max_num_live_def_bits) {
      ret = i;
      max_num_live_def_bits = num_live_def_bits;
      continue;
    }
    ASSERT(num_live_def_bits == max_num_live_def_bits); //tie breaker: FCFS
  }
  return ret;
}

static void
free_for_get_src_iseq_work_multiple(transmap_t **out_tmaps, regset_t **live_out,
    src_insn_t **src_iseq, dst_insn_t ***windfalls, long **windfall_lens,
    itable_t **itables,
    char **taglines, long N)
{
  long i;

  for (i = 0; i < N; i++) {
    //free(out_tmaps[i]);
    delete [] out_tmaps[i];
    //free(live_out[i]);
    delete [] live_out[i];
    //free(src_iseq[i]);
    delete [] src_iseq[i];
    free(windfalls[i]);
    free(windfall_lens[i]);
    //free(itables[i]);
    delete [] itables[i];
    free(taglines[i]);
  }
}

static void
alloc_for_get_src_iseq_work_multiple(transmap_t **out_tmaps, regset_t **live_out,
    src_insn_t **src_iseq, dst_insn_t ***windfalls, long **windfall_lens,
    itable_t **itables,
    char **taglines, long N)
{
  long i;

  for (i = 0; i < N; i++) {
    //out_tmaps[i] = (transmap_t *)malloc(LIVE_OUT_MAX_ALLOC * sizeof(transmap_t));
    out_tmaps[i] = new transmap_t[LIVE_OUT_MAX_ALLOC];
    ASSERT(out_tmaps[i]);
    //live_out[i] = (regset_t *)malloc(LIVE_OUT_MAX_ALLOC * sizeof(regset_t));
    live_out[i] = new regset_t[LIVE_OUT_MAX_ALLOC];
    ASSERT(live_out[i]);
    //src_iseq[i] = (src_insn_t *)malloc(SRC_ISEQ_MAX_ALLOC * sizeof(src_insn_t));
    src_iseq[i] = new src_insn_t[SRC_ISEQ_MAX_ALLOC];
    ASSERT(src_iseq[i]);
    windfalls[i] = (dst_insn_t **)malloc(MAX_NUM_WINDFALLS * sizeof(dst_insn_t *));
    ASSERT(windfalls[i]);
    windfall_lens[i] = (long *)malloc(MAX_NUM_WINDFALLS * sizeof(long));
    ASSERT(windfall_lens[i]);
    itables[i] = (itable_t *)malloc(MAX_NUM_ITABLES * sizeof(itable_t));
    ASSERT(itables[i]);
    taglines[i] = (char *)malloc(TAGLINE_MAX_ALLOC * sizeof(char));
    ASSERT(taglines[i]);
  }
}

/*static void
itables_free(itable_t *itables, long num_itables)
{
  long i;

  for (i = 0; i < num_itables; i++) {
    itable_free(&itables[i]);
  }
}*/

/*static void
read_src_iseq_work_from_peeptab(translation_instance_t *ti,
    char const *peeptab_filename,
    src_insn_t *src_iseq, long src_iseq_max_alloc, long *src_iseq_len,
    regset_t *live_out, transmap_t *in_tmap, transmap_t *out_tmaps,
    long live_out_max_alloc, long *num_live_out)
{
  peep_entry_t const *e;
  long i, j;

  read_peep_trans_table(ti, &ti->peep_table, peeptab_filename);

  //read first entry from peeptab_filename
  for (i = 0; i < ti->peep_table.hash_size; i++) {
    e = ti->peep_table.hash[i];
    if (!e) {
      continue;
    }
    *src_iseq_len = e->src_iseq_len;
    src_iseq_copy(src_iseq, e->src_iseq, e->src_iseq_len);
    ASSERT(e->num_live_outs <= live_out_max_alloc);
    *num_live_out = e->num_live_outs;
    for (j = 0; j < e->num_live_outs; j++) {
      regset_copy(&live_out[j], &e->live_out[j]);
    }
    transmap_copy(in_tmap, e->in_tmap);
    transmaps_copy(out_tmaps, e->out_tmap, e->num_live_outs);
    return;
  }
  printf("empty peephole table!\n");
  NOT_REACHED();
}*/

static void
peepgen_run_mkitab(peep_entry_t const *e, FILE *itab_fp, char const *types_filename)
{
  NOT_IMPLEMENTED();
#if 0
  long const N = 3;
  size_t num_out_tmaps[N], num_itables[N];
  long num_windfalls[N];
  typestate_t *typestate_initial, *typestate_final;
  long i, j, k, c, num_new_itables, num_all_insns;
  long *windfall_lens[N], src_iseq_len[N];
  //size_t new_itable_lens[MAX_NUM_ITABLES];
  itable_t new_itables[MAX_NUM_ITABLES];
  transmap_t in_tmap[N], *out_tmaps[N];
  dst_insn_t **windfalls[N];
  translation_instance_t ti;
  long all_insns_max_alloc;
  src_insn_t *src_iseq[N];
  dst_insn_t *all_insns;
  regset_t *live_out[N];
  bool mem_live_out[N];
  itable_t *itables[N];
  char *taglines[N];

  all_insns_max_alloc = 819200;
  //all_insns = (dst_insn_t *)malloc(all_insns_max_alloc * sizeof(dst_insn_t));
  all_insns = new dst_insn_t[all_insns_max_alloc];
  ASSERT(all_insns);

  num_all_insns = read_all_dst_insns(all_insns, all_insns_max_alloc);

  /*for (i = 0; i < MAX_NUM_ITABLES; i++) {
   new_itables[i] = (itable_entry_t *)malloc(ITABLES_MAX_ALLOC * sizeof(itable_entry_t));
   ASSERT(new_itables[i]);
  }*/

  alloc_for_get_src_iseq_work_multiple(out_tmaps, live_out, src_iseq, windfalls,
      windfall_lens, itables, taglines, N);

  translation_instance_init(&ti, SUPEROPTIMIZE);

  //typestate_initial = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
  typestate_initial = new typestate_t[NUM_FP_STATES];
  ASSERT(typestate_initial);
  //typestate_final = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
  typestate_final = new typestate_t[NUM_FP_STATES];
  ASSERT(typestate_final);

  do {
    if (e) {
      src_iseq_len[0] = e->src_iseq_len;
      src_iseq_copy(src_iseq[0], e->src_iseq, e->src_iseq_len);
      num_out_tmaps[0] = e->num_live_outs;
      for (i = 0; i < e->num_live_outs; i++) {
        regset_copy(&live_out[0][i], &e->live_out[i]);
      }
      transmap_copy(&in_tmap[0], e->in_tmap);
      transmaps_copy(out_tmaps[0], e->out_tmap, e->num_live_outs);
      /* read_src_iseq_work_from_peeptab(&ti, peeptab_filename,
          src_iseq[0], SRC_ISEQ_MAX_ALLOC, &src_iseq_len[0],
          live_out[0], &in_tmap[0], out_tmaps[0], LIVE_OUT_MAX_ALLOC, num_out_tmaps); */
      c = 0;
    } else {
      peepgen_get_src_iseq_work_multiple(&ti, src_iseq, SRC_ISEQ_MAX_ALLOC, src_iseq_len,
          live_out, in_tmap, out_tmaps, LIVE_OUT_MAX_ALLOC, num_out_tmaps,
          mem_live_out, taglines,
          TAGLINE_MAX_ALLOC, windfalls, windfall_lens, MAX_NUM_WINDFALLS, num_windfalls,
          itables, MAX_NUM_ITABLES, num_itables, N);

      c = mkitab_choose_best_match(in_tmap, out_tmaps, num_out_tmaps,
              itables, num_itables, N);
      ASSERT(c >= 0 && c < N);
    }
    DBG(TMP, "chose:\n%s--\n%s--\n",
        src_iseq_to_string(src_iseq[c], src_iseq_len[c], as1, sizeof as1),
        transmaps_to_string(&in_tmap[c], out_tmaps[c], num_out_tmaps[c],
            as2, sizeof as2));
    num_new_itables = build_itables(src_iseq[c], src_iseq_len[c], live_out[c],
        &in_tmap[c],
        out_tmaps[c], num_out_tmaps[c], new_itables,
        MAX_NUM_ITABLES, all_insns, num_all_insns);
    DBG(SUPEROPTDB, "itables built. num_new_itables = %ld.\n", num_new_itables);

    if (e) {
      //fprintf(fp, "Printing itables for:\n");
      //printf("%s\n", src_iseq_to_string(src_iseq[c], src_iseq_len[c], as1, sizeof as1));
      //regsets_to_string(live_out[c], num_out_tmaps[c], as1, sizeof as1);
      //printf("--\n%s\n", as1);
      //transmaps_to_string(&in_tmap[c], out_tmaps[c], num_out_tmaps[c], as1, sizeof as1);
      //printf("--\n%s==\n", as1);
      if (!itab_fp) {
        itab_fp = stdout;
      }
      if (types_filename) {
        regmap_t pregmap;
        int i;

        typestate_from_file(typestate_initial, typestate_final, types_filename);
  
        for (i = 0; i < NUM_FP_STATES; i++) {
          typestate_canonicalize(typestate_initial, typestate_final, &pregmap);
          DBG(TYPESTATE, "pregmap:\n%s\n", regmap_to_string(&pregmap, as1, sizeof as1));
        }
        for (i = 0; i < num_new_itables; i++) {
          ASSERT(NUM_FP_STATES > 0);
          itable_truncate_using_typestate(&new_itables[i], &typestate_initial[0],
              &typestate_final[0]);
          itable_typestates_add_integer_temp_if_needed(&new_itables[i],
              typestate_initial, typestate_final, NUM_FP_STATES);
        }
      }

      NOT_IMPLEMENTED(); //implemented in src_iseq_make_itables
      //itables_print(itab_fp, new_itables, num_new_itables, false);
      //itables_free(new_itables, num_new_itables);
      break;
    }

    DBG2(SUPEROPTDB, "num_new_itables = %ld, num_itables = %ld.\n", num_new_itables,
        num_itables[c]);
    for (j = 0; j < num_itables[c]; j++) {
      bool found;
      found = false;
      for (k = 0; k < num_new_itables; k++) {
        if (itables_equal(&new_itables[k], &itables[c][j])) {
          found = true;
          break;
        }
      }
      if (!found) {
        DBG2(SUPEROPTDB, "calling superoptdb_remove:\n%s--\n%s--\n",
            src_iseq_to_string(src_iseq[c], src_iseq_len[c], as1, sizeof as1),
            transmaps_to_string(&in_tmap[c], out_tmaps[c], num_out_tmaps[c],
                as2, sizeof as2));
        DBG2(SUPEROPTDB, "itable:\n%s\n", itable_to_string(&itables[c][j], as1, sizeof as1));
        superoptdb_remove(NULL, src_iseq[c], src_iseq_len[c], &in_tmap[c], out_tmaps[c],
            live_out[c], num_out_tmaps[c], mem_live_out[c], NULL, 0, &itables[c][j]);
      }
    }
    for (k = 0; k < num_new_itables; k++) {
      DBG2(SUPEROPTDB, "adding new_itables[%ld]:\n%s\n", k,
          itable_to_string(&new_itables[k], as1, sizeof as1));
      superoptdb_add(NULL, src_iseq[c], src_iseq_len[c], &in_tmap[c], out_tmaps[c],
          live_out[c], num_out_tmaps[c], mem_live_out[c],
          taglines[c], NULL, 0, &new_itables[k]);
    }
    ASSERT(!e);
    free_get_src_iseq_retvals(itables, num_itables, windfalls, num_windfalls, N);
    NOT_IMPLEMENTED();
    //itables_free(new_itables, num_new_itables);
  } while (1);
  translation_instance_free(&ti);
  free_for_get_src_iseq_work_multiple(out_tmaps, live_out, src_iseq, windfalls,
      windfall_lens, itables, taglines, N);
  //free(all_insns);
  delete [] all_insns;
  //free(typestate_initial);
  delete [] typestate_initial;
  //free(typestate_final);
  delete [] typestate_final;
#endif
}

static bool
peepgen_run_itable(peep_table_t const *peeptab,
    peep_entry_t const *peep_entry, char const *itab_filename,
    char const *jtab_filename, char const *jtable_cur_regmap_filename,
    char const *types_filename,
    long yield_secs, bool slow_fingerprint,
    use_types_t use_types,
    bool mkjtab_only, char const *equiv_output_filename)
{
  NOT_IMPLEMENTED();
#if 0
  typestate_t typestate_initial[NUM_FP_STATES], typestate_final[NUM_FP_STATES];
  bool found, stop_when_equivalent_sequence_found;
  struct src_fingerprintdb_elem_t **fingerprintdb = NULL;
  char const *orig_equiv_output_filename = NULL;
  long long max_costs[dst_num_costfns];
  regmap_t pregmap, prev_pregmap;
  itable_position_t ipos;
  static itable_t itable;
  char _template[1024];
  jtable_t jtable;
  int i, fd;
  FILE *fp;

  ASSERT(!(peeptab && peep_entry)); //at-most one of them can be non-NULL

  if (itab_filename && peeptab && types_filename) {
    choose_random_itable_ipos_from_file(&itable, &ipos, itab_filename, use_types);
    typestate_from_file(typestate_initial, typestate_final, types_filename);

    for (i = 0; i < NUM_FP_STATES; i++) {
      typestate_canonicalize(&typestate_initial[i], &typestate_final[i], &pregmap);
      if (i > 0) {
        ASSERT(regmaps_equal(&pregmap, &prev_pregmap));
      }
      DBG(TYPESTATE, "pregmap:\n%s\n", regmap_to_string(&pregmap, as1, sizeof as1));
      regmap_copy(&prev_pregmap, &pregmap);
    }

    fingerprintdb = src_fingerprintdb_from_peeptab(peeptab, use_types, &pregmap);

    itable_truncate_using_typestate(&itable, &typestate_initial[0], &typestate_final[0]);
    itable_typestates_add_integer_temp_if_needed(&itable, typestate_initial,
        typestate_final, NUM_FP_STATES);

    src_fingerprintdb_compute_max_costs(fingerprintdb, max_costs);

    orig_equiv_output_filename = equiv_output_filename;
    equiv_output_filename = "equiv.output.tmp";
    stop_when_equivalent_sequence_found = true;
    if (itable.use_types == USE_TYPES_OPS_AND_GE_INIT_MEET_FINAL) {
      DBG(TYPESTATE, "typestate_meet between init and final. before:\n%s\n",
          typestate_to_string(typestate_final, as1, sizeof as1));
      DBG(TYPESTATE, "typestate_initial:\n%s\n",
          typestate_to_string(typestate_initial, as1, sizeof as1));
      typestates_meet(typestate_final, typestate_initial, NUM_FP_STATES);
      DBG(TYPESTATE, "typestate_meet between init and final. after:\n%s\n",
          typestate_to_string(typestate_final, as1, sizeof as1));
    } else if (itable.use_types == USE_TYPES_OPS_AND_GE_MODIFIED_BOTTOM) {
      typestates_set_modified_to_bottom(typestate_final, typestate_initial,
          NUM_FP_STATES);
    }
    DBG(TYPESTATE, "fingerprintdb:\n%s\n",
        ({src_fingerprintdb_to_string(as1, sizeof as1, fingerprintdb, false); as1;}));
    DBG(TYPESTATE, "typestate_initial[0]:\n%s\n",
        typestate_to_string(&typestate_initial[0], as1, sizeof as1));
    DBG(TYPESTATE, "itable.use_types = %s, typestate_final[0]:\n%s\n",
        use_types_to_string(itable.use_types, as2, sizeof as2),
        typestate_to_string(&typestate_final[0], as1, sizeof as1));
    DBG(TYPESTATE, "itable:\n%s\n", itable_to_string(&itable, as1, sizeof as1));
    if (jtab_filename) {
      jtable_read(&jtable, &itable, jtab_filename, jtable_cur_regmap_filename);
    } else {
      jtable_init(&jtable, &itable, jtable_cur_regmap_filename);
    }
    if (mkjtab_only) {
      return false;
    }
  } else if (itab_filename && peep_entry) {
    choose_random_itable_ipos_from_file(&itable, &ipos, itab_filename, use_types);
    typestate_init_using_itable(typestate_initial, typestate_final, &itable);
    fingerprintdb = src_fingerprintdb_from_peep_entry(peep_entry, itable.use_types,
        typestate_initial, typestate_final);
    src_fingerprintdb_compute_max_costs(fingerprintdb, max_costs);
    stop_when_equivalent_sequence_found = true;
    if (itable.use_types == USE_TYPES_OPS_AND_GE_INIT_MEET_FINAL) {
      typestates_meet(typestate_final, typestate_initial, NUM_FP_STATES);
    } else if (itable.use_types == USE_TYPES_OPS_AND_GE_MODIFIED_BOTTOM) {
      typestates_set_modified_to_bottom(typestate_final, typestate_initial,
          NUM_FP_STATES);
    }
    if (jtab_filename) {
      jtable_read(&jtable, &itable, jtab_filename, jtable_cur_regmap_filename);
    } else {
      jtable_init(&jtable, &itable, jtable_cur_regmap_filename);
    }
    if (mkjtab_only) {
      return false;
    }
  } else if (peep_entry) {
    snprintf(_template, sizeof _template, ABUILD_DIR "/superopt-files/itab_tmp.XXXXXX");
    fd = mkstemp(_template);
    ASSERT(fd >= 0);
    fp = fdopen(fd, "w");
    ASSERT(fp);
    peepgen_run_mkitab(peep_entry, fp, types_filename);
    fclose(fp);

    choose_random_itable_ipos_from_file(&itable, &ipos, _template, use_types);
    typestate_init_using_itable(typestate_initial, typestate_final, &itable);
    fingerprintdb = src_fingerprintdb_from_peep_entry(peep_entry, itable.use_types,
        typestate_initial, typestate_final);
    src_fingerprintdb_compute_max_costs(fingerprintdb, max_costs);
    stop_when_equivalent_sequence_found = true;
    if (itable.use_types == USE_TYPES_OPS_AND_GE_INIT_MEET_FINAL) {
      typestates_meet(typestate_final, typestate_initial, NUM_FP_STATES);
    } else if (itable.use_types == USE_TYPES_OPS_AND_GE_MODIFIED_BOTTOM) {
      typestates_set_modified_to_bottom(typestate_final, typestate_initial, NUM_FP_STATES);
    }
    if (jtab_filename) {
      jtable_read(&jtable, &itable, jtab_filename, jtable_cur_regmap_filename);
    } else {
      jtable_init(&jtable, &itable, jtable_cur_regmap_filename);
    }
    if (mkjtab_only) {
      return false;
    }
  }

  for (;;) {
    if (!peep_entry && !peeptab) {
      ASSERT(!itab_filename);
      ASSERT(!peep_entry);
      stop_when_equivalent_sequence_found = false;
      fingerprintdb = superoptdb_get_itable_work(&itable, &ipos,
          typestate_initial, typestate_final);
      if (!fingerprintdb) {
        sleep(10); //retry after 10 seconds
        continue;
      }
      src_fingerprintdb_compute_max_costs(fingerprintdb, max_costs);
      if (itable.use_types == USE_TYPES_OPS_AND_GE_INIT_MEET_FINAL) {
        typestates_meet(typestate_final, typestate_initial, NUM_FP_STATES);
      } else if (itable.use_types == USE_TYPES_OPS_AND_GE_MODIFIED_BOTTOM) {
        typestates_set_modified_to_bottom(typestate_final, typestate_initial, NUM_FP_STATES);
      }
      DBG(TYPESTATE, "typestate_initial[0]:\n%s\n",
          typestate_to_string(&typestate_initial[0], as1, sizeof as1));
      DBG(TYPESTATE, "itable.use_types = %s, typestate_final[0]:\n%s\n",
          use_types_to_string(itable.use_types, as2, sizeof as2),
          typestate_to_string(&typestate_final[0], as1, sizeof as1));
      ASSERT(!jtab_filename);
      jtable_init(&jtable, &itable, jtable_cur_regmap_filename);
    }
    DBG_EXEC3(JTABLE,
        src_fingerprintdb_to_string(as1, sizeof as1, fingerprintdb, false);
        printf("%s() %d: fingerprintdb:\n%s==\n", __func__, __LINE__, as1);
    );

    found = itable_enumerate(&itable, &jtable, fingerprintdb, &ipos, max_costs,
        yield_secs,
        stop_when_equivalent_sequence_found,
        slow_fingerprint,// use_types,
        typestate_initial, typestate_final, equiv_output_filename);

    char ts[256];
    get_cur_timestamp(ts, sizeof ts);
    DBG(JTABLE, "stop: %s.\n", ts);

    if (!peep_entry && !peeptab) {
      superoptdb_itable_work_done(fingerprintdb, &itable, &ipos);
    }

    if (peep_entry) {
      printf("finished one round of itable_enumeration, found = %d:\n", found);
      printf("max_vector: %s\n", itable_position_to_string(&ipos, as1, sizeof as1));
      return found;
    } else if (peeptab) {
      peep_table_t tmp_peeptab;

      printf("finished one round of itable_enumeration, found = %d:\n", found);
      printf("max_vector: %s\n", itable_position_to_string(&ipos, as1, sizeof as1));
      peep_table_init(&tmp_peeptab);
      read_peep_trans_table(NULL, &tmp_peeptab, equiv_output_filename, NULL);

      ASSERT(orig_equiv_output_filename);
      file_append(orig_equiv_output_filename, equiv_output_filename);

      src_fingerprintdb_remove_using_peeptab(fingerprintdb, &tmp_peeptab,
          use_types, &pregmap);
      file_truncate(equiv_output_filename);
      peephole_table_free(&tmp_peeptab);

      if (src_fingerprintdb_is_empty(fingerprintdb)) {
        return true;
      }
      if (!found) {
        return false;
      }
    } else {
      src_fingerprintdb_free(fingerprintdb);
      //itable_free(&itable);
      jtable_free(&jtable);
    }
  }
  NOT_REACHED();
#endif
}

static void
peepgen_run_harvest(char const *harvest_filename, long yield_secs,
    char const *equiv_output_filename)
{
  long const N = 32;
  uint64_t cur_epoch, last_harvest_read_epoch;
  long *windfall_lens[N], *itable_sizes[N];
  size_t num_itables[N], num_out_tmaps[N];
  long src_iseq_len[N], num_windfalls[N];
  //long long cur_costs[i386_num_costfns];
  transmap_t *out_tmaps[N], in_tmap[N];
  dst_insn_t **harvested_dst_iseqs;
  struct time harvest_read_time;
  long *harvested_dst_iseq_lens;
  long num_harvested_dst_iseqs;
  dst_insn_t **windfalls[N];
  translation_instance_t ti;
  src_insn_t *src_iseq[N];
  regset_t *live_out[N];
  bool mem_live_out[N];
  itable_t *itables[N];
  char *taglines[N];
  char tag[1024];
  long i, n, c;
  bool ret;

  alloc_for_get_src_iseq_work_multiple(out_tmaps, live_out, src_iseq, windfalls,
      windfall_lens, itables, taglines, N);

  n = harvest_file_count_entries(harvest_filename);
  printf("Counted %ld entries in harvest file %s.\n", n, harvest_filename);
  harvested_dst_iseqs = (dst_insn_t **)malloc(n * sizeof(dst_insn_t *));
  ASSERT(harvested_dst_iseqs);
  harvested_dst_iseq_lens = (long *)malloc(n * sizeof(long));
  ASSERT(harvested_dst_iseq_lens);

  translation_instance_init(&ti, SUPEROPTIMIZE);

  num_harvested_dst_iseqs = 0;
  //harvest_read_time.flags = SW_WALLCLOCK;

  for (;;) {
    cur_epoch = time_since_beginning(get_cur_timestamp(as1, sizeof as1));
    if (   num_harvested_dst_iseqs == 0
        || (cur_epoch - last_harvest_read_epoch >=
               (yield_secs * harvest_read_time.wallClock_elapsed)/1e+6)) {
      if (num_harvested_dst_iseqs) {
        break;
      }
      stopwatch_reset(&harvest_read_time);
      stopwatch_run(&harvest_read_time);
      num_harvested_dst_iseqs = read_harvest_output_dst(harvested_dst_iseqs,
          harvested_dst_iseq_lens, n, tag, sizeof tag, harvest_filename);
      stopwatch_stop(&harvest_read_time);
      printf("read harvest table tag %s at epoch %llx, num_harvested_dst_iseqs=%ld. "
          "harvest_read_time=%llx.\n", tag,
          (long long)cur_epoch, num_harvested_dst_iseqs,
          harvest_read_time.wallClock_elapsed);
      last_harvest_read_epoch = cur_epoch;
    }

    peepgen_get_src_iseq_work_multiple(&ti, src_iseq, SRC_ISEQ_MAX_ALLOC,
        src_iseq_len, live_out, in_tmap,
        out_tmaps,
        LIVE_OUT_MAX_ALLOC, num_out_tmaps, mem_live_out, taglines,
        TAGLINE_MAX_ALLOC, windfalls, windfall_lens,
        MAX_NUM_WINDFALLS,
        num_windfalls, itables, MAX_NUM_ITABLES,
        num_itables, N);

    c = run_harvest_choose_best_match(tag, taglines, (src_insn_t const **)src_iseq,
        src_iseq_len,
        (regset_t const **)live_out,
        num_out_tmaps, N);

    src_fingerprintdb_elem_t **fingerprintdb;
    char *harvest_output, *ptr, *end;
    size_t max_alloc_harvest_output;

    max_alloc_harvest_output = 409600;
    harvest_output = (char *)malloc(max_alloc_harvest_output * sizeof(char));
    ASSERT(harvest_output);
    
    ptr = harvest_output;
    end = harvest_output + max_alloc_harvest_output;

    ptr += src_harvested_iseq_to_string(ptr, end - ptr, src_iseq[c],
        src_iseq_len[c], live_out[c], num_out_tmaps[c], mem_live_out[c]);
    ptr += snprintf(ptr, end - ptr, "--\n");
    transmaps_to_string(&in_tmap[c], out_tmaps[c], num_out_tmaps[c],
        ptr, end - ptr);
    ptr += strlen(ptr);
    ASSERT(ptr < end);

    DBG(SUPEROPTDB, "%s: chose tagline[c]: %s:\n%s\n", tag, taglines[c],
        harvest_output);

    fingerprintdb = src_fingerprintdb_init();
    src_fingerprintdb_add_from_string(fingerprintdb, harvest_output, USE_TYPES_NONE, NULL, NULL);
    free(harvest_output);

    //compute_min_costs(cur_costs, windfalls[c], windfall_lens[c], num_windfalls[c]);
    src_fingerprintdb_check_with_harvest_output(fingerprintdb,
        harvested_dst_iseqs, harvested_dst_iseq_lens, num_harvested_dst_iseqs,
        equiv_output_filename);

    src_fingerprintdb_free(fingerprintdb);
    free_get_src_iseq_retvals(itables, num_itables, windfalls, num_windfalls, N);
  }

  translation_instance_free(&ti);

  for (n = 0; n < num_harvested_dst_iseqs; n++) {
    //free(harvested_dst_iseqs[n]);
    delete [] harvested_dst_iseqs[n];
  }
  free(harvested_dst_iseqs);
  free(harvested_dst_iseq_lens);
  free_for_get_src_iseq_work_multiple(out_tmaps, live_out, src_iseq, windfalls,
      windfall_lens, itables, taglines, N);
  printf("peepgen_run_harvest done.\n");
}

static void
equiv_failure(peep_entry_t const *peep_entry, char const *str)
{
  printf("%s failed on\n%s\ndst_iseq:\n%s\nlive_out0: %s\n", str,
      src_iseq_to_string(peep_entry->src_iseq, peep_entry->src_iseq_len, as1,
        sizeof as1),
      dst_iseq_to_string(peep_entry->dst_iseq, peep_entry->dst_iseq_len, as2,
        sizeof as2),
      regset_to_string(&peep_entry->live_out[0], as3, sizeof as3));
  if (terminate_on_equiv_failure) {
    exit(1);
  }
}

static char pbuf1[512000];

#if 0
static void
verify_peep_entry(peep_entry_t const *peep_entry, long max_unroll)
{
  if (dst_iseq_contains_reloc_reference(peep_entry->dst_iseq,
      peep_entry->dst_iseq_len)) {
    return;
  }
  if (   verify_fingerprint
      && !transmaps_contain_mapping_to_loc(peep_entry->in_tmap, peep_entry->out_tmap,
             peep_entry->num_live_outs, TMAP_LOC_SYMBOL)
      && !check_fingerprints(peep_entry->src_iseq, peep_entry->src_iseq_len,
             peep_entry->in_tmap, peep_entry->out_tmap, peep_entry->dst_iseq,
             peep_entry->dst_iseq_len, peep_entry->live_out,
             peep_entry->num_live_outs, peep_entry->mem_live_out)) {
    equiv_failure(peep_entry, "check_fingerprints()");
  }

  DBG(EQUIV, "checking peep entry at line number %ld.\n", peep_entry->line_number);
  DBG2(BEQUIV, "checking peep entry at line number %ld.\n", peep_entry->line_number);

#if (ARCH_SRC == ARCH_I386 && ARCH_DST == ARCH_I386)
/*  if (   i386_iseqs_equal_mod_imms(peep_entry->src_iseq, peep_entry->src_iseq_len,
             peep_entry->dst_iseq, peep_entry->dst_iseq_len)
      && transmap_mappings_are_identity(peep_entry->in_tmap)) {
    printf("%s() %d: verifying:\n%s--\n", __func__, __LINE__,
        src_iseq_to_string(peep_entry->src_iseq, peep_entry->src_iseq_len, pbuf1, sizeof pbuf1));
    printf("live :%s\n--\n",
        ({regsets_to_string(peep_entry->live_out, peep_entry->num_live_outs, pbuf1, sizeof pbuf1); pbuf1;}));
    printf("%s--\n",
        transmaps_to_string(peep_entry->in_tmap, peep_entry->out_tmap, peep_entry->num_live_outs, pbuf1, sizeof pbuf1));
    printf("%s==\n",
        dst_iseq_to_string(peep_entry->dst_iseq, peep_entry->dst_iseq_len, pbuf1, sizeof pbuf1));
    DBG(BEQUIV, "%s", "returning true because i386_iseqs_equal_mod_imms\n");
    return;
  }
*/
#endif

  /*if (dst_iseq_contains_div(peep_entry->dst_iseq, peep_entry->dst_iseq_len)) {
    DBG(BEQUIV, "%s", "returning true because i386_iseq contains div\n");
    return;
  }*/

  if (!check_equivalence(peep_entry->src_iseq, peep_entry->src_iseq_len,
        peep_entry->in_tmap, peep_entry->out_tmap,
        peep_entry->dst_iseq, peep_entry->dst_iseq_len,
        peep_entry->live_out, peep_entry->nextpc_indir,
        peep_entry->num_live_outs, peep_entry->mem_live_out,
        //peep_entry->src_locals_info, peep_entry->dst_locals_info,
        //&peep_entry->src_iline_map, &peep_entry->dst_iline_map,
        max_unroll, qt)) {
    //equiv_failure(peep_entry, "check_equivalence()");
  }
}

//extern char const *peeptab_filename;

static void
test_peephole_table(peep_table_t const *tab, long max_unroll)
{
  peep_entry_t const *peep_entry;
  long i;

  for (i = 0; i < tab->hash_size; i++) {
    for (auto iter = tab->hash[i].begin(); iter != tab->hash[i].end(); iter++) {
      peep_entry = *iter;
      verify_peep_entry(peep_entry, max_unroll);
    }
  }
  printf("test_peephole table done.\n");
}
#endif

static void
disable_signals(void)
{
  sigset_t intmask;

  if (sigfillset(&intmask) == -1) {
    printf("Failed to initialize the signal mask.\n");
    NOT_REACHED();
  }
  if (sigdelset(&intmask, SIGINT) == -1) {
    printf("Failed to initialize the signal mask.\n");
    NOT_REACHED();
  }
  if (sigprocmask(SIG_BLOCK, &intmask, NULL) == -1) {
    printf("Failed to initialize the signal mask.\n");
    NOT_REACHED();
  }
}

static bool
enum_verify_peephole_table(struct peep_table_t const *tab, char const *itab_filename, char const *jtab_filename, bool slow_fingerprint,
    use_types_t use_types, bool mkjtab_only, char const *types_filename)
{
  char itab_filename_store[1024];
  peep_entry_t const *e;
  bool ret, found;
  FILE *fp;
  long i;

  ret = true;
  for (i = 0; i < tab->hash_size; i++) {
    for (auto iter = tab->hash[i].begin(); iter != tab->hash[i].end(); iter++) {
      e = *iter;
      if (!superoptimizer_supports_src(e->src_iseq, e->src_iseq_len)) {
        continue;
      }
      if (!itab_filename) {
        snprintf(itab_filename_store, sizeof itab_filename_store, ABUILD_DIR "/superopt-files/itab.XXXXXX");
        int fd = mkstemp(itab_filename_store);
        ASSERT(fd >= 0);
        close(fd);
        fp = fopen(itab_filename_store, "w");
        ASSERT(fp);
        peepgen_run_mkitab(e, fp, types_filename);
        fclose(fp);
        itab_filename = itab_filename_store;
      }
      printf("calling peepgen_run_itable on:\n");
      peeptab_entry_write(stdout, e, NULL);
      found = peepgen_run_itable(NULL, e, itab_filename, jtab_filename, NULL, NULL, LONG_MAX,
          slow_fingerprint,
          use_types, mkjtab_only, NULL);
      if (!found) {
        printf("enum test failed on:\n");
        peeptab_entry_write(stdout, e, NULL);
        ret = false;
      } else {
        printf("enum test passed on:\n");
        peeptab_entry_write(stdout, e, NULL);
      }
    }
  }
  return ret;
}

/*static void
testme(void)
{
  char const *a1 = "testb %al, 123\n";
  char const *a2 = "testb 123, %al\n";
  char buf1[32], buf2[32];
  int len1, len2;

  len1 = x86_assemble(NULL, buf1, a1, I386_AS_MODE_64_REGULAR);
  len2 = x86_assemble(NULL, buf2, a2, I386_AS_MODE_64_REGULAR);

  printf("buf1 : %s.\n", hex_string(buf1, len1, as1, sizeof as1));
  printf("buf2 : %s.\n", hex_string(buf2, len2, as1, sizeof as1));
}*/

static peep_entry_t const *
peeptab_get_one_entry(peep_table_t const *peeptab)
{
  return peep_entry_first(peeptab);
  /*long i;
  for (i = 0; i < peeptab->hash_size; i++) {
    if (peeptab->hash[i].size()) {
      return *peeptab->hash[i].begin();
    }
  }
  return NULL;*/
}

static void
remove_smt_query_files(void)
{
  char prefix[strlen(SMT_QUERY_FILENAME_PREFIX) + 128];
  snprintf(prefix, sizeof prefix, SMT_QUERY_FILENAME_PREFIX "%d.", getpid());
  printf("%s(): removing smt_query files %s.*.smt2\n", __func__, prefix);
  char command[strlen(prefix) + 256];
  snprintf(command, sizeof command, "rm -rf %s.*.smt2", prefix);
  system(command);
}

int
main(int argc, char **argv)
{
  char const *superoptdb_path = SUPEROPTDB_PATH_DEFAULT;
  char const *output_filename = "equiv.output";
  char const *server = SUPEROPTDB_SERVER_DEFAULT;
  char const *jtable_cur_regmap_filename = NULL;
  char const *mkitab_print_filename = NULL;
  long yield_secs = DEFAULT_YIELD_SECS;
  char const *harvest_filename = NULL;
  char const *peeptab_filename = SUPEROPTDBS_DIR "fb.trans.tab";
  char const *types_filename = NULL;
  char const *itab_filename = NULL;
  char const *jtab_filename = NULL;
  char const *dump_filename = NULL;
  //bool ignore_insn_line_map = false;
  bool enum_using_peeptab = false;
  bool enum_using_harvest = false;
  bool slow_fingerprint = false;
  bool type_categorize = false;
  bool gen_candidates = false;
  translation_instance_t ti;
  bool make_itables = false;
  //bool run_forever = false;
  bool enum_verify = false;
  bool mkjtab_only = false;
  peep_entry_t const *e;
  char peeptab_tag[128];
  int random_seed = 0;
  bool verify = false;
  long i, max_unroll;
  int optind;
  char *r;

  running_as_peepgen = true;
  peepgen = true;
  global_timeout_secs = 2000*60*60;
  init_timers();
  cmd_helper_init();

  g_ctx_init(); //this should be first, as it involves a fork which can be expensive if done after usedef_init()
  solver_init(); //this also involves a fork and should be as early as possible
  g_query_dir_init();
  src_init();
  dst_init();
  g_se_init();
  //types_init();
  usedef_init();
  types_init();
  //yices_initialize();

  cpu_set_log_filename(DEFAULT_LOGFILE);

  //testme();

  verify_fingerprint = false;
  verify_execution = false;
  //verify_boolean = false;
  verify_syntactic_check = false;
  max_unroll = 0;
  peeptab_tag[0] = '\0';
  g_exec_name = argv[0];

  optind = 1;
  for (;;) {
    if (optind >= argc) {
      break;
    }
    r = argv[optind];
    if (r[0] != '-') {
      break;
    }
    optind++;
    r++;
    if (!strcmp(r, "-")) {
      break;
    } else if (!strcmp(r, "yield")) {
      ASSERT(optind < argc);
      yield_secs = strtol(argv[optind++], NULL, 0);
    } else if (!strcmp(r, "max_unroll")) {
      ASSERT(optind < argc);
      max_unroll = strtol(argv[optind++], NULL, 0);
    } else if (!strcmp(r, "enumh")) {
      enum_using_harvest = true;
    } else if (!strcmp(r, "enump")) {
      enum_using_peeptab = true;
    } else if (!strcmp(r, "h")) {
      ASSERT(optind < argc);
      harvest_filename = argv[optind++];
    } else if (!strcmp(r, "p")) {
      ASSERT(optind < argc);
      peeptab_filename = argv[optind++];
    } else if (!strcmp(r, "i")) {
      ASSERT(optind < argc);
      itab_filename = argv[optind++];
    } else if (!strcmp(r, "j")) {
      ASSERT(optind < argc);
      jtab_filename = argv[optind++];
    } else if (!strcmp(r, "t")) {
      ASSERT(optind < argc);
      types_filename = argv[optind++];
    } else if (!strcmp(r, "v")) {
      verify = true;
    } else if (!strcmp(r, "enumv")) {
      enum_verify = true;
    } else if (!strcmp(r, "b")) {
      //verify_boolean = true;
    } else if (!strcmp(r, "e")) {
      verify_execution = true;
    } else if (!strcmp(r, "fp")) {
      verify_fingerprint = true;
    } else if (!strcmp(r, "fp_imm_map_is_constant")) {
      fp_imm_map_is_constant = true;
    } else if (!strcmp(r, "dump")) {
      ASSERT(optind < argc);
      dump_filename = argv[optind++];
    } else if (!strcmp(r, "superoptdb")) {
      ASSERT(optind < argc);
      server = argv[optind++];
    } else if (!strcmp(r, "superoptdb_path")) {
      ASSERT(optind < argc);
      superoptdb_path = argv[optind++];
    } else if (!strcmp(r, "mkitab")) {
      make_itables = true;
    } else if (!strcmp(r, "mkjtab_only")) {
      mkjtab_only = true;
    } else if (!strcmp(r, "mkitab_print_filename")) {
      ASSERT(optind < argc);
      mkitab_print_filename = argv[optind++];
    /*} else if (!strcmp(r, "f")) {
      run_forever = true;*/
    } else if (!strcmp(r, "noterm")) {
      terminate_on_equiv_failure = false;
    } else if (!strcmp(r, "slow_fingerprint")) {
      slow_fingerprint = true;
    } else if (!strcmp(r, "use_types")) {
      ASSERT(optind < argc);
      use_types_from_string(&default_use_types, argv[optind++]);
    } else if (!strcmp(r, "o")) {
      ASSERT(optind < argc);
      output_filename = argv[optind++];
    /*} else if (!strcmp(r, "src_stack_gpr")) {
      ASSERT(optind < argc);
      src_stack_gpr = atoi(argv[optind++]);
    } else if (!strcmp(r, "dst_stack_gpr")) {
      ASSERT(optind < argc);
      dst_stack_gpr = atoi(argv[optind++]);*/
    } else if (!strcmp(r, "rand")) {
      ASSERT(optind < argc);
      random_seed = atoi(argv[optind++]);
    } else if (!strcmp(r, "verify_syntactic_check")) {
      ASSERT(optind < argc);
      verify_syntactic_check = bool_from_string(argv[optind++], NULL);
    } else if (!strcmp(r, "peeptab_tag")) {
      ASSERT(optind < argc);
      strncpy(peeptab_tag, argv[optind++], sizeof peeptab_tag);
    } else if (!strcmp(r, "assume_fcall_conventions")) {
      peepgen_assume_fcall_conventions = true;
    } else if (!strcmp(r, "type_categorize")) {
      type_categorize = true;
    } else if (!strcmp(r, "gen_candidates")) {
      gen_candidates = true;
    } else if (!strcmp(r, "jtable_cur_regmap_filename")) {
      ASSERT(optind < argc);
      jtable_cur_regmap_filename = argv[optind++];
    /*} else if (!strcmp(r, "ignore_insn_line_map")) {
      ignore_insn_line_map = true;*/
    } else if (!strcmp(r, "help")) {
      usage();
      exit(0);
    } else {
      usage();
      exit(1);
    }
  }

  printf("pid %d: Using random seed %d\n", getpid(), random_seed);
  srand_random(random_seed);

  if (!verify_fingerprint && !verify_execution/* && !verify_boolean*/) {
    verify_fingerprint = true;
    verify_execution = true;
    //verify_boolean = true;
  }

  DBG(PEEPGEN, "Calling superoptdb_init(%s)\n", server);
  if (   !peeptab_filename
      /*|| !(verify || enum_verify)
      || verify_fingerprint
      || verify_execution*/) {
    if (!superoptdb_init(server, superoptdb_path)) {
      exit(1);
    }
  }
  DBG(PEEPGEN, "superoptdb_server = %s.\n", superoptdb_server);

  disable_signals();  // disable for smt_query and timeout

  if (peeptab_filename && type_categorize) {
    printf("reading peep table %s.\n", peeptab_filename);
    equiv_init();
    translation_instance_init(&ti, SUPEROPTIMIZE);
    read_peep_trans_table(&ti, &ti.peep_table, peeptab_filename, peeptab_tag);
    peephole_table_type_categorize(&ti.peep_table, output_filename);
    return 0;
  }

  if (verify || enum_verify) {
    bool ret;

    equiv_init();
    printf("Verifying current peephole table . . .\n");
    translation_instance_init(&ti, SUPEROPTIMIZE);
  
    if (peeptab_filename) {
      printf("reading peep table %s.\n", peeptab_filename);
      read_peep_trans_table(&ti, &ti.peep_table, peeptab_filename, peeptab_tag);
    } else {
      peep_table_load(&ti);
    }
    /*if (ignore_insn_line_map) {
      peeptab_free_insn_line_maps(&ti.peep_table);
    }*/
    if (verify) {
      NOT_REACHED(); //this flag is deprecated
      //test_peephole_table(&ti.peep_table, max_unroll);
      //ret = true;
    } else if (enum_verify) {
      ret = enum_verify_peephole_table(&ti.peep_table, itab_filename, jtab_filename, slow_fingerprint,
          default_use_types, mkjtab_only, types_filename);
    } else NOT_REACHED();
    translation_instance_free(&ti);
    print_all_timers();
    g_stats_print();
    call_on_exit_function();
    return ret?0:1;
  }

  if (peeptab_filename) {
    equiv_init();
    translation_instance_init(&ti, SUPEROPTIMIZE);
    read_peep_trans_table(&ti, &ti.peep_table, peeptab_filename, peeptab_tag);

    /*if (!types_filename) {
      static char types_filename_store[1024];
      typestate_t *ts_init, *ts_fin;
      peep_entry_t const *e;
      int fd, i;
      FILE *fp;

      snprintf(types_filename_store, sizeof types_filename_store,
          ABUILD_DIR "/superopt-files/types.XXXXXX");
      fd = mkstemp(types_filename_store);
      ASSERT(fd >= 0);
      close(fd);
      types_filename = types_filename_store;

      e = peeptab_get_one_entry(&ti.peep_table);
      //ts_init = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
      ts_init = new typestate_t[NUM_FP_STATES];
      ASSERT(ts_init);
      //ts_fin = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
      ts_fin = new typestate_t[NUM_FP_STATES];
      ASSERT(ts_fin);
      peep_entry_compute_typestate(ts_init, ts_fin, e);
      emit_types_to_file(types_filename, ts_init, ts_fin);
      //free(ts_init);
      delete [] ts_init;
      //free(ts_fin);
      delete [] ts_fin;
    }*/

    if (!itab_filename) {
      static char itab_filename_store[512];
      peep_entry_t const *e;
      FILE *fp;

      itab_filename = itab_filename_store;
      e = peeptab_get_one_entry(&ti.peep_table);
      ASSERT(e);
      snprintf(itab_filename_store, sizeof itab_filename_store,
          ABUILD_DIR "/superopt-files/itab.XXXXXX");
      int fd = mkstemp(itab_filename_store);
      ASSERT(fd >= 0);
      close(fd);
      fp = fopen(itab_filename_store, "w");
      ASSERT(fp);
      peepgen_run_mkitab(e, fp, types_filename);
      fclose(fp);
    }

    peepgen_run_itable(&ti.peep_table, NULL, itab_filename, jtab_filename,
        jtable_cur_regmap_filename,
        types_filename,
        yield_secs, slow_fingerprint, default_use_types, mkjtab_only, output_filename);
    call_on_exit_function();
    return 0;
  }

  if (dump_filename) {
    ASSERT(!peeptab_filename);
    ASSERT(superoptdb_server);
    superoptdb_write_peeptab(dump_filename, false);
    return 0;
  }

  if (make_itables) {
    FILE *itab_fp;

    translation_instance_init(&ti, SUPEROPTIMIZE);
    read_peep_trans_table(&ti, &ti.peep_table, peeptab_filename, peeptab_tag);
    if (mkitab_print_filename) {
      if (!strcmp(mkitab_print_filename, "stdout")) {
        itab_fp = stdout;
      } else {
        itab_fp = fopen(mkitab_print_filename, "w");
        ASSERT(itab_fp);
      }
    } else {
      itab_fp = NULL;
    }

    e = peep_entry_first(&ti.peep_table);
    ASSERT(e);
    peepgen_run_mkitab(e, itab_fp, types_filename);
    if (itab_fp && itab_fp != stdout) {
      fclose(itab_fp);
    }
    translation_instance_free(&ti);
    return 0;
  }

  equiv_init();
  //caml_main(argv);
  if (enum_using_harvest) {
    ASSERT(harvest_filename);
    peepgen_run_harvest(harvest_filename, yield_secs, output_filename);
    return 0;
  }

  if (enum_using_peeptab) {
    ASSERT(peeptab_filename);
    char harvest_filename[strlen(peeptab_filename) + 256];
    snprintf(harvest_filename, sizeof harvest_filename, "%s.harvest", peeptab_filename);
    harvest_from_peeptab(harvest_filename, peeptab_filename);
    peepgen_run_harvest(peeptab_filename, yield_secs, output_filename);
    return 0;
  }

  if (peeptab_filename) {
    translation_instance_init(&ti, SUPEROPTIMIZE);
    read_peep_trans_table(&ti, &ti.peep_table, peeptab_filename, peeptab_tag);
    for (i = 0; i < ti.peep_table.hash_size; i++) {
      for (auto iter = ti.peep_table.hash[i].begin(); iter != ti.peep_table.hash[i].end(); iter++) {
        e = *iter;
        if (!superoptimizer_supports_src(e->src_iseq, e->src_iseq_len)) {
          continue;
        }
        peepgen_run_itable(NULL, e, itab_filename, jtab_filename,
            jtable_cur_regmap_filename, NULL, yield_secs,
            slow_fingerprint,
            default_use_types, mkjtab_only, output_filename);
      }
    }
    translation_instance_free(&ti);
  } else {
    peepgen_run_itable(NULL, NULL, itab_filename, jtab_filename,
        jtable_cur_regmap_filename, NULL, yield_secs,
        slow_fingerprint,
        default_use_types, mkjtab_only, output_filename);
  }

  g_stats_print();
  call_on_exit_function();
  remove_smt_query_files();
  return 0;
}
