#include "tfg/tfg_llvm.h"
#include <set>

using namespace std;

namespace eqspace {

typedef set<string_ref> defined_varnames_t;

class defined_varnames_val_t
{
private:
  bool m_is_top;
  defined_varnames_t m_varnames;
public:
  defined_varnames_val_t() : m_is_top(false), m_varnames() { }
  defined_varnames_val_t(defined_varnames_t const &v) : m_is_top(false), m_varnames(v) { }

  static defined_varnames_val_t
  top()
  {
    defined_varnames_val_t ret;
    ret.m_is_top = true;
    return ret;
  }

  defined_varnames_t const &get_defined_varnames() const { ASSERT(!m_is_top); return m_varnames; }

  bool meet(defined_varnames_t const& b)
  {
    if (m_is_top) {
      this->m_varnames = b;
      this->m_is_top = false;
      return true;
    }
    defined_varnames_t oldval = m_varnames;
    set_intersect(m_varnames, b);
    return oldval != m_varnames;
  }

  static std::function<defined_varnames_t (pc const &)>
  get_boundary_value(graph<pc, tfg_node, tfg_edge> const *g)
  {
    auto f = [g](pc const &p)
    {
      ASSERT(p.is_start());
      graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate>const* gp = dynamic_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const*>(g);

      // add input args
      graph_arg_regs_t const &arg_regs = gp->get_argument_regs();
      defined_varnames_t retval;
      for (auto const& arg : arg_regs.get_map()) {
        if (arg.second->is_var() && (arg.second->is_bv_sort() || arg.second->is_bool_sort())) {
          retval.insert(arg.second->get_name());
        }
      }
      return retval;
    };
    return f;
  }
};

class def_ssa_analysis : public data_flow_analysis<pc, tfg_node, tfg_edge, defined_varnames_val_t, dfa_dir_t::forward>
{
public:

  def_ssa_analysis(graph<pc, tfg_node, tfg_edge> const* g, map<pc, defined_varnames_val_t> &init_vals)
    : data_flow_analysis<pc, tfg_node, tfg_edge, defined_varnames_val_t, dfa_dir_t::forward>(g, init_vals)
  {
    //defined_locs_t()
    ASSERT((dynamic_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const*>(g) != nullptr));
  }

