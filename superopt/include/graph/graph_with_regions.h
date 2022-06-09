#pragma once

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>

#include "graph/graph.h"
#include "graph/graph_bbl.h"
#include "graph/graph_loop.h"
#include "graph/graph_region.h"
#include "graph/dominator_dfa.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E>
class graph_with_regions_t : public graph<T_PC, T_N, T_E>
{
private:
  dshared_ptr<graph_region_node_t<T_PC, T_N, T_E>> m_top_level_region_node;
  mutable map<T_PC, T_PC> m_idominator_relation;
  mutable map<T_PC, T_PC> m_ipostdominator_relation;
//  map<T_PC, set<T_PC>> m_dominance_frontier; // The key-PC doesn't dominate this set of pcs but dominates their predecessor PCs
public:
  graph_with_regions_t() : graph<T_PC, T_N, T_E>()
  { }
  graph_with_regions_t(istream& in);
  void compute_regions();

  void populate_dominator_and_postdominator_relations() const;
  map<T_PC, set<T_PC>> compute_dominance_frontier() const;
//  set<T_PC> get_dominance_frontier_for_pc(T_PC const& p) const
//  {
//    set<T_PC> ret;
//    if(m_dominance_frontier.count(p))
//      ret = m_dominance_frontier.at(p);
//    return ret;
//  }

  T_PC const& get_dominator_for_pc(T_PC const& p) const
  {
    ASSERT(m_idominator_relation.count(p));
    return m_idominator_relation.at(p);
  }
  
  map<T_PC,T_PC> const& get_idominator_relation() const     { return m_idominator_relation; }
  map<T_PC,T_PC> const& get_ipostdominator_relation() const { return m_ipostdominator_relation; }
  
  bool graph_regions_are_consistent() const
  {
    auto all_pcs_from_regions = this->get_all_pcs_from_regions();
    auto all_pcs = this->get_all_pcs();
    DYN_DEBUG2(graph_regions, cout << _FNLN_ << ": all_pcs size " << all_pcs.size() << ", all_pcs_from_regions size " << all_pcs_from_regions.size() << endl);
    for (auto const& p : all_pcs) {
      if (!set_belongs(all_pcs_from_regions, p)) {
        cout << _FNLN_ << ": pc " << p.to_string() << " absent in regions\n";
        cout << _FNLN_ << ": printing graph\n";
        this->graph<T_PC, T_N, T_E>::graph_to_stream(cout);
        cout << _FNLN_ << ": to print the region graph, please use --dyn_debug=graph_regions\n";
        NOT_REACHED();
        return false;
      }
    }
    for (auto const& p : all_pcs_from_regions) {
      if (!set_belongs(all_pcs, p)) {
        cout << _FNLN_ << ": pc " << p.to_string() << " present in regions but absent in graph\n";
        cout << _FNLN_ << ": printing graph\n";
        this->graph<T_PC, T_N, T_E>::graph_to_stream(cout);
        cout << _FNLN_ << ": to print the region graph, please use --dyn_debug=graph_regions\n";
        NOT_REACHED();
        return false;
      }
    }
    return true;
  }
  dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> get_top_level_region_node() const
  {
    return dynamic_pointer_cast<graph_region_node_t<T_PC, T_N, T_E> const>(m_top_level_region_node);
  }
  dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> pc_is_loop_head(T_PC const& p) const;

  dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> graph_with_regions_shared_from_this() const
  {
    dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> ret = dynamic_pointer_cast<graph_with_regions_t<T_PC, T_N, T_E> const>(this->shared_from_this());
    ASSERT(ret);
    return ret;
  }

