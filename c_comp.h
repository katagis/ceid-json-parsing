#ifndef __C_COMP_H_
#define __C_COMP_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DB(z) fprintf(stderr, "%s\n", z);

typedef struct Str_c {
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
        
    void appendAsUtf8 (long ucode) {
        if (ucode < 0x80) {
            addChar(ucode);
        }
        else if (ucode < 0x800) {
            addChar((ucode >> 6)   | 0xC0);
            addChar((ucode & 0x3F) | 0x80);
        }
        else {
            addChar(((ucode >> 12)       ) | 0xE0);
            addChar(((ucode >> 6 ) & 0x3F) | 0x80);
            addChar(((ucode      ) & 0x3F) | 0x80);
        }
    }
} Str_c;

static void printMultiple(char c, int times, FILE* fp) {
    int i = 0;
    for (i = 0; i < times; ++i) {
        fputc(c, fp);
    }
}


#define VecDefine(NAME, VEC_TYPE)  \
typedef struct NAME {  \
    VEC_TYPE* data; \
    unsigned int num;   \
    unsigned int slack; \
    \
    void init() {   \
        data = (VEC_TYPE*) malloc(10 * sizeof(VEC_TYPE));   \
        num = 0;    \
        slack = 10; \
    }   \
    \
    unsigned int size() const { \
        return num; \
    }   \
    \
    void push_back(VEC_TYPE elem) { \
        if (num + 1 >= slack) { \
            expand();   \
        }   \
        data[num++] = elem; \
    }   \
    \
    void expand() { \
        if (slack == 0) {   \
            init(); \
            return; \
        }   \
        slack += slack; \
        data = (VEC_TYPE*) realloc(data, slack * sizeof(VEC_TYPE)); \
    }   \
    \
    void clear()  { \
        if (data) { \
            free(data); \
        }   \
        num = 0;    \
        slack = 0;  \
    }   \
} NAME

VecDefine(Vec_LongLong, long long);
VecDefine(Vec_Str, Str_c);

#define true 1
#define false 0


#endif