#pragma once
#include "expr/expr.h"
#include "gsupport/rodata_map.h"
//#include "cg/parse_edge_composition.h"
#include "graph/graphce.h"
#include <string>
#include <list>
#include <map>

namespace eqspace {
using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_predicates;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_ce;

template<typename T_PC, typename T_N, typename T_E>
class nodece_t;

template<typename T_PC, typename T_N, typename T_E>
using nodece_ref = shared_ptr<nodece_t<T_PC, T_N, T_E>>;

template<typename T_PC, typename T_N, typename T_E>
class nodece_t
{
private:
  //m_ce is not treated as a cache for the counter example generated afetr propagating m_graphce on m_path (in context of a graph), but as the final member
  // When we explore possible correlations in a breadth-first-search, there can exist simutaneously multiple cg_edges which have same edge_id (i.e. from pc,to pc), but the constituent src_paths are different (due to unrolling)
  // The m_path for all of these edges will be same, i.e. same ptr will represent all these edges and the exact edge it represents is identified by interpreting m_path in context of a particular graph (CG).
  // nodece's are thus added to a particular graph PC and are specific wrt to a path/edge in that graph.
  // When we copy one graph to another, we add more edges to it incrementally retaining the existing ones. So the nodece's added to it are still valid
  // To verify this after making any fundamental change in which scenarios graph are copied etc, use the ASSERTCHECKS debug in nodece_get_counter_example function.

  //Further, the counterexample contains a random_seed that cannot be recovered from other information
//  graphce_ref<T_PC,T_N,T_E> m_graphce;
  shared_ptr<graph_edge_composition_t<T_PC,T_E> const> m_path;
  bool m_is_managed;
  counter_example_t m_ce;
  size_t m_num_edges_traversed;
  list<T_PC> m_visited_pcs;
//  invstate_region_t<T_PC> m_last_invstate_region;

public:

  nodece_t(/*dshared_ptr<graphce_t<T_PC,T_N,T_E>> const &gce,*/ shared_ptr<graph_edge_composition_t<T_PC,T_E> const> const &path, counter_example_t const &ce, size_t num_edges_traversed, list<T_PC> const& visited_pcs/*, invstate_region_t<T_PC> const& last_region*/) : /*m_graphce(gce),*/ m_path(path), m_is_managed(false), m_ce(ce), m_num_edges_traversed(num_edges_traversed), m_visited_pcs(visited_pcs)/*, m_last_invstate_region(last_region)*/
  {
    ASSERT(path);
  }

  static manager<nodece_t<T_PC, T_N, T_E>> *get_nodece_manager()
  {
    static manager<nodece_t<T_PC, T_N, T_E>> *ret = NULL;
    if (!ret) {
      ret = new manager<nodece_t<T_PC, T_N, T_E>>;
    }
    return ret;
  }

//  graphce_ref<T_PC,T_N,T_E> get_graphce_ref() const { return m_graphce; }

  size_t nodece_get_num_edges_traversed() const { return m_num_edges_traversed; }

  shared_ptr<graph_edge_composition_t<T_PC,T_E> const> const &nodece_get_path() const { return m_path; }

  //template<typename T_PRED>
  counter_example_t const &
  nodece_get_counter_example(/*graph_with_ce<T_PC, T_N, T_E, T_PRED> const *g*/) const
  {
    //CPP_DBG_EXEC2(ASSERTCHECKS, 
    //    ASSERT(this->nodece_compute_counter_example(g).counter_example_equals(m_ce));
    //);
    return m_ce;
  }

  //template<typename T_PRED>
  //counter_example_t
  //nodece_compute_counter_example(graph_with_ce<T_PC, T_N, T_E, T_PRED> const *g) const
  //{
  //  context *ctx = g->get_context();
  //  counter_example_t ret(ctx);
  //  counter_example_t rand_values(ctx);
  //  counter_example_t in = m_graphce->get_counter_example();
  //  CPP_DBG_EXEC4(CE_ADD, cout << __func__ << " " << __LINE__ << ": in =\n" << in.counter_example_to_string() << endl);
  //  bool success = g->counter_example_translate_on_edge_composition(in, m_path, ret, rand_values, g->get_relevant_memlabels()).first;
  //  ASSERT(success);
  //  ASSERT(rand_values.is_empty());
  //  CPP_DBG_EXEC4(CE_ADD, cout << __func__ << " " << __LINE__ << ": ret =\n" << ret.counter_example_to_string() << endl);
  //  return ret;
  //}

  bool operator==(nodece_t const &other) const
  {
    return /*m_graphce == other.m_graphce &&*/ m_path == other.m_path && m_ce.counter_example_equals(other.m_ce) && m_visited_pcs == other.m_visited_pcs;
  }


  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }

