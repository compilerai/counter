#include "gsupport/state.h"
#include "expr/expr_utils.h"
//#include "graph/tfg.h"
#include "expr/consts_struct.h"
#include "support/c_utils.h"
#include "support/stacktrace.h"
#include "rewrite/transmap.h"
#include "valtag/regmap.h"
#include "gsupport/fsignature.h"

#define DEBUG_TYPE "state"

namespace eqspace {

state::state(string prefix, context *ctx)
{
  map<string_ref, expr_ref> value_expr_map;
  //set_num_exreg_groups(I386_NUM_EXREG_GROUPS);
  //for (int i = 0; i < I386_NUM_EXREG_GROUPS; i++) {
  //  set_num_exregs(i, I386_NUM_EXREGS(i));
  //  for (int j = 0; j < I386_NUM_EXREGS(i); j++) {
  //    string name = string(G_REGULAR_REG_NAME) + int_to_string(i) + "." + int_to_string(j);
  //    //set_expr(name, ctx->mk_var(prefix + "." + name, ctx->mk_bv_sort(I386_EXREG_LEN(i))));;
  //    value_expr_map.insert(make_pair(mk_string_ref(name), ctx->mk_var(prefix + "." + name, ctx->mk_bv_sort(I386_EXREG_LEN(i)))));
  //  }
  //}
  //set_expr(g_mem_symbol, ctx->mk_var(prefix + g_mem_symbol, ctx->mk_array_sort(ctx->mk_bv_sort(DWORD_LEN), ctx->mk_bv_sort(BYTE_LEN))));
  value_expr_map.insert(make_pair(mk_string_ref(g_mem_symbol), ctx->mk_var(prefix + g_mem_symbol, ctx->mk_array_sort(ctx->mk_bv_sort(DWORD_LEN), ctx->mk_bv_sort(BYTE_LEN)))));
  //set_expr(G_CONTAINS_FLOAT_OP_SYMBOL, ctx->mk_var(prefix + G_CONTAINS_FLOAT_OP_SYMBOL, ctx->mk_bool_sort()));
  value_expr_map.insert(make_pair(mk_string_ref(G_CONTAINS_FLOAT_OP_SYMBOL), ctx->mk_var(prefix + G_CONTAINS_FLOAT_OP_SYMBOL, ctx->mk_bool_sort())));
  //set_expr(G_CONTAINS_UNSUPPORTED_OP_SYMBOL, ctx->mk_var(prefix + G_CONTAINS_UNSUPPORTED_OP_SYMBOL, ctx->mk_bool_sort()));
  value_expr_map.insert(make_pair(mk_string_ref(G_CONTAINS_UNSUPPORTED_OP_SYMBOL), ctx->mk_var(prefix + G_CONTAINS_UNSUPPORTED_OP_SYMBOL, ctx->mk_bool_sort())));
  //set_num_invisible_regs(I386_NUM_INVISIBLE_REGS);
  for (int i = 0; i < I386_NUM_INVISIBLE_REGS; i++) {
    string name = string(G_INVISIBLE_REG_NAME) + int_to_string(i);
    //set_expr(name, ctx->mk_var(prefix + name, ctx->mk_bv_sort(I386_INVISIBLE_REGS_LEN(i))));
    value_expr_map.insert(make_pair(mk_string_ref(name), ctx->mk_var(prefix + name, ctx->mk_bv_sort(I386_INVISIBLE_REGS_LEN(i)))));
  }
  //set_num_hidden_regs(0);
  this->m_value_exprs.set_map(value_expr_map);
}

bool state::has_expr(const string& val_name) const
{
  if (m_value_exprs.get_map().count(mk_string_ref(val_name)) == 0) {
    return false;
  }
  return true;
}

string state::has_expr_substr(const string& val_name) const
{
  for (const auto &kv : m_value_exprs.get_map()) {
    if (kv.first->get_str().find(val_name) != string::npos) {
      return kv.first->get_str();
    }
  }
  return "";
}

expr_ref state::get_expr(const string& val_name, state start_state) const
{
  //if (m_value_exprs.get_map().count(mk_string_ref(val_name)) == 0 && start_state.m_value_exprs.get_map().count(mk_string_ref(val_name)) == 0) {
  //  cout << __func__ << " " << __LINE__ << ": val_name = " << val_name << endl;
  //}
  if (m_value_exprs.get_map().count(mk_string_ref(val_name))) {
    return m_value_exprs.get_map().at(mk_string_ref(val_name));
  } else {
    ASSERT(start_state.m_value_exprs.get_map().count(mk_string_ref(val_name)));
    return start_state.m_value_exprs.get_map().at(mk_string_ref(val_name));
  }
}

expr_ref
state::get_expr_for_input_expr(const string& val_name, expr_ref const& input_expr) const
{
  if (m_value_exprs.get_map().count(mk_string_ref(val_name))) {
    expr_ref ret = m_value_exprs.get_map().at(mk_string_ref(val_name));
    //if (input_expr->get_sort() != ret->get_sort()) {
    //  context* ctx = ret->get_context();
    //  cout << __func__ << " " << __LINE__ << ":\n";
    //  cout << "input_expr:\n" << ctx->expr_to_string_table(input_expr) << endl;
    //  cout << "ret:\n" << ctx->expr_to_string_table(ret) << endl;
    //}
    //ASSERT(input_expr->get_sort() == ret->get_sort()); //this can happen for hidden regs
    return ret;
  } else {
    return input_expr;
  }

}

void state::set_expr(const string& val_name, expr_ref val)
{
  //m_value_expr_map[mk_string_ref(val_name)] = val;
  m_value_exprs.insert(make_pair(mk_string_ref(val_name), val));
}

bool state::key_exists(const string& key_name/*, state const &start_state*/) const
{
  if (m_value_exprs.get_map().count(mk_string_ref(key_name)) > 0) {
    return true;
  }
  //if (start_state.m_value_exprs.get_map().count(mk_string_ref(key_name)) > 0) {
  //  return true;
  //}
  return false;
}

//string state::to_string() const
//{
//  stringstream ss;
//  for(const std::pair<string_ref, expr_ref>& val : m_value_exprs.get_map())
//  {
//    context *ctx = val.second->get_context();
//    ss << val.first->get_str() << ":\n" << ctx->expr_to_string_table(val.second) << endl;
//  }
//  return ss.str();
//}

string state::to_string(/*const state& relative, */const string& sep) const
{
  stringstream ss;
  for (const std::pair<string_ref, expr_ref>& val : m_value_exprs.get_map()) {
    if (val.first->get_str().find(g_hidden_reg_name) == string::npos) {
      ss << val.first->get_str() << ": " << expr_string(val.second) << sep;
    }
  }
  return ss.str();
}

string state::to_string_for_eq() const
{
  stringstream ss;
  for(const std::pair<string_ref, expr_ref>& val : m_value_exprs.get_map())
  {
    context *ctx = val.second->get_context();
    ss << "=" << val.first->get_str() << "\n" << ctx->expr_to_string_table(val.second) << "\n";
  }
  return ss.str();
}

state_submap_t
state::create_submap_from_value_expr_map(/*map<string_ref, expr_ref> const &from_value_expr_map, */map<string_ref, expr_ref> const &to_value_expr_map)
{
  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  //map<string_ref, expr_ref> name_submap;
  for (auto e : to_value_expr_map) {
    //if (from_value_expr_map.count(e.first) == 0) {
    //  continue;
    //}
    //expr_ref from_expr = from_value_expr_map.at(e.first);
    context* ctx = e.second->get_context();
    expr_ref from_expr = ctx->get_input_expr_for_key(e.first, e.second->get_sort()); //from_value_expr_map.at(e.first);
    submap[from_expr->get_id()] = make_pair(from_expr, e.second);
    //name_submap[e.first] = e.second;
  }
  return state_submap_t(/*name_submap, */submap);
}

state_submap_t
state::create_submap_from_state(/*state const& from, */state const& to)
{
  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
  //map<string_ref, expr_ref> const &from_value_expr_map = from.get_value_expr_map_ref();
  map<string_ref, expr_ref> const &to_value_expr_map = to.get_value_expr_map_ref();
  return state::create_submap_from_value_expr_map(/*from_value_expr_map, */to_value_expr_map);
}

void
state::state_substitute_variables(/*const state& from, */const state& to)
{
  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
  state_submap_t state_submap = create_submap_from_state(/*from, */to);
  //cout << __func__ << " " << __LINE__ << ": before: state =\n" << this->to_string() << endl;
  substitute_variables_using_state_submap(/*from, */state_submap);
  //cout << __func__ << " " << __LINE__ << ": after: state =\n" << this->to_string() << endl;
}

/*void state::substitute_variables(map<unsigned, pair<expr_ref, expr_ref>> const &submap)
{
  for (const pair<string, expr_ref>& key_expr : m_value_expr_map) {
    string key = key_expr.first;
    expr_ref expr = key_expr.second;
    m_value_expr_map[key] = expr_substitute(expr, submap);
  }
}*/

void
state::state_simplify()
{
  std::function<expr_ref (const string&, expr_ref)> f = [](const string& key, expr_ref e) -> expr_ref {
    //cout << __func__ << " " << __LINE__ << ": backtrace:\n" << backtrace(1) << endl;
    //cout << __func__ << " " << __LINE__ << ": calling simplify for " << key << endl << expr_string(e) << endl;
    context *ctx = e->get_context();
    expr_ref ret = ctx->expr_do_simplify(e);
    //cout << __func__ << " " << __LINE__ << ": done simplify for " << key << endl << expr_string(ret) << endl;
    return ret;
  };
  state_visit_exprs(f);
}

/*void state::specialize(expr_ref pred, const query_comment& qc)
{
  std::function<expr_ref (const string&, expr_ref)> f = [pred, &qc](const string& key, expr_ref e) -> expr_ref { return e->specialize(pred, qc); };
  visit_exprs(f);
}*/

//std::function<bool (pair<string_ref, expr_ref> const&, pair<string_ref, expr_ref> const&)>
//state::value_expr_map_deterministic_sort_fn()
//{
//  return [](pair<string_ref, expr_ref> const&a, pair<string_ref, expr_ref> const&b)
//         { return a.first->get_str() < b.first->get_str(); };
//}


bool
state::state_visit_exprs_for_key(std::function<expr_ref (const string&, expr_ref)> f/*, state const &start_state*/, string const &key)
{
  bool changed = false;
  string_ref key_ref = mk_string_ref(key);
  if (m_value_exprs.get_map().count(key_ref)) {
    expr_ref new_val = f(key, m_value_exprs.get_map().at(key_ref));
    context* ctx = new_val->get_context();
    if (new_val != m_value_exprs.get_map().at(key_ref)) {
      changed = true;
      /*if (   //start_state.m_value_exprs.get_map().count(key_ref)
             !ctx->key_represents_hidden_var(key_ref)
          //&& (new_val == start_state.get_expr(key, start_state))
          && (new_val == ctx->get_input_expr_for_key(key_ref, new_val->get_sort()))
         ) {
        m_value_exprs.erase(key_ref);
        //auto iter = m_determinized_value_expr_map.begin();
        //while (iter != m_determinized_value_expr_map.end()) {
        //  if (iter->first == key_ref) {
        //    iter = m_determinized_value_expr_map.erase(iter);
        //  } else {
        //    iter++;
        //  }
        //}
      } else */{
        //m_value_expr_map[key_ref] = new_val;
        //m_determinized_value_expr_map[key] = new_val;
        m_value_exprs.insert(make_pair(key_ref, new_val));
        //for (auto & pr : m_determinized_value_expr_map) {
        //  if (pr.first == key_ref) {
        //    pr.second = new_val;
        //  }
        //}
      }
    }
  } else {
    NOT_REACHED();
    //ASSERT(start_state.m_value_exprs.get_map().count(key_ref));
    //expr_ref new_val = f(key, start_state.m_value_exprs.get_map().at(key_ref));
    //if (new_val != start_state.m_value_exprs.get_map().at(key_ref)) {
    //  changed = true;
    //  m_value_exprs.insert(make_pair(key_ref, new_val));
    //  //m_value_expr_map[key_ref] = new_val;
    //  //m_determinized_value_expr_map[key] = new_val;
    //  //m_determinized_value_expr_map.push_back(make_pair(key_ref, new_val));
    //  //m_determinized_value_expr_map.sort(value_expr_map_deterministic_sort_fn());
    //}
  }
  return changed;
}

map<string, expr_ref>
state_value_expr_map_t::determinize_value_expr_map(map<string_ref, expr_ref> const& value_expr_map)
{
  autostop_timer func_timer(string(__func__));
  map<string, expr_ref> ret;
  for (auto const& m : value_expr_map) {
    //ret.push_back(m);
    ret.insert(make_pair(m.first->get_str(), m.second));
  }
  //ret.sort(value_expr_map_deterministic_sort_fn());
  return ret;
}

//bool
//state::state_visit_exprs_with_start_state_helper(std::function<expr_ref (const string&, expr_ref)> f, state const &start_state)
//{
//  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
//  bool changed = false;
//  map<string, expr_ref> start_str_expr_map;
//  for (const pair<string, expr_ref>& key_expr : start_state.m_value_exprs.get_det_map()) {
//    changed = state_visit_exprs_for_key(f, start_state, key_expr.first) || changed;
//  }
//  map<string, expr_ref> str_expr_map;
//  for (const pair<string, expr_ref>& key_expr : m_value_exprs.get_det_map()) {
//    if (start_state.m_value_exprs.get_det_map().count(key_expr.first) == 0) {
//      changed = state_visit_exprs_for_key(f, start_state, key_expr.first) || changed;
//    }
//  }
//  return changed;
//}

bool
state::state_visit_exprs_with_start_state(std::function<expr_ref (const string&, expr_ref)> f/*, state const &start_state*/)
{
  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
  bool changed = false;
  //map<string, expr_ref> start_str_expr_map;
  //for (const pair<string, expr_ref>& key_expr : start_state.m_value_exprs.get_det_map()) {
  //  changed = state_visit_exprs_for_key(f, start_state, key_expr.first) || changed;
  //}
  map<string, expr_ref> str_expr_map;
  for (const pair<string, expr_ref>& key_expr : m_value_exprs.get_det_map()) {
    //if (start_state.m_value_exprs.get_det_map().count(key_expr.first) == 0) {
      changed = state_visit_exprs_for_key(f/*, start_state*/, key_expr.first) || changed;
    //}
  }
  return changed;
}

void
state::state_visit_exprs_with_start_state(std::function<void (const string&, expr_ref)> f/*, state const &start_state*/) const
{
  //autostop_timer func_timer(string(__func__) + ".const"); // function called too frequently, use low overhead timers
  //for (const pair<string, expr_ref>& key_expr : start_state.m_value_exprs.get_det_map()) {
  //  if (m_value_exprs.get_det_map().count(key_expr.first)) {
  //    f(key_expr.first, m_value_exprs.get_det_map().at(key_expr.first));
  //  } else {
  //    f(key_expr.first, key_expr.second);
  //  }
  //}
  for (const pair<string, expr_ref>& key_expr : m_value_exprs.get_det_map()) {
    /*if (start_state.m_value_exprs.get_det_map().count(key_expr.first) == 0) */{
      f(key_expr.first, key_expr.second);
    }
  }
/*
  for (const pair<string, expr_ref>& key_expr : m_value_expr_map) {
    f(key_expr.first, key_expr.second);
  }
*/
}

bool
state::state_visit_exprs(std::function<expr_ref (const string&, expr_ref)> f)
{
  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
  return state_visit_exprs_with_start_state(f/*, *this*/);
}

void
state::state_visit_exprs(std::function<void (const string&, expr_ref)> f) const
{
  //autostop_timer func_timer(string(__func__) + ".const"); // function called too frequently, use low overhead timers!
  state_visit_exprs_with_start_state(f/*, *this*/);
}

void state::merge_state(expr_ref pred, const state& st_in/*, state const &start_state*/)
{
  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
  std::function<expr_ref (const string&, expr_ref)> f =
      [pred, &st_in/*, &start_state*/](const string& key, expr_ref e) -> expr_ref {
    context* ctx = e->get_context();
    if (   !st_in.key_exists(key)
        /*&& start_state.m_value_exprs.get_map().count(mk_string_ref(key)) == 0*/
        && ctx->key_represents_hidden_var(mk_string_ref(key))) {
      return e;
    }
    expr_ref key_expr = ctx->get_input_expr_for_key(mk_string_ref(key), e->get_sort());
    expr_ref in_e = st_in.get_expr_for_input_expr(key, key_expr/*start_state*/);
    if (in_e != e) {
      if (in_e->get_sort() != e->get_sort()) { //this can happen  for hidden exprs
        return e; //ignore one of them in this case as it should not matter
      }
      return expr_ite(pred, in_e, e);
    } else {
      return e;
    }
  };

  state_visit_exprs_with_start_state(f/*, start_state*/);

  // add state elements only present in st_in
  std::function<void (const string&, expr_ref)> f_st =
  [this,pred](const string& key, expr_ref e) -> void {
    if (this->key_exists(key)) {
      return;
    }
    context* ctx = e->get_context();
    expr_ref key_expr = ctx->get_input_expr_for_key(mk_string_ref(key), e->get_sort());
    expr_ref out_e = expr_ite(pred, e, key_expr);
    this->set_expr(key,out_e);
  };
  st_in.state_visit_exprs(f_st);
}

/*
void state::get_substitution_vars(vector<expr_ref>& vars) const
{
  std::function<void (const string&, expr_ref)> f =
      [&vars](const string& key, expr_ref e) { vars.push_back(e); };

  visit_exprs(f);
}
*/


void state::get_regs(list<expr_ref>& regs) const
{
  for (const auto& n_r : m_value_exprs.get_map()) {
    regs.push_back(n_r.second);
  }
}

void state::get_regs(vector<expr_ref>& regs) const
{
  for (const auto& n_r : m_value_exprs.get_map()) {
    regs.push_back(n_r.second);
  }
}

void state::get_names(list<string>& names) const
{
  for(const auto& n_r : m_value_exprs.get_map()) {
    names.push_back(n_r.first->get_str());
  }
}

/*
void state::get_mem_slots(list<tuple<expr_ref, memlabel_t, expr_ref, unsigned, bool>>& addrs) const
{
  std::function<void (const string&, expr_ref)> f =
      [&addrs](const string&, expr_ref e)
  {
    e->get_mem_slots(addrs);
  };

  visit_exprs(f);
}
*/

void state::get_expr_with_op_kind(expr::operation_kind k, list<expr_ref>& es) const
{
  std::function<void (const string&, expr_ref)> f =
      [&es, k](const string&, expr_ref e)
  {
    expr_find_op find(e, k);
    expr_vector es_matched = find.get_matched_expr();
    for(expr_ref e : es_matched)
      es.push_back(e);
  };

  state_visit_exprs(f);
}

void state::get_fun_calls(list<expr_ref>& calls) const
{
  get_expr_with_op_kind(expr::OP_FUNCTION_CALL, calls);
}

void state::get_mem_selects_and_stores(list<expr_ref>& es) const
{
  expr_list es_select;
  expr_list es_store;
  get_expr_with_op_kind(expr::OP_SELECT, es_select);
  get_expr_with_op_kind(expr::OP_STORE, es_store);
  es.splice(es.end(), es_select);
  es.splice(es.end(), es_store);
}

/*
void state::get_reg_slots(set<string>& names) const
{
  std::function<void (const string&, expr_ref)> f = [&names](const string& key, expr_ref)
  {
    if (key.substr(0, g_hidden_reg_name.size()) == g_hidden_reg_name) {
      return;
    }
    if (key == g_mem_symbol) {
      return;
    }
    names.insert(key);
  };

  visit_exprs(f);
}

void
state::get_fcall_retregs(set<string>& names) const
{
  std::function<void (const string&, expr_ref)> f = [&names](const string& key, expr_ref e)
  {
    if (   e->contains_function_call()
        && key != g_mem_symbol) {
      names.insert(key);
    }
  };

  visit_exprs(f);
}

string state::get_name_from_expr(expr_ref e_arg) const
{
  string name = "";
  std::function<void (const string&, expr_ref)> f = [&name,e_arg](const string& key, expr_ref e)
  {
    if(e_arg == e)
      name = key;
  };

  visit_exprs(f);

  assert(name.size());
  return name;
}
*/

expr_ref
expr_substitute_using_state(expr_ref e/*, const state& from*/, const state& to)
{
  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
  //if (&from == &to) {
  //  return e;
  //}
  context *ctx = e->get_context();
  map<expr_id_t, pair<expr_ref, expr_ref>> submap = state::create_submap_from_state(/*from, */to).get_expr_submap();
  return ctx->expr_substitute(e, submap);
}

class expr_substitute_and_get_pruned_submap_visitor : public expr_visitor {
public:
  expr_substitute_and_get_pruned_submap_visitor(expr_ref const &e, map<expr_id_t, pair<expr_ref, expr_ref> > const &submap)
    : m_expr(e), m_map_from_to(submap)
  {
    if (submap.size()) {
      m_ctx = e->get_context();
      visit_recursive(m_expr);
    } else {
      m_map_substituted[e->get_id()] = e;
    }
  }
  expr_ref get_substituted() const { return m_map_substituted.at(m_expr->get_id()); }
  map<expr_id_t,pair<expr_ref,expr_ref>> get_pruned_submap() const { return m_map_pruned; }

private:
  virtual void visit(expr_ref const &e);
  expr_ref const &m_expr;
  map<expr_id_t, expr_ref> m_map_substituted;
  map<expr_id_t, pair<expr_ref, expr_ref>> m_map_pruned;
  map<expr_id_t, pair<expr_ref, expr_ref>> const& m_map_from_to;
  context* m_ctx;
};

void expr_substitute_and_get_pruned_submap_visitor::visit(expr_ref const &e)
{
  expr_ref e_sub;
  ASSERT(m_map_substituted.count(e->get_id()) == 0);
  if (m_map_from_to.count(e->get_id())) {
    e_sub = m_map_from_to.at(e->get_id()).second;
    m_map_pruned.insert(make_pair(e->get_id(), make_pair(e, e_sub)));
    //CPP_DBG_EXEC3(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": e_sub->get_id() = " << e_sub->get_id() << endl);
  } else {
    if(e->is_var() || e->is_const()) {
      e_sub = e;
      //CPP_DBG_EXEC3(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": e_sub->get_id() = " << e_sub->get_id() << endl);
    } else {
      expr_vector args_subs;
      for(unsigned i = 0; i < e->get_args().size(); ++i) {
        expr_ref const& arg = e->get_args()[i];
        args_subs.push_back(m_map_substituted.at(arg->get_id()));
      }
      e_sub = m_ctx->mk_app(e->get_operation_kind(), args_subs);
      //CPP_DBG_EXEC3(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": e_sub->get_id() = " << e_sub->get_id() << endl);
    }
  }
  m_map_substituted[e->get_id()] = e_sub;
}

pair<expr_ref, map<expr_id_t,pair<expr_ref,expr_ref>>>
expr_substitute_and_get_pruned_submap_using_states(expr_ref const& e/*, state const& from*/, state const& to)
{
  //if (&from == &to) {
  //  map<expr_id_t,pair<expr_ref,expr_ref>> ret;
  //  return make_pair(e, ret);
  //}

  map<expr_id_t, pair<expr_ref, expr_ref>> submap = state::create_submap_from_state(/*from, */to).get_expr_submap();
  expr_substitute_and_get_pruned_submap_visitor vtor(e, submap);
  return make_pair(vtor.get_substituted(), vtor.get_pruned_submap());
}

/*void
state::state_transform_function_arguments_to_avoid_stack_pointers(consts_struct_t const &cs)
{
  std::function<expr_ref (const string&, expr_ref)> f = [&cs](const string& key, expr_ref e) -> expr_ref {
    //cout << __func__ << " " << __LINE__ << ": transforming " << key << endl;
    return e->expr_transform_function_arguments_to_avoid_stack_pointers(cs);
  };
  visit_exprs(f);
}

void
state::state_rename_locals_to_stack_pointer(consts_struct_t const &cs)
{
  std::function<expr_ref (const string&, expr_ref)> f = [&cs](const string& key, expr_ref e) -> expr_ref { return e->expr_rename_locals_to_stack_pointer(cs); };
  visit_exprs(f);
}*/

/*void
state::state_rename_farg_ptr_memlabel_top_to_notstack(consts_struct_t const &cs)
{
  std::function<expr_ref (const string&, expr_ref)> f = [&cs](const string& key, expr_ref e) -> expr_ref { return e->expr_rename_farg_ptr_memlabel_top_to_notstack(cs); };
  visit_exprs(f);
}

void
state::state_substitute_rodata_reads(tfg const &t, consts_struct_t const &cs)
{
  std::function<expr_ref (const string&, expr_ref)> f = [&t, &cs](const string& key, expr_ref e) -> expr_ref { return e->expr_substitute_rodata_reads(t, cs); };
  visit_exprs(f);
}

void
state::state_set_memlabels_to_top(map<symbol_id_t, pair<string, size_t>> const &symbol_map, graph_locals_map_t const &locals_map, consts_struct_t const &cs)
{
  std::function<expr_ref (const string&, expr_ref)> f = [&symbol_map, &locals_map, &cs](const string& key, expr_ref e) -> expr_ref { return expr_set_memlabels_to_top(e, symbol_map, locals_map, cs); };
  visit_exprs(f, *this);
}*/

/*void
state::set_comments_to_zero()
{
  std::function<expr_ref (const string&, expr_ref)> f = [](const string& key, expr_ref e) -> expr_ref { expr_set_comments_to_zero(e); return e; };
  state_visit_exprs(f, *this);
}*/

/*
void
state::append_pc_string_to_state_elems(pc const &p, bool src)
{
  std::function<expr_ref (const string&, expr_ref)> f = [&src, &p](const string& key, expr_ref e) -> expr_ref { return expr_append_pc_string(e, src, p); };
  visit_exprs(f);
}
*/

/*void
state::append_insn_id_to_comments(int insn_id)
{
  std::function<expr_ref (const string&, expr_ref)> f = [&insn_id](const string& key, expr_ref e) -> expr_ref { expr_append_insn_id_to_comments(e, insn_id); return e; };
  state_visit_exprs(f, *this);
}*/



/*bool
state::get_memory_expr(state const &start_state, expr_ref &ret) const
{
  ASSERT(!ret);
  for (const auto &mv : m_value_expr_map) {
    if (mv.second->is_array_sort()) {
      ASSERT(!ret);
      ret = mv.second;
    }
  }
  if (ret) {
    return true;
  }

  for (const auto &mv : start_state.m_value_expr_map) {
    if (mv.second->is_array_sort()) {
      ASSERT(!ret);
      ret = mv.second;
    }
  }

  if (ret) {
    return true;
  } else {
    return false;
  }
}*/

vector<string>
state::get_memnames() const
{
  vector<string> ret;
  for (const auto &mv : m_value_exprs.get_map()) {
    if (mv.second->is_array_sort()) {
      //ASSERT(ret == "");
      ret.push_back(mv.first->get_str());
    }
  }
  //ASSERT(ret != "");
  return ret;
}

string
state::get_memname() const
{
  vector<string> ret = this->get_memnames();
  ASSERT(ret.size() == 1);
  return ret.at(0);
}

bool
state::get_memory_expr(state const &start_state, expr_ref &ret) const
{
  vector<expr_ref> memory_exprs = get_memory_exprs(start_state);
  ASSERT(memory_exprs.size() <= 1);
  if (memory_exprs.size() == 0) {
    return false;
  }
  ret = memory_exprs.at(0);
  return true;
}

vector<expr_ref>
state::get_memory_exprs(state const &start_state) const
{
  vector<expr_ref> ret;
  for (const auto &mv : start_state.m_value_exprs.get_map()) {
    if (mv.second->is_array_sort()) {
      if (this->m_value_exprs.get_map().count(mv.first)) {
        ret.push_back(this->m_value_exprs.get_map().at(mv.first));
      } else {
        ret.push_back(mv.second);
      }
    }
  }
  return ret;
}

/*struct expr_substitute_using_sprels_callback_struct_t {
  tfg const *tfgp;
  pc const &from_pc;
  sprel_map_t const &sprels;
  map<unsigned, expr_ref> &sprels_subcache;
  eq::consts_struct_t const &cs;
  map<pair<pc, expr_id_t>, graph_loc_id_t> &expr_represents_loc_in_state_cache;
};

static expr_ref
expr_substitute_using_sprels(expr_ref e, expr_substitute_using_sprels_callback_struct_t *cbstruct)
{
  //expr_substitute_using_sprels_callback_struct_t *cbstruct = (expr_substitute_using_sprels_callback_struct_t *)aux;
  expr_ref ret = cbstruct->tfgp->substitute_using_sprels(e, cbstruct->from_pc, cbstruct->tfgp->find_node(cbstruct->from_pc)->get_state(), cbstruct->sprels, cbstruct->sprels_subcache, cbstruct->cs, cbstruct->expr_represents_loc_in_state_cache);
  CPP_DBG_EXEC(SUBSTITUTE_USING_SPRELS, cout << __func__ << " " << __LINE__ << ": returning " << expr_string(ret) << " for " << expr_string(e) << endl);
  return ret;
}

void
state::state_substitute_using_sprels(tfg const *tfg, pc const &from_pc, sprel_map_t const &sprels, map<unsigned, expr_ref> &sprels_subcache, eq::consts_struct_t const &cs, map<pair<pc, expr_id_t>, graph_loc_id_t> &expr_represents_loc_in_state_cache)
{
  expr_substitute_using_sprels_callback_struct_t cbstruct = { .tfgp = tfg, .from_pc = from_pc, .sprels = sprels, .sprels_subcache = sprels_subcache, .cs = cs, .expr_represents_loc_in_state_cache = expr_represents_loc_in_state_cache };
  //transform_all_exprs(expr_substitute_using_sprels_callback, &cbstruct);

  std::function<expr_ref (const string&, expr_ref)> f = [&cbstruct](const string& key, expr_ref e) -> expr_ref { return expr_substitute_using_sprels(e, &cbstruct); };
  visit_exprs(f);
}*/

/*bool
state::substitute_variables_using_submap_slow(state const &start_state, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap)
{
  autostop_timer func_timer(__func__);
  std::function<expr_ref (const string&, expr_ref)> f = [&submap](const string& key, expr_ref e) -> expr_ref
  {
    context *ctx = e->get_context();
    return ctx->expr_substitute(e, submap);
  };
  return state_visit_exprs_with_start_state(f, start_state);
}*/

bool
state::substitute_variables_using_state_submap(/*state const &start_state, */state_submap_t const &state_submap)
{
  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers!
  map<expr_id_t, pair<expr_ref, expr_ref>> const& submap = state_submap.get_expr_submap();
  map<string_ref, expr_ref> const& name_submap = state_submap.get_name_submap();
  std::function<expr_ref (const string&, expr_ref)> f = [&submap](const string& key, expr_ref e) -> expr_ref
  {
    context *ctx = e->get_context();
    return ctx->expr_substitute(e, submap);
  };
  bool changed = false;
  changed = state_visit_exprs(f) || changed;
  for (auto const& sm : name_submap) {
    string_ref const& keyref = sm.first;
    {//assertcheck
      //ASSERT(sm.second.first->is_var());
      //string const& name = sm.second.first->get_name()->get_str();
      //ASSERT(string_has_prefix(name, G_INPUT_KEYWORD "."));
      //string key = name.substr(strlen(G_INPUT_KEYWORD "."));
      //ASSERT(keyref == mk_string_ref(key);
    }
    context* ctx = sm.second->get_context();
    if (   !this->get_value_expr_map_ref().count(keyref)
        && !ctx->key_represents_hidden_var(keyref)) {
      //expr_ref const& e = start_state.get_value_expr_map_ref().at(keyref);
      expr_ref new_e = sm.second;
      expr_ref e = ctx->get_input_expr_for_key(keyref, new_e->get_sort());
      /*if (e != new_e) */{
        this->set_expr(keyref->get_str(), new_e);
        changed = true;
      }
    }
  }
  return changed;
}

void
state::reorder_registers_using_regmap(state const &from_state, struct regmap_t const &regmap, state const &start_state)
{
  state new_state;
  for (auto const& ve: this->get_value_expr_map_ref()) {
    context* ctx = ve.second->get_context();
    exreg_group_id_t exreg_group;
    exreg_id_t exreg_num;
    if (ctx->key_represents_exreg(ve.first, exreg_group, exreg_num)) {
      reg_identifier_t new_reg_id = regmap_get_elem(&regmap, exreg_group, reg_identifier_t(exreg_num));
      if (   !new_reg_id.reg_identifier_denotes_constant()
          && !new_reg_id.reg_identifier_is_unassigned()) {
        int new_reg = new_reg_id.reg_identifier_get_id();
        ASSERT(new_reg >= 0 && new_reg < DST_NUM_EXREGS(exreg_group));
        new_state.set_expr_value(reg_type_exvreg, exreg_group, new_reg, ve.second, start_state);
      }
    } else {
      new_state.set_expr(ve.first->get_str(), ve.second);
    }
  }
  *this = new_state;
  //cout << __func__ << " " << __LINE__ << ": num_exreg groups = " << get_num_exreg_groups() << endl;
  //for (size_t group = 0; group < DST_NUM_EXREG_GROUPS/*(size_t)get_num_exreg_groups()*/; group++) {
  //  //cout << __func__ << " " << __LINE__ << ": num_exregs(" << group << ") = " << get_num_exregs(group) << endl;
  //  for (size_t regnum = 0; regnum < DST_NUM_EXREGS(group)/*(size_t)get_num_exregs(group)*/; regnum++) {
  //    /*if (!from_state.expr_value_exists(reg_type_exvreg, group, regnum)) {
  //      continue;
  //    }*/
  //    expr_ref from_val = from_state.get_expr_value(start_state, reg_type_exvreg, group, regnum);
  //    set_expr_value(reg_type_exvreg, group, regnum, from_val, start_state);
  //  }
  //}
  //orig_state = state::state_union(orig_state, from_state);
  //for (size_t group = 0; group < DST_NUM_EXREG_GROUPS/*(size_t)get_num_exreg_groups()*/; group++) {
  //  for (size_t regnum = 0; regnum < DST_NUM_EXREGS(group)/*(size_t)get_num_exregs(group)*/; regnum++) {
  //    /*if (!from_state.expr_value_exists(reg_type_exvreg, group, regnum)) {
  //      continue;
  //    }*/

  //    expr_ref e;
  //    //int new_reg;

  //    e = orig_state.get_expr_value(start_state, reg_type_exvreg, group, regnum);
  //    //new_reg = regmap.regmap_extable[group][regnum];
  //    reg_identifier_t new_reg_id = regmap_get_elem(&regmap, group, reg_identifier_t(regnum));
  //    //cout << __func__ << " " << __LINE__ << ": group " << group << ", regnum " << regnum << ": new_reg_id " << new_reg_id.reg_identifier_to_string() << endl;
  //    if (   !new_reg_id.reg_identifier_denotes_constant()
  //        && !new_reg_id.reg_identifier_is_unassigned()) {
  //      int new_reg = new_reg_id.reg_identifier_get_id();
  //      expr_ref from_e;
  //      ASSERT(new_reg >= 0 && new_reg < DST_NUM_EXREGS(group)/*get_num_exregs(group)*/);
  //      from_e = from_state.get_expr_value(start_state, reg_type_exvreg, group, new_reg); //This has to be new_reg and not regnum, because expressions have already been renamed (but not reordered) based on the regmap; so the regnum value needs to be compared with from state's new_reg value. Only if the value has changed, does it need to be reflected in the final state (reorder)
  //      if (from_e != e) { //if unchanged, ignore
  //        set_expr_value(reg_type_exvreg, group, new_reg, e, start_state);
  //      }
  //    }
  //  }
  //}
  //cout << __func__ << " " << __LINE__ << ": this = " << this->to_string(*this) << endl;
  //*this = this->state_minus(start_state);
  //cout << __func__ << " " << __LINE__ << ": this = " << this->to_string(*this) << endl;
}

void
state::reorder_state_elements_using_transmap(struct transmap_t const *tmap, state const &start_state)
{
  regmap_t regmap;
  regmap_init(&regmap);
  for (int i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    for (int j = 0; j < MAX_NUM_EXREGS(i); j++) {
      transmap_entry_t const *e;
      //e = &tmap->extable[i][j];
      e = transmap_get_elem(tmap, i, j);
      for (const auto &k : e->get_locs()) {
        if (k.get_loc() == TMAP_LOC_SYMBOL) {
          NOT_IMPLEMENTED();
        }
        ASSERT(k.get_loc() >= TMAP_LOC_EXREG(0) && k.get_loc() < TMAP_LOC_EXREG(MAX_NUM_EXREG_GROUPS));
        int group = k.get_loc() - TMAP_LOC_EXREG(0);
        ASSERT(i == group);
        ASSERT(k.get_regnum() >= 0 && k.get_regnum() < MAX_NUM_EXREGS(i));
        //regmap.regmap_extable[i][e->regnum[k]] = j;
        regmap_set_elem(&regmap, i, k.get_regnum(), j);
      }
    }
  }
  //reorder_registers_using_regmap(from_state, regmap);
  reorder_registers_using_regmap(*this, regmap, start_state);
}

bool state::mem_access::is_select() const
{
  //return m_is_read;
  return !is_farg() && m_data->get_operation_kind() == expr::OP_SELECT
         //|| m_data->get_operation_kind() == expr::OP_FUNCTION_ARGUMENT_PTR
         //|| m_data->get_operation_kind() == expr::OP_RETURN_VALUE_PTR
  ;
}

bool state::mem_access::is_store() const
{
  return !is_farg() && m_data->get_operation_kind() == expr::OP_STORE;
}

bool state::mem_access::is_farg() const
{
  return m_is_farg;
}

/*bool state::mem_access::is_write() const
{
  //return !m_is_read;
  return m_data->get_operation_kind() == expr::OP_STORE;
}*/

expr_ref const &state::mem_access::get_address() const
{
  if (this->is_farg()) {
    return m_data;
  }
  switch (m_data->get_operation_kind()) {
    case expr::OP_SELECT:
    case expr::OP_STORE:
      //ret = m_data->get_args()[2];
      return m_data->get_args().at(2);
      break;
    //case expr::OP_FUNCTION_ARGUMENT_PTR:
    //case expr::OP_RETURN_VALUE_PTR:
      //ret = m_data->get_args()[0];
      //break;
    default:
      NOT_REACHED();
  }
  /*if (!ret->is_bv_sort()) {
    context *ctx = m_data->get_context();
    cout << __func__ << " " << __LINE__ << ": m_data = " << endl << ctx->expr_to_string_table(m_data) << endl;
    cout << __func__ << " " << __LINE__ << ": ret = " << endl << ctx->expr_to_string_table(ret) << endl;
  }
  ASSERT(ret->is_bv_sort());
  return ret;*/
  NOT_REACHED();
}

expr_ref const &state::mem_access::get_store_data() const
{
  ASSERT(m_data->get_operation_kind() == expr::OP_STORE);
  return m_data->get_args().at(3);
}

/*expr_ref state::mem_access::get_mem() const
{
  return m_data->get_args()[0];
}*/

/*expr_ref state::mem_access::get_expr() const
{
  return m_data;
}*/

/*expr::operation_kind
state::mem_access::ma_get_operation_kind() const
{
  return m_data->get_operation_kind();
}*/

string
state::mem_access::get_memname() const
{
  ASSERT(is_select() || is_store());
  expr_ref ret;
  switch (m_data->get_operation_kind()) {
    case expr::OP_SELECT:
    case expr::OP_STORE:
      return get_base_array_name(m_data->get_args()[0]);
      break;
    //case expr::OP_FUNCTION_ARGUMENT_PTR:
    //case expr::OP_RETURN_VALUE_PTR:
      //return get_base_array_name(m_data->get_args()[2]);
      //break;
    default:
      NOT_REACHED();
  }
  NOT_REACHED();
}

memlabel_t
state::mem_access::get_memlabel() const
{
  ASSERT(is_select() || is_store());
  switch (m_data->get_operation_kind()) {
    case expr::OP_SELECT:
    case expr::OP_STORE:
      return m_data->get_args().at(1)->get_memlabel_value();
    //case expr::OP_FUNCTION_ARGUMENT_PTR:
    //case expr::OP_RETURN_VALUE_PTR:
      //return m_data->get_args().at(1)->get_memlabel_value();
    default:
      NOT_REACHED();
  }
  NOT_REACHED();
}

unsigned
state::mem_access::get_nbytes() const
{
  ASSERT(is_select() || is_store());
  switch (m_data->get_operation_kind()) {
    case expr::OP_SELECT:
      return m_data->get_args().at(3)->get_int_value();
    case expr::OP_STORE:
      return m_data->get_args().at(4)->get_int_value();
    //case expr::OP_FUNCTION_ARGUMENT_PTR:
      //return DEFAULT_FUNCTION_ARGUMENT_PTR_NBYTES;
    //case expr::OP_RETURN_VALUE_PTR:
      //return DEFAULT_RETURN_VALUE_PTR_NBYTES;
    default:
      NOT_REACHED();
  }
  NOT_REACHED();
}

bool
state::mem_access::get_bigendian() const
{
  ASSERT(is_select() || is_store());
  switch (m_data->get_operation_kind()) {
    case expr::OP_SELECT:
      return m_data->get_args().at(4)->get_bool_value();
    case expr::OP_STORE:
      return m_data->get_args().at(5)->get_bool_value();
    //case expr::OP_FUNCTION_ARGUMENT_PTR:
      //return DEFAULT_FUNCTION_ARGUMENT_PTR_BIGENDIAN;
    //case expr::OP_RETURN_VALUE_PTR:
      //return DEFAULT_RETURN_VALUE_PTR_BIGENDIAN;
    default:
      NOT_REACHED();
  }
  NOT_REACHED();
}

/*expr_ref state::mem_access::get_data() const
{
  return m_data->get_args()[2];
}*/

/*void state::mem_access::set_expr(expr_ref e)
{
  m_data = e;
}*/

string state::mem_access::to_string() const
{
  stringstream ss;
  ss << (is_farg() ? "farg" : "") << ": ";
  context *ctx = m_data->get_context();
  ss << ctx->expr_to_string(m_data);
  return ss.str();
}

void state::find_all_memory_exprs_for_all_ops(string prefix, expr_ref e, set<state::mem_access>& ret)
{
  state::find_all_memory_exprs(prefix + ".select", e, expr::OP_SELECT, ret);
  state::find_all_memory_exprs(prefix + ".store", e, expr::OP_STORE, ret);
  //state::find_all_memory_exprs(prefix + ".farg_ptr", e, expr::OP_FUNCTION_ARGUMENT_PTR, ret);
  //state::find_all_memory_exprs(prefix + ".retval_ptr", e, expr::OP_RETURN_VALUE_PTR, ret);

  expr_find_op find_es(e, expr::OP_FUNCTION_CALL);
  expr_vector es = find_es.get_matched_expr();
  for (auto ee : es) {
    ASSERT(ee->get_args().size() >= OP_FUNCTION_CALL_ARGNUM_FARG0);
    CPP_DBG_EXEC2(SPLIT_MEM, cout << __func__ << " " << __LINE__ << ": found fcall expression:" << expr_string(ee) << endl);
    for (size_t i = OP_FUNCTION_CALL_ARGNUM_FARG0; i < ee->get_args().size(); i++) {
      expr_ref arg = ee->get_args().at(i);
      CPP_DBG_EXEC2(SPLIT_MEM, cout << __func__ << " " << __LINE__ << ": adding fcall arg " << expr_string(arg) << endl);
      ret.insert(state::mem_access(prefix + ".farg", arg, true));
    }
  }
}

void state::find_all_memory_exprs(string comment, expr_ref e, expr::operation_kind k, set<state::mem_access>& mas)
{
  expr_find_op find_es(e, k);
  expr_vector es = find_es.get_matched_expr();
  for(unsigned i = 0; i < es.size(); ++i) {
    CPP_DBG_EXEC2(SPLIT_MEM, cout << __func__ << " " << __LINE__ << ": found memory access = " << expr_string(es[i]) << endl);
    mas.insert(state::mem_access(comment, es[i], false));
  }
}


/*void state::find_memory_exprs(expr_ref e, expr_ref esp, expr::operation_kind k, map<unsigned, expr_ref>& found_es)
{
  expr_find_op find_es(e, k);
  expr_vector es = find_es.get_matched_expr();

  for(unsigned i = 0; i < es.size(); ++i)
  {
    expr_is_stack_addr visitor(es[i]->get_args()[1], esp);
    //if(!visitor.is_stack_addr())
      found_es.insert(make_pair(es[i]->get_id(), es[i]));
  }
}*/

/*expr_ref
state::convert_function_argument_ptr_to_select_expr(expr_ref e) const
{
  if (e->get_operation_kind() != expr::OP_FUNCTION_ARGUMENT_PTR) {
    return e;
  }
  ASSERT(e->get_operation_kind() == expr::OP_FUNCTION_ARGUMENT_PTR);
  context *ctx = e->get_context();

  return ctx->mk_select(get_memory_expr(), e->get_args()[1]->get_memlabel_value(), e->get_args()[0], DWORD_LEN/BYTE_LEN, false, eq::comment_t());
}*/

/*void state::add_to_mem_reads_write(set<expr_ref, expr_deterministic_cmp_t> const &found_es, list<state::mem_access>& mas)
{
  for (auto e : found_es) {
    mas.push_back(mem_access(e));
  }
}*/

/*void state::populate_mem_reads_writes(expr_ref esp, expr_ref pred)
{
  m_mem_accesses.clear();
  list<expr_ref> regs;
  get_regs(regs);
  map<unsigned, expr_ref> selects;
  for (auto e : regs) {
    find_memory_exprs(e, esp, expr::OP_SELECT, selects);
  }
  find_memory_exprs(pred, esp, expr::OP_SELECT, selects);

  map<unsigned, expr_ref> stores;
  for (auto e : regs) {
    find_memory_exprs(e, esp, expr::OP_SELECT, selects);
  }
  find_memory_exprs(pred, esp, expr::OP_SELECT, selects);

  m_mem_accesses.clear();
  state::add_to_mem_reads_write(selects, m_mem_accesses);
  state::add_to_mem_reads_write(stores, m_mem_accesses);
}*/

/*
void
state::state_annotate_locals(state const &instate, vector<pair<unsigned long, pair<string, unsigned long> > > const &symbol_vars, struct dwarf_map_t const *dwarf_map, struct eq::consts_struct_t const *cs, eq::context *ctx, sym_exec const &se, tfg const *tfg, pc const &from_pc, cspace::locals_info_t *locals_info, map<pc, sprel_map_t, pc_comp> const &sprels, map<pc, map<unsigned, expr_ref>, pc_comp> &sprels_subcache, map<pc, lr_map_t, pc_comp> const &linear_relations, map<pc_exprid_pair, int, pc_exprid_pair_comp> &expr_represents_loc_in_state_cache)
{
  for(size_t i = 0; i < m_mexvregs.size(); ++i) {
    for(size_t j = 0; j < m_mexvregs[i].size(); ++j) {
      DBG_EXEC(ANNOTATE_LOCALS, cout << __func__ << " " << __LINE__ << ": annotating locals at edge due to edge's to-machine_state reg " << i << " " << j << endl);
      m_mexvregs[i][j] = expr_annotate_locals(instate, m_mexvregs[i][j], symbol_vars, dwarf_map, cs, ctx, se, tfg, from_pc, locals_info, sprels, sprels_subcache, linear_relations, expr_represents_loc_in_state_cache);
    }
  }

  for(map<string, expr_ref>::const_iterator iter = m_mem_pools.begin(); iter != m_mem_pools.end(); ++iter) {
    string pool = iter->first;
    DBG_EXEC(ANNOTATE_LOCALS, cout << __func__ << " " << __LINE__ << ": annotating locals at edge due to edge's to-machine_state mem-pool " << pool << endl);
    m_mem_pools[pool] = expr_annotate_locals(instate, m_mem_pools[pool], symbol_vars, dwarf_map, cs, ctx, se, tfg, from_pc, locals_info, sprels, sprels_subcache, linear_relations, expr_represents_loc_in_state_cache);
  }

  for (size_t i = 0; i < m_invisible_regs.size(); i++) {
    DBG_EXEC(ANNOTATE_LOCALS, cout << __func__ << " " << __LINE__ << ": annotating locals at edge due to edge's invisible_reg " << i << endl);
    m_invisible_regs[i] = expr_annotate_locals(instate, m_invisible_regs[i], symbol_vars, dwarf_map, cs, ctx, se, tfg, from_pc, locals_info, sprels, sprels_subcache, linear_relations, expr_represents_loc_in_state_cache);
  }


  for(list<::machine_state::mem_access>::iterator iter = m_mem_accesses.begin(); iter != m_mem_accesses.end(); ++iter) {
    DBG_EXEC(ANNOTATE_LOCALS, cout << __func__ << " " << __LINE__ << ": annotating locals at edge due to edge's mem_access " << iter->to_string() << endl);
    iter->set_expr(expr_annotate_locals(instate, iter->get_expr(), symbol_vars, dwarf_map, cs, ctx, se, tfg, from_pc, locals_info, sprels, sprels_subcache, linear_relations, expr_represents_loc_in_state_cache));
  }

//  for(list<mem_access>::iterator iter = m_mem_writes.begin(); iter != m_mem_writes.end(); ++iter)
//    iter->set_expr(iter->get_expr()->simplify());
}
*/

string
make_string_id_from_pc(bool src, ::pc const &p)
{
  string ret;
  if (p.is_start()) {
    return G_INPUT_KEYWORD;
  }
  if (src) {
    ret = "src";
  } else {
    ret = "dst";
  }
  ret += p.to_string();
  return ret;
}


/*
class expr_append_pc_string_visitor : public expr_visitor
{
public:
  expr_append_pc_string_visitor(eq::context *ctx, expr_ref const e, bool src, pc const &pc) : m_ctx(ctx), m_expr(e), m_src(src), m_pc(pc) {
    m_visit_each_expr_only_once = true;
    visit_recursive(e);
  }
  expr_ref get_result();

private:
  void visit(expr_ref e);
  eq::context *m_ctx;
  expr_ref m_expr;
  bool m_src;
  pc const m_pc;
  map<unsigned, expr_ref> m_map_appended;
};



void
expr_append_pc_string_visitor::visit(expr_ref e)
{
  expr_vector args_subs;
  expr_ref e_sub;
  if (m_map_appended.count(e->get_id()) > 0) {
    return;
    //e_sub = m_map_appended.at(e->get_id());
  } else {
    for (unsigned i = 0; i < e->get_args().size(); i++) {
      expr_ref arg = e->get_args()[i];
      args_subs.push_back(m_map_appended.at(arg->get_id()));
    }
    if (e->is_var()) {
      string new_name = e->get_name();
      string input_keyword = string(G_INPUT_KEYWORD) + ".";
      size_t needle = new_name.find(input_keyword);
      if (needle != string::npos) {
        new_name.replace(needle, input_keyword.length(), make_string_id_from_pc(m_src, m_pc) + ".");
      }
      e_sub = m_ctx->mk_var(new_name, e->get_sort());
    } else if (e->is_const()) {
      e_sub = e;
    } else {
      e_sub = m_ctx->mk_app(e->get_operation_kind(), args_subs);
    }
  }
  m_map_appended[e->get_id()] = e_sub;
}

expr_ref
expr_append_pc_string_visitor::get_result()
{
  return m_map_appended[m_expr->get_id()];
}

expr_ref
expr_append_pc_string(expr_ref in, bool src, ::pc const &p)
{
  expr_append_pc_string_visitor visitor(in->get_context(), in, src, p);
  expr_ref ret = visitor.get_result();
  ASSERT(ret.get_ptr());
  return ret;
}
*/

/*
static expr_ref
expr_split_on_keyword(expr_ref mem,// map<string, expr_ref> const &addr_ref,
    map<unsigned, pair<expr_ref, map<int, lr_status_t>>> const &addr_sym_map,
    map<string, expr_ref> const &mem_pools, trans_fun_graph const *tfg,
    ::pc const &from_pc, ::machine_state const *from_state,
    bool update_function_argument_ptr_memlabels,
    eq::consts_struct_t const &cs,
    map<pc_exprid_pair, int, pc_exprid_pair_comp> &expr_represents_loc_in_state_cache)
{
  //INFO("expr_split_on_keyword: splitting:\n" << expr_string(mem) << "\non keyword " << keyword << endl);
  //INFO("splitting mem (table format):\n" << mem->to_string_table() << endl);
  //ASSERT(keyword == "" || addr_ref.count(keyword) > 0);
}
*/

class expr_identify_memory_arrays : public expr_visitor
{
public:
  expr_identify_memory_arrays(expr_ref const &e) /*: m_expr(e)*/ {
    //m_visit_each_expr_only_once = true;
    visit_recursive(e);
  }
  //vector<pair<expr_ref, eq::comment_t> > const &get_store_addresses(void) { return m_mem_accesses; }
  vector<memlabel_t> const &get_store_addresses(void) { return m_mem_accesses; }
private:
  void previsit(expr_ref const &e, int interesting_args[], int &num_interesting_args)
  {
    /*if (   e->get_operation_kind() == expr::OP_FUNCTION_ARGUMENT_PTR
        || e->get_operation_kind() == expr::OP_RETURN_VALUE_PTR) {
      num_interesting_args = 0; //do not descend this subtree
      return;
    }*/
    num_interesting_args = e->get_args().size();
    for (int i = 0; i < num_interesting_args; i++) {
      interesting_args[i] = i;
    }
  }
  void visit(expr_ref const &e)
  {
    expr::operation_kind k = e->get_operation_kind();
    if (k == expr::OP_STORE) {
      //expr_ref mem = e->get_args()[0];
      memlabel_t ml = e->get_args()[1]->get_memlabel_value();
      //eq::comment_t comment = e->get_args()[6]->get_comment_value();
      //m_mem_accesses.push_back(make_pair(mem, comment));
      //m_mem_accesses.push_back(make_pair(ml, comment));
      m_mem_accesses.push_back(ml);
    } else if (k == expr::OP_SELECT) {
      //expr_ref mem = e->get_args()[0];
      memlabel_t ml = e->get_args()[1]->get_memlabel_value();
      //eq::comment_t comment = e->get_args()[5]->get_comment_value();
      //m_mem_accesses.push_back(make_pair(mem, comment));
      //m_mem_accesses.push_back(make_pair(ml, comment));
      m_mem_accesses.push_back(ml);
    } /*else if (k == expr::OP_FUNCTION_CALL) {
      for (unsigned i = 0; i < e->get_args().size(); i++) {
        if (e->get_args()[i]->get_operation_kind() == expr::OP_FUNCTION_ARGUMENT_PTR) {
          memlabel_t ml = e->get_args()[i]->get_args()[1]->get_memlabel_value();
          //ASSERT(e->get_args()[FUNCTION_CALL_COMMENT_ARG_INDEX + 1]->is_comment_sort());
          //eq::comment_t comment = function_argument_comment(e->get_args()[FUNCTION_CALL_COMMENT_ARG_INDEX + 1]->get_comment_value(), i - (FUNCTION_CALL_COMMENT_ARG_INDEX + 2));
          //eq::comment_t comment;
          //CPP_DBG_EXEC(ANNOTATE_LOCALS, cout << __func__ << " " << __LINE__ << ": pushing to m_mem_accesses: pair of " << memlabel_to_string(&ml, as1, sizeof as1) << ", comment " << comment.to_string() << endl);
          //m_mem_accesses.push_back(make_pair(ml, comment));
          m_mem_accesses.push_back(ml);
        }
      }
    }*/
  }
  //expr_ref const &m_expr;
  //vector<pair<expr_ref, eq::comment_t> >  m_mem_accesses;
  //vector<pair<memlabel_t, eq::comment_t> >  m_mem_accesses;
  vector<memlabel_t>  m_mem_accesses;
};



static void
append_mem_accesses(vector<memlabel_t> *ret, expr_ref e)
{
  expr_identify_memory_arrays marrays(e);
  CPP_DBG2(ANNOTATE_LOCALS, "e: %s\n", expr_string(e).c_str());
  vector<memlabel_t> addrs = marrays.get_store_addresses();
  ret->insert(ret->end(), addrs.begin(), addrs.end());
}

/*
vector<pair<memlabel_t, eq::comment_t> > *
state::state_get_mem_accesses() const
{
  vector<pair<memlabel_t, eq::comment_t> > *ret;
  ret = new vector<pair<memlabel_t, eq::comment_t> >;

  std::function<void (const string&, expr_ref)> f = [&ret](const string& key, expr_ref e) -> void { append_mem_accesses(ret, e); };
  visit_exprs(f);

  return ret;
}
*/

bool
state::contains_function_call() const
{
  bool ret = false;
  std::function<void (const string&, expr_ref)> f = [&ret](const string& key, expr_ref e) -> void { context *ctx = e->get_context(); ret = ret || ctx->expr_contains_function_call(e); };
  state_visit_exprs(f);
  return ret;
}

expr_ref
state::get_function_call_expr() const
{
  expr_ref ret;
  std::function<void (const string&, expr_ref)> f = [&ret](const string& key, expr_ref e) -> void
  {
    if (ret != nullptr) {
      return;
    }
    context *ctx = e->get_context();
    if (ctx->expr_contains_function_call(e)) {
      ret = e;
      return;
    }
  };
  state_visit_exprs(f);
  return ret;
}

void
state::get_function_call_nextpc(expr_ref &ret) const
{
  std::function<void (const string&, expr_ref)> f = [&ret](const string& key, expr_ref e) -> void { if (!ret) { context *ctx = e->get_context(); ctx->expr_get_function_call_nextpc(e, ret); } };
  state_visit_exprs(f);
}

void
state::get_function_call_memlabel_writeable(memlabel_t &ret, graph_memlabel_map_t const &memlabel_map) const
{
  ret.set_to_empty_mem();
  std::function<void (const string&, expr_ref)> f = [&ret, &memlabel_map](const string& key, expr_ref e) -> void
  {
    context *ctx = e->get_context();
    ctx->expr_get_function_call_memlabel_writeable(e, ret, memlabel_map);
  };
  state_visit_exprs(f);
}

/*bool
state::expr_value_exists(reg_type rt, int i, int j) const
{
  switch (rt) {
    case reg_type_exvreg: return expr_exists(reg_name(i, j));
    case reg_type_contains_float_op: return expr_exists(G_CONTAINS_FLOAT_OP_SYMBOL);
    case reg_type_contains_unsupported_op: return expr_exists(G_CONTAINS_UNSUPPORTED_OP_SYMBOL);
    default: assert(0);
  }
  NOT_REACHED();

}*/

expr_ref
state::get_expr_value(state const &start_state, reg_type rt, int i, int j) const
{
  string k;
  switch (rt) {
    //case reg_type_memory: return get_memory_expr();
    //case reg_type_io: return get_io_expr();
    case reg_type_exvreg:
      k = start_state.has_expr_substr(reg_name(i, j));
      ASSERT(k != "");
      return get_expr(k, start_state);
    case reg_type_contains_float_op:
      k = start_state.has_expr_substr(G_CONTAINS_FLOAT_OP_SYMBOL);
      ASSERT(k != "");
      return get_expr(k, start_state);
    case reg_type_contains_unsupported_op:
      k = start_state.has_expr_substr(G_CONTAINS_UNSUPPORTED_OP_SYMBOL);
      ASSERT(k != "");
      return get_expr(k, start_state);
    default: assert(0);
  }
  NOT_REACHED();
}

void
state::set_expr_value(reg_type rt, int i, int j, expr_ref e, state const &start_state)
{
  string k;
  switch(rt) {
    case reg_type_exvreg:
      k = start_state.has_expr_substr(reg_name(i, j));
      ASSERT(k != "");
      set_expr(k, e);
      break;
    case reg_type_contains_float_op:
      k = start_state.has_expr_substr(G_CONTAINS_FLOAT_OP_SYMBOL);
      ASSERT(k != "");
      set_expr(k, e);
      break;
    case reg_type_contains_unsupported_op:
      k = start_state.has_expr_substr(G_CONTAINS_UNSUPPORTED_OP_SYMBOL);
      ASSERT(k != "");
      set_expr(k, e);
      break;
    default: assert(0);
  }
}

void
state::populate_reg_counts(/*state const *start_state*/)
{
  int max_group, max_invisible_regnum, max_hidden_regnum;
  int max_regnum[MAX_NUM_EXREG_GROUPS];
  max_group = -1;
  max_invisible_regnum = -1;
  max_hidden_regnum = -1;
  for (int i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    max_regnum[i] = -1;
  }
  for (auto k : /*start_state ? start_state->m_value_exprs.get_map() : */m_value_exprs.get_map()) {
    string key = k.first->get_str();
    if (   key.find(string(".") + g_regular_reg_name) != string::npos
        || key.find(g_regular_reg_name) == 0) {
      int group, regnum;
      get_exreg_details_from_name(key, &group, &regnum);
      max_group = max(max_group, group);
      max_regnum[group] = max(max_regnum[group], regnum);
    } else if (   key.find(string(".") + g_invisible_reg_name) != string::npos
               || key.find(g_invisible_reg_name) == 0) {
      size_t first_dot = key.find(g_invisible_reg_name);
      ASSERT(first_dot != string::npos);
      first_dot += g_invisible_reg_name.size() - 1;
      ASSERT(key.at(first_dot) == '.');
      string regnum_str = key.substr(first_dot + 1);
      int regnum = stoi(regnum_str);
      max_invisible_regnum = max(max_invisible_regnum, regnum);
    } else if (   key.find(string(".") + g_hidden_reg_name) != string::npos
               || key.find(g_hidden_reg_name) == 0) {
      //size_t first_dot = g_hidden_reg_name.size() - 1;
      size_t first_dot = key.find(g_hidden_reg_name);
      ASSERT(first_dot != string::npos);
      first_dot += g_hidden_reg_name.size() - 1;
      ASSERT(key.at(first_dot) == '.');
      string regnum_str = key.substr(first_dot + 1);
      int regnum = stoi(regnum_str);
      max_hidden_regnum = max(max_hidden_regnum, regnum);
    }
  }
  //m_num_exreg_groups = max_group + 1;
  //m_num_invisible_regs = max_invisible_regnum + 1;
  //m_num_hidden_regs = max_hidden_regnum + 1;
  //for (int i = 0; i < m_num_exreg_groups; i++) {
  //  m_num_exregs.push_back(max_regnum[i] + 1);
  //  ASSERT(m_num_exregs[i]);
  //}
}

void
state::get_exreg_details_from_name(string key, int *group, int *regnum)
{
  //size_t first_dot = g_regular_reg_name.size() - 1;
  size_t first_dot = key.find(g_regular_reg_name); //g_regular_reg_name.size() - 1;
  ASSERT(first_dot != string::npos);
  first_dot += g_regular_reg_name.size() - 1;
  ASSERT(key.at(first_dot) == '.');
  size_t second_dot = key.find('.', first_dot + 1);
  string group_str = key.substr(first_dot + 1, second_dot - first_dot - 1);
  string regnum_str = key.substr(second_dot + 1);
  *group = stoi(group_str);
  *regnum = stoi(regnum_str);
}

vector<expr_ref> const
state::get_all_function_calls() const
{
  vector<expr_ref> ret;

  std::function<void (const string&, expr_ref)> f = [&ret](const string& key, expr_ref e) -> void
  {
    expr_vector fcalls = expr_get_function_calls(e);
    //cout << __func__ << " " << __LINE__ << ": key = " << key << ": fcalls.size() = " << fcalls.size() << endl;
    ret.insert(ret.end(), fcalls.begin(), fcalls.end());
    //cout << __func__ << " " << __LINE__ << ": key = " << key << ": ret.size() = " << ret.size() << endl;
  };
  state_visit_exprs(f);

  return ret;
}

expr_ref
state::reg_apply_function_call(expr_ref nextpc_id, unsigned num_args, int r_esp, int reg_id)
{
  NOT_IMPLEMENTED();
/*
  expr_ref mem, stack, function_name, reg_id_expr;
  vector<sort_ref> args_sort;
  struct context *ctx;
  expr_vector args;
  size_t i;

  ctx = nextpc_id->get_context();

  mem = get_memory_expr(); //m_mem_pools.at(g_mem_symbol);

  reg_id_expr = ctx->mk_bv_const(DWORD_LEN, reg_id);

  args_sort.push_back(mem->get_sort());
  args_sort.push_back(nextpc_id->get_sort());
  args_sort.push_back(reg_id_expr->get_sort());
  for (i = 0; i < num_args; i++) {
    args_sort.push_back(ctx->mk_bv_sort(DWORD_LEN));
  }
  stringstream ss;
  ss << "call." << (num_args + 3) << ".reg";
  function_name = ctx->mk_function_decl(ss.str(), args_sort, ctx->mk_bv_sort(DWORD_LEN));

  ASSERT(r_esp >= 0 && r_esp < I386_NUM_EXREGS(I386_EXREG_GROUP_GPRS));
  expr_ref esp = get_expr_value(reg_type_exvreg, I386_EXREG_GROUP_GPRS, r_esp);
  args.push_back(function_name);
  args.push_back(mem);
  args.push_back(nextpc_id);
  args.push_back(reg_id_expr);

  //ASSERT(m_mem_pools.count(g_stack_symbol) == 0);
  stack = get_memory_expr(); //m_mem_pools.at(g_mem_symbol); //split has not happened yet
  for (i = 0; i < num_args; i++) {
    memlabel_t memlabel;

    expr_ref argloc = ctx->mk_bvadd(esp, ctx->mk_bv_const(DWORD_LEN, (i+1) * DWORD_LEN/BYTE_LEN));
    keyword_to_memlabel(&memlabel, G_MEM_SYMBOL);
    expr_ref arg = ctx->mk_select(stack, memlabel, argloc, DWORD_LEN/BYTE_LEN, false, eq::comment_t());
    args.push_back(arg);
  }
  return ctx->mk_app(expr::OP_FUNCTION_CALL, args);
*/
}

void
state::apply_function_call(expr_ref nextpc_id, unsigned num_args, int r_esp, int r_eax, state const &start_state)
{
  //m_mexvregs[I386_EXREG_GROUP_GPRS][r_eax] = this->reg_apply_function_call(nextpc_id, num_args, r_esp, 1);
  set_expr_value(reg_type_exvreg, I386_EXREG_GROUP_GPRS, r_eax, reg_apply_function_call(nextpc_id, num_args, r_esp, 1), start_state);
  //m_mexvregs[I386_EXREG_GROUP_GPRS][R_ECX] = this->reg_apply_function_call(nextpc_id, num_args, r_esp, 2);
  //m_mexvregs[I386_EXREG_GROUP_GPRS][R_EDX] = this->reg_apply_function_call(nextpc_id, num_args, r_esp, 3);
  //what about memory?
}

/*void
state::state_canonicalize_llvm_symbols(vector<string> &symbol_map)
{
  std::function<expr_ref (const string&, expr_ref)> f = [&symbol_map](const string& key, expr_ref e) -> expr_ref { return e->expr_canonicalize_llvm_symbols(symbol_map); };
  visit_exprs(f);
}

void
state::state_canonicalize_llvm_nextpcs(map<int, string> &nextpc_map)
{
  std::function<expr_ref (const string&, expr_ref)> f = [&nextpc_map](const string& key, expr_ref e) -> expr_ref { return e->expr_canonicalize_llvm_nextpcs(nextpc_map); };
  visit_exprs(f);
}*/

void
state::zero_out_dead_vars(set<string> const &live_vars)
{
  map<string_ref, expr_ref> value_expr_map = m_value_exprs.get_map();
  for (map<string_ref, expr_ref>::iterator iter = value_expr_map.begin();
       iter != value_expr_map.end();
       iter++) {
    string key = iter->first->get_str();
    if (live_vars.find(key) == live_vars.end()) {
      context *ctx = iter->second->get_context();
      if (iter->second->is_bv_sort()) {
        iter->second = ctx->mk_zerobv(iter->second->get_sort()->get_size());
      } else if (iter->second->is_bool_sort()) {
        iter->second = ctx->mk_bool_const(false);
      }
    }
  }
  m_value_exprs.set_map(value_expr_map);
}

void
state::get_live_vars_from_exprs(set<string> &vars, vector<expr_ref> const &live_exprs) const
{
  vars.clear();
  for (map<string_ref, expr_ref>::const_iterator iter = m_value_exprs.get_map().begin();
       iter != m_value_exprs.get_map().end();
       iter++) {
    for (auto live_expr : live_exprs) {
      if (to_value_depends_on_from_value(live_expr, iter->second)) {
        vars.insert(iter->first->get_str());
        break;
      }
    }
  }
}

static string
function_argument_get_name(expr_ref arg)
{
  ASSERT(arg->is_var());
  string name = expr_string(arg);
  ASSERT(name.substr(0, strlen(G_INPUT_KEYWORD) + 1) == string(G_INPUT_KEYWORD) + ".");
  return name.substr(strlen(G_INPUT_KEYWORD) + 1);
}

/*state
state::add_function_arguments(expr_vector const &function_arguments) const
{
  state ret = *this;
  for (auto arg : function_arguments) {
    string argname = function_argument_get_name(arg);
    ret.m_value_expr_map[argname] = arg;
  }
  return ret;
}*/

bool
state::is_exit_function(string const &fun_name)
{
  return fun_name == "myexit" || fun_name == "exit" || fun_name == "_exit";
}

bool
state::contains_exit_fcall(map<int, string> const &nextpc_map, int *nextpc_num) const
{
  expr_ref mem;
  if (!this->get_memory_expr(*this, mem)) {
    return false;
  }
  if (mem->get_operation_kind() != expr::OP_FUNCTION_CALL) {
    return false;
  }
  expr_ref nextpc = mem->get_args().at(OP_FUNCTION_CALL_ARGNUM_NEXTPC_CONST);
  if (!nextpc->is_nextpc_const()) {
    return false;
  }
  ASSERT(nextpc->is_var());
  ASSERT(nextpc->is_nextpc_const());
  *nextpc_num = nextpc->get_nextpc_num();
  if (nextpc_map.count(*nextpc_num) == 0) {
    return false;
  }
  ASSERT(nextpc_map.count(*nextpc_num) > 0);
  return is_exit_function(nextpc_map.at(*nextpc_num));
}

bool
state::is_fcall_edge(nextpc_id_t &nextpc_num) const
{
  expr_ref mem;
  if (!this->get_memory_expr(*this, mem)) {
    return false;
  }
  if (mem->get_operation_kind() != expr::OP_FUNCTION_CALL) {
    return false;
  }
  expr_ref nextpc = mem->get_args().at(OP_FUNCTION_CALL_ARGNUM_NEXTPC_CONST);
  if (!nextpc->is_nextpc_const()) {
    return false;
  }
  ASSERT(nextpc->is_var());
  ASSERT(nextpc->is_nextpc_const());
  nextpc_num = nextpc->get_nextpc_num();
  return true;
}

state
state::state_union(state const &a, state const &b)
{
  //ASSERT(a.m_num_exreg_groups == 0 || b.m_num_exreg_groups == 0);
  state const *src_state;
  state const *dst_state;
  /*if (a.m_num_exreg_groups == 0) */{
    src_state = &a;
    dst_state = &b;
  }/* else {
    src_state = &b;
    dst_state = &a;
  }*/
  state ret = *dst_state;
  for (auto ve : src_state->m_value_exprs.get_map()) {
    if (ret.m_value_exprs.get_map().count(ve.first)) {
      continue;
    }
    //ret.m_value_expr_map[ve.first] = ve.second;
    //ret.m_determinized_value_expr_map[ve.first->get_str()] = ve.second;
    ret.m_value_exprs.insert(make_pair(ve.first, ve.second));
  }
  return ret;
}

void
state::debug_check(consts_struct_t const *cs) const
{
  std::function<void (const string&, expr_ref)> f = [&cs](const string& key, expr_ref e) -> void
  {
    expr_debug_check(e, cs);
  };
  state_visit_exprs(f);
}

state
state::state_minus(state const &other) const
{
  set<string_ref> erase_set;
  state ret = *this;
  for (auto ve : m_value_exprs.get_map()) {
    if (   other.m_value_exprs.get_map().count(ve.first)
        && ve.second == other.m_value_exprs.get_map().at(ve.first)) {
      erase_set.insert(ve.first);
    }
  }
  for (auto e : erase_set) {
     ret.m_value_exprs.erase(e);
    //ret.m_value_expr_map.erase(e);
    //ret.m_determinized_value_expr_map.erase(e->get_str());
  }
  return ret;
}

#if 0
void
state::state_set_memlabels_to_top_or_constant_values(graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map/*, set<cs_addr_ref_id_t> const &relevant_addr_refs*/, consts_struct_t const &cs, state const &start_state, bool update_callee_memlabels, sprel_map_t const *sprel_map, map<graph_loc_id_t, expr_ref> const &locid2expr_map, string const &graph_name, long &mlvarnum, graph_memlabel_map_t &memlabel_map) const
{
  std::function<void (const string&, expr_ref)> f = [&symbol_map, &locals_map/*, &relevant_addr_refs*/, &cs, update_callee_memlabels, sprel_map, &locid2expr_map, &graph_name, &mlvarnum, &memlabel_map](string const & key, expr_ref e) -> void
  {
    //cout << __func__ << " " << __LINE__ << ": key = " << key << ", e = " << expr_string(e) << endl;
    expr_set_memlabels_to_top_or_constant_values(e, symbol_map, locals_map/*, relevant_addr_refs*/, cs, update_callee_memlabels, sprel_map, locid2expr_map, graph_name, mlvarnum, memlabel_map);
    //cout << __func__ << " " << __LINE__ << ": key = " << key << ", ret = " << expr_string(ret) << endl;
    //return ret;
  };
  state_visit_exprs(f, start_state);
}
#endif

void
state::remove_key(string const &val_name)
{
  m_value_exprs.erase(mk_string_ref(val_name));
  //m_value_expr_map.erase(mk_string_ref(val_name));
  //m_determinized_value_expr_map.erase(val_name);
}

vector<farg_t>
state::state_get_fcall_retvals() const
{
  map<int, farg_t> retregs;
  string key_prefix;
  for (pair<string, expr_ref> const& key_expr : this->m_value_exprs.get_det_map()) {
    if (key_expr.second->is_array_sort()) {
      continue;
    }
    if (string_contains(key_expr.first, LLVM_FCALL_ARG_COPY_PREFIX)) {
      continue;
    }
    ASSERT(key_expr.second->is_bv_sort() || key_expr.second->is_bool_sort());
    size_t sz = key_expr.second->is_bool_sort() ? 1 : key_expr.second->get_sort()->get_size();
    farg_t farg(sz);
    string const& k = key_expr.first;
    size_t lastdot = k.rfind('.');
    if (lastdot == string::npos || !string_has_prefix(k.substr(lastdot), "." LLVM_FIELDNUM_PREFIX)) {
      if (retregs.size()) {
        cout << __func__ << " " << __LINE__ << ": this =\n" << this->to_string() << endl;
        cout << __func__ << " " << __LINE__ << ": keys (excluding mem and farg copies) =\n";
        for (auto const& ke : this->m_value_exprs.get_det_map()) {
          if (ke.second->is_array_sort()) {
            continue;
          }
          if (string_contains(ke.first, LLVM_FCALL_ARG_COPY_PREFIX)) {
            continue;
          }
          cout << ke.first << endl;
        }
        cout << "k = " << k << ". lastdot = " << lastdot << endl;
      }
      ASSERT(retregs.size() == 0 && "More than one non-memory state elements are modified by the function call");
      key_prefix = k;
      retregs.insert(make_pair(0, farg));
      continue;
    }
    ASSERT(key_prefix == "" || key_prefix == k.substr(0, lastdot));
    key_prefix = k.substr(0, lastdot);
    int fieldnum = strtol(k.substr(lastdot + strlen("." LLVM_FIELDNUM_PREFIX)).c_str(), nullptr, 10);
    retregs.insert(make_pair(fieldnum, farg));
  }
  vector<farg_t> ret;
  for (auto const& retreg : retregs) {
    ret.push_back(retreg.second);
  }
  return ret;
}

tuple<nextpc_id_t, vector<farg_t>, mlvarname_t, mlvarname_t>
state::get_nextpc_args_memlabel_map_for_edge(state const &start_state, context *ctx) const
{
  autostop_timer func_timer(string(__func__));
  bool found = false;
  expr_ref fcall_expr;
  bool fcall_found = this->get_fcall_expr(fcall_expr);
  ASSERT(fcall_found);
  ASSERT(fcall_expr->get_operation_kind() == expr::OP_FUNCTION_CALL);
  tuple<nextpc_id_t, vector<farg_t>, mlvarname_t, mlvarname_t> ret = ctx->expr_get_nextpc_args_memlabel_map(fcall_expr);
  return ret;
  //vector<farg_t> retvals = this->state_get_fcall_retvals();
  //nextpc_id_t npc_id = get<0>(ret);
  //fsignature_t fsig(get<1>(ret), retvals);
  //mlvarname_t const& mlreadable = get<2>(ret);
  //mlvarname_t const& mlwriteable = get<3>(ret);
  //return make_tuple(npc_id, fsig, mlreadable, mlwriteable);
}

state
state::add_nextpc_retvals(vector<farg_t> const& fretvals, expr_ref const& dst_fcall_retval_buffer, expr_ref const& mem, expr_ref const& ml_callee_readable, expr_ref const& ml_callee_writeable, expr_ref const& nextpc, string const& dst_stackpointer_key, expr_ref const &stackpointer, state const &start_state, context *ctx, string const &graph_name, long &max_memlabel_varnum) const
{
  if (fretvals.size() == 0) {
    return *this;
  }
  state ret = *this;

  vector<expr_ref> args;
  vector<sort_ref> arg_types;

  args.push_back(ml_callee_readable);
  arg_types.push_back(ml_callee_readable->get_sort());
  args.push_back(ml_callee_writeable);
  arg_types.push_back(ml_callee_writeable->get_sort());
  args.push_back(mem);
  arg_types.push_back(mem->get_sort());
  args.push_back(nextpc);
  arg_types.push_back(nextpc->get_sort());
#if (CALLING_CONVENTIONS == CDECL) && (ARCH_DST == ARCH_I386)
  if (fretvals.size() == 1) {
    args.push_back(ctx->mk_regid_const(G_FUNCTION_CALL_REGNUM_RETVAL(0)));
    arg_types.push_back(ctx->mk_regid_sort());
    size_t sz = fretvals.at(0).get_size();
    sort_ref s = ctx->mk_bv_sort(sz);
    expr_ref fun = ctx->get_fun_expr(arg_types, s);
    expr_ref e = ctx->mk_function_call(fun, args);

    stringstream ss;
    ss << G_DST_KEYWORD "." G_REGULAR_REG_NAME << DST_EXREG_GROUP_GPRS << "." << R_EAX;
    string eax = ss.str();
    ss.str("");
    ss << G_DST_KEYWORD "." G_REGULAR_REG_NAME << DST_EXREG_GROUP_GPRS << "." << R_EDX;
    string edx = ss.str();
    ASSERT(ret.key_exists(eax/*, ret*/));
    ASSERT(ret.key_exists(edx/*, ret*/));

    if (sz <= DWORD_LEN) {
      if (sz < DWORD_LEN) {
        ret.set_expr(eax, ctx->mk_bvzero_ext(e, DWORD_LEN - sz));
      } else {
        ret.set_expr(eax, e);
      }
    } else if (sz <= QWORD_LEN) {
      ret.set_expr(eax, ctx->mk_bvextract(e, DWORD_LEN - 1, 0));
      ret.set_expr(edx, ctx->mk_bvextract(e, sz - 1, DWORD_LEN));
    } else {
      NOT_IMPLEMENTED();
    }
  } else {
    size_t off = 0;
    mlvarname_t mlvarname = ctx->memlabel_varname(graph_name, max_memlabel_varnum);
    max_memlabel_varnum++;

    expr_ref smem;
    bool smemfound = this->get_memory_expr(start_state, smem);
    ASSERT(smemfound);
    //cout << __func__ << " " << __LINE__ << ": fretvals.size() = " << fretvals.size() << endl;
    for (size_t i = 0; i < fretvals.size(); i++) {
      auto args_copy = args;
      auto arg_types_copy = arg_types;
      args_copy.push_back(ctx->mk_regid_const(G_FUNCTION_CALL_REGNUM_RETVAL(i)));
      arg_types_copy.push_back(ctx->mk_regid_sort());
      size_t sz = fretvals.at(i).get_size();
      size_t align = fretvals.at(i).get_alignment();
      sort_ref s = ctx->mk_bv_sort(sz);
      expr_ref fun = ctx->get_fun_expr(arg_types_copy, s);
      expr_ref e = ctx->mk_function_call(fun, args_copy);
      while (off % align) {
        off++;
      }
      size_t sz_in_bytes = DIV_ROUND_UP(sz, BYTE_LEN);
      expr_ref addr = dst_fcall_retval_buffer;
      addr = ctx->mk_bvadd(addr, ctx->mk_bv_const(addr->get_sort()->get_size(), off));
      smem = ctx->mk_store(smem, mlvarname, addr, e, sz_in_bytes, false);
      ret.set_expr(start_state.get_memname(), smem);
      off += sz_in_bytes;
    }
    ret.set_expr(dst_stackpointer_key, ctx->mk_bvadd(stackpointer, ctx->mk_bv_const(stackpointer->get_sort()->get_size(), DWORD_LEN/BYTE_LEN)));
  }
  return ret;
#else
  NOT_IMPLEMENTED();
#endif
}

state
state::add_nextpc_args_and_retvals_for_edge(fsignature_t const& fsignature, string const& dst_stackpointer_key, expr_ref const &stackpointer, state const &start_state, context *ctx, string const &graph_name, long &max_memlabel_varnum) const
{
  autostop_timer func_timer(string(__func__));
  expr_ref mem;
  bool memfound = start_state.get_memory_expr(start_state, mem);
  ASSERT(memfound);
  expr_ref dst_fcall_retval_buffer;
  vector<expr_ref> dst_fcall_args = fsignature_t::get_dst_fcall_args_for_fsignature(fsignature, mem, stackpointer, graph_name, max_memlabel_varnum, dst_fcall_retval_buffer);

  expr_ref fcall_expr;
  bool fcall_found = this->get_fcall_expr(fcall_expr);
  ASSERT(fcall_found);
  ASSERT(fcall_expr->get_operation_kind() == expr::OP_FUNCTION_CALL);
  expr_ref const& ml_callee_readable = fcall_expr->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE);
  expr_ref const& ml_callee_writeable = fcall_expr->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE);
  expr_ref const& nextpc = fcall_expr->get_args().at(OP_FUNCTION_CALL_ARGNUM_NEXTPC_CONST);

  //CPP_DBG_EXEC2(TMP, cout << __func__ << " " << __LINE__ << ": this =\n" << this->to_string() << endl);
  state ret = this->add_nextpc_retvals(fsignature.get_retvals(), dst_fcall_retval_buffer, mem, ml_callee_readable, ml_callee_writeable, nextpc, dst_stackpointer_key, stackpointer, start_state, ctx, graph_name, max_memlabel_varnum);
  //CPP_DBG_EXEC2(TMP, cout << __func__ << " " << __LINE__ << ": ret =\n" << ret.to_string() << endl);
  std::function<expr_ref (const string&, expr_ref)> f = [&dst_fcall_args, ctx](string const &key, expr_ref e) -> expr_ref
  {
    return ctx->expr_add_nextpc_args(e, dst_fcall_args);
  };
  ret.state_visit_exprs_with_start_state(f/*, start_state*/);
  //CPP_DBG_EXEC2(TMP, cout << __func__ << " " << __LINE__ << ": ret =\n" << ret.to_string() << endl);
  return ret;
}

