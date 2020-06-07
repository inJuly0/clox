#include "compiler.h"

#include <stdio.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    Token previous;
    Token current;
    bool hadError;
    bool panicMode;
} Parser;

Parser parser;
Chunk* compilingChunk;

// helper functions

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error: ", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // do nothing
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char* message) { errorAt(&parser.current, message); }

static void errorAtCurrent(const char* message) {}

Token advance() {
    parser.previous = parser.current;
    // the while loop helps
    // skip the error tokens
    // that are produced by the lexer
    while (true) {
        parser.current = scanToken();
        // break if the current token is not error
        // if it is an error then report the error
        // and eat following error tokens if any
        if (parser.current.type != TOKEN_ERROR) break;
        errorAtCurrent(parser.current.start);
    }
}

void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static Chunk* currentChunk() { return compilingChunk; }

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() { emitByte(OP_RETURN); }

static void endCompiler() { emitReturn(); }

void expression() {}


bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;
    // initially both parser.current and parser.previous
    // are set to nothing. calling it for the first time
    // sets the current token to the first token in the
    // source file, and sets the previous token to well.. nothing
    advance();
    // since the current token is now set to the first token
    // in source, calling advance() now will set parser.previous
    // to the first token in source and we can go from there.

    // in clox, there is no buffer of tokens, so we just use
    // parser.current instead of peek() to lookahead
    consume(TOKEN_EOF, "Expected end of expression.");
    endCompiler();
    return !parser.hadError;
}