%{
/* Implementation of yyFlexScanner */
#include <string>
#include <sstream>
#include "support/bv_const.h"
#include "parser/parse_tree_z3.h"
#include "parser/z3_neo_scanner.h"

#undef  YY_DECL
#define YY_DECL int Z3_Neo_Scanner::yylex( yy::Z3_Neo_Parser::semantic_type * const yylval, yy::Z3_Neo_Parser::location_type *loc)

/* typedef to make the returns for the tokens shorter */
using token = yy::Z3_Neo_Parser::token;

/* return EOD on exit */
/* #define yyterminate() return(token::END) */

/* update location on token matching */
#define YY_USER_ACTION loc->columns(yyleng);
%}

%option c++
%option prefix="z3Neo"
%option nodefault
%option yyclass="Z3_Neo_Scanner"
%option noyywrap

digit [0-9]
number {digit}+
letter [A-Za-z]
connectors (%|\.|-|_|!|=|\/)
func_name (_)?({letter})({letter}|{digit}|{connectors})*
internal_name {func_name}(!{number}*)+
WS  [ \t\v\f]

%%

%{
  /* this is run each time yylex is called */
  loc->step();
%}

"sat"          ;
"model"	       ;

"(define-fun axpred." {
  int par_balance = 1;
  while (par_balance) {
    char c = yyinput();
    if (c == '(') {
      ++par_balance;
    } else if(c == ')')
      --par_balance;
  }
}

{internal_name}	{
	yylval->build<string>(yytext);
	return token::INTERNAL_NAME;
}

"#x"[0-9a-f]+ { yylval->build<pair<size_t,bv_const>>(str_to_bvlen_val_pair(yytext)); return token::BVCONST; }
"#b"[0-1]+    { yylval->build<pair<size_t,bv_const>>(str_to_bvlen_val_pair(yytext)); return token::BVCONST; }
[-<>*+;/{}_]  { return *yytext; }

"_ BitVec"	{
	  //string str = string(yytext);
	  //int l = str.find_last_not_of("0123456789");
	  //str = str.substr(l + 1);
    //yylval->build<int>(stoi(str));
	  return token::BV;
  }

"_ FloatingPoint"	{
	  //string str = string(yytext);
	  //int l = str.find_last_not_of("0123456789");
	  //str = str.substr(l + 1);
    //yylval->build<int>(stoi(str));
	  return token::FLOATINGPOINT;
  }

"fp"	{
	  return token::FP;
  }

"_ +zero" {
    return token::FP_POSITIVE_ZERO;
  }

"_ -zero" {
    return token::FP_NEGATIVE_ZERO;
  }

"_ NaN" {
    return token::FP_NAN_IDENTIFIER;
  }

"_ +oo" {
    return token::FP_POSITIVE_INFINITY;
  }

"_ -oo" {
    return token::FP_NEGATIVE_INFINITY;
  }

{number}   {
	  string str = string(yytext);
	  //int l = str.find_last_not_of("0123456789");
	  //str = str.substr(l + 1);
    yylval->build<int>(stoi(str));
    return token::NUM;
  }

"_ extract"{WS}+{number}{WS}+{number}	{
  istringstream iss(yytext);
  char underscore;
  string extract;
  int arg1, arg2;
  iss >> underscore >> extract >> arg1 >> arg2;
  yylval->build<pair<int,int>>(make_pair(arg1, arg2));
  return token::BVEXTRACT;
}

"define-fun"	 { return token::DECL; }
"Bool"	       { return token::BOOL; }
"Array"	       { return token::ARRAY; }
"("			       { return token::L_BRAC; }
")"			       { return token::R_BRAC; }
"_ as-array"	 { return token::ARRAY_INTPR; }
"as const"     { return token::AS_CONST; }
"select"       { return token::SELECT; }
"store"        { return token::STORE; }
"let"		       { return token::LET; }
"lambda"       { return token::LAMBDA; }
"forall"       { return token::FORALL; }
"bvnot"        { return token::BVNOT; }
"bvadd"        { return token::BVADD; }
"bvmul"        { return token::BVMUL; }
"bvsle"        { return token::BVSLE; }
"bvshl"        { return token::BVSHL; }
"bvsdiv_i"     { return token::BVSDIV_I; }
"bvudiv_i"     { return token::BVUDIV_I; }
"bvule"        { return token::BVULE; }
"and"          { return token::AND; }
"or"           { return token::OR;  }
"not"          { return token::NOT; }
"xor"          { return token::XOR; }
"="	           { return token::EQ;  }
"ite"          { return token::ITE; }
"true"         { return token::TRUE;  }
"false"        { return token::FALSE; }
"concat"       { return token::CONCAT; }

{func_name}	{
  yylval->build<string>(yytext);
  return token::GLOBAL_FUNC_NAME;
}

{WS}+				{ loc->step(); }
[\n]+				{ loc->lines(yyleng); loc->step(); }
"=>"	{
	cerr << "IMPLIES encountered" << endl;
	exit(0);
}
.  { /*Ignore everything else */ }

%%
