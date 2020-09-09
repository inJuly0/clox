CC = clang
CFLAGS = 
SRC_FILES = memory.c value.c table.c object.c chunk.c debug.c scanner.c compiler.c vm.c main.c
BUILD_DIR = ../build/clox.exe

all:
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(BUILD_DIR).


test:
	..\build\clox ../tests/1.lox

# clean:
#     rm -rf ./bin/clox