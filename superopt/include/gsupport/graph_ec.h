#pragma once

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>
#include <numeric>

#include "support/manager.h"
#include "support/mytimer.h"
#include "support/utils.h"
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/types.h"
#include "support/serpar_composition.h"

#include "expr/defs.h"

#include "gsupport/te_comment.h"
#include "gsupport/edge_with_unroll.h"
#include "gsupport/alloc_dealloc.h"

namespace eqspace {

class state;
class context;
class predicate;
class tfg_edge;
class pc;

//template <typename T_PC, typename T_N, typename T_E> class graph;
//template <typename T_PC, typename T_N, typename T_E, typename T_PRED> class graph_with_predicates;

template <typename T_PC, typename T_E> class graph_edge_composition_t;

template <typename T_PC, typename T_E, typename T_ATTR> class graph_ec_with_attr_t;

template<typename T_PC, typename T_E>
using graph_edge_composition_ref = shared_ptr<graph_edge_composition_t<T_PC,T_E> const>;

using guarded_pred_t = pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<predicate const>>;

template <typename T_PC, typename T_E>
class graph_edge_composition_t : public serpar_composition_node_t<edge_with_unroll_t<T_PC, T_E>>
{
  using serpar_composition_type = typename serpar_composition_node_t<edge_with_unroll_t<T_PC, T_E>>::type;
  using edge_w_unroll = class edge_with_unroll_t<T_PC, T_E>;
public:
  //static map<set<graph_edge_composition_ref<T_PC,T_E>>, graph_edge_composition_ref<T_PC,T_E>> *get_graph_edge_composition_pterms_canonicalize_cache();
  static manager<graph_edge_composition_t<T_PC,T_E>> *get_ecomp_manager();
public:
  graph_edge_composition_t(enum serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::type t, shared_ptr<graph_edge_composition_t const> const &a, shared_ptr<graph_edge_composition_t const> const &b) : serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>(t, a, b), m_deterministic_id(graph_edge_composition_t<T_PC, T_E>::compute_deterministic_id(t, a, b)), m_is_managed(false)
  { }

  //graph_edge_composition_t(T_E const &e) : serpar_composition_node_t<T_E>(e), m_is_managed(false)
  //{ }

  graph_edge_composition_t(dshared_ptr<T_E const> const &e) : serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>(make_dshared<edge_with_unroll_t<T_PC,T_E>>(edge_with_unroll_t<T_PC, T_E>(e, EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN))), m_deterministic_id(graph_edge_composition_t<T_PC, T_E>::compute_deterministic_id(e)), m_is_managed(false)
  { }

  graph_edge_composition_t(dshared_ptr<edge_with_unroll_t<T_PC,T_E> const> const &e) : serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>(edge_with_unroll_t<T_PC, T_E>(e->edge_with_unroll_get_edge(), EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN)), m_deterministic_id(graph_edge_composition_t<T_PC, T_E>::compute_deterministic_id(e->edge_with_unroll_get_edge())), m_is_managed(false)
  { /*ASSERT(e->edge_with_unroll_get_unroll() == EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN);*/ }

  bool operator==(graph_edge_composition_t const &other) const
  {  return serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::operator==(other);  }

  string graph_edge_composition_to_string() const
  {  return this->to_string_concise();  }

  string graph_edge_composition_to_string_using_pc_string_map(map<T_PC, string> const& pc_string_map) const;

  void graph_edge_composition_to_stream(ostream& os, string const& prefix) const;

  //bool graph_edge_composition_is_well_formed() const;

  //expr_ref graph_edge_composition_get_edge_cond(context* ctx, bool simplified = false, bool substituted = true) const;
  expr_ref graph_edge_composition_get_unsubstituted_edge_cond(context* ctx/*, bool simplified = false*/) const;

  expr_ref graph_edge_composition_get_edge_cond(context* ctx, bool substituted = true) const;
  state graph_edge_composition_get_to_state(context* ctx/*, bool simplified = false*/) const;

  //template<typename T_N, typename T_PRED>
  //unordered_set<expr_ref> graph_edge_composition_get_assumes(graph_with_predicates<T_PC,T_N,T_E,T_PRED> const& g) const;

