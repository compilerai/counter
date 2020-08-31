#include "eq/cg_with_safety.h"
#include "eq/corr_graph.h"
#include "tfg/predicate_set.h"
#include "eq/unsafe_cond.h"

namespace eqspace {

//class guarded_expr_t
//{
//private:
//  shared_ptr<tfg_edge_composition_t> m_path; //used to ensure finiteness of weakest-precondition chains
//  predicate m_unsafe_cond;
//public:
//  guarded_expr_t(shared_ptr<tfg_edge_composition_t> const& path, predicate const& unsafe_cond) : m_path(path), m_unsafe_cond(unsafe_cond)
//  {
//  }
//  shared_ptr<tfg_edge_composition_t> const& get_path() const { return m_path; }
//  predicate const& get_unsafe_cond() const { return m_unsafe_cond; }
//  bool path_contains_or_is_contained_in(guarded_expr_t const& other) const
//  {
//    shared_ptr<tfg_edge_composition_t> combo = mk_parallel(this->m_path, other.m_path);
//    if (combo == other.m_path) {
//      return true;
//    }
//    return false;
//  }
//  bool operator<(guarded_expr_t const& other) const
//  {
//    return    m_path < other.m_path
//           || (m_path == other.m_path && m_unsafe_cond < other.m_unsafe_cond)
//    ;
//  }
//  bool operator==(guarded_expr_t const& other) const
//  {
//    return    m_path == other.m_path
//           && m_unsafe_cond == other.m_unsafe_cond;
//  }
//  static list<guarded_expr_t> guarded_expr_ls_meet(list<guarded_expr_t> const& a, list<guarded_expr_t> const& b)
//  {
//    list<guarded_expr_t> ret = a;
//    set<size_t> b_indices_already_added;
//    for (auto& r : ret) {
//      size_t bi = 0;
//      for (auto const& belem : b) {
//        if (belem.get_unsafe_cond() == r.get_unsafe_cond()) {
//          r.m_path = mk_parallel(r.m_path, belem.m_path);
//          b_indices_already_added.insert(bi);
//        }
//        bi++;
//      }
//    }
//    size_t bi = 0;
//    for (auto const& belem : b) {
//      if (!set_belongs(b_indices_already_added, bi)) {
//        ret.push_back(belem);
//      }
//      bi++;
//    }
//    return ret;
//  }
//};

class unsafe_vals_intersect_t
{
private:
  predicate_set_t m_unsafe_preds;
  bool m_is_top;
public:
  unsafe_vals_intersect_t() : m_is_top(false)
  { }
  unsafe_vals_intersect_t(predicate_set_t const& unsafe_preds) : m_unsafe_preds(unsafe_preds), m_is_top(false)
  { }

  static unsafe_vals_intersect_t top()
  {
    unsafe_vals_intersect_t ret;
    ret.m_is_top = true;
    return ret;
  }

  predicate_set_t const& get_preds() const { return m_unsafe_preds; }

  bool is_empty() const { ASSERT(!m_is_top); return m_unsafe_preds.empty(); }
  bool meet(unsafe_vals_intersect_t const& other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    predicate_set_t old_unsafe_preds = m_unsafe_preds;
    predicate_set_intersect(m_unsafe_preds, other.m_unsafe_preds);
    return old_unsafe_preds != m_unsafe_preds;
  }
};

class unsafe_vals_union_t
{
private:
  predicate_set_t m_unsafe_preds;
  bool m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled; //true value serves as bottom
  bool m_is_top;
public:
  unsafe_vals_union_t() : m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled(false), m_is_top(false) { }
  unsafe_vals_union_t(predicate_set_t const& unsafe_preds) : m_unsafe_preds(unsafe_preds), m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled(false), m_is_top(false)
  { }
  unsafe_vals_union_t(predicate_set_t const& unsafe_preds, bool some_pred_has_traveled_across_a_cycle_without_getting_cancelled) : m_unsafe_preds(unsafe_preds), m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled(some_pred_has_traveled_across_a_cycle_without_getting_cancelled), m_is_top(false)
  { }

  static unsafe_vals_union_t top()
  {
    unsafe_vals_union_t ret = unsafe_vals_union_t();
    ret.m_is_top = true;
    return ret;
  }

