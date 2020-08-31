#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <map>
#include "expr/z3_solver.h"
#include "sym_exec/sym_exec.h"
#include "support/debug.h"
#include "superopt/harvest.h"
#include "config-host.h"
#include "rewrite/jumptable.h"
#include "rewrite/dst-insn.h"
#include "valtag/symbol_set.h"
#include "valtag/nextpc_map.h"
#include "rewrite/insn_line_map.h"
#include "rewrite/symbol_map.h"
#include "support/cl.h"
//#include "strtab.h"
//#include "disas.h"

#include "cmn/cmn.h"
#include "valtag/elf/elf.h"

#include "rewrite/static_translate.h"
#include "superopt/typestate.h"
//#include "dbg_functions.h"

#include "i386/opctable.h"
#include "valtag/regset.h"
//#include "temporaries.h"
#include "support/c_utils.h"
#include "rewrite/transmap.h"
#include "rewrite/peephole.h"

#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "ppc/insn.h"
#include "rewrite/src-insn.h"

//#include "live_ranges.h"
#include "ppc/regs.h"
#include "rewrite/rdefs.h"
#include "valtag/memset.h"
#include "ppc/insn.h"

#include "rewrite/edge_table.h"

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
#include "tfg/tfg.h"

//#include <caml/mlvalues.h>
//#include <caml/alloc.h>
//#include <caml/memory.h>
//#include <caml/fail.h>
//#include <caml/callback.h>
//#include <caml/custom.h>
//#include <caml/intext.h>

using namespace eqspace;
static char as1[409600];

#define SRC_ISEQ_MAX_ALLOC 2048
#define ITABLES_MAX_ALLOC (4096 * 1024)
#define LIVE_OUT_MAX_ALLOC SRC_ISEQ_MAX_ALLOC
#define MAX_NUM_WINDFALLS 65536
#define TAGLINE_MAX_ALLOC 409600
#define MAX_NUM_ITABLES 32

//void yices_initialize(void);

static void
usage(void)
{
  NOT_IMPLEMENTED();
  printf("eqgen:\n"
     "usage: eqgen <peeptab_filename>"
     "\n"
     );
}

static char pbuf1[512000];

/*static void
gen_eq_for_peep_entry(peep_entry_t const *peep_entry, long max_unroll, char const *tfg_llvm, char const *function_name, struct context *ctx)
{
  if (dst_iseq_contains_reloc_reference(peep_entry->dst_iseq,
      peep_entry->dst_iseq_len)) {
    return;
  }
  eqcheck_gen_eq_for_peep_entry(peep_entry->src_iseq, peep_entry->src_iseq_len,
        peep_entry->in_tmap, peep_entry->out_tmap,
        peep_entry->dst_iseq, peep_entry->dst_iseq_len,
        peep_entry->live_out, peep_entry->nextpc_indir,
        peep_entry->num_live_outs, peep_entry->mem_live_out,
        peep_entry->src_symbol_map, peep_entry->dst_symbol_map,
        peep_entry->src_locals_info, peep_entry->dst_locals_info,
        &peep_entry->src_iline_map, &peep_entry->dst_iline_map,
        peep_entry->nextpc_function_name_map,
        max_unroll, tfg_llvm, function_name, ctx);
}*/

//extern char const *peeptab_filename;
//extern struct context *g_ctx;

/*static void
gen_eq_for_peephole_table(peep_table_t const *tab, long max_unroll, char const *tfg_llvm, char const *function_name, struct context *ctx)
{
  peep_entry_t const *peep_entry;
  long i;

  for (i = 0; i < tab->hash_size; i++) {
    for (peep_entry = tab->hash[i]; peep_entry; peep_entry = peep_entry->next) {
      gen_eq_for_peep_entry(peep_entry, max_unroll, tfg_llvm, function_name, ctx);
    }
  }
  printf("gen_eq_for_peephole_table done.\n");
}*/

