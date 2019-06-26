#ifndef __JSON_CLASSES_
#define __JSON_CLASSES_

#define OUT stdout
#include "c_comp.h"

struct JObject;
struct JArray;
struct JString;

// Global DB keeping track of ids
typedef struct JsonDB {
    Vec_Str IdStrs;
    Vec_LongLong UserIds;
} JsonDB;

extern JsonDB database;
// Attempts to Insert an id_str element in the database. Returns false if it already existed
boolean MaybeInsertIdStr(const char* id_str);
// Attempts to Insert a user_id element in the database. Returns false if it already existed
boolean MaybeInsertUserId(long long id);


enum JValueType {
    E_Object,
    E_Array,
    E_String,
    E_Float,
    E_Int,
    E_Bool,
    E_NullVal
};

struct JObject;
struct JArray;
struct JString;

union JValueData {
    struct JObject* ObjectData;
    struct JArray* ArrayData;
    struct JString* StringData;
    float FloatData;
    long long IntData;
    boolean BoolData;
};

// Special members are all the members required for the assignment.
enum JSpecialMember {
    E_None, // Not a special member
    E_IdStr,
    E_Text,
    E_CreatedAt,
    E_User,
    E_UName,
    E_UScreenName,
    E_ULocation,
    E_UId,
    // Assignment 2a
    E_TweetObj,
    // Assignment 2b
    E_ExTweet,
    E_Truncated,
    E_DisplayRange,
    E_Entities,
    E_Hashtags,
    E_Indices,
    E_FullText
};

// POD utility for storing a starting point of a hashtag and its text.
typedef struct HashTagData {
    Str_c Tag;
    unsigned int Begin;
} HashTagData;
VecDefine(Vec_Hashtags, VH_add, HashTagData);

// We use our own specialized string struct that stores hash tags, length, byte length.
typedef struct JString {
    // 'actuall' length after merging unicode characters as 1 chars.
    // to get byte length use Text.length()
    unsigned int Length;

    // Converted text.
    Str_c Text;
    // This will contain the hashtags found (if any)
    Vec_Hashtags Hashtags;
    Str_c RetweetUser;
} JString;

JString* Alloc_JString(char* cstring);

typedef struct JValue {
    enum JValueType Type;
    union JValueData Data;

    void Print(int indentation) const;

    JValue() {
        Type = E_NullVal;
        Data.ObjectData = NULL;
    }

    JValue(JObject* data) {
        Type = E_Object;
        Data.ObjectData = data;
    }
    
    JValue(JArray* data) {
        Type = E_Array;
        Data.ArrayData = data;
    }

    JValue(char* data) {
        JString* jstr = Alloc_JString(data);
        Type = E_String;
        Data.StringData = jstr;
    }
    
    JValue(JString* data) {
        Type = E_String;
        Data.StringData = data;
    }

    JValue(float num) {
        Type = E_Float;
        Data.FloatData = num;
    }

    JValue(long long num) {
        Type = E_Int;
        Data.IntData = num;
    }

    JValue(boolean value) {
        Type = E_Bool;
        Data.BoolData = value;
    }
} JValue;

// Utility for ranges: arrays with 2 ints
typedef struct JRange {
    long long Begin;
    long long End;
} JRange;

VecDefine(Vec_JValuePtr, VVP_add, JValue*);
typedef struct JArray {
    Vec_JValuePtr Elements;
    JRange AsRange;

    // this is only used if this is a hashtag array.
    Vec_Hashtags Hashtags;

    void Print(int indentation) const;

    // Attempts to exract and populate the Hashtags vector from the elements.
    // Returns true if this array forms a valid "hashtags" array. 
} JArray;

boolean ExtractHashtags(JArray* a, Str_c* Error);

static JArray* Alloc_JArray() {
    JArray* a;
    a = (JArray*) malloc(sizeof(JArray));
    a->Hashtags.slack = 0;
    a->Elements.slack = 0;

    a->AsRange.Begin = -1;
    a->AsRange.End = -1;
    return a;
}

static JArray* Alloc_JArrayRange(long long from, long long to) {
    JArray* a;
    a = (JArray*) malloc(sizeof(JArray));
    a->Hashtags.slack = 0;
    a->Elements.slack = 0;
    
    a->AsRange.Begin = from;
    a->AsRange.End = to;
    
    // Watch out the order here...
    // We 'emulate' our parsing and push back in reverse order.
    VVP_add(&a->Elements, new JValue(to));
    VVP_add(&a->Elements, new JValue(from));

    return a;
}

typedef struct JMember {
    Str_c Name;
    JValue* Value;
    enum JSpecialMember SpecialType;
} JMember;

static JMember* Alloc_JMember(const char* name, JValue* value, enum JSpecialMember type) {
    JMember* m;
    m = (JMember*) malloc(sizeof(JMember));

    m->Name = STR_make(name);
    m->Value = value;
    m->SpecialType = type;
    return m;
}

// A 'special' members struct that holds pointers to specific assignment dependant members
// We don't save the actual metadata but pointers to the values directly
// when any of these pointers are null it means the object does not contain that specific member at all
//
// eg: we only care about the actual JString of a member with name 'text' and we only accept JString as its value.
typedef struct JSpecialMembers {
    JString* IdStr;
    JString* Text;
    JString* CreatedAt;
    struct JObject* User;

    JString* UName;
    JString* UScreenName;
    JString* ULocation;
    long long* UId;

    struct JObject* TweetObj;
} JSpecialMembers;

// Special members for extended tweets, same as above
typedef struct JExSpecialMembers {
    struct JObject* ExTweet;
    boolean* Truncated;
    JRange* DisplayRange;
    struct JObject* Entities;
    JArray* Hashtags;
    JRange* Indices;
    JString* FullText;
} JExSpecialMembers;

VecDefine(Vec_JMemberPtr, VMP_add, JMember*);

typedef struct JObject {
    Vec_JMemberPtr Memberlist;
    JSpecialMembers Members;
    JExSpecialMembers ExMembers;

    void Print(int indentation) const;
} JObject;

// Add a member to the Memberlist and resolve if it needs to popule some Members.* or ExMembers.* field.
void AddMemberToObject(JObject* o, JMember* member);

// Checks if this JObject forms a valid "outer" object 
// ie MUST have text, valid user, IdStr, date AND extra if truncated = true
boolean FormsValidOuterObject(JObject* o, Str_c* FailMessage);

// Checks if this JObject forms a valid "extended tweet" object
// extended tweet MUST include valid hashtags as entities if there are any.
boolean FormsValidExtendedTweetObj(JObject* o, Str_c* FailMessage);

static JObject* Alloc_JObject() {
    JObject* o;
    o = (JObject*) malloc(sizeof(JObject));

    o->Memberlist.slack = 0;

    o->Members.IdStr = NULL;
    o->Members.Text = NULL;
    o->Members.CreatedAt = NULL;
    o->Members.User = NULL;
    o->Members.UName = NULL;
    o->Members.UScreenName = NULL;
    o->Members.ULocation = NULL;
    o->Members.UId = NULL;
    o->Members.TweetObj = NULL;

    o->ExMembers.ExTweet = NULL;
    o->ExMembers.Truncated = NULL;
    o->ExMembers.DisplayRange = NULL;
    o->ExMembers.Entities = NULL;
    o->ExMembers.Hashtags = NULL;
    o->ExMembers.Indices = NULL;
    o->ExMembers.FullText = NULL;
    return o;
}

#endif //__JSON_CLASSES_