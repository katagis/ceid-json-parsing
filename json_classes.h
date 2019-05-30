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

struct JUserMembers {
    JMember* UName;
    JMember* UScreenName;
    JMember* ULocation;
    JMember* UId;

    JUserMembers()
        : UName(nullptr)
        , UScreenName(nullptr)
        , ULocation(nullptr)
        , UId(nullptr) {}

    bool IsValid() const {
        return UName 
            && UScreenName
            && ULocation
            && UId;
    }
};

struct JGenericMembers {
    JMember* IdStr;
    JMember* Text;
    JMember* CreatedAt;
    JMember* User;

    JGenericMembers()
        : IdStr(nullptr)
        , Text(nullptr)
        , CreatedAt(nullptr)
        , User(nullptr) {}

    bool IsValid() const {
        return IdStr
            && Text
            && User
            && CreatedAt;
    }
};

struct JObject {
    std::vector<JMember*> Members;

    JGenericMembers GenericMembers;
    JUserMembers UserMembers;
    JMember* TweetObjMember;

    JObject() {}

    JObject(std::vector<JMember*>& list) {
        std::swap(Members, list);
    }

    std::ostream& Print(std::ostream& os, int indentation) const;

    void AddMember(JMember* member) {
        Members.push_back(member);
        switch(member->SpecialType) {
            case JSpecialMember::IdStr:         GenericMembers.IdStr = member; break;
            case JSpecialMember::Text:          GenericMembers.Text = member; break;
            case JSpecialMember::CreatedAt:     GenericMembers.CreatedAt = member; break;
            case JSpecialMember::User:          GenericMembers.User = member; break;
            case JSpecialMember::UName:         UserMembers.UName = member; break;
            case JSpecialMember::UScreenName:   UserMembers.UScreenName = member; break;
            case JSpecialMember::ULocation:     UserMembers.ULocation = member; break;
            case JSpecialMember::UId:           UserMembers.UId = member; break;
            case JSpecialMember::TweetObj:      TweetObjMember = member; break;
            default:
                break;
        }
    }

    // Utility getter that returns the data of "text" field of this object.
    // this will return a valid ptr if and only if GenericMembers.Text is valid
    JString* GetGenericText() const {
        if (GenericMembers.Text) {
            return GenericMembers.Text->Value->Data.StringData;
        }
        return nullptr;
    }

    bool IsValidUser() const {
        return UserMembers.IsValid(); 
    }
    
    bool HasValidScreenName() const {
        return UserMembers.UScreenName != nullptr; 
    }

    bool IsValidOuterObject() const {
        return GenericMembers.IsValid();
    }

    bool IsValidTweet() const {
        // To be a valid "tweet" object we need at least valid "text" and "user" fields.
        if (!GenericMembers.Text || !GenericMembers.User) {
            return false;
        }
        // Now that we have them we also need to check if the text contains RT @.
        // At this level we cannot check if the @Username is the correct username yet.
        if (GetGenericText()->RetweetUser.empty()) {
            return false;
        }
        return true;
    }

    // assumes this object has a valid retweet object.
    std::string* GetRetweetUser() const {
        return &GetGenericText()->RetweetUser;
    }

    // assumes this object has a valid "user" subobject with a "screen_name"
    std::string* GetContainedUserScreenName() const {
        // First get the contained "user" as JObject.
        const JObject* IncludedUserObject = GenericMembers.User->Value->Data.ObjectData;
        // From the contained "user" object grab the ScreenName as text
        return &IncludedUserObject->UserMembers.UScreenName->Value->Data.StringData->Text;
    }
};

struct JJson {
    JValue* JsonData;

    JJson(JValue* data)
        : JsonData(data) {}

    std::ostream& Print(std::ostream& os) const;
};

#endif //__JSON_CLASSES_