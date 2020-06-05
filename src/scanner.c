#include "scanner.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

// Unlike the Java implementation of Lox
// The start and current point directly to the
// respective characters in the source string

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd() { return *scanner.current == '\0'; }

Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

Token errorToken(const char* errorMessage) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = errorMessage;
    token.length = (int)strlen(errorMessage);
    token.line = scanner.line;

    return token;
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek() { return *scanner.current; }

static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;

    scanner.current++;
    return true;
}

static void skipWhiteSpace() {
    while (true) {
        switch (peek()) {
            case '\r':
            case ' ':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' && !isAtEnd()) advance();
                }
            default:
                return;
        }
    }
}

static Token string() {
    while (!(isAtEnd() || peek() == '"')) {
        char c = advance();
        if (c == '\n') scanner.line++;
    }

    if (isAtEnd()) return errorToken("Unterminated string.");

    // closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

static Token number() {
    while (!isAtEnd() && isdigit(peek())) advance();
    if (peek() == '.' && isdigit(peekNext())) {
        advance();
        while (isdigit(peek())) advance();
    }
    return makeToken(TOKEN_NUMBER);
}

Token scanToken() {
    skipWhiteSpace();
    scanner.start = scanner.current;

    char c = advance();

    switch (c) {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            return makeToken(TOKEN_DOT);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '+':
            return makeToken(TOKEN_PLUS);
        case '/':
            return makeToken(TOKEN_SLASH);
        case '*':
            return makeToken(TOKEN_STAR);
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return string();
        default:
            if (isdigit(c)) return number();
    }

    if (isAtEnd()) return makeToken(TOKEN_EOF);
    return errorToken("Unexpected character");
}

// for debugging.
char* tokenToString(TokenType tok) {
    switch (tok) {
        case TOKEN_LEFT_PAREN:
            return "LEFT_PAREN";

        case TOKEN_RIGHT_PAREN:
            return "RIGHT_PAREN";

        case TOKEN_LEFT_BRACE:
            return "LEFT_BRACE";

        case TOKEN_RIGHT_BRACE:
            return "RIGHT_BRACE";

        case TOKEN_COMMA:
            return "COMMA";

        case TOKEN_DOT:
            return "DOT";

        case TOKEN_MINUS:
            return "MINUS";

        case TOKEN_PLUS:
            return "PLUS";

        case TOKEN_SEMICOLON:
            return "SEMICOLON";

        case TOKEN_SLASH:
            return "SLASH";

        case TOKEN_STAR:
            return "STAR";

        case TOKEN_BANG:
            return "BANG";

        case TOKEN_BANG_EQUAL:
            return "BANG_EQUAL";

        case TOKEN_EQUAL:
            return "EQUAL";

        case TOKEN_EQUAL_EQUAL:
            return "EQUAL_EQUAL";

        case TOKEN_GREATER:
            return "GREATER";

        case TOKEN_GREATER_EQUAL:
            return "GREATER_EQUAL";

        case TOKEN_LESS:
            return "LESS";

        case TOKEN_LESS_EQUAL:
            return "LESS_EQUAL";

        case TOKEN_IDENTIFIER:
            return "IDENTIFIER";

        case TOKEN_STRING:
            return "STRING";

        case TOKEN_NUMBER:
            return "NUMBER";

        case TOKEN_AND:
            return "AND";

        case TOKEN_CLASS:
            return "CLASS";

        case TOKEN_ELSE:
            return "ELSE";

        case TOKEN_FALSE:
            return "FALSE";

        case TOKEN_FOR:
            return "FOR";

        case TOKEN_FUN:
            return "FUN";

        case TOKEN_IF:
            return "IF";

        case TOKEN_NIL:
            return "NIL";

        case TOKEN_OR:
            return "OR";

        case TOKEN_PRINT:
            return "PRINT";

        case TOKEN_RETURN:
            return "RETURN";

        case TOKEN_SUPER:
            return "SUPER";

        case TOKEN_THIS:
            return "THIS";

        case TOKEN_TRUE:
            return "TRUE";

        case TOKEN_VAR:
            return "VAR";

        case TOKEN_WHILE:
            return "WHILE";

        case TOKEN_ERROR:
            return "ERROR";

        case TOKEN_EOF:
            return "EOF";

        default:
            return "UNKNOWN";
    }
}