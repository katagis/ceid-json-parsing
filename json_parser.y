%{
#include "flex_util.h"

#include <stdio.h>
#include <math.h>
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
    boolean AsBool;
    JValue* AsJValue;
    JArray* AsJArray;
    JMember* AsJMember;
    JObject* AsJObject;
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
%type <AsJValue> json

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
                                  Print_JValue($$, 0);
                                  fprintf(OUT, "\n");
                                  
                                  Str_c Error = STR_make("The outer object was parsed properly but its not valid. Error was:\n");
                                  if (!FormsValidOuterObject($1->Data.ObjectData, &Error)) {
                                      PS_ReportError(Error.ptr);
                                      STR_clear(&Error);
                                      YYERROR;
                                  }
                                  else {
                                      fprintf(OUT, "Input was a complete and valid outer object.\n");
                                  }
                                }
    ;

value:
    object                      { $$ = JV_Object($1); }
    | array                     { $$ = JV_Array ($1); }
    | STRING                    { $$ = JV_String($1); }
    | FLOAT                     { $$ = JV_Float ($1); }
    | POS_INT                   { $$ = JV_Int   ($1); }
    | NEG_INT                   { $$ = JV_Int   ($1); }
    | BOOL                      { $$ = JV_Bool  ($1); }
    | NULL_VAL                  { $$ = JV_Null  ();   }
    | D_DATE                    { $$ = JV_String($1); }
    | D_ID_STR                  { $$ = JV_String($1); }
    | special_asvalues          { $$ = JV_String($1); }
    ;

object:
    '{' members '}'             { 
                                    $$ = $2;
                                }
    | '{' '}'                   { $$ = Alloc_JObject(); }
    ;  

members:
    member                      { $$ = Alloc_JObject(); AddMemberToObject($$, $1); }
    | member ',' members        { $$ = $3;              AddMemberToObject($$, $1); }
    ;

member:
    STRING ':' value            { $$ = Alloc_JMember($1, $3, E_None); }
    | special_member            { $$ = $1; }   
    ;

array:
    '[' values ']'              { $$ = $2; }
    | '[' ']'                   { $$ = Alloc_JArray(); }
    ;

values:
    value                       { $$ = Alloc_JArray(); VVP_add(&$$->Elements, $1); }
    | value ',' values          { $$ = $3;             VVP_add(&$$->Elements, $1); }
    ;

special_intrange: 
    '[' POS_INT ',' POS_INT ']' { 
                                    if ($2 > $4) {
                                        PS_ReportError("In the range ending here: Begin > End.");
                                        YYERROR;
                                    }
                                    $$ = Alloc_JArrayRange($2, $4); 
                                }
    ;

