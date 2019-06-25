#ifndef __C_COMP_H_
#define __C_COMP_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DB(z) fprintf(stderr, "%s\n", z);

struct Str_c {
    char* ptr;
    int len;

    static Str_c make(const char* text) {
        Str_c result;
        result.len = strlen(text);
        result.ptr = strdup(text);
        return result;
    }

    static Str_c makeEmpty() {
        Str_c result;
        result.init();
        return result;
    }

    void append(const char* text) {
        if (!len) {
            ptr = strdup(text);
            len = strlen(text);
            return;
        }
        len += strlen(text);
        ptr = (char*) realloc(ptr, (len + 1) * sizeof(char));
        strcat(ptr, text);
    }

    void addChar(char c) {
        len += 1;
        ptr = (char*) realloc(ptr, (len + 1) * sizeof(char));
        ptr[len - 1] = c;
        ptr[len] = '\0';
    }

    void init() {
        ptr = NULL;
        len = 0;
    }

    // free the current string, init as empty
    void clear() {
        if (ptr) {
            free(ptr);
        }
        init();
    }
};

static void printMultiple(char c, int times, FILE* fp) {
    for (int i = 0; i < times; ++i) {
        fputc(c, fp);
    }
}

#endif