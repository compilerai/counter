#include "graph/graph_with_ce.h"
#include "graph/nodece_set.h"
#include "eq/corr_graph.h"

namespace eqspace {

template<typename T_PC, typename T_N, typename T_E>
nodece_set_ref<T_PC, T_N, T_E>
mk_nodece_set(list<nodece_ref<T_PC, T_N, T_E>> const &nodeces)
{
  nodece_set_t<T_PC, T_N, T_E> ncs(nodeces);
  return nodece_set_t<T_PC, T_N, T_E>::get_nodece_set_manager()->register_object(ncs);
}

template<typename T_PC, typename T_N, typename T_E>
nodece_set_t<T_PC, T_N, T_E>::~nodece_set_t()
{
  if (m_is_managed) {
    this->get_nodece_set_manager()->deregister_object(*this);
  }
}

template<typename T_PC, typename T_N, typename T_E>
manager<nodece_set_t<T_PC, T_N, T_E>> *
nodece_set_t<T_PC, T_N, T_E>::get_nodece_set_manager()
{
  static manager<nodece_set_t<T_PC, T_N, T_E>> *ret = NULL;
  if (!ret) {
    ret = new manager<nodece_set_t<T_PC, T_N, T_E>>;
  }
  return ret;
}

template class nodece_set_t<pcpair, corr_graph_node, corr_graph_edge>;
template nodece_set_ref<pcpair, corr_graph_node, corr_graph_edge> mk_nodece_set<pcpair, corr_graph_node, corr_graph_edge>(list<nodece_ref<pcpair, corr_graph_node, corr_graph_edge>> const &nodeces);

}