void
state::intersect_and_update_callee_memlabels_at_fcalls(state const &start_state, graph_memlabel_map_t &memlabel_map, context *ctx) const
{
  autostop_timer func_timer(string(__func__));
  set<memlabel_t> callee_readable_memlabels, callee_writeable_memlabels;
  std::function<void (const string&, expr_ref)> fconst = [&callee_readable_memlabels, &callee_writeable_memlabels, &memlabel_map, ctx](string const &key, expr_ref e) -> void
  {
    ctx->expr_get_callee_memlabels(e, memlabel_map, callee_readable_memlabels, callee_writeable_memlabels);
  };
  this->state_visit_exprs_with_start_state(fconst/*, start_state*/);
  ASSERT(callee_readable_memlabels.size() <= 2);
  ASSERT(callee_writeable_memlabels.size() <= 2);
  memlabel_t callee_readable = memlabel_t::memlabels_intersect(callee_readable_memlabels);
  memlabel_t callee_writeable = memlabel_t::memlabels_intersect(callee_writeable_memlabels);

  std::function<void (const string&, expr_ref)> f = [&callee_readable, &callee_writeable, &memlabel_map, ctx](string const &key, expr_ref e) -> void
  {
    ctx->expr_update_callee_memlabels(e, memlabel_map, callee_readable, callee_writeable);
  };
  this->state_visit_exprs_with_start_state(f/*, start_state*/);
}

