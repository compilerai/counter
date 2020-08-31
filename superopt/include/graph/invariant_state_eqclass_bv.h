#pragma once
#include <map>
#include <string>

#include "support/bv_const_ref.h"
#include "support/dyn_debug.h"
#include "graph/invariant_state_eqclass.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_bv_t : public invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>
{
private:
  matrix_t<bool> m_coeff_matrix; // FIXME same info as m_bv_solve_output_matrix! get rid of one of them
  map<expr_group_t::expr_idx_t,unsigned> m_masked_expr_ids;
  map<bv_solve_var_idx_t, vector<bv_const_ref>> m_bv_solve_output_matrix;
public:
  invariant_state_eqclass_bv_t(expr_group_ref const &exprs_to_be_correlated, context *ctx) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(exprs_to_be_correlated, ctx)
  { }

  invariant_state_eqclass_bv_t(istream& is, string const& inprefix/*, state const& start_state*/, context* ctx) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(is, inprefix/*, start_state*/, ctx)
  { }

  unsigned num_exprs_ignoring_bool_sort() const
  {
    unsigned ret = 0;
    for(size_t idx = 0; idx < this->get_exprs_to_be_correlated()->size(); idx++)
    {
      expr_ref e = this->get_exprs_to_be_correlated()->at(idx);
      if(!e->is_bool_sort())
        ret++;
    }
    return ret;
  }
  unsigned exprs_has_no_predicate() const
  {
    unsigned ret = 0;
    for(size_t idx = 0; idx < this->get_exprs_to_be_correlated()->size(); idx++)
    {
      if(!this->expr_index_has_some_predicate(idx))
        ret++;
    }
    return ret;
  }
  
  bool expr_index_has_some_predicate(expr_group_t::expr_idx_t e_index) const
  {
    if (m_coeff_matrix.get_rows() == 0) {
      return false;
    }
    return m_coeff_matrix.matrix_column_is_non_zero(e_index);
  }

  virtual void update_state_for_ranking() override
  {
    unordered_set<shared_ptr<T_PRED const>> unused;
    recompute_preds_for_points(false, unused);
  }

  virtual invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> *clone() const override
  {
    PRINT_PROGRESS("%s %s() %d: bv_solve_output_matrix dim = %lu %lu\n", __FILE__, __func__, __LINE__, m_bv_solve_output_matrix.size(), (m_bv_solve_output_matrix.size() ? m_bv_solve_output_matrix.begin()->second.size() : 0));
    return new invariant_state_eqclass_bv_t<T_PC, T_N, T_E, T_PRED>(*this);
  }

  virtual void invariant_state_eqclass_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type bv";
    os << "=" + prefix + "\n";
    this->invariant_state_eqclass_to_stream_helper(os, prefix + " ");
  }
 
	/*static string invariant_state_eqclass_bv_from_stream(istream& is, context* ctx, list<nodece_ref<T_PC, T_N, T_E>> const &counterexamples, invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>*& eqclass)
	{
    return invariant_state_eqclass_from_stream_helper(is, ctx, counterexamples, eqclass);
  }*/

