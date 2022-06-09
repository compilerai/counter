#pragma once

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer boolectorFlexLexer
#include <FlexLexer.h>
#endif

#include "boolector_model_y.hpp"

class boolector_Scanner : public yyFlexLexer {
public:

   boolector_Scanner(std::istream *in) : yyFlexLexer(in) { }
   virtual ~boolector_Scanner() { }

   //get rid of override virtual function warning
   using FlexLexer::yylex;

   int yylex(yy::boolector_Parser::semantic_type * const lval,
             yy::boolector_Parser::location_type *location);
};