void
state::state_rename_keys(map<string, string> const &key_map)
{
  map<string_ref, expr_ref> new_value_expr_map;
  //map<string, expr_ref> new_deterministic_value_expr_map;
  for (const auto &kv : m_value_exprs.get_map()) {
    string key = kv.first->get_str();
    expr_ref const &e = kv.second;
    if (key_map.count(key)) {
      key = key_map.at(key);
    }
    new_value_expr_map[mk_string_ref(key)] = e;
    //new_deterministic_value_expr_map[key] = e;
  }
  //m_value_expr_map = new_value_expr_map;
  //m_determinized_value_expr_map = new_deterministic_value_expr_map;
  m_value_exprs.set_map(new_value_expr_map);
}

void
state::state_introduce_src_dst_prefix(char const *prefix, bool update_exprs)
{
  map<string_ref, expr_ref> new_value_expr_map;
  //map<string, expr_ref> new_deterministic_value_expr_map;
  for (const auto &kv : m_value_exprs.get_map()) {
    string const &key = kv.first->get_str();
    expr_ref const &e = kv.second;
    string new_key = string(prefix) + "." + key;
    new_value_expr_map[mk_string_ref(new_key)] = update_exprs ? expr_introduce_src_dst_prefix(e, prefix) : e;
    //new_deterministic_value_expr_map[new_key] = update_exprs ? expr_introduce_src_dst_prefix(e, prefix) : e;
  }
  m_value_exprs.set_map(new_value_expr_map);
  //m_value_expr_map = new_value_expr_map;
  //m_determinized_value_expr_map = new_deterministic_value_expr_map;
}