static void
get_symbol_set_from_logfile(symbol_set_t *symbol_set, char const *logfile)
{
  long symbol_num, symbol_val, symbol_size, symbol_type, symbol_bind, symbol_shndx;
  char const *remaining1, *remaining2, *space;
  size_t const max_line_size = 2048000;
  string symbol_name;
  int num_read;
  string line;

  //logfp = fopen(logfile, "r");
  //ASSERT(logfp);
  ifstream logfp(logfile);
  ASSERT(logfp.is_open());

  while (getline(logfp, line)) {
    bool is_dynamic;
    if (   !strstart(line.c_str(), "SYMBOL", &remaining1)
        && !strstart(line.c_str(), "DYNSYM", &remaining1)) {
      continue;
    }
    if (strstart(line.c_str(), "SYMBOL", NULL)) {
      is_dynamic = false;
    } else {
      is_dynamic = true;
    }
    DBG(SYMBOLS, "remaining1=%s.\n", remaining1);
    symbol_num = strtol(remaining1, (char **)&remaining2, 0);
    ASSERT(remaining2[0] == ':');
    ASSERT(remaining2[1] == ' ');
    DBG(SYMBOLS, "remaining2=%s.\n", remaining2);
    remaining2 += 2;
    space = strchr(remaining2, ' ');
    DBG(SYMBOLS, "space=%s.\n", space);
    char *symbol_name_chrs = new char[space - remaining2 + 1];
    memcpy(symbol_name_chrs, remaining2, space - remaining2);
    symbol_name_chrs[space - remaining2] = '\0';
    symbol_name = symbol_name_chrs;
    delete[] symbol_name_chrs;
    DBG(SYMBOLS, "symbol_name=%s.\n", symbol_name.c_str());
    remaining1 = space + 1;
    ASSERT(remaining1[0] == ':');
    ASSERT(remaining1[1] == ' ');
    remaining1 += 2;
    DBG(SYMBOLS, "remaining1=%s.\n", remaining1);

    symbol_val = strtol(remaining1, (char **)&remaining2, 0);
    DBG(SYMBOLS, "remaining2=%s.\n", remaining2);
    ASSERT(remaining2[0] == ' ');
    ASSERT(remaining2[1] == ':');
    remaining2 += 2;
    DBG(SYMBOLS, "remaining1=%s.\n", remaining1);
    SWAP_PTRS(remaining1, remaining2);

    symbol_size = strtol(remaining1, (char **)&remaining2, 0);
    DBG(SYMBOLS, "remaining2=%s.\n", remaining2);
    ASSERT(remaining2[0] == ' ');
    ASSERT(remaining2[1] == ':');
    remaining2 += 2;
    DBG(SYMBOLS, "remaining1=%s.\n", remaining1);
    SWAP_PTRS(remaining1, remaining2);

    symbol_bind = strtol(remaining1, (char **)&remaining2, 0);
    ASSERT(remaining2[0] == ' ');
    ASSERT(remaining2[1] == ':');
    remaining2 += 2;
    SWAP_PTRS(remaining1, remaining2);

    symbol_type = strtol(remaining1, (char **)&remaining2, 0);
    ASSERT(remaining2[0] == ' ');
    ASSERT(remaining2[1] == ':');
    remaining2 += 2;
    SWAP_PTRS(remaining1, remaining2);

    symbol_shndx = strtol(remaining1, (char **)&remaining2, 0);
    SWAP_PTRS(remaining1, remaining2);

    symbol_set_add(symbol_set, is_dynamic, symbol_num, symbol_name.c_str(), symbol_val, symbol_size,
        symbol_bind, symbol_type, symbol_shndx);
  }
  //fclose(logfp);
}

static string
get_function_name_from_endline(char const *endline)
{
  string s = endline;
  size_t space1 = s.find(' ');
  ASSERT(space1 != string::npos);
  size_t space2 = s.find(' ', space1 + 1);
  ASSERT(space2 != string::npos);
  size_t space3 = s.find(' ', space2 + 1);
  ASSERT(space3 != string::npos);
  size_t colon = s.find(':', space3 + 1);
  if (colon == string::npos) {
    cout << __func__ << " " << __LINE__ << ": endline =\n" << endline << endl;
  }
  ASSERT(colon != string::npos);
  return s.substr(space3 + 1, colon - space3 - 1);
}

static bool
function_is_my_helper_function(string const &name)
{
  return (name.substr(0, 4) == "MYmy");
}

static bool
function_is_auxilliary_dst_function(string const &name)
{
  return    false
         || name == "register_tm_clones"
         || name == "_start"
         || name == "deregister_tm_clones"
         || name == "__do_global_ctors_aux"
         || name == "__do_global_dtors_aux"
         || name == "_fini"
         || name == "_init"
         || name == "_dl_relocate_static_pie"
         || name == "__libc_csu_init"
         || name == "__libc_csu_fini"
         || name == "frame_dummy"
         || strstart(name.c_str(), "__x86.get_pc_thunk.", nullptr)
  ;
}

