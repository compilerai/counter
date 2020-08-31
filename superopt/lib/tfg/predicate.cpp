#include "tfg/predicate.h"
#include "expr/expr_simplify.h"
#include "support/bv_const_expr.h"
#include "support/globals.h"
#include "tfg/tfg.h"
#include "tfg/predicate_set.h"

namespace std {

std::size_t
hash<eqspace::predicate>::operator()(eqspace::predicate const &e) const
{
  size_t seed = 0;
  myhash_combine<size_t>(seed, hash<eqspace::precond_t>()(e.get_precond()));
  myhash_combine<size_t>(seed, hash<eqspace::expr_ref>()(e.get_lhs_expr()));
  myhash_combine<size_t>(seed, hash<eqspace::expr_ref>()(e.get_rhs_expr()));
  //myhash_combine<size_t>(seed, hash<string>()(e.get_comment())); // comment not used in ==
  return seed;
}

}
namespace eqspace
{

predicate_ref
predicate::false_predicate(context *ctx)
{
  static predicate ret(precond_t(), expr_false(ctx), expr_true(ctx), mk_string_ref("false-predicate"));
  return mk_predicate_ref(ret);
}

predicate_ref
predicate::visit_exprs(std::function<expr_ref (const string &, expr_ref const &)> f) const
{
  predicate ret(m_precond, m_lhs_expr, m_rhs_expr, m_comment);
  ret.m_lhs_expr = f("lhs_expr", ret.m_lhs_expr);
  ret.m_rhs_expr = f("rhs_expr", ret.m_rhs_expr);

  ret.m_precond = m_precond;
  ret.m_precond.visit_exprs(f);
  return mk_predicate_ref(ret);
}

predicate_ref
predicate::set_lhs_and_rhs_exprs(expr_ref const &lhs, expr_ref const& rhs) const
{
  predicate ret = *this;
  ret.m_lhs_expr = lhs;
  ret.m_rhs_expr = rhs;
  return mk_predicate_ref(ret);
}

predicate_ref
predicate::set_edge_guard(edge_guard_t const &eguard) const
{
  predicate ret = *this;
  ret.m_precond.precond_set_guard(eguard);
  return mk_predicate_ref(ret);
}

predicate_ref
predicate::set_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &lsprel_guesses) const
{
  predicate ret = *this;
  ret.m_precond.set_local_sprel_expr_assumes(lsprel_guesses);
  return mk_predicate_ref(ret);
}

static
void get_src_dst_addsub_atoms(expr_vector& src_atoms, expr_vector& dst_atoms, expr_ref const& e)
{
  for (auto const& atom : get_arithmetic_addsub_atoms(e)) {
    if (is_dst_expr(atom))
      dst_atoms.push_back(atom);
    else
      src_atoms.push_back(atom);
  }
}

static
bool const_mul_heuristic_helper(expr_ref& lhs_expr, expr_ref& rhs_expr)
{
  // lhs is mul by const
  //ASSERT(lhs_expr->is_bv_sort());
  //ASSERT(rhs_expr->get_sort() == lhs_expr->get_sort());
  //ASSERT(lhs_expr->get_operation_kind() == expr::OP_BVMUL);
  //ASSERT(lhs_expr->get_args().at(0)->is_const() || lhs_expr->get_args().at(1)->is_const());

  expr_ref multiplicand;
  bv_const bv_multiplier;

  if (lhs_expr->get_args().at(0)->is_const()) {
    bv_multiplier = expr2bv_const(lhs_expr->get_args().at(0));
    multiplicand = lhs_expr->get_args().at(1);
  }
  else {
    multiplicand = lhs_expr->get_args().at(0);
    bv_multiplier = expr2bv_const(lhs_expr->get_args().at(1));
  }

  // simplification is only possible if multiplicative inverse can be calculated
  if (bv_isodd(bv_multiplier)) {
    context* ctx = lhs_expr->get_context();
    unsigned bvlen = lhs_expr->get_sort()->get_size();
    bv_const mod = bv_exp2(bvlen);

    bv_const mul_inv = bv_mulinv(bv_multiplier, mod);

    // in lhs_expr just keep the multiplicand
    lhs_expr = multiplicand;

    // rhs_expr = rhs_expr * mul_inv
    if (rhs_expr->is_const()) {
      bv_const bvc = expr2bv_const(rhs_expr);
      bv_const rhs_expr_const = bv_mod(mul_inv * bvc, mod);
      rhs_expr = bv_const2expr(rhs_expr_const, ctx, bvlen);
    } else {
      expr_ref mul_inv_expr = bv_const2expr(mul_inv, ctx, bvlen);
      rhs_expr = ctx->expr_do_simplify(ctx->mk_bvmul(mul_inv_expr, rhs_expr));
    }

    return true;
  }
  else return false;
}

