#pragma once

#include "graph_with_locs.h"
#include "support/utils.h"
#include "support/log.h"
#include "expr/expr.h"
#include "expr/expr_simplify.h"
#include "graph/graph_with_locs.h"
#include "expr/sprel_map_pair.h"
#include "support/dyn_debug.h"

#include "gsupport/edge_id.h"
#include "gsupport/tfg_edge.h"

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
  graph_with_simplified_assets(string const &name, string const& fname, context* ctx) : graph_with_locs<T_PC,T_N,T_E,T_PRED>(name, fname, ctx)
  {
  }

  graph_with_simplified_assets(graph_with_simplified_assets const &other) : graph_with_locs<T_PC,T_N,T_E,T_PRED>(other),
                                                                            //m_simplified_assert_preds(other.m_simplified_assert_preds),
                                                                            m_edge_to_simplified_edgecond_map(other.m_edge_to_simplified_edgecond_map),
                                                                            m_edge_to_simplified_state_map(other.m_edge_to_simplified_state_map),
                                                                            m_edge_to_simplified_assumes_map(other.m_edge_to_simplified_assumes_map),
                                                                            //m_edge_to_well_formedness_preds_map(other.m_edge_to_well_formedness_preds_map),
                                                                            m_edge_to_assumes_around_edge(other.m_edge_to_assumes_around_edge),
                                                                            m_edge_to_loc_to_expr_map(other.m_edge_to_loc_to_expr_map)
  {
  }

  graph_with_simplified_assets(istream& in, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx);

  virtual ~graph_with_simplified_assets() = default;

  //unordered_set<shared_ptr<T_PRED const>> const &get_simplified_assert_preds(const T_PC& p) const { return m_simplified_assert_preds.get_predicate_set(p); }

  //void set_locs_potentially_written_on_edge(dshared_ptr<T_E const> const& e, map<graph_loc_id_t,expr_ref> const& m) const { m_edge_to_loc_to_expr_map[e->get_edge_id()] = m; }
  map<graph_loc_id_t,expr_ref> const& get_locs_potentially_written_on_edge(dshared_ptr<T_E const> const& e) const { return m_edge_to_loc_to_expr_map.at(e->get_edge_id()); }
  set<graph_loc_id_t>         get_locids_potentially_written_on_edge(dshared_ptr<T_E const> const& e) const { return map_get_keys(this->get_locs_potentially_written_on_edge(e)); }

  virtual void populate_simplified_to_state(dshared_ptr<T_E const> const &e) = 0;
  virtual void populate_simplified_edgecond(dshared_ptr<T_E const> const& e) = 0;
  virtual void populate_simplified_assumes(dshared_ptr<T_E const> const& e) = 0;
  //virtual void populate_locs_potentially_modified_on_edge(shared_ptr<T_E const> const &e) = 0;
  virtual void populate_assumes_around_edge(dshared_ptr<T_E const> const &e) = 0;

  //void populate_simplified_assert_map(set<T_PC> const& pcs)
  //{
  //  graph_memlabel_map_t const& memlabel_map = this->get_memlabel_map();

  //  for (auto const& pc : pcs) {
  //    m_simplified_assert_preds.remove_preds_for_pc(pc);
  //    sprel_map_pair_t const& sprel_map_pair = this->get_sprel_map_pair(pc);
  //    for (auto const& pred : this->get_assert_preds(pc)) {
  //      shared_ptr<T_PRED const> simplified_pred = pred->pred_simplify_using_sprel_pair_and_memlabel_map(sprel_map_pair, memlabel_map);
  //      m_simplified_assert_preds.add_pred(pc, simplified_pred);
  //    }
  //  }
  //}

  //virtual void graph_get_relevant_memlabels(vector<memlabel_ref> &relevant_memlabels) const = 0;
  //virtual void graph_get_relevant_memlabels_except_args(vector<memlabel_ref> &relevant_memlabels) const = 0;
  void populate_simplified_assets(dshared_ptr<T_E const> const& e = dshared_ptr<T_E const>::dshared_nullptr())
  {
    autostop_timer func_timer(__func__);
    this->populate_simplified_edgecond(e);
    this->populate_simplified_to_state(e);
    this->populate_simplified_assumes(e);
    //this->populate_simplified_assert_map(this->get_all_pcs());
    this->populate_assumes_around_edge(e);
  }

  state const& get_simplified_to_state_for_edge(dshared_ptr<T_E const> const& e) const
  {
    if (!m_edge_to_simplified_state_map.count(e->get_edge_id())) {
      cout << _FNLN_ << ": Edges (size " << this->get_edges().size() << ") =\n";
      for (auto const& e : this->get_edges()) {
        cout << "  " << e->get_from_pc().to_string() << " => " << e->get_to_pc().to_string() << endl;
      }
      cout << _FNLN_ << ": m_edge_to_simplified_state_map (size " << m_edge_to_simplified_state_map.size() << ") =\n";
      for (auto const& em : m_edge_to_simplified_state_map) {
        cout << "  " << em.first.get_from_pc().to_string() << " => " << em.first.get_to_pc().to_string() << endl;
      }
      cout << "e =\n" << e->get_edge_id().get_from_pc().to_string() << " => " << e->get_edge_id().get_to_pc().to_string() << endl;
    }
    return m_edge_to_simplified_state_map.at(e->get_edge_id());
  }

  expr_ref const& get_simplified_edge_cond_for_edge(dshared_ptr<T_E const> const& e) const;

  unordered_set<expr_ref> const& get_simplified_assumes_for_edge(dshared_ptr<T_E const> const& e) const
  {
    if (!m_edge_to_simplified_assumes_map.count(e->get_edge_id())) {
      cout << _FNLN_ << ": edge_to_simplified_assumes_map =\n";
      for (auto const& [e,s] : m_edge_to_simplified_assumes_map) {
        cout << "  " << e.get_from_pc().to_string() << "=>" << e.get_to_pc().to_string() << " --> simplified-assumes" << endl;
      }
      cout << _FNLN_ << ": e = " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << endl;
    }
    return m_edge_to_simplified_assumes_map.at(e->get_edge_id());
  }

  //expr_ref get_preimage_for_expr_across_edge(expr_ref const& e, dshared_ptr<T_E const> const& ed) const;
  //virtual list<guarded_pred_t> apply_trans_funs_simplified(graph_edge_composition_ref<T_PC,T_E> const& ec, predicate_ref const &pred) const = 0;

  virtual void add_edge(dshared_ptr<T_E const> e) override;

