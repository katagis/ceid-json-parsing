#ifndef __JSON_CLASSES_
#define __JSON_CLASSES_

#include <vector>
#include <string>
#include <iostream>
#include <unordered_set>
#include <cstring>


struct JObject;
struct JArray;

// Global DB keeping track of ids
struct JsonDB {
    std::unordered_set<std::string> IdStrs;
    std::unordered_set<int> UserIds;

    // Attempts to Insert an id_str element in the database. Returns false if it already existed
    bool MaybeInsertIdStr(const char* id_str);
    // Attempts to Insert a user_id element in the database. Returns false if it already existed
    bool MaybeInsertUserId(int id);
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
    char* StringData;
    float FloatData;
    int IntData;
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
    UId
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
        Type = JValueType::String;
        Data.StringData = data;
    }

    JValue(float num) {
        Type = JValueType::Float;
        Data.FloatData = num;
    }

    JValue(int num) {
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

struct JOuterMembers {
    JMember* IdStr;
    JMember* Text;
    JMember* CreatedAt;
    JMember* User;

    JOuterMembers()
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

    JOuterMembers OuterMembers;
    JUserMembers UserMembers;

    JObject() {}

    JObject(std::vector<JMember*>& list) {
        std::swap(Members, list);
    }

    std::ostream& Print(std::ostream& os, int indentation) const;

    void AddMember(JMember* member) {
        Members.push_back(member);
        switch(member->SpecialType) {
            case JSpecialMember::IdStr:         OuterMembers.IdStr = member; break;
            case JSpecialMember::Text:          OuterMembers.Text = member; break;
            case JSpecialMember::CreatedAt:     OuterMembers.CreatedAt = member; break;
            case JSpecialMember::User:          OuterMembers.User = member; break;
            case JSpecialMember::UName:         UserMembers.UName = member; break;
            case JSpecialMember::UScreenName:   UserMembers.UScreenName = member; break;
            case JSpecialMember::ULocation:     UserMembers.ULocation = member; break;
            case JSpecialMember::UId:           UserMembers.UId = member; break;
            default:
                break;
        }
    }

    bool IsValidUser() const {
        return UserMembers.IsValid(); 
    }

    bool IsValidOuter() const {
        return OuterMembers.IsValid();
    }
};

struct JJson {
    JValue* JsonData;

    JJson(JValue* data)
        : JsonData(data) {}

    std::ostream& Print(std::ostream& os) const;
};

#endif //__JSON_CLASSES_