%option noyywrap

%{
#include "flex_util.h"
#include "y.tab.h"  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#define DBG std::cerr << "> [" << yytext << "]\n";
#define RULE(r) std::cerr<< "> R: " << r << " [" << yytext << "]\n";
#define BRACE_OPEN '{'
#define BRACE_CLOSE '}'
%}

string  \"[^\"\\]*\"
float   -?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?
integ   -?[0-9]+

%%

{string}    { DBG           yylval.AsText    = util::MakeString(yytext);  return STRING; }
{integ}     { RULE("int")   yylval.AsInteger = util::MakeInt(yytext);     return INT; }
{float}     { RULE("float") yylval.AsFloat   = util::MakeFloat(yytext);   return FLOAT; }
"{"         { DBG return BRACE_OPEN;  }
"}"         { DBG return BRACE_CLOSE; }
":"         { DBG return ':'; }
","         { DBG return ','; }
"\["        { DBG return '['; }
"\]"        { DBG return ']'; }
"true"      { RULE("true") yylval.AsBool = true; return BOOL; }
"false"     { RULE("false") yylval.AsBool = false; return BOOL; }
"null"      { RULE("null")  return NULL_VAL; }
\S          { DBG return WS; }
.      	    ;

%%