void
state::state_truncate_register_sizes_using_regset(regset_t const &regset)
{
  //cout << __func__ << " " << __LINE__ << ": this =\n" << this->to_string_for_eq() << endl;
  //set<string> keys_touched;
  for (const auto &g : regset.excregs) {
    for (const auto &r : g.second) {
      //cout << __func__ << " " << __LINE__ << ": g.first = " << g.first << endl;
      //cout << __func__ << " " << __LINE__ << ": r.first = " << r.first.reg_identifier_to_string() << endl;
      //cout << __func__ << " " << __LINE__ << ": r.second = " << hex << r.second << dec << endl;
      int touch_len = bitmap_get_max_bit_set(r.second) + 1;
      ASSERT(touch_len > 0);
      stringstream ss;
      ss << G_SRC_KEYWORD "." G_REGULAR_REG_NAME << g.first << "." << r.first.reg_identifier_to_string();
      string key = ss.str();
      ASSERT(m_value_exprs.get_map().count(mk_string_ref(key)));
      expr_ref e = m_value_exprs.get_map().at(mk_string_ref(key));
      ASSERT(e->is_var());
      context *ctx = e->get_context();
      string ename = expr_string(e);
      e = ctx->mk_var(ename, ctx->mk_bv_sort(touch_len));
      m_value_exprs.insert(make_pair(mk_string_ref(key), e));
      //m_value_expr_map[mk_string_ref(key)] = e;
      //m_determinized_value_expr_map[key] = e;
      //keys_touched.insert(keys);
    }
  }
  //cout << __func__ << " " << __LINE__ << ": this =\n" << this->to_string_for_eq() << endl;
}

