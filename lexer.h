#ifndef LEXER_H
#define LEXER_H
#include <stdio.h>


typedef enum {
    DUP,
    IF,
    ELSE,
    END,
    EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
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
    int line;
    int addr_to; // address to jump to
    int addr_fro; //hack but it works
} Token;

typedef struct {
    Token* items;
     size_t count;
     size_t capacity;
} TokenList;

void init_lexer(char* source);
TokenList generate_tokens(void);
#endif
