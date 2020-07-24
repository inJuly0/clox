#ifndef clox_table_h
#define clox_table_h
#include "common.h"
#include "value.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    int count;
    int cap;
    Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableGet(Table* table, ObjString* key, Value* valueOut);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char chars[], int length,
                           uint32_t hash);

#endif