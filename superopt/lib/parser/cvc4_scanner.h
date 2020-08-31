#pragma once

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer cvc4FlexLexer
#include <FlexLexer.h>
#endif

#include "cvc4_model_y.hpp"

class CVC4_Scanner : public yyFlexLexer {
public:

   CVC4_Scanner(std::istream *in) : yyFlexLexer(in) { }
   virtual ~CVC4_Scanner() { }

   //get rid of override virtual function warning
   using FlexLexer::yylex;

   int yylex(yy::CVC4_Parser::semantic_type * const lval,
             yy::CVC4_Parser::location_type *location);
};
