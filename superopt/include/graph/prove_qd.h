#pragma once

#include <atomic>
#include <thread>

#include "graph/prove.h"

namespace eqspace {

using namespace std;

void prove_qd_init(/*string const& record_filename, string const& replay_filename, string const& stats_filename*/);
pair<proof_status_t, list<counter_example_t>> spawn_qd_query(string filename, context* ctx, std::atomic<bool>& should_return_sharedvar);
void prove_qd_kill();

template<typename T_PRECOND_TFG>
pair<proof_status_t, list<counter_example_t>>
prove_qd(context *ctx, list<guarded_pred_t> const &lhs, precond_t const &precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, expr_ref const &src, expr_ref const &dst, query_comment const &qc/*, bool timeout_res*//*, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map*/, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, relevant_memlabels_t const& relevant_memlabels, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg, std::atomic<bool>& should_return_sharedvar)
{
  stringstream ss;
  static int count = 0;
  string pid = int_to_string(getpid());
  ss << g_query_dir << "/" << PROVE_QD_FILENAME_PREFIX << "." << qc.to_string() << "." << count++;
  string filename = ss.str();
  ofstream fo(filename.data());
  ASSERT(fo);
  output_lhs_set_guard_etc_and_src_dst_to_file(fo, lhs, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, concrete_address_submap, relevant_memlabels, alias_cons, src_tfg);
  fo.close();
  //g_query_files.insert(filename);
  pair<proof_status_t, list<counter_example_t>> ret = spawn_qd_query(filename, ctx, should_return_sharedvar);
  DYN_DEBUG3(prove_debug, cout << __func__ << " " << __LINE__ << ": returned from spawn_qd_query\n");
  if (ret.first == proof_status_timeout) {
    DYN_DEBUG3(prove_debug, cout << __func__ << " " << __LINE__ << ": waiting for should_return_sharedvar to become true\n");
    while (!should_return_sharedvar.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(PROVE_QD_POLL_INTERVAL_IN_MILLISECONDS));
    }
    DYN_DEBUG3(prove_debug, cout << __func__ << " " << __LINE__ << ": should_return_sharedvar became true\n");
  }
  DYN_DEBUG3(prove_debug, cout << __func__ << " " << __LINE__ << ": returning " << proof_status_to_string(ret.first) << endl);
  return ret;
}

}
