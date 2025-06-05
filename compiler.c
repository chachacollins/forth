#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "compiler.h"
#include "fasm_header.h"

#define write_file(fmt, ...) (fprintf(output_file, fmt, ##__VA_ARGS__))

//TODO: ERROR Handling
FILE* output_file;

void init_compiler(void)
{
    output_file = fopen("out.asm", "w");
}

void asm_prelude(void)
{
    write_file(
        "format ELF64 executable\n"
        "segment readable writeable\n"
        "\thello: db 'hello world', 10\n"
        "\thello_len = $ - hello\n"
        "segment readable executable\n"
        "entry main\n"
        "main:\n"
        "\tmov rax, 1\n"
        "\tmov rdi, 1\n"
        "\tmov rsi, hello\n"
        "\tmov rdx, hello_len\n"
        "\tsyscall\n"
    );
}

void asm_epilogue(void)
{
    write_file(
        "\tmov rax, 60\n"
        "\txor rdi, rdi\n"
        "\tsyscall\n"
    );
    fclose(output_file);
}

void build_asm(void)
{
    FILE* assembler = fopen("asm", "wb");
    fwrite(fasm, sizeof(fasm[0]), fasm_len, assembler);
    fclose(assembler);
    chmod("./asm", 0777);
    system("./asm out.asm a.out");
    chmod("./a.out", 0777);
    remove("./asm");
}

void compile(char* source)
{
    init_compiler();
    asm_prelude();
    asm_epilogue();
    build_asm();
}
