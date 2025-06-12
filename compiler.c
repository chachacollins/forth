#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include "compiler.h"
#include "lexer.h"
//TODO: REMOVE NOB DEPENDANCY
#include "nob.h"
#include "fasm_header.h"

#define SCRATCH_BUFFER "10"
#define ASSEMBLER "./fasm"
#define DEFAULT_OUTPUT "out.asm"
#define EXECUTABLE_PERMS  0755
#define MAX_INT_SIZE  10

#define write_asm_file(fmt, ...)                                                   \
      do {                                                                         \
        assert(asm_file != NULL);                                                  \
        fprintf(asm_file, fmt, ##__VA_ARGS__);                                     \
      } while (0)

#define open_asm_file()                                                            \
    do {                                                                           \
        asm_file = fopen("out.asm", "w");                                          \
        if (!asm_file)                                                             \
        {                                                                          \
          nob_log(NOB_ERROR, "Could not open output file %s\n", strerror(errno));  \
          return false;                                                            \
        }                                                                          \
      } while (0)

#define close_file(file)                                                           \
      do {                                                                         \
        fclose(file);                                                              \
        file = NULL;                                                               \
      } while (0)

//TODO: change this someday from being global as of now idgaf
FILE* asm_file;


void asm_prelude(void)
{
    write_asm_file(
        "format ELF64 executable\n"
        "segment readable writeable\n"
        "scratch rb "SCRATCH_BUFFER"\n"
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
    write_asm_file(
        "\tmov rax, 60\n"
        "\txor rdi, rdi\n"
        "\tsyscall\n"
    );
}

bool exec_asm(char* executable_file)
{
    pid_t child = fork();
    if(child < 0) 
    {
        nob_log(NOB_ERROR, "Fork failed: %s", strerror(errno));
        return false;
    } else if (child == 0) 
    {
        char* argv[] = {ASSEMBLER, DEFAULT_OUTPUT, executable_file, NULL};
        (void)execv(argv[0], argv);
        NOB_UNREACHABLE("Execv failed\n");
    } else
    {
        int status;
        pid_t ret = waitpid(child, &status, 0);
        if (ret == -1) {
            nob_log(NOB_ERROR, "Wait failed: %s", strerror(errno));
            return false;
        }
        if (WIFEXITED(status)) 
        {
            int exit_status = WEXITSTATUS(status);
            if(exit_status != 0) 
            {
                nob_log(NOB_INFO, "assembler failed, status=%d\n", WEXITSTATUS(status));
                return false;
            }
        }
    }
    return true;
}

bool build_asm(char* executable_file)
{
    FILE* assembler = fopen(ASSEMBLER, "wb");
    if(!assembler) 
    {
        nob_log(NOB_ERROR, "Could not create assembler: %s", strerror(errno));
        return false;
    }
    size_t written = fwrite(fasm, sizeof(fasm[0]), fasm_len, assembler);
    close_file(assembler);
    if(written != fasm_len)
    {
        nob_log(NOB_ERROR, "Failed to write assembler expected %u wrote %zu", 
                fasm_len, written
                );
        return false;
    }
    chmod(ASSEMBLER, EXECUTABLE_PERMS);
    if(!exec_asm(executable_file)) return false;
    chmod(executable_file, EXECUTABLE_PERMS);
    remove(ASSEMBLER);
    return true;
}

bool parse_int(Token tok, int *n)
{
    assert(tok.len > 0);
    if(tok.len > 10) 
    {
        nob_log(NOB_ERROR, "Please provide a smaller number(we only support 32 bit intergers for now)");
        return false;
    }
    char buffer[MAX_INT_SIZE + 2] = {0};
    memcpy(buffer, tok.start, tok.len);
    buffer[tok.len] = '\0';
    char* endptr;
    long result = strtol(buffer, &endptr, 10);

    if (*endptr != '\0') 
    {
        nob_log(NOB_ERROR, "Invalid number format: %.*s", tok.len, tok.start);
        return false;
    }

    if (result < INT_MIN || result > INT_MAX) 
    {
        nob_log(NOB_ERROR, "Number out of range: %.*s", tok.len, tok.start);
        return false;
    }
    *n = (int)result;
    return true;
}

bool generate_asm(char* source) 
{
    assert(source != NULL);
    init_lexer(source);
    open_asm_file();
    #define clean_up()                                                             \
        do {                                                                       \
          close_file(asm_file);                                                    \
          nob_da_free(token_list);                                                 \
        } while (0)

    #define handle_error()                                                         \
        do {                                                                       \
          clean_up();                                                              \
          return false;                                                            \
        } while (0)
    asm_prelude();
    TokenList token_list = {0};
    if(!generate_tokens(&token_list)) handle_error();
    for(size_t i = 0; i < token_list.count; ++i)
    {
        Token tok = token_list.items[i];
        switch(tok.kind)
        {
            case NUM:
            {
                int n;
                if(!parse_int(tok, &n)) 
                {
                    handle_error();
                }
                write_asm_file("\tpush %d\n", n);
                break;
            }
            case IF:
                write_asm_file(
                    "\tpop rax\n"
                    "\ttest rax, rax\n"
                    "\tjz label_%d\n",
                    tok.addr_to
                );
                break;
            case ELSE:
                write_asm_file(
                    "\tjmp label_%d\n"
                    "label_%d:\n",
                    tok.addr_to,
                    tok.addr_fro
                );
                break;
            case END:
                write_asm_file(
                    "label_%d:\n",
                    tok.addr_fro
                );
                break;
            case EQUAL:
                write_asm_file(
                    "\tmov rcx, 0\n"
                    "\tmov rdx, 1\n"
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tcmp rax, rbx\n"
                    "\tcmove rcx, rdx\n"
                    "\tpush rcx\n"
                );
                break;
            case LESS:
                write_asm_file(
                    "\tmov rcx, 0\n"
                    "\tmov rdx, 1\n"
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tcmp rax, rbx\n"
                    "\tcmovl rcx, rdx\n"
                    "\tpush rcx\n"
                );
                break;
            case LESS_EQUAL:
                write_asm_file(
                    "\tmov rcx, 0\n"
                    "\tmov rdx, 1\n"
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tcmp rax, rbx\n"
                    "\tcmovle rcx, rdx\n"
                    "\tpush rcx\n"
                );
                break;
            case GREATER:
                write_asm_file(
                    "\tmov rcx, 0\n"
                    "\tmov rdx, 1\n"
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tcmp rax, rbx\n"
                    "\tcmovg rcx, rdx\n"
                    "\tpush rcx\n"
                );
                break;
            case GREATER_EQUAL:
                write_asm_file(
                    "\tmov rcx, 0\n"
                    "\tmov rdx, 1\n"
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tcmp rax, rbx\n"
                    "\tcmovge rcx, rdx\n"
                    "\tpush rcx\n"
                );
                break;
            case PLUS: 
                write_asm_file(
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tadd rax, rbx\n"
                    "\tpush rax\n"
                );
                break;
            case MINUS:
                write_asm_file(
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tsub rax, rbx\n"
                    "\tpush rax\n"
                );
                break;
            case MULT:
                write_asm_file(
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tmul rbx\n"
                    "\tpush rax\n"
                );
                break;
            case DIV:
                write_asm_file(
                    "\tpop rbx\n"
                    "\tpop rax\n"
                    "\tdiv rbx\n"
                    "\tpush rax\n"
                );
                break;
            case DUP:
                write_asm_file(
                    "\tpop rax\n"
                    "\tpush rax\n"
                    "\tpush rax\n"
                );
                break;
            case PRINT:
                write_asm_file(
                    "\tpop rdi\n"
                    "\tcall print_number\n"
                );
                break;
            case DOT:
                write_asm_file(
                    "\tpop rdi\n"
                    "\tcall print_number\n"
                );
                break;
            case ILLEGAL:
                //TODO: make errors better
                nob_log(NOB_ERROR, "line:%d illegal token %.*s", tok.line, tok.len, tok.start);
                handle_error();
            case EOFF:
                break;
        }
    }
    asm_epilogue();
    clean_up();
    return true;
}

bool compile(char* source, char* executable_file)
{
    assert(source != NULL);
    if(!generate_asm(source)) return false;
    assert(asm_file == NULL);
    if(!build_asm(executable_file)) return false;
    return true;
}
