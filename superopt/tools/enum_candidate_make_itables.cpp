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
  cl::arg<string> src_iseq_filename(cl::implicit_prefix, "", "filename with src iseq, live out, and tmaps");
  //cl::arg<int> select_opcodes(cl::explicit_prefix, "select", 1, "selective opcodes level; 0=all, 1=related-only; default=1\n");
  //cl::arg<string> rand_states_filename(cl::implicit_prefix, "", "filename with rand states");
  //cl::arg<string> typestates_filename(cl::implicit_prefix, "", "filename with typestates");
  cl::cl cmd("enum_candidate_make_itables");
  cmd.add_arg(&src_iseq_filename);
  //cmd.add_arg(&select_opcodes);
  //cmd.add_arg(&rand_states_filename);
  //cmd.add_arg(&typestates_filename);
  cmd.parse(argc, argv);

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

  cpu_set_log_filename(DEFAULT_LOGFILE);

  equiv_init();

  FILE *fp = fopen(src_iseq_filename.get_value().c_str(), "r");
  ASSERT(fp);
  size_t src_iseq_size = 1024;
  size_t live_out_size = 128;
  size_t num_live_out;
  long src_iseq_len;
  src_insn_t src_iseq[src_iseq_size];
  regset_t live_out[live_out_size];
  bool mem_live_out;
  long num_read;
  int sa;

  int ret = fscanf_harvest_output_src(fp, src_iseq, src_iseq_size, &src_iseq_len, live_out, live_out_size, &num_live_out, &mem_live_out, 0);
  ASSERT(ret != EOF);
  size_t max_alloc = 409600;
  //char *tmap_string = new char[max_alloc];
  sa = fscanf(fp, "--\n");
  //ASSERT(sa != EOF);
  transmap_t in_tmap;
  transmap_t *out_tmaps = new transmap_t[num_live_out];
  //regset_t flexible_regs;
  long num_out_tmaps;
  fscanf_transmaps(fp, &in_tmap, out_tmaps, num_live_out, &num_out_tmaps);
  ASSERT(num_out_tmaps == num_live_out);

  //cout << "src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
  regsets_to_string(live_out, num_live_out, as1, sizeof as1);
  //cout << "live_out = " << as1 << endl;
  //cout << "tmap =\n" << transmaps_to_string(&in_tmap, out_tmaps, num_live_out, as1, sizeof as1) << endl;

  //long all_iseqs_max_alloc = 819200;
  //vector<dst_insn_t> *all_iseqs = new vector<dst_insn_t>[all_iseqs_max_alloc];
  //ASSERT(all_iseqs);
  //int num_all_iseqs = 0;
  //cout << __func__ << " " << __LINE__ << ": calling read_all_dst_insns()\n";
  //ASSERT(   select_opcodes.get_value() == ITABLE_ENUM_ALL_OPCODES
  //       || select_opcodes.get_value() == ITABLE_ENUM_RELATED_OPCODES_ONLY);
  //if (select_opcodes.get_value() == ITABLE_ENUM_RELATED_OPCODES_ONLY) {
  //  num_all_iseqs = read_related_dst_insns(all_iseqs, all_iseqs_max_alloc, src_iseq, src_iseq_len);
  //} else {
  //  num_all_iseqs = read_all_dst_insns(all_iseqs, all_iseqs_max_alloc);
  //}

  vector<itable_t> itables = build_itables(src_iseq, src_iseq_len, live_out,
                                           &in_tmap,
                                           out_tmaps, num_live_out/*,
                                           all_iseqs, num_all_iseqs*/);
  //delete [] all_iseqs;

  itables_print(stdout, itables, false);
  //itables_free(itables, num_itables);

  solver_kill();
  call_on_exit_function();
  return 0;
}
