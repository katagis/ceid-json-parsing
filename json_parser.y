
%{
#include <stdio.h>
#include <math.h>
void yyerror(const char *);
extern FILE *yyin;
extern FILE *yyout;
extern int yylex();
extern int yyparse();
%}

%token INT
%left '+'
%left '*'
%%

program: expr                   { fprintf(yyout, "%i\n", $1); }
    ;
                            
expr: INT
    | expr '+' expr             { $$ = $1 + $3; }
    | expr '*' expr             { $$ = $1 * $3; }
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