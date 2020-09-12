#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/*
    CHUNK:
    A chunk is a wrapper around an array of bytes or
    op codes that are to be executed in the VM
    A chunk also has a constant pool, which is an array of
    values (doubles).

    Whenever the VM comes across a OP_CONSTANT, the very next instruction
    is an index of the constant to be loaded in the constant pool

    OP_CONSTANT 1 would mean grab the first constant in this chunk's constant
   pool

    "int* line" parallels the opcode array.
    an Op code with index i in the bytecode array occurs on line lines[i] in the
    source file.
*/

typedef enum {
    OP_RETURN,
    OP_CONSTANT,
    OP_NIL,
    OP_NOT,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_LESS,
    OP_GREATER,
    OP_NEGATE,
    OP_MULT,
    OP_DIV,
    OP_ADD,
    OP_SUB,
    OP_PRINT,
    OP_POP,
    OP_POPN,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    OP_JUMPZ,
    OP_JUMP
} OpCode;

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t* code;
    ValueArray constants;
    int* lines;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t code, int line);
int addConstant(Chunk* chunk, Value constant);

#endif