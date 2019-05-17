
%{
#include <stdio.h>
#include <math.h>
void yyerror(const char *);
extern FILE *yyin;
extern FILE *yyout;
extern int yylex();
extern int yyparse();
#define DBG(TEXT) fprintf(stderr, "# %s\n", TEXT);

// Super lazy debug macro
#define DBG_F(...) { \
        fprintf(stderr, "# "); \
        fprintf(stderr, __VA_ARGS__ ); \
        fprintf(stderr, "\n"); \
    } 
%}

%union {
    int AsInteger;
    float AsFloat;
    char* AsText;
}

%token <AsText> STRING
%token WS

%%


object:
    '{' members '}'             { DBG("Object") }
    ;  

members:
    member                      { DBG("Last Member") }
    | member ',' members        { DBG("Multiple Members") }
    ;

member:
    STRING ':' element           { DBG_F("Member: %s", $1); }
    ;

array:
    '[' elements ']'            { DBG("Array") }
    ;

value:
    object                      { DBG_F("Value from object") }
    | STRING                    { DBG_F("Value: '%s'", $1) }
    ;

elements:
    element                     {}
    | element ',' elements      {}
    ;

element:
    value                       { DBG("Element") }
    | array                     { DBG("Array") }
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