special_member:
    F_ID_STR ':' D_ID_STR       { 
                                    if (MaybeInsertIdStr($3)) {
                                        $$ = Alloc_JMember($1, JV_String($3), E_IdStr); 
                                    }
                                    else {
                                        PS_ReportError("ID String already exists.");
                                        YYERROR;
                                    }
                                }
    | F_TEXT ':' STRING         { 
                                    JString* str = Alloc_JString($3);
                                    if (str->Length <= ALLOWED_TEXT_LEN) {
                                        $$ = Alloc_JMember($1, JV_StrObj(str), E_Text); 
                                    }
                                    else {
                                        PS_ReportError("This text field is too long.");
                                        fprintf(OUT, "Length: %d/%d\n", str->Text.len, ALLOWED_TEXT_LEN);
                                        YYERROR;
                                    }
                                }
    | F_CREATEDAT ':' D_DATE    { $$ = Alloc_JMember($1, JV_String($3), E_CreatedAt); }
    | F_UNAME ':' STRING        { $$ = Alloc_JMember($1, JV_String($3), E_UName); }
    | F_USCREEN ':' STRING      { $$ = Alloc_JMember($1, JV_String($3), E_UScreenName); }
    | F_ULOCATION ':' STRING    { $$ = Alloc_JMember($1, JV_String($3), E_ULocation); }
    | F_UID ':' POS_INT         { 
                                    if (MaybeInsertUserId($3)) {
                                        $$ = Alloc_JMember($1, JV_Int($3), E_UId); 
                                    }
                                    else {
                                        PS_ReportError("User ID already exists.");
                                        YYERROR;
                                    }
                                }
    | F_USER ':' object         {
                                    int isValidUser = 0;
                                    
                                    // isValidUser =   // allo erwtima
                                    //        $3->Members.UName
                                    //     && $3->Members.UScreenName 
                                    //     && $3->Members.ULocation
                                    //     && $3->Members.UId;
                                    isValidUser = $3->Members.UScreenName != NULL;

                                    if (isValidUser) {
                                        $$ = Alloc_JMember($1, JV_Object($3), E_User);
                                    }
                                    else {
                                        PS_ReportError("User ending here is missing fields. "
                                                          "All user objects must atleast include screen_name.");
                                        YYERROR;
                                    }
                                }
    | F_RT_STATUS ':' object    {
                                    // A "retweet_status" does not always need a "tweet" object.
                                    // but MUST have text and valid User Object
                                    if (!$3->Members.Text || !$3->Members.User) {
                                        PS_ReportError("Retweet status object ending here is invalid. "
                                                          "It is missing 'text' and/or 'user' field." );
                                        YYERROR;
                                    }

                                    // if there is a 'tweet' object we need to verify the username from RT @
                                    if ($3->Members.TweetObj) {
                                        // "user"->"ScreenName"->Text
                                        const char* OriginalTweetAuthor = $3->Members.User->Members.UScreenName->Text.ptr; 

                                        // "tweet"->"text" [@User]
                                        const char* RetweetAtFound = $3->Members.TweetObj->Members.Text->RetweetUser.ptr; 
                                        if (strcmp(OriginalTweetAuthor, RetweetAtFound) != 0) {
                                            Str_c Error = STR_make("Retweet status object ending here is invalid. RT @ user '");
                                            STR_append(&Error, RetweetAtFound);
                                            STR_append(&Error, "' is not the same as the original tweet user. '");
                                            STR_append(&Error, OriginalTweetAuthor);
                                            STR_append(&Error, "'");
                                            PS_ReportError(Error.ptr);
                                            STR_clear(&Error);
                                            YYERROR;
                                        }
                                    }
                                    // this will only run if no YYERROR was run.
                                    $$ = Alloc_JMember($1, JV_Object($3), E_None);
                                }
    | F_RT_TWEET ':' object     {
                                    if ($3->Members.Text 
                                        && $3->Members.User 
                                        && $3->Members.Text->RetweetUser.len) {
                                            
                                        $$ = Alloc_JMember($1, JV_Object($3), E_TweetObj);
                                    }
                                    else {
                                        PS_ReportError("Tweet object ending here is invalid. "
                                            "Tweet objects require 'text' field starting with 'RT @Username', and a valid 'user'.");
                                        YYERROR;
                                    }
                                }
    | F_ET_DECLARATION ':' object   { 
                                        Str_c Error = STR_make("Extended tweet object ending here is invalid: ");
                                        if (!FormsValidExtendedTweetObj($3, &Error)) {
                                            PS_ReportError(Error.ptr);
                                            STR_clear(&Error);
                                            YYERROR;
                                        }
                                        $$ = Alloc_JMember($1, JV_Object($3), E_ExTweet); 
                                    }
    | F_ET_TRUNC ':' BOOL           { $$ = Alloc_JMember($1, JV_Bool($3), E_Truncated); }
    | F_ET_DISPLAYRANGE ':' special_intrange { $$ = Alloc_JMember($1, JV_Array($3), E_DisplayRange); }
    | F_ET_ENTITIES ':' object      { 
                                        if (!$3->ExMembers.Hashtags) {
                                            PS_ReportError("Entities object ending here is missing a 'hashtags' member.");
                                            YYERROR;
                                        }
                                        $$ = Alloc_JMember($1, JV_Object($3), E_Entities); 
                                    }
    | F_ET_HASHTAGS ':' array       { 
                                        Str_c Error = STR_make("Array ending here is not a valid hastags array: ");
                                        boolean IsValidArray = ExtractHashtags($3, &Error);
                                        if (!IsValidArray) {
                                            PS_ReportError(Error.ptr);
                                            STR_clear(&Error);
                                            YYERROR;
                                        }
                                        $$ = Alloc_JMember($1, JV_Array($3), E_Hashtags); 
                                    }
    | F_ET_INDICES ':' special_intrange { $$ = Alloc_JMember($1, JV_Array($3), E_Indices); }
    | F_ET_FULLTEXT ':' STRING      { 
                                        JString* str = Alloc_JString($3);
                                        if (str->Length <= ALLOWED_FULLTEXT_LEN) {
                                            $$ = Alloc_JMember($1, JV_StrObj(str), E_FullText); 
                                        }
                                        else {
                                            char Error[200];
                                            sprintf(Error, "'full_text' is too long: %d/%d", str->Length, ALLOWED_FULLTEXT_LEN);
                                            PS_ReportError(Error);
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
    PS_ReportError(s);
}

void parse_args(int argc, char **argv);

void init() {
    PS_Init();
    database.UserIds.slack = 0;
    database.IdStrs.slack = 0;
}

void freeall() {
    PS_Free();
    free(database.UserIds.data);
    free(database.IdStrs.data);
}

int main (int argc, char **argv) {
    init(); 
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
}