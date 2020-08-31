#include <map>
#include "i386/regs.h"
#include "ppc/regs.h"
#include "eqcheck.h"
#include "correlate.h"
#include "support/sig_handling.h"
#include "config-host.h"
#include "support/log.h"
#include "tfg/predicate.h"
#include "tfg/avail_exprs.h"
#include "expr/z3_solver.h"
#include "eq/cg_with_inductive_preds.h"
#include "eq/cg_with_relocatable_memlabels.h"
#include "eq/cg_with_safety.h"
#include "eq/cg_with_dst_ml_check.h"
#include "tfg/parse_input_eq_file.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "rewrite/dst-insn.h"

#define DEBUG_TYPE "eqcheck"

bool g_correl_locals_consider_all_local_variables = false;

static char as1[409600];

namespace eqspace {

// TODO we are not cleaning properly here, which sometimes does not exit, probably we need to kill the helperprocess
context* g_ctx_for_sig_handler;
static void sig_handler(int signum)
{
  cout << __func__ << " " << __LINE__ << ": received signal " << signum << endl;
  LOG(stats::get());
  LOG("eq-timeout:\n" << endl);
  cout << __func__ << " " << __LINE__ << ": calling call_on_exit_function()" << endl;
  solver_kill();
  call_on_exit_function();
  cout << __func__ << " " << __LINE__ << ": done call_on_exit_function()" << endl;
  clean_query_files();
  cout << __func__ << " " << __LINE__ << ": clean_query_files() done" << endl;
  g_ctx_for_sig_handler->~context();
  cout << __func__ << " " << __LINE__ << ": context destructor called" << endl;
  exit(0);
}

static void sig_handler_int(int signum)
{
  cout << __func__ << " " << __LINE__ << ": received signal " << signum << endl;
  LOG(stats::get());
  LOG("eq-int:\n" << endl);
  cout << __func__ << " " << __LINE__ << ": calling call_on_exit_function()" << endl;
  solver_kill();
  call_on_exit_function();
  cout << __func__ << " " << __LINE__ << ": done call_on_exit_function()" << endl;
  g_ctx_for_sig_handler->~context();
  //clean_query_files();
  exit(0);
}

eqcheck::eqcheck(string const& proof_filename, string const &function_name, shared_ptr<tfg> const & src_tfg, shared_ptr<tfg> const & dst_tfg, fixed_reg_mapping_t const &fixed_reg_mapping, rodata_map_t const &rodata_map, vector<dst_insn_t> const& dst_iseq, vector<dst_ulong> const& dst_insn_pcs, context* ctx/*, memlabel_assertions_t const& mlasserts*/, bool llvm2ir, const quota& max_quota) :
				m_proof_filename(proof_filename), m_function_name(function_name), m_src_tfg(src_tfg), m_dst_tfg(dst_tfg), m_fixed_reg_mapping(fixed_reg_mapping), m_rodata_map(rodata_map), m_dst_iseq(dst_iseq), m_dst_insn_pcs(dst_insn_pcs), m_ctx(ctx), m_max_quota(max_quota), m_cs(ctx->get_consts_struct()), m_memlabel_assertions(m_dst_tfg->get_symbol_map(), m_src_tfg->get_locals_map(), m_src_tfg->get_argument_regs(), rodata_map, m_local_sprel_expr_guesses, ctx), m_llvm2ir(llvm2ir)
{
  //cout << __func__ << " " << __LINE__ << ": m_src_tfg:\n" << m_src_tfg.graph_to_string() << endl;
  sig_handling sh;
  g_ctx_for_sig_handler = ctx;
  //sh.set_alarm_handler(m_max_quota.get_total_timeout(), &sig_handler);
  sh.set_int_handler(&sig_handler_int);

  if (ctx->get_config().enable_local_sprel_guesses) {
    compute_local_sprel_expr_guesses();
  }
}

eqcheck::eqcheck(istream& is, shared_ptr<tfg> const& src_tfg, shared_ptr<tfg> const& dst_tfg) : m_src_tfg(src_tfg), m_dst_tfg(dst_tfg), m_ctx(m_src_tfg->get_context()), m_cs(m_ctx->get_consts_struct()), m_memlabel_assertions(m_ctx)
{
  string line;
  bool end;

  end = !getline(is, line);
  ASSERT(!end);
  ASSERT(line == "=eqcheck");
  end = !getline(is, line);
  ASSERT(!end);
  string const prefix_proof_filename = "=proof_filename ";
  ASSERT(string_has_prefix(line, prefix_proof_filename));
  m_proof_filename = line.substr(prefix_proof_filename.length());
  string const prefix_function_name = "=function_name ";
  end = !getline(is, line);
  ASSERT(!end);
  ASSERT(string_has_prefix(line, prefix_function_name));
  m_fixed_reg_mapping.fixed_reg_mapping_from_stream(is);
  m_rodata_map = rodata_map_t(is);
  end = !getline(is, line);
  ASSERT(!end);
  ASSERT(line == "=dst_iseq");
  m_dst_iseq = dst_iseq_from_stream(is);
  end = !getline(is, line);
  ASSERT(!end);
  ASSERT(line == "=dst_insn_pcs");
  m_dst_insn_pcs = dst_insn_pcs_from_stream(is);
  //m_relevant_memlabels = m_ctx->relevant_memlabels_from_stream(is);
  //m_memlabel_assertions = memlabel_assertions_t(dst_tfg->get_symbol_map(), src_tfg->get_locals_map(), m_rodata_map, m_local_sprel_expr_guesses, m_ctx);
  m_memlabel_assertions = memlabel_assertions_t(is, m_ctx);
  end = !getline(is, line);
  ASSERT(!end);
  string const prefix_llvm2ir = "=llvm2ir ";
  ASSERT(string_has_prefix(line, prefix_llvm2ir));
  m_llvm2ir = string_to_int(line.substr(prefix_llvm2ir.length()));
  end = !getline(is, line);
  ASSERT(!end);
  ASSERT(line == "=eqcheck done");
}

shared_ptr<corr_graph const>
eqcheck::check_eq(shared_ptr<eqcheck> eq, local_sprel_expr_guesses_t const& lsprel_assumes, string const &check_filename)
{
  autostop_timer ft("total-equiv");

  stats::get().set_value("eq-state", "start");
  DYN_DEBUG(dumptfg, cout << __func__ << " " << __LINE__ << ": SRC\n" << eq->m_src_tfg->graph_to_string() << endl);
  DYN_DEBUG(dumptfg, cout << __func__ << " " << __LINE__ << ": DST\n" << eq->m_dst_tfg->graph_to_string() << endl);
  
  MSG("Computing equivalence of the two TFGs (LLVM IR and x86 assembly)...");
  DYN_DEBUG2(oopsla_log, cout << "Printing SRC TFG (C) \n" << eq->m_src_tfg->graph_to_string()  << endl);
  DYN_DEBUG2(oopsla_log, cout << "Printing DST TFG (A) \n" << eq->m_dst_tfg->graph_to_string()  << endl);

  //stats::get().set_value("fcall-side-effects: ", m_fcall_side_effects);
//  stats::get().set_value("max_quota: ", eq->m_max_quota.to_string());
  stats::get().set_value("src-edges", eq->m_src_tfg->get_edge_count());
  stats::get().set_value("src-nodes", eq->m_src_tfg->get_node_count());
  stats::get().set_value("dst-edges", eq->m_dst_tfg->get_edge_count());
  stats::get().set_value("dst-nodes", eq->m_dst_tfg->get_node_count());

  correlate corr(eq);

  shared_ptr<corr_graph const> cg;
  cg = corr.find_correlation(lsprel_assumes, check_filename);
  if (!cg) {
    //DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": correlation check failed\n");
    cout << __func__ << " " << __LINE__ << ": correlation check failed\n";
    if (eq->m_proof_filename != "") {
      corr_graph::write_failure_to_proof_file(eq->m_proof_filename, eq->m_function_name);
    }
    return nullptr;
  }

  CPP_DBG_EXEC(STATS,
    // add stats for CEs
    stats::get().add_counter(string("# of edges in Product-CFG"), cg->get_edges().size() );
    stats::get().add_counter(string("# of nodes in Product-CFG"), cg->get_all_pcs().size() );
    for (auto const& pp : cg->get_all_pcs()) {
      if (pp.is_start() || pp.is_exit())
      continue; // skip start and exit since they do not store any CEs
      stats::get().add_counter(string("Counter-examples-Total-at-") + pp.to_string(), cg->get_counterexamples_at_pc(pp).size());
//      stats::get().add_counter(string("Counter-examples-Generated-at-") + pp.to_string(), cg->get_counterexamples_generated_at_pc(pp)); These are the number of CEs actually generated by solver but some of them may not increase the rank so may not get added
      stats::get().add_counter(string("Counter-examples-Generated-at-") + pp.to_string(), cg->get_counterexamples_generated_added_at_pc(pp));
    }
//    cout << __func__ << ": Dumping CG path stats\n";
//    for (auto const& cge : cg->get_edges()) {
//      shared_ptr<tfg_edge_composition_t> const& src_ec = cge->get_src_edge();
//      shared_ptr<tfg_edge_composition_t> const& dst_ec = cge->get_dst_edge();
//      cout << __func__ << ':' << edge_and_path_stats_string_for_edge_composition(src_ec) << endl;
//      cout << __func__ << ':' << edge_and_path_stats_string_for_edge_composition(dst_ec) << endl;
//    }
//    cout << __func__ << ": Finished CG path stats dump\n";
  );

  shared_ptr<cg_with_inductive_preds> cgi = make_shared<cg_with_inductive_preds>(*cg);
  //DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": checking equivalence proof\n");
  cout << __func__ << " " << __LINE__ << ": checking equivalence proof\n";
  if (!cgi->check_equivalence_proof()) {
    //DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": equivalence proof check failed!\n");
    cout << __func__ << " " << __LINE__ << ": equivalence proof check failed!\n";
    if (eq->m_proof_filename != "") {
      corr_graph::write_failure_to_proof_file(eq->m_proof_filename, eq->m_function_name);
    }
    return nullptr;
  }

  shared_ptr<cg_with_relocatable_memlabels> cgr = make_shared<cg_with_relocatable_memlabels>(*cgi);
  //DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": checking equivalence proof\n");
//  cout << __func__ << " " << __LINE__ << ": checking equivalence proof with relocatable memlabels\n";
//  if (!cgr->check_equivalence_proof()) {
//    //DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": equivalence proof check failed!\n");
//    cout << __func__ << " " << __LINE__ << ": equivalence proof check with relocatable memlabels failed!\n";
//    if (eq->m_proof_filename != "") {
//      corr_graph::write_failure_to_proof_file(eq->m_proof_filename, eq->m_function_name);
//    }
//    return nullptr;
//  }

  //DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": checking CG safety\n");
  cout << __func__ << " " << __LINE__ << ": checking CG safety\n";
  shared_ptr<cg_with_safety> cgs = make_shared<cg_with_safety>(*cgr);
  if (!cgs->check_safety()) {
    DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": Safety check failed on CG!\n");
    cout << __func__ << " " << __LINE__ << ": Safety check failed on CG!\n";
    MSG("Safety check failed on CG!\n");
    if (eq->m_proof_filename != "") {
      corr_graph::write_failure_to_proof_file(eq->m_proof_filename, eq->m_function_name);
    }
    return nullptr;
  }
  MSG("Safety check passed on CG\n");

