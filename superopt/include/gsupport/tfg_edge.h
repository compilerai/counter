#pragma once

#include "support/log.h"
#include "support/types.h"
#include "support/serpar_composition.h"
#include "support/dshared_ptr.h"

#include "expr/state.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/pc.h"

#include "gsupport/graph_ec.h"
#include "gsupport/te_comment.h"
#include "gsupport/itfg_edge.h"
#include "gsupport/itfg_ec.h"

//#include "tfg/nextpc_function_name_map.h"

struct nextpc_map_t;
struct nextpc_function_name_map_t;
class transmap_t;

namespace eqspace {

using namespace std;

using predicate_ref = shared_ptr<predicate const>;

class tfg_edge;
typedef graph_edge_composition_t<pc, tfg_edge> const tfg_edge_composition_t;
typedef shared_ptr<tfg_edge_composition_t> tfg_edge_composition_ref;

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
  //tfg_edge(shared_ptr<tfg_edge_composition_t> const& e, tfg const& t);
  tfg_edge(pc const& from_pc, pc const& to_pc, expr_ref const& cond, state const& to_state, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment)
  : edge_with_cond<pc>(from_pc, to_pc, cond, to_state, assumes),
    m_te_comment(te_comment)
  { }

public:
  tfg_edge(pc const& from_pc, pc const& to_pc) : edge_with_cond<pc>(from_pc, to_pc, nullptr, state(), {}), m_te_comment(false, -1, "") { NOT_REACHED(); }

  virtual ~tfg_edge();

  shared_ptr<tfg_edge const> tfg_edge_shared_from_this() const
  {
    shared_ptr<tfg_edge const> ret = dynamic_pointer_cast<tfg_edge const>(this->shared_from_this());
    ASSERT(ret);
    return ret;
  }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<tfg_edge> *get_tfg_edge_manager();

  //void visit_exprs_const(function<void (string const&, expr_ref const&)> f) const;
  shared_ptr<tfg_edge const> visit_keys(std::function<string (string const&, expr_ref)> update_keys_fn, bool opt/*, state const& start_state*/) const;
  shared_ptr<tfg_edge const> visit_exprs(function<expr_ref (expr_ref const&)> f) const;
  shared_ptr<tfg_edge const> visit_exprs(function<expr_ref (string const&, expr_ref const&)> f) const;

  string to_string_concise() const
  {
    if (!this->is_empty()) {
      return this->get_from_pc().to_string() + "=>" + this->get_to_pc().to_string();
    } else {
      return "<empty>";
    }
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
  list<guarded_pred_t> tfg_edge_get_pathcond_pred_ls_for_div_by_zero(tfg const& t) const;
  list<guarded_pred_t> tfg_edge_get_pathcond_pred_ls_for_region_agrees_with_memlabel_heap(tfg const& t) const;
  list<guarded_pred_t> tfg_edge_get_pathcond_pred_ls_for_ub_trigger_cond(tfg const& t) const;
  list<guarded_pred_t> tfg_edge_get_pathcond_pred_ls_for_is_not_memlabel(tfg const& t, graph_memlabel_map_t const& memlabel_map) const;
  shared_ptr<tfg_edge const> tfg_edge_update_state(std::function<void (pc const&, state&)> update_state_fn) const;
  shared_ptr<tfg_edge const> tfg_edge_update_edgecond(std::function<expr_ref (pc const&, expr_ref const&)> update_expr_fn) const;
  shared_ptr<tfg_edge const> tfg_edge_update_assumes(std::function<expr_ref (pc const&, expr_ref const&)> update_expr_fn) const;
  shared_ptr<tfg_edge const> tfg_edge_set_assumes(unordered_set<expr_ref> const& new_assumes) const;
  shared_ptr<tfg_edge const> tfg_edge_set_state(state const& new_st) const;

  //virtual void edge_visit_exprs_const(std::function<void (pc const&, string const&, expr_ref)> f) const override;

  shared_ptr<tfg_edge const> substitute_variables_at_input(/*state const &start_state,*/ map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, context *ctx) const;

  //bool tfg_edge_is_atomic() const { return m_ec->get_type() == itfg_ec_node_t::atom; }
  static tfg_edge_ref empty();
  static tfg_edge_ref empty(pc const& p);
  virtual bool is_empty() const override { return !(this->get_cond()); }

  //static tfg_edge_ref nop_edge(pc const& p, context* ctx);

  set<allocsite_t> tfg_edge_get_alloca_ptrs() const;
  set<allocsite_t> tfg_edge_get_allocas() const;
  set<allocsite_t> tfg_edge_get_deallocs() const;
  list<alloc_dealloc_t> tfg_edge_get_alloc_dealloc_ops() const;
  expr_ref tfg_edge_get_alloca_expr() const;
  expr_ref tfg_edge_get_dealloc_expr() const;

  bool tfg_edge_is_post_sp_version_assignment_edge() const { return this->get_from_pc().is_sp_version_ext(); };
  bool tfg_edge_is_sp_version_assignment_edge() const { return this->get_to_pc().is_sp_version_ext(); };
  bool tfg_edge_is_alloca_edge() const  { return tfg_edge_get_alloca_expr() != nullptr; }
  bool tfg_edge_is_dealloc_edge() const { return tfg_edge_get_dealloc_expr() != nullptr; }

  static shared_ptr<tfg_edge const> graph_edge_from_stream(istream &in, string const& first_line, string const &prefix, context* ctx);

  static string tfg_edge_set_to_string(set<tfg_edge_ref> const& tfg_edge_set);

  friend tfg_edge_ref mk_tfg_edge(itfg_edge_ref const& ie);
  //friend tfg_edge_ref mk_tfg_edge(shared_ptr<tfg_edge_composition_t> const& e, tfg const& t);
  friend tfg_edge_ref mk_tfg_edge(pc const& from_pc, pc const& to_pc, expr_ref const& cond, state const& to_state, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment);

private:
  list<guarded_pred_t> tfg_edge_get_pathcond_pred_ls_for_unsafe_cond(tfg const& t, std::function<list<guarded_pred_t> (expr_ref const&)> unsafe_cond) const;
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
