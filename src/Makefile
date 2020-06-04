CC = gcc
BUILD_DIR = ../bin/clox.exe

all:
	$(CC) memory.c value.c chunk.c debug.c scanner.c compiler.c vm.c main.c -o $(BUILD_DIR).

exec:
	./bin/clox

# clean:
#     rm -rf ./bin/clox