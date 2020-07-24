CC = gcc
CFLAGS = 
BUILD_DIR = ../build/clox.exe

all:
	$(CC) $(CFLAGS) memory.c value.c table.c object.c chunk.c debug.c scanner.c compiler.c vm.c main.c -o $(BUILD_DIR).

exec:
	./bin/clox

# clean:
#     rm -rf ./bin/clox