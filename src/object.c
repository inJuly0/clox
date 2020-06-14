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

// static ObjString* allocateString(char* chars, int length) {
//     ObjString* string = (ObjString*)allocateObject(
//         sizeof(ObjString) + length * sizeof(char), OBJ_STRING);
//     string->length = length;
//     string->chars = chars;
//     return string;
// }

ObjString* takeString(char* chars, int length) {
    // return allocateString(chars, length);
    return NULL;
}

ObjString* sumString(char* a, char* b, int lenA, int lenB) {
    int length = lenA + lenB;
    size_t size = sizeof(ObjString) + sizeof(char) * (length + 1);
    ObjString* sumstr = (ObjString*)allocateObject(size, OBJ_STRING);

    int i = 0;

    for (; i < lenA; i++) {
        sumstr->chars[i] = a[i];
    }

    for (; i < length; i++) {
        sumstr->chars[i] = b[i - lenA];
    }
    sumstr->chars[i] = '\0';
    return sumstr;
}

ObjString* copyString(const char* chars, int length) {
    // the characters of the string exist in place as a flexible
    // array member
    size_t size = sizeof(ObjString) + ((length + 1) * sizeof(char));
    ObjString* string = (ObjString*)allocateObject(size, OBJ_STRING);

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