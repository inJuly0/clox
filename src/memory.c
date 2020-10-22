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

static void freeObject(Obj* object) {
  switch (object->type) {
  case OBJ_STRING: {
    ObjString* string = (ObjString*)object;
    FREE(ObjString, string);
    break;
  }

  case OBJ_FUNCTION: {
    ObjFunction* func = (ObjFunction*)object;
    freeChunk(&func->chunk);
    FREE(ObjFunction, object);
    break;
  }

  case OBJ_NATIVE:
    FREE(ObjNative, object);
    break;

  case OBJ_CLOSURE:
    ObjClosure* closure = (ObjClosure*)object;
    FREE_ARRAY(closure->upvalues, ObjUpvalue*, closure->upvalueCount);
    FREE(ObjClosure, object);
    break;

  case OBJ_UPVALUE:
    FREE(ObjUpvalue, object);
    break;
  default:
    break;
  }
}

void freeObjects() {
  Obj* object = vm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }
}
