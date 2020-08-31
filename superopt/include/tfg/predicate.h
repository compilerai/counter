#pragma once

#include <set>
#include "gsupport/pc.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "tfg/precond.h"
#include "tfg/avail_exprs.h"
#include "expr/local_sprel_expr_guesses.h"


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
class sprel_map_pair_t;
class memlabel_assertions_t;

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

  bool operator==(predicate const &other) const
  {
    //return    (m_precond.equals_mod_lsprels(other.m_precond))
    return    (m_precond.equals(other.m_precond))
           && (m_lhs_expr == other.m_lhs_expr)
           && (m_rhs_expr == other.m_rhs_expr);
  }

  static predicate_ref false_predicate(context *ctx);
  static list<predicate_ref> determinized_pred_set(unordered_set<predicate_ref> const &pred_set);

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
    //do not consider local_sprel_exprs as we do not expect two identical predicates with different local_sprel_expr_sets in the same set.
    // NOTE: `set' uses < for determining uniqueness: Two elements are equivalent if !(a<b) && !(b<a)).
    // No two elements can be equivalent in a `set'
    // If precond is not used, then predicates with same lhs and rhs but different precond (e.g. edge guard) cannot exist together.
    // NOTE: keep this consistent with definition of `operator=='
    if (m_lhs_expr < that.m_lhs_expr) {
      return true;
    } else if (that.m_lhs_expr < m_lhs_expr) {
      return false;
    }
    if (m_rhs_expr < that.m_rhs_expr) {
      return true;
    } else if (that.m_rhs_expr < m_rhs_expr) {
      return false;
    }
    return m_precond < that.m_precond;
    // m_comment does not matter
  }

  expr_ref get_effective_precond(tfg const &t) const
  {
    return compute_effective_precond(m_precond/*, m_local_sprel_expr_assumes*/, t);
  }

  predicate_ref set_lhs_and_rhs_exprs(expr_ref const &lhs, expr_ref const& rhs) const;
  predicate_ref set_edge_guard(edge_guard_t const &eguard) const;
  predicate_ref flatten_edge_guard_using_conjunction(tfg const& t) const;

  local_sprel_expr_guesses_t const &get_local_sprel_expr_assumes() const
  {
    return m_precond.get_local_sprel_expr_assumes();
  }

  predicate_ref set_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &lsprel_guesses) const;
  /*bool contains_conflicting_local_sprel_expr_guess(pair<local_id_t, expr_ref> const &local_sprel_expr_guess) const
  {
    return m_local_sprel_expr_assumes.contains_conflicting_local_sprel_expr_guess(local_sprel_expr_guess);
  }

  bool remove_local_sprel_expr_guess_if_it_exists(pair<local_id_t, expr_ref> const &local_sprel_expr_guess) const
  {
    return m_local_sprel_expr_assumes.remove_local_sprel_expr_guess_if_it_exists(local_sprel_expr_guess);
  }*/

  predicate subtract_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &guesses) const
  {
    predicate ret = *this;
    ret.m_precond.subtract_local_sprel_expr_assumes(guesses);
    return ret;
  }

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
  bool is_precond() const
  {
    if (string_has_prefix(m_comment->get_str(), PRECOND_PREFIX)) {
      return true;
    }
    return false;
  }

  predicate_ref add_local_sprel_expr_assumes_as_guards(local_sprel_expr_guesses_t const &local_sprel_expr_assumes)
  {
    predicate ret = *this;
    ret.m_precond.add_local_sprel_expr_assumes_as_guards(local_sprel_expr_assumes);
    return mk_predicate_ref(ret);
  }

  string pred_to_string(string const &prefix, size_t padding_width = 20) const
  {
    stringstream ss;
    if (m_precond.precond_is_trivial()) {
      ss << prefix << pad_string(expr_string(m_lhs_expr),padding_width) << "  ==  " << expr_string(m_rhs_expr) << endl;
    }
    return ss.str();
  }

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
    if (!is_expr_equal_syntactic(precond_expr, expr_true(m_ctx))) {
      ss << expr_string(precond_expr) << " => ";
    }
    ss << expr_string(expr_eq(m_lhs_expr, m_rhs_expr));
    return ss.str();
  }

  void predicate_to_stream(ostream& ss, bool use_orig_ids = false) const
  {
    ss << "=Comment\n" << m_comment->get_str() << endl;
    ss << m_precond.precond_to_string_for_eq(use_orig_ids);
    ss << "=LhsExpr\n";
    ss << m_ctx->expr_to_string_table(m_lhs_expr, use_orig_ids) << endl;
    ss << "=RhsExpr\n";
    ss << m_ctx->expr_to_string_table(m_rhs_expr, use_orig_ids) << endl;
  }

  string to_string_for_eq(bool use_orig_ids = false) const
  {
    stringstream ss;
    predicate_to_stream(ss, use_orig_ids);
    return ss.str();
  }

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
  edge_guard_t get_edge_guard() const  { return m_precond.precond_get_guard(); }
  expr_ref const &get_lhs_expr() const { return m_lhs_expr; }
  expr_ref const &get_rhs_expr() const { return m_rhs_expr; }
  string const& get_comment() const    { return m_comment->get_str(); }

  expr_ref predicate_get_expr(tfg const& t, bool simplified) const;

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
  predicate_ref pred_substitute_using_submap(context *ctx, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap) const
  {
    std::function<expr_ref (const string&, expr_ref const &)> f = [ctx, &submap](string const &fieldname, expr_ref const &e)
    {
      return ctx->expr_substitute(e, submap);
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

  pair<bool,predicate_ref> pred_try_unioning_edge_guards(predicate_ref const& other) const;

  bool predicate_eval_on_counter_example(set<memlabel_ref> const& relevant_memlabels, bool &eval_result, counter_example_t &counter_example) const;
  bool predicate_try_adjust_CE_to_disprove_pred(counter_example_t &counter_example) const;
  bool predicate_try_adjust_CE_to_prove_pred(counter_example_t &counter_example) const;

  static expr_ref compute_effective_precond(precond_t const &precond, tfg const &t)
  {
    return precond.precond_get_expr(t, true);
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
      default:
        return UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "some";
    }
    NOT_REACHED();
  }

  static predicate_ref ub_assume_predicate_from_expr(expr_ref const& e);

  static string predicate_from_stream(istream &in/*, state const& start_state*/, context* ctx, predicate_ref &pred);
  static unordered_set<predicate_ref> predicate_set_from_stream(istream &in/*, state const& start_state*/, context* ctx, string const& prefix);
  static void predicate_set_to_stream(ostream &os, string const& prefix, unordered_set<predicate_ref> const& preds);

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

  static predicate_ref mk_predicate_ref(predicate const &s);
  static predicate_ref mk_predicate_ref(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string& comment);
  static predicate_ref mk_predicate_ref(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string_ref& comment);
  static void predicate_set_union(unordered_set<shared_ptr<predicate const>> &dst, unordered_set<shared_ptr<predicate const>> const& src);


private:

  predicate(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string_ref& comment) :
    m_precond(precond), m_lhs_expr(e_lhs), m_rhs_expr(e_rhs), m_comment(comment)
  {
    stats_num_predicate_constructions++;
    m_ctx = m_lhs_expr->get_context();
  }

  predicate_ref pred_do_heuristic_canonicalization() const;

private:

  context *m_ctx;
  precond_t m_precond;
  expr_ref m_lhs_expr;
  expr_ref m_rhs_expr;
  string_ref m_comment;

  bool m_is_managed = false;
};

