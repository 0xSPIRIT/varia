#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#include "util.c"

#include "tokenize.h"
#include "interpret.h"

#include "tokenize.c"
#include "interpret.c"

int main(int argc, char **argv) {
    if (argc == 1) {
        // Error("Sorry! You must call the interpreter with the file name of your source code!\n");
        // return 1;
        argv[1] = "test.v";
    }

    char *source_buffer = read_entire_file(argv[1]);

    struct Tokenizer tokenizer = tokenize(source_buffer);

    interpret(tokenizer);

    free(source_buffer);

    return 0;
}