static
bool const_mul_heuristic(expr_ref& lhs_expr, expr_ref& rhs_expr)
{
  // works only for BVs
  if (   !lhs_expr->is_bv_sort()
      || lhs_expr->get_sort() != rhs_expr->get_sort())
    return false;

  auto is_c_mul = [](expr_ref const& e) -> bool {
    return    e->get_operation_kind() == expr::OP_BVMUL
           && (e->get_args().at(0)->is_const() || e->get_args().at(1)->is_const());
  };

  // try with rhs first
  if (is_c_mul(rhs_expr)) {
    bool success = const_mul_heuristic_helper(rhs_expr, lhs_expr);
    if (success)
      return true;
  }
  if (is_c_mul(lhs_expr)) {
    bool success = const_mul_heuristic_helper(lhs_expr, rhs_expr);
    if (success)
      return true;
  }

  return false;
}

predicate_ref
predicate::pred_do_heuristic_canonicalization() const
{
  if (m_lhs_expr->is_bv_sort()) {
    ASSERT(m_rhs_expr->is_bv_sort());

    // heuristic 1: if either side has bvmul by const; try to remove the bvmul
    //              this works best if the other side is constant
    expr_ref new_lhs_expr = m_lhs_expr;
    expr_ref new_rhs_expr = m_rhs_expr;
    const_mul_heuristic(new_lhs_expr, new_rhs_expr);
    return this->set_lhs_and_rhs_exprs(new_lhs_expr, new_rhs_expr);
  } else {
    return mk_predicate_ref(*this);
  }
  NOT_REACHED();
}
// canonical form: dst exprs in rhs, everything else in lhs
predicate_ref
predicate::predicate_canonicalized() const
{
  if (   m_lhs_expr->is_array_sort()
      || m_lhs_expr->is_bool_sort()) {
    ASSERT(m_lhs_expr->get_sort() == m_rhs_expr->get_sort());
    // handle the easiest case
    if (   is_dst_expr(m_lhs_expr)
        && !is_dst_expr(m_rhs_expr)) {
      return this->set_lhs_and_rhs_exprs(m_rhs_expr, m_lhs_expr);
    }
    return mk_predicate_ref(*this);
  } else if (m_lhs_expr->is_bv_sort()) {
    // assumes linear structure i.e. a1 + a2 + a3 + ... = b1 + b2 + b3 + ..., where ai and bi can be src or dst exprs
    // we will try to move all src exprs to lhs and dst exprs to rhs

    //cout << __func__ << ':' << __LINE__ << ": input predicate: " << this->to_string(true) << endl;
    ASSERT(m_rhs_expr->is_bv_sort());

    unsigned bvlen = m_lhs_expr->get_sort()->get_size();
    expr_ref expr_zero = m_ctx->mk_zerobv(bvlen);

    expr_vector lhs_src_atoms, lhs_dst_atoms;
    expr_vector rhs_src_atoms, rhs_dst_atoms;

    get_src_dst_addsub_atoms(lhs_src_atoms, lhs_dst_atoms, m_lhs_expr);
    get_src_dst_addsub_atoms(rhs_src_atoms, rhs_dst_atoms, m_rhs_expr);

    expr_vector src_atoms = lhs_src_atoms;
    for (auto const& atom : rhs_src_atoms) { src_atoms.push_back(expr_bvneg(atom)); }

    expr_vector dst_atoms = rhs_dst_atoms;
    for (auto const& atom : lhs_dst_atoms) { dst_atoms.push_back(expr_bvneg(atom)); }

    expr_ref new_lhs_expr = src_atoms.size() ? arithmetic_addsub_atoms_to_expr(src_atoms) : expr_zero;
    expr_ref new_rhs_expr = dst_atoms.size() ? arithmetic_addsub_atoms_to_expr(dst_atoms) : expr_zero;

    new_lhs_expr = m_ctx->expr_do_simplify(new_lhs_expr);
    new_rhs_expr = m_ctx->expr_do_simplify(new_rhs_expr);

    return this->set_lhs_and_rhs_exprs(new_lhs_expr, new_rhs_expr)->pred_do_heuristic_canonicalization();
  } else {
    return mk_predicate_ref(*this);// not handling other cases
  }
  NOT_REACHED();
}

