#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "support/utils.h"
#include "graph/dfa.h"
#include "gsupport/pc.h"
#include "gsupport/tfg_edge.h"
#include "gsupport/itfg_edge.h"

using namespace std;

namespace eqspace {

typedef set<graph_loc_id_t> defined_locs_t;

class defined_locs_val_t
{
private:
  defined_locs_t m_locs;
public:
  defined_locs_val_t() : m_locs() { }
  defined_locs_val_t(defined_locs_t const &l) : m_locs(l) { }

  static defined_locs_val_t
  top()
  {
    return defined_locs_val_t();
  }

  defined_locs_t const &get_defined_locs() const { return m_locs; }

  bool meet(defined_locs_t const& b)
  {
    defined_locs_t oldval = m_locs;
    set_union(m_locs, b);
    return oldval != m_locs;
  }

  static std::function<defined_locs_t (pc const &)>
  get_boundary_value(graph<pc, tfg_node, tfg_edge> const *g)
  {
    auto f = [g](pc const &p)
    {
      ASSERT(p.is_start());
      graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate>const* gp = dynamic_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const*>(g);

      // add input args
      defined_locs_t retval = gp->get_argument_regs_locids();
      map<graph_loc_id_t, graph_cp_location> const &locs = gp->get_locs();
      for (auto const& l : locs) {
        if (   l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT
            || l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
          if (   !l.second.m_memlabel->get_ml().memlabel_is_stack()
              && !l.second.m_memlabel->get_ml().memlabel_is_local()) {
            retval.insert(l.first);
          }
        }
      }
      // add regs live at exit minus the ret-reg
      //set_union(retval, gp->get_return_regs_locids());
      //graph_loc_id_t ret_reg_locid = gp->get_ret_reg_locid();
      //if (ret_reg_locid != -1)
      //  retval.erase(ret_reg_locid);

      //cout << "def_analysis::" << __func__ << ':' << __LINE__ << ": locids : " << set_to_string(retval) << endl;
      return retval;
    };
    return f;
  }
};

class def_analysis : public data_flow_analysis<pc, tfg_node, tfg_edge, defined_locs_val_t, dfa_dir_t::forward>
{
public:

  def_analysis(graph<pc, tfg_node, tfg_edge> const* g, map<pc, defined_locs_val_t> &init_vals)
    : data_flow_analysis<pc, tfg_node, tfg_edge, defined_locs_val_t, dfa_dir_t::forward>(g, init_vals)
  {
    //defined_locs_t()
    ASSERT((dynamic_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const*>(g) != nullptr));
  }

  virtual bool xfer_and_meet(shared_ptr<tfg_edge const> const &e, defined_locs_val_t const& in, defined_locs_val_t &out) override
  {
    autostop_timer func_timer(__func__);
    graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate>const* gp = dynamic_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const*>(this->m_graph);
    defined_locs_t retval = in.get_defined_locs();

    defined_locs_t locids_written = gp->get_locids_potentially_written_on_edge(e);
    set_union(retval, locids_written);
    return out.meet(retval);
  }
};

}
