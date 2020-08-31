#pragma once

#include "gsupport/pc.h"
#include "graph/graph.h"
#include "graph/graph_with_aliasing.h"
#include "support/utils.h"
#include "support/log.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "graph/binary_search_on_preds.h"
#include "graph/lr_map.h"
#include "gsupport/sprel_map.h"
/* #include "graph/graph_cp.h" */
#include "expr/local_sprel_expr_guesses.h"
#include "support/globals_cpp.h"
#include "gsupport/edge_with_cond.h"
#include "tfg/edge_guard.h"
//#include "graph/concrete_values_set.h"
#include "support/timers.h"
#include "graph/delta.h"
#include "support/distinct_sets.h"
//#include "graph/invariant_state.h"
#include "expr/aliasing_constraints.h"
#include "support/dyn_debug.h"

#include <map>
#include <list>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_E>
class pc_path_t
{
private:
  T_PC m_pc;
  graph_edge_composition_ref<T_PC,T_E> m_path;
public:
  pc_path_t(T_PC const& p, graph_edge_composition_ref<T_PC,T_E> const& pth) : m_pc(p), m_path(pth)
  { }
  T_PC const& get_pc() const { return m_pc; }
  graph_edge_composition_ref<T_PC,T_E> const& get_path() const { return m_path; }
  bool operator<(pc_path_t const& other) const
  {
    return    m_path < other.m_path
           || (m_path == other.m_path && m_pc < other.m_pc)
    ;
  }
};

