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

    // The contents of the last line we parsed. 
    std::vector<std::string> LineTexts = { "" };
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
        const std::string& last_line = LineTexts[LineNum];
        const size_t slice_start = last_line.length() > offset ? last_line.length() - offset : 0;
       
        std::cerr << "Failed to parse: '" << last_line.substr(slice_start) << "'\n";
        PrintLine(LineNum - 1);
        const int error_loc = PrintLine(LineNum) - offset - 1;
        
        std::cerr << std::string(9, '>') << " " << std::string(error_loc, '-') 
                << " " << std::string(LastMatch.length(), '^') << "\n";
    }

    void ReportLastTokenError() const {
        ReportErrorAtOffset(LastMatch.size());
    }

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

static int MakeInt(char* from) {
    return atoi(from);
}

} // util




#endif //__FLEX_UTIL_H_