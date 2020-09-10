#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

void chunkTest() {
    printf("\n<Chunk tests>\n");
    Chunk chnk;
    initChunk(&chnk);
    writeChunk(&chnk, OP_RETURN, 1);
    int constIndex = addConstant(&chnk, NUMBER_VAL(12.3));
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
    interpret("var a = \"aaa\"; print a + a + a;");
    freeVM();
    printf("\n</VM tests>\n");
}


static void repl() {
    char line[1024];
    while (true) {
        break;
    }
}

static char* readFile(const char* filePath) {
    FILE* file; 
    errno_t err = fopen_s(&file, filePath, "r");
    
    if (!file) {
        fprintf(stderr, "Could not open file \'%s\'\n", filePath);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(fileSize + 1);
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);

    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \'%s\'.\n", filePath);
        exit(EXIT_FAILURE);
    }

    if (bytesRead < fileSize) {
        fprintf(stderr, "Couldn't read file \'%s\'.\n", filePath);
    }

    buffer[bytesRead] = '\0';
    return buffer;
}

static void runFile(const char* filePath) {
    char* sourceCode = readFile(filePath);
    printf("running lox interpreter on file: '%s'\n", filePath);
    interpret(sourceCode);
}

static void runLox(int argc, char const* argv[]) {
    initVM();
    printf("cLox | Crafting Interpreters (Bob Nystrom).\n");

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path].\n");
    }

    freeVM();
}

int main(int argc, char const* argv[]) {
    runLox(argc, argv);
    //vmTest();
    return 0;
}
