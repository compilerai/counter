#pragma once

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer z3NeoFlexLexer
#include <FlexLexer.h>
#endif

#include "z3_neo_model_y.hpp"

class Z3_Neo_Scanner : public yyFlexLexer {
public:

   Z3_Neo_Scanner(std::istream *in) : yyFlexLexer(in) { }
   virtual ~Z3_Neo_Scanner() { }

   //get rid of override virtual function warning
   using FlexLexer::yylex;

   int yylex(yy::Z3_Neo_Parser::semantic_type * const lval,
             yy::Z3_Neo_Parser::location_type *location);
};
