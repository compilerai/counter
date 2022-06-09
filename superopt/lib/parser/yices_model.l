%{
/* Implementation of yyFlexScanner */
#include <string>
#include <sstream>
#include "support/bv_const.h"
#include "parser/parse_tree_yices.h"
#include "parser/yices_scanner.h"

#undef  YY_DECL
#define YY_DECL int Yices_Scanner::yylex( yy::Yices_Parser::semantic_type * const yylval, yy::Yices_Parser::location_type *loc)

/* typedef to make the returns for the tokens shorter */
using token = yy::Yices_Parser::token;

/* return EOD on exit */
/* #define yyterminate() return(token::END) */

/* update location on token matching */
#define YY_USER_ACTION loc->columns(yyleng);
%}

%option c++
%option prefix="yices"
%option nodefault
%option yyclass="Yices_Scanner"
%option noyywrap

digit [0-9]
number {digit}+
letter [A-Za-z]
connectors (%|\.|-|_|!|=|\/)
func_name (_)?({letter})({letter}|{digit}|{connectors})*
internal_func_name @fun_{number}+
WS  [ \t\v\f]

%%

%{
  /* this is run each time yylex is called */
  loc->step();
%}

"sat"          ;
"model"	       ;

"#b"[0-1]+    { yylval->build<pair<size_t,bv_const>>(str_to_bvlen_val_pair(yytext)); return token::BVCONST; }
[-<>*+;/{}_]  { return *yytext; }

"_ BitVec"	{
	return token::BV;
}

{number}   {
	string str = string(yytext);
  yylval->build<int>(stoi(str));
  return token::NUM;
}

"function"	   { return token::DECL; }
"type"	       { return token::TYPE; }
"default"	     { return token::DEFAULT; }
"Bool"	       { return token::BOOL; }
"Array"	       { return token::ARRAY; }
"("			       { return token::L_BRAC; }
")"			       { return token::R_BRAC; }
"="	           { return token::EQ;  }
"true"         { return token::TRUE;  }
"false"        { return token::FALSE; }

{internal_func_name}	{
	yylval->build<string>(yytext);
	return token::INTERNAL_FUNC_NAME;
}

{func_name}	{
  yylval->build<string>(yytext);
  return token::GLOBAL_FUNC_NAME;
}

{WS}+				{ loc->step(); }
[\n]+				{ loc->lines(yyleng); loc->step(); }
.           { /*Ignore everything else */ }

%%
