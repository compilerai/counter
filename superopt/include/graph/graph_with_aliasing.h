#pragma once

#include <memory>
#include "support/utils.h"
#include "support/log.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "graph/graph_with_simplified_assets.h"
#include "graph/lr_map.h"
#include "expr/memlabel_map.h"
#include "graph/alias_dfa.h"
#include "support/timers.h"
#include "support/dyn_debug.h"
#include "tfg/parse_input_eq_file.h"

#include <map>
#include <list>
#include <string>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_aliasing : public graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED>
{
public:
  graph_with_aliasing(string const &name, context* ctx) : graph_with_simplified_assets<T_PC,T_N,T_E,T_PRED>(name, ctx), m_alias_analysis(this, true, this->get_memlabel_map_ref())
  {
  }

  graph_with_aliasing(graph_with_aliasing const &other) : graph_with_simplified_assets<T_PC,T_N,T_E,T_PRED>(other),
                                                          m_relevant_addr_refs(other.m_relevant_addr_refs),
                                                          m_alias_analysis(this, other.m_alias_analysis)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_aliasing(istream& in, string const& name, context* ctx);
  virtual ~graph_with_aliasing() = default;

  set<cs_addr_ref_id_t> const &graph_get_relevant_addr_refs() const { return m_relevant_addr_refs; }
  void set_relevant_addr_refs(set<cs_addr_ref_id_t> const &relevant_addr_refs) { m_relevant_addr_refs = relevant_addr_refs; }

  string to_string_with_linear_relations(map<T_PC, map<graph_loc_id_t, lr_status_ref>> const &linear_relations, bool details = true) const;

  virtual callee_summary_t get_callee_summary_for_nextpc(nextpc_id_t nextpc_num, size_t num_fargs) const = 0;


  virtual void graph_init_memlabels_to_top(bool update_callee_memlabels) override
  {
    if (update_callee_memlabels) {
      this->get_memlabel_map_ref().clear();
    } else {
      for (auto &mlvar : this->get_memlabel_map_ref()) {
        if (string_has_prefix(mlvar.first->get_str(), G_MEMLABEL_VAR_PREFIX)) {
          mlvar.second = mk_memlabel_ref(memlabel_t::memlabel_top());
        }
      }
    }
    graph_memlabel_map_t& memlabel_map = this->get_memlabel_map_ref();
    function<void (string const&, expr_ref)> f = [&memlabel_map](string const& key, expr_ref e)
    {
      expr_set_memlabels_to_top(e, memlabel_map);
    };
    this->graph_visit_exprs_const(f);
  }

  virtual bool populate_auxilliary_structures_dependent_on_locs() override
  {
    autostop_timer func_timer(__func__);

    auto locs_map = this->get_locs();

    this->graph_locs_compute_and_set_memlabels(locs_map);
    this->set_locs(locs_map);

    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": calling populate_locid2expr_map" << endl);
    this->populate_locid2expr_map();

    this->populate_locs_potentially_modified_on_edge(nullptr);
    // populate_loc_liveness uses simplified edgecond in its computation
    this->populate_simplified_edgecond(nullptr);

    this->populate_loc_liveness(/*false*/);
    DYN_DEBUG2(alias_analysis, cout << __func__ << " " << __LINE__ << ": after populate_loc_liveness, TFG:\n" << this->graph_to_string(/*true*/));

    return this->propagate_sprels();
  }

  virtual void graph_locs_compute_and_set_memlabels(map<graph_loc_id_t, graph_cp_location> &locs_map)
  {
    std::function<graph_loc_id_t (expr_ref const &e)> dummy_expr2locid_fn = [](expr_ref const &e)
    { return GRAPH_LOC_ID_INVALID; };

    for (auto &l : locs_map) {
      if (l.second.m_type != GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
        continue;
      }
      lr_status_ref loc_lr = this->compute_lr_status_for_expr(l.second.m_addr, map<graph_loc_id_t, graph_cp_location>(), map<graph_loc_id_t, lr_status_ref>(), this->graph_get_relevant_addr_refs(), this->get_argument_regs(), this->get_symbol_map(), this->get_locals_map(), this->get_consts_struct());
      l.second.m_memlabel = mk_memlabel_ref(loc_lr->to_memlabel(this->get_consts_struct(), this->get_symbol_map(), this->get_locals_map()));
    }
  }

  void
  node_update_memlabel_map_for_mlvars(T_PC const& p, map<graph_loc_id_t, lr_status_ref> const& in, bool update_callee_memlabels, graph_memlabel_map_t& memlabel_map) const
  {
    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << endl);
    sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(p);
    std::function<void (T_PC const&, string const &, expr_ref)> f = [this,update_callee_memlabels,&memlabel_map,&in,&sprel_map_pair](T_PC const&, string const &key, expr_ref e)
    {
      if (e->is_const() || e->is_var()) { //common case
        return;
      }
      this->populate_memlabel_map_for_expr_and_loc_status(e, this->get_locs(), in, sprel_map_pair, this->graph_get_relevant_addr_refs(), this->get_argument_regs(), this->get_symbol_map(), this->get_locals_map(), update_callee_memlabels, memlabel_map/*, this->m_get_callee_summary_fn, this->m_expr2locid_fn*/);
    };

    this->graph_preds_visit_exprs({p}, f);
  }

  void
  edge_update_memlabel_map_for_mlvars(shared_ptr<T_E const> const& e, map<graph_loc_id_t, lr_status_ref> const& in, bool update_callee_memlabels, graph_memlabel_map_t& memlabel_map) const;

  map<graph_loc_id_t, lr_status_ref>
  compute_new_lr_status_on_locs(shared_ptr<T_E const> const &e, map<graph_loc_id_t, lr_status_ref> const &in/*, bool update_callee_memlabels*/) const
  {
    autostop_timer func_timer(__func__);

    T_PC const &to_pc = e->get_to_pc();
    auto const& loc2expr_map = this->get_locs_potentially_written_on_edge(e);

    map<graph_loc_id_t, lr_status_ref> ret;
    const auto &potentially_modified_locs = this->get_locids_potentially_written_on_edge(e);
    for (const auto &loc : this->get_live_locids(to_pc)) {
      DYN_DEBUG3(linear_relations, cout << __func__ << " " << __LINE__ << ": looking at loc" << loc << " at " << to_pc.to_string() << endl);
      if (!set_belongs(potentially_modified_locs, loc)) {
        if (!in.count(loc)) {
          cout << __func__ << " " << __LINE__ << ": looking at loc " << loc << " (" << expr_string(this->get_loc_expr(loc)) << ") at " << to_pc.to_string() << " of edge " << e->to_string() << endl;
          cout.flush();
        }
        ASSERT(in.count(loc));
        ASSERT(!ret.count(loc));
        ret.insert(make_pair(loc, in.at(loc)));
        continue;
      }
      autostop_timer func2_timer(string(__func__) + ".loc_lr_status.simplify_and_compute_lr_status");
      expr_ref new_e = loc2expr_map.at(loc);
      DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": new_e = " << expr_string(new_e) << endl);

      lr_status_ref lrs = this->compute_lr_status_for_expr(new_e, this->get_locs(), in, this->graph_get_relevant_addr_refs(), this->get_argument_regs(), this->get_symbol_map(), this->get_locals_map(), this->get_consts_struct());
      ASSERT(!ret.count(loc));
      ret.insert(make_pair(loc, lrs));
    }
    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": done e = " << e->to_string() << endl);
    return ret;
  }

  void split_memory_in_graph_initialize(bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>())
  {
    autostop_timer func_timer(__func__);

    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": before set_memlabels_to_top_or_constant_values, TFG:\n" << this->graph_to_string(/*true*/) << endl);

    this->graph_init_memlabels_to_top(update_callee_memlabels);
    this->graph_init_sprels();

    DYN_DEBUG(alias_analysis, cout << __func__ << " " << __LINE__ << ": after set_memlabels_to_top_or_constant_values, TFG:\n" << this->graph_to_string(/*true*/) << endl);

    this->init_graph_locs(this->get_symbol_map(), this->get_locals_map(), update_callee_memlabels, llvm_locs);

    m_alias_analysis.reset(update_callee_memlabels); //, this->get_memlabel_map_ref()
  }

  void
  split_memory_in_graph(shared_ptr<T_E const> const &new_edge)
  {
    autostop_timer func_timer(__func__);
    if (new_edge != nullptr) {
      DYN_DEBUG(alias_analysis, cout << _FNLN_ << ": calling incremental solve on edge " << new_edge->to_string() << endl; this->print_graph_locs(); this->print_loc_liveness());
      m_alias_analysis.incremental_solve(new_edge); // updates the memlabel map
    } else {
      DYN_DEBUG(alias_analysis, cout << _FNLN_ << ": starting alias analysis\n"; this->print_graph_locs(); this->print_loc_liveness());
      m_alias_analysis.solve();                    // updates the memlabel map
    }
    this->populate_loc_liveness();
  }

  set<T_PC> get_from_pcs_for_edges(set<shared_ptr<T_E const>> const &edges)
  {
    set<T_PC> ret;
    for (auto e : edges) {
      ret.insert(e->get_from_pc());
    }
    return ret;
  }

  void
  split_memory_in_graph_and_propagate_sprels_until_fixpoint(bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = {});

  virtual void graph_populate_relevant_addr_refs();

  lr_status_ref compute_lr_status_for_expr(expr_ref e, map<graph_loc_id_t, graph_cp_location> const &locs, map<graph_loc_id_t, lr_status_ref> const &prev_lr, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_arg_regs_t const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, eqspace::consts_struct_t const &cs) const;

protected:

  virtual void graph_to_stream(ostream& ss) const override;

private:

  void populate_memlabel_map_for_expr_and_loc_status(expr_ref const &e, map<graph_loc_id_t, graph_cp_location> const &locs, map<graph_loc_id_t, lr_status_ref> const &in, sprel_map_pair_t const &sprel_map_pair, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_arg_regs_t const &argument_regs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, graph_memlabel_map_t &memlabel_map/*, std::function<callee_summary_t (nextpc_id_t, int)> get_callee_summary_fn, std::function<graph_loc_id_t (expr_ref const &)> expr2locid_fn*/) const;

private:

  set<cs_addr_ref_id_t> m_relevant_addr_refs;
  alias_analysis<T_PC, T_N, T_E, T_PRED> m_alias_analysis;
};

}
