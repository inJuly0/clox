CC = clang
CFLAGS = 
SRC_FILES = memory.c value.c table.c object.c chunk.c debug.c scanner.c compiler.c vm.c main.c
BUILD_EXE = ../build/clox.exe
TEST_FILE = ../tests/1.lox

all:
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(BUILD_EXE).


test:
	..\build\clox $(TEST_FILE)

clean:
	rm -rf $(BUILD_EXE)