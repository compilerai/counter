#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

#include "support/debug.h"
#include "support/dyn_debug.h"
#include "config-host.h"

#include "insn/jumptable.h"
//#include "imm_map.h"
#include "i386/disas.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "valtag/ldst_input.h"
#include "insn/cfg.h"
#include "rewrite/tagline.h"
#include "expr/z3_solver.h"
#include "cutils/imap_elem.h"
#include "valtag/elf/elf.h"

#include "i386/opctable.h"
#include "support/c_utils.h"
#include "support/utils.h"
#include "cutils/pc_elem.h"

#include "ppc/regs.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "insn/rdefs.h"
#include "valtag/memset.h"
#include "support/globals.h"
#include "ppc/insn.h"
#include "insn/src-insn.h"

#include "insn/edge_table.h"
#include "support/timers.h"

#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"
#include "rewrite/static_translate.h"
//#include "tfg/annotate_locals_using_dwarf.h"
#include "valtag/transmap.h"
#include "rewrite/peephole.h"
#include "superopt/harvest.h"
#include "support/mytimer.h"
#include "eq/parse_input_eq_file.h"
#include "valtag/nextpc.h"
#include "support/cl.h"

static char as1[409600];
static char as2[1024];
static char as3[1024];

//struct locals_info_t;

using namespace eqspace;

static void
usage(void)
{
  printf("harvest:\n"
      "usage: harvest [-live_all] program1 [program2 ...] harvest.out\n"
      "\n"
      "-live_all      enumerate all subsets of touched registers as live\n"
      "\n");
}

/*typedef struct add_to_harvested_sequences_struct {
  bool live_all;
  bool live_callee_save;
  //bool add_transmap_identity;
  //bool canonicalize_imms_using_symbols;
  bool trunc_ret;
} add_to_harvested_sequences_struct;*/


static void
print_rodata_ptr_content(FILE *fp, input_exec_t const *in, long val)
{
  input_section_t const *rodata;
  char const *ptr, *start;

  rodata = input_exec_get_rodata_section(in);
  start = input_section_get_ptr_from_addr(rodata, val);
  ptr = start;
  while (/*   (*ptr >= 'A' && *ptr <= 'Z')
         || (*ptr >= 'a' && *ptr <= 'z')
         || (*ptr >= '0' && *ptr <= '9')
         || (*ptr == ' ')
         || (*ptr == '\n')
         || (*ptr == '!')
         || (*ptr == '@')
         || (*ptr == '#')
	 || (*ptr == '$')
         || (*ptr == '%')
         || (*ptr == '^')
         || (*ptr == '&')
         || (*ptr == '*')
         || (*ptr == '(')
         || (*ptr == ')')
         || (*ptr == '{')
         || (*ptr == '}')
         || (*ptr == '[')
         || (*ptr == ']')
         || (*ptr == '.')
         || (*ptr == ',')
         || (*ptr == '/')
         || (*ptr == '+')
         || (*ptr == '-')
         || (*ptr == '=')
         || (*ptr == '_')
         || (*ptr == '\"')
         || (*ptr == ':')
         || (*ptr == ';')
         || (*ptr == '!')*/
         *ptr >= 32 && *ptr <= 126) {
    ASSERT(*ptr != '\0');
    fprintf(fp, "%c", *ptr);
    ptr++;
    ASSERT(ptr <= &rodata->data[rodata->size]);
    if (ptr == &rodata->data[rodata->size]) {
      break;
    }
  }
  if (ptr == start) {
    fprintf(fp, "%hhx", ptr[0]);
  }
  fprintf(fp, "\n");
}

static void
print_rodata_ptrs(FILE *fp, input_exec_t const *in)
{
  long i;
  fprintf(fp, "rodata_ptrs:\n");
  for (i = 0; i < in->num_rodata_ptrs; i++) {
    fprintf(fp, "RODATA.%lx:", (long)in->rodata_ptrs[i]);
    print_rodata_ptr_content(fp, in, in->rodata_ptrs[i]);
  }
}

void
test_fork()
{
  printf("testing fork\n"); fflush(stdout);
  pid_t pid = fork();
  if (pid == 0) {
    exit(0);
  } else {
    pid_t w = wait(0);
    ASSERT(w == pid);
  }
  printf("testing fork done\n");
}

