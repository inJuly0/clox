#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"


#define FRAMES_MAX 64
// initial stack size
// grows further dynamically as values are added
#define STACK_SIZE 256

typedef struct {
  ObjClosure *closure;
  uint8_t *ip;
  // the first slot that this function
  // can use;
  Value *slots;
} CallFrame;

/* stackTop points to where the next element is
 supposed to go. (points to an unused slot) */

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;

  ValueStack stack;
  // Interned strings in the VM.
  Table strings;
  Obj *objects;
  // global variables, ObjString* -> Value table
  Table globals;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_RUNTIME_ERROR,
  INTERPRET_COMPILE_ERROR
} InterpretResult;

VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *source);

#endif