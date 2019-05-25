%option noyywrap

%{
#include "flex_util.h"
#include "json_classes.h"
ParserState parse;

#include "y.tab.h"  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#define DBG std::cerr << "> [" << yytext << "]\n";
#define RULE(r) std::cerr << "> R: " << r << " [" << yytext << "]\n";

// Used as workaround for editor highlighter bugs
#define BRACE_CLOSE '}'
#define BRACE_OPEN  '{'

// Use everywhere to report the parse match to our parser state
#define MATCH parse.Match(yytext);

// The string matcher extracts the whole quoted strings without doing any processing.
// The result is given to util::MakeString which in turn replaces all the character escapes
// This makes our grammar rules a lot simpler as we never have to mix strings
%}

float   -?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?
integ   -?[0-9]+
space   [\t ]+
newline \r?\n

escape  n|\"|\\|\/|\u[0-9a-fA-F]{4}
string  \"([^\\\n\r\"]*(\\({escape}))?)*\"

errchar [^\"\[\]\,\:\{\}\r\n\t ]+
%%

{string}    { MATCH yylval.AsText    = util::MakeString(yytext);  return STRING; }
{integ}     { MATCH yylval.AsInteger = util::MakeInt(yytext);     return INT; }
{float}     { MATCH yylval.AsFloat   = util::MakeFloat(yytext);   return FLOAT; }
"{"         { MATCH return BRACE_OPEN;  }
"}"         { MATCH return BRACE_CLOSE; }
":"         { MATCH return ':'; }
","         { MATCH return ','; }
"\["        { MATCH return '['; }
"\]"        { MATCH return ']'; }
"true"      { MATCH yylval.AsBool = true; return BOOL; }
"false"     { MATCH yylval.AsBool = false; return BOOL; }
"null"      { MATCH return NULL_VAL; }
{space}     { MATCH parse.Match(yytext); }; // Consume all whitespaces
{newline}   {       parse.CountLine(); }
\".*\"      { MATCH return INVALID_CHARACTER; } // See *1
{errchar}   { MATCH return INVALID_CHARACTER; } // See *2

%%
// *1
// This will match the whole or parts of incorrectly formatted strings.
// Even if the exact error is not shown its better than no match at all.
// 

// *2
// This will match invalid characters not in quotes.
