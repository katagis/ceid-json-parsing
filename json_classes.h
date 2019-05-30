#ifndef __JSON_CLASSES_
#define __JSON_CLASSES_

#include <vector>
#include <string>
#include <iostream>
#include <unordered_set>
#include <cstring>


struct JObject;
struct JArray;
struct JString;

// Global DB keeping track of ids
struct JsonDB {
    std::unordered_set<std::string> IdStrs;
    std::unordered_set<long long> UserIds;

    // Attempts to Insert an id_str element in the database. Returns false if it already existed
    bool MaybeInsertIdStr(const char* id_str);
    // Attempts to Insert a user_id element in the database. Returns false if it already existed
    bool MaybeInsertUserId(long long id);
};

enum class JValueType {
    Object,
    Array,
    String,
    Float,
    Int,
    Bool,
    NullVal
};

union JValueData {
    JObject* ObjectData;
    JArray* ArrayData;
    JString* StringData;
    float FloatData;
    long long IntData;
    bool BoolData;
    
    JValueData() {};
    ~JValueData() {};
};

// Special members are all the members required for the assignment.
enum class JSpecialMember {
    None, // Not a special member
    IdStr,
    Text,
    CreatedAt,
    User,
    UName,
    UScreenName,
    ULocation,
    UId,
    // Assignment 2a
    TweetObj,
    // Assignment 2b
    ExTweet,
    Truncated,
    DisplayRange,
    Entities,
    Hashtags,
    Indicies,
    FullText
};

// POD utility for storing a starting point of a hashtag and its text.
struct HashTagData {
    std::string Tag;
    unsigned int BeginByte;

    unsigned int GetEndByte() const {
        return BeginByte + Tag.length() + 1; // offset the '#' character
    }
};

// We use our own specialized string struct that stores hash tags, length, byte length.
struct JString {
    // 'actuall' length after merging unicode characters as 1 chars.
    // to get byte length use Text.length()
    unsigned int Length;

    // Converted text.
    std::string Text;
    // This will contain the hashtags found (if any)
    std::vector<HashTagData> Hashtags;
    std::string RetweetUser;

    JString(char* cstring);

    std::ostream& Print(std::ostream& os) const;

    bool IsRetweet() const;
};



struct JValue {
    JValueType Type;
    JValueData Data;

    std::ostream& Print(std::ostream& os, int indentation) const;

    JValue() {
        Type = JValueType::NullVal;
        Data.ObjectData = nullptr;
    }

    JValue(JObject* data) {
        Type = JValueType::Object;
        Data.ObjectData = data;
    }
    
    JValue(JArray* data) {
        Type = JValueType::Array;
        Data.ArrayData = data;
    }

    JValue(char* data) {
        JString* jstr = new JString(data);
        Type = JValueType::String;
        Data.StringData = jstr;
    }
    
    JValue(JString* data) {
        Type = JValueType::String;
        Data.StringData = data;
    }

    JValue(float num) {
        Type = JValueType::Float;
        Data.FloatData = num;
    }

    JValue(long long num) {
        Type = JValueType::Int;
        Data.IntData = num;
    }

    JValue(bool value) {
        Type = JValueType::Bool;
        Data.BoolData = value;
    }
};

// Utility for ranges: arrays with 2 ints
struct JRange {
    long long Begin;
    long long End;

    JRange()
        : Begin(-1)
        , End(-1) {}
    
    JRange(long long b, long long e)
        : Begin(b)
        , End(e) {}
};

struct JArray {
    std::vector<JValue*> Elements;
    
    JRange AsRange;

    JArray() {}

    JArray(long long from, long long to)
        : AsRange(from, to) {
            // Watch out the order here...
            // We 'emulate' our parsing and push back in reverse order.
            Elements.push_back(new JValue(to));
            Elements.push_back(new JValue(from));
        }

    void AddValue(JValue* value) {
        Elements.push_back(value);
    }

    // Test an array to see if it is a valid "IntRange"
    bool IsRange() const {
        return AsRange.Begin >= 0;
    }

    std::ostream& Print(std::ostream& os, int indentation) const;
};

struct JMember {
    std::string Name;
    JValue* Value;
    JSpecialMember SpecialType;

    std::ostream& Print(std::ostream& os, int indentation) const;

    JMember(const char* name, JValue* value, JSpecialMember type = JSpecialMember::None)
        : Name(std::string(name))
        , Value(value)
        , SpecialType(type) {}
};


// A 'special' members struct that holds pointers to specific assignment dependant members
// We don't save the actual metadata but pointers to the values directly
// when any of these pointers are null it means the object does not contain that specific member at all
//
// eg: we only care about the actual JString of a member with name 'text' and we only accept JString as its value.
struct JSpecialMembers {
    JString* IdStr = nullptr;
    JString* Text = nullptr;
    JString* CreatedAt = nullptr;
    JObject* User = nullptr;

    JString* UName = nullptr;
    JString* UScreenName = nullptr;
    JString* ULocation = nullptr;
    long long* UId = nullptr;

    JObject* TweetObj = nullptr;

    bool FormsValidOuterObject() const {
        return IdStr
            && Text
            && User
            && CreatedAt;
    }

    bool FormsValidUser(bool RequireAll = false) const {
        if (RequireAll) {
            return UName && UScreenName && ULocation && UId;
        }
        return UScreenName;
    }
};

// Special members for extended tweets, same as above
struct JExSpecialMembers {
    JObject* ExTweet = nullptr;
    bool* Truncated = nullptr;
    JRange* DisplayRange = nullptr;
    JObject* Entities = nullptr;
    JArray* Hashtags = nullptr;
    JRange* Indicies = nullptr;
    JString* FullText = nullptr;


};

struct JObject {
    std::vector<JMember*> Memberlist;
    JSpecialMembers Members;
    JExSpecialMembers ExMembers;

    std::ostream& Print(std::ostream& os, int indentation) const;

    bool IsValidTweetObj() const {
        // To be a valid "tweet" object we need at least valid "text" and "user" fields.
        if (!Members.Text || !Members.User) {
            return false;
        }
        // Now that we have them we also need to check if the text contains RT @.
        // At this level we cannot check if the @Username is the correct username yet.
        if (Members.Text->RetweetUser.empty()) {
            return false;
        }
        return true;
    }

    // Add a member to the Memberlist and resolve if it needs to popule some Members.* or ExMembers.* field.
    void AddMember(JMember* member);

private:
    // Use another function for all the ExMembers just for code readability
    void SwitchOnExMember(JMember* member);
};

struct JJson {
    JValue* JsonData;

    JJson(JValue* data)
        : JsonData(data) {}

    std::ostream& Print(std::ostream& os) const;
};

#endif //__JSON_CLASSES_