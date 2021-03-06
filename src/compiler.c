#include "compiler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token previous;
  Token current;
  bool hadError;
  bool panicMode;
} Parser;

typedef struct {
  Token name;
  int depth;
  bool isCaptured;
} Local;

typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue;

typedef enum { TYPE_FUNCTION, TYPE_SCRIPT } FunctionType;

typedef struct {
  // the compiler outside this one.
  // is NULL for the global scoped one.
  struct Compiler* enclosing;
  // the current function it
  // compiles the bytecode into
  ObjFunction* function;
  FunctionType type;
  // symbol table of local variables
  Local locals[UINT8_MAX + 1];
  int localCount;
  // Upvalues captured in case this is a
  // closure.
  Upvalue upvalues[UINT8_MAX + 1];
  int upvalueCount;
  int scopeDepth;
} Compiler;

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;

// operator precedence

// the enum values are in numerically
// increasing order because of how
// enums work

typedef enum {
  PREC_NONE,
  PREC_ASSIGN,     // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // ==
  PREC_COMPARISON, // >= <= > <
  PREC_TERM,       // + -
  PREC_FACTOR,     // / * %
  PREC_UNARY,      // - !
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

// A parse rule tells us the
// function to call when encountering a token T
typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

// helper functions

static void errorAt(Token* token, const char* message) {
  if (parser.panicMode)
    return;
  parser.panicMode = true;

  fprintf(stderr, "[line %d] Error: ", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // do nothing
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, " %s\n", message);
  parser.hadError = true;
}

static void error(const char* message) { errorAt(&parser.current, message); }

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;
  // the while loop helps
  // skip the error tokens
  // that are produced by the lexer
  while (true) {
    parser.current = scanToken();
    // break if the current token is not error
    // if it is an error then report the error
    // and eat following error tokens if any
    if (parser.current.type != TOKEN_ERROR)
      break;
    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  errorAtCurrent(message);
}

static Chunk* currentChunk() { return &current->function->chunk; }

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitReturn() {
  emitByte(OP_NIL);
  emitByte(OP_RETURN);
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  // two place holder byte sized instructions
  // that will be later strung together
  // to make a short instruction. (16 bits)

  emitByte(0xff);
  emitByte(0xff);
  // returns index of first byte
  // of the jump address.
  return currentChunk()->count - 2;
}

static void patchJump(int offset) {
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX)
    error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

static uint8_t makeConstant(Value value) {
  // addConstant returns the index in the pool to which
  // the constant was added.
  int constantIndex = addConstant(currentChunk(), value);

  if (constantIndex > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constantIndex;
}

static void emitConstant(Value value) {
  // first add the op_constant opcode
  // then the index in the constant pool
  // as it's operand (returned by makeConstant)
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void initCompiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = (struct Compiler*)current;
  compiler->function = NULL;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->type = type;
  compiler->function = newFunction();
  current = compiler;

  if (type != TYPE_SCRIPT) {
    current->function->name =
        copyString(parser.previous.start, parser.previous.length);
  }

  Local* local = &current->locals[current->localCount++];
  local->depth = 0;
  local->name.start = "";
  local->name.length = 0;
  local->isCaptured = false;
}

static ObjFunction* endCompiler() {
  emitReturn();
  ObjFunction* func = current->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(),
                     func->name != NULL ? func->name->chars : "<script>");
  }
#endif

  current = (Compiler*)current->enclosing;
  return func;
}

static void beginScope() { current->scopeDepth++; }

static void endScope() {
  current->scopeDepth--;
  uint8_t popCount = 0;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

static bool check(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }

  return false;
}

// actual parsing functions and some forward declarations

static void binary(bool canAssign);
static void and (bool canAssign);
static void or (bool canAssign);
static void unary(bool canAssign);
static void literal(bool canAssign);
static void number(bool canAssign);
static void string(bool canAssign);
static void variable(bool canAssign);
static void call(bool canAssign);
static void namedVariable(Token name, bool canAssign);
static void grouping(bool canAssign);
static void expression();
static void declaration();
static void ifStatement();
static void whileStatement();
static void forStatement();
static void statement();
static void printStatement();
static void expressionStatement();
static void varDeclaration();
static void funDeclaration();
static void synchronize();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

