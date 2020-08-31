#include <stdint.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

//#include "config.h"
//#include "cpu.h"
//#include "bbl.h"
//#include "exec-all.h"

#include "debug.h"
//#include "transmap.h"
//#include "dbg_functions.h"
//#include "static_translate.h"
//#include "ppc/regs.h"
//#include "memset.h"

//#include "support/utils.h"

//#include "elf/elf.h"
//#include "rbtree.h"
//#include "elf/elftypes.h"
//#include "translation_instance.h"
//#include "analyze_stack.h"
//#include "ppc/insn.h"

//bool running_as_fbgen = false;
char const *g_exec_name = NULL;

bool peepgen = false;

struct timeval start_time = { .tv_sec=0, .tv_usec = 0 };

int dump_full_state = 0;
bool dyn_debug = false;
int track_time = 0;
//int use_fast_mode = 0;
int mark_peep_translations = 0;
uint32_t track_memlocs[256];
int num_track_memlocs = 0;

unsigned long int pslices_start[256];
unsigned long int pslices_stop[256];
int num_pslices = 0;

char const *speed_functions[32];
int num_speed_functions = 0;

int prune_row_size = 16;

char const *fix_transmaps_logfile = NULL;

//long long stats_num_fingerprinted_dst_sequences = 0;
long long stats_num_fingerprint_tests[ENUM_MAX_LEN];
long long stats_num_fingerprint_type_failures[ENUM_MAX_LEN][ENUM_MAX_LEN];
long long stats_num_fingerprint_typecheck_success[ENUM_MAX_LEN];
long long stats_num_syntactic_match_tests = 0;
long long stats_num_execution_tests = 0;
long long stats_num_boolean_tests = 0;
long long stats_num_cg_constructions = 0, stats_num_cg_destructions = 0;
long long stats_num_aliasing_constraint_constructions = 0, stats_num_aliasing_constraint_destructions = 0;
long long stats_num_memlabel_assertions_constructions = 0, stats_num_memlabel_assertions_destructions = 0;
long long stats_num_invariant_state_constructions = 0, stats_num_invariant_state_destructions = 0;
long long stats_num_invariant_state_eqclass_constructions = 0, stats_num_invariant_state_eqclass_destructions = 0;
long long stats_num_predicate_constructions = 0, stats_num_predicate_destructions = 0;
long long stats_num_point_set_constructions = 0, stats_num_point_set_destructions = 0;
long long stats_num_point_val_constructions = 0, stats_num_point_val_destructions = 0;
long long stats_num_array_constant_constructions = 0, stats_num_array_constant_destructions = 0;
long long stats_num_counter_example_constructions = 0, stats_num_counter_example_destructions = 0;
long long stats_num_graphce_constructions = 0, stats_num_graphce_destructions = 0;

