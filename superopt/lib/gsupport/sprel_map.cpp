#include "gsupport/sprel_map.h"
//#include "graph/tfg.h"
#include "expr/solver_interface.h"
#include "support/debug.h"

namespace eqspace {

void
sprel_map_t::set_sprel_mappings(map<int, sprel_status_t> const &loc_ids)
{
  m_map = loc_ids;
  this->sprel_map_update_submap();
}

string
sprel_map_t::to_string(map<graph_loc_id_t, graph_cp_location> const &locs, eqspace::consts_struct_t const &cs, string prefix) const
{
  stringstream ss;
  for (map<graph_loc_id_t, sprel_status_t>::const_iterator it = m_map.begin(); it != m_map.end(); it++) {
    graph_cp_location loc;
    //loc = tfg->get_loc(pc, it->first);
    loc = locs.at(it->first);
    sprel_status_t const &sprels = it->second;
    if (sprels.get_type() != sprel_status_t::SPREL_STATUS_BOTTOM) {
      ss << prefix << " " << loc.to_string() << " : " << sprels.to_string() << endl;
    }
  }
  return ss.str();
}

/*sprel_status_t
sprel_status_t::meet(sprel_status_t const &other) const
{
  sprel_status_t ret;
  if (this->m_type == sprel_status_t::SPREL_STATUS_TOP) {
    ret = other;
    return ret;
  }
  if (other.m_type == sprel_status_t::SPREL_STATUS_TOP) {
    ret = *this;
    return ret;
  }
  if (   this->m_type == sprel_status_t::SPREL_STATUS_BOTTOM
      || other.m_type == sprel_status_t::SPREL_STATUS_BOTTOM) {
    ret.m_type = sprel_status_t::SPREL_STATUS_BOTTOM;
    return ret;
  }
  ASSERT(this->m_type == sprel_status_t::SPREL_STATUS_CONSTANT);
  ASSERT(other.m_type == sprel_status_t::SPREL_STATUS_CONSTANT);
  query_comment qc(query_comment::sprel_status_meet, "");

  //stable union, i.e., the order of elements in the union should be the same as the order of elements earlier. the caller ensures that the earlier status is in "other", so this is the right way of performing the union (stable union)
  vector<expr_ref> sprel_vals = other.m_sprel_vals;
  expr_vector_union(sprel_vals, this->m_sprel_vals, qc, false);

  if (sprel_vals.size() <= MAX_SPREL_VALUES) {
    ret.m_type = sprel_status_t::SPREL_STATUS_CONSTANT;
    ret.m_sprel_vals = sprel_vals;
    return ret;
  } else {
    ret.m_type = sprel_status_t::SPREL_STATUS_BOTTOM;
    return ret;
  }
}*/

string
sprel_status_t::to_string() const
{
  if (m_type == sprel_status_t::SPREL_STATUS_TOP) {
    return "sprel_status_top";
  } else if (m_type == sprel_status_t::SPREL_STATUS_BOTTOM) {
    return "sprel_status_bottom";
  } else if (m_type == sprel_status_t::SPREL_STATUS_CONSTANT) {
    string ret = "sprel_status_constant(";
    ret += expr_string(m_sprel_val);
    /*for (auto e : m_sprel_vals) {
      ret += expr_string(e) + ",";
    }*/
    ret += ")";
    return ret;
    //+ expr_string(m_sprel_val) + ")";
  } else NOT_REACHED();
}

/*bool
sprel_map_t::is_linearly_related_to_input_sp(int loc_id, expr_ref &out) const
{
  sprel_status_t const &status = m_map.at(loc_id);
  CPP_DBG_EXEC(SUBSTITUTE_USING_SPRELS, cout << "status for loc_id " << loc_id << ": " << status.to_string() << endl);
  if (status.get_type() != sprel_status_t::SPREL_STATUS_CONSTANT) {
    return false;
  }
  out = status.get_sprel_value();
  return true;
}*/

/*sprel_status_t
sprel_map_t::get_status(int loc_id) const
{
  if (m_map.count(loc_id) == 0) {
    cout << __func__ << " " << __LINE__ << ": loc_id = " << loc_id << endl;
  }
  return m_map.at(loc_id);
}

void
sprel_map_t::set_status(int loc_id, sprel_status_t status)
{
  m_map[loc_id] = status;
}*/

/*class expr_unique_sp_relation_holds_visitor : public expr_visitor
{
public:
  expr_unique_sp_relation_holds_visitor(
      expr_ref e, sprel_map_t const &pred_sprel_map,
      eq::consts_struct_t const &cs)
  : m_expr(e), m_pred_sprel_map(pred_sprel_map), m_cs(cs)
  {
    m_holds.clear();
    m_visit_each_expr_only_once = true;
    visit_recursive(e);
  }
  void visit(expr_ref e);
  sprel_status_t get_status()
  {
    ASSERT(m_holds.count(m_expr->get_id()) > 0);
    return m_holds[m_expr->get_id()];
  }

private:
  expr_ref m_expr;
  sprel_map_t const &m_pred_sprel_map;
  eq::consts_struct_t const &m_cs;
  map<expr_id_t, sprel_status_t> m_holds;
};

void
expr_unique_sp_relation_holds_visitor::visit(expr_ref e)
{
  expr::operation_kind op;
  long e_loc_id;

  stopwatch_run(&expr_unique_sp_relation_holds_visit_timer);
  if (   m_holds.count(e->get_id()) > 0
      && m_holds[e->get_id()].get_type() == sprel_status_t::SPREL_STATUS_BOTTOM) {
    stopwatch_stop(&expr_unique_sp_relation_holds_visit_timer);
    return;
  }

  if (e->is_const()) {
    sprel_status_t new_status;
    new_status.set_type(sprel_status_t::SPREL_STATUS_CONSTANT);
    new_status.add_sprel_value(e);
    m_holds[e->get_id()] = new_status;
    stopwatch_stop(&expr_unique_sp_relation_holds_visit_timer);
    return;
  } else if (e->is_var()) {
    if (   !e->is_array_sort()
        && expr_is_consts_struct_constant(e, m_cs) != -1) {
      sprel_status_t new_status;
      new_status.set_type(sprel_status_t::SPREL_STATUS_CONSTANT);
      new_status.add_sprel_value(e);
      m_holds[e->get_id()] = new_status;
      stopwatch_stop(&expr_unique_sp_relation_holds_visit_timer);
      return;
    } else if ((e_loc_id = expr_var_is_pred_dependency(e)) != -1) {
      CPP_DBG_EXEC2(SP_RELATIONS, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << ", e_loc_id = " << e_loc_id << endl);
      //ASSERT(m_pred_sprel_map.count(e_loc_id) > 0);
      //ASSERT(lr_status_meet(m_holds[e->get_id()], m_pred_locs.at(e_loc_id)) == m_pred_locs.at(e_loc_id));
      //m_holds[e->get_id()] = m_pred_sprel_map.at(e_loc_id);
      CPP_DBG_EXEC2(SP_RELATIONS, cout << __func__ << " " << __LINE__ << ": m_pred_sprel_map.get_status(e_loc_id) = " << m_pred_sprel_map.get_status(e_loc_id).to_string() << endl);
      m_holds[e->get_id()] = m_pred_sprel_map.get_status(e_loc_id);
      if (   m_holds.at(e->get_id()).get_type() == sprel_status_t::SPREL_STATUS_CONSTANT
          && !m_holds.at(e->get_id()).constant_is_unique()) {
        m_holds[e->get_id()].set_type(sprel_status_t::SPREL_STATUS_BOTTOM);
      }
      stopwatch_stop(&expr_unique_sp_relation_holds_visit_timer);
      return;
    } else  {
      sprel_status_t new_status;
      new_status.set_type(sprel_status_t::SPREL_STATUS_BOTTOM);
      m_holds[e->get_id()] = new_status;
      stopwatch_stop(&expr_unique_sp_relation_holds_visit_timer);
      return;
    }
    NOT_REACHED();
  }

  ASSERT(e->get_args().size() > 0);
  op = e->get_operation_kind();
  expr_vector const_args;
  context *ctx = e->get_context();
  for (unsigned i = 0; i < e->get_args().size(); i++) {
    int arg_id = e->get_args()[i]->get_id();
    ASSERT(m_holds.count(arg_id) > 0);
    if (m_holds[arg_id].get_type() == sprel_status_t::SPREL_STATUS_TOP) {
      m_holds[e->get_id()] = m_holds[arg_id];
      stopwatch_stop(&expr_unique_sp_relation_holds_visit_timer);
      return;
    } else if (m_holds[arg_id].get_type() == sprel_status_t::SPREL_STATUS_BOTTOM) {
      m_holds[e->get_id()] = m_holds[arg_id];
      stopwatch_stop(&expr_unique_sp_relation_holds_visit_timer);
      return;
    }
    ASSERT(m_holds[arg_id].get_type() == sprel_status_t::SPREL_STATUS_CONSTANT);
    ASSERT(m_holds[arg_id].constant_is_unique());
    const_args.push_back(m_holds[arg_id].get_unique_sprel_value());
  }
  sprel_status_t new_status;
  new_status.set_type(sprel_status_t::SPREL_STATUS_CONSTANT);
  new_status.add_sprel_value(ctx->mk_app(op, const_args));
  m_holds[e->get_id()] = new_status;
  stopwatch_stop(&expr_unique_sp_relation_holds_visit_timer);
}

sprel_status_t
expr_sp_relation_holds(expr_ref e, sprel_map_t const &pred_sprel_map, consts_struct_t const &cs)
{
  set<graph_loc_id_t> found_pred_dependencies;
  sprel_status_t ret;
  context *ctx = e->get_context();

  //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
  if (expr_contains_only_pred_dependencies_or_constants(e, cs, found_pred_dependencies)) {
    //cout << __func__ << " " << __LINE__ << ": contains only pred dependencies or constants, found_pred_dependencies.size() = " << found_pred_dependencies.size() << endl;
    if (found_pred_dependencies.size() == 0) {
      //ret.set_type(sprel_status_t::SPREL_STATUS_CONSTANT);
      //ret.add_sprel_value(e);
      ret = sprel_status_t::constant(e);
      return ret;
    } else if (found_pred_dependencies.size() == 1) {
      graph_loc_id_t pred_loc_id = *(found_pred_dependencies.begin());
      sprel_status_t pred = pred_sprel_map.get_mapping(pred_loc_id);
      if (pred.get_type() == sprel_status_t::SPREL_STATUS_TOP) {
        ret.set_type(sprel_status_t::SPREL_STATUS_TOP);
        return ret;
      } else if (pred.get_type() == sprel_status_t::SPREL_STATUS_CONSTANT) {
        ret.set_type(sprel_status_t::SPREL_STATUS_CONSTANT);
        vector<expr_ref> const &cvals = pred.get_multiple_sprel_values();
        ASSERT(cvals.size() <= MAX_SPREL_VALUES);
        for (auto cval : cvals) {
          map<expr_id_t, pair<expr_ref, expr_ref>> submap;
          string pred_id = pred_identifier(pred_loc_id);
          expr_ref pred_var = ctx->mk_var(pred_id, cval->get_sort());
          submap[pred_var->get_id()] = make_pair(pred_var, cval);
          expr_ref sub_e = expr_substitute(e, submap);
          ret.add_sprel_value(sub_e);
        }
        return ret;
      } //else fallback to bottom
    } //else fallback to bottom
  }
  ret.set_type(sprel_status_t::SPREL_STATUS_BOTTOM);
  return ret;
}*/

map<expr_id_t, pair<expr_ref, expr_ref>> const &
sprel_map_t::sprel_map_get_submap() const
{
  return m_submap;
}

void
sprel_map_t::sprel_map_update_submap()
{
  m_submap.clear();
  for (auto sp : m_map) {
    /*ASSERT(   sp.second.get_type() == sprel_status_t::SPREL_STATUS_CONSTANT
           || sp.second.get_type() == sprel_status_t::SPREL_STATUS_BOTTOM);*/
    if (sp.second.get_type() == sprel_status_t::SPREL_STATUS_CONSTANT) {
      if (!m_locid2expr_map.count(sp.first)) {
        cout << __func__ << " " << __LINE__ << ": could not find expr for loc_id " << sp.first << endl;
      }
      ASSERT(m_locid2expr_map.count(sp.first));
      expr_ref from = m_locid2expr_map.at(sp.first);
      expr_ref to = sp.second.get_constant_value();
      m_submap[from->get_id()] = make_pair(from, to);
    }
  }
}

/*
bool
sprel_map_t::status_contains_top() const
{
  for (auto s : m_map) {
    if (s.second.get_type() == sprel_status_t::SPREL_STATUS_TOP) {
      return true;
    }
  }
  return false;
}
*/

//unordered_set<predicate>
//sprel_map_t::sprel_map_get_assumes(/*map<graph_loc_id_t, expr_ref> const &locid2expr_map*/) const
//{
//  unordered_set<predicate> ret;
//  for (auto sp : m_map) {
//    /*ASSERT(   sp.second.get_type() == sprel_status_t::SPREL_STATUS_CONSTANT
//           || sp.second.get_type() == sprel_status_t::SPREL_STATUS_BOTTOM);*/
//    if (   sp.second.get_type() == sprel_status_t::SPREL_STATUS_CONSTANT/*
//        && !sp.second.constant_is_unique()*/) {
//      //vector<expr_ref> const &vals = sp.second.get_multiple_sprel_values();
//      expr_ref const &val = sp.second.get_constant_value();
//      expr_ref from = m_locid2expr_map.at(sp.first);
//      context *ctx = from->get_context();
//      //expr_ref e = expr_false(ctx);
//      stringstream ss;
//      ss << SPREL_ASSUME_COMMENT_PREFIX << "." << sp.first;
//      string comment = ss.str();
//      //for (auto v : vals) {
//        //e = expr_or(e, expr_eq(from, v));
//      //}
//      //e = expr_eq(from, val);
//      predicate pred(precond_t(), from, val, comment, predicate::assume);
//      ret.insert(pred);
//    }
//  }
//  return ret;
//}

bool
sprel_map_t::equals(sprel_map_t const &other) const
{
  bool ret = (m_map == other.m_map);
  /*cout << __func__ << " " << __LINE__ << ": returning " << ret << endl;
  if (!ret) {
    cout << __func__ << " " << __LINE__ << ": this =\n";
    for (map<graph_loc_id_t, sprel_status_t>::const_iterator it = m_map.begin(); it != m_map.end(); it++) {
      cout << "loc" << it->first << ": " << it->second.to_string() << endl;
    }
    cout << __func__ << " " << __LINE__ << ": other =\n";
    for (map<graph_loc_id_t, sprel_status_t>::const_iterator it = other.m_map.begin(); it != other.m_map.end(); it++) {
      cout << "loc" << it->first << ": " << it->second.to_string() << endl;
    }
  }*/
  return ret;
}

string
sprel_map_t::to_string() const
{
  stringstream ss;
  for (map<graph_loc_id_t, sprel_status_t>::const_iterator it = m_map.begin(); it != m_map.end(); it++) {
    ss << it->first << ": " << it->second.to_string() << endl;
  }
  return ss.str();
}

void
sprel_map_t::sprel_map_union(sprel_map_t const &other)
{
  for (map<graph_loc_id_t, sprel_status_t>::const_iterator it = other.m_map.begin();
       it != other.m_map.end();
       it++) {
    if (m_map.count(it->first) == 0) {
      m_map.insert(*it);
    }
  }
  this->sprel_map_update_submap();
}

bool
sprel_map_t::has_mapping_for_loc(graph_loc_id_t loc_id) const
{
  return    m_map.count(loc_id)
         && m_map.at(loc_id).get_type() == sprel_status_t::SPREL_STATUS_CONSTANT;
}

string
sprel_map_t::to_string_for_eq() const
{
  stringstream ss;
  for (auto s : m_map) {
    sprel_status_t const &st = s.second;
    ASSERT(st.get_type() != sprel_status_t::SPREL_STATUS_TOP);
    if (st.get_type() != sprel_status_t::SPREL_STATUS_CONSTANT) {
      continue;
    }
    ss << "=loc " << s.first << "\n";
    expr_ref const &e = s.second.get_constant_value();
    context *ctx = e->get_context();
    ss << ctx->expr_to_string_table(e, true) << "\n";
  }
  return ss.str();
}

}