  predicate_set_t const& get_preds() const { return m_unsafe_preds; }
  bool some_pred_has_traveled_across_a_cycle_without_getting_cancelled() const { return m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled; }
  void set_some_pred_has_traveled_across_a_cycle_without_getting_cancelled() { m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled = true; }

  bool is_empty() const { return !m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled && m_unsafe_preds.empty(); }
  bool meet(unsafe_vals_union_t const& other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    if (m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled) {
      return false; //already bottom
    }
    if (other.m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled) {
      m_some_pred_has_traveled_across_a_cycle_without_getting_cancelled = true; //set to bottom
      return true; //and return true
    }
    predicate_set_t old_unsafe_preds = m_unsafe_preds;
    unordered_set_union(m_unsafe_preds, other.m_unsafe_preds);
    return old_unsafe_preds != m_unsafe_preds;
  }
};

/*static bool
pred_is_definitely_false(predicate const& p, corr_graph const& cg, pcpair const& pp)
{
  context *ctx = cg.get_context();
  bool guess_made_weaker;
  predicate pred_neg(precond_t(), expr_eq(p.get_lhs_expr(), p.get_rhs_expr()), expr_false(ctx), "pred_is_definitely_false", predicate::provable);
  return cg.graph_with_proofs<pcpair, corr_graph_node, corr_graph_edge, predicate>::decide_hoare_triple({}, pp, mk_epsilon_ec<pcpair, corr_graph_node, corr_graph_edge>(), pred_neg, guess_made_weaker) == proof_status_proven;
}*/


static predicate_set_t
get_potentially_zero_divisors(shared_ptr<tfg_edge_composition_t> const& tfg_ec, tfg const& t, corr_graph const& cg, pcpair const& pp)
{
  std::function<predicate_set_t (tfg_edge_ref const&)> fgen =
    [&t](tfg_edge_ref const& te)
  {
    return te->tfg_edge_get_pathcond_pred_ls_for_div_by_zero(t);
  };
  return get_unsafe_expressions_using_generator_func(tfg_ec, t, cg, pp, fgen);
}

static predicate_set_t
get_potentially_unsafe_memory_accesses(shared_ptr<tfg_edge_composition_t> const& tfg_ec, tfg const& t, corr_graph const& cg, pcpair const& pp)
{
  std::function<predicate_set_t (tfg_edge_ref const&)> fgen =
    [&t](tfg_edge_ref const& te)
  {
    predicate_set_t ret = te->tfg_edge_get_pathcond_pred_ls_for_ismemlabel_heap(t);
    return ret;
  };
  return get_unsafe_expressions_using_generator_func(tfg_ec, t, cg, pp, fgen);
}

class src_must_be_unsafe_dfa_t : public data_flow_analysis<pcpair, corr_graph_node, corr_graph_edge, unsafe_vals_intersect_t, dfa_dir_t::backward>
{
private:
  corr_graph const& m_cg;
  unsafe_condition_t const& m_unsafe_cond;
public:
  src_must_be_unsafe_dfa_t(corr_graph const& cg, map<pcpair, unsafe_vals_intersect_t> &unsafe_vals_map, unsafe_condition_t const& unsafe_cond)
    : data_flow_analysis<pcpair, corr_graph_node, corr_graph_edge, unsafe_vals_intersect_t, dfa_dir_t::backward>(&cg, unsafe_vals_map),
      m_cg(cg), m_unsafe_cond(unsafe_cond)
  { }
  virtual bool xfer_and_meet(shared_ptr<corr_graph_edge const> const& e, unsafe_vals_intersect_t const& in, unsafe_vals_intersect_t& out) override
  {
    //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << endl;
    predicate_set_t ret = m_unsafe_cond.xfer_src(in.get_preds(), e, m_cg);
    return out.meet(unsafe_vals_intersect_t(ret));
  }
};

class dst_may_be_unsafe_dfa_t : public data_flow_analysis<pcpair, corr_graph_node, corr_graph_edge, unsafe_vals_union_t, dfa_dir_t::backward>
{
private:
  corr_graph const& m_cg;
  map<pcpair, unsafe_vals_intersect_t> const& m_src_must_be_unsafe_vals_map;
  unsafe_condition_t const& m_unsafe_cond;
public:
  dst_may_be_unsafe_dfa_t(corr_graph const& cg, map<pcpair, unsafe_vals_union_t> &unsafe_vals_map, map<pcpair, unsafe_vals_intersect_t> const& src_must_be_unsafe_vals_map, unsafe_condition_t const& unsafe_cond)
    : data_flow_analysis<pcpair, corr_graph_node, corr_graph_edge, unsafe_vals_union_t, dfa_dir_t::backward>(&cg, unsafe_vals_map),
      m_cg(cg), m_src_must_be_unsafe_vals_map(src_must_be_unsafe_vals_map), m_unsafe_cond(unsafe_cond)
  { }
  virtual bool xfer_and_meet(shared_ptr<corr_graph_edge const> const& e, unsafe_vals_union_t const& in, unsafe_vals_union_t& out) override
  {
    DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": dst_may_be_unsafe_dfa: e = " << e->to_string() << endl);
    if (out.some_pred_has_traveled_across_a_cycle_without_getting_cancelled()) {
      return false;
    }
    if (in.some_pred_has_traveled_across_a_cycle_without_getting_cancelled()) {
      out.set_some_pred_has_traveled_across_a_cycle_without_getting_cancelled();
      return true;
    }
    bool some_pred_has_traveled_across_a_cycle_without_getting_cancelled = false;
    predicate_set_t ret = m_unsafe_cond.xfer_dst(in.get_preds(), e, m_cg, m_src_must_be_unsafe_vals_map.at(e->get_to_pc()).get_preds(), some_pred_has_traveled_across_a_cycle_without_getting_cancelled);
    DYN_DEBUG(safety, if (some_pred_has_traveled_across_a_cycle_without_getting_cancelled) cout << __func__ << " " << __LINE__ << ": some_pred_has_traveled_across_a_cycle_without_getting_cancelled = true for e = " << e->to_string() << endl);
    return out.meet(unsafe_vals_union_t(ret, some_pred_has_traveled_across_a_cycle_without_getting_cancelled));
  }
};