static void
si_iseq_add_symbols_using_src_insn(si_iseq_t *sii, src_ulong insn_pc,
    src_insn_t const *insn)
{
  long i;
  for (i = 0; i < sii->iseq_len; i++) {
    if (sii->pcs[i] != insn_pc) {
      continue;
    }
    src_insn_copy_symbols(&sii->iseq[i], insn);
  }
}

static void
input_exec_add_symbols_using_src_insn(input_exec_t *in, src_ulong insn_pc,
    src_insn_t const *insn)
{
  long i, j;
  for (i = 0; i < (long)in->num_si; i++) {
    if (in->si[i].pc == insn_pc) {
      src_insn_copy_symbols(in->si[i].fetched_insn, insn);
      DBG(ANNOTATE_LOCALS, "after src_insn_copy_symbols, fetched_insn: %s\n", src_insn_to_string(in->si[i].fetched_insn, as1, sizeof as1));
    }
    si_iseq_add_symbols_using_src_insn(&in->si[i].max_first, insn_pc, insn);
    si_iseq_add_symbols_using_src_insn(&in->si[i].max_middle, insn_pc, insn);
    for (j = 0; j < SI_NUM_CACHED; j++) {
      si_iseq_add_symbols_using_src_insn(&in->si[i].cached[j], insn_pc, insn);
    }
  }
}

static void
input_exec_add_symbols_using_src_iseq(input_exec_t *in, src_ulong const *insn_pcs,
    src_insn_t const *src_iseq_annot, src_insn_t const *src_iseq, long src_iseq_len/*, locals_info_t *locals_info*/)
{
  long i;
  for (i = 0; i < src_iseq_len; i++) {
    if (src_insns_equal(&src_iseq_annot[i], &src_iseq[i])) {
      continue;
    }
    input_exec_add_symbols_using_src_insn(in, insn_pcs[i], &src_iseq_annot[i]);
  }
  /*if (locals_info) {
    input_exec_add_locals_info(in, insn_pcs, src_iseq_len, locals_info);
  }*/
}



static void
identify_symbols_syntactically(src_insn_t *src_iseq, long src_iseq_len, input_exec_t const *in, src_ulong const *insn_pcs/*, long const *allowed_symbol_ids, ssize_t num_allowed_symbol_ids, struct context *ctx, sym_exec const &se*/)
{
  long i;
  for (i = 0; i < src_iseq_len; i++) {
    DBG(SYMBOLS, "\nbefore:\n%s\n", src_insn_to_string(&src_iseq[i], as1, sizeof as1));
    src_insn_rename_addresses_to_symbols(&src_iseq[i], in, insn_pcs[i]/*allowed_symbol_ids, num_allowed_symbol_ids*/);
    DBG(SYMBOLS, "\nafter:\n%s\n", src_insn_to_string(&src_iseq[i], as1, sizeof as1));
  }
}

