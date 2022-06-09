#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <list>
#include <queue>
#include <set>
#include <map>
#include <memory>

#include "ptfg/ftmap_dfa_flow_insensitive.h"

using namespace std;

namespace eqspace {

template<typename T_VAL, dfa_dir_t DIR>
class ftmap_dfa_t : public ftmap_dfa_flow_insensitive_t<map<pc, T_VAL>, DIR, dfa_state_type_t::per_pc>
{
protected:
  //using summary_function_t = std::function<dfa_xfer_retval_t (dshared_ptr<tfg_edge const> const&, T_VAL const&, T_VAL&)>;

public:
  ftmap_dfa_t(dshared_ptr<ftmap_t const> ftmap, int max_call_context_depth, map<call_context_ref, map<pc, T_VAL>>& init_vals) : ftmap_dfa_flow_insensitive_t<map<pc, T_VAL>, DIR, dfa_state_type_t::per_pc>(ftmap, max_call_context_depth, init_vals)
  { }

private: //virtual functions

  virtual dfa_xfer_retval_t ftmap_dfa_xfer_and_meet(call_context_ref const& cc, dshared_ptr<tfg_edge const> const &e, T_VAL const& in, T_VAL &out) = 0;

  virtual dfa_xfer_retval_t ftmap_dfa_update_callee_boundary_value(call_context_ref const& caller_context, dshared_ptr<tfg_edge const> const& fcall_edge, call_context_ref const& callee_context, map<pc, T_VAL> const& in, map<pc, T_VAL>& callee_vals/*, pc const& callee_boundary_pc*/) override
  {
    pc const& caller_boundary_pc = (DIR == dfa_dir_t::forward) ? fcall_edge->get_from_pc() : fcall_edge->get_to_pc();
    pc callee_boundary_pc = (DIR == dfa_dir_t::forward) ? pc::start() : pc(pc::exit);
    //cout << _FNLN_ << ": callee_context = " << callee_context->call_context_to_string() << ", boundary_pc = " << boundary_pc.to_string() << "\n";
    if (!callee_vals.count(callee_boundary_pc)) {
      callee_vals.insert(make_pair(callee_boundary_pc, in.at(caller_boundary_pc)));
      return DFA_XFER_RETVAL_CHANGED;
    }
    ////return callee_vals.at(boundary_pc).meet(in);
    return ftmap_dfa_xfer_and_meet(nullptr, dshared_ptr<tfg_edge const>::dshared_nullptr(), in.at(caller_boundary_pc), callee_vals.at(callee_boundary_pc));
  }

  virtual dfa_xfer_retval_t ftmap_dfa_update_caller_return_value(call_context_ref const& callee_context, dshared_ptr<tfg_edge const> const& fcall_edge, map<pc, T_VAL> const& in, map<pc, T_VAL>& out) override
  {
    //this should be called only if a summary does not exist
    //ASSERT(m_summaries.count(callee_context));
    //ASSERT(!m_summaries.at(callee_context));

    //pc out_pc = (DIR == dfa_dir_t::forward) ? pc(pc::exit) : pc::start();

    //if (!callee_vals.count(out_pc)) {
    //  return DFA_XFER_RETVAL_UNCHANGED;
    //}
    //T_VAL const& exitval = callee_vals.at(out_pc);

    pc retval_pc = (DIR == dfa_dir_t::forward) ? pc(pc::exit) : pc::start();
    pc caller_pc = (DIR == dfa_dir_t::forward) ? fcall_edge->get_to_pc() : fcall_edge->get_from_pc();
    if (!in.count(retval_pc)) {
      return DFA_XFER_RETVAL_UNCHANGED;
    }
    return ftmap_dfa_xfer_and_meet(nullptr, dshared_ptr<tfg_edge const>::dshared_nullptr(), in.at(retval_pc), out.at(caller_pc));
  }

private:
  dfa_xfer_retval_t ftmap_dfa_flow_insensitive_xfer_and_meet(call_context_ref const& cc, dshared_ptr<tfg_edge const> const &e, map<pc, T_VAL> &in_out) override
  {
    pc const& from_pc = (DIR == dfa_dir_t::forward) ? e->get_from_pc() : e->get_to_pc();
    pc const& to_pc = (DIR == dfa_dir_t::forward) ? e->get_to_pc() : e->get_from_pc();
    if (!in_out.count(from_pc)) {
      auto ret = in_out.insert(make_pair(from_pc, T_VAL::top()));
      //cout << __func__ << " " << __LINE__ << ": adding value at " << from_pc.to_string() << endl;
      ASSERT(ret.second);
    }
    if (!in_out.count(to_pc)) {
      auto ret = in_out.insert(make_pair(to_pc, T_VAL::top()));
      //cout << __func__ << " " << __LINE__ << ": adding value at " << to_pc.to_string() << endl;
      ASSERT(ret.second);
    }
    ASSERT(in_out.count(from_pc));
    ASSERT(in_out.count(to_pc));

    return this->ftmap_dfa_xfer_and_meet(cc, e, in_out.at(from_pc), in_out.at(to_pc));
  }

  //summary_function_t get_summary_function(call_context_ref const& caller_context, call_context_ref const& callee_context)
  //{
  //  return compute_summary_function(caller_context, callee_context);

  //  //Do not cache the summaries because they change over time
  //  //if (!m_summaries.count(callee_context)) {
  //  //  m_summaries.insert(make_pair(callee_context, compute_summary_function(caller_context, callee_context)));
  //  //}
  //  //return m_summaries.at(callee_context);
  //}
};

}
