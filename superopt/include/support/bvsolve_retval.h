#pragma once


#include "support/types.h"
#include "support/bv_const.h"

namespace eqspace {

using namespace std;

class bvsolve_retval_t {
private:
  vector<vector<bv_const>> m_points;
  vector<vector<bv_const>> m_id_matrix;
  vector<size_t> m_var_ids;
  map<size_t, bv_solve_var_idx_t> m_rev_index_map;
  unsigned m_num_orig_vars;
  map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> m_solution;

public:
  bvsolve_retval_t(vector<vector<bv_const>> const& points, vector<vector<bv_const>> const& id_matrix, vector<size_t> const& var_ids, map<size_t, bv_solve_var_idx_t> const& rev_index_map, unsigned num_orig_vars, map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> const& solution)
    : m_points(points),
      m_id_matrix(id_matrix),
      m_var_ids(var_ids),
			m_rev_index_map(rev_index_map),
      m_num_orig_vars(num_orig_vars),
      m_solution(solution)
  { }

  vector<vector<bv_const>> const& get_points() const { return m_points; }
  vector<vector<bv_const>> const& get_id_matrix() const { return m_id_matrix; }
  vector<size_t> const& get_var_ids() const { return m_var_ids; }
  map<size_t, bv_solve_var_idx_t> const& get_rev_index_map() const { return m_rev_index_map; }
  unsigned get_num_orig_vars() const { return m_num_orig_vars; }
  map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const>> const& get_solution() const { return m_solution; }

  string bvsolve_retval_to_string() const;
  void bvsolve_retval_to_stream(ostream& os) const;
  static bvsolve_retval_t bvsolve_retval_from_stream(istream& ldr_ifs);
private:
  static vector<vector<bv_const>> read_matrix(istream& ifs);
  static vector<size_t> read_vector(istream& ifs);
  static map<size_t, bv_solve_var_idx_t> read_index_map(istream& ifs);
};

}
