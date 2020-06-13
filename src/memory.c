#include "memory.h"

#include <stdlib.h>

#include "object.h"
#include "vm.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    return realloc(pointer, newSize);
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}

static void freeObject(Obj* object){
    switch (object->type)
    {
    case OBJ_STRING:
        ObjString* string = (ObjString*)object;
        FREE_ARRAY(string->chars, char , string->length + 1);
        FREE(ObjString, object);
        break;
    
    default:
        break;
    }
}