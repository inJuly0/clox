#include "object.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

static ObjString* allocateString(int length) {
    // size of an ObjString + number of characters + one extra
    // for null terminator
    int size = sizeof(ObjString) + sizeof(char) * (length + 1);
    ObjString* string = (ObjString*)allocateObject(size, OBJ_STRING);
    tableSet(&vm.strings, string, NIL_VAL);
    return string;
}

static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }

    return hash;
}

ObjString* sumString(char* a, char* b, int lenA, int lenB) {
    int length = lenA + lenB;
    int size = sizeof(ObjString) + sizeof(char) * (length + 1);
    ObjString* sumstr = (ObjString*)allocateObject(size, OBJ_STRING);
    int i = 0;

    for (; i < lenA; i++) {
        sumstr->chars[i] = a[i];
    }

    for (; i < length; i++) {
        sumstr->chars[i] = b[i - lenA];
    }
    sumstr->chars[i] = '\0';
    sumstr->length = length;
    sumstr->hash = hashString(sumstr->chars, length);
    ObjString* interned = tableFindString(&vm.strings, sumstr->chars,
                                          sumstr->length, sumstr->hash);

    if (interned == NULL) {
        tableSet(&vm.strings, sumstr, NIL_VAL);
        return sumstr;
    }

    FREE_ARRAY(sumstr->chars, char, length);
    return interned;
}

ObjString* copyString(const char* chars, int length) {
    ObjString* string = allocateString(length);
    string->length = length;
    string->hash = hashString(chars, length);
    ObjString* interned =
        tableFindString(&vm.strings, chars, length, string->hash);
    if (interned != NULL) return interned;

    for (int i = 0; i < length; i++) {
        string->chars[i] = chars[i];
    }

    string->chars[length] = '\0';
    return string;
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}