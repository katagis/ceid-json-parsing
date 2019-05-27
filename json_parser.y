
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

JsonDB database;

#define DBG(TEXT) std::cerr << "# " << TEXT << "\n";
%}

%error-verbose

%union {
    long long AsInteger;
    float AsFloat;
    char* AsText;
    bool AsBool;
    JValue* AsJValue;
    JArray* AsJArray;
    JMember* AsJMember;
    JObject* AsJObject;
    JJson* AsJJson;
}

%token <AsText> STRING
%token <AsFloat> FLOAT
%token <AsInteger> POS_INT
%token <AsInteger> NEG_INT
%token <AsBool> BOOL
%token NULL_VAL
%token INVALID_CHARACTER

// Our custom field declarations. 
// Programmatically they are text but we have some extra grammar for them.
%token <AsText> F_ID_STR
%token <AsText> F_TEXT
%token <AsText> F_CREATEDAT
%token <AsText> F_USER
%token <AsText> F_UNAME
%token <AsText> F_USCREEN
%token <AsText> F_ULOCATION
%token <AsText> F_UID

// Custom field Data.
%token <AsText> D_ID_STR
%token <AsText> D_DATE


%type <AsJValue> value
%type <AsJArray> array
%type <AsJMember> member
%type <AsJObject> object
%type <AsJJson> json

%type <AsJArray> values
%type <AsJObject> members


%%
json: 
    value                       { 
                                  //DBG("JSON PARSED") 
                                  $$ = new JJson($1); 
                                  $$->Print(std::cout);
                                }
    ;

value:
    object                      { $$ = new JValue($1); }
    | array                     { $$ = new JValue($1); }
    | STRING                    { $$ = new JValue($1); }
    | FLOAT                     { $$ = new JValue($1); }
    | POS_INT                   { $$ = new JValue($1); }
    | NEG_INT                   { $$ = new JValue($1); }
    | BOOL                      { $$ = new JValue($1); }
    | NULL_VAL                  { $$ = new JValue();   }
    | D_DATE                    { $$ = new JValue($1); }
    | D_ID_STR                  { $$ = new JValue($1); }
    ;

object:
    '{' members '}'             { 
                                    DBG("IsUser: " << $2->IsValidUser() << " IsOuter: " << $2->IsValidOuter())
                                    $$ = $2;
                                }
    | '{' '}'                   { $$ = new JObject(); }
    ;  

members:
    member                      { $$ = new JObject(); $$->AddMember($1); }
    | member ',' members        { $$ = $3;            $$->AddMember($1); }
    ;

member:
    STRING ':' value              { $$ = new JMember($1, $3); }
    | F_ID_STR ':' D_ID_STR       { 
                                    if (database.MaybeInsertIdStr($3)) {
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::IdStr); 
                                    }
                                    else {
                                        parse.ReportError("ID String already exists.");
                                        YYERROR;
                                    }
                                  }
    | F_TEXT ':' STRING           { $$ = new JMember($1, new JValue($3), JSpecialMember::Text); }
    | F_CREATEDAT ':' D_DATE      { $$ = new JMember($1, new JValue($3), JSpecialMember::CreatedAt); }
    | F_UNAME ':' STRING          { $$ = new JMember($1, new JValue($3), JSpecialMember::UName); }
    | F_USCREEN ':' STRING        { $$ = new JMember($1, new JValue($3), JSpecialMember::UScreenName); }
    | F_ULOCATION ':' STRING      { $$ = new JMember($1, new JValue($3), JSpecialMember::ULocation); }
    | F_UID ':' POS_INT           { 
                                    if (database.MaybeInsertUserId($3)) {
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::UId); 
                                    }
                                    else {
                                        parse.ReportError("User ID already exists.");
                                        YYERROR;
                                    }
                                  }
    | F_USER ':' object           { 
                                    if ($3->IsValidUser()) {
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::User);
                                    }
                                    else {
                                        parse.ReportError("User ending here is missing fields.");
                                        YYERROR;
                                    }
                                  }

array:
    '[' values ']'              { $$ = $2; }
    | '[' ']'                   { $$ = new JArray(); }
    ;

values:
    value                       { $$ = new JArray(); $$->AddValue($1); }
    | value ',' values          { $$ = $3;           $$->AddValue($1); }
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