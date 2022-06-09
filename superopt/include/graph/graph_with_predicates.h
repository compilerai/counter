#pragma once

#include <map>
#include <list>
#include <string>
#include <cassert>
#include <set>
#include <memory>

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/memlabel_map.h"
#include "expr/graph_local.h"
#include "expr/graph_locals_map.h"
#include "expr/graph_arg_regs.h"
#include "expr/pc.h"

#include "gsupport/edge_with_cond.h"
#include "gsupport/edge_wf_cond.h"
#include "gsupport/failcond.h"

#include "graph/graph.h"
#include "graph/graph_with_var_versions.h"
//#include "graph/binary_search_on_preds.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_predicates : public graph_with_var_versions<T_PC, T_N, T_E>
{
public:
  typedef list<shared_ptr<T_E const>> T_PATH;

  graph_with_predicates(string const &name, string const& fname, context* ctx) : graph_with_var_versions<T_PC, T_N, T_E>(ctx), m_name(mk_string_ref(name)), m_function_name(mk_string_ref(fname)), /*m_ctx(ctx),*/ m_cs(ctx->get_consts_struct())
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_predicates(graph_with_predicates const &other) : graph_with_var_versions<T_PC, T_N, T_E>(other),
                                                              m_memlabel_maps(other.m_memlabel_maps),
                                                              m_name(other.m_name),
                                                              m_function_name(other.m_function_name),
//                                                              m_ctx(other.m_ctx),
                                                              m_cs(other.m_cs),
                                                              m_symbol_map(other.m_symbol_map),
                                                              m_locals_map(other.m_locals_map),
                                                              m_global_assumes(other.m_global_assumes),
                                                              m_return_regs(other.m_return_regs),
                                                              m_arg_regs(other.m_arg_regs)
                                                              //m_start_state(other.m_start_state)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  void graph_ssa_copy(graph_with_predicates const& other) 
  {
    graph_with_var_versions<T_PC, T_N, T_E>::graph_ssa_copy(other);
//    ASSERT(m_ctx);
    m_memlabel_maps = other.m_memlabel_maps;
    m_symbol_map = other.m_symbol_map;
    m_locals_map = other.m_locals_map;
    m_global_assumes = other.m_global_assumes;
    m_return_regs = other.m_return_regs;
    m_arg_regs = other.m_arg_regs;
  }

  graph_with_predicates(istream& in, string const& name, std::function<dshared_ptr<T_E const> (istream& is, string const&, string const&, context*)> read_edge_fn, context* ctx);
  virtual ~graph_with_predicates() = default;

  string_ref const &get_name() const { return m_name; }
  string_ref const &get_function_name() const { return m_function_name; }
  void graph_set_name(string const& name) { m_name = mk_string_ref(name); }
//  context* get_context() const { return m_ctx; }
  consts_struct_t const &get_consts_struct() const { return m_cs; }

//  start_state_t const &get_start_state() const { return m_start_state; }
//  start_state_t       &get_start_state()       { return m_start_state; }
//
//  void set_start_state(start_state_t const& st) { m_start_state = st; }
//  void set_start_state(state const& st) { 
//    start_state_t start_st(st);
//    m_start_state = start_st; 
//  }

  void graph_return_registers_visit_exprs(std::function<void (T_PC const &, const string&, expr_ref)> f) const
  {
    list<dshared_ptr<T_N>> nodes;
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
  void graph_global_assumes_visit_exprs(function<void (T_PC const &, const string&, expr_ref)> f) const
  {
    for (const auto &assume : m_global_assumes) {
      function<void(string const&, expr_ref const&)> fw = [&f](string const& k, expr_ref const& e)
        {
          f(T_PC::start(), k, e);
        };
      assume->visit_exprs(fw);
    }
  }

  void graph_global_assumes_visit_exprs(function<expr_ref (T_PC const &, const string&, expr_ref)> f)
  {
    unordered_set<shared_ptr<T_PRED const>> new_global_assumes;
    for (const auto &assume : m_global_assumes) {
      function<expr_ref(string const&, expr_ref const&)> fw = [&f](string const& k, expr_ref const& e)
        {
          return f(T_PC::start(), k, e);
        };
      auto new_assume = assume->visit_exprs(fw);
      new_global_assumes.insert(new_assume);
    }
    this->set_global_assumes(new_global_assumes);
  }

  void graph_visit_exprs_and_keys(std::function<expr_ref (T_PC const &, const string&, expr_ref)> f, std::function<string (T_PC const &, const string&, expr_ref)> f_keys, bool opt = true)
  {
    autostop_timer func_timer(__func__);
    list<dshared_ptr<T_E const>> edges, new_edges;
    this->get_edges(edges);

    for (auto const& e : edges) {
      T_PC const &p = e->get_from_pc();
      function<expr_ref (string const&, expr_ref const&)> ff = [&f, &p](string const& s, expr_ref e) {
        return f(p, s, e);
      };
      auto new_e = e->visit_exprs(ff);
      T_PC const &to_p = new_e->get_to_pc();
      function<string (string const&, expr_ref const&)> ff_keys = [&f_keys, &to_p](string const& s, expr_ref e) {
        return f_keys(to_p, s, e);
      };
      auto e_after_keys_renaming = new_e->visit_keys(ff_keys, opt);

      new_edges.push_back(e_after_keys_renaming);
    }
    this->set_edges(new_edges);
    //set<T_PC> all_pcs = this->get_all_pcs();
  }
  void graph_visit_exprs(std::function<expr_ref (T_PC const &, const string&, expr_ref)> f, bool opt = true)
  {
    autostop_timer func_timer(__func__);
    list<dshared_ptr<T_E const>> edges, new_edges;
    this->get_edges(edges);

    for (auto const& e : edges) {
      T_PC const &p = e->get_from_pc();
      function<expr_ref (string const&, expr_ref const&)> ff = [&f, &p](string const& s, expr_ref e) {
        return f(p, s, e);
      };
      auto new_e = e->visit_exprs(ff);
      new_edges.push_back(new_e);
    }
    this->set_edges(new_edges);
    graph_global_assumes_visit_exprs(f);
  }

  void graph_visit_exprs_const(std::function<void (T_PC const &, const string&, expr_ref)> f, bool opt = true) const
  {
    autostop_timer func_timer(string(__func__) + ".const");
    ASSERT(opt);
    list<dshared_ptr<T_E const>> edges;
    this->get_edges(edges);
    for (auto e : edges) {
      e->visit_exprs_const(f);
    }
    graph_return_registers_visit_exprs(f);
    graph_global_assumes_visit_exprs(f);
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

  void substitute_return_regs_at_exit_pc(T_PC const& p, map<expr_id_t, pair<expr_ref, expr_ref>> const& submap);


  bool pc_has_return_regs(T_PC const &p) const { return m_return_regs.count(p) != 0; }

  void eliminate_return_reg_at_exit(string return_reg_name, T_PC const &exit_pc) // used in llvm2tfg
  {
    m_return_regs.at(exit_pc).erase(mk_string_ref(return_reg_name));
  }

  // in CG we return union of src and dst memlabel maps; hence cannot return by const ref
  virtual graph_memlabel_map_t get_memlabel_map(call_context_ref const& cc) const;
  graph_memlabel_map_t&        get_memlabel_map_ref(call_context_ref const& cc);
  void clear_memlabel_maps();
  void memlabel_maps_coarsen_readonly_memlabels_to_rodata_memlabel();
  void memlabel_map_to_stream(ostream& os) const;
  /*static */string read_memlabel_maps(istream& in, string const& line/*, graph_memlabel_map_t &memlabel_map*/);
  static string read_memlabel_map(istream& in, string const& line, graph_memlabel_map_t &memlabel_map);

  //string memlabel_map_to_string_for_eq() const { return memlabel_map_to_string(this->get_memlabel_map()); }
  //void set_memlabel_map(graph_memlabel_map_t const &m) { NOT_IMPLEMENTED(); /*m_memlabel_map = m;*/ }

  graph_arg_regs_t const& get_argument_regs() const { return m_arg_regs; }
  void set_argument_regs(graph_arg_regs_t const& args) { m_arg_regs = args; }

  void set_symbol_map(graph_symbol_map_t const &symbol_map) { m_symbol_map = symbol_map; }
  graph_symbol_map_t const &get_symbol_map() const { return m_symbol_map; }
  graph_symbol_map_t&       get_symbol_map()       { return m_symbol_map; }

  void set_locals_map(graph_locals_map_t const &locals_map) { m_locals_map = locals_map; }
  graph_locals_map_t const &get_locals_map() const { return m_locals_map; }

  unordered_set<shared_ptr<T_PRED const>> const& get_global_assumes() const { return m_global_assumes; }
  virtual unordered_set<shared_ptr<T_PRED const>> get_global_assumes_at_pc(T_PC const& p) const;

  //virtual string read_graph_edge(istream& in, string const& first_line, string const& prefix, context* ctx, shared_ptr<T_E const>& e) const = 0;

  graph_edge_composition_ref<T_PC,T_E> graph_edge_composition_from_stream(istream& in/*,  string const& line*/, string const& prefix/*, context* ctx, graph_edge_composition_ref<T_PC,T_E>& ec*/) const;

  virtual src_dst_cg_path_tuple_t src_dst_cg_path_tuple_from_stream(istream& is, string const& prefix) const = 0;
  failcond_t failcond_from_stream(istream& is, string const& prefix) const;

  pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>> guarded_pred_from_stream(istream& is, string const& prefix, context* ctx) const;

  void add_global_assume(shared_ptr<T_PRED const> const& pred) const { m_global_assumes.insert(pred); }

protected:
  void set_global_assumes(unordered_set<shared_ptr<T_PRED const>> const& global_assumes) { m_global_assumes = global_assumes; }

  virtual void graph_to_stream(ostream& ss, string const& prefix="") const override;
  virtual edge_wf_cond_t const& graph_edge_get_well_formedness_conditions(dshared_ptr<T_E const> const& e) const = 0;

private:

  static void extract_align_assumes(unordered_set<shared_ptr<T_PRED const>> const &theos, unordered_set<shared_ptr<T_PRED const>> &alignment_assumes);
  static void read_in_out(istream& in, context* ctx, map<string_ref, graph_arg_t> &args, map<T_PC, map<string_ref, expr_ref>> &rets);
  static void read_out_values(istream& in, context* ctx, map<string_ref, expr_ref> &rets);
  string read_symbol_map_nodes_and_edges(istream& in, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn);
  //string read_graph_edge(istream& in, string const& first_line, string const& prefix);
  string read_assumes(istream& in, string line, unordered_set<shared_ptr<T_PRED const>> &theos) const;
  static set<T_PC> get_to_pcs_for_edges(list<dshared_ptr<T_E const>> const &edges);
  expr_ref setup_debug_probes(expr_ref e);

protected:

private:

  map<call_context_ref, graph_memlabel_map_t> m_memlabel_maps;

  string_ref m_name;
  string_ref m_function_name;
//  context* m_ctx;
  consts_struct_t const &m_cs;

  graph_symbol_map_t m_symbol_map;
  graph_locals_map_t m_locals_map;

  mutable unordered_set<shared_ptr<T_PRED const>> m_global_assumes;

  map<T_PC, map<string_ref, expr_ref>> m_return_regs;
  //map<string_ref, expr_ref> m_arg_regs;
  graph_arg_regs_t m_arg_regs;

//  start_state_t m_start_state;
};

}