ParseRule rules[] = {
    {grouping, call, PREC_CALL},     // TOKEN_LEFT_PAREN
    {NULL, NULL, PREC_NONE},         // TOKEN_RIGHT_PAREN
    {NULL, NULL, PREC_NONE},         // TOKEN_LEFT_BRACE
    {NULL, NULL, PREC_NONE},         // TOKEN_RIGHT_BRACE
    {NULL, NULL, PREC_NONE},         // TOKEN_COMMA
    {NULL, NULL, PREC_NONE},         // TOKEN_DOT
    {unary, binary, PREC_TERM},      // TOKEN_MINUS
    {NULL, binary, PREC_TERM},       // TOKEN_PLUS
    {NULL, NULL, PREC_NONE},         // TOKEN_SEMICOLON
    {NULL, binary, PREC_FACTOR},     // TOKEN_SLASH
    {NULL, binary, PREC_FACTOR},     // TOKEN_STAR
    {unary, NULL, PREC_UNARY},       // TOKEN_BANG
    {NULL, binary, PREC_COMPARISON}, // TOKEN_BANG_EQUAL
    {NULL, NULL, PREC_NONE},         // TOKEN_EQUAL
    {NULL, binary, PREC_EQUALITY},   // TOKEN_EQUAL_EQUAL
    {NULL, binary, PREC_COMPARISON}, // TOKEN_GREATER
    {NULL, binary, PREC_COMPARISON}, // TOKEN_GREATER_EQUAL
    {NULL, binary, PREC_COMPARISON}, // TOKEN_LESS
    {NULL, binary, PREC_COMPARISON}, // TOKEN_LESS_EQUAL
    {variable, NULL, PREC_NONE},     // TOKEN_IDENTIFIER
    {string, NULL, PREC_NONE},       // TOKEN_STRING
    {number, NULL, PREC_NONE},       // TOKEN_NUMBER
    {NULL, and, PREC_AND},           // TOKEN_AND
    {NULL, NULL, PREC_NONE},         // TOKEN_CLASS
    {NULL, NULL, PREC_NONE},         // TOKEN_ELSE
    {literal, NULL, PREC_NONE},      // TOKEN_FALSE
    {NULL, NULL, PREC_NONE},         // TOKEN_FOR
    {NULL, NULL, PREC_NONE},         // TOKEN_FUN
    {NULL, NULL, PREC_NONE},         // TOKEN_IF
    {literal, NULL, PREC_NONE},      // TOKEN_NIL
    {NULL, or, PREC_OR},             // TOKEN_OR
    {NULL, NULL, PREC_NONE},         // TOKEN_PRINT
    {NULL, NULL, PREC_NONE},         // TOKEN_RETURN
    {NULL, NULL, PREC_NONE},         // TOKEN_SUPER
    {NULL, NULL, PREC_NONE},         // TOKEN_THIS
    {literal, NULL, PREC_NONE},      // TOKEN_TRUE
    {NULL, NULL, PREC_NONE},         // TOKEN_VAR
    {NULL, NULL, PREC_NONE},         // TOKEN_WHILE
    {NULL, NULL, PREC_NONE},         // TOKEN_ERROR
    {NULL, NULL, PREC_NONE},         // TOKEN_EOF
};

static ParseRule* getRule(TokenType type) { return &rules[type]; }

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;

  if (prefixRule == NULL) {
    error("Expected expression");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGN;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length)
    return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
  int count = compiler->function->upvalueCount;

  // check if this upvalue already exists.
  for (int i = 0; i < count; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal)
      return i;
  }

  if (count >= UINT8_MAX) {
    error("Too many upvalues in a function.");
    return -1;
  }

  // if none found, then add this as a new upvalue to the last
  // slot, bump the upvalue count by 1 and return.

  compiler->upvalues[count].index = index;
  compiler->upvalues[count].isLocal = isLocal;

  return compiler->function->upvalueCount++;
}

