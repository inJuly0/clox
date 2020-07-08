#include "table.h"

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
    table->count = 0;
    table->cap = 0;
    table->entries = NULL;
}

void freeTable(Table* table) {
    free(table->entries);
    initTable(table);
}

static Entry* findEntry(Entry* entries, int cap, ObjString* key) {
    uint32_t index = key->hash % cap;
    while (true) {
        Entry* entry = &entries[index];

        if (entry->key == key || entry->key == NULL) {
            return entry;
        }

        index = (index + 1) % cap;
    }
}

static void adjustCapacity(Table* table, int cap) {
    Entry* entries = ALLOCATE(Entry, cap);
    for (int i = 0; i < cap; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    for (int i = 0; i < table->cap; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        Entry* dest = findEntry(entries, cap, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
    }

    FREE_ARRAY(table->entries, Entry, table->cap);
    table->entries = entries;
    table->cap = cap;
}

bool tableGet(Table* table, ObjString* key, Value* valueOut) {
    if (table->count == 0) return false;
    Entry* entry = findEntry(table->entries, table->cap, key);
    if (entry->key == NULL) return false;

    *valueOut = entry->value;
    return true;
}

bool tableSet(Table* table, ObjString* key, Value value) {
    if (table->count + 1 > table->cap) {
        int cap = GROW_CAPACITY(table->cap);
        adjustCapacity(table, cap);
    }

    Entry* entry = findEntry(table->entries, table->cap, key);

    bool isNewKey = entry->key == NULL;
    if (isNewKey) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(Table* table, ObjString* key, Value value) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->cap, key);
    if (entry->key == NULL) return false;

    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->cap; i++) {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}
