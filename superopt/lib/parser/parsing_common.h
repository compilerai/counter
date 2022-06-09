#pragma once

#include <string>
#include <map>
#include <vector>
#include <stack>
#include <set>

#include "expr/defs.h"
#include "expr/context.h"
#include "support/debug.h"
#include "support/bv_const.h"

using namespace std;

namespace eqspace {

class AstNode;
using AstNodeRef = shared_ptr<AstNode const>;

}

namespace std {
template<>
struct hash<eqspace::AstNode>
{
  std::size_t operator()(eqspace::AstNode const &e) const;
};
}

namespace eqspace {

class AstNode
{
public:
  enum Kind {
    ATOM,
    VAR,
    ARRAY_INTERP,
    AS_CONST,
    APPLY,
    LAMBDA,
    FORALL,
    LET,
    ITE,
    COMPOSITE
  };

  static AstNodeRef mk_AstNode(AstNode const& ast);
  static AstNodeRef mk_Var(string const& name);
  static AstNodeRef mk_Atom(expr_ref const& atom);
  static AstNodeRef mk_Array_Interp(string const& fn_name);
  static AstNodeRef mk_As_Const(sort_ref const& sort, AstNodeRef const& arg);
  static AstNodeRef mk_Apply(string const& fn_name, vector<AstNodeRef> const& args);
  static AstNodeRef mk_Lambda(vector<AstNodeRef> const& formal_args, AstNodeRef const& body);
  static AstNodeRef mk_Forall(vector<AstNodeRef> const& formal_args, AstNodeRef const& body);
  static AstNodeRef mk_Let(string const& name, AstNodeRef const& assign, AstNodeRef const& body);
  static AstNodeRef mk_Ite(AstNodeRef const& cond, AstNodeRef const& thenA, AstNodeRef const& elseA);
  static AstNodeRef mk_Composite(expr::operation_kind opk, vector<AstNodeRef> const& args);

  ~AstNode()
  {
    if (!id_is_zero()) {
      AstNode::get_free_id_list().add_free_id(m_id);
      AstNode::get_manager().deregister_object(*this);
      m_id = 0;
    }
  }

  unsigned get_id() const { return m_id; }
  bool id_is_zero() const { return m_id == 0; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(m_id == 0);
    m_id = AstNode::get_free_id_list().get_free_id(suggested_id);
  }

  static free_id_list& get_free_id_list()
  {
    static free_id_list free_ids;
    return free_ids;
  }
  static manager<AstNode>& get_manager()
  {
    static manager<AstNode> manager;
    return manager;
  };

  Kind get_kind() const { return m_kind; }
  vector<AstNodeRef> const& get_args() const { return m_args; }

  expr_ref get_atom() const { ASSERT(m_kind == ATOM); return m_atom; }
  sort_ref get_sort() const { ASSERT(m_kind == AS_CONST); return m_sort; }
  string_ref get_name() const { ASSERT(m_kind == VAR || m_kind == ARRAY_INTERP || m_kind == LET || m_kind == APPLY); return m_name; }
  expr::operation_kind get_composite_operation_kind() const { ASSERT(m_kind == COMPOSITE); return m_op_kind; }

  bool operator==(AstNode const& other) const;

private:
  AstNode(expr_ref const& atom) : m_kind(ATOM), m_atom(atom) { }
  AstNode(Kind k, string const& name) : m_kind(k), m_name(mk_string_ref(name))
  {
    ASSERT(k == VAR || k == ARRAY_INTERP);
  }
  AstNode(sort_ref const& sort, vector<AstNodeRef> const& args) : m_kind(AS_CONST), m_sort(sort), m_args(args) { ASSERT(args.size() == 1); }
  AstNode(Kind k, string const& name, vector<AstNodeRef> const& args) : m_kind(k), m_name(mk_string_ref(name)), m_args(args)
  {
    ASSERT(k == LET || k == APPLY);
    if (k == LET) { ASSERT(args.size() == 2); }
  }
  AstNode(Kind k, vector<AstNodeRef> const& args) : m_kind(k), m_args(args)
  {
    ASSERT(k == ITE || k == LAMBDA || k == FORALL);
    if (k == ITE) { ASSERT(args.size() == 3); }
  }
  AstNode(expr::operation_kind opk, vector<AstNodeRef> const& args) : m_kind(COMPOSITE), m_op_kind(opk), m_args(args) {}

