#ifndef EQCHECKCORRELATE_H
#define EQCHECKCORRELATE_H

#include "expr/counter_example.h"
#include "gsupport/pc_label.h"

#include "graph/graph_with_guessing.h"

#include "eq/correl_entry.h"
#include "eq/eqcheck.h"
#include "eq/corr_graph.h"

namespace eqspace {

class correlate// : public correlate_interface
{
public:
  correlate(dshared_ptr<eqcheck> const& eq) : m_eq(eq)
  {
//    get_dst_edges_in_dfs_and_scc_first_order(m_eq->get_dst_tfg(), m_dst_edges);
//    tfg* collapsed_dst_tfg = new tfg(m_eq->get_dst_tfg());
//    collapsed_dst_tfg->collapse_tfg(false/*collapse_flag_preserve_branch_structure*/);
//    collapsed_dst_tfg->populate_transitive_closure();
//    collapsed_dst_tfg->populate_reachable_and_unreachable_incoming_edges_map();
//    get_dst_edges_in_dfs_and_scc_first_order(*collapsed_dst_tfg, m_dst_edges);
    list<pc> ordered_pcs = m_eq->get_dst_tfg().topological_sorted_labels();
    int i = 0;
    for(auto const p : ordered_pcs)
    {
      //cout << __func__ << " " << __LINE__ << ": ordered_dst_pc[" << i << "] = " << p.to_string() << endl;
      m_dst_pcs_ordered[p] = i++;
    }
  }

  dshared_ptr<eqcheck> const& get_eq() const { return m_eq; }
  bool correl_entry_less(dshared_ptr<backtracker_node_t const> a, dshared_ptr<backtracker_node_t const> b) const;
  shared_ptr<tfg_full_pathset_t const> choose_most_promising_dst_ec(list<dshared_ptr<backtracker_node_t>> const &frontier) const;
  list<dshared_ptr<backtracker_node_t>> get_correl_entries_with_chosen_dst_ec(list<dshared_ptr<backtracker_node_t>> const &frontier, shared_ptr<tfg_full_pathset_t const> const &chosen_dst_ec) const;
  bool correl_entry_b_has_more_promising_dst_ec(dshared_ptr<backtracker_node_t const> _a, dshared_ptr<backtracker_node_t const> _b) const;
  dshared_ptr<correl_entry_t> choose_most_promising_correlation_entry(/*list<backtracker_node_t *> const &frontier*/) const;
  dshared_ptr<correl_entry_t> choose_most_promising_correlation_entry_and_propagate_CEs(/*list<backtracker_node_t *> const &frontier*/) const;
  dshared_ptr<cg_with_asm_annotation const> find_correlation(string const &cg_filename = "");

//  pair<bool,bool> corr_graph_prune_correlations_to_pc(shared_ptr<corr_graph> &cg_in, pcpair const& cg_from_pc, shared_ptr<tfg_edge_composition_t> const& dst_ec, shared_ptr<corr_graph_edge const > const& cg_new_edge) const;

  list<enum_correl_entry_t> corr_graph_enumerate_correlations(dshared_ptr<cg_with_asm_annotation const> const& cg_in, pcpair const& cg_from_pc, shared_ptr<tfg_full_pathset_t const> const& dst_ec, vector<tuple<path_delta_t, path_mu_t, shared_ptr<tfg_full_pathset_t const>>> const& src_pathsets_to_pc, string const& groupname, int& entrynum) const;
  
  static tuple<graph_edge_composition_ref<pcpair,corr_graph_edge>,shared_ptr<tfg_full_pathset_t const>,shared_ptr<tfg_full_pathset_t const>> corr_graph_add_correlation(dshared_ptr<cg_with_asm_annotation> cg_new, pcpair const& cg_from_pc, shared_ptr<tfg_full_pathset_t const> const& dst_ec, shared_ptr<tfg_full_pathset_t const> const& src_path, pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> const& pc_lsprels_allocs, pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> const& pc_lsprels_deallocs);


  map< tuple<pc,path_delta_t,path_mu_t>, shared_ptr<tfg_full_pathset_t const>>
  get_src_unrolled_paths(dshared_ptr<corr_graph const> const &cg, pcpair const &chosen_pcpair, shared_ptr<tfg_full_pathset_t const> const &next_dst_ec/*, bool implying*/) const;
  list<enum_correl_entry_t> get_next_potential_correlations(dshared_ptr<cg_with_asm_annotation const> const &cg, pcpair const &chosen_pcpair, shared_ptr<tfg_full_pathset_t const> const &next_dst_ec/*, bv_rank_val_t const &parent_bv_rank*/, string const& correl_entry_name) const;
//  list<dshared_ptr<tfg_edge const>> const &get_dst_edges() const { return m_dst_edges; }
  map<pc, int> const &get_ordered_dst_pcs() const { return m_dst_pcs_ordered; }
  static void get_dst_edges_in_dfs_and_scc_first_order(tfg const &t, list<dshared_ptr<tfg_edge const>> &dst_edges);
  size_t get_frontier_size() const { return this->get_backtracking_frontier().size(); }

private:
  list<dshared_ptr<backtracker_node_t>> get_backtracking_frontier() const;
  void eliminate_useless_subtree_from_frontier() const;
  static bool pc_types_match(pc const& src_to_pc, pc const& dst_to_pc, tfg const& src_tfg, tfg const& dst_tfg);
  static void get_dst_edges_in_dfs_and_scc_first_order_rec(tfg const &dst_tfg, pc dst_pc, set<tfg_edge_ref> &done, set<pc>& visited, list<dshared_ptr<tfg_edge const>> &dst_edges);
  static string path_to_string(list<dshared_ptr<tfg_edge const>> const &pathlist);
  static string path_list_to_string(list<list<dshared_ptr<tfg_edge const>>> const &pathlist);
  static string path_ec_list_to_string(list<shared_ptr<tfg_edge_composition_t>> const &path_ec_list);
  static bool preds_map_contains_exit_pcpair(map<pcpair, unordered_set<predicate>> const &preds_map);
  //bool correlate_edges(corr_graph& cg, tfg_frontier_t dst_frontier);
  //bool dst_edge_exists_in_cg_outgoing_edges(corr_graph const &cg_in_out, dshared_ptr<corr_graph_node> cg_node, dshared_ptr<tfg_edge const> const &dst_edge);
  //bool correlate_dst_edge_at_cg_node(corr_graph& cg_in_out, tfg_frontier_t const &dst_edges, shared_ptr<corr_graph_node> cg_node, shared_ptr<tfg_edge> const &dst_edge);

