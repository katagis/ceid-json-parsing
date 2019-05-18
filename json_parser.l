%option noyywrap

%{
#include "flex_util.h"
#include "y.tab.h"  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#define DBG std::cerr << "> [" << yytext << "]\n";
#define RULE(r) std::cerr << "> R: " << r << " [" << yytext << "]\n";
#define BRACE_OPEN '{'
#define BRACE_CLOSE '}'
%}

string  \"[^\"\\]*\"
float   -?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?
integ   -?[0-9]+
space   [\n\r\t ]+

%%

{string}    { yylval.AsText    = util::MakeString(yytext);  return STRING; }
{integ}     { yylval.AsInteger = util::MakeInt(yytext);     return INT; }
{float}     { yylval.AsFloat   = util::MakeFloat(yytext);   return FLOAT; }
"{"         { return BRACE_OPEN;  }
"}"         { return BRACE_CLOSE; }
":"         { return ':'; }
","         { return ','; }
"\["        { return '['; }
"\]"        { return ']'; }
"true"      { yylval.AsBool = true; return BOOL; }
"false"     { yylval.AsBool = false; return BOOL; }
"null"      { return NULL_VAL; }
{space}     ; // Consume all whitespaces
.      	    { return ERROR; }

%%