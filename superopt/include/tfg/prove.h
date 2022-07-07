#pragma once

#include "expr/expr.h"
#include "expr/context.h"
#include "expr/aliasing_constraints.h"

#include "gsupport/predicate.h"
#include "gsupport/predicate_set.h"

#include "graph/avail_exprs.h"

namespace eqspace {

class tfg_llvm_t;


//bool expr_set_lhs_prove_src_dst(context *ctx, unordered_set<predicate_ref>const &lhs, edge_guard_t const &guard, expr_ref src, expr_ref dst, query_comment const &qc, bool timeout_res, counter_example_t &counter_example, consts_struct_t const &cs);
//bool is_expr_equal_using_lhs_set_and_precond(context *ctx, list<predicate_ref> const &s, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &avail_exprs, graph_memlabel_map_t const &memlabel_map, /*rodata_map_t const &rodata_map,*/ expr_ref const &e1, expr_ref const &e2, const query_comment& qc, bool timeout_res, list<counter_example_t> &counter_examples/*, std::set<cs_addr_ref_id_t> const &relevant_addr_refs, vector<memlabel_ref> const& relevant_memlabels*/, memlabel_assertions_t const& mlasserts, aliasing_constraints_t const& rhs_alias_cons, tfg_llvm_t const &src_tfg, consts_struct_t const &cs);
void update_simplify_cache(context *ctx, expr_ref const &a, expr_ref const &b, list<predicate_ref> const &lhs);
expr_ref get_simplified_expr_using_simplify_cache(context *ctx, expr_ref const &a, list<predicate_ref> const &lhs);
bool evaluate_counter_example_on_prove_query(context *ctx, list<predicate_ref> const &lhs, expr_ref const &src, expr_ref const &dst, counter_example_t &counter_example);
bool expr_triple_set_contains(list<predicate_ref> const &haystack, list<predicate_ref> const &needle);
//void output_lhs_set_precond_and_src_dst_to_file(ostream &file, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, /*rodata_map_t const &rodata_map,*/ aliasing_constraints_t const& alias_cons, expr_ref const &src, expr_ref const &dst, tfg_llvm_t const &src_tfg);
//void output_lhs_set_precond_and_src_dst_to_file(ostream &file, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, /*rodata_map_t const &rodata_map,*/ aliasing_constraints_t const& alias_cons, expr_ref const &src, expr_ref const &dst, tfg_llvm_t const &src_tfg);
//void output_lhs_set_guard_lsprels_and_src_dst_to_file(ostream &fo, predicate_set_t const &lhs_set, precond_t const& precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, aliasing_constraints_t const& alias_cons, graph_memlabel_map_t const &memlabel_map expr_ref src, expr_ref dst, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, tfg_llvm_t const &src_tfg);
//bool is_expr_equal_using_lhs_set_and_guard_evaluate_file(context *ctx, string const &filename);

void clean_query_files();
void g_query_dir_init();
//map<expr_id_t, pair<expr_ref, expr_ref>> lhs_set_get_submap(unordered_set<predicate_ref>const &lhs_set, precond_t const &precond, context *ctx);
//string read_memlabel_map(istream& in, context* ctx, string line, graph_memlabel_map_t &memlabel_map);
expr_ref expr_assign_mlvars_to_memlabels(expr_ref const &e, map<pc, pair<mlvarname_t, mlvarname_t>> &fcall_mlvars, map<pc, map<expr_ref, mlvarname_t>> &expr_mlvars, string const &graph_name, long &mlvarnum, pc const &p);
proof_status_t is_expr_equal_using_lhs_set_and_precond_helper(context *ctx, list<predicate_ref> const &lhs_set, precond_t const &precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, expr_ref const &src, expr_ref const &dst, const query_comment& qc/*, list<counter_example_t> &counter_examples*/, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, aliasing_constraints_t const& rhs_alias_cons, tfg_llvm_t const &src_tfg);

unordered_set<predicate_ref> expr_get_div_by_zero_cond(expr_ref const& e);
unordered_set<predicate_ref> expr_get_memlabel_heap_access_cond(expr_ref const& e, graph_memlabel_map_t const& memlabel_map);
unordered_set<predicate_ref> expr_is_not_memlabel_access_cond(expr_ref const& e, graph_memlabel_map_t const& memlabel_map);

}
