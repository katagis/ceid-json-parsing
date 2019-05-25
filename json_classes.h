#ifndef __JSON_CLASSES_
#define __JSON_CLASSES_

#include <vector>
#include <string>
#include <ostream>

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
    std::string StringData;
    float FloatData;
    int IntData;
    bool BoolData;
    
    JValueData() {};
    ~JValueData() {};
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

    JValue(const char* string) {
        Type = JValueType::String;
        Data.StringData = std::string(string);
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

    std::ostream& Print(std::ostream& os, int indentation) const;
};

struct JMember {
    std::string Name;
    JValue* Value;

    std::ostream& Print(std::ostream& os, int indentation) const;

    JMember(const char* name, JValue* value)
        : Name(std::string(name))
        , Value(value) {}
};

struct JObject {
    std::vector<JMember*> Members;

    std::ostream& Print(std::ostream& os, int indentation) const;
};

struct JJson {
    JValue* JsonData;

    std::ostream& Print(std::ostream& os) const;
};


#endif //__JSON_CLASSES_