char progress_flag = 0;
DBG_INIT(STATS, 1);
DBG_INIT(QUERY_DECOMPOSE, 0);
DBG_INIT(SYM_EXEC, 0);
DBG_INIT(EQGEN, 0);
DBG_INIT(ALIAS_ANALYSIS, 0);
DBG_INIT(DWARF2, 0);
DBG_INIT(PEEPGEN, 0);
DBG_INIT(DWARF2OUT, 0);
DBG_INIT(DWARF2_DBG, 0);
DBG_INIT(STRICT_CANON, 0);
DBG_INIT(NEXTPC_MAP, 0);
DBG_INIT(H2P, 0);
DBG_INIT(MD_ASSEMBLE, 0);
DBG_INIT(TCDBG_I386, 0);
DBG_INIT(I386_DISAS, 0);
DBG_INIT(I386_READ, 0);
DBG_INIT(EIP_MAP, 0);
DBG_INIT(ITABLE_ENUM, 0);
//DBG_INIT(ENUM_TMAPS, 0);
//DBG_INIT(TMAP_TRANS, 0);
DBG_INIT(PEEP_SEARCH, 0);
DBG_INIT(PEEP_TRANS, 0);
DBG_INIT(PRUNE, 0);
DBG_INIT(PEEP_TRANS_DBG, 0);
//DBG_INIT(PEEP_TAB_ADD, 2);
DBG_INIT(PEEP_TAB, 0);
DBG_INIT(PPC_DISAS, 0);
DBG_INIT(PPC_INSN, 0);
DBG_INIT(DUMP_ELF, 0);
DBG_INIT(RELOC_STRINGS, 0);
DBG_INIT(STAT_TRANS, 0);
DBG_INIT(TB_TRANS, 0);
DBG_INIT(INVOKE_LINKER, 0);
DBG_INIT(TCONS, 0);
DBG_INIT(INFER_CONS, 0);
DBG_INIT(ASSIGN_CREGS, 0);
DBG_INIT(TMAPS_ACQUIESCE, 1);
DBG_INIT(REGMAP, 0);
DBG_INIT(DOMINATORS, 0);
DBG_INIT(MALLOC32, 0);
DBG_INIT(TYPESTATE, 0);
DBG_INIT(TFG_GET_TYPE_CONSTRAINTS, 0);
DBG_INIT(ELF_LD, 0);
DBG_INIT(REWRITE, 1);
//DBG_INIT(HOUDINI, 1);
DBG_INIT(DYNGEN, 0);
DBG_INIT(GEN_CODE, 0);
DBG_INIT(TB, 0);
DBG_INIT(I386_INSN, 0);
DBG_INIT(SRC_INSN, 0);
DBG_INIT(INSTRUMENT_MARKER_CALL, 0);
DBG_INIT(SRC_ISEQ_FETCH, 0);
DBG_INIT(SRC_USEDEF, 0);
DBG_INIT(TFG_GET_USEDEF, 0);
DBG_INIT(ENUMERATE_IN_OUT_TMAPS, 0);
DBG_INIT(I386_REGMAP, 0);
DBG_INIT(UDGRAPH, 0);
DBG_INIT(XMM_ALLOC, 0);
DBG_INIT(FIX_TMAPS, 0);
DBG_INIT(LINK_INDIR, 0);
DBG_INIT(TBS_COLLATE, 0);
DBG_INIT(SRC_EXEC, 0);
DBG_INIT(I386_EXEC, 0);
DBG_INIT(IMM_MAP, 0);
DBG_INIT(BEQUIV, 1);
DBG_INIT(ANALYZE_STACK, 0);
DBG_INIT(UNROLL_LOOPS, 0);
DBG_INIT(UNROLL_LOOPS_INFO, 1);
DBG_INIT(ITABLE, 0);
DBG_INIT(CFG_MAKE_REDUCIBLE, 0);
DBG_INIT(PSLICE, 0);
DBG_INIT(ISEQ_FETCH, 0);
DBG_INIT(PREV_ROW, 0);
DBG_INIT(CAMLARGS, 0);
DBG_INIT(SUPEROPT, 0);
DBG_INIT(SUPEROPTDB_ADD, 0);
DBG_INIT(INFLOOP_WARN, 1);
DBG_INIT(SUPEROPTDB_CLIENT, 0);
DBG_INIT(SUPEROPTDB_SERVER, 1);
DBG_INIT(LEARN, 0);
DBG_INIT(FB_QEMU, 0);
DBG_INIT(FB, 0);
DBG_INIT(FBCACHE, 0);
DBG_INIT(CONNECT_ENDPOINTS, 0);
DBG_INIT(CHOOSE_SOLUTION, 0);
DBG_INIT(REFRESH_DB, 0);
DBG_INIT(IMPLICIT_OPS, 0);
DBG_INIT(INSN_MATCH, 0);
DBG_INIT(I386_MATCH, 0);
DBG_INIT(INSN_BOUNDARIES, 0);
DBG_INIT(ANNOTATE_LOCALS, 0);
DBG_INIT(MEMLABEL_MAP, 0);
DBG_INIT(LOCALS_INFO, 0);
DBG_INIT(EXPR_TABLE_VISITOR, 0);
DBG_INIT(EXPR_FIXED_POINT, 0);
DBG_INIT(DWARF_EXPR_EVAL, 0);
DBG_INIT(FBGEN, 0);
DBG_INIT(ASSERTCHECKS, 1);
DBG_INIT(INSN_LINE_MAP, 0);
DBG_INIT(CMN_ISEQ_FROM_STRING, 0);
DBG_INIT(DST_INSN_RELATED, 0);
DBG_INIT(TFG_DOMINATOR_TREE, 0);
DBG_INIT(SPLIT_MEM, 0);
DBG_INIT(TFG_LOCS, 0);
DBG_INIT(GET_ADDR_SYMBOL, 0);
//DBG_INIT(LINEAR_RELATIONS, 0);
DBG_INIT(EXPR_SIZE_DURING_SIMPLIFY, 0);
DBG_INIT(SP_RELATIONS, 0);
DBG_INIT(EXPR2LOCID, 0);
DBG_INIT(SUBSTITUTE_USING_SPRELS, 0);
DBG_INIT(ADDR_REFS, 0);
DBG_INIT(REMOVE_ITE, 0);
DBG_INIT(CONSTPROP, 0);
DBG_INIT(EXPR_SIMPLIFY, 0);
DBG_INIT(EXPR_SIMPLIFY_SELECT, 0);
DBG_INIT(TFG_SIMPLIFY, 0);
DBG_INIT(IS_EXPR_EQUAL_SYNTACTIC, 0);
DBG_INIT(IS_OVERLAPPING, 0);
DBG_INIT(IS_CONTAINED_IN, 0);
DBG_INIT(EXPR_IS_INDEPENDENT_OF_KEYWORD, 0);
DBG_INIT(LLVM2TFG, 0);
DBG_INIT(LLVM_PASSMANAGER, 0);
DBG_INIT(LLVM_LIVENESS, 0);
DBG_INIT(COUNT_STACK_SLOTS, 0);
DBG_INIT(STACK_SLOT_CONSTRAINTS, 0);
DBG_INIT(SETUP_CODE, 0);
DBG_INIT(REGCONST, 0);
DBG_INIT(SMT_HELPER_PROCESS, 0);
DBG_INIT(PEEPTAB_VERIFY, 0);
DBG_INIT(ISEQ_GET_TFG, 0);
DBG_INIT(COMPUTE_LIVENESS, 0);
DBG_INIT(GRAPH_COLORING, 0);
DBG_INIT(DST_INSN_FOLDING, 0);
DBG_INIT(DFA, 0);
DBG_INIT(SUFFIXPATH_DFA, 0);
DBG_INIT(DST_INSN_DEAD_CODE_ELIM, 0);
DBG_INIT(DST_INSN_SCHEDULE, 0);
DBG_INIT(TI_PREDS, 0);
//DBG_INIT(COPY_PROP, 0);
DBG_INIT(TAIL_CALL, 0);
DBG_INIT(OPEN_DIAMOND, 0);
DBG_INIT(DST_ISEQ_GET_CFG, 0);
DBG_INIT(POPULATE_ESTIMATED_FLOW, 0);
DBG_INIT(WRN, 1);
DBG_INIT(TMP, 5);
DBG_INIT(ERR, 5);
DBG_INIT(INFER_INVARIANTS, 0);
DBG_INIT(BV_SOLVE, 0);
DBG_INIT(SAGE_QUERY, 0);
DBG_INIT(DEBUG_ONLY, 0);
DBG_INIT(CG_IS_WELL_FORMED, 0);
DBG_INIT(CONCRETE_VALUES_SET, 0);
DBG_INIT(INVARIANT_STATE, 0);
DBG_INIT(STABILITY, 0);
DBG_INIT(PRED_AVAIL_EXPR, 0);
DBG_INIT(BV_SOLVE_QUERY, 0);
DBG_INIT(CE_FUZZER, 0);
DBG_INIT(CE_RANDOM_GEN, 0);
DBG_INIT(UNALIASED_MEMSLOT, 0);
DBG_INIT(UNALIASED_BVEXTRACT, 0);
DBG_INIT(ALIAS_CONS, 0);
DBG_INIT(POINT_SET, 0);
DBG_INIT(CE_MEMLABELS, 0);
DBG_INIT(EXPR_ID_DETERMINISM, 0);
DBG_INIT(SSA_VARNAME_DEFINEDNESS, 0);
DBG_INIT(EQCLASS_INEQ, 0);
DBG_INIT(ADD_POINT_USING_CE, 0);
DBG_INIT(GRAPH_WITH_PATHS_CACHE_ASSERTS, 0);
DBG_INIT(EQCLASS_BV, 0);
DBG_INIT(EQCLASS_ARR, 0);
DBG_INIT(EQCLASS_HOUDINI, 0);
DBG_INIT(BV_SOLVE_VERT_DEDUP, 0);
DBG_INIT(SAFETY, 0);
DBG_INIT(READ_ASM_FILE, 0);
DBG_INIT(REMOVE_MARKER_CALL, 0);
DBG_INIT(ARGV_PRINT, 1);
DBG_INIT(ETFG_INSN_FETCH, 0);

