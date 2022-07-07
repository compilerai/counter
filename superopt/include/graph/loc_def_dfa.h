#pragma once

#include "support/types.h"
#include "support/utils.h"

#include "expr/pc.h"

#include "gsupport/tfg_edge.h"
#include "gsupport/itfg_edge.h"

#include "graph/dfa.h"

using namespace std;

namespace eqspace {

typedef set<graph_loc_id_t> defined_locs_t;

class defined_locs_val_t
{
private:
  bool m_is_top;
  defined_locs_t m_locs;
public:
  defined_locs_val_t() : m_is_top(false), m_locs() { }
  defined_locs_val_t(defined_locs_t const &l) : m_is_top(false), m_locs(l) { }

  static defined_locs_val_t
  top()
  {
    defined_locs_val_t ret;
    ret.m_is_top= true;
    return ret;
  }

  defined_locs_t const &get_defined_locs() const { ASSERT(!m_is_top || m_locs.empty()); return m_locs; }

  bool meet(defined_locs_t const& b)
  {
    if (m_is_top) {
      this->m_locs = b;
      this->m_is_top = false;
      return true;
    }
    size_t oldval_size = m_locs.size();
    set_intersect(m_locs, b);
    return oldval_size != m_locs.size();
  }

  static std::function<defined_locs_t (pc const &)>
  get_boundary_value(dshared_ptr<graph<pc, tfg_node, tfg_edge> const> g)
  {
    auto f = [g](pc const &p)
    {
      ASSERT(p.is_start());
      dshared_ptr<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const> gp = dynamic_pointer_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const>(g);
      ASSERT(gp);

      return gp->graph_get_defined_locids_at_entry(p);
    };
    return f;
  }
};

class def_analysis : public data_flow_analysis<pc, tfg_node, tfg_edge, defined_locs_val_t, dfa_dir_t::forward>
{
public:

  def_analysis(dshared_ptr<graph_with_regions_t<pc, tfg_node, tfg_edge> const> g, map<pc, defined_locs_val_t> &init_vals)
    : data_flow_analysis<pc, tfg_node, tfg_edge, defined_locs_val_t, dfa_dir_t::forward>(g, init_vals)
  {
    //defined_locs_t()
    //ASSERT((dynamic_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const*>(g) != nullptr));
  }

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<tfg_edge const> const &e, defined_locs_val_t const& in, defined_locs_val_t &out) override
  {
    autostop_timer func_timer(string("def_analysis::") + __func__);
    dshared_ptr<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const> gp = dynamic_pointer_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const>(this->m_graph);
    defined_locs_t retval = in.get_defined_locs();

    defined_locs_t locids_written = gp->get_locids_potentially_written_on_edge(e);
    set_union(retval, locids_written);
    return out.meet(retval) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

}
