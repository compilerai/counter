#pragma once

#include "gsupport/pc.h"
#include "graph/graph.h"
#include "support/utils.h"
#include "support/log.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
//#include "predicate.h"
#include "graph/binary_search_on_preds.h"
#include "expr/memlabel_map.h"
#include "gsupport/edge_with_cond.h"
#include "support/timers.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/graph_local.h"
#include "expr/graph_arg_regs.h"

#include <map>
#include <list>
#include <string>
#include <cassert>
#include <set>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_predicates : public graph<T_PC, T_N, T_E>
{
public:
  typedef list<shared_ptr<T_E const>> T_PATH;

  graph_with_predicates(string const &name, context* ctx) : m_name(mk_string_ref(name)), m_ctx(ctx), m_cs(ctx->get_consts_struct())
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_predicates(graph_with_predicates const &other) : graph<T_PC, T_N, T_E>(other),
                                                              m_memlabel_map(other.m_memlabel_map),
                                                              m_name(other.m_name),
                                                              m_ctx(other.m_ctx),
                                                              m_cs(other.m_cs),
                                                              m_symbol_map(other.m_symbol_map),
                                                              m_locals_map(other.m_locals_map),
                                                              m_assume_preds(other.m_assume_preds),
                                                              m_assert_preds(other.m_assert_preds),
                                                              m_global_assumes(other.m_global_assumes),
                                                              m_return_regs(other.m_return_regs),
                                                              m_arg_regs(other.m_arg_regs),
                                                              m_start_state(other.m_start_state)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_predicates(istream& in, string const& name, context* ctx);
  virtual ~graph_with_predicates() = default;

  string_ref const &get_name() const { return m_name; }
  context* get_context() const { return m_ctx; }
  consts_struct_t const &get_consts_struct() const { return m_cs; }

  bool add_assume_pred(T_PC const &p, shared_ptr<T_PRED const> const &pred)
  {
    /*if (pred.predicate_is_trivial()) { //do not enable this: causes important ub preds to get eliminated before they are canonicalized
      return false;
    }*/
    return m_assume_preds.add_pred(p, pred);
  }

  bool add_assert_pred(T_PC const &p, shared_ptr<T_PRED const> const &pred)
  {
    return m_assert_preds.add_pred(p, pred);
  }

  void add_assert_preds_at_pc(const T_PC& p, const unordered_set<shared_ptr<T_PRED const>>& preds)
  {
    for (const auto &pred : preds) {
      add_assert_pred(p, pred);
    }
  }

  void add_assume_preds_at_pc(const T_PC& p, const unordered_set<shared_ptr<T_PRED const>>& preds)
  {
    for (const auto &pred : preds) {
      add_assume_pred(p, pred);
    }
  }

  void add_assume_preds_at_all_pcs(unordered_set<shared_ptr<T_PRED const>> const &preds)
  {
    list<shared_ptr<T_N>> nodes;
    this->get_nodes(nodes);
    for (auto n : nodes) {
      this->add_assume_preds_at_pc(n->get_pc(), preds);
    }
  }

  unordered_set<shared_ptr<T_PRED const>> const &get_assume_preds(const T_PC& p) const
  {
    return m_assume_preds.get_predicate_set(p);
  }

  unordered_set<shared_ptr<T_PRED const>> const &get_assert_preds(const T_PC& p) const
  {
    return m_assert_preds.get_predicate_set(p);
  }

  void remove_assume_preds_with_comments(set<string> const &comments)
  {
    for (auto p : this->get_all_pcs()) {
      m_assume_preds.remove_preds_with_comments(p, comments);
    }
  }

  void graph_preds_visit_exprs(set<T_PC> const &nodes_changed, std::function<expr_ref (T_PC const &, const string&, expr_ref const &)> f)
  {
    m_assume_preds.visit_exprs(nodes_changed, f);
    m_assert_preds.visit_exprs(nodes_changed, f);
  }

  void graph_preds_visit_exprs(set<T_PC> const &nodes_changed, std::function<void (T_PC const &, const string&, expr_ref const &)> f) const
  {
    m_assume_preds.visit_exprs(nodes_changed, f);
    m_assert_preds.visit_exprs(nodes_changed, f);
  }

  void graph_remove_preds_for_pc(T_PC const &p)
  {
    m_assume_preds.remove_preds_for_pc(p);
    m_assert_preds.remove_preds_for_pc(p);
  }

  predicate_map<T_PC, T_PRED> const &get_assume_preds_map(void) const { return m_assume_preds; }
  predicate_map<T_PC, T_PRED> const &get_assert_preds_map(void) const { return m_assert_preds; }

  void set_assume_preds_map(predicate_map<T_PC, T_PRED> const &preds) { m_assume_preds = preds; }
  void set_assert_preds_map(predicate_map<T_PC, T_PRED> const &preds) { m_assert_preds = preds; }

  state const &get_start_state() const { return m_start_state; }
  state       &get_start_state()       { return m_start_state; }

  void set_start_state(state const& st) { m_start_state = st; }

  //void graph_add_key_to_start_state(string const &key_name, sort_ref const &key_sort)
  //{
  //  if (this->m_start_state.key_exists(key_name, this->m_start_state)) {
  //    expr_ref v = this->m_start_state.get_expr(key_name, this->m_start_state);
  //    ASSERT(v->get_sort() == key_sort);
  //    return;
  //  }
  //  expr_ref key_val = this->get_context()->mk_var(string(G_INPUT_KEYWORD ".") + key_name, key_sort);
  //  this->m_start_state.set_expr_in_map(key_name, key_val);
  //}

  void graph_return_registers_visit_exprs(std::function<void (T_PC const &, const string&, expr_ref)> f) const
  {
    list<shared_ptr<T_N>> nodes;
    this->get_nodes(nodes);
    for (auto n : nodes) {
      T_PC const &p = n->get_pc();
      if (!p.is_exit()) {
        continue;
      }
      if (m_return_regs.count(p) == 0) {
        continue;
      }
      for (const auto &pr : m_return_regs.at(p)) {
        expr_ref e = pr.second;
        f(p, G_TFG_OUTPUT_IDENTIFIER, e);
      }
    }
  }

  void graph_visit_exprs(std::function<expr_ref (T_PC const &, const string&, expr_ref)> f, bool opt = true)
  {
    autostop_timer func_timer(__func__);
    list<shared_ptr<T_E const>> edges, new_edges;
    this->get_edges(edges);

    for (auto const& e : edges) {
      T_PC const &p = e->get_from_pc();
      function<expr_ref (string const&, expr_ref const&)> ff = [&f, &p](string const& s, expr_ref e) {
        return f(p, s, e);
      };
      auto new_e = e->visit_exprs(ff, opt/*, this->get_start_state()*/);
      new_edges.push_back(new_e);
    }
    this->set_edges(new_edges);
    set<T_PC> all_pcs = this->get_all_pcs();
    graph_preds_visit_exprs(all_pcs, f);
  }

  void graph_visit_exprs_const(std::function<void (T_PC const &, const string&, expr_ref)> f, bool opt = true) const
  {
    autostop_timer func_timer(string(__func__) + ".const");
    ASSERT(opt);
    list<shared_ptr<T_E const>> edges;
    this->get_edges(edges);
    for (auto e : edges) {
      e->visit_exprs_const(f);
    }
    set<T_PC> all_pcs = this->get_all_pcs();
    graph_preds_visit_exprs(all_pcs, f);
    graph_return_registers_visit_exprs(f);
  }

  void graph_visit_exprs_const(function<void (string const&, expr_ref)> f, bool opt = true) const
  {
    function<void (T_PC const&, string const&, expr_ref)> ef =
      [&f](T_PC const& __unused, string const& s, expr_ref e)
      {
        f(s, e);
      };
    graph_visit_exprs_const(ef, opt);
  }

  map<string_ref, expr_ref> const &get_return_regs_at_pc(T_PC const &p) const
  {
    ASSERT(p.is_exit());
    if (m_return_regs.count(p) == 0) {
      cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << endl;
      cout << __func__ << " " << __LINE__ << ": m_return_regs.size() = " << m_return_regs.size() << endl;
    }
    return m_return_regs.at(p);
  }

  map<T_PC, map<string_ref, expr_ref>> const &get_return_regs() const { return m_return_regs; }
  void set_return_regs(map<T_PC, map<string_ref, expr_ref>> const &ret_regs) { m_return_regs = ret_regs; }
  void set_return_regs_at_pc(T_PC const &p, map<string_ref, expr_ref> const &ret_regs) { m_return_regs[p] = ret_regs; }
  bool pc_has_return_regs(T_PC const &p) const { return m_return_regs.count(p) != 0; }

  void eliminate_return_reg_at_exit(string return_reg_name, T_PC const &exit_pc) // used in llvm2tfg
  {
    m_return_regs.at(exit_pc).erase(mk_string_ref(return_reg_name));
  }

  void transfer_preds(T_PC const &from_pc, T_PC const &to_pc)
  {
		m_assume_preds.predicate_map_transfer_preds(from_pc, to_pc);
    m_assert_preds.predicate_map_transfer_preds(from_pc, to_pc);
  }

  // in CG we return union of src and dst memlabel maps; hence cannot return by const ref
  virtual graph_memlabel_map_t get_memlabel_map() const { return m_memlabel_map; }
  graph_memlabel_map_t&        get_memlabel_map_ref()   { return m_memlabel_map; }
  void set_memlabel_map(graph_memlabel_map_t const &m) { m_memlabel_map = m; }

  string memlabel_map_to_string_for_eq() const { return memlabel_map_to_string(this->get_memlabel_map()); }

  graph_arg_regs_t const& get_argument_regs() const { return m_arg_regs; }
  void set_argument_regs(graph_arg_regs_t const& args) { m_arg_regs = args; }

  void set_symbol_map(graph_symbol_map_t const &symbol_map) { m_symbol_map = symbol_map; }
  graph_symbol_map_t const &get_symbol_map() const { return m_symbol_map; }
  graph_symbol_map_t&       get_symbol_map()       { return m_symbol_map; }

  void set_locals_map(graph_locals_map_t const &locals_map) { m_locals_map = locals_map; }
  graph_locals_map_t const &get_locals_map() const { return m_locals_map; }

  unordered_set<shared_ptr<T_PRED const>> const& get_global_assumes() const { return m_global_assumes; }

  static string read_memlabel_map(istream& in, string line, graph_memlabel_map_t &memlabel_map);

protected:
  void add_global_assume(shared_ptr<T_PRED const> const& pred) { m_global_assumes.insert(pred); }
  void set_global_assumes(unordered_set<shared_ptr<T_PRED const>> const& global_assumes) { m_global_assumes = global_assumes; }

  virtual void graph_to_stream(ostream& ss) const override;

private:

  static void extract_align_assumes(unordered_set<shared_ptr<T_PRED const>> const &theos, unordered_set<shared_ptr<T_PRED const>> &alignment_assumes);
  static void read_in_out(istream& in, context* ctx, map<string_ref, expr_ref> &args, map<T_PC, map<string_ref, expr_ref>> &rets);
  static void read_out_values(istream& in, context* ctx, map<string_ref, expr_ref> &rets);
  string read_symbol_map_nodes_and_edges(istream& in/*, string line*/);
  string read_graph_edge(istream& in, string const& first_line, string const& prefix);
  string read_assumes(istream& in, string line, unordered_set<shared_ptr<T_PRED const>> &theos) const;
  static set<T_PC> get_to_pcs_for_edges(set<shared_ptr<T_E const>> const &edges);
  expr_ref setup_debug_probes(expr_ref e);

protected:

  graph_memlabel_map_t m_memlabel_map;

private:

  string_ref m_name;
  context* m_ctx;
  consts_struct_t const &m_cs;

  graph_symbol_map_t m_symbol_map;
  graph_locals_map_t m_locals_map;

  predicate_map<T_PC, T_PRED> m_assume_preds;
  predicate_map<T_PC, T_PRED> m_assert_preds;

  unordered_set<shared_ptr<T_PRED const>> m_global_assumes;

  map<T_PC, map<string_ref, expr_ref>> m_return_regs;
  //map<string_ref, expr_ref> m_arg_regs;
  graph_arg_regs_t m_arg_regs;

  state m_start_state;
};

}
