#ifndef DEBUG_H
#define DEBUG_H

#include <sys/time.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>
//#include "src-defs.h"

/* GCC lets us add "attributes" to functions, function
 *    parameters, etc. to indicate their properties.
 *       See the GCC manual for details. */
#define UNUSED __attribute__ ((unused))
#define NO_RETURN __attribute__ ((noreturn))
#define NO_INLINE __attribute__ ((noinline))
#define PRINTF_FORMAT(FMT, FIRST) __attribute__ ((format (printf, FMT, FIRST)))

#define likely(x)   __builtin_expect(!!(x), 1)

#ifdef offsetof
#undef offsetof
#endif
#define offsetof(type, field) ((size_t) &((type *)0)->field)
#define my_offsetof(type, field) ((size_t) &((type *)0)->field)

//#define DEFAULT_LOGFILE BUILD_DIR_SRC "/rewrite.log"
#define DEFAULT_LOGFILE "rewrite.log"


#ifdef DBG
#undef DBG
#endif

#define NOT_TESTED(x) do { if ((x)) fprintf(stderr, "%s %s() %d: not-tested: %s\n", __FILE__, __func__, __LINE__, #x); } while (0)
#define NOT_IMPLEMENTED() do { fprintf(stderr, "%s() %d: not-implemented\n", __func__, __LINE__); assert(false); } while (0)
#define NOT_REACHED() do { fprintf(stderr, "%s %s() %d: not-reached\n", __FILE__, __func__, __LINE__); ASSERT(0); } while (0)
#define ABORT() do { fprintf(stderr, "%s() %d: abort\n", __func__, __LINE__); ASSERT(0); } while (0)
#define ERR(...)

#define DBG_INIT(l,i) FILE *l##_fp = NULL; char l##_flag = i;
#define DBG_SET(l,i) do { extern char l##_flag; l##_flag = i; } while (0)

#define DBG(...) DBGn(1,__VA_ARGS__)
#define DBG2(...) DBGn(2,__VA_ARGS__)
#define DBG3(...) DBGn(3,__VA_ARGS__)
#define DBG4(...) DBGn(4,__VA_ARGS__)

#define _DBG(...) _DBGn(1,__VA_ARGS__)

#define CPP_DBG(...) CPP_DBGn(1,__VA_ARGS__)
#define CPP_DBG2(...) CPP_DBGn(2,__VA_ARGS__)
#define CPP_DBG3(...) CPP_DBGn(3,__VA_ARGS__)
#define CPP_DBG4(...) CPP_DBGn(4,__VA_ARGS__)

/*#define STATS() do { \
  printf("%s() %d: printing stats\n", __func__, __LINE__); \
  debug_print_stats(); \
} while(0)*/


#define DBG_EXEC(...) DBG_EXECn(1,__VA_ARGS__)
#define DBG_EXEC2(...) DBG_EXECn(2,__VA_ARGS__)
#define DBG_EXEC3(...) DBG_EXECn(3,__VA_ARGS__)
#define DBG_EXEC4(...) DBG_EXECn(4,__VA_ARGS__)

#define CPP_DBG_EXEC(...) CPP_DBG_EXECn(1,__VA_ARGS__)
#define CPP_DBG_EXEC2(...) CPP_DBG_EXECn(2,__VA_ARGS__)
#define CPP_DBG_EXEC3(...) CPP_DBG_EXECn(3,__VA_ARGS__)
#define CPP_DBG_EXEC4(...) CPP_DBG_EXECn(4,__VA_ARGS__)



#define STATS(x) do { x } while (0)


#ifndef NDEBUG

#define ASSERT assert
#define ASSERTCHECK(c, ...) do { if (!c) { __VA_ARGS__; assert(c); } } while (0)
//#define ASSERT2 assert
#define ASSERT2(...)