  state graph_edge_composition_get_to_state_using_to_state_fn(context* ctx, std::function<state (dshared_ptr<edge_with_unroll_t<T_PC,T_E> const> const&)> to_state_fn) const;

  bool graph_ec_outgoing_edges_contain_edge(dshared_ptr<T_E const> const& e) const;

  list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> pth_collect_preds_using_atom_func(std::function<list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> (dshared_ptr<T_E const> const&)> f_atom, list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> const& from_pc_preds, context* ctx) const;

  list<pair<graph_edge_composition_ref<T_PC, T_E>, shared_ptr<predicate const>>> graph_ec_apply_trans_funs_using_to_state_fn(shared_ptr<predicate const> const &pred, context* ctx, std::function<state (dshared_ptr<edge_with_unroll_t<T_PC,T_E> const> const&)> f, bool always_merge= false) const;

  list<pair<graph_edge_composition_ref<T_PC, T_E>, shared_ptr<predicate const>>> graph_ec_apply_trans_funs(/*graph_edge_composition_ref<pc,tfg_edge> const &pth, */shared_ptr<predicate const> const &pred, context* ctx/*, bool simplified = false*/) const;

  shared_ptr<graph_ec_with_attr_t<T_PC,T_E,bool> const> graph_ec_add_last_status() const;

  te_comment_t graph_edge_composition_get_comment() const;

  bool is_epsilon() const
  {
    return    this->get_type() == serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::atom
           && this->get_atom()->is_empty();
  }
  graph_edge_composition_ref<T_PC,T_E> get_a_ec() const
  {
    shared_ptr<serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>> const> a = this->get_a();
    graph_edge_composition_ref<T_PC,T_E> ret = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(a);
    ASSERT(ret);
    return ret;
  }
  graph_edge_composition_ref<T_PC,T_E> get_b_ec() const
  {
    shared_ptr<serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>> const> b = this->get_b();
    graph_edge_composition_ref<T_PC,T_E> ret = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(b);
    ASSERT(ret);
    return ret;
  }
  T_PC graph_edge_composition_get_from_pc() const
  {
    if (this->get_type() == serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::atom) {
      return this->get_atom()->get_from_pc();
    } else if (this->get_type() == serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::series) {
      return this->get_a_ec()->graph_edge_composition_get_from_pc();
    } else if (this->get_type() == serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::parallel) {
      if (!this->get_a_ec()->is_epsilon()) {
        return this->get_a_ec()->graph_edge_composition_get_from_pc();
      }
      if (!this->get_b_ec()->is_epsilon()) {
        return this->get_b_ec()->graph_edge_composition_get_from_pc();
      }
      NOT_REACHED();
    }
    NOT_REACHED();
  }
  int find_max_node_occurrence() const;
  map<T_PC,int> graph_edge_composition_get_node_frequency_map() const;

  T_PC graph_edge_composition_get_to_pc() const
  {
    if (this->get_type() == serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::atom) {
      return this->get_atom()->get_to_pc();
    } else if (this->get_type() == serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::series) {
      return this->get_b_ec()->graph_edge_composition_get_to_pc();
    } else if (this->get_type() == serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::parallel) {
      if (!this->get_a_ec()->is_epsilon()) {
        return this->get_a_ec()->graph_edge_composition_get_to_pc();
      }
      if (!this->get_b_ec()->is_epsilon()) {
        return this->get_b_ec()->graph_edge_composition_get_to_pc();
      }
      NOT_REACHED();
    }
    NOT_REACHED();
  }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  bool id_is_zero() const { return !m_is_managed ; }
  ~graph_edge_composition_t()
  {
    if (m_is_managed) {
      this->get_ecomp_manager()->deregister_object(*this);
    }
  }

  map<T_PC, size_t> graph_edge_composition_get_pc_seen_count() const;

  void visit_exprs_const(function<void (string const&, expr_ref)> f) const;

