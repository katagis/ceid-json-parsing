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
#define MATCH parse.Match(yytext)
#define STORE_TXT yylval.AsText = util::MakeString(yytext)

// The string matcher extracts the whole quoted strings without doing any processing.
// The result is given to util::MakeString which in turn replaces all the character escapes
// This makes our grammar rules a lot simpler as we never have to mix strings

//date        "\"{literaldays} {literalmonths} {numerday} {time} ({timezone} )?{year}\""
%}

float   -?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?
space   [\t ]+
newline \r?\n

escape  n|\"|\\|\/|\u[0-9a-fA-F]{4}
string  \"([^\\\n\r\"]*(\\({escape}))?)*\"

errchar [^\"\[\]\,\:\{\}\r\n\t ]+


literaldays     (Sun|Mon|Tue|Wed|Thu|Fri|Sat)
literalmonths   (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)
numerday        ([0-2][0-9]|3[0-1])     
time            ([0-1][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]
timezone        [+-](0[0-9]|1[0-2])[0-5][0-9]
year            [0-9]{4}
date        \"{literaldays}[ ]{literalmonths}[ ]{numerday}[ ]{time}[ ]({timezone}[ ])?{year}\"

id_str      \"[0-9]+\"



%%

"\"id_str\""       { MATCH; STORE_TXT; return F_ID_STR; }
"\"text\""         { MATCH; STORE_TXT; return F_TEXT; }
"\"created_at\""   { MATCH; STORE_TXT; return F_CREATEDAT; }
"\"user\""         { MATCH; STORE_TXT; return F_USER; }
"\"name\""         { MATCH; STORE_TXT; return F_UNAME; }
"\"screen_name\""  { MATCH; STORE_TXT; return F_USCREEN; }
"\"location\""     { MATCH; STORE_TXT; return F_ULOCATION; }
"\"id\""           { MATCH; STORE_TXT; return F_UID; }

{id_str}    { MATCH; STORE_TXT; return D_ID_STR; }
{date}      { MATCH; STORE_TXT; return D_DATE; }

{string}    { MATCH; yylval.AsText    = util::MakeString(yytext);  return STRING; }
[0-9]+      { MATCH; yylval.AsInteger = util::MakeInt(yytext);     return POS_INT; }
-[0-9]+     { MATCH; yylval.AsInteger = util::MakeInt(yytext);     return NEG_INT; }
{float}     { MATCH; yylval.AsFloat   = util::MakeFloat(yytext);   return FLOAT; }
"{"         { MATCH; return BRACE_OPEN;  }
"}"         { MATCH; return BRACE_CLOSE; }
":"         { MATCH; return ':'; }
","         { MATCH; return ','; }
"\["        { MATCH; return '['; }
"\]"        { MATCH; return ']'; }
"true"      { MATCH; yylval.AsBool = true; return BOOL; }
"false"     { MATCH; yylval.AsBool = false; return BOOL; }
"null"      { MATCH; return NULL_VAL; }
{space}     { MATCH; parse.Match(yytext); }; // Consume all whitespaces
{newline}   {      ; parse.CountLine(); }

{errchar}   { MATCH; return INVALID_CHARACTER; } // See *2

%%
// TODO: Maybe fix this (has a conflict with another rule) and add it again.
// \".*\"      { MATCH return INVALID_CHARACTER; } // See *1 */
// *1
// This will match the whole or parts of incorrectly formatted strings.
// Even if the exact error is not shown its better than no match at all.
// 
// *2
// This will match invalid characters not in quotes.
//
// The other option for error reporing would be to make a rule for INVALID_CHARACTER in grammar
// until we match a specific delimiter and then produce the error.