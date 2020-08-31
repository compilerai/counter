#pragma once

#include "gsupport/pc.h"
#include "graph/graph.h"
#include "graph/graph_with_proofs.h"
#include "graph/graph_with_ce.h"
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

#include "gsupport/edge_with_cond.h"
//#include "graph/edge_guard.h"
//#include "graph/concrete_values_set.h"
#include "support/timers.h"
#include "graph/delta.h"
#include "graph/dfa.h"
#include "graph/point_set.h"
#include "graph/invariant_state.h"
#include "graph/eqclasses.h"
#include "support/dyn_debug.h"
#include "graph/expr_group.h"

#include <map>
#include <list>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class infer_invariants_t : public data_flow_analysis<T_PC, T_N, T_E, invariant_state_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::forward>
{
public:
  infer_invariants_t(graph<T_PC, T_N, T_E> const* g, map<T_PC, invariant_state_t<T_PC, T_N, T_E, T_PRED>> &invariant_state_map)
    : data_flow_analysis<T_PC, T_N, T_E, invariant_state_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::forward>(g, invariant_state_map)
  {
    //invariant_state_map.at(T_PC::start())
    //graph_with_ce<T_PC, T_N, T_E, T_PRED> const *gce = dynamic_cast<graph_with_ce<T_PC, T_N, T_E, T_PRED> const*>(g);
    //ASSERT(gce != nullptr);
    //for (const auto &p : g->get_all_pcs()) {
    //  this->m_vals.insert(make_pair(p, invariant_state_t<T_PC, T_N, T_E, T_PRED>(pc_expr_eqclasses.at(p), pc_points_map.at(p), *gce)));
    //}
  }

  /*invariant_state_t<T_PC, T_N, T_E, T_PRED>
  meet(invariant_state_t<T_PC, T_N, T_E, T_PRED> const &old_val, invariant_state_t<T_PC, T_N, T_E, T_PRED> const &new_val) override
  {
    auto ret = new_val.invariant_state_meet(old_val);
    cout << __func__ << " " << __LINE__ << ": ret =\n" << ret.invariant_state_to_string("  ");
    return ret;
    //invariant_state_t<T_PC, T_N, T_E, T_PRED> ret = a;
    //ret.meet(b);
    //return ret;
  }*/

  virtual bool
  xfer_and_meet(shared_ptr<T_E const> const &e, invariant_state_t<T_PC, T_N, T_E, T_PRED> const& in, invariant_state_t<T_PC, T_N, T_E, T_PRED> &out) override
  {
    //auto old_val = out;
    graph_with_guessing<T_PC, T_N, T_E, T_PRED> const *gg = dynamic_cast<graph_with_guessing<T_PC, T_N, T_E, T_PRED> const *>(this->m_graph);
    ASSERT(gg);
    return out.xfer_on_incoming_edge(e, in, *gg);
    //return old_val != out;
    //invariant_state_t<T_PC, T_N, T_E, T_PRED> ret = this->m_vals.at(e->get_to_pc());
    //ret.xfer_on_incoming_edge(e, in);
    //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": ret =\n" << ret.invariant_state_to_string("  ");
    //return out.invariant_state_meet(ret);
  }
};

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_guessing : public graph_with_ce<T_PC, T_N, T_E, T_PRED>
{
private:
  mutable map<T_PC, invariant_state_t<T_PC, T_N, T_E, T_PRED>> m_invariant_state_map;
public:

  graph_with_guessing(string const &name, context *ctx, memlabel_assertions_t const& memlabel_assertions)
    : graph_with_ce<T_PC, T_N, T_E, T_PRED>(name, ctx, memlabel_assertions)
  { }

  graph_with_guessing(graph_with_guessing const &other)
    : graph_with_ce<T_PC, T_N, T_E, T_PRED>(other),
      m_invariant_state_map(other.m_invariant_state_map)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_guessing(istream& is, string const& name, context* ctx) : graph_with_ce<T_PC, T_N, T_E, T_PRED>(is, name, ctx)
  {
    string line;
    bool end;
    do {
      end = !getline(is, line);
      ASSERT(!end);
    } while (line == "");
    ASSERT(line == "=graph_with_guessing begin");
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    end = !getline(is, line);
    ASSERT(!end);
    ASSERT(line == "=Invariant states");
    end = !getline(is, line);
    ASSERT(!end);
    string const prefix = "=Invariant state at node ";
    while (string_has_prefix(line, prefix)) {
      string pc_str = line.substr(prefix.length());
      T_PC p = T_PC::create_from_string(pc_str);
      string pc_prefix = "pc " + p.to_string() + " ";
      invariant_state_t<T_PC, T_N, T_E, T_PRED> invstate;
      invstate.invariant_state_from_stream(is/*, this->get_start_state()*/, ctx, pc_prefix, this->get_locs());
      m_invariant_state_map.insert(make_pair(p, invstate));
      end = !getline(is, line);
      ASSERT(!end);
    }
    if (line != "=graph_with_guessing done") {
      cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
    }
    ASSERT(line == "=graph_with_guessing done");
    for (auto const& p : this->get_all_pcs()) {
      if (!m_invariant_state_map.count(p)) {
        m_invariant_state_map.insert(make_pair(p, invariant_state_t<T_PC, T_N, T_E, T_PRED>::empty_invariant_state(ctx)));
      }
    }
    this->replay_counter_examples();
  }

  virtual ~graph_with_guessing() = default;


  void graph_with_guessing_add_node(T_PC const &p, eqclasses_ref expr_eqclasses_at_pc = nullptr)
  {
    autostop_timer ft(__func__);
    context *ctx = this->get_context();
    //m_pc_expr_eqclasses_map.insert(make_pair(p, expr_eqclasses_at_pc));
    //list<point_set_ref> points = init_points_map_at_pc(expr_eqclasses_at_pc);
    //m_pc_points_map.insert(make_pair(p, points));
    ASSERT(!m_invariant_state_map.count(p));
    if (p.is_start() || p.is_exit()) {
      m_invariant_state_map.insert(make_pair(p, invariant_state_t<T_PC, T_N, T_E, T_PRED>::empty_invariant_state(ctx)));
    } else {
      if (!expr_eqclasses_at_pc) {
        expr_eqclasses_at_pc = compute_expr_eqclasses_at_pc(p);
      }
      m_invariant_state_map.insert(make_pair(p, invariant_state_t<T_PC, T_N, T_E, T_PRED>::pc_invariant_state(p, expr_eqclasses_at_pc, this->get_locs(), ctx)));
    }
  }

  //returns true if all asserts are still provable
  bool recompute_invariants_at_all_pcs()
  {
    autostop_timer fn(__func__);
    auto old_invariant_state_map = m_invariant_state_map;
    m_invariant_state_map.clear();
    for (auto const& p : this->get_all_pcs()) {
      this->graph_with_guessing_add_node(p, old_invariant_state_map.at(p).get_expr_eqclasses());
    }
    infer_invariants_t<T_PC, T_N, T_E, T_PRED> infer_invariants_dfa(this, m_invariant_state_map);
    bool changed = infer_invariants_dfa.solve();
    for (const auto &pc_inv : m_invariant_state_map) {
      T_PC const &p = pc_inv.first;
      if (!this->check_asserts_on_outgoing_edges(p)) {
        return false;
      }
    }
    return true;
  }

  //returns true if all asserts are still provable
  bool update_invariant_state_over_edge(shared_ptr<T_E const> const &e)
  {
    autostop_timer fn(__func__);

    //cout << __func__ << " " << __LINE__ << ": this->m_edge_to_simplified_state.size() = " << this->m_edge_to_simplified_state.size() << endl;

    auto pth = mk_edge_composition<T_PC,T_E>(e);
    DYN_DEBUG(update_invariant_state_over_edge,
      stringstream ss;
      ss << "edge_from_pc_" << e->get_from_pc().get_first().get_index() << "_" << e->get_from_pc().get_second().get_index() << "_to_pc_" << e->get_to_pc().get_first().get_index() << "_" << e->get_to_pc().get_second().get_index();
      query_comment qc(ss.str());
      ss.str("");
      string pid = int_to_string(getpid());
      ss << g_query_dir << "/" << UPDATE_INVARIANT_STATE_OVER_EDGE_FILENAME_PREFIX << "." << qc.to_string();
      string filename = ss.str();
      ofstream fo(filename.data());
      ASSERT(fo);
      this->graph_to_stream(fo);
      fo << "=edge\n";
      //fo << pth->graph_edge_composition_to_string() << endl;
      fo << pth->graph_edge_composition_to_string_for_eq("=update_invariant_state_over_edge");
      fo << "=end\n";
      fo.close();
      //g_query_files.insert(filename);
      cout << "update_invariant_state_over_edge filename " << filename << '\n';
    );

    auto old_invariant_state_map = m_invariant_state_map;

    infer_invariants_t<T_PC, T_N, T_E, T_PRED> infer_invariants_dfa(this, m_invariant_state_map);
    bool changed = infer_invariants_dfa.incremental_solve(e);
    if (changed) {
      for (const auto &pc_inv : m_invariant_state_map) {
        T_PC const &p = pc_inv.first;
        if (old_invariant_state_map.at(p) != pc_inv.second) {
          DYN_DEBUG2(eqcheck, cout << _FNLN_ << ": invariant state changed at " << p << ", calling check_asserts_on_outgoing_edges" << endl);
          if (!this->check_asserts_on_outgoing_edges(p)) {
            return false;
          }
        }
      }
    }
    return true;
  }

  bool
  invariant_state_exists_for_pc(T_PC const &p) const
  {
    return m_invariant_state_map.count(p) != 0;
  }

  invariant_state_t<T_PC, T_N, T_E, T_PRED> const &
  get_invariant_state_at_pc(T_PC const &p) const
  {
    if (!m_invariant_state_map.count(p)) {
      cout << __func__ << ':' << __LINE__ << ": pc : " << p.to_string() << endl;
      cout << __func__ << ':' << __LINE__ << ": m_invariant_state_map pcs: ";
      for (auto const& pc_inv : m_invariant_state_map) {
        cout << pc_inv.first.to_string() << ' ';
      }
      cout << endl;
      cout.flush();
    }
    ASSERT(m_invariant_state_map.count(p));
    return m_invariant_state_map.at(p);
  }

  virtual unordered_set<shared_ptr<T_PRED const>>
  graph_with_guessing_get_invariants_at_pc(T_PC const &p) const
  {
    if (!m_invariant_state_map.count(p)) {
      return {};
    }
    return m_invariant_state_map.at(p).get_all_preds();
  }

  virtual map<T_PC, unordered_set<shared_ptr<T_PRED const>>>
  graph_with_guessing_get_invariants_map() const
  {
    map<T_PC, unordered_set<shared_ptr<T_PRED const>>> ret;
    for (auto const& p : this->get_all_pcs()) {
      auto const& preds = this->graph_with_guessing_get_invariants_at_pc(p);
      //cout << __func__ << " " << __LINE__ << ": preds.size() = " << preds.size() << endl;
      ret.insert(make_pair(p, preds));
      //cout << __func__ << " " << __LINE__ << ": ret.at(" << p.to_string() << ").size() = " << ret.at(p).size() << endl;
    }
    return ret;
  }

  string graph_with_guessing_invariants_to_string(bool with_points) const
  {
    stringstream ss;
    for (auto const& invstate : m_invariant_state_map) {
      T_PC const &p = invstate.first;
      invariant_state_t<T_PC, T_N, T_E, T_PRED> const &istate = invstate.second;
      ss << p.to_string() << ":\n";
      ss << istate.invariant_state_to_string(/*p, */"  ", with_points);
    }
    return ss.str();
  }

  //friend class invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>;

  bool check_asserts_on_edge(shared_ptr<T_E const> const &e) const
  {
    autostop_timer ft(__func__);
    T_PC const &from_pc = e->get_from_pc();
    T_PC const &to_pc = e->get_to_pc();
    if (!this->graph_edge_is_well_formed(e)) { //well-formedness for CG edge involves checking that dst edgecond implies src edgecond
      return false;
    }
    unordered_set<shared_ptr<T_PRED const>> const &assert_preds = this->get_simplified_assert_preds(to_pc);
    for (const auto &assert_pred : assert_preds) {
      bool guess_made_weaker;
      proof_status_t proof_status = this->decide_hoare_triple({}, from_pc, mk_edge_composition<T_PC,T_E>(e), assert_pred, guess_made_weaker);
      if (proof_status == proof_status_timeout) {
        NOT_IMPLEMENTED();
      }
      if (proof_status != proof_status_proven) {
        cout << __func__ << " " << __LINE__ << ": assert pred failed on edge " << e->to_string() << ": " << assert_pred->to_string(true) << endl;
        return false;
      }
    }
    return true;
  }

  virtual bool need_stability_at_pc(T_PC const& p) const = 0;


protected:

  virtual bool graph_edge_is_well_formed(shared_ptr<T_E const> const& e) const = 0;

  virtual void graph_to_stream(ostream& os) const override
  {
    this->graph_with_ce<T_PC, T_N, T_E, T_PRED>::graph_to_stream(os);
    os << "=graph_with_guessing begin\n";
    os << "=Invariant states\n";
    for (auto const& pi : m_invariant_state_map) {
      os << "=Invariant state at node " << pi.first.to_string() << ":\n";
      string prefix = "pc " + pi.first.to_string() + " ";
      pi.second.invariant_state_to_stream(os, prefix);
    }
    os << "=graph_with_guessing done\n";
  }

  eqclasses_ref get_expr_eqclasses_at_pc(T_PC const& p) const
  {
    if (!m_invariant_state_map.count(p)) {
      return mk_eqclasses(list<expr_group_ref>());
    } else {
      return m_invariant_state_map.at(p).invariant_state_get_expr_eqclasses();
    }
  }
  virtual bool is_stable_at_pc(T_PC const& pp) const override    
  {                                       
    return this->get_invariant_state_at_pc(pp).is_stable(); 
  }


private:

  void replay_counter_examples()
  {
    for (auto const& p : this->get_all_pcs()) {
      list<nodece_ref<T_PC, T_N, T_E>> const& nces = this->get_counterexamples_at_pc(p);
      for (auto const& nce : nces) {
        DYN_DEBUG(replay_counter_examples, cout << __func__ << " " << __LINE__ << ": replaying nce at pc " << p.to_string() << "=\n"; nce->nodece_to_stream(cout, ""));
        this->add_point_using_CE_at_pc(p, nce);
      }
    }
  }

  bool check_asserts_on_outgoing_edges(T_PC const &p) const
  {
    autostop_timer func_timer(__func__);
    list<shared_ptr<T_E const>> outgoing;
    this->get_edges_outgoing(p, outgoing);
    for (auto const& e : outgoing) {
      if (!this->check_asserts_on_edge(e)) {
        return false;
      }
    }
    return true;
  }

  // exprs in same equivalence-class may be related but two exprs in two different eq-classes are definitely not related
  // in addition, the distribution is NOT based on sorts
  // exprs having different sorts can belong to same equivalence-class
  virtual eqclasses_ref compute_expr_eqclasses_at_pc(T_PC const& p) const = 0;

  virtual bool add_CE_at_pc(T_PC const &p, nodece_ref<T_PC, T_N, T_E> const &nce) const override
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    bool ret = p.is_start() || this->add_point_using_CE_at_pc(p, nce);
    if (ret) {
      this->graph_with_ce<T_PC, T_N, T_E, T_PRED>::add_CE_at_pc(p, nce); //add CE at pc only if it increased the rank of the row space in some eqclass
    }
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    return ret;
  }

  bool add_point_using_CE_at_pc(T_PC const &p, nodece_ref<T_PC, T_N, T_E> const &nce) const
  {
    autostop_timer func_timer(__func__);
    PRINT_PROGRESS("%s() %d: entry\n", __func__, __LINE__);
    ASSERT(m_invariant_state_map.count(p));
    bool ret = m_invariant_state_map.at(p).invariant_state_add_point_using_CE(nce, *this);
    PRINT_PROGRESS("%s() %d: exit\n", __func__, __LINE__);
    return ret;
  }

};

}
