#include "gsupport/sprel_map_pair.h"
#include "gsupport/sprel_map.h"
#include "support/debug.h"

namespace eqspace {

sprel_map_pair_t::sprel_map_pair_t(sprel_map_t const &src_sprel/*, map<graph_loc_id_t, expr_ref> const &src_locid2expr_map*/)
{
  m_submap = src_sprel.sprel_map_get_submap(/*src_locid2expr_map*/);
}

sprel_map_pair_t::sprel_map_pair_t(sprel_map_t const &src_sprel/*, map<graph_loc_id_t, expr_ref> const &src_locid2expr_map*/, sprel_map_t const &dst_sprel/*, map<graph_loc_id_t, expr_ref> const &dst_locid2expr_map*/)
{
  map<expr_id_t, pair<expr_ref, expr_ref>> src_submap = src_sprel.sprel_map_get_submap(/*src_locid2expr_map*/);
  map<expr_id_t, pair<expr_ref, expr_ref>> dst_submap = dst_sprel.sprel_map_get_submap(/*dst_locid2expr_map*/);
  m_submap = src_submap;
  for (auto d : dst_submap) {
    m_submap.insert(d);
  }
}

/*expr_ref
sprel_map_pair_t::sprel_map_pair_get_expr(context *ctx) const
{
  expr_vector exprs;
  for (auto s : m_submap) {
    exprs.push_back(ctx->mk_eq(s.second.first, s.second.second));
  }
  if (exprs.size() == 0) {
    return expr_true(ctx);
  } else if (exprs.size() == 1) {
    return exprs.at(0);
  } else {
    return ctx->expr_do_simplify(ctx->mk_app(expr::OP_AND, exprs));
  }
}*/

string
sprel_map_pair_t::to_string_for_eq() const
{
  stringstream ss;
  for (auto s : m_submap) {
    ss << "=loc_expr\n";
    context *ctx = s.second.first->get_context();
    ss << ctx->expr_to_string_table(s.second.first, true) << "\n";
    ss << "=sprel_expr\n";
    ss << ctx->expr_to_string_table(s.second.second, true) << "\n";
  }
  return ss.str();
}

bool
sprel_map_pair_t::sprel_map_pair_implies(sprel_map_pair_t const &other) const
{
  for (auto s : other.m_submap) {
    if (!m_submap.count(s.first)) {
      return false;
    }
    if (s.second.second != m_submap.at(s.first).second) {
      return false;
    }
  }
  return true;
}

map<expr_id_t, pair<expr_ref, expr_ref>> const &
sprel_map_pair_t::sprel_map_pair_get_submap() const
{
  /*map<expr_id_t, pair<expr_ref, expr_ref>> ret;
  for (auto s : m_submap) {
    ret.insert(s);
  }
  return ret;*/
  return m_submap;
}

list<predicate_ref>
sprel_map_pair_t::get_predicates_for_non_var_locs(context *ctx) const
{
  list<predicate_ref> ret;
  for (auto s : m_submap) {
    if (!s.second.first->is_var()) {
      predicate_ref p = predicate::mk_predicate_ref(precond_t(), s.second.first, s.second.second, "sprel_assume");
      ret.push_back(p);
    }
  }
  return ret;
}

}
