#ifndef __FLEX_UTIL_H_
#define __FLEX_UTIL_H_

#include "c_comp.h"

#include <string.h>
#include <stdlib.h>

// Holds parse state, used for reporting errors.
typedef struct ParserState {
    // The line number we are currently parsing.
    int LineNum;

    // The contents of the everything we have parsed so far, by line.
    Vec_Str LineTexts;

    // The last token flex parsed.
    Str_c LastMatch;
   
} ParserState;

// Due to the way each file from flex & bison partialy includes one another we make this extern and define it in the .l
extern ParserState parser;

static void PS_Init() {
    parser.LineNum = 0;
    parser.LineTexts.init();
}

static void PS_Free() {
    parser.LineTexts.clear();
    parser.LastMatch.clear();
}

// Called always when there is a flex rule match
static void PS_Match(char* text) {
    parser.LineTexts.data[parser.LineNum].append(text);
    parser.LastMatch.clear();
    parser.LastMatch.append(text);
}

// Once we found a \n
static void PS_CountLine() {
    ++parser.LineNum;
    parser.LineTexts.push_back(Str_c::makeEmpty());
}

// Prints a "pretty" formatted line
static int PS_PrintLine(int Index) {
    if (Index < 0) {
        return 0;
    }
    fprintf(stderr, "Line %3.d: %s\n", Index, parser.LineTexts.data[Index].ptr);
    return parser.LineTexts.data[Index].len;
}

// Prints a "pretty" fromatted error including the previous line for context.
static void PS_ReportErrorAtOffset(int offset) {
    if (parser.LineTexts.data[parser.LineNum].len > 0) {
        int start_index = parser.LineTexts.data[parser.LineNum].len - offset;
        if (start_index < 0) {
            start_index = 0;
        }
        fprintf(stderr, "Failed to parse: '%s'\n", (parser.LineTexts.data[parser.LineNum].ptr + start_index));
    }

    PS_PrintLine(parser.LineNum - 1);
    int error_loc = PS_PrintLine(parser.LineNum) - offset;

    printMultiple('>', 9, stderr);
    printMultiple('-', error_loc, stderr);
    fputc(' ', stderr);
    printMultiple('^', parser.LastMatch.len, stderr);
    fputc('\n', stderr);
}

static void PS_ReportLastTokenError() {
        if (parser.LastMatch.len) {
            PS_ReportErrorAtOffset(parser.LastMatch.len);
        }
    }

// Called directly from yyerror() and prints the last 2 lines parsed with the last
// matched token underlined
static void PS_ReportError(const char* reason) {
    PS_ReportLastTokenError();
    fprintf(stderr, "Reason: %s\n", reason);
}

static char* util_MakeString(char* from) {
    return strndup(from + 1, strlen(from) - 2);
}

static float util_MakeFloat(char* from) {
    return atof(from);
}

static long long util_MakeInt(char* from) {
    return atoll(from);
}

#endif //__FLEX_UTIL_H_