static void
print_dst_iseq_with_live_regsets_and_symbol_set(dst_insn_t const* dst_iseq, long dst_iseq_len, regset_t const* live_out, size_t num_live_out, imm_vt_map_t const* dst_symbols_map, symbol_set_t const* symbol_set, char* endline)
{
  printf("ENTRY:\n");
  printf("%s--\n", dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
  regsets_to_string(live_out, num_live_out, as1, sizeof as1);
  printf("live : %s\nmemlive : true\n--\n", as1);
  imm_vt_map_to_string_using_symbols(dst_symbols_map, symbol_set, as1, sizeof as1),
  printf("%s--\n", as1);
  char *peep_comment = endline;
  char *end = peep_comment + strlen(peep_comment);
  char *ptr = strrchr(peep_comment, '=');
  ASSERT(ptr && ptr > peep_comment + 1 && ptr[-1] == '=');
  ptr--;
  ptr += snprintf(ptr, end - ptr, "==");
  //ASSERT(ptr < end);
  printf("%s\n", peep_comment);
}

int
main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> harvest_filename(cl::implicit_prefix, "", "harvest filename");
  cl::arg<string> harvest_log_filename(cl::explicit_prefix, "l", "", "harvest log filename");
  cl::arg<string> function_name(cl::explicit_prefix, "f", "ALL", "function name");
  cl::arg<string> output_filename(cl::explicit_prefix, "o", "out.tfg", "output filename");
  cl::arg<string> exec_filename(cl::explicit_prefix, "e", "", "exec filename");
  cl::arg<string> etfg_filename(cl::explicit_prefix, "tfg_llvm", "", "etfg filename containing LLVM tfg");
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");


  cl::cl cmd("eqgen: generates TFGs from the harvest file");
  cmd.add_arg(&harvest_filename);
  cmd.add_arg(&harvest_log_filename);
  cmd.add_arg(&function_name);
  cmd.add_arg(&output_filename);
  cmd.add_arg(&exec_filename);
  cmd.add_arg(&etfg_filename);
  cmd.add_arg(&debug);

  CPP_DBG_EXEC(ARGV_PRINT,
      for (int i = 0; i < argc; i++) {
        cout << " " << argv[i];
      }
      cout << endl;
  );
  cmd.parse(argc, argv);

  eqspace::init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, eqspace::print_debug_class_levels());
  //char const *tfg_llvm = "tfg.llvm.default";
  //char const *function_name = NULL;
  //char const *logfile = NULL;
  //char const *outfile = NULL;
  //char const *execfile = NULL;
  //int optind;
  //char *r;
  //long i;

  //running_as_peepgen = true;
  //peepgen = true;
  global_timeout_secs = 5*60*60; //5 hours
  init_timers();

  g_ctx_init(); //this should be first, as it involves a fork which can be expensive if done after usedef_init()

  context *ctx = g_ctx;
  src_init();
  dst_init();
  //types_init();
  usedef_init();
  g_se_init();
  //yices_initialize();

  printf("%s %d: harvest_filename = %s, tfg_llvm = %s, function_name = %s.\n", __func__, __LINE__, harvest_filename.get_value().c_str(), etfg_filename.get_value().c_str(), function_name.get_value().c_str());
  //printf("pid %d: Using random seed %d\n", getpid(), random_seed);
  //srand_random(random_seed);

  //printf("Verifying current peephole table . . .\n");
  //translation_instance_init(&ti, SUPEROPTIMIZE);

  //printf("reading peep table %s.\n", peeptab_filename);
  //read_peep_trans_table(&ti, &ti.peep_table, peeptab_filename, "");
  //if (ignore_insn_line_map) {
  //  peeptab_free_insn_line_maps(&ti.peep_table);
  //}

  FILE *harvest_fp = fopen(harvest_filename.get_value().c_str(), "r");
  if (!harvest_fp) {
    printf("Could not open harvest filename '%s'\n", harvest_filename.get_value().c_str());
  }
  ASSERT(harvest_fp);

  //autostop_timer func_timer(string(__func__) + ".eqgen");

  imm_vt_map_t *dst_symbols_map;
  dst_insn_t *dst_iseq;
  regset_t *live_out;
  size_t max_alloc = 409600;
  //size_t endline_size = 40960;
  size_t num_live_out;
  long dst_iseq_len;
  bool mem_live_out;
  char *endline;

  //dst_iseq = (dst_insn_t *)malloc(max_alloc * sizeof(dst_insn_t));
  dst_iseq = new dst_insn_t[max_alloc];
  ASSERT(dst_iseq);
  //live_out = (regset_t *)malloc(max_alloc * sizeof(regset_t));
  live_out = new regset_t[max_alloc];
  ASSERT(live_out);
  dst_symbols_map = (imm_vt_map_t *)malloc(sizeof(imm_vt_map_t));
  ASSERT(dst_symbols_map);
  endline = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(endline);
  symbol_set_t *symbol_set = new symbol_set_t;
  //nextpc_map_t *nextpc_map = new nextpc_map_t;

  unlink(output_filename.get_value().c_str());
  regmap_t regmap;

  symbol_set_init(symbol_set);
  get_symbol_set_from_logfile(symbol_set, harvest_log_filename.get_value().c_str());

  string cur_function_name;
  nextpc_function_name_map_t nextpc_function_name_map;
  string assembly;
  vector<dst_ulong> insn_pcs;
  //cout << __func__ << " " << __LINE__ << endl;
  while (harvest_get_next_dst_iseq(harvest_fp, dst_iseq, &dst_iseq_len, live_out, &num_live_out, &mem_live_out, regmap, cur_function_name, nextpc_function_name_map, assembly, insn_pcs, endline, max_alloc)) {
    //cout << __func__ << " " << __LINE__ << ": cur_function_name = " << cur_function_name << endl;

    //cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": nextpc_function_name_map =\n" << nextpc_function_name_map_to_string(&nextpc_function_name_map, as1, sizeof as1) << endl;
    //string cur_function_name = get_function_name_from_endline(endline);
    if (   function_name.get_value() != "ALL"
        && function_name.get_value() != cur_function_name) {
      cout << __func__ << " " << __LINE__ << ": Ignoring: " << cur_function_name << "\n";
      continue;
    }
    if (function_is_auxilliary_dst_function(cur_function_name)) {
      cout << __func__ << " " << __LINE__ << ": Ignoring: " << cur_function_name << "\n";
      continue;
    }
    if (function_is_my_helper_function(cur_function_name)) {
      cout << __func__ << " " << __LINE__ << ": Ignoring: " << cur_function_name << "\n";
      continue;
    }
    cout << __func__ << " " << __LINE__ << ": Doing: " << cur_function_name << "\n";
    cmn_iseq_canonicalize_symbols(dst_iseq, dst_iseq_len, dst_symbols_map);
    //char const *nextpcs_start = strchr(endline, '#');
    //ASSERT(nextpcs_start);
    //char const *nextpcs_end = strchr(nextpcs_start, '=');
    //ASSERT(nextpcs_end);

    //char *nextpcs_string = (char *)malloc(nextpcs_end - nextpcs_start + 1);
    //ASSERT(nextpcs_string);
    //memcpy(nextpcs_string, nextpcs_start + 1, nextpcs_end - nextpcs_start);
    //nextpcs_string[nextpcs_end - nextpcs_start] = '\0';

    //nextpc_map_from_short_string_using_symbols(&nextpc_map, nextpcs_string, &symbol_set);

    CPP_DBG_EXEC(EQGEN, print_dst_iseq_with_live_regsets_and_symbol_set(dst_iseq, dst_iseq_len, live_out, num_live_out, dst_symbols_map, symbol_set, endline));
    static symbol_map_t symbol_map;
    imm_vt_map_to_symbol_map(&symbol_map, dst_symbols_map, symbol_set);

    //ASSERT(outfile);
    //ASSERT(execfile);
    tfg::gen_tfg_for_dst_iseq(output_filename.get_value().c_str(), exec_filename.get_value().c_str(), dst_iseq, dst_iseq_len, regmap, etfg_filename.get_value().c_str(), cur_function_name.c_str(), &symbol_map, nextpc_function_name_map, num_live_out, insn_pcs, ctx);
    //cout << __func__ << " " << __LINE__ << ": done " << cur_function_name << "\n";
    cout << __func__ << " " << __LINE__ << ": Done: " << cur_function_name << "\n";
  }

  fclose(harvest_fp);

  //delete nextpc_map;
  delete symbol_set;
  free(dst_symbols_map);
  delete[] live_out;
  delete[] dst_iseq;
  solver_kill();
  call_on_exit_function();
  return 0;
}
