#pragma once

#include "gsupport/pc.h"
#include "expr/state.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "support/log.h"
#include <list>
#include <map>
#include <stack>
//#include "expr/sprel_map.h"
#include "support/types.h"
#include "support/serpar_composition.h"
//#include "rewrite/transmap.h"
class transmap_t;

//#include "rewrite/symbol_map.h"
#include "rewrite/nextpc_function_name_map.h"
#include "graph/te_comment.h"
#include "gsupport/edge_with_cond.h"
//#include "rewrite/locals_info.h"
//#include "rewrite/nextpc_map.h"
struct nextpc_map_t;
struct nextpc_function_name_map_t;


namespace eqspace {

using namespace std;

class tfg;

class tfg_node : public node<pc>
{
public:
  tfg_node(pc p/*, const state& st*/) : node<pc>(p)//, m_state(st)
  {
  }

  /*tfg_node add_function_arguments(expr_vector const &function_arguments) const
  {
    tfg_node ret(get_pc(), m_state.add_function_arguments(function_arguments));
    return ret;
  }*/

  //const state& get_state() const { return m_state; }

  virtual string to_string() const;

  virtual ~tfg_node() = default;

private:
  //eq::state m_state;
};

class itfg_edge : public edge_with_cond<pc>
{
public:

  virtual ~itfg_edge();

  string to_string_for_eq() const
  {
    string ret;
    string const prefix = "=itfg_edge";
    ret += this->edge_with_cond<pc>::to_string_for_eq(prefix);
    ret += prefix + ".Comment\n";
    if (m_comment.get_string() != "") {
      ret += m_comment.get_string() + "\n";
    }
    return ret;
  }

  //shared_ptr<tfg_node> get_from() const { return m_from; }
  //shared_ptr<tfg_node> get_to() const { return m_to; }

  //const eq::state& get_to_state() const override { return m_to_state; }
  //eq::state& get_to_state() override { return m_to_state; }

  //expr_ref get_cond() const override { return m_pred; }
  //void set_cond(expr_ref pred) { m_pred = pred; }

  //bool get_is_indir_exit() const { return m_is_indir_exit; }
  //expr_ref get_indir_tgt() const { return m_indir_tgt; }
  //bool is_indir_exit() const { return get_is_indir_exit(); }
  //void set_indir_tgt(expr_ref e) { m_indir_tgt = e; }
  //void itfg_edge_truncate_register_sizes_using_regset(regset_t const &regset);
  //virtual string to_string(state const *start_state = NULL) const override;
  string to_string_with_comment() const;
  te_comment_t const& get_comment() const { return m_comment; }
  //virtual string to_string_for_eq() const;
  //string to_string_concise() const {
  //  //return m_from->get_pc().to_string() + "=>" + m_to->get_pc().to_string();
  //  return this->get_from_pc().to_string() + "=>" + this->get_to_pc().to_string();
  //}
  //expr_ref get_function_call_nextpc() const;
  //memlabel_t get_function_call_memlabel_writeable(graph_memlabel_map_t const &memlabel_map) const;
  //bool is_fcall_edge(nextpc_id_t &fcall_nextpc_id) const;

  //void rename_to_pc(struct nextpc_map_t const &nextpc_map, long cur_index);

  //void simplify(consts_struct_t const &cs);

  //void get_mem_slots(list<tuple<memlabel_t, expr_ref, unsigned, bool>>&) const;
  //void get_mem_masks(list<tuple<memlabel_t, expr_ref, unsigned>>&) const;
  //void get_mem_masks(list<tuple<memlabel_t, expr_ref, unsigned>> &mem_masks, vector<memlabel_t> const &relevant_memlabels) const;
  //void get_reg_slots(set<string>&) const;
  //void get_fcall_retregs(set<string>&) const;
  //list<state::mem_access> get_all_mem_accesses() const;
  //vector<expr_ref> const get_all_function_calls() const;

