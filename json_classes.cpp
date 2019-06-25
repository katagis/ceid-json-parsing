#include "json_classes.h"
#include "c_comp.h"

#define OUT stdout

void Indent(int num) {
    printMultiple(' ', num * 2, OUT);
}

bool JsonDB::MaybeInsertIdStr(const char* data) {
    for (int i = 0; i < IdStrs.size(); ++i) {
        if (strcmp(data, IdStrs[i].ptr) == 0) {
            return false;
        }
    }
    IdStrs.push_back(Str_c::make(data));
    return true;
}

bool JsonDB::MaybeInsertUserId(long long id) {
    for (int i = 0; i < UserIds.size(); ++i) {
        if (UserIds[i] == id) {
            return false;
        }
    }
    UserIds.push_back(id);
    return true;
}

void JValue::Print(int indent) const {
    switch(Type) {
        case JValueType::Object:
            Data.ObjectData->Print(indent);
            break;
        case JValueType::Array:
            Data.ArrayData->Print(indent);
            break;
        case JValueType::String:
            Data.StringData->Print();
            break;
        case JValueType::Float:
            fprintf(OUT, "%f", Data.FloatData);
            break;
        case JValueType::Int:
            fprintf(OUT, "%lli", Data.IntData);
            break;
        case JValueType::Bool:
            if (Data.BoolData) {
                fprintf(OUT, "true");
            }
            else {
                fprintf(OUT, "false");
            }
            break;
        case JValueType::NullVal:
            fprintf(OUT, "null");
            break;
    }
}

void JArray::Print(int indent) const {
    fprintf(OUT, "[ ");      // Print [ to OUT
    for (JValue* el : Elements) { // for each element as el
        fprintf(OUT, "\n");
        Indent(indent + 1);    // add (indent + 1) * \t tab characters
        el->Print(indent + 1); // call print for value at indent + 1
        fprintf(OUT, ",");
    }
    fprintf(OUT, "\b \n"); // backspace last comma
    Indent(indent);
    fprintf(OUT, "]"); 
}

void JMember::Print(int indentation) const {
    fprintf(OUT, "\"%s\": ", Name.ptr);
    Value->Print(indentation);
}

void JObject::Print(int indent) const {
    fprintf(OUT, "{ "); 
    for (auto it = Memberlist.rbegin(); it != Memberlist.rend(); ++it) {
        fprintf(OUT, "\n"); 
        Indent(indent + 1);
        (*it)->Print(indent + 1);
        fprintf(OUT, ","); 
    }
    fprintf(OUT, "\b \n");
    Indent(indent);
    fprintf(OUT, "}"); 
}

void JJson::Print() const {
    JsonData->Print(0);
    fprintf(OUT, "\n");
};

void JString::Print() const {
    fprintf(OUT, "\"%s\"", Text.ptr);
};