using itfg_pathcond_pred_ls_t = list<pair<expr_ref, predicate>>;


template <typename T_PC, typename T_PRED>
class predicate_map
{
public:
  predicate_map() {}

  bool add_pred(const T_PC& p , const shared_ptr<T_PRED const>& pred)
  {
    if (check_duplicate(p, pred)) {
      unordered_set<shared_ptr<T_PRED const>> const& preds_at_p = m_pred_map.at(p);
      typename unordered_set<shared_ptr<T_PRED const>>::const_iterator iter = preds_at_p.find(pred);
      ASSERT(iter != preds_at_p.end());
      return false;
    }
    ASSERT(m_pred_map[p].insert(pred).second);
    //ASSERT(!is_empty(p));
    return true;
  }

  void predicate_map_transfer_preds(T_PC const &from_pc, T_PC const &to_pc)
  {
    if (m_pred_map.count(from_pc)) {
      unordered_set<shared_ptr<T_PRED const>> ps = m_pred_map.at(from_pc);
      m_pred_map.erase(from_pc);
      m_pred_map[to_pc] = ps;
    }
  }

  bool check_duplicate(const T_PC& p , const shared_ptr<T_PRED const>& pred)
  {
    return m_pred_map[p].count(pred) > 0;
  }

  bool is_empty() const
  {
    for(const auto& it : m_pred_map)
    {
      if(it.second.size() > 0)
        return false;
    }
    return true;
  }

