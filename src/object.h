#ifndef clox_object_h
#define clox_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_STRING,
} ObjType;

struct sObj {
    ObjType type;
    struct sObj* next;
};

struct sObjString {
    Obj obj;
    int length;
    // using flexible array members to store the string in-place in the struct
    char chars[];
};

// ObjString* takeString(char* chars, int length);
/* takes two character buffers and returns a string object
 whose characters are the concactenated buffers*/
ObjString* sumString(char* a, char* b, int lenA, int lenB);
ObjString* copyString(const char* chars, int length);
void printObject(Value object);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif