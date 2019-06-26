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

union JValueData {
    JObject* ObjectData;
    JArray* ArrayData;
    JString* StringData;
    float FloatData;
    long long IntData;
    boolean BoolData;
    
    JValueData() {};
    ~JValueData() {};
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

    unsigned int GetEnd() const {
        return Begin + Tag.len + 1; // offset the '#' character
    }

    int IsEqual(HashTagData* other) const {
        return Begin == other->Begin && (strcmp(Tag.ptr, other->Tag.ptr) == 0);
    }
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

    JString(char* cstring);

    void Print() const;

    boolean IsRetweet() const;
} JString;

typedef struct JValue {
    JValueType Type;
    JValueData Data;

    void Print(int indentation) const;

    JValue() {
        Type = E_NullVal;
        Data.ObjectData = nullptr;
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
        JString* jstr = new JString(data);
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

    JRange()
        : Begin(-1)
        , End(-1) {}
    
    JRange(long long b, long long e)
        : Begin(b)
        , End(e) {}
} JRange;

VecDefine(Vec_JValuePtr, VVP_add, JValue*);
typedef struct JArray {
    Vec_JValuePtr Elements;
    JRange AsRange;

    // this is only used if this is a hashtag array.
    Vec_Hashtags Hashtags;

    JArray() {
        Hashtags.slack = 0;
        Elements.slack = 0;
    }

    JArray(long long from, long long to)
        : AsRange(from, to) {
            // Watch out the order here...
            // We 'emulate' our parsing and push back in reverse order.
            VVP_add(&Elements, new JValue(to));
            VVP_add(&Elements, new JValue(from));
        }

    void AddValue(JValue* value) {
        VVP_add(&Elements, value);
    }

    // Test an array to see if it is a valid "IntRange"
    boolean IsRange() const {
        return AsRange.Begin >= 0;
    }

    void Print(int indentation) const;

    // Attempts to exract and populate the Hashtags vector from the elements.
    // Returns true if this array forms a valid "hashtags" array. 
    boolean ExtractHashtags(Str_c* Error);
} JArray;

typedef struct JMember {
    Str_c Name;
    JValue* Value;
    JSpecialMember SpecialType;

    void Print(int indentation) const;

    JMember(const char* name, JValue* value, JSpecialMember type = E_None)
        : Name(STR_make(name))
        , Value(value)
        , SpecialType(type) {}
} JMember;


// A 'special' members struct that holds pointers to specific assignment dependant members
// We don't save the actual metadata but pointers to the values directly
// when any of these pointers are null it means the object does not contain that specific member at all
//
// eg: we only care about the actual JString of a member with name 'text' and we only accept JString as its value.
typedef struct JSpecialMembers {
    JString* IdStr = nullptr;
    JString* Text = nullptr;
    JString* CreatedAt = nullptr;
    JObject* User = nullptr;

    JString* UName = nullptr;
    JString* UScreenName = nullptr;
    JString* ULocation = nullptr;
    long long* UId = nullptr;

    JObject* TweetObj = nullptr;

    boolean FormsValidUser(boolean RequireAll = false) const {
        if (RequireAll) {
            return UName && UScreenName && ULocation && UId;
        }
        return UScreenName != NULL;
    }
} JSpecialMembers;

// Special members for extended tweets, same as above
typedef struct JExSpecialMembers {
    JObject* ExTweet = nullptr;
    boolean* Truncated = nullptr;
    JRange* DisplayRange = nullptr;
    JObject* Entities = nullptr;
    JArray* Hashtags = nullptr;
    JRange* Indices = nullptr;
    JString* FullText = nullptr;
} JExSpecialMembers;

VecDefine(Vec_JMemberPtr, VMP_add, JMember*);
typedef struct JObject {
    Vec_JMemberPtr Memberlist;
    JSpecialMembers Members;
    JExSpecialMembers ExMembers;

    JObject() {
        Memberlist.slack = 0;
    }

    void Print(int indentation) const;

    
    boolean FormsValidRetweetObj() const;

    // Add a member to the Memberlist and resolve if it needs to popule some Members.* or ExMembers.* field.
    void AddMember(JMember* member);

    // Checks if this JObject forms a valid "outer" object 
    // ie MUST have text, valid user, IdStr, date AND extra if truncated = true
    boolean FormsValidOuterObject(Str_c* FailMessage) const;
    
    // Checks if this JObject forms a valid "extended tweet" object
    // extended tweet MUST include valid hashtags as entities if there are any.
    boolean FormsValidExtendedTweetObj(Str_c* FailMessage) const;

private:
    // Use another function for all the ExMembers just for code readability
    void SwitchOnExMember(JMember* member);
} JObject;

typedef struct JJson {
    JValue* JsonData;

    JJson(JValue* data)
        : JsonData(data) {}

    void Print() const;
} JJson;

#endif //__JSON_CLASSES_