  bool is_empty(const T_PC& p) const
  {
    //cout << __func__ << " " << __LINE__ << ": m_pred_map.size() = " << m_pred_map.size() << endl;
    //cout << __func__ << " " << __LINE__ << ": m_pred_map.count(p) = " << m_pred_map.count(p) << endl;
    if (m_pred_map.count(p)) {
      return m_pred_map.at(p).size() == 0;
    } else {
      return true;
    }
  }

  unordered_set<shared_ptr<T_PRED const>>& get_predicate_set(const T_PC& p)
  {
    return m_pred_map[p];
  }

  const unordered_set<shared_ptr<T_PRED const>>& get_predicate_set(const T_PC& p) const
  {
    //assert(!is_empty(p));
    static unordered_set<shared_ptr<T_PRED const>> empty_set;
    if (m_pred_map.count(p) == 0) {
      return empty_set;
    } else {
      return m_pred_map.at(p);
    }
  }

  unordered_set<shared_ptr<T_PRED const>>& get_predicate_set_ref(const T_PC& p)
  {
    //assert(!is_empty(p));
    if (m_pred_map.count(p) == 0) {
      m_pred_map[p] = unordered_set<shared_ptr<T_PRED const>>();
    }
    return m_pred_map[p];
  }

  void simplify_preds(const T_PC& p, consts_struct_t const *cs)
  {
    if (m_pred_map.count(p) == 0) {
      return;
    }
    unordered_set<shared_ptr<T_PRED const>> new_preds;
    for (const auto &pred : determinized_pred_set(m_pred_map.at(p))) {
      shared_ptr<T_PRED const> new_pred = pred.simplify();
      new_preds.insert(new_pred);
    }
    m_pred_map[p] = new_preds;
  }

  const map<T_PC, unordered_set<shared_ptr<T_PRED const>>>& get_map() const
  {
    return m_pred_map;
  }

  string to_string(bool detail = false, bool only_proven = true) const
  {
    stringstream ss;
    stringstream ss_summ;
    for(const auto& pc_preds : m_pred_map)
    {
      unsigned id = 0;
      unsigned proven = 0;
      unsigned not_proven = 0;
      ss << "NODE: " << pc_preds.first;
      ss << "\n";
      for(const auto& pred : pc_preds.second) {
        ss << "predicate#" << id << ": " << pred.to_string(true) << endl;
        ++id;
      }
      ss_summ << "NODE: " << pc_preds.first;
    }
    if(detail)
      return ss.str() + "\n" + ss_summ.str();
    else
      return ss_summ.str();
  }

  string to_string_for_eq(T_PC const &p) const
  {
    stringstream ss;
    size_t id;

    const auto &pc_preds = m_pred_map.at(p);

    id = 0;
    for(const auto& pred : pc_preds) {
      ss << "==Predicate#" << id << ": " << pred->to_string(false) << endl;
      ++id;
      context *ctx = pred->get_context();
      ss << ctx->expr_to_string_table(pred.get_expr());
    }
    return ss.str();
  }

  string to_string_for_eq() const;

