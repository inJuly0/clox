#ifndef clox_object_h
#define clox_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)

#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))

typedef enum { OBJ_STRING, OBJ_FUNCTION, OBJ_NATIVE, OBJ_CLOSURE } ObjType;

struct sObj {
  ObjType type;
  struct sObj *next;
};

typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

typedef struct {
  Obj obj;
  ObjFunction *function;
} ObjClosure;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

struct sObjString {
  Obj obj;
  int length;
  uint32_t hash;
  // using flexible array members to store the string in-place in the struct
  char chars[];
};

ObjFunction *newFunction();
ObjClosure *newClosure(ObjFunction *function);
ObjNative *newNative(NativeFn function);
ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
ObjString *xallocateString(int length);
ObjString *validateString(ObjString *string);
void printObject(Value object);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif