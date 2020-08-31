#include "tfg/avail_exprs.h"
#include "tfg/tfg.h"
#include "graph/graph_fixed_point_iteration.h"

namespace eqspace {

string
pred_avail_exprs_to_string(pred_avail_exprs_t const &pred_avail_exprs)
{
  stringstream ss;
  for (const auto &sf_locexpr : pred_avail_exprs) {
    ss << "=suffixpath\n"
      << sf_locexpr.first->graph_edge_composition_to_string_for_eq("=src_suffixpath");
    for (const auto &locid_expr : sf_locexpr.second) {
      context *ctx = locid_expr.second->get_context();
      ss << "=loc " << locid_expr.first << '\n'
         << "=expr\n" << ctx->expr_to_string_table(locid_expr.second, true) << '\n';
    }
  }
  return ss.str();
}

bool
pred_avail_exprs_contains(pred_avail_exprs_t const &haystack, pred_avail_exprs_t const &needle)
{
  for (auto n = needle.begin(); n != needle.end(); ++n) {
    set<graph_loc_id_t> nlocs;
    for (const auto& le : n->second) { nlocs.insert(le.first); }

    bool found = false;
    for (auto h = haystack.begin(); h != haystack.end(); ++h) {
      set<graph_loc_id_t> hlocs;
      for (const auto& le : h->second) { hlocs.insert(le.first); }

      if (   set_contains(hlocs, nlocs) // cheaper test first
          && tfg::tfg_suffixpath_implies(h->first, n->first)) {
        // check if exprs also match
        bool match = true;
        for (const auto& l : nlocs) {
          if (n->second.at(l) != h->second.at(l)) {
            match = false;
            break;
          }
        }
        if (match) {
          found = true;
          break;
        }
      }
    }
    if (!found) return false;
  }
  return true;
}

void
tfg::populate_gen_and_kill_sets_for_edge(tfg_edge_ref const &e, map<graph_loc_id_t, expr_ref> &gen_set, set<graph_loc_id_t> &killed_locs) const
{
  map<graph_loc_id_t, expr_ref> locs_written = this->get_locs_definitely_written_on_edge(e);
  for (const auto &m : locs_written) {
    gen_set.insert(m);
  }
  
  map<graph_loc_id_t, expr_ref> locs_potentially_written = this->get_locs_potentially_written_on_edge(e);
  for (const auto &l : locs_potentially_written) {
    killed_locs.insert(l.first);
  }
}

}