bool
state::key_represents_exreg_key(string const &k, exreg_group_id_t &group, reg_identifier_t &reg_id)
{
  char const *pattern = "." G_REGULAR_REG_NAME;
  size_t prefix;
  if ((prefix = k.find(pattern)) != string::npos) {
    prefix = prefix + strlen(pattern);
    size_t dot = k.find('.', prefix);
    if (dot == string::npos) {
      cout << __func__ << " " << __LINE__ << ": k = " << k << endl;
      cout << __func__ << " " << __LINE__ << ": prefix = " << prefix << endl;
    }
    ASSERT(dot != string::npos);
    string group_str = k.substr(prefix, dot);
    group = stol(group_str);
    string regnum_str = k.substr(dot + 1);
    reg_id = get_reg_identifier_for_regname(regnum_str);
    return true;
  }
  return false;
}

void
state::eliminate_hidden_regs()
{
  set<string> hidden_keys;
  for (const auto &me : m_value_exprs.get_map()) {
    string const &k = me.first->get_str();
    if (k.find(g_hidden_reg_name) != string::npos) {
      hidden_keys.insert(k);
    }
  }
  for (const auto &k : hidden_keys) {
    m_value_exprs.erase(mk_string_ref(k));
    //m_value_expr_map.erase(mk_string_ref(k));
    //m_determinized_value_expr_map.erase(k);
  }
}

vector<expr_ref>
state::state_get_fcall_args() const
{
  vector<expr_ref> ret;
  for (const auto &p : m_value_exprs.get_map()) {
    if (p.second->get_operation_kind() == expr::OP_FUNCTION_CALL) {
      vector<expr_ref> const &args = p.second->get_args();
      ASSERT(args.size() >= OP_FUNCTION_CALL_ARGNUM_FARG0);
      for (size_t i = OP_FUNCTION_CALL_ARGNUM_FARG0; i < args.size(); i++) {
        ret.push_back(args.at(i));
      }
      return ret;
    }
  }
  NOT_REACHED();
}

void
state::replace_fcall_args(vector<expr_ref> const &from, vector<expr_ref> const &to)
{
  ASSERT(from.size() == to.size());
  map<string_ref, expr_ref> value_expr_map = m_value_exprs.get_map();
  for (auto &p : value_expr_map) {
    expr_ref e = p.second;
    context *ctx = e->get_context();
    if (e->get_operation_kind() != expr::OP_FUNCTION_CALL) {
      continue;
    }
    vector<expr_ref> const &args = p.second->get_args();
    vector<expr_ref> new_args = args;
    ASSERT(args.size() >= OP_FUNCTION_CALL_ARGNUM_FARG0);
    ASSERT(from.size() == to.size());
    ASSERT(from.size() == args.size() - OP_FUNCTION_CALL_ARGNUM_FARG0);
    for (size_t i = OP_FUNCTION_CALL_ARGNUM_FARG0; i < args.size(); i++) {
      ASSERT(from.at(i - OP_FUNCTION_CALL_ARGNUM_FARG0) == args.at(i));
      new_args[i] = to.at(i - OP_FUNCTION_CALL_ARGNUM_FARG0);
    }
    p.second = ctx->mk_app(expr::OP_FUNCTION_CALL, new_args);
    //m_determinized_value_expr_map[p.first->get_str()] = p.second;
  }
  m_value_exprs.set_map(value_expr_map);
}