  enum Kind m_kind;

  unsigned m_id = 0;
  // ATOM
  expr_ref m_atom;
  // VAR, ARRAY_INTERP, LET, APPLY
  string_ref m_name;
  // AS_CONST
  sort_ref m_sort;
  // COMPOSITE
  expr::operation_kind m_op_kind;

  vector<AstNodeRef> m_args;
};

class AstNodeVisitor
{
protected:
  // previsit returns TRUE -> visit the node
  virtual bool previsit(AstNodeRef const& ast) { return !visited(ast); }
  virtual void visit(AstNodeRef const& ast) = 0;

  bool visited(AstNodeRef const& ast) const { return this->m_visited.count(ast); }

  void visit_recursive(AstNodeRef const& ast);

private:
  set<AstNodeRef> m_visited;
};

ostream& operator<<(ostream& os, AstNode const& ast);

class ExprDual
{
public:
  static ExprDual dual(expr_ref const& e, expr_ref const& linear) { ASSERT(e); return ExprDual(e, linear); }
  static ExprDual normal(expr_ref const& e)                       { ASSERT(e); return ExprDual(e, nullptr); }
  static ExprDual dual_if_const(expr_ref const& e)
  {
    ASSERT(e);
    if (e->is_const())
      return dual(e, e);
    else return normal(e);
  }

  bool has_linear_form() const { return m_linear != nullptr; }

  expr_ref normal_form() const { ASSERT(m_normal); return m_normal; }
  expr_ref linear_form() const { ASSERT(m_linear);  return m_linear; }

  ExprDual set_normal(expr_ref const& e) const { return ExprDual(e, m_linear); }
  ExprDual set_linear(expr_ref const& e) const { return ExprDual(m_normal, e); }

private:
  ExprDual(expr_ref const& normal, expr_ref const& linear)
    : m_normal(normal),
      m_linear(linear)
  { }

  expr_ref m_normal;
  expr_ref m_linear;
};

class ParsingEnv {
  /* Q: Why distinguish b/w global and local funcs?
   * A: Global funcs are passed down to user -- local funcs are not
   *
   * we maintain both the sort (i.e. domain and sort) and actual expression of the function
   * sort information is essential for type matching
   */
  context* m_ctx;
  set<string_ref> m_global_funcs;                   /* these are returned to user */
  map<string_ref,pair<AstNodeRef,sort_ref>> m_funcs;
  mutable map<string_ref,ExprDual> m_linearized_funcs;

  ExprDual linearize_func_ast_to_expr(AstNodeRef const& ast, sort_ref const& fn_sort) const;

public:
  ParsingEnv(context* ctx) : m_ctx(ctx) { ASSERT(ctx); }

  void add_global_func(string const& fname)         { m_global_funcs.insert(mk_string_ref(fname)); }
  set<string_ref> get_global_funcs() const          { return m_global_funcs; }
  bool is_global_func(string_ref const& name) const { return m_global_funcs.count(name); }

  bool is_func(string_ref const& name) const                   { return m_funcs.count(name); }
  AstNodeRef const& get_func_ast(string_ref const& name) const { ASSERT(m_funcs.count(name)); return m_funcs.at(name).first; }
  sort_ref get_func_sort(string_ref const& name) const         { ASSERT(m_funcs.count(name)); return m_funcs.at(name).second; }
  map<string_ref,pair<AstNodeRef,sort_ref>> const& get_funcs() const  { return m_funcs; }

  AstNodeRef add_func(string const& name, AstNodeRef const& body, vector<expr_ref> const& params, sort_ref const& range_sort);

  ExprDual linearize_func_to_expr(string_ref const& fname) const
  {
    if (m_linearized_funcs.count(fname)) {
      return m_linearized_funcs.at(fname);
    }
    AstNodeRef const& ast = this->get_func_ast(fname);
    sort_ref const& fn_sort = this->get_func_sort(fname);
    auto ret = this->linearize_func_ast_to_expr(ast, fn_sort);
    m_linearized_funcs.insert(make_pair(fname, ret));
    return ret;
  }
};

pair<int,bv_const> str_to_bvlen_val_pair(string const& s);

}
