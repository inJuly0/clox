#include "vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

static void push(Value val) { pushValue(&vm.stack, val); }

static Value pop() { return popValue(&vm.stack); }

static Value peek(size_t distance) { return vm.stack.top[-1 - distance]; }

void printStack() {
    // printValueStack(&vm.stack);
    printf(" (%d)\n\n", (int)(vm.stack.top - vm.stack.values));
}

static Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->function;
        // -1 because the IP is sitting on the next instruction to be
        // executed.
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    initValueStack(&vm.stack, STACK_SIZE);
}

static void defineNative(const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack.values[0]), vm.stack.values[1]);
    pop();
    pop();
}

void initVM() {
    initValueStack(&vm.stack, STACK_SIZE);
    initTable(&vm.strings);
    initTable(&vm.globals);
    vm.objects = NULL;
    vm.frameCount = 0;

    defineNative("clock", clockNative);
}

void freeVM() {
    freeValueStack(&vm.stack);
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeObjects();
}

static bool isFalsey(Value value) {
    // a value is falsy if it's either nil or
    // if it's a boolean and it's value is 'false'
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());
    int len = b->length + a->length;
    ObjString* result = xallocateString(len);

    for (int i = 0; i < a->length; i++) {
        result->chars[i] = a->chars[i];
    }

    for (int i = 0; i < b->length; i++) {
        result->chars[a->length + i] = b->chars[i];
    }

    result = validateString(result);
    push(OBJ_VAL(result));
}

static bool call(ObjFunction* function, int argCount) {
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.", function->arity,
                     argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;

    frame->slots = vm.stack.top - argCount - 1;

    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION:
                return call(AS_FUNCTION(callee), argCount);

            case OBJ_NATIVE:
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stack.top - argCount);
                vm.stack.top -= argCount + 1;
                push(result);
                return true;
            default:
                // Non-callable object type.
                break;
        }
    }

    runtimeError("Can only call functions and classes.");
    return false;
}

static InterpretResult run() {
#define BINARY_OP(valueType, op)                          \
    do {                                                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(pop());                      \
        double a = AS_NUMBER(pop());                      \
        push(valueType(a op b));                          \
    } while (false)

    CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*(frame->ip++))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() (AS_STRING(READ_CONSTANT()))
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | (frame->ip[-1])))
    while (true) {
#ifdef DEBUG_TRACE_EXECTUION
        printStack();
        disassembleInstruction(&frame->function->chunk,
                               (int)(frame->ip - frame->function->chunk.code));
#endif
        uint8_t instruction = READ_BYTE();
        Value valA, valB;
        switch (instruction) {
            case OP_RETURN:
                Value result = pop();
                vm.frameCount--;

                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stack.top = frame->slots;
                push(result);

                frame = &vm.frames[vm.frameCount - 1];
                break;
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_MULT:
                BINARY_OP(NUMBER_VAL, *);
                break;
            case OP_ADD:
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError(
                        "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
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
            // having OP_TRUE
            // and OP_FALSE is cheaper
            // than storing them as value structs
            // in the chunk's constant pool.
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;
            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;
            case OP_LESS:
                BINARY_OP(BOOL_VAL, <);
                break;
            case OP_EQUAL:
                valA = pop();
                valB = pop();
                push(BOOL_VAL(valuesEqual(valA, valB)));
                break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;
            case OP_POP:
                pop();
                break;
            case OP_POPN: {
                uint8_t count = READ_BYTE();
                while (count--) pop();
                break;
            }
            case OP_DEFINE_GLOBAL: {
                // peek becausesomething something garbage collection
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }

            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined global '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }

            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                // if the hash table doesn't already have a string
                // going by that name then it creates a new key
                // and then retruns true (isNewKey). Then we know
                // that the global wasn't already defined and throw an
                // error
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }

            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }

            case OP_JUMPZ: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) frame->ip += offset;
                break;
            }

            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }

            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }

            case OP_CALL: {
                int argCount = READ_BYTE();

                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
        }
    }

#undef READ_CONSTANT
#undef READ_BYTE
#undef READ_STRING
#undef READ_SHORT
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    // the compiler puts all the bytecode
    // into the chunk's opcode array
    ObjFunction* function = compile(source);

    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    // if there are no compile errors
    // then pass the chunk over to
    // the VM to interpret and run the
    // bytecode
    push(OBJ_VAL(function));
    callValue(OBJ_VAL(function), 0);

    return run();
}