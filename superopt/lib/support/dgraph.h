#pragma once

#include "support/utils.h"
#include "support/debug.h"
#include "support/types.h"

#include <map>
#include <list>
#include <vector>
#include <sstream>
#include <set>
#include <stack>
#include <memory>
#include <utility>

using namespace std;

template<typename T_N>
class dgraph_t
{
  public:
    dgraph_t(set<T_N> const& nodes, set<pair<T_N,T_N>> const& edges)
    {
      for (auto const& n : nodes) {
        m_adj[n] = {};
        //m_adj.insert(make_pair<T_N,vector<T_N>>(n,empty));
      }
      for (auto const& e : edges) {
        T_N const& from = e.first;
        T_N const& to = e.second;
        ASSERT (m_adj.count(from));
        m_adj.at(from).push_back(to);
      }
    }

    size_t size() const { return m_adj.size(); }

    vector<T_N> nodes() const
    {
      vector<T_N> ret;
      ret.reserve(m_adj.size());
      for (auto const& p : m_adj) {
        ret.push_back(p.first);
      }
      return ret;
    }

    vector<T_N> const& get_succ(T_N const& n) const
    {
      auto it = m_adj.find(n);
      if (it != m_adj.end()) {
        return it->second;
      }
      else {
        static vector<T_N> empty;
        return empty;
      }
    }

  private:
    map<T_N, vector<T_N>> m_adj;
};

template<typename T_N>
class scc_t
{
  public:

    scc_t(dgraph_t<T_N> const& g)
      : m_graph(g),
        m_idx(1) // 0 for not visited nodes
    {
      for (auto const& n : g.nodes()) {
        m_index.insert(make_pair(n,0));
        m_lowlinks.insert(make_pair(n,0));
        m_in_stack.insert(make_pair(n,false));
      }
    }

    void compute_scc()
    {
      ASSERT(m_scc_stk.size() == 0);
      for (auto const& p : m_index) {
        T_N const& node = p.first;
        if (m_lowlinks.at(node) == 0) {
          strong_connect(node);
        }
      }
      if (m_scc_stk.size()) {
        cout << "stack not empty: " << m_scc_stk.size() << " elements\n";
        do {
          cout << m_scc_stk.top() << ' ';
          m_scc_stk.pop();
        } while (!m_scc_stk.empty());
      }
      ASSERT(m_scc_stk.size() == 0);
    }

    list<vector<T_N>> get_scc() const
    {
      return m_scc;
    }

  private:

    void strong_connect(T_N const& node)
    {
      m_index.at(node) = m_idx;
      m_lowlinks.at(node) = m_idx;
      m_idx++;

      unsigned this_index = m_index.at(node);
      unsigned &this_lowlink = m_lowlinks.at(node);

      m_scc_stk.push(node);
      m_in_stack.at(node) = true;

      for (auto const& succ : m_graph.get_succ(node)) {
        if (m_lowlinks.at(succ) == 0) {
          strong_connect(succ);
          this_lowlink = min(this_lowlink, m_lowlinks.at(succ));
        } else if (m_in_stack.at(succ)) {
          this_lowlink = min(this_lowlink, m_index.at(succ));
        }
      }

      // no change to lowlink
      if (this_lowlink == this_index) {
        vector<T_N> new_scc;
        while (1) {
          T_N const& succ = m_scc_stk.top();
          m_scc_stk.pop();
          m_in_stack.at(succ) = false;
          new_scc.push_back(succ);
          if (succ == node) {
            break;
          }
        }
        m_scc.push_back(new_scc);
      }
    }

    dgraph_t<T_N> const& m_graph;
    unsigned m_idx;
    map<T_N,unsigned> m_index;
    map<T_N,unsigned> m_lowlinks;
    map<T_N,bool> m_in_stack;
    stack<T_N> m_scc_stk;
    list<vector<T_N>> m_scc;
};