protected:

  list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> const& get_assumes_around_edge(dshared_ptr<T_E const> const& te) const;
  //map<graph_edge_composition_ref<T_PC,T_E>, unordered_set<shared_ptr<T_PRED const>>> get_simplified_asserts_on_edge(shared_ptr<T_E const> const& e_in) const;
  //map<graph_edge_composition_ref<T_PC,T_E>, unordered_set<shared_ptr<T_PRED const>>> get_well_formedness_conditions_for_edge(shared_ptr<T_E const> const& e_in) const;
  virtual void graph_to_stream(ostream& ss, string const& prefix="") const override;
  void set_simplified_to_state_for_edge(dshared_ptr<T_E const> const& e, state const& to_state);
  void set_assumes_around_edge_for_edge(dshared_ptr<T_E const> const& e, list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> const& assumes);
  void set_simplified_edge_cond_for_edge(dshared_ptr<T_E const> const& e, expr_ref const& edgecond);
  void set_simplified_assumes_for_edge(dshared_ptr<T_E const> const& e, unordered_set<expr_ref> const& assumes);
  virtual map<string_ref, expr_ref> graph_get_vars_written_on_edge(dshared_ptr<T_E const> const& e) const = 0;
  //virtual map<graph_loc_id_t,expr_ref> compute_simplified_loc_exprs_for_edge(shared_ptr<T_E const> const& e, set<graph_loc_id_t> const& locids, bool force_update = false) const = 0;

  map<edge_id_t<T_PC>, list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>>> const get_edge_to_assumes_around_edge_map() const { return m_edge_to_assumes_around_edge; }
  void set_edge_to_assumes_around_edge_map(map<edge_id_t<T_PC>, list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>>> const& m) { m_edge_to_assumes_around_edge = m; }

  expr_ref graph_cp_location_get_value_simplified(/*T_PC const &p, */state const &s, graph_cp_location const& loc/*graph_loc_id_t loc_index*/, sprel_map_pair_t const& sprel_map_pair, graph_memlabel_map_t const& memlabel_map) const;

  //this function is the optimized version of graph_loc_get_value_simplified(); the additional MEM_SIMPLIFIED argument
  //allows us to avoid repeated simplifications of the memory expressions for memslot/memmasked locs
  expr_ref graph_loc_get_value_simplified_using_mem_simplified(T_PC const &p, state const &to_state, graph_loc_id_t loc_id, map<string,expr_ref> const &mem_simplified) const;

  //map<string,expr_ref> graph_loc_get_mem_simplified(T_PC const &p, state const &to_state) const
  //{
  //  map<string_ref, expr_ref> const &state_map = to_state.get_value_expr_map_ref();
  //  sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(p);
  //  auto const& memlabel_map = this->get_memlabel_map();
  //  map<string, expr_ref> mem_simplified;
  //  for (const auto &sm : state_map) {
  //    if (sm.second->is_array_sort()) {
  //      //state_map_contains_memory = true;
  //      expr_ref smem = this->get_context()->expr_simplify_using_sprel_pair_and_memlabel_maps(sm.second, sprel_map_pair, memlabel_map);
  //      //smem = this->get_context()->expr_replace_alloca_with_nop(smem);
  //      mem_simplified.insert(make_pair(sm.first->get_str(), smem));
  //    }
  //  }
  //  return mem_simplified;
  //}

  map<edge_id_t<T_PC>,map<graph_loc_id_t,expr_ref>> const& get_edge_to_locs_written_map() const
  {
      return m_edge_to_loc_to_expr_map;
  }

  void set_edge_to_locs_written_map(map<edge_id_t<T_PC>,map<graph_loc_id_t,expr_ref>> const& m) const
  {
      m_edge_to_loc_to_expr_map = m;
  }

//private:
  //void populate_well_formedness_preds_for_edge(shared_ptr<T_E const> const& e_in);

private:
  //predicate_map<T_PC, T_PRED> m_simplified_assert_preds;

  map<edge_id_t<T_PC>, expr_ref> m_edge_to_simplified_edgecond_map;
  map<edge_id_t<T_PC>, state> m_edge_to_simplified_state_map;
  map<edge_id_t<T_PC>, unordered_set<expr_ref>> m_edge_to_simplified_assumes_map;

  map<edge_id_t<T_PC>, list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>>> m_edge_to_assumes_around_edge;
  //map<edge_id_t<T_PC>, unordered_set<shared_ptr<T_PRED const>>> m_edge_to_well_formedness_preds_map;
  //map<edge_id_t<T_PC>, list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>>> m_edge_to_well_formedness_conditions_map;
  mutable map<edge_id_t<T_PC>, map<graph_loc_id_t, expr_ref>> m_edge_to_loc_to_expr_map;
};

}
