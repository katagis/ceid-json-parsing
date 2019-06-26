#ifndef __C_COMP_H_
#define __C_COMP_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef int boolean;
#define DB(z) fprintf(stderr, "%s\n", z);

typedef struct Str_c {
    char* ptr;
    int len;

} Str_c;

static void STR_init(Str_c* str) {
    str->ptr = NULL;
    str->len = 0;
}

static Str_c STR_make(const char* text) {
    Str_c result;
    result.len = strlen(text);
    result.ptr = strdup(text);
    return result;
}

static Str_c STR_makeEmpty() {
    Str_c result;
    STR_init(&result);
    return result;
}

static void STR_append(Str_c* str, const char* text) {
    if (!str->len) {
        str->ptr = strdup(text);
        str->len = strlen(text);
        return;
    }
    str->len += strlen(text);
    str->ptr = (char*) realloc(str->ptr, (str->len + 1) * sizeof(char));
    strcat(str->ptr, text);
}

static void STR_addChar(Str_c* str, char c) {
    str->len += 1;
    str->ptr = (char*) realloc(str->ptr, (str->len + 1) * sizeof(char));
    str->ptr[str->len - 1] = c;
    str->ptr[str->len] = '\0';
}

// free the current string, init as empty
static void STR_clear(Str_c* str) {
    if (str->ptr) {
        free(str->ptr);
    }
    STR_init(str);
}
    
static void STR_appendAsUtf8 (Str_c* str, long ucode) {
    if (ucode < 0x80) {
        STR_addChar(str, ucode);
    } 
    else if (ucode < 0x800) {
        STR_addChar(str, (ucode >> 6)   | 0xC0);
        STR_addChar(str, (ucode & 0x3F) | 0x80);
    }
    else {
        STR_addChar(str, ((ucode >> 12)       ) | 0xE0);
        STR_addChar(str, ((ucode >> 6 ) & 0x3F) | 0x80);
        STR_addChar(str, ((ucode      ) & 0x3F) | 0x80);
    }
}

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