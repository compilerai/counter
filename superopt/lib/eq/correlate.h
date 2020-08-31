#ifndef EQCHECKCORRELATE_H
#define EQCHECKCORRELATE_H

#include "eq/eqcheck.h"
#include "eq/corr_graph.h"
#include "graph/graph_with_guessing.h"
#include "expr/counter_example.h"
#include "eq/correl_entry.h"

namespace eqspace {

class correlate// : public correlate_interface
{
public:
  correlate(shared_ptr<eqcheck> const& eq) : m_eq(eq)
  {
    get_dst_edges_in_dfs_and_scc_first_order(m_eq->get_dst_tfg(), m_dst_edges);
//    tfg* collapsed_dst_tfg = new tfg(m_eq->get_dst_tfg());
//    collapsed_dst_tfg->collapse_tfg(false/*collapse_flag_preserve_branch_structure*/);
//    collapsed_dst_tfg->populate_transitive_closure();
//    collapsed_dst_tfg->populate_reachable_and_unreachable_incoming_edges_map();
//    get_dst_edges_in_dfs_and_scc_first_order(*collapsed_dst_tfg, m_dst_edges);
    list<pc> ordered_pcs = m_eq->get_dst_tfg().topological_sorted_labels();
    int i = 0;
    for(auto const p : ordered_pcs)
    {
      m_dst_pcs_ordered[p] = i++;
    }
  }

  shared_ptr<eqcheck> const& get_eq() const { return m_eq; }
  bool correl_entry_less(backtracker_node_t const *a, backtracker_node_t const *b) const;
  shared_ptr<tfg_edge_composition_t> choose_most_promising_dst_ec(list<backtracker_node_t *> const &frontier) const;
  list<backtracker_node_t *> get_correl_entries_with_chosen_dst_ec(list<backtracker_node_t *> const &frontier, shared_ptr<tfg_edge_composition_t> const &chosen_dst_ec) const;
  bool correl_entry_b_has_more_promising_dst_ec(backtracker_node_t const *_a, backtracker_node_t const *_b) const;
  correl_entry_t *choose_most_promising_correlation_entry(list<backtracker_node_t *> const &frontier) const;
  shared_ptr<corr_graph const> find_correlation(local_sprel_expr_guesses_t const& lsprel_assumes, string const &cg_filename = "");

//  pair<bool,bool> corr_graph_prune_correlations_to_pc(shared_ptr<corr_graph> &cg_in, pcpair const& cg_from_pc, shared_ptr<tfg_edge_composition_t> const& dst_ec, shared_ptr<corr_graph_edge const > const& cg_new_edge) const;
  list<correl_entry_t> corr_graph_prune_and_add_correlations_to_pc(shared_ptr<corr_graph const> const& cg_in, pcpair const& cg_from_pc, shared_ptr<tfg_edge_composition_t> const& dst_ec, pc const& src_pc_to, vector<tuple<path_delta_t, path_mu_t, shared_ptr<tfg_edge_composition_t>>> const& src_pathsets_to_pc) const;
  
  shared_ptr<corr_graph> corr_graph_add_correlation(shared_ptr<corr_graph const> const& cg_in, pcpair const& cg_from_pc, shared_ptr<tfg_edge_composition_t> const& dst_ec, shared_ptr<tfg_edge_composition_t> const& src_path, shared_ptr<corr_graph_edge const>& cg_new_edge) const;