bool
state::get_fcall_expr(expr_ref& fcall_expr) const
{
  for (pair<string, expr_ref> const& key_expr : this->m_value_exprs.get_det_map()) {
    if (key_expr.second->get_operation_kind() == expr::OP_FUNCTION_CALL) {
      fcall_expr = key_expr.second;
      return true;
    }
  }
  return false;
}

string
state::get_reg_name(const string& line)
{
  return line.substr(1);
}



string
state::read_state(istream& in, map<string_ref, expr_ref> & st_value_expr_map, context* ctx/*, state const *start_state*/)
{
  //DEBUG("reading state:" << endl);
  string line;
  bool end = !!getline(in, line);
  while (end && line != "") {
    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    /*if (!is_line(line, "=")) {
      line = "";
      break;
    }*/
    if (   is_line(line, "=StateTo")
      	|| is_line(line, "=edge")
        || is_line(line, "=constituent_itfg_edge")
      	|| is_line(line, "=comment")
        || is_line(line, "=Edge")
        || is_line(line, "=Locs")
        || is_line(line, "=Node assumes")
        || is_line(line, "=TFG:")
        || is_line(line, "=tfg")
        || is_line(line, "=pcrel_val")
        || is_line(line, "CP-theorem-suggested:")
        || is_line(line, "Invariant")
        || is_line(line, "OutgoingValue")
        || is_line(line, "=insn")
        || is_line(line, "=itfg_edge.Comment")
        || is_line(line, "Edge.te_comment")
        || is_line(line, "Edge.src_edge_composition")
        || is_line(line, "Edge.dst_edge_composition")
        || is_line(line, "Edge.Assumes.begin")
        || is_line(line, "==")) {
      break;
    }
    string reg = get_reg_name(line);
    //CPP_DBG_EXEC(TMP, cout << "reading reg:" << reg << endl);
    expr_ref e;
    line = read_expr(in, e, ctx);
    //CPP_DBG_EXEC(TMP, cout << "line = " << line << endl);
    //CPP_DBG_EXEC(TMP, cout << "e = " << expr_string(e) << endl);
    //st.set_expr(reg, e);
    st_value_expr_map.insert(make_pair(mk_string_ref(reg), e));
    //cout << __func__ << " " << __LINE__ << ": st =\n" << st.to_string() << endl;
    if (line == "") {
      break;
    }
  }
  return line;
}

}
