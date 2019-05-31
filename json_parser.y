
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

#define ALLOWED_TEXT_LEN 140

// The assignment mentioned 140 length for full_text too. 
// We assumed a value of our own to pass the test cases.
#define ALLOWED_FULLTEXT_LEN 800

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

// Custom field declarations. 
// Programmatically they are just text but we have some extra grammar for them.
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
%token <AsText> F_ET_DISPLAYRANGE // Display text range
%token <AsText> F_ET_ENTITIES
%token <AsText> F_ET_HASHTAGS
%token <AsText> F_ET_INDICES
%token <AsText> F_ET_FULLTEXT



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

// An array with just 2 ints. used as grammar rule to help parsing the array ranges for hastags and indices
%type <AsJArray> special_intrange 

%type <AsText> special_asvalues 


%%
json: 
    value                       { 
                                  //DBG("JSON PARSED") 
                                  $$ = new JJson($1); 
                                  $$->Print(std::cout);
                                  std::string Error = "The outer object was parsed properly but its not valid. Error was:\n";
                                  if (!$1->Data.ObjectData->FormsValidOuterObject(Error)) {
                                      parse.ReportError(Error);
                                      YYERROR;
                                  }
                                  else {
                                      std::cout << "Input was a complete and valid outer object.\n";
                                  }
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
    | special_asvalues          { $$ = new JValue($1); }
    ;

object:
    '{' members '}'             { 
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

special_intrange: 
    '[' POS_INT ',' POS_INT ']' { 
                                    if ($2 > $4) {
                                        parse.ReportError("In the range ending here: Begin > End.");
                                        YYERROR;
                                    }
                                    $$ = new JArray($2, $4); 
                                }
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
                                    if (str->Length <= ALLOWED_TEXT_LEN) {
                                        $$ = new JMember($1, new JValue(str), JSpecialMember::Text); 
                                    }
                                    else {
                                        parse.ReportError("This text field is too long.");
                                        std::cout << "Length: " << str->Text.length() << "/142\n";
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
                                    if ($3->FormsValidRetweetObj()) {
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::TweetObj);
                                    }
                                    else {
                                        parse.ReportError("Tweet object ending here is invalid. "
                                                          "Tweet objects require 'text' field starting with 'RT @Username', and a valid 'user'.");
                                        YYERROR;
                                    }
                                }
    | F_ET_DECLARATION ':' object   { 
                                        std::string Error = "Extended tweet object ending here is invalid: ";
                                        if (!$3->FormsValidExtendedTweetObj(Error)) {
                                            parse.ReportError(Error);
                                            YYERROR;
                                        }
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::ExTweet); 
                                    }
    | F_ET_TRUNC ':' BOOL           { $$ = new JMember($1, new JValue($3), JSpecialMember::Truncated); }
    | F_ET_DISPLAYRANGE ':' special_intrange { $$ = new JMember($1, new JValue($3), JSpecialMember::DisplayRange); }
    | F_ET_ENTITIES ':' object      { 
                                        if (!$3->ExMembers.Hashtags) {
                                            parse.ReportError("Entities object ending here is missing a 'hashtags' member.");
                                            YYERROR;
                                        }
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::Entities); 
                                    }
    | F_ET_HASHTAGS ':' array       { 
                                        std::string Error = "Array ending here is not a valid hastags array: ";
                                        bool IsValidArray = $3->ExtractHashtags(Error);
                                        if (!IsValidArray) {
                                            parse.ReportError(Error);
                                            YYERROR;
                                        }
                                        $$ = new JMember($1, new JValue($3), JSpecialMember::Hashtags); 
                                    }
    | F_ET_INDICES ':' special_intrange { $$ = new JMember($1, new JValue($3), JSpecialMember::Indices); }
    | F_ET_FULLTEXT ':' STRING      { 
                                        JString* str = new JString($3);
                                        if (str->Length <= ALLOWED_FULLTEXT_LEN) {
                                            $$ = new JMember($1, new JValue(str), JSpecialMember::FullText); 
                                        }
                                        else {
                                            parse.ReportError("'full_text' is too long: " + std::to_string(str->Length) +
                                                              "/" + std::to_string(ALLOWED_FULLTEXT_LEN));
                                            YYERROR;
                                        }
                                    }
    ;

special_asvalues:
    F_ID_STR              { $$ = $1; }
    | F_TEXT              { $$ = $1; }
    | F_CREATEDAT         { $$ = $1; }
    | F_UNAME             { $$ = $1; }
    | F_USCREEN           { $$ = $1; }
    | F_ULOCATION         { $$ = $1; }
    | F_UID               { $$ = $1; }
    | F_USER              { $$ = $1; }
    | F_RT_STATUS         { $$ = $1; }
    | F_RT_TWEET          { $$ = $1; }
    | F_ET_DECLARATION    { $$ = $1; }
    | F_ET_TRUNC          { $$ = $1; }
    | F_ET_DISPLAYRANGE   { $$ = $1; }
    | F_ET_ENTITIES       { $$ = $1; }
    | F_ET_HASHTAGS       { $$ = $1; }
    | F_ET_INDICES        { $$ = $1; }
    | F_ET_FULLTEXT       { $$ = $1; }
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