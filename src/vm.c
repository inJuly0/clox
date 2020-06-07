#include "vm.h"

#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "value.h"
#include "compiler.h"

VM vm;

static void push(Value val) { pushValue(&vm.stack, val); }

static Value pop() { return popValue(&vm.stack); }

void printStack() { printValueStack(&vm.stack); }

void initVM() { initValueStack(&vm.stack, STACK_SIZE); }

void freeVM() { freeValueStack(&vm.stack); }

static InterpretResult run() {
#define BINARY_OP(op)     \
    do {                  \
        double b = pop(); \
        double a = pop(); \
        push(a op b);     \
    } while (false)
#define READ_BYTE() (*(vm.ip++))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    while (true) {
#ifdef DEBUG_TRACE_EXECTUION
        printStack();
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
            case OP_RETURN:
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NEGATE:
                push(-pop());
                break;
            case OP_MULT:
                BINARY_OP(*);
                break;
            case OP_ADD:
                BINARY_OP(+);
                break;
            case OP_SUB:
                BINARY_OP(-);
                break;
            case OP_DIV:
                BINARY_OP(/);
                break;
        }
    }

#undef READ_CONSTANT
#undef READ_BYTE
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {

    Chunk chunk;
    initChunk(&chunk);

    // the compiler puts all the bytecode 
    // into the chunk's opcode array

    if(!compile(source, &chunk)){
        // throw error and return
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    // if there are no compile errors
    // then pass the chunk over to 
    // the VM to interpret and run the 
    // bytecode

    vm.chunk = &chunk;
    vm.ip = chunk.code;
    
    // we can't just do return run() because 
    // we have to free the chunk before exiting the function

    InterpretResult result = run();
    freeChunk(&chunk);
    return result;
}