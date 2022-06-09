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
#include "graph/prove.h"

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

#define SRC_ISEQ_MAX_ALLOC 2048
#define ITABLES_MAX_ALLOC (4096 * 1024)
#define LIVE_OUT_MAX_ALLOC SRC_ISEQ_MAX_ALLOC
#define MAX_NUM_WINDFALLS 65536
#define TAGLINE_MAX_ALLOC 409600
#define MAX_NUM_ITABLES 32

static size_t
enumerate_loc_choices(long *loc_choices, long *regnum_choices,
    long src_group, long src_reg,
    long const *num_dst_exregs_used)
{
  size_t ret;
  int i;

  ret = 0;
#if ARCH_SRC == ARCH_DST
  if (num_dst_exregs_used[src_group] == DST_NUM_EXREGS(src_group)) {
    loc_choices[ret] = TMAP_LOC_SYMBOL;
    regnum_choices[ret] = SRC_ENV_EXOFF(src_group, src_reg);
    ret++;
  } else {
    loc_choices[ret] = TMAP_LOC_EXREG(src_group);
    regnum_choices[ret] = num_dst_exregs_used[src_group];
    ret++;
  }
  return ret;
#endif

  if (num_dst_exregs_used[DST_EXREG_GROUP_GPRS] ==
          DST_NUM_EXREGS(DST_EXREG_GROUP_GPRS)) {
    loc_choices[ret] = TMAP_LOC_SYMBOL;
    regnum_choices[ret] = SRC_ENV_EXOFF(src_group, src_reg);
    ret++;
  } else {
    loc_choices[ret] = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
    regnum_choices[ret] = num_dst_exregs_used[DST_EXREG_GROUP_GPRS];
    ret++;
  }
#if ARCH_SRC == ARCH_PPC
  if (src_group == PPC_EXREG_GROUP_CRF) {
    if (num_dst_exregs_used[I386_EXREG_GROUP_EFLAGS_UNSIGNED] == 0) {
      loc_choices[ret] = TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_UNSIGNED);
      regnum_choices[ret] = 0;
      ret++;
    }
    if (num_dst_exregs_used[I386_EXREG_GROUP_EFLAGS_SIGNED] == 0) {
      loc_choices[ret] = TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_SIGNED);
      regnum_choices[ret] = 0;
      ret++;
    }
  }
#endif
  return ret;
}