predicate_ref
predicate::flatten_edge_guard_using_conjunction(tfg const& t) const
{
  context *ctx = t.get_context();
  edge_guard_t eg = this->get_edge_guard();
  expr_ref eg_expr = eg.edge_guard_get_expr(t, true);
  predicate_ref ret = this->set_edge_guard(edge_guard_t());
  if (eg_expr == expr_true(ctx)) {
    return ret;
  }
  expr_ref new_e = expr_and(eg_expr, expr_eq(m_lhs_expr, m_rhs_expr));
  return ret->set_lhs_and_rhs_exprs(new_e, expr_true(ctx));
}

pair<bool,predicate_ref>
predicate::pred_try_unioning_edge_guards(predicate_ref const& other) const
{
  precond_t precond = m_precond;
  if (   m_lhs_expr == other->m_lhs_expr
      && m_rhs_expr == other->m_rhs_expr
      && precond.precond_try_unioning_edge_guards(other->m_precond)) {
    predicate ret = *this;
    ret.m_precond = precond;
    return make_pair(true, mk_predicate_ref(ret));
  }
  return make_pair(false, mk_predicate_ref(*this));
}

expr_ref
predicate::predicate_get_expr(tfg const& t, bool simplified) const
{
  context *ctx = t.get_context();
  expr_ref precond_expr = m_precond.precond_get_expr(t, simplified);
  expr_ref ret = expr_and(precond_expr, expr_eq(m_lhs_expr, m_rhs_expr));
  if (simplified) {
    ret = ctx->expr_do_simplify(ret);
  }
  return ret;
}

string
predicate::predicate_from_stream(istream &in/*, state const& start_state*/, context* ctx, predicate_ref &pred)
{
  string line, comment;
  bool end;
  consts_struct_t const &cs = ctx->get_consts_struct();

  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  end = !!getline(in, line);
  ASSERT(end);
  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  ASSERT(is_line(line, "Comment"));
  end = !!getline(in, line);
  ASSERT(end);
  comment = line;
  //cout << __func__ << " " << __LINE__ << ": comment = " << comment << endl;
  precond_t precond;
  line = precond_t::read_precond(in/*, start_state*/, ctx, precond);
  //end = !!getline(in, line);
  //ASSERT(end);
  //ASSERT(is_line(line, "LocalSprelAssumptions"));
  //local_sprel_expr_guesses_t lsprel_guesses(cs);
  //line = read_local_sprel_expr_assumptions(in, ctx, lsprel_guesses);
  //ASSERT(is_line(line, "Guard"));
  ////cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  //edge_guard_t guard_e;
  //line = edge_guard_t::read_edge_guard(in, guard_e/*, this*/);
  ASSERTCHECK(is_line(line, "LhsExpr"), cout << "line = " << line << endl);
  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  expr_ref lhs_e;
  line = read_expr(in, lhs_e, ctx);
  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  ASSERT(is_line(line, "RhsExpr"));
  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  expr_ref rhs_e;
  line = read_expr(in, rhs_e, ctx);

  //precond_t precond(guard_e, lsprel_guesses);
  pred = mk_predicate_ref(precond, lhs_e, rhs_e, comment);
  //p.set_local_sprel_expr_assumes(lsprel_guesses);
  return line;
}

predicate_set_t
predicate::predicate_set_from_stream(istream &in/*, state const& start_state*/, context* ctx, string const& prefix)
{
  string const pred_prefix = "=" + prefix + "pred ";
  string line;
  bool end;
  end = !getline(in, line);
  ASSERT(!end);
  predicate_set_t preds;
  while (string_has_prefix(line, pred_prefix)) {
    predicate_ref pred;
    line = predicate::predicate_from_stream(in/*, start_state*/, ctx, pred);
    preds.insert(pred);
  }
  //cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
  if (line != "=" + prefix + "predicate_set done") {
    cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
    cout << __func__ << " " << __LINE__ << ": prefix = '" << prefix << "'\n";
  }
  ASSERT(line == "=" + prefix + "predicate_set done");
  return preds;
}