  //void substitute_variables_at_input(state const &start_state, map<expr_id_t, pair<expr_ref, expr_ref> > const &submap, context *ctx);
  //void substitute_variables_at_output(transmap_t const *tmap, state const &start_state);

  //itfg_edge_ref convert_jmp_nextpcs_to_callret(consts_struct_t const &cs/*, map<nextpc_id_t, pair<size_t, callee_summary_t>> const &nextpc_args_map*/, int r_esp, int r_eax, tfg const *tfg);
  //bool is_nop_edge(/*state const &from_state*/) const;
  size_t get_collapse_cost(void) const;
  bool operator==(itfg_edge const& other) const
  {
    return    this->edge_with_cond::operator==(other)
           && this->m_comment == other.m_comment
    ;
  }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }

  //expr_ref get_div_by_zero_cond() const;
  //expr_ref get_ismemlabel_heap_access_cond() const;

  static manager<itfg_edge> *get_itfg_edge_manager();
  //friend shared_ptr<itfg_edge const> mk_itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition, state const &start_state/*, bool is_indir_exit, expr_ref indir_tgt*/, string const& comment);
  friend shared_ptr<itfg_edge const> mk_itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition/*, state const &start_state*/, unordered_set<expr_ref> const& assumes/*, bool is_indir_exit, expr_ref indir_tgt*/, te_comment_t const& te_comment);
  //friend shared_ptr<itfg_edge const> mk_itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition);

  shared_ptr<itfg_edge const> itfg_edge_visit_exprs(std::function<expr_ref (string const&, expr_ref)> f) const;

  //bool itfg_edge_get_size_for_regname(string const &regname, size_t &sz) const; //returns true if regname was found on this edge, and returns size in SZ if so

protected:
  //virtual void add_edge_in_series(shared_ptr<tfg_edge> e, consts_struct_t const &cs);
  //virtual void add_edge_in_parallel(shared_ptr<tfg_edge> e, consts_struct_t const &cs);

private:
public:
  itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition/*, state const &start_state*/, unordered_set<expr_ref> const& assumes/*, bool is_indir_exit, expr_ref indir_tgt*/, te_comment_t const& comment);
  //itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition, state const &start_state/*, bool is_indir_exit, expr_ref indir_tgt*/, string comment = "");
  //itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition/*, bool is_indir_exit, expr_ref indir_tgt*/);

  //shared_ptr<itfg_edge const> itfg_edge_update_state(std::function<void (state&)> update_state_fn) const;

private:
  //shared_ptr<tfg_node const> m_from;
  //shared_ptr<tfg_node const> m_to;
  //expr_ref m_pred;
  //eq::state m_to_state;
  //bool m_is_indir_exit;
  //expr_ref m_indir_tgt;
  te_comment_t m_comment;
  bool m_is_managed;
};

//typedef serpar_composition_node_t<shared_ptr<tfg_edge>> tfg_edge_composition_t;
//typedef graph_edge_composition_t<pc, tfg_node, tfg_edge> const tfg_edge_composition_t;

using itfg_edge_ref = shared_ptr<itfg_edge const>;

//itfg_edge_ref
//mk_itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition, state const &start_state/*, bool is_indir_exit, expr_ref indir_tgt*/, string const& comment = "");

itfg_edge_ref
mk_itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition/*, state const &start_state*/, unordered_set<expr_ref> const& assumes/*, bool is_indir_exit, expr_ref indir_tgt*/, te_comment_t const& te_comment);


//itfg_edge_ref
//mk_itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition);

}


namespace std
{
template<>
struct hash<itfg_edge>
{
  std::size_t operator()(itfg_edge const &e) const
  {
    return myhash_string(e.to_string_for_eq().c_str());
  }
};

template<>
struct hash<itfg_edge_ref>
{
  std::size_t operator()(itfg_edge_ref const &e) const
  {
    return (size_t)e.get();
  }
};

}

#undef DEBUG_TYPE
