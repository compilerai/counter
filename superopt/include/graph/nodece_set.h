#pragma once

#include "graph/nodece.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E>
class nodece_set_t;

template<typename T_PC, typename T_N, typename T_E>
using nodece_set_ref = shared_ptr<nodece_set_t<T_PC, T_N, T_E> const>;


template<typename T_PC, typename T_N, typename T_E>
class nodece_set_t
{
private:
  list<nodece_ref<T_PC, T_N, T_E>> m_nodeces;

  bool m_is_managed;
public:
  ~nodece_set_t();
  list<nodece_ref<T_PC, T_N, T_E>> const &get_nodeces() const
  { return m_nodeces; }
  bool operator==(nodece_set_t const &ncs) const { return m_nodeces == ncs.m_nodeces; }
  size_t size() const { return m_nodeces.size(); }
  //void push_back(nodece_ref<T_PC, T_N, T_E> const &nce) { m_nodeces.push_back(nce); }

  static string nodece_set_from_stream(istream& is, graph_with_predicates<T_PC,T_N,T_E,predicate> const& g, context* ctx, string const& inprefix, nodece_set_ref<T_PC, T_N, T_E>& nce_set)
  {
    string line;
    bool end = !getline(is, line);
    ASSERT(!end);
    list<nodece_ref<T_PC, T_N, T_E>> nces;
    string prefix = inprefix + "nodece ";
    size_t i = 0;
    while (string_has_prefix(line, "=" + prefix)) {
      nodece_ref<T_PC, T_N, T_E> nce;
      stringstream prefixss;
      prefixss << prefix << i << " ";
      line = nodece_t<T_PC, T_N, T_E>::nodece_from_stream(is, g, ctx, prefixss.str(), nce);
      nces.push_back(nce);
      i++;
    }
    nce_set = mk_nodece_set(nces);
    return line;
  }

  void nodece_set_to_stream(ostream& os, string const& prefix) const
  {
    size_t i = 0;
    for (auto const& nce : m_nodeces) {
      stringstream ss;
      ss << prefix + "nodece " << i;
      string nprefix = ss.str();
      os << "=" + nprefix + "\n";
      nce->nodece_to_stream(os, nprefix + " ");
      i++;
    }
  }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  //void nodece_set_union(shared_ptr<nodece_set_t<T_PC, T_N, T_E> const> const& other);
  static manager<nodece_set_t<T_PC, T_N, T_E>> *get_nodece_set_manager();
  template<typename T_PC_ANY, typename T_N_ANY, typename T_E_ANY>
  friend shared_ptr<nodece_set_t<T_PC_ANY, T_N_ANY, T_E_ANY> const> mk_nodece_set(list<nodece_ref<T_PC_ANY, T_N_ANY, T_E_ANY>> const &nodece_ls);
private:
  nodece_set_t() : m_is_managed(false)
  { }
  nodece_set_t(list<nodece_ref<T_PC, T_N, T_E>> const &nodeces) : m_nodeces(nodeces), m_is_managed(false)
  { }
};

template<typename T_PC, typename T_N, typename T_E>
nodece_set_ref<T_PC, T_N, T_E> mk_nodece_set(list<nodece_ref<T_PC, T_N, T_E>> const &nodeces);

}

namespace std
{
using namespace eqspace;
template<typename T_PC, typename T_N, typename T_E>
struct hash<nodece_set_t<T_PC, T_N, T_E>>
{
  std::size_t operator()(nodece_set_t<T_PC, T_N, T_E> const &ncs) const
  {
    std::size_t seed = 0;
    for (auto const& nce : ncs.get_nodeces()) {
      myhash_combine<size_t>(seed, (size_t)nce.get());
    }
    return seed;
  }
};
}
