#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/debug.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "support/src-defs.h"
#include "i386/insn.h"
#include "ppc/insn.h"
#include "codegen/etfg_insn.h"
#include "rewrite/src-insn.h"
#include "rewrite/peephole.h"
//#include "rewrite/harvest.h"
#include "rewrite/transmap_set.h"
#include "valtag/regcons.h"
#include "rewrite/translation_instance.h"

#include <fstream>

#include "support/timers.h"

#include <iostream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;
static char as1[40960];

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> src_iseq_file(cl::implicit_prefix, "", "path to .src_iseq file");
  cl::cl cmd("peep_enumerate_transmaps");
  cmd.add_arg(&src_iseq_file);
  cmd.parse(argc, argv);

  init_timers();

  src_init();
  dst_init();
  g_ctx_init();

  //g_ctx->parse_consts_db(CONSTS_DB_FILENAME);
  cpu_set_log_filename(DEFAULT_LOGFILE);

  usedef_init();

  translation_instance_t ti(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity());
  translation_instance_init(&ti, SUPEROPTIMIZE);

  DYN_DBG_SET(insn_match, 2);
  //DBG_SET(INSN_MATCH, 2);
  DBG_SET(PEEP_TAB, 2);
  //DBG_SET(ENUM_TMAPS, 2);
  DYN_DBG_SET(enum_tmaps, 2);

  FILE *fp = fopen(src_iseq_file.get_value().c_str(), "r");
  ASSERT(fp);

  transmap_set_elem_t *transmap_elem_list;
  size_t src_iseq_size = 1024;
  size_t live_out_size = 128;
  size_t num_live_out;
  long src_iseq_len;
  src_insn_t src_iseq[src_iseq_size];
  regset_t live_out[live_out_size];
  bool mem_live_out;
  long num_read;
  int sa;

  fscanf_harvest_output_src(fp, src_iseq, src_iseq_size, &src_iseq_len, live_out, live_out_size, &num_live_out, &mem_live_out, 0);
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
  /*char **live_regs_string = new char*[live_out_size];
  sa = fscanf_entry_section(fp, src_assembly, max_alloc);
  ASSERT(sa != EOF);
  src_iseq_len = src_iseq_from_string(NULL, src_iseq, src_iseq_size, src_assembly, I386_AS_MODE_32);
  for (size_t i = 0; i < num_live_out; i++) {
    cout << "live_regs_string[" << i << "] = " << live_regs_string[i] << endl;
  }

  fscanf_live_regs_strings(fp, live_regs_string, live_out_size,
      &num_live_out);*/

  cout << "src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
  regsets_to_string(live_out, num_live_out, as1, sizeof as1);
  cout << "live_out = " << as1 << endl;
  cout << "tmap =\n" << transmaps_to_string(&in_tmap, out_tmaps, num_live_out, as1, sizeof as1) << endl;

  //read_peep_trans_table(&ti, &ti.peep_table, SUPEROPTDBS_DIR "fb.trans.tab", NULL);
  read_default_peeptabs(&ti);

  transmap_t *peep_in_tmap = new transmap_t;
  transmap_t *peep_out_tmaps = new transmap_t[num_live_out];
  regcons_t *peep_regcons = new regcons_t;
  regset_t *peep_temp_regs_used = new regset_t;
  nomatch_pairs_set_t *peep_nomatch_pairs_set = new nomatch_pairs_set_t;
  regcount_t *touch_not_live_in = new regcount_t;
  long dst_iseq_size = 128;
  dst_insn_t *dst_iseq = new dst_insn_t[dst_iseq_size];
  long dst_iseq_len;
  nextpc_t *nextpcs = new nextpc_t[num_live_out];
  for (size_t i = 0; i < num_live_out; i++) {
    nextpcs[i] = i;
  }

  NOT_IMPLEMENTED(); //need to get peeptab_entry_id using peep_enumerate_transmaps
  peep_translate(&ti, NULL, src_iseq, src_iseq_len, nextpcs, live_out, num_live_out, &in_tmap, out_tmaps,
      peep_in_tmap, peep_out_tmaps, peep_regcons, peep_temp_regs_used, peep_nomatch_pairs_set, touch_not_live_in, dst_iseq, &dst_iseq_len, dst_iseq_size/*, false*/);

  cout << "dst_iseq:\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;

  delete [] out_tmaps;
  delete peep_in_tmap;
  delete [] peep_out_tmaps;
  delete peep_regcons;
  delete peep_temp_regs_used;
  delete peep_nomatch_pairs_set;
  delete touch_not_live_in;
  delete [] dst_iseq;
  delete [] nextpcs;
  return 0;
}