/* for generation of gprof's gmon.out data, need proper exit. */
//#define ASSERT(x) do { if (!(x)) { printf("%s %d: Assertion '%s' failed.\n", __FILE__, __LINE__, #x); exit(1); }} while (0)
#define FFLUSH fflush
#define DBGn(n,l,x,...) do { /*extern FILE *l##_fp; */extern char l##_flag; if (l##_flag<0) {printf("%s() %d: " x, __func__, __LINE__, __VA_ARGS__);} else if (l##_flag>=n) { /*if (!l##_fp) { l##_fp = fopen("/tmp/"#l, "w"); assert(l##_fp); }*/ fprintf(stdout, "%s() %d: " x, __func__, __LINE__, __VA_ARGS__); fflush(stdout);}} while (0)

#define CPP_DBGn(n,l,x,...) do { if (::l##_flag<0) {printf("%s() %d: " x, __func__, __LINE__, __VA_ARGS__);} else if (::l##_flag>=n) { /*if (!l##_fp) { l##_fp = fopen("/tmp/"#l, "w"); assert(l##_fp); }*/ fprintf(stdout, "%s() %d: " x, __func__, __LINE__, __VA_ARGS__); fflush(stdout);}} while (0)

#define _DBGn(n,l,...) do { extern FILE *l##_fp; extern char l##_flag; if (l##_flag<0) {printf(__VA_ARGS__);} else if (l##_flag>=n) { if (!l##_fp) { l##_fp = fopen("/tmp/"#l, "w"); assert(l##_fp); } fprintf(stdout, __VA_ARGS__); }} while (0)

#define DBG_EXECn(n,l,x) do { extern char l##_flag; if (l##_flag<0 || l##_flag>=n) { x; fflush(stdout); } } while (0)
#define CPP_DBG_EXECn(n,l,x) do { if (::l##_flag<0 || ::l##_flag>=n) { x; fflush(stdout); } } while (0)
#else
#define ASSERT(...)
#define ASSERTCHECK(...)
#define FFLUSH(...)
#define DBGn(...)
#define _DBGn(...)
#define DBG_EXECn(...)
#define CPP_DBG_EXECn(...)
#endif

#define MSG(s) do { printf("MSG: %s\n", s); } while (0)

extern char progress_flag;

extern char gtbuf[512];
extern struct timeval start_time;
static inline char *get_timestamp(char *buf, size_t buf_size)
{
  struct timeval tv;
  int print_progress_ret;
  print_progress_ret = gettimeofday(&tv, NULL);
  ASSERT(print_progress_ret == 0);
  if (!start_time.tv_sec && !start_time.tv_usec) {
    start_time.tv_sec = tv.tv_sec;
    start_time.tv_usec = tv.tv_usec;
  }
  snprintf(buf, buf_size, "%ld:%02ld ",
      (long int)((tv.tv_sec - start_time.tv_sec)/60),
      (tv.tv_sec - start_time.tv_sec)%60);
  return buf;
}

static inline char* timestamp()
{
  static char buf[4096];
  return get_timestamp(buf, sizeof(buf));
}

