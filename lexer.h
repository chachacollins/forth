#ifndef LEXER_H
#define LEXER_H
#include <stdio.h>


typedef enum {
    DUP,
    IF,
    END,
    EQUAL,
    PRINT,
    PLUS,
    MINUS,
    DIV,
    MULT,
    NUM,
    DOT,
    ILLEGAL,
    EOFF
} TokenKind;

typedef struct {
    TokenKind kind;
    const char* start;
    int len;
    int addr;
} Token;

typedef struct {
    Token* items;
     size_t count;
     size_t capacity;
} TokenList;

void init_lexer(char* source);
TokenList generate_tokens(void);
#endif
