#include "expr/context.h"
#include "support/dyn_debug.h"
#include "support/utils.h"
#include "parser/parsing_common.h"
#include "parser/ast_linearizer.h"

using namespace std;

namespace std {
std::size_t
hash<eqspace::AstNode>::operator()(eqspace::AstNode const &e) const
{
  size_t seed = 0;
  myhash_combine<size_t>(seed, static_cast<size_t>(e.get_kind()));
  switch (e.get_kind()) {
    case AstNode::ATOM:
      myhash_combine<size_t>(seed, hash<expr_ref>()(e.get_atom()));
      break;
    case AstNode::VAR:
    case AstNode::ARRAY_INTERP:
    case AstNode::LET:
    case AstNode::APPLY:
      myhash_combine<size_t>(seed, hash<string_ref>()(e.get_name()));
      break;
    case AstNode::AS_CONST:
      myhash_combine<size_t>(seed, hash<sort_ref>()(e.get_sort()));
      break;
    case AstNode::COMPOSITE:
      myhash_combine<size_t>(seed, hash<eqspace::expr::operation_kind>()(e.get_composite_operation_kind()));
      break;
    case AstNode::FORALL:
    case AstNode::LAMBDA:
    case AstNode::ITE:
      break;
    default:
      NOT_IMPLEMENTED();
  }
  for (auto const& arg : e.get_args()) {
    myhash_combine<size_t>(seed, hash<eqspace::AstNodeRef>()(arg));
  }
  return seed;
}
}

namespace eqspace {

AstNodeRef
AstNode::mk_AstNode(AstNode const& ast)
{
  return AstNode::get_manager().register_object(ast);
}

AstNodeRef
AstNode::mk_Atom(expr_ref const& atom)
{
  return mk_AstNode(AstNode(atom));
}

AstNodeRef
AstNode::mk_Var(string const& name)
{
  return mk_AstNode(AstNode(AstNode::VAR, name));
}

AstNodeRef
AstNode::mk_Array_Interp(string const& fn_name)
{
  return mk_AstNode(AstNode(AstNode::ARRAY_INTERP, fn_name));
}

AstNodeRef
AstNode::mk_As_Const(sort_ref const& sort, AstNodeRef const& arg)
{
  return mk_AstNode(AstNode(sort, { arg }));
}

AstNodeRef
AstNode::mk_Apply(string const& fn_name, vector<AstNodeRef> const& args)
{
  return mk_AstNode(AstNode(AstNode::APPLY, fn_name, args));
}

AstNodeRef
AstNode::mk_Lambda(vector<AstNodeRef> const& formal_args, AstNodeRef const& body)
{
  auto tmp = formal_args;
  tmp.push_back(body);
  return mk_AstNode(AstNode(AstNode::LAMBDA, tmp));
}

AstNodeRef
AstNode::mk_Forall(vector<AstNodeRef> const& formal_args, AstNodeRef const& body)
{
  auto tmp = formal_args;
  tmp.push_back(body);
  return mk_AstNode(AstNode(AstNode::FORALL, tmp));
}

AstNodeRef
AstNode::mk_Let(string const& name, AstNodeRef const& assign, AstNodeRef const& body)
{
  return mk_AstNode(AstNode(AstNode::LET, name, { assign, body }));
}

AstNodeRef
AstNode::mk_Ite(AstNodeRef const& cond, AstNodeRef const& thenA, AstNodeRef const& elseA)
{
  return mk_AstNode(AstNode(AstNode::ITE, { cond, thenA, elseA }));
}

AstNodeRef
AstNode::mk_Composite(expr::operation_kind opk, vector<AstNodeRef> const& args)
{
  return mk_AstNode(AstNode(opk, args));
}

bool
AstNode::operator==(AstNode const& other) const
{
  if (this->m_kind != other.m_kind) {
    return false;
  }
  switch (this->m_kind) {
    case ATOM:
      return this->m_atom == other.m_atom;
    case VAR:
    case ARRAY_INTERP:
      return this->m_name == other.m_name;
    case AS_CONST:
      if (this->m_sort != other.m_sort)
        return false;
      return this->m_args.front() == other.m_args.front();
    case COMPOSITE:
      if (this->m_op_kind != other.m_op_kind)
        return false;
    case LAMBDA:
    case FORALL:
    case ITE:
    case APPLY:
    case LET:
      if (this->m_name != other.m_name)
        return false;
      if (this->m_args.size() != other.m_args.size())
        return false;
      for (size_t i = 0; i < this->m_args.size(); ++i) {
        if (this->m_args[i] != other.m_args[i])
          return false;
      }
  }
  return true;
}

void
AstNodeVisitor::visit_recursive(AstNodeRef const& ast)
{
  //if (m_visited.count(ast))
  //  return;
  if (!this->previsit(ast))
    return;
  for (auto const& arg : ast->get_args()) {
    this->visit_recursive(arg);
  }
  this->visit(ast);
  this->m_visited.insert(ast);
}

ostream&
operator<<(ostream& os, AstNode const& ast)
{
#define PRINT_ARGS(itr) do {                                    \
                          bool print_comma = false;             \
                          while (itr != ast.get_args().end()) { \
                            if (print_comma) { os << ", "; }  \
                            print_comma = true;                 \
                            os << **itr;                       \
                            ++itr;                              \
                          }                                     \
                        } while (false)