  map< pair<pc,path_delta_t>, pair<path_mu_t, shared_ptr<tfg_edge_composition_t>>>
  get_src_unrolled_paths(shared_ptr<corr_graph const> const &cg, pcpair const &chosen_pcpair, shared_ptr<tfg_edge_composition_t> const &next_dst_ec/*, bool implying*/) const;
  list<correl_entry_t> get_next_potential_correlations(shared_ptr<corr_graph const> const &cg, pcpair const &chosen_pcpair, shared_ptr<tfg_edge_composition_t> const &next_dst_ec, bv_rank_val_t const &parent_bv_rank) const;
  list<tfg_edge_ref> const &get_dst_edges() const { return m_dst_edges; }
  map<pc, int> const &get_ordered_dst_pcs() const { return m_dst_pcs_ordered; }
  static void get_dst_edges_in_dfs_and_scc_first_order(tfg const &t, list<tfg_edge_ref> &dst_edges);

private:
  static bool pc_types_match(pc const& src_to_pc, pc const& dst_to_pc, tfg const& src_tfg, tfg const& dst_tfg);
  static void get_dst_edges_in_dfs_and_scc_first_order_rec(tfg const &dst_tfg, pc dst_pc, set<tfg_edge_ref> &done, set<pc>& visited, list<tfg_edge_ref> &dst_edges);
  static string path_to_string(list<shared_ptr<tfg_edge const>> const &pathlist);
  static string path_list_to_string(list<list<shared_ptr<tfg_edge const>>> const &pathlist);
  static string path_ec_list_to_string(list<shared_ptr<tfg_edge_composition_t>> const &path_ec_list);
  static bool preds_map_contains_exit_pcpair(map<pcpair, unordered_set<predicate>> const &preds_map);
  //bool correlate_edges(corr_graph& cg, tfg_frontier_t dst_frontier);
  bool dst_edge_exists_in_cg_outgoing_edges(corr_graph const &cg_in_out, shared_ptr<corr_graph_node> cg_node, shared_ptr<tfg_edge const> const &dst_edge);
  //bool correlate_dst_edge_at_cg_node(corr_graph& cg_in_out, tfg_frontier_t const &dst_edges, shared_ptr<corr_graph_node> cg_node, shared_ptr<tfg_edge> const &dst_edge);

  static paths_weight_t paths_weight(shared_ptr<tfg_edge_composition_t> const &path);
  list<backtracker_node_t*> get_correl_entries_with_least_rank(list<backtracker_node_t *> const &frontier) const;

  //void get_dst_edges_in_dfs_and_scc_first_order(list<shared_ptr<tfg_edge>>& dst_edges);
  //void get_dst_edges_in_dfs_and_scc_first_order_rec(pc dst_pc, set<shared_ptr<tfg_edge>>& done, set<pc>& visited, list<shared_ptr<tfg_edge>>& dst_edges);

  //void get_src_paths(const pc& src_pc, map<pc, list<shared_ptr<tfg_edge_composite>>>& src_paths, expr_ref nextpc, consts_struct_t const &cs);
  //static void print_src_paths(tfg const &src_tfg, map<pc, pair<list<list<shared_ptr<tfg_edge>>>, mutex_paths_iterator_t<tfg_edge>>> const &src_paths);

//  shared_ptr<tfg_edge_composition_t> find_implying_paths(shared_ptr<corr_graph> cg, pcpair const &pp, list<shared_ptr<tfg_edge_composition_t>> const &src_paths, shared_ptr<tfg_edge_composition_t> const &dst_edge/*, predicate &edge_cond_pred*/) const;
//  shared_ptr<tfg_edge_composition_t> find_implied_path(shared_ptr<corr_graph const> cg_parent, shared_ptr<corr_graph> cg, pcpair const &pp, shared_ptr<tfg_edge_composition_t> const &src_path, shared_ptr<tfg_edge_composition_t> const &dst_edge, bool &implied_check_status/*, predicate &edge_cond_pred*/) const;

  shared_ptr<corr_graph_edge const> add_corr_graph_edge(shared_ptr<corr_graph> cg, pcpair const &from_pc, pcpair const &to_pc, shared_ptr<tfg_edge_composition_t> src_edge, shared_ptr<tfg_edge_composition_t> const &dst_edge/*, edge_guard_t const &g*/) const;
  void remove_corr_graph_edge(corr_graph& cg, shared_ptr<corr_graph_edge const> edge);


  list<tfg_edge_ref> m_dst_edges;
  shared_ptr<eqcheck> m_eq;
  map<pc, int> m_dst_pcs_ordered;
  mutable size_t m_corr_counter = 0;
};


}

#endif
