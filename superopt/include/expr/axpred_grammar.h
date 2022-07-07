#pragma once


#include "boost/fusion/include/std_pair.hpp"
#include "boost/spirit/home/x3.hpp"

#include "expr/context.h"
#include "expr/expr.h"

namespace eqspace {
namespace axpred {

namespace grammar {

namespace x3 = boost::spirit::x3;
namespace fu = boost::fusion;

context *GContext = nullptr;

// Symbol tables
struct RuleOpST : x3::symbols<std::pair<std::string, expr::operation_kind>> {
  RuleOpST() {
    addNonAxPred("eq", expr::OP_EQ);
    addNonAxPred("apply", expr::OP_APPLY);
    addNonAxPred("select", expr::OP_SELECT);
    addNonAxPred("not", expr::OP_NOT);
    addNonAxPred("bvadd", expr::OP_BVADD);
    addNonAxPred("bvsub", expr::OP_BVSUB);
    addNonAxPred("and", expr::OP_AND);
    addNonAxPred("or", expr::OP_OR);
  }

  void addAxPred(const std::string &Name) {
    add(Name, {Name, expr::OP_AXPRED});
  }

  void addNonAxPred(const std::string &Name, expr::operation_kind Op) {
    add(Name, {Name, Op});
  }
} RuleOpST_;

// Actions
const auto addAxPredName = [](auto &Ctx) -> void {
  RuleOpST_.addAxPred(x3::_attr(Ctx));
  x3::_val(Ctx) = x3::_attr(Ctx);
};

const auto createVarExpr = [](auto &Ctx) -> void {
  const std::string &Name = x3::_attr(Ctx);
  x3::_val(Ctx) = GContext->mk_var(Name, GContext->mk_unresolved_sort());
};

const auto createOpExpr = [](auto &Ctx) -> void {
  const std::string &Name = fu::at_c<0>(x3::_attr(Ctx)).first;
  expr::operation_kind Op = fu::at_c<0>(x3::_attr(Ctx)).second;
  expr_vector Args = fu::at_c<1>(x3::_attr(Ctx));
  if (Op == expr::OP_AXPRED) {
    x3::_val(Ctx) = GContext->mk_axpred(mk_axpred_desc_ref(Name), Args);
  } else {
    x3::_val(Ctx) = GContext->mk_app_unresolved(Op, Args);
  }
};

// Rule declarations
const x3::rule<class Name, std::string> Name("Name");
const x3::rule<class AxPredName, std::string> AxPredName("AxPredName");
const x3::rule<class VarName, std::string> VarName("VarName");
const x3::rule<class ExprRef, expr_ref> ExprRef("ExprRef");

// Rule definitions
// clang-format off
const auto Name_def = x3::lexeme[x3::alpha >> *(x3::alnum | x3::char_('_'))];

const auto AxPredName_def = Name[addAxPredName];

const auto VarName_def = Name;

const auto ExprRef_def = (RuleOpST_ >> x3::lit('(') >> (ExprRef % x3::lit(',')) >> x3::lit(')'))[createOpExpr] |
                        VarName[createVarExpr];
// clang-format on

BOOST_SPIRIT_DEFINE(Name, AxPredName, VarName, ExprRef);

} // namespace grammar

} // namespace axpred
} // namespace eqspace
