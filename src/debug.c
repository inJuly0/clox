#include "debug.h"

#include <stdio.h>

#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n\n", name);
    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t index = chunk->code[offset + 1];
    Value constant = chunk->constants.values[index];
    printf("%-16s\t%4d '", name, index);
    printValue(constant);
    printf("'\n");
    return offset + 2;
}

static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s\t%4d\n", name, slot);
    return offset + 2;
}

static int jumpInstruction(char* name, Chunk* chunk, int offset) {
    uint8_t low = chunk->code[offset + 1];
    uint8_t high = chunk->code[offset + 2];

    uint16_t slot = (uint16_t)((low << 8) | (high & 0xff));
    printf("%-16s\t%4d\n", name, slot);
    return offset + 3;
}

int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d\t", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("   | ");
    else
        printf("%4d ", chunk->lines[offset]);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUB:
            return simpleInstruction("OP_SUB", offset);
        case OP_MULT:
            return simpleInstruction("OP_MULT", offset);
        case OP_DIV:
            return simpleInstruction("OP_DIV", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_POPN:
            return byteInstruction("OP_POPN", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_JUMPZ:
            return jumpInstruction("OP_JUMPZ", chunk, offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        default:
            printf("Unknown opcode.. %d\n", chunk->code[offset]);
            return offset + 1;
    }
}