#define PRINT_PROGRESS(f, ...) do { \
  if (progress_flag) { \
    long long total_bytes, rss_bytes; \
    struct timeval tv; \
    int __print_progress_ret; \
    __print_progress_ret = gettimeofday(&tv, NULL); \
    ASSERT(__print_progress_ret == 0); \
    if (!start_time.tv_sec && !start_time.tv_usec) { \
      start_time.tv_sec = tv.tv_sec; \
      start_time.tv_usec = tv.tv_usec; \
    } \
    debug_mem_stats(&total_bytes, &rss_bytes); \
    fprintf(stderr, "%ld:%02ld :: mem tot %.3fG rss %.3fG :: num_cg_constructions %lld num_cg_destructions %lld active %lld :: aliasing_constraints constructions %lld destructions %lld active %lld :: memlabel_assertions constructions %lld destructions %lld active %lld :: invariant_state constructions %lld destructions %lld active %lld :: invariant_state_eqclass constructions %lld destructions %lld active %lld :: predicate constructions %lld destructions %lld active %lld :: point_set constructions %lld destructions %lld active %lld :: point_val constructions %lld destructions %lld active %lld :: array_constant constructions %lld destructions %lld active %lld :: counter_example_t constructions %lld destructions %lld active %lld :: graphce_t constructions %lld destructions %lld active %lld :: " f, \
        (long int)((tv.tv_sec-start_time.tv_sec)/60), \
        (tv.tv_sec-start_time.tv_sec)%60, (float)total_bytes/1e+9, \
        (float)rss_bytes/1e+9, stats_num_cg_constructions, stats_num_cg_destructions, stats_num_cg_constructions - stats_num_cg_destructions, \
        stats_num_aliasing_constraint_constructions, stats_num_aliasing_constraint_destructions, stats_num_aliasing_constraint_constructions - stats_num_aliasing_constraint_destructions, \
        stats_num_memlabel_assertions_constructions, stats_num_memlabel_assertions_destructions, stats_num_memlabel_assertions_constructions - stats_num_memlabel_assertions_destructions, \
        stats_num_invariant_state_constructions, stats_num_invariant_state_destructions, stats_num_invariant_state_constructions - stats_num_invariant_state_destructions, \
        stats_num_invariant_state_eqclass_constructions, stats_num_invariant_state_eqclass_destructions, stats_num_invariant_state_eqclass_constructions - stats_num_invariant_state_eqclass_destructions, \
        stats_num_predicate_constructions, stats_num_predicate_destructions, stats_num_predicate_constructions - stats_num_predicate_destructions, \
        stats_num_point_set_constructions, stats_num_point_set_destructions, stats_num_point_set_constructions - stats_num_point_set_destructions, \
        stats_num_point_val_constructions, stats_num_point_val_destructions, stats_num_point_val_constructions - stats_num_point_val_destructions, \
        stats_num_array_constant_constructions, stats_num_array_constant_destructions, stats_num_array_constant_constructions - stats_num_array_constant_destructions, \
        stats_num_counter_example_constructions, stats_num_counter_example_destructions, stats_num_counter_example_constructions - stats_num_counter_example_destructions, \
        stats_num_graphce_constructions, stats_num_graphce_destructions, stats_num_graphce_constructions - stats_num_graphce_destructions, \
        __VA_ARGS__); \
    stats::get().print_manager_sizes(cerr); \
  } \
} while (0)

#define PROGRESS(f, ...) do { \
  if (progress_flag) { \
    fprintf (stderr, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"\
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
				"                                                                  " \
        "              " \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"); \
     PRINT_PROGRESS(f, __VA_ARGS__); \
  } \
} while (0)
#define PROGRESS_CPP(f, ...) do { } while(0)
/*#define PROGRESS_CPP(f, ...) do { \
  if (progress_flag) { \
    long long total_bytes, rss_bytes; \
    struct timeval tv; \
    int __progress_ret; \
    __progress_ret = gettimeofday(&tv, NULL); \
    ASSERT(__progress_ret == 0); \
    if (!start_time.tv_sec && !start_time.tv_usec) { \
      start_time.tv_sec = tv.tv_sec; \
      start_time.tv_usec = tv.tv_usec; \
    } \
    debug_mem_stats(&total_bytes, &rss_bytes); \
    fprintf (stderr, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"\
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
				"                                                                  " \
        "              " \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" \
        "%ld:%02ld :: mem tot %.2fG rss %.2fG :: " f, \
        (long int)((tv.tv_sec-start_time.tv_sec)/60), \
        (tv.tv_sec-start_time.tv_sec)%60, (float)total_bytes/1e+9, \
        (float)rss_bytes/1e+9, __VA_ARGS__); \
  } \
} while (0)*/


#define dbg(x,...) do { printf("%s() %d: " x, __func__, __LINE__, __VA_ARGS__); } while (0);
#define err(x,...) do { fprintf(stdout, "%s() %d: " x, __func__, __LINE__, __VA_ARGS__); fprintf(stderr, "%s() %d: " x, __func__, __LINE__, __VA_ARGS__); } while (0)

#define TIME(x,...) do { \
  if (track_time) { \
    struct timeval tv; \
    if (!gettimeofday(&tv, NULL)) { \
      if (!start_time.tv_sec && !start_time.tv_usec) \
      { \
	start_time.tv_sec = tv.tv_sec; \
	start_time.tv_usec = tv.tv_usec; \
      } \
      printf("%ld:%02ld :: " x, (long int)((tv.tv_sec-start_time.tv_sec)/60), \
	  (tv.tv_sec-start_time.tv_sec)%60, __VA_ARGS__); \
    } \
  } \
} while (0)

#define PRINT_BACKTRACE() do { \
  void *array[64]; \
  size_t size; \
  size = backtrace(array, 64); \
  printf("%s() %d: printing backtrace\n", __func__, __LINE__); \
  backtrace_symbols_fd(array, size, STDOUT_FILENO); \
} while(0)