// for debugging of dynamic debugger itself
DBG_INIT(DYN_DEBUG, 1);

//typical flags used for debugging harvest
DBG_INIT(HARVEST, 0);
DBG_INIT(CFG, 0);
DBG_INIT(SYMBOLS, 0);
DBG_INIT(JUMPTABLE, 0);

//typical flags used for debugging eq
DBG_INIT(CORRELATE, 0);
DBG_INIT(CE_ADD, 0);
DBG_INIT(INVARIANTS_MAP, 0);
DBG_INIT(UPDATE_DST_FCALL_EDGE, 0);
//DBG_INIT(EXPR_EQCLASSES, 0);
DBG_INIT(MEMLABEL_ASSERTIONS, 0);
DBG_INIT(GENERAL, 0);
DBG_INIT(EQCHECK, 0);

//typical flags used for debugging typecheck
DBG_INIT(EQUIV, 0);
DBG_INIT(SRC_TYPESTATE, 0);
DBG_INIT(JTABLE, 0);
//DBG_INIT(GEN_SANDBOXED_ISEQ, 0);
DBG_INIT(TYPECHECK, 0);
DBG_INIT(RAND_STATES, 0);
DBG_INIT(SUPEROPTDB, 0);
DBG_INIT(ETFG_ISEQ_GET_TFG, 0);

