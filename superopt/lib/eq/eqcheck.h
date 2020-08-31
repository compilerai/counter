#pragma once
#ifndef EQCHECKEQ_H
#define EQCHECKEQ_H

#include <sys/time.h>
#include <sys/resource.h>
#include "tfg/tfg.h"
#include "expr/expr.h"
#include "expr/local_sprel_expr_guesses.h"
#include "gsupport/rodata_map.h"
#include "expr/consts_struct.h"
#include "valtag/fixed_reg_mapping.h"
#include "eq/quota.h"
#include "support/src-defs.h"
#include "graph/memlabel_assertions.h"

namespace eqspace {
class corr_graph;

class eqcheck
{
public:
  eqcheck(string const& proof_filename, string const &function_name, shared_ptr<tfg> const& src_tfg, shared_ptr<tfg> const& dst_tfg, fixed_reg_mapping_t const &fixed_reg_mapping, rodata_map_t const &rodata_map, vector<dst_insn_t> const& dst_iseq, vector<dst_ulong> const& dst_insn_pcs, context* ctx/*, vector<memlabel_ref> const& relevant_memlabels*/, bool llvm2ir, const quota& max_quota);

  eqcheck(istream& is, shared_ptr<tfg> const& src_tfg, shared_ptr<tfg> const& dst_tfg);

  void eqcheck_to_stream(ostream& os);

  static shared_ptr<corr_graph const> check_eq(shared_ptr<eqcheck> eq, local_sprel_expr_guesses_t const& lsprel_assumes = local_sprel_expr_guesses_t(), string const &check_filename = "");
  bool check_proof(vector<memlabel_ref> const &relevant_memlabels, const string& proof_file);
  //void clear_fcall_side_effects() { m_fcall_side_effects = false; }

  tfg const& get_src_tfg() const;
  //tfg& get_src_tfg();
  tfg const& get_dst_tfg() const;
  //tfg& get_dst_tfg();

  memlabel_assertions_t const& get_memlabel_assertions() const;

  int get_stackpointer_gpr() const
  {
    exreg_id_t ret = m_fixed_reg_mapping.get_mapping(DST_EXREG_GROUP_GPRS, DST_STACK_REGNUM);
    ASSERT(ret != -1);
    return ret;
  }

  rodata_map_t const &get_rodata_map() const
  {
    return m_rodata_map;
  }

  context* get_context() const { return m_ctx; }

  const quota& get_quota() const;
  quota& get_quota();
  local_sprel_expr_guesses_t const &get_local_sprel_expr_guesses() { return m_local_sprel_expr_guesses; }
  /*map<graph_loc_id_t, graph_loc_id_t> get_base_loc_id_map_for_dst_locs() const { return m_base_loc_id_map_for_dst_locs; }
  graph_loc_id_t get_base_loc_id_for_dst_loc(graph_loc_id_t dst_loc) const
  {
    if (m_base_loc_id_map_for_dst_locs.count(dst_loc)) {
      return m_base_loc_id_map_for_dst_locs.at(dst_loc);
    } else {
      return dst_loc;
    }
  }
  void init_base_loc_id_map_for_dst_locs();
  void init_dst_locs_assigned_to_constants_map();
  map<graph_loc_id_t, mybitset> const &get_dst_locs_assigned_to_constants_map(pc const &p) { return m_dst_locs_assigned_to_constants_map.at(p); } */

  long get_global_timeout() const
  {
    return m_max_quota.get_total_timeout();
  }
  float get_memory_threshold_gb() const { return m_max_quota.get_memory_threshold_gb(); }
  string const &get_proof_filename() const { return m_proof_filename; }
  string const &get_function_name() const { return m_function_name; }
  //vector<memlabel_ref> const& get_relevant_memlabels() const { return m_relevant_memlabels; }
  //corr_graph corr_graph_read_from_file(string const& proof_filename) const;
  vector<dst_insn_t> const& get_dst_iseq() const { return m_dst_iseq; }
  vector<dst_ulong> const& get_dst_insn_pcs() const { return m_dst_insn_pcs; }
  graph_symbol_map_t const& get_symbol_map() const { return m_dst_tfg->get_symbol_map(); }
  static bool istream_position_at_function_name(istream& in, string const& function_name);
  static string proof_file_identify_failed_function_name(string const& proof_file);
  static set<string> read_existing_proof_file(string const& existing_proof_file);

private:
  static bool local_size_is_compatible_with_sprel_expr(size_t local_size, expr_ref sprel_expr, consts_struct_t const &cs);
  void compute_local_sprel_expr_guesses();
  void collapse_dst_tfg();
  //void preprocess_tfgs();
  void set_quota();

private:
  string m_proof_filename;
  string m_function_name;
  shared_ptr<tfg> m_src_tfg;
  shared_ptr<tfg> m_dst_tfg;
  fixed_reg_mapping_t m_fixed_reg_mapping;
  rodata_map_t m_rodata_map;

  vector<dst_insn_t> m_dst_iseq;
  vector<dst_ulong> m_dst_insn_pcs;

  context* m_ctx;

  quota m_max_quota;
  //bool m_fcall_side_effects;
  local_sprel_expr_guesses_t m_local_sprel_expr_guesses;
  consts_struct_t const &m_cs;
  //vector<memlabel_ref> m_relevant_memlabels;
  memlabel_assertions_t m_memlabel_assertions;
  bool m_llvm2ir;
  /*map<graph_loc_id_t, graph_loc_id_t> m_base_loc_id_map_for_dst_locs;
  map<pc, map<graph_loc_id_t, mybitset>> m_dst_locs_assigned_to_constants_map;*/
};

}

#endif
