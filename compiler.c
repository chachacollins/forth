#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include "compiler.h"
#include "lexer.h"
#include "fasm_header.h"

#define write_file(fmt, ...) (fprintf(output_file, fmt, ##__VA_ARGS__))

//TODO: ERROR Handling
FILE* output_file;

void init_compiler(void)
{
    output_file = fopen("out.asm", "w");
    if(!output_file) 
    {
        perror("Error");
        exit(69);
    }
}

void asm_prelude(void)
{
    write_file(
        "format ELF64 executable\n"
        "segment readable writeable\n"
        "scratch rb 10\n"
        "segment readable executable\n"
        "print_number:\n"
        "\tmov qword [scratch], 0\n"
        "\tmov word [scratch+8], 0\n"
        "\tmov rbp, rsp\n"
        "\txor rcx, rcx\n"
        "\tmov rax, rdi\n"
        ".loop:\n"
        "\tmov rbx, 10\n"
        "\txor rdx, rdx\n"
        "\tdiv rbx\n"
        "\tadd rdx, 0x30\n"
        "\tpush rdx\n"
        "\tinc rcx\n"
        "\tcmp rax, 0\n"
        "\tjne .loop\n"
        "\txor rax, rax\n"
        ".iter: \n"
        "\tpop rdx\n"
        "\tmov byte[scratch+rax], dl\n"
        "\tinc rax\n"
        "\tdec rcx\n"
        "\tcmp rcx, 0\n"
        "\tjne .iter\n"
        "\tinc rax\n"
        "\tmov byte[scratch+rax], 10\n"
        "\tinc rax\n"
        "\tmov rsp, rbp\n"
        "\tmov rdx, rax\n"
        "\tmov rax, 1\n"
        "\tmov rdi, 1\n"
        "\tmov rsi, scratch\n"
        "\tsyscall\n"
        "\tret\n"
        "entry main\n"
        "main:\n"
        "\tmov rbp, rsp\n"
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

void exec_asm(void)
{
    pid_t child = fork();
    assert(child >= 0);
    if(child == 0) 
    {
        char* argv[] = {"./asm", "out.asm", "a.out", NULL};
        int result = execv(argv[0], argv);
        assert(result == 0);
    } else
    {
        pid_t ret = wait(NULL);
        assert(ret >= 0);
    }
}

void build_asm(void)
{
    FILE* assembler = fopen("asm", "wb");
    assert(assembler != NULL);
    fwrite(fasm, sizeof(fasm[0]), fasm_len, assembler);
    fclose(assembler);
    chmod("./asm", 0777);
    exec_asm();
    chmod("./a.out", 0777);
    remove("./asm");
}

void compile(char* source)
{
    init_compiler();
    init_lexer(source);
    asm_prelude();
    bool loop = true;
    while(loop)
    {
        Token tok = next_token();
        switch(tok.kind)
        {
            case NUM:
                write_file("\tpush %d\n", atoi(tok.start));
                break;
            case PLUS: 
                write_file(
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tadd rax, rbx\n"
                    "\tpush rax\n"
                );
                break;
            case MINUS:
                write_file(
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tsub rax, rbx\n"
                    "\tpush rax\n"
                );
                break;
            case MULT:
                write_file(
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tmul rbx\n"
                    "\tpush rax\n"
                );
                break;
            case DIV:
                write_file(
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tdiv rbx\n"
                    "\tpush rax\n"
                );
                break;
            case PRINT:
                write_file(
                    "\tpop rdi\n"
                    "\tcall print_number\n"
                );
                break;
            case EOFF:
                loop = false;
                break;
        }
    }
    asm_epilogue();
    build_asm();
}
