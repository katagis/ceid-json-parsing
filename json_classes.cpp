#include "json_classes.h"
#include "c_comp.h"

void Indent(int num) {
    printMultiple(' ', num * 2, OUT);
}

boolean MaybeInsertIdStr(const char* data) {
    for (int i = 0; i < database.IdStrs.size; ++i) {
        if (strcmp(data, database.IdStrs.data[i].ptr) == 0) {
            return false;
        }
    }
    VS_add(&database.IdStrs, STR_make(data));
    return true;
}

boolean MaybeInsertUserId(long long id) {
    for (int i = 0; i < database.UserIds.size; ++i) {
        if (database.UserIds.data[i] == id) {
            return false;
        }
    }
    VLL_add(&database.UserIds, id);
    return true;
}

void Print_JValue(JValue* v, int indent) {
    switch(v->Type) {
        case E_Object:
            Print_JObject(v->Data.ObjectData, indent);
            break;
        case E_Array:
            Print_JArray(v->Data.ArrayData, indent);
            break;
        case E_String:
            fprintf(OUT, "\"%s\"", v->Data.StringData->Text.ptr);
            break;
        case E_Float:
            fprintf(OUT, "%f", v->Data.FloatData);
            break;
        case E_Int:
            fprintf(OUT, "%lli", v->Data.IntData);
            break;
        case E_Bool:
            if (v->Data.BoolData) {
                fprintf(OUT, "true");
            }
            else {
                fprintf(OUT, "false");
            }
            break;
        case E_NullVal:
            fprintf(OUT, "null");
            break;
    }
}

void Print_JArray(JArray* a, int indent) {
    fprintf(OUT, "[ ");      // Print [ to OUT
    int i = 0;
    for (i = 0; i < a->Elements.size; ++i) { // for each element as el
        fprintf(OUT, "\n");
        Indent(indent + 1);    // add (indent + 1) * \t tab characters
        Print_JValue(a->Elements.data[i], indent + 1); // call print for value at indent + 1
        fprintf(OUT, ",");
    }
    fprintf(OUT, "\b \n"); // backspace last comma
    Indent(indent);
    fprintf(OUT, "]"); 
}

void Print_JObject(JObject* o, int indent) {
    fprintf(OUT, "{ "); 
    int i = 0;
    for (i = o->Memberlist.size - 1; i >= 0; --i) { // reverse of push_back'ed order.
        fprintf(OUT, "\n"); 
        Indent(indent + 1);
        fprintf(OUT, "\"%s\": ", o->Memberlist.data[i]->Name.ptr);
        Print_JValue(o->Memberlist.data[i]->Value, indent + 1);
        fprintf(OUT, ","); 
    }
    fprintf(OUT, "\b \n");
    Indent(indent);
    fprintf(OUT, "}"); 
}


