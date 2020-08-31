#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "graph/point_set.h"
#include "graph/expr_group.h"
#include "graph/proof_timeout_info.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_arr_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_bv_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_ineq_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_ineq_loose_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_t
{
protected:
  using expr_idx_t = size_t;
  point_set_t m_point_set;
  proof_timeout_info_t m_timeout_info;
  context *m_ctx;

private:
  unordered_set<shared_ptr<T_PRED const>> m_preds; // IMPORTANT: no eqclass is allowed to directly modify m_preds
  bool m_preds_modified_by_propagation;

public:
  invariant_state_eqclass_t(expr_group_ref const &exprs_to_be_correlated, context *ctx)
  : m_point_set(exprs_to_be_correlated, ctx), m_timeout_info(ctx), m_ctx(ctx),
    m_preds_modified_by_propagation(false)
  {
    stats_num_invariant_state_eqclass_constructions++;
    m_preds.insert(T_PRED::false_predicate(m_ctx));
  }

  virtual ~invariant_state_eqclass_t()
  {
    stats_num_invariant_state_eqclass_destructions++;
  }

  invariant_state_eqclass_t(invariant_state_eqclass_t const &other) : m_point_set(other.m_point_set),
                                                                      m_timeout_info(other.m_timeout_info),
                                                                      m_ctx(other.m_ctx),
                                                                      m_preds(other.m_preds),
                                                                      m_preds_modified_by_propagation(other.m_preds_modified_by_propagation)
  {
    stats_num_invariant_state_eqclass_constructions++;
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    ASSERT(m_point_set.size() == other.m_point_set.size());
  }

  invariant_state_eqclass_t(istream& is, string const& prefix/*, state const& start_state*/, context* ctx) : m_point_set(is, prefix, ctx), m_timeout_info(ctx), m_ctx(ctx), m_preds(T_PRED::predicate_set_from_stream(is/*, start_state*/, ctx, prefix)), m_preds_modified_by_propagation(false)
  {
    stats_num_invariant_state_eqclass_constructions++;
  }

  virtual invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> *clone() const = 0;

  void operator=(invariant_state_eqclass_t const &other)
  {
    //ASSERT(&m_graph == &other.m_graph);
    m_point_set = other.m_point_set;
    ASSERT(m_point_set.size() == other.m_point_set.size());
    m_ctx = other.m_ctx;
    m_preds = other.m_preds;
    m_preds_modified_by_propagation = other.m_preds_modified_by_propagation;
  }

  expr_group_ref const&
  get_exprs_to_be_correlated() const
  {
    return m_point_set.get_exprs();
  }

  unordered_set<shared_ptr<T_PRED const>> const &
  get_preds() const
  {
    return m_preds;
  }
  
  void
  clear_preds()
  {
    m_preds.clear();
  }

  string eqclass_type_to_string() const
  {
    return expr_group_t::expr_group_type_to_string(this->get_exprs_to_be_correlated()->get_type());
  }

  string eqclass_exprs_to_string() const
  {
    stringstream ss;
    expr_group_ref const &exprs_to_be_correlated = m_point_set.get_exprs();
    ss <<  "eqclass type " << eqclass_type_to_string();
    ss << ", exprs [" << exprs_to_be_correlated->size() << "]: ";
    for (size_t i = 0; i < exprs_to_be_correlated->size(); ++i) {
      expr_ref const& e = exprs_to_be_correlated->at(i);
      ss << expr_string(e);
      ss << " ; ";
    }
    return ss.str();
  }

  bool
  eqclass_xfer_on_incoming_edge(shared_ptr<T_E const> const &e, invariant_state_t<T_PC, T_N, T_E, T_PRED> const &from_pc_state, graph_with_ce<T_PC, T_N, T_E, T_PRED> const &g)
  {
    return eqclass_xfer_on_incoming_edge_helper(e, from_pc_state, {}, {}, g);
  }

  // any override should call this function eventually
  virtual bool invariant_state_eqclass_add_point_using_CE(nodece_ref<T_PC, T_N, T_E> const &nce, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g)
  {
    autostop_timer func_timer(string(__FILE__) + "." + __func__);
    DYN_DEBUG2(add_point_using_ce, cout << __func__ << " " << __LINE__ << ": adding point using CE\n" << this->eqclass_exprs_to_string() << endl);
    DYN_DEBUG2(add_point_using_ce, cout << __func__ << " " << __LINE__ << ": old point set =\n" << m_point_set.point_set_to_string("  ") << endl);

    bool point_added = m_point_set.template add_point_using_CE<T_PC, T_N, T_E, T_PRED>(nce, g);
    if (!point_added) {
      DYN_DEBUG(add_point_using_ce,
        cout << __func__ << " " << __LINE__ << ": " << this->eqclass_type_to_string() << ": add_point_using_CE failed; point discarded\n"
      );
      return false;
    }
    bool preds_modified = this->recomputed_preds_would_be_different_from_current_preds();
    if (preds_modified) {
      DYN_DEBUG(add_point_using_ce,
        cout << __func__ << " " << __LINE__ << ": " << this->eqclass_type_to_string() << ": retained point\n"
      );
      DYN_DEBUG2(add_point_using_ce, cout << __func__ << " " << __LINE__ << ": new point set =\n" << m_point_set.point_set_to_string("  ") << endl);
      DYN_DEBUG(add_point_using_ce, cout << __func__ << " " << __LINE__ << ": " << this->eqclass_exprs_to_string() << ": new preds: " << this->eqclass_preds_to_string("  ", m_preds) << endl);
      return true;
    } else {
      DYN_DEBUG(add_point_using_ce,
        cout << __func__ << " " << __LINE__ << ": " << this->eqclass_type_to_string() << ": new_preds == old_preds; point discarded\n"
      );
      this->invariant_state_eqclass_pop_back_last_point();
      return false;
    }
  }

  virtual void invariant_state_eqclass_pop_back_last_point()
  {
    m_point_set.pop_back(); //get rid of this point as it adds no value
  }

  bool operator!=(invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> const &other) const
  {
    //ASSERT(m_point_set.get_exprs() == other.get_exprs_to_be_correlated());
    //Cannot assert this  as the size of expr_group may change when new CE is added
    return (m_preds != other.m_preds) || (m_preds_modified_by_propagation != other.m_preds_modified_by_propagation);
  }

	string invariant_state_eqclass_to_string(string const &prefix, bool with_points) const;
  void invariant_state_eqclass_get_asm_annotation(ostream& os) const;

  virtual void invariant_state_eqclass_to_stream(ostream& os, string const& inprefix) const = 0;

  static invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>* invariant_state_eqclass_from_stream(istream& is, string const& prefix/*, state const& start_state*/, context* ctx, map<graph_loc_id_t, graph_cp_location> const& locs);

  bool invariant_state_eqclass_is_well_formed() const
  {
    for (auto const& pred : m_preds) {
      shared_ptr<T_PRED const> false_pred = T_PRED::false_predicate(m_ctx);
      if (pred == false_pred) {
        CPP_DBG_EXEC(INVARIANTS_MAP, cout << __func__ << " " << __LINE__ << ": returning false for:\n" << this->invariant_state_eqclass_to_string("  ", true) << endl);
        return false;
      }
    }
    return true;
  }

  virtual bool check_stability() const { return true; } // stability must be computed from latest set of points

  // returns TRUE if preds were changed
  bool sync_preds()
  {
    if (m_preds_modified_by_propagation) {
      // if m_preds were already modified during propagation
      // we need not update the preds again
      m_preds_modified_by_propagation = false;
      return true;
    } else {
      unordered_set<shared_ptr<T_PRED const>> old_preds = m_preds;
      this->recompute_preds_for_points(false, m_preds);
      return (old_preds != m_preds);
    }
  }

  virtual void update_state_for_ranking() { }

protected:
  void invariant_state_eqclass_to_stream_helper(ostream& os, string const& prefix) const
  {
    m_point_set.point_set_to_stream(os, prefix);
    T_PRED::predicate_set_to_stream(os, prefix, m_preds);
  }
private:
  static string eqclass_preds_to_string(string const& prefix, unordered_set<shared_ptr<T_PRED const>> const& preds)
  {
    stringstream ss;
    ss << prefix << preds.size() << " preds:\n";
    size_t n = 0;
    for (const auto &ps : preds) {
      string new_prefix = prefix + "    ";
      ss << prefix << "  " << n << ".:"
         << ps->pred_to_string(" ");
      n++;
    }
    return ss.str();
  }

  bool eqclass_xfer_on_incoming_edge_helper(shared_ptr<T_E const> const &e, invariant_state_t<T_PC, T_N, T_E, T_PRED> const &from_pc_state, set<expr_idx_t> const &ignore_exprs, unordered_set<shared_ptr<T_PRED const>> const &preds_that_need_no_proof, graph_with_ce<T_PC, T_N, T_E, T_PRED> const &g)
  {
    //DYN_DEBUG(invariant_state_eqclass, cout << __func__ << ':' << __LINE__ << ": " << timestamp() << ": " << this->eqclass_exprs_to_string() << endl);
    T_PC const &from_pc = e->get_from_pc();
    T_PC const &to_pc = e->get_to_pc();
    bool changed = this->sync_preds();
    unordered_set<shared_ptr<T_PRED const>> orig_old_preds = m_preds;

    while (m_preds.size()) {
      //except the first iteration, the size of CEs is strictly increasing
      size_t CEs_size = m_point_set.size();

      CPP_DBG_EXEC(CE_ADD, cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << endl);
      CPP_DBG_EXEC(CE_ADD, cout << __func__ << " " << __LINE__ << ": m_point_set.size() = " << m_point_set.size() << endl);
      CPP_DBG_EXEC(CE_ADD, cout << __func__ << " " << __LINE__ << ": m_preds.size() = " << m_preds.size() << endl);
      bool proof_succeeded = true;
      m_timeout_info.clear();
      unordered_set<shared_ptr<T_PRED const>> old_preds = m_preds;
      for (const auto &pred : m_preds) {
        if (unordered_set_belongs(preds_that_need_no_proof, pred)) {
          continue;
        }
        PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
        bool guess_made_weaker;
        proof_status_t proof_status = g.decide_hoare_triple({}, from_pc, mk_edge_composition<T_PC,T_E>(e), pred, guess_made_weaker);
        if (proof_status == proof_status_timeout) {
          m_timeout_info.register_timeout(pred);
        }
        PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
        if (proof_status != proof_status_proven) {
          proof_succeeded = false;
          break;
        }
        //It is possible for the false-predicate to be proven on this path (even though the dst edge did not prove false). This can happen because
        //of extra assumes that are encountered on the path (e.g., aliasing constraints) that make this path-condition false. Thus commenting
        //out the following assert; notice that based on this, we will only get a false invariant at this node which will cause all outgoing
        //dst edges to prove false; so there is no performance problem
        //ASSERT(pred.get_comment() != "false-predicate");
      }
      if (proof_succeeded) {
        if (this->recompute_preds_for_points(true, m_preds)) {
          break;
        }
        ASSERT(old_preds != m_preds);
      } else {
        this->recompute_preds_for_points(false, m_preds);
        PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
        if(g.check_stability_for_added_edge(e))
          ASSERT(old_preds != m_preds);
        else
          break;
      }
    }
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    unordered_set_union(m_preds, preds_that_need_no_proof);
    DYN_DEBUG2(invariant_state_eqclass, cout << _FNLN_ << ": provable preds (" << m_preds.size() << ")[" << changed << "][" << (orig_old_preds != m_preds) << "]:\n"; for (auto const& p : m_preds) cout << p->pred_to_string("", 80); cout << endl);
    changed = changed || (m_preds != orig_old_preds);
    return changed;
  }

  // Virtual functions to be overriden in dervied classes
  // NOTE: we allow eqclasses to generate their preds either lazily or eagerly
  // the only requirement is that the following functions specs are respected

  // set preds to latest preds value
  // returns TRUE if we are done when last_proof_success is TRUE
  // NOTE: this function is supposed to be pure in the sense that consecutive calls
  // without updating points should return same value
  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>> &preds) = 0;

  // return TRUE if preds computed from latest points would be different from
  // preds generated before adding last point
  // This function is ALWAYS called after a point is added and can be used as a proxy for it
  // as we only need the modification information, m_preds need not be modified here
  // this allows an implementation to be lazy about pred generation
  virtual bool recomputed_preds_would_be_different_from_current_preds()
  {
    unordered_set<shared_ptr<T_PRED const>> old_preds = m_preds;
    recompute_preds_for_points(false, m_preds); // m_preds must be updated to reflect change in old_preds in next call
    if (old_preds != m_preds) {
      m_preds_modified_by_propagation = true; // m_preds was modified; next sync should consider this
      return true;
    } else return false;
  }
  //void replay_counter_examples(list<nodece_ref<T_PC, T_N, T_E>> const& counterexamples);
};

}
