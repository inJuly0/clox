#ifndef clox_memory_h
#define clox_memory_h
#include "common.h"
#define GROW_CAPACITY(capacity) (((capacity) < 8) ? 8 : (capacity)*2)

void* reallocate(void* pointer, size_t oldCapacity, size_t newCapacity);

// clever macro
// moves a bunch of bytes from one position in memory to 
// a larger position in memory with more empty space

#define GROW_ARRAY(arr, type, oldCapacity, newCapacity)  \
    (type*)reallocate(arr, sizeof(type) * (oldCapacity), \
                      sizeof(type) * (newCapacity))

#define FREE_ARRAY(arr, type, oldCount) \
    reallocate(arr, sizeof(type) * (oldCount), 0)

#endif