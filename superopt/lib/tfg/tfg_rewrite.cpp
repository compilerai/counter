#include "graph/tfg_rewrite.h"
#include "support/debug.h"

shared_ptr<tfg>
tfg::tfg_rewrite(tfg const &t)
{
  shared_ptr<tfg> ret = t.tfg_copy();
  //ret->tfg_perform_tail_recursion_optimization(); //NOT_IMPLEMENTED();
  return ret;
}

std::set<pc>
tfg::get_all_recursive_fcalls()
{
  nextpc_id_t nextpc_id = get_nextpc_id_from_function_name(G_SELFCALL_IDENTIFIER);
  if (nextpc_id == -1) {
    return set<pc>();
  }
  set<pc> all_pcs = this->get_all_pcs();
  set<pc> ret;
  for (const auto &p : this->get_all_pcs()) {
    if (   p.get_subsubindex() == PC_SUBSUBINDEX_FCALL_END
	&& this->get_incident_fcall_nextpc_id(p) == nextpc_id) {
      ret.insert(p);
    }
  }
  return ret;
}

tfg *
tfg::get_prefix_tfg(pc const &p) const
{
  NOT_IMPLEMENTED();
}

tfg *
tfg::get_suffix_tfg_if_single_entry_and_single_exit(pc const &p) const
{
  NOT_IMPLEMENTED();
}

set<graph_loc_id_t>
tfg::tfg_edge_get_modified_locs(shared_ptr<tfg_edge const> const &e) const
{
  NOT_IMPLEMENTED();
}

set<graph_loc_id_t>
tfg::tfg_get_modified_locs_for_sub_tfg(tfg const *sub_tfg) const
{
  NOT_IMPLEMENTED();
}

bool
tfg::tfg_output_circuits_are_associative_on_inputs(tfg const *sub,
  set<graph_loc_id_t> const &fcall_locs_modified,
  set<graph_loc_id_t> const &suffix_tfg_remaining_locs,
  map<graph_loc_id_t, expr_ref> const &fcall_loc_identity_values) const
{
  NOT_IMPLEMENTED();
}

void
tfg::tfg_perform_tail_recursion_optimization()
{
  set<pc> recursive_calls = this->get_all_recursive_fcalls();
  bool found_opt_possibility = false;
  map<graph_loc_id_t, expr_ref> fcall_loc_identity_values;
  set<graph_loc_id_t> fcall_locs_modified;
  shared_ptr<tfg_edge const> fcall_edge;
  pc fcall_pc;
  for (const auto &p : recursive_calls) {
    tfg *suffix_tfg = this->get_suffix_tfg_if_single_entry_and_single_exit(p);
    if (!suffix_tfg) {
      continue;
    }
    list<shared_ptr<tfg_edge const>> incoming;
    this->get_edges_incoming(p, incoming);
    ASSERT(incoming.size() == 1);
    fcall_edge = *incoming.begin();
    fcall_locs_modified = this->tfg_edge_get_modified_locs(fcall_edge);
    set<graph_loc_id_t> suffix_tfg_locs_modified = this->tfg_get_modified_locs_for_sub_tfg(suffix_tfg);
    if (!set_intersection_is_empty(fcall_locs_modified, suffix_tfg_locs_modified)) {
      delete suffix_tfg;
      continue;
    }
    set<graph_loc_id_t> const &live_locs_at_pc = this->get_loc_liveness().at(p);
    set<graph_loc_id_t> suffix_tfg_remaining_locs;
    set_intersect(fcall_locs_modified, live_locs_at_pc);
    set_difference(live_locs_at_pc, fcall_locs_modified, suffix_tfg_remaining_locs);
    if (this->tfg_output_circuits_are_associative_on_inputs(suffix_tfg,
			    fcall_locs_modified, suffix_tfg_remaining_locs,
			    fcall_loc_identity_values)) {
      found_opt_possibility = true;
      fcall_pc = p;
      delete suffix_tfg;
      break;
    }
    delete suffix_tfg;
  }
  if (!found_opt_possibility) {
    return;
  }
  tfg *suffix_tfg = this->get_suffix_tfg_if_single_entry_and_single_exit(fcall_pc);
  ASSERT(suffix_tfg);
  tfg *prefix_tfg = this->get_prefix_tfg(fcall_edge->get_from_pc());
  NOT_IMPLEMENTED();
  //1. initialize state elements associated with fcall_locs_modified using fcall_loc_identity_values
  //2. paste prefix tfg
  //3. paste suffix tfg
  //4. create unconditional branch to start of prefix tfg in (2)
  //5. At all return points, paste the suffix tfg, where fcall_locs_modified are replaced by (or assigned to) the return values
  //TESTCASES: fib on integers, fib on structures (where each structure represents a very long integer)

  delete suffix_tfg;
  delete prefix_tfg;
}
