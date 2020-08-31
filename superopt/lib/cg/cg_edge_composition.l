%{
#include <stdio.h>
#include <iostream>
#include <string>

#define YY_DECL int cglex()
// #define cgtext yytext

#include "cg_edge_composition_y.hpp"
using namespace std;
//extern cglval;
//string str; int l;
%}

%option noyywrap
%option nounput

digit [0-9]
number [0-9]+
letter [A-Za-z]
whitespace [ \t\n]+
dot .

%%

epsilon                   { return LABEL_EPSILON; }
[0-9a-zA-Z\._%]+     { cglval.str = strdup(cgtext); return LABEL; }
"=>"	                        return ARROW;
"("	                        return LBRAC;
")"	                        return RBRAC;
"*"                             return STAR;
"+"                             return PLUS;
[ \t\n]+        ;       /*ignore whitespace */
.               {cout << "um token: " <<cgtext << endl; cglval.str = strdup(cgtext); return UNMATCHED;}