  template<bool POSTDOM>
  map<T_PC, list<T_PC>> find_dominators() const
  {
    autostop_timer func_timer(__func__);

    map<T_PC, dominator_val_t<T_PC>> domvals;

    if (POSTDOM) {
      for (auto const& p : this->get_exit_pcs()) {
        domvals.insert(make_pair(p, dominator_val_t<T_PC>::singleton(p)));
      }
    } else {
      auto v = dominator_val_t<T_PC>::singleton(this->get_entry_pc());
      domvals.insert(make_pair(this->get_entry_pc(), v));
    }

    find_dominators_dfa_t<T_PC, T_N, T_E, POSTDOM ? dfa_dir_t::backward : dfa_dir_t::forward> dfa(this->shared_from_this(), domvals);
    dfa.solve();

    map<T_PC, list<T_PC>> ret;
    for (auto const& dv : domvals) {
      ret.insert(make_pair(dv.first, dv.second.get_pc_list()));
    }

    return ret;
  /*
    map<T_PC, set<T_PC>> ret;
    set<T_PC> all_nodes;
    for (pair<T_PC, dshared_ptr<T_N>> pc_node: m_nodes) {
      all_nodes.insert(pc_node.first);
    }
    for (pair<T_PC, dshared_ptr<T_N>> pc_node: m_nodes) {
      if (!postdom && pc_node.first.is_start()) {
        ret[pc_node.first].insert(pc_node.first);
      } else if (postdom && pc_node.first.is_exit()) {
        ret[pc_node.first].insert(pc_node.first);
      } else {
        ret[pc_node.first] = all_nodes;
      }
    }
    DYN_DEBUG(graph_regions, cout << _FNLN_ << ": starting fixed point loop\n");

    size_t num_iter = 0;
    bool change = true;
    while (change) {
      change = false;
      DYN_DEBUG(graph_regions, cout << _FNLN_ << ": num_iter = " << num_iter << "\n");
      cout << _FNLN_ << ": num_iter = " << num_iter << "\n";
      for (pair<T_PC, dshared_ptr<T_N>> pc_node: m_nodes) {
        T_PC p = pc_node.first;
        list<dshared_ptr<T_E const>> es;
        if (!postdom) {
          get_edges_incoming(p, es);
        } else {
          get_edges_outgoing(p, es);
        }
        set<T_PC> new_doms;
        for (typename list<dshared_ptr<T_E const>>::const_iterator iter = es.begin(); iter != es.end(); ++iter) {
          dshared_ptr<T_E const> e = *iter;
          if (iter == es.begin()) {
            new_doms = ret[postdom ? e->get_to_pc() : e->get_from_pc()];
          } else {
            set<T_PC> doms_pred = ret[postdom ? e->get_to_pc() : e->get_from_pc()];
            set<T_PC> intersect;
            set_intersect(new_doms, doms_pred, intersect);
            new_doms = intersect;
          }
        }
        new_doms.insert(p);
        set<T_PC> cur_doms = ret[p];
        if (new_doms != cur_doms) {
          ret[p] = new_doms;
          change = true;
        }
      }
      num_iter++;
    }
    DYN_DEBUG(graph_regions, cout << _FNLN_ << ": done fixed point loop\n");

    // print
  //    cout << "dominators\n";
  //    for(typename map<T_PC, set<T_PC>>::const_iterator iter = ret.begin(); iter != ret.end(); ++iter)
  //    {
  //      cout << "pc: " << iter->first.to_string() << ": ";
  //      const set<T_PC>& doms = iter->second;
  //      for(typename set<T_PC>::const_iterator iter = doms.begin(); iter != doms.end(); ++iter)
  //      {
  //        cout << iter->to_string() << ", ";
  //      }
  //      cout << endl;
  //    }
    return ret;
  */
  }

