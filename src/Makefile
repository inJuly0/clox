CC = clang
CFLAGS = 
SRC_FILES = memory.c value.c table.c object.c chunk.c debug.c scanner.c compiler.c vm.c 
MAIN = main.c
TEST_ENTRY = scanner_test.c
BUILD_EXE = ../build/clox.exe
TEST_FILE = ../tests/1.lox

all:
	$(CC) $(CFLAGS) $(SRC_FILES) $(MAIN) -o $(BUILD_EXE)


run:
	../build/clox $(TEST_FILE)

scantest:
	$(CC) $(CFLAGS) $(SRC_FILES) $(TEST_ENTRY) -o $(BUILD_EXE).


clean:
	rm -rf $(BUILD_EXE)