DBG_INIT(LOCKSTEP, 1);

bool assertcheck_si_init_done = false;

char *
track_memlocs_to_string(char *buf, size_t size)
{
  char *ptr = buf, *end = buf + size;
  int i;

  buf[0] = '\0';
  for (i = 0; i < num_track_memlocs; i++) {
    ptr += snprintf(ptr, end - ptr, "0x%x", track_memlocs[i]);
    if (i < num_track_memlocs - 1)
      ptr += snprintf(ptr, end - ptr, ",");
  }
  return buf;
}

void read_track_memlocs (char const *str)
{
  char const *ptr = str;
  num_track_memlocs = 0;


  for(;;) {
    char nextchar;
    int num_read, numchars;
    num_read = sscanf (ptr, "%x%c%n",
        &track_memlocs[num_track_memlocs++], &nextchar, &numchars);
    /*
       dbg ("ptr = %s, num_read = %d, num_track_memlocs=%d, latest 0x%x nc %c\n",
       ptr, num_read, num_track_memlocs, track_memlocs[num_track_memlocs-1],
       nextchar);
       */
    ptr += numchars;
    if (num_read < 2 || nextchar == ' ')
      break;
    assert (nextchar==',');
  }
}

/*
void
parse_stacklocs_string(char const *str)
{
  char const *ptr = str;
  num_stacklocs = 0;
  char const *comma;

  do {
    char *buf, *colon;
    comma = strchr(ptr, ',');
    if (comma) {
      buf = (char *)malloc(comma - ptr + 1);
      memcpy(buf, ptr, comma - ptr);
      buf[comma - ptr] = '\0';
    } else {
      buf = (char *)malloc(strlen(ptr) + 1);
      strncpy(buf, ptr, strlen(ptr) + 1);
    }

    colon = strchr(buf, ':');
    ASSERT(colon);
    *colon = '\0';

    stacklocs[num_stacklocs].function = strtoul(buf, NULL, 16);
    stacklocs[num_stacklocs].offset = strtoul(colon + 1, NULL, 16);
    //printf("added 0x%x:0x%x\n", stacklocs[num_stacklocs].function,
    //   stacklocs[num_stacklocs].offset);
    num_stacklocs++;
    ASSERT(num_stacklocs <= stacklocs_size);
    free(buf);
    ptr = comma + 1;
  } while (comma);
}
*/