bool
cg_with_safety::check_safety_for(unsafe_condition_t const& unsafe_cond) const
{
  map<pcpair, unsafe_vals_intersect_t> src_unsafe_vals;
  src_must_be_unsafe_dfa_t src_must_be_unsafe_dfa(*this, src_unsafe_vals, unsafe_cond);
  src_must_be_unsafe_dfa.solve();

  map<pcpair, unsafe_vals_union_t> dst_unsafe_vals;
  dst_may_be_unsafe_dfa_t dst_may_be_unsafe_dfa(*this, dst_unsafe_vals, src_unsafe_vals, unsafe_cond);
  dst_may_be_unsafe_dfa.solve();

  DYN_DEBUG2(safety,
    auto const& unsafe_val_at_entry = dst_unsafe_vals.at(pcpair::start());
    if (!unsafe_val_at_entry.is_empty()) {
      if (unsafe_val_at_entry.some_pred_has_traveled_across_a_cycle_without_getting_cancelled()) {
        cout << _FNLN_ << ": At entry, some_pred_has_traveled_across_a_cycle_without_getting_cancelled = true\n";
      }
      cout << _FNLN_ << ": dst_unsafe_vals.at(pcpair::start()).get_preds():\n";
      for (auto const& p : dst_unsafe_vals.at(pcpair::start()).get_preds()) {
        cout << p->to_string(true) << endl;
      }
    }
  );
  if (!dst_unsafe_vals.count(pcpair::start())) {
    CPP_DBG_EXEC(EQCHECK, cout << __func__ << " " << __LINE__ << ": dst_unsafe_vals.count(pcpair::start()) == 0. Indicates that the exit nodes are not present in the CG which indicates that no legal input can reach exit in the SRC TFG. Returning true for this vacuous case.\n");
    return true;
  }
  return dst_unsafe_vals.at(pcpair::start()).is_empty();
}


bool
cg_with_safety::check_safety() const
{
  autostop_timer ft(__func__);

  if (!this->check_safety_for(unsafe_condition_t(get_potentially_zero_divisors))) {
    DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": safety check failed for zero divisors\n");
    return false;
  }
  if (!this->check_safety_for(unsafe_condition_t(get_potentially_unsafe_memory_accesses))) {
    DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": safety check failed for unsafe memory accesses\n");
    return false;
  }
  return true;
}

}
