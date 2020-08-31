#include "gsupport/parse_edge_composition.h"
#include "gsupport/pc.h"
#include "gsupport/graph_ec.h"
#include "graph/edge_id.h"
#include "gsupport/tfg_edge.h"
#include "gsupport/corr_graph_edge.h"
#include "support/debug.h"

namespace eqspace {

graph_edge_composition_ref<pc,edge_id_t<pc>> parse_tfg_edge_composition(const char* str);
graph_edge_composition_ref<pcpair,edge_id_t<pcpair>> parse_cg_edge_composition(const char* str);

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E>
parse_edge_composition(const char* str)
{
  //TODO: because I could not figure a way to templatize the Bison specs, I am doing two separate (near duplicate) implementations of the parser for now. Fix this whenever possible.
  if (std::is_same<T_PC, pc>::value) {
    //cout << __func__ << " " << __LINE__ << ": str = '" << str << "'\n";
    graph_edge_composition_ref<pc,edge_id_t<pc>> tfg_ec = parse_tfg_edge_composition(str);
    graph_edge_composition_ref<T_PC,T_E> ret = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(tfg_ec);
    return ret;
  } else if (std::is_same<T_PC, pcpair>::value) {
    graph_edge_composition_ref<pcpair,edge_id_t<pcpair>> cg_ec = parse_cg_edge_composition(str);
    graph_edge_composition_ref<T_PC,T_E> ret = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(cg_ec);
    return ret;
  } else NOT_REACHED();
}

template<typename T_PC, typename T_E>
string
graph_edge_composition_t<T_PC,T_E>::graph_edge_composition_from_stream(istream& in, string const& first_line, string const& prefix/*, state const& start_state*/, context* ctx, graph_edge_composition_ref<T_PC,T_E>& ec)
{
  string line;
  bool end;

  ASSERTCHECK((first_line == prefix + ".graph_edge_composition"),  cout << "first_line = " << first_line << ", prefix = " << prefix << endl);
  end = !getline(in, line);
  ASSERT(!end);
  graph_edge_composition_ref<T_PC,edge_id_t<T_PC>> ecid = parse_edge_composition<T_PC,edge_id_t<T_PC>>(line.c_str());
  ASSERT(ecid);
  end = !getline(in, line);
  ASSERT(!end);
  if (ecid->is_epsilon()) {
    ec = mk_epsilon_ec<T_PC,T_E>();
    return line;
  }
  ASSERT(is_line(line, prefix + ".graph_edge_composition.Edge"));

  //vector<shared_ptr<T_E const>> edges;
  map<edge_id_t<T_PC>, shared_ptr<T_E const>> edges;
  while (is_line(line, prefix + ".graph_edge_composition.Edge")) {
    bool end = !getline(in, line);
    ASSERT(!end);
    shared_ptr<T_E const> e;
    if (line == string("(") + EPSILON + ")") {
      //ASSERT (te->is_empty());
      e = T_E::empty();
      getline(in, line);
      edges.insert(make_pair(edge_id_t<T_PC>::empty(), e));
    } else {
      line = T_E::graph_edge_from_stream(in, line, prefix + ".Edge"/*, start_state*/, ctx, e);
      edges.insert(make_pair(edge_id_t<T_PC>(e->get_from_pc(), e->get_to_pc()), e));
    }
  }

  //function<void (shared_ptr<edge_id_t<T_PC> const> const&)> atom_f = [&in,&prefix,&line,&edges/*,&start_state*/,ctx](shared_ptr<edge_id_t<T_PC> const> const& te) {
  //  ASSERTCHECK(is_line(line, prefix + ".graph_edge_composition.Edge"), cout << "line = " << line << ", prefix = " << prefix << endl);
  //  bool end = !getline(in, line);
  //  ASSERT(!end);
  //  shared_ptr<T_E const> e;
  //  if (line == string("(") + EPSILON + ")") {
  //    ASSERT (te->is_empty());
  //    e = T_E::empty();
  //    getline(in, line);
  //  } else {
  //    line = T_E::graph_edge_from_stream(in, line, prefix + ".Edge"/*, start_state*/, ctx, e);
  //  }
  //  //edges.push_back(e);
  //  edges.insert(make_pair(edge_id_t<T_PC>(e->get_from_pc(), e->get_to_pc()), e));
  //};
  //ecid->visit_nodes_const(atom_f);

  //auto itr = edges.begin();
  function<graph_edge_composition_ref<T_PC,T_E> (shared_ptr<edge_id_t<T_PC> const> const&, graph_edge_composition_ref<T_PC,T_E> const&)> atom_f2 = [&edges](shared_ptr<edge_id_t<T_PC> const> const& eid, graph_edge_composition_ref<T_PC,T_E> const& preorder_val) {
    //shared_ptr<T_E const> e = *itr;
    //++itr;
    //ASSERT(e->get_from_pc() == eid->get_from_pc());
    //ASSERT(e->get_to_pc() == eid->get_to_pc());
    ASSERT(edges.count(*eid));
    return mk_series<T_PC,T_E>(preorder_val, mk_edge_composition<T_PC,T_E>(edges.at(*eid)));
  };
  function<graph_edge_composition_ref<T_PC,T_E> (graph_edge_composition_ref<T_PC,T_E> const&, graph_edge_composition_ref<T_PC,T_E> const&)> fold_f = [](graph_edge_composition_ref<T_PC,T_E> const& a, graph_edge_composition_ref<T_PC,T_E> const& b) {
    return mk_parallel<T_PC,T_E>(a,b);
  };
  ec = ecid->template visit_nodes_preorder_series<graph_edge_composition_ref<T_PC,T_E>>(atom_f2, fold_f, mk_epsilon_ec<T_PC,T_E>());
  return line;
}

template graph_edge_composition_ref<pc,edge_id_t<pc>> parse_edge_composition(char const *str);
template graph_edge_composition_ref<pcpair,edge_id_t<pcpair>> parse_edge_composition(char const *str);

template string graph_edge_composition_t<pc,tfg_edge>::graph_edge_composition_from_stream(istream& in, string const& first_line, string const& prefix/*, state const& start_state*/, context* ctx, graph_edge_composition_ref<pc,tfg_edge>& ec);
template string graph_edge_composition_t<pcpair,corr_graph_edge>::graph_edge_composition_from_stream(istream& in, string const& first_line, string const& prefix/*, state const& start_state*/, context* ctx, graph_edge_composition_ref<pcpair,corr_graph_edge>& ec);

}
