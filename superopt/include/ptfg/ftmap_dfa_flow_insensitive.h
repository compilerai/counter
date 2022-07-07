#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <list>
#include <queue>
#include <set>
#include <map>
#include <memory>

#include "support/dyn_debug.h"
#include "support/dshared_ptr.h"

#include "expr/call_context.h"

#include "graph/dfa_types.h"
#include "graph/dfa_flow_insensitive.h"

#include "ptfg/ftmap.h"
#include "ptfg/ftmap_call_graph.h"
//#include "ptfg/ftmap_add_context.h"
//#include "ptfg/ftmap_dfa_val.h"

using namespace std;

namespace eqspace {

template<typename T_VAL, dfa_dir_t DIR, dfa_state_type_t DFA_STATE_TYPE>
class ftmap_dfa_flow_insensitive_t
{
protected:
  using summary_function_t = std::function<dfa_xfer_retval_t (dshared_ptr<tfg_edge const> const&, T_VAL&)>;
private:
  map<call_context_ref, T_VAL>& m_vals;
  int m_max_call_context_depth;
  //map<call_context_ref, summary_function_t> m_summaries;
protected:
  dshared_ptr<ftmap_t const> m_ftmap;

public:
  ftmap_dfa_flow_insensitive_t(ftmap_dfa_flow_insensitive_t const& other) = delete;

  ftmap_dfa_flow_insensitive_t(dshared_ptr<ftmap_t const> ftmap, int max_call_context_depth, map<call_context_ref, T_VAL>& init_vals) : m_vals(init_vals), m_max_call_context_depth(max_call_context_depth), m_ftmap(ftmap)
  { }

  bool ftmap_dfa_solve(/*call_context_ref const& cc*/)
  {
    ASSERT(this->m_ftmap);
    dshared_ptr<ftmap_call_graph_t> const& call_graph = this->m_ftmap->get_call_graph();

    map<call_context_ref, dshared_ptr<tfg_ssa_t>> const& function_tfg_map = m_ftmap->get_function_tfg_map();

    list<ftmap_call_graph_pc_t> start_pcs;
    if (DIR == dfa_dir_t::forward) {
      auto es = call_graph->get_edges_outgoing(ftmap_call_graph_pc_t::start());
      for (auto const& e : es) {
        ASSERT(e->get_to_pc().get_pc().is_start());
        start_pcs.push_back(e->get_to_pc());
      }
    } else {
      NOT_IMPLEMENTED();
    }
    bool ret = false;
    for (auto const& startpc : start_pcs) {
      call_context_ref cc = startpc.get_call_context();
      ASSERT(function_tfg_map.count(cc));
      dshared_ptr<tfg> ftfg = function_tfg_map.at(cc)->get_ssa_tfg();
      if (!this->m_vals.count(cc)) {
        /*bool inserted = m_vals.insert(make_pair(cc, T_VAL())).second;
        ASSERT(inserted);*/
        ftmap_dfa_init_vals_at_entry(cc, this->m_vals/*.at(cc)*/);
        ASSERT(this->m_vals.count(cc));

        fdfa_t fdfa(ftfg, *this, cc);
        bool fret = fdfa.solve();
        ret = ret || fret;
      }
    }
    return ret;
  }

private: //virtual functions

  virtual dfa_xfer_retval_t ftmap_dfa_flow_insensitive_xfer_and_meet(call_context_ref const& cc, dshared_ptr<tfg_edge const> const &e, T_VAL &in_out) = 0;

  virtual void ftmap_dfa_init_vals_at_entry(call_context_ref const& cc, map<call_context_ref, T_VAL>& out) = 0;

  virtual dfa_xfer_retval_t ftmap_dfa_update_callee_boundary_value(call_context_ref const& caller_context, dshared_ptr<tfg_edge const> const& fcall_edge, call_context_ref const& callee_context, T_VAL const& in, T_VAL& callee_vals/*, pc const& boundary_pc*/) = 0;

  virtual dfa_xfer_retval_t ftmap_dfa_update_caller_return_value(call_context_ref const& callee_context, call_context_ref const& caller_context, dshared_ptr<tfg_edge const> const& fcall_edge, T_VAL const& in, T_VAL& out) = 0;

  call_context_ref ftmap_dfa_identify_callee_context_and_tfg(call_context_ref const& caller_cc, pc const& caller_pc, string const& callee_name, dshared_ptr<tfg>& ftfg)
  {
    call_context_ref ret = call_context_t::call_context_append_with_bound(caller_cc, caller_pc, callee_name, m_max_call_context_depth);

    map<call_context_ref, dshared_ptr<tfg_ssa_t>> const& function_tfg_map = m_ftmap->get_function_tfg_map();

    call_context_ref cc = call_context_t::empty_call_context(ret->call_context_get_cur_fname()->get_str());

    if (!function_tfg_map.count(cc)) {
      return nullptr;
    }
    ftfg = function_tfg_map.at(cc)->get_ssa_tfg();

    return ret;
  }

  virtual summary_function_t compute_summary_function(call_context_ref const& caller_context, call_context_ref const& callee_context)
  {
    //return the empty function by default (no summaries)
    return summary_function_t();
  }

private:
  //summary_function_t get_summary_function(call_context_ref const& caller_context, call_context_ref const& callee_context)
  //{
  //  return compute_summary_function(caller_context, callee_context);

  //  //Do not cache the summaries because they change over time
  //  //if (!m_summaries.count(callee_context)) {
  //  //  m_summaries.insert(make_pair(callee_context, compute_summary_function(caller_context, callee_context)));
  //  //}
  //  //return m_summaries.at(callee_context);
  //}