static transmap_entry_t
transmap_entry_from_loc_and_regnum(uint8_t loc, dst_ulong regnum)
{
  transmap_entry_t ret;
  if (loc == TMAP_LOC_SYMBOL) {
    ret.set_to_symbol_loc(loc, regnum);
  } else {
    ASSERT(loc >= TMAP_LOC_EXREG(0) && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
    ret.set_to_exreg_loc(loc, regnum, TMAP_LOC_MODIFIER_DST_SIZED);
  }
  return ret;
}

static size_t
transmaps_enumerate_rec(transmap_t *tmaps, size_t tmaps_size,
    regset_t const *live_in_or_out, long const *num_dst_exregs_used,
    size_t cur_num_tmaps, long cur_src_group,
    long cur_src_reg)
{
  long dst_exregs_used[DST_NUM_EXREG_GROUPS], num_loc_choices, i;
  long loc_choices[DST_NUM_EXREG_GROUPS + 1];
  long regnum_choices[DST_NUM_EXREG_GROUPS + 1];
  transmap_t cur_tmap;

  ASSERT(cur_src_group >= 0 && cur_src_group <= SRC_NUM_EXREG_GROUPS);
  ASSERT(cur_src_reg >= 0 && cur_src_reg <= SRC_NUM_EXREGS(cur_src_group));
  ASSERT(cur_num_tmaps >= 0 && cur_num_tmaps <= tmaps_size);

  if (cur_src_reg == SRC_NUM_EXREGS(cur_src_group)) {
    cur_src_reg = 0;
    cur_src_group += 1;
  }
  if (cur_src_group == SRC_NUM_EXREG_GROUPS) {
    cur_src_reg = 0;
    cur_src_group = 0;
    return cur_num_tmaps + 1;
  }

  ASSERT(cur_num_tmaps <= tmaps_size);
  if (cur_num_tmaps == tmaps_size) {
    return cur_num_tmaps;
  }

  if (cur_src_group == 0 && cur_src_reg == 0) {
    transmap_copy(&tmaps[cur_num_tmaps], transmap_none());
  }

  while (!regset_belongs_ex(live_in_or_out, cur_src_group, cur_src_reg)) {
    cur_src_reg += 1;
    if (cur_src_reg == SRC_NUM_EXREGS(cur_src_group)) {
      cur_src_group += 1;
      cur_src_reg = 0;
    }
    if (cur_src_group == SRC_NUM_EXREG_GROUPS) {
      ASSERT(cur_src_reg == 0);
      cur_src_group = 0;
      return cur_num_tmaps + 1;
    }
  }

  num_loc_choices = enumerate_loc_choices(loc_choices, regnum_choices,
      cur_src_group, cur_src_reg, num_dst_exregs_used);
  ASSERT(num_loc_choices >= 1);
  transmap_copy(&cur_tmap, &tmaps[cur_num_tmaps]);

  for (i = 0; i < num_loc_choices && cur_num_tmaps < tmaps_size; i++) {
    transmap_copy(&tmaps[cur_num_tmaps], &cur_tmap);
    memcpy(dst_exregs_used, num_dst_exregs_used, sizeof(dst_exregs_used));
    transmap_entry_t tmap_entry = transmap_entry_from_loc_and_regnum(loc_choices[i], regnum_choices[i]);
    tmaps[cur_num_tmaps].extable[cur_src_group][cur_src_reg] = tmap_entry;
    //tmaps[cur_num_tmaps].extable[cur_src_group][cur_src_reg].num_locs = 1;
    //tmaps[cur_num_tmaps].extable[cur_src_group][cur_src_reg].loc[0] =
    //    loc_choices[i];
    //tmaps[cur_num_tmaps].extable[cur_src_group][cur_src_reg].regnum[0] =
    //    regnum_choices[i];
    if (   loc_choices[i] >= TMAP_LOC_EXREG(0)
        && loc_choices[i] < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
      dst_exregs_used[loc_choices[i] - TMAP_LOC_EXREG(0)]++;
    }
    cur_num_tmaps = transmaps_enumerate_rec(tmaps, tmaps_size, live_in_or_out,
        dst_exregs_used, cur_num_tmaps, cur_src_group, cur_src_reg + 1);
  }
  return cur_num_tmaps;
}

static size_t
transmaps_enumerate(transmap_t *tmaps, size_t tmaps_size,
    regset_t const *live_in_or_out)
{
  long num_dst_exregs_used[DST_NUM_EXREG_GROUPS];

  memset(num_dst_exregs_used, 0, sizeof(num_dst_exregs_used));
  return transmaps_enumerate_rec(tmaps, tmaps_size, live_in_or_out,
      num_dst_exregs_used, 0, 0, 0);
}

static void
superoptdb_addp(char const *peeptab_filename, bool add_dst_iseq)
{
  translation_instance_t ti(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity());

  translation_instance_init(&ti, SUPEROPTIMIZE);

  read_peep_trans_table(&ti, &ti.peep_table, peeptab_filename, NULL);

  ti_add_peep_table_to_superoptdb(&ti, add_dst_iseq);

  translation_instance_free(&ti);
}

static void
remove_peeptab_from_superoptdb(char const *peeptab_filename)
{
  translation_instance_t ti(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity());

  translation_instance_init(&ti, SUPEROPTIMIZE);
  read_peep_trans_table(&ti, &ti.peep_table, peeptab_filename, NULL);
  ti_remove_peep_table_from_superoptdb(&ti);
  translation_instance_free(&ti);
}

static void
add_harvest_output_to_superoptdb(char const *filename)
{
  long src_iseq_len;
  size_t num_live_out;
  //itable_entry_t *itable[MAX_NUM_ITABLES];
  //size_t itable_len[MAX_NUM_ITABLES];
  src_insn_t *src_iseq;
  regset_t *live_out;
  bool mem_live_out;
  transmap_t *tmaps;
  char *line;
  FILE *fp;
  long i;

  fp = fopen(filename, "r");
  if (!fp) {
    return;
  }
  //src_iseq = (src_insn_t *)malloc(SRC_ISEQ_MAX_ALLOC * sizeof(src_insn_t));
  src_iseq = new src_insn_t[SRC_ISEQ_MAX_ALLOC];
  ASSERT(src_iseq);
  //live_out = (regset_t *)malloc(LIVE_OUT_MAX_ALLOC * sizeof(regset_t));
  live_out = new regset_t[LIVE_OUT_MAX_ALLOC];
  ASSERT(live_out);
  line = (char *)malloc(409600 * sizeof(char));
  ASSERT(line);
  /*for (i = 0; i < MAX_NUM_ITABLES; i++) {
    itable[i] = (itable_entry_t *)malloc(ITABLES_MAX_ALLOC * sizeof(itable_entry_t));
    ASSERT(itable[i]);
  }*/

#define MAX_TMAPS_PER_ISEQ 64
#define MAX_LIVE_OUT_ENUM 64
  //tmaps = (transmap_t *)malloc(MAX_TMAPS_PER_ISEQ * sizeof(transmap_t));
  tmaps = new transmap_t[MAX_TMAPS_PER_ISEQ];
  ASSERT(tmaps);

  while (fscanf_harvest_output_src(fp, src_iseq, SRC_ISEQ_MAX_ALLOC, &src_iseq_len,
      live_out, LIVE_OUT_MAX_ALLOC, &num_live_out, &mem_live_out, 0) != EOF) {
    int num_read;
    num_read = fscanf (fp, "== %*d %*d %s ==\n", line);
    ASSERT(num_read == 1);
    if (!src_iseq_merits_optimization(src_iseq, src_iseq_len)) {
      continue;
    }
    //printf("read_harvested_iseq. ftell=%lx\n", ftell(fp));

    regset_t use, def, touch, live_in_or_out;
    long num_tmaps, i, j, k, l; //num_itables, 

    src_iseq_get_usedef_regs(src_iseq, src_iseq_len, &use, &def);
    regset_copy(&live_in_or_out, &use);
    regset_copy(&touch, &use);
    regset_union(&touch, &def);

    for (i = 0; i < num_live_out; i++) {
      regset_union(&live_in_or_out, &live_out[i]);
    }

    num_tmaps = transmaps_enumerate(tmaps, MAX_TMAPS_PER_ISEQ, &live_in_or_out);

    for (i = 0; i < num_tmaps; i++) {
      transmap_t out_tmaps[num_live_out];
      for (j = 0; j < num_live_out; j++) {
        transmap_copy(&out_tmaps[j], &tmaps[i]);
      }
      /*num_itables = build_itables(src_iseq, src_iseq_len, &tmaps[i], out_tmaps,
          num_live_out, itable,
          itable_len, ITABLES_MAX_ALLOC, MAX_NUM_ITABLES);*/
      superoptdb_add(NULL, src_iseq, src_iseq_len, &tmaps[i], out_tmaps, live_out,
          num_live_out, mem_live_out, line, NULL, 0, NULL);
      /*for (k = 0; k < num_itables; k++) {
        DBG(SUPEROPT, "itable_len[%ld] = %ld\n", k, itable_len[k]);
        superoptdb_add(NULL, src_iseq, src_iseq_len, &tmaps[i], out_tmaps, live_out,
            num_live_out, line, NULL, 0, itable[k], itable_len[k]);
      }*/
    }
  }
  //free(tmaps);
  delete [] tmaps;
  /*for (i = 0; i < MAX_NUM_ITABLES; i++) {
    free(itable[i]);
  }*/
  //free(src_iseq);
  delete [] src_iseq;
  //free(live_out);
  delete [] live_out;
  free(line);
  fclose(fp);
}

static void
usage(void)
{
  NOT_REACHED();
}


/*static void
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
}*/

static char pbuf1[512000];

int
main(int argc, char **argv)
{
  char const *superoptdb_path = SUPEROPTDB_PATH_DEFAULT;
  bool add_harvested_sequences_to_superoptdb = NULL;
  char const *server = SUPEROPTDB_SERVER_DEFAULT;
  char const *remove_peeptab_filename = NULL;
  char const *harvest_filename = NULL;
  char const *addps_filename = NULL;
  char const *dump_filename = NULL;
  char const *addp_filename = NULL;
  //bool ignore_insn_line_map = false;
  //bool run_forever = false;
  bool shutdown = false;
  peep_entry_t const *e;
  bool verify = false;
  int optind;
  long i;
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
  usedef_init();
  types_init();

  cpu_set_log_filename(DEFAULT_LOGFILE);

  //testme();

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
    } else if (!strcmp(r, "shutdown")) {
      shutdown = true;
    } else if (!strcmp(r, "addh")) {
      ASSERT(optind < argc);
      add_harvested_sequences_to_superoptdb = true;
    } else if (!strcmp(r, "addp")) {
      ASSERT(optind < argc);
      addp_filename = argv[optind++];
    } else if (!strcmp(r, "addps")) {
      ASSERT(optind < argc);
      addps_filename = argv[optind++];
    } else if (!strcmp(r, "removep")) {
      ASSERT(optind < argc);
      remove_peeptab_filename = argv[optind++];
    } else if (!strcmp(r, "h")) {
      ASSERT(optind < argc);
      harvest_filename = argv[optind++];
    } else if (!strcmp(r, "dump")) {
      ASSERT(optind < argc);
      dump_filename = argv[optind++];
    } else if (!strcmp(r, "superoptdb")) {
      ASSERT(optind < argc);
      server = argv[optind++];
    } else if (!strcmp(r, "superoptdb_path")) {
      ASSERT(optind < argc);
      superoptdb_path = argv[optind++];
    } else if (!strcmp(r, "use_types")) {
      ASSERT(optind < argc);
      use_types_from_string(&default_use_types, argv[optind++]);
    } else if (!strcmp(r, "help")) {
      usage();
      exit(0);
    } else {
      usage();
      exit(1);
    }
  }

  if (!superoptdb_init(server, superoptdb_path)) {
    solver_kill();
    call_on_exit_function();
    return 1;
  }
  DBG(PEEPGEN, "superoptdb_server = %s.\n", superoptdb_server);

  if (shutdown) {
    superoptdb_shutdown();
    solver_kill();
    call_on_exit_function();
    return 0;
  }

  if (add_harvested_sequences_to_superoptdb) {
    ASSERT(superoptdb_server);
    ASSERT(harvest_filename);
    add_harvest_output_to_superoptdb(harvest_filename);
    solver_kill();
    call_on_exit_function();
    return 0;
  }
  if (addp_filename) {
    ASSERT(superoptdb_server);
    superoptdb_addp(addp_filename, true);
    solver_kill();
    call_on_exit_function();
    return 0;
  }
  if (addps_filename) {
    ASSERT(superoptdb_server);
    superoptdb_addp(addps_filename, false);
    solver_kill();
    call_on_exit_function();
    return 0;
  }
  if (remove_peeptab_filename) {
    ASSERT(superoptdb_server);
    remove_peeptab_from_superoptdb(remove_peeptab_filename);
    solver_kill();
    call_on_exit_function();
    return 0;
  }
  if (dump_filename) {
    ASSERT(superoptdb_server);
    superoptdb_write_peeptab(dump_filename, false);
    solver_kill();
    call_on_exit_function();
    return 0;
  }
  NOT_REACHED();
  return 0;
}
