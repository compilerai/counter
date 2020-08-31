#include <sys/stat.h>
#include <sys/types.h>
#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "expr/memlabel.h"
#include "expr/z3_solver.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "eq/corr_graph.h"
#include "support/globals_cpp.h"
#include "codegen/etfg_insn.h"
#include "i386/insn.h"
#include "rewrite/src-insn.h"
#include "rewrite/dst-insn.h"
#include "rewrite/peep_entry_list.h"
#include "superopt/rand_states.h"
#include "support/timers.h"
#include "rewrite/peephole.h"
#include "sym_exec/sym_exec.h"
#include "equiv/bequiv.h"
#include "equiv/equiv.h"
#include "superopt/superopt.h"
#include "support/c_utils.h"
#include "superopt/itable.h"
#include "equiv/jtable.h"
#include "superopt/itable_stopping_cond.h"
#include "rewrite/peephole.h"
#include "support/cmd_helper.h"
#include "superopt/typesystem.h"
#include "rewrite/translation_instance.h"
#include "support/debug.h"
#include <iostream>
#include <fstream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;

static char as1[409600];
//typedef size_t typesystem_id_t;
//typedef size_t tmp_reg_count_t;
//typedef size_t tmp_stackslot_count_t;
//typedef bool convert_addr_to_intaddr_t;
//typedef bool make_non_poly_copyable_t;
//typedef bool set_poly_bytes_in_non_poly_regs_to_bottom_t;
#define MAX_NUM_TMP_REGS (typesystem_t::max_num_temp_regs)
#define MAX_NUM_TMP_STACKSLOTS (typesystem_t::max_num_stack_slots)
//#define TMP_STACK_SLOT_SIZE (DWORD_LEN/BYTE_LEN)

//typedef tuple<use_types_t, convert_addr_to_intaddr_t, make_non_poly_copyable_t, set_poly_bytes_in_non_poly_regs_to_bottom_t> typesystem_t;
//vector<typesystem_t> typesystem_hierarchy;

static bool
typestate_config_is_dominated(set<tuple<typesystem_id_t, tmp_reg_count_t, tmp_stackslot_count_t>> const &dominators, typesystem_id_t t, tmp_reg_count_t r, tmp_stackslot_count_t s)
{
  for (const auto &d : dominators) {
    if (   get<0>(d) <= t
        && get<1>(d) <= r
        && get<2>(d) <= s) {
      return true;
    }
  }
  return false;
}

static vector<size_t>
gen_max_num_temps_for_tmp_reg_count(tmp_reg_count_t r)
{
#if ARCH_DST == ARCH_I386
  vector<size_t> ret;
  for (size_t i = 0; i < I386_NUM_EXREG_GROUPS; i++) {
    if (i == I386_EXREG_GROUP_GPRS) {
      ret.push_back(r);
    } else {
      tmp_reg_count_t num_temps = standard_max_num_temps().at(i);
      ret.push_back(num_temps);
      //cout << __func__ << " " << __LINE__ << ": group " << i << ": num_temps = " << num_temps << endl;
    }
  }
  return ret;
#else
  NOT_IMPLEMENTED();
#endif
}

static itable_t
make_itable_with_single_dst_iseq(dst_insn_t const *dst_iseq, long dst_iseq_len,
    src_insn_t const *src_iseq, long src_iseq_len, regset_t const *live_out,
    transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_live_out/*,
    use_types_t use_types, tmp_reg_count_t r, tmp_stackslot_count_t s*/,
    typesystem_t const &typesystem)
{
  autostop_timer func_timer(__func__);
  //cout << "r = " << r << endl;
  CPP_DBG_EXEC(TYPECHECK, cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);
  vector<dst_insn_t> dst_insn_vec = dst_insn_vec_from_iseq(dst_iseq, dst_iseq_len);
  use_types_t use_types = typesystem.get_use_types();
  tmp_reg_count_t r = typesystem.get_tmp_reg_count();
  //vector<size_t> max_num_temps = gen_max_num_temps_for_tmp_reg_count(r);
  tmp_stackslot_count_t s = typesystem.get_tmp_stackslot_count();
  itable_t ret = itable_all(src_iseq, src_iseq_len, live_out, in_tmap, out_tmaps, num_live_out, &dst_insn_vec, 1/*, NULL, max_num_temps, s, false*/, typesystem);
  CPP_DBG_EXEC(TYPECHECK, cout << __func__ << " " << __LINE__ << ": ret =\n" << itable_to_string(&ret, as1, sizeof as1) << endl);
  ret.itable_eliminate_equivalent_instructions();
  ret.itable_sort();
  //ret.set_use_types(use_types);
  cout << __func__ << " " << __LINE__ << ": ret =\n" << itable_to_string(&ret, as1, sizeof as1) << endl;
  return ret;
}

