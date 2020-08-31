#pragma once

#include <stdbool.h>
//#include "graph/tfg.h"
#include "expr/state.h"
#include "graph/graph_cp_location.h"
#include "graph/callee_summary.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "support/log.h"
#include "gsupport/suffixpath.h"
#include "tfg/precond.h"
#include "graph/memlabel_assertions.h"
#include "tfg/predicate_set.h"
#include "tfg/avail_exprs.h"
#include "expr/memlabel_map.h"
#include <list>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include "support/src-defs.h"

struct dst_insn_t;

namespace eqspace {

class local_sprel_expr_guesses_t;
class predicate;
class sprel_map_pair_t;
class rodata_map_t;
class aliasing_constraints_t;

string read_local_sprel_expr_assumptions(istream &in, context *ctx, local_sprel_expr_guesses_t &lsprel_guesses);

//string read_state(istream& in, map<string_ref, expr_ref>& st_value_expr_map, context* ctx, state const *start_state);
//pair<bool, string> read_tfg(istream& in, tfg **t, string const &tfg_name, context* ctx, bool is_ssa);
//string read_tfg_llvm(ifstream& in, tfg& t, context* ctx);
string read_tfg_from_ifstream(istream& db, context* ctx, state const &base_state);
bool parse_input_eq_file(std::string const &function_name, const std::string& filename_llvm, istream &in_dst/*std::string const &filename_dst*/, shared_ptr<tfg> *tfg_dst, shared_ptr<tfg> *tfg_llvm, rodata_map_t &rodata_map, std::vector<dst_insn_t>& dst_iseq_vec, std::vector<dst_ulong>& dst_insn_pcs_vec, context* ctx, bool llvm2ir);
predicate_set_t const read_assumes_from_file(string const &assume_file, context* ctx);
bool is_next_line(istream& in, const string& keyword);
void tfgs_get_relevant_memlabels(vector<memlabel_ref> &relevant_memlabels, tfg const &tfg_llvm, tfg const &tfg_dst);
void edge_read_pcs_and_comment(const string& line, pc& p1, pc& p2, string &comment);
string read_sprel_map_pair(istream &in, context *ctx, sprel_map_pair_t &sprel_map_pair);
//string read_rodata_map(istream &in, context *ctx, rodata_map_t &rodata_map);
string read_aliasing_constraints(istream &in, tfg const* t, context *ctx, aliasing_constraints_t &rodata_map);
string read_src_suffixpath(istream &in/*, state const& start_state*/, context* ctx, tfg_suffixpath_t &src_suffixpath);
string read_src_pred_avail_exprs(istream &in/*, state const& start_state*/, context *ctx, pred_avail_exprs_t &src_pred_avail_exprs);
map<string, shared_ptr<tfg>> get_function_tfg_map_from_etfg_file(string const &etfg_filename, context* ctx);
//string read_tfg_edge_using_start_state(istream &in, string const& first_line, context *ctx, string const &prefix, state const &start_state, shared_ptr<tfg_edge const>& out_tfg_edge);
//string read_memlabel_map(istream& in, context* ctx, string line, graph_memlabel_map_t &memlabel_map);
void read_lhs_set_guard_lsprels_and_src_dst_from_file(istream& in, context* ctx, predicate_set_t& lhs_set, precond_t& precond, local_sprel_expr_guesses_t& lsprel_guesses, sprel_map_pair_t& sprel_map_pair, shared_ptr<tfg_edge_composition_t>& src_suffixpath, pred_avail_exprs_t& src_pred_avail_exprs, aliasing_constraints_t& alias_cons, graph_memlabel_map_t& memlabel_map, set<local_sprel_expr_guesses_t>& all_guesses, expr_ref& src, expr_ref& dst, map<expr_id_t, expr_ref>& src_map, map<expr_id_t, expr_ref>& dst_map, shared_ptr<memlabel_assertions_t>& mlasserts, shared_ptr<tfg>& src_tfg);

}
