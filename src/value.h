#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

typedef struct {
    Value* top;
    size_t size;
    Value* values;
}ValueStack;

void initValueArray(ValueArray* array);
ValueArray* newValueArray();
void printValue(Value value);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

// stack

void initValueStack(ValueStack* stack, size_t size);
void freeValueStack(ValueStack* stack);
Value popValue(ValueStack* stack);
void pushValue(ValueStack* stack, Value value);
void printValueStack(ValueStack* stack);


#endif