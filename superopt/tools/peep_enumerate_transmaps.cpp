#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
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
#include "rewrite/translation_instance.h"
#include "support/debug.h"

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

  translation_instance_t *ti = new translation_instance_t(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity());
  ASSERT(ti);
  translation_instance_init(ti, SUPEROPTIMIZE);

  DYN_DBG_SET(insn_match, 2);
  //DBG_SET(PEEP_TAB_ADD, 1);
  DYN_DBG_SET(peep_tab_add, 1);
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
  src_insn_t *src_iseq = new src_insn_t[src_iseq_size];
  regset_t *live_out = new regset_t[live_out_size];
  bool mem_live_out;
  long num_read;
  int sa;

  fscanf_harvest_output_src(fp, src_iseq, src_iseq_size, &src_iseq_len, live_out, live_out_size, &num_live_out, &mem_live_out, 0);
  /*size_t max_alloc = 409600;
  char *src_assembly = new char[max_alloc];
  char **live_regs_string = new char*[live_out_size];
  sa = fscanf_entry_section(fp, src_assembly, max_alloc);
  ASSERT(sa != EOF);
  src_iseq_len = src_iseq_from_string(NULL, src_iseq, src_iseq_size, src_assembly, I386_AS_MODE_32);
  for (size_t i = 0; i < num_live_out; i++) {
    cout << "live_regs_string[" << i << "] = " << live_regs_string[i] << endl;
  }

  fscanf_live_regs_strings(fp, live_regs_string, live_out_size,
      &num_live_out);*/

  cout << "src_iseq_len = " << src_iseq_len << endl;
  cout << "src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
  regsets_to_string(live_out, num_live_out, as1, sizeof as1);
  cout << "live_out = " << as1 << endl;

  read_default_peeptabs(ti);
  //read_peep_trans_table(ti, &ti->peep_table, SUPEROPTDBS_DIR "/fb.trans.tab", NULL);

  si_entry_t si;
  si.pc = 0;
  si.index = 0;
  si.bbl = NULL;
  si.bblindex = 0;
  transmap_elem_list = peep_enumerate_transmaps(ti, &si, src_iseq, src_iseq_len,
      0, live_out, num_live_out, &ti->peep_table, NULL,
      0, NULL);
  cout << "transmap_elem_list = " << transmap_elem_list << endl;

  cout << "\n\n\nPRINTING ENUMERATED TRANSMAPS:\n";
  long tmap_num = 0;
  for (transmap_set_elem_t const *e = transmap_elem_list;
       e != NULL;
       e = e->next) {
    cout << "Transmap #" << tmap_num << endl;
    cout << transmaps_to_string(e->in_tmap, e->out_tmaps, 1/*num_live_out*/, as1, sizeof as1);
    tmap_num++;
  }
  delete [] src_iseq;
  delete [] live_out;
  delete ti;
  return 0;
}
