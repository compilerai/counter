#pragma once

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/local_sprel_expr_guesses.h"
#include "expr/aliasing_constraints.h"
#include "expr/pc.h"
#include "expr/proof_result.h"

#include "gsupport/precond.h"
#include "gsupport/predicate.h"
#include "gsupport/suffixpath.h"

#include "graph/avail_exprs.h"

namespace eqspace {

//class memlabel_assertions_t;
class tfg_llvm_t;

class query_decomposer
{
private:
  map<expr_ref,expr_ref> m_dst_to_src_map;       // dst-expr to equivalent src-expr map

  // m_src and m_dst are the expressions which are needed to be proved equal by SMT solver under precondition expr m_precond_expr but such a query is getting timed out, so we will try to prove them using query decomposition approach.
  expr_ref m_src, m_dst;
  context* m_ctx;
  vector<pair<expr_ref,unsigned long>> m_src_expr_to_size;         // vector of src-expr and src expr size
  vector<pair<expr_ref,unsigned long>> m_dst_expr_to_size;         // vector of dst-expr and dst expr size
  map<expr_ref, list<expr_ref>> m_src_dst_ce_eval;
  // this function replaces the nodes in expression tree 'e' by their equivalent smaller size subexpression trees of src
  expr_ref replace_expr_helper(expr_ref e);

public:
  query_decomposer(context* ctx, expr_ref const &src, expr_ref const &dst);

  // Comparator to sort src_expr_to_size and dst_expr_to_size in increasing order according to expr size
  static bool sortbysec(const pair<expr_ref,unsigned long> &a, const pair<expr_ref,unsigned long> &b)
  {
    return (a.second < b.second);
  }

  // this is the function which fills the dst_to_src_map variable by filling the src_id of equivalent subexpressoins for respective dst_id
//  bool find_mappings(list<guarded_pred_t> const &lhs_set, precond_t const &precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, const query_comment& qc, bool timeout_res, list<counter_example_t> &counter_examples, memlabel_assertions_t const& mlasserts, aliasing_constraints_t const& rhs_alias_cons, tfg_llvm_t const &src_tfg, consts_struct_t const &cs);
  proof_result_t find_mappings_and_perform_query(list<guarded_pred_t> const &lhs_set, precond_t const &precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, const query_comment& qc/*, list<counter_example_t> &counter_examples*/, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, relevant_memlabels_t const& relevant_memlabels, aliasing_constraints_t const& rhs_alias_cons, tfg_llvm_t const &src_tfg);
};
 
}