//graph_with_proof_obligations; graph_with_proofs for short
template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_paths : public graph_with_aliasing<T_PC, T_N, T_E, T_PRED>
{
private:
  mutable map<pc_path_t<T_PC,T_E>, unordered_set<shared_ptr<T_PRED const>>> m_path_assumes_cache;
  mutable map<pc_path_t<T_PC,T_E>, aliasing_constraints_t> m_path_aliasing_constraints_cache;
  using T_PRED_REF = shared_ptr<T_PRED const>;
  using T_PRED_SET = unordered_set<T_PRED_REF>;

public:
  using edge_with_cond_ec_t = serpar_composition_node_t<shared_ptr<edge_with_cond<T_PC> const>>;
  using edge_with_cond_ec_ref = shared_ptr<edge_with_cond_ec_t>;

  graph_with_paths(string const &name, context* ctx) : graph_with_aliasing<T_PC, T_N, T_E, T_PRED>(name, ctx)
  {
  }

  graph_with_paths(graph_with_paths const &other) : graph_with_aliasing<T_PC, T_N, T_E, T_PRED>(other)//,
                                                    //m_path_assumes_cache(other.m_path_assumes_cache) //deliberately not copying the caches, it should not matter
                                                    //m_path_aliasing_constraints_cache(other.m_path_aliasing_constraints_cache) //deliberately not copying the caches, it should not matter
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_paths(istream& is, string const& name, context* ctx) : graph_with_aliasing<T_PC, T_N, T_E, T_PRED>(is, name, ctx)
  {
  }

  virtual ~graph_with_paths() = default;

  virtual expr_ref edge_guard_get_expr(edge_guard_t const& eg, bool simplified) const = 0;

  virtual T_PRED_SET collect_assumes_for_edge(shared_ptr<T_E const> const& te) const = 0;
  virtual T_PRED_SET collect_assumes_around_edge(shared_ptr<T_E const> const& te) const = 0;

  void graph_with_paths_clear_aliasing_constraints_cache() const
  {
    //cout << __func__ << " " << __LINE__ << ": clearing aliasing_constraints cache\n";
    m_path_aliasing_constraints_cache.clear();
  }

  T_PRED_SET collect_assumes_around_path(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const &ec) const
  {
    autostop_timer func_timer(__func__);
    pc_path_t<T_PC,T_E> pc_path(from_pc, ec);
    if (m_path_assumes_cache.count(pc_path)) {
      if (!GRAPH_WITH_PATHS_CACHE_ASSERTS_flag) {
        return m_path_assumes_cache.at(pc_path);
      }
    }

    T_PRED_SET ret = this->collect_assumes_around_path_helper(from_pc, ec);
    m_path_assumes_cache.insert(make_pair(pc_path, ret));
    DYN_DEBUG2(decide_hoare_triple, cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << "\n");
    return ret;
  }

  // this function is used in most edge composition related computation, hence must be as fast as possible
  static expr_ref
  graph_get_cond_for_edge_composition(graph_edge_composition_ref<T_PC,T_E> const& ec, context* ctx, bool simplified, bool substituted = true)
  {
    autostop_timer ft(__func__);

    //context* ctx = this->get_context();
    expr_ref const& post_cons = expr_true(ctx);

    if (!ec) {
        return post_cons;
    }

    //state const& start_state = this->get_start_state();
    std::function<expr_ref (expr_ref const&, expr_ref const&, state const&, unordered_set<expr_ref> const&)> atom_f = [simplified, substituted, ctx/*, &start_state*/](expr_ref const& postorder_val, expr_ref const& edgecond, state const& to_state, unordered_set<expr_ref> const& assumes)
    {
      expr_ref const& esub = substituted ? expr_substitute_using_state(postorder_val/*, start_state*/, to_state) : postorder_val;
      expr_ref ret_econd = expr_and(edgecond, esub);
      if (simplified) {
        ret_econd = ctx->expr_do_simplify(ret_econd);
      }
      return ret_econd;
    };
    std::function<expr_ref (expr_ref const&, expr_ref const&)> meet_f =
      [simplified, ctx](expr_ref const& a, expr_ref const& b)
      {
        expr_ref econd = expr_or(a, b);
        if (simplified) {
          econd = ctx->expr_do_simplify(econd);
        }
        return econd;
      };
    return ec->template xfer_over_graph_edge_composition<expr_ref,xfer_dir_t::backward>(post_cons, atom_f, meet_f, simplified);
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
    pc_path_t<T_PC,T_E> pc_path(from_pc, pth);
    if (m_path_aliasing_constraints_cache.count(pc_path)) {
      if (!GRAPH_WITH_PATHS_CACHE_ASSERTS_flag) {
        auto const& ret = m_path_aliasing_constraints_cache.at(pc_path);
        //cout << __func__ << " " << __LINE__ << ": returning from cache for " << pth->graph_edge_composition_to_string() << ":\n" << ret.to_string_for_eq() << endl;
        return ret;
      }
    }

    std::function<aliasing_constraints_t (shared_ptr<T_E const> const &)> edge_atom_f = [this](shared_ptr<T_E const> const& e)
    {
      return this->get_aliasing_constraints_for_edge(e);
    };
    //cout << "graph_with_paths::" << __func__ << " " << __LINE__ << ": from_pc = " << from_pc.to_string() << ", pth = " << pth->graph_edge_composition_to_string() << ":\n" << ret.to_string_for_eq() << endl;
    ret = this->collect_aliasing_constraints_around_path_helper(from_pc, pth, edge_atom_f);

    if (GRAPH_WITH_PATHS_CACHE_ASSERTS_flag) {
      if (m_path_aliasing_constraints_cache.count(pc_path)) {
        if (!(ret == m_path_aliasing_constraints_cache.at(pc_path))) {
          cout << "pc = " << from_pc.to_string() << ", path = " << pth->to_string_concise() << endl;
          cout << "ret = " << ret.to_string_for_eq() << endl;
          cout << "m_path_aliasing_constraints_cache.at(pc_path) = " << m_path_aliasing_constraints_cache.at(pc_path).to_string_for_eq() << endl;
          cout.flush();
        }
        ASSERT(ret == m_path_aliasing_constraints_cache.at(pc_path));
      }
    }
    //cout << __func__ << " " << __LINE__ << ": inserting into cache for " << pth->graph_edge_composition_to_string() << ":\n" << ret.to_string_for_eq() << endl;
    m_path_aliasing_constraints_cache.insert(make_pair(pc_path, ret));
    return ret;
  }

  aliasing_constraints_t
  collect_aliasing_constraints_around_path_helper(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const& pth, std::function<aliasing_constraints_t (shared_ptr<T_E const> const&)> atom_f, aliasing_constraints_t const& post_cons = aliasing_constraints_t()) const
  {
    autostop_timer ft(__func__);

    std::function<aliasing_constraints_t (shared_ptr<T_E const> const &, aliasing_constraints_t const &postorder_val)> fold_atom_f =
			[this, &atom_f](shared_ptr<T_E const> const &e, aliasing_constraints_t const &postorder_val)
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
      list<shared_ptr<T_E const>> outgoing;
      this->get_edges_outgoing(from_pc, outgoing);
      aliasing_constraints_t ret;
      for (auto const& o : outgoing) {
        ret.aliasing_constraints_union(fold_atom_f(o, aliasing_constraints_t()));
      }
      return ret;
    }
    return pth->template visit_nodes_postorder_series<aliasing_constraints_t>(fold_atom_f, f_fold_parallel, post_cons);
  }

  T_PRED_SET pth_collect_preds_using_atom_func(graph_edge_composition_ref<T_PC,T_E> const &pth, std::function<unordered_set<shared_ptr<T_PRED const>> (shared_ptr<T_E const> const&)> f_atom, unordered_set<shared_ptr<T_PRED const>> const& from_pc_preds, bool simplified = true) const
  {
    if (!pth || pth->is_epsilon()) {
      return from_pc_preds;
    }

    std::function<unordered_set<shared_ptr<T_PRED const>> (shared_ptr<T_E const> const&, unordered_set<shared_ptr<T_PRED const>> const&)> f_ec_atom =
      [this, f_atom, simplified](shared_ptr<T_E const> const& e, unordered_set<shared_ptr<T_PRED const>> const& in)
    {
      T_PRED_SET ret;
      for (auto const& ipred : in) {
        list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<T_PRED const>>> ipred_apply = this->graph_apply_trans_funs(mk_edge_composition<T_PC,T_E>(e), ipred, simplified);
        list<shared_ptr<T_PRED const>> ipred_eval = fold_paths_into_preds(ipred_apply);
        for (auto const& ipreda : ipred_eval) {
          ret.insert(ipreda);
        }
      }
      T_PRED::predicate_set_union(ret, f_atom(e));
      return ret;
    };

    std::function<unordered_set<shared_ptr<T_PRED const>> (unordered_set<shared_ptr<T_PRED const>> const&, unordered_set<shared_ptr<T_PRED const>> const&)> f_fold_parallel =
      [](unordered_set<shared_ptr<T_PRED const>> const& a, unordered_set<shared_ptr<T_PRED const>> const& b)
    {
      T_PRED_SET preds_ret = a;
      T_PRED::predicate_set_union(preds_ret, b);
      return preds_ret;
    };
    unordered_set<shared_ptr<T_PRED const>> path_preds = pth->template visit_nodes_postorder_series<unordered_set<shared_ptr<T_PRED const>>>(f_ec_atom, f_fold_parallel, unordered_set<shared_ptr<T_PRED const>>());
    return path_preds;
  }

  virtual list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<T_PRED const>>> graph_apply_trans_funs(graph_edge_composition_ref<T_PC,T_E> const &pth, shared_ptr<T_PRED const> const &pred, bool simplified = false) const = 0;

  virtual T_PRED_SET get_simplified_non_mem_assumes(shared_ptr<T_E const> const& e) const = 0;

