#pragma once

#include "support/manager.h"
#include "support/mytimer.h"
#include "support/utils.h"
#include "support/debug.h"
#include "support/types.h"
#include "support/serpar_composition.h"
#include "graph/edge_id.h"
#include "gsupport/graph_ec.h"

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC>
class node
{
public:
  node(const T_PC& p) : m_pc(p)
  {
/*#ifndef NDEBUG
    m_num_allocated_node++;
#endif*/
  }
  node(node<T_PC> const &other) : m_pc(other.m_pc)
  {
/*#ifndef NDEBUG
    m_num_allocated_node++;
#endif*/
  }

  virtual ~node()
  {
/*#ifndef NDEBUG
    m_num_allocated_node--;
#endif*/
  }

  const T_PC& get_pc() const
  {
    return m_pc;
  }

  void set_pc(T_PC const &p)
  {
    m_pc = p;
  }

  virtual string to_string() const
  {
    return m_pc.to_string();
  }

  friend ostream& operator<<(ostream& os, const node& n)
  {
    os << n.to_string();
    return os;
  }

/*#ifndef NDEBUG
  static long long get_num_allocated()
  {
    return m_num_allocated_node;
  }
#endif*/

private:
  T_PC m_pc;

/*#ifndef NDEBUG
  static long long m_num_allocated_node;
#endif*/
};

template <typename T_PC>
class edge
{
public:
  edge(const T_PC& pc_from, const T_PC& pc_to) : m_from_pc(pc_from), m_to_pc(pc_to), m_is_back_edge(false)
  {
/*#ifndef NDEBUG
    m_num_allocated_edge++;
#endif*/
  }

  edge(edge<T_PC> const &other) : m_from_pc(other.m_from_pc), m_to_pc(other.m_to_pc), m_is_back_edge(other.m_is_back_edge)
  {
/*#ifndef NDEBUG
    m_num_allocated_edge++;
#endif*/
  }

  virtual ~edge()
  {
/*#ifndef NDEBUG
    m_num_allocated_edge--;
#endif*/
  }

  const T_PC& get_from_pc() const
  {
    return m_from_pc;
  }

  const T_PC& get_to_pc() const
  {
    return m_to_pc;
  }

  edge_id_t<T_PC> get_edge_id() const { return edge_id_t<T_PC>(m_from_pc, m_to_pc); }

  string to_string_for_eq(string const& prefix) const
  {
    stringstream ss;
    ss << prefix << ": " << this->get_from_pc().to_string() << " => " << this->get_to_pc().to_string() << endl;
    return ss.str();
  }

  bool is_back_edge() const
  {
    return m_is_back_edge;
  }

  void set_backedge(bool be) const
  {
    m_is_back_edge = be;
  }

  //void set_from_pc(const T_PC& p)
  //{
  //  m_from_pc = p;
  //}

  //void set_to_pc(const T_PC& p)
  //{
  //  m_to_pc = p;
  //}

  virtual string to_string() const
  {
    stringstream ss;
    ss << m_from_pc.to_string() << "=>" << m_to_pc.to_string();
    return ss.str();
  }

  friend ostream& operator<<(ostream& os, const edge& e)
  {
    os << e.to_string();
    return os;
  }
  bool operator==(edge const& other) const
  {
    return    this->m_from_pc == other.m_from_pc
           && this->m_to_pc == other.m_to_pc
           //&& this->m_is_back_edge == other.m_is_back_edge // mutables should not be used in equality check
    ;
  }

  static void edge_read_pcs_and_comment(const string& pcs, T_PC& p1, T_PC& p2, string &comment)
  {
    //cout << __func__ << " " << __LINE__ << ": pcs = " << pcs << ".\n";
    size_t pos = pcs.find("=>");
    if (pos == string::npos) {
      cout << __func__ << " " << __LINE__ << ": pcs = " << pcs << endl;
      NOT_REACHED();
    }
    string pc1 = pcs.substr(0, pos);
    size_t lbrack = pcs.find("[", pos + 2);
    string pc2;
    if (lbrack == string::npos) {
      pc2 = pcs.substr(pos + 2);
      comment = "";
    } else {
      pc2 = pcs.substr(pos + 2, pos + 2 + lbrack - 1);
      comment = pcs.substr(lbrack);
    }
    p1 = T_PC::create_from_string(pc1);
    p2 = T_PC::create_from_string(pc2);
  }

private:
  T_PC const m_from_pc;
  T_PC const m_to_pc;
  mutable bool m_is_back_edge;

/*#ifndef NDEBUG
  static long long m_num_allocated_edge;
#endif*/
};
}

namespace eqspace {

template <typename T_PC>
class scc
{
public:
  scc(const set<T_PC>& pcs = set<T_PC>()) : m_pcs(pcs) {}

  void add_pc(const T_PC& p)
  {
    m_pcs.insert(p);
  }

  bool is_member(const T_PC& p) const
  {
    return m_pcs.count(p) == 1;
  }

  unsigned size() const
  {
    return m_pcs.size();
  }

  T_PC get_first_pc() const
  {
    return *m_pcs.begin();
  }

  bool is_component_without_backedge() const
  {
    return m_component_without_backedge;
  }

  void set_component_without_backedge(bool val)
  {
    m_component_without_backedge = val;
  }
  
  string to_string()
  {
    stringstream ss;
    for(auto const & p : m_pcs)
      ss << p.to_string() << ", ";
    ss << endl;
    return ss.str();
  }
private:
  set<T_PC> m_pcs;
  bool m_component_without_backedge;
};

template <typename T_PC, typename T_N, typename T_E>
class rgraph_node
{
public:
  rgraph_node(T_PC const &p) : m_pc(p)
  {
  }
  rgraph_node(T_PC const &p, graph<T_PC, T_N, T_E> const &g) : m_pc(p), m_graph(g)
  {
  }

  const T_PC& get_pc() const
  {
    return m_pc;
  }

  graph<T_PC, T_N, T_E> const &get_graph() const
  {
    return m_graph;
  }

  graph<T_PC, T_N, T_E> &get_graph()
  {
    return m_graph;
  }

  /*virtual*/string to_string() const
  {
    return m_pc.to_string();
  }

  friend ostream& operator<<(ostream& os, const rgraph_node& n)
  {
    os << n.to_string();
    return os;
  }

private:
  T_PC m_pc;
  graph<T_PC, T_N, T_E> m_graph;
};

template <typename T_PC, typename T_N, typename T_E>
class rgraph_edge
{
public:
  rgraph_edge(const T_PC& pc_from, const T_PC& pc_to) : m_from_pc(pc_from), m_to_pc(pc_to)
  {
  }

  void add_constituent_edge(shared_ptr<T_E const> e)
  {
    m_constituent_edges.push_back(e);
  }

  list<shared_ptr<T_E const>> const &get_constituent_edges() const
  {
    return m_constituent_edges;
  }

  void set_constituent_edges(list<shared_ptr<T_E const>> const &es)
  {
    m_constituent_edges = es;
  }

  const T_PC& get_from_pc() const
  {
    return m_from_pc;
  }

  const T_PC& get_to_pc() const
  {
    return m_to_pc;
  }

  bool is_back_edge() const
  {
    NOT_REACHED();
  }

  void set_backedge(bool be)
  {
    NOT_REACHED();
  }

  void set_from_pc(const T_PC& p)
  {
    m_from_pc = p;
  }

  void set_to_pc(const T_PC& p)
  {
    m_to_pc = p;
  }

  /*virtual*/string to_string() const
  {
    stringstream ss;
    ss << m_from_pc.to_string() << "=>" << m_to_pc.to_string();
    return ss.str();
  }

