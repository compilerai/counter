#pragma once

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer exprFlexLexer
#include <FlexLexer.h>
#endif

#include "expr_parse_y.hpp"

class Expr_Scanner : public yyFlexLexer {
public:

   Expr_Scanner(std::istream *in) : yyFlexLexer(in) { }
   virtual ~Expr_Scanner() { }

   //get rid of override virtual function warning
   using FlexLexer::yylex;

   int yylex(yy::Expr_Parser::semantic_type * const lval,
             yy::Expr_Parser::location_type *location);
};
