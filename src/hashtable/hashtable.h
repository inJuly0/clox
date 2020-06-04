#ifndef clox_hashtable_h
#define clox_hashtable_h
#include <stdint.h>

typedef struct {
    char* key;
    char* value;
} ht_item;

typedef struct {
    size_t capacity;
    size_t count;
    ht_item** items;
} ht_hash_table;

// This hash function is a slightly modified version of the one  from Brian
// Kernighan and Dennis Ritchie's book "The C Programming Language". I found
// this one at https://www.partow.net/programming/hashfunctions/#StringHashing

uint32_t BKDRHash(const char* str);

#endif