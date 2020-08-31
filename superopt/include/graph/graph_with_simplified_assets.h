#pragma once

#include "graph_with_locs.h"
#include "support/utils.h"
#include "support/log.h"
#include "expr/expr.h"
#include "expr/expr_simplify.h"
#include "graph/graph_with_locs.h"
#include "graph/edge_id.h"
#include "expr/sprel_map_pair.h"
#include "support/dyn_debug.h"

#include <map>
#include <string>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_simplified_assets : public graph_with_locs<T_PC, T_N, T_E, T_PRED>
{
public:
  graph_with_simplified_assets(string const &name, context* ctx) : graph_with_locs<T_PC,T_N,T_E,T_PRED>(name, ctx)
  {
  }

  graph_with_simplified_assets(graph_with_simplified_assets const &other) : graph_with_locs<T_PC,T_N,T_E,T_PRED>(other),
                                                                            m_simplified_assert_preds(other.m_simplified_assert_preds),
                                                                            m_edge_to_loc_to_expr_map(other.m_edge_to_loc_to_expr_map)
  {
  }

  graph_with_simplified_assets(istream& in, string const& name, context* ctx) : graph_with_locs<T_PC,T_N,T_E,T_PRED>(in, name, ctx)
  {
  }

  virtual ~graph_with_simplified_assets() = default;

  unordered_set<shared_ptr<T_PRED const>> const &get_simplified_assert_preds(const T_PC& p) const { return m_simplified_assert_preds.get_predicate_set(p); }

  map<graph_loc_id_t,expr_ref>& get_locs_potentially_written_on_edge(shared_ptr<T_E const> const& e) const { return m_edge_to_loc_to_expr_map.at(e->get_edge_id()); }
  set<graph_loc_id_t>         get_locids_potentially_written_on_edge(shared_ptr<T_E const> const& e) const { return map_get_keys(this->get_locs_potentially_written_on_edge(e)); }

  virtual void populate_simplified_to_state(shared_ptr<T_E const> const &e) const = 0;
  virtual void populate_simplified_edgecond(shared_ptr<T_E const> const& e) const = 0;
  virtual void populate_simplified_assumes(shared_ptr<T_E const> const& e) const = 0;
  virtual void populate_locs_potentially_modified_on_edge(shared_ptr<T_E const> const &e) const = 0;

  void populate_simplified_assert_map(set<T_PC> const& pcs)
  {
    graph_memlabel_map_t const& memlabel_map = this->get_memlabel_map();

    for (auto const& pc : pcs) {
      m_simplified_assert_preds.remove_preds_for_pc(pc);
      sprel_map_pair_t const& sprel_map_pair = this->get_sprel_map_pair(pc);
      for (auto const& pred : this->get_assert_preds(pc)) {
        shared_ptr<T_PRED const> simplified_pred = pred->pred_simplify_using_sprel_pair_and_memlabel_map(sprel_map_pair, memlabel_map);
        m_simplified_assert_preds.add_pred(pc, simplified_pred);
      }
    }
  }

  map<graph_loc_id_t, expr_ref> get_locs_definitely_written_on_edge(shared_ptr<T_E const> const& e) const
  {
    //autostop_timer func(__func__);
    //this function only needs to be over-approximate. i.e., the returned locs must be definitely written-to;
    //but perhaps some more locs (that are not returned) are also written to
    map<graph_loc_id_t, expr_ref> all = this->get_locs_potentially_written_on_edge(e);
    map<graph_loc_id_t, expr_ref> ret;
    for (const auto &le : all) {
      if (this->expr_indicates_definite_write(le.first, le.second)) {
        ret.insert(le);
      }
    }
    return ret;
  }

  //virtual void graph_get_relevant_memlabels(vector<memlabel_ref> &relevant_memlabels) const = 0;
  //virtual void graph_get_relevant_memlabels_except_args(vector<memlabel_ref> &relevant_memlabels) const = 0;

protected:

  virtual map<string_ref, expr_ref> graph_get_vars_written_on_edge(shared_ptr<T_E const> const& e) const = 0;
  virtual map<graph_loc_id_t,expr_ref> compute_simplified_loc_exprs_for_edge(shared_ptr<T_E const> const& e, set<graph_loc_id_t> const& locids, bool force_update = false) const = 0;

  expr_ref graph_loc_get_value_simplified(T_PC const &p, state const &s, graph_loc_id_t loc_index) const
  {
    sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(p);
    auto const& memlabel_map = this->get_memlabel_map();
    expr_ref ret = this->graph_loc_get_value(p, s, loc_index);
    context* ctx = this->get_context();
    //cout << __func__ << " " << __LINE__ << ": " << p.to_string() << ": loc_index " << loc_index << ": before simplify, memlabel_map.size() = " << memlabel_map.size() << ", ret = " << expr_string(ret) << endl;
    ret = ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(ret, sprel_map_pair, memlabel_map);
    //cout << __func__ << " " << __LINE__ << ": " << p.to_string() << ": loc_index " << loc_index << ": after simplify, ret = " << expr_string(ret) << endl;
    return ret;
  }

  //this function is the optimized version of graph_loc_get_value_simplified(); the additional MEM_SIMPLIFIED argument
  //allows us to avoid repeated simplifications of the memory expressions for memslot/memmasked locs
  expr_ref graph_loc_get_value_simplified_using_mem_simplified(T_PC const &p, state const &to_state, graph_loc_id_t loc_id, map<string,expr_ref> const &mem_simplified) const;

  map<string,expr_ref> graph_loc_get_mem_simplified(T_PC const &p, state const &to_state) const
  {
    map<string_ref, expr_ref> const &state_map = to_state.get_value_expr_map_ref();
    sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(p);
    auto const& memlabel_map = this->get_memlabel_map();
    map<string, expr_ref> mem_simplified;
    for (const auto &sm : state_map) {
      if (sm.second->is_array_sort()) {
        //state_map_contains_memory = true;
        expr_ref smem = this->get_context()->expr_simplify_using_sprel_pair_and_memlabel_maps(sm.second, sprel_map_pair, memlabel_map);
        smem = this->get_context()->expr_replace_alloca_with_nop(smem);
        mem_simplified.insert(make_pair(sm.first->get_str(), smem));
      }
    }
    return mem_simplified;
  }

  map<edge_id_t<T_PC>,map<graph_loc_id_t,expr_ref>>& get_edge_to_locs_written_map() const
  {
      return m_edge_to_loc_to_expr_map;
  }

private:

  predicate_map<T_PC, T_PRED> m_simplified_assert_preds;

  mutable map<edge_id_t<T_PC>, map<graph_loc_id_t, expr_ref>> m_edge_to_loc_to_expr_map;
};

}
