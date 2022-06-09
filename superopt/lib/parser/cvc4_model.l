%{
/* Implementation of yyFlexScanner */
#include <string>
#include <sstream>
#include "support/bv_const.h"
#include "parser/parse_tree_cvc4.h"
#include "parser/cvc4_scanner.h"

#undef  YY_DECL
#define YY_DECL int CVC4_Scanner::yylex( yy::CVC4_Parser::semantic_type * const yylval, yy::CVC4_Parser::location_type *loc)

/* typedef to make the returns for the tokens shorter */
using token = yy::CVC4_Parser::token;

/* return EOD on exit */
/* #define yyterminate() return(token::END) */

/* update location on token matching */
#define YY_USER_ACTION loc->columns(yyleng);
%}

%option c++
%option prefix="cvc4"
%option nodefault
%option yyclass="CVC4_Scanner"
%option noyywrap

digit [0-9]
number {digit}+
letter [A-Za-z]
connectors (%|\.|-|_|!|=|\/)
func_name (_)?({letter})({letter}|{digit}|{connectors})*
WS  [ \t\v\f]

%%

%{
  /* this is run each time yylex is called */
  loc->step();
%}

"sat"          ;
"model"	       { return token::MODEL; }

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

"BOUND_VARIABLE_"{number}	{
  yylval->build<string>(yytext);
  return token::PARAMVAR;
}

"_arg_"{func_name}	{
  yylval->build<string>(yytext);
  return token::PARAMVAR;
}

"k!"{number}  {
  yylval->build<string>(yytext);
  return token::INTERNAL_FUNC_NAME;
}

"a!"{number} {
  yylval->build<string>(yytext);
  return token::LETVAR;
}

"_ bv"{number}{WS}*{number}	{
  istringstream iss(yytext);
  char underscore;
  string bvstr;
  size_t bvlen;
  iss >> underscore >> bvstr >> bvlen;
  const char* cbvstr = bvstr.c_str()+2; // skip prefix "bv"
  yylval->build<pair<size_t,bv_const>>(make_pair(bvlen, bv_const(cbvstr,10)));
  return token::BVCONST;
}

"#x"[0-9a-f]+ { yylval->build<pair<size_t,bv_const>>(str_to_bvlen_val_pair(yytext)); return token::BVCONST; }
"#b"[0-1]+    { yylval->build<pair<size_t,bv_const>>(str_to_bvlen_val_pair(yytext)); return token::BVCONST; }

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
