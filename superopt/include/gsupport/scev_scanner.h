#pragma once

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer scevFlexLexer
#include <FlexLexer.h>
#endif

#include "scev_parse_y.hpp"

class Scev_Scanner : public yyFlexLexer {
public:

   Scev_Scanner(std::istream *in) : yyFlexLexer(in) { }
   virtual ~Scev_Scanner() { }

   //get rid of override virtual function warning
   using FlexLexer::yylex;

   int yylex(yy::Scev_Parser::semantic_type * const lval,
             yy::Scev_Parser::location_type *location);
};