  template<typename T_RET>
  T_RET graph_ec_visit_nodes_preorder_series(std::function<T_RET (dshared_ptr<T_E const> const &, T_RET const &)> fold_atom_f,
                                             std::function<T_RET (T_RET const &, T_RET const &)> fold_parallel_f,
                                             T_RET const &preorder_val,
                                             std::function<T_RET (T_RET const &, T_RET const &)> fold_series_f = ignore_first_arg_and_return_second<T_RET>) const
  {
    std::function<T_RET (dshared_ptr<edge_w_unroll const> const&, T_RET const&)> f = [&fold_atom_f]
      (dshared_ptr<edge_with_unroll_t<T_PC, T_E> const> const& eu, T_RET const& in)
    {
      return fold_atom_f(eu->edge_with_unroll_get_edge(), in);
    };
    return this->template visit_nodes_preorder_series<T_RET>(f, fold_parallel_f, preorder_val, fold_series_f);
  }

  template<typename T_RET>
  T_RET graph_ec_visit_nodes_postorder_series(std::function<T_RET (dshared_ptr<T_E const> const &, T_RET const &)> fold_atom_f,
                                              std::function<T_RET (T_RET const &, T_RET const &)> fold_parallel_f,
                                              T_RET const &postorder_val,
                                              std::function<T_RET (T_RET const &, T_RET const &)> fold_series_f = ignore_second_arg_and_return_first<T_RET>) const
  {
    std::function<T_RET (dshared_ptr<edge_w_unroll const> const&, T_RET const&)> f = [&fold_atom_f]
      (dshared_ptr<edge_with_unroll_t<T_PC, T_E> const> const& eu, T_RET const& in)
    {
      return fold_atom_f(eu->edge_with_unroll_get_edge(), in);
    };
    return this->template visit_nodes_postorder_series<T_RET>(f, fold_parallel_f, postorder_val, fold_series_f);
  }

  template<typename T_RET>
  T_RET graph_ec_visit_nodes(std::function<T_RET (dshared_ptr<T_E const> const &)> f, std::function<T_RET (typename graph_edge_composition_t<T_PC,T_E>::type, T_RET const &, T_RET const &)> fold_f) const
  {
    std::function<T_RET (dshared_ptr<edge_w_unroll const> const&)> f2 = [&f]
      (dshared_ptr<edge_with_unroll_t<T_PC, T_E> const> const& eu)
    {
      return f(eu->edge_with_unroll_get_edge());
    };
    return this->template visit_nodes<T_RET>(f, fold_f);
  }

  void graph_ec_visit_nodes_const(std::function<void (dshared_ptr<T_E const> const &)> f) const
  {
    std::function<void (dshared_ptr<edge_w_unroll const> const&)> f2 = [&f]
      (dshared_ptr<edge_with_unroll_t<T_PC, T_E> const> const& eu)
    {
      return f(eu->edge_with_unroll_get_edge());
    };
    return this->visit_nodes_const(f);
  }

  set<shared_ptr<T_E const>> graph_edge_composition_get_all_edges() const;

  static graph_edge_composition_ref<T_PC, T_E> mk_edge_composition(dshared_ptr<T_E const> const &e);
  static graph_edge_composition_ref<T_PC, T_E> mk_edge_composition(dshared_ptr<edge_with_unroll_t<T_PC,T_E> const> const &e);
  
  static graph_edge_composition_ref<T_PC,T_E> graph_edge_composition_construct_edge_from_serial_edgelist(list<graph_edge_composition_ref<T_PC,T_E>> const &edgelist);
  
  static graph_edge_composition_ref<T_PC,T_E> graph_edge_composition_construct_edge_from_parallel_edgelist(list<graph_edge_composition_ref<T_PC,T_E>> const &edgelist);
  
  static graph_edge_composition_ref<T_PC,T_E> graph_edge_composition_construct_edge_from_parallel_edges(set<graph_edge_composition_ref<T_PC,T_E>> const &parallel_edges);
  
  static list<graph_edge_composition_ref<T_PC,T_E>>
  graph_edge_composition_get_parallel_paths(graph_edge_composition_ref<T_PC,T_E> const &ec);
  
  static graph_edge_composition_ref<T_PC,T_E> mk_epsilon_ec(T_PC const& p);
  
  static graph_edge_composition_ref<T_PC,T_E> mk_epsilon_ec();
  
