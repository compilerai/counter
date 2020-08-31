%{
#include <stdio.h>
#include <iostream>
#include <string>

#define YY_DECL int eglex()
// #define egtext yytext

#include "edge_guard_y.hpp"
using namespace std;
//extern eglval;
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

L[0-9a-zA-Z\.]+%[0-9]+%[0-9]+     { eglval.str = strdup(egtext); return LABEL_INTERNAL; }
E[0-9]+.[0-9]+.[0-9]+     { eglval.str = strdup(egtext); return LABEL_EXIT; }
epsilon                   { return LABEL_EPSILON; }
"=>"	                        return ARROW;
"("	                        return LBRAC;
")"	                        return RBRAC;
"*"                             return STAR;
"+"                             return PLUS;
[ \t\n]+        ;       /*ignore whitespace */
.               {cout << "um token: " <<egtext << endl; eglval.str = strdup(egtext); return UNMATCHED;}