static void
input_file_annotate_symbols(input_exec_t *in,
    char const *function_name)
{
  src_insn_t *src_iseq, *src_iseq_annot;
  long num_nextpcs, num_lastpcs, src_iseq_len;
  src_ulong /**nextpcs, */*lastpcs, *insn_pcs;
  imm_vt_map_t imm_vt_map;
  //bool annotated_insn_line_map;
  nextpc_t *nextpcs;
  regset_t *live_out;
  double *lastpc_weights;
  long index, max_alloc;
  src_ulong pc;
  bool fetch;

  //printf("%s() %d:\n", __func__, __LINE__);

  //cout << __func__ << " " << __LINE__ << endl;
  //annotated_insn_line_map = false;
  //DISABLE_FUNCTION_CALL = false;

  stopwatch_run(&input_file_annotate_symbols_timer);
  /*if (!init) {
    ASSERT(g_ctx);
    cs.parse_consts_db(g_ctx);
    PROGRESS_CPP("%s() %d: after parse_consts_db\n", __func__, __LINE__);
    se.src_parse_sym_exec_db(g_ctx);
    PROGRESS_CPP("%s() %d: after src_parse_sym_exec_db\n", __func__, __LINE__);
    se.set_consts_struct(cs);
    init = true;
  }*/

  max_alloc = 2 * (in->num_si + 256);
  //nextpcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  //ASSERT(nextpcs);
  nextpcs = new nextpc_t[max_alloc];
  ASSERT(nextpcs);
  lastpcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  ASSERT(lastpcs);
  insn_pcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  ASSERT(insn_pcs);
  //live_out = (regset_t *)malloc(max_alloc * sizeof(regset_t));
  live_out = new regset_t[max_alloc];
  ASSERT(live_out);
  //src_iseq = (src_insn_t *)malloc(max_alloc * sizeof(src_insn_t));
  src_iseq = new src_insn_t[max_alloc];
  ASSERT(src_iseq);
  //src_iseq_annot = (src_insn_t *)malloc(max_alloc * sizeof(src_insn_t));
  src_iseq_annot = new src_insn_t[max_alloc];
  ASSERT(src_iseq_annot);
  lastpc_weights = (double *)malloc(max_alloc * sizeof(double));
  ASSERT(lastpc_weights);

  //printf("%s() %d: in->num_si = %zd\n", __func__, __LINE__, (size_t)in->num_si);
  int num_functions = 0;
  for (index = 0; index < (long)in->num_si; index++) {
    if (!pc2function_name(in, in->si[index].pc)) {
      CPP_DBG_EXEC(HARVEST, printf("%s() %d: pc2function name returned false for pc %ld\n", __func__, __LINE__, (long)in->si[index].pc));
      continue;
    }
    pc = in->si[index].pc;
    fetch = src_iseq_fetch(src_iseq, max_alloc, &src_iseq_len, in, NULL,
        pc, LONG_MAX, &num_nextpcs, nextpcs, insn_pcs, &num_lastpcs, lastpcs,
        lastpc_weights, true);
    if (strcmp(function_name, "ALL") && strcmp(function_name, lookup_symbol(in, pc))) {
      continue;
    }
    if (!fetch) {
      printf("Warning: %s(): function at %lx (%s) was not harvested.\n", __func__,
          (long)pc, lookup_symbol(in, pc));
      continue;
    }
    /*if (num_functions >= 30) {
      continue;
    }*/
    num_functions++;

    DBG(ANNOTATE_LOCALS, "annotating %s\n", lookup_symbol(in, pc));
    //dwarf_map_t dwarf_map;
    //cspace::locals_info_t *locals_info = cspace::locals_info_new((cspace::context *)g_ctx);
    //cspace::locals_info_t *locals_info = cspace::locals_info_new_using_g_ctx();
    PROGRESS_CPP("%s() %d: annotating %s with %ld instructions\n", __func__, __LINE__, lookup_symbol(in, pc), src_iseq_len);
    /*if (filename) {
      ASSERT(types_filename);
      DBG2(ANNOTATE_LOCALS, "calling dwarf_map_read for function %s.\n", lookup_symbol(in, pc));
      dwarf_map.dwarf_map_read(filename, types_filename, pc, insn_pcs, src_iseq_len, g_ctx, se);
      PROGRESS_CPP("%s() %d: after dwarf_map_read\n", __func__, __LINE__);
    }*/
    DBG(ANNOTATE_LOCALS, "function %s\n", lookup_symbol(in, pc));
    DBG(ANNOTATE_LOCALS, "src_iseq:\n%s\n", src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
    src_iseq_canonicalize_symbols(src_iseq, src_iseq_len, &imm_vt_map);
    PROGRESS_CPP("%s() %d: after canonicalize_symbols\n", __func__, __LINE__);
    src_iseq_copy(src_iseq_annot, src_iseq, src_iseq_len);
    PROGRESS_CPP("%s() %d: after src_iseq_copy\n", __func__, __LINE__);

    //cout << __func__ << " " << __LINE__ << endl;
    DBG(SYMBOLS, "Calling identify_symbols_syntactically:\n%s\n", src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
    identify_symbols_syntactically(src_iseq_annot, src_iseq_len, in, insn_pcs/*, g_ctx, se*/);
    PROGRESS_CPP("%s() %d: after identify_symbols_syntactically\n", __func__, __LINE__);
    DBG(SYMBOLS, "after Calling identify_symbols_syntactically:\n%s\n", src_iseq_to_string(src_iseq_annot, src_iseq_len, as1, sizeof as1));

    /*if (filename) {
      //function should be renamed src_iseq_annotate_locals()
      src_iseq_annotate_locals(src_iseq_annot, src_iseq_len, in, &dwarf_map, g_ctx, se, locals_info);
      PROGRESS_CPP("%s() %d: after src_iseq_annotate_locals\n", __func__, __LINE__);
    }*/

/*
#define MAX_NUM_SYMBOL_IDS_PER_FUNCTION 256
    ssize_t num_allowed_symbol_ids;
    long allowed_symbol_ids[MAX_NUM_SYMBOL_IDS_PER_FUNCTION];

    num_allowed_symbol_ids = get_allowed_symbol_ids(in, function_name, input_symbols_filename, allowed_symbol_ids, MAX_NUM_SYMBOL_IDS_PER_FUNCTION);
*/

    /*if (output_symbols_filename) {
      output_symbols_to_file_for_function(in, src_iseq_annot, src_iseq_len, function_name, output_symbols_filename);
    }*/

    /*DBG_EXEC(SYMBOLS, 
      locals_info_to_string(locals_info, as1, sizeof as1);
      printf("locals_info:\n%s\n", as1);
    );*/
    DBG(SYMBOLS, "after src_iseq_annotate_symbols: src_iseq:\n%s\n",
        src_iseq_to_string(src_iseq_annot, src_iseq_len, as1, sizeof as1));

    input_exec_add_symbols_using_src_iseq(in, insn_pcs,
        src_iseq_annot, src_iseq, src_iseq_len/*, locals_info*/);

    /*if (filename) {
      if (!annotated_insn_line_map) {
        input_exec_annotate_src_line_numbers_using_dwarf_map(in, &dwarf_map);
        annotated_insn_line_map = true;
      }
    }
    DBG_EXEC(SYMBOLS,
      fetch = src_iseq_fetch(src_iseq, &src_iseq_len, in, NULL,
          pc, LONG_MAX, &num_nextpcs, nextpcs, insn_pcs, &num_lastpcs, lastpcs,
          lastpc_weights, true);
      printf("on refetching src_iseq at %lx:\n%s\n", (long)pc, src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
      locals_info_to_string(locals_info, as1, sizeof as1);
      printf("locals_info:\n%s\n", as1);
    );*/
  }
  //free(nextpcs);
  delete[] nextpcs;
  free(lastpcs);
  free(insn_pcs);
  //free(live_out);
  delete [] live_out;
  //free(src_iseq);
  delete [] src_iseq;
  //free(src_iseq_annot);
  delete [] src_iseq_annot;
  free(lastpc_weights);
  stopwatch_stop(&input_file_annotate_symbols_timer);

  //printf("%s(): Printing context solver stats:\n", __func__);
  //g_ctx->get_solver()->print_stats();
}

static void
harvest_input_filename(char const *filename, int len, char const *log_filename, context *ctx, char const *ddsum, bool functions_only, char const *function_name/*, bool touch_callee_saves*/, bool fcall_convention64, bool mem_live_out, bool canonicalize_imms, bool live_all, bool live_callee_save, bool trunc_ret/*, bool sort_increasing, bool sort_by_occurrences*/, bool eliminate_duplicates, bool use_orig_regnames, string const& harvest_output_filename)
{
  //cout << __func__ << " " << __LINE__ << "\n";
  input_exec_t *in;
  in = new input_exec_t;
  ASSERT(in);
  cpu_set_log_filename(log_filename);
  //PRINT_PROGRESS("%s() %d: before read_input_file\n", __func__, __LINE__);
#if ARCH_SRC == ARCH_ETFG
  map<string, dshared_ptr<tfg>> function_tfg_map = get_function_tfg_map_from_etfg_file(filename, ctx);
  etfg_read_input_file(in, function_tfg_map);
#else
  read_input_file(in, filename);
#endif
  //PRINT_PROGRESS("%s() %d: after read_input_file\n", __func__, __LINE__);
  if (ddsum) {
    input_file_annotate_using_ddsum(in, ddsum);
    //PRINT_PROGRESS("%s() %d: after input_file_annotate_using_ddsum\n", __func__, __LINE__);
  }
  init_input_file(in, function_name);
  //PRINT_PROGRESS("%s() %d: after init_input_file\n", __func__, __LINE__);
  /*if (function_name) */{
    input_file_annotate_symbols(in, function_name);
    //PRINT_PROGRESS("%s() %d: after input_file_annotate_locals_and_symbols\n", __func__, __LINE__);
  }

  //PRINT_PROGRESS("%s() %d: before harvest_input_exec\n", __func__, __LINE__);
  //cout << __func__ << " " << __LINE__ << "\n";
  harvest_input_exec(in, len, functions_only, function_name,// touch_callee_saves,
      fcall_convention64, mem_live_out, canonicalize_imms, live_all, live_callee_save, trunc_ret,
      //sort_increasing, sort_by_occurrences,
      eliminate_duplicates/*,
      &add_to_harvested_sequences, aths_struct*/, use_orig_regnames, harvest_output_filename);
  //PRINT_PROGRESS("%s() %d: after harvest_input_exec\n", __func__, __LINE__);

  /*for (i =0; i < in.num_input_sections; i++) {
    if (in.input_sections[i].flags & SHF_EXECINSTR) {
      PROGRESS("Harvesting %s section %ld", strip_path(filename), (long)i);
      harvest_section(&in, &in.input_sections[i], len, &add_to_harvested_sequences,
          &live_all);
    }
  }*/
  print_rodata_ptrs(logfile, in);
  delete in;
}

int
main(int argc, char **argv)
{
  //bool fcall_convention64, trunc_ret, mem_live_out;
  //bool live_all, live_callee_save, functions_only, touch_callee_saves;
  //bool sort_by_occurrences = true;
  //bool sort_increasing = false;
  //char const *annotate_locals_using_dwarf = NULL;
  //char const *output_symbols_filename = NULL;
  //char const *log_filename = DEFAULT_LOGFILE;
  //char const *input_symbols_filename = NULL;
  //char const *function_name = "ALL";
  //bool eliminate_duplicates;
  //bool canonicalize_imms;
  //char const *ddtypes;
  //char const *ddsum;
  //char *r, *ofile;
  //int optind;
  //size_t i;

  //functions_only = false;
  //live_all = false;
  //live_callee_save = false;
  //harvest_unsupported_opcodes = false;
  //touch_callee_saves = false;
  //fcall_convention64 = false;
  //mem_live_out = true;
  //trunc_ret = false;
  //canonicalize_imms = true;
  //eliminate_duplicates = true;
  //annotate_locals_using_dwarf = NULL;

  CPP_DBG_EXEC(ARGV_PRINT,
    for (int i = 0; i < argc; i++) {
      cout << " " << argv[i];
    }
    cout << endl;
  );

  //g_exec_name = argv[0];
  //optind = 1;
  //ofile = "harvest.out";
  //ddtypes = NULL;
  //ddsum = NULL;
  //len = 0;
/*
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
    } else if (!strcmp(r, "functions_only")) {
      functions_only = true;
      extern bool ignore_fetchlen_reset;
      ignore_fetchlen_reset = true;
    } else if (!strcmp(r, "annotate_locals_using_dwarf")) {
      ASSERT(optind < argc);
      annotate_locals_using_dwarf = argv[optind++];
    } else if (!strcmp(r, "no_eliminate_duplicates")) {
      eliminate_duplicates = false;
    } else if (!strcmp(r, "allow_unsupported")) {
      harvest_unsupported_opcodes = true;
    } else if (!strcmp(r, "output_symbols_filename")) {
      ASSERT(optind < argc);
      output_symbols_filename = argv[optind++];
    } else if (!strcmp(r, "input_symbols_filename")) {
      ASSERT(optind < argc);
      input_symbols_filename = argv[optind++];
    } else if (!strcmp(r, "no_canonicalize_imms")) {
      canonicalize_imms = false;
    } else if (!strcmp(r, "no_eliminate_unreachable_bbls")) {
      cfg_eliminate_unreachable_bbls = false;
    } else if (!strcmp(r, "live_all")) {
      live_all = true;
    } else if (!strcmp(r, "live_callee_save")) {
      live_callee_save = true;
    } else if (!strcmp(r, "no_sort_occur")) {
      sort_by_occurrences = false;
    } else if (!strcmp(r, "touch_callee_saves")) {
      touch_callee_saves = true;
    //} else if (!strcmp(r, "add_transmap_identity")) {
    //  add_transmap_identity = true;
    } else if (!strcmp(r, "fcall_convention64")) {
      fcall_convention64 = true;
    } else if (!strcmp(r, "mem_live_out")) {
      ASSERT(optind < argc);
      mem_live_out = bool_from_string(argv[optind++], NULL);
    } else if (!strcmp(r, "function_name")) {
      ASSERT(optind < argc);
      function_name = argv[optind++];
      printf("function_name = %s.\n", function_name);
    } else if (!strcmp(r, "trunc_ret")) {
      trunc_ret = true;
    } else if (!strcmp(r, "sort_inc")) {
      sort_increasing = true;
    } else if (!strcmp(r, "ddtypes")) {
      ASSERT(optind < argc);
      ddtypes = argv[optind++];
    } else if (!strcmp(r, "ddsum")) {
      ASSERT(optind < argc);
      ddsum = argv[optind++];
    } else if (!strcmp(r, "o")) {
      ASSERT(optind < argc);
      ofile = argv[optind++];
    } else if (!strcmp(r, "l")) {
      ASSERT(optind < argc);
      log_filename = argv[optind++];
    } else if (!strcmp(r, "len")) {
      ASSERT(optind < argc);
      len = atoi(argv[optind++]);
      ASSERT(len >= 0);
    } else if (!strcmp(r, "help")) {
      usage();
      exit(0);
    } else {
      printf("Could not parse arg: %s.\n", r);
      usage();
      exit(1);
    }
  }

  if (argc == optind) {
    usage();
    exit(1);
  }
*/

  cl::cl cmd("Harvester: harvests instruction sequences from ELF object files");
  cl::arg<bool> functions_only(cl::explicit_flag, "functions_only", false, "harvest only full functions");
  cmd.add_arg(&functions_only);
  //cl::arg<bool> annotate_locals_using_dwarf(cl::explicit_flag, "annotate_locals_using_dwarf", false, "annotate local variables (for stack slots) using DWARF debug headers");
  //cmd.add_arg(&annotate_locals_using_dwarf);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --dyn-debug=correlate=2,smt_query=1,dumptfg,decide_hoare_triple,update_invariant_state_over_edge");
  cmd.add_arg(&debug);
  cl::arg<bool> no_eliminate_duplicates(cl::explicit_flag, "no_eliminate_duplicates", false, "do not eliminate duplicate harvested sequences");
  cmd.add_arg(&no_eliminate_duplicates);
  cl::arg<bool> allow_unsupported(cl::explicit_flag, "allow_unsupported", false, "harvest opcodes that are otherwise unsupported by the superoptimizer");
  cmd.add_arg(&allow_unsupported);
  //cl::arg<string> output_symbols_filename(cl::explicit_prefix, "output_symbols_filename", "", "Output symbols filename (not used now)");
  //cmd.add_arg(&output_symbols_filename);
  //cl::arg<string> input_symbols_filename(cl::explicit_prefix, "input_symbols_filename", "", "Input symbols filename (not used now)");
  //cmd.add_arg(&input_symbols_filename);
  cl::arg<bool> no_canonicalize_imms(cl::explicit_flag, "no_canonicalize_imms", false, "do not canonicalize immediates");
  cmd.add_arg(&no_canonicalize_imms);
  cl::arg<bool> no_eliminate_unreachable_bbls(cl::explicit_flag, "no_eliminate_unreachable_bbls", false, "do not eliminate unreachable basic blocks while harvesting");
  cmd.add_arg(&no_eliminate_unreachable_bbls);
  cl::arg<bool> live_all(cl::explicit_flag, "live_all", false, "consider all touched registers as live");
  cmd.add_arg(&live_all);
  cl::arg<bool> live_callee_save(cl::explicit_flag, "live_callee_save", false, "consider callee-save registers as live");
  cmd.add_arg(&live_callee_save);
  cl::arg<bool> no_sort_occur(cl::explicit_flag, "no_sort_occur", false, "do not sort the harvested sequences by their frequency of occurrence");
  cmd.add_arg(&no_sort_occur);
  //cl::arg<bool> touch_callee_saves(cl::explicit_flag, "touch_callee_saves", false, "prefix each harvested sequence with a set of instructions that touch (read/write) the callee-save registers");
  //cmd.add_arg(&touch_callee_saves);
  cl::arg<bool> fcall_convention64(cl::explicit_flag, "fcall_convention64", false, "Use 64-bit function call calling convention");
  cmd.add_arg(&fcall_convention64);
  cl::arg<string> mem_live_out(cl::explicit_prefix, "mem_live_out", "true", "Consider memory live at exit of a harvested sequence");
  cmd.add_arg(&mem_live_out);
  cl::arg<string> function_name(cl::explicit_prefix, "f", "ALL", "Function name to harvest");
  cmd.add_arg(&function_name);
  cl::arg<bool> trunc_ret(cl::explicit_flag, "trunc_ret", false, "Truncate the return instruction");
  cmd.add_arg(&trunc_ret);
  cl::arg<bool> sort_inc(cl::explicit_flag, "sort_inc", false, "Sort in increasing order (of frequency of occurrence, for example)");
  cmd.add_arg(&sort_inc);
  //cl::arg<string> ddtypes(cl::explicit_prefix, "ddtypes", "", "ddtypes");
  //cmd.add_arg(&ddtypes);
  //cl::arg<string> ddsum(cl::explicit_prefix, "ddsum", "", "ddsum");
  //cmd.add_arg(&ddsum);
  cl::arg<string> output_file(cl::explicit_prefix, "o", "harvest.out", "Output file where harvested sequences are printed");
  cmd.add_arg(&output_file);
  cl::arg<string> log_filename(cl::explicit_prefix, "l", DEFAULT_LOGFILE, "Log file generated during the harvest (contains CFG, etc.)");
  cmd.add_arg(&log_filename);
  cl::arg<int> len(cl::explicit_prefix, "len", 0, "Maximum length of sequences to harvest (use 0 for no limit)");
  cmd.add_arg(&len);
  cl::arg<string> filename(cl::implicit_prefix, "", "Name of the ELF object file which needs to be harvested");
  cmd.add_arg(&filename);
  cl::arg<bool> use_orig_regnames(cl::explicit_flag, "use_orig_regnames", false, "Use original register names");
  cmd.add_arg(&use_orig_regnames);
  cmd.parse(argc, argv);

  //set<string> filenames;
  //while (optind < argc) {
  //  filenames.insert(argv[optind]);
  //  optind++;
  //}
  global_timeout_secs = 2000*60*60;
  init_timers();

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  src_init();
  dst_init();

  g_ctx_init();

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);

  usedef_init();

  //aths_struct.live_all = live_all;
  //aths_struct.live_callee_save = live_callee_save;
  //aths_struct.trunc_ret = trunc_ret;
  //aths_struct.add_transmap_identity = add_transmap_identity;

  //cout << __func__ << " " << __LINE__ << ": filename = " << filename.get_value() << ".\n";
  //for (const auto &filename : filenames) {
  //  harvest_input_filename(filename.c_str(), len, log_filename, g_ctx, ddsum, functions_only.get_value(), function_name.get_value(), touch_callee_saves.get_value(), fcall_convention64.get_value(), mem_live_out.get_value(), canonicalize_imms, live_all.get_value(), live_callee_save.get_value(), trunc_ret.get_value(), !no_eliminate_duplicates.get_value());
  //}
  canonicalize_imms = !no_canonicalize_imms.get_value();
  harvest_input_filename(filename.get_value().c_str(), len.get_value(), log_filename.get_value().c_str(), g_ctx, nullptr/*ddsum.get_value().c_str()*/, functions_only.get_value(), function_name.get_value().c_str()/*, touch_callee_saves.get_value()*/, fcall_convention64.get_value(), bool_from_string(mem_live_out.get_value().c_str(), nullptr), canonicalize_imms, live_all.get_value(), live_callee_save.get_value(), trunc_ret.get_value(), !no_eliminate_duplicates.get_value(), use_orig_regnames.get_value(), output_file.get_value());

  //print_harvested_sequences(output_file.get_value().c_str());
  solver_kill();
  call_on_exit_function();
  CPP_DBG_EXEC2(STATS, g_stats_print());
  return 0;
}
