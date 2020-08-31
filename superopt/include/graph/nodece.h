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
  graphce_ref<T_PC,T_N,T_E> m_graphce;
  shared_ptr<graph_edge_composition_t<T_PC,T_E> const> m_path;
  bool m_is_managed;
  counter_example_t m_ce;

public:

  nodece_t(shared_ptr<graphce_t<T_PC,T_N,T_E>> const &gce, shared_ptr<graph_edge_composition_t<T_PC,T_E> const> const &path, counter_example_t const &ce) : m_graphce(gce), m_path(path), m_is_managed(false), m_ce(ce)
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

  graphce_ref<T_PC,T_N,T_E> get_graphce_ref() const { return m_graphce; }

  shared_ptr<graph_edge_composition_t<T_PC,T_E> const> const &nodece_get_path() const { return m_path; }

  template<typename T_PRED>
  counter_example_t const &
  nodece_get_counter_example(graph_with_ce<T_PC, T_N, T_E, T_PRED> const *g) const
  {
    CPP_DBG_EXEC2(ASSERTCHECKS, 
        ASSERT(this->nodece_compute_counter_example(g).counter_example_equals(m_ce));
    );
    return m_ce;
  }

  template<typename T_PRED>
  counter_example_t
  nodece_compute_counter_example(graph_with_ce<T_PC, T_N, T_E, T_PRED> const *g) const
  {
    context *ctx = g->get_context();
    counter_example_t ret(ctx);
    counter_example_t rand_values(ctx);
    counter_example_t in = m_graphce->get_counter_example();
    CPP_DBG_EXEC4(CE_ADD, cout << __func__ << " " << __LINE__ << ": in =\n" << in.counter_example_to_string() << endl);
    bool success = g->counter_example_translate_on_edge_composition(in, m_path, ret, rand_values, g->get_relevant_memlabels()).first;
    ASSERT(success);
    ASSERT(rand_values.is_empty());
    CPP_DBG_EXEC4(CE_ADD, cout << __func__ << " " << __LINE__ << ": ret =\n" << ret.counter_example_to_string() << endl);
    return ret;
  }

  bool operator==(nodece_t const &other) const
  {
    return m_graphce == other.m_graphce && m_path == other.m_path && m_ce.counter_example_equals(other.m_ce);
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

  static string nodece_from_stream(istream& is/*, state const& start_state*/, context* ctx, string const& prefix, nodece_ref<T_PC, T_N, T_E>& nce)
  {
    string line;
    bool end;
    end = !getline(is, line);
    ASSERT(!end);
    if (!is_line(line, "=" + prefix + "nodece_graphce")) {
      cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
      cout << __func__ << " " << __LINE__ << ": prefix = '" << prefix << "'\n";
    }
    ASSERT(is_line(line, "=" + prefix + "nodece_graphce"));
    graphce_ref<T_PC, T_N, T_E> gce;
    line = graphce_t<T_PC, T_N, T_E>::graphce_from_stream(is, ctx, gce);
    ASSERT(gce);
    ASSERT(is_line(line, "=" + prefix + "nodece_path"));
    end = !getline(is, line);
    ASSERT(!end);
    //cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
    //shared_ptr<graph_edge_composition_t<T_PC,T_E> const> path = parse_edge_composition<T_PC,T_E>(line.c_str());
    //end = !getline(is, line);
    //ASSERT(!end);
    shared_ptr<graph_edge_composition_t<T_PC,T_E> const> path;
    line = graph_edge_composition_t<T_PC,T_E>::graph_edge_composition_from_stream(is, line, "=" + prefix + "nodece_path"/*, start_state*/, ctx, path);
    ASSERT(is_line(line, "=" + prefix + "nodece_cached_counterexample"));
    counter_example_t ce(ctx);
    ce.counter_example_from_stream(is);
    nce = mk_nodece(gce, path, ce);
    end = !getline(is, line);
    ASSERT(!end);
    return line;
  }

  void nodece_to_stream(ostream& os, string const& prefix) const
  {
    os << "=" + prefix + "nodece_graphce\n";
    m_graphce->graphce_to_stream(os);
    os << "=" + prefix + "nodece_path\n";
    //os << m_path->graph_edge_composition_to_string() << endl;
    os << m_path->graph_edge_composition_to_string_for_eq("="+prefix+"nodece_path");
    os << "=" + prefix + "nodece_cached_counterexample\n";
    m_ce.counter_example_to_stream(os);
  }

  template<typename Y_PC,typename Y_N,typename Y_E>
  friend shared_ptr<nodece_t<Y_PC, Y_N, Y_E>> mk_nodece(shared_ptr<graphce_t<Y_PC,Y_N,Y_E>> const &gce, shared_ptr<graph_edge_composition_t<Y_PC,Y_E> const> path, counter_example_t const &ce);
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
    unsigned long gce = (unsigned long)(void *)nce.get_graphce_ref().get();
    myhash_combine(ret, gce);
    unsigned long pth = (unsigned long)(void *)nce.nodece_get_path().get();
    myhash_combine(ret, pth);
    return ret;
  }
};

}

namespace eqspace {

template<typename T_PC, typename T_N, typename T_E>
nodece_ref<T_PC, T_N, T_E>
mk_nodece(shared_ptr<graphce_t<T_PC,T_N,T_E>> const &gce, shared_ptr<graph_edge_composition_t<T_PC,T_E> const> path, counter_example_t const &ce)
{
  nodece_t<T_PC, T_N, T_E> nce(gce, path, ce);
  return nodece_t<T_PC, T_N, T_E>::get_nodece_manager()->register_object(nce);
}


}
