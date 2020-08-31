#pragma once

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer z3FlexLexer
#include <FlexLexer.h>
#endif

#include "z3_model_y.hpp"

class Z3_Scanner : public yyFlexLexer {
public:

   Z3_Scanner(std::istream *in) : yyFlexLexer(in) { }
   virtual ~Z3_Scanner() { }

   //get rid of override virtual function warning
   using FlexLexer::yylex;

   int yylex(yy::Z3_Parser::semantic_type * const lval,
             yy::Z3_Parser::location_type *location);
};
