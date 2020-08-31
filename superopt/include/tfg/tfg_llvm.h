#pragma once

#include "tfg/tfg.h"

namespace eqspace {

class tfg_llvm_t : public tfg
{
private:
  map<pc, set<string_ref>> m_defined_ssa_varnames_map;
  //acting as cache
  mutable map<pair<shared_ptr<tfg_edge_composition_t>,bool>, expr_ref> m_ec_econd_map;
  virtual void get_path_enumeration_stop_pcs(set<pc> &stop_pcs) const override;
public:
  tfg_llvm_t(string const &name, context *ctx/*, state const &start_state*/) : tfg(name, ctx/*, start_state*/)
  { }
  tfg_llvm_t(tfg_llvm_t const &other) : tfg(other), 
                                        m_defined_ssa_varnames_map(other.m_defined_ssa_varnames_map),
                                        m_ec_econd_map(other.m_ec_econd_map)
  { }
  tfg_llvm_t(istream& is, string const &name, context *ctx/*, state const &start_state*/) : tfg(is, name, ctx/*, start_state*/)
  { }
  virtual shared_ptr<tfg> tfg_copy() const override;
  virtual void get_path_enumeration_reachable_pcs(pc const &from_pc, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, set<pc> &reachable_pcs) const override;
  virtual void tfg_preprocess(bool collapse, list<string> const& sorted_bbl_indices, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>()) override;
  set<expr_ref> get_interesting_exprs_till_delta(pc const& p, delta_t delta) const override;
  void tfg_llvm_populate_varname_definedness();
  void populate_exit_return_values_for_llvm_method();
  virtual expr_ref tfg_edge_composition_get_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified) const override;
  virtual predicate_set_t collect_assumes_around_edge(tfg_edge_ref const& te) const override;
  virtual map<pair<pc, int>, shared_ptr<tfg_edge_composition_t>> get_composite_path_between_pcs_till_unroll_factor(pc const& from_pc, set<pc> const& to_pcs, map<pc,int> const& visited_count, int unroll_factor, set<pc> const& stop_pcs) const override;
  //virtual expr_ref tfg_edge_composition_get_unsubstituted_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified) const override;
};

}