JString* Alloc_JString(char* source) {
    JString* s = (JString *)malloc(sizeof(JString));

    s->RetweetUser = STR_makeEmpty();
    s->Text = STR_makeEmpty();
    s->Hashtags.slack = 0;
    const int sourcelen = strlen(source);

    s->Length = 0;
    for (int i = 0; i < sourcelen; ++i) {
        char c = source[i];
        
        if (c == '\\') {
            // This will never overflow beause we only allow valid strings through flex.
            char esc = source[i+1];

            switch (esc) {
                case 'n': {
                    STR_addChar(&s->Text, '\n');
                    s->Length++;
                    i++;
                    break;
                }
                case 'u': {
                    // we are sure that this will always read the correct number of bytes because we check for this in our lexer.
                    // explicit type ensures we have enough width and correct representation for the conversion
                    long u_code = strtol(&source[i+2], NULL, 16);
                    STR_appendAsUtf8(&s->Text, u_code);

                    s->Length++; // Count this as 1 'actual' character
                    i += 5;   // increase by characters read after \ (eg: u2345)
                    break;
                }
                default: {
                    // All other cases just push the character that was escaped.
                    STR_addChar(&s->Text, esc);
                    s->Length++;
                    i++;
                    break;
                }
            }
        }
        else if (c == '%') {
            // Care here to not go out of bounds, its possible to have "... %\0"
            // it is assumed that what does not match our 5 special characters gets added as literal text.
            boolean FoundMatch = false;
            
            if (source[i+1] == '2') {
                // source[i+2] is not out of bounds here. (it can be '\0')
                switch(source[i+2]) {
                    case 'B': {
                        STR_addChar(&s->Text, '+');
                        s->Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case '1': {
                        STR_addChar(&s->Text, '!');
                        s->Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case '0': {
                        STR_addChar(&s->Text, ' ');
                        s->Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case 'C': {
                        STR_addChar(&s->Text, ',');
                        s->Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                    case '6': {
                        STR_addChar(&s->Text, '&');
                        s->Length++;
                        i += 2;
                        FoundMatch = true;
                        break;
                    }
                }
            }

            // if no match found just add '%'
            if (!FoundMatch) {
                STR_addChar(&s->Text, c);
                s->Length++;
            }
        }
        else if (c == '#') {
            // Hashtag begins here.
            HashTagData hashtag;
            hashtag.Tag = STR_makeEmpty();
            char *readptr = &source[i+1];
            while((*readptr >= 'a' && *readptr <= 'z') || // Allow a-z
                  (*readptr >= 'A' && *readptr <= 'Z') || // Allow A-Z
                  (*readptr >= '0' && *readptr <= '9') || // Allow 0-9
                  *readptr == '_')  // Allow underscore
            {                                           // Everything else stops the hashtag
                STR_addChar(&hashtag.Tag, *readptr);
                readptr++;
            }
            
            if (hashtag.Tag.len > 0) {
                // We have a valid hashtag.
                // Assumes indices count escaped sequences as 1 character. (eg: "text"="/u2330 #abc" starts at 2)
                hashtag.Begin = s->Length; // conatins the '#' param
                VH_add(&s->Hashtags, hashtag);
            } // the rest of the code works both for empty or non-empty TempHashtag

            STR_addChar(&s->Text, '#');
            STR_append(&s->Text, hashtag.Tag.ptr);

            s->Length += hashtag.Tag.len + 1; // Count unicode formatted characters added to text.

            i += hashtag.Tag.len; // Forward by the length of the hashtag
        }
        else {
            STR_addChar(&s->Text, c);
            s->Length++;
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
            STR_addChar(&s->RetweetUser, *readptr);
            readptr++;
        }
    }
    return s;
}

void AddMemberToObject(JObject* o, JMember* member) {
    // When adding a member if it is special we populate the specific Object Field with its data.
    // the final (simplified) JObject outline looks something like this after we are done adding members.
    // JObject 
    //    Members[]           - A reversed list with all the members (used for printing output)
    //    ...
    //    IdStr = nullptr     - field 'IdStr' is not present in this object
    //    Text = JString*     - value of 'text' json field
    //    User = JObject*     - value of 'user' json field
    //    
    VMP_add(&o->Memberlist, member);
    switch(member->SpecialType) {
        case E_IdStr:         o->Members.IdStr       = member->Value->Data.StringData; break;
        case E_Text:          o->Members.Text        = member->Value->Data.StringData; break;
        case E_CreatedAt:     o->Members.CreatedAt   = member->Value->Data.StringData; break;
        case E_User:          o->Members.User        = member->Value->Data.ObjectData; break;
        case E_UName:         o->Members.UName       = member->Value->Data.StringData; break;
        case E_UScreenName:   o->Members.UScreenName = member->Value->Data.StringData; break;
        case E_ULocation:     o->Members.ULocation   = member->Value->Data.StringData; break;
        case E_UId:           o->Members.UId         = &member->Value->Data.IntData; break;
        case E_TweetObj:      o->Members.TweetObj    = member->Value->Data.ObjectData; break;
        case E_ExTweet:       o->ExMembers.ExTweet       = member->Value->Data.ObjectData; break;
        case E_Truncated:     o->ExMembers.Truncated     = &member->Value->Data.BoolData; break;
        case E_DisplayRange:  o->ExMembers.DisplayRange  = &member->Value->Data.ArrayData->AsRange; break;
        case E_Entities:      o->ExMembers.Entities      = member->Value->Data.ObjectData; break;
        case E_Hashtags:      o->ExMembers.Hashtags      = member->Value->Data.ArrayData; break;
        case E_Indices:       o->ExMembers.Indices       = &member->Value->Data.ArrayData->AsRange; break;
        case E_FullText:      o->ExMembers.FullText      = member->Value->Data.StringData; break;
    }
}

boolean FormsValidOuterObject(JObject* o, Str_c* FailMessage) {
    
    // Our outer object MUST include IdStr, Text, User, Date.
    if (!o->Members.IdStr || !o->Members.Text || !o->Members.User || !o->Members.CreatedAt) {
        STR_append(FailMessage, "Missing field IdStr/Text/User/CreatedAt");
        return false;
    }

    // if we dont find a truncated value or found one that == false we are done.
    if (!o->ExMembers.Truncated || *o->ExMembers.Truncated == false) {
        return true;
    }

    // Otherwise truncated = true so the object MUST include:
    // * a valid display_text_range [0, Members.Text.Length]
    // * an exteneded tweet object 

    if (!o->ExMembers.DisplayRange) {
        STR_append(FailMessage, "Missing display range when truncated == true.");
        return false;
    }

    // The display range must be from x->x+Length and x+Length must be less than 
    unsigned int TextSize = o->Members.Text->Length;
    if (o->ExMembers.DisplayRange->Begin != 0 ||
        o->ExMembers.DisplayRange->End != TextSize) {

        STR_append(FailMessage, "Display Range did not match the given text.");
        return false;
    }

    // Finally check for the extended tweet object. 
    if (!o->ExMembers.ExTweet) {
        STR_append(FailMessage, "Missing extended tweet when truncated == true.");
        return false;
    }

    // We found an ExTweet. 
    // This cannot be invalid since it has been already checked it was first parsed.
    
    // All required fields have been found and validated.
    return true;
}

boolean FormsValidExtendedTweetObj(JObject* o, Str_c* FailMessage) {

    // Require full_text and display range.
    if (!o->ExMembers.FullText || !o->ExMembers.DisplayRange) {
        STR_append(FailMessage, "Missing 'full_text' and/or 'display_text_range'.");
        return false;
    }

    // Validate text length with display range
    unsigned int TextSize = o->ExMembers.FullText->Length;
    if (o->ExMembers.DisplayRange->Begin != 0 ||
        o->ExMembers.DisplayRange->End != TextSize) {

        char Message[200];
        sprintf(Message, 
            "Display Range did not match the given full_text.\nText Size: %d DisplayRange: [%lli, %lli]",
            TextSize, o->ExMembers.DisplayRange->Begin, o->ExMembers.DisplayRange->End
        );
        STR_append(FailMessage, Message);
        return false;
    }
    
    const JString& TextObj = *o->ExMembers.FullText;

    // Even if we have NO hashtags in the text, we still need to verify that
    // there are no recorded hashtags in the array (but only if one exists.)

    int HashtagsInArray = 0;
    if (o->ExMembers.Entities) {
        HashtagsInArray = o->ExMembers.Entities->ExMembers.Hashtags->Hashtags.size;
    }

    if (HashtagsInArray != TextObj.Hashtags.size) {
        STR_append(FailMessage, "Hashtags found in text did not match all the hashtags in the entites.");
        return false;
    }

    // Hashtag count was equal. If it is also 0 then this is a valid extended_tweet
    if (HashtagsInArray == 0) {
        return true;
    }

    // The hashtags found in the entities array.
    Vec_Hashtags* EntitiesTags = &o->ExMembers.Entities->ExMembers.Hashtags->Hashtags;

    // All that is left is to verify hashtag positions on the actual text.
    // The hash tags could be in random order so do N^2 for now. 
    int i = 0;
    int j = 0;
    for (i = 0; i < TextObj.Hashtags.size; ++i) {
        const HashTagData* Outer = &TextObj.Hashtags.data[i];
        int found = 0;
        for (j = 0; j < EntitiesTags->size; ++j) {
            HashTagData* other = &EntitiesTags->data[j];  
            
            int equal = 
                Outer->Begin == other->Begin && 
                (strcmp(Outer->Tag.ptr, other->Tag.ptr) == 0);

            if (equal) {
                found = 1;
                break;
            }
        }

        if (!found) {
            char AddedStr[200];
            sprintf(AddedStr, "Hashtag: '%s' is missing from the entities array or has incorrect Indices.", Outer->Tag.ptr);
            STR_append(FailMessage, AddedStr);
            return false;
        }
    }

    return true;
}

boolean ExtractHashtags(JArray* a, Str_c* Error) {
    // This array must ONLY include objects that contain 'text', 'indices'
    // the array can be empty.
    // Actual checking for hashtag locations cannot be performed at this stage
    
    int i = 0;
    for (i = 0; i < a->Elements.size; ++i) {
        JValue* Element = a->Elements.data[i];
        // Must be an object type
        if (Element->Type != E_Object) {
            STR_append(Error, "An element of the array is not an object.");
            return false;
        }

        // Now check this object to find if it includes "text" & "indices" they are required.
        JObject* Subobject = Element->Data.ObjectData;
        if (!Subobject->Members.Text || !Subobject->ExMembers.Indices) {
            STR_append(Error, "An element of the array is missing 'text' and/or 'indices'.");
            return false;
        }

        // Also validate indice length. The actual # is not included in the text but it is accounted in indice length
        // Therefore we need to offset the length by 1
        long long IndiceLength = Subobject->ExMembers.Indices->End - Subobject->ExMembers.Indices->Begin;
        if (IndiceLength != Subobject->Members.Text->Length + 1) {
            STR_append(Error, "Indice range did not match the text length.");
            return false;

        }

        // Everything is correct. Collect this result and add it to the hashtags vector.
        HashTagData Data;
        Data.Tag = STR_make(Subobject->Members.Text->Text.ptr);
        Data.Begin = Subobject->ExMembers.Indices->Begin;

        VH_add(&a->Hashtags, Data);
    }
    return true;
}