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
#include "support/cl.h"
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

static char as1[40960000], as2[40960], as3[40960];

#define SRC_ISEQ_MAX_ALLOC 2048
#define ITABLES_MAX_ALLOC (4096 * 1024)
#define LIVE_OUT_MAX_ALLOC SRC_ISEQ_MAX_ALLOC
#define MAX_NUM_WINDFALLS 65536
#define TAGLINE_MAX_ALLOC 409600
#define MAX_NUM_ITABLES 32

int
main(int argc, char **argv)
{
  cl::arg<string> itable_filename(cl::implicit_prefix, "", "filename with itable");
  cl::arg<string> jtable_cur_regmap_filename(cl::explicit_prefix, "jtable-cur-regmap", "", "filename with jtable_cur_regmap");
  cl::arg<string> output_filename(cl::explicit_prefix, "o", "jtab.out", "filename where output jtable will be written");
  cl::cl cmd("jtable_from_itable");
  cmd.add_arg(&itable_filename);
  cmd.add_arg(&output_filename);
  cmd.add_arg(&jtable_cur_regmap_filename);
  cmd.parse(argc, argv);

  init_timers();
  cmd_helper_init();
  malloc32(1);

  g_ctx_init(); //this should be first, as it involves a fork which can be expensive if done after usedef_init()
  solver_init(); //this also involves a fork and should be as early as possible
  g_query_dir_init();
  src_init();
  dst_init();
  g_se_init();
  //types_init();
  usedef_init();
  types_init();

  cpu_set_log_filename(DEFAULT_LOGFILE);

  equiv_init();



  char const *jtable_cur_regmap_filename_ptr;
  if (jtable_cur_regmap_filename.get_value() != "") {
    jtable_cur_regmap_filename_ptr = jtable_cur_regmap_filename.get_value().c_str();
  } else {
    jtable_cur_regmap_filename_ptr = NULL;
  }

  FILE *ifp = fopen(itable_filename.get_value().c_str(), "r");
  ASSERT(ifp);
  itable_position_t ipos = itable_position_from_stream(ifp);
  itable_t itable = itable_from_stream(ifp);
  //strncpy(itable.id_string, "enum", sizeof itable.id_string);
  fclose(ifp);

#define MAX_NUM_STATES 1
  jtable_t jtable;
  jtable_init(&jtable, &itable, jtable_cur_regmap_filename_ptr, MAX_NUM_STATES);

  string output_filename_val = output_filename.get_value();
  char const *output_filename_ptr = output_filename_val.c_str();
  string output_filename_gencode = output_filename_val + ".gencode.c";
  string output_filename_gencode_so = output_filename_val + ".gencode.so";
  char const *output_filename_gencode_ptr = output_filename_gencode.c_str();
  char const *output_filename_gencode_so_ptr = output_filename_gencode_so.c_str();
  //string jtab_cur = string(ABUILD_DIR "/superopt-files/jtab.") + itable.get_id_string();
  stringstream ss;
  ss << ABUILD_DIR "/superopt-files/jtab." << getpid();
  string jtab_cur = ss.str();
  string jtab_cur_gencode =  jtab_cur + ".gencode.c";
  string jtab_cur_gencode_so =  jtab_cur + ".gencode.so";
  raw_copy(output_filename_ptr, jtab_cur.c_str());
  raw_copy(output_filename_gencode_ptr, jtab_cur_gencode.c_str());
  raw_copy(output_filename_gencode_so_ptr, jtab_cur_gencode_so.c_str());
  solver_kill();
  call_on_exit_function();
  return 0;
}