static bool
typecheck_dst_iseq_against_typestates_for_config(dst_insn_t const *dst_iseq, long dst_iseq_len, src_insn_t const *src_iseq, long src_iseq_len, regset_t const *live_out, transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_live_out, bool mem_live_out, fixed_reg_mapping_t const &dst_fixed_reg_mapping, rand_states_t const &rand_states, typestate_t const *typestate_initial, typestate_t const *typestate_final, vector<typesystem_t> const &typesystem_hierarchy, typesystem_id_t t/*, tmp_reg_count_t r, tmp_stackslot_count_t s*/)
{
  autostop_timer func_timer(__func__);
  //cout << "r = " << r << endl;
  typesystem_t const &typesystem = typesystem_hierarchy.at(t);
  //use_types_t use_types = typesystem.get_use_types();
  itable_t itable = make_itable_with_single_dst_iseq(dst_iseq, dst_iseq_len, src_iseq, src_iseq_len, live_out, in_tmap, out_tmaps, num_live_out/*, use_types*/, typesystem/*r, s*/);
  rand_states_t *rand_states_cp = new rand_states_t(rand_states.rand_states_copy());

  rand_states_cp->make_dst_stack_regnum_point_to_stack_top(itable.get_fixed_reg_mapping());

  ASSERT(itable.get_num_entries() <= 1);
  size_t num_states = rand_states_cp->get_num_states();
  typestate_t *ts_init = new typestate_t[num_states];
  typestate_t *ts_fin = new typestate_t[num_states];
  for (size_t i = 0; i < num_states; i++) {
    typestate_copy(&ts_init[i], &typestate_initial[i]);
    typestate_copy(&ts_fin[i], &typestate_final[i]);
  }
  typestates_set_stack_regnum_to_bottom(ts_init, ts_fin, num_states, itable.get_fixed_reg_mapping());
  convert_addr_to_intaddr_t convert_addr_to_intaddr = itable.get_typesystem().get_convert_addr_to_intaddr();
  make_non_poly_copyable_t make_non_poly_copyable = itable.get_typesystem().get_make_non_poly_copyable();
  set_poly_bytes_in_non_poly_regs_to_bottom_t set_poly_bytes_in_non_poly_regs_to_bottom = itable.get_typesystem().get_set_poly_bytes_in_non_poly_regs_to_bottom();

  if (convert_addr_to_intaddr) {
    typestates_convert_addr_datatype_to_intaddr_datatype(ts_init, num_states);
    typestates_convert_addr_datatype_to_intaddr_datatype(ts_fin, num_states);
  }
  if (make_non_poly_copyable) {
    typestates_make_non_poly_copyable(ts_init, num_states);
  }
  if (set_poly_bytes_in_non_poly_regs_to_bottom) {
    typestates_set_poly_bytes_in_non_poly_regs_to_bottom(ts_init, num_states);
    typestates_set_poly_bytes_in_non_poly_regs_to_bottom(ts_fin, num_states);
  }

  jtable_t jtable;
  jtable_init(&jtable, &itable, NULL, num_states);
  itable_position_t itable_start_pos;
  itable_position_copy(&itable_start_pos, itable_position_zero());

  src_fingerprintdb_elem_t **fdb;
  fdb = src_fingerprintdb_init();
  ASSERT(fdb);

  src_fingerprintdb_add_variants(fdb, src_iseq, src_iseq_len, live_out,
      in_tmap, out_tmaps, num_live_out,
      /*use_types, */ts_init,
      ts_fin, mem_live_out, NULL, "", *rand_states_cp);

  //cout << "src_iseq:\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
  //cout << "live_out:\n" << regset_to_string(live_out, as1, sizeof as1) << endl;
  //cout << "tmaps:\n" << transmaps_to_string(in_tmap, out_tmaps, num_live_out, as1, sizeof as1) << endl;
  //cout << "ts_init[0]:\n" << typestate_to_string(&ts_init[0], as1, sizeof as1) << endl;
  //cout << "ts_fin[0]:\n" << typestate_to_string(&ts_fin[0], as1, sizeof as1) << endl;

  uint64_t yield_secs = 3600;
  long long max_costs[dst_num_costfns];
  src_fingerprintdb_compute_max_costs(fdb, max_costs);

  //DBG_SET(GEN_SANDBOXED_ISEQ, 1);
  //DBG_SET(EQUIV, 3);
  //DBG_SET(JTABLE, 4);
  //DBG_SET(SUPEROPTDB, 2);
  regset_t dst_touch = transmaps_get_used_dst_regs(in_tmap, out_tmaps, num_live_out);

  bool found = itable_enumerate(&itable, &jtable, fdb, &itable_start_pos, max_costs, yield_secs, itable_stopping_cond_t("L1"), false, ts_init, ts_fin, dst_touch, NULL, *rand_states_cp);
  jtable_free(&jtable);
  delete [] ts_init;
  delete [] ts_fin;
  delete rand_states_cp;
  return found;
}