  bool id_is_zero() const { return !m_is_managed; }

  ~nodece_t()
  {
    if (m_is_managed) {
      this->get_nodece_manager()->deregister_object(*this);
    }
  }

  map<T_PC, size_t> nodece_get_pc_seen_count(void) const
  {
    return m_path->graph_edge_composition_get_pc_seen_count();
  }

  list<T_PC> const& get_visited_pcs() const
  {
    ASSERT(m_visited_pcs.size());
    return m_visited_pcs;
  }

  T_PC nodece_get_last_visited_pc() const
  {
    ASSERT(m_visited_pcs.size());
    return *(m_visited_pcs.rbegin());
  }

  bool ce_visited_input_pc(T_PC const& p) const
  {
    ASSERT(m_visited_pcs.size());
    if (std::find(m_visited_pcs.begin(), m_visited_pcs.end(), p) != m_visited_pcs.end())
      return true;
    else
      return false;
  }


  static string nodece_from_stream(istream& is, graph_with_predicates<T_PC,T_N,T_E,predicate> const& g, context* ctx, string const& prefix, nodece_ref<T_PC, T_N, T_E>& nce);

  string nodece_to_string() const;

  void nodece_to_stream(ostream& os, string const& prefix) const
  {
    os << "=" + prefix + "nodece " << m_ce.get_ce_name()->get_str() << ".num_edges_traversed" << m_num_edges_traversed << "\n";
//    os << "=" + prefix + "nodece_graphce " << m_graphce->graphce_get_name()->get_str() << ".num_edges_traversed" << m_num_edges_traversed << "\n";
//    m_graphce->graphce_to_stream(os);
    os << "=" + prefix + "nodece_path\n";
    //os << m_path->graph_edge_composition_to_string() << endl;
    m_path->graph_edge_composition_to_stream(os, "="+prefix+"nodece_path");
    os << "=" + prefix + "nodece_cached_counterexample\n";
    m_ce.counter_example_to_stream(os);
    os << "=" + prefix + "num_edges_traversed " << m_num_edges_traversed << "\n";
    os << "=" + prefix + "nodece_visted_pcs: " << endl;
    for (auto const& p : m_visited_pcs) {
      if ( p != this->nodece_get_last_visited_pc())
        os << p.to_string() << " -> ";
      else
        os << p.to_string() << endl;
    }

//    os << "=" + prefix + "nodece_invstate_region \n";
//    os << m_last_invstate_region.invstate_region_to_string() << "\n";
  }

  template<typename Y_PC,typename Y_N,typename Y_E>
  friend shared_ptr<nodece_t<Y_PC, Y_N, Y_E>> mk_nodece(/*dshared_ptr<graphce_t<Y_PC,Y_N,Y_E>> const &gce,*/ shared_ptr<graph_edge_composition_t<Y_PC,Y_E> const> path, counter_example_t const &ce, size_t num_edges_traversed, list<T_PC> const& visited_pcs/*, invstate_region_t<T_PC> const& last_region*/);
};
}

namespace std
{
using namespace eqspace;
template<typename T_PC, typename T_N, typename T_E>
struct hash<nodece_t<T_PC, T_N, T_E>>
{
  std::size_t operator()(nodece_t<T_PC, T_N, T_E> const &nce) const
  {
    size_t ret = 0;
    //unsigned long gce = (unsigned long)(void *)nce.get_graphce_ref().get();
//    unsigned long gce = nce.get_graphce_ref()->graphce_get_hash_code();
//    myhash_combine(ret, gce);
    myhash_combine(ret, nce.nodece_get_counter_example().get_ce_name().get());
    unsigned long pth = (unsigned long)(void *)nce.nodece_get_path().get();
    myhash_combine(ret, pth);
    myhash_combine(ret, std::hash<T_PC>()(nce.nodece_get_last_visited_pc()));
    return ret;
  }
};

}

namespace eqspace {

template<typename T_PC, typename T_N, typename T_E>
nodece_ref<T_PC, T_N, T_E>
mk_nodece(/*dshared_ptr<graphce_t<T_PC,T_N,T_E>> const &gce,*/ shared_ptr<graph_edge_composition_t<T_PC,T_E> const> path, counter_example_t const &ce, size_t num_edges_traversed, list<T_PC> const& visited_pcs/*, invstate_region_t<T_PC> const& last_region*/)
{
  nodece_t<T_PC, T_N, T_E> nce(/*gce,*/ path, ce, num_edges_traversed, visited_pcs/*, last_region*/);
  return nodece_t<T_PC, T_N, T_E>::get_nodece_manager()->register_object(nce);
}


}
