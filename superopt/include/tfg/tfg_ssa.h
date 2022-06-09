#pragma once

#include "tfg/tfg.h"

namespace eqspace {

using namespace std;

class tfg_ssa_t
{
private:
  dshared_ptr<tfg> m_tfg;

public:

//  tfg_ssa_t(tfg const& t) : m_tfg((t.construct_ssa_graph()))
//  { }

  static dshared_ptr<tfg_ssa_t> tfg_ssa_construct_from_an_already_ssa_tfg(dshared_ptr<tfg> const& t);

  static dshared_ptr<tfg_ssa_t> tfg_ssa_construct_from_non_ssa_tfg(dshared_ptr<tfg const> const& t, dshared_ptr<tfg const> const& src_tfg);

  static dshared_ptr<tfg_ssa_t> tfg_ssa_from_stream(istream& in, context* ctx);

  tfg_ssa_t(tfg_ssa_t const& other) : m_tfg(other.m_tfg)
  { }

  context * get_context() const;

  void graph_to_stream(ostream& os, string const& prefix="") const;

  dshared_ptr<tfg> get_ssa_tfg() const
  {
    return m_tfg;
  }


private:

 // constructor for creating SSA from an already SSA TFG after adding non-det cloned edges
  tfg_ssa_t(dshared_ptr<tfg> const& t) : m_tfg(t)
  {
    ASSERT(t->get_context()->get_config().disable_SSA || t->graph_is_ssa());
  }

  //XXX: Can optimize the construct_ssa_graph to use the copy constructor if no new phi edges are added
  tfg_ssa_t(dshared_ptr<tfg const> t, dshared_ptr<tfg const> const& src_tfg) : m_tfg(t->construct_ssa_graph(src_tfg))
  { }

  tfg_ssa_t(istream& in, context* ctx);

  friend class make_dshared_enabler_for_tfg_ssa_t;
  //friend class corr_graph;
};

struct make_dshared_enabler_for_tfg_ssa_t : public tfg_ssa_t { template <typename... Args> make_dshared_enabler_for_tfg_ssa_t(Args &&... args):tfg_ssa_t(std::forward<Args>(args)...) {} };

}
