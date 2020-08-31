#include "support/debug.h"
#include "graph/pred_avail_exprs_dfa.h"
#include "tfg/tfg.h"

#define DBG_NAME "pred_avail_exprs"

namespace eqspace {
  
pred_avail_exprs_analysis::pred_avail_exprs_analysis(graph<pc, tfg_node, tfg_edge> const* g, map<pc,tfg_suffixpath_t> const &suffixpaths, map<pc, pred_avail_exprs_val_t> &init_vals) : 
  data_flow_analysis<pc, tfg_node, tfg_edge, pred_avail_exprs_val_t, dfa_dir_t::forward>(g, init_vals),
  m_suffixpaths(suffixpaths)
  {
    //pred_avail_exprs_t()
  }

static string
pred_avail_expr_to_string(pred_avail_exprs_t const &pls)
{
  stringstream ss;
  for (const auto &sf_av: pls) {
    ss << " suffixpath: " << graph_edge_composition_to_pos_string(sf_av.first) << '\n';
    //ss << " suffixpath[" << sf_av.first << "]: " << sf_av.first->graph_edge_composition_to_string() << '\n';
    for (const auto &loc_expr: sf_av.second) {
      ss << "  loc  " << loc_expr.first << " -> [" << loc_expr.second << "] " << expr_string(loc_expr.second) << '\n';
    }
  }
  return ss.str();
}

void
tfg::print_pred_avail_exprs(map<pc, pred_avail_exprs_t> const &pred_avail_exprs) const
{
  cout << __func__ << " " << __LINE__ << ": Printing predicated available expressions:\n";
  for (const auto &plem : pred_avail_exprs) {
    pc const &p = plem.first;
    cout << '\n' << p.to_string() << ": \n";
    cout << pred_avail_expr_to_string(plem.second);
  }
}

/*pred_avail_exprs_t
pred_avail_exprs_analysis::copy_live_locs_from_in(pred_avail_exprs_t const &in, tfg const *t, pc const &p)
{
  pred_avail_exprs_t ret;
  for (const auto &suffixpath_m : in) {
    const auto &suffixpath = suffixpath_m.first;
    for (const auto &l : t->get_live_locids(p)) {
      if (suffixpath_m.second.count(l)) {
        //cout << __func__ << " " << __LINE__ << ": p = "  << p.to_string() << endl;
        //cout << "l = " << l << endl;
        //cout << "t->graph_loc_get_value_for_node(l) = " << t->graph_loc_get_value_for_node(l) << endl;
        ret[suffixpath][l] = suffixpath_m.second.at(l);
      }
    }
  }
  return ret;
}*/

avail_exprs_t
meet_avail_exprs(avail_exprs_t const& a, avail_exprs_t const& b)
{
  avail_exprs_t ret = a;
  for (auto const& ple : b) {
    if (   ret.count(ple.first)
        && ret.at(ple.first) != ple.second) {
      context* ctx = ple.second->get_context();
      ret[ple.first] = ctx->mk_uninit_var(ple.second->get_sort());
    } else {
      ret.insert(ple);
    }
  }
  return ret;
}

/* transfer function for predicated available expression analysis:
 *
 * Algorithm:
 *
 * 1. Find gen and kill sets i.e., locs generated and killed over this edge
 * 2. Remove killed locs from `in` set
 * 3. For the remaining elements in `in` set:
 * 3.1 Kill locs dependent on killed locs (dep-kill)
 * 3.2 update suffixpath to include new edge and store
 * 4. Calculate suffixpath ending at this edge
 * 5 For locs in gen set:
 * 5.1 Store (suffixpath, expr)
 * 6. return
 */
bool
pred_avail_exprs_analysis::xfer_and_meet(shared_ptr<tfg_edge const> const &e, pred_avail_exprs_val_t const& in, pred_avail_exprs_val_t &out)
{
  mytimer t1(__func__);
  t1.start();

  map<graph_loc_id_t, expr_ref> gen_set;
  set<graph_loc_id_t> killed_locs;
  tfg const* g = dynamic_cast<tfg const*>(this->m_graph);
  auto this_out = in.get_avail(); // take a copy
  context *ctx = g->get_context();

  // populate gen and kill sets

  g->populate_gen_and_kill_sets_for_edge(e, gen_set, killed_locs/*, m_expr2deps_cache*/);
  CPP_DBG_EXEC2(PRED_AVAIL_EXPR, cout << __func__ << ':' << __LINE__ << ": " << e->to_string_concise() << " gen set:\n"; for (auto const& g : gen_set) cout << g.first << " -> " << expr_string(g.second) << endl; cout << endl);
  CPP_DBG_EXEC2(PRED_AVAIL_EXPR, cout << __func__ << ':' << __LINE__ << ": " << e->to_string_concise() << " kill set: "; for (auto const& k : killed_locs) cout << k << ' '; cout << endl);
  for (auto &gen : gen_set) {
    set<graph_loc_id_t> deps = dynamic_cast<tfg const *>(m_graph)->get_locs_potentially_read_in_expr(gen.second);
    set_intersect(deps, killed_locs);
    if (deps.size()) {
      gen.second = ctx->mk_uninit_var(gen.second->get_sort());
    }
  }
  // kill and dep-kill
  for (auto& sf_av : this_out) {
    auto& av = sf_av.second;
    for (const auto&k : killed_locs) {
      av.erase(k); // killed loc is holding a different expr now
    }
    for (auto& loc_expr : av) {
      if (!loc_expr.second->is_uninit()) {
        set<graph_loc_id_t> deps = dynamic_cast<tfg const *>(m_graph)->get_locs_potentially_read_in_expr(loc_expr.second);
        set_intersect(deps, killed_locs);
        if (deps.size()/*|| !g->loc_is_live_at_pc(e->get_to_pc(), loc_expr.first)*/) {
          loc_expr.second = ctx->mk_uninit_var(loc_expr.second->get_sort());
        }
      }
    }
  }

  // update the suffix path of existing exprs
  vector<tfg_suffixpath_t> sfs(this_out.size());
  size_t i = 0;
  for (const auto& sf_av : this_out) { sfs[i++] = sf_av.first; }
  for (const auto& sf : sfs) {
    auto new_sf = mk_suffixpath_series_with_edge(sf, e);
    if (this_out.count(new_sf)) {
      this_out[new_sf] = meet_avail_exprs(this_out.at(new_sf),this_out.at(sf));
    } else {
      swap(this_out[new_sf], this_out[sf]);
    }
    this_out.erase(sf);
  }

  // insert the gen set
  pc const& from_pc = e->get_from_pc();
  ASSERT(m_suffixpaths.count(from_pc) > 0); // we need suffix path at from_pc
  auto out_sf = mk_suffixpath_series_with_edge(m_suffixpaths.at(from_pc), e);
  if (   !gen_set.empty()
      && this_out.count(out_sf) == 0) {
    this_out[out_sf] = {};
  }
  for (auto const& gen : gen_set) {
    const auto& loc = gen.first;
    const auto& e = gen.second;
    // assert that this loc was not already present
    ASSERT(this_out.at(out_sf).insert(make_pair(loc, e)).second);
  }

  t1.stop();
  CPP_DBG_EXEC(PRED_AVAIL_EXPR, cout << __func__ << ':' << __LINE__ << " xfer " << e->to_string() << " took " << t1.get_elapsed() / 1.0e6 <<  endl);
  CPP_DBG_EXEC2(PRED_AVAIL_EXPR, cout << "pred_avail_exprs in:\n" << pred_avail_expr_to_string(in.get_avail()));
  CPP_DBG_EXEC2(PRED_AVAIL_EXPR, cout << "pred_avail_exprs out:\n" << pred_avail_expr_to_string(this_out));

  return out.meet(this_out, g);
}

void populate_disjoint_suffixpaths(vector<set<pair<tfg_suffixpath_t,expr_ref>>>& disjoint_sfs, map<tfg_suffixpath_t,expr_ref> const& a, map<tfg_suffixpath_t,expr_ref> const& b, map<pair<tfg_suffixpath_t,tfg_suffixpath_t>,bool> const& nondisjointedness_cache)
{
  map<tfg_suffixpath_t, size_t> tag;

  disjoint_sfs.clear();
  for (const auto& ea : a) {
    auto const& sf_a = ea.first;
    auto const& expr_a = ea.second;
    bool sf_a_is_disjoint = true;

    for (const auto& eb : b) {
      auto const& sf_b = eb.first;
      auto const& expr_b = eb.second;

      // check if are non-disjoint
      auto itr_ndisjn = nondisjointedness_cache.find(make_pair(sf_a,sf_b));
      ASSERT(itr_ndisjn != nondisjointedness_cache.end());
      if (itr_ndisjn->second) {
        sf_a_is_disjoint = false;
        auto it_b = tag.find(sf_b);
        if (it_b != tag.end()) {
          // sf_b is already part of some set, add sf_a to it
          auto& sf_b_set = disjoint_sfs.at(it_b->second);

          auto it_a = tag.find(sf_a);
          if (   it_a != tag.end()
              && it_a->second != it_b->second) {
            // if sf_a is also part of some different set then
            // 1. update tag of its members
            // 2. fold sf_a's set into sf_b's set
            // 3. clear sf_a's set
            auto sf_a_old_tag = it_a->second;
            for (auto const& sf_a_element : disjoint_sfs.at(sf_a_old_tag)) {
              tag[sf_a_element.first] = it_b->second;
            }
            set_union(sf_b_set, disjoint_sfs.at(sf_a_old_tag));
            disjoint_sfs.at(sf_a_old_tag) = {};
          }
          else {
            sf_b_set.insert(make_pair(sf_a,expr_a));
            tag[sf_a] = it_b->second; // update sf_a's tag
          }
        }
        else {
          // if sf_a is part of some disjoint group, then add sf_b to it
          // otherwise create a new disjoint set containing sf_a and sf_b
          auto it_a = tag.find(sf_a);
          if (it_a != tag.end()) {
            disjoint_sfs.at(it_a->second).insert(make_pair(sf_b,expr_b));
            tag[sf_b] = it_a->second;
          }
          else {
            disjoint_sfs.push_back({ make_pair(sf_a,expr_a), make_pair(sf_b,expr_b) });
            tag[sf_a] = disjoint_sfs.size()-1;
            tag[sf_b] = disjoint_sfs.size()-1;
          }
        }
      }
    }
    if (sf_a_is_disjoint)
      disjoint_sfs.push_back({ make_pair(sf_a,expr_a) });
  }
  // add remaining b's suffixpaths to disjoint_sfs
  for (const auto& eb : b) {
    auto const& sf_b = eb.first;
    if (tag.count(sf_b) == 0) {
      disjoint_sfs.push_back({ make_pair(sf_b,eb.second) });
    }
  }
}

long make_sf_parallel_hits = 0;
long make_sf_parallel_misses = 0;

// TODO: limit size of cache?
tfg_suffixpath_t
pred_avail_exprs_analysis::make_sf_from_parallel_sf(set<pair<tfg_suffixpath_t,expr_ref>> const& sfs, map<vector<tfg_suffixpath_t>,tfg_suffixpath_t> &make_parallel_cache)
{
  vector<tfg_suffixpath_t> sfv; sfv.reserve(sfs.size());
  for (auto const& sfe : sfs) { sfv.push_back(sfe.first); }
  if (make_parallel_cache.count(sfv)) { ++make_sf_parallel_hits; return make_parallel_cache.at(sfv); }
  else {
    ++make_sf_parallel_misses;
    auto this_sf = graph_edge_composition_construct_edge_from_parallel_edgelist(list<tfg_suffixpath_t>(sfv.begin(), sfv.end()));
    make_parallel_cache[sfv] = this_sf;
    return this_sf;
  }
}

void
pred_avail_exprs_analysis::verify_meet_invariant(pred_avail_exprs_t const& this_out, pred_avail_exprs_t const& a, pred_avail_exprs_t const& b)
{
  for (auto const& p1 : this_out) {
    auto sf1 = p1.first;
    for (auto const& p2: this_out) {
      auto sf2 = p2.first;
      if (sf1 != sf2 && !graph_edge_composition_are_disjoint(sf1, sf2)) {
        set<graph_loc_id_t> locs1, locs2, intersection;
        for (auto const& p : p1.second) { locs1.insert(p.first); }
        for (auto const& p : p2.second) { locs2.insert(p.first); }
        set_intersect(locs1, locs2, intersection);
        if (intersection.size()) {
          // graph_edge_composition_are_disjoint returns an over-approximate answer.  Just signalling a warning is enough.
          CPP_DBG_EXEC(WRN, cout << DBG_NAME << "::" << __func__ << ": WARNING: " << " found potentially non-disjoint edge compositions where disjoint were expected.  Please verify!\n");
          CPP_DBG_EXEC3(PRED_AVAIL_EXPR, cout << "sf1: " << sf1->graph_edge_composition_to_string() << "\nsf2: " << sf2->graph_edge_composition_to_string() << "\nintersection: ";
                            for (auto const& a : intersection) { cout << a << ' '; }
                            cout << "\nIN a:\n" << pred_avail_expr_to_string(a);
                            cout << "\nIN b:\n" << pred_avail_expr_to_string(b);
                            cout << "\nOUT:\n" << pred_avail_expr_to_string(this_out));
        }
      }
    }
  }
}


/* meet function for predicated available expression analysis:
 *
 * Invariant respected and expected: No same loc can be present in two non-disjoint paths
 * Implication: Every conflict will arise out of suffixpaths coming from both `a' and `b'.
 *
 * Useful but not really important: dedup same <loc,expr> into a single suffixpath
 *
 * Algorithm:
 *
 * 1. Cache "disjointedness" information for each suffixpath pair (sfa, sfb)
 * 2. For each loc,
 *   2.1 Form set of non-disjoint paths
 *   2.2 Resolve each non-disjoint path
 * 3. Dedup paths with same avail expr to preserve minimality
 *
 * The following data structures are used for efficiency:
 * 1. disjoint_sfs: vector<set<pair<tfg_suffixpath_t,expr_ref>>>
 *    Each element stoes set of suffixpaths which are non-disjoint
 * 2. tag: map<tfg_suffixpath_t, int>
 *    Stores index into disjoint_sfs at which suffixpath is present
 */
bool
pred_avail_exprs_val_t::meet(pred_avail_exprs_val_t const& other, tfg const *g)
{
  ASSERT(!other.m_is_top);
  // fast path: just use other value if we are top
  if (this->m_is_top) {
    *this = other;
    return true;
  }

  mytimer t1(__func__);
  t1.start();

  context* ctx = g->get_context();

  // step 1: for each loc, populate the inverse map; loc -> (suffixpath, expr)

  set<graph_loc_id_t> all_locs;
  map<graph_loc_id_t, map<tfg_suffixpath_t,expr_ref>> loc2sfexpr_a;
  map<graph_loc_id_t, map<tfg_suffixpath_t,expr_ref>> loc2sfexpr_b;
  auto populate_loc2sfexpr = [&all_locs](map<graph_loc_id_t, map<tfg_suffixpath_t,expr_ref>>& loc2sfexpr, tfg_suffixpath_t const& sf, avail_exprs_t const& av) -> void {
    for (const auto& l : av) {
      all_locs.insert(l.first);
      if (loc2sfexpr.count(l.first) == 0) { loc2sfexpr[l.first] = {}; }
      auto itr = loc2sfexpr.find(l.first);
      ASSERT (itr->second.count(sf) == 0);
      (itr->second)[sf] = l.second;
    }
  };

  for (const auto& ea : this->get_avail()) {
    populate_loc2sfexpr(loc2sfexpr_a, ea.first, ea.second);
  }
  for (const auto& eb : other.get_avail()) {
    populate_loc2sfexpr(loc2sfexpr_b, eb.first, eb.second);
  }

  CPP_DBG_EXEC3(PRED_AVAIL_EXPR, cout << __func__ << ':' << __LINE__ << " loc2sfexpr_a: ";
                     for (const auto& lsfe : loc2sfexpr_a) {
                       cout << "loc" << lsfe.first << ":\n";
                       for (const auto& sfe: lsfe.second) cout << sfe.first->graph_edge_composition_to_string() << ": " << expr_string(sfe.second) << endl;
                       cout << "\n";
                     }
                     cout << "\nloc2sfexpr_b: ";
                     for (const auto& lsfe : loc2sfexpr_b) {
                       cout << "loc" << lsfe.first << ":\n";
                       for (const auto& sfe: lsfe.second) cout << sfe.first->graph_edge_composition_to_string() << ": " << expr_string(sfe.second) << endl;
                       cout << "\n";
                     }
               );


  pred_avail_exprs_t this_out;
  map<pair<tfg_suffixpath_t,tfg_suffixpath_t>,bool> nondisjointedness_cache; // 0 -> are disjoint, 1 -> non-disjoint
  /* map<vector<tfg_suffixpath_t>,tfg_suffixpath_t> make_parallel_cache; */

  // step 2: cache disjointedness information

  for (const auto& ea : this->get_avail()) {
    auto const& sf_a = ea.first;
    for (const auto& eb : other.get_avail()) {
      auto const& sf_b = eb.first;
      bool res = (sf_a == sf_b) || !graph_edge_composition_are_disjoint(sf_a, sf_b);
      nondisjointedness_cache[make_pair(sf_a,sf_b)] = res;
    }
  }

  // step 3: for each loc, construct set of non-disjoint paths and resolve conflicts using this information

  map<tfg_suffixpath_t,expr_ref> empty_sfe;
  for (auto const& locid : all_locs) {
    vector<set<pair<tfg_suffixpath_t,expr_ref>>> disjoint_sfs;

    auto itr_a = loc2sfexpr_a.find(locid);
    auto itr_b = loc2sfexpr_b.find(locid);
    populate_disjoint_suffixpaths(disjoint_sfs,
                                  itr_a == loc2sfexpr_a.end() ? empty_sfe : itr_a->second,
                                  itr_b == loc2sfexpr_b.end() ? empty_sfe : itr_b->second,
                                  nondisjointedness_cache);

    CPP_DBG_EXEC4(PRED_AVAIL_EXPR, cout << __func__ << ':' << __LINE__ << "loc" << locid << " disjoint_sfs:\n");
    for (const auto& set_sfs : disjoint_sfs) {
      CPP_DBG_EXEC4(PRED_AVAIL_EXPR, cout << "{ ";
                         for (auto const& sfe : set_sfs) { cout << "    " << sfe.first->graph_edge_composition_to_string() << ": " << expr_string(sfe.second) << endl; }
                         cout << "}\n");

      if (set_sfs.size() > 1) {
        // >= 2 non-disjoint suffixpaths
        // A conflict must be resolved: check if all suffixpaths agree on same expr
        list<tfg_suffixpath_t> sfl;
        for (auto const& sfe : set_sfs) { sfl.push_back(sfe.first); }
        auto this_sf = graph_edge_composition_construct_edge_from_parallel_edgelist(sfl);
        /* auto this_sf = make_sf_from_parallel_sf(set_sfs, make_parallel_cache); */

        expr_ref ref_expr = nullptr;
        bool all_same = true;
        for (const auto& sfe : set_sfs) {
          if (ref_expr == nullptr) { ref_expr = sfe.second; }
          else if (ref_expr != sfe.second) { all_same = false; }
          if (!all_same) break;
        }

        if (this_out.count(this_sf) == 0) { this_out[this_sf] = { }; }
        if (all_same) {
          ASSERT(ref_expr != nullptr);
          this_out[this_sf][locid] = ref_expr;
        }
        else {
          this_out[this_sf][locid] = ctx->mk_uninit_var(g->graph_loc_get_value_for_node(locid)->get_sort());
        }
      }
      else if (set_sfs.size() == 1) {
        tfg_suffixpath_t this_sf = set_sfs.begin()->first;
        if (this_out.count(this_sf) == 0) { this_out[this_sf] = { }; }
        this_out[this_sf][locid] = set_sfs.begin()->second;
      }
    }
  }

  // step 3 : dedup same <loc,expr> under disjoint suffixpaths -- invariant is still respected

  // populate loc-to-set of suffixpaths map
  map<pair<graph_loc_id_t,expr_ref>, set<tfg_suffixpath_t>> exprloc2sfs;
  for (const auto& sf_av : this_out) {
    for (const auto& av : sf_av.second) {
      auto pp = make_pair(av.first, av.second);
      if (exprloc2sfs.count(pp) == 0) { exprloc2sfs[pp] = {}; }
      exprloc2sfs[pp].insert(sf_av.first);
    }
  }
  for (const auto& exprloc_sfs : exprloc2sfs) {
    const auto& pp = exprloc_sfs.first;
    const auto& sfs = exprloc_sfs.second;
    if (sfs.size() >= 2) {
      list<tfg_suffixpath_t> sfl;
      for (const auto& sf : sfs) {
        this_out.at(sf).erase(pp.first);
        sfl.push_back(sf);
      }
      auto this_sf = graph_edge_composition_construct_edge_from_parallel_edgelist(sfl);
      if (this_out.count(this_sf) == 0) { this_out[this_sf] = {}; }
      this_out[this_sf][pp.first] = pp.second;
    }
  }

  // clean up: remove suffixpaths which contain empty avail_exprs_t

  for (auto it = this_out.begin(); it != this_out.end(); ) {
    if (it->second.empty())
      it = this_out.erase(it);
    else ++it;
  }

  // assertion check -- ensures that no loc is lost in meet
  set<graph_loc_id_t> locs_after;
  for (const auto& sfle : this_out) {
    for (const auto& le : sfle.second) {
      locs_after.insert(le.first);
    }
  }
  if (all_locs.size() != locs_after.size()) {
    cout << "locs_before: ";
    for (auto const& l : all_locs) { cout << l << ' '; }
    cout << "\nlocs_after: ";
    for (auto const& l : locs_after) { cout << l << ' '; }
    cout << "\nIN a:\n" << pred_avail_expr_to_string(this->get_avail());
    cout << "\nIN b:\n" << pred_avail_expr_to_string(other.get_avail());
    cout << "\nOUT:\n" << pred_avail_expr_to_string(this_out);
    cout.flush();
  }
  ASSERT(all_locs.size() == locs_after.size());

  // assertion check -- ensures that our invariant is respected
  CPP_DBG_EXEC(PRED_AVAIL_EXPR, pred_avail_exprs_analysis::verify_meet_invariant(this_out, this->get_avail(), other.get_avail()));
  t1.stop();
  CPP_DBG_EXEC(PRED_AVAIL_EXPR, cout << DBG_NAME << "::" << __func__ << " took " << t1.get_elapsed() / 1.0e6 << endl);

  if (this_out != this->get_avail()) {
    this->m_avail = this_out;
    return true;
  }
  return false;
}

bool 
pred_avail_exprs_analysis::postprocess(bool changed) { 

// remove all uninits
  for (auto &plse: m_vals) {
    auto itr = plse.second.get_avail_ref().begin();
    while (itr != plse.second.get_avail_ref().end()) {
      auto itr2 = itr->second.begin();
      while (itr2 != itr->second.end()) {
        if (itr2->second->is_uninit())
          itr2 = itr->second.erase(itr2);
        else
          ++itr2;
      }
      if (itr->second.empty())
        itr = plse.second.get_avail_ref().erase(itr);
      else
        ++itr;
    }
  }

return changed; 
}

void
tfg::populate_pred_avail_exprs()
{
  autostop_timer func_timer(__func__);

  ASSERT(m_suffixpaths.size());
  map<pc, pred_avail_exprs_val_t> vals;
  pred_avail_exprs_analysis pred_av_exprs(this, m_suffixpaths, vals);
  pred_av_exprs.initialize(pred_avail_exprs_val_t::template get_boundary_value<pc, tfg_node, tfg_edge, predicate>(this, m_suffixpaths, pred_av_exprs.get_all_uninit_ref()));
  pred_av_exprs.solve();

  m_pred_avail_exprs.clear();
  for (auto const &v : vals) {
    m_pred_avail_exprs.insert(make_pair(v.first, v.second.get_avail()));
  }
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
std::function<pred_avail_exprs_val_t (pc const &)>
pred_avail_exprs_val_t::get_boundary_value(graph<T_PC, T_N, T_E> const *g, map<pc, tfg_suffixpath_t> const &suffixpaths, avail_exprs_t &all_uninit)
{
  auto f = [g, &suffixpaths, &all_uninit](T_PC const &p)
  {
    tfg const* gp = dynamic_cast<tfg const*>(g);   
    ASSERT(gp);
    T_PC const& entry = gp->find_entry_node()->get_pc();
    ASSERT(p == entry);
  
    if (!all_uninit.size()) {
      context *ctx = gp->get_context();
      // initialize all locs to uninit for entry
      for (auto const &locid_expr: gp->get_locid2expr_map()) {
        all_uninit[locid_expr.first] = ctx->mk_uninit_var(locid_expr.second->get_sort());
      }
    }
  
    pred_avail_exprs_t ret;
    ret[suffixpaths.at(entry)] = all_uninit;
    return ret;
  };
  return f;
}

expr_ref
pred_avail_exprs_and(pred_avail_exprs_t const &pred_avail_exprs, expr_ref const &in, tfg const &t, tfg_suffixpath_t const &cg_src_suffixpath)
{
  autostop_timer ft(__func__);
  expr_ref cur = in;
  context* ctx = in->get_context();
  map<graph_loc_id_t, expr_ref> const &locid2expr = t.get_locid2expr_map();
  for (const auto& sf_av : pred_avail_exprs) {
    const auto& sf = sf_av.first;
    // if the corr graph src suffixpath implies the suffixpath of predicated expr
    // (i.e. the pred suffixpath expr is weaker than cg src suffixpath expr)
    // then we can safely skip the implication
    expr_ref sfexpr = tfg::tfg_suffixpath_implies(cg_src_suffixpath, sf) ? expr_true(ctx)
                                                                         : t.tfg_suffixpath_get_expr(sf);

    for (const auto& p_loc_expr : sf_av.second) {
      auto const& locid_expr = locid2expr.at(p_loc_expr.first);
      auto const& expr = p_loc_expr.second;
      cur = (sfexpr == expr_true(ctx)) ? expr_and(cur, expr_eq(locid_expr, expr))
                 : expr_and(cur, expr_implies(sfexpr, expr_eq(locid_expr, expr)));
    }
  }
  return cur;
}



}
