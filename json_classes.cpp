#include "json_classes.h"
#include <ostream>
#include <iomanip>
#include <codecvt>

void Indent(std::ostream& os, int num) {
    os << std::string(num * 2, ' ');
}

bool JsonDB::MaybeInsertIdStr(const char* data) {
    auto result = IdStrs.insert(std::string(data));
    // return if insert actually happened
    return result.second;
}

bool JsonDB::MaybeInsertUserId(int id) {
    auto result = UserIds.insert(id);
    return result.second;
}

std::ostream& JValue::Print(std::ostream& os, int indent) const {
    switch(Type) {
        case JValueType::Object:
            Data.ObjectData->Print(os, indent);
            break;
        case JValueType::Array:
            Data.ArrayData->Print(os, indent);
            break;
        case JValueType::String:
            Data.StringData->Print(os);
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
    os << "["; 
    for (auto it = Elements.rbegin(); it != Elements.rend(); ++it) {
        os << "\n"; 
        Indent(os, indent + 1);
        (*it)->Print(os, indent + 1);
        os << ",";
    }
    os << "\b \n";
    Indent(os, indent);
    os << "]"; 
    return os;
}

std::ostream& JMember::Print(std::ostream& os, int indentation) const {
    os << "\"" << Name << "\": ";
    Value->Print(os, indentation);
    return os;
}

std::ostream& JObject::Print(std::ostream& os, int indent) const {
    os << "{"; 
    for (auto it = Members.rbegin(); it != Members.rend(); ++it) {
        os << "\n"; 
        Indent(os, indent + 1);
        (*it)->Print(os, indent + 1);
        os << ",";
    }
    os << "\b \n";
    Indent(os, indent);
    os << "}"; 
    return os;
}

std::ostream& JJson::Print(std::ostream& os) const {
    JsonData->Print(os, 0);
    os << "\n";
    return os;
};

std::ostream& JString::Print(std::ostream& os) const {
    os << "\"" << Text << "\"";
    return os;
};

JString::JString(char* source) {
    const int sourcelen = strlen(source);
    
    Text.reserve(sourcelen);

    Length = 0;
    for (int i = 0; i < sourcelen; ++i) {
        char c = source[i];
        
        if (c == '\\') {
            // This will never overflow beause we only allow valid strings through flex.
            char esc = source[i+1];

            switch (esc) {
                case 'n': {
                    Text.push_back('\n');
                    Length++;
                    i++;
                    break;
                }
                case '\\': {
                    Text.push_back('\\');
                    Length++;
                    i++;
                    break;
                }
                case '/': {
                    Text.push_back('/');
                    Length++;
                    i++;
                    break;
                }
                case 'u': {
                    // we are sure that this will always read the correct number of bytes because we check for this in our lexer.
                    // explicit type ensures we have enough width and correct representation for the conversion
                    uint32_t u_code = strtol(&source[i+2], nullptr, 16);
    
                    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
                    std::string utf8str = conv.to_bytes(u_code); // convert our U+(u_code) bytes to the actual 3 bytes of UTF-8 escaped string

                    Text.append(utf8str);
                    Length++; // Count this as 1 'actual' character
                    i += 5; // increase by characters read after \ (eg: u2345)
                    break;
                }
                default: {
                    // All other cases just push the character that was escaped.
                    Text.push_back(esc);
                    Length++;
                    i++;
                }
            }
        }
        else if (c == '%') {
            // Care here to not go out of bounds, its possible to have "... %\0"
            // it is assumed that what does not match our 5 special characters gets added as literal text.
            bool FoundMatch = false;
            
            if (source[i+1] == '2') {
                // source[i+2] is not out of bounds here. (it can be '\0')
                switch(source[i+2]) {
                    case 'B': {
                        Text.push_back('+');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case '1': {
                        Text.push_back('!');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case '0': {
                        Text.push_back(' ');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case 'C': {
                        Text.push_back(',');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case '6': {
                        Text.push_back('&');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                }
            }

            // if no match found just add '%'
            if (!FoundMatch) {
                Text.push_back(c);
                Length++;
            }
        }
        else if (c == '#') {
            // Hashtag begins here.
            HashTagData hashtag;
            char *readptr = &source[i+1];
            while((*readptr > 'a' && *readptr < 'z') || // Allow a-z
                  (*readptr > 'A' && *readptr < 'Z') || // Allow A-Z
                  (*readptr > '0' && *readptr < '9') || // Allow 0-9
                  *readptr == '_')  // Allow underscore
            {                                           // Everything else stops the hashtag
                hashtag.Tag.push_back(*readptr);
            }

            if (hashtag.Tag.length() > 0) {
                // We have a valid hashtag.
                hashtag.BeginByte = i; // conatins the '#' param
                Hashtags.push_back(hashtag);
            } // the rest of the code works both for empty or non-empty TempHashtag

            Text.push_back('#');
            Text.append(hashtag.Tag);

            Length += hashtag.Tag.length() + 1; // Count unicode formatted characters added to text.

            i += hashtag.Tag.length(); // Forward by the length of the hashtag
        }
        else {
            Text.push_back(c);
            Length++;
        }
    }

    // finally extract RT @user from the string if exists
    if (source[0] == 'R' &&
        source[1] == 'T' &&
        source[2] == ' ' &&
        source[3] == '@') 
    {
        char *readptr = &source[4];
        while(  (*readptr > 'a' && *readptr < 'z') || // Allow a-z
                (*readptr > 'A' && *readptr < 'Z') || // Allow A-Z
                (*readptr > '0' && *readptr < '9') || // Allow 0-9
                 *readptr == '_')  // Allow underscore
        {
            RetweetUser.push_back(*readptr);
        }
    }
}

bool JString::IsRetweet() const {
    return RetweetUser.length() > 0;
}