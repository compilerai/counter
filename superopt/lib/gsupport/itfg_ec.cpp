#include "gsupport/itfg_edge.h"
#include "gsupport/itfg_ec.h"
#include "support/debug.h"

namespace eqspace {

itfg_ec_node_t::~itfg_ec_node_t()
{
  if (m_is_managed) {
    this->get_itfg_ec_manager()->deregister_object(*this);
  }
}

manager<itfg_ec_node_t> *
itfg_ec_node_t::get_itfg_ec_manager()
{
  static manager<itfg_ec_node_t> *ret = NULL;
  if (!ret) {
    ret = new manager<itfg_ec_node_t>;
  }
  return ret;
}

shared_ptr<itfg_ec_node_t const>
itfg_ec_node_t::itfg_ec_node_create_from_string(string const& str, std::function<itfg_edge_ref (string const&)> f_atom)
{
  string a, b;
  type typ = find_pivot(str, a, b);
  switch(typ) {
    case atom:
    {
      //T_ATOM a = T_ATOM::read_from_string(str, aux);
      //cout << __func__ << " " << __LINE__ << ": str = " << str << ".\n";
      itfg_edge_ref const& a = f_atom(str);
      //cout << __func__ << " " << __LINE__ << ": a = " << a.to_string_for_eq() << ".\n";
      //return make_shared<serpar_composition_node_t const>(a);
      //return make_shared<itfg_ec_node_t const>(a);
      return mk_itfg_ec(a);
    }
    case parallel:
    case series:
    {
      shared_ptr<itfg_ec_node_t const> e_a = itfg_ec_node_create_from_string(a, f_atom);
      shared_ptr<itfg_ec_node_t const> e_b = itfg_ec_node_create_from_string(b, f_atom);
      ASSERT(e_a && e_b);
      //return make_shared<serpar_composition_node_t const>(typ, e_a, e_b);
      //return make_shared<itfg_ec_node_t const>(typ, e_a, e_b);
      return mk_itfg_ec(typ, e_a, e_b);
    }
  }
  NOT_REACHED();
}

itfg_ec_ref
mk_itfg_ec(serpar_composition_node_t<itfg_edge_ref>::type t, shared_ptr<itfg_ec_node_t const> const& a, shared_ptr<itfg_ec_node_t const> const& b)
{
  itfg_ec_node_t ec(t, a, b);
  return itfg_ec_node_t::get_itfg_ec_manager()->register_object(ec);
}

itfg_ec_ref
mk_itfg_ec(itfg_edge_ref const& ie)
{
  itfg_ec_node_t ec(ie);
  return itfg_ec_node_t::get_itfg_ec_manager()->register_object(ec);
}

itfg_ec_ref
itfg_ec_node_t::itfg_ec_visit_nodes(std::function<itfg_edge_ref (itfg_edge_ref const &)> f) const
{
  std::function<itfg_ec_ref (itfg_edge_ref const&)> f_atom = [f](itfg_edge_ref const& e)
  {
    return mk_itfg_ec(f(e));
  };
  std::function<itfg_ec_ref (serpar_composition_node_t<itfg_edge_ref>::type, itfg_ec_ref const&, itfg_ec_ref const&)> f_fold =
    [](serpar_composition_node_t<itfg_edge_ref>::type t, itfg_ec_ref const& a, itfg_ec_ref const& b)
  {
    return mk_itfg_ec(t, a, b);
  };
  return this->visit_nodes<itfg_ec_ref>(f_atom, f_fold);
}

void
itfg_ec_node_t::itfg_ec_visit_nodes_const(std::function<void (itfg_edge_ref const &)> f) const
{
  this->visit_nodes_const(f);
}





itfg_ec_ref
itfg_ec_node_t::itfg_ec_update_state(std::function<void (pc const&, state&)> update_state_fn) const
{
  std::function<itfg_edge_ref (itfg_edge_ref const&)> f_atom = [update_state_fn](itfg_edge_ref const& e)
  {
    state st = e->get_to_state();
    update_state_fn(e->get_from_pc(), st);
    return mk_itfg_edge(e->get_from_pc(), e->get_to_pc(), st, e->get_cond()/*, state()*/, e->get_assumes(), e->get_comment());
  };
  return itfg_ec_visit_nodes(f_atom);
}

shared_ptr<itfg_ec_node_t const>
itfg_ec_node_t::itfg_ec_update_edgecond(std::function<expr_ref (pc const&, expr_ref const&)> update_expr_fn) const
{
  //ASSERT(this->get_type() == serpar_composition_node_t<itfg_edge_ref const>::atom);
  std::function<itfg_edge_ref (itfg_edge_ref const&)> f_atom = [update_expr_fn](itfg_edge_ref const& e)
  {
    expr_ref cond = e->get_cond();
    cond = update_expr_fn(e->get_from_pc(), cond);
    return mk_itfg_edge(e->get_from_pc(), e->get_to_pc(), e->get_to_state(), cond/*, state()*/, e->get_assumes(), e->get_comment());
  };
  return itfg_ec_visit_nodes(f_atom);
}

itfg_ec_ref
itfg_ec_node_t::itfg_ec_visit_exprs(std::function<expr_ref (string const&, expr_ref)> update_expr_fn) const
{
  std::function<itfg_edge_ref (itfg_edge_ref const&)> f_atom = [update_expr_fn](itfg_edge_ref const& e)
  {
    return e->itfg_edge_visit_exprs(update_expr_fn);
  };
  return itfg_ec_visit_nodes(f_atom);

}

void
itfg_ec_node_t::itfg_ec_visit_exprs_const(std::function<void (pc const&, string const&, expr_ref const&)> f) const
{
  std::function<void (itfg_edge_ref const&)> f_atom = [f](itfg_edge_ref const& e)
  {
    pc const& from_pc = e->get_from_pc();
    f(from_pc, "edgecond", e->get_cond());
    std::function<void (string const&, expr_ref)> fstate = [f, &from_pc](string const& k, expr_ref e)
    {
      f(from_pc, k, e);
    };
    e->get_to_state().state_visit_exprs(fstate);
  };
  this->itfg_ec_visit_nodes_const(f_atom);
}

}