static bool
typecheck_dst_iseq_against_typestates(dst_insn_t const *dst_iseq, long dst_iseq_len, src_insn_t const *src_iseq, long src_iseq_len, regset_t const *live_out, transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_live_out, bool mem_live_out, fixed_reg_mapping_t const &dst_fixed_reg_mapping, rand_states_t const &rand_states, typestate_t const *typestate_initial, typestate_t const *typestate_final, string comment, vector<typesystem_t> const &typesystem_hierarchy, int tregs, int tslots, bool exact_counts)
{
  autostop_timer func_timer(__func__);
  set<typesystem_id_t> dominators; 
  for (typesystem_id_t t = 0; t < typesystem_hierarchy.size(); t++) {
    const auto &typesystem = typesystem_hierarchy.at(t);
    if (typesystem_t::typesystem_is_dominated(typesystem_hierarchy, dominators, t)) {
      continue;
    }
    if (tregs < typesystem.get_tmp_reg_count()) {
      continue;
    }
    if (tslots < typesystem.get_tmp_stackslot_count()) {
      continue;
    }
    if (exact_counts && tregs != typesystem.get_tmp_reg_count()) {
      continue;
    }
    if (exact_counts && tslots != typesystem.get_tmp_stackslot_count()) {
      continue;
    }
    if (typecheck_dst_iseq_against_typestates_for_config(dst_iseq, dst_iseq_len, src_iseq, src_iseq_len, live_out, in_tmap, out_tmaps, num_live_out, mem_live_out, dst_fixed_reg_mapping, rand_states, typestate_initial, typestate_final, typesystem_hierarchy, t)) {
      cout << __func__ << " " << __LINE__ << ": typecheck passed for (" << typesystem.typesystem_to_string() << ")\n";
      dominators.insert(t);
    } else {
      cout << __func__ << " " << __LINE__ << ": typecheck failed for (" << typesystem.typesystem_to_string() << ")\n";
    }
  }
  if (dominators.size() == 0) {
    cout << comment << ": Typecheck failed for all typesystem configurations!\n";
    return false;
  } else {
    typesystem_t::typesystem_hierarchy_print_matrix_using_dominators(typesystem_hierarchy, dominators);
    return true;
  }
  //ASSERT(dominators.size() > 0);
}

