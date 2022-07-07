#pragma once

#include "support/log.h"
#include "support/src-defs.h"

#include "expr/state.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/memlabel_map.h"

#include "gsupport/suffixpath.h"
#include "gsupport/precond.h"
#include "gsupport/memlabel_assertions.h"
#include "gsupport/predicate_set.h"
#include "gsupport/edge_wf_cond.h"

#include "graph/graph_cp_location.h"
#include "graph/callee_summary.h"
#include "graph/avail_exprs.h"

#include "tfg/tfg.h"
#include "tfg/tfg_ssa.h"

#include "x64/insntypes.h"

struct dst_insn_t;

namespace eqspace {

class llptfg_t;
class predicate;
class sprel_map_pair_t;
class rodata_map_t;
class aliasing_constraints_t;
class tfg_llvm_t;
class corr_graph;
class cg_with_asm_annotation;


class dst_parsed_tfg_t {
private:
  dshared_ptr<tfg_ssa_t> m_tfg;
  rodata_map_t m_rodata_map;
  vector<dst_insn_t> m_dst_iseq_vec;
  vector<dst_ulong> m_dst_insn_pcs_vec;
public:
  dst_parsed_tfg_t(dshared_ptr<tfg_ssa_t> const& t, rodata_map_t const& rodata_map, vector<dst_insn_t> const& dst_iseq_vec, vector<dst_ulong> const& dst_insn_pcs_vec) : m_tfg(t), m_rodata_map(rodata_map), m_dst_iseq_vec(dst_iseq_vec), m_dst_insn_pcs_vec(dst_insn_pcs_vec)
  { }
  dshared_ptr<tfg_ssa_t> const& get_tfg() const { return m_tfg; }
  rodata_map_t const& get_rodata_map() const { return m_rodata_map; }
  vector<dst_insn_t> const& get_dst_iseq_vec() const { return m_dst_iseq_vec; }
  vector<dst_ulong> const& get_dst_insn_pcs_vec() const { return m_dst_insn_pcs_vec; }

  static dst_parsed_tfg_t parse_dst_asm_tfg_for_eqcheck(string const &function_name, istream &in, context* ctx);
};

//string read_state(istream& in, map<string_ref, expr_ref>& st_value_expr_map, context* ctx, state const *start_state);
//pair<bool, string> read_tfg(istream& in, tfg **t, string const &tfg_name, context* ctx, bool is_ssa);
//string read_tfg_llvm(ifstream& in, tfg& t, context* ctx);
string read_tfg_from_ifstream(istream& db, context* ctx, state const &base_state);
//shared_ptr<llptfg_t const> read_llvm_function_tfg_map(const std::string& filename_llvm, context* ctx);
map<string, dst_parsed_tfg_t> parse_dst_function_tfg_map(const std::string& filename_dst, llptfg_t const& llptfg, context* ctx);
//bool parse_tfgs_for_eqcheck(std::string const &function_name, const std::string& filename_llvm, istream &in_dst/*std::string const &filename_dst*/, shared_ptr<tfg> *tfg_dst, shared_ptr<tfg_llvm_t> *tfg_llvm, rodata_map_t &rodata_map, std::vector<dst_insn_t>& dst_iseq_vec, std::vector<dst_ulong>& dst_insn_pcs_vec, context* ctx/*, bool llvm2ir*/);
predicate_set_t const read_assumes_from_file(string const &assume_file, context* ctx);
//void tfgs_get_relevant_memlabels(vector<memlabel_ref> &relevant_memlabels, tfg const &tfg_llvm, tfg const &tfg_dst);
void edge_read_pcs_and_comment(const string& line, pc& p1, pc& p2, string &comment);
string read_sprel_map_pair(istream &in, context *ctx, sprel_map_pair_t &sprel_map_pair);
//string read_rodata_map(istream &in, context *ctx, rodata_map_t &rodata_map);
tfg_suffixpath_t read_src_suffixpath(istream &in, tfg const& t/*, tfg_suffixpath_t &src_suffixpath*/);
//string read_src_avail_exprs(istream &in/*, state const& start_state*/, context *ctx, avail_exprs_t &src_avail_exprs);
//string read_tfg_edge_using_start_state(istream &in, string const& first_line, context *ctx, string const &prefix, state const &start_state, shared_ptr<tfg_edge const>& out_tfg_edge);
//string read_memlabel_map(istream& in, context* ctx, string line, graph_memlabel_map_t &memlabel_map);
void read_lhs_set_guard_etc_and_src_dst_from_file(istream& in, context* ctx, list<guarded_pred_t>& lhs_set, precond_t& precond, graph_edge_composition_ref<pc, tfg_edge>& eg, sprel_map_pair_t& sprel_map_pair, shared_ptr<tfg_edge_composition_t>& src_suffixpath, avail_exprs_t& src_avail_exprs, aliasing_constraints_t& alias_cons, graph_memlabel_map_t& memlabel_map, expr_ref& src, expr_ref& dst, map<expr_id_t, expr_ref>& src_map, map<expr_id_t, expr_ref>& dst_map, map<expr_id_t, pair<expr_ref, expr_ref>> &concrete_address_submap/*, dshared_ptr<memlabel_assertions_t>& mlasserts*/, dshared_ptr<tfg_llvm_t>& src_tfg, relevant_memlabels_t& relevant_memlabels);
set<string> etfg_file_get_uncovered_functions(string const& etfg_filename, map<string, dshared_ptr<cg_with_asm_annotation const>> const& function_cg_map);
void skip_till_next_function(ifstream &in);

edge_wf_cond_t edge_wf_cond_from_stream(istream& is, string const& prefix, corr_graph const& cg, tfg const& src_tfg, tfg const& dst_tfg);
string expr_xml_print_after_rename_to_source_using_tfg_llvms(expr_ref const& e, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, context::xml_output_format_t xml_output_format, dshared_ptr<tfg_llvm_t const> const& t_llvm, pc const& llvm_pc, dshared_ptr<tfg const> const& t_dst, pc const& dst_pc);
string counter_example_xml_print_after_rename_to_source_using_tfg_llvms(context* ctx, counter_example_t const& ce, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, context::xml_output_format_t xml_output_format, dshared_ptr<tfg_llvm_t> const& t_llvm, pc const& llvm_pc, dshared_ptr<tfg> const& t_dst, pc const& dst_pc, string const& prefix = "");

string counter_example_xml_print_only_renamed_to_source_mapping(context* ctx, counter_example_t const& ce_in, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, context::xml_output_format_t xml_output_format, dshared_ptr<tfg_llvm_t> const& t_llvm, pc const& llvm_pc, string const& prefix = "");

}
