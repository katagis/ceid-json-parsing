#include "json_classes.h"
#include <ostream>

std::ostream& JValue::Print(std::ostream& os, int indent) const {
    switch(Type) {
        case JValueType::Object:
            Data.ObjectData->Print(os, indent);
            break;
        case JValueType::Array:
            Data.ArrayData->Print(os, indent);
            break;
        case JValueType::String:
            os << "\"" << Data.StringData << "\"";
            break;
        case JValueType::Float:
            os << Data.FloatData;
            break;
        case JValueType::Int:
            os << Data.IntData;
            break;
        case JValueType::Bool:
            if (Data.BoolData) {
                os << "true";
            }
            else {
                os << "false";
            }
            break;
        case JValueType::NullVal:
            os << "null";
            break;
    }
    return os;
}

std::ostream& JArray::Print(std::ostream& os, int indent) const {
    os << "[\n"; 
    for (JValue* value : Elements) {
        os << std::string("\t", indent + 1);
        value->Print(os, indent + 1);
        os << ",\n";
    }
    os << std::string("\t", indent) << "]"; 
    return os;
}

std::ostream& JMember::Print(std::ostream& os, int indentation) const {
    os << "\"" << Name << "\": ";
    Value->Print(os, indentation);
    return os;
}

std::ostream& JObject::Print(std::ostream& os, int indent) const {
    os << "{\n"; 
    for (JMember* member : Members) {
        os << std::string("\t", indent + 1);
        member->Print(os, indent + 1);
        os << ",\n";
    }
    os << std::string("\t", indent) << "}"; 
    return os;
}

std::ostream& JJson::Print(std::ostream& os) const {
    JsonData->Print(os, 0);
};