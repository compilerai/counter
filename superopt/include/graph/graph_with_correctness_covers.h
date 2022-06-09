#pragma once

#include <map>
#include <list>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

#include "graph/graph_with_guessing.h"
#include "graph/dfa_over_paths.h"
#include "graph/reason_for_counterexamples.h"


namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_correctness_covers : public graph_with_guessing<T_PC, T_N, T_E, T_PRED>
{
private:
  //mutable map<T_PC, nodece_set_ref<T_PC, T_N, T_E>> m_CE_pernode_that_yield_correct_behaviour_map;
  //mutable map<T_PC, invariant_state_t<T_PC, T_N, T_E, T_PRED>> m_correctness_cover_map;
  //mutable map<T_PC, unordered_set<shared_ptr<T_PRED const>>> m_correctness_cover_preds;
  mutable map<T_PC, expr_ref> m_correctness_condition_map;
  //mutable bool m_inside_decide_hoare_triple_for_correctness_cover = false;
public:

  graph_with_correctness_covers(string const &name, string const& fname, context *ctx/*, memlabel_assertions_t const& memlabel_assertions*/)
    : graph_with_guessing<T_PC, T_N, T_E, T_PRED>(name, fname, ctx/*, memlabel_assertions*/)
  { }

  graph_with_correctness_covers(graph_with_correctness_covers const &other)
    : graph_with_guessing<T_PC, T_N, T_E, T_PRED>(other)//,
      //m_correctness_cover_map(other.m_correctness_cover_map)
  { }

  graph_with_correctness_covers(istream& is, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx) : graph_with_guessing<T_PC, T_N, T_E, T_PRED>(is, name, read_edge_fn, ctx)
  {
    string line;
    bool end;
    do {
      end = !getline(is, line);
      ASSERT(!end);
    } while (line == "");
    ASSERT(line == "=graph_with_correctness_covers begin");
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    end = !getline(is, line);
    ASSERT(!end);

    //string const prefix = "=counterexamples that yield correct behaviour at node ";
    //while (is_line(line, prefix)) {
    //  istringstream iss(line);
    //  string pc_str = line.substr(prefix.length());
    //  T_PC p = T_PC::create_from_string(pc_str);
    //  nodece_set_ref<T_PC, T_N, T_E> nodece_set;
    //  string prefix = "correctness cover at pc " + p.to_string() + " ";
    //  line = nodece_set_t<T_PC, T_N, T_E>::nodece_set_from_stream(is, *this, ctx, prefix, nodece_set);
    //  m_CE_pernode_that_yield_correct_behaviour_map.insert(make_pair(p, nodece_set));
    //}

    //ASSERT(line == "=Correctness covers");
    //end = !getline(is, line);
    //ASSERT(!end);

    //map<T_PC, graph_edge_composition_ref<T_PC, T_E>> graph_ecs;
    //map<T_PC, dshared_ptr<graph_ec_unroll_t<T_PC, T_E>>> graph_ecs;

    //string const prefix2 = "=Correctness cover at node ";
    //while (string_has_prefix(line, prefix2)) {
    //  //string const pattern = ": ec_is_epsilon ";
    //  //size_t colon = line.find(pattern);
    //  //ASSERT(colon != string::npos);
    //  string pc_str = line.substr(prefix2.length());
    //  T_PC p = T_PC::create_from_string(pc_str);
    //  //bool ec_is_epsilon = string_to_bool(line.substr(colon + pattern.length()));
    //  string pc_prefix = "pc " + p.to_string() + " ";
    //  invariant_state_t<T_PC, T_N, T_E, T_PRED> cover;
    //  cover.invariant_state_from_stream(is/*, this->get_start_state()*/, pc_prefix, *this);
    //  //auto epsilon_ec = graph_edge_composition_t<T_PC, T_E>::mk_epsilon_ec();
    //  m_correctness_cover_map.insert(make_pair(p, cover));
    //  end = !getline(is, line);
    //  ASSERT(!end);
    //}

    /*for (auto const& graph_ec : graph_ecs) {
      //ASSERT(!graph_ec.second->is_epsilon());
      ASSERT(graph_ec.second->get_all_pcs().size());
      //T_PC const& from_pc = graph_ec.second->graph_edge_composition_get_from_pc();
      T_PC const& from_pc = graph_ec.second->graph_ec_unroll_get_source_pc().get_pc();
      ASSERT(m_correctness_cover_map.count(from_pc));
      //ASSERT(m_correctness_cover_map.at(from_pc).get_ec()->is_epsilon());
      ASSERT(!m_correctness_cover_map.at(from_pc).get_subgraph()->get_nodes().size());
      m_correctness_cover_map.insert(make_pair(graph_ec.first, m_correctness_cover_map.at(from_pc), graph_ec.second));
    }*/

    if (line != "=graph_with_correctness_covers done") {
      cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
    }
    ASSERT(line == "=graph_with_correctness_covers done");
    //for (auto const& p : this->get_all_pcs()) {
    //  if (!m_correctness_cover_map.count(p)) {
    //    m_correctness_cover_map.insert(make_pair(p, invariant_state_t<T_PC, T_N, T_E, T_PRED>::empty_invariant_state(ctx)));
    //  }
    //}
    //{
    //  bool all_are_absent = true;
    //  bool all_are_present = true;
    //  for (auto const& p : this->get_all_pcs()) {
    //    if (m_correctness_cover_map.count(p)) {
    //      ASSERT(all_are_present);
    //      all_are_absent = false;
    //    } else {
    //      ASSERT(all_are_absent);
    //      all_are_present = false;
    //    }
    //  }
    //  if (all_are_present) {
    //    this->replay_counter_examples();
    //  }
    //}
  }

  virtual ~graph_with_correctness_covers() = default;

  void compute_correctness_covers(T_PC const& p, failcond_t const& correctness_cond);

  proof_status_t covers_post_condition(unordered_set<shared_ptr<T_PRED const>> const& post, T_PC const& from_pc, T_PC const& to_pc, graph_edge_composition_ref<T_PC, T_E> const& pth, shared_ptr<T_PRED const> const& candidate_pred) const;

protected:
  unsigned get_num_correct_nodes() const
  {
    context* ctx = this->get_context();
    unsigned ret = 0;

    //map<T_PC, invariant_state_t<T_PC, T_N, T_E, T_PRED>> const& correctness_covers = this->get_invariant_state_map(reason_for_counterexamples_t::correctness_covers());
    for (auto const& p : this->get_all_pcs()) {
      unordered_set<shared_ptr<T_PRED const>> preds = this->graph_with_guessing_get_invariants_at_pc(p, reason_for_counterexamples_t::correctness_covers());
      if (preds.size() == 0) {
        ret++;
      }
      //if (!m_correctness_cover_preds.count(p)) {
      //  continue;
      //}
      //if (m_correctness_cover_preds.at(p).size() == 0) {
      //  ret++;
      //}
    }
    return ret;
  }

  //map<T_PC, invariant_state_t<T_PC, T_N, T_E, T_PRED>> const& get_correctness_cover_map() const
  //{
  //  return m_correctness_cover_map;
  //}

  //unordered_set<shared_ptr<T_PRED const>> get_correctness_cover_preds(T_PC const& p) const
  //{
  //  if (!m_correctness_cover_preds.count(p)) {
  //    return unordered_set<shared_ptr<T_PRED const>>();
  //  } else {
  //    return m_correctness_cover_preds.at(p);
  //  }
  //}

  //returns true if preds changed at p
  //virtual bool add_CE_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples, nodece_ref<T_PC, T_N, T_E> const &nce) const override;

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override
  {
    this->graph_with_guessing<T_PC, T_N, T_E, T_PRED>::graph_to_stream(os, prefix);
    os << "=graph_with_correctness_covers begin\n";
    //for (auto const& pc_ces : m_CE_pernode_that_yield_correct_behaviour_map) {
    //  os << "=counterexamples that yield correct behaviour at node " << pc_ces.first.to_string() << ":\n";
    //  string prefix = "correctness cover at pc " + pc_ces.first.to_string() + " ";
    //  pc_ces.second->nodece_set_to_stream(os, prefix);
    //}
    //os << "=Correctness covers\n";
    //for (auto const& pi : m_correctness_cover_map) {
    //  os << "=Correctness cover at node " << pi.first.to_string() << "\n";
    //  string prefix = "pc " + pi.first.to_string() + " ";
    //  pi.second.invariant_state_to_stream(os, prefix);
    //}
    os << "=graph_with_correctness_covers done\n";
  }

private:
  unordered_set<shared_ptr<T_PRED const>> graph_with_correctness_covers_eliminate_inductively_provable_preds(T_PC const& p, unordered_set<shared_ptr<T_PRED const>> const& in) const;

  //list<nodece_ref<T_PC, T_N, T_E>> const& get_counterexamples_at_pc_that_yield_correct_behaviour(T_PC const &p) const
  //{
  //  if (!m_CE_pernode_that_yield_correct_behaviour_map.count(p)) {
  //    static list<nodece_ref<T_PC, T_N, T_E>> empty_set;
  //    return empty_set;
  //  }
  //  return m_CE_pernode_that_yield_correct_behaviour_map.at(p)->get_nodeces();
  //}

  //void replay_counter_examples()
  //{
  //  for (auto const& p : this->get_all_pcs()) {
  //    list<nodece_ref<T_PC, T_N, T_E>> const& nces = this->get_counterexamples_at_pc_that_yield_correct_behaviour(p);
  //    for (auto const& nce : nces) {
  //      DYN_DEBUG(replay_counter_examples, cout << __func__ << " " << __LINE__ << ": replaying nce at pc " << p.to_string() << "=\n"; nce->nodece_to_stream(cout, ""));
  //      this->add_point_using_CE_at_pc(p, nce);
  //    }
  //  }
  //}

  //bool add_point_using_CE_at_pc(T_PC const &p, nodece_ref<T_PC, T_N, T_E> const &nce) const override;
};

}
