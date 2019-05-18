#ifndef __FLEX_UTIL_H_
#define __FLEX_UTIL_H_

#include <string.h>
#include <stdlib.h>

namespace util {

char* MakeString(char* from) {
    return strndup(from + 1, strlen(from) - 2);
}

float MakeFloat(char* from) {
    return atof(from);
}

int MakeInt(char* from) {
    return atoi(from);
}

} // util

#endif //__FLEX_UTIL_H_