  void visit_exprs(std::set<T_PC> const &nodes_changed, std::function<expr_ref (T_PC const &, const string&, expr_ref const &)> f) {
    for (auto p : nodes_changed) {
      //T_PC p = entry.first;
      if (m_pred_map.count(p) == 0) {
        continue;
      }
      unordered_set<shared_ptr<T_PRED const>> const &preds = m_pred_map.at(p);
      unordered_set<shared_ptr<T_PRED const>> new_preds;
      for (shared_ptr<T_PRED const> const& dpred : predicate::determinized_pred_set(preds)) {
        std::function<expr_ref (const string&, expr_ref const &)> pf = [&f, &p](string const &fieldname, expr_ref const &expr)
        {
          return f(p, G_TFG_PRED_IDENTIFIER, expr);
        };

        shared_ptr<T_PRED const> pred = dpred->visit_exprs(pf);
        new_preds.insert(pred);
      }
      m_pred_map[p] = new_preds;
    }
  }

  void visit_exprs(std::set<T_PC> const &nodes_changed, std::function<void (T_PC const &, const string&, expr_ref const &)> f) const {
    for (auto p : nodes_changed) {
      //T_PC p = entry.first;
      if (m_pred_map.count(p) == 0) {
        continue;
      }
      unordered_set<shared_ptr<T_PRED const>> const &preds = m_pred_map.at(p);
      for (typename unordered_set<shared_ptr<T_PRED const>>::const_iterator iter = preds.begin(); iter != preds.end(); iter++) {
        std::function<void (const string&, expr_ref const &)> pf = [&f, &p](string const &fieldname, expr_ref const &expr)
        {
          return f(p, G_TFG_PRED_IDENTIFIER, expr);
        };
        (*iter)->visit_exprs(pf);
      }
    }
  }

  void remove_preds_for_pc(T_PC const &p) {
    m_pred_map.erase(p);
  }

  void remove_preds_with_comments(T_PC const &p, std::set<string> const &comments)
  {
    if (m_pred_map.count(p) == 0) {
      return;
    }
    unordered_set<shared_ptr<T_PRED const>> const &preds = m_pred_map.at(p);
    unordered_set<shared_ptr<T_PRED const>> new_preds;
    for (auto pred : preds) {
      string pcomment = pred->get_comment();
      bool ignore = false;
      for (auto c : comments) {
        if (pcomment.find(c) != string::npos) {
          ignore = true;
          break;
        }
      }
      if (!ignore) {
        new_preds.insert(pred);
      } else {
        cout << __func__ << " " << __LINE__ << ": " << p.to_string() << ": removing " << pcomment << endl;
      }
    }
    m_pred_map[p] = new_preds;
  }

private:
  map<T_PC, unordered_set<shared_ptr<T_PRED const>>> m_pred_map;
};

bool lhs_set_includes_nonstack_mem_equality(list<predicate_ref> const &lhs_set, memlabel_assertions_t const& mlasserts, map<expr_id_t, expr_ref> &input_mem_exprs);
list<predicate_ref> lhs_set_substitute_using_submap(list<predicate_ref> const &lhs, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, context *ctx);
list<predicate_ref> lhs_set_eliminate_constructs_that_the_solver_cannot_handle(list<predicate_ref> const &lhs_set, context *ctx);
expr_ref predicate_set_and(list<predicate_ref> const &preds, expr_ref const &in, precond_t const& precond, tfg const &t);
expr_ref pred_avail_exprs_and(pred_avail_exprs_t const &pred_avail_exprs, expr_ref const &in, tfg const &t, tfg_suffixpath_t const &cg_src_suffixpath);

unordered_set<predicate_ref> expr_get_div_by_zero_cond(expr_ref const& e);
unordered_set<predicate_ref> expr_get_memlabel_heap_access_cond(expr_ref const& e, graph_memlabel_map_t const& memlabel_map);
unordered_set<predicate_ref> expr_is_not_memlabel_access_cond(expr_ref const& e, graph_memlabel_map_t const& memlabel_map);
expr_ref expr_assign_mlvars_to_memlabels(expr_ref const &e, map<pc, pair<mlvarname_t, mlvarname_t>> &fcall_mlvars, map<pc, map<expr_ref, mlvarname_t>> &expr_mlvars, string const &graph_name, long &mlvarnum, pc const &p);

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
