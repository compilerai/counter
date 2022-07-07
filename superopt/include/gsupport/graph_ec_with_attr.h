#pragma once

#include "support/manager.h"
#include "support/mytimer.h"
#include "support/utils.h"
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/types.h"
#include "support/serpar_composition.h"

#include "expr/defs.h"

#include "gsupport/te_comment.h"
#include "gsupport/edge_with_unroll.h"
#include "gsupport/alloc_dealloc.h"
#include "gsupport/graph_ec.h"

namespace eqspace {

template <typename T_E, typename T_ATTR>
class edge_with_attr_t
{
private:
  T_E const& m_e;
  T_ATTR m_attr;
public:
  edge_with_attr_t(T_E const& e, T_ATTR const& attr) : m_e(e), m_attr(attr)
  { }
  T_E const& get_edge() const { return m_e; }
  T_ATTR const& get_attr() const { return m_attr; }
  bool is_empty() const { return m_e.is_empty(); }
  string to_string_concise() const { return m_e.to_string_concise(); }
};

template <typename T_PC, typename T_E, typename T_ATTR>
class graph_ec_with_attr_t : public graph_edge_composition_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>
{
private:
  bool m_is_managed = false;
private:
  /*graph_ec_with_attr_t(dshared_ptr<T_E const> const& e, T_ATTR const& attr)
    : graph_edge_composition_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>(edge_with_attr_t<T_E,T_ATTR>(e, attr))
  { }*/
  //graph_ec_with_attr_t(graph_edge_composition_t<T_PC,edge_with_attr_t<T_E,T_ATTR>> const& ec) : graph_edge_composition_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>(ec)
  //{ }
  graph_ec_with_attr_t(dshared_ptr<edge_with_attr_t<T_E,T_ATTR> const> const &e) : graph_edge_composition_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>(e)
  { }
  graph_ec_with_attr_t(enum serpar_composition_node_t<edge_with_unroll_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>>::type t, shared_ptr<graph_ec_with_attr_t const> const &a, shared_ptr<graph_ec_with_attr_t const> const &b) : graph_edge_composition_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>(t, a, b)
  { }

public:
  static shared_ptr<graph_ec_with_attr_t const> mk_graph_ec_with_attr(dshared_ptr<T_E const> const& e, T_ATTR const& attr)
  {
    graph_ec_with_attr_t ec(make_dshared<edge_with_attr_t<T_E,T_ATTR> const>(*e, attr));
    return graph_ec_with_attr_t<T_PC,T_E,T_ATTR>::get_ec_with_attr_manager()->register_object(ec);
    //return graph_edge_composition_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>::mk_edge_composition(make_dshared<edge_with_attr_t<T_E,T_ATTR>>(*e, attr));
  }

  static shared_ptr<graph_ec_with_attr_t const> graph_ec_with_attr_mk_parallel(shared_ptr<graph_ec_with_attr_t const> const& a, shared_ptr<graph_ec_with_attr_t const> const& b)
  {
    graph_ec_with_attr_t ec(serpar_composition_node_t<edge_with_unroll_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>>::parallel, a, b);
    return graph_ec_with_attr_t<T_PC,T_E,T_ATTR>::get_ec_with_attr_manager()->register_object(ec);
  }

  static shared_ptr<graph_ec_with_attr_t const> graph_ec_with_attr_mk_series(shared_ptr<graph_ec_with_attr_t const> const& a, shared_ptr<graph_ec_with_attr_t const> const& b)
  {
    graph_ec_with_attr_t ec(serpar_composition_node_t<edge_with_unroll_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>>::series, a, b);
    return graph_ec_with_attr_t<T_PC,T_E,T_ATTR>::get_ec_with_attr_manager()->register_object(ec);
  }

  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  bool id_is_zero() const { return !m_is_managed ; }
  ~graph_ec_with_attr_t()
  {
    if (m_is_managed) {
      this->get_ec_with_attr_manager()->deregister_object(*this);
    }
  }

private:
  static manager<graph_ec_with_attr_t<T_PC,T_E,T_ATTR>> *get_ec_with_attr_manager()
  {
    static manager<graph_ec_with_attr_t<T_PC,T_E,T_ATTR>> *ret = NULL;
    if (!ret) {
      ret = new manager<graph_ec_with_attr_t<T_PC,T_E,T_ATTR>>;
    }
    return ret;
  }
};

}

namespace std
{
using namespace eqspace;
template<typename T_PC, typename T_E, typename T_ATTR>
struct hash<graph_ec_with_attr_t<T_PC,T_E,T_ATTR>>
{
  std::size_t operator()(graph_ec_with_attr_t<T_PC,T_E,T_ATTR> const &s) const
  {
    graph_edge_composition_t<T_PC,edge_with_attr_t<T_E,T_ATTR>> const &sd = s;
    return hash<graph_edge_composition_t<T_PC,edge_with_attr_t<T_E,T_ATTR>>>()(sd);
  }
};

}
