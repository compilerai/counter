#pragma once

#include "gsupport/pc.h"
#include "graph/graph.h"
#include "support/utils.h"
#include "support/log.h"
#include "expr/context.h"
#include "expr/expr_utils.h"
#include "graph/binary_search_on_preds.h"
#include "support/timers.h"
#include "expr/state.h"

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC>
class edge_with_cond : public edge<T_PC>
{
public:
  edge_with_cond(const T_PC& pc_from, const T_PC& pc_to, expr_ref const &condition, state const &to_state, unordered_set<expr_ref> const& assumes) : edge<T_PC>(pc_from, pc_to), m_pred(condition), m_to_state(to_state), m_assumes(assumes)
  {
  }
  edge_with_cond(const T_PC& pc_from, const T_PC& pc_to, expr_ref const &condition, state const &to_state, state const &start_state, unordered_set<expr_ref> const& assumes)
    : edge_with_cond<T_PC>(pc_from, pc_to, condition, to_state.state_minus(start_state), assumes)
  {
  }

  virtual void visit_exprs_const(std::function<void (string const&, expr_ref)> f) const
  {
    f(G_EDGE_COND_IDENTIFIER, this->get_cond());
    this->get_to_state().state_visit_exprs(f);
    unordered_set_visit_exprs_const(this->get_assumes(), f);
  }
  virtual void visit_exprs_const(function<void (T_PC const&, string const&, expr_ref)> f) const
  {
    T_PC const& p = this->get_from_pc();
    function<void (string const&, expr_ref)> ef = [&p,f](string const& s, expr_ref e) { f(p, s, e); };
    this->visit_exprs_const(ef);
  }

  state const& get_to_state() const { return m_to_state; }
  state const& get_simplified_to_state() const { return m_simplified_to_state; }


  expr_ref const& get_cond() const { return m_pred; }
  expr_ref const& get_simplified_cond() const { return m_simplified_pred; }

  unordered_set<expr_ref> const& get_assumes() const { return m_assumes; }
  unordered_set<expr_ref> const& get_simplified_assumes() const { return m_simplified_assumes; }

  void set_simplified_to_state(state const &s) const { m_simplified_to_state = s; }
  void set_simplified_cond(expr_ref const& pred) const { m_simplified_pred = pred; }
  void set_simplified_assumes(unordered_set<expr_ref> const& assumes) const { m_simplified_assumes = assumes; }

  bool contains_function_call(void) const
  {
    context *ctx = m_pred->get_context();
    //bool cond_contains = ctx->expr_contains_function_call(get_cond()); // not required as fcalls have their dedicated edges
    bool state_contains = get_to_state().contains_function_call();
    return /*cond_contains || */state_contains;
  }

  bool is_fcall_edge(nextpc_id_t &nextpc_num) const
  {
    return this->get_to_state().is_fcall_edge(nextpc_num);
  }

  using edge<T_PC>::to_string; // let the compiler know that we are overloading the virtual function to_string
  virtual string to_string(/*state const *start_state*/) const;
  string to_string_for_eq(string const& prefix) const;

  bool contains_exit_fcall(map<int, string> const &nextpc_map, int *nextpc_num) const
  {
    return get_to_state().contains_exit_fcall(nextpc_map, nextpc_num);
  }
  memlabel_t
  get_function_call_memlabel_writeable(graph_memlabel_map_t const &memlabel_map) const
  {
    memlabel_t ret;
    state const &to_state = this->get_to_state();
    to_state.get_function_call_memlabel_writeable(ret, memlabel_map);
    return ret;
  }

  expr_ref
  get_function_call_nextpc() const
  {
    expr_ref ret;
    state const &to_state = this->get_to_state();
    to_state.get_function_call_nextpc(ret);
    return ret;
  }

  vector<expr_ref> const
  get_all_function_calls() const
  {
    state const &to_state = this->get_to_state();
    return to_state.get_all_function_calls();
  }

  bool is_nop_edge(/*state const &from_state*/) const
  {
    for (const std::pair<string_ref, expr_ref>& val : this->get_to_state().get_value_expr_map_ref()) {
      context* ctx = val.second->get_context();
      if (   val.first->get_str().find(g_hidden_reg_name) != string::npos
          || ctx->get_input_expr_for_key(val.first, val.second->get_sort()) != val.second) {
        return false;
      }
    }
    return true;
  }