  friend ostream& operator<<(ostream& os, const rgraph_edge& e)
  {
    os << e.to_string();
    return os;
  }
  static bool constituent_edges_are_contained(list<shared_ptr<T_E const>> const& needle, list<shared_ptr<T_E const>> const& haystack)
  {
    for (auto const& e : needle) {
      bool found = false;
      for (auto const& oe : haystack) {
        if (e == oe) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
    return true;
  }

  /*bool operator==(rgraph_edge<T_PC, T_N, T_E> const& other) const
  {
    if (this->m_from_pc != other.m_from_pc) {
      return false;
    }
    if (this->m_to_pc != other.m_to_pc) {
      return false;
    }
    if (!constituent_edges_are_contained(this->m_constituent_edges, other.m_constituent_edges)) {
      return false;
    }
    if (!constituent_edges_are_contained(other.m_constituent_edges, this->m_constituent_edges)) {
      return false;
    }
    return true;
  }*/

private:
  T_PC m_from_pc;
  T_PC m_to_pc;
  list<shared_ptr<T_E const>> m_constituent_edges;
};

template<typename T_E> class mutex_paths_iterator_object_t;

template<typename T_E>
using mutex_paths_iterator_t = shared_ptr<mutex_paths_iterator_object_t<T_E>>;

template<typename T_E>
mutex_paths_iterator_t<T_E> mutex_paths_iter_begin(list<list<shared_ptr<T_E const>>> const &paths);
//template<typename T_E>
//mutex_paths_iterator_t<T_E> mutex_paths_iter_end(list<list<shared_ptr<T_E>>> const &paths);
template<typename T_E>
mutex_paths_iterator_t<T_E> mutex_paths_iter_next(mutex_paths_iterator_t<T_E> const &iter);


template <typename T_PC, typename T_N, typename T_E>
class graph
{
public:
  typedef multimap<T_PC, shared_ptr<T_E const>> multimap_edges;
  typedef typename multimap<T_PC, shared_ptr<T_E const>>::const_iterator multimap_const_iterator;
  typedef typename multimap<T_PC, shared_ptr<T_E const>>::iterator multimap_iterator;

  class graph_edge_sort_fn_t {
  public:
    graph_edge_sort_fn_t(graph<T_PC, T_N, T_E> const &g, std::function<bool (T_PC const&, T_PC const&)> cmpF) : m_graph(g), m_sorted_pcs(sort_pcs(g.topological_sorted_labels(), cmpF))
    {
      //cout << __func__ << " " << __LINE__ << ": printing topsorted labels\n";
      //for (auto const& p : m_topsorted_pcs) {
      //  cout << p.to_string() << endl;
      //}
    }
    bool operator()(shared_ptr<T_E const> a, shared_ptr<T_E const> b) {
      //cout << __func__ << " " << __LINE__ << ": a = " << a->to_string() << endl;
      //cout << __func__ << " " << __LINE__ << ": b = " << b->to_string() << endl;
      T_PC const &a_from_pc = a->get_from_pc();
      T_PC const &b_from_pc = b->get_from_pc();
      if (is_less(a_from_pc, b_from_pc)) {
        //cout << __func__ << " " << __LINE__ << ": returning true\n";
        return true;
      }
      if (is_less(b_from_pc, a_from_pc)) {
        //cout << __func__ << " " << __LINE__ << ": returning false\n";
        return false;
      }
      //a_from_pc == b_from_pc at this point
      T_PC const &a_to_pc = a->get_to_pc();
      T_PC const &b_to_pc = b->get_to_pc();
      T_PC const &a_from_pc_next = m_graph.get_successor_pc(a_from_pc);
      T_PC const &b_from_pc_next = m_graph.get_successor_pc(b_from_pc);
      //we want to keep the edge pointing to fall-through pc (from_pc_next) at the very end.
      if (b_to_pc == b_from_pc_next) {
        //cout << __func__ << " " << __LINE__ << ": returning true\n";
        return true;
      }
      if (a_to_pc == a_from_pc_next) {
        //cout << __func__ << " " << __LINE__ << ": returning false\n";
        return false;
      }
      bool ret = is_less(a_to_pc, b_to_pc);
      //cout << __func__ << " " << __LINE__ << ": returning " << ret << "\n";
      return ret;
    }
  private:
    bool is_less(T_PC const& a, T_PC const& b) {
      if (a == b) {
        return false;
      }
      for (auto const& p : m_sorted_pcs) {
        if (a == p) {
          return true;
        }
        if (b == p) {
          return false;
        }
      }
      cout << __func__ << " " << __LINE__ << ": a = " << a.to_string() << endl;
      cout << __func__ << " " << __LINE__ << ": b = " << b.to_string() << endl;
      cout << __func__ << " " << __LINE__ << ": sorted_pcs =";
      for (auto const& p : m_sorted_pcs) {
        cout << " " << p.to_string();
      }
      cout << endl;
      NOT_REACHED();
    }
  private:
    static list<T_PC> sort_pcs(list<T_PC> const& topsorted_pcs, std::function<bool (T_PC const&, T_PC const&)> cmpF)
    {
      list<T_PC> ret = topsorted_pcs;
      //cout << __func__ << " " << __LINE__ << ": topsorted_pcs =";
      //for (auto const& p : topsorted_pcs) {
      //  cout << " " << p.to_string();
      //}
      //cout << endl;
      std::function<bool (T_PC const&, T_PC const&)> f = [&topsorted_pcs, cmpF](T_PC const& a, T_PC const& b)
      {
        //cout << __func__ << " " << __LINE__ << ": a = " << a.to_string() << ", b = " << b.to_string() << endl;
        if (a == b) {
          //cout << __func__ << " " << __LINE__ << ": returning false\n";
          return false;
        }
        if (a.is_start()) {
          //cout << __func__ << " " << __LINE__ << ": returning true\n";
          return true;
        }
        if (b.is_start()) {
          //cout << __func__ << " " << __LINE__ << ": returning false\n";
          return false;
        }
        bool is_less = cmpF(a, b);
        bool is_greater = cmpF(b, a);
        if (is_less) {
          //cout << __func__ << " " << __LINE__ << ": returning true\n";
          return true;
        }
        if (is_greater) {
          //cout << __func__ << " " << __LINE__ << ": returning false\n";
          return false;
        }
        for (auto const& p : topsorted_pcs) {
          if (p == a) {
            //cout << __func__ << " " << __LINE__ << ": returning true\n";
            return true;
          }
          if (p == b) {
            //cout << __func__ << " " << __LINE__ << ": returning false\n";
            return false;
          }
        }
        NOT_REACHED();
      };
      ret.sort(f);
      //cout << __func__ << " " << __LINE__ << ": sorted pcs =\n";
      //for (auto p : ret) {
      //  cout << p.to_string() << "\n";
      //}
      return ret;
    }
  private:
    graph<T_PC, T_N, T_E> const &m_graph;
    list<T_PC> m_sorted_pcs;
  };

  graph()
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph(graph const &other)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    map<shared_ptr<T_E const>, shared_ptr<T_E const>> edge_rename_map;

    for (const auto &n : other.m_nodes) {
      shared_ptr<T_N> nn = make_shared<T_N>(*n.second);
      m_nodes.insert(make_pair(n.first, nn));
    }
    for (const auto &e : other.m_edges) {
      shared_ptr<T_E const> ee = e;
      edge_rename_map.insert(make_pair(e, ee));
      m_edges.push_back(ee);
    }
    for (const auto &mo : other.m_edges_lookup_outgoing) {
      m_edges_lookup_outgoing.insert(make_pair(mo.first, edge_rename_map.at(mo.second)));
    }
    for (const auto &mi : other.m_edges_lookup_incoming) {
      m_edges_lookup_incoming.insert(make_pair(mi.first, edge_rename_map.at(mi.second)));
    }
    // due to tfg collapsing, re populating instead of copying
    populate_transitive_closure();
    populate_reachable_and_unreachable_incoming_edges_map();
//    m_transitive_closure = other.m_transitive_closure;
//    for (const auto &mir : other.m_incoming_reachable_edges_map) {
//      list<shared_ptr<T_E const>> nls;
//      for (const auto &mir_e : mir.second) {
//        nls.push_back(edge_rename_map.at(mir_e));
//      }
//      m_incoming_reachable_edges_map.insert(make_pair(mir.first, nls));
//    }
//    for (const auto &miu : other.m_incoming_unreachable_edges_map) {
//      list<shared_ptr<T_E const>> nls;
//      for (const auto &miu_e : miu.second) {
//        nls.push_back(edge_rename_map.at(miu_e));
//      }
//      m_incoming_unreachable_edges_map.insert(make_pair(miu.first, nls));
//    }
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph(istream& is)
  {
    string line;
    bool end;

    do {
      end = !getline(is, line);
      ASSERT(!end);
    } while (line != "=graph done");
  }

  unsigned get_node_count() const
  {
    return m_nodes.size();
  }

  unsigned get_edge_count() const
  {
    return m_edges.size();
  }

  void add_node(shared_ptr<T_N> n)
  {
    assert(n);
    assert(m_nodes.count(n->get_pc()) == 0);
    m_nodes[n->get_pc()] = n;
  }

  void add_edge(shared_ptr<T_E const> e)
  {
    ASSERT(e);
    m_edges.push_back(e);
    //cout << __func__ << " " << __LINE__ << ": e->get_from_pc() = " << e->get_from_pc().to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": e->get_to_pc() = " << e->get_to_pc().to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << endl;
    m_edges_lookup_outgoing.insert(make_pair(e->get_from_pc(), e));
    m_edges_lookup_incoming.insert(make_pair(e->get_to_pc(), e));
    //{//assertcheck
    //  list<shared_ptr<T_E>> incoming;
    //  this->get_edges_incoming(e->get_to_pc(), incoming);
    //  cout << __func__ << " " << __LINE__ << ": e->get_to_pc() = " << e->get_to_pc().to_string() << ": incoming.size() = " << incoming.size() << endl;
    //  incoming.clear();
    //  this->get_edges_incoming(e->get_from_pc(), incoming);
    //  cout << __func__ << " " << __LINE__ << ": e->get_from_pc() = " << e->get_from_pc().to_string() << ": incoming.size() = " << incoming.size() << endl;
    //  incoming.clear();
    //  this->get_edges_incoming(e->get_to_pc(), incoming);
    //  cout << __func__ << " " << __LINE__ << ": e->get_to_pc() = " << e->get_to_pc().to_string() << ": incoming.size() = " << incoming.size() << endl;
    //  cout << __func__ << " " << __LINE__ << ":\n" << this->incoming_sizes_to_string() << endl;
    //}
  }

  shared_ptr<T_N> find_node(T_PC const &p) const
  {
    if (m_nodes.count(p) > 0) {
      return m_nodes.at(p);
    } else {
      return nullptr;
    }
  }

  shared_ptr<T_N> find_entry_node() const
  {
    for (const auto& p_n : m_nodes) {
      if (p_n.first.is_start()) {
        return p_n.second;
      }
    }
    return nullptr;
  }

  T_PC const& get_entry_pc() const
  {
    auto entry_node = find_entry_node();
    ASSERT(entry_node != nullptr);
    return entry_node->get_pc();
  }

  //T_PC const& get_entry_pc() const
  //{
  //  auto entry_node = find_entry_node();
  //  ASSERT(entry_node != nullptr);
  //  return entry_node->get_pc();
  //}

  void get_exit_pcs(list<T_PC>& pcs) const
  {
    for(const auto& p_n : m_nodes)
    {
      if(p_n.first.is_exit())
      {
        pcs.push_back(p_n.first);
      }
    }
  }

  shared_ptr<T_E const> get_edge(const T_PC& from, const T_PC& to) const
  {
    list<shared_ptr<T_E const>> out;
    get_edges_outgoing(from, out);
    typename list<shared_ptr<T_E const>>::const_iterator iter;
    for(iter = out.begin(); iter != out.end(); ++iter)
    {
      shared_ptr<T_E const> e = *iter;
      if(e->get_to_pc() == to)
        return e;
    }
    return 0;
  }

  void get_edges_outgoing(const T_PC& p, list<shared_ptr<T_E const>>& out) const
  {
    //for (auto const& e : m_edges) {
    //  if (e->get_from_pc() == p) {
    //    out.push_back(e);
    //  }
    //}
    //cout << __func__ << " " << __LINE__ << ": calling get_edges_outgoing...\n";
    return get_edges_helper(p, m_edges_lookup_outgoing, out);
  }

  void get_edges_incoming(const T_PC& p, list<shared_ptr<T_E const>>& out) const
  {
    //list<shared_ptr<T_E const>> out_tmp;
    //for (auto const& e : m_edges) {
    //  if (e->get_to_pc() == p) {
    //    out_tmp.push_back(e);
    //  }
    //}
    //cout << __func__ << " " << __LINE__ << ": calling get_edges_incoming for " << p.to_string() << "\n";
    get_edges_helper(p, m_edges_lookup_incoming, out);
    //ASSERT(out_tmp == out);
    //cout << __func__ << " " << __LINE__ << ": out.size() = " << out.size() << " for " << p.to_string() << "\n";
  }

  void get_edges_exiting(list<shared_ptr<T_E const>>& out) const
  {
    out.clear();
    for (auto const& e : m_edges) {
      if (   e->get_to_pc().is_start()
          || e->get_to_pc().is_exit()) {
        out.push_back(e);
      }
    }
  }

  void remove_node(const T_PC& p)
  {
    remove_node(find_node(p));
  }

  void remove_node(shared_ptr<T_N> const &n)
  {
    list<shared_ptr<T_E const>> out;
    get_edges_outgoing(n->get_pc(), out);
    assert(!out.size());
    list<shared_ptr<T_E const>> in;
    get_edges_incoming(n->get_pc(), in);
    assert(!in.size());
    m_nodes.erase(n->get_pc());
  }

  void remove_edge(shared_ptr<T_E const> const &e)
  {
    ASSERT(get_edge(e->get_from_pc(), e->get_to_pc()));
    for(typename list<shared_ptr<T_E const>>::iterator iter = m_edges.begin(); iter != m_edges.end(); ++iter)
    {
      if(*iter == e) {
        m_edges.erase(iter);
        break;
      }
    }

    for(typename multimap_edges::iterator iter = m_edges_lookup_outgoing.begin(); iter != m_edges_lookup_outgoing.end(); )
    {
      if(iter->second == e)
      {
        typename multimap_edges::iterator del_iter = iter;
        ++iter;
        m_edges_lookup_outgoing.erase(del_iter);
      }
      else ++iter;
    }

    for(typename multimap_edges::iterator iter = m_edges_lookup_incoming.begin(); iter != m_edges_lookup_incoming.end(); )
    {
      if(iter->second == e)
      {
        typename multimap_edges::iterator del_iter = iter;
        ++iter;
        m_edges_lookup_incoming.erase(del_iter);
      }
      else ++iter;
    }
  }

  void remove_edges(const list<shared_ptr<T_E const>>& es)
  {
    for(const auto& e : es)
      remove_edge(e);
  }

  typedef list<shared_ptr<T_E const>> T_PATH;

  bool path_contains_function_call(T_PATH const &path) const
  {
    for (auto p : path) {
      if (p->contains_function_call()) {
        return true;
      }
    }
    return false;
  }

  /*
  class maximal_matching_iterator_t {
  public:
    enum present_status_t
    {
      present,
      not_present,
    };
    //map<T_PC, present_status_t> m_status;
    list<pair<T_PC, present_status_t>> m_status;
    set<T_PC> begin_pcs;
    list<T_PC> completed_pcs;
    bool m_is_end;

    maximal_matching_iterator_t() : m_is_end(false) {
      begin_pcs.clear();
      completed_pcs.clear();
    }
    //bool operator==(maximal_matching_iterator_t const &other) const
    //{
    //  //cout << __func__ << " " << __LINE__ << ": m_is_end = " << m_is_end << ", other.m_is_end = " << other.m_is_end << endl;
    //  return m_is_end == other.m_is_end; //equality defined only for end/non-end bit
    //}
    string to_string() const
    {
      stringstream ss;
      ss << "status:";
      for (const auto &s : m_status) {
        ss << "  " << s.first.to_string() << ", " << ((s.second == present) ? "present" : "not_present")  << endl;
      }
      ss << "is_end: " << m_is_end;
      return ss.str();
    }
    bool is_end() const { return m_is_end; }
    bool is_present_status_for_pc(T_PC const &p) const
    {
      for (auto ps : m_status) {
        if (ps.first == p && ps.second == present) {
          return true;
        }
      }
      return false;
    }

  };
*/
  void set_edges(list<shared_ptr<T_E const>> const& es)
  {
    m_edges.clear();
    m_edges_lookup_outgoing.clear();
    m_edges_lookup_incoming.clear();
    for (auto const& e : es) {
      this->add_edge(e);
    }
  }

  void get_edges(list<shared_ptr<T_E const>>& es) const
  {
    es = m_edges;
  }

  list<shared_ptr<T_E const>> const &get_edges_ordered() const
  {
    return m_edges;
  }

  set<shared_ptr<T_E const>> get_edges() const
  {
    set<shared_ptr<T_E const>> ret;
    for (auto e : m_edges) {
      ret.insert(e);
    }
    return ret;
  }

  size_t get_num_nodes() const
  {
    return m_nodes.size();
  }

  void get_nodes(list<shared_ptr<T_N>>& ns) const
  {
    for (const auto& p_n : m_nodes) {
      ns.push_back(p_n.second);
    }
  }

  set<T_PC> get_all_pcs() const
  {
    set<T_PC> ret;
    for (const auto & p_n : m_nodes) {
      //cout << __func__ << " " << __LINE__ << ": p_n.first = " << p_n.first.to_string() << endl;
      //cout << __func__ << " " << __LINE__ << ": p_n.second = " << p_n.second->get_pc().to_string() << endl;
      ret.insert(p_n.second->get_pc());
    }
    return ret;
  }

  T_PC const &get_successor_pc(T_PC const &p) const
  {
    ASSERT(p.is_label());
    auto iter = m_nodes.find(p);
    ASSERT(iter != m_nodes.end());
    iter++;
    ASSERT(iter != m_nodes.end()); //we expect exit pcs after label pcs
    return iter->first;
  }

  bool is_empty_nodes() const
  {
    return m_nodes.empty();
  }

  string to_string() const
  {
    stringstream ss;
    for(const auto& pc_node : m_nodes)
      ss << "NODE: " << *pc_node.second << "\n";

    for(const auto edge : m_edges)
      ss << "EDGE: " << edge->to_string() << "\n";

    return ss.str();
  }

  void populate_backedges() const
  {
    //cout << __func__ << ": populating back edges\n";
    auto f_back_edge = [] (shared_ptr<T_E const> e)
    {
      //cout << __func__ << ": setting " << e->to_string() << " as backedge\n";
      e->set_backedge(true);
    };

    auto nop = [](shared_ptr<T_E const>){};
    dfs(f_back_edge, nop);
  }

  bool contains_backedge()
  {
    populate_backedges();
    for(auto& e : m_edges)
    {
      if(e->is_back_edge())
        return true;
    }
    return false;
  }

  void get_sccs(list<scc<T_PC>>& sccs) const
  {
    map<T_PC, bool> visited;

    // mark visited false
    for(auto& n : m_nodes)
      visited[n.first] = false;

    stack<T_PC> st;
    // first dfs, forward
    get_sccs_forward_dfs(find_entry_node()->get_pc(), visited, st);

    // mark visited false
    for(auto& n : m_nodes)
      visited[n.first] = false;

    while(!st.empty())
    {
      T_PC p = st.top(); st.pop();

      if(!visited[p])
      {
        scc<T_PC> s;
        get_sccs_reverse_dfs(p, visited, s);
        sccs.push_back(s);
      }
    }

    for(scc<T_PC>& s : sccs)
    {
      if(s.size() > 1)
        s.set_component_without_backedge(false);
      else
      {
        T_PC p = s.get_first_pc();
        auto back_edge = get_edge(p, p);
        s.set_component_without_backedge(!back_edge);
      }
    }
  }

  //This function returns PCs in topsort order; there is another property that we depend on: for PC FROM that has only one outgoing edge FROM->TO, FROM and TO are present consecutively in the returned list.
  list<T_PC> topological_sorted_labels() const
  {
    populate_backedges();
    map<T_PC, unsigned> in_count;
    for (auto& e : m_edges) {
      if (e->is_back_edge()) {
        continue;
      }
      if (in_count.count(e->get_to_pc()) == 0) {
        in_count[e->get_to_pc()] = 1;
      } else {
        in_count[e->get_to_pc()]++;
      }
    }
    set<T_PC> const& all_pcs = this->get_all_pcs();

    list<T_PC> todo;
    todo.push_back(find_entry_node()->get_pc());

    list<T_PC> ret;
    while (ret.size() < all_pcs.size()) {
      while (todo.size() > 0) {
        T_PC cur = *todo.rbegin();
        todo.pop_back();
        ret.push_back(cur);

        list<shared_ptr<T_E const>> es;
        get_edges_outgoing_without_backedges(cur, es);
        for (auto &e : es) {
          T_PC next = e->get_to_pc();
          in_count[next]--;
          if (in_count[next] == 0) {
            todo.push_back(next);
          }
        }
      }
      for (auto const& p : all_pcs) {
        if (!list_contains(ret, p)) {
          todo.push_back(p);
          break;
        }
      }
    }
    return ret;
  }

  //list<T_PC> topsort_increasing(set<T_PC> const &in)
  //{
  //  list<T_PC> all_pcs = topological_sorted_labels();
  //  list<T_PC> ret;
  //  for (const auto &p : all_pcs) {
  //    //bool found = (std::find(in.begin(), in.end(), p) != in.end());
  //    if (set_belongs(in, p)) {
  //      //cout << __func__ << " " << __LINE__ << ": pushing back to ret" << endl;
  //      ret.push_back(p);
  //      //cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl;
  //    }
  //  }
  //  //cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl;
  //  if (ret.size() != in.size()) {
  //    cout << "ret.size() = " << ret.size() << endl;
  //    cout << "in.size() = " << in.size() << endl;
  //    for (auto p : in) {
  //      cout << "in p = " << p.to_string() << endl;
  //    }
  //    for (auto p : ret) {
  //      cout << "ret p = " << p.to_string() << endl;
  //    }
  //  }
  //  ASSERT(ret.size() == in.size());
  //  return ret;
  //}

  //list<T_PC> topsort_decreasing(set<T_PC> const &in)
  //{
  //  list<T_PC> all_pcs = topological_sorted_labels();
  //  list<T_PC> ret;
  //  for (typename list<T_PC>::const_reverse_iterator iter = all_pcs.rbegin();
  //       iter != all_pcs.rend();
  //       iter++) {
  //    T_PC const &p = *iter;
  //    //bool found = (std::find(in.begin(), in.end(), p) != in.end());
  //    if (set_belongs(in, p)) {
  //      ret.push_back(p);
  //      //cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl;
  //    }
  //  }
  //  //cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl;
  //  if (ret.size() != in.size()) {
  //    cout << "ret.size() = " << ret.size() << endl;
  //    cout << "in.size() = " << in.size() << endl;
  //    for (auto p : in) {
  //      cout << "in p = " << p.to_string() << endl;
  //    }
  //    for (auto p : ret) {
  //      cout << "ret p = " << p.to_string() << endl;
  //    }
  //  }

  //  ASSERT(ret.size() == in.size());
  //  return ret;
  //}

  void find_dominators(map<T_PC, set<T_PC>>& ret)
  {
    set<T_PC> all_nodes;
    for(pair<T_PC, shared_ptr<T_N>> pc_node: m_nodes)
      all_nodes.insert(pc_node.first);

    for(pair<T_PC, shared_ptr<T_N>> pc_node: m_nodes)
    {
      if(pc_node.first.is_start())
        ret[pc_node.first].insert(pc_node.first);
      else
        ret[pc_node.first] = all_nodes;
    }

    bool change = true;
    while(change)
    {
      change = false;
      for(pair<T_PC, shared_ptr<T_N>> pc_node: m_nodes)
      {
        T_PC p = pc_node.first;
        list<shared_ptr<T_E const>> es;
        get_edges_incoming(p, es);
        set<T_PC> new_doms;
        for(typename list<shared_ptr<T_E const>>::const_iterator iter = es.begin(); iter != es.end(); ++iter)
        {
          shared_ptr<T_E const> e = *iter;
          if(iter == es.begin())
            new_doms = ret[e->get_from_pc()];
          else
          {
            set<T_PC> doms_pred = ret[e->get_from_pc()];
            set<T_PC> intersect;
            set_intersect(new_doms, doms_pred, intersect);
            new_doms = intersect;
          }
        }
        new_doms.insert(p);
        set<T_PC> cur_doms = ret[p];
        if(new_doms != cur_doms)
        {
          ret[p] = new_doms;
          change = true;
        }
      }
    }

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
  }

  void find_idominator(map<T_PC, T_PC>& ret, vector<T_PC>& topsorted)
  {
    map<T_PC, set<T_PC>> doms;
    find_dominators(doms);
    list<T_PC> todo;
    for(const auto& pc_doms : doms)
    {
      if(pc_doms.second.size() == 1)
      {
        todo.push_back(pc_doms.first);
        topsorted.push_back(pc_doms.first);
      }
    }

    while(todo.size() > 0)
    {
//      cout << "todo: " << list_to_string<T_PC>(todo) << endl;
      T_PC curr = *todo.begin();
      todo.pop_front();

      for(auto& pc_doms : doms)
      {
        set<T_PC> new_doms;
        set_difference(pc_doms.second, {curr}, new_doms);
        if(new_doms.size() == 1 && pc_doms.second != new_doms)
        {
          assert(*new_doms.begin() == pc_doms.first);
          todo.push_back(pc_doms.first);
          topsorted.push_back(pc_doms.first);
          ret[pc_doms.first] = curr;
        }
        pc_doms.second = new_doms;
      }
    }

    // print
//    cout << "immediate dominator map:\n";
//    for(auto e : ret)
//      cout << "idom(" << e.first << ") = " << e.second << "\n";
  }

  vector<T_PC> get_dominator_tree_topsorted_pcs()
  {
    vector<T_PC> topsorted;
    map<T_PC, T_PC> idom;
    find_idominator(idom, topsorted);
    return topsorted;
  }

  graph<T_PC, T_N, T_E> make_reducible(bbl_index_t start_pc_for_new_nodes, set<T_PC> const &do_not_copy_pcs) const
  {
    CPP_DBG_EXEC(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": before, cfg =\n" << this->to_string() << endl);
    graph<T_PC, rgraph_node<T_PC, T_N, T_E>, rgraph_edge<T_PC, T_N, T_E>> rgraph;
    list<shared_ptr<T_N>> ns;
    this->get_nodes(ns);
    for (auto n : ns) {
      shared_ptr<rgraph_node<T_PC, T_N, T_E>> gn = make_shared<rgraph_node<T_PC, T_N, T_E>>(n->get_pc());
      graph<T_PC, T_N, T_E> &gn_graph = gn->get_graph();
      gn_graph.add_node(n);
      rgraph.add_node(gn);
    }
    list<shared_ptr<T_E const>> es;
    this->get_edges(es);
    for (auto e : es) {
      shared_ptr<rgraph_edge<T_PC, T_N, T_E>> ge = make_shared<rgraph_edge<T_PC, T_N, T_E>>(e->get_from_pc(), e->get_to_pc());
      ge->add_constituent_edge(e);
      rgraph.add_edge(ge);
    }
    CPP_DBG_EXEC(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": rgraph =\n" << rgraph.to_string() << endl);
    rgraph_reduce(rgraph, start_pc_for_new_nodes, do_not_copy_pcs);
    CPP_DBG_EXEC(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": rgraph =\n" << rgraph.to_string() << endl);
    ASSERT(rgraph.get_num_nodes() == 1);
    list<shared_ptr<rgraph_node<T_PC, T_N, T_E>>> gns;
    rgraph.get_nodes(gns);
    ASSERT(gns.size() == 1);
    graph<T_PC, T_N, T_E> const &ret = (*gns.begin())->get_graph();
    CPP_DBG_EXEC(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": returning, cfg =\n" << ret.to_string() << endl);
    return ret;
  }

  static void rgraph_reduce(graph<T_PC, rgraph_node<T_PC, T_N, T_E>, rgraph_edge<T_PC, T_N, T_E>> &rgraph, bbl_index_t start_pc_for_new_nodes, set<T_PC> const &do_not_copy_pcs)
  {
    //cout << __func__ << " " << __LINE__ << ": rgraph =\n" << rgraph_to_string(rgraph) << endl;
    bbl_index_t cur_pc_for_new_nodes = start_pc_for_new_nodes;
    bool done;
    do {
      done = false;
      //this terminology of T1T2/T3 moves is taken from the dragon book: code optimization chapter, section on "Node Splitting", example 10.36
      while (rgraph_make_T1T2_move(rgraph, do_not_copy_pcs))
      {
        CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": after T1, T2 move: rgraph =\n" << rgraph_to_string(rgraph) << endl);
      }
      if (rgraph.get_num_nodes() == 1) {
        done = true;
      } else {
        cur_pc_for_new_nodes = rgraph_make_T3_move(rgraph, cur_pc_for_new_nodes, do_not_copy_pcs);
      }
      CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": after T3 move: rgraph =\n" << rgraph_to_string(rgraph) << endl);
    } while (!done);
  }