JString::JString(char* source) {
    RetweetUser = Str_c::makeEmpty();
    Text = Str_c::makeEmpty(); // normally we should do reserve here
    const int sourcelen = strlen(source);

    Length = 0;
    for (int i = 0; i < sourcelen; ++i) {
        char c = source[i];
        
        if (c == '\\') {
            // This will never overflow beause we only allow valid strings through flex.
            char esc = source[i+1];

            switch (esc) {
                case 'n': {
                    Text.addChar('\n');
                    Length++;
                    i++;
                    break;
                }
                case 'u': {
                    // we are sure that this will always read the correct number of bytes because we check for this in our lexer.
                    // explicit type ensures we have enough width and correct representation for the conversion
                    long u_code = strtol(&source[i+2], nullptr, 16);
                    Text.appendAsUtf8(u_code);

                    Length++; // Count this as 1 'actual' character
                    i += 5;   // increase by characters read after \ (eg: u2345)
                    break;
                }
                default: {
                    // All other cases just push the character that was escaped.
                    Text.addChar(esc);
                    Length++;
                    i++;
                    break;
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
                        Text.addChar('+');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case '1': {
                        Text.addChar('!');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case '0': {
                        Text.addChar(' ');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case 'C': {
                        Text.addChar(',');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case '6': {
                        Text.addChar('&');
                        Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                }
            }

            // if no match found just add '%'
            if (!FoundMatch) {
                Text.addChar(c);
                Length++;
            }
        }
        else if (c == '#') {
            // Hashtag begins here.
            HashTagData hashtag;
            char *readptr = &source[i+1];
            while((*readptr >= 'a' && *readptr <= 'z') || // Allow a-z
                  (*readptr >= 'A' && *readptr <= 'Z') || // Allow A-Z
                  (*readptr >= '0' && *readptr <= '9') || // Allow 0-9
                  *readptr == '_')  // Allow underscore
            {                                           // Everything else stops the hashtag
                hashtag.Tag.addChar(*readptr);
                readptr++;
            }
            
            if (hashtag.Tag.len > 0) {
                // We have a valid hashtag.
                // Assumes indices count escaped sequences as 1 character. (eg: "text"="/u2330 #abc" starts at 2)
                hashtag.Begin = Length; // conatins the '#' param
                Hashtags.push_back(hashtag);
            } // the rest of the code works both for empty or non-empty TempHashtag

            Text.addChar('#');
            Text.append(hashtag.Tag.ptr);

            Length += hashtag.Tag.len + 1; // Count unicode formatted characters added to text.

            i += hashtag.Tag.len; // Forward by the length of the hashtag
        }
        else {
            Text.addChar(c);
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
            RetweetUser.addChar(*readptr);
            readptr++;
        }
    }
}

bool JString::IsRetweet() const {
    return RetweetUser.len > 0;
}

void JObject::AddMember(JMember* member) {
    // When adding a member if it is special we populate the specific Object Field with its data.
    // the final (simplified) JObject outline looks something like this after we are done adding members.
    // JObject 
    //    Members[]           - A reversed list with all the members (used for printing output)
    //    ...
    //    IdStr = nullptr     - field 'IdStr' is not present in this object
    //    Text = JString*     - value of 'text' json field
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
            SwitchOnExMember(member);
    }
}

void JObject::SwitchOnExMember(JMember* member) {
    switch(member->SpecialType) {
        case JSpecialMember::ExTweet:       ExMembers.ExTweet       = member->Value->Data.ObjectData; break;
        case JSpecialMember::Truncated:     ExMembers.Truncated     = &member->Value->Data.BoolData; break;
        case JSpecialMember::DisplayRange:  ExMembers.DisplayRange  = &member->Value->Data.ArrayData->AsRange; break;
        case JSpecialMember::Entities:      ExMembers.Entities      = member->Value->Data.ObjectData; break;
        case JSpecialMember::Hashtags:      ExMembers.Hashtags      = member->Value->Data.ArrayData; break;
        case JSpecialMember::Indices:       ExMembers.Indices      = &member->Value->Data.ArrayData->AsRange; break;
        case JSpecialMember::FullText:      ExMembers.FullText      = member->Value->Data.StringData; break;
    }
}

bool JObject::FormsValidRetweetObj() const {
    // To be a valid "tweet" object we need at least valid "text" and "user" fields.
    if (!Members.Text || !Members.User) {
        return false;
    }
    // Now that we have them we also need to check if the text contains RT @.
    // At this level we cannot check if the @Username is the correct username yet.
    if (!Members.Text->RetweetUser.len) {
        return false;
    }
    return true;
}

bool JObject::FormsValidOuterObject(Str_c* FailMessage) const {
    
    // Our outer object MUST include IdStr, Text, User, Date.
    if (!Members.IdStr || !Members.Text || !Members.User || !Members.CreatedAt) {
        FailMessage->append("Missing field IdStr/Text/User/CreatedAt");
        return false;
    }

    // if we dont find a truncated value or found one that == false we are done.
    if (!ExMembers.Truncated || *ExMembers.Truncated == false) {
        return true;
    }

    // Otherwise truncated = true so the object MUST include:
    // * a valid display_text_range [0, Members.Text.Length]
    // * an exteneded tweet object 

    if (!ExMembers.DisplayRange) {
        FailMessage->append("Missing display range when truncated == true.");
        return false;
    }

    // The display range must be from x->x+Length and x+Length must be less than 
    unsigned int TextSize = Members.Text->Length;
    if (ExMembers.DisplayRange->Begin != 0 ||
        ExMembers.DisplayRange->End != TextSize) {

        FailMessage->append("Display Range did not match the given text.");
        return false;
    }

    // Finally check for the extended tweet object. 
    if (!ExMembers.ExTweet) {
        FailMessage->append("Missing extended tweet when truncated == true.");
        return false;
    }

    // We found an ExTweet. 
    // This cannot be invalid since it has been already checked it was first parsed.
    
    // All required fields have been found and validated.
    return true;
}

bool JObject::FormsValidExtendedTweetObj(Str_c* FailMessage) const {

    // Require full_text and display range.
    if (!ExMembers.FullText || !ExMembers.DisplayRange) {
        FailMessage->append("Missing 'full_text' and/or 'display_text_range'.");
        return false;
    }

    // Validate text length with display range
    unsigned int TextSize = ExMembers.FullText->Length;
    if (ExMembers.DisplayRange->Begin != 0 ||
        ExMembers.DisplayRange->End != TextSize) {

        char Message[200];
        sprintf(Message, 
            "Display Range did not match the given full_text.\nText Size: %d DisplayRange: [%lli, %lli]",
            TextSize, ExMembers.DisplayRange->Begin, ExMembers.DisplayRange->End
        );
        FailMessage->append(Message);
        return false;
    }
    
    const JString& TextObj = *ExMembers.FullText;

    // Even if we have NO hashtags in the text, we still need to verify that
    // there are no recorded hashtags in the array (but only if one exists.)

    int HashtagsInArray = 0;
    if (ExMembers.Entities) {
        HashtagsInArray = ExMembers.Entities->ExMembers.Hashtags->Hashtags.size();
    }

    if (HashtagsInArray != TextObj.Hashtags.size()) {
        FailMessage->append("Hashtags found in text did not match all the hashtags in the entites.");
        return false;
    }

    // Hashtag count was equal. If it is also 0 then this is a valid extended_tweet
    if (HashtagsInArray == 0) {
        return true;
    }

    // The hashtags found in the entities array.
    std::vector<HashTagData>& Tags = ExMembers.Entities->ExMembers.Hashtags->Hashtags;

    // All that is left is to verify hashtag positions on the actual text.
    // The hash tags could be in random order so do N^2 for now. 
    // TODO: this could be optimized by using unordered_sets instead of vectors
    for (const HashTagData& Outer : TextObj.Hashtags) {
        int found = 0;
        for(int i = 0; i < Tags.size(); ++i) {
            if (Outer.IsEqual(&Tags[i])) {
                found = 1;
                break;
            }
        }

        if (!found) {
            char AddedStr[200];
            sprintf(AddedStr, "Hashtag: '%s' is missing from the entities array or has incorrect Indices.", Outer.Tag.ptr);
            FailMessage->append(AddedStr);
            return false;
        }
    }

    return true;
}

bool JArray::ExtractHashtags(Str_c* Error) {
    // This array must ONLY include objects that contain 'text', 'indices'
    // the array can be empty.
    // Actual checking for hashtag locations cannot be performed at this stage
    
    for (JValue* Element : Elements) {
        // Must be an object type
        if (Element->Type != JValueType::Object) {
            Error->append("An element of the array is not an object.");
            return false;
        }

        // Now check this object to find if it includes "text" & "indices" they are required.
        JObject* Subobject = Element->Data.ObjectData;
        if (!Subobject->Members.Text || !Subobject->ExMembers.Indices) {
            Error->append("An element of the array is missing 'text' and/or 'indices'.");
            return false;
        }

        // Also validate indice length. The actual # is not included in the text but it is accounted in indice length
        // Therefore we need to offset the length by 1
        long long IndiceLength = Subobject->ExMembers.Indices->End - Subobject->ExMembers.Indices->Begin;
        if (IndiceLength != Subobject->Members.Text->Length + 1) {
            Error->append("Indice range did not match the text length.");
            return false;

        }

        // Everything is correct. Collect this result and add it to the hashtags vector.
        HashTagData Data;
        Data.Tag = Str_c::make(Subobject->Members.Text->Text.ptr);
        Data.Begin = Subobject->ExMembers.Indices->Begin;

        Hashtags.push_back(Data);
    }
    return true;
}