  shared_ptr<cg_with_dst_ml_check> cgd = make_shared<cg_with_dst_ml_check>(*cgs);
  //DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": checking DST TFG memlabels\n");
  cout << __func__ << " " << __LINE__ << ": checking DST TFG memlabels\n";
  if (!cgd->check_dst_mls()) {
    //DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": DST TFG memlabel check failed on CG!\n");
    cout << __func__ << " " << __LINE__ << ": DST TFG memlabel check failed on CG!\n";
    MSG("DST Memlabel check failed on CG\n");
    if (eq->m_proof_filename != "") {
      corr_graph::write_failure_to_proof_file(eq->m_proof_filename, eq->m_function_name);
    }
    return nullptr;
  }
  MSG("DST Memlabel check passed on CG\n");

  if (eq->m_proof_filename != "") {
    cgd->write_provable_cg_to_proof_file(eq->m_proof_filename, eq->m_function_name);
  }

  return cgd;
}

#if 0
bool eqcheck::check_proof(vector<memlabel_t> const &relevant_memlabels, const string& proof_file)
{
//  collapse_dst_tfg();
//  m_src_tfg.remove_trivial_nodes();
//  preprocess_tfgs();
  //preprocess_tfgs();
  //INFO("SRC-preprocessed2:\n" << m_src_tfg.tfg_to_string(true) << endl);
  //INFO("DST-preprocessed:\n" << m_dst_tfg.tfg_to_string(true) << endl);
  shared_ptr<corr_graph const> cg = corr_graph::read_from_proof_file(proof_file, this);
  ASSERT(cg->cg_is_well_formed());

  //cg->houdini_check_all_nodes();
  //INFO("Correlation Graph:\n" << cg->cg_to_string(true, "") << endl);
  bool res = cg->cg_is_provable();
  //LOG(stats::get());
  cg->cg_print_stats();
  return res;
}
#endif

