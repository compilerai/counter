%{
#include <cstdio>
#include <iostream>
#include <string>
#include "parser/parse_tree_yices.h"
#include "yices_model_y.hpp"

#define YY_DECL int yclex()
// #define lctext yytext
using namespace std;

//string str; int l;
%}

%option noyywrap

digit [0-9]
number {digit}+
letter [A-Za-z]
whitespace [ \t\n]+
connectors (%|\.|-|_)
func_name (_)?({letter})({letter}|{digit}|{connectors})*

%%

"@fun_"{number}	{
					yclval.str = strdup(yctext);
					return FUNC_NAME;
				}

"#b"[0-1]+	{
				yclval.str = strdup(yctext);
				return CONST;
			}
"true"          return TRUE;
"false"         return FALSE;
[-<>*+;/{}_]	return *yctext;

"function"	return DECL;
"default"	return DEFAULT;
"_ BitVec"{whitespace}[1-9][0-9]*	{
			string str = string(yctext);
			int l = str.find_last_not_of("0123456789");
			str = (str.substr(l + 1));
			yclval.num = atoi(str.c_str());
			return BV;
		}
"type"            return TYPE;
"("               return L_BRAC;
")"               return R_BRAC;
"Array"           return ARRAY;
"FunType"{number} return FUNTYPE;
"="               return EQ;
"sat"	            ;

{func_name}	{	yclval.str = strdup(yctext);
				return GLOBAL_FUNC_NAME;
			}
{whitespace}	;	/*Ignore Whitespace */

.	;