  static bool graph_edge_composition_is_epsilon(graph_edge_composition_ref<T_PC,T_E> const &ec);
  
  static graph_edge_composition_ref<T_PC,T_E> mk_series(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b);
  
  static graph_edge_composition_ref<T_PC,T_E> mk_parallel(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b);
  
  static tuple<int,int,int,int,int>
  get_path_and_edge_counts_for_edge_composition(graph_edge_composition_ref<T_PC,T_E> const& ec);
  
  static string
  edge_and_path_stats_string_for_edge_composition(graph_edge_composition_ref<T_PC,T_E> const& ec);
  
  // a POS string is (supposed to be) easier to understand for humans than normal graph_edge_composition_to_string
  // by removing extra parentheses wherever applicable (without introducing ambiguity)
  // e.g. POS string of
  //          ((a=>b).((b=>c).((c=>d)+(c=>e))))
  //  is
  //          (a=>b).(b=>c).((c=>d)+(c=>e))
  static string graph_edge_composition_to_pos_string(graph_edge_composition_ref<T_PC,T_E> const &a);
  
  /* returns over-approximate sound answer to the edge composition disjointness query
   * i.e. will return true iff the edge compositions are provable disjoint
   * otherwise return false
   */
  //static bool
  //graph_edge_composition_are_disjoint(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b);

  static bool graph_edge_composition_implies(shared_ptr<graph_edge_composition_t<T_PC,T_E> const> const &a, shared_ptr<graph_edge_composition_t<T_PC,T_E> const> const &b);

  static graph_edge_composition_ref<T_PC,T_E> mk_suffixpath_series_with_edge(graph_edge_composition_ref<T_PC,T_E> const &a, shared_ptr<T_E const> const &e);

  std::string get_deterministic_id() const { return m_deterministic_id; }

  list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>>
  pth_collect_preds_using_atom_func_and_to_state_fn(std::function<list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> (dshared_ptr<T_E const> const&)> f_atom, list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> const& from_pc_preds, context* ctx, std::function<state (dshared_ptr<edge_with_unroll_t<T_PC,T_E> const> const&)> to_state_fn) const;

  list<T_PC> graph_edge_composition_compute_topsort() const;

  pair<map<T_PC,list<dshared_ptr<T_E const>>>,map<T_PC,list<dshared_ptr<T_E const>>>> graph_edge_composition_get_incoming_outgoing_edges() const;

private:
  list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>>
  graph_ec_apply_trans_funs_helper(shared_ptr<predicate const> const &pred, context* ctx, std::function<state (dshared_ptr<edge_with_unroll_t<T_PC,T_E> const> const&)> to_state_fn, bool always_merge) const;


  graph_edge_composition_ref<T_PC,T_E> get_managed_reference() const;
  static std::string compute_deterministic_id(dshared_ptr<T_E const> const& e);
  static std::string compute_deterministic_id(enum serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>::type t, shared_ptr<graph_edge_composition_t const> const &a, shared_ptr<graph_edge_composition_t const> const &b);

  //template<typename T_N, typename T_PRED>
  //static
  //list<shared_ptr<predicate const>>
  //fold_paths_into_preds(list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> const &ps, graph_with_predicates<T_PC,T_N,T_E,T_PRED> const& g);

private:
  std::string m_deterministic_id;
  bool m_is_managed;
};

}

namespace std
{
using namespace eqspace;
template<typename T_PC, typename T_E>
struct hash<graph_edge_composition_t<T_PC,T_E>>
{
  std::size_t operator()(graph_edge_composition_t<T_PC,T_E> const &s) const
  {
    serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>> const &sd = s;
    return hash<serpar_composition_node_t<edge_with_unroll_t<T_PC,T_E>>>()(sd);
  }
};

}


namespace eqspace {

//template<typename T_PC, typename T_E>
//graph_edge_composition_ref<T_PC,T_E> mk_edge_composition(T_E const &e) //this constructor is required for parser where it is used with T_E=edge_id_t<pc>
//{
//  graph_edge_composition_t<T_PC,T_E> ec(e);
//  return graph_edge_composition_t<T_PC,T_E>::get_ecomp_manager()->register_object(ec);
//}


}
