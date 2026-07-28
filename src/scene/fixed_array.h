#pragma once

#include <stdlib.h>
#include <string.h>


#define FixedArray(T) \
typedef struct { \
    T* data; \
    size_t size; \
} T##_array;