protected:

  virtual void graph_to_stream(ostream& os) const override
  {
    this->graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::graph_to_stream(os);
  }

  list<T_PRED_REF>
  fold_paths_into_preds(list<pair<graph_edge_composition_ref<T_PC,T_E>, T_PRED_REF>> const &ps) const
  {
    list<T_PRED_REF> ret;
    for (const auto &p : ps) {
      graph_edge_composition_ref<T_PC,T_E> ec = p.first;
      T_PRED_REF pred = p.second;
      edge_guard_t eg = this->graph_get_edge_guard_from_edge_composition(ec);
      eg.add_guard_in_series(pred->get_edge_guard());
      pred = pred->set_edge_guard(eg);
      ret.push_back(pred);
    }
    return ret;
  }

  virtual unordered_set<shared_ptr<T_PRED const>> collect_inductive_preds_around_path(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const &ec) const
  {
    return unordered_set<shared_ptr<T_PRED const>>(); //return empty set for now; will return non-empty if inductive preds are computed (e.g., for TFG, CG)
  }

  T_PRED_SET
  collect_non_mem_assumes_from_edge_list(list<shared_ptr<T_E const>> const& elist) const
  {
    T_PRED_SET ret;
    for (auto const& e : elist) {
      T_PRED::predicate_set_union(ret, this->get_simplified_non_mem_assumes(e));
    }
    return ret;
  }

  aliasing_constraints_t graph_generate_aliasing_constraints_for_edgecond_and_to_state(expr_ref const& econd, state const& to_state) const
  {
    auto const& memlabel_map = this->get_memlabel_map();
    aliasing_constraints_t ret;
    // edgecond
    ret.aliasing_constraints_union(generate_aliasing_constraints_from_expr(econd, memlabel_map));

    // to_state
    std::function<void (const string&, expr_ref)> fs = [&ret,&memlabel_map](const string& key, expr_ref e) {
      ret.aliasing_constraints_union(generate_aliasing_constraints_from_expr(e, memlabel_map));
    };
    //state const& start_state = this->get_start_state();
    to_state.state_visit_exprs_with_start_state(fs/*, start_state*/);

    //XXX: also add aliasing constraints due to the locs by using m_linear_relations?
    return ret;
  }

