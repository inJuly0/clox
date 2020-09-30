#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>

static void printTokens(const char *source) {
  initScanner(source);
  int line = -1;
  for (;;) {
    Token token = scanToken();
    if (token.line != line) {
      printf("%4d ", token.line);
      line = token.line;
    } else {
      printf("   | ");
    }
    printf("%s '%.*s'\n", tokenToString(token.type), token.length, token.start);

    if (token.type == TOKEN_EOF)
      break;
  }
}

static char *readFile(const char *filePath) {
  FILE *file;
  errno_t err = fopen_s(&file, filePath, "r");

  if (!file) {
    fprintf(stderr, "Could not open file \'%s\'\n", filePath);
    exit(EXIT_FAILURE);
  }

  fseek(file, 0, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);
  char *buffer = (char *)malloc(fileSize + 1);
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

int main(int argc, const char **argv) {
  char *source = readFile("../tests/1.lox");
  printTokens(source);
  free(source);
  return 0;
}
