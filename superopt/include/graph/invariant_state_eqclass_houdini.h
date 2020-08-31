#pragma once
#include <map>
#include <string>

#include "graph/invariant_state_eqclass.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_houdini_t : public invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>
{
public:
  invariant_state_eqclass_houdini_t(expr_group_ref const &exprs_to_be_correlated, context *ctx) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(exprs_to_be_correlated, ctx)
  { }

  invariant_state_eqclass_houdini_t(istream& is, string const& inprefix/*, state const& start_state*/, context* ctx) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(is, inprefix/*, start_state*/, ctx)
  { }

  virtual invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> *clone() const override
  {
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return new invariant_state_eqclass_houdini_t<T_PC, T_N, T_E, T_PRED>(*this);
  }

  virtual void invariant_state_eqclass_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type houdini";
    os << "=" + prefix + "\n";
    this->invariant_state_eqclass_to_stream_helper(os, prefix + " ");
  }

private:

  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>>& preds) override
  {
    autostop_timer func_timer(demangled_type_name(typeid(*this))+"."+__func__);
    if (last_proof_success) {
      return true;
    }

    if (this->m_timeout_info.timeout_occurred()) {
      NOT_IMPLEMENTED(); //hopefully, ismemlabel query will not timeout!
    }

    vector<point_ref> points = this->m_point_set.get_points_vec();
    size_t num_exprs = this->get_exprs_to_be_correlated()->size();

    preds.clear();
    for (size_t i = 0; i < num_exprs; i++) {
      bool gen_pred = true;
      for (const auto &pt : points) {
        if(pt->at(i).get_bv_const() == bv_zero()){
          gen_pred = false;
          break;
        }
      }
      if(gen_pred){
        predicate_ref pred = predicate::mk_predicate_ref(precond_t(), this->get_exprs_to_be_correlated()->at(i), expr_true(this->m_ctx), "houdini-guess");
        preds.insert(pred);
      }
    }
    CPP_DBG_EXEC(EQCLASS_HOUDINI,
      cout << __func__ << " " << __LINE__ << ": returning preds =\n";
      size_t n = 0;
      for (auto const& pred : preds) {
        cout << "Pred " << n << ":\n" << pred->pred_to_string("  ");
        n++;
      }
    );
    return false;
  }
};

}
