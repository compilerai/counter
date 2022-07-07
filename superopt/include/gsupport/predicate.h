#pragma once

#include "expr/expr.h"
#include "expr/expr_simplify.h"
#include "expr/expr_utils.h"
//#include "expr/local_sprel_expr_guesses.h"
#include "expr/pc.h"

#include "gsupport/precond.h"

#include "graph/avail_exprs.h"

namespace std {

template<>
struct hash<eqspace::predicate>
{
  std::size_t operator()(eqspace::predicate const &e) const;
};

}

namespace eqspace
{

class tfg;
class tfg_llvm_t;
class tfg_asm_t;
class sprel_map_pair_t;
//class memlabel_assertions_t;

class predicate;
using predicate_ref = shared_ptr<predicate const>;

class predicate
{
public:
  predicate(predicate const &other) : m_precond(other.m_precond), m_lhs_expr(other.m_lhs_expr), m_rhs_expr(other.m_rhs_expr), m_comment(other.m_comment)
  {
    stats_num_predicate_constructions++;
    m_ctx = m_lhs_expr->get_context();
  }

  ~predicate()
  {
    if (m_is_managed) {
      this->get_predicate_manager()->deregister_object(*this);
    }
    stats_num_predicate_destructions++;
  }

  bool operator==(predicate const &other) const;

  static predicate_ref false_predicate(context *ctx);
  static predicate_ref true_predicate(context *ctx);
  static list<guarded_pred_t> lhs_set_sort(list<guarded_pred_t> const &lhs_set, expr_ref const& pred_src, expr_ref const& pred_dst);


  //predicate_ref predicate_add_guard_expr(expr_ref const& guard_expr) const;
  predicate_ref visit_exprs(std::function<expr_ref (const string &, expr_ref const &)> f) const;

  void visit_exprs(std::function<void (const string &, expr_ref const &)> f) const
  {
    f("lhs_expr", m_lhs_expr);
    f("rhs_expr", m_rhs_expr);
    m_precond.visit_exprs(f);
  }

  bool contains_array_constant() const
  {
    /*if (   expr_contains_array_constant(m_lhs_expr)
        || expr_contains_array_constant(m_rhs_expr)) {
      return true;
    }*/
    bool ret = false;
    std::function<void (const string &, expr_ref const &)> f = [&ret](string const &fieldname, expr_ref const &e)
    {
      ret = ret || expr_contains_array_constant(e);
    };
    this->visit_exprs(f);
    return ret;
  }

  /*bool contains_empty_memlabel_in_select_or_store_op() const
  {
    bool ret = false;
    std::function<void (const string &, expr_ref const &)> f = [&ret](string const &l, expr_ref const &e)
    {
      ret = ret || expr_contains_empty_memlabel_in_select_or_store_op(e);
    };
    this->visit_exprs(f);
    return ret;
  }*/

  bool operator<(const predicate& that) const
  {
    // NOTE: keep this consistent with definition of `operator=='
    return make_tuple(m_lhs_expr, m_rhs_expr, m_precond, m_comment) < make_tuple(that.m_lhs_expr, that.m_rhs_expr, that.m_precond, that.m_comment);
  }

  expr_ref get_effective_precond(/*tfg const &t*/) const
  {
    return compute_effective_precond(m_precond, m_lhs_expr->get_context()/*t*/);
  }

  predicate_ref set_lhs_and_rhs_exprs(expr_ref const &lhs, expr_ref const& rhs) const;
  //predicate_ref set_edge_guard(edge_guard_t const &eguard) const;
  //predicate_ref flatten_edge_guard_using_conjunction(tfg const& t) const;
  predicate_ref fold_guard_using_implication(expr_ref const& eg_expr) const;

  //local_sprel_expr_guesses_t const &get_local_sprel_expr_assumes() const
  //{
  //  return m_precond.get_local_sprel_expr_assumes();
  //}

  //predicate_ref set_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &lsprel_guesses) const;
  /*bool contains_conflicting_local_sprel_expr_guess(pair<local_id_t, expr_ref> const &local_sprel_expr_guess) const
  {
    return m_local_sprel_expr_assumes.contains_conflicting_local_sprel_expr_guess(local_sprel_expr_guess);
  }

  bool remove_local_sprel_expr_guess_if_it_exists(pair<local_id_t, expr_ref> const &local_sprel_expr_guess) const
  {
    return m_local_sprel_expr_assumes.remove_local_sprel_expr_guess_if_it_exists(local_sprel_expr_guess);
  }*/