  static bool rgraph_make_T1T2_move(graph<T_PC, rgraph_node<T_PC, T_N, T_E>, rgraph_edge<T_PC, T_N, T_E>> &rgraph, set<T_PC> const &do_not_copy_pcs)
  {
    list<shared_ptr<rgraph_node<T_PC, T_N, T_E>>> ns;
    rgraph.get_nodes(ns);
    for (auto n : ns) {
      list<shared_ptr<rgraph_edge<T_PC, T_N, T_E> const>> incoming;
      rgraph.get_edges_incoming(n->get_pc(), incoming);
      if (   incoming.size() == 1
          && !((*incoming.begin())->get_from_pc() == n->get_pc())) {
        auto i = *incoming.begin();
        CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": collapsing " << i->to_string() << endl);
        rgraph_collapse_edge_and_node(rgraph, i, n);
        rgraph.remove_node(n);
        return true;
      }
      if (set_belongs(do_not_copy_pcs, n->get_pc())) {
        for (auto e : incoming) {
          CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": collapsing " << e->to_string() << endl);
          rgraph_collapse_edge_and_node(rgraph, e, n);
        }
        rgraph.remove_node(n);
        return true;
      }
      for (auto e : incoming) {
        if (e->get_from_pc() == n->get_pc()) {
          CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": collapsing " << e->to_string() << endl);
          rgraph_collapse_self_edge(rgraph, e);
          return true;
        }
      }
    }
    return false;
  }

  static void rgraph_collapse_edge_and_node(graph<T_PC, rgraph_node<T_PC, T_N, T_E>, rgraph_edge<T_PC, T_N, T_E>> &rgraph, shared_ptr<rgraph_edge<T_PC, T_N, T_E> const> e, shared_ptr<rgraph_node<T_PC, T_N, T_E>> n)
  {
    T_PC const &from_pc = e->get_from_pc();
    shared_ptr<rgraph_node<T_PC, T_N, T_E>> from_node = rgraph.find_node(from_pc);
    ASSERT(from_node);
    graph<T_PC, T_N, T_E> &from_node_graph = from_node->get_graph();
    graph<T_PC, T_N, T_E> const &n_graph = n->get_graph();
    list<shared_ptr<T_N>> n_constituent_nodes;
    n_graph.get_nodes(n_constituent_nodes);
    for (auto nc : n_constituent_nodes) {
      if (!from_node_graph.find_node(nc->get_pc())) {
        from_node_graph.add_node(nc);
      }
    }
    list<shared_ptr<T_E const>> n_constituent_edges;
    n_graph.get_edges(n_constituent_edges);
    for (auto ne : n_constituent_edges) {
      if (!from_node_graph.find_edge(ne->get_from_pc(), ne->get_to_pc())) {
        from_node_graph.add_edge(ne);
      }
    }
    for (auto ce : e->get_constituent_edges()) {
      if (!from_node_graph.find_edge(ce->get_from_pc(), ce->get_to_pc())) {
        from_node_graph.add_edge(ce);
      }
    }
    list<shared_ptr<rgraph_edge<T_PC, T_N, T_E> const>> outgoing;
    rgraph.get_edges_outgoing(n->get_pc(), outgoing);
    for (auto const& o : outgoing) {
      rgraph.remove_edge(o);
      shared_ptr<rgraph_edge<T_PC, T_N, T_E> const> found_matching_edge = rgraph.find_edge(from_pc, o->get_to_pc());
      if (!found_matching_edge) {
        shared_ptr<rgraph_edge<T_PC, T_N, T_E>> new_o = make_shared<rgraph_edge<T_PC, T_N, T_E>>(*o);
        new_o->set_from_pc(from_pc);
        rgraph.add_edge(new_o);
      } else {
        CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": removing matching edge: " << found_matching_edge->to_string() << endl);
        rgraph.remove_edge(found_matching_edge);
        shared_ptr<rgraph_edge<T_PC, T_N, T_E>> new_matching_edge = make_shared<rgraph_edge<T_PC, T_N, T_E>>(*found_matching_edge);
        CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": new_matching_edge.get_constituent_edges().size() = " << new_matching_edge->get_constituent_edges().size() << endl);
        for (auto ce : o->get_constituent_edges()) {
          CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": adding constituent edge to new edge: " << ce->to_string() << endl);
          new_matching_edge->add_constituent_edge(ce);
        }
        CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": new_matching_edge.get_constituent_edges().size() = " << new_matching_edge->get_constituent_edges().size() << endl);
        CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": adding new edge in place of matching edge: " << found_matching_edge->to_string() << endl);
        rgraph.add_edge(new_matching_edge);
        CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": new_matching_edge.get_constituent_edges().size() = " << new_matching_edge->get_constituent_edges().size() << endl);
        CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": rgraph =\n" << rgraph_to_string(rgraph) << endl);
      }
    }
    rgraph.remove_edge(e);
    CPP_DBG_EXEC2(CFG_MAKE_REDUCIBLE, cout << __func__ << " " << __LINE__ << ": rgraph =\n" << rgraph_to_string(rgraph) << endl);
  }

  static void rgraph_collapse_self_edge(graph<T_PC, rgraph_node<T_PC, T_N, T_E>, rgraph_edge<T_PC, T_N, T_E>> &rgraph, shared_ptr<rgraph_edge<T_PC, T_N, T_E> const> e)
  {
    T_PC const &from_pc = e->get_from_pc();
    ASSERT(e->get_from_pc() == e->get_to_pc());
    shared_ptr<rgraph_node<T_PC, T_N, T_E>> from_node = rgraph.find_node(from_pc);
    ASSERT(from_node);
    graph<T_PC, T_N, T_E> &from_node_graph = from_node->get_graph();
    for (auto ce : e->get_constituent_edges()) {
      from_node_graph.add_edge(ce);
    }
    rgraph.remove_edge(e);
  }

  static bbl_index_t rgraph_make_T3_move(graph<T_PC, rgraph_node<T_PC, T_N, T_E>, rgraph_edge<T_PC, T_N, T_E>> &rgraph, bbl_index_t cur_graph_node_index, set<T_PC> const &do_not_copy_pcs)
  {
    //cout << __func__ << " " << __LINE__ << ": before, rgraph =\n" << rgraph_to_string(rgraph) << endl;
    list<shared_ptr<rgraph_node<T_PC, T_N, T_E>>> ns;
    rgraph.get_nodes(ns);
    std::function<bool (shared_ptr<rgraph_node<T_PC, T_N, T_E>> const &, shared_ptr<rgraph_node<T_PC, T_N, T_E>> const &)> f = [&rgraph](shared_ptr<rgraph_node<T_PC, T_N, T_E>> const &a, shared_ptr<rgraph_node<T_PC, T_N, T_E>> const &b)
    {
      list<shared_ptr<rgraph_edge<T_PC, T_N, T_E> const>> a_in, b_in;
      rgraph.get_edges_incoming(a->get_pc(), a_in);
      rgraph.get_edges_incoming(b->get_pc(), b_in);
      return a_in.size() < b_in.size();
    };
    ns.sort(f);
    for (auto n : ns) {
      list<shared_ptr<rgraph_edge<T_PC, T_N, T_E> const>> incoming;
      rgraph.get_edges_incoming(n->get_pc(), incoming);
      //cout << __func__ << " " << __LINE__ << ": n = " << n->get_pc().to_string() << ": incoming.size() = " << incoming.size() << endl;
      if (incoming.size() >= 2) {
        auto iter = incoming.begin();
        for (iter++; iter != incoming.end(); iter++) { //ignore the first edge, for all other edges, create ncopy
          auto incoming_e = *iter;
          pair<shared_ptr<rgraph_node<T_PC, T_N, T_E>>, map<T_PC, T_PC>> pr = rgraph_node_copy(n, &cur_graph_node_index, do_not_copy_pcs);
          shared_ptr<rgraph_node<T_PC, T_N, T_E>> ncopy = pr.first;
          map<T_PC, T_PC> const &rename_map = pr.second;
          rgraph.add_node(ncopy);
          //shared_ptr<rgraph_edge<T_PC, T_N, T_E>> e = *incoming.begin();
          rgraph.remove_edge(incoming_e);
          auto new_incoming_e = make_shared<rgraph_edge<T_PC, T_N, T_E>>(*incoming_e);
          new_incoming_e->set_to_pc(ncopy->get_pc());
          rgraph_edge_rename_constituent_edges(new_incoming_e, rename_map);
          rgraph.add_edge(new_incoming_e);

          list<shared_ptr<rgraph_edge<T_PC, T_N, T_E> const>> outgoing;
          rgraph.get_edges_outgoing(n->get_pc(), outgoing);
          for (auto o : outgoing) {
            shared_ptr<rgraph_edge<T_PC, T_N, T_E>> new_o = make_shared<rgraph_edge<T_PC, T_N, T_E>>(ncopy->get_pc(), o->get_to_pc());
            for (auto ce : o->get_constituent_edges()) {
              ASSERT(rename_map.count(ce->get_from_pc()));
              shared_ptr<T_E const> new_ce = make_shared<T_E>(rename_map.at(ce->get_from_pc()), ce->get_to_pc());
              new_o->add_constituent_edge(new_ce);
            }
            rgraph.add_edge(new_o);
          }
        }
        //cout << __func__ << " " << __LINE__ << ": after, rgraph =\n" << rgraph_to_string(rgraph) << endl;
        return cur_graph_node_index;
      }
    }
    NOT_REACHED();
  }

  static void rgraph_edge_rename_constituent_edges(shared_ptr<rgraph_edge<T_PC, T_N, T_E>> e, map<T_PC, T_PC> const &rename_map)
  {
    list<shared_ptr<T_E const>> new_ces;
    for (auto const& ce : e->get_constituent_edges()) {
      ASSERT(rename_map.count(ce->get_to_pc()) > 0);
      //auto new_ce = make_shared<T_E>(*ce);
      //new_ce->set_to_pc(rename_map.at(ce->get_to_pc()));
      auto new_ce = make_shared<T_E>(ce->get_from_pc(), rename_map.at(ce->get_to_pc()));
      new_ces.push_back(new_ce);
    }
    e->set_constituent_edges(new_ces);
  }

  static string rgraph_to_string(graph<T_PC, rgraph_node<T_PC, T_N, T_E>, rgraph_edge<T_PC, T_N, T_E>> const &rgraph)
  {
    stringstream ss;
    ss << "rgraph:\n" << rgraph.to_string() << "\n";
    list<shared_ptr<rgraph_node<T_PC, T_N, T_E>>> ns;
    rgraph.get_nodes(ns);
    for (auto n : ns) {
      ss << n->get_pc().to_string() << ":\n";
      ss << n->get_graph().to_string() << endl;
    }
    list<shared_ptr<rgraph_edge<T_PC, T_N, T_E> const>> es;
    rgraph.get_edges(es);
    for (auto e : es) {
      ss << e->to_string() << ":\n";
      for (auto ce : e->get_constituent_edges()) {
        ss << "  " << ce->to_string() << "\n";
      }
    }
    return ss.str();
  }

  static pair<shared_ptr<rgraph_node<T_PC, T_N, T_E>>, map<T_PC, T_PC>>
  rgraph_node_copy(shared_ptr<rgraph_node<T_PC, T_N, T_E>> n,
                   bbl_index_t *cur_graph_node_index,
                   set<T_PC> const &do_not_copy_pcs)
  {
    graph<T_PC, T_N, T_E> g = n->get_graph();
    map<T_PC, T_PC> rename_map = g.graph_reduce_set_pcs(cur_graph_node_index, do_not_copy_pcs);
    ASSERT(rename_map.count(n->get_pc()));
    T_PC new_pc = rename_map.at(n->get_pc());
    //cout << __func__ << " " << __LINE__ << ": n = " << n->get_pc().to_string() << ": new_pc = " << new_pc.to_string() << endl;
    shared_ptr<rgraph_node<T_PC, T_N, T_E>> ret = make_shared<rgraph_node<T_PC, T_N, T_E>>(new_pc, g);
    return make_pair(ret, rename_map);
  }

  map<T_PC, T_PC> graph_reduce_set_pcs(bbl_index_t *cur_graph_node_index, set<T_PC> const &do_not_copy_pcs)
  {
    map<T_PC, T_PC> rename_map;
    list<shared_ptr<T_N>> ns;
    T_PC root_node_pc;

    this->get_nodes(ns);
    m_nodes.clear();
    for (auto n : ns) {
      list<shared_ptr<T_E const>> incoming;
      T_PC p = n->get_pc();
      this->get_edges_incoming(p, incoming);
      if (!set_belongs(do_not_copy_pcs, p)) {
        T_PC new_pc = p;
        new_pc.set_bbl_index_for_graph_reduction(*cur_graph_node_index);
        rename_map[p] = new_pc;
        (*cur_graph_node_index)++;
      } else {
        rename_map[p] = n->get_pc();
      }
      shared_ptr<T_N> new_n = make_shared<T_N>(rename_map[p]);
      m_nodes[rename_map[p]] = new_n;
    }
    list<shared_ptr<T_E const>> es;
    this->get_edges(es);
    for (auto e : es) {
      this->remove_edge(e);
    }
    ASSERT(m_edges.size() == 0);
    for (auto e : es) {
      ASSERT(rename_map.count(e->get_from_pc()));
      ASSERT(rename_map.count(e->get_to_pc()));
      shared_ptr<T_E const> new_e = make_shared<T_E>(rename_map.at(e->get_from_pc()), rename_map.at(e->get_to_pc()));
      this->add_edge(new_e);
    }
    return rename_map;
  }

  void sort_edges(std::function<bool (T_PC const&, T_PC const&)> cmpF) {
    //cout << "before sorting edges:" << endl;
    //for (typename list<shared_ptr<T_E const>>::const_iterator iter = m_edges.begin(); iter != m_edges.end(); iter++) {
    //  cout << (*iter)->to_string() << endl;
    //}
    graph_edge_sort_fn_t graph_edge_sort_fn(*this, cmpF);
    m_edges.sort(graph_edge_sort_fn);
    //cout << "after sorting edges:" << endl;
    //for (typename list<shared_ptr<T_E const>>::const_iterator iter = m_edges.begin(); iter != m_edges.end(); iter++) {
    //  cout << (*iter)->to_string() << endl;
    //}
  }

  static string path_to_string(T_PATH pth)
  {
    stringstream ss;
    for (auto e : pth) {
      ss << e->to_string() << "-";
    }
    return ss.str();
  }

  static size_t path_num_occurrences_for_pc(T_PATH const &pth, T_PC const &p)
  {
    size_t ret = 0;
    for (auto e : pth) {
      if (e->get_to_pc() == p) {
        ret++;
      }
    }
    return ret;
  }

  list<T_PATH> get_paths_bfs_rec(list<pair<T_PC, T_PATH>> const &frontier, T_PC const &p, int max_occur, long long max_num_paths) const
  {
    cout << __func__ << " " << __LINE__ << ": frontier = ";
    for (auto f : frontier) {
      cout << f.first.to_string() << " ";
    }
    cout << endl;
    if (frontier.empty() || max_num_paths == 0) {
      return list<T_PATH>();
    }
    list<pair<T_PC, T_PATH>> new_frontier;
    list<T_PATH> ret;
    for (auto fpp : frontier) {
      T_PC const &fp = fpp.first;
      T_PATH const &fpth = fpp.second;
      ASSERT(fp != p);
      list<shared_ptr<T_E const>> outgoing;
      get_edges_outgoing(fp, outgoing);
      for (auto e : outgoing) {
        T_PATH fpth_e = fpth;
        fpth_e.push_back(e);
        T_PC fpth_to = e->get_to_pc();
        if (   fpth_to == p
            && ret.size() < max_num_paths) {
          ret.push_back(fpth_e);
        } else if (path_num_occurrences_for_pc(fpth_e, fpth_to) <= max_occur) {
          pair<T_PC, T_PATH> new_e = make_pair(fpth_to, fpth_e);
          new_frontier.push_back(new_e);
        }
      }
    }
    ASSERT(ret.size() <= max_num_paths);
    list<T_PATH> more_ret = get_paths_bfs_rec(new_frontier, p, max_occur, max_num_paths - ret.size());
    ret.splice(ret.end(), more_ret);
    return ret;
  }

  list<T_PATH> get_randomly_chosen_paths_from_start_pc(T_PC const &p) const
  {
    autostop_timer func_timer(__func__);
    /*if (m_path_cache.count(p)) {
      return m_path_cache.at(p);
    }*/
    list<pair<T_PC, T_PATH>> frontier;
    T_PATH path_empty;
    frontier.push_back(make_pair(find_entry_node()->get_pc(), path_empty));
    list<T_PATH> ret = get_paths_bfs_rec(frontier, p, 2, 256);
    //m_path_cache[p] = ret;
    return ret;
  }

  shared_ptr<T_E const> find_edge(T_PC const &from_pc, T_PC const &to_pc) const
  {
    for (auto e : m_edges) {
      if (   (e->get_from_pc() == from_pc)
          && (e->get_to_pc() == to_pc)) {
        return e;
      }
    }
    return NULL;
  }

  set<T_PC> const &get_all_reachable_pcs(T_PC const &p) const
  {
    ASSERT(m_transitive_closure.count(p));
    return m_transitive_closure.at(p);
  }

  bool pc_is_reachable(T_PC const &from, T_PC const &to) const
  {
    ASSERT(m_transitive_closure.count(from));
    return set_belongs(m_transitive_closure.at(from), to);
  }

  void populate_transitive_closure()
  {
    //map<T_PC, set<T_PC>> ret;
    m_transitive_closure.clear();
    set<T_PC> all_pcs = this->get_all_pcs();
    for (auto p : all_pcs) {
      m_transitive_closure[p] = compute_all_reachable_pcs(p);
    }
    //return ret;
  }

  void populate_reachable_and_unreachable_incoming_edges_map()
  {
    m_incoming_reachable_edges_map.clear();
    m_incoming_unreachable_edges_map.clear();
    set<T_PC> all_pcs = this->get_all_pcs();
    for (auto const& p : all_pcs) {
      list<shared_ptr<T_E const>> in;
      get_edges_incoming(p, in);
      set<T_PC> const &reachable = get_all_reachable_pcs(p);
      m_incoming_reachable_edges_map[p] = list<shared_ptr<T_E const>>();
      m_incoming_unreachable_edges_map[p] = list<shared_ptr<T_E const>>();
      for (auto e : in) {
        if (set_belongs(reachable, e->get_from_pc())) {
          m_incoming_reachable_edges_map[p].push_back(e);
        } else {
          m_incoming_unreachable_edges_map[p].push_back(e);
        }
      }
    }
  }

  list<shared_ptr<T_E const>> const &get_edges_incoming_reachable(T_PC const &p) const
  {
    return m_incoming_reachable_edges_map.at(p);
  }

  list<shared_ptr<T_E const>> const &get_edges_incoming_unreachable(T_PC const &p) const
  {
    return m_incoming_unreachable_edges_map.at(p);
  }

  set<T_PC> compute_all_reachable_pcs_using_edge_select_fn(T_PC const &p, std::function<bool (shared_ptr<T_E const> const &)> edge_select_fn) const
  {
    size_t orig_retsize;
    set<T_PC> frontier;
    set<T_PC> ret;

    frontier.insert(p);
    do {
      orig_retsize = ret.size();
      /*cout << __func__ << " " << __LINE__ << ": ret =";
      for (auto r : ret) {
        cout << " " << r.to_string();
      }
      cout << "\n" << __func__ << " " << __LINE__ << ": frontier =";
      for (auto f : frontier) {
        cout << " " << f.to_string();
      }
      cout << "\n";*/
      frontier = frontier_expand(frontier, edge_select_fn);
      set_union(ret, frontier);
      //cout << __func__ << " " << __LINE__ << ": orig_retsize = " << orig_retsize << ", ret.size() = " << ret.size() << endl;
    } while (ret.size() > orig_retsize);
    return ret;
  }

  static bool edge_select_fn_all(shared_ptr<T_E const> const &e)
  {
    return true;
  }

  set<T_PC> compute_all_reachable_pcs(T_PC const &p) const
  {
    return compute_all_reachable_pcs_using_edge_select_fn(p, edge_select_fn_all);
  }

  void incoming_edges_split_into_reachable_and_unreachable(T_PC const &p, list<shared_ptr<T_E const>> const &incoming, list<shared_ptr<T_E const>> &incoming_reachable, list<shared_ptr<T_E const>> &incoming_unreachable) const
  {
    set<T_PC> const &reachable = get_all_reachable_pcs(p);
    for (auto e : incoming) {
      if (set_belongs(reachable, e->get_from_pc())) {
        incoming_reachable.push_back(e);
      } else {
        incoming_unreachable.push_back(e);
      }
    }
  }


  list<graph_edge_composition_ref<T_PC,T_E>> get_incoming_paths_at_depth(T_PC const &p, int m_depth) const
  {
    if (m_depth == 0) {
      return list<graph_edge_composition_ref<T_PC,T_E>>();
    }
    set<T_PC> stop_pcs;
    stop_pcs.insert(p);
    return get_incoming_paths_at_depth_stopping_at_stop_pc(p, m_depth, stop_pcs);
  }

  list<graph_edge_composition_ref<T_PC,T_E>> get_outgoing_paths_at_lookahead_and_max_unrolls(T_PC const &p, int lookahead, int max_unrolls) const
  {
    //ASSERT(outgoing_paths.size() == 0);
    set<T_PC> stop_pcs;
    stop_pcs.insert(p);
    return get_outgoing_paths_at_lookahead_and_max_unrolls_stopping_at_stop_pc(p, lookahead, max_unrolls, stop_pcs);
    /*cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ": lookahead = " << lookahead << ": max_unrolls = " << max_unrolls << endl;
    for (auto pth : outgoing_paths) {
      for (auto e : pth) {
        cout << e->to_string() << "-";
      }
      cout << endl;
    }*/
  }

  void remove_node_and_incident_edges(T_PC const &p)
  {
    list<shared_ptr<T_E const>> incoming, outgoing;
    this->get_edges_outgoing(p, outgoing);
    for (auto e : outgoing) {
      remove_edge(e);
    }
    this->get_edges_incoming(p, incoming);
    for (auto e : incoming) {
      remove_edge(e);
    }
    remove_node(p);
  }

