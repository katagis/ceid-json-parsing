#ifndef __FLEX_UTIL_H_
#define __FLEX_UTIL_H_

#include <string.h>

namespace util {

char* MakeString(char* from) {
    return strndup(from + 1, strlen(from) - 2);
}

} // util

#endif //__FLEX_UTIL_H_