tfg const& eqcheck::get_src_tfg() const
{
  return *m_src_tfg;
}

/*tfg& eqcheck::get_src_tfg()
{
  return m_src_tfg;
}*/

tfg const&
eqcheck::get_dst_tfg() const
{
  return *m_dst_tfg;
}

/*tfg& eqcheck::get_dst_tfg()
{
  return m_dst_tfg;
}*/

/*void eqcheck::collapse_dst_tfg()
{
  stats::get().set_value("eq-state", "collapse_dst_tfg");
  m_dst_tfg.to_dot("dst-tfg-uncollapsed.dot", true);
  m_dst_tfg.collapse_tfg();
  DEBUG("DST-collapsed:\n" << m_dst_tfg.to_string(true) << "\n");
}
*/

/*void eqcheck::preprocess_tfgs()
{
  autostop_timer func_timer(__func__);
  stats::get().set_value("eq-state", "src_tfg.preprocess");
  //INFO("SRC-preprocessed0:\n" << m_src_tfg.tfg_to_string(true) << endl);
  m_src_tfg.preprocess();
  //INFO("SRC-preprocessed1:\n" << m_src_tfg.tfg_to_string(true) << endl);
  stats::get().set_value("eq-state", "dst_tfg.preprocess");
  m_dst_tfg.preprocess();
  //m_ctx->set_theorems(m_src_tfg.get_all_cps(), m_dst_tfg.get_all_cps());
}*/

