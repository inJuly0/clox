#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include "compiler.h"

void chunkTest() {
    printf("\n<Chunk tests>\n");
    Chunk chnk;
    initChunk(&chnk);
    writeChunk(&chnk, OP_RETURN, 1);
    int constIndex = addConstant(&chnk, 12.3);
    writeChunk(&chnk, OP_CONSTANT, 1);
    writeChunk(&chnk, constIndex, 1);
    writeChunk(&chnk, OP_NEGATE, 1);
    disassembleChunk(&chnk, "test chunk");
    freeChunk(&chnk);
    printf("\n</Chunk tests>\n");
}

void vmTest() {
    printf("\n<VM tests>\n");
    initVM();
    Chunk chnk;
    initChunk(&chnk);
    int constIndex = addConstant(&chnk, 5);
    writeChunk(&chnk, OP_CONSTANT, 1);
    writeChunk(&chnk, constIndex, 1);
    writeChunk(&chnk, OP_NEGATE, 1);
    int constIndex2 = addConstant(&chnk, 7);
    writeChunk(&chnk, OP_CONSTANT, 1);
    writeChunk(&chnk, constIndex2, 1);
    writeChunk(&chnk, OP_MULT, 1);
    writeChunk(&chnk, OP_RETURN, 1);
    // interpret(&chnk);
    freeChunk(&chnk);
    freeVM();
    printf("\n</VM tests>\n");
}

void compilerTest(){
    // compile("var anum = -1 * 2 or false\0");
}

static void repl() {
    char line[1024];
    while (true) {
        break;
    }
}

static char* readFile(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    if (!file) {
        fprintf(stderr, "Could not open file \'%s\'\n", filePath);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(fileSize + 1);
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if(buffer == NULL){
        fprintf(stderr, "Not enough memory to read \'%s\'", filePath);
        exit(EXIT_FAILURE);
    }
    if(bytesRead < fileSize) {
        fprintf(stderr, "Couldn't read file \'%s\'.\n");
    }
    buffer[bytesRead] = '\0';
    return buffer;
}

static void runFile(const char* filePath) {}

static void runLox(int argc, char const* argv[]){
    initVM();
    printf("cLox | Crafting Interpreters (Bob Nystrom).\n");
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        char* s = readFile("vm.c");
        fprintf(stderr, "Usage: clox [path].\n");
        printf("%s", s);
    }
    freeVM();
}

int main(int argc, char const* argv[]) {
    // runLox(argc, argv);
    compilerTest();
    return 0;
}


// compile using : gcc memory.c value.c chunk.c debug.c scanner.c compiler.c vm.c main.c