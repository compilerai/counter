#include "eq/cg_with_dst_ml_check.h"
#include "eq/corr_graph.h"
#include "tfg/predicate_set.h"
#include "eq/unsafe_cond.h"

namespace eqspace {

predicate_set_t
cg_with_dst_ml_check::tfg_ec_memlabel_assumptions_negated(shared_ptr<tfg_edge_composition_t> const& tfg_ec, tfg const& t, corr_graph const& cg, pcpair const& pp, graph_memlabel_map_t const& dst_memlabel_map)
{
  tfg const& src_tfg = cg.get_src_tfg();
  tfg const& dst_tfg = cg.get_dst_tfg();
  ASSERT(t.get_name() == src_tfg.get_name() || t.get_name() == dst_tfg.get_name());
  ASSERT(src_tfg.get_name() != dst_tfg.get_name());
  graph_memlabel_map_t const& memlabel_map_to_use = (t.get_name() == src_tfg.get_name()) ? cg.get_memlabel_map() : dst_memlabel_map;
  std::function<predicate_set_t (tfg_edge_ref const&)> fgen =
    [&t, &memlabel_map_to_use](tfg_edge_ref const& te)
  {
    return te->tfg_edge_get_pathcond_pred_ls_for_is_not_memlabel(t, memlabel_map_to_use);
  };
  return get_unsafe_expressions_using_generator_func(tfg_ec, t, cg, pp, fgen);
}

bool
cg_with_dst_ml_check::check_dst_mls() const
{
  autostop_timer ft(__func__);

  cg_with_dst_ml_check cg(*this);
  graph_memlabel_map_t dst_memlabel_map = cg.cg_eliminate_dst_memlabels_in_memlabel_map();
  std::function<predicate_set_t (shared_ptr<tfg_edge_composition_t> const&, tfg const&, corr_graph const&, pcpair const&)> f_unsafe_cond =
    [&dst_memlabel_map](shared_ptr<tfg_edge_composition_t> const& ec, tfg const& t, corr_graph const& cg, pcpair const& pp)
  {
    return tfg_ec_memlabel_assumptions_negated(ec, t, cg, pp, dst_memlabel_map);
  };
  return cg.check_safety_for(unsafe_condition_t(f_unsafe_cond));
}

graph_memlabel_map_t
cg_with_dst_ml_check::cg_eliminate_dst_memlabels_in_memlabel_map()
{
  context* ctx = this->get_context();
  graph_memlabel_map_t old_dst_memlabel_map = this->m_dst_memlabel_map;
  memlabel_ref ml_all_relevant = mk_memlabel_ref(this->get_memlabel_assertions().get_memlabel_all());
  for (auto& mm : this->m_dst_memlabel_map) {
    mm.second = ml_all_relevant;
  }
  this->graph_with_paths_clear_aliasing_constraints_cache();
  return old_dst_memlabel_map;
}

graph_memlabel_map_t
cg_with_dst_ml_check::get_memlabel_map() const
{
    graph_memlabel_map_t memlabel_map = this->get_src_tfg().get_memlabel_map();
    memlabel_map_union(memlabel_map, m_dst_memlabel_map);
    return memlabel_map;
}

aliasing_constraints_t
cg_with_dst_ml_check::get_aliasing_constraints_for_edge(corr_graph_edge_ref const& e) const
{
  autostop_timer ft(string("cg_with_dst_ml_check::")+__func__);

  tfg const& src_tfg = this->get_src_tfg();
  pcpair const& from_pc = e->get_from_pc();
  shared_ptr<tfg_edge_composition_t> src_ec = e->get_src_edge();

  // collect just the src aliasing constraints as we are proving the dst ones
  aliasing_constraints_t ret;
  ret.aliasing_constraints_union(src_tfg.graph_with_paths<pc, tfg_node, tfg_edge, predicate>::collect_aliasing_constraints_around_path(from_pc.get_first(), src_ec));

  // further, assume that the current value of the stack pointer is within the stack region
  tfg const& dst_tfg = this->get_dst_tfg();
  shared_ptr<tfg_edge_composition_t> dst_ec = e->get_dst_edge();
  ret.aliasing_constraints_union(dst_tfg.graph_with_paths<pc, tfg_node, tfg_edge, predicate>::collect_stack_sanity_assumptions_around_path(from_pc.get_second(), dst_ec));

  std::function<expr_ref (expr_ref const&)> f = [this, &from_pc](expr_ref const& e)
  {
    return this->expr_simplify_at_pc(e, from_pc);
  };
  ret.visit_exprs(f);
  //cout << "cg_with_dst_ml_check::" << __func__ << " " << __LINE__ << ": returning for " << e->to_string() << ":\n" << ret.to_string_for_eq() << endl;
  return ret;
}


}
