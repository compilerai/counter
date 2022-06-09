#include <sys/stat.h>
#include <sys/types.h>
#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "expr/memlabel.h"
#include "expr/z3_solver.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/debug.h"
#include "rewrite/translation_instance.h"
#include "eq/corr_graph.h"
#include "support/globals_cpp.h"
#include "etfg/etfg_insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "insn/src-insn.h"
#include "insn/dst-insn.h"
#include "rewrite/peep_entry_list.h"

#include "support/timers.h"
#include "rewrite/peephole.h"
#include "sym_exec/sym_exec.h"
#include "equiv/bequiv.h"
#include "equiv/equiv.h"

#include <iostream>
#include <fstream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;

static char as1[409600];

static bool
peep_entry_verify(struct peep_entry_t const *e, context *ctx, quota const &qt, string verify_mode)
{
  src_insn_t const *src_iseq = e->src_iseq;
  long src_iseq_len = e->src_iseq_len;
  dst_insn_t const *dst_iseq = e->dst_iseq;
  long dst_iseq_len = e->dst_iseq_len;
  regset_t const *live_out = e->live_out;
  long num_live_out = e->num_live_outs;
  int nextpc_indir = e->nextpc_indir;
  bool mem_live_out = e->mem_live_out;
  transmap_t const *in_tmap = e->get_in_tmap();
  transmap_t const *out_tmap = e->out_tmap;
  fixed_reg_mapping_t const &dst_fixed_reg_mapping = e->dst_fixed_reg_mapping;

#if ARCH_SRC == ARCH_ETFG
  if (src_iseq[0].m_comment.find("call_") == 0) {
    verify_mode = "boolean";
  }
  if (/*   src_iseq[0].m_comment.find("call_") == 0
      || */src_iseq[0].m_comment == "alloca:1"
      || src_iseq[0].m_comment == "ret0:1"
      || src_iseq[0].m_comment == "ret1:1") {
    cout << __func__ << " " << __LINE__ << ": ignoring iseqs with call/ret: " << src_iseq[0].m_comment << "\n";
    return true;
  }
  cout << __func__ << " " << __LINE__ << ": verifying " << src_iseq[0].m_comment << endl;
#endif


  bool eq;
  if (verify_mode == "all") {
    eq = check_equivalence(src_iseq, src_iseq_len, in_tmap, out_tmap, dst_iseq, dst_iseq_len, live_out, nextpc_indir, num_live_out, mem_live_out, dst_fixed_reg_mapping, MAX_UNROLL, qt);
  } else if (verify_mode == "syntactic") {
    eq = iseqs_match_syntactically(src_iseq, src_iseq_len, in_tmap, out_tmap, dst_iseq, dst_iseq_len, live_out, nextpc_indir, num_live_out);
  } else if (verify_mode == "execution") {
    DYN_DBG_SET(gen_sandboxed_iseq, 1);
    DBG_SET(EQUIV, 3);
    DBG_SET(JTABLE, 4);
    eq = execution_check_equivalence(src_iseq, src_iseq_len, in_tmap, out_tmap, dst_iseq, dst_iseq_len, live_out, num_live_out, mem_live_out, dst_fixed_reg_mapping);
  } else if (verify_mode == "boolean") {
    eq = boolean_check_equivalence(src_iseq, src_iseq_len, in_tmap, out_tmap, dst_iseq, dst_iseq_len, live_out, nextpc_indir, num_live_out, mem_live_out, dst_fixed_reg_mapping, MAX_UNROLL, qt);
  } else if (verify_mode == "fingerprint") {
    NOT_IMPLEMENTED();
  } else {
    cout << __func__ << " " << __LINE__ << ": incorrect verify mode: " << verify_mode << endl;
    NOT_REACHED();
  }
  if (!eq) {
    cout << __func__ << " " << __LINE__ << ": FAILED verification of peep_entry (mode " << verify_mode << ")\n";
    cout << "src_iseq:\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
    cout << "dst_iseq:\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
    cout << "in_tmap:\n" << transmap_to_string(in_tmap, as1, sizeof as1) << endl;
    for (long i = 0; i < num_live_out; i++) {
      if (transmaps_equal(in_tmap, &out_tmap[i])) {
        cout << "out_tmap[" << i << "] = same as in_tmap\n";
      } else {
        cout << "out_tmap[" << i << "] =\n" << transmap_to_string(&out_tmap[i], as1, sizeof as1) << endl;;
      }
    }
    for (long i = 0; i < num_live_out; i++) {
      cout << "live_out[" << i << "] = " << regset_to_string(&live_out[i], as1, sizeof as1) << endl;
    }
  } else {
    cout << __func__ << " " << __LINE__ << ": PASSED verification of peep_entry\n";
  }
  return eq;
}