void eqcheck::set_quota()
{
  context::config cfg = m_ctx->get_config();
  cfg.smt_timeout_secs = m_max_quota.get_smt_query_timeout();
  cfg.sage_timeout_secs = m_max_quota.get_sage_query_timeout();
  m_ctx->set_config(cfg);
}

const quota& eqcheck::get_quota() const
{
  return m_max_quota;
}

quota& eqcheck::get_quota()
{
  return m_max_quota;
}

bool
eqcheck::local_size_is_compatible_with_sprel_expr(size_t local_size, expr_ref sprel_expr, consts_struct_t const &cs)
{
  context *ctx = sprel_expr->get_context();
  expr_ref sp = cs.get_input_stack_pointer_const_expr();
  ASSERT(sp->is_bv_sort());
  expr_vector sp_atoms, sprel_expr_atoms, max_local_expr_atoms;
  sprel_expr_atoms = get_arithmetic_addsub_atoms(sprel_expr);
  sp_atoms = get_arithmetic_addsub_atoms(sp);
  arithmetic_addsub_atoms_pair_minimize(sp_atoms, sprel_expr_atoms, ctx);
  if (   sp_atoms.size() == 0
      && sprel_expr_atoms.size() == 1
      && sprel_expr_atoms.at(0)->is_const()
      && sprel_expr_atoms.at(0)->get_int_value() > 0) {
    return true; //sprel_expr is greater than sp; is_compatible.
  }
  if (   sp_atoms.size() == 1
      && sprel_expr_atoms.size() == 1
      && sprel_expr_atoms.at(0)->is_const()
      && sp_atoms.at(0)->get_int_value() == 0
      && sprel_expr_atoms.at(0)->get_int_value() > 0) {
    return true; //sprel_expr is greater than sp; is_compatible.
  }

  //sprel_expr is less than sp
  expr_ref max_local_expr = ctx->mk_bvadd(sp, ctx->mk_bv_const(sp->get_sort()->get_size(), -(int)local_size));
  //max_local_expr should be >= sprel_expr for compatibility

  //cout << __func__ << " " << __LINE__ << ": max_local_expr = " << expr_string(max_local_expr) << endl;
  //cout << __func__ << " " << __LINE__ << ": sprel_expr = " << expr_string(sprel_expr) << endl;

  sprel_expr_atoms = get_arithmetic_addsub_atoms(sprel_expr);
  max_local_expr_atoms = get_arithmetic_addsub_atoms(max_local_expr);
  arithmetic_addsub_atoms_pair_minimize(sprel_expr_atoms, max_local_expr_atoms, ctx);
  if (   sprel_expr_atoms.size() == 0
      && max_local_expr_atoms.size() == 1
      && max_local_expr_atoms.at(0)->is_const()
      && max_local_expr_atoms.at(0)->get_int_value() < 0) {
    return false;
  }
  if (   sprel_expr_atoms.size() == 1
      && max_local_expr_atoms.size() == 0
      && sprel_expr_atoms.at(0)->is_const()
      && sprel_expr_atoms.at(0)->get_int_value() > 0) {
    return false;
  }
  if (   sprel_expr_atoms.size() == 1
      && max_local_expr_atoms.size() == 1
      && max_local_expr_atoms.at(0)->is_const()
      && sprel_expr_atoms.at(0)->is_const()
      && sprel_expr_atoms.at(0)->get_int_value() > max_local_expr_atoms.at(0)->get_int_value()) {
    return false;
  }

  return true;
}

