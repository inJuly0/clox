#include "vm.h"

#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"

VM vm;

static void push(Value val) { pushValue(&vm.stack, val); }

static Value pop() { return popValue(&vm.stack); }

static Value peek(size_t distance) { return vm.stack.top[-1 - distance]; }

void printStack() { printValueStack(&vm.stack); }

static void runTimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script", line);

    initValueStack(&vm.stack, STACK_SIZE);
}

void initVM() { initValueStack(&vm.stack, STACK_SIZE); }

void freeVM() { freeValueStack(&vm.stack); }

static InterpretResult run() {
#define BINARY_OP(valueType, op)                          \
    do {                                                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runTimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(pop());                      \
        double a = AS_NUMBER(pop());                      \
        push(valueType(a op b));                          \
    }                                                     \
    while (false)
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
                if (!IS_NUMBER(peek(0))) {
                    runTimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_MULT:
                BINARY_OP(NUMBER_VAL, *);
                break;
            case OP_ADD:
                BINARY_OP(NUMBER_VAL, +);
                break;
            case OP_SUB:
                BINARY_OP(NUMBER_VAL, -);
                break;
            case OP_DIV:
                BINARY_OP(NUMBER_VAL, /);
                break;
            case OP_NIL:
                push(NIL_VAL);
                break;
            case OP_TRUE: 
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
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

    if (!compile(source, &chunk)) {
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