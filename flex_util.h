#ifndef __FLEX_UTIL_H_
#define __FLEX_UTIL_H_

#include "c_comp.h"

#include <string.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <iomanip>

// Holds parse state, used for reporting errors.
struct ParserState {
    // The line number we are currently parsing.
    int LineNum = 0;

    // The contents of the everything we have parsed so far, by line.
    std::vector<Str_c> LineTexts = { Str_c::makeEmpty() };

    // The last token flex parsed.
    Str_c LastMatch;

    // Once we found a \n
    void CountLine() {
        ++LineNum;
        LineTexts.push_back(Str_c::makeEmpty());
    }

    // Called always when there is a flex rule match
    void Match(char* text) {
        LineTexts[LineNum].append(text);
        LastMatch.clear();
        LastMatch.append(text);
    }

    // Prints a "pretty" formatted line
    int PrintLine(int Index) const {
        if (Index < 0) {
            return 0;
        }
        fprintf(stderr, "Line %3.d: %s\n", Index, LineTexts[Index].ptr);
        return LineTexts[Index].len;
    }

    // Prints a "pretty" fromatted error including the previous line for context.
    void ReportErrorAtOffset(int offset) const {
        if (LineTexts[LineNum].len > 0) {
            int start_index = LineTexts[LineNum].len - offset;
            if (start_index < 0) {
                start_index = 0;
            }
            fprintf(stderr, "Failed to parse: '%s'\n", (LineTexts[LineNum].ptr + start_index));
        }

        PrintLine(LineNum - 1);
        int error_loc = PrintLine(LineNum) - offset;

        printMultiple('>', 9, stderr);
        printMultiple('-', error_loc, stderr);
        fputc(' ', stderr);
        printMultiple('^', LastMatch.len, stderr);
        fputc('\n', stderr);
    }

    void ReportLastTokenError() const {
        if (LastMatch.len) {
            ReportErrorAtOffset(LastMatch.len);
        }
    }

    // Called directly from yyerror() and prints the last 2 lines parsed with the last
    // matched token underlined
    void ReportError(const std::string& reason) const {
        ReportLastTokenError();
        fprintf(stderr, "Reason: %s\n", reason.c_str());
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