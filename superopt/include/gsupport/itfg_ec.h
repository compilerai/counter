#pragma once

#include "support/manager.h"
#include "support/mytimer.h"
#include "support/utils.h"
#include "support/debug.h"
#include "support/types.h"
#include "support/serpar_composition.h"

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>

namespace eqspace {

class itfg_ec_node_t : public serpar_composition_node_t<itfg_edge_ref>
{
private:
  bool m_is_managed;
public:

  ~itfg_ec_node_t();

  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  bool id_is_zero() const { return !m_is_managed ; }

  bool operator==(itfg_ec_node_t const &other) const
  {
    return this->serpar_composition_node_t<itfg_edge_ref>::operator==(other);
  }
  string itfg_ec_to_string() const
  {
    std::function<string (itfg_edge_ref const &)> f = [](itfg_edge_ref const& ie)
    {
      return ie->get_from_pc().to_string() + " => " + ie->get_to_pc().to_string();
    };
    return this->to_string(f);
  }
  shared_ptr<itfg_ec_node_t const> get_a_ec() const
  {
    shared_ptr<serpar_composition_node_t<itfg_edge_ref> const> a = this->get_a();
    shared_ptr<itfg_ec_node_t const> ret = dynamic_pointer_cast<itfg_ec_node_t const>(a);
    ASSERT(ret);
    return ret;
  }
  shared_ptr<itfg_ec_node_t const> get_b_ec() const
  {
    shared_ptr<serpar_composition_node_t<itfg_edge_ref> const> b = this->get_b();
    shared_ptr<itfg_ec_node_t const> ret = dynamic_pointer_cast<itfg_ec_node_t const>(b);
    ASSERT(ret);
    return ret;
  }
  //itfg_ec_ref
  //deep_clone() const
  //{
  //  if (this->get_type() == serpar_composition_node_t::atom) {
  //    //return make_shared<itfg_ec_node_t const>(this->get_atom());
  //    return mk_itfg_ec(this->get_atom());
  //  } else {
  //    //return make_shared<itfg_ec_node_t const>(this->get_type(), this->get_a_ec(), this->get_b_ec());
  //    return mk_itfg_ec(this->get_type(), this->get_a_ec(), this->get_b_ec());
  //  }
  //}
  static shared_ptr<itfg_ec_node_t const>
  itfg_ec_node_create_from_string(string const& str, std::function<itfg_edge_ref (string const&)> f_atom);

  shared_ptr<itfg_ec_node_t const> itfg_ec_update_state(std::function<void (pc const&, state&)> update_state_fn) const;
  shared_ptr<itfg_ec_node_t const> itfg_ec_update_edgecond(std::function<expr_ref (pc const&, expr_ref const&)> update_expr_fn) const;
  shared_ptr<itfg_ec_node_t const> itfg_ec_visit_exprs(std::function<expr_ref (string const&, expr_ref)> update_expr_fn) const;

  void itfg_ec_visit_exprs_const(std::function<void (pc const&, string const&, expr_ref const&)> f) const;

  friend shared_ptr<itfg_ec_node_t const> mk_itfg_ec(serpar_composition_node_t<itfg_edge_ref>::type t, shared_ptr<itfg_ec_node_t const> const& a, shared_ptr<itfg_ec_node_t const> const& b);
  friend shared_ptr<itfg_ec_node_t const> mk_itfg_ec(itfg_edge_ref const& ie);
private:
  shared_ptr<itfg_ec_node_t const> itfg_ec_visit_nodes(std::function<itfg_edge_ref (itfg_edge_ref const &)> f) const;
  void itfg_ec_visit_nodes_const(std::function<void (itfg_edge_ref const &)> f) const;

  itfg_ec_node_t(serpar_composition_node_t<itfg_edge_ref>::type t, shared_ptr<itfg_ec_node_t const> const& a, shared_ptr<itfg_ec_node_t const> const& b) : serpar_composition_node_t<itfg_edge_ref>(t, a, b), m_is_managed(false)
  { }
  itfg_ec_node_t(itfg_edge_ref const& ie) : serpar_composition_node_t<itfg_edge_ref>(ie), m_is_managed(false)
  { }

  static manager<itfg_ec_node_t> *get_itfg_ec_manager();
};

using itfg_ec_ref = shared_ptr<itfg_ec_node_t const>;

itfg_ec_ref mk_itfg_ec(serpar_composition_node_t<itfg_edge_ref>::type t, shared_ptr<itfg_ec_node_t const> const& a, shared_ptr<itfg_ec_node_t const> const& b);
itfg_ec_ref mk_itfg_ec(itfg_edge_ref const& ie);

}

namespace std
{
using namespace eqspace;
template<>
struct hash<itfg_ec_node_t>
{
  std::size_t operator()(itfg_ec_node_t const &ec) const
  {
    serpar_composition_node_t<itfg_edge_ref> const &sd = ec;
    return hash<serpar_composition_node_t<itfg_edge_ref>>()(sd);
  }
};

}
