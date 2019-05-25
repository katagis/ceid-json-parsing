
%{
#include "flex_util.h"
#include "json_classes.h"

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

%error-verbose

%union {
    int AsInteger;
    float AsFloat;
    char* AsText;
    bool AsBool;
    JValue* AsJValue;
    JArray* AsJArray;
    JMember* AsJMember;
    JObject* AsJObject;
    JJson* AsJJson;
    std::vector<JValue*>* AsIntermValues;
    std::vector<JMember*>* AsIntermMembers;
}

%token <AsText> STRING
%token <AsFloat> FLOAT
%token <AsInteger> INT
%token <AsBool> BOOL
%token NULL_VAL
%token INVALID_CHARACTER

%type <AsJValue> value
%type <AsJArray> array
%type <AsJMember> member
%type <AsJObject> object
%type <AsJJson> json

%type <AsIntermValues> values
%type <AsIntermMembers> members



%%
json: 
    value                       { 
                                  //DBG("JSON PARSED") 
                                  $$ = new JJson($1); 
                                  $$->Print(std::cout);
                                }
    ;

value:
    object                      { $$ = new JValue($1);  }
    | array                     { $$ = new JValue($1);  }
    | STRING                    { $$ = new JValue($1); }
    | FLOAT                     { $$ = new JValue($1);  }
    | INT                       { $$ = new JValue($1);  }
    | BOOL                      { $$ = new JValue($1);  }
    | NULL_VAL                  { $$ = new JValue();    }
    ;

object:
    '{' members '}'             { $$ = new JObject(*$2); }
    | '{' '}'                   { $$ = new JObject(   ); }
    ;  

members:
    member                      { $$ = new std::vector<JMember*>(); $$->push_back($1); }
    | member ',' members        { $3->push_back($1); $$ = $3; }
    ;

member:
    STRING ':' value            { $$ = new JMember($1, $3); }
    ;

array:
    '[' values ']'              { $$ = new JArray(*$2);}
    | '[' ']'                   { $$ = new JArray(   ); }
    ;

values:
    value                       { $$ = new std::vector<JValue*>(); $$->push_back($1); }
    | value ',' values          { $3->push_back($1); $$ = $3; }
    ;

%%

void yyerror(const char *s) {
    parse.ReportError(s);
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