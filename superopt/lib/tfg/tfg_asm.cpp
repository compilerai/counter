#include "tfg/tfg_asm.h"
#include <set>

using namespace std;

namespace eqspace {

void
tfg_asm_t::tfg_preprocess(bool collapse, list<string> const& sorted_bbl_indices, map<graph_loc_id_t, graph_cp_location> const &llvm_locs)
{
  this->tfg::tfg_preprocess(collapse, sorted_bbl_indices, llvm_locs);
}

shared_ptr<tfg>
tfg_asm_t::tfg_copy() const
{
  shared_ptr<tfg> ret = make_shared<tfg_asm_t>(this->get_name()->get_str(), get_context());
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
}