  //predicate subtract_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &guesses) const
  //{
  //  predicate ret = *this;
  //  ret.m_precond.subtract_local_sprel_expr_assumes(guesses);
  //  return ret;
  //}

  predicate_ref simplify() const
  {
    context *ctx = this->get_context();
    std::function<expr_ref (const string&, expr_ref const &)> f = [ctx](string const &fieldname, expr_ref const &e)
    {
      expr_ref ret = ctx->expr_do_simplify(e);
      return ret;
    };
    return this->visit_exprs(f);
  }

  predicate_ref pred_substitute(/*state const &start_state, */state const &to_state) const
  {
    //cout << __func__ << " " << __LINE__ << ": to_state =\n" << to_state.to_string() << endl;
    std::function<expr_ref (const string &, expr_ref const &)> f = [/*&start_state, */&to_state](string const &l, expr_ref const &e)
    {
      return expr_substitute_using_state(e/*, start_state*/, to_state);
    };
    return this->visit_exprs(f);
  }

  bool is_guess() const
  {
    if (string_has_prefix(m_comment->get_str(), GUESS_PREFIX)) {
      return true;
    }
    return false;
  }
  //bool is_precond() const
  //{
  //  if (string_has_prefix(m_comment->get_str(), PRECOND_PREFIX)) {
  //    return true;
  //  }
  //  return false;
  //}

  //predicate_ref add_local_sprel_expr_assumes_as_guards(local_sprel_expr_guesses_t const &local_sprel_expr_assumes)
  //{
  //  predicate ret = *this;
  //  ret.m_precond.add_local_sprel_expr_assumes_as_guards(local_sprel_expr_assumes);
  //  return mk_predicate_ref(ret);
  //}

  string pred_to_string(string const &prefix, size_t padding_width = 20) const
  {
    stringstream ss;
    if (m_precond.precond_is_trivial()) {
      ss << prefix << pad_string(expr_string(m_lhs_expr),padding_width) << "  ==  " << expr_string(m_rhs_expr) << endl;
    }
    return ss.str();
  }

  string predicate_get_query_type_from_comment() const;

  void predicate_xml_print(ostream& os, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, map<expr_id_t, pair<expr_ref, expr_ref>> const& mlasserts_submap, std::function<void (expr_ref&)> src_rename_expr_fn, std::function<void (expr_ref&)> dst_rename_expr_fn, context::xml_output_format_t xml_output_format, string const& prefix, bool negate) const;

  void predicate_get_asm_annotation(ostream& os) const;

  string to_string(bool full = false) const
  {
    stringstream ss;
    ss << " comment=" << get_comment();
    if(full) {
      ss << ", precond = " << m_precond.precond_to_string();
      ss << ", lhs_expr = " << m_ctx->expr_to_string(m_lhs_expr);
      ss << ", rhs_expr = " << m_ctx->expr_to_string(m_rhs_expr);
    }
    return ss.str();
  }

  string to_string_final() const
  {
    stringstream ss;
    expr_ref precond_expr = m_precond.precond_get_expr_for_lsprels(m_ctx);
    /*expr_ref sprel_guard = m_local_sprel_expr_assumes.get_expr();
    if (!sprel_guard) {
      sprel_guard = expr_true(ctx);
    }
    ASSERT(m_guard.is_true());*/
    if (!expr_simplify::is_expr_equal_syntactic(precond_expr, expr_true(m_ctx))) {
      ss << expr_string(precond_expr) << " => ";
    }
    ss << expr_string(expr_eq(m_lhs_expr, m_rhs_expr));
    return ss.str();
  }

  void predicate_to_stream(ostream& ss, bool use_orig_ids = false) const;

  string to_string_for_eq(bool use_orig_ids = false) const
  {
    stringstream ss;
    predicate_to_stream(ss, use_orig_ids);
    return ss.str();
  }

