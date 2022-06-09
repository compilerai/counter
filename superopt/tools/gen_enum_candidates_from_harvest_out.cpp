#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <memory>
#include <functional>
#include <cassert>
#include "support/cl.h"
#include "support/debug.h"
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



using namespace std;

static void
output_enum_candidate(src_insn_t const *src_iseq, long src_iseq_len, regset_t const *live_out, long num_live_out, bool mem_live_out, transmap_t const &in_tmap, vector<transmap_t> const &out_tmaps)
{
  long max_alloc = 409600;
  char *buf = new char[max_alloc];
  char *ptr = buf;
  char *end = ptr + max_alloc;
  ptr += src_harvested_iseq_to_string(ptr, end - ptr, src_iseq, src_iseq_len, live_out, num_live_out, mem_live_out);
  ptr += snprintf(ptr, end - ptr, "--\n");
  transmap_t out_tmaps_arr[num_live_out];
  ASSERT(out_tmaps.size() == num_live_out);
  long i = 0;
  for (const auto &out_tmap : out_tmaps) {
    out_tmaps_arr[i] = out_tmap;
    i++;
  }
  transmaps_to_string(&in_tmap, out_tmaps_arr, num_live_out, ptr, end - ptr);
  cout << buf << "==\n";
  delete [] buf;
}

static void
output_enum_candidates(src_insn_t const *src_iseq, long src_iseq_len, regset_t const *live_out, long num_live_out, bool mem_live_out, int prioritize)
{
  list<in_out_tmaps_t> in_out_tmaps = src_iseq_enumerate_in_out_transmaps(src_iseq, src_iseq_len, live_out, num_live_out, mem_live_out, prioritize);
  for (const auto &in_out_tmap : in_out_tmaps) {
    output_enum_candidate(src_iseq, src_iseq_len, live_out, num_live_out, mem_live_out, in_out_tmap.first, in_out_tmap.second);
  }
}

int main(int argc, char **argv)
{
  cl::arg<string> harvest_filename_opt(cl::implicit_prefix, "", "filename with harvest output");
  cl::arg<int> prioritize(cl::explicit_prefix, "p", 0, "generate only high-priority candidates; specify priority level; default 0");
  cl::cl cmd("gen_enum_candidates_from_harvest_out");
  cmd.add_arg(&harvest_filename_opt);
  cmd.add_arg(&prioritize);
  cmd.parse(argc, argv);
  string harvest_filename = harvest_filename_opt.get_value();


  init_timers();
  g_ctx_init();
  solver_init(); //this also involves a fork and should be as early as possible
  src_init();
  dst_init();
  g_se_init();
  usedef_init();
  types_init();
  cpu_set_log_filename(DEFAULT_LOGFILE);

  equiv_init();



  FILE *fp = fopen(harvest_filename.c_str(), "r");
  size_t iseq_max_alloc = 4096;
  size_t live_out_max_alloc = 512;
  src_insn_t *iseq = new src_insn_t[iseq_max_alloc];
  regset_t *live_out = new regset_t[live_out_max_alloc];
  long iseq_len;
  size_t num_live_out;
  bool mem_live_out;
  int num_read;
  while (fscanf_harvest_output_src(fp, iseq, iseq_max_alloc, &iseq_len, live_out, live_out_max_alloc, &num_live_out, &mem_live_out, 0) != EOF) {
    num_read = fscanf(fp, "%*[^\n]\n");
    output_enum_candidates(iseq, iseq_len, live_out, num_live_out, mem_live_out, prioritize.get_value());
  }
  delete [] iseq;
  delete [] live_out;
  fclose(fp);
  solver_kill();
  call_on_exit_function();
  return 0;
}
