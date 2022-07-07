#pragma once

#include "support/types.h"

#include "gsupport/failcond.h"

#include "graph/point_set.h"
#include "graph/expr_group.h"
#include "graph/proof_timeout_info.h"

namespace eqspace {

using namespace std;

class tfg_llvm_t;
class tfg_asm_t;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_correctness_covers;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_arr_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_bv_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_ineq_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_ineq_loose_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_ineq_const_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_t
{
protected:
  point_set_t const& m_point_set;
  set<expr_group_t::expr_idx_t> const& m_out_of_bound_exprs;
  expr_group_ref m_global_eg_at_p;
  proof_timeout_info_t m_timeout_info;
  context *m_ctx;
  mutable set<string_ref> m_visited_ces;

private:
  mutable unordered_set<shared_ptr<T_PRED const>> m_preds; // IMPORTANT: no eqclass is allowed to directly modify m_preds
  mutable bool m_preds_out_of_sync;
  mutable bool m_sync_preds_retval;

public:
  smallest_point_cover_t(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, expr_group_ref const &global_eg_at_p, context *ctx)
  : m_point_set(point_set/*exprs_to_be_correlated, ctx*/), m_out_of_bound_exprs(out_of_bound_exprs), m_global_eg_at_p(global_eg_at_p), m_timeout_info(ctx), m_ctx(ctx),
    m_preds_out_of_sync(false), m_sync_preds_retval(false)
  {
    //ASSERT(exprs_to_be_correlated->size());
    stats_num_smallest_point_cover_constructions++;
    m_preds.insert(T_PRED::false_predicate(m_ctx));
  }

  virtual ~smallest_point_cover_t();

  smallest_point_cover_t(smallest_point_cover_t const &other, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs)
                                                                    : m_point_set(point_set),
                                                                      m_out_of_bound_exprs(out_of_bound_exprs),
                                                                      m_global_eg_at_p(other.m_global_eg_at_p),
                                                                      m_timeout_info(other.m_timeout_info),
                                                                      m_ctx(other.m_ctx),
                                                                      m_visited_ces(other.m_visited_ces),
                                                                      m_preds(other.m_preds),
                                                                      m_preds_out_of_sync(other.m_preds_out_of_sync),
                                                                      m_sync_preds_retval(other.m_sync_preds_retval)
  {
    stats_num_smallest_point_cover_constructions++;
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    ASSERT(m_point_set.size() == other.m_point_set.size());
  }

  smallest_point_cover_t(smallest_point_cover_t const &other) = delete;