static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];

    if (identifiersEqual(name, &(local->name))) {
      if (local->depth == -1) {
        error("Cannot read local variable in it's own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static int resolveUpvalue(Compiler* compiler, Token* name) {
  Compiler* enclosing = (Compiler*)compiler->enclosing;
  if (enclosing == NULL)
    return -1;

  int local = resolveLocal(enclosing, name);
  bool isLocal = true;

  if (local == -1) {
    // if upvalue isn't found, look for an upvalue in the outer scope
    // of the enclosing compiler
    local = resolveUpvalue(enclosing, name);
    // if no upvalue is found even in the chain of enclosing scopes
    // exit.
    if (local == -1)
      return -1;
    isLocal = false;
  }
  enclosing->locals[local].isCaptured = true;
  return addUpvalue(compiler, (uint8_t)local, isLocal);
}

static void addLocal(Token name) {
  if (current->localCount == UINT8_MAX + 1) {
    error("Too many local variables in function.\n");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}

static void declareVariable() {
  // globals are implicitly declared
  if (current->scopeDepth == 0)
    return;

  Token* name = &parser.previous;

  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth)
      break;

    if (identifiersEqual(name, &local->name)) {
      error("Variable with this name already declared in this scope.");
    }
  }

  addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
  declareVariable();
  if (current->scopeDepth > 0)
    return 0;
  return identifierConstant(&parser.previous);
}

static void markInitialized() {
  if (current->scopeDepth == 0)
    return;
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void and (bool canAssign) {
  // left operand has already been compiled
  // and is at the top of the VM's stack
  // at this time. so we jump to the
  // right hand operand if it's false.
  int lJump = emitJump(OP_JUMPZ);
  emitByte(OP_POP);          // pop left operand, since it's falsy
  parsePrecedence(PREC_AND); // op-codes for the right operand
  patchJump(lJump);
}

static void or (bool canAssign) {
  int elseJump = emitJump(OP_JUMPZ);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void string(bool canAssign) {
  emitConstant(OBJ_VAL(
      copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);

  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void literal(bool canAssign) {
  switch (parser.previous.type) {
  case TOKEN_TRUE:
    emitByte(OP_TRUE);
    break;
  case TOKEN_FALSE:
    emitByte(OP_FALSE);
    break;
  case TOKEN_NIL:
    emitByte(OP_NIL);
    break;
  case TOKEN_EQUAL_EQUAL:
    emitByte(OP_EQUAL);
    break;
  default:
    return; // Unreachable
  }
}

static void expression() { parsePrecedence(PREC_ASSIGN); }

static uint8_t parseArgs() {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      argCount++;
      if (argCount >= UINT8_MAX) {
        error("Cannot have more than 255 arguments.");
      }

    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expected ')' after argument list.");
  return argCount;
}

static void call(bool canAssign) {
  uint8_t argCount = parseArgs();
  emitBytes(OP_CALL, argCount);
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

static void declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }
  if (parser.panicMode)
    synchronize();
}

static void varDeclaration() {
  uint8_t global = parseVariable("Expected variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  defineVariable(global);
}

static void returnStatement() {
  if (current->type == TYPE_SCRIPT) {
    error("Cannot return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}

static void statement() {
  if (match(TOKEN_FUN)) {
    funDeclaration();
  } else if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
  } else {
    expressionStatement();
  }
}

static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  // Compile parameter list
  consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Cannot have more than 255 parameters.");
      }

      uint8_t paramConstant = parseVariable("Expect paramter name.");
      defineVariable(paramConstant);
    } while (check(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after function parameters.");

  consume(TOKEN_LEFT_BRACE, "Expected '{' before function body.");
  block();

  ObjFunction* function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void funDeclaration() {
  uint8_t global = parseVariable("Expected function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after value.");
  emitByte(OP_PRINT);
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "expected ';' after expression");
  emitByte(OP_POP);
}

static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'");

  // compile the condition.
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after if condition.");

  int thenJump = emitJump(OP_JUMPZ);

  // if the condition was true, pop it off the stack and run
  // the statement body.
  emitByte(OP_POP);
  statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP);

  if (match(TOKEN_ELSE))
    statement();
  patchJump(elseJump);
}

static void whileStatement() {
  int loopStart = currentChunk()->count;

  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

  int exitJump = emitJump(OP_JUMPZ);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

static void forStatement() {
  beginScope();
  consume(TOKEN_LEFT_PAREN, "Expected '(' after for.");

  if (match(TOKEN_SEMICOLON)) {
    // no initializer.
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  int loopStart = currentChunk()->count;
  int exitJmp = -1;

  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';'");
    exitJmp = emitJump(OP_JUMPZ);
    emitByte(OP_POP);
  }

  // increment.

  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJmp = emitJump(OP_JUMP);
    int incrStart = currentChunk()->count;
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after for-loop clause.");

    emitByte(OP_POP);

    emitLoop(loopStart);
    loopStart = incrStart;
    patchJump(bodyJmp);
  }

  statement();
  emitLoop(loopStart);
  endScope();

  if (exitJmp != -1) {
    patchJump(exitJmp);
    emitByte(OP_POP); // pop the condition off the stack.
  }
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;
    switch (parser.current.type) {
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;

    default:;
      // Do nothing.
    }

    advance();
  }
}

static void binary(bool canAssign) {
  TokenType operator= parser.previous.type;

  //  Compile the right hand operand
  ParseRule* rule = getRule(operator);
  //  + 1 because binary operators like +, -, %, *
  //  assosciate to the left , so 1 + 2 + 3
  //  should be parsed as (1 + 2) + 3 instead of  1 + (2 + 3)
  //  Each binary operator’s right-hand operand precedence is one level higher
  //  than its own.
  parsePrecedence((Precedence)(rule->precedence + 1));

  //  Emit the operator instruction
  switch (operator) {
  case TOKEN_BANG_EQUAL:
    emitBytes(OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    emitByte(OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emitByte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emitBytes(OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    emitByte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emitBytes(OP_GREATER, OP_NOT);
    break;
  case TOKEN_PLUS:
    emitByte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emitByte(OP_SUB);
    break;
  case TOKEN_STAR:
    emitByte(OP_MULT);
    break;
  case TOKEN_SLASH:
    emitByte(OP_DIV);
    break;
  default:
    return; // Unreachable.
  }
}

static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  // compile the operand
  parsePrecedence(PREC_UNARY);
  // emit the operator instruction.
  switch (operatorType) {
  case TOKEN_MINUS:
    emitByte(OP_NEGATE);
    break;
  case TOKEN_BANG:
    emitByte(OP_NOT);
    break;
  default:
    return; // Unreachable
  }
}

// assume the initial '(' is already consumed
static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

ObjFunction* compile(const char* source) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);
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
  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction* function = endCompiler();
  return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    markObject((Obj*)compiler->function);
    compiler = (Compiler*)compiler->enclosing;
  }
}