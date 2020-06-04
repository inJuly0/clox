#include "vm.h"

#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "value.h"

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

InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = chunk->code;
    return run();
}