  smallest_point_cover_t(istream& is, string const& prefix, context* ctx, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : m_point_set(point_set), m_out_of_bound_exprs(out_of_bound_exprs), m_global_eg_at_p(mk_expr_group(is, ctx, prefix)), m_timeout_info(ctx), m_ctx(ctx), m_preds(T_PRED::predicate_set_from_stream(is/*, start_state*/, ctx, prefix)), m_preds_out_of_sync(false), m_sync_preds_retval(false)
  {
    string ce_prefix = "=" + prefix + "visited ce ";
    string line;
    bool end;
    end = !getline(is, line);
    ASSERT(!end);
    while (string_has_prefix(line, ce_prefix)) {
      size_t idx = line.find(": "); 
      ASSERT(idx != string::npos);
      string visited_ce = line.substr(idx + 2);
      m_visited_ces.insert(mk_string_ref(visited_ce));
      end = !getline(is, line);
      ASSERT(!end);
    }
    //cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
    if (line != "=" + prefix + "visited ces set done") {
      cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
      cout << __func__ << " " << __LINE__ << ": prefix = '" << ce_prefix << "'\n";
    }
    ASSERT(line == "=" + prefix + "visited ces set done");

    stats_num_smallest_point_cover_constructions++;
  }

  virtual dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>> clone(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) const = 0;

  virtual bool is_eqclass_used_for_correl_ranking() const {return false; }

  void operator=(smallest_point_cover_t const &other) = delete;

  //void operator=(smallest_point_cover_t const &other)
  //{
  //  //ASSERT(&m_graph == &other.m_graph);
  //  ASSERT(&m_point_set == &other.m_point_set);
  //  ASSERT(m_point_set.size() == other.m_point_set.size());
  //  m_ctx = other.m_ctx;
  //  m_preds = other.m_preds;
  //  m_preds_out_of_sync = other.m_preds_out_of_sync;
  //  m_sync_preds_retval = other.m_sync_preds_retval;
  //}

  expr_group_ref const&
  get_exprs_to_be_correlated() const
  {
    return m_global_eg_at_p; //m_point_set.get_exprs();
  }

  set<string_ref>&
  get_visited_ces_ref() const
  {
    return m_visited_ces;
  }

  set<string_ref> const&
  get_visited_ces() const
  {
    return m_visited_ces;
  }

  //void add_visted_ce(string_ref const& ce_name) const {
  //  m_visited_ces.insert(ce_name);
  //}

  void pop_back_point(string_ref const& ce_name) const {
    m_visited_ces.erase(ce_name);
  }

  bool point_has_mapping_for_all_exprs(point_ref const& pt) const
  {
    set<expr_group_t::expr_idx_t> eg_expr_ids = map_get_keys(this->get_exprs_to_be_correlated()->get_expr_vec()); 
    //set<expr_group_t::expr_idx_t> const& out_of_bound_expr_ids = m_point_set.get_out_of_bound_exprs();
    set_difference(eg_expr_ids, m_out_of_bound_exprs);
    set<expr_group_t::expr_idx_t> point_expr_ids = map_get_keys(pt->get_vals());
    set<expr_group_t::expr_idx_t> expr_ids_in_eg_not_in_point;
    set_difference(eg_expr_ids, point_expr_ids, expr_ids_in_eg_not_in_point);
    if (expr_ids_in_eg_not_in_point.size()) {
      cout << _FNLN_ << ": Could not find mapping in point '" << pt->get_point_name()->get_str() << "' for the following exprs:\n";
      for (auto const& egid : expr_ids_in_eg_not_in_point) {
        cout << "  " << egid << " -> " << this->get_exprs_to_be_correlated()->get_expr_vec().at(egid).point_exprs_to_string() << endl;
      }
      return false;
    } else {
      return true;
    }
  }

  //set<expr_ref> const&
  //get_unreachable_correlation_exprs() const
  //{
  //  return m_point_set.get_unreachable_exprs();
  //}

  unordered_set<shared_ptr<T_PRED const>> const&
  smallest_point_cover_get_preds() const
  {
    this->sync_preds();
    ASSERT(!m_preds_out_of_sync);
    return m_preds;
  }

  //void
  //clear_preds()
  //{
  //  m_preds.clear();
  //}

  string smallest_point_cover_get_name() const
  {
    return this->get_exprs_to_be_correlated()->get_name();
  }

  string smallest_point_cover_type_to_string() const
  {
    return expr_group_t::expr_group_type_to_string(this->get_exprs_to_be_correlated()->get_type());
  }

  string eqclass_exprs_to_string() const
  {
    stringstream ss;
    expr_group_ref const &exprs_to_be_correlated = this->get_exprs_to_be_correlated();
    ss << "smallest point cover name " << exprs_to_be_correlated->get_name()
       << ", type " << smallest_point_cover_type_to_string()
       << ", exprs [" << exprs_to_be_correlated->expr_group_get_num_exprs() << "]: ";
    for (auto const& [i, pe] : exprs_to_be_correlated->get_expr_vec()) {
      //expr_ref const& e = exprs_to_be_correlated->at(i);
      ss << i << "-->" << expr_string(pe.get_expr());
      ss << " ; ";
    }
    return ss.str();
  }

  bool
  smallest_point_cover_inductive_invariant_xfer_on_incoming_edge_composition(graph_edge_composition_ref<T_PC,T_E> const &ec, invariant_state_t<T_PC, T_N, T_E, T_PRED> const &from_pc_state, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g);

  bool smallest_point_cover_correctness_cover_xfer_on_outgoing_edge_composition(graph_edge_composition_ref<T_PC,T_E> const &ec, invariant_state_t<T_PC, T_N, T_E, T_PRED> const &to_pc_state, graph_with_correctness_covers<T_PC, T_N, T_E, T_PRED> const &g);

  bool smallest_point_cover_add_point_using_CE(nodece_ref<T_PC, T_N, T_E> const &nce, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g)
  {
    autostop_timer func_timer(string(__FILE__) + "." + __func__);
    DYN_DEBUG3(add_point_using_ce, cout << __func__ << " " << __LINE__ << ": adding point using CE\n" << this->eqclass_exprs_to_string() << endl);
//    DYN_DEBUG3(add_point_using_ce, cout << __func__ << " " << __LINE__ << ": old point set =\n" << m_point_set.point_set_to_string("  ") << endl);

    //bool point_added = m_point_set.template point_set_add_point_using_CE<T_PC, T_N, T_E, T_PRED>(nce, g);
    //if (!point_added) {
    //  DYN_DEBUG(add_point_using_ce,
    //    cout << __func__ << " " << __LINE__ << ": " << this->smallest_point_cover_type_to_string() << ": add_point_using_CE failed; point discarded\n"
    //  );
    //  return false;
    //}
//    cout << __func__ << " " << __LINE__ << ": " << this->smallest_point_cover_type_to_string() << ": Nodece= " << nce->nodece_get_counter_example().get_ce_name()->get_str() << endl;
//    cout << __func__ << " " << __LINE__ << ": " << ": Visited CEs: "  << endl;
//    for (auto const& s : m_visited_ces) {
//      cout << s->get_str() << ", ";
//    }
//    cout << endl;
    string_ref const& visited_ce_name = nce->nodece_get_counter_example().get_ce_name();
    if (m_visited_ces.count(visited_ce_name)) {
      cout << _FNLN_ << ": m_visited_ces =\n";
      for (auto const& ce : m_visited_ces) {
        cout << "  " << ce->get_str() << endl;
      }
      cout << "visited_ce_name = " << visited_ce_name->get_str() << endl;
    }
    ASSERT(!m_visited_ces.count(visited_ce_name));
    //cout << _FNLN_ << ": inserting " << visited_ce_name->get_str() << " to m_visited_ces\n";
    bool inserted = m_visited_ces.insert(visited_ce_name).second;
    ASSERT(inserted);

    DYN_DEBUG(print_progress_debug,
      cout << _FNLN_ << ": " << timestamp() << ": " << this->smallest_point_cover_get_name() << " type " << this->smallest_point_cover_type_to_string() << ": calling recomputed_preds_would_be_different_from_current_preds after adding CE " << visited_ce_name->get_str() << "\n"
    );

    bool preds_modified = this->recomputed_preds_would_be_different_from_current_preds();

    DYN_DEBUG(print_progress_debug,
      cout << _FNLN_ << ": " << timestamp() << ": " << this->smallest_point_cover_get_name() << " type " << this->smallest_point_cover_type_to_string() << ": calling recomputed_preds_would_be_different_from_current_preds\n"
    );


    if (preds_modified) {
      DYN_DEBUG(add_point_using_ce,
        cout << __func__ << " " << __LINE__ << ": " << this->smallest_point_cover_type_to_string() << ": retained point " << nce->nodece_get_counter_example().get_ce_name()->get_str() << "\n"
      );
//      DYN_DEBUG2(add_point_using_ce, cout << __func__ << " " << __LINE__ << ": new point set =\n" << m_point_set.point_set_to_string("  ") << endl);
      DYN_DEBUG(add_point_using_ce, cout << __func__ << " " << __LINE__ << ": " << this->eqclass_exprs_to_string() << ": new preds: " << this->smallest_point_cover_preds_to_string("  ", m_preds) << endl);
      return true;
    } else {
      DYN_DEBUG(add_point_using_ce,
        cout << _FNLN_ << ": " << this->smallest_point_cover_type_to_string() << ": new_preds == old_preds; point " << nce->nodece_get_counter_example().get_ce_name()->get_str() << " discarded\n"
      );
      //this->smallest_point_cover_pop_back_last_point();
      return false;
    }
  }

  //void smallest_point_cover_pop_back_last_point()
  //{
  //  m_point_set.pop_back(); //get rid of this point as it adds no value
  //}

  bool operator!=(smallest_point_cover_t<T_PC, T_N, T_E, T_PRED> const &other) const
  {
    //ASSERT(m_point_set.get_exprs() == other.get_exprs_to_be_correlated());
    //Cannot assert this  as the size of expr_group may change when new CE is added
    if (m_preds == other.m_preds && m_preds_out_of_sync != other.m_preds_out_of_sync) {
      return true;
    }
    if (m_preds == other.m_preds && m_sync_preds_retval != other.m_sync_preds_retval) {
      return true;
    }
    return m_preds != other.m_preds;
  }

	string smallest_point_cover_to_string(string const &prefix, bool with_points) const;
  //void smallest_point_cover_xml_print(ostream& os, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, std::function<void (expr_ref&)> src_rename_expr_fn, std::function<void (expr_ref&)> dst_rename_expr_fn, context::xml_output_format_t xml_output_format, string const& prefix, bool negate) const;
  void smallest_point_cover_get_asm_annotation(ostream& os) const;

  virtual void smallest_point_cover_to_stream(ostream& os, string const& inprefix) const = 0;

  static dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>> smallest_point_cover_from_stream(istream& is, string const& prefix/*, state const& start_state*/, context* ctx/*, map<graph_loc_id_t, graph_cp_location> const& locs*/, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs);

  //void smallest_point_cover_clear_points_and_preds();

  //bool smallest_point_cover_is_well_formed() const
  //{
  //  for (auto const& pred : m_preds) {
  //    shared_ptr<T_PRED const> false_pred = T_PRED::false_predicate(m_ctx);
  //    if (pred == false_pred) {
  //      CPP_DBG_EXEC(INVARIANTS_MAP, cout << __func__ << " " << __LINE__ << ": returning false for:\n" << this->smallest_point_cover_to_string("  ", true) << endl);
  //      return false;
  //    }
  //  }
  //  return true;
  //}

  virtual failcond_t eqclass_check_stability() const { return failcond_t::failcond_null(); } // stability must be computed from latest set of points

  // returns TRUE if preds were changed
  bool sync_preds() const
  {
    //if (m_preds_out_of_sync) {
    //  m_preds_out_of_sync = false;
    //  return true;
    //} else {
    //  unordered_set<shared_ptr<T_PRED const>> old_preds = m_preds;
    //  this->recompute_preds_for_points(false, m_preds);
    //  return (old_preds != m_preds);
    //}
    if (m_preds_out_of_sync) {
      ASSERT(m_sync_preds_retval);
      unordered_set<shared_ptr<T_PRED const>> old_preds = m_preds;
      this->recompute_preds_for_points(false, m_preds);
      m_preds_out_of_sync = false;
      m_sync_preds_retval = false;
      return old_preds != m_preds;
    } else {
      bool old_sync_preds_retval = m_sync_preds_retval;
      m_sync_preds_retval = false;
      return old_sync_preds_retval;
    }
  }

  point_set_t const& get_point_set() const { return m_point_set; }
  //point_set_t& get_point_set_ref() { return m_point_set; }

protected:
  void smallest_point_cover_to_stream_helper(ostream& os, string const& prefix) const
  {
    //m_point_set.point_set_to_stream(os, prefix);
    m_global_eg_at_p->expr_group_to_stream(os, prefix);
    T_PRED::predicate_set_to_stream(os, prefix, this->smallest_point_cover_get_preds());
    size_t i = 0;
    for (auto const& ce : m_visited_ces) {
      os << "=" << prefix << "visited ce " << i << ": " << ce->get_str() << endl;
      i++;
    }
    os << "=" << prefix << "visited ces set done\n";
    //os << "=" << prefix << "out-of-bound exprs\n";
    //for (auto const& eid : eg_set) {
    //  os << " " << eid;
    //}
  }
  void smallest_point_cover_mark_preds_out_of_sync()
  {
    m_preds_out_of_sync = true;
    m_sync_preds_retval = true;
  }
private:
  bool smallest_point_cover_xfer_using_fn(graph_edge_composition_ref<T_PC,T_E> const &ec, T_PC const& neighbor_pc, invariant_state_t<T_PC, T_N, T_E, T_PRED> const &neighbor_pc_state, std::function<proof_status_t (T_PC const&, graph_edge_composition_ref<T_PC,T_E>, shared_ptr<T_PRED const>)> fn, std::function<bool (graph_edge_composition_ref<T_PC,T_E> const&)> can_exit_early);

  static string smallest_point_cover_preds_to_string(string const& prefix, unordered_set<shared_ptr<T_PRED const>> const& preds)
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

  // Virtual functions to be overriden in dervied classes
  // NOTE: we allow eqclasses to generate their preds either lazily or eagerly
  // the only requirement is that the following functions specs are respected

  // set preds to latest preds value
  // returns TRUE if we are done when last_proof_success is TRUE
  // NOTE: this function is supposed to be pure in the sense that consecutive calls
  // without updating points should return same value
  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>> &preds) const = 0;

  // return TRUE if preds computed from latest points would be different from
  // preds generated before adding last point
  // This function is ALWAYS called after a point is added and can be used as a proxy for it
  // By default, this function keeps preds in sync.  Derived classes may override to
  // leave preds out of sync and let sync_preds() sync them later.
  virtual bool recomputed_preds_would_be_different_from_current_preds()
  {
    autostop_timer ft(__func__);
    unordered_set<shared_ptr<T_PRED const>> old_preds = m_preds;
    ASSERT(!m_preds_out_of_sync);
    recompute_preds_for_points(false, m_preds); // m_preds must be updated to reflect change in old_preds in next call
    m_sync_preds_retval = (old_preds != m_preds);
    return m_sync_preds_retval;
  }
  //void replay_counter_examples(list<nodece_ref<T_PC, T_N, T_E>> const& counterexamples);
};

}