//#define TIME(x,...)

//#define TMP 1

#include <stdint.h>
struct transmap_t;
struct bbl_t;
struct regset_t;
struct memset_t;

extern int dump_full_state;
extern int track_time;
extern bool dyn_debug;
//extern int use_fast_mode;
extern int mark_peep_translations;
extern uint32_t track_memlocs[];
extern int num_track_memlocs;
extern int prune_row_size;
extern char const *fix_transmaps_logfile;

extern int num_pslices;
extern unsigned long int pslices_start[];
extern unsigned long int pslices_stop[];

extern char const *speed_functions[];
extern int num_speed_functions;

struct translation_instance_t;

void parse_splice_string(char const *str);
bool belongs_to_peep_program_slice(unsigned pc);
int print_debug_info_in(unsigned pc);
int print_debug_info_out(unsigned pc);

void read_track_memlocs (char const *str);
char *track_memlocs_to_string(char *buf, size_t size);

size_t dump_regs(uint8_t *buf, size_t buf_size, struct transmap_t const *tmap,
    struct regset_t const *live_in);
size_t restore_regs(uint8_t *buf, size_t buf_size,struct transmap_t const *tmap,
    struct regset_t const *live_in);
int call_print_machine_state(struct translation_instance_t const *ti,
    struct bbl_t *tb, uint8_t *buf, size_t buf_size, unsigned nip,
    int len, struct memset_t const *memtouch, struct regset_t const *live_regs);
size_t dyn_debug_before_setup_on_startup(struct translation_instance_t *ti, uint8_t *buf,
    size_t size);
size_t dyn_debug_after_setup_on_startup(struct translation_instance_t *ti, uint8_t *buf,
    size_t size, char const *filename);
size_t dyn_debug_print_machine_state_if_required(uint8_t *buf, size_t size,
    struct translation_instance_t const *ti, struct bbl_t *tb, int n,
    int len, struct transmap_t const *in_tmap);
void debug_mem_stats(long long *total, long long *rss);
void cpu_set_log(int log_flags);
void cpu_set_log_filename(const char *filename);
void parse_stacklocs_string(char const *str);
void print_stack_pointer();
float get_rss_gb();

#define ENUM_MAX_LEN 20
//extern long long stats_num_fingerprinted_dst_sequences;
extern long long stats_num_fingerprint_tests[ENUM_MAX_LEN];
extern long long stats_num_fingerprint_type_failures[ENUM_MAX_LEN][ENUM_MAX_LEN];
extern long long stats_num_fingerprint_typecheck_success[ENUM_MAX_LEN];
extern long long stats_num_syntactic_match_tests;
extern long long stats_num_execution_tests;
extern long long stats_num_boolean_tests;
extern long long stats_num_cg_constructions, stats_num_cg_destructions;
extern long long stats_num_aliasing_constraint_constructions, stats_num_aliasing_constraint_destructions;
extern long long stats_num_memlabel_assertions_constructions, stats_num_memlabel_assertions_destructions;
extern long long stats_num_invariant_state_constructions, stats_num_invariant_state_destructions;
extern long long stats_num_invariant_state_eqclass_constructions, stats_num_invariant_state_eqclass_destructions;
extern long long stats_num_predicate_constructions, stats_num_predicate_destructions;
extern long long stats_num_point_set_constructions, stats_num_point_set_destructions;
extern long long stats_num_point_val_constructions, stats_num_point_val_destructions;
extern long long stats_num_array_constant_constructions, stats_num_array_constant_destructions;
extern long long stats_num_counter_example_constructions, stats_num_counter_example_destructions;
extern long long stats_num_graphce_constructions, stats_num_graphce_destructions;