void
eqcheck::compute_local_sprel_expr_guesses()
{
  consts_struct_t const &cs = m_src_tfg->get_consts_struct();
  set<local_id_t> locals;
  //if (g_correl_locals_consider_all_local_variables) {
    for (auto const lp : m_src_tfg->get_locals_map().get_map()) {
      locals.insert(lp.first);
    }
  //} else {
    //locals = m_src_tfg.identify_address_taken_local_variables();
  //}
  set<expr_ref> sprel_exprs = m_dst_tfg->identify_stack_pointer_relative_expressions();
  CPP_DBG_EXEC(EQCHECK,
      cout << __func__ << " " << __LINE__ << ": address taken locals:";
      for (auto local : locals) {
        cout << " " << local;
      }
      cout << endl;
      cout << __func__ << " " << __LINE__ << ": stack pointer relative expressions:";
      for (auto se : sprel_exprs) {
        cout << " " << expr_string(se);
      }
      cout << endl;
  );
  stats::get().set_value("correl_locals.num_locals", locals.size());
  stats::get().set_value("correl_locals.num_sprel_exprs", sprel_exprs.size());

  for (auto local : locals) {
    for (auto se : sprel_exprs) {
      if (local_size_is_compatible_with_sprel_expr(m_src_tfg->get_local_size_from_id(local), se, cs)) {
        //cout << __func__ << " " << __LINE__ << ": adding guess: local " << local << " stack expr " << expr_string(se) << endl;
        m_local_sprel_expr_guesses.add_local_sprel_expr_guess(local, se);
      } else {
        CPP_DBG_EXEC(EQCHECK, cout << __func__ << " " << __LINE__ << ": local_size is not compatible with sprel expr: local " << local << " stack expr " << expr_string(se) << endl);
      }
    }
  }
  DYN_DEBUG(local_sprel_expr, cout << _FNLN_ << ": m_local_sprel_expr_guesses.size() = " << m_local_sprel_expr_guesses.size() << endl);
  DYN_DEBUG2(local_sprel_expr, cout << _FNLN_ << ": m_local_sprel_expr_guesses:\n" << m_local_sprel_expr_guesses.to_string() << endl);
}

