#include "equiv/bequiv.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <inttypes.h>
#include "support/debug.h"
#include "rewrite/transmap.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "ppc/insn.h"
#include "i386/regs.h"
#include "ppc/regs.h"
#include "valtag/regset.h"
#include "support/timers.h"
#include "rewrite/insn_line_map.h"
#include "eq/quota.h"
#include "valtag/fixed_reg_mapping.h"
#include "rewrite/dst-insn.h"
#include "rewrite/src-insn.h"
#include "tfg/parse_input_eq_file.h"
#include "eq/eqcheck.h"
#include "eq/corr_graph.h"
#include "sym_exec/sym_exec.h"
#include "support/globals_cpp.h"

static char as1[40960];
//#include <caml/mlvalues.h>
//#include <caml/alloc.h>
//#include <caml/memory.h>
//#include <caml/fail.h>
//#include <caml/callback.h>
//#include <caml/custom.h>
//#include <caml/intext.h>

/*
char as1[4096], as2[4096], as3[4096];

//extern bool eqcheck_check_equivalence(value);
*/

static bool
tfgs_compare_eq(shared_ptr<tfg> t_src, shared_ptr<tfg> t_dst, fixed_reg_mapping_t const &dst_fixed_reg_mapping, vector<dst_insn_t> const& dst_iseq, vector<dst_ulong> const& dst_insn_pcs, context *ctx, quota const &qt, string const &name)
{
  //consts_struct_t &cs = ctx->get_consts_struct();

  //vector<memlabel_ref> relevant_memlabels;
  //tfgs_get_relevant_memlabels(relevant_memlabels, *t_src, *t_dst);
  //cs.solver_set_relevant_memlabels(relevant_memlabels);
  //ctx->set_memlabel_assertions(memlabel_assertions_t(t_dst->get_symbol_map(), t_dst->get_locals_map(), ctx, true));

  //t_src->populate_transitive_closure();
  //t_src->populate_reachable_and_unreachable_incoming_edges_map();
  //t_dst->populate_transitive_closure();
  //t_dst->populate_reachable_and_unreachable_incoming_edges_map();
  t_src->tfg_preprocess_before_eqcheck({}, {}, true);
  t_dst->tfg_preprocess_before_eqcheck({}, {}, false);

  //t_src->populate_auxilliary_structures_dependent_on_locs();
  //t_dst->populate_auxilliary_structures_dependent_on_locs();

  //t_src->resize_farg_exprs_for_function_calling_conventions();
  rodata_map_t rodata_map;
  ctx->clear_cache();
  shared_ptr<eqspace::eqcheck> e = make_shared<eqspace::eqcheck>("", "bequiv", t_src, t_dst, dst_fixed_reg_mapping, rodata_map, dst_iseq, dst_insn_pcs, ctx/*, relevant_memlabels*/, false, qt);
  auto cg = eqcheck::check_eq(e);
  ctx->clear_cache();
  if (cg) {
    CPP_DBG_EXEC2(BEQUIV,
        string sname = name;
        string_replace(sname, ":", "_");
        string proof_file = string(g_query_dir + "/proof.") + sname;
        cg->write_provable_cg_to_proof_file(proof_file, "bequiv");
        cout << __func__ << " " << __LINE__ << ": written proof to " << proof_file << endl;
        string s = t_src->graph_to_string();
        string src_iseq_file = string(g_query_dir + "/src_iseq.") + sname;
        ofstream ofs;
        ofs.open(src_iseq_file);
        ofs << s;
        ofs.close();
        cout << __func__ << " " << __LINE__ << ": written src_tfg to " << src_iseq_file << endl;
    );
  }
  bool ret = (cg != nullptr);
  return ret;
}



bool
boolean_check_equivalence(src_insn_t const src_iseq[], long src_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmap,
    dst_insn_t const dst_iseq[], long dst_iseq_len,
    regset_t const *live_out, int nextpc_indir,
    long num_live_out, bool mem_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    long max_unroll/*, sym_exec &se*/, quota const &qt)
{
  context *ctx = g_ctx;
  stopwatch_run(&boolean_check_equivalence_timer);
  CPP_DBG_EXEC2(BEQUIV, cout << __func__ << " " << __LINE__ << ": called for\n");
  CPP_DBG_EXEC2(BEQUIV, cout << __func__ << " " << __LINE__ << ": src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl);
  CPP_DBG_EXEC2(BEQUIV, cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);
  if (num_live_out > 0) {
    CPP_DBG_EXEC2(BEQUIV, cout << __func__ << " " << __LINE__ << ": out_tmaps[0] =\n" << transmap_to_string(&out_tmap[0], as1, sizeof as1) << endl);
  }
  map<pc, map<string_ref, expr_ref>> return_reg_map = get_return_reg_map_from_live_out(live_out, num_live_out, ctx);
  shared_ptr<tfg> t_src = src_iseq_get_preprocessed_tfg(src_iseq, src_iseq_len, live_out, num_live_out, return_reg_map, ctx, *g_se);
  t_src->populate_symbol_map_using_memlocs_in_tmaps(in_tmap, out_tmap, num_live_out);
  t_src->collapse_tfg(true);
  ctx->clear_cache();
  shared_ptr<tfg> t_dst = dst_iseq_get_preprocessed_tfg(dst_iseq, dst_iseq_len, in_tmap, out_tmap, num_live_out, t_src->get_argument_regs(), return_reg_map, dst_fixed_reg_mapping, ctx, *g_se);
  t_dst->collapse_tfg(true);
  ctx->clear_cache();

  string comment;
#if ARCH_SRC == ARCH_ETFG
  CPP_DBG_EXEC2(EQCHECK, cout << __func__ << " " << __LINE__ << ": t_src " << src_iseq[0].m_comment << " =\n" << t_src->graph_to_string() << endl);
  comment = src_iseq[0].m_comment;
#endif
  CPP_DBG_EXEC2(EQCHECK, cout << __func__ << " " << __LINE__ << ": t_dst =\n" << t_dst->graph_to_string() << endl);
  //t_dst->add_transmap_translation_edges_at_entry_and_exit(in_tmap, out_tmap, num_live_out);
  //cout << __func__ << " " << __LINE__ << ": verifying peep_entry " << comment << "\n";
  vector<dst_insn_t> dst_iseq_vec;
  vector<dst_ulong> insn_pcs_vec;
  for (long i = 0; i < dst_iseq_len; i++) {
    dst_iseq_vec.push_back(dst_iseq[i]);
    insn_pcs_vec.push_back(PC_UNDEFINED);
  }

  bool ret = tfgs_compare_eq(t_src, t_dst, dst_fixed_reg_mapping, dst_iseq_vec, insn_pcs_vec, ctx, qt, comment);
  //delete t_src;
  //delete t_dst;
  stopwatch_stop(&boolean_check_equivalence_timer);
  return ret;
}