  //bool predicate_represents_ismemlabel(void) const;
  bool predicate_relates_memlabel_addresses(void) const;
  bool predicate_is_trivial() const {
    expr_ref lhs_simpl = m_ctx->expr_do_simplify(m_lhs_expr);
    expr_ref rhs_simpl = m_ctx->expr_do_simplify(m_rhs_expr);
    if (lhs_simpl == rhs_simpl) {
      return true;
    }
    /*if (   is_expr_equal_syntactic(m_lhs_expr, expr_true(m_lhs_expr->get_context()))
        && is_expr_equal_syntactic(m_guard.edge_guard_get_expr(), m_rhs_expr)) {
      return true;
    }
    if (   is_expr_equal_syntactic(m_rhs_expr, expr_true(m_rhs_expr->get_context()))
        && is_expr_equal_syntactic(m_guard.edge_guard_get_expr(), m_lhs_expr)) {
      return true;
    }*/
    return false;
  }

  context* get_context() const         { return m_ctx; }
  precond_t const &get_precond() const { return m_precond; }
  //edge_guard_t get_edge_guard() const  { return m_precond.precond_get_guard(); }
  expr_ref const &get_lhs_expr() const { return m_lhs_expr; }
  expr_ref const &get_rhs_expr() const { return m_rhs_expr; }
  string const& get_comment() const    { return m_comment->get_str(); }

  expr_ref predicate_get_expr(/*tfg const& t, */bool simplified = true) const;

  predicate_ref pred_eliminate_constructs_that_the_solver_cannot_handle(context *ctx) const
  {
    std::function<expr_ref (const string&, expr_ref const &)> f = [ctx](string const &fieldname, expr_ref const &e)
    {
      return ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(e);
    };
    return this->visit_exprs(f);
  }

  predicate_ref pred_simplify_using_sprel_pair_and_memlabel_map(sprel_map_pair_t const &sprel_map_pair, graph_memlabel_map_t const &memlabel_map) const
  {
    std::function<expr_ref (const string&, expr_ref const &)> f = [this,&sprel_map_pair, &memlabel_map](string const &fieldname, expr_ref const &e)
    {
      return m_ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(e, sprel_map_pair, memlabel_map);
    };
    return this->visit_exprs(f);
  }
  predicate_ref pred_substitute_using_submap(context *ctx, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, set<pair<expr::operation_kind, int>> ops_to_avoid_descending = {}) const
  {
    std::function<expr_ref (const string&, expr_ref const &)> f = [ctx, &submap, &ops_to_avoid_descending](string const &fieldname, expr_ref const &e)
    {
      return ctx->expr_substitute(e, submap, ops_to_avoid_descending);
    };
    return this->visit_exprs(f);
  }
  predicate_ref pred_substitute_using_var_to_expr_map(map<string, expr_ref> const &var2expr) const
  {
    context *ctx = this->get_context();
    std::function<expr_ref (const string&, expr_ref const &)> f = [ctx, &var2expr](string const &fieldname, expr_ref const &e)
    {
      return ctx->expr_do_simplify(ctx->expr_substitute_using_var_to_expr_map(e, var2expr));
    };
    return this->visit_exprs(f);
  }

  bool predicate_is_ub_assume() const
  {
    return m_comment->get_str().find(UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX) == 0;
  }

  bool predicate_is_ub_memaccess_langtype_assume() const
  {
    return m_comment->get_str().find(UNDEF_BEHAVIOUR_ASSUME_IS_MEMACCESS_LANGTYPE) == 0;
  }

  bool predicate_is_ub_langaligned_assume() const
  {
    return m_comment->get_str().find(UNDEF_BEHAVIOUR_ASSUME_ALIGN_ISLANGALIGNED) == 0;
  }

  bool predicate_evaluates_to_constant() const
  {
    if (!m_precond.precond_is_trivial()) {
      return false;
    }
    expr_ref e = m_ctx->expr_do_simplify(m_ctx->mk_eq(m_lhs_expr, m_rhs_expr));
    return e->is_const();
  }

  bool predicate_evaluates_to_false() const
  { 
    if (!m_precond.precond_is_trivial()) {
      return false;
    }
    expr_ref e = m_ctx->expr_do_simplify(m_ctx->mk_eq(m_lhs_expr, m_rhs_expr));
    return e->is_const() && !e->get_bool_value();
  }

  predicate_ref predicate_canonicalized() const;

  //pair<bool,predicate_ref> pred_try_unioning_edge_guards(predicate_ref const& other) const;