void
predicate::predicate_set_to_stream(ostream &os, string const& prefix, predicate_set_t const& preds)
{
  size_t i = 0;
  for (auto const& pred : preds) {
    os << "=" << prefix << "pred " << i << endl;
    pred->predicate_to_stream(os);
    i++;
  }
  os << "=" << prefix << "predicate_set done\n";
}

void
predicate::predicate_get_asm_annotation(ostream& os) const
{
  if (!m_precond.precond_is_trivial()) {
    os << "precond => {";
  }
  os << expr_string(m_lhs_expr) << " == " << expr_string(m_rhs_expr);
  if (!m_precond.precond_is_trivial()) {
    os << "}";
  }
}

predicate_ref
predicate::ub_assume_predicate_from_expr(expr_ref const& e)
{
  ASSERT(e);
  context* ctx = e->get_context();
  return mk_predicate_ref(precond_t(), e, expr_true(ctx), predicate::op_kind_to_assume_comment(e->get_operation_kind()));
}

predicate_ref
predicate::mk_predicate_ref(predicate const &p)
{
  return predicate::get_predicate_manager()->register_object(p);
}

predicate_ref
predicate::mk_predicate_ref(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string_ref& comment)
{
  return mk_predicate_ref(predicate(precond, e_lhs, e_rhs, comment));
}

predicate_ref
predicate::mk_predicate_ref(precond_t const &precond, expr_ref const &e_lhs, expr_ref const &e_rhs, const string& comment)
{
  return mk_predicate_ref(predicate(precond, e_lhs, e_rhs, mk_string_ref(comment)));
}

manager<predicate> *
predicate::get_predicate_manager()
{
  static manager<predicate> *ret = nullptr;
  if (!ret) {
    ret = new manager<predicate>;
  }
  return ret;
}

list<predicate_ref>
predicate::determinized_pred_set(predicate_set_t const &pred_set)
{
  autostop_timer func_timer(__func__);
  list<predicate_ref> lhs_set_sorted;
  for (auto const &l : pred_set) {
    lhs_set_sorted.push_back(l);
  }
  lhs_set_sorted.sort(predicate::pred_compare_ids);
  return lhs_set_sorted;
}

void
predicate::predicate_set_union(predicate_set_t &dst, predicate_set_t const& src)
{
  set<size_t> s_indices_already_added;
  predicate_set_t new_dst;
  for (auto const& r : dst) {
    size_t si = 0;
    predicate_ref new_r = r;
    for (auto const& selem : src) {
      bool edge_guard_union_succeeded;
      tie(edge_guard_union_succeeded, new_r) = new_r->pred_try_unioning_edge_guards(selem);
      if (edge_guard_union_succeeded) {
        s_indices_already_added.insert(si);
      }
      si++;
    }
    new_dst.insert(new_r);
  }
  dst = new_dst;
  size_t si = 0;
  for (auto const& selem : src) {
    if (!set_belongs(s_indices_already_added, si)) {
      dst.insert(selem);
    }
    si++;
  }
}

list<predicate_ref>
lhs_set_substitute_using_submap(list<predicate_ref> const &lhs, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, context *ctx)
{
  autostop_timer func_timer(__func__);
  list<predicate_ref> ret;
  for (auto const& p : lhs) {
    predicate_ref pnew = p->pred_substitute_using_submap(ctx, submap);
    pnew = pnew->simplify();
    if (pnew->predicate_is_trivial()) {
      continue;
    }
    //cout << __func__ << " " << __LINE__ << ": changing from " << p.to_string(true) << " to " << pnew.to_string(true) << endl;
    ret.push_back(pnew);
  }
  return ret;
}

list<predicate_ref>
lhs_set_eliminate_constructs_that_the_solver_cannot_handle(list<predicate_ref> const &lhs_set, context *ctx)
{
  autostop_timer func_timer(__func__);
  list<predicate_ref> ret;
  for (auto const& p : lhs_set) {
    predicate_ref pnew = p->pred_eliminate_constructs_that_the_solver_cannot_handle(ctx);
    //cout << __func__ << " " << __LINE__ << ": pnew = " << pnew.to_string(true) << endl;
    ret.push_back(pnew);
  }
  return ret;
}


