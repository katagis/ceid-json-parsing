
%{
#include <stdio.h>
#include <math.h>
#include <iostream>
void yyerror(const char *);
extern FILE *yyin;
extern FILE *yyout;
extern int yylex();
extern int yyparse();

#define DBG(TEXT) std::cerr << "# " << TEXT << "\n";
%}


%union {
    int AsInteger;
    float AsFloat;
    char* AsText;
    bool AsBool;
}

%token <AsText> STRING
%token <AsFloat> FLOAT
%token <AsInteger> INT
%token <AsBool> BOOL
%token NULL_VAL
%token ERROR

%%
json: 
    element                     { DBG("JSON") }
    ;

value:
    object                      { DBG("Value from object") }
    | array                     { DBG("Value from array")  }
    | STRING                    { DBG("String ~" << $1) }
    | number                    {}
    | BOOL                      { DBG("Bool ~" << $1) }
    | NULL_VAL                  { DBG("Null ~ !") }
    ;

object:
    '{' members '}'             { DBG("Object") }
    | '{' '}'                   { DBG("Empty Object") }
    ;  

members:
    member                      { DBG("Last Member") }
    | member ',' members        { DBG("Multiple Members") }
    ;

member:
    STRING ':' element           { DBG("Member @" << $1) }
    ;

array:
    '[' elements ']'            { DBG("Array") }
    | '[' ']'                   { DBG("Empty Array") }
    ;

elements:
    element                     {}
    | element ',' elements      {}
    ;

element:
    value                       { DBG("Element") }
    ;

number:
    FLOAT                       { DBG("Float ~" << $1) }
    | INT                       { DBG("Integ ~" << $1) }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
}

void parse_args(int argc, char **argv);

int main (int argc, char **argv) {
    parse_args(argc, argv);
    yyparse();
    return 0;
}

void parse_args(int argc, char **argv) {
    yyin = stdin;
    yyout = stdout;

    if (argc > 1) {
        yyin = fopen(argv[1], "r");
    }
    if (argc > 2) {
        yyout = fopen(argv[2], "w");
    }
}