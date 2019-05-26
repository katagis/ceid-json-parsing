#ifndef __JSON_CLASSES_
#define __JSON_CLASSES_

#include <vector>
#include <string>
#include <iostream>
#include <cstring>


struct JObject;
struct JArray;

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

    JArray(std::vector<JValue*>& list) {
        std::swap(Elements, list);
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

struct JObject {
    std::vector<JMember*> Members;

    JObject() {}

    JObject(std::vector<JMember*>& list) {
        std::swap(Members, list);
    }

    std::ostream& Print(std::ostream& os, int indentation) const;
};

struct JJson {
    JValue* JsonData;

    JJson(JValue* data)
        : JsonData(data) {}

    std::ostream& Print(std::ostream& os) const;
};

struct JMemberList {
    std::vector<JMember*> MemberList;

    JMember* IdStr;
    JMember* Text;
    JMember* CreatedAt;
    JMember* User;
    JMember* UName;
    JMember* UScreenName;
    JMember* ULocation;
    JMember* UId;

    JMemberList()
        : IdStr(nullptr)
        , Text(nullptr)
        , CreatedAt(nullptr)
        , User(nullptr)
        , UName(nullptr)
        , UScreenName(nullptr)
        , ULocation(nullptr) {}

    void AddMember(JMember* member) {
        MemberList.push_back(member);
        switch(member->SpecialType) {
            case JSpecialMember::IdStr:         IdStr = member; break;
            case JSpecialMember::Text:          Text = member; break;
            case JSpecialMember::CreatedAt:     CreatedAt = member; break;
            case JSpecialMember::User:          User = member; break;
            case JSpecialMember::UName:         UName = member; break;
            case JSpecialMember::UScreenName:   UScreenName = member; break;
            case JSpecialMember::ULocation:     ULocation = member; break;
            case JSpecialMember::UId:           UId = member; break;
            default:
                break;
        }
    }

    bool IsValidUser() const {
        return UName && UScreenName && ULocation && UId;
    }

    bool IsValidOuter() const {
        return IdStr && Text && User && CreatedAt;
    }
};


#endif //__JSON_CLASSES_