/*
void
eqcheck::init_base_loc_id_map_for_dst_locs()
{
  map<graph_loc_id_t, graph_cp_location> const &dst_locs = m_dst_tfg.get_locs();
  for (auto dl : dst_locs) {
    if (dl.second.is_memslot()) {
      for (auto dl2 : dst_locs) {
        if (   dl2.second.is_memslot()
            && dl2.second.m_memname == dl.second.m_memname
            && is_expr_equal_syntactic(dl2.second.m_addr, dl.second.m_addr)
            && dl2.second.m_nbytes > dl.second.m_nbytes
            && (m_base_loc_id_map_for_dst_locs.count(dl.first) && dl2.second.m_nbytes > dst_locs.at(m_base_loc_id_map_for_dst_locs[dl.first]).m_nbytes)) {
          // base loc mapping is from smaller to bigger;
          // If the bigger locid is stable then smaller is automatically assumed to be stable
          m_base_loc_id_map_for_dst_locs[dl.first] = dl2.first;
          //cout << __func__ << " " << __LINE__ << ": base loc_id for dst loc" << dl.first << " is loc" << dl2.first << endl;
        }
      }
    }
  }
}

void
eqcheck::init_dst_locs_assigned_to_constants_map()
{
  m_dst_locs_assigned_to_constants_map = m_dst_tfg.compute_dst_locs_assigned_to_constants_map();
}
*/

void
eqcheck::eqcheck_to_stream(ostream& os)
{
  os << "=eqcheck\n";
  os << "=proof_filename " << m_proof_filename << endl;
  os << "=function_name " << m_function_name << endl;
  m_fixed_reg_mapping.fixed_reg_mapping_to_stream(os);
  m_rodata_map.rodata_map_to_stream(os);
  os << "=dst_iseq\n";
  os << dst_insn_vec_to_string(m_dst_iseq, as1, sizeof as1);
  os << "=dst_iseq done\n";
  os << "=dst_insn_pcs\n";
  dst_insn_pcs_to_stream(os, m_dst_insn_pcs);
  //m_ctx->relevant_memlabels_to_stream(os, m_relevant_memlabels);
  m_memlabel_assertions.memlabel_assertions_to_stream(os);
  os << "=llvm2ir " << m_llvm2ir << endl;
  os << "=eqcheck done\n";
}

memlabel_assertions_t const&
eqcheck::get_memlabel_assertions() const
{
  return m_memlabel_assertions;
}

bool
eqcheck::istream_position_at_function_name(istream& in, string const& function_name)
{
  string fname;
  string line;
  bool end;
  do {
    end = !getline(in, line);
    if (end) {
      return false;
    }
    if (string_has_prefix(line, FUNCTION_NAME_FIELD_IDENTIFIER " ")) {
      fname = line.substr(strlen(FUNCTION_NAME_FIELD_IDENTIFIER " "));
    }
  } while (fname != function_name);
  ASSERT(fname == function_name);
  return true;
}

string
eqcheck::proof_file_identify_failed_function_name(string const& proof_file)
{
  string line;
  bool end;
  ifstream in(proof_file);
  while (getline(in, line)) {
    if (!string_has_prefix(line, FUNCTION_NAME_FIELD_IDENTIFIER " ")) {
      continue;
    }
    string fname = line.substr(strlen(FUNCTION_NAME_FIELD_IDENTIFIER " "));
    end = !getline(in, line);
    ASSERT(!end);
    ASSERT(string_has_prefix(line, PROOF_FILE_RESULT_PREFIX));
    string res = line.substr(strlen(PROOF_FILE_RESULT_PREFIX));
    ASSERT(res == "0" || res == "1");
    if (res == "0") {
      return fname;
    }
  }
  cout << __func__ << " " << __LINE__ << ": there is no failed function in proof file '" << proof_file << "'\n";
  NOT_REACHED();
}

//set<string>
//eqcheck::read_existing_proof_file(string const& existing_proof_file)
//{
//  NOT_IMPLEMENTED();
//}

}