bool
lhs_set_includes_nonstack_mem_equality(list<predicate_ref> const &lhs_set, memlabel_assertions_t const& mlasserts, map<expr_id_t, expr_ref> &input_mem_exprs)
{
  autostop_timer func_timer(__func__);
  for (auto const& lhs_pred : lhs_set) {
    if (!lhs_pred->get_precond().precond_is_trivial()) {
      continue;
    }
    expr_ref const &src = lhs_pred->get_lhs_expr();
    expr_ref const &dst = lhs_pred->get_rhs_expr();
    context* ctx = src->get_context();
    //if (   src->is_array_sort()
    //    && dst->is_array_sort()
    //    && src->get_operation_kind() == expr::OP_MEMMASK
    //    && dst->get_operation_kind() == expr::OP_MEMMASK
    //    && src->get_args().at(0)->is_var()
    //    && dst->get_args().at(0)->is_var()
    //    //&& dst->get_args().at(1)->get_memlabel_value().memlabel_captures_all_nonstack_mls(ctx->get_memlabel_assertions()/*relevant_memlabels*/)
    //    //&& src->get_args().at(1)->get_memlabel_value().memlabel_captures_all_nonstack_mls(ctx->get_memlabel_assertions()/*relevant_memlabels*/)
    //    && dst->get_args().at(1)->get_memlabel_value() == mlasserts.get_memlabel_all_except_stack()
    //    && src->get_args().at(1)->get_memlabel_value() == mlasserts.get_memlabel_all_except_stack()
    //   ) {
    //  input_mem_exprs.insert(make_pair(src->get_args().at(0)->get_id(), src->get_args().at(0)));
    //  input_mem_exprs.insert(make_pair(dst->get_args().at(0)->get_id(), dst->get_args().at(0)));
    //  return true;
    //}
    if (   src->get_operation_kind() == expr::OP_MEMMASKS_ARE_EQUAL
        && src->get_args().at(0)->is_var()
        && src->get_args().at(1)->is_var()
        && src->get_args().at(2)->get_memlabel_value() == mlasserts.get_memlabel_all_except_stack()
       ) {
      input_mem_exprs.insert(make_pair(src->get_args().at(0)->get_id(), src->get_args().at(0)));
      input_mem_exprs.insert(make_pair(src->get_args().at(1)->get_id(), src->get_args().at(1)));
      return true;
    } else if (   dst->get_operation_kind() == expr::OP_MEMMASKS_ARE_EQUAL
               && dst->get_args().at(0)->is_var()
               && dst->get_args().at(1)->is_var()
               && dst->get_args().at(2)->get_memlabel_value() == mlasserts.get_memlabel_all_except_stack()
        ) {
      input_mem_exprs.insert(make_pair(dst->get_args().at(0)->get_id(), dst->get_args().at(0)));
      input_mem_exprs.insert(make_pair(dst->get_args().at(1)->get_id(), dst->get_args().at(1)));
      return true;
    }
  }
  return false;
}

expr_ref
predicate_set_and(list<predicate_ref> const &preds, expr_ref const &in, precond_t const& precond, tfg const &t)
{
  autostop_timer ft(__func__);
  expr_ref cur = in;
  context* ctx = in->get_context();
  for (auto const& p : preds) {
    if (p->predicate_is_trivial()) {
      continue;
    }
    // we need these preds for proper evaluation of counter example
    /*if (!precond.precond_get_guard().edge_guard_may_imply(p.get_edge_guard())) {
      CPP_DBG_EXEC2(HOUDINI, cout << __func__ << ':' << __LINE__ << ": skipping predicate as its edgeguard is not implied by precond's edgeguard\n p = " << p.to_string(true) << endl);
      continue;
    }*/
    expr_ref eq_e = expr_eq(p->get_lhs_expr(), p->get_rhs_expr());
    expr_ref pc_e = p->get_effective_precond(t);
    if (   pc_e == expr_true(ctx)
        || precond.precond_implies(p->get_precond())) {
      cur = expr_and(cur, eq_e);
    } else {
      //pc_e = ctx->expr_substitute(pc_e, rodata_submap);
      pc_e = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(pc_e);
      cur = expr_and(cur, expr_implies(pc_e, eq_e));
    }
  }
  return cur;
}


