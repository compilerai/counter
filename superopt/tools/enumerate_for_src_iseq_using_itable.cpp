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
#include "support/c_utils.h"
#include "support/src-defs.h"
#include "support/debug.h"
//#include "rewrite/harvest.h"
#include "codegen/etfg_insn.h"
#include "i386/insntypes.h"
#include "rewrite/dst-insn.h"
#include "rewrite/insn_list.h"
//#include "config.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "rewrite/jumptable.h"
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
#include "rewrite/transmap.h"
#include "rewrite/peephole.h"
#include "superopt/rand_states.h"
#include "i386/insn.h"
#include "ppc/insn.h"
#include "rewrite/src-insn.h"

//#include "live_ranges.h"
#include "ppc/regs.h"
#include "rewrite/rdefs.h"
#include "valtag/memset.h"
#include "ppc/insn.h"

#include "rewrite/edge_table.h"
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
#include "superopt/itable_stopping_cond.h"

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

static void
shuffle_states(rand_states_t *rand_states, typestate_t *typestate_initial, typestate_t *typestate_final)
{
  size_t num_states = rand_states->get_num_states();
  size_t indices[num_states];
  for (size_t i = 0; i < num_states; i++) {
    indices[i] = i;
  }
  qshuffle(indices, num_states, sizeof(indices[0]));
  rand_states->reorder_states_using_indices(indices);
  typestate_t *typestate_initial_copy = new typestate_t[num_states];
  typestate_t *typestate_final_copy = new typestate_t[num_states];
  for (size_t i = 0; i < num_states; i++) {
    typestate_copy(&typestate_initial_copy[i], &typestate_initial[indices[i]]);
    typestate_copy(&typestate_final_copy[i], &typestate_final[indices[i]]);
  }
  for (size_t i = 0; i < num_states; i++) {
    typestate_copy(&typestate_initial[i], &typestate_initial_copy[i]);
    typestate_copy(&typestate_final[i], &typestate_final_copy[i]);
  }
  delete [] typestate_initial_copy;
  delete [] typestate_final_copy;
}