  switch (ast.get_kind()) {
    case AstNode::ATOM:
      os << "ATOM[" << expr_string(ast.get_atom()) << "]";
      break;
    case AstNode::VAR:
      os << ast.get_name()->get_str();
      break;
    case AstNode::ARRAY_INTERP:
      os << "ARRAY_INTERP[" << ast.get_name()->get_str() << "]";
      break;
    case AstNode::AS_CONST:
      os << "AS_CONST[" << *ast.get_args().front() << "::" << ast.get_sort()->to_string() << "]";
      break;
    case AstNode::APPLY: {
      os << "APPLY[" << ast.get_name()->get_str() << "(";
      auto itr = ast.get_args().begin();
      PRINT_ARGS(itr);
      os << ")]";
      break;
    }
    case AstNode::LAMBDA: {
      os << "LAMBDA[";
      auto itr = ast.get_args().begin();
      PRINT_ARGS(itr);
      os << "]";
      break;
    }
    case AstNode::FORALL: {
      os << "FORALL[";
      auto itr = ast.get_args().begin();
      PRINT_ARGS(itr);
      os << "]";
      break;
    }
    case AstNode::LET:
      ASSERT(ast.get_args().size() == 2);
      os << "LET[" << ast.get_name()->get_str() << " := " << *ast.get_args().front() << " IN " << *ast.get_args().back() << "]";
      break;
    case AstNode::ITE:
      os << "ITE[" << *ast.get_args().at(0) << ", " << *ast.get_args().at(1) << ", " << *ast.get_args().at(2) << "]";
      break;
    case AstNode::COMPOSITE: {
      os << "[" << op_kind_to_string(ast.get_composite_operation_kind()) << "(";
      auto itr = ast.get_args().begin();
      PRINT_ARGS(itr);
      os << ")]";
      break;
    }
    default:
      NOT_IMPLEMENTED();
  }
  return os;
}

AstNodeRef
ParsingEnv::add_func(string const& name, AstNodeRef const& body, vector<expr_ref> const& params, sort_ref const& range_sort)
{
  string_ref name_ref = mk_string_ref(name);
  if (m_funcs.count(name_ref)) {
    cout << "Error! Multiple definitions of ``" << name << "''!" << endl;
    NOT_IMPLEMENTED();
  }
	auto tmp = vmap<expr_ref,AstNodeRef>(params, [](expr_ref const& e) { return AstNode::mk_Atom(e); });
	auto fn_expr = AstNode::mk_Lambda(tmp, body);

  auto domain_sort = expr_vector_to_sort_vector(params);
  auto fn_sort = m_ctx->mk_function_sort(domain_sort, range_sort);

  m_funcs.insert(make_pair(name_ref, make_pair(fn_expr, fn_sort)));
  return fn_expr;
}