/*
  // We want to find maximal matching for the line graph(nodes-edges dual) of conflict graph
	maximal_matching_iterator_t maximal_matching_iterator_begin() const
  {
    maximal_matching_iterator_t ret;
    graph<T_PC, T_N, T_E> tmp(*this);
    ASSERT(tmp.get_all_pcs().size());
    ret.begin_pcs = tmp.get_all_pcs();
    //cout << __func__ << " " << __LINE__ << ": this =\n" << this->to_string() << endl;
    while (tmp.get_all_pcs().size()) {
      list<shared_ptr<T_E const>> outgoing_edges, incoming_edges;
      T_PC p = (ret.m_status.size() == 0) ? *(ret.begin_pcs.begin()) : (*tmp.get_all_pcs().begin());
      if(ret.m_status.size() == 0)
      {
        ret.begin_pcs.erase(ret.begin_pcs.begin());
        ret.completed_pcs.push_back(p);
      }

      tmp.get_edges_outgoing(p, outgoing_edges);
      tmp.get_edges_incoming(p, incoming_edges);
      ASSERT(outgoing_edges.size() == incoming_edges.size());
      ret.m_status.push_back(make_pair(p, maximal_matching_iterator_t::present));
      if (outgoing_edges.size()) {
        for (auto e : outgoing_edges) {
          //cout << __func__ << " " << __LINE__ << ": removing " << e->to_string() << endl;
          tmp.remove_node_and_incident_edges(e->get_to_pc());
        }
        //cout << __func__ << " " << __LINE__ << ": tmp =\n" << tmp.to_string() << endl;
        incoming_edges.clear();
        tmp.get_edges_incoming(p, incoming_edges); //recompute incoming edges as they may have changed
        for (auto e : incoming_edges) {
          //cout << __func__ << " " << __LINE__ << ": removing " << e->to_string() << endl;
          tmp.remove_node_and_incident_edges(e->get_from_pc());
        }
      }
      tmp.remove_node_and_incident_edges(p);
    }
    return ret;
  }

  maximal_matching_iterator_t maximal_matching_iterator_next(maximal_matching_iterator_t const &iter) const
  {
    maximal_matching_iterator_t ret(iter);
    ret.m_status.clear();
    bool is_old_path;
    
    while(ret.begin_pcs.size())
    {
      graph<T_PC, T_N, T_E> tmp(*this);
      is_old_path = false;
      while (tmp.get_all_pcs().size()) {
        list<shared_ptr<T_E const>> outgoing_edges, incoming_edges;
        T_PC p = (ret.m_status.size() == 0) ? *(ret.begin_pcs.begin()) : (*tmp.get_all_pcs().begin());
        if(ret.m_status.size() == 0)
        {
          ret.begin_pcs.erase(ret.begin_pcs.begin());
          ret.completed_pcs.push_back(p);
        }
        else
        {
          for(auto const &cpc : ret.completed_pcs)
          {
          	if(p == cpc)
          	{
          		is_old_path = true;
          		break;
          	}
          }
          if(is_old_path)
          {
          	break;
          }
        }

        tmp.get_edges_outgoing(p, outgoing_edges);
        tmp.get_edges_incoming(p, incoming_edges);
        ASSERT(outgoing_edges.size() == incoming_edges.size());
        ret.m_status.push_back(make_pair(p, maximal_matching_iterator_t::present));
        if (outgoing_edges.size()) {
          for (auto e : outgoing_edges) {
            //cout << __func__ << " " << __LINE__ << ": removing " << e->to_string() << endl;
            tmp.remove_node_and_incident_edges(e->get_to_pc());
          }
          //cout << __func__ << " " << __LINE__ << ": tmp =\n" << tmp.to_string() << endl;
          incoming_edges.clear();
          tmp.get_edges_incoming(p, incoming_edges); //recompute incoming edges as they may have changed
          for (auto e : incoming_edges) {
            //cout << __func__ << " " << __LINE__ << ": removing " << e->to_string() << endl;
            tmp.remove_node_and_incident_edges(e->get_from_pc());
          }
        }
        tmp.remove_node_and_incident_edges(p);
      }
      if(is_old_path)
        ret.m_status.clear();
      else
    	  return ret;
    }
    ASSERT(ret.begin_pcs.size() == 0);
    ret.m_is_end = true;
    return ret; //XXX: TO_BE_TESTED
  }

  set<T_PC> get_maximal_matching(maximal_matching_iterator_t const &iter) const
  {
    set<T_PC> ret;
    ASSERT(!iter.m_is_end);
    ASSERT(this->get_all_pcs().size() > 0);
    graph<T_PC, T_N, T_E> tmp(*this);
    while (tmp.get_all_pcs().size()) {
      list<shared_ptr<T_E const>> outgoing_edges, incoming_edges;
      T_PC p = (*tmp.get_all_pcs().begin());
      if (iter.is_present_status_for_pc(p)) {
        ret.insert(p);
        tmp.get_edges_outgoing(p, outgoing_edges);
        tmp.get_edges_incoming(p, incoming_edges);
        ASSERT(outgoing_edges.size() == incoming_edges.size());
        for (auto e : outgoing_edges) {
          //cout << __func__ << " " << __LINE__ << ": removing " << e->to_string() << endl;
          tmp.remove_node_and_incident_edges(e->get_to_pc());
        }
        incoming_edges.clear();
        tmp.get_edges_incoming(p, incoming_edges); //recompute incoming edges as they may have changed
        for (auto e : incoming_edges) {
          //cout << __func__ << " " << __LINE__ << ": removing " << e->to_string() << endl;
          tmp.remove_node_and_incident_edges(e->get_from_pc());
        }
      }
      tmp.remove_node_and_incident_edges(p);
    }
    ASSERT(ret.size() > 0);
    return ret;
  }
*/
  size_t num_edges() const { return m_edges.size(); }

  //manager<graph_edge_composition_t<T_PC, T_N, T_E>> &get_ecomp_manager() const
  //{
  //  return m_ecomp_manager;
  //}

  virtual ~graph() = default;

  string graph_to_string() const
  {
    ostringstream oss;
    this->graph_to_stream(oss);
    return oss.str();
  }

  set<T_PC> collect_all_intermediate_pcs(T_PC const& from_pc, T_PC const& to_pc) const
  {
    set<T_PC> visited;
    stack<pair<T_PC, set<T_PC>>> stk;
    set<T_PC> init_set = { to_pc };
    stk.push(make_pair(to_pc, init_set));
    set<T_PC> ret;

    while (stk.size()) {
      pair<T_PC, set<T_PC>> p_cur_pc = stk.top();
      stk.pop();
      T_PC cur_pc = p_cur_pc.first;
      visited.insert(cur_pc);

      if (from_pc == cur_pc) {
        ret.insert(p_cur_pc.second.begin(), p_cur_pc.second.end());
        continue;
      }
      else if (cur_pc.is_start()) {
        continue;
      }
      else {
        list<shared_ptr<T_E const>> incoming_edges;
        this->get_edges_incoming(cur_pc, incoming_edges);
        for (auto const& ie : incoming_edges) {
          if (!visited.count(ie->get_from_pc())) {
            auto chain = p_cur_pc.second;
            chain.insert(ie->get_from_pc());
            stk.push(make_pair(ie->get_from_pc(), chain));
          }
        }
      }
    }
    return ret;
  }

  virtual void graph_to_stream(ostream& os) const
  {
    list<shared_ptr<T_N>> ns;
    this->get_nodes(ns);

    os << "=Nodes:";
    for (auto const& n : ns) {
      T_PC const& p = n->get_pc();
      os << " " << p.to_string();
    }
    os << endl;
    list<shared_ptr<T_E const>> es;
    this->get_edges(es);
    os << "=Edges:\n";
    for (auto const& e : es) {
      os << e->get_from_pc().to_string() << " => " << e->get_to_pc().to_string() << endl;
    }
    os << "=graph done\n";
  }


