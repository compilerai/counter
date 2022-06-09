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
#include "graph/prove.h"
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
#include "graph/avail_exprs.h"

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
#include "superopt/rand_states.h"

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
  init_timers();
  cmd_helper_init();

  malloc32(1); //allocate one byte to initialize the malloc32 structures in the beginning; late initialization can cause memory errors because of already existing heap mappings at the desired addresses
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

  cl::arg<string> src_iseq_filename(cl::implicit_prefix, "", "filename with src iseq, live out, and tmaps");
  cl::arg<int> random_seed(cl::explicit_prefix, "rand", 0, "Random seed");
  //cl::arg<int> min_thresh(cl::explicit_prefix, "min", DEFAULT_MIN_THRESH_STATES_PER_SRC_ISEQ, "minimum number of states that must be meaningful (i.e., do not trigger undef behaviour) for any one src_iseq");
  cl::cl cmd("src_iseq_gen_rand_states");
  cmd.add_arg(&src_iseq_filename);
  cmd.add_arg(&random_seed);
  //cmd.add_arg(&min_thresh);
  cmd.parse(argc, argv);

  FILE *fp = fopen(src_iseq_filename.get_value().c_str(), "r");
  ASSERT(fp);
  size_t src_iseq_size = 1024;
  size_t live_out_size = 128;
  size_t num_live_out;
  long src_iseq_len;
  src_insn_t *src_iseq = new src_insn_t[src_iseq_size];
  regset_t *live_out = new regset_t[live_out_size];
  bool mem_live_out;
  long num_read;
  int sa;

  size_t max_alloc = 409600;
  rand_states_t *rand_states = new rand_states_t;
  fscanf_harvest_output_src(fp, src_iseq, src_iseq_size, &src_iseq_len, live_out, live_out_size, &num_live_out, &mem_live_out, 0);
  sa = fscanf(fp, "--\n");
  //ASSERT(sa != EOF);
  transmap_t in_tmap;
  transmap_t *out_tmaps = new transmap_t[num_live_out];
  long num_out_tmaps;
  sa = fscanf_transmaps(fp, &in_tmap, out_tmaps, num_live_out, &num_out_tmaps);
  ASSERT(num_out_tmaps == num_live_out);

  rand_states->init_rand_states_using_src_iseq(src_iseq, src_iseq_len, in_tmap, out_tmaps, live_out, num_out_tmaps);
  delete [] out_tmaps;

  //cout << "src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
  //regsets_to_string(live_out, num_live_out, as1, sizeof as1);
  //cout << "live_out = " << as1 << endl;
  //cout << "tmap =\n" << transmaps_to_string(&in_tmap, out_tmaps, num_live_out, as1, sizeof as1) << endl;

  cout << rand_states->rand_states_to_string();

  solver_kill();
  call_on_exit_function();
  return 0;
}