  dfa_xfer_retval_t
  xfer_across_fcall(call_context_ref const& caller_context, call_context_ref const& callee_context, dshared_ptr<tfg> const& callee_tfg, dshared_ptr<tfg_edge const> const& e/*, T_VAL const& in*/, T_VAL& in_out)
  {
    //pc boundary_pc = (DIR == dfa_dir_t::forward) ? pc::start() : pc(pc::exit);
    call_context_ref callee_rcc = this->m_ftmap->ftmap_identify_most_relevant_call_context(callee_context);

    //bool first_visit_to_callee_rcc = false;
    if (!this->m_vals.count(callee_rcc)) {
      this->m_vals.insert(make_pair(callee_rcc, T_VAL::top()));
      //ftmap_dfa_init_vals_at_entry(callee_context, this->m_vals);
      //ASSERT(this->m_vals.count(callee_rcc));
      //first_visit_to_callee_rcc = true;
    }

    dfa_xfer_retval_t changed = this->ftmap_dfa_update_callee_boundary_value(caller_context, e, callee_context, in_out, this->m_vals.at(callee_rcc)/*, boundary_pc*/);

    if (   changed == DFA_XFER_RETVAL_CHANGED/*
        || first_visit_to_callee_context*/) {
      ftmap_dfa_run_till_call_graph_loop_fixpoint_starting_at_loop_head(callee_context, callee_tfg);
    }

    summary_function_t summary_fn = this->compute_summary_function(caller_context, callee_context);
    if (summary_fn) {
      DYN_DEBUG(ftmap_pointsto_debug, cout << _FNLN_ << ": caller " << caller_context->call_context_to_string() << ": callee " << callee_context->call_context_to_string() << ": calling the computed summary function across edge " << e->to_string_concise() << endl);
      return summary_fn(e/*, in*/, in_out);
    } else {
      ASSERT(this->m_vals.count(callee_rcc));
      //pc retval_pc = (DIR == dfa_dir_t::forward) ? pc(pc::exit) : pc::start();
      auto const& callee_vals = this->m_vals.at(callee_rcc);
      /*if (callee_vals.count(retval_pc)) */{
        //auto const& retval = callee_vals.at(retval_pc);
        return this->ftmap_dfa_update_caller_return_value(callee_context, caller_context, e, callee_vals/*retval*/, in_out);
      /*} else {
        return DFA_XFER_RETVAL_UNCHANGED;
      */}
    }
  }

  void ftmap_dfa_run_till_call_graph_loop_fixpoint_starting_at_loop_head(call_context_ref const& cc, dshared_ptr<tfg> const& t)
  {
    call_context_ref rcc = this->m_ftmap->ftmap_identify_most_relevant_call_context(cc);
    do {
      fdfa_t fdfa(t, *this, cc);
      fdfa.solve();
      //for each call-graph retreating-edge to rcc:
      // - run fdfa on the function containing the retreating-edge fcall with the fcall from-pc as the start
      //   - for summary-based DFAs, this would re-compute and re-execute the summary function
      //   - for non-summary-based DFAs, this would cause the transfer to the recursion-head and the fcall's to-pc
      //XXX: TODO: FIXME: This needs to be done for soundness!
    } while (false /*the re-execution of the summary function returned DFA_XFER_RETVAL_CHANGED*/);
  }

private:
  class fdfa_t : public dfa_flow_insensitive<pc, tfg_node, tfg_edge, T_VAL, DIR, DFA_STATE_TYPE>
  {
  private:
    ftmap_dfa_flow_insensitive_t& m_ftmap_dfa;
    dshared_ptr<tfg> m_tfg;
    call_context_ref m_cc;
  public:
    fdfa_t(dshared_ptr<tfg> g/*, string_ref const& fname*/, ftmap_dfa_flow_insensitive_t& ftmap_dfa, call_context_ref cc/*, map<call_context_ref, map<pc, T_VAL>>& ftmap_vals*/) : dfa_flow_insensitive<pc, tfg_node, tfg_edge, T_VAL, DIR, DFA_STATE_TYPE>(g, ftmap_dfa.m_vals[ftmap_dfa.m_ftmap->ftmap_identify_most_relevant_call_context(cc)]), m_ftmap_dfa(ftmap_dfa), m_tfg(g)/*, m_fname(fname)*/, m_cc(cc)
    { }

    dfa_xfer_retval_t xfer_and_meet_flow_insensitive(dshared_ptr<tfg_edge const> const &e/*, T_VAL const& in*/, T_VAL &in_out) override
    {
      nextpc_id_t nextpc_num;
      if (e->is_fcall_edge(nextpc_num)) {
        ASSERT(m_tfg);
        map<nextpc_id_t, string> const& nextpc_map = m_tfg->get_nextpc_map();
        if (nextpc_map.count(nextpc_num)) {
          string const& callee_name = nextpc_map.at(nextpc_num);

          call_context_ref  callee_context;
          dshared_ptr<tfg> ftfg;
          if (callee_context = m_ftmap_dfa.ftmap_dfa_identify_callee_context_and_tfg(m_cc, e->get_from_pc(), callee_name, ftfg)) {
            return m_ftmap_dfa.xfer_across_fcall(m_cc, callee_context, ftfg, e/*, in*/, in_out);
          }
        }
      }
      return m_ftmap_dfa.ftmap_dfa_flow_insensitive_xfer_and_meet(m_cc, e/*, in*/, in_out);
    }
  };

protected:
  int get_max_call_context_depth() const { return m_max_call_context_depth; }
};

}