  //bool predicate_eval_on_counter_example(set<memlabel_ref> const& relevant_memlabels, bool &eval_result, counter_example_t &counter_example) const;
  //bool predicate_try_adjust_CE_to_disprove_pred(counter_example_t &counter_example) const;
  //bool predicate_try_adjust_CE_to_prove_pred(counter_example_t &counter_example) const;

  static expr_ref compute_effective_precond(precond_t const &precond, context* ctx/*tfg const &t*/)
  {
    return precond.precond_get_expr(ctx/*, true*/);
  }

  static bool pred_compare_ids(predicate_ref const &a, predicate_ref const &b)
  {
    if (a->get_lhs_expr()->get_id() != b->get_lhs_expr()->get_id()) {
      return a->get_lhs_expr()->get_id() < b->get_lhs_expr()->get_id();
    }
    if (a->get_rhs_expr()->get_id() != b->get_rhs_expr()->get_id()) {
      return a->get_rhs_expr()->get_id() < b->get_rhs_expr()->get_id();
    }
    if (precond_t::precond_compare_ids(a->get_precond(), b->get_precond())) {
      return true;
    }
    return false;
  }

  static string op_kind_to_assume_comment(expr::operation_kind op_kind)
  {
    switch (op_kind) {
      case expr::OP_IS_MEMACCESS_LANGTYPE:
        return UNDEF_BEHAVIOUR_ASSUME_IS_MEMACCESS_LANGTYPE;
      case expr::OP_ISLANGALIGNED:
        return UNDEF_BEHAVIOUR_ASSUME_ALIGN_ISLANGALIGNED;
      case expr::OP_ISSHIFTCOUNT:
        return UNDEF_BEHAVIOUR_ASSUME_ISSHIFTCOUNT;
      case expr::OP_ISGEPOFFSET:
        return UNDEF_BEHAVIOUR_ASSUME_ISGEPOFFSET;
      case expr::OP_ISINDEXFORSIZE:
        return UNDEF_BEHAVIOUR_ASSUME_ISINDEXFORSIZE;
      case expr::OP_REGION_AGREES_WITH_MEMLABEL:
        return UNDEF_BEHAVIOUR_ASSUME_REGION_AGREES_WITH_MEMLABEL;
      case expr::OP_BELONGS_TO_SAME_REGION:
        return UNDEF_BEHAVIOUR_ASSUME_BELONGS_TO_SAME_REGION;
      default:
        return UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "some";
    }
    NOT_REACHED();
  }

  static predicate_ref ub_assume_predicate_from_expr(expr_ref const& e, string const& comment_suffix = "");

  static void predicate_from_stream(istream &in/*, state const& start_state*/, context* ctx, predicate_ref &pred);
  static unordered_set<predicate_ref> predicate_set_from_stream(istream &in/*, state const& start_state*/, context* ctx, string const& prefix);
  static void predicate_set_to_stream(ostream &os, string const& prefix, unordered_set<predicate_ref> const& preds);

  bool contains_axpred() const;

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }

  static manager<predicate> *get_predicate_manager();

  //friend predicate_ref mk_predicate_ref(predicate const &pred);
  //friend predicate_ref mk_predicate_ref(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string& comment);
  //friend predicate_ref mk_predicate_ref(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string_ref& comment);

  static predicate_ref mk_predicate_ref(expr_ref const& e, string const& comment);
  static predicate_ref mk_predicate_ref(predicate const &s);
  static predicate_ref mk_predicate_ref(istream& is, context* ctx);
  static predicate_ref mk_predicate_ref(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string& comment);
  static predicate_ref mk_predicate_ref(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string_ref& comment);
  //static void predicate_set_union(unordered_set<shared_ptr<predicate const>> &dst, unordered_set<shared_ptr<predicate const>> const& src);

  template<typename T_PC, typename T_E>
  static void guarded_predicate_set_union(list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> &dst, list<pair<graph_edge_composition_ref<T_PC, T_E>, shared_ptr<predicate const>>> const& src);

  //template<typename T_PC, typename T_E>
  //static unordered_set<shared_ptr<predicate const>> gen_predicate_set_from_ec_preds(list<pair<graph_edge_composition_ref<T_PC, T_E>, shared_ptr<predicate const>>> const& ec_preds);

private:

  predicate_ref pred_rename_symbols_using_memlabel_assertions_and_simplify(map<expr_id_t, pair<expr_ref, expr_ref>> const& submap) const;