  static paths_weight_t paths_weight(shared_ptr<tfg_full_pathset_t const> const &path);
  list<dshared_ptr<backtracker_node_t>> get_correl_entries_with_least_rank(list<dshared_ptr<backtracker_node_t>> const &frontier) const;

  //void get_dst_edges_in_dfs_and_scc_first_order(list<shared_ptr<tfg_edge>>& dst_edges);
  //void get_dst_edges_in_dfs_and_scc_first_order_rec(pc dst_pc, set<shared_ptr<tfg_edge>>& done, set<pc>& visited, list<shared_ptr<tfg_edge>>& dst_edges);

  //void get_src_paths(const pc& src_pc, map<pc, list<shared_ptr<tfg_edge_composite>>>& src_paths, expr_ref nextpc, consts_struct_t const &cs);
  //static void print_src_paths(tfg const &src_tfg, map<pc, pair<list<list<shared_ptr<tfg_edge>>>, mutex_paths_iterator_t<tfg_edge>>> const &src_paths);

//  shared_ptr<tfg_edge_composition_t> find_implying_paths(shared_ptr<corr_graph> cg, pcpair const &pp, list<shared_ptr<tfg_edge_composition_t>> const &src_paths, shared_ptr<tfg_edge_composition_t> const &dst_edge/*, predicate &edge_cond_pred*/) const;
//  shared_ptr<tfg_edge_composition_t> find_implied_path(shared_ptr<corr_graph const> cg_parent, shared_ptr<corr_graph> cg, pcpair const &pp, shared_ptr<tfg_edge_composition_t> const &src_path, shared_ptr<tfg_edge_composition_t> const &dst_edge, bool &implied_check_status/*, predicate &edge_cond_pred*/) const;

  static shared_ptr<corr_graph_edge const> create_new_corr_graph_edge(dshared_ptr<corr_graph> cg, pcpair const &from_pc, pcpair const &to_pc, shared_ptr<tfg_full_pathset_t const> src_edge, shared_ptr<tfg_full_pathset_t const> const &dst_ec, list<dshared_ptr<corr_graph_node>>* added_nodes = nullptr);
  static dshared_ptr<corr_graph_edge const> add_corr_graph_edge(dshared_ptr<corr_graph> cg, dshared_ptr<corr_graph_edge const> const& e);
  void remove_corr_graph_edge(corr_graph& cg, dshared_ptr<corr_graph_edge const> edge);

  static graph_edge_composition_ref<pcpair,corr_graph_edge> corr_graph_create_and_add_cg_edge_composition_using_src_and_dst_fp(dshared_ptr<cg_with_asm_annotation> cg_new, pcpair const& cg_from_pc, pcpair const& cg_to_pc, shared_ptr<tfg_full_pathset_t const> const& src_path, shared_ptr<tfg_full_pathset_t const> const& dst_path);
  static graph_edge_composition_ref<pcpair,corr_graph_edge> corr_graph_try_creating_edges_by_pairing_allocdeallocs_together(dshared_ptr<cg_with_asm_annotation> cg_new, pcpair const& cg_from_pc, pcpair const& cg_to_pc, shared_ptr<tfg_full_pathset_t const> const& src_path, shared_ptr<tfg_full_pathset_t const> const& dst_path, map<pair<alloc_dealloc_pc_t,alloc_dealloc_t>,pair<pc,list<pc>>> const& common_allocdeallocs);
  static graph_edge_composition_ref<pcpair,corr_graph_edge> corr_graph_try_adding_edges_by_pairing_src_dst_edges_based_on_to_pc(dshared_ptr<cg_with_asm_annotation> cg_new, pcpair const& cg_from_pc, pcpair const& cg_end_pc, function<map<pc_label_t,list<shared_ptr<tfg_full_pathset_t const>>>(pc const&)> get_relevant_src_paths_from, function<map<pc_label_t,list<shared_ptr<tfg_full_pathset_t const>>>(pc const&)> get_relevant_dst_paths_from, list<dshared_ptr<corr_graph_edge const>>& to_be_added_edges, list<dshared_ptr<corr_graph_node>>& added_nodes);

//  list<dshared_ptr<tfg_edge const>> m_dst_edges;
  dshared_ptr<eqcheck> m_eq;
  map<pc, int> m_dst_pcs_ordered;
  mutable size_t m_corr_counter = 0;
  dshared_ptr<backtracker> m_backtracker;
};


}

#endif
