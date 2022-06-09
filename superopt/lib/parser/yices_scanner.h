#pragma once

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer yicesFlexLexer
#include <FlexLexer.h>
#endif

#include "yices_model_y.hpp"

class Yices_Scanner : public yyFlexLexer {
public:

   Yices_Scanner(std::istream *in) : yyFlexLexer(in) { }
   virtual ~Yices_Scanner() { }

   //get rid of override virtual function warning
   using FlexLexer::yylex;

   int yylex(yy::Yices_Parser::semantic_type * const lval,
             yy::Yices_Parser::location_type *location);
};
