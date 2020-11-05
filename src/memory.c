#include "memory.h"
#include "common.h"
#include <stdlib.h>

#include "object.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include "debug.h"
#include <stdio.h>
#endif

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif
  }

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }
  return realloc(pointer, newSize);
}

static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#endif

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

static void markArray();

static void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

  switch (object->type) {
  case OBJ_NATIVE:
  case OBJ_STRING:
    break;
  case OBJ_UPVALUE:
    markValue(((ObjUpvalue*)object)->closed);
    break;
  case OBJ_FUNCTION: {
    ObjFunction* func = (ObjFunction*)object;
    markObject((Obj*)func->name);
    markArray(&(func->chunk.constants));
    break;
  }
  case OBJ_CLOSURE: {
    ObjClosure* closure = (ObjClosure*)object;
    markObject((Obj*)closure->function);
    for (int i = 0; i < closure->upvalueCount; i++) {
      markObject((Obj*)closure->upvalues[i]);
    }
    break;
  }
  default:
    break;
  }
}

static void markRoots() {
  for (Value* slot = vm.stack.values; slot < vm.stack.top; slot++) {
    markValue(*slot);
  }

  for (int i = 0; i < vm.frameCount; i++) {
    markObject((Obj*)vm.frames[i].closure);
  }

  for (ObjUpvalue* upval = vm.openUpvalues; upval != NULL;
       upval = upval->next) {
    markObject((Obj*)upval);
  }
}

static void traceRefs() {
  while (vm.grayCount > 0) {
    Obj* object = vm.grayStack[--vm.grayCount];
    blackenObject(object);
  }
}

static void markArray(ValueArray* array) {
  for (int i = 0; i < array->count; i++) {
    markValue(array->values[i]);
  }
}

void collectGarbage() {
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
#endif

  markRoots();
  traceRefs();

#ifdef DEBUG_LOG_GC
  printf("\n-- gc end\n");
#endif
}

void markValue(Value val) {
  if (!IS_OBJ(val))
    return;
  markObject(AS_OBJ(val));
}

void markObject(Obj* object) {
  if (object == NULL)
    return;
  if (object->isMarked)
    return;
#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  object->isMarked = true;

  if (vm.grayCapacity < vm.grayCount + 1) {
    vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
    vm.grayStack = realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
    if (vm.grayStack == NULL)
      exit(1);
  }
  vm.grayStack[vm.grayCount++] = object;
}