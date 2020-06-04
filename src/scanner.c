#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

// Unlike the Java implementation of Lox
// The start and current point directly to the 
// respective characters in the source string

typedef struct{
    const char* start;
    const char* current;
    int line;
}Scanner;

Scanner scanner;

void initScanner(const char* source){
    scanner.start = source;
    scanner.current = source;
    scanner.line  = 1;
}

static bool isAtEnd(){
    return *scanner.current == '\0';
}

Token makeToken(TokenType type){
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

Token errorToken(const char* errorMessage){
    Token token;
    token.type = TOKEN_ERROR;
    token.start = errorMessage;
    token.length = (int)strlen(errorMessage);
    token.line = scanner.line;

    return token;
}

static char advance(){
    scanner.current++;
    return scanner.current[-1];
}

Token scanToken(){
    scanner.start = scanner.current;

    char c = advance();

    switch(c){
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
    }

    if(isAtEnd()) return makeToken(TOKEN_EOF);
    return errorToken("Unexpected character");
}

