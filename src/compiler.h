#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "object.h"

ObjFunction *compile(const char *source);
void printTokens(const char *source);

#endif