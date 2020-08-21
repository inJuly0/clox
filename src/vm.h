#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"
#include "table.h"

// initial stack size
// grows further dynamically as values are added
#define STACK_SIZE 256

/* stackTop points to where the next element is
 supposed to go. (points to an unused slot) */

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    ValueStack stack;
    Table strings;
    Obj* objects;
    Table globals;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_COMPILE_ERROR
} InterpretResult;


VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);

#endif