class AstDagSizeVisitor : public AstNodeVisitor
{
public:
  AstDagSizeVisitor(AstNodeRef const& ast) { visit_recursive(ast); }
  unsigned get_result() const { return m_size; }

protected:
  void visit(AstNodeRef const& ast) override { ++m_size; }

private:
  unsigned m_size = 0;
};

unsigned ast_dag_size(AstNodeRef const& ast)
{
  AstDagSizeVisitor vtor(ast);
  return vtor.get_result();
}

class AstSizeVisitor : public AstNodeVisitor
{
public:
  AstSizeVisitor(AstNodeRef const& ast) : m_ast(ast) { visit_recursive(ast); }
  unsigned get_result() const { return m_size.at(m_ast); }

protected:
  void visit(AstNodeRef const& ast) override
  {
    unsigned size = 1;
    for (auto const& arg : ast->get_args()) {
      size += m_size[arg];
    }
    m_size[ast] = size;
  }

private:
  AstNodeRef m_ast;
  map<AstNodeRef,unsigned> m_size;
};

unsigned ast_size(AstNodeRef const& ast)
{
  AstSizeVisitor vtor(ast);
  return vtor.get_result();
}
AstNodeRef
ast_canonicalize_ite(AstNodeRef const& ast)
{
  // XXX TODO
  return ast;
}

class AstCanonicalizeTopLevelBooleansVisitor : public AstNodeVisitor
{
public:
  AstCanonicalizeTopLevelBooleansVisitor(AstNodeRef const& ast, context* ctx) : m_ast(ast), m_ctx(ctx) { visit_recursive(ast); }
  AstNodeRef get_result() const { return m_map.at(m_ast); }

protected:
  bool is_boolean_opk(expr::operation_kind opk) const
  {
    return opk == expr::OP_NOT || opk == expr::OP_AND || opk == expr::OP_OR || opk == expr::OP_EQ;
  }
  AstNodeRef top_level_boolean_to_ite(AstNodeRef const& ast)
  {
    if (!(   ast->get_kind() == AstNode::COMPOSITE
          && is_boolean_opk(ast->get_composite_operation_kind()))) {
      return ast;
    }
    return AstNode::mk_Ite(ast, AstNode::mk_Atom(m_ctx->mk_bool_true()), AstNode::mk_Atom(m_ctx->mk_bool_false()));
  }
  void visit(AstNodeRef const& ast) override
  {
    vector<AstNodeRef> args;
    for (auto const& arg : ast->get_args()) {
      args.push_back(m_map.at(arg));
    }
    switch (ast->get_kind()) {
      case AstNode::ATOM:
      case AstNode::VAR:
      case AstNode::ARRAY_INTERP:
        m_map[ast] = ast;
        return;
      case AstNode::AS_CONST:
        m_map[ast] = AstNode::mk_As_Const(ast->get_sort(), args.front());
        return;
      case AstNode::APPLY:
        m_map[ast] = AstNode::mk_Apply(ast->get_name()->get_str(), args);
        return;
      case AstNode::LAMBDA: {
        vector<AstNodeRef> formal_args(args.cbegin(), prev(args.cend()));
        AstNodeRef body = top_level_boolean_to_ite(args.back());
        m_map[ast] = AstNode::mk_Lambda(formal_args, body);
        return;
      }
      case AstNode::FORALL: {
        vector<AstNodeRef> formal_args(args.cbegin(), prev(args.cend()));
        AstNodeRef body = top_level_boolean_to_ite(args.back());
        m_map[ast] = AstNode::mk_Forall(formal_args, body);
        return;
      }
      case AstNode::LET: {
        AstNodeRef assign = top_level_boolean_to_ite(args.front());
        AstNodeRef body = top_level_boolean_to_ite(args.back());
        m_map[ast] = AstNode::mk_Let(ast->get_name()->get_str(), assign, body);
        return;
      }
      case AstNode::ITE:
        m_map[ast] = AstNode::mk_Ite(args.at(0), args.at(1), args.at(2));
        return;
      case AstNode::COMPOSITE:
        m_map[ast] = AstNode::mk_Composite(ast->get_composite_operation_kind(), args);
        return;
      default:
        NOT_REACHED();
    }
  }

private:
  AstNodeRef m_ast;
  context* m_ctx;
  map<AstNodeRef,AstNodeRef> m_map;
};