int
main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> peeptab_name(cl::explicit_prefix, "t", SUPEROPTDBS_DIR "fb.trans.tab", "peeptab name to be verified");
  cl::arg<string> verify_mode(cl::explicit_prefix, "mode", "all", "Verification mode: all|boolean|execution|syntactic|fingerprint");
  cl::arg<unsigned long> max_address_space_size(cl::explicit_prefix, "max-address-space-size", (unsigned long)-1, "Max address space size");
  cl::arg<unsigned> global_timeout(cl::explicit_prefix, "global-timeout", 3600, "Total timeout (s)");
  cl::arg<unsigned> query_timeout(cl::explicit_prefix, "query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> unroll_factor(cl::explicit_prefix, "unroll-factor", 1, "Loop unrolling factor");
  cl::arg<bool> cleanup_query_files(cl::explicit_flag, "cleanup-query-files", false, "Cleanup all query files like is_expr_equal_using_lhs_set, etc., on pass result");
  cl::cl cmd("peeptab_verify: verifies if the translations in a peeptab are valid");
  cmd.add_arg(&peeptab_name);
  cmd.add_arg(&verify_mode);
  cmd.add_arg(&max_address_space_size);
  cmd.add_arg(&global_timeout);
  cmd.add_arg(&query_timeout);
  cmd.add_arg(&unroll_factor);
  cmd.add_arg(&cleanup_query_files);
  cmd.parse(argc, argv);

  init_timers();
  cout << "Verifying " << peeptab_name.get_value() << endl;

  g_query_dir_init();

  /*g_helper_pid = */eqspace::solver_init();


  src_init();
  dst_init();

  g_ctx_init();
  g_se_init();
  context *ctx = g_ctx;

  usedef_init();
  equiv_init();

  ctx->disable_expr_query_caches(); //XXX: enabling expr query caches seems to result in memory errors, perhaps due to dangling pointers left in query caches
  //ctx->parse_consts_db(CONSTS_DB_FILENAME);

  cpu_set_log_filename(DEFAULT_LOGFILE);

  //context::config cfg(qt.get_query_timeout()/*, true, true, true*/);
  consts_struct_t &cs = ctx->get_consts_struct();

  //static sym_exec se(true);
  //se.src_parse_sym_exec_db(ctx);
  //se.dst_parse_sym_exec_db(ctx);
  //se.set_consts_struct(cs);

  translation_instance_t *ti = new translation_instance_t(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity());
  ASSERT(ti);
  translation_instance_init(ti, SUPEROPTIMIZE);
  read_peep_trans_table(ti, &ti->peep_table, peeptab_name.get_value().c_str(), NULL);

  quota qt;//(unroll_factor.get_value(), query_timeout.get_value(), global_timeout.get_value(), max_address_space_size.get_value(), 0); //max_address_space_size limits the memory usable by this process causing malloc failures!

  bool failure_found = false;
  for (long i = 0; i < ti->peep_table.hash_size; i++) {
    for (auto iter = ti->peep_table.hash[i].begin(); iter != ti->peep_table.hash[i].end(); iter++) {
      struct peep_entry_t const *e = *iter;
      if (!peep_entry_verify(e, ctx, qt, verify_mode.get_value())) {
        failure_found = true;
        break;
      }
    }
    if (failure_found) {
      break;
    }
  }
  CPP_DBG_EXEC(STATS,
    cout << __func__ << " " << __LINE__ << ":\n" << stats::get();
    cout << ctx->stats() << endl;
  );

  solver_kill();
  call_on_exit_function();

  if (cleanup_query_files.get_value()) {
    clean_query_files();
  }

  return 0;
}