protected:

  list<shared_ptr<T_E const>>
  get_maximal_basic_block_edge_list_ending_at_pc(T_PC to_pc) const
  {
    list<shared_ptr<T_E const>> ret;
    list<shared_ptr<T_E const>> incoming, outgoing;

    this->get_edges_incoming(to_pc, incoming);
    if (incoming.size() != 1) {
      return {};
    }

    do {
      ret.push_front(incoming.front());
      to_pc = incoming.front()->get_from_pc();

      incoming.clear();
      outgoing.clear();
      this->get_edges_incoming(to_pc, incoming);
      this->get_edges_outgoing(to_pc, outgoing);
    } while (incoming.size() == 1 && outgoing.size() == 1);

    return ret;
  }

private:
  list<graph_edge_composition_ref<T_PC,T_E>> get_outgoing_paths_at_lookahead_and_max_unrolls_stopping_at_stop_pc(T_PC const &p, int lookahead, int max_unrolls, set<T_PC> const &stop_pcs) const
  {
    list<graph_edge_composition_ref<T_PC,T_E>> outgoing_paths;
    list<shared_ptr<T_E const>> outgoing_edges;
    this->get_edges_outgoing(p, outgoing_edges);
    ASSERT(lookahead >= 1);
    ASSERT(max_unrolls >= 0);

    list<shared_ptr<T_E const>> edges_with_no_outgoing_paths;
    list<graph_edge_composition_ref<T_PC,T_E>> outgoing_paths_at_lookahead;
    for (auto e : outgoing_edges) {
      list<graph_edge_composition_ref<T_PC,T_E>> ls;
      int to_pc_max_unrolls = max_unrolls;
      if (lookahead > 1) {
        if (set_belongs(stop_pcs, p )) {
          to_pc_max_unrolls--;
        }
        if (to_pc_max_unrolls >= 0) {
          set<T_PC> stop_pcs_e = stop_pcs;
          //stop_pcs_e.insert(e->get_to_pc());
          ls = get_outgoing_paths_at_lookahead_and_max_unrolls_stopping_at_stop_pc(e->get_to_pc(), lookahead - 1, to_pc_max_unrolls, stop_pcs_e);
          for (auto e2 : ls) {
            graph_edge_composition_ref<T_PC,T_E> new_e = mk_series(mk_edge_composition<T_PC,T_E>(e), e2);
            outgoing_paths_at_lookahead.push_back(new_e);
          }
        }
      }
      if (ls.size() == 0) {
        if (lookahead == 1) {
          graph_edge_composition_ref<T_PC,T_E> ec = mk_edge_composition<T_PC,T_E>(e);
          outgoing_paths_at_lookahead.push_back(ec);
        } else {
          edges_with_no_outgoing_paths.push_back(e);
        }
      }
    }

    if (outgoing_paths_at_lookahead.size() > 0) {
      for (auto e : outgoing_paths_at_lookahead) {
        outgoing_paths.push_back(e);
      }
      for (auto e : edges_with_no_outgoing_paths) {
        //list<shared_ptr<T_E>> els;
        //els.push_back(e);
        graph_edge_composition_ref<T_PC,T_E> ec = mk_edge_composition<T_PC,T_E>(e);
        //outgoing_paths.push_back(els);
        outgoing_paths.push_back(ec);
      }
    }
    //cout << __func__ << " " << __LINE__ << " : p = " << p.to_string() << ": lookahead = " << lookahead << ": max_unrolls = " << max_unrolls << ": stop_pcs =";
    //for (auto sp : stop_pcs) {
    //  cout << " " << sp.to_string();
    //}
    //cout << endl;
    //cout << "outgoing_paths:\n";
    //for (auto pth : outgoing_paths) {
    //  cout << pth->graph_edge_composition_to_string() << endl;
    //}
    return outgoing_paths;
  }

  list<graph_edge_composition_ref<T_PC,T_E>> get_incoming_paths_at_depth_stopping_at_stop_pc(T_PC const &p, int m_depth, set<T_PC> const &stop_pcs) const
  {
    list<graph_edge_composition_ref<T_PC,T_E>> incoming_paths;
    list<shared_ptr<T_E const>> incoming;

    ASSERT(m_depth > 0);
    this->get_edges_incoming(p, incoming);

    list<shared_ptr<T_E const>> edges_with_no_incoming_paths;
    list<graph_edge_composition_ref<T_PC,T_E>> incoming_paths_at_depth;
    for (auto e : incoming) {
      list<graph_edge_composition_ref<T_PC,T_E>> ls;
      if (!set_belongs(stop_pcs, e->get_from_pc()) && m_depth > 1) {
        set<T_PC> stop_pcs_e = stop_pcs;
        stop_pcs_e.insert(e->get_from_pc());
        ls = get_incoming_paths_at_depth_stopping_at_stop_pc(e->get_from_pc(), m_depth - 1, stop_pcs_e);
        for (auto e2 : ls) {
          //e2.add_edge_in_series(e, this->get_start_state(), this->get_consts_struct());
          graph_edge_composition_ref<T_PC,T_E> new_e = mk_series(e2, mk_edge_composition<T_PC,T_E>(e));
          incoming_paths_at_depth.push_back(new_e);
        }
      }
      if (ls.size() == 0) {
        if (m_depth == 1) {
          //list<shared_ptr<T_E>> els;
          //els.push_back(e);
          graph_edge_composition_ref<T_PC,T_E> ec = mk_edge_composition<T_PC,T_E>(e);
          incoming_paths_at_depth.push_back(ec);
        } else {
          edges_with_no_incoming_paths.push_back(e);
        }
      }
    }
    if (incoming_paths_at_depth.size() > 0) {
      for (auto e : incoming_paths_at_depth) {
        incoming_paths.push_back(e);
      }
      for (auto e : edges_with_no_incoming_paths) {
        //list<shared_ptr<T_E>> els;
        //els.push_back(e);
        graph_edge_composition_ref<T_PC,T_E> ec = mk_edge_composition<T_PC,T_E>(e);
        incoming_paths.push_back(ec);
      }
    }
    return incoming_paths;
  }

  set<T_PC> frontier_expand(set<T_PC> const &cur, std::function<bool (shared_ptr<T_E const> const &)> edge_select_fn) const
  {
    set<T_PC> ret;
    for (auto p : cur) {
      list<shared_ptr<T_E const>> outgoing;
      this->get_edges_outgoing(p, outgoing);
      for (auto e : outgoing) {
        if (edge_select_fn(e)) {
          ret.insert(e->get_to_pc());
        }
      }
    }
    return ret;
  }

  void get_edges_helper(T_PC const &p, multimap_edges const &mm, list<shared_ptr<T_E const>> &out) const
  {
    //cout << __func__ << " " << __LINE__ << ": mm.size() = " << mm.size() << endl;
    pair<multimap_const_iterator, multimap_const_iterator> iters = mm.equal_range(p);
    multimap_const_iterator i;
    for (i = iters.first; i != iters.second; ++i) {
      //cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", returning " << i->second->to_string() << endl;
      out.push_back(i->second);
    }
  }

  void dfs(std::function<void (shared_ptr<T_E const>)> f_back_edge,
           std::function<void (shared_ptr<T_E const>)> f_forward_edge) const
  {
    map<T_PC, dfs_color> visited;
    for(const auto& p_n : m_nodes)
      visited[p_n.first] = dfs_color_white;

    shared_ptr<T_N> entry_node = find_entry_node();
    ASSERT(entry_node);
    dfs_rec(entry_node->get_pc(), visited, f_back_edge, f_forward_edge);
    for (const auto& p_n : m_nodes) {
      if (visited[p_n.first] != dfs_color_black) {
        //cout << __func__ << " " << __LINE__ << ": WARNING: " << p_n.first.to_string() << " is unreachable!\n";
        //NOT_REACHED();
      }
    }
  }

  void dfs_rec(const T_PC& p, map<T_PC, dfs_color>& visited,
                std::function<void (shared_ptr<T_E const>)> f_back_edge,
                std::function<void (shared_ptr<T_E const>)> f_forward_edge) const
  {
    visited[p] = dfs_color_gray;

    list<shared_ptr<T_E const>> out_edges;
    get_edges_outgoing(p, out_edges);

    for(shared_ptr<T_E const> out_edge : out_edges)
    {
      T_PC p_next = out_edge->get_to_pc();
      dfs_color next_color = visited.at(p_next);

      if(next_color == dfs_color_white)
        dfs_rec(p_next, visited, f_back_edge, f_forward_edge);
      else if(next_color == dfs_color_gray)
        f_back_edge(out_edge);
      else if(next_color == dfs_color_black)
        f_forward_edge(out_edge);
      else
        assert(false && "invalid dfs color");
    }
    visited[p] = dfs_color_black;
  }

  void get_edges_outgoing_without_backedges(T_PC p, list<shared_ptr<T_E const>>& out) const
  {
    list<shared_ptr<T_E const>> out_edges;
    get_edges_outgoing(p, out_edges);
    for(auto& e : out_edges)
    {
      if(!e->is_back_edge())
      {
        out.push_back(e);
      }
    }
  }

  void get_edges_in_scc(const set<T_PC>& scc, set<shared_ptr<T_E const>>& es) const
  {
    for(T_PC & p : scc)
    {
      list<shared_ptr<T_E const>> out;
      get_edges_outgoing(p, out);
      for(auto& e : out)
      {
        if(scc.count(e->get_to_pc()) > 0)
          es.insert(e);
      }
    }
  }

  void get_sccs_forward_dfs(T_PC p, map<T_PC, bool>& visited, stack<T_PC>& st) const
  {
    visited[p] = true;

    list<shared_ptr<T_E const>> out;
    get_edges_outgoing(p, out);
    for(auto& e : out)
    {
      T_PC next_pc = e->get_to_pc();
      if(!visited[next_pc])
        get_sccs_forward_dfs(next_pc, visited, st);
    }
    st.push(p);
  }

  void get_sccs_reverse_dfs(T_PC p, map<T_PC, bool>& visited, scc<T_PC>& s) const
  {
    visited[p] = true;
    s.add_pc(p);

    list<shared_ptr<T_E const>> in;
    get_edges_incoming(p, in);
    for(auto& e : in)
    {
      T_PC next_pc = e->get_from_pc();
      if(!visited[next_pc])
        get_sccs_reverse_dfs(next_pc, visited, s);
    }
  }

  map<T_PC, shared_ptr<T_N>> m_nodes;
  list<shared_ptr<T_E const>> m_edges;
  //map<T_PC, list<T_PATH>> m_path_cache;

  multimap_edges m_edges_lookup_outgoing;
  multimap_edges m_edges_lookup_incoming;

  //the following fields are only used for constprop (graph_cp.h)
  map<T_PC, set<T_PC>> m_transitive_closure;
  map<T_PC, list<shared_ptr<T_E const>>> m_incoming_reachable_edges_map;
  map<T_PC, list<shared_ptr<T_E const>>> m_incoming_unreachable_edges_map;

  //mutable manager<graph_edge_composition_t<T_PC, T_N, T_E>> m_ecomp_manager;

  mutable map<set<graph_edge_composition_ref<T_PC,T_E>>, graph_edge_composition_ref<T_PC,T_E>> m_graph_edge_composition_pterms_canonicalize_cache;
};