int
main(int argc, char **argv)
{
  cl::arg<string> src_iseq_filename(cl::implicit_prefix, "", "filename with src iseq, live out, and tmaps");
  cl::arg<string> rand_states_filename(cl::implicit_prefix, "", "filename with rand states");
  cl::arg<string> typestate_filename(cl::implicit_prefix, "", "filename with typestates");
  cl::arg<string> itable_filename(cl::implicit_prefix, "", "filename with itable");
  cl::arg<string> jtable_filename_cl(cl::explicit_prefix, "jtab", "", "filename with jtable");
  cl::arg<string> equiv_output_filename(cl::explicit_prefix, "o", "equiv.out", "Output equiv filename: the equivalent sequence is written to this");
  cl::arg<string> ipos_output_filename(cl::explicit_prefix, "ipos_o", "ipos.out", "Output ipos filename: the final ipos (at which enumeration was stopped) is written to this");
  cl::arg<string> stop_when(cl::explicit_prefix, "stop-when", "equiv", "Stop when never|equiv|1|2|3|... (1|2|3..: enumerate <num> iseqs, equiv: equivalent sequence found)");
  cl::arg<bool> shuffle_states_flag(cl::explicit_prefix, "shuffle-states", true, "Shuffle states for fingerprint/execution test? Default: yes; can disable for deterministic debugging");
  cl::cl cmd("enumerate_for_src_iseq_using_itable");
  cmd.add_arg(&src_iseq_filename);
  cmd.add_arg(&rand_states_filename);
  cmd.add_arg(&typestate_filename);
  cmd.add_arg(&itable_filename);
  cmd.add_arg(&jtable_filename_cl);
  cmd.add_arg(&equiv_output_filename);
  cmd.add_arg(&ipos_output_filename);
  cmd.add_arg(&stop_when);
  cmd.add_arg(&shuffle_states_flag);
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

  string jtable_filename = jtable_filename_cl.get_value();
  if (jtable_filename != "" && jtable_filename.at(0) != '/' && jtable_filename.at(0) != '.') {
    jtable_filename = string("./") + jtable_filename;
  }

  char const *jtable_cur_regmap_filename = NULL;
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
  fclose(fp);

  //cout << "src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
  regsets_to_string(live_out, num_live_out, as1, sizeof as1);
  //cout << "live_out = " << as1 << endl;
  //cout << "tmap =\n" << transmaps_to_string(&in_tmap, out_tmaps, num_live_out, as1, sizeof as1) << endl;

  FILE *ifp = fopen(itable_filename.get_value().c_str(), "r");
  ASSERT(ifp);
  itable_position_t ipos = itable_position_from_stream(ifp);
  itable_t itable = itable_from_stream(ifp);
  //strncpy(itable.id_string, "enum", sizeof itable.id_string);
  fclose(ifp);

  ifstream ifs;
  ifs.open(rand_states_filename.get_value());
  rand_states_t *rand_states = rand_states_t::read_rand_states(ifs);
  ifs.close();

  size_t num_states = rand_states->get_num_states();

  src_fingerprintdb_elem_t **fdb;
  fdb = src_fingerprintdb_init();
  ASSERT(fdb);

  typestate_t *typestate_initial, *typestate_final;

  typestate_initial = new typestate_t[num_states];
  ASSERT(typestate_initial);
  for (size_t i = 0; i < num_states; i++) {
    typestate_init(&typestate_initial[i]);
  }
  typestate_final = new typestate_t[num_states];
  ASSERT(typestate_final);
  for (size_t i = 0; i < num_states; i++) {
    typestate_init(&typestate_final[i]);
  }
  typestate_from_file(typestate_initial, typestate_final, num_states, typestate_filename.get_value().c_str());

  if (shuffle_states_flag.get_value()) {
    shuffle_states(rand_states, typestate_initial, typestate_final);
  }

  itable_stopping_cond_t itable_stopping_cond(stop_when.get_value());
  if (itable_stopping_cond.is_counter(1)) {
    DBG_SET(JTABLE, 4);
    DBG_SET(TYPESTATE, 4);
    DBG_SET(EQUIV, 3);
  }

  src_fingerprintdb_add_variants(fdb, src_iseq, src_iseq_len, live_out,
      &in_tmap, out_tmaps, num_live_out,
      //itable.get_use_types(),
      typestate_initial,
      typestate_final, mem_live_out, NULL, "", *rand_states);

  rand_states->make_dst_stack_regnum_point_to_stack_top(itable.get_fixed_reg_mapping());
  typestates_set_stack_regnum_to_bottom(typestate_initial, typestate_final, num_states, itable.get_fixed_reg_mapping());

  long long max_costs[dst_num_costfns];
  src_fingerprintdb_compute_max_costs(fdb, max_costs);
  /*if (itable.get_use_types() == USE_TYPES_OPS_AND_GE_INIT_MEET_FINAL) {
    typestates_meet(typestate_final, typestate_initial, NUM_FP_STATES);
  } else if (itable.get_use_types() == USE_TYPES_OPS_AND_GE_MODIFIED_BOTTOM) {
    typestates_set_modified_to_bottom(typestate_final, typestate_initial,
        NUM_FP_STATES);
  }*/
  jtable_t jtable;
  if (jtable_filename != "") {
    cout << __func__ << " " << __LINE__ << ": reading jtable " << jtable_filename << endl;
    jtable_read(&jtable, &itable, jtable_filename.c_str(), jtable_cur_regmap_filename, num_states);
    cout << __func__ << " " << __LINE__ << ": done reading jtable " << jtable_filename << endl;
  } else {
    cout << __func__ << " " << __LINE__ << ": initing new jtable" << endl;
    jtable_init(&jtable, &itable, jtable_cur_regmap_filename, num_states);
    cout << __func__ << " " << __LINE__ << ": done initing new jtable" << endl;
  }

  disable_signals();
  printf("starting itable_enumeration\n");
  bool found;
  uint64_t yield_secs = 360/*0*/;
  regset_t dst_touch = transmaps_get_used_dst_regs(&in_tmap, out_tmaps, num_live_out);
  found = itable_enumerate(&itable, &jtable, fdb, &ipos, max_costs,
      yield_secs,
      itable_stopping_cond,
      false,
      typestate_initial, typestate_final, dst_touch,
      equiv_output_filename.get_value().c_str(),
      *rand_states);

  printf("finished one round of itable_enumeration, found = %d:\n", (int)found);
  printf("max_vector: %s\n", itable_position_to_string(&ipos, as1, sizeof as1));
  ofstream ofs(ipos_output_filename.get_value());
  ofs << itable_position_to_string(&ipos, as1, sizeof as1);
  ofs.close();

  delete rand_states;
  CPP_DBG_EXEC(TMP,
    print_all_timers();
    cout << stats::get() << endl;
  );

  solver_kill();
  call_on_exit_function();
  return 0;
}
