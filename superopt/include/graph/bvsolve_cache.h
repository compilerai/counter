#pragma once

#include "support/types.h"
#include "support/bv_const.h"

#include "graph/expr_group.h"
#include "graph/bvsolve_cache_entry.h"

using namespace std;

namespace eqspace {

class bvsolve_cache_t
{
private:
  list<bvsolve_cache_entry_t> m_entries;
public:
  map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>>
  bvsolve_cache_query(expr_group_ref const& exprs, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, set<string_ref> const& visited_ces, map<string_ref, map<bv_solve_var_idx_t, bv_const>> const& input_points);

  static map<string_ref, map<bv_solve_var_idx_t, bv_const>> read_named_points(istream& is);

  static bvsolve_retval_t
  update_points_matrix_with_new_points_and_ldr_decompose(vector<vector<bv_const>>& updated_points, vector<vector<bv_const>>& updated_id_matrix, vector<size_t>& updated_var_ids, map<size_t, bv_solve_var_idx_t> const& rev_index_map, unsigned num_orig_vars, set<string_ref> const& new_ce_names, map<string_ref, map<bv_solve_var_idx_t, bv_const>> const& input_points, bv_const const& mod, unsigned dim);

private:


  list<bvsolve_cache_entry_t>::iterator find_maximally_matching_bvsolve_cache_entry(expr_group_ref const& exprs, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, set<string_ref> const& visited_ces);

  static vector<vector<bv_const>> construct_updated_matrices(vector<vector<bv_const>>& existing_points, vector<vector<bv_const>>& existing_id_matrix, vector<size_t>& var_ids, map<size_t, bv_solve_var_idx_t> const& rev_index_map, set<string_ref> const& new_ce_names, map<string_ref, map<bv_solve_var_idx_t, bv_const>> const& input_points, bv_const const& mod, unsigned dim);

  static void make_square_after_adding_new_points(vector<vector<bv_const>>& updated_points, vector<vector<bv_const>>& updated_id_matrix, vector<size_t>& var_ids);

  static void rearrange_point(vector<bv_const>& point_vec, vector<size_t> const& var_ids);

  static void add_point_after_multiplying_with_id_matrix(vector<bv_const>& point_vec, vector<vector<bv_const>> const& modified_id_matrix, vector<vector<bv_const>> const& cur_points, vector<size_t> const& var_ids, bv_const const& mod, unsigned dim);

  static vector<bv_const> obtain_point_vec(map<bv_solve_var_idx_t, bv_const> const& point, vector<size_t> const& var_ids, map<size_t, bv_solve_var_idx_t> const& rev_index_map);
  static vector<vector<bv_const>> modify_id_matrix(vector<vector<bv_const>> const& updated_id_matrix, vector<size_t> const& var_ids, bv_const const& mod, unsigned dim);
};

}
