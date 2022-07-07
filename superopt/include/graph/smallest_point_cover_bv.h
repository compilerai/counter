#pragma once

#include "support/bv_const.h"
#include "support/bv_const_ref.h"
#include "support/dyn_debug.h"

#include "graph/smallest_point_cover.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_bv_t : public smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>
{
private:
  using expr_idx_t = expr_group_t::expr_idx_t;
  mutable map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const_ref>> m_bv_solve_output_matrix;
  static std::function<bool (bv_solve_var_idx_t, bv_solve_var_idx_t)> const bvcmp;

  //mutable set<expr_group_t::expr_idx_t> m_masked_expr_idxs;
public:
  smallest_point_cover_bv_t(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, expr_group_ref const &global_eg_at_p, context *ctx) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(point_set, out_of_bound_exprs, global_eg_at_p, ctx)
  {
  }

  smallest_point_cover_bv_t(istream& is, string const& inprefix, context* ctx, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(is, inprefix, ctx, point_set, out_of_bound_exprs)
  {
    this->state_elements_from_stream(is, inprefix);
  }

  smallest_point_cover_bv_t(smallest_point_cover_bv_t const& other, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(other, point_set, out_of_bound_exprs), m_bv_solve_output_matrix(other.m_bv_solve_output_matrix)
  { }

  virtual bool is_eqclass_used_for_correl_ranking() const override
  {
    return true;
    //return (this->get_exprs_to_be_correlated()->expr_group_has_ghost_exprs(this->m_ctx));
  }

//  int remove_exprs_with_no_predicate()
//  {
//    ASSERT(this->m_point_set.size());
//    // Do not try to remove independent exprs till point_set is empty
//    set<expr_group_t::expr_idx_t> no_pred_expr_ids;
//    expr_group_ref const& corr_expr_group = this->get_exprs_to_be_correlated();
//    for(auto const& [idx,pe] : corr_expr_group->get_expr_vec()) {
//      if(this->expr_index_has_some_predicate(idx))
//        continue;
//
//      DYN_DEBUG(eqclass_bv, cout << _FNLN_ << ": identified " << idx << " as a no-pred expr-id\n");
//      no_pred_expr_ids.insert(idx);
//    }
//    //auto no_pred_expr_id_keys = map_get_keys(no_pred_expr_ids);
//    if(no_pred_expr_ids.size()){
//      map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const_ref>> bv_solve_output_matrix_prunned;
//      for(auto const& row: m_bv_solve_output_matrix)
//      {
//        bv_solve_var_idx_t free_var_idx = row.first;
//        map<bv_solve_var_idx_t, bv_const_ref> row_prunned;
//        for (auto const& [i,vi] : row.second) {
//          if (!set_belongs(no_pred_expr_ids, i)) {
//            row_prunned.insert(make_pair(i, vi));
//          }
//        }
//        bv_solve_output_matrix_prunned.insert(make_pair(free_var_idx, row_prunned));
//      }
//      m_bv_solve_output_matrix = bv_solve_output_matrix_prunned;
//      this->m_point_set.remove_exprs_at_index(no_pred_expr_ids);
//
//      DYN_DEBUG(eqclass_bv, cout << __func__ << " " << __LINE__ << ": ret_matrix after removing exprs with no predicate =\n";
//        cout << "       ";
//        for (size_t i = 0; i < corr_expr_group->expr_group_get_num_exprs(); ++i) {
//          cout << setw(10) << i+1 << ", " ;
//        }
//        cout << setw(10) << "constant\n";
//        for (auto const& p : m_bv_solve_output_matrix) {
//          cout << setw(2) << p.first;
//          cout  << ":  [ " ;
//          for (auto const& c : p.second)
//            cout << setw(10) << c.first << "->" << c.second->get_bv() << ", ";
//          cout << " ]\n";
//        }
//      );
//    }
//    return no_pred_expr_ids.size();
//  }

  static int compute_arity(map<expr_group_t::expr_idx_t, bv_const_ref> const& in_vec)
  {
    int ret = 0;
    for(auto const& val : in_vec) {
      if(val.second->get_bv() != bv_zero())
        ret++;
    }
    return ret;
  }

  //bool row_has_rank_0_exprs(vector<bv_const_ref> const& in_vec)
  //{
  //  expr_group_ref const& corr_expr_group = this->get_exprs_to_be_correlated();
  //  for(int i = 0; i < in_vec.size() -1 ; ++i) { // last element is constant term
  //    if(in_vec.at(i)->get_bv() != bv_zero() && !corr_expr_group->get_ranking_for_expr_at(i))
  //      return true;
  //  }
  //  return false;
  //}

  bool expr_index_has_some_predicate(expr_ref const& e, expr_idx_t e_index) const
  {
    ASSERT(e_index >= 0);
    unsigned int max_bvlen = this->get_exprs_to_be_correlated()->get_max_bvlen();
    unsigned int e_bvlen = e->is_bool_sort() ? 1 : e->get_sort()->get_size();
    for (auto const& row : m_bv_solve_output_matrix) {
      if (row.second.count(e_index) && row.second.at(e_index)->get_bv() != bv_zero()) {
        if (point_set_t::get_min_bvlen_for_generating_pred_from_coeff_row(row.second, max_bvlen) < e_bvlen) {
          continue;
        }
        return true;
      }
    }
    return false;
  }

  virtual dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>> clone(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) const override
  {
    PRINT_PROGRESS("%s %s() %d: bv_solve_output_matrix dim = %lu %lu\n", __FILE__, __func__, __LINE__, m_bv_solve_output_matrix.size(), (m_bv_solve_output_matrix.size() ? m_bv_solve_output_matrix.begin()->second.size() : 0));
    return make_dshared<smallest_point_cover_bv_t<T_PC, T_N, T_E, T_PRED>>(*this, point_set, out_of_bound_exprs);
  }

  virtual void smallest_point_cover_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type bv";
    os << "=" + prefix + "\n";
    this->smallest_point_cover_to_stream_helper(os, prefix + " ");
    this->state_elements_to_stream(os, prefix + " ");
  }

	/*static string smallest_point_cover_bv_from_stream(istream& is, context* ctx, list<nodece_ref<T_PC, T_N, T_E>> const &counterexamples, smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>*& eqclass)
	{
    return smallest_point_cover_from_stream_helper(is, ctx, counterexamples, eqclass);
  }*/
private:

  static void multiply_row_by_two_and_eliminate_if_zero_row(map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const_ref>>& matrix, bv_solve_var_idx_t rownum, bv_const const& mod)
  {
    ASSERT(matrix.count(rownum));
    auto& row = matrix.at(rownum);
    bool all_zero = true;
    for (auto& [idx, bvr] : row) {
      auto const& bv = bvr->get_bv();
      bv_const out;
      out = MUL(bv, 2, mod);
      if (out != 0) {
        all_zero = false;
      }
      bvr = mk_bv_const_ref(out);
    }
    if (all_zero) {
      matrix.erase(rownum);
    }
  }

  static expr_idx_t find_rightmost_expr_idx_with_non_zero_entry(map<bv_solve_var_idx_t, bv_const_ref> const& row)
  {
    set<bv_solve_var_idx_t, decltype(bvcmp)> indices(bvcmp);
    for (auto const& [k,v] : row) {
      indices.insert(k);
    }
    for (auto iter = indices.rbegin(); iter != indices.rend(); iter++) {
      if (row.at(*iter)->get_bv() != 0) {
        return *iter;
      }
    }
    NOT_REACHED();
  }

  void bvcover_weaken_till_arity_within_bound(int arity_bound) const
  {
    autostop_timer func_timer(demangled_type_name(typeid(*this))+"."+__func__);
    cout << _FNLN_ << ": bvcover_weaken_till_arity_within_bound() called with arity bound " << arity_bound << endl;
    bool arity_bound_violation;
    do {
      cout << _FNLN_ << ": outer loop iteration" << endl;
      arity_bound_violation = false;
      for (auto const& row : m_bv_solve_output_matrix) {
        unsigned arity = compute_arity(row.second);
        if(arity > arity_bound) {
          //expr_idx_t expr_idx = row.first;
          this->bvcover_weaken_row_till_arity_within_bound(row.first/*, expr_idx*/, arity_bound);
          arity_bound_violation = true;
          break; //break immediately because m_bv_solve_output_matrix could have changed
        }
      }
    } while (arity_bound_violation);
  }

  void bvcover_weaken_row_till_arity_within_bound(expr_group_t::expr_idx_t rownum/*, expr_group_t::expr_idx_t expr_idx*/, int arity_bound) const
  {
    unsigned int bvlen = this->get_exprs_to_be_correlated()->get_max_bvlen();
    const bv_const mod = bv_exp2(bvlen);

    auto const& row = m_bv_solve_output_matrix.at(rownum);

    do {
      cout << _FNLN_ << ": Weakening row at " << rownum << " till arity_bound " << arity_bound << endl;
      //cout << "m_bv_solve_output_matrix =\n";
      //bv_matrix_map_ref_to_stream(cout, m_bv_solve_output_matrix);

      expr_idx_t expr_idx = find_rightmost_expr_idx_with_non_zero_entry(row);
      //if (!expr_idx_has_non_zero_value_in_matrix_only_for_one_row(expr_idx, m_bv_solve_output_matrix, rownum)) {
      //  cout << _FNLN_ << ": found rightmost expr-idx " << expr_idx << " that has non-zero values at other rows while weakening row at " << rownum << " till arity_bound " << arity_bound << ", m_bv_solve_output_matrix =\n";
      //  bv_matrix_map_ref_to_stream(cout, m_bv_solve_output_matrix);
      //  NOT_REACHED();
      //}
      bv_const_ref coeff = row.at(expr_idx);
      ASSERT(coeff->get_bv() != 0);
      unsigned int coeff_rank = bv_rank(coeff->get_bv(), bvlen);
      ASSERT(coeff_rank < bvlen);

      coeff_rank++;
      ASSERT(coeff_rank <= bvlen);
      //if (coeff_rank < bvlen) {
        this->m_point_set.point_set_weaken_expr_idx_till_coeff_rank(expr_idx, coeff_rank, bvlen, this->get_visited_ces_ref());
      /*} else {
        ASSERT(coeff->get_bv() == bv_zero() || coeff->get_bv() == bv_exp2(bvlen - 1));
        //cout << _FNLN_ << ": bv cover masking out expr_idx " << expr_idx << endl;
        //this->bvcover_mask_out_expr_idx(expr_idx);
      }*/
      //m_bv_solve_output_matrix = this->m_point_set.solve_for_bv_points();

      multiply_row_by_two_and_eliminate_if_zero_row(m_bv_solve_output_matrix, rownum, mod);
      //this->bvcover_solve_and_update_output_matrix();
      //ASSERT(coeff_rank < bvlen || !m_bv_solve_output_matrix.count(rownum));
      if (   m_bv_solve_output_matrix.count(rownum)
          && bv_rank(m_bv_solve_output_matrix.at(rownum).at(expr_idx)->get_bv(), bvlen) != coeff_rank) {
        cout << _FNLN_ << ": WARNING: An assertion failed.  bv_rank(matrix.at(rownum).at(expr_idx)) = " << bv_rank(m_bv_solve_output_matrix.at(rownum).at(expr_idx)->get_bv(), bvlen) << endl;
        cout << "coeff_rank = " << coeff_rank << endl;
        cout << "rownum = " << rownum << ", expr_idx = " << expr_idx << ", matrix size = " << m_bv_solve_output_matrix.size() << ", matrix.at(rownum).size = " << m_bv_solve_output_matrix.at(rownum).size() << ", bvlen = " << bvlen << endl;
        cout << "m_bv_solve_output_matrix =\n";
        bv_matrix_map_ref_to_stream(cout, m_bv_solve_output_matrix);
        cout << endl;
      }
      //ASSERT(!m_bv_solve_output_matrix.count(expr_idx) || bv_rank(m_bv_solve_output_matrix.at(expr_idx).at(expr_idx)->get_bv(), bvlen) == coeff_rank);
      ASSERT(!m_bv_solve_output_matrix.count(rownum) || bv_rank(m_bv_solve_output_matrix.at(rownum).at(expr_idx)->get_bv(), bvlen) == coeff_rank);
    } while (m_bv_solve_output_matrix.count(rownum) && compute_arity(m_bv_solve_output_matrix.at(rownum)) > arity_bound);
    bvcover_solve_and_update_output_matrix();
  }

  //void bvcover_mask_out_expr_idx(expr_group_t::expr_idx_t expr_idx) const
  //{
  //  m_masked_expr_idxs.insert(expr_idx);
  //}

  void
  bvcover_solve_and_update_output_matrix() const
  {
    for (auto const& [ptname, pt] : this->m_point_set.get_points_vec()) {
      ASSERT(ptname == pt->get_point_name());
      // All exprs should be defined in a visited point
      if(this->get_visited_ces().count(pt->get_point_name()) && !this->point_has_mapping_for_all_exprs(pt))
      {
        cout <<_FNLN_ << ": Current point: " << pt->get_point_name()->get_str() << ", Full Point set: \n" <<  this->m_point_set.point_set_to_string("") << endl;
        cout.flush();
      }

      ASSERT(!this->get_visited_ces().count(pt->get_point_name()) || this->point_has_mapping_for_all_exprs(pt));
    }

    m_bv_solve_output_matrix = this->m_point_set.solve_for_bv_points(this->get_exprs_to_be_correlated(), this->get_visited_ces(), this->m_out_of_bound_exprs);
    //for (auto masked_expr_idx : m_masked_expr_idxs) {
    //  m_bv_solve_output_matrix.erase(masked_expr_idx);
    //}
  }

  void state_elements_to_stream(ostream& os, string const& prefix) const
  {
    //os << "=" << prefix << "masked expr ids\n";
    //for (auto const& mei : m_masked_expr_idxs) {
    //  os << " " << mei;
    //}
    //os << endl;
    os << "=" << prefix << "output matrix\n";
    bv_matrix_map_ref_to_stream(os, m_bv_solve_output_matrix);
    //for (auto const& ivec : m_bv_solve_output_matrix) {
    //  os << ivec.first << " ";
    //  for (auto const& b : ivec.second) {
    //    os << " " << b->get_bv();
    //  }
    //  os << endl;
    //}
    os << "=" << prefix << "state elems done\n";
  }
  void state_elements_from_stream(istream& is, string const& prefix)
  {
    string line;
    bool end;

    do {
      end = !getline(is, line);
      ASSERT(!end);
    } while (line == "");
    //ASSERT(string_has_prefix(line, string("=") + prefix + "masked expr ids"));

    //end = !getline(is, line);
    //ASSERT(!end);

    //istringstream iss(line);
    //expr_group_t::expr_idx_t idx;
    //while (iss >> idx) {
    //  bool inserted = m_masked_expr_idxs.insert(idx).second;
    //  ASSERT(inserted);
    //}

    //while (line.at(0) != '=') {
    //  istringstream iss(line);
    //  expr_idx_t eidx;
    //  unsigned count;
    //  iss >> eidx;
    //  iss >> count;
    //  bool inserted = m_masked_expr_ids.insert(make_pair(eidx, count)).count;
    //  ASSERT(inserted);

    //  end = !getline(is, line);
    //  ASSERT(!end);
    //}

    //end = !getline(is, line);
    //ASSERT(!end);

    if (!string_has_prefix(line, string("=") + prefix + "output matrix")) {
      cout << _FNLN_ << ": line =\n" << line << endl;
      cout << "prefix =\n" << prefix << endl;
    }
    ASSERT(string_has_prefix(line, string("=") + prefix + "output matrix"));

    m_bv_solve_output_matrix = bv_matrix_map_ref_from_stream(is);

    end = !getline(is, line);
    ASSERT(!end);
    ASSERT(string_has_prefix(line, string("=") + prefix + "state elems done"));
  }

  //bool
  //is_masked_predicate_row(bv_solve_var_idx_t const& free_var_idx, map<expr_group_t::expr_idx_t, bv_const_ref> const& coeff_row) const
  //{
  //  if (free_var_idx == (expr_idx_t)-1) {
  //    return false;
  //  }
  //  unsigned arity = compute_arity(coeff_row);
  //  if (   m_masked_expr_ids.count(free_var_idx) > 0
  //      && arity >= m_masked_expr_ids.at(free_var_idx)) {
  //    return true;
  //  }
  //  return false;
  //}

  //void
  //eliminate_preds_for_masked_expr_ids(unordered_set<shared_ptr<T_PRED const>> &preds) const
  //{
  //  set<shared_ptr<T_PRED const>> preds_to_mask;
  //  //cout << __func__ << " " << __LINE__ << ": before preds.size() = " << preds.size() << endl;
  //  for (auto const& pred : preds) {
  //    if (pred == predicate::false_predicate(this->m_ctx)) {
  //      continue;
  //    }
  //    expr_idx_t expr_idx = get_free_var_idx_from_pred_comment(pred->get_comment());
  //    if (expr_idx == (expr_idx_t)-1) {
  //      continue;
  //    }
  //    unsigned arity = get_arity_from_pred_comment(pred->get_comment());
  //    if (   m_masked_expr_ids.count(expr_idx) > 0
  //        && arity >= m_masked_expr_ids.at(expr_idx)) {
  //      DYN_DEBUG(eqclass_bv, cout << __func__ << " " << __LINE__ << ": erasing pred due to index " << expr_idx << ":\n" << pred->pred_to_string("  ") << endl);
  //      preds_to_mask.insert(pred);
  //    }
  //  }
  //  for (auto const& pred : preds_to_mask) {
  //    preds.erase(pred);
  //  }
  //}

  bool recomputed_preds_would_be_different_from_current_preds() override
  {
    autostop_timer func_timer(demangled_type_name(typeid(*this))+"."+__func__);
    map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const_ref>> old_bv_solve_output_matrix = m_bv_solve_output_matrix;
//    DYN_DEBUG2(eqclass_bv, cout << __func__ << " " << __LINE__ << ": point set =\n" << this->m_point_set.point_set_to_string("  ") << endl);
    //m_bv_solve_output_matrix = this->m_point_set.solve_for_bv_points();
    this->bvcover_solve_and_update_output_matrix();
    bvcover_weaken_till_arity_within_bound(BV_PRED_ARITY_THRESHOLD);
    ///*int num_rows_deleted = */remove_higher_arity_pred_rows();
    if (m_bv_solve_output_matrix != old_bv_solve_output_matrix) {
      this->smallest_point_cover_mark_preds_out_of_sync();
//      /*int num_exprs_removed = */remove_exprs_with_no_predicate();
      return true;
    } else {
      return false;
    }
  }

  //returns TRUE if we are done
  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>>& preds) const override
  {
    autostop_timer func_timer(demangled_type_name(typeid(*this))+"."+__func__);

    if (last_proof_success) {
      return true;
    }

    if (this->m_timeout_info.timeout_occurred()) {
      DYN_DEBUG(eqclass_bv, cout << _FNLN_ << ": timeout occurred\n");
      autostop_timer func_timer(demangled_type_name(typeid(*this))+"."+string(__func__)+string(".timeout_occured"));
      unordered_set<shared_ptr<T_PRED const>> const& timeout_preds = this->m_timeout_info.get_preds_that_timed_out();
      for (auto const& timeout_pred : timeout_preds) {
        if (timeout_pred == predicate::false_predicate(this->m_ctx)) {
          cout << _FNLN_ << ": Unfortunately, it seems that the false-predicate itself timed out!  Can't handle this!\n";
          NOT_REACHED(); //hopefully, proof for the false predicate should never timeout!
        } else {
          expr_idx_t row_id = get_free_var_idx_from_pred_comment(timeout_pred->get_comment());
          DYN_DEBUG(eqclass_bv, cout << _FNLN_ << ": weakening row " << row_id << "because timeout occurred\n");
          if (m_bv_solve_output_matrix.count(row_id)) { //this row (row_id) may have been eliminated due to previous weakening
            bvcover_weaken_row_till_arity_within_bound(row_id, BV_PRED_ARITY_THRESHOLD);
          }
        }
      }
    }

    //cout << _FNLN_ << ": calling compute_preds_for_bv_points()\n";
//    DYN_DEBUG(eqclass_bv, cout << __func__ << " " << __LINE__ << ": point set =\n" << this->m_point_set.point_set_to_string("  ") << endl);
    auto const& corresponding_exprs = this->get_exprs_to_be_correlated();
    DYN_DEBUG(eqclass_bv, cout << __func__ << " " << __LINE__ << ": exprs_to_be_correlated=\n" << corresponding_exprs->expr_group_to_string());
    DYN_DEBUG(eqclass_bv, cout << __func__ << " " << __LINE__ << ": ret_matrix from bv solver=\n";
      cout << "       ";
      for (auto const& [i,pe] : corresponding_exprs->get_expr_vec()) {
        cout << setw(10) << i << ", " ;
      }
      cout << setw(10) << "constant\n";
      for (auto const& p : m_bv_solve_output_matrix) {
        cout << setw(2) << p.first;
        cout  << ":  [ " ;
        for (auto const& c : p.second)
          cout << setw(10) << c.first << "->" << c.second->get_bv() << ", ";
        cout << " ]\n";
      }
    );
    preds = this->m_point_set.compute_preds_for_bv_points(m_bv_solve_output_matrix, corresponding_exprs);
    //cout << _FNLN_ << ": done calling compute_preds_for_bv_points()\n";
    //this->eliminate_preds_for_masked_expr_ids(preds);

    DYN_DEBUG(eqclass_bv,
      cout << __func__ << " " << __LINE__ << ": returning preds =\n";
      size_t n = 0;
      for (auto const& pred : preds) {
        //if (pred == predicate::false_predicate(this->m_ctx)) {
          cout << "Pred " << n << ":\n" << pred->pred_to_string("  ");
        //} else {
        //  expr_idx_t expr_idx = get_free_var_idx_from_pred_comment(pred->get_comment());
        //  cout << "Pred " << n << ": expr " << expr_string(this->get_exprs_to_be_correlated()->at(expr_idx)) << "\n" << pred->pred_to_string("  ");
        //}
        n++;
      }
    );
    return false;
  }

  static expr_idx_t
  get_free_var_idx_from_pred_comment(string const& comment)
  {
    size_t found = comment.find(FREE_VAR_IDX_PREFIX);
    if (found == string::npos) {
      return (expr_idx_t)-1;
      //cout << __func__ << " " << __LINE__ << ": comment = " << comment << ".\n";
    }
    ASSERT(found != string::npos);
    return string_to_int(comment.substr(found + strlen(FREE_VAR_IDX_PREFIX)));
  }
  static unsigned
  get_arity_from_pred_comment(string const& comment)
  {
    size_t prefix = comment.find(PRED_COMMENT_LINEAR);
    ASSERT(prefix != string::npos);
    unsigned ret;
    string wo_prefix = comment.substr(prefix + strlen(PRED_COMMENT_LINEAR));
    istringstream ss(wo_prefix);
    ss >> ret;
    ASSERTCHECK(ret, cout << "wo_prefix = " << wo_prefix << "; comment = " << comment << endl);
    return ret;
  }
};

}