  set<state::mem_access> get_all_mem_accesses(string prefix) const
  {
    //set<expr_ref, expr_deterministic_cmp_t> selects, stores, fcalls, farg_ptrs, retval_ptrs;
    set<state::mem_access> ret;
    list<expr_ref> exprs;

    //get_to_state().debug_check(NULL);
    get_to_state().get_regs(exprs);
    for(auto e : exprs) {
      //expr_debug_check(e, NULL);
      state::find_all_memory_exprs_for_all_ops(prefix + ".to_state", e, ret);
    }

    state::find_all_memory_exprs_for_all_ops(prefix + ".cond", get_cond(), ret);

    //state::add_to_mem_reads_write(selects, ret);
    //state::add_to_mem_reads_write(stores, ret);
    //state::add_to_mem_reads_write(farg_ptrs, ret);
    //state::add_to_mem_reads_write(retval_ptrs, ret);
    return ret;
  }

  bool edge_get_size_for_regname(string const &regname, size_t &sz) const
  {
    bool ret = false;
    std::function<void (const string&, expr_ref)> f = [&regname, &ret, &sz](string const &key, expr_ref e)
    {
      if (ret) {
        return;
      }
      if (   key == regname
          && (e->is_bv_sort() || e->is_bool_sort())) {
        sz = e->is_bool_sort() ? 1 : e->get_sort()->get_size();
        ret = true;
      }
      if (!ret && expr_get_size_for_regname(e, regname, sz)) {
        ret = true;;
      }
    };
    if (!ret && expr_get_size_for_regname(this->get_cond(), regname, sz)) {
      ret = true;
    }
    if (!ret) {
      this->get_to_state().state_visit_exprs(f);
    }
    return ret;
  }

  size_t get_collapse_cost(void) const
  {
    size_t size = expr_size(this->get_cond());
    std::function<void (const string&, expr_ref)> f = [&size](const string& key, expr_ref e) -> void
    {
      if (size < expr_size(e)) {
        size = expr_size(e);
      }
    };

    get_to_state().state_visit_exprs(f);
    return size;
  }

  bool edge_is_indirect_branch() const
  {
    return this->get_to_state().has_expr_substr(LLVM_STATE_INDIR_TARGET_KEY_ID) != "";
  }

  expr_ref edge_get_indir_target() const
  {
    string k;
    if ((k = this->get_to_state().has_expr_substr(LLVM_STATE_INDIR_TARGET_KEY_ID)) != "") {
      return this->get_to_state().get_expr(k/*, this->get_to_state()*/);
    } else NOT_REACHED();
  }
  bool operator==(edge_with_cond const& other) const
  {
    return    this->edge<T_PC>::operator==(other)
           && this->m_pred == other.m_pred
           && state::states_equal(this->m_to_state, other.m_to_state)
           && this->m_assumes == other.m_assumes;
    ;
  }

  static string read_edge_with_cond_using_start_state(istream &in, string const& first_line, string const &prefix/*, state const &start_state*/, context* ctx, T_PC& p1, T_PC& p2, expr_ref& cond, state& state_to, unordered_set<expr_ref>& assumes);

private:
  static unordered_set<expr_ref> read_assumes_from_stream(istream& in, string const& prefix, context* ctx);

private:
  expr_ref const m_pred;
  state const m_to_state;
  unordered_set<expr_ref> const m_assumes;

  mutable expr_ref m_simplified_pred;
  mutable state m_simplified_to_state;
  mutable unordered_set<expr_ref> m_simplified_assumes;
};

}

namespace std
{
using namespace eqspace;
template<typename T_PC>
struct hash<edge_with_cond<T_PC>>
{
  std::size_t operator()(edge_with_cond<T_PC> const& te) const
  {
    size_t seed = 0;
    myhash_combine<size_t>(seed, hash<edge<pc>>()(te));
    if (te.get_cond()) {
      myhash_combine<size_t>(seed, hash<expr_ref>()(te.get_cond()));
    }
    myhash_combine<size_t>(seed, te.get_assumes().size());
    return seed;
  }
};

}
