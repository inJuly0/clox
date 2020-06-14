#include "object.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
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
    return (ObjString*)allocateObject(size, OBJ_STRING);
}


ObjString* sumString(char* a, char* b, int lenA, int lenB) {
    int length = lenA + lenB;
    ObjString* sumstr = allocateString(length);

    int i = 0;

    for (; i < lenA; i++) {
        sumstr->chars[i] = a[i];
    }

    for (; i < length; i++) {
        sumstr->chars[i] = b[i - lenA];
    }
    sumstr->chars[i] = '\0';
    sumstr->length = length;
    return sumstr;
}

ObjString* copyString(const char* chars, int length) {

    ObjString* string = allocateString(length);
    string->length = length;

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