class expr_unsafe_cond_visitor : public expr_visitor
{
public:
  expr_unsafe_cond_visitor(expr_ref const &e, std::function<expr_ref (expr_ref const&)> f) : m_expr(e), m_unsafe_cond_func(f), m_ctx(e->get_context())
  {
    visit_recursive(m_expr);
  }
  predicate_set_t const& get_result() const { return m_result; }
private:
  virtual void visit(expr_ref const &e);

private:
  expr_ref const &m_expr;
  std::function<expr_ref (expr_ref const&)> m_unsafe_cond_func;
  context *m_ctx;
  predicate_set_t m_result;
};

void
expr_unsafe_cond_visitor::visit(expr_ref const& e)
{
  expr_ref e_is_unsafe = m_unsafe_cond_func(e);
  if (e_is_unsafe != expr_false(m_ctx)) {
    predicate_ref pred = predicate::mk_predicate_ref(precond_t(), e_is_unsafe, expr_true(m_ctx), "unsafety-cond");
    m_result.insert(pred);
  }
}




predicate_set_t
expr_get_div_by_zero_cond(expr_ref const& e)
{
  context *ctx = e->get_context();
  std::function<expr_ref (expr_ref const&)> f = [ctx](expr_ref const& e)
  {
    if (   e->get_operation_kind() == expr::OP_BVUDIV
        || e->get_operation_kind() == expr::OP_BVSDIV
        || e->get_operation_kind() == expr::OP_BVUREM
        || e->get_operation_kind() == expr::OP_BVSREM
       ) {
      ASSERT(e->get_args().size() == 2);
      expr_ref const& divisor = e->get_args().at(1);
      ASSERT(divisor->is_bv_sort());
      size_t divisor_bvsize = divisor->get_sort()->get_size();
      return ctx->mk_eq(divisor, ctx->mk_zerobv(divisor_bvsize));
    }
    return expr_false(ctx);
  };
  expr_unsafe_cond_visitor visitor(e, f);
  return visitor.get_result();
}

predicate_set_t
expr_get_memlabel_heap_access_cond(expr_ref const& e, graph_memlabel_map_t const& memlabel_map)
{
  context *ctx = e->get_context();
  consts_struct_t const& cs = ctx->get_consts_struct();
  std::function<expr_ref (expr_ref const&)> f = [ctx, &memlabel_map](expr_ref const& e)
  {
    if (e->get_operation_kind() == expr::OP_SELECT || e->get_operation_kind() == expr::OP_STORE) {
      expr_ref const& addr = e->get_args().at(2);
      expr_ref const& mlexpr = e->get_args().at(1);
      memlabel_t ml;
      if (mlexpr->is_var()) {
        ASSERT(memlabel_map.count(mlexpr->get_name()));
        ml = memlabel_map.at(mlexpr->get_name())->get_ml();
      } else {
        ASSERT(mlexpr->is_const());
        ml = mlexpr->get_memlabel_value();
      }
      memlabel_t ml_heap = memlabel_t::memlabel_heap();
      if (memlabel_t::memlabel_contains(&ml, &ml_heap)) {
        size_t count = (e->get_operation_kind() == expr::OP_STORE) ? e->get_args().at(4)->get_int_value() : e->get_args().at(3)->get_int_value();
        expr_ref last_addr = ctx->mk_bvadd(addr, ctx->mk_bv_const(addr->get_sort()->get_size(), count - 1));
        expr_ref heap_unallocated_start_addr = ctx->mk_var(HEAP_UNALLOCATED_START_ADDR, addr->get_sort());
        expr_ref heap_unallocated_end_addr = ctx->mk_var(HEAP_UNALLOCATED_END_ADDR, addr->get_sort());
        ASSERT(g_page_size_for_safety_checks > 0);
        ASSERT(is_power_of_two(g_page_size_for_safety_checks, NULL));
        expr_ref page_mask = ctx->expr_do_simplify(ctx->mk_bvnot(ctx->mk_bv_const(addr->get_sort()->get_size(), (uint64_t)g_page_size_for_safety_checks - 1)));
        expr_ref heap_unallocated_start_addr_page_round_up = ctx->mk_bvand(ctx->mk_bvadd(heap_unallocated_start_addr, ctx->mk_bv_const(addr->get_sort()->get_size(), (uint64_t)g_page_size_for_safety_checks - 1)), page_mask);
        expr_ref heap_unallocated_end_addr_page_round_down = ctx->mk_bvand(heap_unallocated_end_addr, page_mask);
        expr_ref unallocated_region_is_non_empty = ctx->mk_bvult(heap_unallocated_start_addr_page_round_up, heap_unallocated_end_addr_page_round_down);
        expr_ref is_unallocated = ctx->mk_or(
            ctx->mk_or(
                       ctx->mk_and(ctx->mk_bvuge(addr, heap_unallocated_start_addr_page_round_up), // start addr inside unallocated region
                                   ctx->mk_bvult(addr, heap_unallocated_end_addr_page_round_down)),
                       ctx->mk_and(ctx->mk_bvuge(last_addr, heap_unallocated_start_addr_page_round_up), // last addr inside unallocated region
                                   ctx->mk_bvult(last_addr, heap_unallocated_end_addr_page_round_down))),
             // unallocated region inside [addr,last_addr]
             ctx->mk_and(ctx->mk_bvult(addr, heap_unallocated_start_addr_page_round_up),
                         ctx->mk_bvuge(last_addr, heap_unallocated_end_addr_page_round_down)));
        //expr_ref is_unallocated = ctx->mk_or(ctx->mk_bvuge(last_addr, heap_allocated_start_addr_page_round_up), ctx->mk_bvult(addr, heap_allocated_end_addr_page_round_down));
        return ctx->mk_and(ctx->mk_implies(unallocated_region_is_non_empty, is_unallocated), ctx->mk_ismemlabel(addr, count, memlabel_t::memlabel_heap()));
      }
    }
    return expr_false(ctx);
  };
  expr_unsafe_cond_visitor visitor(e, f);
  return visitor.get_result();
}