class int_pc {
private:
  int m_index;
public:
  int_pc(int index) : m_index(index) {}
  int get_index() const { return m_index; }
  string to_string() const { stringstream ss; ss << m_index; return ss.str(); }
  bool operator<(int_pc const &other) const { return m_index < other.m_index; }
  bool operator==(int_pc const &other) const { return m_index == other.m_index; }
};
}

namespace std
{
using namespace eqspace;
template<>
struct hash<int_pc>
{
  std::size_t operator()(int_pc const &s) const
  {
    return s.get_index();
  }
};
template<typename T_PC>
struct hash<edge<T_PC>>
{
  std::size_t operator()(edge<T_PC> const &e) const
  {
    hash<T_PC> pc_hasher;
    size_t seed = 0;
    myhash_combine<size_t>(seed, pc_hasher(e.get_from_pc()));
    myhash_combine<size_t>(seed, pc_hasher(e.get_to_pc()));
    return seed;
  }
};

}

/*
namespace eqspace {
template <typename T_E>
class mutex_paths_iterator_object_t {
public:
  graph<int_pc, node<int_pc>, edge<int_pc>> m_conflict_graph;
  graph<int_pc, node<int_pc>, edge<int_pc>>::maximal_matching_iterator_t m_iter;
  mutex_paths_iterator_object_t(list<list<shared_ptr<T_E const>>> const &paths)
  {
    ASSERT(paths.size() > 0);
    int count1 = 0;
    auto pitr = paths.begin();
    while (pitr != paths.end()) {
      auto const& path1 = *pitr;
      int count2 = 0;
      m_conflict_graph.add_node(make_shared<node<int_pc>>(count1));
      auto p2itr = paths.begin();
      while (p2itr != paths.end()) {
        auto const& path2 = *p2itr;
        if (path1 != path2) {
          if (path_is_prefix_of(path1, path2) || path_is_prefix_of(path2, path1)) {
            m_conflict_graph.add_edge(make_shared<edge<int_pc>>(int_pc(count1), int_pc(count2)));
          }
        }
        count2++;
        ++p2itr;
      }
      count1++;
      ++pitr;
    }
    ASSERT((m_conflict_graph.get_all_pcs().size() == paths.size()));
  }
  string to_string() const
  {
    stringstream ss;
    ss << " conflict_graph =\n" << m_conflict_graph.to_string() << endl;
    ss << " maximal_matching =\n" << m_iter.to_string() << endl;
    return ss.str();
  }
  //bool operator==(mutex_paths_iterator_object_t<T_E> const &other) { return m_iter == other.m_iter; }
  bool is_end() const { return m_iter.is_end(); }
};
*/
template<typename T_E>
bool path_is_prefix_of(list<shared_ptr<T_E const>> const &needle, list<shared_ptr<T_E const>> const &haystack)
{
  typename list<shared_ptr<T_E const>>::const_iterator iter1, iter2;
  for (iter1 = needle.begin(), iter2 = haystack.begin();
       iter1 != needle.end() && iter2 != haystack.end();
       iter1++, iter2++) {
    if (*iter1 != *iter2) {
      return false;
    }
  }
  if (iter1 != needle.end()) {
    return false;
  }
  return true;
}