void
parse_splice_string(char const *str)
{
  char buf[strlen(str)+1];
  strncpy(buf, str, sizeof buf);

  char *ptr = buf;
  num_pslices = 0;

  char *comma;
  do {
    comma = strchr(ptr, ',');
    if (comma) {
      *comma = '\0';
    }

    char *dash = strchr(ptr, '-');
    assert (dash);
    *dash = '\0';

    pslices_start[num_pslices] = strtol(ptr, NULL, 16);
    ptr = dash+1;
    pslices_stop[num_pslices] = strtol(ptr, NULL, 16);
    num_pslices++;
    ptr = comma+1;
  } while (comma);
}


/* log support */
char *logfilename = NULL;
FILE *logfile = NULL;
int loglevel;



/* enable or disable low levels log */
void cpu_set_log(int log_flags)
{
    loglevel = log_flags;

}

void cpu_set_log_filename(const char *filename)
{
  if (logfilename) {
    ASSERT(logfile);
    free(logfilename);
    fclose(logfile);
  }
  logfilename = strdup(filename);
  logfile = fopen(logfilename, "w");
  if (!logfile) {
    perror(logfilename);
    _exit(1);
  }
  setvbuf(logfile, NULL, _IOLBF, 0);
}

void
debug_mem_stats(long long *total_bytes, long long *rss_bytes)
{
  //long value, active_mem_anon;
  //char attr[256], unit[64];
  long total, rss;
  //char chr;
  FILE *fp;
  int ret;

  fp = fopen("/proc/self/statm", "r");
  ret = fscanf(fp, "%ld %ld", &total, &rss);
  ASSERT(ret == 2);
  /*active_mem_anon = -1;
  while ((ret = fscanf(fp, " %s", attr)) != EOF) {
    ASSERT(ret == 1);
    ret = fscanf(fp, " %ld%c", &value, &chr);
    ASSERT(ret == 2);
    if (chr == ' ') {
      ret = fscanf(fp, " %s\n", unit);
      ASSERT(ret == 1);
    } else ASSERT(chr == '\n');
    //printf("attr='%s'\n", attr);
    if (!strcmp(attr, "Active(anon):")) {
      active_mem_anon = value;
    }
  }
  ASSERT(active_mem_anon != -1);
  printf("  active mem: %fGB\n", (float)active_mem_anon/1e+6);*/
  fclose(fp);
  /*printf("  total : %fGB, rss : %fGB\n",
      (float)total * 4096/(1 << 30),
      (float)rss * 4096/(1 << 30));*/
  *total_bytes = (long long)total * 4096;
  *rss_bytes = (long long)rss * 4096;
}

void
print_stack_pointer()
{
  void *p = NULL;
  printf("%s(): %p\n", __func__, &p);
}

char gtbuf[512];

float
get_rss_gb()
{
  long long total_bytes, rss_bytes;
  debug_mem_stats(&total_bytes, &rss_bytes);
  return rss_bytes/1e9;
}