static bool
typecheck(src_insn_t const *src_iseq, long src_iseq_len, transmap_t const *in_tmap,
    transmap_t const *out_tmap, dst_insn_t const *dst_iseq, long dst_iseq_len,
    regset_t const *live_out, long num_live_out, bool mem_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping, rand_states_t const &rand_states,
    string comment, int tregs, int tslots, bool exact_counts)
{
  autostop_timer func_timer(__func__);
  size_t num_states = rand_states.get_num_states();
  typestate_t *typestate_initial = new typestate_t[num_states];
  ASSERT(typestate_initial);
  typestate_t *typestate_final = new typestate_t[num_states];
  ASSERT(typestate_final);

  CPP_DBG_EXEC(TYPECHECK, cout << __func__ << " " << __LINE__ << ": src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl);

  //cout << __func__ << " " << __LINE__ << ": generating src typestates. num_states = " << num_states << "\n";
  src_iseq_compute_typestate_using_transmaps(typestate_initial, typestate_final, src_iseq, src_iseq_len, live_out, in_tmap, out_tmap, num_live_out, mem_live_out, rand_states);
  vector<typesystem_t> typesystem_hierarchy = typesystem_t::get_typesystem_hierarchy_for_typecheck();

  //cout << __func__ << " " << __LINE__ << ": checking dst_iseq against typestates\n";
  CPP_DBG_EXEC(TYPECHECK, cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);
  bool ret = typecheck_dst_iseq_against_typestates(dst_iseq, dst_iseq_len, src_iseq, src_iseq_len, live_out, in_tmap, out_tmap, num_live_out, mem_live_out, dst_fixed_reg_mapping, rand_states, typestate_initial, typestate_final, comment, typesystem_hierarchy, tregs, tslots, exact_counts);
  //cout << __func__ << " " << __LINE__ << ": done checking dst_iseq against typestates\n";
  delete [] typestate_initial;
  delete [] typestate_final;
  return ret;
}

static void
peep_entry_typecheck(struct peep_entry_t const *e, rand_states_t *rand_states, int tregs, int tslots, string const &failures_outfilename, bool exact_counts)
{
  autostop_timer func_timer(__func__);
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
  bool alloced_rand_states = false;
  stringstream ss;
  ss << "peep_entry.linenum." << e->peep_entry_get_line_number();

#if ARCH_SRC == ARCH_ETFG
  if (   src_iseq[0].m_comment.find("call_") == 0
      || src_iseq[0].m_comment == "alloca:1"
      || src_iseq[0].m_comment == "ret0:1"
      || src_iseq[0].m_comment == "ret1:1"
     ) {
    cout << __func__ << " " << __LINE__ << ": ignoring iseqs with call/ret: " << src_iseq[0].m_comment << "\n";
    return; 
  }
  cout << __func__ << " " << __LINE__ << ": typechecking " << src_iseq[0].m_comment << endl;
#endif

  transmap_t *out_tmap_renamed = new transmap_t[num_live_out];
  dst_insn_t *dst_iseq_canon = new dst_insn_t[dst_iseq_len];
  regset_t live_out_renamed[num_live_out];
  transmap_t in_tmap_renamed;
  regmap_t dst_regmap;

  dst_iseq_copy(dst_iseq_canon, dst_iseq, dst_iseq_len);

  dst_iseq_rename_fixed_regs(dst_iseq_canon, dst_iseq_len, dst_fixed_reg_mapping, fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_at_end());

  dst_iseq_canonicalize_vregs(dst_iseq_canon, dst_iseq_len, &dst_regmap, fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_at_end_var_to_var());
  transmap_copy(&in_tmap_renamed, in_tmap);
  transmaps_copy(out_tmap_renamed, out_tmap, num_live_out);
  transmaps_rename_using_dst_regmap(&in_tmap_renamed, out_tmap_renamed, num_live_out, &dst_regmap, NULL);
  fixed_reg_mapping_t dst_fixed_reg_mapping_renamed = dst_fixed_reg_mapping.fixed_reg_mapping_rename_using_regmap(dst_regmap);
  for (size_t i = 0; i < num_live_out; i++) {
    regset_rename(&live_out_renamed[i], &live_out[i], &dst_regmap);
  }

  bool rand_states_found = false;
  if (!rand_states) {
    rand_states = new rand_states_t;
    rand_states_found = rand_states->init_rand_states_using_src_iseq(src_iseq, src_iseq_len, in_tmap_renamed, out_tmap_renamed, live_out_renamed, num_live_out);
    alloced_rand_states = true;
  }

  CPP_DBG_EXEC(TYPECHECK,
      cout << __func__ << " " << __LINE__ << ": src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
      cout << __func__ << " " << __LINE__ << ": dst_iseq_canon =\n" << dst_iseq_to_string(dst_iseq_canon, dst_iseq_len, as1, sizeof as1) << endl;
      cout << __func__ << " " << __LINE__ << ": in_tmap_renamed =\n" << transmap_to_string(&in_tmap_renamed, as1, sizeof as1) << endl;
      for (int i = 0; i < num_live_out; i++) {
        cout << __func__ << " " << __LINE__ << ": out_tmap_renamed[" << i << "] =\n" << transmap_to_string(&out_tmap_renamed[i], as1, sizeof as1) << endl;
      }
      regsets_to_string(live_out, num_live_out, as1, sizeof as1);
      cout << __func__ << " " << __LINE__ << ": live_out =\n" << as1 << endl;
      cout << __func__ << " " << __LINE__ << ": mem_live_out = " << mem_live_out << "\n";
      cout << __func__ << " " << __LINE__ << ": dst_fixed_reg_mapping_renamed =\n" << dst_fixed_reg_mapping_renamed.fixed_reg_mapping_to_string() << "\n";
  );
  bool pass = false;

  if (rand_states_found) {
    pass = typecheck(src_iseq, src_iseq_len, &in_tmap_renamed, out_tmap_renamed, dst_iseq_canon, dst_iseq_len, live_out, num_live_out, mem_live_out, dst_fixed_reg_mapping_renamed, *rand_states, ss.str(), tregs, tslots, exact_counts);
  } else {
    cout << __func__ << " " << __LINE__ << ": No Random input state found, typecheck failed \n";
  }

  if (alloced_rand_states) {
    delete rand_states;
  }
  delete [] out_tmap_renamed;
  delete [] dst_iseq_canon;

  if (!pass) {
    FILE *fp = fopen(failures_outfilename.c_str(), "a");
    ASSERT(fp);
    peeptab_entry_write(fp, e, NULL);
    fclose(fp);
  }
  /*if (!pass) {
    cout << __func__ << " " << __LINE__ << ": FAILED typecheck of peep_entry\n";
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
  return pass;*/
}

int
main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> peeptab_name(cl::explicit_prefix, "t", SUPEROPTDBS_DIR "fb.trans.tab", "peeptab name to be verified");
  cl::arg<string> rand_states_filename(cl::explicit_prefix, "r", "", "filename with rand states");
  cl::arg<string> failures(cl::explicit_prefix, "fail", "", "filename where typecheck failures are written as a peeptab");
  cl::arg<int> tregs(cl::explicit_prefix, "tregs", typesystem_t::max_num_temp_regs_for_typecheck, "maximum number of tmp regs");
  cl::arg<int> tslots(cl::explicit_prefix, "tslots", typesystem_t::max_num_stack_slots_for_typecheck, "maximum number of tmp stack slots");
  cl::arg<bool> exact_counts_flag(cl::explicit_prefix, "exact-counts", false, "Treat tregs and tslots as exact counts (and not upper-bounds); default: false");
  cl::cl cmd("typecheck: typechecks the translations in a peeptab");
  cmd.add_arg(&peeptab_name);
  cmd.add_arg(&failures);
  cmd.add_arg(&rand_states_filename);
  cmd.add_arg(&tregs);
  cmd.add_arg(&tslots);
  cmd.add_arg(&exact_counts_flag);
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=insn_match=2,smt_query=1");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  eqspace::init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, eqspace::print_debug_class_levels());

  malloc32(1); //allocate one byte to initialize the malloc32 structures in the beginning; late initialization can cause memory errors because of already existing heap mappings at the desired addresses
  if (failures.get_value() == "") {
    stringstream ss;
    ss << peeptab_name.get_value() << ".typecheck_fail.tregs" << tregs.get_value() << ".tslots" << tslots.get_value();
    failures.set_value(ss.str());
  }

  init_timers();
  cmd_helper_init();
  cout << "Typechecking " << peeptab_name.get_value() << ", outputting failures to " << failures.get_value() << endl;

  g_query_dir_init();

  g_ctx_init();
  /*g_helper_pid = */eqspace::solver_init();
  context *ctx = g_ctx;
  context::config cfg(3600, 3600);
  cfg.DISABLE_CACHES = true;
  ctx->set_config(cfg);


  src_init();
  dst_init();

  g_se_init();

  usedef_init();
  types_init();
  equiv_init();

  disable_signals();

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

  rand_states_t *rand_states = NULL;
  if (rand_states_filename.get_value() != "") {
    ifstream ifs;
    ifs.open(rand_states_filename.get_value());
    rand_states = rand_states_t::read_rand_states(ifs);
    ifs.close();
  }

  //init_typesystem_hierarchy();

  bool failure_found = false;
  for (long i = 0; i < ti->peep_table.hash_size; i++) {
    for (auto iter = ti->peep_table.hash[i].begin(); iter != ti->peep_table.hash[i].end(); iter++) {
      struct peep_entry_t const *e = *iter;
      peep_entry_typecheck(e, rand_states, tregs.get_value(), tslots.get_value(), failures.get_value(), exact_counts_flag.get_value());
    }
  }
  CPP_DBG_EXEC(STATS,
    print_all_timers();
    g_stats_print();
    cout << ctx->stat() << endl;
  );

  solver_kill();
  call_on_exit_function();

  return 0;
}
