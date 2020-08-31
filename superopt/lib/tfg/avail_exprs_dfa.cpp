#include "tfg/avail_exprs_dfa.h"
#include "tfg/tfg.h"

namespace eqspace {

void
tfg::substitute_gen_set_exprs_using_avail_exprs(map<graph_loc_id_t, expr_ref> &gen_set/*, map<expr_ref, set<graph_loc_id_t>> &expr2deps_cache*/, avail_exprs_t const& this_avail_exprs) const
{
  vector<pair<expr_ref,expr_ref>> avail_expr_submap;
  avail_expr_submap.reserve(this_avail_exprs.size());
  context *ctx = this->get_context();
  for (auto const& av : this_avail_exprs) {
    expr_ref e = this->get_loc_expr(av.first);
    //cout << __func__ << " " << __LINE__ << ": inserting into submap: e = " << expr_string(e) << ", av.second(to) = " << expr_string(av.second) << endl;
    avail_expr_submap.push_back(make_pair(e, av.second));
  }
  for (auto &g : gen_set) {
    //cout << __func__ << " " << __LINE__ << ": gen_set.first = " << g.first << ", gen_set.second = " << expr_string(g.second) << endl;
    expr_ref esub = expr_substitute(g.second, avail_expr_submap);
    //cout << __func__ << " " << __LINE__ << ": g.second = " << expr_string(g.second) << endl;
    //cout << __func__ << " " << __LINE__ << ": esub = " << expr_string(esub) << endl;
    expr_ref ge_sub = ctx->expr_do_simplify(esub);
    if (ge_sub != g.second) {
      //cout << "Substituting " << expr_string(g.second) <<" with " << expr_string(ge_sub) << endl;
      g.second = ge_sub;
    }
  }
}

avail_exprs_analysis::avail_exprs_analysis(graph<pc, tfg_node, tfg_edge> const* g, map<pc, avail_exprs_val_t> &init_vals)
  : data_flow_analysis<pc, tfg_node, tfg_edge, avail_exprs_val_t, dfa_dir_t::forward>(g, init_vals)
{
  ASSERT(dynamic_cast<tfg const*>(g) != nullptr);
}

bool
avail_exprs_analysis::xfer_and_meet(shared_ptr<tfg_edge const> const &e, avail_exprs_val_t const& in, avail_exprs_val_t &out)
{
  //autostop_timer func_timer(string("avail_exprs_analysis.") + __func__);

  map<graph_loc_id_t, expr_ref> gen_set;
  set<graph_loc_id_t> killed_locs;
  tfg const *g  = dynamic_cast<tfg const*>(m_graph);
  avail_exprs_t this_avail_exprs = in.get_avail(); // take a copy

  // populate gen and kill sets
  g->populate_gen_and_kill_sets_for_edge(e, gen_set, killed_locs/*, m_expr2deps_cache*/);
  // use existing available exprs to simplify the exprs
  g->substitute_gen_set_exprs_using_avail_exprs(gen_set/*, m_expr2deps_cache*/, this_avail_exprs);

  for (const auto &k : killed_locs) {
    // killed loc is holding a different expr now
    this_avail_exprs.erase(k);
    //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << ": killed loc " << k << endl;
  }

  // insert the gen set
  this_avail_exprs.insert(gen_set.begin(), gen_set.end());
  for (const auto &g : gen_set) {
    //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << ": gened loc " << g.first << endl;
  }

  // remove expressions which depend on a killed loc
  // Note that we are doing it on the out set
  // This is required because the expressions are referring to src PC value of loc
  auto itr = this_avail_exprs.begin();
  while (itr != this_avail_exprs.end()) {
    set<graph_loc_id_t> deps = dynamic_cast<tfg const *>(m_graph)->get_locs_potentially_read_in_expr(itr->second);
    set_intersect(deps, killed_locs);
    if (deps.size()/* || (!g->loc_is_live_at_pc(e->get_to_pc(), itr->first))*/) { //do not mix liveness with avail-exprs as avail-exprs are computed for sprels even before liveness is computed
      //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << ": erasing loc " << itr->first << ": deps.size() = " << deps.size() << endl;
      itr = this_avail_exprs.erase(itr);
    } else {
      ++itr;
    }
  }

  //cout << "edge " << e->to_string() << ", out:\n";
  //for (const auto &lem: this_avail_exprs) {
  //  cout << " loc  " << lem.first << " -> " << expr_string(lem.second) << '\n';
  //}
  return out.meet(this_avail_exprs);
}

map<pc, avail_exprs_t>
tfg::compute_loc_avail_exprs() const
{
  autostop_timer func_timer(__func__);

  map<pc, avail_exprs_val_t> vals;
  avail_exprs_analysis av_exprs(this, vals);
  av_exprs.initialize(avail_exprs_val_t());
  av_exprs.solve();

  map<pc, avail_exprs_t> ret;
  for (const auto &v : vals) {
    ret.insert(make_pair(v.first, v.second.get_avail()));
  }
  return ret;
}

void
tfg::print_loc_avail_expr(map<pc, avail_exprs_t> loc_avail_exprs) const
{
  cout << __func__ << " " << __LINE__ << ": Printing available expressions:\n";
  for (const auto &plem : loc_avail_exprs) {
    string const& pc_str = plem.first.to_string();
    for (const auto &lem: plem.second) {
      cout << pc_str << ": loc  " << lem.first << " -> " << expr_string(lem.second) << '\n';
    }
    cout << '\n';
  }
}

}
