#include "value.h"

#include <stdio.h>
#include <stdlib.h>

#include "memory.h"

// Value array is used to hold the values / constants
// It is the constant pool in the chunk.
// A value is just a typedef for the double data type

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

ValueArray* newValueArray() {
    ValueArray* temp;
    temp = (ValueArray*)reallocate(temp, 0, sizeof(ValueArray));
    initValueArray(temp);
    return temp;
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
    }
}

void pushValue(ValueStack* stack, Value value) {
    if ((stack->top - stack->values) > stack->size) {
        int oldSize = stack->size;
        stack->size = GROW_CAPACITY(stack->size);
        stack->values = GROW_ARRAY(stack->values, Value, oldSize, stack->size);
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
    stack->values = NULL;
    stack->top = NULL;
    stack->size = 0;
    free(stack);
}

void printValueStack(ValueStack* stack) {
    printf("           ");
    for (Value* slot = stack->values; slot < stack->top; slot++) {
        printf("[");
        printValue(*slot);
        printf("]");
    }
    printf("\n");
}