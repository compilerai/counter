#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "support/globals_cpp.h"
#include "support/timers.h"
#include "support/distinct_sets.h"
#include "support/dyn_debug.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "expr/local_sprel_expr_guesses.h"
#include "expr/aliasing_constraints.h"
#include "expr/pc.h"

#include "gsupport/sprel_map.h"
#include "gsupport/edge_with_cond.h"
#include "gsupport/edge_with_graph_ec_no_unroll.h"
#include "gsupport/tfg_enum_props.h"
#include "gsupport/graph_full_pathset.h"

#include "graph/graph.h"
#include "graph/graph_with_aliasing.h"
#include "graph/graph_with_path_sensitive_locs.h"
//#include "graph/binary_search_on_preds.h"
#include "graph/lr_map.h"

namespace eqspace {

using namespace std;

//template <typename T_PC, typename T_E>
//class pc_path_t
//{
//private:
//  T_PC m_pc;
//  graph_edge_composition_ref<T_PC,T_E> m_path;
//public:
//  pc_path_t(T_PC const& p, graph_edge_composition_ref<T_PC,T_E> const& pth) : m_pc(p), m_path(pth)
//  { }
//  T_PC const& get_pc() const { return m_pc; }
//  graph_edge_composition_ref<T_PC,T_E> const& get_path() const { return m_path; }
//  bool operator<(pc_path_t const& other) const
//  {
//    return    m_path < other.m_path
//           || (m_path == other.m_path && m_pc < other.m_pc)
//    ;
//  }
//};

//graph_with_proof_obligations; graph_with_proofs for short
template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_paths : public graph_with_path_sensitive_locs<T_PC, T_N, T_E, T_PRED>
{
private:
  //mutable map<pc_path_t<T_PC,T_E>, list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>>> m_path_assumes_cache;
  //mutable map<pc_path_t<T_PC,T_E>, aliasing_constraints_t> m_path_aliasing_constraints_cache;
  using T_PRED_REF = shared_ptr<T_PRED const>;
  using T_PRED_SET = unordered_set<T_PRED_REF>;

public:
  using edge_with_cond_ec_t = serpar_composition_node_t<dshared_ptr<edge_with_cond<T_PC> const>>;
  using edge_with_cond_ec_ref = dshared_ptr<edge_with_cond_ec_t>;

  graph_with_paths(string const &name, string const& fname, context* ctx) : graph_with_path_sensitive_locs<T_PC, T_N, T_E, T_PRED>(name, fname, ctx)
  {
  }

  graph_with_paths(graph_with_paths const &other) : graph_with_path_sensitive_locs<T_PC, T_N, T_E, T_PRED>(other)//,
                                                    //m_path_assumes_cache(other.m_path_assumes_cache) //deliberately not copying the caches, it should not matter
                                                    //m_path_aliasing_constraints_cache(other.m_path_aliasing_constraints_cache) //deliberately not copying the caches, it should not matter
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_paths(istream& is, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx) : graph_with_path_sensitive_locs<T_PC, T_N, T_E, T_PRED>(is, name, read_edge_fn, ctx)
  {
    autostop_timer ft(string("graph_with_paths_constructor.") + name);
  }

  virtual ~graph_with_paths() = default;

  dshared_ptr<graph_with_paths<T_PC, T_N, T_E, T_PRED> const> graph_with_paths_shared_from_this() const
  {
    dshared_ptr<graph_with_paths<T_PC, T_N, T_E, T_PRED> const> ret = dynamic_pointer_cast<graph_with_paths<T_PC, T_N, T_E, T_PRED> const>(this->shared_from_this());
    ASSERT(ret);
    return ret;
  }

  virtual list<pair<graph_edge_composition_ref<pc, tfg_edge>, shared_ptr<T_PRED const>>> collect_assumes_around_path(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const &ec) const
  {
    autostop_timer func_timer(string("graph_with_paths::") + __func__);
//    pc_path_t<T_PC,T_E> pc_path(from_pc, ec);
//    if (!g_disable_all_caches && m_path_assumes_cache.count(pc_path)) {
//      if (!GRAPH_WITH_PATHS_CACHE_ASSERTS_flag) {
//        return m_path_assumes_cache.at(pc_path);
//      }
//    }

    list<pair<graph_edge_composition_ref<pc, tfg_edge>, shared_ptr<T_PRED const>>> ret = this->collect_assumes_around_path_helper(from_pc, ec);
//    if (!g_disable_all_caches) {
//      m_path_assumes_cache.insert(make_pair(pc_path, ret));
//    }
    DYN_DEBUG2(decide_hoare_triple_debug, cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << "\n");
    return ret;
  }


  aliasing_constraints_t
  collect_stack_sanity_assumptions_around_path(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const& pth) const
  {
    return aliasing_constraints_t();
  }

  aliasing_constraints_t
  collect_aliasing_constraints_around_path(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const& pth) const
  {
    autostop_timer func_timer(__func__);
    aliasing_constraints_t ret;
//    pc_path_t<T_PC,T_E> pc_path(from_pc, pth);
//    if (!g_disable_all_caches && m_path_aliasing_constraints_cache.count(pc_path)) {
//      if (!GRAPH_WITH_PATHS_CACHE_ASSERTS_flag) {
//        auto const& ret = m_path_aliasing_constraints_cache.at(pc_path);
//        //cout << __func__ << " " << __LINE__ << ": returning from cache for " << pth->graph_edge_composition_to_string() << ":\n" << ret.to_string_for_eq() << endl;
//        return ret;
//      }
//    }

    std::function<aliasing_constraints_t (dshared_ptr<T_E const> const &)> edge_atom_f = [this](dshared_ptr<T_E const> const& e)
    {
      return this->get_aliasing_constraints_for_edge(e);
    };
    aliasing_constraints_t alias_cons_at_to_pc = this->graph_generate_aliasing_constraints_at_path_to_pc(pth);
    //cout << "graph_with_paths::" << __func__ << " " << __LINE__ << ": from_pc = " << from_pc.to_string() << ", pth = " << pth->graph_edge_composition_to_string() << ":\n" << ret.to_string_for_eq() << endl;
    ret = this->collect_aliasing_constraints_around_path_helper(from_pc, pth, edge_atom_f, alias_cons_at_to_pc);

//    if (GRAPH_WITH_PATHS_CACHE_ASSERTS_flag) {
//      if (m_path_aliasing_constraints_cache.count(pc_path)) {
//        if (!(ret == m_path_aliasing_constraints_cache.at(pc_path))) {
//          cout << "pc = " << from_pc.to_string() << ", path = " << pth->to_string_concise() << endl;
//          cout << "ret = " << ret.to_string_for_eq() << endl;
//          cout << "m_path_aliasing_constraints_cache.at(pc_path) = " << m_path_aliasing_constraints_cache.at(pc_path).to_string_for_eq() << endl;
//          cout.flush();
//        }
//        ASSERT(ret == m_path_aliasing_constraints_cache.at(pc_path));
//      }
//    }
//    //cout << __func__ << " " << __LINE__ << ": inserting into cache for " << pth->graph_edge_composition_to_string() << ":\n" << ret.to_string_for_eq() << endl;
//    if (!g_disable_all_caches) {
//      m_path_aliasing_constraints_cache.insert(make_pair(pc_path, ret));
//    }
    return ret;
  }

  aliasing_constraints_t
  collect_aliasing_constraints_around_path_helper(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const& pth, std::function<aliasing_constraints_t (dshared_ptr<T_E const> const&)> atom_f, aliasing_constraints_t const& post_cons) const
  {
    autostop_timer ft(__func__);

    std::function<aliasing_constraints_t (dshared_ptr<T_E const> const &, aliasing_constraints_t const &postorder_val)> fold_atom_f =
			[this, &atom_f](dshared_ptr<T_E const> const &e, aliasing_constraints_t const &postorder_val)
      {
        if (e->is_empty()) {
          return postorder_val;
        }
        aliasing_constraints_t ret;
        ret = atom_f(e);
        //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << ": ret = " << ret.to_string_for_eq() << endl;
        ret.aliasing_constraints_union(this->graph_apply_trans_funs_on_aliasing_constraints(e, postorder_val));
        return ret;
      };
    std::function<aliasing_constraints_t (aliasing_constraints_t const&, aliasing_constraints_t const&)> f_fold_parallel =
      [](aliasing_constraints_t const& a, aliasing_constraints_t const& b)
      {
        aliasing_constraints_t ret = a;
        ret.aliasing_constraints_union(b);
        return ret;
      };

    if (!pth || pth->is_epsilon()) {
      list<dshared_ptr<T_E const>> outgoing;
      this->get_edges_outgoing(from_pc, outgoing);
      aliasing_constraints_t ret;
      for (auto const& o : outgoing) {
        ret.aliasing_constraints_union(fold_atom_f(o, aliasing_constraints_t()));
      }
      return ret;
    }
    return pth->template graph_ec_visit_nodes_postorder_series<aliasing_constraints_t>(fold_atom_f, f_fold_parallel, post_cons);
  }

  //virtual list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<T_PRED const>>> graph_apply_trans_funs(graph_edge_composition_ref<T_PC,T_E> const &pth, shared_ptr<T_PRED const> const &pred, bool simplified = false) const = 0;

  virtual list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> get_simplified_non_mem_assumes(dshared_ptr<T_E const> const& e) const = 0;

  virtual list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> pth_collect_simplified_preds_using_atom_func(shared_ptr<graph_edge_composition_t<T_PC, T_E> const> const& ec, std::function<list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> (dshared_ptr<T_E const> const&)> f_atom, list<pair<graph_edge_composition_ref<pc, tfg_edge>, shared_ptr<T_PRED const>>> const& from_pc_preds, context* ctx) const = 0;
  aliasing_constraints_t graph_generate_aliasing_constraints_for_locs(T_PC const& p) const;

  map<pair<T_PC,int>, shared_ptr<graph_full_pathset_t<T_PC,T_E> const>> get_unrolled_paths_from(tfg_enum_props_t::type const& tfg_enum_props, T_PC const &pc_from, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, int max_occurrences_of_a_node) const;

  dshared_ptr<graph_with_regions_t<T_PC, T_N, edge_with_graph_ec_no_unroll_t<T_PC, T_E>>> graph_get_reduced_graph_with_anchor_nodes_only() const;

  void get_path_enumeration_stop_pcs(tfg_enum_props_t::type const& tfg_enum_props, set<T_PC> &stop_pcs) const;
  map<pair<T_PC, int>, shared_ptr<graph_full_pathset_t<T_PC,T_E> const>> get_composite_path_between_pcs_till_unroll_factor_helper(T_PC const& from_pc, set<T_PC> const& to_pcs/*, map<pc,int> const& visited_count, int pth_length*/, int unroll_factor, set<T_PC> const& stop_pcs) const;

//  graph_edge_composition_ref<T_PC,T_E> graph_identify_corresponding_full_pathset_with_cloned_pcs(graph_edge_composition_ref<T_PC,T_E> const& in_ec/*, T_PC const& orig_to_pc*/) const;


protected:

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override
  {
    this->graph_with_path_sensitive_locs<T_PC, T_N, T_E, T_PRED>::graph_to_stream(os, prefix);
  }


  virtual list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> collect_inductive_preds_around_path(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const &ec) const
  {
    return  list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>>(); //return empty set for now; will return non-empty if inductive preds are computed (e.g., for TFG, CG)
  }

  //T_PRED_SET
  list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>>
  collect_non_mem_assumes_from_edge_list(list<dshared_ptr<T_E const>> const& elist) const
  {
    list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> ret;
    for (auto const& e : elist) {
      T_PRED::guarded_predicate_set_union(ret, this->get_simplified_non_mem_assumes(e));
    }
    return ret;
  }

  aliasing_constraints_t graph_generate_aliasing_constraints_for_edgecond_and_to_state(expr_ref const& econd, state const& to_state) const;

  virtual aliasing_constraints_t graph_generate_aliasing_constraints_at_path_to_pc(graph_edge_composition_ref<T_PC,T_E> const& pth) const = 0;

  map<pair<T_PC, int>, shared_ptr<graph_full_pathset_t<T_PC,T_E> const>> get_composite_path_between_pcs_till_unroll_factor(tfg_enum_props_t::type const& tfg_enum_props, T_PC const& from_pc, set<T_PC> const& to_pcs/*, map<pc,int> const& visited_count*/, int unroll_factor, set<T_PC> const& stop_pcs) const;
  void find_loop_anchors_collapsing_into_unconditional_fcalls(set<T_PC> &loop_heads) const;

private:

  virtual void get_anchor_pcs(set<T_PC>& anchor_pcs) const = 0;

  graph_edge_composition_ref<T_PC,T_E> get_path_starting_with_edge_and_ending_at_non_phinum_node_or_to_pc(dshared_ptr<T_E const> const& e, T_PC const& to_pc) const;

  map<T_PC, shared_ptr<graph_full_pathset_t<T_PC,T_E> const>> get_paths_from(T_PC const &pc_from, set<T_PC> const& to_pcs) const;

  void get_path_enumeration_reachable_pcs(tfg_enum_props_t::type const& tfg_enum_props, T_PC const &from_pc, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, set<T_PC> &reachable_pcs) const;

  bool find_fcall_pc_along_maximal_linear_path(T_PC const& p) const;

  bool find_fcall_pc_along_maximal_linear_path_from_loop_head(T_PC const& loop_head, set<T_PC>& fcall_heads) const;
  bool find_fcall_pc_along_maximal_linear_path_to_loop_tail(T_PC const& loop_tail, set<T_PC>& fcall_heads) const;

  void get_bbl_start_stop_pcs_stopping_at_fcalls(T_PC const &p, set<T_PC> &reachable_pcs) const;
  void get_reachable_loop_heads_fcalls_exit_pcs_stopping_at_fcalls(T_PC const &p, set<T_PC> &reachable_pcs) const;
  bool pc_is_bbl_start(T_PC const &p) const;
  bool pc_is_bbl_stop(T_PC const &p) const;
  bool pc_is_bbl_start_stop(T_PC const &p) const;

  bool is_pc_pair_compatible(T_PC const &p1, int p2_subsubindex, expr_ref const& p2_incident_fcall_nextpc) const;
  static bool incident_fcall_nextpcs_are_compatible(expr_ref nextpc1, expr_ref nextpc2);
  static bool is_fcall_pc(T_PC const &p);


  void dfs_find_loop_heads_and_tails(T_PC const &p, map<T_PC, dfs_color> &visited, set<T_PC> &loop_heads, set<T_PC>& loop_tails) const;
  void find_loop_heads(set<T_PC> &loop_heads) const;
  void find_loop_tails(set<T_PC>& loop_tails) const;

  virtual expr_ref get_incident_fcall_nextpc(T_PC const &p) const = 0;
	virtual aliasing_constraints_t get_aliasing_constraints_for_edge(dshared_ptr<T_E const>const& e) const = 0;
  virtual aliasing_constraints_t graph_apply_trans_funs_on_aliasing_constraints(dshared_ptr<T_E const> const& e, aliasing_constraints_t const& als) const = 0;

  //virtual edge_guard_t graph_get_edge_guard_from_edge_composition(graph_edge_composition_ref<T_PC,T_E> const &ec) const = 0;

  list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> collect_assumes_around_path_helper(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const &ec) const;

  //static typename edge_with_cond_ec_t::type convert_graph_ec_type_to_edge_with_cond_ec_type(typename graph_edge_composition_t<T_PC,T_E>::type t)
  //{
  //  switch (t) {
  //    case graph_edge_composition_t<T_PC,T_E>::atom:
  //      return edge_with_cond_ec_t::atom;
  //    case graph_edge_composition_t<T_PC,T_E>::series:
  //      return edge_with_cond_ec_t::series;
  //    case graph_edge_composition_t<T_PC,T_E>::parallel:
  //      return edge_with_cond_ec_t::parallel;
  //  }
  //  NOT_REACHED();
  //}

  //static typename graph_edge_composition_t<T_PC,T_E>::type convert_graph_ec_type_to_edge_with_cond_ec_type(typename edge_with_cond_ec_t::type t)
  //{
  //  switch (t) {
  //    case edge_with_cond_ec_t::atom:
  //      return graph_edge_composition_t<T_PC,T_E>::atom;
  //    case edge_with_cond_ec_t::series:
  //      return graph_edge_composition_t<T_PC,T_E>::series;
  //    case edge_with_cond_ec_t::parallel:
  //      return graph_edge_composition_t<T_PC,T_E>::parallel;
  //  }
  //  NOT_REACHED();
  //}

  //shared_ptr<edge_with_cond_ec_t> get_edge_with_cond_composition_from_graph_edge_composition(graph_edge_composition_ref<T_PC,T_E> const& graph_ec) const
  //{
  //  std::function<edge_with_cond_ec_ref (shared_ptr<T_E const> const&)> f_atom = [this](shared_ptr<T_E const> const& e)
  //  {
  //    shared_ptr<edge_with_cond<T_PC> const> ewc = static_pointer_cast<edge_with_cond<T_PC> const>(e);
  //    ASSERT(ewc);
  //    return make_shared<edge_with_cond_ec_t>(ewc);
  //  };
  //  std::function<edge_with_cond_ec_ref (typename graph_edge_composition_t<T_PC,T_E>::type, edge_with_cond_ec_ref const&, edge_with_cond_ec_ref const&)> f_fold =
  //    [](typename graph_edge_composition_t<T_PC,T_E>::type t, edge_with_cond_ec_ref const& a, edge_with_cond_ec_ref const& b)
  //  {
  //    typename edge_with_cond_ec_t::type ect = convert_graph_ec_type_to_edge_with_cond_ec_type(t);
  //    return make_shared<edge_with_cond_ec_t>(ect, a, b);
  //  };
  //  return graph_ec->visit_nodes(f_atom, f_fold);
  //}

  //graph_edge_composition_ref<T_PC,T_E> get_graph_edge_composition_from_edge_with_cond_composition(shared_ptr<edge_with_cond_ec_t> const& ec) const
  //{
  //  std::function<graph_edge_composition_ref<T_PC,T_E> (shared_ptr<edge_with_cond<T_PC> const> const&)> f_atom =
  //    [](shared_ptr<edge_with_cond<T_PC> const> const& e)
  //  {
  //    return graph_edge_composition_t<T_PC,T_E>::mk_edge_composition(e);
  //  };
  //  std::function<graph_edge_composition_ref<T_PC,T_E> (typename edge_with_cond_ec_t::type, graph_edge_composition_ref<T_PC,T_E> const&, graph_edge_composition_ref<T_PC,T_E> const&)> f_fold =
  //    [](typename edge_with_cond_ec_t::type t, graph_edge_composition_ref<T_PC,T_E> const& a, graph_edge_composition_ref<T_PC,T_E> const& b)
  //  {
  //    if (t == edge_with_cond_ec_t::series) {
  //      return graph_edge_composition_t<T_PC,T_E>::mk_series(a, b);
  //    } else if (t == edge_with_cond_ec_t::parallel) {
  //      return graph_edge_composition_t<T_PC,T_E>::mk_parallel(a, b);
  //    } else NOT_REACHED();
  //    //typename graph_edge_composition_t<T_PC, T_N, T_E>::type gt = convert_edge_with_cond_ec_type_to_graph_ec_type(t);
  //  };
  //  return ec->visit_nodes(f_atom, f_fold);
  //}
};

}
