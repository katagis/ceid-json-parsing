
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
// Retweet tokens
%token <AsText> F_RT_STATUS
%token <AsText> F_RT_TWEET
// Extended tweets tokens
%token <AsText> F_ET_DECLARATION
%token <AsText> F_ET_TRUNC
%token <AsText> F_ET_DISPRANGE // Display text range



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

// Special member is just a member but acts as a wrapper for the assignment specific rules.
%type <AsJMember> special_member
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
                                    DBG("IsUser: " << $2->Members.FormsValidUser(true) << " IsOuter: " << $2->Members.FormsValidOuterObject())
                                    $$ = $2;
                                }
    | '{' '}'                   { $$ = new JObject(); }
    ;  

members:
    member                      { $$ = new JObject(); $$->AddMember($1); }
    | member ',' members        { $$ = $3;            $$->AddMember($1); }
    ;

member:
    STRING ':' value            { $$ = new JMember($1, $3); }
    | special_member            { $$ = $1; }   
    ;


array:
    '[' values ']'              { $$ = $2; }
    | '[' ']'                   { $$ = new JArray(); }
    ;

values:
    value                       { $$ = new JArray(); $$->AddValue($1); }
    | value ',' values          { $$ = $3;           $$->AddValue($1); }
    ;

special_member:
    F_ID_STR ':' D_ID_STR       { 
                                    if (database.MaybeInsertIdStr($3)) {
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::IdStr); 
                                    }
                                    else {
                                        parse.ReportError("ID String already exists.");
                                        YYERROR;
                                    }
                                }
    | F_TEXT ':' STRING         { 
                                    JString* str = new JString($3);
                                    if (str->Length <= 142) { // The length of the example in the forums was 142 instead of 140.
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::Text); 
                                    }
                                    else {
                                        parse.ReportError("This text field is too long.");
                                        std::cout << "Length: " << str->Text.length() << "|" << str->Length << "/140\n";
                                        YYERROR;
                                    }
                                }
    | F_CREATEDAT ':' D_DATE    { $$ = new JMember($1, new JValue($3), JSpecialMember::CreatedAt); }
    | F_UNAME ':' STRING        { $$ = new JMember($1, new JValue($3), JSpecialMember::UName); }
    | F_USCREEN ':' STRING      { $$ = new JMember($1, new JValue($3), JSpecialMember::UScreenName); }
    | F_ULOCATION ':' STRING    { $$ = new JMember($1, new JValue($3), JSpecialMember::ULocation); }
    | F_UID ':' POS_INT         { 
                                    if (database.MaybeInsertUserId($3)) {
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::UId); 
                                    }
                                    else {
                                        parse.ReportError("User ID already exists.");
                                        YYERROR;
                                    }
                                }
    | F_USER ':' object         { 
                                    if ($3->Members.FormsValidUser(false)) {
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::User);
                                    }
                                    else {
                                        parse.ReportError("User ending here is missing fields. "
                                                          "All user objects must atleast include screen_name.");
                                        YYERROR;
                                    }
                                }
    | F_RT_STATUS ':' object    {
                                    // A "retweet_status" does not always need a "tweet" object.
                                    // but MUST have text and valid User Object
                                    if (!$3->Members.Text || !$3->Members.User) {
                                        parse.ReportError("Retweet status object ending here is invalid. "
                                                          "It is missing 'text' and/or 'user' field." );
                                        YYERROR;
                                    }

                                    // if there is a 'tweet' object we need to verify the username from RT @
                                    if ($3->Members.TweetObj) {
                                        
                                        // "user"->"ScreenName"->Text
                                        const std::string& OriginalTweetAuthor = $3->Members.User->Members.UScreenName->Text; 

                                        // "tweet"->"text" [@User]
                                        const std::string& RetweetAtFound = $3->Members.TweetObj->Members.Text->RetweetUser; 
                                        
                                        if (OriginalTweetAuthor != RetweetAtFound) { 
                                            parse.ReportError("Retweet status object ending here is invalid. RT @ user '" + RetweetAtFound 
                                                            + "' is not the same as the original tweet user. '" + OriginalTweetAuthor + "'");
                                            YYERROR;
                                        }
                                    }
                                    // this will only run if no YYERROR was run.
                                    $$ = new JMember($1, new JValue($3));
                                }
    | F_RT_TWEET ':' object     {
                                    if ($3->IsValidTweetObj()) {
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::TweetObj);
                                    }
                                    else {
                                        parse.ReportError("Tweet object ending here is invalid. "
                                                          "Tweet objects require 'text' field starting with 'RT @Username', and a valid 'user'.");
                                        YYERROR;
                                    }
                                }
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