predicate_set_t
expr_is_not_memlabel_access_cond(expr_ref const& e, graph_memlabel_map_t const& memlabel_map)
{
  context *ctx = e->get_context();
  consts_struct_t const& cs = ctx->get_consts_struct();
  std::function<expr_ref (expr_ref const&)> f = [ctx, &memlabel_map](expr_ref const& e)
  {
    if (e->get_operation_kind() == expr::OP_SELECT || e->get_operation_kind() == expr::OP_STORE) {
      expr_ref const& addr = e->get_args().at(2);
      expr_ref const& mlexpr = e->get_args().at(1);
      memlabel_t ml;
      if (mlexpr->is_var()) {
        if (!memlabel_map.count(mlexpr->get_name())) {
          cout << __func__ << " " << __LINE__ << ": mlexpr->get_name() = " << mlexpr->get_name()->get_str() << endl;
        }
        ASSERT(memlabel_map.count(mlexpr->get_name()));
        ml = memlabel_map.at(mlexpr->get_name())->get_ml();
      } else {
        ASSERT(mlexpr->is_const());
        ml = mlexpr->get_memlabel_value();
      }
      size_t count = (e->get_operation_kind() == expr::OP_STORE) ? e->get_args().at(4)->get_int_value() : e->get_args().at(3)->get_int_value();
      return ctx->mk_not(ctx->mk_ismemlabel(addr, count, ml));
    }
    return expr_false(ctx);
  };
  expr_unsafe_cond_visitor visitor(e, f);
  return visitor.get_result();
}

bool
predicate::predicate_eval_on_counter_example(set<memlabel_ref> const& relevant_memlabels, bool &eval_result, counter_example_t &counter_example) const
{
  context *ctx = m_lhs_expr->get_context();
  if (!m_precond.precond_eval_on_counter_example(relevant_memlabels, eval_result, counter_example)) {
    return false;
  }
  if (!eval_result) {
    eval_result = true; //if precond's eval_result is false, pred's eval_result is true
    return true;
  }

  expr_ref const_lhs_expr, const_rhs_expr;
  counter_example_t rand_vals(ctx);
  if (!counter_example.evaluate_expr_assigning_random_consts_as_needed(m_lhs_expr, const_lhs_expr, rand_vals, relevant_memlabels)) {
    return false;
  }
  ASSERT(const_lhs_expr->is_const());
  if (!counter_example.evaluate_expr_assigning_random_consts_as_needed(m_rhs_expr, const_rhs_expr, rand_vals, relevant_memlabels)) {
    return false;
  }
  ASSERT(const_rhs_expr->is_const());
  counter_example.counter_example_union(rand_vals);
  eval_result = (const_lhs_expr == const_rhs_expr);
  return true;
}

