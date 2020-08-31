#pragma once

#include "gsupport/pc.h"
#include "expr/state.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "support/log.h"
#include <list>
#include <map>
#include <stack>
#include "support/types.h"
#include "support/serpar_composition.h"
#include "gsupport/graph_ec.h"
#include "graph/te_comment.h"
class transmap_t;

#include "rewrite/nextpc_function_name_map.h"
struct nextpc_map_t;
struct nextpc_function_name_map_t;

#include "gsupport/itfg_edge.h"
#include "gsupport/itfg_ec.h"

namespace eqspace {

using namespace std;

using predicate_ref = shared_ptr<predicate const>;

class tfg_edge;
typedef graph_edge_composition_t<pc, tfg_edge> const tfg_edge_composition_t;

using tfg_edge_ref = shared_ptr<tfg_edge const>;

tfg_edge_ref mk_tfg_edge(itfg_edge_ref const& ie);
tfg_edge_ref mk_tfg_edge(shared_ptr<tfg_edge_composition_t> const& e, tfg const& t);
tfg_edge_ref mk_tfg_edge(pc const& from_pc, pc const& to_pc, expr_ref const& cond, state const& to_state, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment);

class tfg_edge : public edge_with_cond<pc>
{
private:
  te_comment_t const m_te_comment;
  bool m_is_managed = false;

private:
  tfg_edge(itfg_edge_ref const& ie) : edge_with_cond<pc>(ie->get_from_pc(), ie->get_to_pc(), ie->get_cond(), ie->get_to_state(), ie->get_assumes()),
                                      m_te_comment(ie->get_comment())
  { }
  tfg_edge(shared_ptr<tfg_edge_composition_t> const& e, tfg const& t);
  tfg_edge(pc const& from_pc, pc const& to_pc, expr_ref const& cond, state const& to_state, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment)
  : edge_with_cond<pc>(from_pc, to_pc, cond, to_state, assumes),
    m_te_comment(te_comment)
  { }

public:

  virtual ~tfg_edge();

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<tfg_edge> *get_tfg_edge_manager();

  shared_ptr<tfg_edge const> visit_exprs(function<expr_ref (expr_ref const&)> f) const;
  shared_ptr<tfg_edge const> visit_exprs(function<expr_ref (string const&, expr_ref const&)> f, bool opt = true/*, state const& start_state = state()*/) const;

  string to_string_concise() const {
    return this->get_from_pc().to_string() + "=>" + this->get_to_pc().to_string();
  }
  shared_ptr<tfg_edge const> convert_jmp_nextpcs_to_callret(consts_struct_t const &cs, int r_esp, int r_eax, tfg const *tfg) const;
  te_comment_t const& get_te_comment() const { return m_te_comment; }
  string to_string_with_comment() const;
  string to_string_for_eq(string const& prefix) const;
  bool operator==(tfg_edge const& other) const
  {
    return    this->edge_with_cond::operator==(other)
           && this->m_te_comment == other.m_te_comment;
  }
  bool operator<(tfg_edge const& other) const = delete;
  static itfg_ec_ref
  create_itfg_edge_composition_from_tfg_edge_composition(shared_ptr<tfg_edge_composition_t> const& e, tfg const& t);
  unordered_set<predicate_ref> tfg_edge_get_pathcond_pred_ls_for_div_by_zero(tfg const& t) const;
  unordered_set<predicate_ref> tfg_edge_get_pathcond_pred_ls_for_ismemlabel_heap(tfg const& t) const;
  unordered_set<predicate_ref> tfg_edge_get_pathcond_pred_ls_for_is_not_memlabel(tfg const& t, graph_memlabel_map_t const& memlabel_map) const;
  shared_ptr<tfg_edge const> tfg_edge_update_state(std::function<void (pc const&, state&)> update_state_fn) const;
  shared_ptr<tfg_edge const> tfg_edge_update_edgecond(std::function<expr_ref (pc const&, expr_ref const&)> update_expr_fn) const;
  shared_ptr<tfg_edge const> tfg_edge_update_assumes(unordered_set<expr_ref> const& new_assumes) const;
  shared_ptr<tfg_edge const> tfg_edge_set_state(state const& new_st) const;

  //virtual void edge_visit_exprs_const(std::function<void (pc const&, string const&, expr_ref)> f) const override;

  shared_ptr<tfg_edge const> substitute_variables_at_input(state const &start_state, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, context *ctx) const;

  //bool tfg_edge_is_atomic() const { return m_ec->get_type() == itfg_ec_node_t::atom; }
  template<typename T_VAL, xfer_dir_t T_DIR>
  T_VAL xfer(T_VAL const& in, function<T_VAL (T_VAL const&, expr_ref const&, state const&, unordered_set<expr_ref> const&)> atom_f, function<T_VAL (T_VAL const&, T_VAL const&)> meet_f, bool simplified) const
  {
    expr_ref const& econd = simplified ? this->get_simplified_cond()     : this->get_cond();
    state const& to_state = simplified ? this->get_simplified_to_state() : this->get_to_state();
    unordered_set<expr_ref> const& assumes = simplified ? this->get_simplified_assumes() : this->get_assumes();

    ASSERTCHECK(econd, cout << _FNLN_ << ": [" << this << "]: " << (simplified ? "(simplified) " : "") << "edge = " << this->to_string_concise() << endl);
    T_VAL const& ret_val = atom_f(in, econd, to_state, assumes);
    return ret_val;
  }

  static tfg_edge_ref empty();
  static tfg_edge_ref empty(pc const& p);
  bool is_empty() const { return !(this->get_cond()); }

  static string graph_edge_from_stream(istream &in, string const& first_line, string const &prefix/*, state const &start_state*/, context* ctx, shared_ptr<tfg_edge const>& te);

  friend tfg_edge_ref mk_tfg_edge(itfg_edge_ref const& ie);
  friend tfg_edge_ref mk_tfg_edge(shared_ptr<tfg_edge_composition_t> const& e, tfg const& t);
  friend tfg_edge_ref mk_tfg_edge(pc const& from_pc, pc const& to_pc, expr_ref const& cond, state const& to_state, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment);

private:
  unordered_set<predicate_ref> tfg_edge_get_pathcond_pred_ls_for_unsafe_cond(tfg const& t, std::function<unordered_set<predicate_ref> (expr_ref const&)> unsafe_cond) const;
  //static serpar_composition_node_t<itfg_edge_ref>::type
  //convert_type(serpar_composition_node_t<tfg_edge>::type t)
  //{
  //  if (t == serpar_composition_node_t<edge_id_t<pc>>::atom) {
  //    return serpar_composition_node_t<itfg_edge_ref>::atom;
  //  } else if (t == serpar_composition_node_t<edge_id_t<pc>>::series) {
  //    return serpar_composition_node_t<itfg_edge_ref>::series;
  //  } else if (t == serpar_composition_node_t<edge_id_t<pc>>::parallel) {
  //    return serpar_composition_node_t<itfg_edge_ref>::parallel;
  //  } else NOT_REACHED();
  //}
};

}

namespace std
{
using namespace eqspace;
template<>
struct hash<tfg_edge>
{
  std::size_t operator()(tfg_edge const& te) const
  {
    size_t seed = 0;
    myhash_combine<size_t>(seed, hash<edge_with_cond<pc>>()(te));
    return seed;
  }
};

}