  virtual bool xfer_and_meet(shared_ptr<tfg_edge const> const &e, defined_varnames_val_t const& in, defined_varnames_val_t &out) override
  {
    autostop_timer func_timer(__func__);
    graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate>const* gp = dynamic_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const*>(this->m_graph);
    defined_varnames_t retval = in.get_defined_varnames();

    map<graph_loc_id_t, expr_ref> locs_written = gp->get_locs_potentially_written_on_edge(e);
    for (auto  const& loc : locs_written) {
      expr_ref loc_expr = gp->graph_loc_get_value_for_node(loc.first);
      if (!loc_expr->is_var()) {
        continue;
      }
      if (!loc_expr->is_bv_sort() && !loc_expr->is_bool_sort()) {
        continue;
      }
      retval.insert(loc_expr->get_name());
    }
    //set_union(retval, map_get_keys(locs_written));
    bool ret = out.meet(retval);
    return ret;
  }
};

void
tfg_llvm_t::get_path_enumeration_stop_pcs(set<pc> &stop_pcs) const
{
  for (auto const& pp : this->get_all_pcs()) {
    if (pp.is_exit() || is_fcall_pc(pp)) {
      stop_pcs.insert(pp);
    }
  }
}

map<pair<pc, int>,  shared_ptr<tfg_edge_composition_t>>
tfg_llvm_t::get_composite_path_between_pcs_till_unroll_factor(pc const& from_pc, set<pc> const& to_pcs, map<pc,int> const& visited_count, int unroll_factor, set<pc> const& stop_pcs) const
{
  map<pair<pc, int>, shared_ptr<tfg_edge_composition_t>> ret_pths = get_composite_path_between_pcs_till_unroll_factor_helper(from_pc, to_pcs, visited_count, 0, unroll_factor, stop_pcs);
  return ret_pths;

}

void
tfg_llvm_t::get_path_enumeration_reachable_pcs(pc const &from_pc, int dst_to_pc_subsubindex, expr_ref const& dst_to_pc_incident_fcall_nextpc, set<pc> &reachable_pcs) const
{
  get_reachable_pcs_stopping_at_fcalls(from_pc, reachable_pcs);
  for (set<pc>::iterator it = reachable_pcs.begin(); it != reachable_pcs.end(); ) {
    if(is_pc_pair_compatible((*it), dst_to_pc_subsubindex, dst_to_pc_incident_fcall_nextpc))
      it++;
    else {
      CPP_DBG_EXEC(GENERAL, cout << __func__ << " " << __LINE__ << ": ignoring to_pc " << (*it).to_string() << " because it is not compatible with subsubindex " << dst_to_pc_subsubindex << ", incident_fcall_nextpc " << (dst_to_pc_incident_fcall_nextpc ? expr_string(dst_to_pc_incident_fcall_nextpc) : "null") << endl);
      it = reachable_pcs.erase(it);
    }
  }
}

void
tfg_llvm_t::tfg_llvm_populate_varname_definedness()
{
  map<pc, defined_varnames_val_t> vals;
  def_ssa_analysis definedness_ssa_analysis(this, vals);
  definedness_ssa_analysis.initialize(defined_varnames_val_t::get_boundary_value(this));
  definedness_ssa_analysis.solve();
  m_defined_ssa_varnames_map.clear();
  for (auto const& v : vals) {
    m_defined_ssa_varnames_map.insert(make_pair(v.first, v.second.get_defined_varnames()));
    CPP_DBG_EXEC(SSA_VARNAME_DEFINEDNESS,
      cout << __func__ << " " << __LINE__ << ": defined varnames at " << v.first.to_string() << " [size " << v.second.get_defined_varnames().size() << "]:";
      for (auto const& s : v.second.get_defined_varnames()) {
        cout << " " << s->get_str();
      }
      cout << endl;
    );
  }
}

void
tfg_llvm_t::tfg_preprocess(bool collapse, list<string> const& sorted_bbl_indices, map<graph_loc_id_t, graph_cp_location> const &llvm_locs)
{
  this->tfg::tfg_preprocess(collapse, sorted_bbl_indices, llvm_locs);
  this->tfg_llvm_populate_varname_definedness();
}

set<expr_ref>
tfg_llvm_t::get_interesting_exprs_till_delta(pc const& p, delta_t delta) const
{
  set<expr_ref> all = this->tfg::get_interesting_exprs_till_delta(p, delta);
  set<expr_ref> ret;
  for (auto const& e : all) {
    if (!e->is_var()) {
      ret.insert(e);
      continue;
    }
    string_ref const &name = e->get_name();
    if (!context::is_llvmvarname(name->get_str())) {
      ret.insert(e);
      continue;
    }
    if (set_belongs(m_defined_ssa_varnames_map.at(p), name)) {
      ret.insert(e);
    }
  }
  return ret;
}

expr_ref
tfg_llvm_t::tfg_edge_composition_get_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified) const
{
  context *ctx = this->get_context();
  if (!src_ec) {
    return expr_true(ctx);
  }
  if(m_ec_econd_map.count(make_pair(src_ec,simplified)))
    return m_ec_econd_map.at(make_pair(src_ec,simplified));
  expr_ref const& ret = this->tfg::tfg_edge_composition_get_edge_cond(src_ec, simplified);
  m_ec_econd_map.insert(make_pair(make_pair(src_ec,simplified), ret));
  return ret;
}

//expr_ref
//tfg_llvm_t::tfg_edge_composition_get_unsubstituted_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, bool simplified) const
//{
//  context *ctx = this->get_context();
//  if (!src_ec) {
//    return expr_true(ctx);
//  }
//  state const &start_state = this->get_start_state();
//  if(m_ec_econd_map.count(make_pair(src_ec,simplified)))
//    return (m_ec_econd_map.at(make_pair(src_ec,simplified))).second;
//  tuple<expr_ref, expr_ref, state> ret = this->tfg_edge_composition_get_to_state_and_edge_cond(src_ec, start_state, simplified);
//  m_ec_econd_map[make_pair(src_ec,simplified)] = make_pair(get<0>(ret), get<1>(ret));
//  return get<1>(ret);
//}

