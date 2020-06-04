#include "hashtable.h"

#include <stdlib.h>
#include <string.h>

uint32_t BKDRHash(const char* str) {
    uint32_t seed = 131; /* 31 131 1313 13131 131313 etc.. */
    uint32_t hash = 0;

    for (uint32_t i = 0; str[i]; ++str, ++i) {
        hash = (hash * seed) + (*str);
    }

    return hash;
}