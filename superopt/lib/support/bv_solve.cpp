#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

#include "support/bv_const.h"
#include "support/utils.h"
#include "support/bv_solve.h"
#include "support/linear_solver.h"
#include "support/sage_interface.h"
#include "support/mytimer.h"
#include "support/query_comment.h"
//#include "expr/context.h"

using namespace std;

namespace eqspace {

//extern context* g_ctx;

static void
add_zero_row_if_required(vector<vector<bv_const>>& points, size_t dimension)
{
    // add zero rows / remove points if needed
    if (points.size() < dimension) {
      CPP_DBG_EXEC(BV_SOLVE, cout << __func__ << ":" << __LINE__ << ": not enough points; adding zero rows\n");
      vector<bv_const> zero_row(points[0].size(), 0);
      while (points.size() < dimension) {
        points.push_back(zero_row);
      }
    }
    ASSERT(points.size() >= dimension);
}

static map<size_t, map<bv_const, vector<size_t>>>
find_columns_related_by_offset(vector<vector<bv_const>> const& ipoints, bv_const const& mod, size_t dedup_threshold)
{
  ASSERT(ipoints.size() > 0);
  ASSERT(ipoints[0].size() > 0);

  if (ipoints[0].size() < dedup_threshold || mod == 2) {
    // optimization is useful only if the column count can cause sage slowdown
    return {};
  }

  const size_t rows = ipoints.size();
  const size_t columns = ipoints[0].size();

  set<size_t> visited;
  map<size_t, map<bv_const, vector<size_t>>> ret;
  // scan left to right to find columns which may be related by const offset
  for (size_t i = 0; i < columns; ++i) {
    if (visited.count(i)) continue;
    for (size_t j = i+1; j < columns; ++j) {
      if (visited.count(j)) continue;
      bv_const v = bv_mod(ipoints[0][i] - ipoints[0][j], mod);
      bool all_equal = true;
      for (size_t k = 1; k < rows; ++k)
        if (bv_mod(ipoints[k][i] - ipoints[k][j], mod) != v) {
          all_equal = false;
          break;
        }
      if (all_equal) {
        visited.insert(j);
        if (!ret.count(i))
          ret[i] = { };
        if (!ret[i].count(v))
          ret[i][v] = { };
        ret[i][v].push_back(j);
      }
    }
  }

  return ret;
}

static vector<vector<bv_const>>
gather_columns_and_prefix_each_row_with_one(vector<vector<bv_const>> const& ipoints, vector<size_t> const& idx)
{
  vector<vector<bv_const>> ret(ipoints.size());
  // copy and prefix every point with 1
  for (size_t i = 0; i < ipoints.size(); ++i) {
    auto& pp = ret[i];
    pp.resize(idx.size()+1);
    pp[0] = 1;
    for (size_t j = 0; j < idx.size(); ++j) {
      ASSERT(idx[j] < ipoints[i].size());
      pp[j+1] = ipoints[i][idx[j]];
    }
  }
  return ret;
}

static vector<size_t>
create_gather_index_from_ref_offset_idx_map(map<size_t, map<bv_const, vector<size_t>>> const& ref_offset_idx_map, int dimension)
{
  vector<size_t> idx;
  if (ref_offset_idx_map.size()) {
    vector<size_t> to_be_skipped;
    for (auto const& p_ref_offset_idx : ref_offset_idx_map) {
      for (auto const& p_offset_idx : p_ref_offset_idx.second)
        to_be_skipped.insert(to_be_skipped.end(), p_offset_idx.second.begin(), p_offset_idx.second.end());
    }
    sort(to_be_skipped.begin(), to_be_skipped.end());

    int index = -1;
    for (auto v : to_be_skipped) {
      while (++index < v)
        idx.push_back(index);
    }
    while (++index < dimension)
      idx.push_back(index);
    CPP_DBG_EXEC(BV_SOLVE, cout << __func__ << ':' << __LINE__ << ": dedup'ed to " << idx.size() << " columns\n";);
  }
  else {
   // create an identity map
    idx.resize(dimension);
    for (size_t i = 0; i < dimension; ++i)
      idx[i] = i;
  }
  return idx;
}

static map<bv_solve_var_idx_t, vector<bv_const>>
scatter_and_fill_using_ref_offset_idx_map(map<bv_solve_var_idx_t, vector<bv_const>> const& solns, vector<size_t> const& idx, int dimension, bv_const const& mod, map<size_t, map<bv_const, vector<size_t>>> const& ref_offset_idx_map)
{
  if (ref_offset_idx_map.size()) {
    map<bv_solve_var_idx_t, vector<bv_const>> ret_solns;
    //ret_solns.reserve(solns.size());

    // scatter and fill the existing soln rows
    for (auto const& row : solns) {
      auto const& soln_row = row.second;

      ASSERT(soln_row.size() == idx.size()+1); // +1 for const term
      vector<bv_const> this_row(dimension, 0);
      for (size_t j = 0; j < idx.size(); ++j) {
        this_row[idx[j]] = soln_row[j];
      }
      this_row.back() = soln_row.back(); // add the const term

      ASSERT(row.first < idx.size());
      auto const& var_idx = idx[row.first];
      bool inserted = ret_solns.insert(make_pair(var_idx, this_row)).second;
      ASSERT(inserted);
    }

    // add the offset relations
    for (auto const& p_ref_offset_idx : ref_offset_idx_map) {
      size_t   ref_index = p_ref_offset_idx.first;
      bv_const const& ref_coeff = mod - 1;
      bv_const const& ref_const = 0;

      // ref - term = const => term - ref + const = 0
      // coefficient of term = 1; coefficient of ref = -1 (mod `mod')
      for (auto const& p_offset_idx : p_ref_offset_idx.second) {
        bv_const const& offset_const = p_offset_idx.first;
        for (auto index : p_offset_idx.second) {
          vector<bv_const> this_row(dimension, 0);
          this_row[ref_index] = ref_coeff;
          this_row[index]     = 1;
          this_row.back()     = bv_mod(offset_const + ref_const, mod);

          bool inserted = ret_solns.insert(make_pair(index, this_row)).second;
          ASSERT(inserted);
        }
      }
    }
    return ret_solns;
  }
  else
    return solns;
}

vector<vector<bv_const>> prefix_each_row_with_one(vector<vector<bv_const>> const& ipoints)
{
  vector<vector<bv_const>> ret;
  ret.reserve(ipoints.size());
  for (auto const& row : ipoints) {
    vector<bv_const> prow;
    prow.reserve(row.size()+1);
    prow.push_back(1);
    prow.insert(prow.end(), row.begin(), row.end());
    ret.push_back(prow);
  }
  return ret;
}

map<bv_solve_var_idx_t, vector<bv_const>>
bv_solve(vector<vector<bv_const>> const& input_points, const unsigned dim)
{
  autostop_timer timer(__func__);

  ASSERT(dim > 0);
  ASSERT(input_points.size() > 0);
  ASSERT(input_points.at(0).size() >= 1);
  size_t num_vars = input_points[0].size();
  size_t dimension = num_vars+1; // +1 for constant coefficient
  const bv_const mod = bv_exp2(dim);

  CPP_DBG_EXEC(STATS, stats::get().new_bv_solve_input(input_points.size(), num_vars));

  CPP_DBG_EXEC(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": input_points:\n"; print_2d_matrix<bv_const>(input_points));
  CPP_DBG_EXEC(BV_SOLVE, cout << __func__ << ':' << __LINE__ << ": input matrix size = " << input_points.size() << 'x' << num_vars << endl;);
  CPP_DBG_EXEC2(BV_SOLVE, cout << __func__ << ":" << __LINE__ << ": input matrix is :\n"; for (const auto& point : input_points) { cout << '[' << bv_const_vector2str(point) << "]\n";});

  // dedup horizontally -- preserving the points at the end
  vector<vector<bv_const>> ipoints = input_points;
  dedup_vector_preserving_order(ipoints);
  size_t num_pts = ipoints.size();
  CPP_DBG_EXEC(BV_SOLVE, cout << __func__ << ':' << __LINE__ << ": after horizontal dedup matrix size = " << num_pts << 'x' << num_vars << endl;);

  // dedup vertically
  size_t dedup_threshold = BVSOLVE_DEDUP_THRESHOLD; //ctx->get_config().sage_dedup_threshold;
  // (reference index, offset value -> list of indices)
  map<size_t, map<bv_const, vector<size_t>>> ref_offset_idx_map = find_columns_related_by_offset(ipoints, mod, dedup_threshold); 
  CPP_DBG_EXEC2(BV_SOLVE_VERT_DEDUP,
    cout << __func__ << ':' << __LINE__ << ": ref_offset_idx_map:\n";
    for (auto const& p : ref_offset_idx_map) {
      cout << "ref index: " << p.first << endl;
      for (auto const& pp : p.second) {
        cout << "\toffset: " << pp.first << ": ";
        for (auto const& v : pp.second) {
          cout << v << ", ";
        }
        cout << endl;
       }
    }
  );
  vector<size_t> idx = create_gather_index_from_ref_offset_idx_map(ref_offset_idx_map, dimension-1); // 1 will be added later
  CPP_DBG_EXEC2(BV_SOLVE_VERT_DEDUP,
    cout << __func__ << ':' << __LINE__ << ": gather index: "; for (auto const& i : idx) cout << i << ", "; cout << endl);
  vector<vector<bv_const>> points = gather_columns_and_prefix_each_row_with_one(ipoints, idx);

  // add 1 for constant coefficient
  //vector<vector<bv_const>> points = prefix_each_row_with_one(ipoints);

  map<bv_solve_var_idx_t, vector<bv_const>> solns;

  //context* ctx = g_ctx;
  //if (ctx && ctx->get_config().use_sage) {
  //  vector<vector<bv_const>> sage_solns;
  //  add_zero_row_if_required(points, dimension);
  //  ctx->get_sage_interface()->get_nullspace_basis(points, bv_exp2(dim), query_comment("bvs"), sage_solns);
  //  for (size_t i = 0; i < sage_solns.size(); ++i) {
  //    solns.insert(make_pair(i, sage_solns.at(i)));
  //  }
  //} else {
    // call bitvector linear solver
    get_nullspace_basis(points, dim, solns);
    //CPP_DBG_EXEC(BV_SOLVE, cout << __func__ << " " << __LINE__ << ": solns.size() = " << solns.size() << endl);
  //}

  map<bv_solve_var_idx_t, vector<bv_const>> ret_solns = scatter_and_fill_using_ref_offset_idx_map(solns, idx, dimension, mod, ref_offset_idx_map);
  CPP_DBG_EXEC2(BV_SOLVE_VERT_DEDUP, cout << __func__ << " " << __LINE__ << ": ret_solns =\n"; for (auto const& p : ret_solns) cout << p.first << ":  [ " << bv_const_vector2str(p.second) << " ]\n");
  return ret_solns;
}

}