predicate_set_t
tfg_llvm_t::collect_assumes_around_edge(tfg_edge_ref const& te) const
{
  predicate_set_t from_preds;
  if (!te->is_empty()) {
    from_preds = this->collect_assumes_for_edge(te);
    list<tfg_edge_ref> elist = this->get_maximal_basic_block_edge_list_ending_at_pc(te->get_from_pc());
    // SSA form allows us to collect preds not containing memory ops without computing postcondition
    predicate_set_t const& ret_nonmem = this->collect_non_mem_assumes_from_edge_list(elist);
    predicate::predicate_set_union(from_preds, ret_nonmem);

    DYN_DEBUG2(decide_hoare_triple,
        cout << _FNLN_ << ": returning:\n";
        for (auto const& fpred : from_preds) {
        cout << fpred->pred_to_string("  ") << "\n";
        }
        cout << "\n";
        );
  }
  return from_preds;
}

shared_ptr<tfg>
tfg_llvm_t::tfg_copy() const
{
  shared_ptr<tfg> ret = make_shared<tfg_llvm_t>(this->get_name()->get_str(), get_context());
  ret->set_start_state(this->get_start_state());
  list<shared_ptr<tfg_node>> nodes;
  this->get_nodes(nodes);
  for (auto n : nodes) {
    shared_ptr<tfg_node> new_node = make_shared<tfg_node>(*n/*->add_function_arguments(function_arguments)*/);
    ret->add_node(new_node);
    predicate_set_t const &theos = this->get_assume_preds(n->get_pc());
    ret->add_assume_preds_at_pc(n->get_pc(), theos);
  }
  list<shared_ptr<tfg_edge const>> edges;
  get_edges(edges);
  for (auto e : edges) {
    //ASSERT(e->get_from_pc() == e->get_from()->get_pc());
    //ASSERT(e->get_to_pc() == e->get_to()->get_pc());
    shared_ptr<tfg_node> from_node = ret->find_node(e->get_from_pc());
    shared_ptr<tfg_node> to_node = ret->find_node(e->get_to_pc());
    ASSERT(from_node);
    ASSERT(to_node);
    state to_state = e->get_to_state();
    //to_state = to_state.add_function_arguments(function_arguments);
    shared_ptr<tfg_edge const> new_edge = mk_tfg_edge(mk_itfg_edge(from_node->get_pc(), to_node->get_pc(), to_state, e->get_cond()/*, this->get_start_state()*/, e->get_assumes(), e->get_te_comment()));
    ASSERT(from_node->get_pc() == new_edge->get_from_pc());
    ASSERT(to_node->get_pc() == new_edge->get_to_pc());
    ret->add_edge(new_edge);
  }
  //ret->m_arg_regs = m_arg_regs;
  ret->set_argument_regs(this->get_argument_regs());
  ret->set_return_regs(this->get_return_regs());
  ret->set_symbol_map(this->get_symbol_map());
  ret->set_locals_map(this->get_locals_map());
  ret->set_nextpc_map(this->get_nextpc_map());
  //ret->m_locs = m_locs;
  ret->set_locs(this->get_locs());
  ret->set_assume_preds_map(get_assume_preds_map());
  //ret->set_preds_rejection_cache_map(get_preds_rejection_cache_map());
  return static_pointer_cast<tfg>(ret);
}

void
tfg_llvm_t::populate_exit_return_values_for_llvm_method()
{
  map<pc, map<string_ref, expr_ref>> ret;
  list<pc> exit_pcs;
  this->get_exit_pcs(exit_pcs);

  for (const auto &exit_pc : exit_pcs) {
    map<string_ref, expr_ref> return_regs;
    return_regs = this->get_start_state_mem_pools_except_stack_locals_rodata_and_memlocs_as_return_values();

    list<shared_ptr<tfg_edge const>> incoming;
    this->get_edges_incoming(exit_pc, incoming);
    ASSERT(incoming.size() >= 1);
    shared_ptr<tfg_edge const> in_e = incoming.front();
    string k;
    if ((k = in_e->get_to_state().has_expr_substr(LLVM_STATE_INDIR_TARGET_KEY_ID)) != "") {
      return_regs[mk_string_ref(k)] = in_e->get_to_state().get_expr(k/*, this->get_start_state()*/);
    } else {
      NOT_IMPLEMENTED();
    }
    if (   atoi(exit_pc.get_index()) == 0
        && (k = in_e->get_to_state().has_expr_substr(G_LLVM_RETURN_REGISTER_NAME)) != "") {
      expr_ref r = in_e->get_to_state().get_expr(k/*, this->get_start_state()*/);
      return_regs[mk_string_ref(k)] = r;
    }
    ret[exit_pc] = return_regs;
  }
  this->set_return_regs(ret);
}

}