private:
  void
  eliminate_preds_for_masked_expr_ids(unordered_set<shared_ptr<T_PRED const>> &preds) const
  {
    set<shared_ptr<T_PRED const>> preds_to_mask;
    //cout << __func__ << " " << __LINE__ << ": before preds.size() = " << preds.size() << endl;
    for (auto const& pred : preds) {
      if (pred == predicate::false_predicate(this->m_ctx)) {
        continue;
      }
      expr_group_t::expr_idx_t expr_idx = get_free_var_idx_from_pred_comment(pred->get_comment());
      unsigned arity = get_arity_from_pred_comment(pred->get_comment());
      if (   m_masked_expr_ids.count(expr_idx) > 0
          && arity >= m_masked_expr_ids.at(expr_idx)) {
        DYN_DEBUG(invariant_state_masked_expr, cout << __func__ << " " << __LINE__ << ": erasing pred due to index " << expr_idx << ":\n" << pred->pred_to_string("  ") << endl);
        preds_to_mask.insert(pred);
      }
    }
    for (auto const& pred : preds_to_mask) {
      preds.erase(pred);
    }
  }

  virtual bool recomputed_preds_would_be_different_from_current_preds() override
  {
    autostop_timer func_timer(demangled_type_name(typeid(*this))+"."+__func__);
    map<bv_solve_var_idx_t, vector<bv_const_ref>> old_bv_solve_output_matrix = m_bv_solve_output_matrix;
    m_bv_solve_output_matrix = this->m_point_set.solve_for_bv_points();
    return (m_bv_solve_output_matrix != old_bv_solve_output_matrix);
  }

  //returns TRUE if we are done
  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>>& preds) override
  {
    autostop_timer func_timer(demangled_type_name(typeid(*this))+"."+__func__);

    if (last_proof_success) {
      return true;
    }

    if (this->m_timeout_info.timeout_occurred()) {
      unordered_set<shared_ptr<T_PRED const>> const& timeout_preds = this->m_timeout_info.get_preds_that_timed_out();
      for (auto const& timeout_pred : timeout_preds) {
        if (timeout_pred == predicate::false_predicate(this->m_ctx)) {
          NOT_REACHED(); //hopefully, proof for the false predicate should never timeout!
        } else {
          expr_group_t::expr_idx_t expr_idx = get_free_var_idx_from_pred_comment(timeout_pred->get_comment());
          unsigned arity = get_arity_from_pred_comment(timeout_pred->get_comment());
          CPP_DBG_EXEC(EQCLASS_BV, cout << __func__ << " " << __LINE__ << ": masking out expr at index " << expr_idx << ": " << expr_string(this->get_exprs_to_be_correlated()->at(expr_idx)) << " for relations with arity >= " << arity << endl);
          ASSERT(!m_masked_expr_ids.count(expr_idx) || m_masked_expr_ids.at(expr_idx) > arity); //if expr_idx is already masked, then this pred's arity must have been less than previous masked arity
          m_masked_expr_ids[expr_idx] = arity;
        }
      }
    }

    m_coeff_matrix.clear();
    preds = this->m_point_set.compute_preds_for_bv_points(m_coeff_matrix, m_bv_solve_output_matrix);
    this->eliminate_preds_for_masked_expr_ids(preds);

    CPP_DBG_EXEC(EQCLASS_BV,
      cout << __func__ << " " << __LINE__ << ": returning preds =\n";
      size_t n = 0;
      for (auto const& pred : preds) {
        if (pred == predicate::false_predicate(this->m_ctx)) {
          cout << "Pred " << n << ":\n" << pred->pred_to_string("  ");
        } else {
          expr_group_t::expr_idx_t expr_idx = get_free_var_idx_from_pred_comment(pred->get_comment());
          cout << "Pred " << n << ": expr " << expr_string(this->get_exprs_to_be_correlated()->at(expr_idx)) << "\n" << pred->pred_to_string("  ");
        }
        n++;
      }
    );
    return false;
  }

  static expr_group_t::expr_idx_t
  get_free_var_idx_from_pred_comment(string const& comment)
  {
    size_t found = comment.find(FREE_VAR_IDX_PREFIX);
    if (found == string::npos) {
      cout << __func__ << " " << __LINE__ << ": comment = " << comment << ".\n";
    }
    ASSERT(found != string::npos);
    return string_to_int(comment.substr(found + strlen(FREE_VAR_IDX_PREFIX)));
  }
  static unsigned
  get_arity_from_pred_comment(string const& comment)
  {
    ASSERT(string_has_prefix(comment, PRED_LINEAR_PREFIX));
    unsigned ret;
    string wo_prefix = comment.substr(strlen(PRED_LINEAR_PREFIX));
    istringstream ss(wo_prefix);
    ss >> ret;
    ASSERT(ret != 0);
    return ret;
  }
};

}