  predicate(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string_ref& comment) :
    m_precond(precond), m_lhs_expr(e_lhs), m_rhs_expr(e_rhs), m_comment(comment)
  {
    stats_num_predicate_constructions++;
    m_ctx = m_lhs_expr->get_context();
  }
  predicate(istream &is, context* ctx);

  predicate_ref pred_do_heuristic_canonicalization() const;

  template<typename T_PC, typename T_E>
  static void populate_map_using_guarded_preds(map<shared_ptr<predicate const>, graph_edge_composition_ref<T_PC,T_E>>& m, list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> const& ls);
  template<typename T_PC, typename T_E>
  static list<pair<graph_edge_composition_ref<T_PC,T_E>, shared_ptr<predicate const>>> get_guarded_preds_from_map(map<shared_ptr<predicate const>, graph_edge_composition_ref<T_PC,T_E>> const& m);

private:

  context *m_ctx;
  precond_t m_precond;
  expr_ref m_lhs_expr;
  expr_ref m_rhs_expr;
  string_ref m_comment;

  bool m_is_managed = false;
};

using itfg_pathcond_pred_ls_t = list<pair<expr_ref, predicate>>;

bool guarded_lhs_set_includes_nonstack_mem_equality(list<guarded_pred_t> const &lhs_set, /*dshared_ptr<memlabel_assertions_t> const& mlasserts,*/ relevant_memlabels_t const& relevant_memlabels, set<pair<expr_ref, expr_ref>> &input_mem_exprs);
//memlabel_t lhs_set_mem_equality_memlabels(list<guarded_pred_t> const &lhs_set, map<expr_id_t, expr_ref> &input_mem_exprs);
list<predicate_ref> lhs_set_substitute_using_submap(list<predicate_ref> const &lhs, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, context *ctx);
list<guarded_pred_t> lhs_set_eliminate_constructs_that_the_solver_cannot_handle(list<guarded_pred_t> const &lhs_set, context *ctx);
expr_ref guarded_predicate_set_and(list<guarded_pred_t> const &preds, expr_ref const &in, precond_t const& precond, tfg const &t);
//expr_ref pred_avail_exprs_and(pred_avail_exprs_t const &pred_avail_exprs, expr_ref const &in, tfg const &t, tfg_suffixpath_t const &cg_src_suffixpath);

list<guarded_pred_t> expr_get_div_by_zero_cond(expr_ref const& e);
list<guarded_pred_t> expr_get_memlabel_heap_access_cond(expr_ref const& e, graph_memlabel_map_t const& memlabel_map);
list<guarded_pred_t> expr_is_not_memlabel_access_cond(expr_ref const& e, graph_memlabel_map_t const& memlabel_map);
expr_ref expr_assign_mlvars_to_memlabels(expr_ref const &e, map<pc, pair<mlvarname_t, mlvarname_t>> &fcall_mlvars, map<pc, map<expr_ref, mlvarname_t>> &expr_mlvars, string const &graph_name, long &mlvarnum, pc const &p);
list<guarded_pred_t> sprel_map_pair_get_predicates_for_non_var_locs(sprel_map_pair_t const& sprel_map_pair, context *ctx);
predicate_ref get_predicate_from_guarded_pred(guarded_pred_t const& src_pred);
list<guarded_pred_t> guarded_predicate_set_eliminate_axpreds(list<guarded_pred_t> const& guarded_pred_set);
list<guarded_pred_t> convert_exprs_to_ec_preds(unordered_set<expr_ref> const& exprs);
list<shared_ptr<predicate const>> get_uapprox_predicate_list_from_guarded_preds_and_graph_ec(list<guarded_pred_t> const& lhs_set, graph_edge_composition_ref<pc, tfg_edge> const& eg);
//predicate_ref get_predicate_from_guarded_pred_by_conjuncting_guard(guarded_pred_t const& src_pred);

/*
template <typename T_PC, typename T_PRED>
class predicate_acceptance
{
public:
  predicate_acceptance()
  {
  }

  bool is_predicate_acceptable(const T_PC& p, const T_PRED& pred, string& reason)
  {
    if(p.is_start())
    {
      reason = "ignore-at-start";
      return false;
    }
    return true;
  }
};
*/

//set<pair<expr_ref, pair<expr_ref, expr_ref>>> unordered_set<predicate_ref>o_expr_tuple_set(set<predicate> const &pset);

}
