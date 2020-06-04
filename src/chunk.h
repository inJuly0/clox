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

    OP_CONSTANT 1 would mean grab the first constant in this chunk's constant pool

    "int* line" parallels the opcode array. 
    an Op code with index i in the bytecode array occurs on line lines[i] in the 
    source file.
*/

typedef enum {
    OP_RETURN,
    OP_CONSTANT,
    OP_NEGATE,
    OP_MULT,
    OP_DIV,
    OP_ADD,
    OP_SUB
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
// code: Opcode, line: line
void writeChunk(Chunk* chunk, uint8_t code, int line);
int addConstant(Chunk* chunk, Value constant);
Chunk* newChunk();

#endif