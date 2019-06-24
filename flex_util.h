#ifndef __FLEX_UTIL_H_
#define __FLEX_UTIL_H_

#include <string.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>

// Holds parse state, used for reporting errors.
struct ParserState {
    // The line number we are currently parsing.
    int LineNum = 0;

    // The contents of the everything we have parsed so far, by line.
    std::vector<std::string> LineTexts = { "" };

    // The last token flex parsed.
    std::string LastMatch = "";

    // Once we found a \n
    void CountLine() {
        ++LineNum;
        LineTexts.push_back(std::string());
    }

    // Called always when there is a flex rule match
    void Match(char* text) {
        LineTexts[LineNum].append(text);
        LastMatch = text;
    }

    // Prints a "pretty" formatted line
    int PrintLine(int Index) const {
        if (Index < 0) {
            return 0;
        }
        std::cerr << "Line " << std::setw(3) << Index << ": " << LineTexts[Index] << "\n";
        return LineTexts[Index].length();
    }

    // Prints a "pretty" fromatted error including the previous line for context.
    void ReportErrorAtOffset(int offset) const {
        std::string error_token = "";
        
        const std::string& last_line = LineTexts[LineNum];
        const size_t slice_start = last_line.length() > offset ? last_line.length() - offset : 0;
       
       if (last_line.length() > 0) {
           error_token = last_line.substr(slice_start);
       }

        std::cerr << "Failed to parse: '" << error_token << "'\n";
        PrintLine(LineNum - 1);
        const int error_loc = PrintLine(LineNum) - offset;
        
        std::cerr << std::string(9, '>') << std::string(std::max(error_loc, 0), '-') 
                << " " << std::string(LastMatch.length(), '^') << "\n";
    }

    void ReportLastTokenError() const {
        if (LastMatch.size()) {
            ReportErrorAtOffset(LastMatch.size());
        }
    }

    // Called directly from yyerror() and prints the last 2 lines parsed with the last
    // matched token underlined
    void ReportError(const std::string& reason) const {
        ReportLastTokenError();
        std::cerr << "Reason: " << reason << "\n";
    }
};

// Due to the way each file from flex & bison partialy includes one another we make this extern and define it in the .l
extern ParserState parse;

namespace util {
static char* MakeString(char* from) {
    return strndup(from + 1, strlen(from) - 2);
}

static float MakeFloat(char* from) {
    return atof(from);
}

static long long MakeInt(char* from) {
    return atoll(from);
}
} // util

#endif //__FLEX_UTIL_H_