  template<bool POSTDOM>
  void find_idominator(map<T_PC, T_PC>& ret/*, vector<T_PC>& topsorted*/) const
  {
    map<T_PC, list<T_PC>> doms = find_dominators<POSTDOM>();
    for (auto const& pd : doms) {
      ASSERT(pd.second.size() >= 1); //at least P itself should be a part of this list
      ASSERT(pd.second.back() == pd.first);
      if (pd.second.size() < 2) {
        continue;
      }
      auto last_iter = pd.second.rbegin();
      ASSERT(*last_iter == pd.first);
      last_iter++;
      ASSERT(last_iter != pd.second.rend()); //because pd.second.size() >= 2
      ret.insert(make_pair(pd.first, *last_iter));
    }
    //list<T_PC> todo;
    //for (const auto& pc_doms : doms) {
    //  if(pc_doms.second.size() == 1) {
    //    todo.push_back(pc_doms.first);
    //    topsorted.push_back(pc_doms.first);
    //  }
    //}

    //while (todo.size() > 0) {
    //    //cout << "todo: " << list_to_string<T_PC>(todo) << endl;
    //  T_PC curr = *todo.begin();
    //  todo.pop_front();

    //  for (auto& pc_doms : doms) {
    //    set<T_PC> new_doms;
    //    set_difference(pc_doms.second, {curr}, new_doms);
    //    if (new_doms.size() == 1 && pc_doms.second != new_doms) {
    //      assert(*new_doms.begin() == pc_doms.first);
    //      todo.push_back(pc_doms.first);
    //      topsorted.push_back(pc_doms.first);
    //      ret[pc_doms.first] = curr;
    //    }
    //    pc_doms.second = new_doms;
    //  }
    //}
  }

  //vector<T_PC> get_dominator_tree_topsorted_pcs() const;
  dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> edge_is_loop_exit(dshared_ptr<T_E const> const& e) const;
  set<T_PC> get_all_loop_pcs() const;

protected:
  void graph_ssa_copy(graph_with_regions_t const& other)
  {
    graph<T_PC, T_N, T_E>::graph_ssa_copy(other);
    m_top_level_region_node = other.m_top_level_region_node;
  }

  graph_with_regions_t(graph_with_regions_t const& other) : graph<T_PC, T_N, T_E>(other), 
                                                            m_top_level_region_node(other.m_top_level_region_node),
                                                            m_idominator_relation(other.m_idominator_relation),
                                                            m_ipostdominator_relation(other.m_ipostdominator_relation)
  { }

  dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> edge_is_back_edge(dshared_ptr<T_E const> const& e) const;
  dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> get_loop_body_region_for_loop_region(dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> const& loop_region_node) const;
  set<T_PC> loop_region_get_tails(dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> const& loop) const;

private:
  void visit_loop_regions(std::function<void (dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> const&)> f) const;
  void visit_loop_regions_recursive(dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> const& region_node, std::function<void (dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> const&)> f) const;
  dshared_ptr<graph_region_node_t<T_PC, T_N, T_E>> graph_get_instruction_region_node(T_PC const& p) const;
  graph_loop_t<T_PC, T_N, T_E> graph_get_loop_for_back_edge(dshared_ptr<T_E const> const& back_edge) const;
  list<graph_loop_t<T_PC, T_N, T_E>> graph_identify_natural_loops() const;
  list<dshared_ptr<T_E const>> graph_identify_back_edges() const;
  pair<list<dshared_ptr<T_E const>>, list<dshared_ptr<T_E const>>> region_graph_add_internal_edges_and_return_back_edges_and_outgoing_edges(dshared_ptr<graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC,T_N,T_E>, graph_region_edge_t<T_PC,T_N,T_E>>> region_graph, T_PC const& loop_head) const;
  void graph_get_nodes_that_reach_tail_without_going_through_stop_pcs(T_PC const& tail, set<T_PC> const& stop_pcs, set<T_PC>& ret) const;
  set<graph_bbl_t<T_PC,T_N,T_E>> graph_loop_get_bbls(graph_loop_t<T_PC, T_N, T_E> const& l) const;
  void graph_loop_get_bbls_helper(graph_loop_t<T_PC, T_N, T_E> const* l, T_PC const& bbl_head, set<graph_bbl_t<T_PC,T_N,T_E>>& ret) const;
  void print_regions(ostream& os) const;
  dshared_ptr<graph_region_node_t<T_PC, T_N, T_E>> graph_get_bbl_region_node(graph_bbl_t<T_PC,T_N,T_E> const& bbl) const;
  set<graph_bbl_t<T_PC,T_N,T_E>> graph_get_bbls() const;

  set<T_PC>
  get_all_pcs_from_regions() const
  {
    ASSERT(m_top_level_region_node);
    return m_top_level_region_node->graph_region_node_get_all_constituent_pcs();
  }
};

}