AstNodeRef
ast_canonicalize_top_level_booleans_to_ite(AstNodeRef const& ast, context* ctx)
{
  AstCanonicalizeTopLevelBooleansVisitor vtor(ast, ctx);
	return vtor.get_result();
}

bool
sorts_are_compatible(sort_ref const& a, sort_ref const& b)
{
  return    array_constant::sorts_agree(a, b)
         || array_constant::sorts_agree(b, a);
}

void
validate_sort(ExprDual const& ed, sort_ref const& fn_sort)
{
  if (fn_sort->get_domain_sort().size()) {
    if (   !sorts_are_compatible(fn_sort, ed.normal_form()->get_sort())
        || !IMPLIES(ed.has_linear_form(), sorts_are_compatible(fn_sort, ed.linear_form()->get_sort()))) {
      cout << _FNLN_ << " fn_sort: " << fn_sort->to_string() << endl;
      cout << _FNLN_ << " ed.nf sort: " << ed.normal_form()->get_sort()->to_string() << endl;
      cout << _FNLN_ << " ed.nf = " << expr_string(ed.normal_form()) << endl;
      if (ed.has_linear_form()) {
        cout << _FNLN_ << " ed.lf sort: " << ed.linear_form()->get_sort()->to_string() << endl;
        cout << _FNLN_ << " ed.lf = " << expr_string(ed.linear_form()) << endl;
      }
    }
    ASSERT(sorts_are_compatible(fn_sort, ed.normal_form()->get_sort()));
    ASSERT(IMPLIES(ed.has_linear_form(), sorts_are_compatible(fn_sort, ed.linear_form()->get_sort())));
  } else {
    if (   !sorts_are_compatible(fn_sort->get_range_sort(), ed.normal_form()->get_sort())
        || !IMPLIES(ed.has_linear_form(), sorts_are_compatible(fn_sort->get_range_sort(), ed.linear_form()->get_sort()))) {
      cout << _FNLN_ << " fn_sort: " << fn_sort->to_string() << endl;
      cout << _FNLN_ << " ed.nf sort: " << ed.normal_form()->get_sort()->to_string() << endl;
      cout << _FNLN_ << " ed.nf = " << expr_string(ed.normal_form()) << endl;
      if (ed.has_linear_form()) {
        cout << _FNLN_ << " ed.lf sort: " << ed.linear_form()->get_sort()->to_string() << endl;
        cout << _FNLN_ << " ed.lf = " << expr_string(ed.linear_form()) << endl;
      }
    }
    ASSERT(sorts_are_compatible(fn_sort->get_range_sort(), ed.normal_form()->get_sort()));
    ASSERT(IMPLIES(ed.has_linear_form(), sorts_are_compatible(fn_sort->get_range_sort(), ed.linear_form()->get_sort())));
  }
}

ExprDual
ParsingEnv::linearize_func_ast_to_expr(AstNodeRef const& in_ast, sort_ref const& fn_sort) const
{
  //cout << _FNLN_ << ": in_ast size = " << ast_size(in_ast) << endl;
  //cout << _FNLN_ << ": in_ast dag_size = " << ast_dag_size(in_ast) << endl;
  AstNodeRef ast = in_ast;
  // fix quirks before linearization
  ast = ast_canonicalize_top_level_booleans_to_ite(ast, m_ctx);
  ast = ast_canonicalize_ite(ast);

  AstLinearizer linearizer(ast, m_ctx, *this);
  ExprDual ret = linearizer.get_result();

  validate_sort(ret, fn_sort);
  return ret;
}

}