private:

	virtual aliasing_constraints_t get_aliasing_constraints_for_edge(shared_ptr<T_E const>const& e) const = 0;
  virtual aliasing_constraints_t graph_apply_trans_funs_on_aliasing_constraints(shared_ptr<T_E const> const& e, aliasing_constraints_t const& als) const = 0;

  virtual edge_guard_t graph_get_edge_guard_from_edge_composition(graph_edge_composition_ref<T_PC,T_E> const &ec) const = 0;

  T_PRED_SET collect_assumes_around_path_helper(T_PC const& from_pc, graph_edge_composition_ref<T_PC,T_E> const &ec) const
  {
    std::function<unordered_set<shared_ptr<T_PRED const>> (shared_ptr<T_E const> const&)> pf = [this](shared_ptr<T_E const> const& te)
    {
      return this->collect_assumes_for_edge(te);
    };

    T_PRED_SET ret = this->get_assume_preds(from_pc);
    ASSERT(ret.empty() || from_pc.is_start());
    if (!ec || ec->is_epsilon()) {
      list<shared_ptr<T_E const>> outgoing;
      this->get_edges_outgoing(from_pc, outgoing);
      for (auto const& o : outgoing) {
        T_PRED::predicate_set_union(ret, this->collect_assumes_for_edge(o));
      }
    } else {
      ret = pth_collect_preds_using_atom_func(ec, pf, ret);
    }

    list<shared_ptr<T_E const>> elist = this->get_maximal_basic_block_edge_list_ending_at_pc(from_pc);
    // SSA form allows us to collect preds not containing memory ops without computing postcondition
    T_PRED_SET ret_nonmem = this->collect_non_mem_assumes_from_edge_list(elist);
    T_PRED::predicate_set_union(ret, ret_nonmem);

    return ret;
  }

  static typename edge_with_cond_ec_t::type convert_graph_ec_type_to_edge_with_cond_ec_type(typename graph_edge_composition_t<T_PC,T_E>::type t)
  {
    switch (t) {
      case graph_edge_composition_t<T_PC,T_E>::atom:
        return edge_with_cond_ec_t::atom;
      case graph_edge_composition_t<T_PC,T_E>::series:
        return edge_with_cond_ec_t::series;
      case graph_edge_composition_t<T_PC,T_E>::parallel:
        return edge_with_cond_ec_t::parallel;
    }
    NOT_REACHED();
  }

  static typename graph_edge_composition_t<T_PC,T_E>::type convert_graph_ec_type_to_edge_with_cond_ec_type(typename edge_with_cond_ec_t::type t)
  {
    switch (t) {
      case edge_with_cond_ec_t::atom:
        return graph_edge_composition_t<T_PC,T_E>::atom;
      case edge_with_cond_ec_t::series:
        return graph_edge_composition_t<T_PC,T_E>::series;
      case edge_with_cond_ec_t::parallel:
        return graph_edge_composition_t<T_PC,T_E>::parallel;
    }
    NOT_REACHED();
  }

  shared_ptr<edge_with_cond_ec_t> get_edge_with_cond_composition_from_graph_edge_composition(graph_edge_composition_ref<T_PC,T_E> const& graph_ec) const
  {
    std::function<edge_with_cond_ec_ref (shared_ptr<T_E const> const&)> f_atom = [this](shared_ptr<T_E const> const& e)
    {
      shared_ptr<edge_with_cond<T_PC> const> ewc = static_pointer_cast<edge_with_cond<T_PC> const>(e);
      ASSERT(ewc);
      return make_shared<edge_with_cond_ec_t>(ewc);
    };
    std::function<edge_with_cond_ec_ref (typename graph_edge_composition_t<T_PC,T_E>::type, edge_with_cond_ec_ref const&, edge_with_cond_ec_ref const&)> f_fold =
      [](typename graph_edge_composition_t<T_PC,T_E>::type t, edge_with_cond_ec_ref const& a, edge_with_cond_ec_ref const& b)
    {
      typename edge_with_cond_ec_t::type ect = convert_graph_ec_type_to_edge_with_cond_ec_type(t);
      return make_shared<edge_with_cond_ec_t>(ect, a, b);
    };
    return graph_ec->visit_nodes(f_atom, f_fold);
  }

  graph_edge_composition_ref<T_PC,T_E> get_graph_edge_composition_from_edge_with_cond_composition(shared_ptr<edge_with_cond_ec_t> const& ec) const
  {
    std::function<graph_edge_composition_ref<T_PC,T_E> (shared_ptr<edge_with_cond<T_PC> const> const&)> f_atom =
      [](shared_ptr<edge_with_cond<T_PC> const> const& e)
    {
      return mk_edge_composition<T_PC,T_E>(e);
    };
    std::function<graph_edge_composition_ref<T_PC,T_E> (typename edge_with_cond_ec_t::type, graph_edge_composition_ref<T_PC,T_E> const&, graph_edge_composition_ref<T_PC,T_E> const&)> f_fold =
      [](typename edge_with_cond_ec_t::type t, graph_edge_composition_ref<T_PC,T_E> const& a, graph_edge_composition_ref<T_PC,T_E> const& b)
    {
      if (t == edge_with_cond_ec_t::series) {
        return mk_series<T_PC, T_N, T_E>(a, b);
      } else if (t == edge_with_cond_ec_t::parallel) {
        return mk_parallel<T_PC, T_N, T_E>(a, b);
      } else NOT_REACHED();
      //typename graph_edge_composition_t<T_PC, T_N, T_E>::type gt = convert_edge_with_cond_ec_type_to_graph_ec_type(t);
    };
    return ec->visit_nodes(f_atom, f_fold);
  }

};

}
