project(clox)
cmake_minimum_required(VERSION 3.13)

add_executable(clox src/memory.c src/value.c src/table.c src/object.c
    src/chunk.c src/debug.c src/scanner.c src/compiler.c src/vm.c src/main.c)