extern char *logfilename;
extern int loglevel;

extern bool assertcheck_si_init_done;
extern bool peepgen;

#define DBG_DECL(l) extern char l##_flag

DBG_DECL(GENERAL);
DBG_DECL(IS_EXPR_EQUAL_SYNTACTIC);
DBG_DECL(ANNOTATE_LOCALS);
DBG_DECL(IS_OVERLAPPING);
DBG_DECL(IS_CONTAINED_IN);
DBG_DECL(TFG_LOCS);
DBG_DECL(EXPR_SIMPLIFY);
DBG_DECL(TFG_SIMPLIFY);
DBG_DECL(GET_ADDR_SYMBOL);
DBG_DECL(REMOVE_ITE);
DBG_DECL(EXPR_FIXED_POINT);
DBG_DECL(EXPR_SIMPLIFY_SELECT);
//DBG_DECL(LINEAR_RELATIONS);
DBG_DECL(ADDR_REFS);
DBG_DECL(SPLIT_MEM);
DBG_DECL(PEEP_TAB);
DBG_DECL(PEEP_SEARCH);
//DBG_DECL(ENUM_TMAPS);
DBG_DECL(SYM_EXEC);
DBG_DECL(SUBSTITUTE_USING_SPRELS);
//DBG_DECL(LINEAR_RELATIONS);
DBG_DECL(EXPR_SIZE_DURING_SIMPLIFY);
DBG_DECL(ASSERTCHECKS);
DBG_DECL(SP_RELATIONS);
DBG_DECL(CONSTPROP);
DBG_DECL(TFG_LOCS);
DBG_DECL(EQGEN);
DBG_DECL(ALIAS_ANALYSIS);
DBG_DECL(EQCHECK);
DBG_DECL(DWARF2);
//DBG_DECL(HOUDINI);
DBG_DECL(DWARF_EXPR_EVAL);
DBG_DECL(SRC_USEDEF);
DBG_DECL(STAT_TRANS);
DBG_DECL(WRN);
DBG_DECL(TMP);
DBG_DECL(ERR);
DBG_DECL(INFER_INVARIANTS);
DBG_DECL(BV_SOLVE);
DBG_DECL(SAGE_QUERY);
DBG_DECL(DEBUG_ONLY); // for use in GDB; Protip: in GDB, set debug flags using `set <flagname>_flag = 1` e.g. `set DEBUG_ONLY_flag = 1`
DBG_DECL(STATS);
DBG_DECL(INSN_MATCH);
DBG_DECL(CMN_ISEQ_FROM_STRING);
DBG_DECL(LLVM2TFG);
DBG_DECL(LLVM_PASSMANAGER);
DBG_DECL(LLVM_LIVENESS);
DBG_DECL(QUERY_DECOMPOSE);
DBG_DECL(FB);
DBG_DECL(COUNT_STACK_SLOTS);
//DBG_DECL(TMAP_TRANS);
DBG_DECL(TMAPS_ACQUIESCE);
DBG_DECL(REGCONST);
DBG_DECL(SMT_HELPER_PROCESS);
DBG_DECL(PEEPTAB_VERIFY);
DBG_DECL(STRICT_CANON);
DBG_DECL(TYPESTATE);
DBG_DECL(TFG_GET_TYPE_CONSTRAINTS);
DBG_DECL(INFER_CONS);
DBG_DECL(ISEQ_GET_TFG);
//DBG_DECL(GEN_SANDBOXED_ISEQ);
DBG_DECL(EQUIV);
DBG_DECL(SUPEROPTDB);
DBG_DECL(JTABLE);
DBG_DECL(MALLOC32);
DBG_DECL(COMPUTE_LIVENESS);
DBG_DECL(BEQUIV);
DBG_DECL(CONNECT_ENDPOINTS);
DBG_DECL(TFG_GET_USEDEF);
DBG_DECL(ENUMERATE_IN_OUT_TMAPS);
DBG_DECL(RELOC_STRINGS);
DBG_DECL(ITABLE);
DBG_DECL(CFG_MAKE_REDUCIBLE);
DBG_DECL(STACK_SLOT_CONSTRAINTS);
DBG_DECL(SETUP_CODE);
DBG_DECL(PRUNE);
DBG_DECL(SRC_ISEQ_FETCH);
DBG_DECL(GRAPH_COLORING);
DBG_DECL(DST_INSN_FOLDING);
DBG_DECL(DFA);
DBG_DECL(SUFFIXPATH_DFA);
DBG_DECL(DST_INSN_SCHEDULE);
DBG_DECL(TI_PREDS);
//DBG_DECL(COPY_PROP);
DBG_DECL(TAIL_CALL);
DBG_DECL(OPEN_DIAMOND);
DBG_DECL(DST_ISEQ_GET_CFG);
DBG_DECL(POPULATE_ESTIMATED_FLOW);
DBG_DECL(DST_INSN_DEAD_CODE_ELIM);
DBG_DECL(CG_IS_WELL_FORMED);
DBG_DECL(CONCRETE_VALUES_SET);
DBG_DECL(INVARIANT_STATE);
DBG_DECL(STABILITY);
DBG_DECL(PRED_AVAIL_EXPR);
DBG_DECL(BV_SOLVE_QUERY);
DBG_DECL(UPDATE_DST_FCALL_EDGE);
DBG_DECL(CE_ADD);
DBG_DECL(CE_FUZZER);
DBG_DECL(CE_RANDOM_GEN);
DBG_DECL(UNALIASED_MEMSLOT);
DBG_DECL(UNALIASED_BVEXTRACT);
DBG_DECL(POINT_SET);
DBG_DECL(ALIAS_CONS);
DBG_DECL(CE_MEMLABELS);
DBG_DECL(EXPR_ID_DETERMINISM);
DBG_DECL(SSA_VARNAME_DEFINEDNESS);
DBG_DECL(ADD_POINT_USING_CE);
DBG_DECL(EQCLASS_INEQ);
DBG_DECL(GRAPH_WITH_PATHS_CACHE_ASSERTS);
DBG_DECL(EQCLASS_BV);
DBG_DECL(EQCLASS_ARR);
DBG_DECL(EQCLASS_HOUDINI);
DBG_DECL(BV_SOLVE_VERT_DEDUP);
DBG_DECL(SAFETY);
DBG_DECL(SRC_INSN);
DBG_DECL(INSTRUMENT_MARKER_CALL);
DBG_DECL(INVARIANTS_MAP);
//DBG_DECL(EXPR_EQCLASSES);
DBG_DECL(CFG);
DBG_DECL(SYMBOLS);
DBG_DECL(NEXTPC_MAP);
DBG_DECL(HARVEST);
DBG_DECL(READ_ASM_FILE);
DBG_DECL(MEMLABEL_ASSERTIONS);
DBG_DECL(REMOVE_MARKER_CALL);
DBG_DECL(ARGV_PRINT);
DBG_DECL(ETFG_INSN_FETCH);
DBG_DECL(RAND_STATES);
DBG_DECL(TYPECHECK);
DBG_DECL(CORRELATE);
DBG_DECL(DYN_DEBUG);
DBG_DECL(ETFG_ISEQ_GET_TFG);
DBG_DECL(LOCKSTEP);

#define DEBUG_TMAP_CHOICE_LOGIC 1

#endif
