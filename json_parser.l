%option noyywrap

%{
#include "flex_util.h"
#include "y.tab.h"  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBG fprintf(stderr, "> [%s]\n", yytext);
#define BRACE_OPEN '{'
#define BRACE_CLOSE '}'
%}

string  \"[^\"\\]*\"

%%

{string}    { DBG yylval.AsText = util::MakeString(yytext); return STRING; }
"{"         { DBG return BRACE_OPEN;  }
"}"         { DBG return BRACE_CLOSE; }
":"         { DBG return ':'; }
","         { DBG return ','; }
"\["        { DBG return '['; }
"\]"        { DBG return ']'; }
\S          { DBG return WS; }
.      	    ;

%%