/*template<typename T_E>
mutex_paths_iterator_t<T_E> mutex_paths_iter_begin(list<list<shared_ptr<T_E const>>> const &paths)
{
  mutex_paths_iterator_t<T_E> ret = make_shared<mutex_paths_iterator_object_t<T_E>>(paths);
  ret->m_iter = ret->m_conflict_graph.maximal_matching_iterator_begin();
  return ret;
}

//template<typename T_E>
//mutex_paths_iterator_t<T_E> mutex_paths_iter_end(list<list<shared_ptr<T_E>>> const &paths)
//{
//  mutex_paths_iterator_t<T_E> ret = make_shared<mutex_paths_iterator_object_t<T_E>>(paths);
//  ret->m_iter = ret->m_conflict_graph.maximal_matching_iterator_end();
//  return ret;
//}

template<typename T_E>
mutex_paths_iterator_t<T_E> mutex_paths_iter_next(mutex_paths_iterator_t<T_E> const &iter)
{
  mutex_paths_iterator_t<T_E> ret(iter);
  ret->m_iter = ret->m_conflict_graph.maximal_matching_iterator_next(ret->m_iter);
  return ret;
}

template<typename T_E>
list<list<shared_ptr<T_E const>>>
get_mutex_paths(list<list<shared_ptr<T_E const>>> const &paths, mutex_paths_iterator_t<T_E> const &iter)
{
  //cout << __func__ << " " << __LINE__ << ": iter =\n" << iter->to_string() << endl;
  set<int_pc> matching = iter->m_conflict_graph.get_maximal_matching(iter->m_iter);
  ASSERT(matching.size() > 0);
  vector<list<shared_ptr<T_E const>>> paths_vec;
  for (const auto &p : paths) {
    paths_vec.push_back(p);
  }
  list<list<shared_ptr<T_E const>>> ret;
  for (auto p : matching) {
    ret.push_back(paths_vec.at(p.get_index()));
  }
  return ret;
}


}
*/
