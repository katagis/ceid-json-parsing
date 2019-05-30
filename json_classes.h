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
    TweetObj,
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

struct JArray {
    std::vector<JValue*> Elements;

    JArray() {}

    void AddValue(JValue* value) {
        Elements.push_back(value);
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
    JString* IdStr;
    JString* Text;
    JString* CreatedAt;
    JObject* User;

    JString* UName;
    JString* UScreenName;
    JString* ULocation;
    long long* UId;

    JObject* TweetObj;

    JSpecialMembers()
        : IdStr(nullptr)
        , Text(nullptr)
        , CreatedAt(nullptr)
        , User(nullptr)
        , UName(nullptr)
        , UScreenName(nullptr)
        , ULocation(nullptr)
        , UId(nullptr) {}

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

struct JObject {
    std::vector<JMember*> Memberlist;
    JSpecialMembers Members;

    std::ostream& Print(std::ostream& os, int indentation) const;

    void AddMember(JMember* member) {
        // When adding a member if it is special we populate the specific Object Field with its data.
        // the final (simplified) JObject outline looks something like this after we are done adding members.
        // JObject 
        //    Members[]           - A reversed list with all the members (used for printing output)
        //    ...
        //    IdStr = nullptr     - field 'IdStr' is not present in this object
        //    Text = JStringData* - value of 'text' json field
        //    User = JObject*     - value of 'user' json field
        //    
        Memberlist.push_back(member);
        switch(member->SpecialType) {
            case JSpecialMember::IdStr:         Members.IdStr       = member->Value->Data.StringData; break;
            case JSpecialMember::Text:          Members.Text        = member->Value->Data.StringData; break;
            case JSpecialMember::CreatedAt:     Members.CreatedAt   = member->Value->Data.StringData; break;
            case JSpecialMember::User:          Members.User        = member->Value->Data.ObjectData; break;
            case JSpecialMember::UName:         Members.UName       = member->Value->Data.StringData; break;
            case JSpecialMember::UScreenName:   Members.UScreenName = member->Value->Data.StringData; break;
            case JSpecialMember::ULocation:     Members.ULocation   = member->Value->Data.StringData; break;
            case JSpecialMember::UId:           Members.UId         = &member->Value->Data.IntData; break;
            case JSpecialMember::TweetObj:      Members.TweetObj    = member->Value->Data.ObjectData; break;
            default:
                break;
        }
    }

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
};

struct JJson {
    JValue* JsonData;

    JJson(JValue* data)
        : JsonData(data) {}

    std::ostream& Print(std::ostream& os) const;
};

#endif //__JSON_CLASSES_