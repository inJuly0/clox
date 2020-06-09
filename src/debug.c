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

int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d\t", offset);
    if(offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("   | ");
    else printf("%4d ", chunk->lines[offset]);
    
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
        default:
            printf("Unknown opcode.\n");
            return offset + 1;
    }
}
