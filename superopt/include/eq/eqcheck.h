#pragma once

#include <sys/resource.h>
#include <sys/time.h>

#include "support/src-defs.h"

#include "valtag/fixed_reg_mapping.h"

#include "expr/consts_struct.h"
#include "expr/expr.h"
#include "expr/local_sprel_expr_guesses.h"

#include "gsupport/memlabel_assertions.h"
#include "gsupport/pc_local_sprel_expr_guesses.h"
#include "gsupport/rodata_map.h"

#include "tfg/tfg.h"
#include "tfg/tfg_ssa.h"
#include "tfg/tfg_llvm.h"

#include "x64/insn.h"

#include "eq/failed_cg_set.h"
#include "eq/quota.h"
#include "eq/correl_hints.h"

namespace eqspace {
class corr_graph;
class cg_with_asm_annotation;
class cg_with_failcond;

class eqcheck
{
public:
  eqcheck(string const& proof_filename, string const &function_name, dshared_ptr<tfg_ssa_t> const& src_tfg, dshared_ptr<tfg_ssa_t> const& dst_tfg, fixed_reg_mapping_t const &fixed_reg_mapping, rodata_map_t const &rodata_map, pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> const& pc_lsprels, vector<dst_insn_t> const& dst_iseq, vector<dst_ulong> const& dst_insn_pcs, context* ctx, string const& asm_filename/*, bool llvm2ir*/, const quota& max_quota, bool use_only_relocatable_memlabels, bool run_relocatable_memlabels_and_safety_check, context::xml_output_format_t xml_output_format, correl_hints_t const& correl_hints);

  eqcheck(istream& is, dshared_ptr<tfg_ssa_t> const& src_tfg, dshared_ptr<tfg_ssa_t> const& dst_tfg);

  void eqcheck_to_stream(ostream& os);
  string const& get_asm_filename() const { return m_asm_filename; }

  static dshared_ptr<cg_with_asm_annotation const> check_eq(dshared_ptr<eqcheck> eq, string const &check_filename = "");
  bool check_proof(vector<memlabel_ref> const &relevant_memlabels, const string& proof_file);
  //void clear_fcall_side_effects() { m_fcall_side_effects = false; }

  context::xml_output_format_t get_xml_output_format() const { return m_xml_output_format; }

  dshared_ptr<tfg_ssa_t> get_src_tfg_ptr() const { return m_src_tfg; }
  dshared_ptr<tfg_ssa_t> get_dst_tfg_ptr() const { return m_dst_tfg; }
  tfg_llvm_t const& get_src_tfg() const {
    dshared_ptr<tfg> t = m_src_tfg->get_ssa_tfg();
    dshared_ptr<tfg_llvm_t> t_llvm = dynamic_pointer_cast<tfg_llvm_t>(t);
    ASSERT(t_llvm);
    return *t_llvm;
  }
  //tfg& get_src_tfg();
  tfg const& get_dst_tfg() const { return *m_dst_tfg->get_ssa_tfg(); }
  //tfg& get_dst_tfg();

  //memlabel_assertions_t const& eqcheck_get_memlabel_assertions() const;

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

  failed_cg_set_t const& get_failed_cg_set() const { return m_failed_cg_set; }
  const quota& get_quota() const;
  quota& get_quota();
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> const& get_debug_header_pc_local_sprel_expr_assumes() { return m_debug_header_pc_lsprels; }
  bool get_use_only_relocatable_memlabels() const { return m_use_only_relocatable_memlabels; }
  bool get_run_relocatable_memlabels_and_safety_check() const { return m_run_relocatable_memlabels_and_safety_check; }
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
  graph_symbol_map_t const& get_symbol_map() const;
  static bool istream_position_at_function_name(istream& in, string const& function_name);
  static string proof_file_identify_failed_function_name(string const& proof_file);
  static set<string> read_existing_proof_file(string const& existing_proof_file);
  void register_failed_corr_graph(dshared_ptr<cg_with_failcond> const& cg);
  void clear_failed_corr_graph_list();
  correl_hints_t const& eqcheck_get_correl_hints() const { return m_correl_hints; }

private:
  static bool local_size_is_compatible_with_sprel_expr(size_t local_size, expr_ref sprel_expr, consts_struct_t const &cs);
  void collapse_dst_tfg();
  //void preprocess_tfgs();
  void set_quota();

private:
  string m_proof_filename;
  string m_function_name;
  string m_asm_filename;
  dshared_ptr<tfg_ssa_t> m_src_tfg;
  dshared_ptr<tfg_ssa_t> m_dst_tfg;
  fixed_reg_mapping_t m_fixed_reg_mapping;
  rodata_map_t m_rodata_map;

  vector<dst_insn_t> m_dst_iseq;
  vector<dst_ulong> m_dst_insn_pcs;

  context* m_ctx;

  quota m_max_quota;
  //bool m_fcall_side_effects;
  //bool m_llvm2ir;
  context::xml_output_format_t m_xml_output_format;
  failed_cg_set_t m_failed_cg_set;
  /*map<graph_loc_id_t, graph_loc_id_t> m_base_loc_id_map_for_dst_locs;
  map<pc, map<graph_loc_id_t, mybitset>> m_dst_locs_assigned_to_constants_map;*/

  bool m_use_only_relocatable_memlabels;
  bool m_run_relocatable_memlabels_and_safety_check;
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> m_debug_header_pc_lsprels;
  correl_hints_t m_correl_hints;
};

}