//predicate_try_adjust_CE_to_disprove_pred assigns random values to (potentially already assigned) vars to try and disprove pred
bool
predicate::predicate_try_adjust_CE_to_disprove_pred(counter_example_t &counter_example) const
{
  return true; //NOT_IMPLEMENTED(); //can do more stuff here
}

//predicate_try_adjust_CE_to_disprove_pred assigns random values to (potentially already assigned) vars to try and prove pred
bool
predicate::predicate_try_adjust_CE_to_prove_pred(counter_example_t &counter_example) const
{
  return true; //NOT_IMPLEMENTED(); //can do more stuff here
}

class expr_assign_mlvars_to_memlabels_visitor : public expr_visitor {
public:
  expr_assign_mlvars_to_memlabels_visitor(context *ctx, expr_ref const &e, map<pc, pair<mlvarname_t, mlvarname_t>> &fcall_mlvars, map<pc, map<expr_ref, mlvarname_t>> &expr_mlvars, string const &graph_name, long &mlvarnum, pc const &p) : m_ctx(ctx), m_expr(e), m_fcall_mlvars(fcall_mlvars), m_expr_mlvars(expr_mlvars), m_graph_name(graph_name), m_mlvarnum(mlvarnum), m_pc(p)
  {
    visit_recursive(m_expr);
  }
  expr_ref get_result() const { return m_map.at(m_expr->get_id()); }
private:
  virtual void visit(expr_ref const &e);

  context *m_ctx;
  expr_ref const &m_expr;
  map<pc, pair<mlvarname_t, mlvarname_t>> &m_fcall_mlvars;
  map<pc, map<expr_ref, mlvarname_t>> &m_expr_mlvars;
  string const &m_graph_name;
  long &m_mlvarnum;
  pc const &m_pc;
  map<expr_id_t, expr_ref> m_map;
};

void
expr_assign_mlvars_to_memlabels_visitor::visit(expr_ref const &e)
{
  if (e->is_var() || e->is_const()) {
    m_map[e->get_id()] = e;
    return;
  }
  expr_vector new_args;
  for (auto arg : e->get_args()) {
    new_args.push_back(m_map.at(arg->get_id()));
  }
  if (   e->get_operation_kind() == expr::OP_SELECT
      || e->get_operation_kind() == expr::OP_STORE) {
    expr_ref addr = new_args.at(2);
    mlvarname_t mlvarname;
    if (m_expr_mlvars.count(m_pc) && m_expr_mlvars.at(m_pc).count(addr)) {
      mlvarname = m_expr_mlvars.at(m_pc).at(addr);
    } else {
      mlvarname = m_ctx->memlabel_varname(m_graph_name, m_mlvarnum);
      m_expr_mlvars[m_pc][addr] = mlvarname;
      m_mlvarnum++;
    }
    new_args[1] = m_ctx->mk_var(mlvarname->get_str(), m_ctx->mk_memlabel_sort());
  } else if (e->get_operation_kind() == expr::OP_FUNCTION_CALL) {
    pair<mlvarname_t, mlvarname_t> mlvarnames;
    if (m_fcall_mlvars.count(m_pc)) {
      mlvarnames = m_fcall_mlvars.at(m_pc);
    } else {
      mlvarnames = make_pair(m_ctx->memlabel_varname_for_fcall(m_graph_name, m_mlvarnum), m_ctx->memlabel_varname_for_fcall(m_graph_name, m_mlvarnum + 1));
      m_fcall_mlvars[m_pc] = mlvarnames;
      m_mlvarnum+=2;
    }
    new_args[OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE] = m_ctx->mk_var(mlvarnames.first->get_str(), m_ctx->mk_memlabel_sort());
    new_args[OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE] = m_ctx->mk_var(mlvarnames.second->get_str(), m_ctx->mk_memlabel_sort());
  }
  m_map[e->get_id()] = m_ctx->mk_app(e->get_operation_kind(), new_args);
}

expr_ref
expr_assign_mlvars_to_memlabels(expr_ref const &e, map<pc, pair<mlvarname_t, mlvarname_t>> &fcall_mlvars, map<pc, map<expr_ref, mlvarname_t>> &expr_mlvars, string const &graph_name, long &mlvarnum, pc const &p)
{
  expr_assign_mlvars_to_memlabels_visitor visitor(e->get_context(), e, fcall_mlvars, expr_mlvars, graph_name, mlvarnum, p);
  return visitor.get_result();
}

}
