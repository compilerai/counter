#pragma once

#include "support/types.h"

//#include "gsupport/memlabel_assertions.h"

#include "graph/point_set.h"
#include "graph/smallest_point_cover.h"
#include "graph/smallest_point_cover_bv.h"
#include "graph/smallest_point_cover_arr.h"
#include "graph/smallest_point_cover_ineq.h"
#include "graph/smallest_point_cover_ineq_loose.h"
#include "graph/smallest_point_cover_ineq_consts.h"
#include "graph/smallest_point_cover_houdini.h"
#include "graph/smallest_point_cover_houdini_axiom_based.h"
#include "graph/smallest_point_cover_houdini_expects_stability.h"
#include "graph/eqclasses.h"

namespace eqspace {

using namespace std;
class tfg_llvm_t;
class tfg_asm_t;
template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_guessing;
template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_correctness_covers;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_t
{
private:
  map<eqclasses_t::expr_group_id_t, dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>>> m_eqclasses;
  //graph_with_ce<T_PC, T_N, T_E, T_PRED> const *m_graph;
  bool m_is_top;
  bool m_is_stable; // not stable => bottom
  failcond_t m_failcond; //non-null if not stable

  //for debugging purposes only
  T_PC m_pc;  //this can be empty (is not initialized properly in all constructors)

  //for optimization (to avoid recomputing)
  //eqclasses_ref m_expr_eqclasses;
public:
  invariant_state_t() : m_is_top(true), //default constructor creates TOP
                        m_is_stable(true), m_failcond(failcond_t::failcond_null())//,
                        //m_expr_eqclasses(mk_eqclasses({}))
  {
    stats_num_invariant_state_constructions++;
  }
  invariant_state_t(invariant_state_t const& other, point_set_t const &point_set_map, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : m_is_top(other.m_is_top), m_is_stable(other.m_is_stable), m_failcond(other.m_failcond), m_pc(other.m_pc)//, m_expr_eqclasses(other.m_expr_eqclasses)
  {
    stats_num_invariant_state_constructions++;
    for (auto const& [egid,eqclass] : other.m_eqclasses) {
//      ASSERT(point_set_map.count(egid));
      bool inserted = m_eqclasses.insert(make_pair(egid, eqclass->clone(point_set_map, out_of_bound_exprs))).second;
      ASSERT(inserted);
    }
  }

  invariant_state_t(invariant_state_t const& other) = default; //unfortunately, we need to support a public copy-constructor, e.g., for pair construction, etc., but ideally we would like to privatize/delete this.  Now that this is public, this opens the door for the common mistake where the graph-with-guessing's copy-constructor simply uses this default copy constructor causing a shallow copy (when a deep copy was required), in turn, causing segfaults.  Thus, need to be careful about this in the graph-copy code.

  ~invariant_state_t()
  {
    stats_num_invariant_state_destructions++;
    //for (auto eqclass : m_eqclasses) {
    //  delete eqclass.second;
    //}
  }

  static invariant_state_t
  pc_invariant_state(T_PC const &p, point_set_t const &point_set_map, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, map<eqclasses_t::expr_group_id_t, expr_group_ref> const& global_eg_at_p, set<expr_ref> const& graph_bvconsts/*, map<graph_loc_id_t, graph_cp_location> const& locs*/, context *ctx)
  {
    return invariant_state_t(p, point_set_map, out_of_bound_exprs, global_eg_at_p/*, locs*/, graph_bvconsts, ctx);
  }

  static invariant_state_t
  empty_invariant_state(context *ctx)
  {
    invariant_state_t empty_invariant_state(ctx);
    ASSERT(!empty_invariant_state.m_is_top);
    //empty m_eqclasses
    //indicate TRUE
    return empty_invariant_state;
  }

  static invariant_state_t
  top()
  {
    invariant_state_t ret;
    ASSERT(ret.m_is_top);
    return ret;
  }

  bool is_top() const
  {
    return m_is_top;
  }

  size_t get_num_bv_exprs_correlated() const;

  //void operator=(invariant_state_t const &other, map<eqclasses_t::expr_group_id_t, point_set_t> const &point_set_map)
  //{
  //  ASSERT(!other.m_is_top);
  //  m_eqclasses.clear();
  //  for (auto const& [egid,eqclass] : other.m_eqclasses) {
  //    ASSERT(point_set_map.count(egid));
  //    bool inserted = m_eqclasses.insert(make_pair(egid, eqclass->clone(point_set_map.at(egid)))).second;
  //    ASSERT(inserted);
  //  }
  //  m_is_top = other.m_is_top;
  //  m_is_stable = other.m_is_stable;
  //  m_failcond = other.m_failcond;
  //  m_pc = other.m_pc;
  //  //m_expr_eqclasses = other.m_expr_eqclasses;
  //}

  bool
  correctness_cover_xfer_on_outgoing_edge_composition(graph_edge_composition_ref<T_PC,T_E> const &ec, invariant_state_t const &to_pc_invariants, graph_with_correctness_covers<T_PC, T_N, T_E, T_PRED> const &g);

  bool
  inductive_invariant_xfer_on_incoming_edge_composition(graph_edge_composition_ref<T_PC, T_E> const &ec, invariant_state_t const &from_pc_invariants, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g, string const& graph_debug_name);

  //smallest_point_cover_t<T_PC, T_N, T_E, T_PRED> const&
  //get_eqclass_for_expr_group(expr_group_ref const &expr_group) const
  //{
  //  for (auto const& eqclass : m_eqclasses) {
  //    if (eqclass->get_exprs_to_be_correlated() == expr_group) {
  //      return *eqclass;
  //    }
  //  }
  //  NOT_REACHED();
  //}

  map<eqclasses_t::expr_group_id_t, dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED> const>>
  get_eqclasses() const
  {
    map<eqclasses_t::expr_group_id_t, dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED> const>> ret;
    for (auto const& [egid, eqclass] : m_eqclasses) {
      ret.insert(make_pair(egid, eqclass));
    }
    return ret;
  }

  unordered_set<shared_ptr<T_PRED const>>
  get_all_preds() const
  {
    ASSERT(!this->m_is_top);
    //if (!this->m_is_stable) {
    //  return unordered_set<shared_ptr<T_PRED const>>();
    //}
    unordered_set<shared_ptr<T_PRED const>> ret;
    for (auto const& [egid, eqclass] : m_eqclasses) {
      unordered_set_union(ret, eqclass->smallest_point_cover_get_preds());
    }
    return ret;
  }

  void invariant_state_sync_preds_for_all_eqclasses() const;

  bool invariant_state_contains_false_pred() const;

  bool operator!=(invariant_state_t<T_PC, T_N, T_E, T_PRED> const &other) const
  {
    ASSERT(!this->m_is_top);
    ASSERT(!other.m_is_top);
    ASSERT(m_eqclasses.size() == other.m_eqclasses.size());
    typename map<eqclasses_t::expr_group_id_t, dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>>>::const_iterator iter1, iter2;
    for (iter1 = m_eqclasses.cbegin(), iter2 = other.m_eqclasses.cbegin();
         iter1 != m_eqclasses.cend() && iter2 != other.m_eqclasses.cend();
         iter1++, iter2++) {
      if (*iter1->second != *iter2->second) {
        return true;
      }
    }
    return this->m_is_stable != other.m_is_stable;
  }

  bool operator==(invariant_state_t<T_PC, T_N, T_E, T_PRED> const &other) const
  {
    return !(*this != other);
  }

  string invariant_state_to_string(/*T_PC const &p, */string prefix, bool full = false) const;
  void invariant_state_xml_print(ostream& os, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, map<expr_id_t, pair<expr_ref, expr_ref>> const& memlabel_assertion_submap, std::function<void (expr_ref&)> src_rename_expr_fn, std::function<void (expr_ref&)> dst_rename_expr_fn, context::xml_output_format_t xml_output_format, string const& prefix, bool negate) const;
  void invariant_state_get_asm_annotation(ostream& os) const;

  //returns if the invariant state was changed due to the CE
  set<eqclasses_t::expr_group_id_t> invariant_state_add_point_using_CE(nodece_ref<T_PC, T_N, T_E> const &nce, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g);

  //bool invariant_state_is_well_formed() const
  //{
  //  size_t eqclass_id = 0;
  //  for (auto const& eqclass : m_eqclasses) {
  //    if (!eqclass->smallest_point_cover_is_well_formed()) {
  //      cout << __func__ << " " << __LINE__ << ": returning false for eqclass id " << eqclass_id << endl;
  //      return false;
  //    }
  //    eqclass_id++;
  //  }
  //  return true;
  //}

  //eqclasses_ref const& get_expr_eqclasses() const;

  bool is_stable() const { return m_is_stable; }
  failcond_t const& invstate_get_failcond() const { return m_failcond; }

  void invariant_state_from_stream(istream& is, string const& inprefix, graph_with_points<T_PC,T_N,T_E,T_PRED> const& g, T_PC const& p/*, map<graph_loc_id_t, graph_cp_location> const& locs*/);

  void invariant_state_to_stream(ostream& os, string const& inprefix) const;

  //void invariant_state_clear_points_and_preds();

//  void invariant_state_remove_bv_exprs(set<expr_group_t::expr_idx_t> const& bv_exprs_to_remove);

private:

  eqclasses_ref compute_expr_eqclasses() const;

  invariant_state_t(T_PC const &p,/* eqclasses_ref const &eqclasses*/ point_set_t const& point_set_map, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, map<eqclasses_t::expr_group_id_t, expr_group_ref> const& global_eg_at_p, set<expr_ref> const& graph_bvconsts/*, map<graph_loc_id_t, graph_cp_location> const& locs*/, context *ctx);

  invariant_state_t(context *ctx) : m_is_top(false), m_is_stable(true), m_failcond(failcond_t::failcond_null())//, m_expr_eqclasses(mk_eqclasses({}))
  {
    stats_num_invariant_state_constructions++;
    //empty m_eqclasses
    //indicate TRUE
  }

  //bool check_stability() const
  //{
  //  for (auto const* eqclass : this->m_eqclasses) {
  //    failcond_t failcond = eqclass->eqclass_check_stability();
  //    if(!failcond.failcond_is_null()) {
  //      return false;
  //    }
  //  }
  //  return true;
  //}

};
}
