#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"

// Value array is used to hold the values / constants
// It is the constant pool in the chunk.
// A value is just a typedef for the double data type

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;

    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: {
            ObjString* aString = AS_STRING(a);
            ObjString* bString = AS_STRING(b);
            return aString->length == bString->length &&
                   memcmp(aString->chars, bString->chars, aString->length) == 0;
        }
    }
}

void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value));
            break;
        case VAL_OBJ:
            printObject(value);
            break;
    }
}

// value array functions

void initValueArray(ValueArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void writeValueArray(ValueArray* array, Value value) {
    if (array->count + 1 > array->capacity) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values =
            GROW_ARRAY(array->values, Value, oldCapacity, array->capacity);
    }
    array->values[array->count++] = value;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(array->values, Value, array->count);
    initValueArray(array);
}

// Value stack functions

void pushValue(ValueStack* stack, Value value) {
    if ((stack->top - stack->values) > stack->size) {
        int oldSize = stack->size;
        stack->size = GROW_CAPACITY(stack->size);
        stack->values = GROW_ARRAY(stack->values, Value, oldSize, stack->size);
        stack->top = stack->values + oldSize;
    }
    *(stack->top) = value;
    stack->top++;
}

Value popValue(ValueStack* stack) { return *(--stack->top); }

void initValueStack(ValueStack* stack, size_t size) {
    stack->values = NULL;
    stack->values = GROW_ARRAY(stack->values, Value, 0, size);
    stack->size = size;
    stack->top = stack->values;
}

void freeValueStack(ValueStack* stack) {
    free(stack->values);
    stack->top = NULL;
    stack->size = 0;
}

void printValueStack(ValueStack* stack) {
    printf("           ");
    for (Value* slot = stack->values; slot < stack->top; slot++) {
        printf("[");
        printValue(*slot);
        printf("]");
    }
}